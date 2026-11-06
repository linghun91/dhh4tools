# 投资、发展度与上缴

特产品文档把「投资金额 → 发展度 / 份额」和「上缴金额」标成 `[未知]`。本篇从垫板、宫殿、月结三条链把式子钉死。份额加减的**效果**仍属 factions；本系统算钱、算发展度、发事件。

`CTown.cpp` / `CForce.cpp` 是存档拷贝，没有公式。公式在未挂名的 `fn_140009ae8`、`fn_140004b8d`、`CTownElement::vfunc_04`、`CForceElement::vfunc_04`、`CItemElement::vfunc_04`。

## 1. 职责边界

本系统管：

- 投资垫板输入的金币如何扣、如何写入港口「待转化投资」
- 待转化投资 → 商业 / 军事发展度（含 9999 封顶、独占半价）
- 投资成功时拟增加的份额点数（交给 factions 封顶）
- 独占港月结上缴、附庸 1% 上缴、珍品出租费
- 发展度涨完后按份额给势力影响力（不是金币）

不管：

- 垫板矩形、按钮皮肤（`CInvestmentPadView` 是 View）
- 份额槽怎么占、合约拒签文案的 UI（factions / town-life 发命令）
- 舰队代理账本买货亏损（`CFleetElement` 账本；见 `exe_caps.md`）
- 市价、特产品供给数量（`02` / `04`）
- 港口坐标、region（world）

## 2. 原版事实

### 2.1 调用图

```
CInvestmentPadView::ctor          0x140291c70   UI 矩形；无公式
fn_14001019a                      0x140297240   构造垫板，跑 fn_1400145ec，返回金币
  ├─ CTrade / fn_14000b2fd        0x140275f20   交易所「要投资吗」
  ├─ 宫殿 fn_140013d1d            0x140161350   新合约（两轨对半）
  └─ 宫殿 fn_14001077b            0x1401619a0   军事轨加投资
        → fn_14001117b            扣 CForce+0x0C 金币
        → fn_140004b8d            写入 CTown+0x24[track]
        → fn_1400122f6            份额点数 = gold / min_fee

世界月结（各 Element::vfunc_04）
  CTownElement::vfunc_04          0x1402668f0   反编译失败；objdump 可读
        每座活港 → fn_140009ae8  0x14025f3c0   待转化 → 发展度
        SCalendar+0x1c==2 时
              aulExclusiveProfit[25]
              独占上缴 → AddGold，消息 0x5A
  CForceElement::vfunc_04         0x14010dc20
        DAT+0x7c==6 且附庸位 != 0x1A
              tribute = gold / 100，消息 0x167
  CItemElement::vfunc_04          0x140129170
        出租费 Σ price/50，消息 0x5B
  CFleetElement::vfunc_04         0x1400ff2c0
        账本付不起转入下月；不是上缴公式
```

`CTownElement::vfunc_04` 的 RTC 名 `aulExclusiveProfit`（100 字节 = 25×uint32）就是独占上缴的按势力累加桶。

### 2.2 港口上的数

`CTown` 存档 `vfunc_07` / `vfunc_06` `[确认]` `CTown.cpp`：

| 偏移 | 宽 | 循环 | 含义 |
| --- | --- | --- | --- |
| `+0x18` / `+0x19` | u8+u8 | 3 | 合约槽：势力 id、份额。空槽 `force=0x1A`、`share=0` |
| `+0x1e` | u16 | 2 | 商业 / 军事发展度。`eTownDevelop` 与 2 比 |
| `+0x24` | u32 | 2 | **待转化投资金币**，与发展度一一对应 |
| `+0x74` | 字节 | 1 | 读出后与 `3` / `4` / `0xC` 比较。`[推断]` `eTownKind` |

关系文档把 `+0x24` 两个 dword 标成未核实。本篇钉成待转化投资：`fn_1400117ca` 原样读 `*(town + track*4 + 0x24)`；转化后写 0。

发展度上限 **9999**（`0x270F`）在转化函数里 `min(9999, dev)`，与 `exe_caps.md` 港口商业/军事补丁点同一立即数。USHORT 硬顶 65535。

### 2.3 势力上的数

`CForce` 存档 `[确认]` `CForce.cpp`；金币读写是独立小函数：

| 偏移 | 宽 | 含义 |
| --- | --- | --- |
| `+0x0C` | u32 | 金币。`fn_140002725` 读；`fn_140002cb6` 加（封 **99 999 999** = `0x5F5E0FF`）；`fn_14001117b` 是取负再加 |
| `+0x10` | u16 × 7 | 影响力。`fn_14000ae7a(force, theater, d)` 加完封 `[0, 9999]` |
| `+0x1E` | u8 | 附庸对象势力 id。`0x1A` = 无。`fn_140012788` |

玩家垫板投资**只扣** `CForce+0x0C`，不写舰队账本。账本 `CFleetElement+0x3738 + i*0x18` 的 `+0x0C` 支出 / `+0x10` 收入 / `+0x14` 投资是代理舰队那一套（`fn_140005849` / `fn_14000ff88` / `fn_140005e8e`）。

### 2.4 垫板是 UI

`CInvestmentPadView` 构造写一串矩形立即数（`0x13e, 0x6c, 0x41…`），`vfunc_02` 拆子控件。金额由 `fn_14001019a(min, max, 0x191)` 返回；`0x191=401` 是第三参，**不是**金币上限。取消返回 0。

### 2.5 文案

`mesfile_sc.txt`：

- 「没有占有率，交易也不能进行，要投资吗」「要投资多少呢？」「占有率我会稍微优待一点给你」
- 「最低合约费是%s枚金币」「根据你支付的合约金来承认占有率」
- 「独占合约，无法再与其他势力签合约」
- 「多亏了巨额的商业投资，%s的景气恢复了 / 越来越繁荣」
- 「本月的上缴金额是%s枚金币。请缴纳。」
- 「%s以独占的城市得到上缴金额%s枚金币。当作工具的出租费，有%s枚金币的收入。」
- 「若是这件，每月支付%s枚金币」

代码侧消息 id：独占收入 `0x5A`，出租费 `0x5B`，附庸上缴 `0x167`。与 mesfile **文件偏移**不是同一套数。

### 2.6 独占判定

`fn_140004c19(town)` `[确认]`：

```
force = share_slot[0].force
if force == 0x1A: return empty
if share_slot[0].share < 100: return empty
return force_id(slot0)
```

只看**槽 0**、份额 **≥ 100**。宫殿拒签：三槽占满，或（1 槽或 2 槽）且份额合计 == 100。

## 3. 实体与 ID

| 实体 | ID | 计数 | 空 |
| --- | --- | --- | --- |
| 发展轨 | `eTownDevelop` 0 商业 / 1 军事 | 2 | — |
| 待转化投资 | `(town_id, track)` | 每港 2 个 u32 | 0 |
| 合约槽 | `(town_id, slot 0–2)` | 3 | `force_id = 26` |
| 独占港 | 派生：槽 0 份额 ≥ `share_cap` | — | 非独占 |
| 附庸 | `CForce+0x1E` | 每势力 1 | `0x1A` |
| 出租中的道具 | `item_id`，运行时 `+0x3c` 位测试 | 原版 231 | 位未置 |

重构主键只用 `town_id` / `force_id` / `item_id` / `track`。禁止在配置里存 `CTown*`。

## 4. 算法 / 公式

命名参数进 `config/formulas/investment.yaml` 与 `tribute.yaml`。实现用有符号饱和算术；除数 0 必须先夹到 `min_fee_floor`（原版合约在两轨发展度都为 0 时最低额是 0，随后 `gold / min_fee` 会 `#DE`）。

辅助：`max_s` = 有符号 max（`fn_14000939f`），`min_s` = 有符号 min（`fn_140011bd0`），`max_u` = 无符号 max（`fn_1400013cf`），`min_u` = 无符号 min（`fn_140003f9e`），`rand(n)` = `fn_140005272`（`n<=0` 则 0，否则 `rand32 % n`）。

### 4.1 最低 / 最高投资额

交易所、军事加投资共用 `fn_14000ba41(town, track)`：

```
min_invest(town_id, track) =
    max_s(invest_min_floor, floor(develop(town_id, track) / 2))
    # invest_min_floor = 1
    # /2 是 cdq; sub eax,edx; sar 1（向 0 的有符号 ÷2）
```

交易所最高额 `fn_14000bc80`：

```
max_invest_trade(town_id) =
    min_invest(town_id, commerce) * invest_max_factor
    # invest_max_factor = 20
再与玩家金币取 min 后送进垫板。
```

宫殿**新合约**最低额 `fn_140013d1d`：

```
min_contract_fee(town_id) =
    floor( (develop(commerce) + develop(military)) / 4 )
    # cdq; and edx,3; add eax,edx; sar 2  → 有符号 ÷4
```

宫殿**军事加投资** `fn_14001077b`：先 `min_invest(town, military)`。若 `DAT_144296500+0x7c == 0`：

```
min_fee = max_u(1, floor(min_fee * 8 / 10))   # 八折
```

`+0x7c` 语义 `[未知]`，键先占位。

### 4.2 写入待转化

`fn_140004b8d(town, track, investor_force, gold)`：

```
if block_if_here(town) and investor_force != player_force: return
pending[track] = min_u(pending_gold_cap, pending[track] + gold)
# pending_gold_cap = 9_999_999 (0x98967F)
```

`block_if_here` = `fn_140009b42`：用 `DAT+0x7c` 与港口 id 比。`[推断]` 「玩家正在这座港」时 NPC 不能往待转化里加钱。

三条入口怎么分轨：

| 入口 | 扣金币 | 写入 |
| --- | --- | --- |
| 交易所 `fn_14000b2fd` | 全额 | 只 **商业** `track=0` |
| 宫殿新合约 | 全额 | **对半**：`half = gold/2`（无符号）各写一轨。奇数余 1 丢弃 |
| 宫殿军事加投资 | 全额 | 只 **军事** `track=1` |

### 4.3 份额优待

已有份额（或新合约）时：

```
share_gain = floor(gold_paid / min_fee)     # 无符号 div
Share.apply(town_id, force_id, share_gain)  # fn_1400122f6 → fn_14000957a
```

交易所：仅当该势力**已经**有份额（`fn_1400092af > 0`）才走这条。这就是文案「稍微优待」——优待量是「付了几个最低额」，不是固定 +5。

`fn_14000957a` 本体 1844 字节（RTC `nShareAmount` / `anProtectShare`），封顶仍是既有的 `100 − 已占`。内部保护槽细节不在本篇展开。

军事加投资另有：`fn_14000f759(town)` 为真且 `share_gain >= rand(100)` 时 `fn_14000e90d`。谓词与效果 `[未知]`，不要写成介绍信确认式。

### 4.4 待转化 → 发展度（F-ECO-11）

`fn_140009ae8`，由 `CTownElement::vfunc_04` 对每座活港调用。本函数内**没有**旬/月立即数。

独占半价：`share_slot[0]` 非空且份额 == 100 → `cost_unit = 10`，否则 `20`。

```
exclusive = slot0_occupied and slot0.share == share_cap     # 100
cost_unit = exclusive ? exclusive_cost_unit : normal_cost_unit  # 10 / 20

for track in {commerce, military}:
    dev = develop[track]
    gold = pending[track]
    while gold > 0:
        bracket     = floor(dev / develop_bracket)          # 1000
        cost        = (bracket + 1) * cost_unit
        can_buy     = floor(gold / cost)                    # 无符号
        points      = max_s(1, can_buy)                     # 余数也强买 1 点
        to_boundary = (bracket + 1) * develop_bracket - dev
        n = min(points, to_boundary)
        dev  += n
        gold -= n * cost                                    # 可变为负，随后 while 退出
    develop[track] = min_s(town_develop_cap, dev)           # 9999
    pending[track] = 0                                      # 余金清掉
```

含义：

- 每 1000 发展度一档，档内一点的价钱是 `(档+1)*20`（独占 `*10`）。0→999 一点 20；1000→1999 一点 40。
- `max(1, can_buy)`：桶里只要还剩正数，就再买 1 点（透支该桶，然后清零）。不是「余金结转下月」。
- 先买到 1000 边界再换档价，避免一笔钱按旧单价连跨两档。

商业发展度相对转化**前**的旧值：

```
if new_commerce >= old_commerce * 3 / 2:          # 有符号 *3/2
    if town_kind == kind_recover:                 # 4
        emit 景气恢复          # 消息 0x80
    else if town_kind == kind_flourish            # 0xC
         and new_commerce >= old_commerce * 2:
        emit 越来越繁荣        # 消息 0x81
        fn_14000bcf8(town, 3, 1)                  # 额外效果 [未知]
```

`town_kind` 来自 `town+0x74`。`4` / `0xC` 的枚举名未恢复。

特产品槽：空槽 `item==106` 跳过；槽记录 `+0x2` 的 WORD **>** 商业发展度则跳过；否则走解锁（`fn_14000513c`）。世界表 JSON `grade` 5–25 与这列 WORD 对齐是 `[推断]`。

转化若使发展度增加 `total_gain`（两轨点数之和）：

```
for each occupied share slot:
    d = floor(total_gain * slot.share / influence_div)   # 2000
    Influence.add(slot.force, theater_of(town), d)       # 再封 9999
```

`theater_of` = `fn_140014e48(town)`。`[推断]` 大战区下标。这是影响力，不是金币。

### 4.5 独占上缴（F-ECO-12 的城市侧）

`CTownElement::vfunc_04` 后半，仅当 `SCalendar+0x1c == 2`（事件文档：旬三态 0/1/2）。

```
profit[force] = 0  for 25 势力          # aulExclusiveProfit
for town in alive_towns:
    owner = exclusive_owner(town)       # §2.6
    if owner == empty: continue
    p = floor(develop(town, commerce) / exclusive_div)   # 10，有符号 idiv
    k = town_kind(town)
    if k == kind_bonus:    p = floor(p * 3 / 2)          # kind==3，无符号
    if k == kind_penalty:  p = floor(p / 2)              # kind==4
    profit[owner] += p
for force in 0..24:
    if profit[force] == 0: continue
    if force == player: emit ExclusiveTribute { gold: profit[force] }  # 0x5A
    Gold.add(force, profit[force])
```

`kind==4` 在繁荣文案里当「景气恢复」的城，在上缴里当 **×½**。同一字节两种用法，不要合并成一个策划名。

### 4.6 附庸上缴（F-ECO-12 的势力侧）

`CForceElement::vfunc_04`：

```
if DAT_144296500+0x7c != suzerain_gate: return     # suzerain_gate=6
if not fn_1400104b5(...): return                   # 未还原的许可
suzerain = force.overlord                          # +0x1E
if suzerain == empty: return
tax = floor(gold(player_force) / suzerain_div)     # 100，无符号
if tax == 0: return
emit SuzerainTributeDue { gold: tax }              # 0x167 「请缴纳」
Gold.add(suzerain, tax)
Gold.sub(player_force, tax)
```

这是**当前金币的 1%**，不读发展度、不读份额。与独占港上缴是两条。

### 4.7 出租费

宫殿陈列 `fn_140003afd`（消息 `0x555` / `0x557`）：

```
rent_quote(item_id) = floor(item_price / treasure_rent_div)   # 50
# item_price = CItem+0x2C（fn_140007423）
# 报价 ≥ 100000 时多一句 0x556
同意后 fn_140010393(item, …, 6)
```

月结收取 `CItemElement::vfunc_04`：

```
total = 0
for item in items:                                 # 原版 0..230
    if bit_test(item+0x3C, rent_bit):              # rent_bit=4
        total += floor(item_price / treasure_rent_div)
if total > 0:
    emit ToolRentIncome { gold: total }            # 0x5B
    Gold.add(player_force, total)
```

`rent_bit=4` 与陈列写入的 `6` 是否同一套状态 `[未知]`。本函数同样没有旬门。

### 4.8 立即数总表

| 参数 | 原版 | 出现 |
| --- | --- | --- |
| `town_develop_cap` | 9999 | `fn_140009ae8` `min_s`；影响力加值同样 9999 |
| `develop_bracket` | 1000 | `idiv 0x3E8` |
| `normal_cost_unit` | 20 | 非独占 |
| `exclusive_cost_unit` | 10 | 槽 0 份额 100 |
| `invest_min_floor` | 1 | `max_s(1, dev/2)` |
| `invest_max_factor` | 20 | 交易所最高 = 20×最低 |
| `contract_fee_div` | 4 | `(商+军)/4` |
| `pending_gold_cap` | 9 999 999 | 待转化 |
| `force_gold_cap` | 99 999 999 | 势力金币 |
| `share_cap` | 100 | 独占、优待、封顶 |
| `share_from_gold_div` | min_fee | `floor(gold/min_fee)` |
| `influence_div` | 2000 | `gain*share/2000` |
| `exclusive_div` | 10 | 商业发展度 / 10 |
| `kind_bonus` / `kind_penalty` | 3 / 4 | 上缴 ×1.5 / ×0.5 |
| `kind_recover` / `kind_flourish` | 4 / 12 | 繁荣文案 |
| `suzerain_div` | 100 | 附庸税 |
| `suzerain_gate` | 6 | `DAT+0x7c` |
| `calendar_decade_collect` | 2 | `SCalendar+0x1c` |
| `treasure_rent_div` | 50 | 报价与月结同一除数 |
| `treasure_price_hi` | 100000 | 陈列加句 |
| `pad_arg_2` | 401 | 垫板第三参，非金币 |
| `empty_force` | 26 | `0x1A` |

未出现在反汇编里的键保持 `null`。

## 5. 耦合分析

原版：

- 一次「投资」改港口 `+0x24`、势力金币、份额槽、稍后才改 `+0x1e`
- 转化函数自己读份额槽 0 决定单价，自己写影响力，自己解锁特产品
- 独占上缴、附庸税、出租费分三个 Element 的 `vfunc_04`，文案却写在同一句里
- `CInvestmentPadView` 与公式零耦合

重构：

```
economy.invest(town_id, force_id, gold, track)     # 扣金、记 pending
time.TownTicked { town_id }  →  economy.convert_pending
time.DecadeTicked { decade=2 } → factions 收独占上缴（金额问 economy.quote_exclusive）
factions 听 InvestmentMade 改 share
town 听 TownDeveloped 写两轨
items 听 TreasureDisplayed；月结出租费由 items 报金额、economy 入账
```

World0 / World1 只换港口内容，不换这些除数。

## 6. 解耦后的配置

```yaml
# config/formulas/investment.yaml
schema: dk4.formulas.investment
version: 1
# F-ECO-11。所属：本篇。

pending_gold_cap: 9999999
invest_min_floor: 1
invest_max_factor: 20          # 交易所最高 = 最低 × 此值
contract_fee_div: 4            # (commerce + military) / 4
military_min_discount:
  when_field: dat_plus_0x7c
  equals: 0
  num: 8
  den: 10
  # when_field 语义 [未知]；equals/num/den 是指令里的立即数

develop_bracket: 1000
normal_cost_unit: 20
exclusive_cost_unit: 10
exclusive_share: 100           # 槽 0
town_develop_cap: 9999         # 亦见 caps.yaml
remainder_buys_one: true       # max(1, gold/cost)

flourish:
  recover_factor_num: 3
  recover_factor_den: 2        # new >= old * 3/2
  recover_kind: 4              # town+0x74；名 null
  flourish_kind: 12
  flourish_factor: 2           # new >= old * 2
  extra_fn: null               # fn_14000bcf8 语义未还原

share_from_gold:
  formula: floor(gold_paid / min_fee)
  only_if_already_has_share_on_trade: true

influence_from_gain:
  formula: floor(total_gain * slot_share / influence_div)
  influence_div: 2000
  cap: 9999

pad:
  arg2: 401                    # 非金币上限
```

```yaml
# config/formulas/tribute.yaml
schema: dk4.formulas.tribute
version: 1
# F-ECO-12。所属：本篇。

exclusive:
  calendar_field: s_calendar_plus_0x1c
  calendar_equals: 2           # 旬 == 2
  base: floor(commerce / exclusive_div)
  exclusive_div: 10
  owner: share_slot_0_if_share_ge
  owner_share_min: 100
  kind_bonus: { kind: 3, num: 3, den: 2 }
  kind_penalty: { kind: 4, num: 1, den: 2 }
  kind_names: null

suzerain:
  gate_field: dat_plus_0x7c
  gate_equals: 6               # 字段名 null
  overlord_offset: 0x1E
  empty_force: 26
  formula: floor(gold / suzerain_div)
  suzerain_div: 100
  payer: player_force          # 原版这条只处理玩家势力

treasure_rent:
  quote: floor(item_price / treasure_rent_div)
  treasure_rent_div: 50
  price_hi: 100000
  collect_bit: 4               # item+0x3C；与陈列写入 6 是否同一套: null
  display_write_arg: 6
```

```yaml
# config/economy/investment.yaml   （内容/开关，不是除数）
schema: dk4.economy.investment
version: 1
brand_grade_rank: 1            # 仍见 04；本篇只提供发展度
tracks: [commerce, military]
share_slots_per_town: 3
```

`caps.yaml` 已有 `town_develop: 9999`。金币硬顶进 `engine_limits.yaml`（`force_gold: 99999999`），不要把 9999999 的待转化帽当成势力金库帽。

## 7. 独立旋钮

改这些值不应迫使贸易 / 战斗改代码：

- `develop_bracket`、`normal_cost_unit`、`exclusive_cost_unit`
- `invest_min_floor`、`invest_max_factor`、`contract_fee_div`
- `town_develop_cap`（须 ≤ USHORT 硬顶）
- `remainder_buys_one`（关掉则余金结转，须同时改 pending 清零）
- `exclusive_div`、`suzerain_div`、`treasure_rent_div`
- `influence_div`
- `calendar_equals`（哪一旬发独占上缴）
- 每港合约槽数（与 factions 同一键）

改「伦敦初始发展度」只动内容包。改「槽 0 才叫独占」只动 `owner` 策略。

## 8. 对外接口

```
Invest.min_fee(town_id, reason) -> gold
    reason: trade | contract | military
Invest.max_fee(town_id, reason, payer_gold) -> gold
Invest.pay(town_id, force_id, gold, reason) -> { pending, share_gain }
Develop.get(town_id) -> { commerce, military }
Develop.convert_pending(town_id) -> { old, new, total_gain }   # 月结
Tribute.exclusive(town_id) -> { force_id | empty, gold }
Tribute.exclusive_month(force_id) -> gold                      # 扫全部独占港
Tribute.suzerain(force_id) -> { overlord_id | empty, gold }
TreasureRent.quote(item_id) -> gold
TreasureRent.month(force_id) -> gold
```

事件：

```
InvestmentMade { town_id, force_id, gold, tracks[], share_gain }
TownDeveloped { town_id, track, old, new, reason: investment }
ExclusiveTribute { force_id, gold, town_ids[] }
SuzerainTributeDue { from_force, to_force, gold }
ToolRentIncome { force_id, gold, item_ids[] }
SpecialUnlocked { town_id, slot, item_id }
```

查询 factions：`query_share(town_id, force_id)`、`query_slots(town_id)`。经济不写份额槽。

`03.Trade` 无份额时只发 `OpenInvestment`，不自己改发展度。

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| `DAT_144296500+0x7c` | 与 6、与 0 比较；16 字节 getter `fn_140004a1b` | 「月份==6」或「剧本==6」 |
| `SCalendar+0x1c` 的 0/1/2 | 独占上缴只要 ==2；事件文档当旬 | 确认中文「下旬」以外的玩法名 |
| `CTownElement::vfunc_04` 的调用频率 | 转化无旬门；独占有 | 「每天转化一次」或「每月一次」 |
| `town+0x74` 的 3 / 4 / 12 | 上缴倍率与繁荣文案 | `eTownKind` 的中文城级名 |
| `fn_14000bcf8(town, 3, 1)` | 繁荣分支 | 任何确定效果 |
| `fn_140009b42` | `DAT+0x7c` vs 港 id | 确认「玩家在港」以外的名字 |
| `fn_14000957a` 保护槽 | 1844 字节、`anProtectShare` | 优待公式本身（优待量已钉） |
| `fn_14000f759` / `fn_14000e90d` | 军事投资支路 vs `rand(100)` | 介绍信 |
| 出租 `bit 4` vs 陈列写入 `6` | 两个立即数 | 同一状态位 |
| `CItem+0x2C` | 出租用的「价」 | 与目录 DATAPRICE 同一列 |
| 消息 `0x5A`/`0x5B`/`0x167` 与 mesfile 偏移 | 不是同一空间 | 「0x9e7 就是 0x5A」 |
| 账本 6 行 | 仍是存档循环 6 | 本篇改判定 |
| `pad_arg_2=401` | 垫板第三参 | 金币上限 |
| 世界表 JSON `grade` 与槽 `+0x2` | 比较对象是商业发展度 | 确认 15 就是「15 级商品」 |
| `abSpCharge`（RTC 76 字节） | 同在 `CTownElement::vfunc_04` | 上缴公式的一项 |

`CTownElement::vfunc_04` 前半还有特产品补货、流行、连接港（RTC `abSpCharge` / `aeConnectTown` / `eTownFashion`）。那些不是投资/上缴，保持 `[未知]`，不要塞进本公式包。
