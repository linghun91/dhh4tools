# 特产品、配额与月账

每座港口最多 5 个特产品槽：原版世界表里写着「这座城产哪件、商业多高才上架、每月基准量多少、底价多少」，重构后整行槽数据（含产哪件）归经济；供给、本月已购、份额配额、补货、月账也在经济系统。份额、合约、介绍信在势力；投资→发展度的公式在 `07-investment-tribute.md`。本系统只发事件、只读份额。

## 1. 职责边界

本系统管：

- 每港 5 槽的运行时状态：供给 `supply`、本月已购 `month_bought`、待种锁、槽底价
- 种植：玩家交出发现物解开待种锁（§2.9）
- 每月补货（按种类到货月历）
- 份额配额：某势力这个月在这一槽最多买几件（玩家亲买、舰队代买两个通道，§2.3）
- 买卖消耗：会话确认、NPC 自动交易
- 品牌商品判定要用的**经济侧**条件（「１级」）
- 地方舰队账本：收入、支出、投资

不管：

- 港口坐标、名称、文化圈 `culture`（world；运行时 `culture_id`，港口 `+0x25`）
- 市场份额、合约、上缴对象、介绍信 / 推荐信（factions；本系统 `emit` / 查询）
- 投资垫板扣款、待转化投资 → 发展度、上缴金额（`07`）
- 等级 1–5 怎么由发展度算（`05` §2.4；本系统只读 `grade_rank`）
- 流行品清单（factions / town fashion；买卖件数由 `03` 上报）
- 交易所 UI、投资垫板控件（`CInvestmentPadView`、`CSpecialProductView` 是 View）

拆分原则（DECISIONS D10「一条 DLL 港口记录按所有权拆表」）：特产品槽整行归 economy（`town` 外键 + 产哪件 + 门槛 + 基准量 + 底价 + 运行态）；world 的 towns 表不再持有特产品列表。

## 2. 原版事实

### 2.1 每港 5 槽：两段静态数据

`NUM_SPECIALPRODUCT = 5`，`INVALID_SPECIALPRODUCT = -1` `[确认]` `SpecialProductID.h`。会话里「没命中」用 **6**（`fn_14000680c`、`fn_140276350` 返回 6）`[确认]`。

World0 城镇记录步长 `0x84`（`GetFixTownProc`；运行时副本 VA `0x1442c8330 + id*0x84`，`fn_140004a66`）。特产品占两段：

**定义段** `+0x28 + slot*6` `[确认]`：

| 偏移 | 宽 | 含义 | 读者 |
| --- | --- | --- | --- |
| +0 | u8 | `item_id`（`0x6A` 空） | `fn_140009101` |
| +1 | u8 | 0 | — |
| +2 | u16 | **上架门槛**：商业发展度 ≥ 此值才上架 | `fn_1400101e5`、`fn_140015168`、`fn_140009ae8` |
| +4 | u16（读低字节） | **每月基准量** | `fn_140012da0` |

**运行时初值段** `+0x60 + slot*6` → `CTown+0x2c+slot*6`（`CTown::vfunc_05` `rep movsb` 6 字节 ×5，`0x1402ae5e6`）`[确认]`：

| CTown 偏移 | 宽 | 含义 | 读写者 |
| --- | --- | --- | --- |
| +0x2c | u8 | 状态位；bit0 = **待种锁**（不上架，交出对应发现物才解，§2.9） | 读 `fn_140010375`→`fn_140262120(·, 0)`；清 `fn_140012dc3`；`SPECIALPRODUCTSTATE_BITS` `[推断]` |
| +0x2d | u8 | 观察全 0 | — |
| +0x2e | u16 | **槽底价**（买入报价用） | `fn_140013881` |
| +0x30 | u8 | **供给** `supply` | 读 `fn_140001ab4`；写 `fn_14000ff65`、`fn_14000712b`、`fn_140008aee` |
| +0x31 | u8 | **本月已购** `month_bought` | 读 `fn_140009b0b`；写 `fn_14001227e`（经 `fn_140005a6f` 累加）、`fn_140008aee` 清 0 |

伦敦（World0 id 0）实值：

| slot | item | 门槛 | 基准量 | 槽底价 | 初始供给 | 待种锁 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 99 煤 | 4500 | 15 | 550 | 15 | 0 |
| 1 | 47 羊毛 | 2000 | 25 | 345 | 25 | 0 |
| 2 | 51 毛织品 | 2000 | 25 | 485 | 25 | 0 |
| 3 | 90 洋枪 | 4000 | 12 | 1150 | 12 | 0 |
| 4 | 91 大炮 | 3500 | 12 | 2400 | 12 | 0 |

布里斯托尔 slot 2（毛织品）、汉堡 slot 2（南瓜）开局待种锁为 1（毛织品对织布机、南瓜对南瓜种子，§2.9）。

`Docs/_data/towns_world0.json` 已改名：旧键 `supply_or_price` 实为**上架门槛**（现 `listing_threshold`），`grade` 实为**每月基准量**（现 `supply_base`）；运行时初值段（含槽底价）还埋在 `rest_hex` 里没拆。旧稿把 4500 当伦敦煤的底价、把 5–25 当「语义未知」，全部更正。全仓统一键名（`00-overview/10` §10）：`trading_item`（+0）、`listing_threshold`（+2）、`supply_base`（+4）、`data_price`（初值段 +2）；`05`、`07` 与本篇一致。

97 港共 45 个空槽；**105 件交易品都至少在一座城当特产品出现** `[确认]`。

`SPECIALPRODUCT_SUPPLY_MIN/MAX`：`UCHAR` `[确认]` `asserts.txt:308`。

### 2.2 上架与等级

上架谓词 `fn_1400101e5`、等级 `fn_140015168` 见 `05` §2.3–2.4（门槛 = 定义段 +2；等级 1 最好，6 = 不上架）。本系统只用结论：

- 等级 `< 6` 的非空槽进买货栏
- `fn_140009101(town, slot, 1)` 对不上架槽返回 `0x6A`；第三参 0 时返回原始 item

### 2.3 份额配额与可买件数

`[确认]` `fn_140012da0`（body `0x140261b20`）：

```
quota(town, slot) = supply_base(slot) * (commerce(town) / 50 + 100) / 100      # 两次有符号 idiv
```

伦敦煤：`15 * (4600/50 + 100) / 100 = 28`（商业 4600 时）。商业 9999 时约 ×3。

`[确认]` `fn_140001a37`（body `0x140275410`）：

```
cap   = quota(town, slot) * player_share(town) / 100 - month_bought(town, slot)   # 有符号
avail = min(supply(town, slot), cap)
return max(avail, 0)
```

`player_share` = `fn_1400092af(town, player_force)`：扫 `CTown+0x18` 三格合约，取该势力份额（`07` §2.2）。

舰队自动交易用的是**另一条**配额 `fn_140013345`（body `0x140261f10`，`06` §2.5）`[确认]`：底数是 `fn_14000c3f1` 直接读定义段 `+4` 的 `supply_base`，**不乘**商业系数：

```
npc_quota = supply_base * share(town, force) / 100            # 有符号 idiv
force == player : max(0, min(supply, npc_quota - month_bought))
else            : keep  = supply_base * (100 - Σ非玩家份额) / 100      # fn_140005e1b
                  max(0, min(supply, supply - keep + month_bought, npc_quota))
```

所以同一港同一槽，玩家亲自来买的配额按商业放大（伦敦煤商业 4600 时 28×份额%），派舰队来买只有 15×份额%。两条都扣同一个 `month_bought`。重构保留成两个 `channel`，由一个查询出口给出（§8 `Economy.buy_cap`），不让 `03` / `06` 各算一套。

会话打开时（`fn_14000efed`，见 `03` §2.8）还有两条：

- 配额算出 0、本月没买过、供给 > 0 → **可试买 1 件**
- 会计长技能 ≥160 且份额 <100：配额用完后进超额模式，**按双倍价**买到港口剩余供给

旧稿「买后 `fn_14000833c` 一类函数减供给」不对：`fn_14000833c` 是超额模式判定，`BuyOne` 只减会话卡上的可买数。

### 2.4 消耗

| 来源 | 动作 | 标记 |
| --- | --- | --- |
| 玩家会话确认 `CommitDeal` | `month_bought += n`（`fn_140005a6f`，`0x140277af2`）；`supply -= n` 仅当港口供给 ≠ 会话卡剩余可买数（`0x140277aa4`）；`n` = 本笔该槽件数 | [确认] `03` §2.6 第 11 步 |
| 玩家会话买 / 退 | 只动会话卡，不动港口 | [确认] |
| NPC 自动交易 | `supply = max(0, supply - qty)`（`fn_14000712b`）；有账本（玩家势力舰队）再 `month_bought += qty` | [确认] `06` §2.5 |

`month_bought` 只算玩家势力的买量（玩家会话 + 玩家势力有账本的舰队），所以别的商会的 NPC 买货只会抢供给，不吃玩家配额。`[确认]` 调用点；「设计意图」`[推断]`。

### 2.5 月初补货

`[确认]` `CTownElement::vfunc_04` 月初那一拍（原版门 `fn_14000e9da`；重构订阅 `MonthStarted`）在市价漂移（`02` §2.3，`0x14026765e`–`0x140267762`）**之后**，先建 19 格到货表（`0x140267781`–`0x140267952`），`month` 为 0 起月份：

| 条件 | 到货种类 |
| --- | --- |
| 每月 | 0 食品、1 调味料、5 酒类、8 纤维、17 矿石 |
| `(month+1)` 奇数 | 2 珍品、3 香辣料、4 嗜好品、11 香料、14 药 |
| `(month+1)` 偶数 | 7 工艺品、9 纺织品、12 装饰品、15 染料、16 武器、18 杂货 |
| `(month+1) % 3 == 0` | 6 贵金属、10 奢侈品、13 宝石 |

与买货栏「每月 / 奇数月 / 偶数月 / ３、６、９、１２月到货」文案一致（`05` §2.8）。

然后对每座活港：先 `fn_140009ae8`（投资→发展度，`07`），再 `fn_140010d7a(town, table)`：

```
for slot in 0..4:
    item = fn_140009101(town, slot, 1)          # 不上架则 0x6A，跳过
    if item == 0x6A or not table[kind_of(item)]: continue
    supply[slot] = quota(town, slot) & 0xFF      # 直接覆盖，不是累加；u8 截断
    month_bought[slot] = 0
```

`[确认]` `fn_140010d7a`（body `0x14025eac0`）用 `fn_140009101(town, slot, 1)`：只补**已上架**的槽（已发现、商业达门槛、无待种锁）。`fn_140008aee`（body `0x14025eb90`）写入。补货是**重置到配额**，没卖完的不累积；不到货的月份 `month_bought` 也不清零（配额跨到下一次到货）。

逐港交错 `[确认]`：同一循环里对港 A 先转化再补货，然后才轮到港 B。转化会全局置发现位（`07` §4.4），所以港 B 本月的转化解锁某货时，序号在前的港 A 本月不会补这件货——要等下一次到货月。重构用 `tick.yaml` 的 `{ per: town, steps: [economy.investment_convert, economy.restock] }` 复刻（§6）；改成两步各自整表跑是可选的「修正」。

原版包 `quota` 最大 `25 × 299 / 100 = 74`，u8 截断不触发；重构要显式夹到 `supply_max`。

### 2.6 品牌商品

mesfile：

- 「今年我商会经营的１级%s作为品牌商品得到了市场的认可。」
- 「遗憾！今年没有得到市场认可的品牌商品。」
- 「发放品牌商品%s，向城市居民宣传」
- 「通过将品牌商品的%s奉献给城市的掌权人，交涉一下扩大市场份额」

代码：`CTradingItemElement::vfunc_04` 年初扫全部港 `[确认]`（世界 tick 门 `fn_140014dd0` = 0 月月初第一拍，在本月转化 / 补货之后；`05` §2.5）：

```
if player_share(town) < 100: skip          # fn_1400088f0 < 0x64，只看玩家
for slot in 5:
    item = special_item(town, slot)
    if item == empty: skip
    if fn_140015168(town, slot) != 1: skip
    collect unique item
star_item = random(candidates) or empty    # CTradingItemElement+0x1a48
```

星品在报价里给 1 级 200%（其余 1 级 150%，`02` §2.5），舱位显示 ★（`05` §2.5）。份额本身属 factions；经济提供「该槽是不是 1 级」。

投资 → 发展度 → 跨过门槛 + `5*grade_step` 才到 1 级：公式在 `07` §4 与 `05` §2.4。

### 2.9 待种锁与种植（发现物交给城市）

`[确认]` 交易所入口 `fn_1400085d0`（body `0x140274be0`，RTC `eSpecialProduct` / `eItem` / `aeSpecialProduct` / `aeItem`）：

```
for item in 玩家持有的 eItem (0..0xE6):             # 循环上界 0xE7 写死；持有判定骨架 [确认]
    for slot in 0..4:
        t = fn_140009101(town, slot, 0)          # 原始 item，不看上架
        if t == empty: continue
        if catalog[t].unlock_item != item: continue   # 交易品对象 +0x15 = 目录 +11
        if not lock_bit(town, slot): continue     # fn_140010375：CTown+0x2c+slot*6 bit0
        if commerce(town) < listing_threshold(slot): continue   # fn_140008c0b 读定义段 +2
        candidates += (item, slot)
pick = candidates[rand(n)]                        # fn_140005272
ask mes 0x374                                     # 句文对应 mesfile 0x7737「如果有那%s，%s会成为这个城市的特产品」[推断]
yes: set_discovered(t)                            # fn_14000513c
     clear lock_bit(town, slot)                   # fn_140012dc3 → fn_140265b50(·,0,0)
     item 物主改 kind 6（交出）                    # fn_14000b771(·,6) + fn_140010393(item, owner, 6, 0)
     mes 0x375
no:  mes 0x376
```

所以待种锁 = 「这港这槽要先交一件对应的发现物才开」；交了以后货**全局**变已发现、本槽解锁，槽里的 `item_id` 不改。一次进交易所最多种一件（随机抽）。发现物 → 交易品对照见 `05` §2.7。

### 2.7 投资与占有率（文案）

mesfile：

- 「没有占有率，交易也不能进行，要投资吗」
- 「要投资多少呢？」
- 「占有率我会稍微优待一点给你」
- 「本月的上缴金额是%s枚金币」
- 「%s以独占的城市得到上缴金额%s枚金币」
- 「%s的市场份额上涨了」

「没有占有率，交易也不能进行」与 §2.3 一致：份额 0 时配额为 0，只能试买 1 件。

玩家垫板投资**只扣** `CForce+0x0C` 金币，写 `CTown+0x24[track]` 待转化投资，**不写**舰队账本 `investment` `[确认]` `07` §2.3。旧稿「投资扣款、记入月账 investment」更正：账本 `+0x14` 只属代理舰队那一套。

### 2.8 月账（代理舰队）

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

地方舰队买货：`record.expense += cost`（`0x1400ea7f0`）；无账本则直接扣势力金币。月结 `CFleetElement::vfunc_04`（约 `0x140100180`）付不起的转入下月支出 `[确认]` `exe_caps.md`、`06` §2.7。

## 3. 实体与 ID

| 实体 | ID | 计数 | 空 |
| --- | --- | --- | --- |
| 特产品槽 | `(town_id, slot 0–4)` | 每港 5 | `item = null`（原版 `0x6A`）；查找未命中返回 `null`（原版 6） |
| 上架门槛 | 槽定义 | u16 | — |
| 每月基准量 | 槽定义 | u8（u16 存、读低字节） | — |
| 槽底价 | 槽运行时 | u16 | — |
| 供给 | 槽运行时 | u8 0–255 | 0 不能买（试买规则除外也要 >0） |
| 本月已购 | 槽运行时 | u8 | 到货月清 0 |
| 待种锁 | 槽运行时 bit0 | — | 1 = 不上架，种植后清 |
| 品牌标记 | 槽上 `grade_rank==1` | — | 非 1 不是品牌 |
| 账本行 | `(force_or_fleet_id, book_index)` | 原版 6 行 × 0x18 | — |

`eSpecialProduct` 是槽下标，不是第二种商品目录。槽里的货物仍是 `eTradingItem`。

## 4. 算法 / 公式

### 4.1 配额

```
quota(town_id, slot) =
    supply_base(slot) * (commerce(town_id) / quota.commerce_div + quota.pct_base) / 100   # 50, 100

buy_cap(town_id, slot, force_id, channel):            # 唯一出口；03 与 06 都调它
    if channel == player_trade:                      # fn_140001a37
        q = quota(town_id, slot) * share(town_id, force_id) / 100
        return max(0, min(supply, q - month_bought))
    if channel == npc_proxy:                          # fn_140013345
        q = supply_base(slot) * share(town_id, force_id) / 100     # npc_quota.scale_by_commerce: false
        if force_id == player_force:
            return max(0, min(supply, q - month_bought))
        keep = supply_base(slot) * (100 - Σ share(非玩家)) / 100
        return max(0, min(supply, supply - keep + month_bought, q))
```

`share` 向 factions 查询；`player_force` 取 `Session.player_force()`。`npc_proxy` 不乘商业系数是原版怪癖，进 `quirks.fleet_quota_ignores_commerce`（缺省 true）。

### 4.2 会话报价（给 `03`）

```
Economy.special_offer(town_id, slot, force_id):
    price = Economy.quote(town_id, item, grade_rank).buy_gold
    avail = buy_cap(town_id, slot, force_id, player_trade)
    if avail == 0 and month_bought == 0 and supply > 0:
        avail = trial_buy.qty                         # 1
    return { price, price_over = price * over_quota.price_mult, avail, supply, month_bought }
```

超额模式是否开启由 `03` 按会计长技能值（`Modifiers.eval("skill.accounting", …)`）决定；本系统只给 `price_over` 与 `supply`。

### 4.3 消耗

```
Economy.settle_special(town_id, slot, qty, counts_toward_quota):
    supply = clamp(supply - qty, supply_min, supply_max)
    if counts_toward_quota:
        month_bought = clamp(month_bought + qty, 0, supply_max)
```

玩家会话：`counts_toward_quota = true`。NPC：舰队属玩家势力且有账本时为 true，否则 false。

### 4.4 月初补货

```
on MonthStarted(month):                       # schedule: month_start
    kinds = restock_calendar.kinds_for(month)
    for town in towns:                        # tick.yaml per: town（原版逐港交错）
        economy.investment_convert(town)      # 07 §4.4；先转化
        for slot in visible_slots(town):      # 已发现 + 达门槛 + 无待种锁
            if kind_of(slot.item) in kinds:
                slot.supply = clamp(quota(town, slot), supply_min, supply_max)
                slot.month_bought = 0
```

顺序：先 `07` 的投资转化（发展度变了，`quota` 跟着变），再补货；两步逐港交错（§2.5）。跨系统先后写在 `tick.yaml`（§6），不靠订阅注册顺序（DECISIONS D4）。

### 4.5 品牌

```
is_brand_product(town_id, slot) =
    special_item(town_id, slot) != empty
    and grade_rank(town_id, slot) == brand_grade_rank    # vanilla 1

brands(force_id) = unique [
    item_id
    for town in towns_where(share(force_id, town) >= brand_share_need)  # vanilla 100，读 factions
    for slot in 0..4
    if is_brand_product(town, slot)
]
```

原版只对玩家势力算。经济不算 share，向 factions `Share.of(town_id, force_id)`。`grade_rank`、`quota`、`buy_cap`、`is_brand` 都是派生量：本系统唯一计算、不存档（DECISIONS D6）。

### 4.6 月账

```
balance = income - expense - investment
month_roll:
    last_month = { income, expense, investment, balance }
    income = expense = investment = 0
    if unpaid > 0: expense += unpaid          # 原版付不起转入下月
```

NPC 买货：`expense += npc_buy_cost(...)`（`06`）。

## 5. 耦合分析

原版：

- 世界 DLL 城镇 hex 里两段塞 5 槽：item / 门槛 / 基准量，以及状态 / 底价 / 供给 / 已购初值
- `CTrade` 确认时改供给与已购；会话打开时读份额算配额
- `CTownElement::vfunc_04` 把补货、投资转化、市价漂移写在同一个月循环里
- 账本挂在 `CFleetElement+0x3738`
- 品牌扫描读 share

重构：

```
world.towns.yaml          # 只有港口本身；不含特产品
economy.special_products  # 门槛、基准量、槽底价、待种锁初值；运行时 supply / month_bought
economy.restock_calendar  # 种类 → 到货周期（本篇唯一拥有；05 文案、黄金航路奖金只引用）
factions.share            # 只被查询
economy.ledger            # 代理舰队账本（本篇拥有；fleets 只持有 ledger 引用）
engine.tick               # 月初各步先后
```

World1 换包只换哪座城产什么、门槛与量，不换 5 槽逻辑。

## 6. 解耦后的配置

```yaml
# config/economy/special_products.yaml
schema: dk4.economy.special_products
version: 1
# 原版包由导出脚本从 World0 城镇表 +0x28 / +0x60 两段生成；本表是「港口产什么」的唯一来源
towns:
  - town: vanilla:london
    slots:
      - slot: 0
        trading_item: vanilla:coal
        listing_threshold: 4500  # 定义段 +2
        supply_base: 15          # 定义段 +4（u16 存、读低字节）
        data_price: 550          # 初值段 +2 → CTown+0x2e
        initial_supply: 15       # 初值段 +4
        plant_lock: false        # 初值段 +0 bit0（待种锁，§2.9）
      - { slot: 1, trading_item: vanilla:wool,       listing_threshold: 2000, supply_base: 25, data_price: 345,  initial_supply: 25, plant_lock: false }
      - { slot: 2, trading_item: vanilla:woolen,     listing_threshold: 2000, supply_base: 25, data_price: 485,  initial_supply: 25, plant_lock: false }
      - { slot: 3, trading_item: vanilla:musket,     listing_threshold: 4000, supply_base: 12, data_price: 1150, initial_supply: 12, plant_lock: false }
      - { slot: 4, trading_item: vanilla:cannon,     listing_threshold: 3500, supply_base: 12, data_price: 2400, initial_supply: 12, plant_lock: false }
```

```yaml
# config/economy/restock_calendar.yaml   （本篇拥有；唯一一份）
schema: dk4.economy.restock_calendar
version: 1
# 三处共用（DECISIONS D3/D10「一张表一个 owner」）：
#   04 月初补货掩码、05 买货栏到货文案、黄金航路奖金 fn_1400108fc 的到货判定
month_base: 1                    # 规则月份按 1 起；原版判断用 (0 起月份)+1
periods:
  - key: monthly
    label_key: ui.trade.arrival.monthly
    every: 1
    kinds: [vanilla:food, vanilla:seasoning, vanilla:alcohol, vanilla:fiber, vanilla:ore]
  - key: odd_months
    label_key: ui.trade.arrival.odd_months
    every: 2
    offset: 1
    kinds: [vanilla:delicacy, vanilla:spice, vanilla:stimulant, vanilla:incense, vanilla:medicine]
  - key: even_months
    label_key: ui.trade.arrival.even_months
    every: 2
    offset: 0
    kinds: [vanilla:craft, vanilla:textile, vanilla:ornament, vanilla:dye, vanilla:weapon, vanilla:sundries]
  - key: quarterly_3_6_9_12
    label_key: ui.trade.arrival.quarterly_3_6_9_12
    every: 3
    offset: 0
    kinds: [vanilla:precious_metal, vanilla:luxury, vanilla:gem]
unlisted_kind: never             # 原版 kind>18 文案返回空指针、补货掩码 0
```

```yaml
# config/formulas/economy.yaml  （本篇段）
supply:
  impl: vanilla.supply
  params:
    min: 0
    max: 255
quota:                         # player_trade 通道，fn_140012da0 / fn_140001a37
  impl: vanilla.quota
  int_mode: vanilla_i32        # 两次有符号 idiv（§2.3）
  params:
    commerce_div: 50
    pct_base: 100
    share_div: 100
npc_quota:                     # npc_proxy 通道，fn_140013345
  impl: vanilla.npc_quota
  int_mode: vanilla_i32        # 有符号 idiv（§2.3）
  params:
    scale_by_commerce: false   # 原版怪癖：不乘商业系数
    share_div: 100
    keep_free_share_for_player: true
trial_buy:
  impl: vanilla.trial_buy
  params:
    qty: 1
over_quota:
  impl: vanilla.over_quota
  params:
    price_mult: 2
restock:
  impl: vanilla.restock
  schedule: month_start
  params:
    mode: reset_to_quota       # 原版覆盖；可选 add_quota
    # 逐港「转化→补货」交错由 tick.yaml 的 per: town 决定（§6 tick 片段），不在本文件
    clamp_to_supply_max: true  # 原版 u8 截断不夹；原版包数值不触发
brand:
  impl: vanilla.brand
  schedule: year_start
  params:
    grade_rank: 1
    share_need: 100            # 查询 factions；不是经济自己存
planting:                      # §2.9
  impl: vanilla.planting
  params:
    require_commerce_ge_unlock: true
    max_per_visit: 1
ledger:
  impl: vanilla.ledger
  params:
    slots: 6
```

```yaml
# config/engine/tick.yaml 片段（engine 拥有，全文见 14-config-schemas/examples/engine/tick.yaml；phase id 见 00-overview/10 §11）
# economy 只声明 step 名；先后以这里为准，不靠事件订阅注册顺序（DECISIONS D4）
# 原版顺序来自世界 tick 分发 0x1402aae14–0x1402aaf64 与 CTownElement::vfunc_04 体内顺序
phases:
  - id: items                          # CItemElement::vfunc_04，世界 tick 里排在 CTownElement 之前（07 §2.1；词表 §14.17）
    schedule: [month_end]
    steps: [economy.treasure_rent]     # 07 §4.7；items 报金额、economy 入账
  - id: towns
    schedule: [month_start, month_end]
    steps:
      month_start:
        - economy.town_state_update      # 02 §2.3 ①②③：结束/计月 → 蔓延 → 新发；景气状态归 economy（00-overview/06）
        - economy.price_drift            # 02 §2.3：状态目标 + 两格流行推种类（逐港）
        - { per: town, steps: [economy.investment_convert, economy.restock] }   # 07 §4.4 转化 → 本篇 §4.4 补货（逐港交错）
      month_end:
        - economy.exclusive_tribute      # 07 §4.5；出租费已在上面 items phase 先结
  - id: forces
    schedule: [month_start]
    steps: [factions.suzerain_tribute]   # 07 §4.6；CForceElement::vfunc_04；factions 拥有
  - id: star_goods
    schedule: [year_start]
    steps: [economy.star_goods]          # 本篇 §2.6 / 05 §2.5，在当月补货之后
  - id: gold_road_bonus
    system: ocean.gold_road              # fn_1400108fc；读 restock_calendar
    schedule: [month_start]
# fleets（npc_fleets phase）每拍跑账本门（06 §2.7）
```

`world/towns.yaml` 不再有 `special_item_ids`：「伦敦产煤」只在本表一处。别的系统要问「这港产什么」走 `Economy.specials(town_id)`。交易品 key 以 `01` §3.2 为准。

## 7. 独立旋钮

- 每港槽数（原版 5；引擎按表长，caps 给硬顶）
- 供给上下限
- 配额曲线（`commerce_div`、`pct_base`）、试买件数
- 舰队配额是否乘商业系数（`npc_quota.scale_by_commerce`）
- 到货月历、补货模式（覆盖 / 累加）、逐港交错还是全转化后再补（`tick.yaml` 的 `per: town`）
- 种植门槛与每次件数
- `brand_grade_rank`、`brand_share_need`
- 账本行数

改「伦敦产什么」只动内容包。改「1 级才算品牌」只动 `brand.grade_rank`。改份额门槛只动 factions 配置，经济仍按查询结果过滤。

## 8. 对外接口

```
SpecialSlot = { town_id, slot, trading_item | null, supply, month_bought, data_price, grade_rank, plant_lock }

Economy.specials(town_id) -> [SpecialSlot]          # 空槽 trading_item = null
Economy.buy_cap(town_id, slot, force_id, channel) -> int   # channel: player_trade（乘商业系数）| npc_proxy（不乘）；03 与 06 共用
Economy.special_offer(town_id, slot, force_id) -> { price, price_over, avail, supply, month_bought }
Economy.settle_special(town_id, slot, qty, counts_toward_quota)    # 03 Commit / 06 NPC 都只调它
PlantSpecialProduct { fleet_id, town_id, slot, item_id } -> { trading_item } | null   # 命令（items / narrative → economy，00-overview/10 §13.3）；§2.9；slot 取自 PlantOffered（原版 pick = (item, slot)）；fleet_id / item_id 原样回填到 SpecialPlanted；items 负责扣发现物
Economy.plant_candidates(town_id, held_item_ids[]) -> [(item_id, slot)]
Economy.is_brand(town_id, slot) -> bool
Economy.list_brands(force_id) -> [trading_item_id]  # 内部查 Share.of

Ledger.of(fleet_id) -> ledger_id | null             # 原版 fn_14000c342；无账本 = null
Ledger.get(ledger_id) -> { income, expense, investment, last_month }
Ledger.post_expense(ledger_id, gold)
Ledger.post_income(ledger_id, gold)
```

事件：

```
BrandRecognized   { force_id, item_id }             # 年初
SupplyChanged     { town_id, slot, item_id, supply, month_bought }
SpecialRestocked  { town_id, month, slots[] }
SpecialPlanted    { town_id, slot, trading_item, fleet_id, item_id }   # 由命令 PlantSpecialProduct 成功后发（economy 唯一发出，§13.3）；载荷按词表 §14.25（带 fleet_id，items 不必自持待决记录）；items 收到后按 fleet_id 改物主
```

查询 factions：`Share.of(town_id, force_id) -> int`。不反向写。查询 world：`Towns.develop(town_id, commerce)`。

`03.Trade.commit` 与 `06.NpcTrade` 内部只调 `Economy.settle_special` / `Economy.buy_cap`。投资事件见 `07`。

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| 运行时 `+0x2d` | 观察全 0，未见读取 |
| `SPECIALPRODUCTSTATE_BITS` 除 bit0 外 | 只有符号 |
| ~~隐藏位何时被清~~ | 已钉：待种锁，交易所入口种植清（§2.9） |
| 待种锁是否还有别的写入点 | 只查了 `fn_140012dc3` 清；置位除开局初值外未扫 |
| 槽底价 `+0x2e` 运行时是否会变 | setter `fn_14000d7a1` 无直接调用者 |
| 账本 6 行含义 | 存档循环 6；月还是分类 `[未知]` |
| `+0x00..0x0B` 上月内部布局 | 12 字节未拆 |
| 原产物导入才出现的特产品（攻略红字） | 即 §2.9 种植；由交易所入口触发，不是叙事命令 |
| World1 槽内容 | 未对照 `World1_town.c` |
| ~~`towns_world0.json` 字段名~~ | 已改：旧键 `supply_or_price` / `grade` → `listing_threshold` / `supply_base`（`_data` 已用新键） |
| `CommitDeal` 扣供给的「供给 ≠ 卡上剩余」条件 | 条件已钉，设计意图（防超额模式重复扣？）`[未知]` |
