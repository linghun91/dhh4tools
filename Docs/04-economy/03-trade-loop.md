# 买卖会话

玩家在港口交易所里打开的那一次交易：从船舱 unpack，买特产品、卖舱货、退货、确认或取消。会话对象原版叫 `CTrade`。

## 1. 职责边界

本系统管：

- 交易会话状态机（打开 / 买一件 / 点舱位 / 全卖 / 退货 / 确认 / 取消）
- 把船舱 `HoldRecord` 拷进会话 `ExchangeSlot`，确认时再写回
- 会话内的买货栏快照（每槽买价、可买件数、超额模式）
- 堆叠规则（引擎允许 `>1`，原版包 `=1`）
- 本笔金币合计、会计长杀价、与玩家金库的结算
- 向势力发「卖了流行品 / 卖了某商品」事件（份额、介绍信不在本系统写）

不管：

- 市价怎么算（`02.quote`）
- 特产品供给、配额、补货（`04`；本系统在确认时上报本笔每槽件数）
- 货舱作为船舱室存在（`05-ships-fleet` 拥有 `HoldRecord` 持久化）
- 人物技能值（只经 `Fleets.post_holder` 找会计长、`Modifiers.eval("skill.accounting", ctx)` 取值；DECISIONS D2）
- 委托 / 剧情扣货（舰队接口 `Ships.count_item` / `Ships.add_load`，由 town-life / narrative 调用）
- UI 列宽、按钮皮肤、`%6d`（`12-ui-save`）

## 2. 原版事实

来源：`srcdata/docs/trade_stack.md`（全文）、`srcdata/include/trade.h`、`dk4_types.h`、`srcdata/source/game/trade/Trade.cpp`、`CTrade.cpp`、`Constants.h`、`exe_caps.md`；下文未注明的地址均为 sc 反汇编（`srcdata/disasm_sc/text_full_sc.txt`）。

### 2.1 格子

| 结构 | 大小 | 位置 | 用途 |
| --- | --- | --- | --- |
| `HoldRecord` | 8 B | `ship+0x58+j*8` | 船上持久货 |
| `ExchangeSlot` | 12 B | `CTrade+0xE8 + ship*0x3C + slot*0xC` | 会话舱位 |
| `SellRecord` | 8 B | `CTrade+0x214 + ship*0x28 + j*8` | 本笔待卖列表 |

`GetExchangeSlot`（thunk `0x140006181`，body `0x140275620`）：第二参 `*0x3C`、第三参 `*0xC` `[确认]`，与 `Trade.cpp` 一致。`trade_stack.md` 把 ship/j 写反了，以函数体为准。

玩家舰队：`NUM_FLEETSHIP=5` × `NUM_SHIPLOAD_EXCH=5`。`CountHolds(0x0A)` 扫描上限 10（`NUM_HOLD_SCAN`），不是第二套 10 格交易所。

其他势力不走 25 格真舱，而走 `fleet+0x74`「一条记录 + 数量」`[确认]` `trade_stack.md` 委托/自动交易两支。

### 2.2 字节

```
HoldRecord:
  +0 u8  item       # 0x6A 空
  +1 u8  grade      # 买入当时的等级 1–5
  +2 u8  town_id    # 买入港；0x62 = 无港（清空、NPC 装货、战利品一类）
  +3 u8  count      # 原版不用；堆叠补丁写件数，0 当 1
  +4 u32 buy_price  # 0 = 战利品
ExchangeSlot: 同上前 8 字节，再 +8 u32 just_bought  # 本笔刚买=1，unpack 写 0
```

`[确认]` `fn_140014d80(rec, item, grade, town_id, price)`（body `0x1400fe870`）按 +0/+1/+2/+4 写；`BuyOne` 传的是 `fn_140015168` 等级与 `fn_14000f498(town)`。旧稿 `b1/b2 语义未知` 更正。

原版拷贝 `110b8` 只拷 `+0/+1/+2/+4`。清空 `c662`：`item=0x6A`，`+2=0x62`，价 0。

### 2.3 会话对象 `CTrade`（观察字段）

| 偏移 | 名 | 说明 | 标记 |
| --- | --- | --- | --- |
| +0x14 | 杀价已成功 | `CommitDeal` 杀价成功后写 1；为 1 时不再杀价 | [确认] `0x14027865f`、`0x1402771af` |
| +0x18 | `ulTotalExpenditure` | **买入合计**（玩家付出）。`BuyOne` 加、退货减，封 `[0, 99999999]` | [确认] `fn_14000e9c6`；`CommitDeal` RTC 槽 `+0x54` 读它 |
| +0x1C | `ulTotalIncome` | **卖出合计**（玩家收入） | [确认] `fn_14000f97a`；RTC 槽 `+0x34` |
| +0x20 | `int[5]` 按**特产品槽** | 本次会话从该槽买的件数（买 +1、退 -1） | [确认] `fn_14001454c` / `fn_14000a3c6` |
| +0x34 | `int[5]` 按特产品槽 | 其中在超额模式下买的件数 | [确认] 同上 |
| +0x48 | `int[5]` | 打开时该槽可买件数 | [确认] `fn_14000efed` |
| +0x5c | `int[5]` | 超额模式（会计长） | [确认] `fn_14000833c` |
| +0x70 | `int[5]` | 「零配额试买 1 件」标记 | [确认] `fn_14000efed` |
| +0x84 + sp*0x14 | `{price, price_x2, sell, avail, supply}` | 每槽：当前买价、双倍价、卖价、可买、港口原始供给 | [确认] `fn_14000efed` |
| +0xE8 | `ExchangeSlot[5][5]` | | [确认] |
| +0x214 | `SellRecord[5][5]` | | [确认] |
| +0x2dc + k*0x10 | 买货栏第 k 张卡 `{sp, item, grade, avail}` | 只排可上架的槽，k 连续 | [确认] `fn_140001816` |
| +0x32c | dword | ctor 清 0 | [确认] |
| +0x588 / +0x590 | 见下 | | |

旧稿与 `Trade.cpp` / `trade.h` 有三处要改：

- `+0x18` / `+0x1C` 的 RTC 名写反。`CommitDeal` 的 RTC 表 `ulTotalIncome@0x34`、`ulTotalExpenditure@0x54`，而 `0x34 ← [this+0x1C]`、`0x54 ← [this+0x18]`。`Trade.cpp` 中文注释（+0x18 买入合计）是对的，变量名配错了。
- `+0x20` / `+0x34` 不是「每船买/卖小计」。`UnpackFromShip` 清它们的循环变量 RTC 名是 `eSpecialProduct`（`0x140279c2e`），`CommitDeal` 也按特产品槽读。
- `BuyOne` / `RefundJustBought` 的 `this` 先取 `[this+0x588]` 才调 `GetExchangeSlot`。`+0x588` 指向持有 `ExchangeSlot` 的会话对象；「`CTrade+0x588` 是 `CTradingView*`」可能把两个对象倒过来了 `[推断]`。`+0x590` 是买入价列开关，按钮 `0x15` 切换，属 UI。

对象大小 ctor 分配 `0x330`。

### 2.4 按钮（`OnWindowButton` `0x14028bbc0`）

| 号 | 动作 |
| --- | --- |
| 0x15 | 切换 `+0x590`（UI） |
| 0x17 | `CommitDeal` |
| 0x18 | `SellAll`（只卖 `just_bought==0`） |
| 0x19 | 取消：再 `UnpackFromShip`，丢弃未确认购买 |

### 2.5 堆叠

| 包 | 行为 |
| --- | --- |
| 原版 | `VANILLA_STACK=1`：买一件占一个空槽；卖出记录无数量 |
| DK4STK2/3 | 件数在 hold `+3`；补丁，不是设计约束 |

显示曾被价格列 6 个字逼到上限 9。**那是 UI，不是规则。** `caps.yaml.cargo_stack` 才是规则；引擎允许 `>1`。

### 2.6 确认 `CommitDeal`（body `0x140277090`）

RTC 局部：`anBuyTradingItemKindNum[19]`、`anSellTradingItemKindNum[19]`、`anSellTradingItemNum[105]`、`aulBuyTradingItemKindExpenses[19]`、`aulSellTradingItemKindExpenses[19]`、`ulTotalIncome`、`ulTotalExpenditure`。

`[确认]` 顺序：

1. `income = +0x1C`，`expense = +0x18`。两者都为 0 → 消息 `0x378`，返回
2. `expense > income` 且玩家势力金币 `< expense - income` → 消息 `0x380`，返回（金币 `fn_140002725` 读 `CForce+0x0C`）
3. 确认对话框（`fn_1400087fb(3, …)`）；否 → 返回
4. **会计长杀价**（下节）：会计长存在且 `+0x14 == 0` 时调 `fn_1400137b4(this, &income, &expense, purser)`，成功则两者被改写
5. 金币净额：`expense > income` → `fn_14001117b` 扣差额；否则 `fn_140002cb6` 加差额（封 99 999 999）
6. `PackToShip`（`fn_14000ea52`）
7. 统计：对每船 `n = CountHolds(10)` 个格：
   - 交易槽 `just_bought` → 买入种类件数 +1；买入种类金额 `+= fn_14000a691(town, item, hold.grade)`（**当前**报价）
   - 卖出记录 → 卖出种类件数 +1；卖出种类金额 `+= fn_140001b04(town_id, item, hold.grade)`；`anSellTradingItemNum[item]++`
8. 对 19 种：买入件数 > 0 → `fn_1400108b6(town, kind, n, 金额/n)`；卖出件数 > 0 → `fn_140007301(...)`（见 `02` §2.4）
9. 流行门槛 `thr = ceil(Σ CountHolds / 2)`（`0x1402776ea`）
10. 逐件（0–104）：卖出 ≥15 → 文化圈卖价回升（`02` §2.6）；同时记下两格流行品的本笔卖出件数
11. 特产品槽结算（`0x14027794f`–`0x140277b07`），交给 `04`：对每个可上架槽取 `n = +0x20[sp]`（零配额且未进超额模式时另有两条置 0 的保护分支）；若港口供给 ≠ 卡上剩余可买数则 `supply = supply - n`（`fn_14000ff65`）；`month_bought += n`（`fn_140005a6f`）
12. 杀价成功 → 消息（`0x14055e2b0`）
13. 流行品份额 / 信件（下面）
14. 卖出了流行品（两格件数和 `x > 0`）且上一步有结果 → 玩家势力在该港所属大海域（`major_sea_id`）影响力 `+x`（`fn_140010f46()==2` 即剧本 2 时 `x*2`；`eScenario` 证据见 `07` §2.1），消息 `0x12a`。重构：×2 是剧本包向 `factions.influence_gain`（`ctx.source: fashion_sale`）登记的贡献，由 factions 结算，economy 不乘
15. 成长：`fn_140004e17()` 所指人物（`[推断]` 主角）`fn_140006e88(·, 0xA, 1)`；会计长成功 `(0xA, 1)` 并置 `+0x14=1`，失败 `(5, 1)`
16. 流行计数 `fn_14000dc6a(town, slot, 件数)`（两格各一次）；刷新会话

流行品（原版写在 Commit 里，重构拆走状态）：

- `x0`、`x1` = 两格流行品本笔卖出件数；`x = x0 + x1`
- 玩家份额 `< 80`：
  - 两格都卖了：**两格都** `≥ thr` 才加
  - 只卖了一格：该格 `≥ thr` 才加
  - 加 `x / 6 + 1`（有符号 idiv）→ `fn_1400122f6(town, player_force, delta, 1)`，封到 `100-已占` 在 factions
- 份额 `≥ 80`：无信（`bd98==2`）且 `x ∈ [10,15]` 给介绍信、`x ≥ 16` 给推荐信；已有介绍信（`==0`）且 `x ≥ 16` 升推荐信
- 杀价成功与否只换消息号（`0x38f/0x391`、`0x390/0x392`）

这些数字是 factions 的参数（`formulas/factions.yaml` 的 `fashion_share_threshold` / `fashion_share_gain` / `fashion_letter`，`02-factions/03`、`02-factions/04`；词表 §14.9）；economy 只发 `SoldFashionGoods`，`share` / 信件 / 影响力由 factions 执笔。

### 2.7 会计长杀价 `fn_1400137b4`（body `0x140276c50`）

`[确认]` 反汇编：

```
purser = Fleets.post_holder(fleet, purser) # 原版 fn_140010cee(fleet, 3)；无人 = 0xEC → null
if purser == none or trade.bargain_used: skip
s = fn_1400135cf(purser, 4, 0x14)          # 与 fn_14000e1ce(person, 4) 同一取值器
chance = min(100, (s - 60) / 2)            # 有符号截断；s < 62 时 ≤ 0
if rand(100) >= chance: fail
bonus = min(15, (player_share(town) + 6) / 7)
income  = income  * (100 + bonus) / 100    # 无符号
expense = expense * (100 - bonus) / 100
success
```

- 份额 0 时 `bonus = 0`：杀价「成功」也不改金额，但仍置 `+0x14` 并给经验。
- 成功一次后本会话不再杀价（`+0x14`）。是否在别处清零 `[未知]`。
- 下标 4 是技能「会计术」（与 NPC 买货 `fn_14000e1ce(person, 4)` 同一取值器，`13-algorithms/01-formulas.md`）；岗位 3 = 会计长。第三参 `0x14` 的语义见 `03-characters`。

### 2.8 打开 / 打包

`UnpackFromShip`（`0x140279b80`）：清空 5×5 槽与卖出格；按 `eSpecialProduct` 0–4 调 `fn_140014899` 并清 `+0x20[sp]`、`+0x34[sp]`；合计清零；再从玩家五船 hold 生成槽（`just_bought=0`）；最后对等级 `< 6` 且非空的槽建买货栏卡 `k`（`fn_140001816(this, k, sp, item, grade, +0x48[sp])`）。

买货栏快照 `fn_14000efed`（`0x140279440`），每槽 `sp`：

```
price  = fn_14000a691(town, item, grade_rank)      # 当前买价
price2 = price * 2
avail  = fn_140001a37(sp)                           # 份额配额内可买，见 04 §4.2
if avail == 0 and month_bought == 0 and supply > 0:
    avail = 1; trial[sp] = 1                         # 零配额也能试买 1 件
sell   = fn_140001b04(town_id, item, grade_rank)
fn_14000833c(sp, 1, 1)                              # 会计长超额模式，见下
+0x48[sp] = avail
```

`fn_14000833c` `[确认 骨架]`：会计长存在、`fn_1400135cf(purser, 4, 0x14) >= 160`、玩家份额 `< 100`、港口供给 `> 0`，且配额内已买满（`avail == 0` 或本会话已买到 `avail`）并且本月/本会话确有买过 → 进入**超额模式**：买价换成 `price2`，可买数换成港口原始供给（扣掉本会话已买），`+0x5c[sp]=1`。`BuyOne` / 退货后都会重算。各分支的精确布尔组合见反汇编 `0x140278d00`–`0x14027920b`；重构按上句实现，边界用回放测试对齐。

`PackToShip`：只从 `CommitDeal` 调用。对每船 `n=CountHolds(10)`，按槽序把占用的 exchange 写回 hold。

`BuyOne(k)`（`0x14028ef50`）：

- `k` 是**买货栏卡下标**，经卡 `+0` 映射到特产品槽 `sp`；不是 `eTradingItem`，也不一定等于 `sp`
- 卡上可买数 `<= 0` 或当前船索引为 6 → 失败
- 原版只填空槽，且槽序 `< CountHolds`
- 卡上可买数 -1；写 `HoldRecord{item, grade_rank, town_id, +0x84[sp] 当前买价}`；`just_bought=1`
- `+0x18 += 买价`（封 99 999 999）
- `+0x20[sp]++`（超额模式下 `+0x34[sp]++`），重跑 `fn_14000833c`

**港口供给不在 BuyOne 扣**，在 `CommitDeal` 第 11 步一次结算。

点舱位：`just_bought` → 退货：重跑 `fn_14000833c`、`+0x20[sp]--`、清格、卡上可买数 +1、`+0x18 -= +0x84[sp]`（**当前**买价，超额模式切换会让它与当初付的不同）。否则进卖出列表，`+0x1C += fn_140001b04(town_id, item, hold.grade)`（`fn_14001073a`，封 99 999 999）。

### 2.9 舰队自动交易 / 委托

`fn_140010d25`（`0x1400f6d20`）：NPC / 地方舰队，见 `06`。玩家势力走真舱位。

委托 / 剧情只通过：

- `CountFleetItem(fleet, item)` — 玩家支原版数格；堆叠补丁改成加件数
- `AddLoad(fleet, item, …, n)` — `n>0` 原版一格一件；`n<0` 原版整格扣

重构后两函数都按件数，容量 = `ships × slots × cargo_stack`。

## 3. 实体与 ID

| ID | 原版 | 空 |
| --- | --- | --- |
| `eFleetShip` | 0–4，5 艘 | `INVALID_FLEETSHIP=-1`；会话里「当前船」6 = 无 |
| `eShipLoad` | 0–4，每船 5 货格 | `INVALID_SHIPLOAD=-1` |
| `eTradingItem` | 0–104 | 空格 106 |
| `eSpecialProduct` | 每港 0–4 | 槽内 item=106；`fn_14000680c` 未命中返回 6 |
| 买货栏卡 | 0..(可上架槽数-1) | — |
| 岗位 | 会计长 = 3 | 人物空 `0xEC` |

会话本身没有全局 ID，随 `ArrivedAtTown` 创建、关闭销毁。

## 4. 算法 / 公式

### 4.1 堆叠

```
qty(hold) = cargo_stack==1 ? 1 : (hold.count==0 ? 1 : hold.count)
can_merge(a, b) = cargo_stack > 1 and a.item==b.item and a.item != empty
                  and a.grade==b.grade and a.town_id==b.town_id
                  and qty(a)+qty(b) <= cargo_stack
```

合并必须同等级、同买入港——卖价按格里等级算（`02`），星品判定看等级（`05`）。原版包 `cargo_stack: 1` → 永不合并。不要用显示列宽倒推上限。

### 4.2 会话净额

```
if income == 0 and expense == 0: reject(nothing_to_do)
if expense > income and player_gold < expense - income: reject(no_gold)
if not confirm(): return
if purser and not session.bargain_used:
    (income, expense, ok) = bargain(income, expense)
player_gold += income - expense          # 夹 caps.gold_max
pack holds
```

`gold_max`（`caps.yaml`，本轮补登）原版 99 999 999（`fn_140002cb6`、`fn_14000e9c6` 的 `0x5F5E0FF`）。

### 4.3 会计长杀价

```
bargain(income, expense):
    # purser = Fleets.post_holder(fleet, bargain.purser_post)；无人则不杀价
    s = Modifiers.eval("skill.accounting", { person: purser })   # 技能值 0–skill_value_cap
    chance = min(bargain.chance_cap, (s - bargain.skill_floor) / bargain.skill_div)   # 100, 60, 2
    if rand_int(0, 99) >= chance: return (income, expense, false)
    bonus = min(bargain.bonus_cap, (player_share(town) + bargain.share_bias) / bargain.share_div)   # 15, 6, 7
    return (income * (100 + bonus) / 100, expense * (100 - bonus) / 100, true)
```

### 4.4 退货

```
refund_gold = session.slot_price[sp] * qty     # 当前买价，含超额模式
session.bought[sp] -= qty
card.avail += qty
clear exchange slot
```

按当前买价，不按 hold 里记下的买入价。港口供给未动过，无需补。

### 4.5 流行品事件参数（原版缺省；归 `formulas/factions.yaml` 的 `fashion_share_threshold` / `fashion_share_gain` / `fashion_letter`，此处只作对照）

```
hold_sum = Σ CountHolds(fleet_ship)
threshold = ceil(hold_sum * fashion_threshold_numer / fashion_threshold_denom)   # 1/2
pass = (both sold) ? (x0 >= threshold and x1 >= threshold) : (x0 >= threshold or x1 >= threshold)
share_delta = (x0 + x1) / fashion_share_div + fashion_gain_add    # div=6, add=1
letter_intro_min = 10
letter_intro_max = 15
letter_recommend_min = 16
letter_upgrade_min = 16          # 介绍信升推荐信，独立立即数 0x14027829d
share_letter_cut = 80
influence_base = x0 + x1        # factions 再经 factions.influence_gain（ctx.source: fashion_sale）修正；economy 不乘
```

经济只 `emit SoldFashionGoods { town_id, force_id, items[], qtys[], hold_sum, bargain_ok }`。

### 4.6 NPC 买货

见 `06`。自动交易调用 `quote` + 提督会计术技能值（`Modifiers.eval("skill.accounting", …)`），再 `Ships.add_load` / 账本 `expense`。

## 5. 耦合分析

原版 `CTrade` 直接：

- `GetPlayerFleet` / `CShip::CountHolds`
- 读本港特产品、配额、份额；确认时写供给
- 改金币、改种类市价、改文化圈卖价、改份额、发信、改影响力、给经验
- 读剧本号（`fn_140010f46`）决定影响力倍数（重构：`factions.influence_gain` 修正器，factions 侧结算）
- 读会计长岗位与技能

重构允许的方向：economy ← ships.hold（拷入拷出）、economy ← Modifiers 注册表（只读，按 key）、economy ← factions.share（只读）、economy → 事件给 factions / town / progression。

禁止 economy 改人物经验（发 `TradeCommitted`，progression 自己给）、禁止 UI 写 hold。

## 6. 解耦后的配置

```yaml
# config/caps.yaml
fleet_ships: 5
hold_slots_per_ship: 5
cargo_stack: 1          # 原版包；引擎可 >1
gold_max: 99999999
```

```yaml
# config/engine_limits.yaml
cargo_stack_max: 255    # 存档 u8；不是 UI 的 9
hold_slots_max: 32
fleet_ships_max: 16
```

```yaml
# config/formulas/economy.yaml  （会话相关段）
commit:
  impl: vanilla.commit
  params:
    same_item_adjust_qty: 15   # 文化圈卖价回升，公式在 02
bargain:                       # 会计长杀价（§4.3）
  impl: vanilla.bargain
  rng_stream: economy
  refs:
    purser_post: vanilla:purser   # 岗位 key；原版 3
    skill: vanilla:accounting     # 原版 fn_1400135cf(person, 4, 0x14)，eSkill 4
  params:
    chance_cap: 100
    skill_floor: 60
    skill_div: 2
    bonus_cap: 15
    share_bias: 6
    share_div: 7
    once_per_session: true
over_quota:                    # 会计长超额模式
  impl: vanilla.over_quota
  params:
    skill_need: 160
    share_below: 100
    price_mult: 2
trial_buy:
  impl: vanilla.trial_buy
  params:
    qty_when_no_quota: 1
# 流行品门槛 / 份额 / 信件参数归 formulas/factions.yaml（fashion_share_threshold / fashion_share_gain / fashion_letter，02-factions/03、04；词表 §14.9），本文件不重复定义（§4.5）
```

技能值由 characters 经修正器注册表给出（原版 `fn_1400135cf(person, 4, 0x14)`，4 = `eSkill` 会计术），经济只按 key `skill.accounting` 取，不读下标。没有「列宽」键。空货是 `null`，不在 caps 里放 106。

## 7. 独立旋钮

- `cargo_stack`（1 → 99 只改 caps 与存档宽度）
- 每船货格、每舰队船数（内容包；引擎按表长）
- 会计长杀价曲线、超额模式门槛与倍价
- 流行品门槛与信件阈值（factions 参数，见 §4.5）
- 文化圈回升件数 15

改堆叠不必改市价公式。改船数不必改 105 件目录。

## 8. 对外接口

### 命令

```
Trade.open(fleet_id, town_id) -> session_id
Trade.buy(session_id, card_index, qty)
Trade.click_hold(session_id, ship_index, slot_index)   # 刚买则退，否则挂卖
Trade.sell_all(session_id)
Trade.withdraw_sell(session_id, sell_index)
Trade.commit(session_id) -> Result{ ok | nothing_to_do | no_gold | declined }
Trade.cancel(session_id)                               # unpack 丢未确认
```

### 向船

```
Ships.holds(fleet_id, ship_index) -> [ { item_id, qty, grade, bought_town_id, buy_gold } ]
Ships.replace_holds(fleet_id, ship_index, holds)       # 仅 commit
Ships.count_item(fleet_id, item_id) -> int             # 按件数
Ships.add_load(fleet_id, item_id, qty, buy_gold)       # qty 可负
```

Live 取船；舰队不存在则整个会话 no-op。hold 里不存 `World*` / `Player*`。

### 向人物 / 势力 / 特产品

```
Modifiers.eval("skill.accounting", { person }) -> int   # 会计长、超额模式门槛
Fleets.post_holder(fleet_id, post_key) -> person_id | null
Share.of(town_id, force_id) -> int                     # factions
Economy.special_offer(town_id, slot, force_id) -> { price, price_over, avail, supply, month_bought }   # 04 §8
Economy.settle_special(town_id, slot, qty, counts_toward_quota=true)                         # 04 §8
```

### 事件

```
TradeOpened      { session_id, fleet_id, town_id }
TradeCommitted   { fleet_id, town_id, bought[], sold[], gold_net, bargain: none|ok|fail, purser_id }
SoldFashionGoods { town_id, force_id, items[], qtys[], hold_sum, bargain_ok }
SoldBulkItem     { town_id, item_id, qty }            # qty≥same_item_adjust_qty
TradeCancelled   { session_id }
```

`SoldFashionGoods` 由 factions 决定是否加份额、发信、加影响力。经济不写 `share`。`TradeCommitted` 由 progression 决定给谁多少经验（原版主角 `0xA`，会计长成功 `0xA` / 失败 `5`）。

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| `CTrade+0x14` 何时清 0 | ctor 只见 `+0x32c`；会话复用时是否重置 `[未知]` |
| `fn_14000833c` 全部分支 | 骨架已钉，布尔边界未逐条抄 |
| `fn_1400135cf` 第三参 `0x14` | 取值器上下文，人物文档定 |
| `CTrade` `+0x32c`、`+0x3e0`、`+0x4b0` | 控件指针 / 未命名 |
| `+0x588` 两个对象的指向关系 | `[推断]` 见 §2.3 |
| `CountHolds(10)` 的 10 与 5 舱室的精确关系 | 扫描上限 10，交易所 5 |
| 仓库 `CWarehouseView` 是否另一套持久库存 | 有 View；会话卖出列表不是仓库 |
| World1 是否同一 5×5 | 会话逻辑在 exe，与世界包无关 `[推断]` |
