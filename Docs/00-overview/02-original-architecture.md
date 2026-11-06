# 原版架构地图

来源：`srcdata/docs/architecture.md`、`srcdata/INDEX.md`、`srcdata/catalog/class_hierarchy.txt`、`srcdata/catalog/world_tables.json`、`srcdata/source/game/world/`。

## 两层可执行文件

1. **KTGL / Steam 壳** — `DK4HD_sc.exe` 等四语言 exe。D3D11、ktsl2 音频、g1t 贴图、SteamAPI。入口见 `srcdata/source/steam/Startup.cpp`（互斥体 `KOUKAI4PK_MUTEX`）。
2. **大航海4 本体** — 从 PC/PS 版 C++ 迁来。主对象 `CDk4Game`（体积 0x98，全局实例 `DAT_144296500`）继承 `CSceneManager`。顶层循环 `fn_1400079a0` → `fn_14000e8a4`（this = `CDk4Game`），按 `+0x88` 流程态分派：0 标题、1 游戏中、2 请求读档、3 Game Over（`+0x90` 子码）、4 剧本通关、5 退出；`+0x7c` 是剧本号。每帧的玩法推进在 `CScene` 子类里。见 `12-ui-save/01-scenes-flow.md`。

## 内容与逻辑分离的原版雏形

World0.dll / World1.dll 已是「两份静态世界表、同一套 GetFix* 接口」：

| 导出 | 计数 | 步长 | 头文件 |
| --- | --- | --- | --- |
| GetFixTownProc | 97 | 0x84 | TownID.h |
| GetFixPersonProc | 235 | 0x38 | PersonID.h |
| GetFixItemProc | 231 | 0x20 | ItemID.h |
| GetFixForceProc | 25 | 0x58 | ForceID.h |
| GetFixFleetProc | 69 | 0x18 | FleetID.h |

- `[确认]` DLL 由 `CDllManager` 加载，不是 `CWorldData` 自己：ctor → `fn_14000c702`（body `0x1400bc720`）`LoadLibraryA("SC/World%d.DLL")`，句柄存 `+0x8`。下标来自 `fn_14000ff60`（body `0x1402a89a0`）：`scenario_id >= 4` → 1，否则 0。来源：`srcdata/source/game/world/CDllManager.cpp`、`unassigned/fn_1400b93e0.cpp`、反汇编。
- `[确认]` `GetFix*` 越界返回 0，否则返回 DLL 静态数组元素的**非 const 指针**（`return &s_aTown[id]`）。来源：`srcdata/source/world/DllMain.cpp`。
- `[确认]` DLL **只有这五张表**。没有 `GetFixCultureProc` / `GetFixSchemeProc`（`strings World0.dll` 只有五个导出名）。村落、遗迹、文化、计谋、船型、交易品、航线等表在 exe 内或开局运行时生成，两个世界包共用。
- `[确认]` World0 与 World1 逐记录比 hex：town 78/97、person 153/235、item 231/231、force 25/25、fleet 57/69 条不同。来源：`srcdata/catalog/world_tables.json`。差异语义见 `10-narrative/04-world0-world1-diff.md`。

重构时把 DLL 换成内容包即可，不必保留 PE 导出。注意「世界包」在原版只覆盖五张表；exe 内的表在重构后是 `vanilla-core` 包，两个世界包都依赖它。

## 世界对象与原版 tick 顺序

`[确认]` `CWorld`（体积 `0x1a5e0`）多重继承 `CWorldData`、`CClimate` 和 14 个 `C*Element` 表管理器（Fleet / MobileShip / Item / Person / Town / Remains / Force / ShipType / Ship / Culture / Scheme / TradingItem / Village / Request）。全局单例 `DAT_1442a7b40`，各功能直接用 `单例 + 偏移` 读写（例如交易 `+0x184e8` 交易品表）。来源：`class_hierarchy.txt`、`CWorld.cpp` ctor、`unassigned/fn_1402755b0.cpp`。

`[确认]` 时间推进 `fn_140012049`（body `0x1402a7780`）：`CWorld+0x10 += 1`（1 拍 = 1/48 日），然后调世界 tick `fn_14000277f`（body `0x1402aad80`），再调 `fn_140011414`（body `0x1400ec820`，玩家舰队侧）。世界 tick 按**固定顺序**、按日历门调用各表：

| 序 | 门 | 调用 | 表 |
| --- | --- | --- | --- |
| 1 | 日初（`SCalendar+0x14 == 0`，`fn_14000906b`） | `0x1402ac1d0` | 气候（`CWorld::vfunc_04`） |
| 2 | 每拍 | `0x1400ff2c0` | `CFleetElement::vfunc_04` |
| 3 | 月末最后一拍（`+0x1c == 2`，`fn_14000f8b7`） | `0x140129170` | `CItemElement::vfunc_04` |
| 4 | 日初 | `0x1401cc4c0` | `CPersonElement::vfunc_04` |
| 5 | 日初 或 月末 | `0x1402668f0` | `CTownElement::vfunc_04` |
| 6 | 月初（`+0x1c == 0`，`fn_14000e9da`）或 半年（`fn_14001374b`） | `0x14010dc20` | `CForceElement::vfunc_04` |
| 7 | 日初 | `0x1402059e0` | `CSchemeElement::vfunc_04` |
| 8 | 月初 | `0x1401f0a20` | `CRequestElement::vfunc_04` |
| 9 | 年初（`+0x1c == 0 && month % 12 == 0`，`fn_140014dd0`） | `0x1402912c0` | `CTradingItemElement::vfunc_04` |
| 10 | 月初 | `0x140291650` | 黄金航路月奖金 `fn_1400108fc`：扫 105 交易品，累加后经 `fn_140002cb6` 入账，文案「从公会获得了报告黄金航路的奖金」 |

门函数的字段语义（`+0x0c` 月、`+0x14` 拍、`+0x1c` 月初/中/末三态）见 `01-world/07-calendar-and-coords.md`。「半年」门是「月初」的子集，第 6 行的 OR 实际等于「月初」。

第 5 行 `CTownElement::vfunc_04` 体内还有第二层门：月初先全体市价漂移，再逐港「投资转化（`0x1402679cf`）→ 补货（`0x1402679f8`）」交错执行；独占上缴在月末最后一拍（`0x140267a0a`）；出租费不在这里，在第 3 行 `CItemElement::vfunc_04`，同一拍里先于独占上缴。第 9 行年初（0 月月初第一拍）跑在当月补货之后。见 `04-economy/02`、`04`、`07`。

「过 n 天」（`fn_14000b122`）不是直接改时钟，而是逐拍跑 48·n 次完整调度。

结论：原版没有事件总线。顺序靠这一个函数写死；后跑的表能看到先跑的表本拍写的状态。重构的 tick 顺序合同见 `05-config-architecture.md`。

## 运行时对象族（玩法相关）

按 RTTI 基类，不是 UI 控件：

| 领域 | 关键类 | 基类链 |
| --- | --- | --- |
| 世界 | CWorld, CWorldData, CTown, CVillage, CRemains, CCulture, CClimate | CWorld → CWorldData → CSaveObject；CVillage → CObject（不经 CSaveObject）；CClimate 只作 CWorldData 基类出现 |
| 人物 | CPerson, CPlayerPerson, CPersonElement | CPerson → CSaveObject；`CCharacter` 是动画精灵（→ CAnimeSprite），不是人物 |
| 势力 | CForce, CFleet, CMobileShip | CSaveObject / CLocator |
| 经济 | CTrade, CTradingItemElement, CItem | CTrade 无基类，会话对象；`SpecialProduct` / `Price` 只是源码头文件名（`SpecialProduct.h` `Price.h`），不是 RTTI 类 |
| 船 | CShip, CShipBase, CShipObject, CShipType | CShip → CShipObject → CShipBase |
| 海 | COceanScene, CSeaAreaScene, COceanMapData, CRouteNavigator | 前两者 → CScene（CSeaAreaScene 经 CRouteMap）；后两者无基类 |
| 战斗 | CCombatScreen, CCombatCrew, CBattleGun, CBattleResult, CBattleShip | 混合 View / SaveObject |
| 港口设施 | CBar, CInn, CGuild, CPalace, CPort, CShipyard, CPlazaView | 前六个无基类；CPlazaView → CView |
| 事件 | CEvent, CEventData, CEventSeeker, CEventScriptBuffer | 脚本缓冲 |
| 存档 | CSaveObject → CObject | 几乎所有模拟状态 |

UI 控件（`CSprite` / `CView` / `CButton` 家族）不进入玩法配置。场景流单独见 `12-ui-save`。

## 场景切分（原版已有的解耦点）

`[确认]` RTTI 里继承 `CScene` 的只有三个：`COceanScene`、`CSeaAreaScene`、`CTownScene`。战斗屏、广场、设施是挂在场景上的 View / Sprite，不是独立 Scene；`CEvent` 是 `CSaveObject`。来源：`class_hierarchy.txt`。

- `COceanScene` / `CSeaAreaScene` — 大地图与海域
- `CTownScene` / `CPlazaView` — 入港与广场
- `CCombatScreen` / `CCombatCrewScreen` / `CCombatSingleScreen` — 海战与接舷
- `CEvent` — 剧情回放
- 设施 View：交易所、造船、酒馆、公馆、公会、教会、宫殿

重构时每个场景只订阅自己需要的系统接口，禁止场景直接读另一场景的成员。

## 资源文件（内容，不是逻辑）

| 文件 | 用途 |
| --- | --- |
| `World.dat` | 6250000 字节 = 2500×1250 **uint16** LE 贴图编号（`fn_14000ecdc` body `0x140183740`：`imul eax,[pt+4],0x9c4; movzx eax, word [buf+rax*2]`）；航行坐标 `/16` → chip 下标，X 在 2500 环绕。见 `01-world/01`、`06` |
| `SC/EV0..EV6.dk4` | 事件触发表 + execute 索引（头 `WE`，版本字如 `0x0106`） |
| `SC/Sc0..Sc6.dk4` | 事件指令流 + 内嵌对白（首 `uint32` = 触发条数，记录魔数 `DS`）。不是「场景图」 |
| `SC/EVA.DK4` | 事件动画包，七剧本共用（无 `WE` 头） |
| `mesfile.dk4` | 文案 2055 条（SC） |
| 其余 `*.dk4` 图形（`Chip` `ship` `CharaXX` …） | G1T 容器（头 `GT1G4600`） |
| `res/*.ktsl2*` | 音频 |

## 原版耦合病

这些是重构要拆掉的：

- `NUM_*` 写死在循环和断言里，加港口必须改 exe。另：`srcdata` 头文件里部分 `NUM_*` 是 RTC 立即数投票（`tools/fill_gaps.py`）猜出来的，不是原值：`NUM_SHIPTYPE=5`（实 39）、`NUM_BATTLEGUN=5`（实 21）、`NUM_SHIPMAST=100`（实 3）、`NUM_HOLD_SCAN=10`（实为舱室种类 10）
- 哨兵 = `NUM+1`（`0x62` 无港、`0x6A` 空货、`0xEC` 无人…），同一整数在不同枚举含义不同（`0x1A` 既是空遗迹又是空势力），见 `07-open-unknowns.md`「原版哨兵对照」
- 剧本差异散在系统代码里比较 `DAT_144296500+0x7c`（剧本 0 / 2 / 6 各有特例，见 `09-architecture-decisions.md` D5）
- 技能不存盘，用属性+等级现场算（`fn_140009214`，body `0x1401c9d50`：`(A+B) * (min(level,200)+100) / 150`，配对表 `0x1406864a0` 10×2），公式与人物系统缠在一起。配对表按 eSkill 0–9（eAbility 0 体力 1 敏捷 2 魅力 3 智力 4 精神 5 运气）：`(0,4) (2,4) (1,4) (1,3) (3,4) (0,1) (1,5) (2,3) (3,5) (4,5)`。注意 7 说服力 = 魅力 + 智力，不是手册常写的魅力 + 运气
- 交易槽、船舱、市价、特产品都假设「5 艘船 × 5 货格 × 商品不可堆叠」
- 技能值直接出现在买货折扣、战斗结算、道具价格里（`1000-a`、`500-a`、`100-a/50`），没有独立修正器。取值口 `fn_14000e1ce(person, eSkill)`：NPC 买货与道具价传 4（会计术），`fn_14000660e` 的 `500-a` 传 1（统率力）。第二参是 `eSkill` 的证据：`fn_1400099e9`（body `0x1401ca010`）断言 `0 <= k < 10`，且 `fn_1401cb7f0.cpp` 传入的 RTC 局部名就叫 `eSkill`。结算链见 `03-decoupling-principles.md` 原则 6
- 全局单例 `CWorld` 被各功能按偏移直接读写；顺序耦合写死在世界 tick（见上）
- 随机数是单一 CRT `rand()`，首次调用时用 `time()` 播种，不进存档（见 `05-config-architecture.md`）
- World DLL 表是 packed C struct，字段无名字，逻辑靠偏移

详见 `03-decoupling-principles.md`。
