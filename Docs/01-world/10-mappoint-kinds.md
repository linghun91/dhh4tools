# 地图点种类

`eMapPointKind` 是**港口地图**上 12 种可点击热点的枚举。world 只拥有种类目录（ID、名字、图标 key、与设施/遗迹的外键）。点下去打开酒馆还是离港，属于 town-life / ui。

这不是大地图实体分类。港口、村落、遗迹、航线、结点、黄金航路各有自己的 ID 空间（`eTown` / `eVillage` / `eRemains` / `eRoute` / `eJunction` / `gold_road_id`）。不要把 12 填成 town / village / remain / dock / goldroad / junction。

`01-map-and-sea-areas.md` 里「`aszMapPointName` 未抽出」以本文为准。

## 1. 职责边界

管：

- `eMapPointKind` 12 条定义（原版包实例计数）
- 显示名（四语言 exe 字面量）与稳定 key
- 图标资源 key（`mappoint.dk4` 的索引规则）
- 种类 → 打开哪一类玩法的**外键**（不实现菜单）
- `MAPPOINTKIND_BITS` 作为「某港开了哪些热点」的数据形状

不管：

- 港口坐标、首都、region（`02-towns.md`）
- 村落 / 世界遗迹发现（`03-villages-discovery.md`）；本表 kind 9 只是港内「遗址」热点
- 航线、结点（`05-routes-navigation.md`）
- 黄金航路（`06-ocean/03-gold-road.md`）
- 设施里具体干什么、酒价、住宿（`08-town-life/01-facilities.md`）
- `mappoint.dk4` / `TownGrp.DK4` / `IconSC.dk4` 解码（`config/ui/`）
- 入港场景流（`12-ui-save/01-scenes-flow.md`）

town-life 的设施表用 `map_point_kind_id` 做外键。禁止再手编一套 0–8 的 `facility_id` 冒充本枚举（`01-facilities.md` 那 9 行是重构编号，顺序与本文不一致）。

## 2. 原版事实

### 2.1 计数与头文件

- `[确认]` `enum eMapPointKind { NUM_MAPPOINTKIND = 12, INVALID_MAPPOINTKIND = -1 }`。来源：`srcdata/source/MapPointID.h`（原始路径 `f:\work\daikoukai4\svn\trunk\source\MapPointID.h`），`num_values.json`。
- `[确认]` World DLL 另有符号 `eMAPPOINTKIND` / `NUM_MAPPOINTKIND` / `INVALID_MAPPOINTKIND`，头文件未写死 12。来源：`MapPointKindID.h`，`asserts.txt`。
- `[确认]` 断言含 `MAPPOINTKIND_BITS`、`MAPPOINTKIND_ID`：
  - `0 <= eData && eData < NUM_MAPPOINTKIND`
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
- `[确认]` `CTownScene::vfunc_02` RTC 局部：`cShipyard, cRemains, cGuild, cInn, cPalace, cBar, cPort, cTrade, cPlazaView, bItemPosInformation`。来源：`functions.tsv` / `rtc.json`（该函数反编译失败，只信 RTC 名）。无独立 `cGovernor` / `cChurch` / `cGate`。
- `[确认]` 同函数后半比较立即数 `0xb`、`0xc`–`0x10`。来源：反汇编 `0x14026d160`。
- `[推断]` kind 6/7 共用 `CPalace`（只有一个 `cPalace` 局部，但名字是两个槽）。kind 10 走 `CChurchGameView`（类在，但不在 vfunc_02 RTC）。kind 11 没有对应设施类，更像离港。
- `[确认]` `CMpDock` 体积 `0x10`，`CMpRemains` 体积 `0x18`。来源：各自 cpp dtor。`CMpDock` 构造出现在带 RTC `cDock` + `asShipLoad` 的函数里（`fn_14016cc40.cpp`）。`[推断]` 它们是码头/遗址热点的附属对象，不是大地图村落或黄金航路节点。`01-facilities.md` 写「CMpDock 是海域停泊点」与类名、目录、kind 3 文案冲突，实现以本文为准。

### 2.4 图标包

- `[确认]` `Game/mappoint.dk4` 头 `GT1G4600`，`ntex = 0x94 = 148`。来源：文件偏移 `+0x10`。
- `[确认]` `fn_14000a0c4`：`this+0xa8 = kind`；若 `fn_14001055a()` 返回 `0x12` 则 `tex = kind + 12` 否则 `tex = kind`；写入 `this+0xac`；`fn_140006faa(this, "mappoint.dk4", tex, …)`。
- `[确认]` 绘制 `fn_140010b1d` / `fn_14000aaba` 三次加载 `mappoint.dk4`：一次下标 `stored+0x25`，一次 `stored`。`stored` `[推断]` 即 `+0xac`。
- `[未知]` 148 张里其余帧（动画、语言、按下态）。
- `[确认]` 港内背景是 `TownGrp.DK4`（62 张），不是本表。来源：`fn_140013ea8` 字符串。
- `[确认]` 额外 5 按钮用 `IconSC.dk4`，与 kind 图标分家。

### 2.5 可见性位

- `[确认]` `fn_140002806(obj, kind)`：`return ( *(DWORD*)(obj+0x14) & (1u << kind) ) > 0`。移位在 `fn_140265040`（`shl eax, cl`，即 `FLAG`）。位测试在 `fn_140260b50`。来源：反汇编，不是反编译（反编译把移位丢成了 `s0 = 1`）。
- `[确认]` `fn_14000b6db` 对 kind 0–11 调 `fn_140002806`，假则跳过该 `CMapPointView`。
- `[确认]` `fn_1402658e0` 按 FLAG 置位/清位。
- `[推断]` 这就是 `MAPPOINTKIND_BITS`：运行时按 DWORD 测 12 位。`sizeof` 未恢复；断言允许的 bit 总数是 `sizeof * 8`，可能大于 12。
- `[未知]` 哪一个 `CTown` 存档字段是这组位。`02-towns.md` 的 `+0x74/+0x75` 曾出现立即数 `0xC`（`fn_14000c509` 写 `+0x74=0xc`），**禁止**据此写成设施开关。

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
| 地图点种类 | `map_point_kind_id` / `eMapPointKind` | 12 | -1 | `plaza` … `gate` |
| 种类位域 | `MAPPOINTKIND_BITS` | `[未知]` sizeof；运行时 DWORD | — | 每港一组 bit |
| 港内热点实例 | `(town_id, map_point_kind_id)` | 每港最多 12 | 缺位 = 不显示 | — |
| 底栏按钮 | 点击号 12–16 | 5 | — | **不是**本枚举 |

哨兵：未选 / 非法用 `-1`。不要用 `0`（0 是广场），不要用 `13` 当「无」（ctor 的 `0xd` 只是占位）。

模组追加种类：`id >= 1000`。运行时按表长循环，不写死 `0xc`。

`facility_id`：原版没有 `eFacility`。重构以 `map_point_kind_id` 为共享词汇。若 town-life 需要比 12 更细的菜单项，另开子 key，不要改 0–11。

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

`mode_byte == 0x12` 的语义 `[未知]`（`fn_14001055a` 读某对象 `+0x25`）。缺省内容包按「不等于 0x12 → 用 kind 本身」对照原版常见路径。

可见性：

```
def visible(town_id, kind) -> bool:
    bits = Town.map_point_bits(town_id)   # 存档/内容；原版字段 [未知]
    if bits is None:
        return True                       # 原版包未还原前：表里有的都显示
    return (bits & flag(kind)) != 0
```

点击：

```
# ui 只发事件，world 不弹窗
on click id:
    if 0 <= id < kind_count:
        emit MapPointClicked { town_id, map_point_kind_id: id }
    elif kind_count <= id < kind_count + toolbar_slots:
        emit TownToolbarClicked { town_id, toolbar_index: id - kind_count }
```

王宫 / 总督府是否互斥、城门是否 `LeaveTown`：接口预留，原版谓词 `[未知]`，见第 9 节。

## 5. 耦合分析

原版：

- 12 个名字编译进四份 exe 的 `.rdata`，改一个字要重链。
- `CTownSceneView` 写死 12 + 5 指针槽、`0x1c0` / `0x260` 体积。
- `CTownScene::vfunc_02` 直接持有 `CTrade`/`CBar`/… 对象。
- 图标下标、`0x12`、`+0x25` 写在加载/绘制函数里。
- 可见性位与 `CTown` 其它字节打在同一对象上（字段未钉）。
- 遗址热点与世界遗迹、道具发现扫同一类函数（`03-villages-discovery.md`）。

重构切断：

```
world:   MapPointKind 目录 + 每港 bit（若有）
ui:      场景摆 12 个按钮，贴图 key 来自本表
town-life: 订阅 MapPointClicked，按 kind 发 OpenExchange / RestAtInn / …
ocean:   只收 LeaveTown（若 kind 11 绑定到它）
remains: kind 9 只传 remains 入口，发现位仍在 remains 表
```

禁止：ocean 读 `eMapPointKind` 画大地图船；economy 按 kind 改价；把 `gold_road_id` 存进本表。

`01-facilities.md` 的设施 0–8 与本文 0–11 **不对齐**（例如那边 `inn=3` 是旅馆，这里 3 是码头）。迁移时改外键，不要两套整数并存。

## 6. 解耦后的配置

`config/world/mappoint_kinds.yaml`：

```yaml
schema: dk4.world.mappoint_kinds
version: 1
pack: vanilla-world0
count_original: 12
invalid: -1
name_slot_bytes: 8
bits:
  storage: dword          # 运行时 FLAG 测的是 DWORD
  sizeof_original: unknown
  unused_bits: unknown    # 可能 12..31 闲置
texture:
  pack: mappoint.dk4
  count: 148
  alt_offset: 12          # fn_14000a0c4；当 mode==0x12
  draw_plus: 0x25         # fn_140010b1d / fn_14000aaba
  mode_alt_value: 0x12
toolbar:                  # 不是 kind；占在这里以免和 12 搞混
  count: 5
  click_lo: 12
  click_hi: 16
  object_size: 0x260
  class: CStandardGraphicalButton
  pack: IconSC.dk4        # 其它语言 IconJP/TC/KR.dk4
  index: 0x16
kinds:
  - id: 0
    key: plaza
    name: 广场
    name_key: ui.mappoint.plaza
    name_i18n:
      zh-CN: 广场
      ja-JP: 広場
      zh-TW: 廣場
      ko-KR: 광장
    original_class: CPlazaView
    opens: townlife.plaza
  - id: 1
    key: exchange
    name: 交易所
    name_key: ui.mappoint.exchange
    name_i18n: { zh-CN: 交易所, ja-JP: 交易所, zh-TW: 交易所, ko-KR: 교역소 }
    original_class: CTrade
    opens: economy.exchange
  - id: 2
    key: shipyard
    name: 造船厂
    name_key: ui.mappoint.shipyard
    name_i18n: { zh-CN: 造船厂, ja-JP: 造船所, zh-TW: 造船廠, ko-KR: 조선소 }
    original_class: CShipyard
    opens: ships.shipyard
  - id: 3
    key: dock
    name: 码头
    name_key: ui.mappoint.dock
    name_i18n: { zh-CN: 码头, ja-JP: 出港所, zh-TW: 碼頭, ko-KR: 항구 }
    original_class: CPort
    related_class: CMpDock    # [推断]
    opens: ships.supply
  - id: 4
    key: bar
    name: 酒馆
    name_key: ui.mappoint.bar
    name_i18n: { zh-CN: 酒馆, ja-JP: 酒場, zh-TW: 酒館, ko-KR: 술집 }
    original_class: CBar
    opens: townlife.bar
  - id: 5
    key: guild
    name: 行会
    name_key: ui.mappoint.guild
    name_i18n: { zh-CN: 行会, ja-JP: ギルド, zh-TW: 同業公會, ko-KR: 조합 }
    original_class: CGuild
    opens: townlife.guild
  - id: 6
    key: palace
    name: 王宫
    name_key: ui.mappoint.palace
    name_i18n: { zh-CN: 王宫, ja-JP: 王宮, zh-TW: 王宮, ko-KR: 왕궁 }
    original_class: CPalace
    opens: factions.palace
  - id: 7
    key: governor
    name: 总督府
    name_key: ui.mappoint.governor
    name_i18n: { zh-CN: 总督府, ja-JP: 総督府, zh-TW: 總督府, ko-KR: 총독부 }
    original_class: CPalace    # [推断] 无独立类
    opens: factions.palace
  - id: 8
    key: inn
    name: 旅馆
    name_key: ui.mappoint.inn
    name_i18n: { zh-CN: 旅馆, ja-JP: 宿屋, zh-TW: 旅館, ko-KR: 여관 }
    original_class: CInn
    opens: townlife.inn
  - id: 9
    key: remains
    name: 遗址
    name_key: ui.mappoint.remains
    name_i18n: { zh-CN: 遗址, ja-JP: 遺跡, zh-TW: 遺跡, ko-KR: 유적 }
    original_class: CRemains
    related_class: CMpRemains  # [推断]
    opens: world.remains_entrance
  - id: 10
    key: church
    name: 教会
    name_key: ui.mappoint.church
    name_i18n: { zh-CN: 教会, ja-JP: 教会, zh-TW: 教堂, ko-KR: 교회 }
    original_class: CChurchGameView
    opens: townlife.church
  - id: 11
    key: gate
    name: 城门
    name_key: ui.mappoint.gate
    name_i18n: { zh-CN: 城门, ja-JP: 城門, zh-TW: 城門, ko-KR: 성문 }
    original_class: null
    opens: unknown            # [推断] 离港；原版包可先绑 ocean.leave_town
```

`name` / `name_i18n` 仅对照 exe。运行时 UI 走 `name_key` → `config/ui/strings.*.yaml`。逻辑代码不得出现「广场」二字。

每港开哪些热点：原版字段未钉时**不要**在 `towns.yaml` 猜 bitmask。需要按港裁剪时另写：

```yaml
# config/town/town_map_points.yaml  仅 availability 需要时
schema: dk4.town.town_map_points
version: 1
mode: all                 # all = 原版包默认 12 槽都实例化，位域全 1
# mode: per_town
# - town_id: 0
#   kinds: [plaza, exchange, shipyard, dock, bar, guild, palace, inn, remains, church, gate]
```

`palace` 与 `governor` 是否按 `is_capital` 二选一：内容包用显式列表，禁止 `if capital: 王宫 else: 总督府` 写进引擎。

## 7. 独立旋钮

改这些值不应迫使贸易/战斗改代码：

- 种类条数（表长；引擎硬顶另写 `engine_limits`）
- 名字 / key / i18n
- `opens` 指向哪条命令（town-life 收事件）
- 图标包、下标公式（`alt_offset`、`draw_plus`、`mode_alt_value`）
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
  key, name_key, original_class, opens, texture
}
MapPointKind.by_key(key) -> kind_id
MapPointKind.count() -> int          # 表长
Town.map_points(town_id) -> [kind_id]  # 可见的；无位域则返回全表
Town.map_point_visible(town_id, kind_id) -> bool
```

命令：world 不执行「进入酒馆」。ui 在热点上按下后：

```
MapPointClicked { town_id, map_point_kind_id, fleet_id }
TownToolbarClicked { town_id, toolbar_index }   # 0..4，不是 kind
```

town-life / economy / ships / factions / ocean 按 `opens` 订阅。world 只保证 kind 合法。

存档：种类定义不存。若还原出每港 bit，作为 town 状态字段存；没有则每次按内容表计算。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| `sizeof(MAPPOINTKIND_BITS)` | 断言只有宏 | `[未知]`；运行时按 DWORD 测 |
| 位域在 `CTown` 哪一偏移 | 未见字段名 | `[未知]`；禁止把 `+0x74=0xc` 当开关 |
| `fn_14001055a` 返回 0x12 的对象/语义 | `+0x25` 字节 | `[未知]` |
| `mappoint.dk4` 148 张除 kind / kind+12 / +0x25 以外的布局 | G1T | `[未知]` |
| `CMapPointView+0xa8` ctor 写 `0xd` | 随后被 kind 覆盖 | 占位；不要当第 13 种 |
| 点击号 12–16 → 哪五个底栏功能 | `CStandardGraphicalButton` + `IconSC.dk4[0x16]` | `[未知]` |
| kind 6/7 是否每港只显示一个 | 一个 `cPalace` 局部 + 两句文案 | `[未知]`；配置显式写 |
| kind 11 是否离港 | 无设施类、无 mesfile「城门」玩法句 | `[推断]` |
| kind 10 如何从 `CTownScene` 进 `CChurchGameView` | 不在 vfunc_02 RTC | `[未知]` 路径 |
| `CMpDock` / `CMpRemains` 与 kind 3/9 的对象级绑定 | 仅类名 + 体积 | `[推断]` |
| `eTOWNACCESS` / `STownAccess` 是否管热点解锁 | dtor 写 `0xc9` | `[未知]`；不进本表 |
| `eTownKind` | 海域图标 2 档 | 不是本枚举 |
| 黄金航路 / 村落 / 航线结点 | 各有文档 | **已确认不是** 12 种之一 |
| `01-facilities.md` 设施 id 0–8 | 重构编号 | 与本枚举冲突；以 `map_point_kind_id` 为准 |
| World1 是否 12 个名字不同 | 名字在主 exe 不在 World DLL | `[推断]` 同 12 句；未逐字节对 World1 |

`CLocator` 是港口/村落/结点的基类，**不是** `CMapPointView` 的基类。热点是 UI 按钮，坐标跟港口 `x1/y1` 不是同一套数。港内按钮像素位置在 `fn_140014394` 一长串立即数里，属 ui，不进本 YAML。
