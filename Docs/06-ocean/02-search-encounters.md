# 探索与遭遇

海上点「探索」、撞见 NPC 舰队、海豚 / 鲨鱼 / 老鼠 / 暴风雨。World 拥有村落和遗迹的坐标与发现位；Ocean 拥有这次按下探索键、以及把舰队遭遇交给 Combat。

## 1. 职责边界

本系统管：

- 探索命令（确认框「要探索吗？」）与搜索圈
- 探索结果的**掷骰与派发**（圈内道具、水粮、绿洲、温泉、野兽、当地人、水手逃走、金币……；村落 / 遗迹**不**由探索掷出）
- 圈内道具写成 `DiscoveryFound { kind: item }`，并作为叙事触发源 kind 0（`10-narrative/03-event-commands.md`）
- NPC 活动舰队的接近检测与 `FleetSpotted`
- 海上 happening 只引用 `05-sea-happenings.md`（结局表、掷骰、效果都在那里）
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
- `fn_140001005`（`fn_1401ac810.cpp`）是**温泉**结局处理器，不是总检定（见下「探索流程」）。旧稿的 `((x-5)>>1)` 是反编译把 `edx=5` 残留当成减数；原始指令是 `cdq; sub eax,edx; sar eax,1`，即 `x/2`。

### 探索流程 `fn_14001266b` `[确认]`（原始指令）

body `0x1401b4d20`，RTC `apfnSearchEvent`(12 指针)、`cptItem`、`eItem`、`posPlayerFleet`。三段：

1. **圈内道具**：`eItem` 0..`0xE6` 扫 Locator，`dist² <= r²`（`r` = `fn_14019dd60` 搜索半径，见 `13-algorithms/03-adjustment-factors.md` §2.2）。命中即发现道具并结束，同时问叙事 kind 0 触发（`fn_14001245e`，`10-narrative/03`）。道具种类 8（霸者之证）调 `fn_14018b880(200,200)`，其余 `fn_14018b880(10,5)`（语义 `[未知]`，像奖励经验 / 名声）。
2. **近岸门**：`fn_140012535(scene+0x88, pos)`（RTC `ptNextChip/ptWorldChip`，`[推断]` 附近有陆地）为假 → 失败。
3. `rand(4) == 0`（25%）→ **地区事件**；否则（75%）→ **补给检定**。

补给检定：

```
rich = fn_14000f218(lat, lon)            # 13 个经纬矩形，返回 0–4
ok   = rand(100) <= 10*rich + skill(admiral, 9) / 50     # idiv 0x32 @ 0x1401b5201
if ok:
    n     = fn_140005614(fleet, 9)       # 全队物资仓库（舱种 9）间数
    water = (rand(rich+2) + 1) * 5 * n
    food  = (rand(rich+1) + 1) * 5 * n
    Supplies.add(water, food)            # fn_1400044f8，按仓库容量分船（ships）
    crew_exp(10, 5)                      # fn_14018ba70 → 全员 fn_140011c34
else: fail
fail:  fatigue += 3 (msg 0x147)；crew_exp(15, 10)
```

`fn_14000f218` 富度表（body `0x1401b3d60`，同一经纬约定，首个命中）：

| rich | 矩形 |
| ---: | --- |
| 4 | lat −15..15 且（lon −90..30 或 lon 90..150） |
| 3 | lat −45..−15 & lon 105..150；lat 15..45 & lon −60..−30；lat −45..−30 & lon −105..−60 |
| 2 | lat 15..30 & lon 30..60；lat −15..0 & lon 60..90；lat −60..−45 & lon −30..30 |
| 0 | lat −30..−15 & lon −30..60；lat −45..−30 & lon −120..−90；lat 15..30 & lon 90..150；lat −90..−60 且（lon −180..0 或 lon 30..180） |
| 1 | 其余 |

近岸门通过且圈内无道具时：25% 走地区事件；75% 走补给检定，其成功率 `min(1, (10*rich + skill/50 + 1) / 100)`。

`admiral` = `fn_140004e17()`；`skill(p,k)` = `fn_14000e1ce`（`k` 是 eSkill，技能值 0–500）；`adj(p,k)` = `fn_14000e098`（六维经精神修正，0–100）。下标：技能 1 统率、5 剑术、9 观察；能力 2 魅力、5 运气（`03-characters/03`）。疲劳写入 `fn_140009467(fleet, d)` = `CFleet+0x71 = clamp(+0x71 + d, 0, 100)` → `+0x71` 即疲劳 `[确认]`（同一字节被情报面板画在 `posTired`）。`fn_14000b537(d)` = 改疲劳并播「上升 / 恢复」（`0x147` / `0x148`）。

地区事件：`region = fn_140009aca(lat, lon)`（0–5），`handler = table[region][rand(2)]`：

| region | 经纬矩形（`nLatitude` 南为正、`nLongitude` 东为正，闭区间，按源码顺序首个命中） | 槽 0 | 槽 1 |
| ---: | --- | --- | --- |
| 0 | lat 0..60 & lon 0..60；lat 0..30 & lon −105..−30；lat −30..15 & lon 90..150 | `oasis_or_attack` | `desertion_small` |
| 1 | lat −45..0 & lon −105..−30 | `oasis_or_attack` | `beasts` |
| 2 | lat −45..−30 & lon 120..150 | `oasis_or_attack` | `hot_spring` |
| 3 | lat −60..−30 & lon −150..−105 | `hot_spring` | `beasts` |
| 4 | lat −30..0 & lon −30..60；lat 15..45 & lon 90..135 | `rest_or_tire` | `crew_loss_5` |
| 5 | 其余 | `gold` | `locals_attack` |

处理器（帧号 = `Search.DK4` 第几帧；`crew_loss(n)` = 旗舰水手 `-min(n, crew-1)`，`fn_140007496`）：

| key | 函数 | 帧 | 规则 |
| --- | --- | ---: | --- |
| `oasis_or_attack` | `fn_14000b361` | 0 / 1 | `a = adj(admiral, 2)`。`a >= rand(101)`：疲劳 −10；再 `a/2 >= rand(100)` 则水、粮各 +20。否则：`crew_loss(rand(3)+10)`，经验 (15,10) |
| `desertion_small` | `fn_140007c02` | 7 | `crew_loss(rand(3)+3)`，经验 (15,10) |
| `beasts` | `fn_1400052a9` | 3 | SE `0x1D`；`crew_loss(21 - skill(admiral,5)/25 + rand(3))`；`adj(admiral,5)/2 >= rand(100)` 则经验 (15,10)，否则若 happening 态空闲（27）写成结局 23（传染病，05 篇名表）并经验 (15,15) |
| `hot_spring` | `fn_140001005` | 5 | 疲劳 −15；`adj(admiral,5)/2 >= rand(100)` 则经验 (10,10) + 消息 `0x149` |
| `rest_or_tire` | `fn_140011289` | 6 | `adj(admiral,5)/2 >= rand(100)` 疲劳 −5，否则 +5 并经验 (15,10) |
| `crew_loss_5` | `fn_140002694` | 4 | `crew_loss(5)`，经验 (15,10) |
| `gold` | `fn_14000613b` | 8 | 金币 `+= rand(adj(admiral,5))*10 + 100`（`fn_140002cb6`，封 99 999 999） |
| `locals_attack` | `fn_140002009` | 2 | `crew_loss(11 - skill(admiral,1)/50 + rand(3))`，经验 (15,10) |

key 名是本篇起的 `[推断]`，依据帧号与文案组（发现绿洲 / 被当地人袭击 / 被野兽袭击 / 温泉 / 沼泽 / 水手逃走）；六维 / 技能下标的中文名见 `03-characters`。经验 `(a,b)` 是 `fn_140011c34` 的两段经验增量 `[推断]`。

### 村落 `[确认]`

- `NUM_VILLAGE=24`，`INVALID_VILLAGE=-1`（`VillageID.h`）。村表在 exe `.data 0x140687b40`，每村两块盲区壳矩形，`NUM_VILLAGEBLINDAREA=2` `[推断，强]`（`01-world/03-villages-discovery.md`）。村落发现走半径自动（`fn_140001186`），不走探索。
- 点击检测 `fn_1400044da`（`fn_140188eb0.cpp`，RTC `eVillage`、`posVillage`、`posPlayerFleet`、`vecVillage`）：循环 `eVillage < 0x18`，把世界坐标 `+0x10` 后投到显示空间。显示空间矩形 `0×0`–`0x1E0×0x100`（480×256）。与鼠标距离平方 `> 0x100`（半径 16 像素）则忽略。另有 `fn_140001e29`、`fn_14000e4e4` 同类 RTC。
- 海域图：`CSeaAreaVillageView` 继承 `CSeaAreaMapObject`。
- 文案：「提督！发现了村庄！」（`0xCA27`）。未发现遗迹：「没有被发现的遗迹。」（`0xB3F`）。

### 遗迹 `[确认]`

- `eRemains`、`INVALID_REMAINS=-1`、`REMAINSSTATE_BITS`（`Remains.h` / `RemainsID.h`）。`NUM_REMAINS=25` `[确认]`（名字指针表 25 条 + NULL，`.rdata 0x1405b2f80`，`01-world/09`）。遗迹挂港，入口是港内地图点 kind 9（`CMpRemains`，`01-world/10-mappoint-kinds.md`），海上探索原版不发现遗迹。
- `CRemains` 是 `CSaveObject`，表属 World。Ocean 不持有遗迹对象。
- 探索队文案（`0x1024B` 起）：发现物品、发现水粮、被当地人袭击、什么也没发现且水手逃走、被野兽袭击、发现温泉（消疲劳）、发现绿洲、陷入沼泽、发现「%s的霸者之证%s」。
- `CItemMysteryHunting`（`srcdata/source/game/item/CItemMysteryHunting.cpp`，对象 0x320）是用地图 / 道具找隐藏物的 View，布局常数一堆，逻辑未还原。与海上探索圈不是同一条输入路径。

### 发现演出 `[确认]`

- `CDiscoverTitleView`（0x100）+ `CDiscoverTitleMiniMap`。`vfunc_04` 把小地图矩形按 `+0x40` 反复右移直到 `< 8`，再线性插值 `0x14=20` 帧。这是「发现了某某」的镜头，不是规则。

### NPC 舰队与战斗移交 `[确认]`

- `COceanScene::vfunc_02` 调用 `fn_14000cd8d`（RTC `sChangeBattle`、`sSceneDesc`、`sEventTrigger`、`eMobileFleet`）。
- 循环 `eMobileFleet` 从 1 到 `< 4`（`NUM_MOBILEFLEET=4`，0 槽 `[推断]` 玩家）。槽内 `fleet_id == 0x46`（70 = 空槽哨兵，见 06 篇 §2.2）则跳过。距离判定后写战斗切入结构。
- 海域图：`CSeaAreaFleetView`。`CForceSelectView` 有 3 个指针槽（ctor `s20 >= 3`），用于挑势力 / 舰队。
- 文案：「在%s发现%s的舰队！进入战斗状态！」（`0x5AFF` 起，多句）。主动进攻：「发现了%s的舰队！攻击吧！」（`0x133CF`）。海盗：「在%s发现了海盗%s。」
- Ocean **不**结算伤害。切入后是 Combat 的场景。

### 迷你遭遇 `[确认]` 文案与类，`[推断]` 掷骰表

日步掷骰、两张 20 槽累积表、槽 → 结局映射、结局名表（`.data 0x140686550`，26 项）全部在 [05-sea-happenings.md](05-sea-happenings.md) §2 `[确认]`。本篇不再重复，只保留与探索 / 舰队遭遇的接口。

- `NUM_HAPPENING=5` 与 20 槽不是同一计数（05 篇已证）。
- 每日门闩 `rand(30)==0`；只在手动航行时掷表，自动航行 / 海战中只有 1/3 机会强制槽 2（05 §2.3）。

文案可独立确认的细项：

| 遭遇 | 文案位置 | 效果（文案能看到的） |
| --- | --- | --- |
| 暴风雨 | `0x3B8B` | 告警；虎鲸船首像文案「连可怕的暴风雨也可以躲过」，但 mapper 里未见暴风雨护佑检查 `[未知]` |
| 暴风雪 | `0x3C03` | 告警 |
| 坏血病 | `0x3C2B` | 需靠岸；莱姆可防；mapper 里船首像 9（圣母像 `[推断，强]`）挡坏血病 / 传染病 |
| 老鼠 | `0x3D6F`、`0x126C3` | 粮食危险；「现在开始捉老鼠游戏。请在３０秒之内捉住各船舱出现的老鼠。」 |
| 鲨鱼 | `0x4A0F`、`0x11473` | 小心；「鲨鱼吃了%s」；「得到了鲨鱼的幼鱼」；「也许会成为新的交易品」（结局 25；播放函数 `[未知]`） |
| 海豚 | `0x11A4F` | 「可以将它们记录到航海日志中吗？」无战斗 |
| 无风 | `0x48f3` | happening 结局 24 `calm`（不是气候 id），风速 0 |

捉鼠：`CRatGameView`（`srcdata/source/game/game/CRatGameView.cpp`）ctor 循环 `s20 >= 0x14`，20 只老鼠槽，每槽初始 `0x20`。属 UI / 小游戏。失败文案要求靠港灭鼠；成功「捉住了所有老鼠！那么就可以放心继续航海了。」猫道具可替代。

鲨鱼幼鱼：文案明确「也许会成为新的交易品」。Ocean 只发候选，Economy 决定是否出现新 `trading_item_id`。已有交易品「大型鲨鱼的鳍」（`0x1773`）。

### 航线与测量（与探索共用岗位）

无测量员不能开辟航线，见 `01-sailing.md`。瞭望手道具文案：「如果瞭望手有了它，就会很容易发现宝物和港口。」「可以发现的范围更大。」搜索半径是派生量，唯一计算方是 F-ADJ-01（查询 `Ocean.search_radius(fleet_id)`；船首像经修正器 `fleet.search_radius_add` 贡献，D2 / D6），不读六维。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key | 拥有者 |
| --- | --- | --- | --- | --- |
| `eVillage` | 24 | -1 | `village_id` | World |
| `eRemains` | 25 | -1 | `remains_id` | World |
| `REMAINSSTATE_BITS` | 位域 | | `remains_state` | World 存档 |
| `eSeaArea` | 25 | -1 | `sea_area_id` | World |
| `eMobileFleet` | 4 | | `mobile_fleet_slot` | Fleets |
| `eMapPointKind` | 12 | -1 | `map_point_kind` | World（`01-world/10`） |
| happening 槽 / 结局 | 20 槽 / 26 结局 | `null` | `happening` / `outcome` key | Ocean，05 篇 |
| 霸者之证区域 | 文案 7 个 | | `proof_region` | Items / Narrative |

七个霸者之证区域（文案 `0x1E247` 起，**不是** 25 海域）：北海、地中海、非洲、印度洋、东南亚、东亚、新大陆。Ocean 发现遗迹时只带 `remains_id`；区域归属在 World / Items。

日志条目没有独立枚举。重构用 `log_entry_id`，内容包列出海豚等可记录事件。

## 4. 算法 / 公式

### 4.1 探索命令

```
on command Search { fleet_id }:
  show confirm (ui.ocean.search_confirm)
  if item = first_item_in_radius(fleet): emit DiscoveryFound{item}; return   # 圈内道具优先
  if not near_land(fleet): fail(); return                                    # fn_140012535
  if rand(event_roll_den) == 0:                                              # 原版 4
      region = first_match(region_rects, lat, lon)                           # 原版 6 区
      run(pick_uniform(region_events[region]))                               # 每区 2 个
  else:
      rich = first_match(richness_rects, lat, lon)                           # 原版 0–4
      if rand(100) <= rich*rich_mul + skill(admiral, observe_skill)/skill_div:   # 10, 9, 50
          grant_supplies(rich, warehouse_rooms)
      else: fail()
fail(): fatigue += fail_fatigue (3); crew_exp(15, 10)
```

全部 `[确认]`（§2「探索流程」）。`admiral` 原版是提督本人，不是了望手；了望手只经 `fn_14019dd60` 改半径。重构把「谁的技能」做成 `search.check_person: admiral | post:lookout`，原版包 `admiral`。

### 4.2 探索结果表

内容在 `config/ocean/search.yaml`。地区事件 key 与公式见 §2 表；原版文案组还能对上：

| key | 事件 / 他系统 |
| --- | --- |
| 圈内道具 | `DiscoveryFound { kind: item, item_id }`（霸者之证同路，`kind==8`） |
| 补给检定成功 | `Supplies.add(water, food)`（ships；按物资仓库分船） |
| `oasis_or_attack` | 成功：`Fleets.add_fatigue(-10)` + 可能水粮 +20；失败：`Crew.lose` → `CrewChanged { reason: search }` |
| `hot_spring` | `Fleets.add_fatigue(-15)` + 可能 `Progression.grant_exp` |
| `rest_or_tire` | `Fleets.add_fatigue(±5)` |
| `beasts` / `locals_attack` / `desertion_small` / `crew_loss_5` | `Crew.lose(n)` → `CrewChanged { reason: search }` + `Progression.grant_exp` |
| `gold` | `Force.add_gold(force_id, n, reason: search)` |
| `beasts` 未过检定 | 态空闲时置 happening 结局 `plague`（原版 23，05 篇） |

村落 / 遗迹**不在**本函数里：村落靠 `fn_140001186` 半径自动发现（`13-algorithms` §2.2），遗迹走道具 Locator（霸者之证 / 地图）。旧稿把「发现村庄 / 遗迹」列为探索掷骰结果，没有源码依据，已删。

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
  if slot.fleet_id is null: continue                    # 原版 0x46 空槽，仅 legacy_codec
  if not in_range(player, fleet, spot_range): continue
  emit FleetSpotted { fleet_id: player, other_fleet_id, major_sea_id, sea_area_id?, aggressive }
  # Ocean 停手。Combat 订阅后切 CCombatScreen
```

`spot_range` `[确认]`：`fn_140008530` 分带（`06-npc-fleet-ai.md` §2.5），`|dx| < 0x60 && |dy| < 0x60` 为带 0（方盒，不是圆）；`fn_14000b5ff` 取绝对差并在东西向 `> 20000` 时按 40000 回绕。**不用**搜索半径，瞭望不改它（`13-algorithms` §2.2）。`aggressive` 对应文案「进入战斗状态」vs「向我们进攻了」。海战态（`SceneDesc+0xC==1`）时不调本检测。

玩家主动攻击走命令 `Ocean.engage { other_fleet_id }`，同样只发事件。

### 4.5 海上 happening

全部归 [05-sea-happenings.md](05-sea-happenings.md)：舰队日步（`DayStarted` → `daily` phase 的 `ocean.happening_roll` 步，只走海上枝）掷 20 槽 CDF，结局按 key 查结局表执行 `Action`（D9）。本篇唯一的交叉点：探索处理器 `beasts` 失败时若 happening 态空闲，把它写成结局 23（`plague` 传染病，见下 YAML）。

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

`config/ocean/search.yaml`（地区事件 = D9 `Action`；数值 `[确认]`，key 名 `[推断]`）：

```yaml
schema: dk4.ocean.search
version: 3
confirm_message: ui.ocean.search_confirm
asset: Search.DK4
check_person: admiral            # 原版提督；可改 post:lookout
order: [item_in_radius, near_land_gate, region_or_supply]
item_in_radius:
  radius_ref: Ocean.search_radius   # F-ADJ-01，唯一计算方；贡献 key fleet.search_radius_add
  narrative_trigger: search_item    # 叙事 kind 0
near_land_required: true         # fn_140012535
event_roll_den: 4                # rand(4)==0 → 地区事件
supply_check:
  roll: { impl: expr, int_mode: vanilla_i32,
          expr: "rand(100) <= rich*10 + skill(check_person, observation)/50" }   # idiv 0x32 @ 0x1401b5201
  water: { rand_add: 2, mul: 5 } # (rand(rich+2)+1)*5*rooms
  food:  { rand_add: 1, mul: 5 }
  per_cabin_role: supply_room    # Ships.cabin_count；原版 kind 9
  exp: [10, 5]
fail: { fatigue: 3, exp: [15, 10] }
# 地理表：内容包数据，World1 可换。lat 南为正、lon 东为正，闭区间，首个命中
richness_rects:                  # fn_14000f218；default 1
  default: 1
  rows:
    - { value: 4, lat: [-15, 15], lon: [-90, 30] }
    - { value: 4, lat: [-15, 15], lon: [90, 150] }
    - { value: 3, lat: [-45, -15], lon: [105, 150] }
    - { value: 3, lat: [15, 45], lon: [-60, -30] }
    - { value: 3, lat: [-45, -30], lon: [-105, -60] }
    - { value: 2, lat: [15, 30], lon: [30, 60] }
    - { value: 2, lat: [-15, 0], lon: [60, 90] }
    - { value: 2, lat: [-60, -45], lon: [-30, 30] }
    - { value: 0, lat: [-30, -15], lon: [-30, 60] }
    - { value: 0, lat: [-45, -30], lon: [-120, -90] }
    - { value: 0, lat: [15, 30], lon: [90, 150] }
    - { value: 0, lat: [-90, -60], lon: [-180, 0] }
    - { value: 0, lat: [-90, -60], lon: [30, 180] }
region_rects:                    # fn_140009aca；default 5
  default: 5
  rows:
    - { region: 0, lat: [0, 60], lon: [0, 60] }
    - { region: 1, lat: [-45, 0], lon: [-105, -30] }
    - { region: 0, lat: [0, 30], lon: [-105, -30] }
    - { region: 2, lat: [-45, -30], lon: [120, 150] }
    - { region: 0, lat: [-30, 15], lon: [90, 150] }
    - { region: 3, lat: [-60, -30], lon: [-150, -105] }
    - { region: 4, lat: [-30, 0], lon: [-30, 60] }
    - { region: 4, lat: [15, 45], lon: [90, 135] }
region_events:                   # 每区等概率；可写 weight
  0: [oasis_or_attack, desertion_small]
  1: [oasis_or_attack, beasts]
  2: [oasis_or_attack, hot_spring]
  3: [hot_spring, beasts]
  4: [rest_or_tire, crew_loss_5]
  5: [gold, locals_attack]
actions:                         # D9 Action：roll 是本次掷骰；effects 里 when: ok|fail 按 roll 结果分枝
  # roll 比较都是「v >= rand(den)」，成功率 (v+1)/den；half = v/2
  oasis_or_attack:
    roll: { ability: charm, adjusted: true, den: 101 }
    effects:
      - { when: ok,   do: [ { frame: 0 }, { fatigue: -10 },
                            { chance: { ability: charm, adjusted: true, half: true, den: 100 },
                              do: [ { supply: { water: 20, food: 20 } } ] } ] }
      - { when: fail, do: [ { frame: 1 }, { crew_loss: { base: 10, rand: 3 } }, { crew_exp: [15, 10] } ] }
  desertion_small:
    effects: [ { frame: 7 }, { crew_loss: { base: 3, rand: 3 } }, { crew_exp: [15, 10] } ]
  beasts:
    roll: { ability: luck, adjusted: true, half: true, den: 100 }
    effects:
      - { frame: 3 }
      - { se: 0x1D }
      - { crew_loss: { base: 21, rand: 3, minus_skill: { skill: fencing, div: 25 } } }
      - { when: ok,   do: [ { crew_exp: [15, 10] } ] }
      - { when: fail, do: [ { set_happening_if_idle: plague }, { crew_exp: [15, 15] } ] }   # 原版结局 23
  hot_spring:
    roll: { ability: luck, adjusted: true, half: true, den: 100 }
    effects:
      - { frame: 5 }
      - { fatigue: -15 }
      - { when: ok, do: [ { crew_exp: [10, 10] }, { message: 0x149 } ] }
  rest_or_tire:
    roll: { ability: luck, adjusted: true, half: true, den: 100 }
    effects:
      - { frame: 6 }
      - { when: ok,   do: [ { fatigue: -5 } ] }
      - { when: fail, do: [ { fatigue: 5 }, { crew_exp: [15, 10] } ] }
  crew_loss_5:
    effects: [ { frame: 4 }, { crew_loss: { base: 5 } }, { crew_exp: [15, 10] } ]
  gold:
    effects: [ { frame: 8 }, { gold: { base: 100, rand_ability: luck, adjusted: true, mul: 10 } } ]
  locals_attack:
    effects: [ { frame: 2 }, { crew_loss: { base: 11, rand: 3, minus_skill: { skill: leadership, div: 50 } } },
               { crew_exp: [15, 10] } ]
crew_loss_floor: 1               # fn_140007496：旗舰至少留 1 人
```

`fatigue` 效果调 `Fleets.add_fatigue`，钳制归 fleets（原版 `fn_140009467` 钳 0..100）；`crew_loss` 调 `Crew.lose`，`crew_exp` 调 `Progression.grant_exp`（全员广播由 progression 展开），`gold` 调 `Force.add_gold`。`gold` 封顶走 `caps.gold_max`。能力 / 技能写 key，不写下标（D3）。

`config/ocean/encounters.yaml`：

```yaml
schema: dk4.ocean.encounters
version: 2
# happening 掷骰表 / 结局表归 05 篇 config/ocean/happenings.yaml，这里不重复
player_mobile_slot: 0
spot_box: 96                    # fn_140008530：dx<96 且 dy<96；分带表归 06 篇
fleet_spot:
  message: ui.ocean.fleet_spotted
  engage_message: ui.ocean.enter_battle
# 空槽 0x46 只在 ids/legacy_codec.yaml；运行时 null
```

地点实例不进这些文件。24 村、遗迹表在 `config/world/`。

## 7. 独立旋钮

- 探索权重、确认是否需要瞭望手、搜索圈顶点数
- 舰队遭遇分带
- 点击盒 480×256 / 半径 16（纯 UI）

加一种地区事件 = 在 `search.yaml` 的 `actions` 加一项并挂到 `region_events`，不改代码。加海上 happening 见 05 篇。

## 8. 对外接口

事件：

```
DiscoveryFound { fleet_id, kind: item, item_id }   # 探索只发道具；村落发现走半径（13-algorithms F-ADJ-02），不在本命令
FleetSpotted   { fleet_id, other_fleet_id, major_sea_id, sea_area_id?, aggressive: bool }   # major_sea_id 必填；sea_area_id 可选、默认 null（§13-2）
SuppliesDepleted   # 见 01；探索水粮走 `Supplies.add`，不走这个
# 海上 happening 事件见 05 篇
SearchNothing  { fleet_id }
SearchEvent    { fleet_id, key, frame }          # 地区事件，UI 播 Search.DK4 帧
HappeningSet   { fleet_id, outcome }            # beasts 失败；05 篇状态所有者执行
```

查询：

```
Village.def(village_id) -> { x, y, sea_area_id }；Village.discovered(village_id) -> bool
Remains.def(remains_id) -> { x, y, sea_area_id }；Remains.discovered(remains_id) -> state_bits
Ocean.search_radius(fleet_id) -> int   # F-ADJ-01 唯一计算方；贡献 key fleet.search_radius_add
Ocean.can_search(fleet_id) -> { ok, reason }
Fleets.mobile(slot) -> { fleet_id, state, x, y } | none
Fleets.post_holder(fleet_id, post) -> person_id | null
Ships.cabin_count(ship_id, role) -> int
```

命令：

```
Ocean.search { fleet_id }
Ocean.engage { fleet_id, other_fleet_id }
```

探索结果调他系统命令（Ocean 不发对应事件）：

```
Crew.lose(ship_id, n, reason: search)            # ships 发 CrewChanged { ship_id, crew, reason }；原版只扣旗舰，至少留 1
Progression.grant_exp(person_id, add{track: n}, roll_mode, source: search)   # 原版 fn_14018ba70 全员广播，由 progression 展开
Fleets.add_fatigue(fleet_id, delta, reason: search)   # fleets 钳 0..100 并发 FatigueChanged
Force.add_gold(force_id, delta, reason: search)       # factions 封顶 caps.gold_max
```

World 命令（Ocean 不调用，由会话总线转）：

```
World.mark_village_discovered(village_id, via, fleet_id?, item_id?) / World.mark_remains_discovered(remains_id, via, town_id?, fleet_id?, item_id?)   # 形状见 01-world/03 §8；道具触发 via: item（词表 §14-23）
```

Combat 命令由 Combat 自己在收到 `FleetSpotted` 后发。

## 9. 未核实字段

| 项 | 依据 | 状态 |
| --- | --- | --- |
| `NUM_REMAINS` | 名字指针表 25 + NULL | `[确认]` 25（`01-world/09`） |
| `REMAINSSTATE_BITS` 各位 | 仅类型名 | `[未知]` |
| `NUM_VILLAGEBLINDAREA` | 村表每条两块壳矩形 | `[推断，强]` 2（`01-world/03`） |
| 探索流程与概率 | `fn_14001266b` 原始指令 | `[确认]`（§2）；结果 key 名 `[推断]` |
| `Search.DK4` 帧 0–8 的画面 | 帧号已对到处理器 | `[推断]` 画面名 |
| happening 槽、结局名 | 05 篇 CDF 与名表 `.data 0x140686550` | `[确认]` 表；对齐 `[推断，强]` |
| `0x46` | 活动槽空 `fleet_id`（`NUM_FLEET+1`） | `[确认]`，见 06 篇；只在 `legacy_codec` |
| 舰队遭遇方盒 96 的坐标单位 | `fn_14000b5ff` 差值 | `[推断]` 原版坐标 |
| 温泉经验 | `fn_14018ba70(10,10)`，按能力 5（运气）掷 | `[确认]` 数值 |
| 「沼泽」对应哪个处理器 | 8 个处理器与 9 组文案未一一对齐 | `[未知]` |
| 水手损失人数 | 见 §2 处理器表 | `[确认]` |
| `CItemMysteryHunting` 字段 | 大量布局立即数 | `[未知]` |
| `CDiscoverTitleView+0x40` 初始缩放 | 右移直到 `<8` | `[未知]` 演出参数 |
| 霸者之证 7 区与 `eSeaArea` 25 的映射 | 文案区域 ≠ 海域枚举 | `[未知]` |
| `eMapPointKind` 12 种名字 | `01-world/10-mappoint-kinds.md`（9/10/11 共用 `CMpRemains`） | 见该篇 |
