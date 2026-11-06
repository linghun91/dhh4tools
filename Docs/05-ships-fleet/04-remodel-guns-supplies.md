# 改装、火炮与补给

造船厂把一艘已有的船改成另一套帆 / 部件 / 炮 / 船首像，并修理、补给。战斗里的炮弹对象也在这里给目录，伤害公式留给 `07-combat`。

## 1. 职责边界

本系统管：

- 改装件 `eAddParts`（9）与甲板改装槽（4）
- 桅 / 帆 `eShipMast`（目录 100；甲板 UI 画 3 根）
- 船首像 `eFigureHead`（与道具 114–123 重叠，见下）
- 火炮种类 `eGunType`（6）与战斗炮位 `eBattleGun`（5）
- 飞行炮弹实例 `eShell`（100）——是弹道对象，不是弹药种类表
- 补给 `eShipSupply`（水、粮食至少两种）
- 修理 / 改造 / 买船的**天数与费用参数**

本系统不管：

- 船体目录与货格（`01`）
- 舱室绑定（`03`）；本系统只发「追加船舱」命令去改 `cabin_kind`
- 市价（经济系统报价，造船厂付钱）
- 炮击伤害、装填时间的战斗结算（本系统提供「炮种 id、炮位、有几个炮手室」）

## 2. 原版事实

### 计数

| 符号 | 值 | 来源 |
| --- | --- | --- |
| `NUM_ADDPARTS` | 9 | `num_values.json` RTC |
| `NUM_DECKADDPARTS` | 4 | `num_values.json` |
| `NUM_SHIPMAST` | 100 | `ShipMastID.h` |
| `NUM_GUNTYPE` | 6 | `GunTypeID.h` RTC |
| `NUM_BATTLEGUN` | 5 | `BattleGunID.h` |
| `NUM_SHELL` | 100 | `ShellID.h` |
| `NUM_FIGUREHEAD` | `[未知]` | `FigureHeadID.h` |
| `NUM_SHIPSUPPLY` | `[未知]` | `ShipSupplyID.h` |

甲板改装 View `CDeckRemodelView` RTC：`eDeckAddParts`、`eShipMast`、`eShipCabin`、`eButton`。桅循环 `>= 3`，改装件循环 `>= 4`，舱室循环 `>= 0x1F`。来源：`CDeckRemodelView.cpp`。

### 改装件与帆（文案）

来源：`mesfile_sc.txt` `0xc39f`–`0xc767`。

**部件（与 `eAddParts` 0–8 的下标对应未还原）：**

- 追加船舱：速度降低
- 撞角：侧面冲撞
- 船首炮：向前攻击，威力小
- 船首帆：中型以上；提高**横帆**，顺风加速
- 装甲板：耐久提高、速度稍降
- 二层装甲板：耐久再提高、速度再降
- 船尾炮：向后攻击，命中率低
- 船尾帆：大型船；提高**纵帆**，逆风加速
- 更换船首像
- 更改大炮种类

9 个 `eAddParts` 与上面清单不是逐条对齐（清单把船首像 / 换炮也写进去了）。`fn_14000c35b` 对 **`eAddParts` 2 和 3** 使用 `anAddArmorSizeTable`，所以 2、3 是装甲 `[推断]`。

**帆（`eShipMast` / 甲板加帆按钮 `CDeckAddSailButton`）：**

| 帆 | 效果 | 约束 |
| --- | --- | --- |
| 船首帆 | 横帆↑，顺风↑ | 中型以上 |
| 船尾帆 | 纵帆↑，逆风↑ | 大型 |
| 上帆 | 横帆↑，顺风↑ | — |
| 斜帆 | 纵帆↑，逆风↑ | — |
| 三角帆 | 逆风↑ | 不能张挂在四角帆**之前** |
| 四角帆 | 顺风↑；文案另说「能提高纵帆、横帆两种」 | 不能张挂在三角帆**之后** |

百分比 / 速度加算 `[未知]`。禁止编造。

「在海上不能更换船首像。」来源：`0x3753`。

### 装甲数值

`fn_14000c35b`（`0x1400a0420`）RTC `anAddArmorSizeTable`：

```
table = [6, 8, 10]     # 12 字节，3 个 int
eAddParts 从 2 循环到 3
装上且原来没有：耐久相关值 += table[size_class]
拆掉：-=
```

`size_class` 来自船体耐久阈值，见 `01`（4000 / 7000）。来源：`game/unassigned/fn_140095cd0.cpp` 附近的 `fn_14000c35b`。

### 改装费

`fn_140013d68`（`0x1400a9d20`）RTC `ulPrice`、`eAddParts`、`aulAddPartsPrice`、`eShipMast`。相对「当前船 vs 预览船」差价：

| 条件 | 加价 |
| --- | --- |
| 某根桅的种类变了 | `+2000`（`0x7D0`） |
| 该桅另外两个布尔状态从无到有（两个独立检查） | 各 `+1000`（`0x3E8`） |
| 某个 `eAddParts` 从无到有 | `+ aulAddPartsPrice[size_class]`，表 `{1000, 800, 1200}` |

货格数变化再走 `fn_140013593` 做一次差价（半价分支 `>> 1`）。最后 `ulPrice` 交给另一个函数（可能含人物折扣）。

帮助改造：「如果那个人能够帮忙，改造费可以给你减少%s％。」来源：`0xc337`。折扣公式 `[未知]`，只有百分比占位。

### 天数

| 行动 | 文案 | 来源 |
| --- | --- | --- |
| 修理 | 「到修理完成前，还得等一天。」 | `0x8037` |
| 改造 | 「到改造完成要等２天。」另有「要等%s天。」 | `0xc39f` / `0xc3c3` |
| 买船 | 「准备就需要%s天的时间，买吗？」 | `0x8053` |

「购买与已改造好的船完全一样的船？好了，那就先存放在船坞吧。」「假若和%s是同一规格。就没有必要进行改造了。」来源：`0x7ea3` / `0x80a7`。

### 修理费

「修理要花%s枚金币，可以吗？」「如果那个人肯帮忙，修理费用就少算%s％。」「花%s枚金币就可以完全修理。」没钱：`0x7fcb`。来源：`0x7fcb`–`0x8053`。底价公式 `[未知]`。

### 船首像

道具表 114–123 都是「×像」，与 mesfile 船首像说明一一对应。来源：`Docs/_data/items.tsv`、`mesfile_sc.txt` `0x1b2cb`–`0x1b52f`。

| item_id | 名 | 文案效果 |
| --- | --- | --- |
| 114 | 大鹫像 | 船飞速前进 |
| 115 | 小猪像 | 减少饭量 |
| 116 | 虎鲸像 | 躲暴风雨 |
| 117 | 白鲸像 | 不易沉 |
| 118 | 龙像 | 对敌炮击提高 |
| 119 | 海豚像 | 看见更多 |
| 120 | 处女像 | 舒适安逸 |
| 121 | 魔鬼像 | 各种效果；不祥 |
| 122 | 王者像 | 鼓舞水手 |
| 123 | 圣母像 | 抵御瘟疫 |

`NUM_FIGUREHEAD` 未还原；这 10 个是道具目录里的船首像 `[确认 道具]`，是否等于 `eFigureHead` 0–9 `[推断]`。

拒绝：「噢，真的要把这船首像安装上去吗？那，那船首像的船我可驾驶不了。」来源：`0xc767`。哪几个 id 触发、是否只对特定主角 `[未知]`。UI：`CFigureHeadButton`。另有 RTC `_bTakeAwayFigureHead`（`0x14011d1f0`）。

### 火炮

- `[确认]` `CSelectGunTypeView` 选炮种。来源：`game/ship/CSelectGunTypeView.cpp`。
- `[确认]` `fn_140003bb1`（`0x1400a3290`）RTC `eGunType`、`anNecessaryDevelopment`（24 字节 = 6 个 int）：

```
1, 1, 2000, 3500, 5000, 7000
```

对 `eGunType` 0–5：若当前港口发展度 `s70 < 表[gun]` 则该炮种不上架。`s70` 来自 `fn_14000bc03(..., 1)`，商业 / 军事哪一条 `[未知]`，但单位与港口发展度 0–9999 同量级。

- `[确认]` 6 个炮种的**中文名不在** mesfile。保持 `[未知]`。
- `[确认]` `CBattleGun` 存档两个 u16：`+0x8`、`+0xA`。来源：`CBattleGun.cpp`。
- `[确认]` `NUM_BATTLEGUN=5`：每艘战斗船 5 个炮位 `[推断 用途]`（与玩家 5 船不是同一枚举：战斗里攻方 / 守方也各 5 船，`NUM_ATTACKERFLEETSHIP=5` 等）。
- `[确认]` `CShell` 存档：`+0x0C..+0x20` 六个 dword，`+0x24/+0x26/+0x28` 三个 u16。100 个实例挂在 `CMobileShipElement+0x1230`。来源：`CShell.cpp`、`CMobileShipElement.cpp`。这是飞行中的弹，不是「炮弹种类=100」。

### 补给

- `[确认]` 文案把物资分成**水**和**粮食**。来源：`0x33ab`「得到了%s桶水，%s桶粮食」、`0x822f`「改变供水和供食物的比例吗？」、`0x81af`「水好像很少，要平均补给吗？」、`0xc59f`「收藏水和粮食的房间」。
- `[确认]` `CSupplyRateView` 改补给比例。来源：类名 + `0x822f`。
- `[确认]` `anSupply` RTC 8 字节 = 两个 int32。来源：`fn_14000646f`（`0x140173490`）。
- `[确认]` 该函数：目标量 = 某容量 `s28` × 单价或定额 `s54/s58`；缺口 × 单价 `s6C/s70` 相加得花费；钱够才补。两条都够则花费 0。来源：`game/unassigned/fn_14016cc40.cpp` 内 `fn_14000646f`。
- `[确认]` 符号 `SUPPLYAMOUNT_MIN/MAX`。来源：`constants.json`。十进制 `[未知]`。
- `[未知]` `NUM_SHIPSUPPLY`。从文案与 8 字节 `anSupply` **推断为 2**（水、粮）。弹药是否第三种补给：mesfile 造船段没写，保持 `[未知]`。
- `[确认]` 大厨 / 饲养员节约水粮（`03` 的 flags）。老鼠事件吃粮食（航海系统）。

### 其它符号

`ADDPARTS_BITS`、`MASTSTATE_BITS`、`FIGUREHEAD_ID`、`GUNTYPE_ID`、`SHELL_ID`、`BATTLEGUN_ID`。来源：`constants.json` MAXMIN。位宽未还原。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空 | 重构 key |
| --- | --- | --- | --- | --- |
| 改装件 | `eAddParts` | 9 | `INVALID_ADDPARTS=-1` | `add_part_id` |
| 甲板改装槽 | `eDeckAddParts` | 4 | — | `deck_add_part_slot` |
| 桅定义 | `eShipMast` | 100 | `INVALID_SHIPMAST=-1` | `mast_id` |
| 船首像 | `eFigureHead` | `[未知]` | `INVALID_FIGUREHEAD=-1` | `figurehead_id` |
| 炮种 | `eGunType` | 6 | `INVALID_GUNTYPE=-1` | `gun_type_id` |
| 战斗炮位 | `eBattleGun` | 5 | `INVALID_BATTLEGUN=-1` | `battle_gun_slot` |
| 飞行炮弹 | `eShell` | 100 | `INVALID_SHELL=-1` | `shell_instance_id` |
| 补给种类 | `eShipSupply` | `[未知]`（文案 2） | `INVALID_SHIPSUPPLY=-1` | `supply_id` |

船首像与 `eItem` 114–123 的关系：内容包可以 `figurehead.item_id` 外键到道具，避免两套效果表。

## 4. 算法 / 公式

全部进 `config/formulas/ships.yaml`。帆的「加快」没有原版数字，参数留 null，直到抽出。

```
# 体型档：见 01 size_class(durability)

# 装甲耐久
armor_bonus = sum(an_add_armor_size[size_class] for each installed armor part)
# an_add_armor_size default [6, 8, 10]

# 改装差价（fn_140013d68）
price = 0
for mast in 0..2:
    if mast_kind_changed: price += mast_kind_fee          # 2000
    if mast_flag_a_on_new and not old: price += mast_flag_fee   # 1000
    if mast_flag_b_on_new and not old: price += mast_flag_fee   # 1000
for part in 0..8:
    if part_on_new and not old: price += add_parts_price[size_class]
      # add_parts_price default [1000, 800, 1200]  下标是 size_class 原版 0/1/2
price += hold_delta_fee(old_holds, new_holds)             # fn_140013593，含 >>1 分支
price = apply_helper_percent(price, helper_discount_pct)  # 文案 %s％，公式未知
if price < 0: price = 0

# 炮种解锁
gun_unlocked(gun_type_id, town_develop) =
    town_develop >= gun_necessary_development[gun_type_id]
# default [1, 1, 2000, 3500, 5000, 7000]

# 三角帆 / 四角帆互斥
not (lateen_before_square)     # 「不能张挂在四角帆之前」
not (square_after_lateen)      # 「不能张挂在三角帆之后」

# 修理
repair_days = formulas.repair_days                        # 1
repair_gold = unknown_base(durability_missing) * (100 - helper_discount_pct) / 100

# 改造
remodel_days = formulas.remodel_days                      # 2；文案另有 %s 天
# 买船
purchase_prep_days = hull.prep_days                       # 文案 %s，按船体表

# 补给
target[water] = capacity_factor * water_rate
target[food]  = capacity_factor * food_rate
need[s]       = max(0, target[s] - current[s])
supply_gold   = need[water]*price[water] + need[food]*price[food]
# CSupplyRateView 改的是 water_rate : food_rate
```

人物帮忙的百分比：UI 传入，本系统不读六维。

## 5. 耦合分析

- 改装费用耐久推体型档，耐久属于 `01`。
- 追加船舱改 31 槽布局，发 `Cabins.setKind`。
- 炮种解锁读港口发展度（世界 / 势力），本系统只收一个 `town_develop` 标量。
- 船首像效果（避风暴、减粮、炮击）是修正器，航海 / 战斗来查，不在本系统结算伤害。
- 拒绝驾驶某船首像：人物 / 剧本条件，本系统提供 `figurehead_id` 与 `refusable` 标记。
- `CShell` 池建在移动船元素里（`02` 的对象树），重构后炮弹池归战斗。
- 补给消耗在航海 tick；本系统管港口补给会话。

## 6. 解耦后的配置

```yaml
# config/formulas/ships.yaml  （续 01）
repair_days: 1
remodel_days: 2
mast_kind_fee: 2000
mast_flag_fee: 1000
add_parts_price: [1000, 800, 1200]    # 按 size_class 0/1/2
an_add_armor_size: [6, 8, 10]
gun_necessary_development: [1, 1, 2000, 3500, 5000, 7000]
```

```yaml
# config/ships/add_parts.yaml
schema: dk4.ships.add_parts
version: 1
- id: 0
  key: add_part_0
  name: "[未知]"
- id: 2
  key: armor_1
  name: 装甲板                    # [推断] 下标 2、3 走装甲表
  durability_add_by_size: [6, 8, 10]
  speed_penalty: null
- id: 3
  key: armor_2
  name: 二层装甲板
  durability_add_by_size: [6, 8, 10]
  speed_penalty: null
```

```yaml
# config/ships/sails.yaml
schema: dk4.ships.sails
version: 1
rules:
  lateen_not_before_square: true
  square_not_after_lateen: true
  bow_sail_min_size: medium
  stern_sail_min_size: large
kinds:
  - key: bow_sail
    name: 船首帆
    square_sail: +unknown
    downwind: +unknown
  - key: stern_sail
    name: 船尾帆
    lateen_sail: +unknown
    upwind: +unknown
  - key: top_sail
    name: 上帆
    square_sail: +unknown
    downwind: +unknown
  - key: staysail
    name: 斜帆
    lateen_sail: +unknown
    upwind: +unknown
  - key: lateen
    name: 三角帆
    upwind: +unknown
  - key: square
    name: 四角帆
    downwind: +unknown
    square_sail: +unknown
    lateen_sail: +unknown          # 文案「两种帆」
```

```yaml
# config/ships/figureheads.yaml
schema: dk4.ships.figureheads
version: 1
- id: 0
  key: eagle
  item_id: 114
  name: 大鹫像
  modifiers: { sail_speed: unknown }
- id: 1
  key: piglets
  item_id: 115
  name: 小猪像
  modifiers: { food_use: unknown }
- id: 2
  key: orca
  item_id: 116
  name: 虎鲸像
  modifiers: { storm_avoid: unknown }
- id: 3
  key: white_whale
  item_id: 117
  name: 白鲸像
  modifiers: { sink_resist: unknown }
- id: 4
  key: dragon
  item_id: 118
  name: 龙像
  modifiers: { gun_attack: unknown }
- id: 5
  key: dolphin
  item_id: 119
  name: 海豚像
  modifiers: { discovery: unknown }
- id: 6
  key: maiden
  item_id: 120
  name: 处女像
  modifiers: { comfort: unknown }
- id: 7
  key: devil
  item_id: 121
  name: 魔鬼像
  modifiers: { mixed: unknown }
  refusable: true                  # 拒驾名单未还原，先可配
- id: 8
  key: king
  item_id: 122
  name: 王者像
  modifiers: { morale: unknown }
- id: 9
  key: madonna
  item_id: 123
  name: 圣母像
  modifiers: { plague_resist: unknown }
```

```yaml
# config/ships/guns.yaml
schema: dk4.ships.guns
version: 1
- id: 0
  key: gun_type_0
  name: "[未知]"
  necessary_development: 1
- id: 1
  key: gun_type_1
  name: "[未知]"
  necessary_development: 1
- id: 2
  key: gun_type_2
  name: "[未知]"
  necessary_development: 2000
- id: 3
  key: gun_type_3
  name: "[未知]"
  necessary_development: 3500
- id: 4
  key: gun_type_4
  name: "[未知]"
  necessary_development: 5000
- id: 5
  key: gun_type_5
  name: "[未知]"
  necessary_development: 7000
```

```yaml
# config/ships/shells.yaml
schema: dk4.ships.shells
version: 1
max_in_flight: 100
# 飞行实例，不是 100 种弹药
```

```yaml
# config/ships/supplies.yaml
schema: dk4.ships.supplies
version: 1
- id: 0
  key: water
  name: 水
  unit: 桶
- id: 1
  key: food
  name: 粮食
  unit: 桶
```

## 7. 独立旋钮

- 改装件行数、装甲加算、加价表
- 桅种类互斥规则
- 炮种解锁发展度
- 修理 / 改造天数
- 补给种类（原版 2；可加弹药，航海来消耗）
- 船首像修正器（战斗 / 航海只读 modifier key）
- 飞行炮弹池大小

改炮种名字只动 `guns.yaml` + 文案包。改「7000 发展度才能买 5 号炮」只动 `necessary_development`。

## 8. 对外接口

事件：

- `RemodelQuoted { ship_instance_id, gold, days }`
- `RemodelCommitted { ship_instance_id, gold, days }`
- `Repaired { ship_instance_id, gold, days }`
- `GunTypeChanged { ship_instance_id, gun_type_id }`
- `FigureheadChanged { ship_instance_id, figurehead_id }`
- `FigureheadRefused { ship_instance_id, figurehead_id, person_id }`
- `Supplied { fleet_id, water, food, gold }`

查询：

- `Yard.quoteRemodel(instance_id, preview) -> { gold, days }`
- `Yard.quoteRepair(instance_id) -> { gold, days }`
- `Yard.gunTypesAvailable(town_id) -> [gun_type_id]`
- `Yard.figureheadModifiers(figurehead_id) -> ModifierSet`
- `Supplies.of(instance_id) -> { water, food }`

命令：

- `Yard.commitRemodel(instance_id, preview, helper_person_id?)`
- `Yard.repair(instance_id, helper_person_id?)`
- `Yard.setGunType(instance_id, gun_type_id)`
- `Yard.setFigurehead(instance_id, figurehead_id)`
- `Yard.supply(fleet_id, water, food)`
- `Yard.setSupplyRate(fleet_id, water_rate, food_rate)`

付钱走经济 `Gold.debit`，本系统不算市价。发展度走 `Towns.develop(town_id)`。

## 9. 未核实字段

- `NUM_FIGUREHEAD`、`NUM_SHIPSUPPLY` 整数
- `eAddParts` 0–8 与文案清单的一一对应（仅 2、3 较像装甲）
- 桅循环 3 根 vs `NUM_SHIPMAST=100` 目录怎么索引
- 三角帆 / 四角帆互斥的数据结构（`MASTSTATE_BITS`）
- 帆、船首像、装甲的**速度 / 耐久以外**数值
- 改造 / 修理的人物折扣公式
- 买船准备天数按什么表
- 修理底价
- `fn_140013593` 货格差价完整式
- 6 个炮种名称、弹种、威力、装填
- `CBattleGun+0x8/+0xA` 含义
- `CShell` 各 dword（位置 / 速度候选）
- 发展度读的是商业还是军事
- 拒驾船首像的条件（主角？特定像？）
- 补给是否含弹药；`SUPPLYAMOUNT_MIN/MAX` 十进制
- `anSupply` 两个 int 的顺序（水、粮谁先）
- `_bTakeAwayFigureHead` 的玩法含义
