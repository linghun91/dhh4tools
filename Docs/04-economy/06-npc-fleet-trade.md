# NPC / 地方 / 委任舰队自动交易

到港后的**无窗买卖**：先卖掉整船货，再按特产品利润决定买什么、买几件、记到账本或势力金库。原版入口 `fn_140010d25`（body `0x1400f6d20`）。

玩家在交易所点的那一次会话是 `03-trade-loop.md` 的 `CTrade`。本篇**不打开** `CTrade`，不走 unpack / 退货 / 确认 / 取消，也不发介绍信、推荐信。

## 1. 职责边界

本系统管：

- 舰队已到港且目标种类为「交易」时的自动卖 → 清舱 → 选下一港 → 买特产品
- 买入成本（`quote` × 件数 × `npc_buy_cost`，只读 `ModifierSet.buy_cost_factor`）
- 玩家势力走真舱 `HoldRecord`；其他势力走 `CFleet+0x74` 紧凑货记
- 卖出后改种类指数、上报流行件数（份额由 factions 执笔；**不发信**）
- 有账本记 `income` / `expense`；无账本改势力金币

不管：

- 玩家交易所会话、堆叠 UI、退货、`CommitDeal` 发信（`03`）
- 市价公式本体（`02.quote`；本系统只调用）
- 特产品 5 槽定义与供给列语义（`04`；本系统 `consume` 一列，另写一列）
- 地方舰队怎么建、100000 建队金、命令 UI（`05-ships-fleet/02-fleet-proxy.md`）
- 月结滚动、上缴、付不起转下月的**整段**日历（`04` 月账；本系统只往账本加减）
- 人物六维 / 技能下标（禁止读 `eAbility` / `eSkill`）
- 委托 / 剧情扣货（仍走 `CountFleetItem` / `AddLoad`，`03` 已约定按件数）

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

提督空（人物 `0xEC`）时 `fn_14000d599` 返回势力哨兵 `0x1A=26`，与合约空势力同一哨兵 `[确认]` `03-relations-influence.md`。

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
8. 再按份额决定是否留下 / 换目标（`fn_1400072cf` / `fn_14000c621`，完整选港 `[未知]`）

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
- 且 `sold >= capacity/2`（`008d41` 的舱位和 / 2）才把增益写成 **2**
- 然后 `fn_1400122f6(town, force, 2, 1)` ——与 `CommitDeal` **同一个加份额函数**，但增益固定 2，不是 `sold/6+1`
- 份额 `≥80`：整段跳过，**不加份额**
- **没有** `bd98` / 介绍信 / 推荐信 分支

`DK4FSH1` 只补 `CTrade::CommitDeal`。舰队自动交易发信不受该补丁影响，因为这里本来就不发信。

重构：经济 `emit NpcFashionSold`；factions 用 **NPC 规则** 加份额；`grant_letters: false`。禁止复用玩家 `SoldFashionGoods` 的信件门槛。

### 2.5 买：下一港、买哪件、几件

`fn_140013ee4`（body `0x1400f3640`）`[确认 骨架]`：

- 势力 `==0x1A` 或没有提督 → 返回空港 `0x62`
- `fn_14000e1ce(提督, 4)`（原版能力/技能下标 4；本系统改读修正器，见 §4）
  - `≥ 0xB4`（180）：两格流行，收集候选港 `fn_140005c22`，随机取
  - `≥ 0xC8`（200）：五槽特产品，`fn_140014060` 收集候选港
  - 否则 / 上面为空：`fn_140012800` 再 `fn_140009bb0`（同文化/连港一类，筛选 `[未知]`）
- 仍空 → `0x62`

买货发生在**当前港**。选出的镇是**卖货目标 / 下一站**。当前港特产品：

1. 若 `ePurchaseTradingItem != 空` 且 `fn_14000680c` 命中本港槽（0–4；未命中返回 6）→ 用该槽
2. 否则扫 5 槽，把「当前港买入价 `00a691` < 目标港卖价 `0097af`」的槽放进 `aeProfitSpecialProduct`，`fn_140005272` 随机取
3. 仍是 6 → 不买

件数 `[确认 夹紧顺序]`：

```
unit = max(1, quote_gold(当前港, 该槽))     # fn_140001735 → 010492；再 fn_1400013cf(1, ·)
cap  = fn_140008d41(fleet)                  # 五船 CountHolds(10) 之和（真舱位数，不是紧凑 qty）
qty  = fn_140013345(town, slot, force_id)   # 供给/份额有关，体内 001ab4/00c3f1/0092af；式子未全还原
qty  = min(qty, cap)
if ledger:
    qty = min(qty, ledger_buy_budget_gold / unit)    # 立即数 0x186A0=100000
else:
    qty = min(qty, (force_gold * 4 / 5) / unit)      # 花金币的 80%
```

`013345` 对玩家势力与其他势力分支不同（读 `0054cf`）。细节标未核实，YAML 留钩子，禁止在代码里写死 0x64。

有货可买时：

- `fn_1400108b6` 改买入种类指数（与 `03.CommitDeal` 买入同类）
- `raw = unit * qty`
- 有提督：`cost = raw * (1000 - Ability(提督,4) + rand(100)) / 1000`，再 `fn_140006e88(提督, 0xA, 1)`（成长调用，0xA 语义 `[未知]`）
- `fn_140006294` 装货（b1=槽等级，b2=`0x62`，价=0）
- `fn_14000712b` 扣特产品记录 **+0** 供给（`town+0x30+slot*6` 的 u8）
- 有账本：`fn_140005a6f` 把件数加到特产品记录 **+1**（`009b0b` 读 `town+0x31+slot*6`，**不是同一字节**）+ `fn_140005849` 支出 `+= cost`（账本 `+0x0C`）
- 无账本：`fn_14001117b` 扣势力金币

`04` 曾写 `005a6f`「加供给一类」。两列偏移不同，禁止实现成「刚扣的供给加回去」。+1 列语义 `[未知]`（月销量 / 到货 / 第二计数）。

`qty==0` 且非玩家势力时还有 `fn_1400035ee` 分支（投资一类）`[未知]`，本循环缺省不实现，见 §9。

### 2.6 买入成本与 500 夹紧

sc `0x1400f742c` `[确认]` `exe_caps.md`、`dhh4tools/dk4save.c` `PROXY_BUY_*`：

```
a    = fn_14000e1ce(提督, 4)     # 结果已按属性结算上限夹过
cost = unit * qty
cost = cost * (1000 - a + rand(100)) / 1000    # 32 位 imul + 无符号 div
```

`rand(100)` 为 `[0, 100)`。`a=500` 时约五折；`a>1000` 括号为负，无符号除法变成几百万。月报亏损来自这里，不是卖价公式。

补丁：先把 `a` 夹到 500 再减。不改结算上限，不改战斗 `500-a`、道具 `100-a/50`。

下标 4 是 `eAbility` 精神还是 `eSkill` 会计术：人物篇与公式总表不一致，见 `00-overview/07-open-unknowns.md`。**本系统不读下标。**

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

本系统不重写整段月结（上缴、势力 1% 一类在同函数后部）。只保证买卖写入的 `income`/`expense` 能被 `04.month_roll` 消费。

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
| 势力 | `eForce` | 空 `26`（`0x1A`） | `force_id` |
| 港口 | `eTown` | 非法镇 `98`（`0x62`） | `town_id` |
| 交易品 | `eTradingItem` | 货格空 `106` | `trading_item_id` |
| 特产品槽 | 0–4 | 未命中 **6**（不是 −1） | `special_slot` |
| 流行槽 | 0–1 | 非流行 **3** | `fashion_slot` |
| 目标种类 | `eFleetAimKind` | `INVALID=-1`；交易观察值 1 | `fleet_aim_kind_id` |
| 账本槽 | 6 × `0x18` | 取不到 = 无账本 | `ledger_slot` |
| 货 | 玩家：`HoldRecord[ships][slots]`；其他：紧凑 1 条 + `qty` | item=106 | `CargoView` |

运行时读表长。不要写死 69 / 25 / 5。原版包实例计数仍是那些数。

没有独立的 `npc_trade_session_id`。一次 `simulate` 同步完成。

## 4. 算法 / 公式

全部立即数进 `config/formulas/economy.yaml` 的 `npc_buy_cost` / `npc_loop`。代码禁止出现 1000、500、100、60、80、180、200、100000。

人物只提供 `ModifierSet.buy_cost_factor`。原版包约定：这是**已经夹到 `buy_ability_clamp` 的输入**（对应 `Ability(提督,4)` 的量级 0–500），本系统再算倍率。禁止再读六维。`apply_formula_in` 只能 `economy`（原版算在 `010d25`）；不要人物算一遍、经济再算一遍。

### 4.1 触发

```
can_run(fleet_id) =
    Fleets.exists(fleet_id)
    and Fleets.current_town(fleet_id) == Fleets.dest_town(fleet_id)
    and Fleets.dest_town(fleet_id) != town_empty
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
        if rec.item == empty: return []
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
    fslot = fashion_slot(town_id, rec.item)               # 0/1 或 fashion_none=3
    if fslot != fashion_none: fashion_qty[fslot] += qty_of(rec)

for kind if kind_count[kind] > 0:
    Economy.on_kind_traded(town, kind, sell, kind_count[kind], kind_gold[kind])

emit NpcFashionSold { town_id, force_id, qtys[2], hold_capacity }
clear_cargo(fleet)
credit(total_sell)    # 账本 income 或势力金币
```

factions 收 `NpcFashionSold`：

```
if share(town, force) < npc_fashion.share_cut:          # 80
    if sum(qtys) >= ceil(hold_capacity * numer / denom): # 原版 capacity/2
        apply_share(town, force, npc_fashion.gain)       # 2
grant_letters = false
```

### 4.4 选下一港 / 买哪件

```
dest = pick_next_town(fleet_id, current_town_id, modifiers.buy_cost_factor)
# 原版门槛 180 / 200 用的是 Ability(·,4)，与买货折扣同一输入
# 重构：用同一个 buy_cost_factor 比 npc_loop.pick_fashion_min / pick_special_min
# 候选表 005c22 / 014060 / 012800 未还原前 pick_source: unverified

if dest == town_empty: pick_connect_town(fleet); return

slot = special_slot_of(current, hinted_item)            # 6 = 无
if slot == special_none:
    cand = [s for s in 0..4
            if special_item(current,s) != empty
            and quote(current, item).buy_gold < quote(dest, item).sell_gold]
    if cand empty: skip buy
    slot = cand[rand(len(cand))]
```

### 4.5 件数与成本

```
unit = max(quote_unit_min, quote(current, special_item(current,slot)).buy_gold)
qty  = special_buy_qty(town, slot, force_id)            # 013345；未还原前 unverified 钩子
qty  = min(qty, hold_capacity(fleet))
if has_ledger(fleet):
    qty = min(qty, ledger_buy_budget_gold / unit)
else:
    qty = min(qty, force_gold(force) * gold_spend_numer / gold_spend_denom / unit)
if qty <= 0: skip buy

a = clamp(modifiers.buy_cost_factor, 0, buy_ability_clamp)
term = buy_cost_scale - a + rand_int(0, buy_cost_jitter)   # jitter=100 → [0,99]
if term < 0: term = 0                                      # 有符号饱和；禁止无符号 div
cost = saturating_mul_div(unit, qty, term, buy_cost_scale)

consume_supply(town, slot, qty)                            # 列 +0
if has_ledger(fleet):
    bump_special_aux(town, slot, qty)                      # 列 +1；语义未知但原版有写
    ledger.expense += cost
else:
    force_gold -= cost

place_cargo(fleet, item, qty, b1=grade_rank(slot), b2=clear_mark)
Economy.on_kind_traded(town, kind, buy, qty, unit*qty)
```

`hold_capacity`：玩家势力 = `Σ CountHolds(scan)`，堆叠时 × `cargo_stack`。其他势力原版仍用五船 `CountHolds` 当件数上界，落货却写紧凑数量。重构可配 `other_force_capacity: hold_scan_sum | compact_qty_max`。

### 4.6 月结（本系统只认账）

```
# 04 已写。这里只声明买卖写入的字段
ledger.income  += sell_gold
ledger.expense += buy_cost
# vfunc_04 @ 0x140100180：付不起的支出滚下月
```

## 5. 耦合分析

原版硬编码：

- `010d25` 直接 `Ability(提督, 4)`、直接改 `CTown` 指数 / 特产品字节、直接改账本与势力金币
- `e638` 把「是不是玩家势力」焊进展开 / 装货 / 清空三支
- 流行加份额调用与 `CommitDeal` 同函数 `0122f6`，但参数不同、无发信
- 月结在 `CFleetElement::vfunc_04`，与补给、旗舰、上缴挤在同一虚函数
- 紧凑货挂在 `CFleet` 对象上，和阵型、aim、驻港字节挤在 `+0x70` 一带

重构允许的方向：

```
ocean / fleets  --ArrivedAtTown + aim-->  economy.NpcTrade
NpcTrade        --quote / on_kind_traded-->  economy.Price
NpcTrade        --consume / aux-->           economy.Specials
NpcTrade        --holds / compact-->         ships / fleets
NpcTrade        --buy_cost_factor-->         characters.ModifierSet   (只读)
NpcTrade        --income/expense-->          fleets.Ledger | factions.Gold
NpcTrade        --NpcFashionSold-->          factions.Share           (不写信)
```

禁止：经济改人物经验（`006e88` 原版有调用，重构改 `emit AdmiralPracticed`，人物自己加）；UI 写货舱；本系统发介绍信。

建队 100000：`Fleets.found_proxy` 扣玩家金。扣完之后这支队才进入本循环。

## 6. 解耦后的配置

```yaml
# config/economy/npc_trade.yaml
schema: dk4.economy.npc_trade
version: 1
# 循环规则。公式数字在 formulas/economy.yaml。

trigger:
  require_arrived: true
  aim_kind: 1                    # [推断] 跳表下标；枚举名未恢复
  aim_kind_key: trade

cargo:
  player_force_uses_holds: true  # 真 5×5 HoldRecord，与 03 同一布局
  other_force_compact: true      # CFleet compact 1 条 + u8 qty
  compact_expand_slots: 25       # asShipLoad 25；运行时读表长
  compact_qty_byte: u8
  sell_all_before_buy: true
  clear_after_sell: true
  place_buy_price: 0             # 原版 6294 第五参
  place_b2: 0x62                 # ClearHoldRecord 空标；与空港 98 同字节、不同域

letters:
  grant_on_npc_sell: false       # 相对 CommitDeal 的硬差异

special_aux:
  consume_column: 0              # town+0x30+slot*6
  ledger_bump_column: 1          # town+0x31+slot*6；语义 unverified
  ledger_bump_on_buy: true

unverified:
  - pick_next_town_tables        # 005c22 / 014060 / 012800
  - special_buy_qty              # 013345
  - dest_after_deal              # 00c621
  - prefer_buy_below_local       # 卖时写入未见再读
  - qty_zero_invest              # 0035ee
  - compact_save_bytes           # 存档是否整块 HoldRecord
```

```yaml
# config/formulas/economy.yaml  （本篇段；npc_buy_cost 与 02 / F-ECO-01 同一段）
npc_buy_cost:
  scale: 1000                    # 亦名 ref_1000
  jitter: 100                    # rand [0, jitter)
  factor_cap: 500                # 写入 caps.buy_ability_clamp；经济再夹一次
  overflow: saturate_signed
  apply_formula_in: economy      # 禁止 characters 再算一遍
  # 不要在本文件放 buy_skill_index：那是人物生成 ModifierSet 的事

npc_loop:
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
  special_none: 6
  fashion_none: 3
```

```yaml
# config/caps.yaml（本篇用到的键，不重复发明）
buy_ability_clamp: 500
cargo_stack: 1
fleet_ships: 5
hold_slots_per_ship: 5
hold_scan: 10
local_fleet_founding_gold: 100000   # 建队；属 fleets，这里只引用
ledger_slots: 6
trading_item_empty: 106
town_empty: 98
force_empty: 26
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

- `npc_buy_cost.scale / jitter / factor_cap`（折扣曲线；夹紧独立于 `ability_settled_cap`）
- `ledger_buy_budget_gold`、`gold_spend_numer/denom`
- `pick_fashion_min` / `pick_special_min`（选港能力门槛）
- `fashion_npc.gain / share_cut / capacity_*`（NPC 份额；与玩家 `fashion_share_div=6` 分开）
- `grant_on_npc_sell`（必须保持 false 才是原版信规则）
- `other_force_compact`（关掉则其他势力也走真舱；贸易代码仍只查 `CargoView`）
- `cargo_stack`（只影响玩家势力真舱）
- `compact_expand_slots`
- `ledger_bump_on_buy`（是否写特产品 +1 列）

改折扣不必改 `CTrade`。改发信门槛不必动本循环。改建队金不必改买货预算。

## 8. 对外接口

### 命令

```
NpcTrade.simulate(fleet_id) ->
    { sold[], bought, sell_gold, buy_cost, dest_town_id | empty }
```

只由 fleets / ocean 在 `ArrivedAtTown` 且 `aim==trade` 时调用。UI 不准直接开。

### 查询（只读）

```
Characters.modifier_set(admiral_person_id) -> ModifierSet
    # 只取 buy_cost_factor
Economy.quote(town_id, item_id) -> PriceSnapshot
Factions.share(town_id, force_id) -> int          # 选港 / 留下；加份额不在本系统
Fleets.aim_kind(fleet_id) -> id
Fleets.current_town / dest_town(fleet_id) -> town_id
Fleets.ledger(fleet_id) -> { income, expense, investment } | none
Fleets.force(fleet_id) -> force_id
Ships.cargo_view(fleet_id) ->
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
Specials.consume(town_id, slot, qty)          # 列 0
Specials.bump_aux(town_id, slot, qty)         # 列 1；列语义未命名
Specials.item / grade_rank(town_id, slot)
```

### 事件

```
NpcSoldAll        { fleet_id, town_id, force_id, items[], qtys[], gold }
NpcBought         { fleet_id, town_id, slot, item_id, qty, cost_gold }
NpcFashionSold    { town_id, force_id, item_id, qtys[2], hold_capacity }
KindRateChanged   { town_id, kind_id, old, new }     # 经 02
LedgerCredited    { ledger_slot, gold }              # income
LedgerDebited     { ledger_slot, gold }              # expense
ForceGoldChanged  { force_id, delta }
AdmiralPracticed  { person_id }                      # 原版 006e88；人物自己加
NpcTradeSkipped   { fleet_id, reason }               # no_force / no_dest / no_profit
```

`NpcFashionSold`：**不**触发信件。factions 不得在监听里走 `03` 的 `letter_lo/hi`。

`SoldFashionGoods` 名称留给玩家 `CommitDeal`。

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| `eFleetAimKind` 名与 `NUM_FLEETAIMKIND` | 只有符号；交易是跳表下标 1 |
| 谁把 aim 写成 1 | 到港分发器以外未追 |
| `fn_14000e1ce(·, 4)` 是精神还是会计术 | `07-open-unknowns`；本系统不读 |
| `013ee4` 三张候选港表 | 有调用无公式 |
| `013345` 供给/份额件数 | 有 `idiv 0x64` 与玩家/NPC 分支 |
| 卖循环写入的「指数<60 item」 | 本函数未见再读 |
| `0097af` 第三参 | 传 `hold.b1`（槽等级），不是 hold 买入价 |
| 特产品 6 字节记录 +1 | 有账本买货会加件数；不是 +0 供给 |
| `qty==0` 的 `0035ee` | 可能投资，未实现 |
| 成交后 `00c621` 选港 | 份额=0 时玩家必走、NPC 掷骰 |
| `006e88(person, 0xA, 1)` | 买货折扣后调用；0xA 不是 0–9 技能下标 |
| `00dc6a` | 流行槽计数，不是信 |
| 紧凑货存档 | 见到 `+0x74/+0x75/+0x7c` 字节；价 dword 是否落盘未知 |
| `qty>25` 展开 | 栈数组 25 格；原版是否先夹未知 |
| `00c342` 下标 7 = 无账本 | 观察；与 6 槽关系未知 |
| 建队 100000 与买货预算 100000 | 同立即数，未证明同一键 |
| World1 是否同一跳表 | 函数在 exe 不在 DLL，逻辑应同；未对照 World1 舰队 aim |
| 主队在玩家操作时是否也会 aim=1 | 未证明；本规格按「非会话自动循环」实现 |

实现前必须钉死：`013345` 件数、选港候选表、+1 列语义。这三项空着时，原版包可以：买货件数退化为 `min(supply, capacity, budget/unit)`，选港退化为「有利润的特产品随机 + 随机连港」，+1 列只留写入钩子、不解释。
