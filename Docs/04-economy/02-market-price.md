# 市价与报价

港口对**种类**存一条市价指数，对**单件货物**再算金币报价。本篇是 `quote()` 的规格。买卖会话怎么扣钱见 `03-trade-loop.md`。

## 1. 职责边界

本系统管：

- 每座港口、每个 `trading_item_kind_id` 的市价指数（存档状态）
- 把指数、基础价、是否本地特产品合成金币报价
- 买卖发生后指数怎么滑
- 航路市价图作为**只读投影**（原版 `CMarketPriceRouteMap`）

不管：

- 交易品目录（`01`）
- 货舱、会话、堆叠（`03`）
- 特产品供给、投资、品牌（`04`）
- 市场份额、合约（`02-factions`，本系统只读 `share` 当报价修正输入若公式需要；原版 `quote` 主路径不读 share）
- 人物六维（只读 `ModifierSet`，见 `03` 的 NPC 买货）
- UI 把报价打成 `%6d` 还是两行（`12-ui-save`）

## 2. 原版事实

### 2.1 两种「价格」不要混

| 符号 | 宽度 | 存在哪 | 含义 |
| --- | --- | --- | --- |
| `TRADINGITEM_DATAPRICE_MIN/MAX` | `USHORT` | 内容 / 特产品槽 | 基础价。World0 槽观察值 100–6000 | [确认] `Price.h`、`asserts.txt`、`towns_world0.json` |
| `MARKET_PRICE_MIN/MAX` | `USHORT` | 港口运行时 | 按**种类**存的指数，不是金币 | [确认] `Price.h`、`CTown` `+0x4a` |

金币报价是算出来的，上限观察为 `1 … 999999`（`fn_14001377d(1, x, 0xf423f)`）。

### 2.2 指数存在港口对象上，19 条

`CTown` 存档 / 复制：

```
for kind in 0 .. 19:
    *(town + 0x4a + kind * 2) = uint16
```

[确认] `CTown::vfunc_07` / `vfunc_06`（`srcdata/source/game/town/CTown.cpp`）；`fields.tsv` `CTown 0x4a×3`。

读取：`fn_1400095ed` → `fn_1402650b0(town + kind*2 + 0x4a)`。RTC 名 `nMarketPrice`。

地方舰队卖货时若 `nMarketPrice < 0x3C`（60）会优先补货该种类 `[确认]` `fn_140010d25`。

### 2.3 指数更新

`fn_140003620` 对 19 个种类调 `fn_140010e83(town, kind, target)`：

1. 读当前指数 `cur`
2. 与目标 `target` 比较（目标来源 `[未知]`，抬码把第三参写成 `*(s58 + kind*4)`）
3. `cur < target`：`cur += (target - cur + 9)` 再加 `rand(2)`（`+9` 后有 `idiv`，步长像 10）
4. `cur == target`：`cur += rand(2) - rand(2)`
5. `cur > target`：`cur += (target - cur - 9)` 再减 `rand(2)`
6. `cur = clamp(cur, 25, 500)` — `fn_1400103d4(0x19, cur, 0x1f4)`
7. 写回 `uint16`

`fn_1400103d4` 是三参夹值：若 `lo <= hi` 返回落在 `[lo, hi]` 的值 `[确认]` `fn_14007d3c0.cpp`。

另外两条短函数（RTC 仍名 `nMarketPrice`）：

- `fn_140009c00`：指数减少（`fn_14000939f(0x19, cur - delta)` 再写回）
- `fn_14000b721`：指数增加后夹到 500（`fn_140011bd0(cur + delta, 0x1f4)`）

买卖后种类指数：`CommitDeal` 对买入种类调 `fn_1400108b6`、卖出种类调 `fn_140007301`（参数含该种类件数与金额合计）。内部公式抬码未还原 `[未知]`。

### 2.4 金币报价 `fn_140010492`

`fn_14000a691(town, item, arg)` 是对外入口：

- 若该 item 是本港特产品（`fn_14000680c != 6`）：走本地分支再进 `fn_140010492`
- 否则取一件基础价（`fn_140007022`）再进 `fn_140010492`

`fn_140010492` 体内能读到的命名参数：

| 量 | 原版观察 | 标记 |
| --- | --- | --- |
| `data_price` | 第三参 / 特产品槽价 | [确认] 量级 |
| `kind_rate` | `fn_1400095ed` = 港口该种类指数 | [确认] |
| `locality_factor` | 见下表 | [确认] 立即数 |
| `relation_factor` | `fn_1400131ce(town, item)` 映射 | [确认] 分支 |
| 两处 `idiv` | 除数未恢复 | [未知] |
| 最终夹值 | `[1, 999999]` | [确认] |

`locality_factor`（`s2C`，立即数）：

| 分支 | 值 |
| --- | --- |
| 类型 1 且就是本港该特产品 | 200 (`0xC8`) |
| 类型 1 但不是 | 150 (`0x96`) |
| 类型 2 | 130 (`0x82`) |
| 类型 3 | 120 (`0x78`) |
| 类型 4 | 110 (`0x6E`) |
| 其他 | 100 (`0x64`) |

类型枚举名 `[未知]`（抬码 `s38 == 1..4`）。卖价路径 `fn_1400097af` 也进同一 `fn_140010492`。

`relation_factor`（`s24`，默认 100）：

| `r = fn_1400131ce(town, item)` | 因子 |
| --- | --- |
| `r < 21` | 100 |
| `21 ≤ r ≤ 100` | `r + 100` |
| `100 < r < 200` | `300 - r` |
| `r ≥ 200` | 100 |

`r` 的语义 `[未知]`（可能是距离 / 亲疏 / 供需差，字段名未恢复）。

玩家交易所**买入**展示价：会话里 `fn_140010771` 读 `CTrade` 上按特产品槽缓存的整数，再 `fn_1400055f1` 取出。这是会话缓存，不是第二套公式。退货按**当前**这套买入展示价退，不按舱位写入的买入价 `[确认]` `trade_stack.md`、`CTrade::RefundJustBought`。

卖出展示价：`fn_140001b04(town_or_view, item, stored_buy_price)`。空格 `item==0x6A` 或 hold `+2==0x62` 时返回 1。若某港口状态 `fn_140010f46==2` 会再乘一次 `[确认]` 分支，乘数 `[未知]`。

### 2.5 航路市价图

`CMarketPriceRouteMap` : `CSelectTownRouteMap` → `CRouteMap` → `CView`。构造加载 `RouteMkr.dk4`。虚函数只做选港 / 画标记，**不存**市价表 `[确认]` `CMarketPriceRouteMap.cpp`。重构后它是 UI，调 `quote()`。

### 2.6 断言

```
MARKET_PRICE_MAX <= _GetDataSizeMax_BYTE2DWORD(sizeof(USHORT))
MARKET_PRICE_MIN <= data && data <= MARKET_PRICE_MAX
TRADINGITEM_DATAPRICE_MAX <= sizeof(USHORT) 同上
```

整数 MIN/MAX 未写进头文件。更新路径观察夹在 25–500，当作原版包缺省，不把 500 写成引擎硬顶。

## 3. 实体与 ID

| 实体 | 键 | 范围 | 存哪 |
| --- | --- | --- | --- |
| 种类市价指数 | `(town_id, trading_item_kind_id)` | 原版包 25–500 | 存档 / 港口状态 |
| 基础价 | `(town_id, slot)` 或目录缺省 | USHORT | 内容包 |
| 金币报价 | 纯函数输出 | 原版包 1–999999 | 不存 |

没有「每港口 × 105 件」的价格矩阵。

## 4. 算法 / 公式

全部参数进 `config/formulas/economy.yaml`。代码里禁止出现 25、500、1000、999999。

### 4.1 夹值

```
clamp(x, lo, hi) = min(hi, max(lo, x))     # 原版 fn_1400103d4，要求调用方保证 lo<=hi
```

### 4.2 种类指数滑动 `kind_rate_step`

```
kind_rate' = clamp(
    kind_rate_towards(kind_rate, target, step_bias, jitter),
    kind_rate_min,          # vanilla 25
    kind_rate_max           # vanilla 500
)
```

`kind_rate_towards`：

```
if kind_rate < target:
    kind_rate += (target - kind_rate + step_bias) / step_div + rand_int(0, jitter)
elif kind_rate > target:
    kind_rate += (target - kind_rate - step_bias) / step_div - rand_int(0, jitter)
else:
    kind_rate += rand_int(0, jitter) - rand_int(0, jitter)
```

`step_bias` 原版 9，`step_div` `[未知]`（`idiv`），`jitter` 原版 2。

### 4.3 金币报价 `quote_gold`

```
quote_gold(town_id, item_id, quote_kind) =
    clamp(
        data_price
        * locality_factor(quote_kind, is_local_special)
        * kind_rate(town_id, kind_of(item_id))
        * relation_factor(town_id, item_id)
        / (locality_base * relation_base),   # 两处 idiv，名字是占位
        gold_quote_min,                      # vanilla 1
        gold_quote_max                       # vanilla 999999
    )
```

`quote_kind` 对应原版类型 1–4，缺省走 `locality_other`。

```
relation_factor(r):
    r = town_item_relation(town_id, item_id)    # fn_1400131ce，语义 [未知]
    if r < relation_low:            return relation_flat        # 21, 100
    if r <= relation_mid:           return r + relation_flat    # 100, 100
    if r < relation_high:           return relation_high + relation_flat - r
    return relation_flat
```

`is_local_special`：该 `item_id` 出现在本港 5 个特产品槽且非 empty。

**除数未恢复。** 实现必须把 `locality_base`、`relation_base` 留在 YAML；在反汇编补全 `idiv` 的除数之前，原版包可以先标 `unverified: true`，校验器拒绝静默用 100/100 当「大概对」。禁止在代码里写死 100。

### 4.4 NPC 买货成本（能力修正）

地方舰队自动交易 `fn_140010d25`（`exe_caps.md`）：

```
raw = quote_gold(...) * qty
a   = ModifierSet.buy_cost_factor          # 原版 Ability(captain, 4)=精神，夹在 [0, ability_settled_cap]
cost = raw * (buy_cost_scale - a + rand_int(0, buy_cost_jitter)) / buy_cost_scale
```

原版包：`buy_cost_scale: 1000`，`buy_cost_jitter: 100`。

`a > buy_cost_scale` 时括号为负，原版 32 位无符号 `div` 会炸成数百万。引擎必须饱和：`buy_cost_factor` 先夹到 `[0, buy_cost_factor_cap]`（补丁曾把能力夹到 500）。经济代码不读六维。

记入账本 `expense`（`04`），无账本则扣势力金币。

## 5. 耦合分析

原版：

- `CTown+0x4a` 把市价焊在港口对象上
- `quote` 直接 `Ability(captain, 4)`
- `CMarketPriceRouteMap` 继承航路图 View，图形资源 `RouteMkr.dk4`
- 买卖后改指数的函数与 `CTrade::CommitDeal` 同文件家族

重构：

- 市价表属 economy，键是 `town_id`
- 港口坐标、名称属 world
- 航路图属 ocean/ui，只调 `quote()`
- 人物只出 `ModifierSet`

## 6. 解耦后的配置

```yaml
# config/formulas/economy.yaml
schema: dk4.formulas.economy
version: 1
kind_rate:
  min: 25
  max: 500
  step_bias: 9
  step_div: null          # [未知] 补全前不要填 10 装确认
  jitter: 2
  prefer_buy_below: 60    # NPC 补货阈值 0x3C
quote:
  gold_min: 1
  gold_max: 999999
  locality_base: null     # [未知]
  relation_base: null     # [未知]
  locality:
    type1_local: 200
    type1_other: 150
    type2: 130
    type3: 120
    type4: 110
    other: 100
  relation:
    low: 21
    mid: 100
    high: 200
    flat: 100
npc_buy_cost:
  scale: 1000
  jitter: 100
  factor_cap: 500         # 写入 caps；原版不封顶会在 a>1000 时炸
```

```yaml
# config/economy/price_curves.yaml
schema: dk4.economy.price_curves
version: 1
# 内容包可覆盖「某种类的目标指数怎么从供给/季节来」
# 原版 target 来源未还原，先留空钩子
kind_target_source: unverified
```

```yaml
# config/caps.yaml（与本篇相关的键）
market_price_min: 25
market_price_max: 500
trading_item_data_price_min: 0      # 符号有，整数 [未知]；观察槽 ≥100
trading_item_data_price_max: 65535  # USHORT 硬顶；内容包观察 ≤6000
gold_quote_max: 999999
buy_cost_factor_cap: 500
```

存档只存 `kind_rate[town_id][kind_id]`（19 个 u16）。不存公式原文。

## 7. 独立旋钮

- 指数上下限、滑动步长、NPC 补货阈值
- 各地倍率、关系曲线拐点
- `buy_cost_scale` / `jitter` / `factor_cap`
- 报价金币夹值

改这些不必改货舱或战斗。`buy_cost_factor` 改成由魅力而不是精神派生，只动人物系统的 ModifierSet 映射。

## 8. 对外接口

```
PriceSnapshot = {
  town_id,
  item_id,
  kind_id,
  kind_rate,          # 指数
  data_price,         # 基础价
  buy_gold,           # quote_gold(..., buy)
  sell_gold,          # quote_gold(..., sell)
}

Economy.quote(town_id, item_id) -> PriceSnapshot
Economy.kind_rate(town_id, kind_id) -> int
Economy.set_kind_rate(town_id, kind_id, value)   # 仅系统内部 / 读档
Economy.on_kind_traded(town_id, kind_id, side, qty, gold)  # Commit 后滑动
```

只传 ID 与整数。不传 `CTown*`、`World*`。

UI：`CMarketPriceRouteMap` 对选中 `town_id` 调 `quote`，画航路。

事件：

```
KindRateChanged { town_id, kind_id, old, new }
```

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| `MARKET_PRICE_MIN/MAX` 头文件整数值 | 只有符号；更新夹 25–500 是观察缺省 |
| `fn_140010e83` 的 target 从哪来 | 第三参未命名 |
| `step_div`（`+9` 后的 `idiv`） | 未恢复 |
| `quote_gold` 两个除数 | 未恢复；YAML 留 `null` |
| `quote_kind` 1–4 的原名 | 立即数分支 |
| `fn_1400131ce` 返回值语义 | 只知分段 |
| `fn_1400108b6` / `fn_140007301` | 买卖后改指数，公式未知 |
| `fn_140001b04` 在 `fn_140010f46==2` 时的乘数 | 有分支无常数 |
| 玩家买入是否 100% 等于 `quote_gold` | 会话缓存 `CTrade+0x2dc` 的填法未完整抬码 |
| 指数初值 | `CTown` 构造写 `+0x4a`，立即数未读出 |
| 季节 / 到货月是否改 target | 攻略有「每月/奇数月」到货，代码未钉到本函数 |
