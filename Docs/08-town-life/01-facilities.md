# 港口设施

城镇生活的入口层。玩家入港后看到的是设施图，不是贸易公式、船数值或人物六维。

设施只做两件事：决定「这座港能点开什么」，以及把点击翻译成对其他系统的命令。价格、船耐久、合约金、人物属性一律不在本系统计算。

## 1. 职责边界

本系统管：

- 设施目录（稳定 `facility_id` / key）
- 每座港开放哪些设施（原版内容包默认「几乎全开」）
- 设施内菜单 → 命令的映射
- 广场作为入港枢纽：人群事件、流行展示、情报入口
- 旅馆休息的**资费与天数**（恢复效果发给人物系统）
- 造船厂修理等待日、木工折扣的**入口参数**（折扣率查询人物修正器）

本系统不管：

| 事项 | 归属 |
| --- | --- |
| 市价、买卖、特产品、投资金额曲线 | economy（`CTrade`、`CInvestmentPadView`） |
| 船型、耐久、改装件、水粮补给量计算 | ships / fleets |
| 招募谁、六维、岗位技能 | characters |
| 合约金、份额、友好度、势力关系 | factions |
| 委托/计略目录与完成判定 | 同目录 `02-requests-schemes.md` |
| 小游戏规则与奖励 | 同目录 `03-minigames.md` |
| 港口坐标、首都标记、region | world |
| 文案字符串 | ui / localization |

交易所在原版是 `CTrade`，挂在广场按钮上，但会话与货格属于经济系统。本系统只发 `OpenExchange { town_id, fleet_id }`。

## 2. 原版事实

### 2.1 设施对象是空壳，玩法在场景

`[确认]` `CBar` `CInn` `CGuild` `CPalace` `CPort` `CShipyard` 的还原 cpp 只有 ctor / dtor，没有菜单逻辑。`CBar` 写 `+0x14=0`、`+0x18=5`；`CGuild` 写 `+0x1c=0`。来源：`srcdata/source/game/town/CBar.cpp` 等。

真正的入港场景是：

| 类 | 来源 | 作用 |
| --- | --- | --- |
| `CTownScene` / `CTownSceneView` | `town/CTownScene.cpp` `CTownSceneView.cpp` | 港口地图；`eMapPointKind` 槽 0–11 |
| `CPlazaView` / `CPlazaScreen` / `CPlazaButton` | `town/CPlazaView.cpp` 等 | 广场枢纽与设施按钮 |
| `CShopButton` | `town/CShopButton.cpp` | 广场上的店铺按钮 |
| `CMapPointView` | `town/CMapPointView.cpp` | 地图热点；ctor 写 `+0xa8=0xd` |
| `CTownInformationView` | `town/CTownInformationView.cpp` | 港口情报面板 |
| `CPlazaSylphid` | `town/CPlazaSylphid.cpp` | 广场角色动画，循环 10 槽 |

`CMpDock` 同目录，但是海域地图的停泊点，不是港内「码头」菜单。

### 2.2 广场点击编号

`[确认]` `CPlazaView::vfunc_13`（`CPlazaView.cpp`）：

- `4 <= id < 9`：走 `CPlazaScreen` 上 5 个槽（对象 `+0xf0` 起，ctor 清 5 个指针）
- `id == 1` `2` `3` `0xA`：另四条分支

`[确认]` `CTownSceneView::vfunc_13`：

- `0 <= id < 0xC`：12 个 `eMapPointKind`（`NUM_MAPPOINTKIND=12`，`MapPointID.h`）
- `0xC <= id < 0x11`：再 5 个点击号（12–16）

`[推断]` 12 个地图热点 + 广场 5 槽，是港内可点物的两套坐标，不是 17 种不同设施。依据：设施类只有上表那些；`eMapPointKind` 循环上界固定 12，与城镇表无 bitmask。

`CPlazaScreen` ctor 写死四对整数：`(0x96,0xDA)` `(0x8A,0xBA)` `(0x3AB,0x287)` `(0x3C0,0x316)`。`[推断]` 广场布局坐标，不是玩法规则。

### 2.3 文案里的设施与菜单

SC `mesfile_sc.txt`（用户检索词：广场 / 酒馆 / 旅馆 / 公馆 / 行会 / 造船 / 码头 / 教会 / 宫殿 / 委托 / 计略 / 投资 / 合约 / 情报 / 流行）：

| 设施 | 文案用法 | 标记 |
| --- | --- | --- |
| 广场 | 「提督。好像广场上聚集了一群人。」（多条变体，offset `0x12c` 起） | `[确认]` |
| 酒馆 | 迎客、倒酒、情报、招募；「如果参谋在，可在酒馆里拟定策略。」`0xc237` | `[确认]` |
| 旅馆 | 「要１枚金币。好的，您请休息吧。」「住１晚１枚金币，要住几天？」`0xba97` `0xbab7` | `[确认]` |
| 造船 / 船坞 | 买船入坞、修理、改造；「新船放在船坞里。」`0x7df7` | `[确认]` |
| 码头 / 港 | 「在%s停靠吗？」补给水粮比例 `0x81af` `0x822f` | `[确认]` |
| 同业公会 | 委托交付点「送到%s的同业公会」`0x15267`；停战入口「前去『同业公会』要求停战吗？」`0x18463` | `[确认]` |
| 教会 | 祈祷、仪式、巡礼、绝食、取圣水；`CChurchGameView` | `[确认]` |
| 王宫 / 总督府 | 「拿着这个到小城市的王宫或者总督府去」`0x7d47`；签合约、份额 | `[确认]` |
| 交易所 | 「如果是大宗买卖，请去交易所。」`0x5fc3`；主计长杀价 `0xc237` | `[确认]` 入口在本系统，算法在 economy |
| 公馆 | SC mesfile **无**「公馆」二字 | `[未知]` 是否为总督府别译 |
| 行会 | 无独立词；功能由「同业公会」承担 | `[推断]` 与 guild 同一设施 |

旅馆恢复：`0xc0b3`「在这里船员身体和精神都能够恢复。」`[确认]` 身体 / 精神是人物状态，本系统只发休息命令。

木工折扣：`0x7fcb`「如果那个人肯帮忙，修理费用就少算%s％吧……。」`0xc337` 改造费同样「减少%s％」。`[确认]` 折扣百分比来自在队木工，本系统不读六维。

修理等待：`0x8037`「到修理完成前，还得等一天。」改造：`0xc39f`「到改造完成要等２天。」另有「要等%s天」`0xc3c3`。`[确认]` 修理默认 1 日；改造默认 2 日，也可走 `%s` 日。

补给：`0x81cb` 水手疲劳提示休息；`0x822f`「改变供水和供食物的比例吗？」；`CSupplyRateView`（`game/CSupplyRateView.cpp`）是比例 UI。`[确认]` 码头打开补给，数量公式在 ships。

### 2.4 每港是否缺设施

`[确认]` `GetFixTownProc` 记录 0x84 字节（`CTown::vfunc_05` RTC `r_sFixTown` 132b）。序列化循环：3 个流行槽、2 个字/dword、5 个特产品、**19** 个 word（= `NUM_TRADINGITEMKIND`）、2 字节、`+0x74/+0x75`。没有设施 bitmask。

`[确认]` 小城市也有王宫/总督府（`0x7d47`）。首都标记是 world 的 `is_capital`（伦敦等 21 座，见 `04-id-registry.md`），只改称号与海域图标，不删设施。

`[推断]` 原版内容包 97 港「设施全开」。`eTownKind` 在海域图循环上界为 2（`CSeaAreaMapView` `eTownKind >= 2`），对应首都 / 非首都两种画法，不是「有无宫殿」。

`[确认]` 流行每港 3 槽：`CTown::vfunc_05` 槽值 `<=0` 时写入 `0x1A`。`NUM_TOWNFASHION=26`，`0x1A` 即空哨兵。`[推断]` 空流行 = 26，不是设施开关。

### 2.5 广场人群与流行

`[确认]` 入港台词「广场上聚集了一群人」多主角变体（`0x12c`–`0x217`）。

`[确认]` 广场可散发交易品：传教士在队时「在广场上还能散发交易品。」`0xc1fb`。

`[确认]` 零售店招牌听流行：「请选择零售店的招牌，就可以听取流行情报。」`0x1143`；阿科布把流行带到别的广场 `0x6fa3`。流行目录属 factions（`eTownFashion=26`），广场只展示与触发。

### 2.6 酒馆配角

`[确认]` RTC `eBarmaid`，循环 `eBarmaid >= 0x15`（`fn_14000448f`，`fn_140032ba0.cpp`）。`[推断]` 女招待表长 21。`CBar+0x18=5` `[推断]` 同屏最多 5 人，不是表长。

酒馆酒价文案：`0x733`「需要１０枚金币。5枚金币。」`0x7bf`「收您２０枚金币。10枚金币。」`[确认]` 出现 5 / 10 / 20，档位如何绑定人物未还原。

招募入口：`0x7627`「有人要乘坐提督的船……让他上船吧？」`[确认]` 酒馆发招募命令，名单在 characters。

划拳：`0x753f`「喂，我们划拳吧。」不是 `C*GameView` 类，见小游戏文档「酒吧划拳」。

### 2.7 世界对象持有

`[确认]` `CWorld` ctor 在 `+0x182a0` 构造 `CSchemeElement`，`+0x19f48` 构造 `CRequestElement`（`world/CWorld.cpp` 调 `fn_1402a65c0` / `fn_1402a64d0`）。委托/计略表挂在世界对象上，入口仍是公会/酒馆。

## 3. 实体与 ID

| 实体 | 原版符号 | 计数 | 空/非法 | 来源 |
| --- | --- | --- | --- | --- |
| 设施 | 无独立 `eFacility` | 内容包 9 个入口 | — | 类名 + 文案 |
| 地图热点种类 | `eMapPointKind` | 12 | `INVALID_MAPPOINTKIND=-1` | `MapPointID.h` / RTC |
| 城镇种类 | `eTownKind` | `[未知]` 整型；循环上界 2 | `INVALID_TOWNKIND=-1` | `TownKindID.h` |
| 城镇通行 | `eTOWNACCESS` | `[未知]` | `INVALID_TOWNACCESS=-1` | `TownaccessID.h`；`STownAccess` |
| 流行 | `eTownFashion` | 26 | 槽写入 `0x1A` 为空 `[推断]` | `TownFashionID.h` |
| 女招待 | `eBarmaid` | `[推断]` 21 | — | RTC 循环 `0x15` |
| 港口 | `eTown` | 97 | `INVALID_TOWN=-1` | world |

重构设施 ID（原版无枚举，内容包赋予稳定 key；整数从 0 起，仅原版包）：

| id | key | 原版类 | 中文 | 默认命令目标 |
| --- | --- | --- | --- | --- |
| 0 | `plaza` | `CPlazaView` | 广场 | 本系统（人群/流行/情报） |
| 1 | `exchange` | `CTrade` | 交易所 | economy |
| 2 | `bar` | `CBar` | 酒馆 | characters / 计略 / 本系统酒价 |
| 3 | `inn` | `CInn` | 旅馆 | characters（休息） |
| 4 | `shipyard` | `CShipyard` | 造船所 | ships |
| 5 | `port` | `CPort` | 码头 | ships（补给/入坞） |
| 6 | `guild` | `CGuild` | 同业公会 | 委托 / factions 停战入口 |
| 7 | `church` | `CChurchGameView` | 教会 | 本系统小游戏 + characters 休息 |
| 8 | `palace` | `CPalace` | 王宫/总督府 | factions 合约；economy 投资入口 |

广场点击号 1–3、4–8、`0xA` 与上表 id **尚未一一还原**，配置里单独放 `click_id`，禁止在代码里写死 `if (click==4)`。

## 4. 算法 / 公式

设施层公式极少。全部进 `config/formulas/town.yaml`，禁止在场景代码里写立即数。

```
inn_cost = inn_gold_per_night * nights
inn_gold_per_night     # 原版默认 1
nights ∈ [nights_min, nights_max]

repair_wait_days       # 原版默认 1
remodel_wait_days      # 原版默认 2；文案另有 %s 日
repair_price_quoted    = Ships.quoteRepair(fleet_id)          # 外系统
repair_price_charged   = repair_price_quoted
                       * Characters.modifier(carpenter_repair_factor)
# 文案「少算%s％」→ factor = 1 - percent/100；percent 由人物系统提供

bar_drink_cost ∈ drink_price_table     # 原版文案出现 5, 10, 20
```

休息效果不在本系统：

```
TownLife.RestAtInn { person_ids, nights }
  → Characters.applyRest(body, mind, nights, inn_rest_params)
```

`inn_rest_params`（每夜恢复多少身体/精神、是否清疲劳）`[未知]`，人物文档填，本系统只传 `nights`。

补给比例 UI 打开后：

```
TownLife.OpenSupply { fleet_id, town_id }
  → Ships.restock(fleet_id, water_ratio, food_ratio, town_prices)
```

比例与桶数属于 ships；本系统只提供「打开码头补给」命令。

人群事件：

```
on ArrivedAtTown:
  if rand() < plaza_crowd_chance:   # 原版触发率 [未知]
    emit PlazaCrowdGathered { town_id }
```

原版是否每次入港都喊「聚集了一群人」，未从 cpp 还原，只确认文案存在。

## 5. 耦合分析

原版硬编码：

- 广场按钮直接 new `CTrade` / 造船 UI / 人物招募 UI
- 旅馆直接改人物身体/精神字节
- 造船修理读在队人物是否「船木工」（岗位名来自 mesfile，技能枚举未名）
- 宫殿直接改份额与合约（factions 状态）
- 公会直接扫 `CRequest` 表
- `NUM_MAPPOINTKIND=12`、广场 5 槽写死在 ctor
- 酒价 5/10/20、住宿 1 金币写在 mesfile 与代码立即数两侧

允许的依赖方向（`03-decoupling-principles.md`）：

```
town-life → world.town_id
town-life → characters（查询 ModifierSet、发 Rest / Recruit 命令）
town-life → economy（OpenExchange / OpenInvestment）
town-life → ships（OpenShipyard / OpenSupply / QuoteRepair）
town-life → factions（OpenContract / 停战入口）
town-life → 本目录 requests / schemes / minigames
```

禁止：economy 改设施表；combat 写旅馆；ui 写存档设施状态。

## 6. 解耦后的配置

`config/town/facilities.yaml` — 本系统唯一设施表。

```yaml
schema: dk4.town.facilities
version: 1

facilities:
  - id: 0
    key: plaza
    name: 广场
    name_i18n: ui.facility.plaza
    original_class: CPlazaView
    commands:
      - listen_rumor
      - show_fashion
      - distribute_goods      # 需 characters.modifier.can_distribute_goods
      - launch_crowd_event
  - id: 1
    key: exchange
    name: 交易所
    original_class: CTrade
    commands: [open_exchange]
  - id: 2
    key: bar
    name: 酒馆
    original_class: CBar
    commands: [buy_drink, recruit, open_scheme, listen_bar_rumor, rps_game]
  - id: 3
    key: inn
    name: 旅馆
    original_class: CInn
    commands: [rest]
  - id: 4
    key: shipyard
    name: 造船所
    original_class: CShipyard
    commands: [buy_ship, sell_ship, repair, remodel, name_ship]
  - id: 5
    key: port
    name: 码头
    original_class: CPort
    commands: [supply, dock, undock]
  - id: 6
    key: guild
    name: 同业公会
    original_class: CGuild
    commands: [list_requests, deliver_request, ask_ceasefire]
  - id: 7
    key: church
    name: 教会
    original_class: CChurchGameView
    commands: [pray, church_minigame, rest]
  - id: 8
    key: palace
    name: 王宫
    name_variants:
      capital: ui.facility.palace
      town: ui.facility.governor
    original_class: CPalace
    commands: [open_contract, open_investment, donate_brand]

# 原版包：97 港全开。模组可按 town_id 减设施。
availability:
  mode: all           # all | per_town
  palace_always: true
  capital_uses_palace_title: true

click_map:            # 原版点击号，未还原的标 null
  plaza_view:
    1: null
    2: null
    3: null
    4: null
    5: null
    6: null
    7: null
    8: null
    10: null
  town_scene:
    map_point_count: 12
    extra_click_lo: 12
    extra_click_hi: 16
```

`config/formulas/town.yaml`：

```yaml
schema: dk4.formulas.town
version: 1
inn_gold_per_night: 1
inn_nights_min: 1
inn_nights_max: null          # 原版上限 [未知]
repair_wait_days: 1
remodel_wait_days: 2
bar_drink_prices: [5, 10, 20]
plaza_crowd_chance: null      # [未知]
```

港口实例不重复抄设施列表。`config/world/towns.yaml` 只存 `town_id`；若 `availability.mode=per_town`，另写：

```yaml
# config/town/town_facilities.yaml  仅 mode=per_town 时需要
- town_id: 0
  key: london
  facilities: [plaza, exchange, bar, inn, shipyard, port, guild, church, palace]
```

原版包可省略该文件（`mode: all`）。

## 7. 独立旋钮

改这些值不应迫使其他系统改代码：

- 增减设施（新 key + commands 列表）
- 某港关闭宫殿或教会
- 住宿单价、修理/改造等待日
- 酒价表
- 广场点击号 → 设施映射
- 首都用「王宫」还是「总督府」的文案 key

改这些会碰到边界，但仍不应改贸易/战斗代码：

- 休息恢复曲线 → 只动 `characters` 的 rest 公式
- 木工折扣 → 只动 `ModifierSet.carpenter_repair_factor`
- 补给桶数 → ships

## 8. 对外接口

事件（本系统发出）：

```
TownEntered { fleet_id, town_id }
FacilityOpened { town_id, facility_id }
PlazaCrowdGathered { town_id }
InnRested { town_id, nights, gold_paid }
BarDrinkPaid { town_id, gold_paid }
```

查询：

```
TownLife.facilities(town_id) -> [facility_id]
TownLife.isOpen(town_id, facility_id) -> bool
TownLife.innQuote(nights) -> gold
TownLife.plazaFashion(town_id) -> [fashion_id]   # 经 factions 查，空槽丢弃
```

命令（本系统接收点击后转发，参数只有 ID 与标量）：

```
OpenExchange { town_id, fleet_id }          → economy
OpenShipyard { town_id, fleet_id }          → ships
QuoteRepair { fleet_id }                    → ships
OpenSupply { fleet_id, town_id }            → ships
RecruitAtBar { town_id }                    → characters
RestAtInn { town_id, nights }               → characters
OpenContract { town_id }                    → factions
OpenInvestment { town_id }                  → economy
ListRequests { town_id }                    → town-life/requests
OpenScheme { town_id }                      → town-life/schemes
LaunchMinigame { minigame_id, source }      → town-life/minigames
DistributeGoods { town_id, item_id }        → economy（数量/效果由其定义）
```

存档：设施定义不存。状态若有「今晚是否已休息」`[未知]`；有则进 save schema，无则每次入港按表计算。

## 9. 未核实字段

- 无 `eFacility` 枚举；上表 id 0–8 是重构编号，不是原版符号。
- `CPlazaView` 点击 1/2/3/4–8/`0xA` 各对应哪座设施。
- `eMapPointKind` 12 值的名字；`CMapPointView+0xa8=0xd` 的语义。
- `eTownKind` 的 `NUM_TOWNKIND`；是否仅首都/非首都。
- `eTOWNACCESS` / `STownAccess`：dtor 把槽写成 `0xC9`，语义未还原。
- 城镇表 0x84 里 `+0x10/+0x14`、`+0x70` 两字节、`+0x74/+0x75`：不是设施开关，具体名见 world 文档。
- 「公馆」一词未出现在 SC mesfile。
- 酒价 5/10/20 如何选档；是否与魅力修正器有关。
- `eBarmaid=21` 与 `CBar+0x18=5` 的关系。
- 旅馆 `nights` 上限；每夜恢复身体/精神的数量。
- 改造「%s 天」与默认 2 天何时分叉。
- 木工折扣百分比公式（人物系统）。
- 人群事件触发率、以及它打开的是小游戏还是阿科布/流行。
- `CPlazaSylphid` 10 槽状态机（`+0x2d4` 出现 0–4）是否纯表现。
- `CShopButton+0xa8=0x6A`（106）是否误用 `ITEM_EMPTY` 哨兵。
- `CAcrossGameView` 是否也从广场进入（见小游戏文档）。
