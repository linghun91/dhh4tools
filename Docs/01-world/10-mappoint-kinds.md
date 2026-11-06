# 地图点种类

`eMapPointKind` 是**港口地图**上 12 种可点击热点的枚举。world 只拥有种类目录（key、名字、图标 key）。点下去由哪个设施处理器接手（「地图点 kind → 处理器 + 参数」，一个处理器可服务多个 kind）写在 town-life 的 `facilities.yaml`，属于 town-life / ui。

这不是大地图实体分类。港口、村落、遗迹、航线、结点、黄金航路各有自己的 ID 空间（`eTown` / `eVillage` / `eRemains` / `eRoute` / `eJunction` / `gold_road_id`）。不要把 12 填成 town / village / remain / dock / goldroad / junction。

`01-map-and-sea-areas.md` 里「`aszMapPointName` 未抽出」以本文为准。

## 1. 职责边界

管：

- `eMapPointKind` 12 条定义（原版包实例计数）
- 显示名（四语言 exe 字面量）与稳定 key
- 图标资源 key（`mappoint.dk4` 的索引规则）
- `MAPPOINTKIND_BITS` 作为「某港开了哪些热点」的数据形状

不管：

- 港口坐标、首都、文化圈（`02-towns.md`）
- kind → 设施处理器绑定（`08-town-life/01-facilities.md` 的 `map_point_kinds`）
- 村落 / 世界遗迹发现（`03-villages-discovery.md`）；本表 kind 9 只是港内「遗址」热点
- 航线、结点（`05-routes-navigation.md`）
- 黄金航路（`06-ocean/03-gold-road.md`）
- 设施里具体干什么、酒价、住宿（`08-town-life/01-facilities.md`）
- `mappoint.dk4` / `TownGrp.DK4` / `IconSC.dk4` 解码（`config/ui/`）
- 入港场景流（`12-ui-save/01-scenes-flow.md`）

town-life 的设施表用本表的 kind key 做外键。禁止再手编一套整数 `facility_id` 冒充本枚举。

## 2. 原版事实

### 2.1 计数与头文件

- `[确认]` `enum eMapPointKind { NUM_MAPPOINTKIND = 12, INVALID_MAPPOINTKIND = -1 }`。来源：`srcdata/source/MapPointID.h`（原始路径 `f:\work\daikoukai4\svn\trunk\source\MapPointID.h`），`num_values.json`。
- `[确认]` World DLL 另有符号 `eMAPPOINTKIND` / `NUM_MAPPOINTKIND` / `INVALID_MAPPOINTKIND`，头文件未写死 12。来源：`MapPointKindID.h`，`asserts.txt`。
- `[确认]` 断言含 `MAPPOINTKIND_BITS`、`MAPPOINTKIND_ID`：
  - `( 0 <= eData && eData < NUM_MAPPOINTKIND ) || eData == INVALID_MAPPOINTKIND`（及 `idData` 同形）
  - `0 <= eVal && eVal < NUM_MAPPOINTKIND`、`eVal <= NUM_MAPPOINTKIND`
  - `0 <= eData && eData < _GetBitTotal_BYTE2DWORD(sizeof(MAPPOINTKIND_BITS))`
  - `INVALID_MAPPOINTKIND <= _GetDataSizeMax_BYTE2DWORD(sizeof(MAPPOINTKIND_ID))`
  - 来源：`catalog/asserts.txt`，`AccessMacro.h`（`_GetBitTotal_BYTE2DWORD(sz) = sz * 8`，`FLAG(b) = 1u << b`）。
- `[确认]` 循环上界一律 `>= 0xc`。来源：`CTownSceneView.cpp` ctor、`fn_140004ade`、`fn_140014394`、`fn_14000b6db`。

### 2.2 12 个名字（`aszMapPointName`）

- `[确认]` 取名函数 `fn_140002d4c`（body `0x1401417a0`，RTC 局部 `aszMapPointName` 96 字节 = 12 个指针）。`e < 0 || e >= 12` 返回 NULL，否则返回对应字面量。来源：`srcdata/source/game/unassigned/fn_14013c6e0.cpp`；SC 反汇编：12 次 `lea rax, [rip+…]` 后 `mov rax, [rsp+rax*8+0x30]`。
- `[确认]` 字面量不是指针表，是 `.rdata` 里 12 个 8 字节槽（GBK/SJIS/Big5/CP949，`\0` 填充）。SC 槽起始 `0x14055d5a4`。
- `[确认]` 悬停/绘制会调用该函数再画 `mappoint.dk4`。来源：`fn_14000aaba` 调 `fn_140002d4c(*(this 相关偏移))`。

四语言同一顺序（getter：SC `0x1401417a0` / JP `0x1401417e0` / TC `0x140141ae0` / KR `0x140144b80`）：

| id | key | zh-CN | ja-JP | zh-TW | ko-KR |
| --- | --- | --- | --- | --- | --- |
| 0 | `plaza` | 广场 | 広場 | 廣場 | 광장 |
| 1 | `exchange` | 交易所 | 交易所 | 交易所 | 교역소 |
| 2 | `shipyard` | 造船厂 | 造船所 | 造船廠 | 조선소 |
| 3 | `dock` | 码头 | 出港所 | 碼頭 | 항구 |
| 4 | `bar` | 酒馆 | 酒場 | 酒館 | 술집 |
| 5 | `guild` | 行会 | ギルド | 同業公會 | 조합 |
| 6 | `palace` | 王宫 | 王宮 | 王宮 | 왕궁 |
| 7 | `governor` | 总督府 | 総督府 | 總督府 | 총독부 |
| 8 | `inn` | 旅馆 | 宿屋 | 旅館 | 여관 |
| 9 | `remains` | 遗址 | 遺跡 | 遺跡 | 유적 |
| 10 | `church` | 教会 | 教会 | 教堂 | 교회 |
| 11 | `gate` | 城门 | 城門 | 城門 | 성문 |

JP 的 kind 3 是「出港所」不是「波止場」：这是离港/补给点，不是世界图停泊锚。SC mesfile `0x7d47` 把「王宫」「总督府」写成两个去处，与 kind 6/7 两个槽一致。

### 2.3 视图对象

- `[确认]` `CTownSceneView` 体积 `0x108`。ctor：kind 0–11 把 `this+0x80+i*8` 清零；再循环 5 次把 `this+0xe0+i*8` 清零。来源：`CTownSceneView.cpp`。`0x80 + 12*8 = 0xe0`，`0xe0 + 5*8 = 0x108`。
- `[确认]` `CTownSceneView::vfunc_13`：`0 <= id < 0xC` 与 `0xC <= id < 0x11` 都进 `fn_140014d58`（写按钮标志，不是设施分发器）。点击号 0–11 = 本枚举；12–16 = 另外 5 个按钮。
- `[确认]` `fn_140014394` 按 kind 0–11：`fn_14000c0b3(0x1c0)` 分配、`CMapPointView` ctor（`0x14026fc80`）、存进 `+0x80`、`fn_14000a0c4(view, kind, 0x12, &arc)` 加载图标。随后循环 5 次分配 `0x260`，ctor `CStandardGraphicalButton`（`0x140247630`），存进 `+0xe0`，资源 **`IconSC.dk4`** 索引 `0x16`。来源：`fn_14026b040.cpp`。
- `[确认]` 那 5 个 **不是** `eMapPointKind`。它们是港内地图底栏按钮。映射 `[未知]`。
- `[确认]` `CMapPointView` 体积 `0x1c0`（dtor `fn_14000750e(..., 0x1c0)`）。基类链含 `CPushButton`。来源：`CMapPointView.cpp`，`class_hierarchy.txt`。
- `[确认]` `CMapPointView` ctor 写 `+0xa8 = 0xd`（13）、`+0xac = 0`。`fn_14000a0c4` 随即把 `+0xa8` 改成传入的 kind。`0xd` 是赋值前的占位，不是第 13 种热点。
- `[确认]` `CTownScene::vfunc_02`（body `0x14026d160`）RTC 局部：`cShipyard, cRemains, cGuild, cInn, cPalace, cBar, cPort, cTrade, cPlazaView, bItemPosInformation`。反编译失败，分发以下面的反汇编跳表为准。无独立 `cGovernor` / `cChurch` / `cGate`。
- `[确认]` 同函数后半比较立即数 `0xb`、`0xc`–`0x10`。来源：反汇编 `0x14026d160`。
- `[确认]` 点击分发：`CTownScene::vfunc_02` 在 `0x14026d501` 处 `if kind <= 0xb` 走跳表 `0x14026d92c`（12 项）。每项先构造栈上设施对象再调其运行函数：

| kind | 目标 | 构造 → 运行 |
| --- | --- | --- |
| 0 | `0x14026d52a` | `CPlazaView` → `fn_14000fd8a` |
| 1 | `0x14026d562` | `CTrade` → `fn_14000e0e3` |
| 2 | `0x14026d87a` | `CShipyard` → `fn_140010eb0` |
| 3 | `0x14026d610` | `CPort` → `fn_140004cb4` |
| 4 | `0x14026d71f` | `CBar` → `fn_1400035cb` |
| 5 | `0x14026d7f0` | `CGuild` → `fn_140009af7` |
| 6、7 | `0x14026d763`（同一入口） | `CPalace` → `fn_14000c5fe` |
| 8 | `0x14026d7a7` | `CInn` → `fn_14000b8e3` |
| 9、10、11 | `0x14026d834`（同一入口） | `CMpRemains` → `fn_14000f803(obj, scene, kind)`（kind 作第三参传入） |

- `[确认]` 故王宫/总督府同一处理；**教会、城门与遗址都进 `CMpRemains`**，不是 `CChurchGameView`、也不是离港。三者只在 World1 位域里出现（见 2.5）。`CMpRemains` 内部 `fn_14000f803` 按该港遗迹是否已发现分两路：首访 `fn_14000c4d2`（置发现位 + 按 kind 行加七大海域影响力，`09` §2.6）；重访 `fn_14000eacf`（body `0x140173d80`）按 kind 9/10/11 分到 `0x140008e9a` / `0x140002171` / `0x14000e840` 三支按日期的仪式（`08-town-life/05-inn-church-palace.md`）。`CChurchGameView` 只由事件脚本小游戏号 5 构造（`10-narrative/03`），与 kind 10 无关。
- `[确认]` `CMpDock` 体积 `0x10`，`CMpRemains` 体积 `0x18`。来源：各自 cpp dtor。`CMpDock` 构造出现在带 RTC `cDock` + `asShipLoad` 的函数里（`fn_14016cc40.cpp`）。kind 3 的分发目标是 `CPort`，`CMpDock` 只是其内部子对象 `[推断]`；`CMpRemains` 是 kind 9/10/11 的直接处理类 `[确认]`。都不是大地图村落或黄金航路节点。`01-facilities.md` 写「CMpDock 是海域停泊点」与类名、目录、kind 3 文案冲突，实现以本文为准。

### 2.4 图标包

- `[确认]` `Game/mappoint.dk4` 头 `GT1G4600`，`ntex = 0x94 = 148`。来源：文件偏移 `+0x10`。
- `[确认]` `fn_14000a0c4`：`this+0xa8 = kind`；若 `fn_14001055a()` 返回 `0x12` 则 `tex = kind + 12` 否则 `tex = kind`；写入 `this+0xac`；`fn_140006faa(this, "mappoint.dk4", tex, …)`。
- `[确认]` 绘制 `fn_140010b1d`（body `0x140272640`）/ `fn_14000aaba`（`0x140272940`）各三处加载 `mappoint.dk4`：参数非 0（按下）或 `fn_14000e55c` 为真（悬停）→ 下标 `stored+0x25`；否则 `stored`。`stored` 即 `+0xac`（代码以 `this-0x110` 为基址读 `[-0x64]`）。故 148 帧里 0–23 是常态、37–60 是高亮态。
- `[确认]` `mode_byte` 的来源：`fn_14001055a(town)`（body `0x140260370`）= `STown[town_id]` 记录 `+0x25` 字节，即 `culture_id`（`02-towns.md` 旧称 `region`，见 `02-factions/06-seven-culture-circles.md`）。`0x12 = 18`。World0/World1 中 culture 18（日本）恰为 长崎、大阪、那霸、江户。即日本港用 `kind+12` 那套图标。「日本风格」是 `[推断]` 的语义标签；机制 `[确认]`。
- `[未知]` 148 张里 24–36、61–147 帧（动画、语言等）。
- `[确认]` 港内背景是 `TownGrp.DK4`（62 张），不是本表。来源：`fn_140013ea8` 字符串。
- `[确认]` 额外 5 按钮用 `IconSC.dk4`，与 kind 图标分家。

### 2.5 可见性位

- `[确认]` `fn_140002806(obj, kind)`：`return ( *(DWORD*)(obj+0x14) & (1u << kind) ) > 0`。移位在 `fn_140265040`（`shl eax, cl`，即 `FLAG`）。位测试在 `fn_140260b50`。来源：反汇编，不是反编译（反编译把移位丢成了 `s0 = 1`）。
- `[确认]` `fn_14000b6db` 对 kind 0–11 调 `fn_140002806`，假则跳过该 `CMapPointView`。
- `[确认]` `fn_1402658e0` 按 FLAG 置位/清位。
- `[确认]` `obj` 是运行时 `CTown`：`fn_14000b6db` 先 `fn_140008f1c()` = 当前港 `eTown`（`0..0x60`）→ `fn_1400103bb(eTown)` = `world+0xc340`（`CTownElement`）`+8 + eTown*0x78`。故位域是 **`CTown+0x14`（u32）**。
- `[确认]` 初值：`CTown::vfunc_05`（body `0x1402ae3c0`）把 DLL 记录 `STown+0x46`（u16）写 `CTown+0x10`，`STown+0x48`（u32）写 `CTown+0x14`。`CTown::vfunc_07` 存档第二个字段就是 `+0x14`。伦敦 `0x17f` = 广场、交易所、造船厂、码头、酒馆、行会、王宫、旅馆；布里斯托尔 `0x19f` 把行会/王宫换成总督府。来源：`catalog/world_tables.json` `town.records[].hex`。
- `[确认]` 数据统计（97 港）：王宫(6) 与总督府(7) **从不同时置位**，4 港两者皆无；码头(3) 97 港全有；江户 `0x48` 只有码头+王宫。World0 从不置 9/10/11（遗址/教会/城门）；World1 置位 6/3/15 港（例：伦敦 `0x97f` 含城门，汉堡 `0x57f` 含教会，雅典 `0x37f` 含遗址）。
- `[确认]` 运行时改位：`fn_14000e142(town, kind, on)`（body `0x140265880`）→ `fn_1402658e0(&town+0x14, kind, on)`（或 / 与非）。两处调用：
  1. `fn_140008fbc`（body `0x140263110`）：发展度解锁。表 `0x1405a0490`（`.rdata`），4 行 × 6 项 × `{need_a, need_b, kind}`（12 字节）。行号 = `game(0x144296500)+0x7c`（`eScenario`），`>=4`（World1 剧本，`fn_14000864d`）时取 2。按序检查商业 `CTown+0x1e`、军事 `+0x20` 两个发展度（经 `fn_14000bc03(town,0/1)`）都 `>=` 门槛才置位，**首个不满足即停**。由 `fn_140009ae8`（RTC `nNewDevelopment,eTownDevelop`）在 `CTownElement::vfunc_04` 里调用。
  2. `fn_140001f87`（RTC `eTown`）：kind `0..0xb` 范围检查后置/清位，值来自 `fn_140004417`。调用者 `fn_140008a08`，用途 `[推断]` 事件指令。
- `[推断]` `sizeof(MAPPOINTKIND_BITS)=4`：初值按 u32 拷、按 DWORD 测；断言允许 32 位。
- `02-towns.md` 的 `+0x74/+0x75`（`fn_14000c509` 写 `+0x74=0xc`）**不是**这组位，禁止据此写成设施开关。

发展度解锁表原值（`need_a, need_b → kind`）：

| 行 | 1 | 2 | 3 | 4 | 5 | 6 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 1500,0→总督府 | 2500,500→广场 | 4000,1000→旅馆 | 4500,2000→酒馆 | 5000,5000→造船厂 | 9999,9999→行会 |
| 1 | 0,500→交易所 | 1000,1000→广场 | 2000,2000→旅馆 | 2500,3000→酒馆 | 4000,6000→造船厂 | 9999,9999→行会 |
| 2 | 0,500→交易所 | 1000,1000→广场 | 2000,1500→酒馆 | 3500,2000→旅馆 | 6000,4000→造船厂 | 9999,9999→行会 |
| 3 | 1000,0→总督府 | 2000,2000→广场 | 3000,3000→旅馆 | 4000,4000→酒馆 | 6000,6000→造船厂 | 9999,9999→行会 |

`+0x7c` = `eScenario` `[确认]`（`04-economy/07-investment-tribute.md` §2.1）。重构不在引擎里按剧本号选行：每个剧本在剧本包里声明自己用哪张解锁表（`map_point_unlock_table: <key>`），原版剧本 0–3 各用行 0–3，剧本 4–6 用行 2。`need_a/need_b` = 商业 / 军事发展度，状态归 economy。

`[确认]` 查表前有**据点港门**：`fn_140009ae8` 在 `0x14025f867` 先调 `fn_140009b42(town)`（→ `fn_14000d6c0(town, eScenario)`，本剧本据点港），为真才在 `0x14025f878` 调 `fn_140008fbc`。非据点港不走本表；剧本 4/5 无据点港，实际只有剧本 6 用行 2（`04-economy/07` §4.2/§4.4）。据点港表归剧本包 `base_towns`（`10-narrative/01`）。

### 2.6 与其它 12 / 5 的边界

| 东西 | 是不是 `eMapPointKind` |
| --- | --- |
| 港内 12 个热点 | 是 |
| `CTownSceneView` 额外 5 个 `CStandardGraphicalButton` | 否（点击号 12–16） |
| 广场 `CPlazaView` 5 槽 | 否（`01-facilities.md`） |
| 黄金航路 5 条 / 30 节点 | 否 |
| `eVillage` 24 | 否 |
| `eRemains` 世界遗迹 | 否；kind 9 只是港内入口 |
| `eJunction` / `eRoute` | 否 |
| `eTownKind`（海域图标首都/非首都，循环上界 2） | 否 |

黄金航路绘制和 `CTownSceneView` 落在同一份反编译单元 `fn_14026b040.cpp`，只说明还原时函数扎堆，不说明 kind 含黄金航路。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 非法 | 重构 key |
| --- | --- | --- | --- | --- |
| 地图点种类 | `map_point_kind_id` / `eMapPointKind` | 12 | `null`（原版 -1） | `plaza` … `gate` |
| 种类位域 | `MAPPOINTKIND_BITS` = `CTown+0x14` u32，初值 `STown+0x48` | 4 字节 `[推断]` | — | 每港一组 bit |
| 港内热点实例 | `(town_id, map_point_kind_id)` | 每港最多 12 | 缺位 = 不显示 | — |
| 底栏按钮 | 点击号 12–16 | 5 | — | **不是**本枚举 |

空值：运行时 `null`。不要用 `0`（0 是广场），不要用 `13` 当「无」（ctor 的 `0xd` 只是占位）。

模组追加种类用自己命名空间的 key（`mymod:lighthouse`），加载器顺序分配整数。位域按表长分配，不受原版 u32 的 32 位限制。运行时按表长循环，不写死 `0xc`。

`facility`：原版没有 `eFacility`。重构以 kind key 为共享词汇，town-life 的 `map_point_kinds` 把 kind 映射到处理器 + 参数（6/7 → palace，标题不同；9/10/11 → 原版同一 `CMpRemains`，town-life 拆成 ruins / church / gate 三个仪式变体）。

## 4. 算法 / 公式

无伤害、无价格。立刻数进配置。

```
NUM_MAPPOINTKIND          = 12
INVALID_MAPPOINTKIND      = -1
name_slot_bytes           = 8          # exe 字面量槽
view_slots                = 12         # CTownSceneView+0x80
toolbar_slots             = 5          # +0xe0，非 kind
map_point_object_size     = 0x1c0
toolbar_object_size       = 0x260
tex_alt_offset            = 12         # mode==0x12 时
tex_draw_plus             = 0x25       # 绘制另一态
mappoint_texture_count    = 148
iconsc_toolbar_index      = 0x16       # 额外 5 按钮
flag(kind)                = 1 << kind  # kind 0..31 才合法于 DWORD
```

取名：

```
def name(kind) -> str | None:
    if kind < 0 or kind >= len(kinds):   # 原版 12
        return None
    return kinds[kind].name_key          # 逻辑层禁止内嵌汉字
```

图标下标（实现读表，不写死 12/0x25）：

```
def texture_index(kind, mode_byte) -> int:
    base = kind + kinds.tex_alt_offset if mode_byte == 0x12 else kind
    return base
# 绘制态：index 或 index + tex_draw_plus
```

`mode_byte` = 港口 `culture_id`（`STown+0x25`），`0x12` = 日本文化圈 4 港。重构改为按港（或按文化圈）配 `icon_set`，不在引擎里比 18。

可见性：

```
def visible(town_id, kind) -> bool:
    bits = TownLife.map_point_bits(town_id)   # town-life 运行时状态（06）；初值来自内容表 STown+0x48
    return (bits & flag(kind)) != 0

# 发展度解锁（原版 fn_140008fbc；town-life 订阅 TownDeveloped 后跑）
def unlock_by_development(town_id):
    if town_id not in Scenario.base_towns():            # 据点港门：原版 fn_140009b42（0x14025f867），先过门再查表
        return
    table = Scenario.current().map_point_unlock_table   # 剧本包声明，不比较剧本号
    for step in unlock_tables[table]:     # 表长，不写死 6
        if dev_a(town_id) < step.need_a or dev_b(town_id) < step.need_b:
            break                         # 首个不满足即停
        TownLife.set_map_point(town_id, step.kind, True)
```

点击：

```
# ui 只发事件，world 不弹窗
on click id:
    if 0 <= id < kind_count:
        # ui 输入交 town-life；town-life 分派后发 FacilityEntered { town_id, map_point_kind: id }
    elif kind_count <= id < kind_count + toolbar_slots:
        emit TownToolbarClicked { town_id, toolbar_index: id - kind_count }
```

王宫 / 总督府由内容位域互斥（引擎无规则）；城门不是 `LeaveTown`，原版进 `CMpRemains`（见 2.3）。

## 5. 耦合分析

原版：

- 12 个名字编译进四份 exe 的 `.rdata`，改一个字要重链。
- `CTownSceneView` 写死 12 + 5 指针槽、`0x1c0` / `0x260` 体积。
- `CTownScene::vfunc_02` 直接持有 `CTrade`/`CBar`/… 对象。
- 图标下标、`0x12`、`+0x25` 写在加载/绘制函数里。
- 可见性位 `CTown+0x14` 初值嵌在 DLL `STown+0x48`；发展度解锁表 `0x1405a0490` 编进主 exe、按剧本号 `+0x7c` 选行。
- 图标套选择写死 `culture_id == 0x12`。
- 遗址热点与世界遗迹、道具发现扫同一类函数（`03-villages-discovery.md`）。

重构切断：

```
world:   MapPointKind 目录（每港位域归 town-life）
ui:      场景摆 12 个按钮，贴图 key 来自本表
town-life: 收 ui 点击，按 facilities.yaml 的 kind → 处理器表分派，发 FacilityEntered；订阅 TownDeveloped 解锁地图点
ocean:   只收 LeaveTown（来自 dock/底栏，不来自 kind 11）
world:   订阅 FacilityEntered（kind 9/10/11），on_enter_town_map_point 内首访置遗迹位、加影响力、发 RemainsDiscovered；重访发 RemainsRevisited（town-life 跑仪式）
```

禁止：ocean 读 `eMapPointKind` 画大地图船；economy 按 kind 改价；把 `gold_road_id` 存进本表。

`01-facilities.md` 的设施有自己的 key（如 kind `dock` → 设施 `port`），两者只经 `map_point_kinds` 映射；不要再出现两套整数并存。

## 6. 解耦后的配置

`config/world/map_point_kinds.yaml`（`01` / `02` 引用的同一文件）：

```yaml
schema: dk4.world.map_point_kinds
version: 2
pack: vanilla-core        # 名字与图标规则在主 exe，World0/World1 共用
bits:                     # 原版布局，仅 legacy 导入 STown+0x48 / 存档 CTown+0x14 用
  storage_original: u32   # [推断] sizeof(MAPPOINTKIND_BITS)=4；按 kind 顺序取位
texture:
  pack: mappoint.dk4
  count: 148
  icon_sets:              # 按港口文化圈选；原版只有 japan（culture 18）用 alt
    default: { base: 0 }
    japan:   { base: 12, cultures: [japan] }
  highlight_plus: 0x25    # fn_140010b1d / fn_14000aaba：按下 / 悬停
toolbar:                  # 不是 kind；占在这里以免和 12 搞混
  count: 5
  click_lo: 12
  click_hi: 16
  object_size: 0x260
  class: CStandardGraphicalButton
  pack: IconSC.dk4        # 其它语言 IconJP/TC/KR.dk4
  index: 0x16
kinds:
  - key: vanilla:plaza            # 原版 0
    name: 广场
    name_key: ui.mappoint.plaza
    name_i18n:
      zh-CN: 广场
      ja-JP: 広場
      zh-TW: 廣場
      ko-KR: 광장
    original_class: CPlazaView
  - key: vanilla:exchange            # 原版 1
    name: 交易所
    name_key: ui.mappoint.exchange
    name_i18n: { zh-CN: 交易所, ja-JP: 交易所, zh-TW: 交易所, ko-KR: 교역소 }
    original_class: CTrade
  - key: vanilla:shipyard            # 原版 2
    name: 造船厂
    name_key: ui.mappoint.shipyard
    name_i18n: { zh-CN: 造船厂, ja-JP: 造船所, zh-TW: 造船廠, ko-KR: 조선소 }
    original_class: CShipyard
  - key: vanilla:dock            # 原版 3
    name: 码头
    name_key: ui.mappoint.dock
    name_i18n: { zh-CN: 码头, ja-JP: 出港所, zh-TW: 碼頭, ko-KR: 항구 }
    original_class: CPort        # 跳表 [3]；CMpDock 是其内部子对象 [推断]
  - key: vanilla:bar            # 原版 4
    name: 酒馆
    name_key: ui.mappoint.bar
    name_i18n: { zh-CN: 酒馆, ja-JP: 酒場, zh-TW: 酒館, ko-KR: 술집 }
    original_class: CBar
  - key: vanilla:guild            # 原版 5
    name: 行会
    name_key: ui.mappoint.guild
    name_i18n: { zh-CN: 行会, ja-JP: ギルド, zh-TW: 同業公會, ko-KR: 조합 }
    original_class: CGuild
  - key: vanilla:palace            # 原版 6
    name: 王宫
    name_key: ui.mappoint.palace
    name_i18n: { zh-CN: 王宫, ja-JP: 王宮, zh-TW: 王宮, ko-KR: 왕궁 }
    original_class: CPalace
  - key: vanilla:governor            # 原版 7
    name: 总督府
    name_key: ui.mappoint.governor
    name_i18n: { zh-CN: 总督府, ja-JP: 総督府, zh-TW: 總督府, ko-KR: 총독부 }
    original_class: CPalace    # [推断] 无独立类
  - key: vanilla:inn            # 原版 8
    name: 旅馆
    name_key: ui.mappoint.inn
    name_i18n: { zh-CN: 旅馆, ja-JP: 宿屋, zh-TW: 旅館, ko-KR: 여관 }
    original_class: CInn
  - key: vanilla:remains            # 原版 9
    name: 遗址
    name_key: ui.mappoint.remains
    name_i18n: { zh-CN: 遗址, ja-JP: 遺跡, zh-TW: 遺跡, ko-KR: 유적 }
    original_class: CMpRemains   # 跳表 0x14026d92c[9]
  - key: vanilla:church            # 原版 10
    name: 教会
    name_key: ui.mappoint.church
    name_i18n: { zh-CN: 教会, ja-JP: 教会, zh-TW: 教堂, ko-KR: 교회 }
    original_class: CMpRemains   # 跳表 [10]，与 9 同入口；不是 CChurchGameView
  - key: vanilla:gate            # 原版 11
    name: 城门
    name_key: ui.mappoint.gate
    name_i18n: { zh-CN: 城门, ja-JP: 城門, zh-TW: 城門, ko-KR: 성문 }
    original_class: CMpRemains   # 跳表 [11]，与 9 同入口；不是离港
```

`name` / `name_i18n` 仅对照 exe。运行时 UI 走 `name_key` → `config/ui/strings.*.yaml`。逻辑代码不得出现「广场」二字。kind 由哪个设施处理器接（原 `opens`）只写在 town-life `facilities.yaml` 的 `map_point_kinds`，本表不重复。

每港开哪些热点：原版初值是 DLL `STown+0x48`。导出脚本抽成按 key 的列表，不存整数掩码（town-life 拥有）：

```yaml
# config/town/town_map_points.yaml
schema: dk4.town.town_map_points
version: 1
pack: vanilla-world0
initial:                  # 来自 STown+0x48；World1 包另一份
  - { town: vanilla:london,  kinds: [vanilla:plaza, vanilla:exchange, vanilla:shipyard, vanilla:dock, vanilla:bar, vanilla:guild, vanilla:palace, vanilla:inn] }   # 0x17f
  - { town: vanilla:bristol, kinds: [vanilla:plaza, vanilla:exchange, vanilla:shipyard, vanilla:dock, vanilla:bar, vanilla:governor, vanilla:inn] }         # 0x19f
  # ……其余 95 港由导出脚本生成
unlock_tables:            # fn_140008fbc；表 0x1405a0490。剧本包用 map_point_unlock_table 选一张；只对剧本包 base_towns 生效
  vanilla:map_point_unlock_0:   # 行 0；每项 {need: [商业, 军事], kind}（原版 12 字节 = i32 need_a, i32 need_b, i32 kind，exe 复核 [确认]）
    - {need: [1500, 0],    kind: vanilla:governor}
    - {need: [2500, 500],  kind: vanilla:plaza}
    - {need: [4000, 1000], kind: vanilla:inn}
    - {need: [4500, 2000], kind: vanilla:bar}
    - {need: [5000, 5000], kind: vanilla:shipyard}
    - {need: [9999, 9999], kind: vanilla:guild}
  # vanilla:map_point_unlock_1 … _3 见 2.5 表；原版剧本 4–6 引用 vanilla:map_point_unlock_2
stop_at_first_unmet: true
```

`palace` 与 `governor` 原版靠数据互斥，不是 `is_capital` 规则（王宫 35 港 ≠ 首都 21 港）。内容包用显式列表，禁止 `if capital: 王宫 else: 总督府` 写进引擎。

## 7. 独立旋钮

改这些值不应迫使贸易/战斗改代码：

- 种类条数（表长；引擎硬顶另写 `engine_limits`）
- 名字 / key / i18n
- 图标包、图标套（`icon_sets`）、高亮偏移
- 发展度解锁表（门槛、顺序、是否首个不满足即停；哪个剧本用哪张；据点港集合在剧本包 `base_towns`）
- 每港可见集合
- 底栏 5 按钮的资源（仍不是 kind）

改这些会碰到边界，但仍不应改本表 schema：

- 酒馆酒价、旅馆金币 → `formulas/town.yaml`
- 遗址发现位 → `remains.yaml`
- 离港航速 → ocean
- 广场 5 槽点击号 → `facilities.yaml` 的 `click_map`

## 8. 对外接口

查询（只读，ID + 值）：

```
MapPointKind.def(kind_id) -> {
  key, name_key, original_class, texture
}
MapPointKind.by_key(key) -> kind_id
MapPointKind.count() -> int          # 表长
# 每港可见集归 town-life（06），列此仅供对照：
TownLife.map_points(town_id) -> [kind_id]  # 可见的
TownLife.has_map_point(town_id, kind_key) -> bool
TownLife.set_map_point(town_id, kind_id, on)   # town-life 命令；发 MapPointUnlocked / MapPointLocked
```

命令：world 不执行「进入酒馆」。ui 在热点上按下后交 town-life，由它发：

```
FacilityEntered { town_id, map_point_kind }   # town-life 发（06）；world 订阅 kind 9/10/11（遗迹首访 / 重访，11 §4.3）
TownToolbarClicked { town_id, toolbar_index }   # 0..4，不是 kind
```

town-life 按自己的 `map_point_kinds` 分派（economy / ships / factions 的入口都经它）。world 只保证 kind 合法。

存档：种类定义不存。每港位域是 town-life 状态（原版 `CTown+0x14`，06），存档存 kind key 列表。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| `sizeof(MAPPOINTKIND_BITS)` | 按 u32 拷/测 | `[推断]` 4 |
| 位域偏移 | `CTown+0x14`，初值 `STown+0x48` | `[确认]`；`+0x74=0xc` 不是开关 |
| `fn_14001055a` 返回 0x12 | `STown+0x25` culture_id | `[确认]`；18 = 日本 4 港 |
| `game+0x7c`（解锁表选行） | `0x144296500` | `[确认]` `eScenario`；重构改剧本包声明 |
| `fn_140001f87` 改位的触发者 | `fn_140008a08` | `[推断]` 事件 |
| `mappoint.dk4` 148 张除 0–23 / 37–60 以外的布局 | G1T | `[未知]` |
| `CMapPointView+0xa8` ctor 写 `0xd` | 随后被 kind 覆盖 | 占位；不要当第 13 种 |
| 点击号 12–16 → 哪五个底栏功能 | `CStandardGraphicalButton` + `IconSC.dk4[0x16]` | `[未知]` |
| kind 6/7 是否每港只显示一个 | 97 港数据两位从不同时置位 | `[确认]`（数据）；引擎不加规则 |
| kind 9/10/11 在 `CMpRemains` 里的分支 | 首访按 kind 取影响力行；重访 `fn_14000eacf` 三支仪式 | `[确认]` |
| `CChurchGameView` 从哪进 | 事件脚本小游戏号 5 | `[确认]`（`10-narrative/03`、`08-town-life/03`）；不是 kind 10 |
| `CMpDock` 与 kind 3 | 跳表是 `CPort` | `[推断]` 子对象 |
| `eTOWNACCESS` / `STownAccess` 是否管热点解锁 | dtor 写 `0xc9` | `[未知]`；不进本表 |
| `eTownKind` | 海域图标 2 档 | 不是本枚举 |
| 黄金航路 / 村落 / 航线结点 | 各有文档 | **已确认不是** 12 种之一 |
| `01-facilities.md` 设施 key | 经 `map_point_kinds` 映射 | 以本表 kind key 为外键 |
| World1 是否 12 个名字不同 | 名字在主 exe 不在 World DLL | `[确认]` 同 12 句（getter 在 exe）；World1 只改位域初值 |

`CLocator` 是港口/村落/结点的基类，**不是** `CMapPointView` 的基类。热点是 UI 按钮，坐标跟港口 `x1/y1` 不是同一套数。港内按钮像素位置在 `fn_140014394` 一长串立即数里，属 ui，不进本 YAML。
