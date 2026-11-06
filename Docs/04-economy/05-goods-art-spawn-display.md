# 交易品：图、出现、等级、星品、显示

本篇管**玩家在港口交易所里看见的货**：图标、何时出现在买货栏、舱位左列写「N等级」还是「★」。目录「是什么」见 `01-trading-items.md`，金币报价见 `02-market-price.md`，买卖会话见 `03-trade-loop.md`，特产品供给/投资/品牌判定见 `04-special-products-investment.md`。玩家舰队 ↔ NPC 舰队交易是另一篇（`06-npc-fleet-trade.md`）。

禁止把 `eItem`（231 件宝物/发现物，图标 `Item32.dk4` / `Item56.dk4`）和 `eTradingItem`（105 件货，图标 `TItem24.dk4` / `TItem48.dk4`）当成同一套 ID。

## 1. 职责边界

本系统管：

- 105 件交易品的**图标集 / 图标下标**（与 `trading_item_id` 对齐）
- 某港买货栏**何时出现**某件货（特产品槽 ∩ 发现位 ∩ 发展门槛）
- 运行时等级 `TRADINGITEMGRADE`（1 最好 … 5 最差，6 = 未上架）及舱位/买货栏怎么画
- **星品**：年结从 1 级品牌里抽出的那一件，舱位画 `★`（不是 mesfile 里的「品牌商品」文案本身）
- 到货周期文案（每月 / 奇偶月 / 季月）作为买货栏旁的只读投影

不管：

- 目录短名、种类归属、百科段落（`01`）
- `quote()` 金币、种类市价指数（`02`）
- 货舱堆叠、确认/取消、流行品卖出改份额（`03`）
- 供给加减、投资扣款、月账、`is_brand` 的份额门槛（`04`；本篇只提供 `grade_rank`）
- NPC 舰队买货折扣（`06`）
- 列宽、`%6d` 放得下几个字、按钮皮肤（`12-ui-save`）
- 发现物 `eItem` 的效果与背包（`09-items`）；本篇只读「哪件发现物能解锁哪件货」的外键
- 文化 105 格、流行 2 格（`02-factions`；**不是**上架条件）

## 2. 原版事实

### 2.1 两套图，不要混

| 文件 | G1T 魔数 | `tex_count` | 文件头尺寸字段 | 用途 |
| --- | ---: | ---: | --- | --- |
| `Game/TItem24.dk4` | `GT1G4600` | **105** (`0x69`) | 48×48 | 交易品小图 |
| `Game/TItem48.dk4` | `GT1G4600` | **105** | 96×96 | 交易品大图（舱位/特产品卡） |
| `Game/Item32.dk4` | `GT1G0600` | **231** (`0xe7`) | （另一套） | `eItem`，本篇禁用 |
| `Game/Item56.dk4` | `GT1G4600` | **231** | 112×112 | `eItem`，本篇禁用 |
| `Game/TradeWin.dk4` | `GT1G4600` | **5** | 窗框若干 | 交易所铬，不是货 |

`[确认]` 头 `uint32 tex_count` 在文件偏移 `0x10`；`TItem*` 的 105 与 `NUM_TRADINGITEM` 一致，`Item*` 的 231 与 `NUM_ITEM` 一致。文件名 24/48 是原版像素；HD 贴图边长×2。加载：`fn_14000b46a` → `TItem24.dk4`，尺寸立即数 `DAT_1405a9a18 = (48,48)`；`fn_140005560` → `TItem48.dk4`，`DAT_1405a9a10 = (96,96)`。`fn_140013a48` 第三参把 **`trading_item_id` 当贴图下标** 送进 `fn_140010e6f(..., "TItem48.dk4", id)`。

因此原版包：`icon_index == trading_item_id`，合法 0–104。没有第二套重排表。

### 2.2 目录二进制（exe `.rdata`，不是 World DLL）

`Game/DK4HD_sc.exe` 文件偏移 `0x686b90`（VA `0x140688190`）起 105 条、步长 16：

| 偏移 | 宽 | 观察 | 标记 |
| --- | --- | --- | --- |
| +0 | `QWORD` | 名称指针（「小麦」…「鸟粪石」） | [确认] `01` / `15-catalogs/trading-items.md` |
| +8 | `u16` | 目录底价 1000–9500 | [确认] `fn_140007022` 读对象 `+0x10` |
| +10 | `u8` | `kind_id` 0–18 | [确认] 与百科 19 组一致 |
| +11 | `u8` | 发现物 `eItem` 0–19，或 `0xE8=232` 无 | [确认] 值域；语义见 2.6 |
| +12 | `u16` | 等级步长 300–1900 | [确认] `fn_140005da8` 读对象 `+0x12` |
| +14 | `u8` | 开局已发现 0/1（12 件为 0） | [确认] `fn_14000f9e3` 读对象 `+0x16` |
| +15 | `u8` | 全 0 | [确认] 未用 |

运行时 `CTradingItem` 步长 `0x40`，`CTradingItemElement+8+id*0x40`（`fn_140006884`）。数组后 `+0x1a48` 是**当前星品**的 `eTradingItem` 字节（空 = `0x6A`）。`fields.tsv`：`CTradingItemElement 0x1a48×5`。

### 2.3 买货栏 = 每港 5 个特产品槽，不是 105 件全上架

`CTradingView` 构造按 `eSpecialProduct` 0–4 建控件，资源 `TradeWin.dk4`。`CTrade::UnpackFromShip`（`0x140279b80`）只把**船舱**拷进会话槽，不填市场。

上架谓词 `fn_1400101e5`（`0x1402649e0`）：

```
rec = GetFixTown.special[slot]          # 6 字节，城镇 +0x28 + slot*6
item = BYTE(rec+0)
if item == 0x6A: return 0
if not discovered(item): return 0       # fn_14000f9e3：对象 +0x16 > 0
if WORD(rec+2) > town.commerce_develop: return 0   # 发展门槛；town+0x1e 轨 0
if bit0(runtime_slot_byte): return 0    # town+slot*6+0x2c，edx=0 → mask=1
return 1
```

`[确认]` 反汇编如上。`WORD(rec+2)` 即 JSON `supply_or_price`（100–6000），`04` 按 `TRADINGITEM_DATAPRICE` 解释同一列。本篇出现条件把它和商业发展度（夹到 9999）比较：门槛未到则不上架。

买一件还要供给 `fn_140010cda > 0`（`BuyOne`）。等级函数 `fn_140015168` 在 `0101e5==0` 时返回 **6**，买货栏/卖价路径 `>=6` 跳过。

`CPriceInformationView` 另有一条「物价情报」：扫 105 件，只要求已发现且 `kind_id` 对上。那不是这座城的买货栏。

### 2.4 等级：1 最好，不是 JSON `grade` 5–25

`fn_140015168`（`0x140261c00`）：

```
if not fn_1400101e5(town, slot): return 6
diff = commerce_develop - WORD(rec+2)     # 已保证 diff >= 0
step = item.grade_step                    # +0x12；<=0 则返回 5
rank = 6 - (diff / step)
return clamp(rank, 1, 5)                  # fn_1400103d4(1, rank, 5)
```

`[确认]`。`BuyOne` 把返回值写进货格 `hold+1`（`fn_140014d80` 的第三参）。拷贝 `110b8` 带 `+1`，所以船上记住的是**买入当时**的等级。

JSON 第三列 `grade` 观察 5–25（伦敦 12/15/25）**不是**这套 1–5，也不是 mesfile「１级品牌」。`04` 已写；`13-algorithms/02-caps-ranges.md` 把 12–25 填进 `TRADINGITEMGRADE` 是样本误植，以本篇公式为准。

品牌（`04`）：`share>=100` 且 `grade_rank==1`。投资不直接改等级，只把发展度抬过门槛、再抬 `5*step` 才能到 1 级。

### 2.5 星品是年结抽中的那一件 1 级品牌，显示为 ★

mesfile **没有**「星品」一词。exe `.rdata` 紧挨交易所按钮：

```
0x1405a4d70  %s等级
0x1405a4d78  战利品
0x1405a4d80  —
0x1405a4d84  ★
0x1405a4d88  %6d
0x1405a9918  进价表示
0x1405a9928  等级表示
```

`CTradingItemElement::vfunc_04` 年结：

1. 扫 97 港，`share>=100` 且 `015168==1` 的特产品，去重进候选
2. 有候选：随机抽一件；写入 `element+0x1a48`；已是同一件则消息 `0x84`（「继续得到了市场的认可」），否则 `0x82`（「得到了市场的认可」）
3. 无候选：消息 `0x83`（「没有得到市场认可的品牌商品」），写 `0x6A`

舱位 `★` 谓词 `fn_14000bf00`：

```
item(slot) == star_item()     # fn_14022fa20 / fn_14028c220：读 +0x1a48
and hold+1 == 1
and buy_price != 0            # 战利品排除
```

战利品是另一态：`fn_140003cf6` = `buy_price==0`。进价列画「战利品」；等级列仍走 ★ / `%s等级`，战利品**不会**变成 ★。

### 2.6 舱位左列（`CTrade::DrawCargoList` `0x140285130`）

RTC 名：`rcGrade` / `posGrade`（左）、`rcPrice` / `posPrice`（右）。`CTrade+0x590` 由按钮 `0x15` 切换（文案「进价表示」/「等级表示」）。

空格 `fn_1400087d8==0`：左右都画 `—`。

有货、且 `just_bought==0`（`fn_1400110c7` 读槽 `+8`）且 `+0x590==1`（进价模式）：橙色 `0xffffc080`；战利品画「战利品」，否则 `%6d` 买入价。

否则（等级模式，或本笔刚买）：底色灰 `0xffeaeaea`，刚买改红 `0xffff8080`；`00bf00` 则 `★`，否则 `wsprintf("%s等级", itoa(hold+1))`。

卖出列表同三串（`0x14028917d` 起）。整理舱/捕获窗 `0x1401e9809` 用同一 `00bf00`，字形取 `0x140586758` 那颗独立的 `★`（紧挨 `TItem48.dk4` 文件名）。

特产品买货卡 `0x140281438`：缓存等级 `==1` 且 `item==star_item` 画 `★`，否则 `%s等级`。

### 2.7 发现物 / 发展度 / 投资 / 文化

开局 `+0x16=0` 的 12 件（目录 +14 = 0）：蜂蜜、鱼子酱、鱼翅、罗望子、白金、黄金艺品、麝香、龙涎香、金刚石、冬虫夏草、山竹果、鸟粪石。发展度跨过该槽 `WORD+2` 时 `fn_140009ae8` 调 `fn_14000513c` 置发现位。

目录 +11 ≠ `0xE8` 时是 `eItem` 0–19（`09-items` 种类 `plant_discovery`，恰好 20 件）。对照：

| eItem | 发现物 | 解锁的 eTradingItem |
| ---: | --- | --- |
| 0 | 南瓜种子 | 7 南瓜 |
| 1 | 番茄苗 | 9 番茄 |
| 2 | 香蕉树 | 10 香蕉 |
| 3 | 蜜蜂窝 | 12 蜂蜜、102 蜡 |
| 4 | 小鲨鱼 | 20 鱼子酱、21 鱼翅 |
| 5–12 | 胡椒粒…烟草苗 | 22、23、25、26、31–34 |
| 13 | 炼金术书 | 41 白金 |
| 14 | 玻璃的制作辞典 | 42 玻璃 |
| 15 | 蚕 | 49 丝绸 |
| 16 | 织布机 | 51–54 四种织品 |
| 17 | 珍珠贝 | 73 珍珠 |
| 18 | 中医宝典 | 83 冬虫夏草 |
| 19 | 山竹果种子 | 89 山竹果 |

宫殿文案「如果有那%s，%s会成为这个城市的特产品」（mesfile `0x7737`）。写发现位的具体命令在 `09-items` / 叙事；经济只收「此 `trading_item_id` 已发现」。

投资：不读本篇表。投资抬发展度 → `009ae8` / `0101e5` / `015168` 自然变化。

文化：`CCulture` 每块 105 个 word，**不上** `0101e5`。流行 2 格只改卖出份额（`03`）。都不是买货栏出现条件。

### 2.8 到货周期（种类 → 四句文案）

`fn_14000fc13` 按 `kind_id` 0–18 跳表，`fn_140013359`（RTC `as_szArrivalDayName`）取字符串：

| 返回 | 文案 VA | 种类 |
| ---: | --- | --- |
| 0 | `0x1405a9970` 每月到货 | 0 食品、1 调味料、5 酒类、8 纤维、17 矿石 |
| 1 | `0x1405a9980` 奇数月到货 | 2 珍品、3 香辣料、4 嗜好品、11 香料、14 药 |
| 2 | `0x1405a9990` 偶数月到货 | 7 工艺品、9 纺织品、12 装饰品、15 染料、16 武器、18 杂货 |
| 3 | `0x1405a99a0` ３、６、９、１２月到货 | 6 贵金属、10 奢侈品、13 宝石 |

`kind>18` 返回 4（空指针）。这是买货栏旁的说明；补货加减仍见 `04` 的 `restock_calendar: unverified`（日历 tick 未钉到供给函数）。

## 3. 实体与 ID

| 实体 | 键 | 范围 | 空 |
| --- | --- | --- | --- |
| 交易品 | `trading_item_id` | 原版包 0–104 | `106` |
| 图标下标 | 同 `trading_item_id` | 与 `TItem24/48` 帧数同长 | 无图 |
| 特产品槽 | `(town_id, slot 0–4)` | 每港 5 | `item=empty` |
| 发现位 | `trading_item_id` | 0/1 | 0 = 未发现 |
| 等级 | `trading_item_grade` | 观察 1–5；6 = 未上架 | 6 不进买货栏 |
| 星品 | 全局一个 `trading_item_id` | 年结写入 | `empty` |
| 发现物外键 | `item_id`（`eItem`） | 0–19 或哨兵 232 | 232 = 无植物/秘方 |

货格：`hold+0` 货，`hold+1` 买入时等级，`hold+4` 买入价（0 = 战利品）。

## 4. 算法 / 公式

参数进 `config/formulas/economy.yaml` 的 `grade` / `star` 段。代码禁止写死 1、5、6、100、`0x6A`。

### 4.1 发现

```
discovered(item_id) = catalog[item_id].discovered     # 存档覆盖开局 initial_discovered
```

置位：发展跨门槛（`fn_14000513c`）或发现物事件。只加不减（年结不清发现位）。

### 4.2 买货栏上架 `market_visible`

```
market_visible(town_id, slot) =
    item = special_item(town_id, slot)
    and item != empty
    and discovered(item)
    and slot_threshold(town_id, slot) <= commerce_develop(town_id)   # vanilla WORD+2 vs +0x1e
    and not slot_hidden_bit(town_id, slot)                           # vanilla bit0 @ +0x2c+slot*6
```

这是 `fn_1400101e5` / `grade_rank!=unavailable`。画 5 张特产品卡用它。`BuyOne` 另外要求 `supply>=1`（`04.can_buy`），0 供给仍可能占着槽位。

`spawn_rules` 是这组谓词的数据化，不是第二套 if。

### 4.3 等级 `grade_rank`

```
grade_rank(town_id, slot) =
    if not market_visible(town_id, slot):
        return grade_unavailable          # vanilla 6
    step = catalog[item].grade_step
    if step <= 0:
        return grade_worst                # vanilla 5
    q = (commerce_develop(town_id) - slot_threshold(town_id, slot)) / step
    return clamp(grade_unavailable - q, grade_best, grade_worst)   # 6-q → [1,5]
```

买入：`hold.grade = grade_rank(...)`。之后显示读货格，不回查港口。

### 4.4 左列 `cargo_left_text`

```
if hold.item == empty:
    return dash                           # —
if show_buy_price and not just_bought:
    return loot_label if hold.buy_price == 0 else format_gold(hold.buy_price)
if is_star_cargo(hold):
    return star_glyph                     # ★
return grade_format.format(rank=hold.grade)   # "{rank}等级"
```

```
is_star_cargo(hold) =
    hold.item == star_item
    and hold.grade == grade_best
    and hold.buy_price != 0
```

`star_item` 年结从 `04.list_brands(force)` 随机抽一件；空则 `empty`。

### 4.5 到货文案

```
arrival_label(kind_id) = restock_calendar[kind_id]   # 四选一；缺省 null
```

不在本函数里加减供给。

## 5. 耦合分析

原版：

- 图标文件名、帧下标、`eTradingItem` 焊在一起
- 上架读城镇 6 字节槽 + 发展度 + 交易品对象 `+0x16`
- 等级公式读同一槽 `WORD+2` 和目录步长
- 星品字节挂在 `CTradingItemElement+0x1a48`，年结扫份额（factions）
- 发现物 0–19 与交易品 +11 交叉；宫殿对话在叙事

重构：

```
config/economy/trading_items.yaml     # 图、步长、发现初值、植物外键、spawn_rules
config/economy/special_products.yaml  # 04：每港 5 槽 item / 门槛列 / 供给
config/formulas/economy.yaml          # grade_* / star
config/items/items.yaml               # plant_item_id 指向这里，本篇不拥有 eItem
config/ui/strings.*.yaml              # %s等级、★、战利品、到货四句
```

World1 换包只换槽里的 `item_id` 与门槛，不换 1–5 公式、不换 105 帧图集的下标规则。

## 6. 解耦后的配置

```yaml
# config/economy/trading_items.yaml  （在 01 的字段上追加）
schema: dk4.economy.trading_items
version: 1
empty_id: 106
icon_sets:
  titem24:
    resource: TItem24.dk4
    frame_count: 105          # 运行时读表长；原版包 105
    pixel: 48                 # HD 存图边长；逻辑名仍叫 24
  titem48:
    resource: TItem48.dk4
    frame_count: 105
    pixel: 96
grade_scale:
  best: 1
  worst: 5
  unavailable: 6
  format_key: ui.trade.grade          # "{rank}等级"
star_display:
  glyph_key: ui.trade.star            # ★
  loot_key: ui.trade.loot             # 战利品
  dash_key: ui.trade.dash             # —
  requires_grade_best: true
  exclude_loot: true
restock_calendar:                     # kind_id → 文案 key；原版包
  monthly: [0, 1, 5, 8, 17]
  odd_months: [2, 3, 4, 11, 14]
  even_months: [7, 9, 12, 15, 16, 18]
  quarterly_3_6_9_12: [6, 10, 13]
items:
  - id: 0
    key: wheat
    name: 小麦
    kind_id: 0
    data_price: 1100
    icon_set: titem48                 # 大图；小图同 index 取 titem24
    icon_index: 0                     # == id；校验器拒绝错位
    grade_step: 500
    initial_discovered: true
    plant_item_id: 232                # 无；caps.item_empty_slot
    spawn_rules:
      - when: special_slot
      - when: development_gte
        params: { source: slot_threshold }   # 原版 WORD+2
  - id: 7
    key: pumpkin
    name: 南瓜
    kind_id: 0
    data_price: 1300
    icon_set: titem48
    icon_index: 7
    grade_step: 500
    initial_discovered: true
    plant_item_id: 0                  # 南瓜种子
    spawn_rules:
      - when: special_slot
      - when: development_gte
        params: { source: slot_threshold }
      - when: event_unlock
        params: { plant_item_id: 0 }
  - id: 12
    key: honey
    name: 蜂蜜
    kind_id: 1
    data_price: 1800
    icon_set: titem48
    icon_index: 12
    grade_step: 1000
    initial_discovered: false         # 原版 12 件之一
    plant_item_id: 3                  # 蜜蜂窝
    spawn_rules:
      - when: special_slot
      - when: development_gte
        params: { source: slot_threshold }
      - when: event_unlock
        params: { plant_item_id: 3 }
```

`when: always` 留给模组「不占特产品槽也上架」。原版买货栏没有这条；物价情报是 `discovered && kind_id==选中`，不要用 `always` 冒充原版市场。

```yaml
# config/formulas/economy.yaml  （本篇段）
grade:
  best: 1
  worst: 5
  unavailable: 6
  # rank = clamp(unavailable - (develop - threshold)/step, best, worst)
star:
  pick: random_from_brands     # 04.list_brands
  persist: one_item
```

```yaml
# config/ui/strings.zh-CN.yaml  （本篇用到的 key，文本不进逻辑表）
ui.trade.grade: "{rank}等级"
ui.trade.star: "★"
ui.trade.loot: "战利品"
ui.trade.dash: "—"
ui.trade.mode_price: "进价表示"
ui.trade.mode_grade: "等级表示"
ui.trade.arrival.monthly: "每月到货"
ui.trade.arrival.odd_months: "奇数月到货"
ui.trade.arrival.even_months: "偶数月到货"
ui.trade.arrival.quarterly_3_6_9_12: "３、６、９、１２月到货"
```

`special_products.yaml` 仍由 `04` 拥有槽位 `item_id` / `data_price`（门槛列）/ 供给。本篇不复制 97×5。

## 7. 独立旋钮

- 换 `TItem24/48`、加帧：只改 `icon_sets` + 表长；`icon_index` 仍等于该行 `id` 除非模组显式重映射
- `grade_step`、`grade_scale.best/worst`：改 1 级有多难，不改报价公式
- `initial_discovered` / `plant_item_id`：改开局锁与发现物，不改宫殿剧本文本
- `restock_calendar`：改到货说明；真补货仍走 `04`
- `star_display.exclude_loot`：战利品是否允许 ★
- `when: always`：模组才能让非特产品进买货栏

改「伦敦槽里是煤」只动 `04` 内容包。改「★ 的字形」只动 strings。

## 8. 对外接口

```
IconRef = { set: titem24|titem48, index: int }

Economy.icon(trading_item_id, size) -> IconRef
Economy.market_list(town_id) -> [SpecialSlot]     # 已滤上架；SpecialSlot 见 04
Economy.grade_rank(town_id, slot) -> int          # 1–5 或 unavailable
Economy.cargo_left_text(hold, mode) -> string_key + args
Economy.star_item() -> trading_item_id            # empty 若今年无品牌
Economy.arrival_label(kind_id) -> string_key
Economy.discovered(trading_item_id) -> bool
Economy.set_discovered(trading_item_id, true)     # 只接受 true；发现物/发展回调
```

只传 ID 与整数。不传 `CTradingItem*`、G1T 句柄、`CTown*`。

事件：

```
StarItemChanged { old_id, new_id }               # 年结；new 可为 empty
TradingItemDiscovered { item_id }
```

`BrandRecognized` 仍由 `04` 在扫描命中时发。本篇年结抽星品在同一扫描之后，UI 订 `StarItemChanged` 刷新 ★。

查询：`Factions.share`、`World.town` 发展度、`Items.owned(plant_item_id)`（发现物是否在袋，本篇不读背包，由叙事/items 发 `set_discovered`）。

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| JSON 第三列 `grade` 5–25 | 不是 1–5 等级，不是品牌；可能是初始供给（`SPECIALPRODUCT_SUPPLY` 是 `UCHAR`） |
| `WORD(rec+2)` 是否同时是 quote 底价 | 出现条件当发展门槛 `[确认]`；`04` 当 `DATAPRICE` `[推断]`。未在 `quote` 里逐指令钉死同一加载 |
| 运行时 `town+slot*6+0x2c` 的 bit0 谁置位 | `0101e5` 为 1 则不上架；写入点未扫完 |
| `TRADINGITEMGRADE_MIN/MAX` 头文件整数 | 只有符号；观察返回值 1–6 |
| `hold+2` | `014d80` 写了；清空写 `0x62`（`03`）；与等级无关 |
| 消息 `0x82/0x83/0x84` 与 mesfile 块的精确切片 | 三句都在 `0xfa7`/`0x1027` 连写里，按年结分支选用 `[推断]` |
| 发现物交给城市：是置 `discovered` 还是改某港槽 `item_id` | 文案像「成为这个城市的特产品」；`009ae8` 只置发现位。槽内容是否被改写 `[未知]` |
| 物价情报列表是否显示未上架但已发现的货 | 扫 105 件只看发现 + 种类，是 |
| `CCulture` 105 word 是否间接改门槛 | 不上 `0101e5`；当价格/需求 `[推断]`，本篇当非出现条件 |
| World1 目录 16 字节表是否同一份 | 未对照 `DK4HD` 其它语言以外的 World1 |
| `TradeWin.dk4` 5 帧各自用途 | 构造里下标 1/2/3/4，铬；不进 `icon_index` |
| G1T 压缩格式 | 每帧 payload 等长（TItem24=1180，TItem48=4636），导入器再解 |
