# 探索与遭遇

海上点「探索」、撞见 NPC 舰队、海豚 / 鲨鱼 / 老鼠 / 暴风雨。World 拥有村落和遗迹的坐标与发现位；Ocean 拥有这次按下探索键、以及把舰队遭遇交给 Combat。

## 1. 职责边界

本系统管：

- 探索命令（确认框「要探索吗？」）与搜索圈
- 探索结果的**掷骰与派发**（发现村、遗迹、水粮、温泉、绿洲、落空、水手逃走……）
- 把「发现了村庄 / 遗迹」写成 `DiscoveryFound`，不改 World 表结构
- NPC 活动舰队的接近检测与 `FleetSpotted`
- 迷你遭遇：海豚（航海日志）、鲨鱼（货损或新商品苗头）、老鼠（触发捉鼠小游戏）、暴风雨 / 暴风雪 / 坏血病
- 探索称号演出的**触发**（`CDiscoverTitleView` 是 UI）

不管：

- 村落、遗迹的坐标、名称、盲区（`config/world/villages.yaml` `remains.yaml`）
- 遗迹内部、霸者之证解谜（Items / Narrative）
- 捉鼠小游戏规则（`CRatGameView` 属 `town/minigames.yaml`）；Ocean 只发「开始捉鼠」
- 海战结算、接舷、俘虏（Combat）
- 市价、新交易品如何进交易所（Economy 收 `NewTradeGoodCandidate`）
- 航海日志存储格式（Save）；Ocean 只发「可记一条」
- `CItemMysteryHunting` 的地图道具解谜 UI（Items 拥有；Ocean 可在探索命中「藏宝图」时打开它）

## 2. 原版事实

### 探索 UI `[确认]`

- `CSearchCircleSolidSprite`（`srcdata/source/game/ocean/CSearchCircleSolidSprite.cpp`）：实心圈，ctor 以 0x40=64 个顶点走一圈，对象 0x638。绘制 `vfunc_07`。这是搜索半径的投影，不是规则。
- 贴图 `Search.DK4`（`Game/Search.DK4`）。加载索引至少有 1、3、5、6、7、8（`fn_1401ac810.cpp`、`fn_140188eb0.cpp`、`fn_1401b0e80.cpp`、`fn_140194480.cpp`）。
- 确认句：`mesfile_sc.txt` `0x1145B`–`0x11473`「您探索吗？」「要探索吗？」「是要探索吗？」
- `fn_140001005`（`fn_1401ac810.cpp`）加载 `Search.DK4` 第 5 帧，掷 `rand(0x64)`，与 `((x-5)>>1)` 比较后可能再调 `fn_14018ba70` 并播 mesfile 索引 `0x149`。这是一次探索检定，比较式的设计含义 `[未知]`。

### 村落 `[确认]`

- `NUM_VILLAGE=24`，`INVALID_VILLAGE=-1`（`VillageID.h`）。另有 `NUM_VILLAGEBLINDAREA`（未恢复）。
- 点击检测 `fn_1400044da`（`fn_140188eb0.cpp`，RTC `eVillage`、`posVillage`、`posPlayerFleet`、`vecVillage`）：循环 `eVillage < 0x18`，把世界坐标 `+0x10` 后投到显示空间。显示空间矩形 `0×0`–`0x1E0×0x100`（480×256）。与鼠标距离平方 `> 0x100`（半径 16 像素）则忽略。另有 `fn_140001e29`、`fn_14000e4e4` 同类 RTC。
- 海域图：`CSeaAreaVillageView` 继承 `CSeaAreaMapObject`。
- 文案：「提督！发现了村庄！」（`0xCA27`）。未发现遗迹：「没有被发现的遗迹。」（`0xB3F`）。

### 遗迹 `[确认]` 有类型，`[未知]` 计数

- `eRemains`、`INVALID_REMAINS=-1`、`REMAINSSTATE_BITS`（`Remains.h` / `RemainsID.h`）。`NUM_REMAINS` 整数未恢复。
- `CRemains` 是 `CSaveObject`，表属 World。Ocean 不持有遗迹对象。
- 探索队文案（`0x1024B` 起）：发现物品、发现水粮、被当地人袭击、什么也没发现且水手逃走、被野兽袭击、发现温泉（消疲劳）、发现绿洲、陷入沼泽、发现「%s的霸者之证%s」。
- `CItemMysteryHunting`（`srcdata/source/game/item/CItemMysteryHunting.cpp`，对象 0x320）是用地图 / 道具找隐藏物的 View，布局常数一堆，逻辑未还原。与海上探索圈不是同一条输入路径。

### 发现演出 `[确认]`

- `CDiscoverTitleView`（0x100）+ `CDiscoverTitleMiniMap`。`vfunc_04` 把小地图矩形按 `+0x40` 反复右移直到 `< 8`，再线性插值 `0x14=20` 帧。这是「发现了某某」的镜头，不是规则。

### NPC 舰队与战斗移交 `[确认]`

- `COceanScene::vfunc_02` 调用 `fn_14000cd8d`（RTC `sChangeBattle`、`sSceneDesc`、`sEventTrigger`、`eMobileFleet`）。
- 循环 `eMobileFleet` 从 1 到 `< 4`（`NUM_MOBILEFLEET=4`，0 槽 `[推断]` 玩家）。状态 `== 0x46` 则跳过。距离判定后写战斗切入结构。
- 海域图：`CSeaAreaFleetView`。`CForceSelectView` 有 3 个指针槽（ctor `s20 >= 3`），用于挑势力 / 舰队。
- 文案：「在%s发现%s的舰队！进入战斗状态！」（`0x5AFF` 起，多句）。主动进攻：「发现了%s的舰队！攻击吧！」（`0x133CF`）。海盗：「在%s发现了海盗%s。」
- Ocean **不**结算伤害。切入后是 Combat 的场景。

### 迷你遭遇 `[确认]` 文案与类，`[推断]` 掷骰表

`fn_1400f2d90`（RTC `eHappening`）：

- 某值 `>= 0x1E` 时用表 1，否则表 0 `[推断]` 两套权重（例如近岸 / 远洋，或月份）。
- `rand(0xC8=200)`，然后 `eHappening` 从 0 走到 `< 0x14=20`，与表项比较，命中即停。
- `num_values.json` 的 `NUM_HAPPENING=5` 来自 RTC 立即数 5 出现 6 次，**与 20 槽权重表不是同一计数**。5 可能是大类，20 是细项。大类切分 `[未知]`。

文案可独立确认的细项：

| 遭遇 | 文案位置 | 效果（文案能看到的） |
| --- | --- | --- |
| 暴风雨 | `0x3B8B` | 告警；虎鲸船首像「连可怕的暴风雨也可以躲过」 |
| 暴风雪 | `0x3C03` | 告警 |
| 坏血病 | `0x3C2B` | 需靠岸；莱姆可防 |
| 老鼠 | `0x3D6F`、`0x126C3` | 粮食危险；「现在开始捉老鼠游戏。请在３０秒之内捉住各船舱出现的老鼠。」 |
| 鲨鱼 | `0x4A0F`、`0x11473` | 小心；「鲨鱼吃了%s」；「得到了鲨鱼的幼鱼」；「也许会成为新的交易品」 |
| 海豚 | `0x11A4F` | 「可以将它们记录到航海日志中吗？」无战斗 |
| 无风 | 见航行文档气候 id `0x18` | 风力 0 |

捉鼠：`CRatGameView`（`srcdata/source/game/game/CRatGameView.cpp`）ctor 循环 `s20 >= 0x14`，20 只老鼠槽，每槽初始 `0x20`。属 UI / 小游戏。失败文案要求靠港灭鼠；成功「捉住了所有老鼠！那么就可以放心继续航海了。」猫道具可替代。

鲨鱼幼鱼：文案明确「也许会成为新的交易品」。Ocean 只发候选，Economy 决定是否出现新 `trading_item_id`。已有交易品「大型鲨鱼的鳍」（`0x1773`）。

### 航线与测量（与探索共用岗位）

无测量员不能开辟航线，见 `01-sailing.md`。瞭望手道具文案：「如果瞭望手有了它，就会很容易发现宝物和港口。」「可以发现的范围更大。」搜索半径修正走 `ModifierSet.spotting`，不读六维。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key | 拥有者 |
| --- | --- | --- | --- | --- |
| `eVillage` | 24 | -1 | `village_id` | World |
| `eRemains` | `[未知]` | -1 | `remains_id` | World |
| `REMAINSSTATE_BITS` | 位域 | | `remains_state` | World 存档 |
| `eSeaArea` | 25 | -1 | `sea_area_id` | World |
| `eMobileFleet` | 4 | | `mobile_fleet_slot` | Fleets |
| `eMapPointKind` | 12 | -1 | `map_point_kind` | World |
| happening 细项 | 循环 20 | | `encounter_id` | Ocean 表 |
| happening 大类 | RTC 5？ | | `encounter_kind` | `[未知]` 是否独立 |
| 霸者之证区域 | 文案 7 个 | | `proof_region` | Items / Narrative |

七个霸者之证区域（文案 `0x1E247` 起，**不是** 25 海域）：北海、地中海、非洲、印度洋、东南亚、东亚、新大陆。Ocean 发现遗迹时只带 `remains_id`；区域归属在 World / Items。

日志条目没有独立枚举。重构用 `log_entry_id`，内容包列出海豚等可记录事件。

## 4. 算法 / 公式

### 4.1 探索命令

```
on command Search { fleet_id }:
  if no_lookout_and_required: warn; still allowed or not → 配置 search.require_lookout
  show confirm (ui.ocean.search_confirm)
  roll = uniform(0, search_roll_max)          # 原版 100
  threshold = search_threshold(skill, items)  # 原版 ((x-5)>>1)，x [未知]
  if roll > threshold: emit SearchNothing; return
  outcome = weighted_pick(search_outcomes, sea_area_id, climate)
  apply outcome
```

`search_roll_max` 默认 100。阈值与瞭望修正进 `formulas/ocean.yaml`，原版闭合式 `[未知]`。

### 4.2 探索结果表

内容在 `config/ocean/search.yaml`，公式只放权重缩放。原版文案能列的 outcome key：

| key | 事件 | World / 他系统 |
| --- | --- | --- |
| `village` | `DiscoveryFound { kind: village, village_id }` | World 把该村 `discovered=true` |
| `remains` | `DiscoveryFound { kind: remains, remains_id }` | World 写 `REMAINSSTATE_BITS` |
| `water_food` | 给 `supply_kind` 加桶 | Fleets 存货 |
| `item` | `DiscoveryFound { kind: item, item_id }` | Items |
| `proof` | `DiscoveryFound { kind: proof, item_id, region }` | Items / Narrative |
| `hot_spring` | `FatigueChanged { delta: -all }` + 经验文案 | Characters |
| `oasis` | 同探索演出，具体补给 `[未知]` | |
| `swamp` | 负面，细项 `[未知]` | |
| `locals_attack` | 水手伤亡 | Fleets |
| `beasts` | 水手伤亡 | Fleets |
| `nothing_desertion` | 水手逃走 | Fleets |
| `mystery_map` | 打开 Items.mystery_hunt | Items |

权重原版 20 槽表的具体整数未抽出（`DAT_14054e1d0`）。重构把表放 YAML，禁止在代码里写 20。

### 4.3 村落命中（显示空间，仅 UI）

```
disp = world_to_view(village.pos + village_draw_offset)  # offset 原版 0x10
if disp outside view_rect (0,0)-(view_w, view_h): ignore
if dist2(disp, mouse) > click_radius^2: ignore          # 16^2 = 256
```

这是 HD 前 480×256 的点击盒。逻辑发现不走鼠标，走 4.1 的 outcome。运行时 view 尺寸可配，默认包填原版 480、256、半径 16，HD 缩放由 UI 做。

### 4.4 舰队遭遇

```
for slot in mobile_fleets where slot != player_slot:
  if fleet.state == skip_state: continue          # 原版 0x46
  if not in_range(player, fleet, spot_range): continue
  emit FleetSpotted { fleet_id: player, other_fleet_id, sea_area_id, aggressive }
  # Ocean 停手。Combat 订阅后切 CCombatScreen
```

`spot_range` `[未知]`。`aggressive` 对应文案「进入战斗状态」vs「向我们进攻了」。瞭望扩大 `spot_range`。

玩家主动攻击走命令 `Ocean.engage { other_fleet_id }`，同样只发事件。

### 4.5 迷你遭遇掷骰

```
table = happening_tables[table_index]    # 原版 2 张 × 20 槽
r = uniform(0, happening_roll_max)       # 原版 200
encounter_id = first i where r < cdf[i]  # 原版顺序扫描
emit corresponding event
```

`table_index`：原版 `value >= 30` 选 1 否则 0。`value` 是什么（离岸距离？天数？）`[未知]`，参数名 `happening_table_split`。

老鼠：发 `MinigameStart { key: rats }`。30 秒、20 只进 `town/minigames.yaml`。失败不改 Ocean 规则，只继续扣粮（由补给公式处理）。

海豚：发 `LogbookOffer { key: dolphins }`。拒绝则无状态。

鲨鱼：先 `SharkEncounter { fleet_id }`。若咬货：`CargoLost { ship_slot, item_or_supply }`。若抓到幼鱼：`NewTradeGoodCandidate { hint_key: shark_fry }`，**不**直接插入交易品表。

暴风雨：发 `StormHit`。船首像 / 戒指的免伤是 Items 修正器 `storm_protect`，Ocean 读修正器决定是否跳过耐久损失。损失公式 `[未知]`。

## 5. 耦合分析

原版：

- 探索结果直接改水手人数、补给、道具、遗迹位、疲劳
- `fn_14000cd8d` 在 Ocean tick 里组装战斗场景描述
- 捉鼠 View 从海上对话直接 new
- 霸者之证、温泉经验、新交易品苗头都在同一段 mesfile 链
- 村落点击用 480×256 与半径 16 写死
- happening 权重表在 `.rdata`，20 与 5 两套计数缠在一起

重构：

- World 是地点的唯一写者。Ocean 发 `DiscoveryFound`，World 置位
- Combat 订阅 `FleetSpotted`，Ocean 不 include 战斗头文件
- 小游戏、日志、交易品候选全部事件化
- `CItemMysteryHunting` 不进 Ocean 目录

## 6. 解耦后的配置

`config/ocean/search.yaml`：

```yaml
schema: dk4.ocean.search
version: 1
confirm_message: ui.ocean.search_confirm
circle_vertices: 64
asset: Search.DK4
asset_frames: [1, 3, 5, 6, 7, 8]
view_click:
  width: 480
  height: 256
  radius: 16
  world_to_view_offset: 16      # 原版 +0x10
require_lookout: false
outcomes:
  - key: village
    event: DiscoveryFound
    payload: { kind: village }
  - key: remains
    event: DiscoveryFound
    payload: { kind: remains }
  - key: water_food
    grants_supply: true
  - key: item
    event: DiscoveryFound
    payload: { kind: item }
  - key: proof
    event: DiscoveryFound
    payload: { kind: proof }
  - key: hot_spring
    clear_fatigue: true
  - key: oasis
  - key: swamp
  - key: locals_attack
    crew_delta: negative
  - key: beasts
    crew_delta: negative
  - key: nothing_desertion
    crew_delta: negative
  - key: mystery_map
    open: items.mystery_hunt
# 权重按海域覆盖；缺省用 default。整数未还原，默认包全 0 表示「未抽出」
weights_default: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
```

`config/ocean/encounters.yaml`：

```yaml
schema: dk4.ocean.encounters
version: 1
happening_roll_max: 200
happening_slots: 20
happening_table_split: 30
skip_mobile_state: 70           # 0x46
player_mobile_slot: 0
spot_range: null                # [未知]
encounters:
  - id: 0
    key: storm
    event: StormHit
    messages: ui.ocean.storm
  - id: 1
    key: blizzard
    event: StormHit
    climate: snow
  - id: 2
    key: scurvy
    event: ScurvyOutbreak
  - id: 3
    key: rats
    event: MinigameStart
    minigame: rats
  - id: 4
    key: shark
    event: SharkEncounter
  - id: 5
    key: dolphins
    event: LogbookOffer
    log_key: dolphins
# id 与原版 20 槽的对齐 [未知]；上表按文案列的是 kind，不是抽出的下标
fleet_spot:
  message: ui.ocean.fleet_spotted
  engage_message: ui.ocean.enter_battle
rats_minigame:                  # 只引用，规则在 town/minigames.yaml
  key: rats
  seconds: 30
  slots: 20
```

`config/formulas/ocean.yaml` 追加：

```yaml
search_roll_max: 100
search_threshold_bias: 5
search_threshold_shift: 1
spotting_radius_base: null
storm_durability_loss: null
```

地点实例不进这些文件。24 村、遗迹表在 `config/world/`。

## 7. 独立旋钮

- 探索权重、确认是否需要瞭望手、搜索圈顶点数
- happening 槽数、掷骰上限、两表分界
- 老鼠是否必须玩小游戏（可改成自动成功率）
- 海豚是否强制写日志
- 鲨鱼幼鱼是否允许成为交易品（Economy 开关，Ocean 只发候选）
- 舰队遭遇半径、跳过状态码
- 点击盒 480×256 / 半径 16（纯 UI）

加一种遭遇 = 在 `encounters.yaml` 加一行 + 文案 key，不改 tick。

## 8. 对外接口

事件：

```
DiscoveryFound { fleet_id, kind: village|remains|item|proof, village_id?, remains_id?, item_id? }
FleetSpotted   { fleet_id, other_fleet_id, sea_area_id, aggressive: bool }
SuppliesDepleted   # 见 01；探索水粮走 Fleets 直接加，不走这个
StormHit       { fleet_id, sea_area_id, climate_kind_id }
LogbookOffer   { fleet_id, log_key }
SharkEncounter { fleet_id }
CargoLost      { fleet_id, ship_slot, trading_item_id? , supply_kind? }
NewTradeGoodCandidate { hint_key }
MinigameStart  { key: rats, fleet_id }
ScurvyOutbreak { fleet_id }
SearchNothing  { fleet_id }
```

查询：

```
World.village(village_id) -> { x, y, discovered, sea_area_id }
World.remains(remains_id) -> { x, y, state_bits, sea_area_id }
Ocean.search_radius(fleet_id) -> int
Ocean.can_search(fleet_id) -> { ok, reason }
Fleets.mobile(slot) -> { fleet_id, state, x, y } | none
```

命令：

```
Ocean.search { fleet_id }
Ocean.engage { fleet_id, other_fleet_id }
Ocean.logbook_accept { fleet_id, log_key }
Ocean.logbook_reject { fleet_id, log_key }
```

World 命令（Ocean 不调用，由会话总线转）：

```
World.mark_discovered { village_id | remains_id }
```

Combat 命令由 Combat 自己在收到 `FleetSpotted` 后发。

## 9. 未核实字段

| 项 | 依据 | 状态 |
| --- | --- | --- |
| `NUM_REMAINS` | 头文件空 | `[未知]` |
| `REMAINSSTATE_BITS` 各位 | 仅类型名 | `[未知]` |
| `NUM_VILLAGEBLINDAREA` | 符号 | `[未知]` |
| 探索 `fn_140001005` 的 `x` 与阈值语义 | `((x-5)>>1)` vs `rand(100)` | `[未知]` |
| `Search.DK4` 各帧含义 | 索引 1,3,5,6,7,8 | `[未知]` |
| happening 20 槽权重整数 | `DAT_14054e1d0` 未 dump | `[未知]` |
| `NUM_HAPPENING=5` 与 20 槽关系 | RTC 立即数 | `[未知]` |
| 选 happening 表 0/1 的那个 `value` | `>= 0x1E` | `[未知]` |
| 遭遇 id 与文案行的对齐 | 只有文案 | `[未知]` |
| 舰队 `state == 0x46` 的枚举名 | `NUM_FLEETSTATE` 未恢复 | `[未知]` |
| `spot_range`、探索世界半径 | 点击半径 16 是 UI | `[未知]` |
| 温泉经验加多少 | 「经验上升了」无数字 | `[未知]` |
| 绿洲 / 沼泽的数值 | 仅文案 | `[未知]` |
| 水手逃走人数公式 | 仅「减少了%s人」 | `[未知]` |
| 鲨鱼咬哪一格货 | 「鲨鱼吃了%s」 | `[未知]` |
| 幼鱼对应哪个 `trading_item_id` | 未进 105 项表则是后解锁 | `[未知]` |
| `CItemMysteryHunting` 字段 | 大量布局立即数 | `[未知]` |
| `CDiscoverTitleView+0x40` 初始缩放 | 右移直到 `<8` | `[未知]` 演出参数 |
| 霸者之证 7 区与 `eSeaArea` 25 的映射 | 文案区域 ≠ 海域枚举 | `[未知]` |
| `eMapPointKind` 12 种名字 | 仅计数 | `[未知]` |
