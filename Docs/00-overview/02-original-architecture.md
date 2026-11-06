# 原版架构地图

来源：`srcdata/docs/architecture.md`、`srcdata/INDEX.md`、`srcdata/catalog/class_hierarchy.txt`、`srcdata/source/WorldData.h`。

## 两层可执行文件

1. **KTGL / Steam 壳** — `DK4HD_sc.exe` 等四语言 exe。D3D11、ktsl2 音频、g1t 贴图、SteamAPI。入口见 `srcdata/source/steam/Startup.cpp`（互斥体 `KOUKAI4PK_MUTEX`）。
2. **大航海4 本体** — 从 PC/PS 版 C++ 迁来。主对象 `CDk4Game`（体积 0x98）继承 `CSceneManager`，真正循环在 `CScene` 及其子类。

## 内容与逻辑分离的原版雏形

World0.dll / World1.dll 已是「两份静态世界表、同一套 GetFix* 接口」：

| 导出 | 计数 | 步长 | 头文件 |
| --- | --- | --- | --- |
| GetFixTownProc | 97 | 0x84 | TownID.h |
| GetFixPersonProc | 235 | 0x38 | PersonID.h |
| GetFixItemProc | 231 | 0x20 | ItemID.h |
| GetFixForceProc | 25 | 0x58 | ForceID.h |
| GetFixFleetProc | 69 | 0x18 | FleetID.h |

`CWorldData` 拥有 Town / Person / Item / Force / Fleet / Village / Culture / Scheme 表，并加载对应 DLL。重构时把 DLL 换成内容包即可，不必保留 PE 导出。

## 运行时对象族（玩法相关）

按 RTTI 基类，不是 UI 控件：

| 领域 | 关键类 | 基类链 |
| --- | --- | --- |
| 世界 | CWorldData, CTown, CVillage, CRemains, CCulture, CClimate | 多为 CSaveObject |
| 人物 | CPerson, CPlayerPerson, CCharacter | CPerson → CSaveObject |
| 势力 | CForce, CFleet, CMobileShip | CSaveObject / CLocator |
| 经济 | CTrade, CItem, SpecialProduct, Price | CTrade 是会话对象 |
| 船 | CShip, CShipBase, CShipObject, CShipType | CShip → CShipObject → CShipBase |
| 海 | COceanScene, CSeaAreaScene, COceanMapData, CRouteNavigator | CScene |
| 战斗 | CCombatScreen, CCombatCrew, CBattleGun, CBattleResult, CBattleShip | 混合 View / SaveObject |
| 港口设施 | CBar, CInn, CGuild, CPalace, CPort, CShipyard, CPlazaView | 设施对象 |
| 事件 | CEvent, CEventData, CEventSeeker, CEventScriptBuffer | 脚本缓冲 |
| 存档 | CSaveObject → CObject | 几乎所有模拟状态 |

UI 控件（`CSprite` / `CView` / `CButton` 家族）不进入玩法配置。场景流单独见 `12-ui-save`。

## 场景切分（原版已有的解耦点）

- `COceanScene` / `CSeaAreaScene` — 大地图与海域
- `CTownScene` / `CPlazaView` — 入港与广场
- `CCombatScreen` / `CCombatCrewScreen` / `CCombatSingleScreen` — 海战与接舷
- `CEvent` — 剧情回放
- 设施 View：交易所、造船、酒馆、公馆、公会、教会、宫殿

重构时每个场景只订阅自己需要的系统接口，禁止场景直接读另一场景的成员。

## 资源文件（内容，不是逻辑）

| 文件 | 用途 |
| --- | --- |
| `World.dat` | 2500×2500 uint8 大地图像素 |
| `EV0..EV6.dk4` `EVA.DK4` | 事件脚本（头 `WE`） |
| `Sc0..Sc6.dk4` | 场景图 |
| `mesfile.dk4` | 文案 2055 条（SC） |
| `*.dk4` 图形 | G1T 容器 |
| `res/*.ktsl2*` | 音频 |

## 原版耦合病

这些是重构要拆掉的：

- `NUM_*` 写死在循环和断言里，加港口必须改 exe
- 技能不存盘，用属性+等级现场算，公式与人物系统缠在一起
- 交易槽、船舱、市价、特产品都假设「5 艘船 × 5 货格 × 商品不可堆叠」
- 能力值直接出现在买货折扣、战斗结算、道具价格里（`1000-a`、`500-a`、`100-a/50`），没有独立修正器
- World DLL 表是 packed C struct，字段无名字，逻辑靠偏移

详见 `03-decoupling-principles.md`。
