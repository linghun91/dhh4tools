# 买卖会话

玩家在港口交易所里打开的那一次交易：从船舱 unpack，买特产品、卖舱货、退货、确认或取消。会话对象原版叫 `CTrade`。

## 1. 职责边界

本系统管：

- 交易会话状态机（打开 / 买一件 / 点舱位 / 全卖 / 退货 / 确认 / 取消）
- 把船舱 `HoldRecord` 拷进会话 `ExchangeSlot`，确认时再写回
- 堆叠规则（引擎允许 `>1`，原版包 `=1`）
- 本笔金币合计、与玩家金库的结算
- 向势力发「卖了流行品 / 卖了某商品」事件（份额、介绍信不在本系统写）

不管：

- 市价怎么算（`02.quote`）
- 特产品供给怎么补（`04`，本系统只 `consume` / `refund`）
- 货舱作为船舱室存在（`05-ships-fleet` 拥有 `HoldRecord` 持久化）
- 人物六维（只读 `ModifierSet.buy_cost_factor` 给 NPC 自动交易）
- 委托 / 剧情扣货（舰队接口 `CountItem` / `AddLoad`，由 town-life / narrative 调用）
- UI 列宽、按钮皮肤、`%6d`（`12-ui-save`）

## 2. 原版事实

来源：`srcdata/docs/trade_stack.md`（全文）、`srcdata/include/trade.h`、`dk4_types.h`、`srcdata/source/game/trade/Trade.cpp`、`CTrade.cpp`、`Constants.h`、`exe_caps.md`。

### 2.1 格子

| 结构 | 大小 | 位置 | 用途 |
| --- | --- | --- | --- |
| `HoldRecord` | 8 B | `ship+0x58+j*8` | 船上持久货 |
| `ExchangeSlot` | 12 B | `CTrade+0xE8 + ship*0x3C + slot*0xC` | 会话舱位 |
| `SellRecord` | 8 B | `CTrade+0x214 + ship*0x28 + j*8` | 本笔待卖列表 |

`GetExchangeSlot` 公式以 `Trade.cpp` 为准（ship 步长 `0x3C=5*12`）。`dk4_types.h` 注释把 ship/j 写反了，以函数体为准。

玩家舰队：`NUM_FLEETSHIP=5` × `NUM_SHIPLOAD_EXCH=5`。`CountHolds(0x0A)` 扫描上限 10（`NUM_HOLD_SCAN`），不是第二套 10 格交易所。

其他势力不走 25 格真舱，而走 `fleet+0x74`「一条记录 + 数量」`[确认]` `trade_stack.md` 委托/自动交易两支。

### 2.2 字节

```
HoldRecord:
  +0 u8  item     # 0x6A 空
  +1 u8  b1       # 原版拷贝；语义 [未知]
  +2 u8  b2       # 清空时写 0x62
  +3 u8  count    # 原版不用；堆叠补丁写件数，0 当 1
  +4 u32 buy_price
ExchangeSlot: 同上前 8 字节，再 +8 u32 just_bought  # 本笔刚买=1，unpack 写 0
```

原版拷贝 `110b8` 只拷 `+0/+1/+2/+4`。清空 `c662`：`item=0x6A`，`+2=0x62`。

### 2.3 会话对象 `CTrade`（观察字段）

| 偏移 | 名 | 说明 |
| --- | --- | --- |
| +0x18 | `ulTotalIncome` | 买入合计（RTC 名；金币） |
| +0x1C | `ulTotalExpenditure` | 卖出合计 |
| +0x20 | 每船买入小计 `[5]` | Unpack 清零 |
| +0x34 | 每船卖出小计 `[5]` | 同上 |
| +0xE8 | `ExchangeSlot[5][5]` | |
| +0x214 | `SellRecord[5][5]` | |
| +0x32c | dword | ctor 清 0 |
| +0x588 | 关联 `CTradingView*` | 最多被访问 |
| +0x590 | 显示买入价列 | 按钮 `0x15` 切换，属 UI |

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

### 2.6 确认 `CommitDeal`（`0x140277090`）

RTC 局部：`anBuyTradingItemKindNum[19]`、`anSellTradingItemKindNum[19]`、`anSellTradingItemNum[105]`、两类金额合计。

顺序观察：

1. 读买入合计 / 卖出合计；都为 0 则提示离开（文案 id `0x378`）
2. 若卖 < 买，检查玩家金币够不够差额；不够文案 `0x380`
3. 扣/加金币（净额）
4. `PackToShip`
5. 按种类重计件数与金额；买入种类 `fn_1400108b6`、卖出种类 `fn_140007301`（改市价指数，见 `02`）
6. 同一 `item` 卖出 `≥15`：调整该城该商品数值（非流行品，`trade_stack.md`）
7. 流行品门槛与份额 / 介绍信（见下，发事件给 factions）
8. 特产品槽供给结算（`04`）

流行品（原版写在 Commit 里，重构拆走状态）：

- `s4A4` = 五船 `CountHolds(10)` 之和
- 门槛 `s4D0 = ceil(s4A4 / 2)`（补丁改成可配）
- `share < 80`：每种流行品件数 ≥ 门槛才加份额，`delta = sold/6+1`，再封到 `100-已占`
- `share ≥ 80`：无信且件数 ∈[10,15] 介绍信，≥16 推荐信；已有介绍信且 ≥16 升推荐信

这些数字进 `formulas/economy.yaml` 的 **事件参数**；`share` / 信件由 factions 执笔。

### 2.7 打开 / 打包

`UnpackFromShip`：清空 5×5 槽与卖出格，合计清零，再从玩家五船 hold 生成槽（`just_bought=0`），并读本港最多 5 个特产品进买入栏。

`PackToShip`：只从 `CommitDeal` 调用。对每船 `n=CountHolds(10)`，按槽序把占用的 exchange 写回 hold。

`BuyOne(itemIndex)`：`itemIndex` 是本港特产品槽 0–4，不是 `eTradingItem`。供给 `≤0` 或当前船索引为 6 则失败。原版只填空槽。买完把该槽 `just_bought=1`，扣供给。

点舱位：`just_bought` → 按**当前买入展示价**退货并补供给；否则进卖出列表。

### 2.8 舰队自动交易 / 委托

`fn_140010d25`（`0x1400f6d20`）：NPC / 地方舰队。买入成本见 `02` 的 `npc_buy_cost`。玩家势力走真舱位。

委托 / 剧情只通过：

- `CountFleetItem(fleet, item)` — 玩家支原版数格；堆叠补丁改成加件数
- `AddLoad(fleet, item, …, n)` — `n>0` 原版一格一件；`n<0` 原版整格扣

重构后两函数都按件数，容量 = `ships × slots × cargo_stack`。

## 3. 实体与 ID

| ID | 原版 | 空 |
| --- | --- | --- |
| `eFleetShip` | 0–4，5 艘 | `INVALID_FLEETSHIP=-1` |
| `eShipLoad` | 0–4，每船 5 货格 | `INVALID_SHIPLOAD=-1` |
| `eTradingItem` | 0–104 | 空格 106 |
| `eSpecialProduct` | 每港 0–4 | 槽内 item=106 |

会话本身没有全局 ID，随 `ArrivedAtTown` 创建、关闭销毁。

## 4. 算法 / 公式

### 4.1 堆叠

```
qty(hold) = cargo_stack==1 ? 1 : (hold.count==0 ? 1 : hold.count)
can_merge(a, b) = cargo_stack > 1 and a.item==b.item and a.item != empty
                  and qty(a)+qty(b) <= cargo_stack
```

原版包 `cargo_stack: 1` → 永不合并。不要用显示列宽倒推上限。

### 4.2 会话净额

```
net = sell_total - buy_total
if net < 0 and player_gold < -net: reject   # 文案不够钱
commit:
    player_gold += net
    pack holds
```

金币整数，饱和到 `caps.gold_max`（原版宽度 `[未知]`，实现用足够大的有符号整数并在 caps 夹）。

### 4.3 退货

```
refund_gold = Economy.quote(town_id, item_id).buy_gold * qty
supply[slot] += qty
clear exchange slot
```

按当前市价，不按 hold 里记下的买入价。

### 4.4 流行品事件参数（原版缺省）

```
hold_sum = Σ CountHolds(fleet_ship)
threshold = ceil(hold_sum * fashion_threshold_numer / fashion_threshold_denom)
# vanilla numer=1 denom=2
share_delta = sold_fashion_qty / fashion_share_div + 1    # vanilla div=6
letter_lo = 10
letter_hi = 15
letter_rec = 16
share_letter_cut = 80
```

经济只 `emit SoldFashionGoods { town_id, force_id, items[], qtys[], hold_sum }`。

### 4.5 NPC 买货

见 `02` `npc_buy_cost`。本系统在自动交易里调用 `quote` + `ModifierSet.buy_cost_factor`，再 `Hold.place` / 账本 `expense`。

## 5. 耦合分析

原版 `CTrade` 直接：

- `GetPlayerFleet` / `CShip::CountHolds`
- 读本港特产品、写供给
- 改金币、改种类市价、改份额、发信、改流行

重构允许的方向：economy ← ships.hold（拷入拷出）、economy ← characters.ModifierSet（只读）、economy → 事件给 factions / town。

禁止 economy 改人物经验、禁止 UI 写 hold。

## 6. 解耦后的配置

```yaml
# config/caps.yaml
fleet_ships: 5
hold_slots_per_ship: 5
hold_scan: 10
cargo_stack: 1          # 原版包；引擎可 >1
trading_item_empty: 106
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
  same_item_adjust_qty: 15     # 卖同一商品 ≥15 调城市数值
fashion_event:
  threshold_numer: 1
  threshold_denom: 2
  share_div: 6
  share_letter_cut: 80
  letter_intro_min: 10
  letter_intro_max: 15
  letter_recommend_min: 16
```

没有「列宽」键。

## 7. 独立旋钮

- `cargo_stack`（1 → 99 只改 caps 与存档宽度）
- 每船货格、每舰队船数（内容包；引擎按表长）
- 流行品门槛与信件阈值（事件参数）
- 同一商品调价件数 15

改堆叠不必改市价公式。改船数不必改 105 件目录。

## 8. 对外接口

### 命令

```
Trade.open(fleet_id, town_id) -> session_id
Trade.buy(session_id, special_slot, qty)
Trade.click_hold(session_id, ship_index, slot_index)   # 刚买则退，否则挂卖
Trade.sell_all(session_id)
Trade.withdraw_sell(session_id, sell_index)
Trade.commit(session_id) -> Result
Trade.cancel(session_id)                               # unpack 丢未确认
```

### 向船

```
Ships.holds(fleet_id, ship_index) -> [ { item_id, qty, buy_gold } ]
Ships.replace_holds(fleet_id, ship_index, holds)       # 仅 commit
Ships.count_item(fleet_id, item_id) -> int             # 按件数
Ships.add_load(fleet_id, item_id, qty, buy_gold)       # qty 可负
```

Live 取船；舰队不存在则整个会话 no-op。hold 里不存 `World*` / `Player*`。

### 向人物

```
Characters.modifiers(person_id) -> ModifierSet
# 本系统只读 buy_cost_factor（NPC 买货）
```

### 事件

```
TradeOpened      { session_id, fleet_id, town_id }
CargoCommitted   { fleet_id, town_id, bought[], sold[], gold_net }
SoldFashionGoods { town_id, force_id, items[], qtys[], hold_sum }
SoldBulkItem     { town_id, item_id, qty }            # qty≥same_item_adjust_qty
TradeCancelled   { session_id }
```

`SoldFashionGoods` 由 factions 决定是否加份额、发信。经济不写 `share`。

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| `HoldRecord.b1` | 原版随拷贝走，无名字 |
| `HoldRecord.b2` 除清空写 `0x62` 外 | 卖价把 `0x62` 当空 |
| `CTrade` `+0x32c`、`+0x3e0`、`+0x4b0` | 控件指针 / 未命名 |
| `+0x2dc` 买入展示价数组的填法 | `fn_140010771` 只是取值 |
| 玩家金币的整数宽度与 caps | 未在 Constants.h |
| `CountHolds(10)` 的 10 与 5 舱室的精确关系 | 扫描上限 10，交易所 5 |
| 仓库 `CWarehouseView` 是否另一套持久库存 | 有 View；会话卖出列表不是仓库 |
| World1 是否同一 5×5 | 未对照 |
| `CommitDeal` 里 `s80==0xec` 分支 | 人物/对话相关，未命名 |
