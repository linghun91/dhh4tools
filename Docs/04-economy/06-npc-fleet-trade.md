# NPC / 地方 / 委任舰队自动交易

到港后的**无窗买卖**：先卖掉整船货，再按特产品利润决定买什么、买几件、记到账本或势力金库。原版入口 `fn_140010d25`（body `0x1400f6d20`）。

玩家在交易所点的那一次会话是 `03-trade-loop.md` 的 `CTrade`。本篇**不打开** `CTrade`，不走 unpack / 退货 / 确认 / 取消，也不发介绍信、推荐信。

## 1. 职责边界

本系统管：

- 舰队已到港且目标种类为「交易」时的自动卖 → 清舱 → 选下一港 → 买特产品
- 买入成本（`quote` × 件数 × `npc_buy_cost`，提督会计术技能值经 `Modifiers.eval("skill.accounting", …)` 只读）
- 玩家势力走真舱 `HoldRecord`；其他势力走 `CFleet+0x74` 紧凑货记
- 卖出后改种类指数、上报流行件数（份额由 factions 执笔；**不发信**）
- 有账本记 `income` / `expense`；无账本改势力金币

不管：

- 玩家交易所会话、堆叠 UI、退货、`CommitDeal` 发信（`03`）
- 市价公式本体（`02.quote`；本系统只调用）
- 特产品 5 槽定义、配额与供给（`04`；本系统只调 `Economy.buy_cap` / `Economy.settle_special`）
- 地方舰队怎么建、100000 建队金、命令 UI（`05-ships-fleet/02-fleet-proxy.md`）
- 月结滚动、付不起转下月的**整段**日历（`04` 账本 `Ledger.*`；本系统只往账本加减）；上缴不在舰队月结，见 `07`
- 人物六维 / 技能下标（禁止读 `eAbility` / `eSkill` 下标；只按 key `skill.accounting` 取技能值）
- 委托 / 剧情扣货（仍走 `Ships.count_item` / `Ships.add_load(fleet_id, item_id, delta)`，`03` 已约定按件数）

拆分：`03` 是玩家点的会话；本篇是 NPC 循环。两边可以共用 `HoldRecord` 布局和 `quote()`，**状态机不得合并**。

## 2. 原版事实

来源：`srcdata/docs/exe_caps.md`、`srcdata/docs/trade_stack.md`、`dhh4tools/README.md`、`srcdata/include/dk4_types.h`、`srcdata/source/game/unassigned/fn_1400f2f80.cpp`（`fn_140010d25`）、sc 反汇编 `0x1400f6d20` / `0x1400f9f90` / `0x1400ff2c0`、RTC `catalog/rtc.json` `0x1400f6d20`。

### 2.1 谁调用、何时走

唯一 xref：`fn_140007f4a` body `0x1400f9f90` 在 `0x1400fa273` `call fn_140010d25`。

到港判定 `[确认]`：当前港 `fn_1400094fd`（`CFleet+0x73`）== 目标港 `fn_14000c144`（`+0x70`，若 `+0x80` 某标志则当作空港 `0x62`）。未到港走航行/补给分支，不交易。

到港后按 `fn_14000aeac`（`CFleet+0x72`，`eFleetAimKind`）跳表 0–9：

| 下标 | 入口 | 本篇 |
| --- | --- | --- |
| 1 | `fn_140010d25` | **自动交易** `[确认]` 跳表 `0x1400fa49c` |
| 0, 2–9 | 其它（寻路、战斗、连港…） | 不管 |

`NUM_FLEETAIMKIND` 整数未还原；跳表按 0–9 处理，故原版至少 10 档 `[确认 观察]`。下标 1 的枚举名未恢复，重构 key 用 `trade` `[推断]`。

谁把 aim 写成 1：未钉。玩家委任命令文案（买仓库货、护送、催债）在 `02-fleet-proxy.md`，**不是**本跳表的命名来源。

提督空（人物 `0xEC`）时 `fn_14000d599` 返回势力哨兵 `0x1A=26`，与合约空势力同一哨兵 `[确认]` `03-relations-influence.md`。重构里两者都是 `null`（DECISIONS D1）。

### 2.2 一次循环的顺序

RTC 局部：`asShipLoad[25]`（200 B）、`anKindAmount[19]`、`ulKindPriceTotal[19]`、`anFashionSellAmount[2]`、`ePurchaseTradingItem`、`eSpProduct`、`aeProfitSpecialProduct[5]`、`eTownFashion`。

`[确认]` 反汇编顺序：

1. 读当前港对象、势力 id、势力对象、账本指针；卖出合计清零
2. `fn_140007266` 把货物展开进 `asShipLoad`，返回件数 `n`
3. **若 `n>0`（有货才卖）**
   - 逐件：卖价 `fn_1400097af`、种类指数 `fn_1400095ed`；指数 `< 0x3C`（60）把该 item 记进局部
   - `anKindAmount[kind]++`，`ulKindPriceTotal[kind] += 卖价`，总卖金累加
   - `fn_14000b5be(town, item)` 得流行槽 0/1；返回 3 则不是本港流行，不加 `anFashionSellAmount`
   - 对 `anKindAmount>0` 的种类：`fn_140007301(town, kind, count, avg_price)` 改指数（与 `03.CommitDeal` 卖出同类）
   - `fn_140013192(town, force, anFashionSellAmount, 舱位容量)` 流行结算
   - `fn_1400013ca` **整舱清空**
   - 有账本：`fn_14000ff88` 收入 `+=` 卖金（账本 `+0x10`）
   - 无账本、有势力对象：`fn_140002cb6` 势力金币 `+=` 卖金
4. **无势力对象**：`fn_14000628a` 另选连港，结束（不买）
5. **买**：`fn_140013ee4(fleet, 当前港, &ePurchaseTradingItem)` 选出**下一港**（或 `0x62` 失败）
6. 失败：`fn_14000632a` 随机连港，结束
7. 成功：把目标港写成该镇；在**当前港**特产品里挑能赚钱的槽；算件数、扣供给、装货、记支出
8. 再按份额决定是否留下 / 换目标（`fn_1400072cf` / `fn_14000c621`；控制流见 §2.5 末，`c621` 体内 `[未知]`）

没有 `ExchangeSlot`、没有 `just_bought`、没有取消。卖是全卖；买是一次一槽特产品。

### 2.3 两种货物存法

`fn_14000e638`：舰队势力 == 玩家势力 `[确认]` `trade_stack.md`。

| 谁 | 存哪 | 展开 `7266` | 清空 `13ca` | 装货 `6294` |
| --- | --- | --- | --- | --- |
| 玩家势力（主队、地方舰队、属下 NPC） | 每船 `HoldRecord`：`ship+0x58+j*8`，与玩家交易所同一结构 | 5 船 × 5 格，占用格拷 8 字节 | 每格 `fn_140006474` | 逐空格 `fn_140005baf` 写一格 |
| 其他势力（别的商会、海盗等） | `CFleet+0x74` 一条 8 字节记录 + `+8` 的 u8 数量 | 把同一 8 字节复制 `qty` 次进栈数组 | `fn_14000c662` 清 `+0x74`（与 `CTrade` 清空同一函数） | `fn_140014d80` 写记录 + `fn_1400086c0` 写数量 |

`[确认]` `dk4_types.h` `HoldRecord`：`+0` item（空=`0x6A`）、`+1` b1、`+2` b2、`+3` count（原版不用）、`+4` u32 买入价。`GetLoad` `fn_14000a6aa` = `ship+0x58+j*8`。`ClearHoldRecord` `0x1400fce20`：item=`0x6A`，`+2=0x62`，价=0。

紧凑数量 `[确认]`：`fn_14000a038` 读 `record+8` 的 **u8**，`fn_1400086c0` 写该字节。记录基址是 `fleet+0x74`，故数量在 `CFleet+0x7C`。不是玩家格的 `+3`。

`dhh4tools/README.md`：「其他势力的货物存法不同，保持原版。」堆叠补丁 **只改玩家势力** 真舱：容量 `8d41×N`、卖价 ×件数、装货每格 `min(N, 剩余)`。紧凑支 `7266` 靠「复制 N 份记录」本来就是按件数，未改。

栈数组 `asShipLoad` 200 字节 = 25×8。紧凑 `qty` 是 u8（最多 255）。`qty>25` 是否会写出数组 `[未知]`；原版是否先夹到 25 `[未知]`。重构夹到 `compact_expand_slots`（原版包 25）。

玩家势力地方舰队：**用同一套 `HoldRecord`**，不是紧凑记。其他势力：**结构同形的一条记录，外加独立数量字节**。

### 2.4 卖价、指数、流行

卖价：`fn_1400097af(town, item, hold.b1)`。装货时 b1 来自 `fn_140015168(town, slot)`（槽等级；品牌判定 `==1`，见 `04`）。这与玩家舱位展示卖价 `fn_140001b04(..., stored_buy_price)` **第三参不是同一字段** `[确认 传参]`。报价仍进 `02` 的 `fn_140010492`。

种类指数 `< 60` 时把该 item 写入局部 `s148`。本函数后半**没有再读**这个局部 `[确认 只写]`。`02` 写「优先补货该种类」——若补货发生，不在本函数可见部分。标未核实，禁止当买货条件实现。

流行：`CTown+0x70` 两格。`00b5be` 匹配则下标 0/1，否则 3。`013192` 汇总两格件数，取件数最多的那件，调 `fn_14000dc6a`（流行槽计数，语义 `[未知]`）和 `fn_14000d891`。

`00d891` `[确认]`：

- `fn_1400092af(town, force)` 得份额；`< 0x50`（80）才继续
- 且 `sold >= floor(capacity/2)`（`008d41` 的舱位和；`cdq; sub; sar 1` 向 0 取整，比较是 `capacity/2 > sold` 则跳过）才把增益写成 **2**
- 然后 `fn_1400122f6(town, force, 2, 1, 0)` ——与 `CommitDeal` **同一个加份额函数**，但增益固定 2，不是 `sold/6+1`
- 份额 `≥80`：整段跳过，**不加份额**
- **没有** `bd98` / 介绍信 / 推荐信 分支
- 加份额后若该港对玩家可见（`fn_14000c65d`，`CTown+0x10` bit 1）且 `fn_14000e52a(force) > 0`：消息 `0x66`（mesfile `0xaf7`「在%s，%s大量推销%s，扩大了占有率！」）；玩家原有份额 `>0` 时后面还有一段（`0xb1b`「%s的占有率下降了！」一类）`[推断]`。只是播报，不改数

`DK4FSH1` 只补 `CTrade::CommitDeal`。舰队自动交易发信不受该补丁影响，因为这里本来就不发信。

重构：经济 `emit NpcFashionSold`；factions 用 **NPC 规则** 加份额；`grant_letters: false`。禁止复用玩家 `SoldFashionGoods` 的信件门槛。

### 2.5 买：下一港、买哪件、几件

`fn_140013ee4`（body `0x1400f3640`）`[确认 骨架]`：

- 势力 `==0x1A` 或没有提督 → 返回空港 `0x62`
- `fn_14000e1ce(提督, 4)` = 提督**会计术**技能值 `[确认]`（`eSkill` 4；`fn_1400099e9` 断言 k<10、查技能配对表 `.data 0x1406864a0`）。两道门**依次**试，前一道为空才走下一道：
  - `≥ 0xB4`（180）：当前港两格流行品，`fn_140005c22` 把两格的候选港**合并**后随机取一座。不写 `ePurchaseTradingItem`
  - `≥ 0xC8`（200）：当前港特产品槽 **0→4 依次**，`fn_140014060` 收候选；**第一个有候选的槽**就定局：写 `*ePurchaseTradingItem = 该槽 item`，随机取一座
  - 否则 / 上面为空：`fn_140012800(…, sea)` → `fn_140009bb0(…, sea)` → `fn_140009bb0(…, 8)`，依次到非空为止（筛选内容 `[未知]`）
- 仍空 → `0x62`

搜索范围参数 `[确认]`：舰队属玩家势力时，流行/特产品两道门传当前港的 `major_sea_id`（`fn_140014e48`，经 town→culture→`CCulture+0x10`，`02-factions` 已钉）；其他势力传 **8**。`8` 在 7 个大海域下标之外，`[推断]` 「不限海域」。回退链三步对所有势力都是先本海域、最后 8。

买货发生在**当前港**。选出的镇是**卖货目标 / 下一站**。当前港特产品：

1. 若 `ePurchaseTradingItem != 空` 且 `fn_14000680c` 命中本港槽（0–4；未命中返回 6）→ 用该槽
2. 否则扫 5 槽，把「当前港买入价 `00a691` < 目标港卖价 `0097af`」的槽放进 `aeProfitSpecialProduct`，`fn_140005272` 随机取
3. 仍是 6 → 不买

件数 `[确认 夹紧顺序]`：

```
unit = max(1, quote_gold(当前港, 该槽))     # fn_140001735 → 010492；再 fn_1400013cf(1, ·)
cap  = fn_140008d41(fleet)                  # 五船 CountHolds(10) 之和（真舱位数，不是紧凑 qty）
qty  = fn_140013345(town, slot, force_id)   # 份额配额，见下
qty  = min(qty, cap)
if ledger:
    qty = min(qty, ledger_buy_budget_gold / unit)    # 立即数 0x186A0=100000
else:
    qty = min(qty, (force_gold * 4 / 5) / unit)      # 花金币的 80%；shl 2 后无符号 div 5
```

`fn_140013345`（body `0x140261f10`）`[确认]`：

```
supply = CTown+0x30+slot*6                 # fn_140001ab4，当前供给 u8
base   = GetFixTown.special[slot]+4        # fn_14000c3f1，基础供给 = supply_base（旧 JSON grade）
aux    = CTown+0x31+slot*6                 # fn_140009b0b，month_bought（玩家势力本月已购）
quota  = base * share(town, force) / 100   # 有符号 idiv 0x64
if force == player_force:
    return max_s(0, min_s(supply, quota - aux))
else:
    free  = 100 - Σ share(非玩家势力)        # fn_140005e1b = 玩家份额 + 无主份额
    keep  = base * free / 100              # 留给玩家/无主的那份
    avail = min_s(supply, supply - keep + aux)
    return max_s(0, min_s(avail, quota))
```

即：舰队通道每座港每槽每月的可买量 = 基础供给 × 份额%，**不乘**商业系数（玩家亲买的 `fn_140001a37` 乘，见 `04` §2.3）。玩家势力扣掉本月已购；NPC 不能吃掉「玩家 + 无主」那一份里玩家还没买走的部分。份额 0 → 配额 0 → 不买。重构由 `04` 的 `Economy.buy_cap(town, slot, force, npc_proxy)` 统一给出。

有货可买时：

- `fn_1400108b6` 改买入种类指数（与 `03.CommitDeal` 买入同类）
- `raw = unit * qty`
- 有提督：`cost = raw * (1000 - Skill(提督, 会计术) + rand(100)) / 1000`，再 `fn_140006e88(提督, 0xA, 1)`（经验调用，与 `03` Commit 后主角 `(0xA, 1)` 同函数；0xA 语义归 progression）
- `fn_140006294` 装货（b1=槽等级，b2=`0x62`，价=0）
- `fn_14000712b` 扣特产品记录 **+0** 供给（`town+0x30+slot*6` 的 u8）
- 有账本：`fn_140005a6f` 把件数加到特产品记录 **+1**（`009b0b` 读 `town+0x31+slot*6`，**不是同一字节**）+ `fn_140005849` 支出 `+= cost`（账本 `+0x0C`）
- 无账本：`fn_14001117b` 扣势力金币

`005a6f` 不是「加供给」：两列偏移不同，禁止实现成「刚扣的供给加回去」。+1 列 = `month_bought` `[确认]`：月初补货 `fn_140008aee` 清 0（`04` §2.5）；`013345` 与玩家会话配额 `fn_140001a37` 都从配额里扣它；写入有两处——本函数（仅有账本时，`0x1400f74ef`）与玩家 `CommitDeal`（`0x140277af2`）。所以它是**玩家势力本月已购**，玩家亲买与玩家代理舰队共用一个计数。

扣供给 `fn_14000712b`：`supply = (u8)max_s(0, supply - qty)`。

`qty<=0` 且**非玩家势力**时 `[确认]`：

```
if base/2 < aux:                                  # c3f1/2 有符号；aux=+0x31
    fn_1400035ee(CForceFailure[major_sea_of(town)], force_id, 1, town_id, item_id)
```

即 NPC 买不到、且玩家势力本月已买走超过基础供给一半时，往该大海域的 `CForceFailure` 记一条采购失败（写入对象见 `02-factions/05` §2.4）。它之后被谁读、产生什么玩法 `[未知]`；本循环发 `NpcPurchaseFailed` 事件，记录归 factions。

成交后（无论买没买到）`[确认]`：

```
room = fn_1400072cf(town, force, 1)   # 本势力在槽内 且 调整后份额合计 < 100；据点港对非玩家恒 0
go = true
if room == 0:
    if force == player_force: go = false
    elif rand(2) == 0: go = false
    elif rand(1000) < commerce/10: go = false
    elif rand(1000) < military/10: go = false
if go:
    fn_14000c621(fleet, town_id, max_s(0, sell_total - buy_cost), room)
```

`0072cf` 的「调整后合计」：本势力份额 + 他势力份额；他势力与本势力关系 `fn_140012dc8 == 2` 时只计 `min(份额, fn_14000cffe)`（该港是对方本据港才非 0）。关系 2 的名字 `[未知]`。`00c621` 体内 `[未知]`。

### 2.6 买入成本与 500 夹紧

sc `0x1400f742c` `[确认]` `exe_caps.md`、`dhh4tools/dk4save.c` `PROXY_BUY_*`：

```
a    = fn_14000e1ce(提督, 4)     # 会计术技能值，已按 skill_value_cap（500）夹过
cost = unit * qty
cost = cost * (1000 - a + rand(100)) / 1000    # 32 位 imul + 无符号 div
```

`rand(100)` 为 `[0, 100)`。`a=500` 时约五折；`a>1000` 括号为负，无符号除法变成几百万。月报亏损来自这里，不是卖价公式。

补丁：先把 `a` 夹到 500 再减。不改技能值上限，不改战斗 `500-a`、道具 `100-a/50`。

下标 4 = `eSkill` 会计术 `[确认]`（见 §2.5）。**本系统不读下标**，按 key `skill.accounting` 取值。

无账本时直接扣势力金币；有账本只记账，金币在月结从势力账户兑付。

### 2.7 账本与月结

账本 `[确认]` `exe_caps.md`：

```
CFleetElement + 0x3738 + idx * 0x18
+0x00..0x0B  上月三元组
+0x0C        expense   fn_140005849  body 0x1400ea7f0
+0x10        income    fn_14000ff88  body 0x1400ea8c0
+0x14        investment
balance = income - expense - investment
```

`fn_14000c342`：取该舰队账本；某下标 `==7` 则空指针（不当作委任账本）`[确认 观察]`。`NUM_PROXYFLEET` 未还原；6 槽是月份窗口还是 6 支委任 `[未知]`（`04` / `02-fleet-proxy`）。

`CFleetElement::vfunc_04` body `0x1400ff2c0`–`0x140100cb1`。`0x140100180` 附近 `[确认]`：

- `fn_140005164` 读余额
- 余额 `<0` 且 `|余额| >` 势力金币：把金币扣光，差额 `005849` **加回支出**（下月继续欠）
- 否则 `002cb6` 把余额并进势力金币（正为进账，负为兑付）
- 付不起时置标志，可发消息

本系统不重写整段月结。只保证买卖写入的 `income`/`expense` 能被 `04.month_roll` 消费。附庸 1% 在 `CForceElement::vfunc_04`、独占上缴在 `CTownElement::vfunc_04`，**都不在**本函数（见 `07`）；世界 tick 每拍无条件调 `CFleetElement::vfunc_04`（`0x1402aae0f`），月账门在其体内。

### 2.8 建立地方舰队 100000

mesfile `0x2f33`：「建立地方舰队需要１０００００枚金币。」`[确认]` 建队扣玩家金，属舰队系统。

本函数里的 `0x186A0` 是**有账本时买货预算**（`100000 / 单价` 件数上界），不是再建一支队。两个 100000 同立即数，**不一定**同一设计意图；配置里分成两键，禁止偷用建队金当买货预算，除非策划明确合并。

建队成功的是**玩家势力**舰队 → 真舱 + 账本 + 本循环。其他势力舰队不花这 100000，用势力金库。

### 2.9 与玩家 `CTrade` 对照

| | 玩家会话 `03` | 本循环 |
| --- | --- | --- |
| 对象 | `CTrade` 0x330 | 无会话对象 |
| 卖 | 点舱 / 全卖，确认才清 | 到港有货即全卖并清舱 |
| 买 | 特产品栏，可退 | 一次一槽，不可退 |
| 堆叠 | 会话槽 `+3` / `just_bought` | 真舱走 hold；紧凑走 `+8` u8 |
| 流行 | `CommitDeal`：份额或**发信** | `013192`：份额 +2 或什么都不做，**不发信** |
| 金币 | 玩家金库净额 | 账本或势力金库 |
| 成本修正 | 玩家买入不走 `1000-a` | 走 `npc_buy_cost` |

## 3. 实体与 ID

| 实体 | 原版 | 空 / 哨兵 | 重构 key |
| --- | --- | --- | --- |
| 舰队 | `eFleet` 0–68 | `INVALID_FLEET=-1` | `fleet_id` |
| 势力 | `eForce` | 空 `26`（`0x1A`） | `force_id`；空 = `null` |
| 港口 | `eTown` | 非法镇 `98`（`0x62`） | `town_id`；空 = `null` |
| 交易品 | `eTradingItem` | 货格空 `106` | `trading_item_id`；空 = `null` |
| 特产品槽 | 0–4 | 未命中 **6**（不是 −1） | `special_slot`；未命中 = `null` |
| 流行槽 | 0–1 | 非流行 **3** | `fashion_slot`；非流行 = `null` |
| 目标种类 | `eFleetAimKind` | `INVALID=-1`；交易观察值 1 | `fleet_aim_kind_id` |
| 账本槽 | 6 × `0x18` | 取不到 = 无账本 | `ledger_slot` |
| 货 | 玩家：`HoldRecord[ships][slots]`；其他：紧凑 1 条 + `qty` | item=106 | `CargoView` |

运行时读表长。不要写死 69 / 25 / 5。原版包实例计数仍是那些数。原版哨兵列只用于导入 / 导出（`ids/legacy_codec.yaml`），运行时一律 `null`（DECISIONS D1）。

没有独立的 `npc_trade_session_id`。一次 `simulate` 同步完成。

## 4. 算法 / 公式

全部立即数进 `config/formulas/economy.yaml` 的 `npc_buy_cost` / `npc_loop`。代码禁止出现 1000、500、100、60、80、180、200、100000。

人物只经修正器注册表提供技能值：`a = Modifiers.eval("skill.accounting", { person: admiral })`（原版 `Skill(提督, 会计术)`，0–`skill_value_cap`=500）。提督 = `Fleets.admiral(fleet_id)`（提督不是岗位，`03-characters/03`）。本系统再按 `npc_buy_cost.factor_cap` 夹一次后算倍率。禁止读六维。公式只在 economy 算（原版算在 `010d25`）；不要人物算一遍、经济再算一遍（DECISIONS D2）。

### 4.1 触发

```
can_run(fleet_id) =
    Fleets.exists(fleet_id)
    and Fleets.location(fleet_id).town_id == Fleets.dest_town(fleet_id)
    and Fleets.dest_town(fleet_id) is not null
    and Fleets.aim_kind(fleet_id) == aim_trade
```

舰队或港口取不到：整次 no-op。不缓存 `World*` / `CFleet*`。

### 4.2 展开与件数

```
qty_of(hold) =
    cargo_stack == 1 ? 1 : (hold.count == 0 ? 1 : hold.count)

expand(fleet):
    if force_of(fleet) == player_force:
        copy occupied HoldRecord from each ship slot   # 玩家势力；堆叠时按件数拆或按格×count
    else:
        rec = compact_record(fleet)                    # fleet compact at +0x74
        if rec.item is null: return []
        repeat rec, n = compact_qty(fleet)             # u8 at +8；夹到 compact_expand_slots
```

原版包 `cargo_stack: 1` 时玩家支与数格等价。引擎按件数。

### 4.3 卖

```
for rec in expand(fleet):
    gold = Economy.quote(town_id, rec.item).sell_gold     # 原版 0097af(town,item,b1)
    kind = kind_of(rec.item)
    kind_count[kind] += qty_of(rec)
    kind_gold[kind]  += gold
    total_sell       += gold
    fslot = fashion_slot(town_id, rec.item)               # 0/1 或 null（原版 3）
    if fslot is not null: fashion_qty[fslot] += qty_of(rec)

for kind if kind_count[kind] > 0:
    Economy.on_kind_traded(town, kind, sell, kind_count[kind], kind_gold[kind])

emit NpcFashionSold { town_id, force_id, qtys[2], hold_capacity }
clear_cargo(fleet)
credit(total_sell)    # 账本 income 或势力金币
```

factions 收 `NpcFashionSold`：

```
if share(town, force) < npc_fashion.share_cut:          # 80
    if sum(qtys) >= floor(hold_capacity * numer / denom): # 原版 capacity/2 向 0 取整
        apply_share(town, force, npc_fashion.gain)       # 2
grant_letters = false
```

### 4.4 选下一港 / 买哪件

```
a = Modifiers.eval("skill.accounting", { person: admiral })
dest = pick_next_town(fleet_id, current_town_id, a)
# 原版门槛 180 / 200 用的是 Skill(提督, 会计术)，与买货折扣同一输入
# 比 npc_loop.pick_fashion_min / pick_special_min；范围参数 = major_sea_id 或 any
# 候选表 005c22 / 014060 / 012800 未还原前 pick_source: unverified

if dest is null: pick_connect_town(fleet); return

slot = special_slot_of(current, hinted_item)            # null = 无（原版 6）
if slot is null:
    cand = [s for s in 0..4
            if special_item(current,s) is not null
            and quote(current, item).buy_gold < quote(dest, item).sell_gold]
    if cand empty: skip buy
    slot = cand[rand(len(cand))]
```

### 4.5 件数与成本

```
unit = max(quote_unit_min, quote(current, special_item(current,slot)).buy_gold)
qty  = Economy.buy_cap(town, slot, force_id, npc_proxy)   # 013345；supply_base*share/100，§2.5
qty  = min(qty, hold_capacity(fleet))
if has_ledger(fleet):
    qty = min(qty, ledger_buy_budget_gold / unit)
else:
    qty = min(qty, force_gold(force) * gold_spend_numer / gold_spend_denom / unit)
if qty <= 0: skip buy

a = clamp(a, 0, npc_buy_cost.factor_cap)                  # 500
term = buy_cost_scale - a + rand_int(0, buy_cost_jitter)   # jitter=100 → [0,99]
if term < 0: term = 0                                      # 有符号饱和；禁止无符号 div
cost = saturating_mul_div(unit, qty, term, buy_cost_scale)

ledger = Ledger.of(fleet)
Economy.settle_special(town, slot, qty, counts_toward_quota = ledger != null)   # supply -= qty；有账本才 month_bought += qty
if ledger: Ledger.post_expense(ledger, cost)
else:      Force.add_gold(force, -cost, npc_trade)
emit NpcTradeCommitted { fleet_id, admiral_id, ... }       # progression 据此给经验（原版 006e88(提督,0xA,1)）

place_cargo(fleet, item, qty, grade=grade_rank(slot), bought_town=null)
Economy.on_kind_traded(town, kind, buy, qty, unit*qty)
```

`hold_capacity`：玩家势力 = `Σ CountHolds(scan)`，堆叠时 × `cargo_stack`。其他势力原版仍用五船 `CountHolds` 当件数上界，落货却写紧凑数量。重构可配 `other_force_capacity: cargo_room_sum | compact_qty_max`。

### 4.6 月结（本系统只认账）

```
# 04 已写。这里只声明买卖写入的字段
ledger.income  += sell_gold
ledger.expense += buy_cost
# vfunc_04 @ 0x140100180：付不起的支出滚下月
```

## 5. 耦合分析

原版硬编码：

- `010d25` 直接 `fn_14000e1ce(提督, 4)`（会计术）、直接改 `CTown` 指数 / 特产品字节、直接改账本与势力金币
- `e638` 把「是不是玩家势力」焊进展开 / 装货 / 清空三支
- 流行加份额调用与 `CommitDeal` 同函数 `0122f6`，但参数不同、无发信
- 月结在 `CFleetElement::vfunc_04`，与补给、旗舰挤在同一虚函数（上缴不在这里，见 `07`）
- 供给配额读份额槽、静态基础供给、`month_bought` 三处（`013345`），补货清计数在城镇月初（`CTownElement`）
- 紧凑货挂在 `CFleet` 对象上，和阵型、aim、驻港字节挤在 `+0x70` 一带

重构允许的方向：

```
ocean / fleets  --ArrivedAtTown + aim-->  economy.NpcTrade
NpcTrade        --quote / on_kind_traded-->  economy.Price
NpcTrade        --buy_cap / settle-->        Economy.buy_cap / settle_special (04)
NpcTrade        --holds / compact-->         ships / fleets
NpcTrade        --skill.accounting-->        Modifiers 注册表         (只读)
NpcTrade        --income/expense-->          economy.Ledger (04) | Force.add_gold (factions)
NpcTrade        --NpcFashionSold-->          factions.Share           (不写信)
```

禁止：经济改人物经验（`006e88` 原版有调用，重构改 `emit NpcTradeCommitted`，progression 自己加，与 `03` 的 `TradeCommitted` 同一处理方）；UI 写货舱；本系统发介绍信。

建队 100000：`Fleets.create_proxy(gold, …)` 扣玩家金。扣完之后这支队才进入本循环。

## 6. 解耦后的配置

```yaml
# config/economy/npc_trade.yaml
schema: dk4.economy.npc_trade
version: 1
# 循环规则。公式数字在 formulas/economy.yaml。

trigger:
  require_arrived: true
  aim_kind: vanilla:fleet_aim.trade   # 原版跳表下标 1 [推断]；枚举名未恢复

cargo:
  player_force_uses_holds: true  # 真 5×5 HoldRecord，与 03 同一布局
  other_force_compact: true      # CFleet compact 1 条 + u8 qty
  compact_expand_slots: 25       # asShipLoad 25；运行时读表长
  compact_qty_byte: u8
  sell_all_before_buy: true
  clear_after_sell: true
  place_buy_price: 0             # 原版 6294 第五参
  place_bought_town: null        # 原版 b2=0x62（ClearHoldRecord 空标）；导出时由 legacy_codec 编码

letters:
  grant_on_npc_sell: false       # 相对 CommitDeal 的硬差异

buy_quota:                       # 013345；计算归 04 Economy.buy_cap(…, npc_proxy)，这里只声明通道
  channel: npc_proxy
  bump_month_bought_only_with_ledger: true   # 原版 005a6f 只在有账本时调

pick_scope:
  player_force: current_major_sea  # 当前港 major_sea_id
  other_force: any                 # 原版传 8
  fallback_chain: [connect_sea, culture_sea, culture_any]   # 012800 / 009bb0(sea) / 009bb0(8)；名字为占位

stay_after_deal:                 # 072cf 为 0 时
  player_force: leave
  npc_coin: 2                    # rand(2)==0 → 走
  npc_dev_div: 10                # rand(1000) < 发展度/10 → 走；商业、军事各掷一次
  npc_dev_roll: 1000

npc_starved_hook:                # 买不到且 supply_base/2 < month_bought → 0035ee
  base_div: 2
  action: emit_only              # NpcPurchaseFailed → factions 记 CForceFailure[major_sea]

unverified:
  - pick_next_town_tables        # 005c22 / 014060 / 012800 / 009bb0 的筛选
  - dest_after_deal              # 00c621 体内
  - prefer_buy_below_local       # 卖时写入未见再读
  - npc_starved_action           # CForceFailure 记录之后被谁读
  - compact_save_bytes           # 存档是否整块 HoldRecord
```

```yaml
# config/formulas/economy.yaml  （本篇段；npc_buy_cost 与 02 / F-ECO-01 同一段）
npc_buy_cost:                    # F-ECO-01（§4.5）
  impl: expr
  int_mode: { div: trunc, width: int64 }
  quirks: { unsigned_div_when_negative: false }   # 原版 a>1000 时括号为负、无符号除法得巨数；原版包可开以复刻
  params:
    scale: 1000                  # 亦名 ref_1000
    jitter: 100                  # rand [0, jitter)
    factor_cap: 500              # 技能值再夹一次；与 skill_value_cap 同值但独立
    skill_key: skill.accounting  # 修正器注册表 key；不放 eSkill 下标
    apply_formula_in: economy    # 禁止 characters 再算一遍
  inputs: [unit, qty, ability]   # ability = Modifiers.eval(skill_key, { person: admiral })
  rng_stream: economy
  expr: "unit * qty * max(0, scale - clamp(ability, 0, factor_cap) + rand(jitter)) / scale"   # term < 0 饱和到 0（§4.5）

npc_loop:
  impl: vanilla.npc_loop
  params:
    prefer_buy_below: 60           # 0x3C；仅记录，默认不驱动买货
    pick_fashion_min: 180          # 0xB4
    pick_special_min: 200          # 0xC8
    ledger_buy_budget_gold: 100000 # 0x186A0；不是建队金
    gold_spend_numer: 4
    gold_spend_denom: 5            # 无账本最多花 80% 势力金
    quote_unit_min: 1
    fashion_npc:
      share_cut: 80
      gain: 2
      capacity_numer: 1
      capacity_denom: 2
      grant_letters: false
```

```yaml
# config/caps.yaml（本篇用到的键，不重复发明）
cargo_stack: 1
fleet_ships: 5
hold_slots_per_ship: 5
local_fleet_founding_gold: 100000   # 建队；属 fleets，这里只引用
ledger_slots: 6
# 空值一律 null；原版 106 / 98 / 26 / 6 / 3 只在 ids/legacy_codec.yaml
```

```yaml
# config/engine_limits.yaml
cargo_stack_max: 255            # HoldRecord.count u8
compact_qty_max: 255            # compact +8 u8
hold_slots_max: 32
fleet_ships_max: 16
```

```yaml
# config/fleets/proxy_rules.yaml  （建队，不在本系统改代码）
create_cost_gold: 100000        # 与 caps.local_fleet_founding_gold 同一值；一处生成
# 建出来的是玩家势力 → cargo.player_force_uses_holds
```

World0 / World1 同一 schema。内容包只换港口特产品与舰队模板，不换本循环。

## 7. 独立旋钮

- `npc_buy_cost.scale / jitter / factor_cap`（折扣曲线；夹紧独立于 `skill_value_cap`）
- `ledger_buy_budget_gold`、`gold_spend_numer/denom`
- `pick_fashion_min` / `pick_special_min`（选港能力门槛）
- `fashion_npc.gain / share_cut / capacity_*`（NPC 份额；与玩家 `fashion_share_div=6` 分开）
- `grant_on_npc_sell`（必须保持 false 才是原版信规则）
- `other_force_compact`（关掉则其他势力也走真舱；贸易代码仍只查 `CargoView`）
- `cargo_stack`（只影响玩家势力真舱）
- `compact_expand_slots`
- `buy_quota.bump_month_bought_only_with_ledger`（无账本的玩家势力舰队是否也吃玩家配额）

改折扣不必改 `CTrade`。改发信门槛不必动本循环。改建队金不必改买货预算。

## 8. 对外接口

### 命令

```
NpcTrade.simulate(fleet_id) ->
    { sold[], bought, sell_gold, buy_cost, dest_town_id | null }
```

只由 fleets / ocean 在 `ArrivedAtTown` 且 `aim==trade` 时调用。UI 不准直接开。

### 查询（只读）

```
Fleets.admiral(fleet_id) -> person_id | null      # 提督不是岗位；不写 post_holder(…, admiral)
Modifiers.eval("skill.accounting", { person }) -> int   # 选港门槛与买货折扣同一输入
Economy.quote(town_id, item_id) -> PriceSnapshot
Share.of(town_id, force_id) -> int                # factions；选港 / 留下；加份额不在本系统
Fleets.aim_kind(fleet_id) -> id
Fleets.location(fleet_id) -> { town_id | x, y }    # 当前港
Fleets.dest_town(fleet_id) -> town_id | null
Ledger.of(fleet_id) -> ledger_id | null            # 04 拥有账本
Fleets.force(fleet_id) -> force_id
Ships.holds(ship_id) ->                           # 逐船；NPC 紧凑货同一视图
    Holds | Compact { item_id, qty, b1, b2, buy_gold }
```

Live 取。舰队不存在或港口不存在：整次 no-op。货舱不存 `World*`。

### 向船 / 货

```
Ships.clear_cargo(fleet_id)
Ships.place(fleet_id, item_id, qty, b1, b2, buy_gold)
Ships.hold_capacity(fleet_id) -> int
```

`place` 内部按 `force==player` 选真舱或紧凑。调用方不分支。

### 向特产品（`04`）

```
Economy.buy_cap(town_id, slot, force_id, npc_proxy) -> int
Economy.settle_special(town_id, slot, qty, counts_toward_quota)   # supply 与 month_bought 一次结算
Economy.specials(town_id) / Economy.grade_rank(town_id, slot)
Ledger.post_income / post_expense(ledger_id, gold)
```

### 事件

```
NpcSoldAll        { fleet_id, town_id, force_id, items[], qtys[], gold }
NpcBought         { fleet_id, town_id, slot, item_id, qty, cost_gold }
NpcFashionSold    { town_id, force_id, item_id, qtys[2], hold_capacity }
KindRateChanged   { town_id, kind_id, old, new }     # 经 02
LedgerCredited    { ledger_slot, gold }              # income；账本事件（economy），不是 ForceGoldChanged 的别名
LedgerDebited     { ledger_slot, gold }              # expense；同上
# 无账本时走 Force.add_gold，ForceGoldChanged 由 factions 发（00-overview/06），本系统不发
NpcTradeCommitted { fleet_id, admiral_id | null, town_id, sold_gold, buy_cost }   # progression 给经验（原版 006e88(提督,0xA,1)）
NpcPurchaseFailed { town_id, force_id, item_id, major_sea_id }                    # 原版 0035ee → CForceFailure
NpcTradeSkipped   { fleet_id, reason }               # no_force / no_dest / no_profit
```

`NpcFashionSold`：**不**触发信件。factions 不得在监听里走发信判定（`letter_intro_min` 等，`02-factions/04`）。

`SoldFashionGoods` 名称留给玩家 `CommitDeal`。

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| `eFleetAimKind` 名与 `NUM_FLEETAIMKIND` | 只有符号；交易是跳表下标 1 |
| 谁把 aim 写成 1 | 到港分发器以外未追 |
| ~~`fn_14000e1ce(·, 4)` 是精神还是会计术~~ | 已钉：`eSkill` 4 会计术（§2.5） |
| `013ee4` 候选港收集函数 | 控制流、顺序、范围参数已钉；`005c22` / `014060` / `012800` / `009bb0` 的筛选条件未读 |
| ~~`013345` 供给/份额件数~~ | 已钉（§2.5） |
| 卖循环写入的「指数<60 item」 | 本函数未见再读 |
| `0097af` 第三参 | 传 `hold.b1`（槽等级），不是 hold 买入价 |
| ~~特产品 6 字节记录 +1~~ | 已钉：`month_bought`，玩家 `CommitDeal` 与本函数（有账本）两处写（§2.5） |
| `qty<=0` 的 `0035ee` | 触发条件已钉（非玩家、`base/2 < month_bought`）；写 `CForceFailure[major_sea]`（`02-factions/05`）；之后谁读未查 |
| 成交后 `00c621` | 调用条件已钉（§2.5 末）；体内未读 |
| `006e88(person, 0xA, 1)` | 买货折扣后调用；与 `03` Commit 后经验同函数；0xA 语义归 progression |
| `00dc6a` | 流行槽计数，不是信 |
| 紧凑货存档 | 见到 `+0x74/+0x75/+0x7c` 字节；价 dword 是否落盘未知 |
| `qty>25` 展开 | 栈数组 25 格；原版是否先夹未知 |
| `00c342` 下标 7 = 无账本 | 观察；与 6 槽关系未知 |
| 建队 100000 与买货预算 100000 | 同立即数，未证明同一键 |
| World1 是否同一跳表 | 函数在 exe 不在 DLL，逻辑应同；未对照 World1 舰队 aim |
| 主队在玩家操作时是否也会 aim=1 | 未证明；本规格按「非会话自动循环」实现 |

`013345` 件数与 +1 列已钉。仍空着的是选港候选收集函数的筛选。空着时原版包可以：选港退化为「有利润的特产品随机 + 随机连港」，范围按 `pick_scope`。
