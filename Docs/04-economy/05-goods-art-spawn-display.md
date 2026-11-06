# 交易品：图、出现、等级、星品、显示

本篇管**玩家在港口交易所里看见的货**：图标、何时出现在买货栏、舱位左列写「N等级」还是「★」。目录「是什么」见 `01-trading-items.md`，金币报价见 `02-market-price.md`，买卖会话见 `03-trade-loop.md`，特产品供给/投资/品牌判定见 `04-special-products-investment.md`。玩家舰队 ↔ NPC 舰队交易是另一篇（`06-npc-fleet-trade.md`）。

禁止把 `eItem`（231 件宝物/发现物，图标 `Item32.dk4` / `Item56.dk4`）和 `eTradingItem`（105 件货，图标 `TItem24.dk4` / `TItem48.dk4`）当成同一套 ID。

## 1. 职责边界

本系统管：

- 105 件交易品的**图标集 / 图标下标**（与 `trading_item_id` 对齐）
- 某港买货栏**何时出现**某件货（特产品槽 ∩ 发现位 ∩ 发展门槛）
- 运行时等级 `TRADINGITEMGRADE`（1 最好 … 5 最差，6 = 未上架）及舱位/买货栏怎么画
- **星品**：年初从 1 级品牌里抽出的那一件，舱位画 `★`（不是 mesfile 里的「品牌商品」文案本身）
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

### 2.2 目录二进制（exe `.data`，不是 World DLL）

`Game/DK4HD_sc.exe` 文件偏移 `0x686b90`（VA `0x140688190`，落在 `.data` 段 `0x140682000`–`0x140694400`）起 105 条、步长 16：

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

`[确认]` 反汇编如上。`WORD(rec+2)` 即旧 JSON `supply_or_price`（100–6000），规范名 `listing_threshold`（`04` §2.1，`_data` 已改）。它只当商业门槛用，没有一处当价格读；槽底价另在城镇记录 `+0x62+slot*6`。bit0 是待种锁（`04` §2.9）。

买一件还要供给 `fn_140010cda > 0`（`BuyOne`）。等级函数 `fn_140015168` 在 `0101e5==0` 时返回 **6**，买货栏/卖价路径 `>=6` 跳过。

`CPriceInformationView` 另有一条「物价情报」：扫 105 件，只要求已发现且 `kind_id` 对上。那不是这座城的买货栏。

### 2.4 等级：1 最好，不是 `supply_base` 5–25（旧 JSON `grade`）

`fn_140015168`（`0x140261c00`）：

```
if not fn_1400101e5(town, slot): return 6
diff = commerce_develop - WORD(rec+2)     # 已保证 diff >= 0
step = item.grade_step                    # +0x12；<=0 则返回 5
rank = 6 - (diff / step)
return clamp(rank, 1, 5)                  # fn_1400103d4(1, rank, 5)
```

`[确认]`。`BuyOne` 把返回值写进货格 `hold+1`（`fn_140014d80` 的第三参）。拷贝 `110b8` 带 `+1`，所以船上记住的是**买入当时**的等级。

槽字段 `supply_base`（旧 JSON 第三列 `grade`）观察 5–25（伦敦 12/15/25）**不是**这套 1–5，也不是 mesfile「１级品牌」。`04` 已写；`13-algorithms/02-caps-ranges.md` 把 12–25 填进 `TRADINGITEMGRADE` 是样本误植，以本篇公式为准。

品牌（`04`）：`share>=100` 且 `grade_rank==1`。投资不直接改等级，只把发展度抬过门槛、再抬 `5*step` 才能到 1 级。

### 2.5 星品是年初抽中的那一件 1 级品牌，显示为 ★

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

`CTradingItemElement::vfunc_04`（body `0x1402912c0`）年初结算。世界 tick 只在 `fn_140014dd0` 为真时调它：`SCalendar+0x1c==0` 且 `month % 12 == 0`，即**每年 0 月月初第一拍** `[确认]` `0x1402aaf1c`。

1. 扫 97 港（上界 `0x61` 写死），**玩家势力**份额 `fn_1400088f0 >= 100` 的港；5 槽里 `item != 0x6A` 且 `015168==1` 的，去重进候选
2. 有候选：`rand(n)` 抽一件；写入 `element+0x1a48`；与旧值相同消息 `0x84`（「继续得到了市场的认可」），否则 `0x82`（「得到了市场的认可」）
3. 无候选：**仅当旧值 != `0x6A`** 才发 `0x83`（「没有得到市场认可的品牌商品」）；然后写 `0x6A`。连续两年没有不会重复播报

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

`[确认]` `fn_140009ae8` 每月初对 97 港都跑（不论有无投资，见 `07` §4.4），条件与上架谓词同形：`item!=0x6A`、未发现、`WORD+2 <= 商业`、运行时 bit0 未置。所以「解锁」= 任何一座港的商业发展度够了，该货**全局**变为已发现，之后所有有此槽且达门槛的港同时上架。

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

种植 `[确认]`：交易所入口 `fn_1400085d0`（不是宫殿）对玩家持有的发现物逐件找「本港某槽的货的目录 +11 == 该发现物、该槽待种锁 bit0 置位、商业 ≥ 门槛」的组合，随机挑一个问「如果有那%s，%s会成为这个城市的特产品」（代码消息 `0x374`；与 mesfile `0x7737` 的对应 `[推断]`）；同意则置发现位、清该槽待种锁、发现物交出。细节与接口归 `04` §2.9（命令 `PlantSpecialProduct`）。一件发现物可对应多件货（蜜蜂窝 → 蜂蜜、蜡），查询 `Economy.plant_items_of(item)` 返回列表。

注意两条解锁路径互斥：发展度解锁（`fn_140009ae8`）**跳过**有待种锁的槽，种植路径**只看**有待种锁的槽。

投资：不读本篇表。投资抬发展度 → `009ae8` / `0101e5` / `015168` 自然变化。

文化：`CCulture` 每块 105 个 word 是卖出底价（`02` §2.6），**不上** `0101e5`。流行 2 格影响报价与份额（`02` / `03`）。都不是买货栏出现条件。

### 2.8 到货周期（种类 → 四句文案）

`fn_14000fc13` 按 `kind_id` 0–18 跳表，`fn_140013359`（RTC `as_szArrivalDayName`）取字符串：

| 返回 | 文案 VA | 种类 |
| ---: | --- | --- |
| 0 | `0x1405a9970` 每月到货 | 0 食品、1 调味料、5 酒类、8 纤维、17 矿石 |
| 1 | `0x1405a9980` 奇数月到货 | 2 珍品、3 香辣料、4 嗜好品、11 香料、14 药 |
| 2 | `0x1405a9990` 偶数月到货 | 7 工艺品、9 纺织品、12 装饰品、15 染料、16 武器、18 杂货 |
| 3 | `0x1405a99a0` ３、６、９、１２月到货 | 6 贵金属、10 奢侈品、13 宝石 |

`kind>18` 返回 4（空指针）。

**补货已钉** `[确认]`（规格归 `04` §2.5 / §4.4，这里只留与文案的对照）：`CTownElement::vfunc_04` 月初第一拍（`fn_14000e9da`）建一张 19 格种类掩码，然后对每座活港先 `fn_140009ae8`（投资转化）再 `fn_140010d7a(town, mask)`（`0x140267781`–`0x1402679f8`）：

```
m = month + 1                         # 1-based；SCalendar+0xc 是 0–11
mask = {0,1,5,8,17}                   # 每月
mask |= m odd ? {2,3,4,11,14} : {7,9,12,15,16,18}
if m % 3 == 0: mask |= {6,10,13}
for slot in 0..4:                     # fn_140010d7a
    item = fn_140009101(town, slot, 1)  # 第三参 1：未上架（未发现 / 未达门槛 / 待种锁）返回 0x6A
    if item != 0x6A and mask[kind(item)]:
        fn_140008aee(town, slot)      # RTC nSupply
fn_140008aee:
    supply = base * (commerce/50 + 100) / 100      # fn_140012da0，两次有符号 idiv
    CTown+0x30+slot*6 = (u8)supply                 # 截低字节，不夹
    CTown+0x31+slot*6 = 0                          # month_bought 清零（玩家势力本月已购，04 §2.1）
```

`base` = 静态城镇记录定义段 `+0x28+slot*6+4`（`fn_140012da0` 写成 `rec+0x2c+slot*6`）的字节 = 槽字段 `supply_base`（5–25；旧 JSON 第三列 `grade`）`[确认]`。补货是**重置**为公式值，不是累加；非到货月不动。原版上限 25×(9999/50+100)/100 = 74，`u8` 放得下。

到货月谓词另有一份 `fn_140004eee(elem, kind)`（RTC `nTime,nDay,nMonth`，同样 `month+1`），唯一调用者是黄金航路奖金 `fn_1400108fc`，与补货不共用代码。`restock_calendar` 只有一份，归 `04`（`config/economy/restock_calendar.yaml`）；本篇的四句文案与黄金航路都按 key 引用它。

## 3. 实体与 ID

| 实体 | 键 | 范围 | 空 |
| --- | --- | --- | --- |
| 交易品 | `trading_item` key | 原版包 105 件 | `null`（原版 `106`，只在 `legacy_codec`） |
| 图标下标 | 同 `trading_item_id` | 与 `TItem24/48` 帧数同长 | 无图 |
| 特产品槽 | `(town_id, slot 0–4)` | 每港 5 | `item = null` |
| 发现位 | `trading_item_id` | 0/1 | 0 = 未发现 |
| 等级 | `trading_item_grade` | 观察 1–5；6 = 未上架 | 6 不进买货栏 |
| 星品 | 全局一个 `trading_item_id` | 年初写入 | `null` |
| 发现物外键 | `unlock_item`（`eItem` key） | 原版 20 件 | `null`（原版哨兵 232） |

货格：`hold+0` 货，`hold+1` 买入时等级，`hold+4` 买入价（0 = 战利品）。

## 4. 算法 / 公式

参数进 `config/formulas/economy.yaml` 的 `grade` / `star` 段。代码禁止写死 1、5、6、100、`0x6A`；空值一律 `null`。

### 4.1 发现

```
discovered(item) = state.discovered[item]             # 存档按 key 存；开局取 01 的 initially_discovered
```

置位：发展跨门槛（`fn_140009ae8` → `fn_14000513c`）或种植（`04` §2.9）。只加不减（年初不清发现位）。

### 4.2 买货栏上架 `market_visible`

```
market_visible(town_id, slot) =
    item = special_item(town_id, slot)
    and item != empty
    and discovered(item)
    and slot_threshold(town_id, slot) <= commerce_develop(town_id)   # vanilla WORD+2 vs +0x1e
    and not plant_lock(town_id, slot)                                # vanilla bit0 @ CTown+0x2c+slot*6，04 §2.9
```

这是 `fn_1400101e5` / `grade_rank!=unavailable`。画 5 张特产品卡用它。`BuyOne` 另外要求会话卡可买数 ≥1（`04` `Economy.buy_cap`），0 供给仍可能占着槽位。

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

`star_item` 年初从 `Economy.list_brands(force)` 随机抽一件；空则 `null`。

### 4.5 到货文案

```
arrival_label(kind) = restock_calendar.period_of(kind).label_key   # 04 拥有的表；未列种类 null
```

不在本函数里加减供给。

## 5. 耦合分析

原版：

- 图标文件名、帧下标、`eTradingItem` 焊在一起
- 上架读城镇 6 字节槽 + 发展度 + 交易品对象 `+0x16`
- 等级公式读同一槽 `WORD+2` 和目录步长
- 星品字节挂在 `CTradingItemElement+0x1a48`，年初扫份额（factions）
- 发现物 0–19 与交易品 +11 交叉；种植对话在交易所入口（`fn_1400085d0`）

重构：

```
config/economy/trading_items.yaml     # 图、步长、发现初值、植物外键、spawn_rules
config/economy/special_products.yaml  # 04：每港 5 槽 item / 门槛列 / 供给
config/formulas/economy.yaml          # grade_* / star
config/items/items.yaml               # unlock_item 指向那里，本篇不拥有 eItem
config/ui/strings.*.yaml              # %s等级、★、战利品、到货四句
```

World1 换包只换槽里的 `item_id` 与门槛，不换 1–5 公式、不换 105 帧图集的下标规则。

## 6. 解耦后的配置

```yaml
# config/economy/trading_items.yaml  （01 拥有该表；本篇只在同一行追加 icon / spawn 字段）
schema: dk4.economy.trading_items
version: 1
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
  unavailable: 6              # 原版返回值；运行时也可用 null 表示未上架
  format_key: ui.trade.grade          # "{rank}等级"
star_display:
  glyph_key: ui.trade.star            # ★
  loot_key: ui.trade.loot             # 战利品
  dash_key: ui.trade.dash             # —
  requires_grade_best: true
  exclude_loot: true
# 到货周期表不在这里：见 04 的 restock_calendar.yaml（唯一一份）
items:
  - key: vanilla:wheat                # 01 的字段（vanilla_index / kind / data_price / grade_step / initially_discovered / unlock_item）略
    icon_set: titem48                 # 大图；小图同 index 取 titem24
    icon_index: 0                     # 原版包 == vanilla_index；校验器拒绝错位
    spawn_rules:
      - when: special_slot
      - when: development_gte
        params: { source: slot_threshold }   # 原版 WORD+2 = listing_threshold
  - key: vanilla:pumpkin              # 01：unlock_item: vanilla:pumpkin_seed（eItem 0）
    icon_set: titem48
    icon_index: 7
    spawn_rules:
      - when: special_slot
      - when: development_gte
        params: { source: slot_threshold }
      - when: planted                 # 待种锁的槽要先种植（04 §2.9）
  - key: vanilla:honey                # 01：initially_discovered: false；unlock_item: vanilla:beehive（eItem 3）
    icon_set: titem48
    icon_index: 12
    spawn_rules:
      - when: special_slot
      - when: development_gte
        params: { source: slot_threshold }
      - when: planted
```

`when: always` 留给模组「不占特产品槽也上架」。原版买货栏没有这条；物价情报是 `discovered && kind==选中`，不要用 `always` 冒充原版市场。发现物 key 名以 `09-items` 为准。

```yaml
# config/formulas/economy.yaml  （本篇段）
grade:                         # grade_rank，fn_140015168（§4.3）；不可见 / step<=0 分支在 impl
  impl: vanilla.grade_rank
  params:
    best: 1
    worst: 5
    unavailable: 6
  # rank = clamp(unavailable - (develop - threshold)/step, best, worst)
star:
  impl: vanilla.star
  schedule: year_start         # 订阅 YearStarted；原版 fn_140014dd0 = 0 月月初第一拍
  params:
    pick: random_from_brands   # 04 Economy.list_brands（玩家势力 share >= 100 的港）
    persist: one_item
    announce_none_only_on_change: true   # 旧值非空才发 0x83
```

`restock_calendar` 不只是文案：它同时是 `04` 月初补货的种类掩码与黄金航路奖金的到货判定（§2.8）。表与补货公式都归 `04`；本篇只按 `label_key` 取文案。

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

`special_products.yaml` 由 `04` 拥有槽位 `trading_item` / `listing_threshold`（门槛）/ `supply_base` / `data_price`（槽底价）/ 供给。本篇不复制 97×5。

## 7. 独立旋钮

- 换 `TItem24/48`、加帧：只改 `icon_sets` + 表长；`icon_index` 仍等于该行 `id` 除非模组显式重映射
- `grade_step`、`grade_scale.best/worst`：改 1 级有多难，不改报价公式
- `initially_discovered` / `unlock_item`（`01`）：改开局锁与发现物，不改种植对白
- `restock_calendar`（`04` 拥有）：改到货周期——同时改文案、补货掩码、黄金航路判定（原版三处共用一张语义表）
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
Economy.star_item() -> trading_item_id | null     # null 若今年无品牌
Economy.arrival_label(kind_id) -> string_key | null   # 读 04 的 restock_calendar
Economy.discovered(trading_item_id) -> bool
Economy.set_discovered(trading_item_id, true)     # 只接受 true；发展转化 / 种植回调
Economy.plant_items_of(item_id) -> [trading_item_id]   # 发现物 → 可种的货（一对多）
```

只传 ID 与整数。不传 `CTradingItem*`、G1T 句柄、`CTown*`。

事件：

```
StarItemChanged { old_id, new_id }               # 年初；new 可为 null
TradingItemDiscovered { item_id }
```

`BrandRecognized` 仍由 `04` 在扫描命中时发。本篇年初抽星品在同一扫描之后，UI 订 `StarItemChanged` 刷新 ★。

查询：`Share.of`、`Towns.develop(town_id, commerce)`。发现物是否在袋由 `04` 种植流程向 items 查询；本篇不读背包。

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| ~~JSON 第三列 `grade` 5–25~~ | 已钉：基础供给 `supply_base`，`fn_140012da0` 读 `rec+4`（§2.8） |
| ~~`WORD(rec+2)` 是否同时是 quote 底价~~ | 已钉：只当门槛；槽底价是 `CTown+0x2e`（初值段 +0x62），`04` §2.1 |
| 运行时 `town+slot*6+0x2c` 的 bit0 | 已钉为待种锁：开局初值置、种植清（`04` §2.9）；运行时其它置位点未扫 |
| `TRADINGITEMGRADE_MIN/MAX` 头文件整数 | 只有符号；观察返回值 1–6 |
| ~~`hold+2`~~ | 已钉：买入港 `town_id`，清空写 `0x62`（`03` §2.2） |
| 消息 `0x82/0x83/0x84` 与 mesfile 块的精确切片 | 三句都在 `0xfa7`/`0x1027` 连写里，按年初分支选用 `[推断]` |
| ~~发现物交给城市：是置 `discovered` 还是改某港槽 `item_id`~~ | 已钉：置发现位 + 清该槽待种锁，不改 `item_id`（`04` §2.9） |
| 物价情报列表是否显示未上架但已发现的货 | 扫 105 件只看发现 + 种类，是 |
| ~~`CCulture` 105 word 是否间接改门槛~~ | 已钉：是卖出底价（`02` §2.6），不上 `0101e5` |
| World1 目录 16 字节表是否同一份 | 未对照 `DK4HD` 其它语言以外的 World1 |
| `TradeWin.dk4` 5 帧各自用途 | 构造里下标 1/2/3/4，铬；不进 `icon_index` |
| G1T 压缩格式 | 每帧 payload 等长（TItem24=1180，TItem48=4636），导入器再解 |
