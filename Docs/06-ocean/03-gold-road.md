# 黄金航路

独立的发现 / 贸易路线系统，有自己的 View 和一份 10MB 级图集。不是 `eRoute` 那 200 条港到港导航航线。Ocean 记进度、发 `GoldRoadProgress`；奖励（称号、道具、名声）由 Progression / Items 领取。

## 1. 职责边界

本系统管：

- 黄金航路有几条、每条有哪些节点、节点权重 / 图形布局
- 进度（哪条路的哪个节点已点亮）
- 打开 / 关闭情报画面
- 进度变化时发事件

不管：

- 港到港导航 `eRoute`（`01-sailing.md` / `config/world/routes.yaml`）
- 市价、特产品、份额（Economy）。黄金航路可以**要求**某港已发现或某货已成交，但报价不在本系统
- 发现称号的镜头动画（`CDiscoverTitleView` 是 UI；本系统只说「该播哪条称号」）
- 霸者之证解谜（Items / Narrative）
- `GoldRoad.dk4` 怎么解码成贴图（资源管线）

原版类都在 `game/` 而不是 `ocean/`：`CGoldRoadInfomationView`、`CGoldRoadInfoSprite`。玩法上仍属 Ocean 域，配置落 `config/ocean/gold_road.yaml`。

## 2. 原版事实

### 资源 `[确认]`

| 文件 | 大小（约） | 用途 |
| --- | --- | --- |
| `Game/GoldRoad.dk4` | 10 810 996 | 主图集 |
| `Game/GoldRoadAnimeSC.dk4` | 23 359 828 | 简中动画 |
| `GoldRoadAnimeTC/JP/KR.dk4` | 同大 | 其它语言 |

加载点：

- `fn_1402267d0.cpp`：`GoldRoad.dk4` 索引 3、4；另一次索引 5
- `fn_14024c280.cpp`：索引 4
- `fn_14026b040.cpp`：索引 0、1、3，以及 `GoldRoadAnimeSC.dk4`

索引 0、1、3、4、5 被用到。2 是否存在 `[未知]`。

### View `[确认]`

`CGoldRoadInfomationView`（`srcdata/source/game/game/CGoldRoadInfomationView.cpp`）：

- 对象大小 0x7F8（dtor `fn_14000750e(..., 0x7f8)`）
- ctor 写 `*(this+0xB0) = 5`，随后 `for i in 0..<(this+0xB0)` 把 `this+0x1E0` 起的指针清零。5 个子对象槽 `[确认]`；它们是否就是「5 条航路」`[推断]`，没有别的证据。
- 同 ctor 写 `+0xB4 = 0x780`、`+0xB8 = 0x438` → 1920×1080 布局。后面一长串立即数是按钮 / 面板矩形（HD 坐标）。
- `vfunc_02` 再按 `+0xB0` 销毁那 5 个指针。

`CGoldRoadInfoSprite`（同目录 `CGoldRoadInfoSprite.cpp`，对象 0x3C8）：

- ctor 从 `+0x38` 起写 **30 对** `(frame, hold)`。**这是动画时间轴，不是节点权重** `[确认]`：`fn_1400036ac`（body `0x1402742f0`，字符串 `GoldRoadAnimeSC.dk4`）每 tick `+0x3C4 += 1`；当计数追上累计阈值 `+0x3C0` 时，`+0x3BC`（步号）`+= 1`、`+0x3C0 += table[step].hold`，并从 `GoldRoadAnimeSC.dk4` 取第 `table[step].frame` 帧；步号到 `0x1E`=30 时停、释放精灵。即 30 步动画，每步停留 `hold` 个 tick，总长 236 tick。

```
0:12  1:4   2:4   3:4   4:2   5:2
6:2   7:1   8:12  9:1  10:12 11:3
12:20 13:3  14:3  15:3  16:20 17:3
18:3  19:20 20:3  21:3  22:20 23:4
24:4  25:30 26:4  27:6  28:8  29:20
```

- `+0x128` 起是一批坐标（如 `0x4F0, 0x267, 0x2B2, 0x4E…`），1920×1080 布局 `[推断]` 动画 / 面板位置。
- `vfunc_03` 空。绘制在父 View。

`fn_14026b040.cpp` 一处循环 `s44 >= 5` 并对 `this+0xE0+i*8` 调绘制，再次确认 5。

### 与发现称号 `[推断]`

`CDiscoverTitleView` 的小地图缩放（见 `02-search-encounters.md`）是通用「发现了」镜头。黄金航路完成时是否走同一 View：无字符串直接写「黄金航路」，但资源名与独立 View 说明它不是普通遗迹。`[推断]` 完成一条路时播称号 + `GoldRoadAnime*.dk4`。

### 文案 `[确认]` 无「黄金航路」四字，`[未知]` 五条路的中文名

`mesfile_sc.txt` 检索「黄金航路 / GoldRoad / 黄金之路」无玩法条目。出现的「黄金」是道具与剧情（金沙、金冠、黄金面具、黄金储备）。因此：

- 系统内部名以资源文件 `GoldRoad.dk4` 与类名 `CGoldRoad*` 为准 `[确认]`
- 五条路的玩家可见名称 **未从 SC mesfile 抽出**，可能在图集纹理里或其它语言 mesfile `[未知]`
- 禁止把香料 / 丝绸 / 金银等常见二创名字写成 `[确认]`

### 不是导航航线 `[确认]`

`eRoute=200` 由 `CRouteNavigator` 使用，资源 `Route.dk4`。黄金航路不走该枚举。`CMarketPriceRouteMap` / `CSelectTownRouteMap` 是市价 / 选港投影，属 Economy / World。

## 3. 实体与 ID

| 实体 | 原版 | 重构 key | 备注 |
| --- | --- | --- | --- |
| 航路槽 | 5（View `+0xB0`） | `gold_road_id` 0–4 | 运行时读表长 |
| 节点 | `[未知]` | `gold_road_node_id` | 旧稿把 30 对动画帧当节点，已撤回。节点表与数量未还原 |
| 完成动画步 | 30 步 `(frame, hold)` | UI 资源 | `[确认]` 见 §2，属 UI，不进逻辑表 |
| 进度 | 未还原存档偏移 | `gold_road_progress` | 应用位域或每路一个 cursor |
| 地图点种类 | `eMapPointKind=12` | `map_point_kind` | 黄金航路绘制函数附近用到，12 名 `[未知]` |

空值：未开始的路用 `progress=0`。不要用 -1 当节点 id（0 是表里第一点）。

World0 / World1 若节点港不同，用同一 schema 两份内容包。目前 **没有** 从 DLL 抽出的黄金航路表，默认包 `nodes` 留 `[未知]`，直到还原（30 对是动画时间轴，不进节点表，见 §2）。

## 4. 算法 / 公式

原版「何时点亮下一节点」的函数未挂到类上，不能编造成交量公式。能钉死的只有结构：

```
roads: 5 [推断]    # View 子对象槽数
nodes: [未知]      # 没有还原出节点表；30 是动画步数，不是节点数
```

重构进度机（公式包可替换，默认「事件驱动、不自动按贸易额」）：

```
on GoldRoadTrigger { road_id, node_id }:
  if node already lit: return
  if node requires previous and previous not lit: return
  light node
  emit GoldRoadProgress { fleet_id, gold_road_id, node_id, lit_count, complete }
  if complete: emit GoldRoadComplete { gold_road_id }
```

触发源（全部是**订阅**，Ocean 黄金航路模块不轮询市价）：

| 触发 | 来源 | 原版是否如此 |
| --- | --- | --- |
| 发现指定遗迹 / 村 | `DiscoveryFound` | `[未知]` |
| 指定港入港 | `ArrivedAtTown` | `[未知]` |
| 指定货成交 | Economy `TradeCommitted` | `[未知]` |
| 剧本事件 | Narrative | `[未知]` |

在触发条件还原之前，默认包的 `triggers: []`、`nodes: []`，进度只由 Narrative 命令点亮。禁止用「参考大航海 4」写一条贸易额公式。

完成奖励：

```
on GoldRoadComplete:
  Progression.add_fame(reward.fame)      # 数值 [未知]
  Items.acquire(reward.item_id)          # 可选
  UI.play(GoldRoadAnime, language)
```

Ocean 自己不改名声数字，只带 `reward_key`。

### 月初奖金 `[确认]`（tick phase `gold_road_bonus`）

`engine/tick.yaml` 的 `{ id: gold_road_bonus, system: ocean.gold_road, schedule: [month_start] }` 归本系统。原版世界 tick 第 10 步 `fn_1400108fc`（body `0x140291650`）：

```
total = 0
for item in trading_items:                          # 原版 0x69 = 105 个元素，步长 0x40
    if fn_1400040ca(item) != 4: continue            # 某状态 == 4，名 [未知]
    if fn_140004eee(kind_of(item)): total += fn_140001384(item)   # 到货月谓词（04-economy/05）；奖金额
    if fn_140013692(item) == 1: fn_1400093d1(item)  # [未知]
if total != 0:
    message("从公会获得了报告黄金航路的奖金，金币%d枚。", total)
    Force.add_gold(player_force, total, reason: gold_road_bonus)  # fn_140002cb6，封 caps.gold_max
```

「航路」与交易品状态 4 的对应、奖金额式 `fn_140001384` 未拆 `[未知]`。

## 5. 耦合分析

原版：

- View 放在 `CDk4Game` 的 game 功能目录，和道具情报、发现称号挤在一起
- 图集索引、1920×1080 矩形、30 步动画时间轴全写在 ctor
- 5 写死在 `+0xB0` 和两处循环
- 无独立枚举头文件（没有 `GoldRoadID.h`）
- 可能间接依赖 `eMapPointKind` 画点

重构：

- 逻辑表与 UI 矩形分开：`gold_road.yaml` vs `ui/scenes.yaml`
- 5 是默认内容包长度；30 步动画进 UI 资源表
- 触发用事件，不读 `CTrade*`
- 动画文件名按语言包替换，不在逻辑里写 `GoldRoadAnimeSC.dk4`

## 6. 解耦后的配置

`config/ocean/gold_road.yaml`：

```yaml
schema: dk4.ocean.gold_road
version: 1
asset: GoldRoad.dk4
asset_frames: [0, 1, 3, 4, 5]
anime:
  zh-CN: GoldRoadAnimeSC.dk4
  zh-TW: GoldRoadAnimeTC.dk4
  ja-JP: GoldRoadAnimeJP.dk4
  ko-KR: GoldRoadAnimeKR.dk4
roads:                          # 5 条 [推断]；节点未还原。主键是命名空间 key（D1），原版槽位只作 vanilla_index（导入导出用）
  - { key: vanilla:gold_road_0, vanilla_index: 0, name_key: null, nodes: [] }
  - { key: vanilla:gold_road_1, vanilla_index: 1, name_key: null, nodes: [] }
  - { key: vanilla:gold_road_2, vanilla_index: 2, name_key: null, nodes: [] }
  - { key: vanilla:gold_road_3, vanilla_index: 3, name_key: null, nodes: [] }
  - { key: vanilla:gold_road_4, vanilla_index: 4, name_key: null, nodes: [] }
nodes: []                       # schema: { key, at: LocatorRef(kind, id), requires?: [node_key] }
rewards: []                     # [未知] 未还原
triggers: []                    # [未知] 未还原
```

完成动画时间轴（`[确认]`，属 UI，落 `config/ui/anim/gold_road.yaml`）：

```yaml
gold_road_anime:
  asset_by_lang_ref: ocean/gold_road.yaml#anime
  steps: [[0,12],[1,4],[2,4],[3,4],[4,2],[5,2],[6,2],[7,1],[8,12],[9,1],
          [10,12],[11,3],[12,20],[13,3],[14,3],[15,3],[16,20],[17,3],[18,3],[19,20],
          [20,3],[21,3],[22,20],[23,4],[24,4],[25,30],[26,4],[27,6],[28,8],[29,20]]
          # [frame, hold_ticks]，CGoldRoadInfoSprite +0x38
```


UI 矩形不进此文件。`CGoldRoadInfomationView` ctor 里 `0x780×0x438` 及后续 box 进 `config/ui/scenes.yaml` 的 `gold_road_info` 场景。

存档只存：

```yaml
gold_roads:                     # 按 key 存，不按下标（D7）
  vanilla:gold_road_0:
    lit_nodes: [n0, n1]         # 已点亮的 node key
  vanilla:gold_road_1:
    lit_nodes: []
```

不存权重原文。读档后用当前内容包的表重绘。

## 7. 独立旋钮

- 路数、每路节点（只改 YAML）
- 节点绑哪个 `town_id` / `remains_id`（还原后填，现为 null）
- 触发器种类（发现 / 入港 / 成交 / 剧本）
- 完成奖励（名声、道具、是否播动画）
- 图集帧与动画文件（语言包）

加第 6 条路：表长变 6，引擎按表长循环。不要在代码里写 `5`。

## 8. 对外接口

事件：

```
GoldRoadProgress { fleet_id, gold_road_id, node_id, lit_count, complete: bool }
GoldRoadComplete { fleet_id, gold_road_id, reward_key }
```

查询：

```
Ocean.gold_road(gold_road_id) -> { nodes: [{ id, lit, town_id?, remains_id? }] }
Ocean.gold_road_all() -> [同上]
Ocean.gold_road_complete(gold_road_id) -> bool
```

命令：

```
Ocean.gold_road_open_ui {}
Ocean.gold_road_light { gold_road_id, node_id }     # 供 Narrative / 会话总线
```

不暴露 ctor 里的 1920×1080 矩形，不暴露 `CGoldRoadInfoSprite*`。

与其它系统：

- 订阅 `DiscoveryFound`、`ArrivedAtTown`（触发器还原后）
- 完成时通知 Progression / Items，不自己改人物经验
- 月初奖金调 `Force.add_gold(force_id, delta, reason)`，不直接写金币
- UI 只投影 `gold_road_all()`

## 9. 未核实字段

| 项 | 依据 | 状态 |
| --- | --- | --- |
| 五条路的中文 / 日文名 | mesfile SC 无「黄金航路」 | `[未知]` |
| 节点表 / 节点数 | 30 对已证明是动画时间轴 | `[未知]` |
| `+0x128` 起坐标与节点的对应 | 布局立即数 | `[推断]` UI 节点 |
| `GoldRoad.dk4` 帧 2 是否使用 | 未见加载 | `[未知]` |
| 存档偏移 / 位宽 | `save_format.md` 未列 | `[未知]` |
| 点亮条件 | 无还原函数 | `[未知]` |
| 完成奖励（名声、道具、称号 id） | 无 | `[未知]` |
| 月初奖金 `fn_1400108fc` | 原始指令 `0x140291650`（扫 105 元素、状态 4、到货谓词、`!= 0` 才入账） | `[确认]` 流程；状态 4 名、奖金式 `fn_140001384`、`fn_1400093d1` `[未知]` |
| 是否与 `CDiscoverTitleView` 共用 | 仅类并存 | `[推断]` 完成时播称号 |
| `eMapPointKind` 12 名与航路点的关系 | 附近循环用该枚举 | `[未知]` |
| World1 是否另一套节点 | 无抽出表 | `[未知]` |
| View 里除 5 个指针外 `+0x1D8` 那张精灵 | 销毁路径有 | `[未知]` |
| ctor 中 `0x67B`、`0x74B` 等大量矩形 | HD 布局 | `[未知]` 逐个命名 |
| 类名 Infomation 的拼写 | 原版 RTTI 如此 | 保持，不改正 |
