# 特产品、投资与月账

每座港口最多 5 个特产品槽：世界表只说「这座城产哪件」，供给、等级、买卖消耗、投资花销、月账在经济系统。份额、合约、介绍信在势力；本系统只发事件。

## 1. 职责边界

本系统管：

- 每港 5 槽的运行时供给、可买数量、与目录 `item_id` 的绑定
- 特产品基础价（内容包从世界表带入）
- 投资扣款、记入月账 `investment`
- 品牌商品判定要用的**经济侧**条件（「１级」）
- 地方舰队 / 玩家商会月账：收入、支出、投资

不管：

- 港口坐标、名称、region（world）
- 市场份额、合约、上缴对象、介绍信 / 推荐信（factions；本系统 `emit`）
- 流行品清单（factions / town fashion；买卖件数由 `03` 上报）
- 交易所 UI、投资垫板控件（`CInvestmentPadView`、`CSpecialProductView` 是 View）
- 「上缴金额」对谁缴（文案在 mesfile，状态在势力）

拆分原则：world 存「哪座城默认产哪件」；economy 存「现在还能买几件、花了多少钱」。

## 2. 原版事实

### 2.1 每港 5 槽

`NUM_SPECIALPRODUCT = 5`，`INVALID_SPECIALPRODUCT = -1` `[确认]` `SpecialProductID.h`。

World0 城镇记录步长 `0x84`。抽出表每港 5 槽：

```
{ item_id: u16, supply_or_price: u16, grade: u16, empty: item_id==0x6A }
```

`[确认]` `Docs/_data/towns_world0.json`。空槽 `item_id=106`。97 港共 45 个空槽；**105 件交易品都至少在一座城当特产品出现**。

伦敦例：`(99,4500,15), (47,2000,25), (51,2000,25), (90,4000,12), (91,3500,12)` → 煤 / 羊毛 / 毛织品 / 洋枪 / 大炮。

`supply_or_price` 观察 100–6000，宽度对得上 `TRADINGITEM_DATAPRICE`（USHORT）`[推断]`。JSON 字段名未定死。

JSON `grade` 观察 5–25 `[未知]` 语义。它**不是** mesfile「１级品牌」：品牌判定读运行时 `fn_140015168(town, slot)==1`（`CTradingItemElement::vfunc_04`）。

运行时 `CTown` 存档对 5 槽还写 `+0x2c / +0x2e / +0x30 / +0x31`（`CTown.cpp` 循环 `s28>=5`）。与世界表 3 个 u16 如何对齐 `[未知]`。

`SPECIALPRODUCT_SUPPLY_MIN/MAX`：`UCHAR` `[确认]` `asserts.txt`。供给是 0–255 的运行时值，与世界表那个 100–6000 的字段不是同一列。

`SPECIALPRODUCTSTATE_BITS` 只有符号。

### 2.2 买卖消耗

`BuyOne`：该槽供给 `fn_140010cda > 0` 才能买；买后 `fn_14000833c` 一类函数减供给。退货 `RefundJustBought` 按 item 对槽，补供给。

`CommitDeal` 末尾再扫 5 槽，对照会话里记下的买入件数结算供给（`eSpecialProduct` RTC 局部）。公式抬码混乱，只确认「确认时还会再碰供给」`[确认]` 有循环，`[未知]` 精确加减。

NPC 自动交易 `fn_140010d25` 也会对特产品槽调 `fn_140005a6f`（加供给一类）和买货。

### 2.3 品牌商品

mesfile：

- 「今年我商会经营的１级%s作为品牌商品得到了市场的认可。」
- 「遗憾！今年没有得到市场认可的品牌商品。」
- 「发放品牌商品%s，向城市居民宣传」
- 「通过将品牌商品的%s奉献给城市的掌权人，交涉一下扩大市场份额」

代码：`CTradingItemElement::vfunc_04` 扫全部港：

```
if share(town) < 100: skip          # fn_1400088f0 < 0x64
for slot in 5:
    item = special_item(town, slot)
    if item == empty: skip
    if fn_140015168(town, slot) != 1: skip
    collect unique item             # 品牌候选
```

`[确认]` 品牌要 **份额 ≥100** 且 **槽等级函数 == 1**。份额本身属 factions；经济提供「该槽是不是 1 级」。

投资文案把「巨额商业投资 → 城市繁荣 → 1 级货评上品牌」连在一起。投资金额怎么改等级 / 发展度，函数未还原 `[未知]`。

### 2.4 投资与占有率（文案）

mesfile：

- 「没有占有率，交易也不能进行，要投资吗」
- 「要投资多少呢？」
- 「占有率我会稍微优待一点给你」
- 「本月的上缴金额是%s枚金币」
- 「%s以独占的城市得到上缴金额%s枚金币」
- 「%s的市场份额上涨了」

`CInvestmentPadView` 是投资垫板 UI（构造里一堆矩形立即数），不是公式。

份额增减原版出现在 `CommitDeal` 卖流行品、宫殿对话、投资成功等处。重构：经济扣金币并发 `InvestmentMade` / `TributeDue`；factions 改 `share`、合约。

### 2.5 月账

`exe_caps.md`：

```
ledger = mgr + 0x3738 + idx * 0x18
+0x00..0x0B  上月
+0x0C        支出 expense
+0x10        收入 income
+0x14        投资 investment
余额 = income - expense - investment
```

`CFleetElement::vfunc_01` 存档循环 `idx < 6` 写这 6 条 `[确认]` `fields.tsv` `0x3738×3`。6 是月份窗口还是 6 本账 `[未知]`。

地方舰队买货：`record.expense += cost`（`0x1400ea7f0`）；无账本则直接扣势力金币。月结 `CFleetElement::vfunc_04`（约 `0x140100180`）付不起的转入下月支出 `[确认]` `exe_caps.md`。

NPC 买货成本公式见 `02` `npc_buy_cost`（`ModifierSet.buy_cost_factor`，原版精神）。

### 2.6 到货周期（攻略，非代码）

中文攻略按种类写「每月 / 奇数月 / 偶数月 / 3·6·9·12 月」。本仓库没有对应立即数钉死在特产品函数上。当作内容包可选 `restock_calendar`，标 `[推断]`，实现前要再对日历 tick。

## 3. 实体与 ID

| 实体 | ID | 计数 | 空 |
| --- | --- | --- | --- |
| 特产品槽 | `(town_id, slot 0–4)` | 每港 5 | `item_id = empty` |
| 供给 | `UCHAR` | 0–255 原版包 | 0 不能买 |
| 品牌标记 | 槽上 `grade_rank==1` | — | 非 1 不是品牌 |
| 账本行 | `(force_or_fleet_id, book_index)` | 原版 6 行 × 0x18 | — |

`eSpecialProduct` 是槽下标，不是第二种商品目录。槽里的货物仍是 `eTradingItem`。

## 4. 算法 / 公式

### 4.1 能否买

```
can_buy(town_id, slot, qty) =
    item = special_item(town_id, slot)
    item != empty
    and supply(town_id, slot) >= qty
    and qty >= 1
    and qty <= cargo_stack     # 由 03 再夹货格
```

### 4.2 消耗 / 退回

```
consume(town_id, slot, qty):
    supply = clamp(supply - qty, supply_min, supply_max)

refund_supply(town_id, slot, qty):
    supply = clamp(supply + qty, supply_min, supply_max)
```

`supply_min/max` 来自 `SPECIALPRODUCT_SUPPLY_*`，原版包 0–255。

### 4.3 品牌

```
is_brand_product(town_id, slot) =
    special_item(town_id, slot) != empty
    and grade_rank(town_id, slot) == brand_grade_rank    # vanilla 1
```

列出某势力的品牌：

```
brands(force_id) = [
    item_id
    for town in towns_where(share(force_id, town) >= brand_share_need)  # vanilla 100，读 factions
    for slot in 0..4
    if is_brand_product(town, slot)
]
```

经济不算 share，向 factions `query_share(town_id, force_id)`。

### 4.4 投资

金额由玩家在垫板输入，夹到 `[invest_min, player_gold]`。原版优待份额的函数未恢复，故：

```
on_invest(town_id, force_id, gold):
    debit gold
    ledger.investment += gold
    emit InvestmentMade { town_id, force_id, gold }
```

factions 听事件改 share / 发展度。不要在经济里写死「投资 X 金币 → +Y 份额」。

### 4.5 月账

```
balance = income - expense - investment
month_roll:
    last_month = { income, expense, investment, balance }
    income = expense = investment = 0
    if unpaid > 0: expense += unpaid          # 原版付不起转入下月
```

NPC 买货：`expense += npc_buy_cost(...)`（公式在 `02`）。

上缴：

```
emit TributeDue { force_id, town_id, gold }   # 文案「本月的上缴金额」
```

谁收、收多少：factions / narrative。经济只记账、扣款。

### 4.6 补货（钩子）

```
# restock_calendar 未在 exe 钉死前保持 unverified
on_calendar(town_id, month):
    for slot in specials(town_id):
        if calendar_allows(kind_of(slot.item), month):
            supply = clamp(supply + restock_amount, min, max)
```

`restock_amount` `[未知]`。没有公式就不要假装每月 +N。

## 5. 耦合分析

原版：

- 世界 DLL 城镇 hex 里直接塞 5 槽 item/价/第三字段
- `CTrade` 确认时改供给、改份额、发信
- 账本挂在 `CFleetElement+0x3738`
- 品牌扫描读 share

重构：

```
world.towns.yaml          # town_id, special_slots[].item_id （定义）
economy.special_products  # 同 id 的 data_price 缺省、运行时 supply
factions.share            # 只被查询
fleets / forces           # 账本归属
```

World1 换包只换哪座城产什么，不换 5 槽逻辑。

## 6. 解耦后的配置

```yaml
# config/economy/special_products.yaml
schema: dk4.economy.special_products
version: 1
# 原版包从 World0 城镇表生成；world/towns.yaml 只留 item_id 外键
towns:
  - town_id: 0
    key: london
    slots:
      - slot: 0
        item_id: 99          # coal
        data_price: 4500
        field_b: 15          # JSON grade；语义未命名
      - slot: 1
        item_id: 47
        data_price: 2000
        field_b: 25
      - slot: 2
        item_id: 51
        data_price: 2000
        field_b: 25
      - slot: 3
        item_id: 90
        data_price: 4000
        field_b: 12
      - slot: 4
        item_id: 91
        data_price: 3500
        field_b: 12
```

```yaml
# config/economy/investment.yaml
schema: dk4.economy.investment
version: 1
brand_grade_rank: 1
brand_share_need: 100      # 查询 factions；不是经济自己存
ledger_slots: 6
ledger_stride: 24          # 0x18
```

```yaml
# config/formulas/economy.yaml  （本篇段）
supply:
  min: 0
  max: 255
invest:
  min: 1
  # 份额优待曲线：未还原
  share_bonus: null
restock_calendar: unverified
tribute:
  # 金额曲线未还原
  formula: null
```

```yaml
# config/world/towns.yaml  （对照，本系统不拥有）
- id: 0
  key: london
  special_item_ids: [99, 47, 51, 90, 91]
```

world 那份只存 id；价与供给初值在 economy 内容包。避免两份真相。

## 7. 独立旋钮

- 每港槽数（原版 5；引擎按表长，caps 给硬顶）
- 供给上下限
- `brand_grade_rank`
- 账本行数
- 投资最小额

改「伦敦产什么」只动内容包。改「1 级才算品牌」只动 `brand_grade_rank`。改份额门槛只动 factions 配置，经济仍按查询结果过滤。

## 8. 对外接口

```
SpecialSlot = { town_id, slot, item_id, supply, data_price, grade_rank }

Economy.specials(town_id) -> [SpecialSlot]          # empty 槽可省略或 item=empty
Economy.consume_special(town_id, slot, qty)
Economy.refund_special(town_id, slot, qty)
Economy.is_brand(town_id, slot) -> bool
Economy.list_brands(force_id) -> [item_id]          # 内部 query share
Economy.invest(town_id, force_id, gold)
Economy.ledger(owner_id) -> { income, expense, investment, last_month }
Economy.post_expense(owner_id, gold)
Economy.post_income(owner_id, gold)
```

事件：

```
InvestmentMade { town_id, force_id, gold }
BrandRecognized { force_id, town_id, item_id }      # 年结 / 月结扫描命中
TributeDue { force_id, town_id, gold }
SupplyChanged { town_id, slot, item_id, supply }
```

查询 factions：`query_share(town_id, force_id) -> int`。不反向写。

`03.Trade.buy` 内部只调 `consume_special`。

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| JSON `supply_or_price` | 100–6000，按 DATAPRICE `[推断]`，未在头文件点名 |
| JSON `grade` 5–25 | 不是「１级」；可能是另一套刻度或初始供给 |
| `fn_140015168` 读的运行时字节 | 只知 `==1` 为品牌，`>=6` 在卖价路径被跳过 |
| `CTown+0x2c/+0x2e/+0x30/+0x31` 与世界表三字段对齐 | 未还原 |
| `SPECIALPRODUCTSTATE_BITS` | 只有符号 |
| 投资 → 发展度 / 份额 / 升到 1 级 的公式 | 仅有文案 |
| 上缴金额公式 | 仅有 mesfile |
| 账本 6 行含义 | 存档循环 6；月还是分类 `[未知]` |
| `+0x00..0x0B` 上月内部布局 | 12 字节未拆 |
| 种类到货月历 | 攻略有、exe 未钉 |
| 发展后才出现的特产品（攻略蓝字） | 世界表已有 5 槽；解锁条件 `[未知]` |
| 原产物导入才出现的特产品（攻略红字） | 叙事 / 发现物侧；经济只收「解锁槽」命令 |
| World1 槽内容 | 未对照 `World1_town.c` |
