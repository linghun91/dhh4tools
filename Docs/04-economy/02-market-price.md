# 市价与报价

港口对**种类**存一条市价指数，对**单件货物**再算金币报价。本篇是 `quote()` 的规格。买卖会话怎么扣钱见 `03-trade-loop.md`。

## 1. 职责边界

本系统管：

- 每座港口、每个 `trading_item_kind_id` 的市价指数（存档状态）
- 三张底价表怎么选：目录价（`01`）、港口特产品槽价（`04`）、文化圈卖价（本篇 §2.6）
- 把底价、等级、指数、流行度合成金币报价
- 指数的月度漂移（按港口状态）与买卖后的滑动
- 文化圈卖价的回升
- 航路市价图作为**只读投影**（原版 `CMarketPriceRouteMap`）

不管：

- 交易品目录（`01`）
- 货舱、会话、堆叠、会计长杀价（`03`）
- 特产品供给、可买件数、等级怎么从发展度算出（`04` / `05`）
- 市场份额、合约（`02-factions`；报价公式**不读** share，可买件数才读）
- 港口状态（战争/瘟疫/繁荣…）何时切换（town / world；本系统只按状态取目标）
- 流行品是哪两件、流行度怎么涨（factions / town fashion；本系统只读 `CTown+0x70` 的两格）
- 人物技能值（经修正器注册表 `Modifiers.eval("skill.accounting", ctx)` 只读，见 `06` 的 NPC 买货；DECISIONS D2）
- UI 把报价打成 `%6d` 还是两行（`12-ui-save`）

## 2. 原版事实

### 2.1 四种「价格」不要混

| 符号 / 位置 | 宽度 | 存在哪 | 含义 | 标记 |
| --- | --- | --- | --- | --- |
| 目录 `data_price` | u16 | exe 目录 +8 → `CTradingItem+0x10` | 非特产品买入底价；文化圈价回升上限 | [确认] `01` §2.1 |
| 港口槽价 | u16 | `CTown+0x2e+slot*6`（初值 = 城镇记录 `+0x62+slot*6`） | 本港特产品买入底价 | [确认] `fn_140013881`；`CTown::vfunc_05` 拷贝 |
| 文化圈卖价 | u16 ×105 | `CCulture+0x14+item*2`（初值 = exe VA `0x1405b6160 + c*0xE0 + 0xC`） | 非本港特产品的卖出底价 | [确认] `fn_140010a23`；`CCulture::vfunc_05` 拷贝 |
| `MARKET_PRICE_MIN/MAX` | USHORT | `CTown+0x4a+kind*2` | 按**种类**存的指数（100 = 平价），不是金币 | [确认] `Price.h`、`asserts.txt:294` |

`TRADINGITEM_DATAPRICE_MIN/MAX` 是 USHORT `[确认]` `asserts.txt`，整数未写进头文件。

金币报价是算出来的，夹在 `[1, 999999]`（`fn_14001377d(1, x, 0xf423f)`，**无符号**比较）`[确认]`。

### 2.2 指数存在港口对象上，19 条

```
for kind in 0 .. 19:
    *(u16*)(town + 0x4a + kind * 2)
```

- `[确认]` 存读档 `CTown::vfunc_07` / `vfunc_06`；`fields.tsv` `CTown 0x4a×3`。
- `[确认]` 初值 **100**：`CTown::vfunc_05`（body `0x1402ae3c0`）对 19 种写 `0x64`（`0x1402ae64b`）。
- 读取：`fn_1400095ed(town, kind)`。RTC 名 `nMarketPrice`。

地方舰队卖货时把「指数 `< 0x3C`」的货写进一个局部，函数后半**不再读**它 `[确认 只写]`（`06` §2.4）。旧稿「优先补货」没有依据，删除。

### 2.3 月初漂移（港口状态 → 目标）

`[确认]` `CTownElement::vfunc_04`（body `0x1402668f0`）在 `fn_14000e9da(calendar)` 为真（`SCalendar+0x1c == 0`，即月初那一拍，见 `01-world/07-calendar-and-coords.md` §2.3）时，对 97 座活港（对象步长 `0x78`，`vfunc+0x20` 判活）按 `CTown+0x74`（`fn_14001244f`）查跳表 `0x140267cc8`（13 项）：

| 状态 | 动作 | 目标表（`.data`，19 个 i32；文件偏移 = VA − `0x140001600`） |
| --- | --- | --- |
| 0 | 若 `CTown+0x75 == 1`：武器(16) **直接写** 150（`fn_140004930`，不夹）；否则向 T_war 走一步。**然后**再向 T_food 走一步 | T_war `0x1406879e0`：全 150，武器 200 |
| 1 | 向 T_plague 走一步 | `0x140687980`：种类 0–4 为 120，5–13 为 60，药(14) 200，15–18 为 60 |
| 2 | 向 T_food 走一步 | `0x140687a40`：食品(0) 180，1–4 为 140，5–18 为 80 |
| 3 | 19 种全部 `+= 4 + rand(2)`，封 500 | — |
| 4 | 19 种全部 `-= 2 + rand(2)`，底 25 | — |
| 5–11 | 某一种 `+= 50`，封 500。种类 = `fn_140011379(state)`：5→调味料 1、6→香辣料 3、7→嗜好品 4、8→酒类 5、9→纤维 8、10→染料 15、11→矿石 17 | — |
| 12 | 向 T_normal 走一步 | `0x140687920`：全 100 |
| >12 | `fn_140011379` 返回 20，不动 | — |

状态名 `[推断]`：mesfile `0x638f`–`0x64af` 一组「景气？」回答依次是「希望尽快恢复和平」「发生瘟疫」「吃饭问题」「笑到停不下来」「请不要问」，与 0–4 的目标表（武器高 / 药高 / 食品高 / 全涨 / 全跌）逐条对上；12 走全 100 表即平常。枚举名未恢复。

状态的写入 `[确认]`：`fn_14000bcf8(town, state, flag)`（body `0x140265d40`）写 `CTown+0x74 = state`、`+0x75 = flag`，`state != 12` 时再调 `fn_14000db4d(town)`。`07` §4.4 投资「越来越繁荣」调 `fn_14000bcf8(town, 3, 1)` = 切到 boom 并置 `+0x75`。`fn_14000c509`（body `0x140265270`）写 `+0x74 = 12`、`+0x75 = 0` `[确认]`（`07` §4.4「景气恢复」同一函数）。`+0x75` **不是**单纯标志，是「本状态已持续月数」计数：写入方都传 1，`fn_140002d1a`（body `0x14025dfa0`）每月加 1，`fn_1400092b9`（body `0x140262aa0`）读它 `[确认]`。状态 0 时它为 1（即入状态后的第一个月初）武器直接写 150。

同一 `vfunc_04` 在漂移**之前**还有三段（都只在月初那一拍）`[确认]` `0x140266fd2`–`0x140267440`：

```
# ① 结束 / 计月：对每座状态 != 12 的活港
if state in 5..11:                                   # fn_140011379(state) != 20
    fn_14000c509(town)                               # 回 normal，+0x75 = 0
    rate[kind_of(state)] -= 50                       # fn_140009c00(town, kind, 0x32)，底 25
else:                                                # 0..4
    if months >= 3 and rand(3) == 0:                 # fn_1400092b9；fn_140005272(3)
        if state == 0: rate[16] = 80                 # fn_140004930(town, 0x10, 0x50)，武器直接写
        fn_14000c509(town)
    else:
        if months < 7: months += 1 else fn_14000c509(town)   # fn_140002d1a
# ② 蔓延：从仍非 normal 的港里 rand 挑一座 src，再从它的连接港（aeConnectTown，fn_140003b2a）rand 挑一座 dst
if state(dst) != state(src): fn_14000bcf8(dst, state(src), 1)
# ③ 新发：rand(10) == 0 时
town = fn_140007fbd(8)                               # 随机港，参数 8 语义未拆
row  = fn_140014e48(town)                            # = 该港 major_sea_id（§9 末行）
s = T_new[row][rand(10)]                             # .rdata 0x1405a0790，8 行 × 10
if s == 5: s = T_demand[row][rand(10)] + 5           # .rdata 0x1405a08e0 → 5..11
fn_14000bcf8(town, s, 1)
```

`T_new`（行 0–7）：`0,0,0,1,1,3,3,4,4,5` / 同行 0 / `0,1,1,1,2,2,2,3,4,5` / `0,0,0,1,1,2,2,3,4,5` / `0,0,1,1,2,2,2,3,4,5` / 同行 4 / `1,1,1,1,2,2,3,4,5,5` / 全 0。`T_demand`：`1,1,1,1,2,2,2,4,5,6` / 同行 0 / `0,0,3,3,4,4,4,5,6,6` / `0,1,1,1,2,2,5,5,6,6` / `0,0,2,2,3,3,4,4,5,6` / `0,0,2,3,3,4,4,5,5,6` / `0,0,1,1,2,2,3,3,4,6` / 全 0。重构进 `economy/town_state_price_rules.yaml` 的 `lifecycle:` 段（§6），不在代码里写。其余调用 `fn_14000c509` 的点：`fn_14000938b`（`0x14016b53d`，消息 `0x32b`–`0x32d` 的某事件）、`fn_140009ae8`（`0x14025f624`，发展度里程碑），未逐条拆。

本篇统一用的状态 key（`07` 上缴 / 繁荣、`05-ships-fleet` 造船计价同用这套，不再叫 `town_kind`）：

| 原值 | key | 价格侧 | 上缴（`07` §4.5） | 造船 / 卖船（§2.9） |
| --- | --- | --- | --- | --- |
| 0 | `war` | 武器高 | — | 买 ×2 |
| 1 | `plague` | 药高 | — | — |
| 2 | `famine` | 食品高 | — | — |
| 3 | `boom` | 全涨 | ×3/2 | 买 / 卖 ×12/10 |
| 4 | `slump` | 全跌 | ×1/2；投资达标发「景气恢复」 | 买 / 卖 ×8/10 |
| 5–11 | `demand_*` | 单种 +50 | — | — |
| 12 | `normal` | 回 100 | 投资翻倍发「越来越繁荣」并切 `boom` | — |

走一步 `fn_140010e83(town, kind, target)`（`[确认]` 反汇编 `0x14025dcf0`）：

```
cur < target:  cur += (target - cur + 9) / 10 + rand(2)     # idiv ecx=10，有符号截断
cur == target: cur  = cur - rand(2) + rand(2)
cur > target:  cur += (target - cur - 9) / 10 - rand(2)
cur = clamp(cur, 25, 500)                                    # fn_1400103d4(0x19, cur, 0x1f4)，有符号
```

`rand(n)` = `fn_140005272` = `rand() % n`，取值 `[0, n)`；`rand(2)` 只有 0/1 `[确认]`。

状态动作之后，同一循环再按**两格流行**推种类 `[确认]` `0x14026765e`–`0x140267762`：

```
for i in 0, 1:
    slot = town.fashion[i]                   # CTown+0x70+i*2：u8 item、u8 level
    if slot.item == 0x6A: continue
    k = kind_of(slot.item)
    if slot.level < 120:                     # 0x78
        rate[k] += culture_b(town, 0) + 4 + rand(5)      # fn_14000b721，封 500
    else:
        rate[k] -= rand(culture_b(town, 1) + 1)          # fn_140009c00，底 25
```

`culture_b(town, i)` = `fn_1400077bb` = `CCulture+0x11+i`（初值 = 静态文化行 +9 / +10，例：英吉利 7 / 4）；港口无文化圈时返回 7 `[确认]`。

### 2.4 买卖后的滑动

`[确认]` `CTrade::CommitDeal`（body `0x140277090`）按种类统计后调用；NPC 自动交易（`06`）调同一对函数。

买入 `fn_1400108b6(town, kind, n, avg)` / 卖出 `fn_140007301(town, kind, n, avg)`：

```
ref = mean( P(town, item, grade=4) for item in kind if discovered(item) )   # 无符号整除
      # 买：P = fn_14000a691（买入报价）；卖：P = fn_1400097af（文化圈卖价报价）
if count == 0 or ref == 0: return
delta = clamp(n * avg / ref, 1, 13)          # 32 位 imul + 无符号 div；fn_1400103d4(1, ·, 0xd)
买：rate[kind] += delta    （fn_14000b721，封 500）
卖：rate[kind] -= delta    （fn_140009c00，底 25）
```

`CommitDeal` 传入的 `avg`：

- 买：`Σ fn_14000a691(town, item, hold.grade) / n`，对本笔 `just_bought` 的格。用**确认时**的报价，不是格里记的买价
- 卖：`Σ fn_140001b04(town_id, item, hold.grade) / n`，对卖出列表的格

`n * avg / ref` ≈ 「本笔金额折合多少件参考货」。原版每格 1 件、一舰队最多 25 格，所以一笔最多推 13。

### 2.5 金币报价 `fn_140010492`

签名 `[确认]`：`(town*, data_price, item, grade)`。反汇编 `0x140262ac0`：

```
g = 100
if grade == 1: g = (item == star_item()) ? 200 : 150     # fn_14025fba0 → CTradingItemElement+0x1a48
elif grade == 2: g = 130
elif grade == 3: g = 120
elif grade == 4: g = 110

r = fashion_level(town, item)       # fn_1400131ce：扫 CTown+0x70 两格，item 相同取 +0x71 字节，否则 0
f = 100
if   21 <= r <= 100: f = r + 100
elif 100 < r < 200:  f = 300 - r
# r < 21 或 r >= 200：f = 100

q = data_price * g * rate[kind_of(item)]       # 32 位 imul
q = q / 100                                     # 无符号 div
q = q * f / 10000                               # imul，再无符号 div
return clamp_u(q, 1, 999999)
```

旧稿把 `g` 叫「本地倍率 / 类型 1–4」、把 `r` 叫「关系，语义未知」、两处除数写成未恢复——全部更正：`g` 是**等级倍率**（第四参就是货格 `+1` 等级，见 `05` §2.4），`r` 是**流行度**（`CTown+0x70` 两格，`06` §2.4 同一结构），除数是 **100** 与 **10000**。

`star_item`：年初抽中的那件 1 级品牌（`05` §2.5）。只有它在 1 级时给 200%，其他 1 级给 150%。

`f` 是钟形：流行度 100 时 ×2，200 起回落到 ×1。

溢出：最大 `9500 × 200 × 500 = 9.5e8` < 2³¹；`/100` 后 `× 200` < 2³²。原版包不溢出。

#### 底价从哪来

| 入口 | 用途 | 底价 | 标记 |
| --- | --- | --- | --- |
| `fn_14000a691(town, item, grade)` | 买入 | `slot = fn_14000680c(town, item)`（扫 5 槽，`item==0x6A` 或没命中返回 **6**）。`slot != 6` → 港口槽价 `CTown+0x2e+slot*6`；否则目录 `data_price` | [确认] `0x140262c70` |
| `fn_1400097af(town, item, grade)` | 卖出底层 | 文化圈卖价 `CCulture[culture(town)].price[item]`。`culture(town)` = 城镇记录 `+0x25`（`fn_14001055a`）；`0x16` 视为无文化圈、返回空指针 | [确认] `0x140262d40`、`fn_140013a02` |
| `fn_140001b04(town_id, item, grade)` | 玩家看到 / 拿到的卖价 | 见下 | [确认] `0x1402758a0` |

`fn_140001b04` `[确认]`：

```
if item == 0x6A or town_id == 0x62: return 1          # 0x62 = 无港哨兵
slot = fn_140276350(town_id, item)                     # 本港特产品且等级 < 6 才算命中；否则 6
p = (slot != 6) ? fn_14000a691(town, item, grade) / 2  # 在产地卖 = 产地买价的一半
                : fn_1400097af(town, item, grade)
if fn_140010f46() == 2: p = p * 105 / 100              # 无符号
return max(1, p)                                        # fn_1400013cf
```

第三参是**货格里的等级**（`hold+1`），所有调用者（`DrawCargoList`、`OnClickSlot`、`SellAll`、`WithdrawSell`、`CommitDeal`）都这样传；第一参是当前港 `town_id`。旧稿写成「`(town_or_view, item, stored_buy_price)`，`hold+2==0x62` 返回 1」，更正。

`fn_140010f46()` = `DAT_144296500+0x7c`（`fn_140004a1b`）= **`eScenario`** `[确认]`：`fn_140004381`（玩家势力）= `fn_140009df9(DAT+0x7c)` 用 7 元素表 `{0,2,1,3,20,13,21}` 取势力，`NUM_SCENARIO=7`，`fn_14000864d` 以 `>=4` 判 World1（证据见 `07` §2.1）。`==2` 是丽璐剧本。同一判断还把流行品影响力翻倍（`03` §2.6）。重构**禁止**比较剧本号：剧本 2 的剧本包在开局向修正器注册表登记 `trade.sell_price_pct` 贡献，影响力翻倍登记 `factions.influence_gain`（`ctx.source: fashion_sale`）（DECISIONS D5，§6）。

### 2.6 文化圈卖价表

- `[确认]` 静态初值：exe `.rdata` VA `0x1405b6160`（文件偏移 `0x5b4f60`；不要把 VA 低位当文件偏移读），21 行，步长 `0xE0`（`fn_140010f73`）。行布局：+0 名称指针（英吉利、佛兰德、德意志、北欧、葡萄牙、西班牙、意大利、希腊、土耳其、埃及、西非、东非、阿拉伯、印度、印度支那、印尼、中国、朝鲜、日本、加勒比海、墨西哥）、+8 u8（0–6，与七大海域同序）、+9 / +10 两个 u8（月初流行推力，§2.3）、+0xC 105 个 u16 卖价（观察 50–9600）。
- `[确认]` `CCulture::vfunc_05`（`0x1402ac830`）把 +8→`+0x10`、+9/+10→`+0x11/+0x12`、105 价→`+0x14` 拷进运行时对象（步长 `0xE8`）。
- 运行时只有两处写价（`fn_140006ae1` 的全部调用者）：

**同一货本笔卖 ≥15 件** `[确认]` `CommitDeal` `0x1402777a6`–`0x140277896`：

```
for item in 0 .. 105:
    if sold_count[item] < 15: continue          # anSellTradingItemNum，原版=格数
    base = catalog[item].data_price
    cp   = culture_price[culture(town)][item]
    if base == 0 or base <= cp: continue        # 无符号
    cp = cp + (base - cp) * 2 / 10              # 无符号
    culture_price[...][item] = min(cp, base)
```

整片文化圈的该货收购价向目录价收拢 20%，**只升不降**，以目录价封顶。

**`fn_14000c658`**（body `0x140166120`，唯一调用者 `fn_140011e19`，RTC `asShipLoad` / `nLoadTotal`）：`cp += (base - cp) * qty / 50`，再 `min(·, base)`。是哪种玩法（推销 / 发放）`[未知]`。

没有找到让文化圈价**下降**的写入点。存读档原样。

### 2.7 航路市价图

`CMarketPriceRouteMap` : `CSelectTownRouteMap` → `CRouteMap` → `CView`。构造加载 `RouteMkr.dk4`。虚函数只做选港 / 画标记，**不存**市价表 `[确认]` `CMarketPriceRouteMap.cpp`。重构后它是 UI，调 `quote()`。

### 2.8 断言

```
MARKET_PRICE_MAX <= _GetDataSizeMax_BYTE2DWORD(sizeof(USHORT))
MARKET_PRICE_MIN <= data && data <= MARKET_PRICE_MAX
TRADINGITEM_DATAPRICE_MAX <= sizeof(USHORT) 同上
```

整数 MIN/MAX 未写进头文件。代码里所有改指数的路径都夹 `[25, 500]`（`fn_140010e83` 双向、`fn_14000b721` 上、`fn_140009c00` 下），只有状态 0 的「武器直接写 150」绕过夹值。当作原版包缺省，不把 500 写成引擎硬顶。

### 2.9 港口状态对其他价格的倍率

`[确认]` `fn_14000643d(town, &price)`（body `0x140261a60`）按 `fn_14001244f`（`CTown+0x74`）改写传入的价：0 → `price<<1`；3 → `price*12/10`；4 → `price<<3 / 10`（两处都是无符号 `div 10`）；其它不动。调用点 4 个：`0x1400aa196`、`0x1401760d7`、`0x1401764be`、`0x14022b8f7`（造船 / 改装计价，用途归属见 `05-ships-fleet`）。

`[确认]` 卖船 `fn_14000bb95`（body `0x140177f60`）不调它，自己内联：3 → `*12/10`，4 → `<<3 /10`；没有 0 → ×2 这档（`0x1401780ce`–`0x14017810f`）。

这些倍率不改交易品报价（`quote_gold` 不读 `+0x74`），但和市价漂移是同一个状态字节。本系统把它们收成一张表供别的系统查询（§6 `economy/town_state_price_rules.yaml` 的 `contexts` 段）。

## 3. 实体与 ID

| 实体 | 键 | 范围 | 存哪 |
| --- | --- | --- | --- |
| 种类市价指数 | `(town_id, trading_item_kind_id)` | 原版包 25–500，初值 100 | 存档 / 港口状态 |
| 港口槽价 | `(town_id, slot)` | USHORT | 内容包初值；运行时可改 `[推断]`（setter `fn_14000d7a1` 存在、未见直接调用） |
| 文化圈卖价 | `(culture_id, trading_item_id)` | USHORT | 内容包初值；存档 |
| 港口状态 | `town_id → town_state_id` | 原版 0–12 | town 拥有；本系统只读 |
| 金币报价 | 纯函数输出 | 原版包 1–999999 | 不存 |

没有「每港口 × 105 件」的价格矩阵。卖价矩阵是「每文化圈 × 105」。

## 4. 算法 / 公式

全部参数进 `config/formulas/economy.yaml`。代码里禁止出现 25、500、100、10000、999999、15、13。

### 4.1 夹值

```
clamp(x, lo, hi) = min(hi, max(lo, x))     # 原版 fn_1400103d4，有符号
```

报价末尾的 `[1, 999999]` 原版是无符号比较；引擎用足够宽的有符号整数，结果相同。

### 4.2 种类指数：月初漂移

```
on_month_start(town_id):              # schedule: month_start；与 07 转化、04 补货的先后见 04 §6 tick 片段
    rule = state_rules[town_state(town_id)] or state_rules.default
    apply(rule)                         # step_to / add_all / add_kind / set_kind，见 YAML
    for slot in Fashion.current(town_id): # factions 只读查询
        k = kind_of(slot.item_id)
        if slot.level < fashion_push.level_cut:
            add(k, culture.fashion_push_up + fashion_push.up_base + rand_int(0, fashion_push.up_jitter))
        else:
            add(k, -rand_int(0, culture.fashion_push_down))

step_to(kind, target):
    d = target - rate
    if d > 0:  rate += trunc((d + step.bias) / step.div) + rand_int(0, step.jitter)
    elif d < 0: rate += trunc((d - step.bias) / step.div) - rand_int(0, step.jitter)
    else:      rate += rand_int(0, step.jitter) - rand_int(0, step.jitter)
    rate = clamp(rate, kind_rate.min, kind_rate.max)
```

`rand_int(0, n)` 指 `[0, n]` 闭区间；原版 `rand(2)` 对应 `jitter: 1`，`rand(5)` 对应 `up_jitter: 4`，`rand(b+1)` 对应 `[0, b]`。

### 4.3 种类指数：成交滑动

```
on_kind_traded(town_id, kind_id, side, n, avg_gold):
    ref = mean(P_side(town_id, item, trade_slide.ref_grade) for item in kind if discovered)
    if n == 0 or ref == 0: return
    delta = clamp(n * avg_gold / ref, trade_slide.min, trade_slide.max)
    rate += delta if side == buy else -delta
    rate = clamp(rate, kind_rate.min, kind_rate.max)
```

### 4.4 金币报价 `quote_gold`

```
quote_gold(town_id, item_id, grade, base_price) =
    clamp(
        base_price
        * grade_pct(grade, item_id == star_item)
        * kind_rate(town_id, kind_of(item_id)) / quote.rate_div       # 100
        * fashion_pct(fashion_level(town_id, item_id)) / quote.pct_div2  # 10000
        , quote.gold_min, quote.gold_max)

fashion_pct(r):
    if fashion.rise_from <= r <= fashion.peak:   return quote.flat + r
    if fashion.peak < r < fashion.fall_to:       return fashion.fall_to + quote.flat - r
    return quote.flat
```

保留两次整除的顺序（先 `/100` 再 `/10000`），否则与原版差 1。

### 4.5 三种报价入口

```
buy_quote(town_id, item_id, grade):
    slot = special_slot_of(town_id, item_id)
    base = slot ? special_price(town_id, slot) : catalog[item_id].data_price
    return quote_gold(town_id, item_id, grade, base)

sell_base(town_id, item_id, grade):
    return quote_gold(town_id, item_id, grade, culture_price(Culture.of_town(town_id), item_id))

sell_quote(town_id, item_id, grade):            # 玩家与 CommitDeal 用这个
    if item_id == empty or town_id == none: return sell.min
    if special_slot_of(town_id, item_id) with grade_rank < unavailable:
        p = buy_quote(...) * sell.at_source_num / sell.at_source_den      # 1/2
    else:
        p = sell_base(...)
    p = p * Modifiers.eval("trade.sell_price_pct", ctx) / 100           # 缺省 100；原版剧本 2 的剧本包贡献 105
    return max(sell.min, p)
```

### 4.6 文化圈卖价回升

```
on_bulk_sold(town_id, item_id, count):          # CommitDeal 每件统计后
    if count < culture_recover.bulk_qty: return    # 15
    base = catalog[item_id].data_price
    cp = culture_price(c, item_id)
    if base <= cp: return
    culture_price(c, item_id) = min(base, cp + (base - cp) * culture_recover.num / culture_recover.den)   # 2/10
```

### 4.7 NPC 买货成本

见 `06` §2.6。本系统只出 `quote_gold`；提督会计术技能值与 500 夹紧在 `06`。`fn_14000e1ce(person, k)` 的 `k` 是 `eSkill`，4 = 会计术 `[确认]`（`fn_1400099e9` 断言 k<10、查技能配对表 `.data 0x1406864a0`；会计长杀价 `fn_1400135cf(purser, 4, 0x14)` 同一下标，`03` §2.7），不是精神。

## 5. 耦合分析

原版：

- `CTown+0x4a` 把市价焊在港口对象上；`+0x74` 状态字节直接驱动目标表
- 文化圈卖价焊在 `CCulture`（factions 的文化圈对象）里
- `quote` 直接读 `CTradingItemElement+0x1a48`（星品）与 `CTown+0x70`（流行）
- 卖价按剧本号 `==2` 加 5%
- `CTown+0x74` 一个字节同时驱动市价漂移、独占上缴倍率（`07`）、造船 / 卖船价（`05-ships-fleet`）
- `CMarketPriceRouteMap` 继承航路图 View，图形资源 `RouteMkr.dk4`
- 买卖后改指数的函数被 `CTrade::CommitDeal` 与 NPC 自动交易共用

重构：

- 市价指数、文化圈卖价、港口槽价都属 economy，键是 `town_id` / `culture_id`
- 港口景气状态（`CTown+0x74`）属 economy（`00-overview/06`）：月初状态机在本系统，别的系统经 `Economy.town_state(town_id)` 读
- 流行两格属 factions；economy 只查 `(item_id, level)`
- 星品属 economy（`05`）
- 剧本差异改成剧本包登记的修正器（`trade.sell_price_pct`；流行品影响力 ×2 走 `factions.influence_gain`，`ctx.source: fashion_sale`，由 factions 结算），经济不读剧本号
- 港口状态对别的系统的价格倍率集中在本系统 `economy/town_state_price_rules.yaml`（`contexts` 段），造船（`ship_buy`）/ 改装（`remodel`）按 `context` 查询，不各自比 0/3/4
- 航路图属 ocean/ui，只调 `quote()`

## 6. 解耦后的配置

```yaml
# config/formulas/economy.yaml（本篇段）
schema: dk4.formulas.economy
version: 1
kind_rate:                  # 种类市价指数初值 / 夹值（§2.2、§2.8）
  impl: vanilla.kind_rate
  params:
    initial: 100
    min: 25
    max: 500
step:                       # step_to，fn_140010e83（§4.2）
  impl: vanilla.step
  int_mode: vanilla_i32     # idiv ecx=10，有符号截断
  rng_stream: economy
  params:
    bias: 9
    div: 10
    jitter: 1               # rand(2) → [0,1]
fashion_push:               # 月初两格流行推种类
  impl: vanilla.fashion_push
  schedule: month_start
  rng_stream: economy
  params:
    level_cut: 120
    up_base: 4
    up_jitter: 4            # rand(5)
    # 上推额外量 / 下拉上限取文化圈 fashion_push_up / fashion_push_down
    no_culture_value: 7
trade_slide:                # 成交滑动，fn_1400108b6 / fn_140007301（§4.3）
  impl: vanilla.trade_slide
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }   # 32 位 imul + 无符号 div
  params:
    ref_grade: 4
    min: 1
    max: 13
quote:                      # quote_gold，fn_140010492（§4.4）
  impl: vanilla.quote_gold
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }   # /100、/10000 均无符号 div
  params:
    star_grade1_pct: 200
    rate_div: 100
    pct_div2: 10000
    flat: 100
    gold_min: 1
    gold_max: 999999
  tables:
    grade_pct: { 1: 150, 2: 130, 3: 120, 4: 110, default: 100 }
fashion:                    # fashion_pct 钟形（§4.4）；报价侧，不是 factions 的流行品份额
  impl: vanilla.fashion_pct
  params:
    rise_from: 21
    peak: 100
    fall_to: 200
sell:                       # sell_quote，fn_140001b04（§4.5）
  impl: vanilla.sell_quote
  modifiers: trade.sell_price_pct
  params:
    min: 1
    at_source_num: 1
    at_source_den: 2
culture_recover:            # 文化圈卖价回升（§4.6；F-ECO-10）
  impl: expr
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }   # 原版比较与 *2/10 均无符号
  params:
    bulk_qty: 15
    num: 2
    den: 10
  inputs: [count, base, cp]        # base = 目录 data_price；cp = 文化圈卖价
  expr: "count < bulk_qty || base <= cp ? cp : min(base, cp + (base - cp) * num udiv den)"
```

```yaml
# config/economy/town_state_price_rules.yaml
schema: dk4.economy.town_state_price_rules
version: 1
schedule: month_start         # 订阅 time 事件 MonthStarted；不读 SCalendar 字段
targets:                     # 19 项，按 kind_id
  normal: [100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100]
  war:    [150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,200,150,150]
  plague: [120,120,120,120,120,60,60,60,60,60,60,60,60,60,200,60,60,60,60]
  food:   [180,140,140,140,140,80,80,80,80,80,80,80,80,80,80,80,80,80,80]
rules:                       # 键是 town 系统的状态 key；原版 id 写在注释
  war:      # 0
    - { op: set_kind, kind: vanilla:weapon, value: 150, when: town_flag_0x75 }   # 不夹值
    - { op: step_to, target: war, unless: town_flag_0x75 }
    - { op: step_to, target: food }
  plague:   [ { op: step_to, target: plague } ]          # 1
  famine:   [ { op: step_to, target: food } ]            # 2
  boom:     [ { op: add_all, base: 4, jitter: 1 } ]      # 3
  slump:    [ { op: add_all, base: -2, jitter: -1 } ]    # 4
  demand_seasoning: [ { op: add_kind, kind: vanilla:seasoning, value: 50 } ]   # 5
  demand_spice:     [ { op: add_kind, kind: vanilla:spice, value: 50 } ]       # 6
  demand_stimulant: [ { op: add_kind, kind: vanilla:stimulant, value: 50 } ]   # 7
  demand_alcohol:   [ { op: add_kind, kind: vanilla:alcohol, value: 50 } ]     # 8
  demand_fiber:     [ { op: add_kind, kind: vanilla:fiber, value: 50 } ]       # 9
  demand_dye:       [ { op: add_kind, kind: vanilla:dye, value: 50 } ]         # 10
  demand_ore:       [ { op: add_kind, kind: vanilla:ore, value: 50 } ]         # 11
  normal:   [ { op: step_to, target: normal } ]          # 12
  default:  []
lifecycle:                   # §2.3 ①②③，在 rules 漂移之前跑；同一 month_start
  end:
    demand_reverts_after_months: 1      # 5–11：下个月初回 normal，并把该种 -= 50（底 25）
    demand_revert_add: -50
    min_months: 3                       # 0–4：持续月数 >= 3 才掷
    chance: { num: 1, den: 3 }          # rand(3) == 0
    max_months: 7                       # 计到 7 仍未结束则强制回 normal
    on_end: { war: { set_kind: vanilla:weapon, value: 80 } }   # 状态 0 结束时武器直接写 80，不夹
  spread: { enabled: true, via: connected_towns, start_months: 1 }   # aeConnectTown；目标已同状态则不动
  new_state:
    chance: { num: 1, den: 10 }
    town_pick: vanilla_random_town_8    # fn_140007fbd(8)，参数语义未拆
    start_months: 1
    roll: 10                            # rand(10) 取列
    first_table: from_export            # .rdata 0x1405a0790，行 = major_sea_id（0–7），值 0–4 = 状态，5 = 改查 demand_table
    demand_table: from_export           # .rdata 0x1405a08e0，值 +5 = 状态 5–11；逐行见 §2.3，导出脚本生成，不手抄
    # 行 7 两表全 0（=war）；fn_140014e48 何时返回 7 未查，原样导出
```

状态 key 的中文名是 `[推断]`（§2.3）。`town_flag_0x75` = 状态持续月数 `+0x75 == 1`（入状态后第一个月初；§2.3），原名保留到命名为止。状态 key 由 economy 拥有（`00-overview/06`），`07` 上缴、`05-ships-fleet` 计价引用同一套 key。

```yaml
# config/economy/town_state_initial.yaml   （本篇拥有；开局景气状态，世界 DLL 城镇记录 +0x82，见 01-world/02 §6）
schema: dk4.economy.town_state_initial
version: 1
default: normal              # 原版包全部港口为 12 = normal
towns: {}                    # 只列非 normal 的港口：{ vanilla:london: boom }
```

```yaml
# config/economy/culture_prices.yaml   （原版包由导出脚本从 exe .rdata 0x1405b6160 按段换算生成，不手抄）
schema: dk4.economy.culture_prices
version: 1
# 所有权（DECISIONS D10）：文化圈行的 key、名字、major_sea 归 factions（02-factions/06）；
# 本表只拥有数值列：105 卖价与两个月初流行推力。以 culture key 关联，不按下标。
default_sell_price: from_catalog_data_price   # 缺项走目录价；加载期报覆盖率
coverage: warn                                # 原版包要求 105/105，模组缺项只告警
cultures:
  - culture: vanilla:england   # 原版下标 0「英吉利」
    fashion_push_up: 7         # 静态 +9 → CCulture+0x11
    fashion_push_down: 4       # 静态 +10 → CCulture+0x12
    sell_price: { vanilla:wheat: 800, vanilla:rice: 1230, vanilla:corn: 1550, vanilla:wool: 850, vanilla:coal: 1350 }   # 稀疏映射，原版 105 项
no_culture:                    # 港口 culture = null（原版 0x16）
  fashion_push: 7              # fn_1400077bb 无文化圈返回 7
  sell_quote: null             # 原版返回空指针；sell_base 不可用
```

```yaml
# config/economy/town_state_price_rules.yaml（续：同一文件的 contexts 段；本篇拥有；别的系统按 context 查询）
# 键是 town 状态 key（§2.3 表）；原版 CTown+0x74。倍率 = num/den，整数无符号截断
contexts:
  ship_buy:        # fn_14000643d，4 个调用点（造船 / 改装，05-ships-fleet 认领具体用途）
    war:   { num: 2,  den: 1 }
    boom:  { num: 12, den: 10 }
    slump: { num: 8,  den: 10 }
  remodel:         # 改装；原版与 ship_buy 同一函数 fn_14000643d、同值。单列一行，模组可单独调（词表 §14.14）
    war:   { num: 2,  den: 1 }
    boom:  { num: 12, den: 10 }
    slump: { num: 8,  den: 10 }
  ship_sell:       # fn_14000bb95 内联；无 war 档
    boom:  { num: 12, den: 10 }
    slump: { num: 8,  den: 10 }
  exclusive_tribute:   # 07 §4.5；表在这里，公式在 07
    boom:  { num: 3, den: 2 }
    slump: { num: 1, den: 2 }
default: { num: 1, den: 1 }
```

查询：`Economy.town_state(town_id)` 取状态 key，再按 `context` 查本表得 `(num, den)`。造船、改装、卖船、上缴都只走这一个出口，代码里不出现 0 / 3 / 4。

```yaml
# 剧本包 scenario 片段（narrative 拥有；DECISIONS D5。本篇只列要读的修正器 key）
- key: vanilla:lulu                   # 原版 eScenario 2（字段名以 10-narrative/01 v2 为准）
  modifiers:
    - { key: trade.sell_price_pct,   stage: mul, value: { num: 105, den: 100 } }              # 02 §4.5
    - { key: factions.influence_gain, stage: mul, value: { num: 2, den: 1 }, when: { source: fashion_sale } }  # 03 §2.6；factions 结算，economy 公式不再乘
# 剧本 0 军事投资八折、剧本 6 附庸税见 07 §6
```

```yaml
# config/caps.yaml（与本篇相关的键）
market_price_min: 25
market_price_max: 500
trading_item_data_price_max: 65535  # USHORT 硬顶；目录观察 ≤9500
gold_quote_max: 999999
```

存档按 key 存 `kind_rate[town][kind]` 与 `culture_price[culture][item]`（DECISIONS D7：不按下标、模组条目原样保留）。不存公式原文，不存派生的金币报价。

## 7. 独立旋钮

- 指数初值 / 上下限、滑动步长、抖动
- 各港口状态的目标表与动作（改「战争时武器 200」只动规则表）
- 等级倍率、星品倍率、流行钟形拐点
- 成交滑动的参考等级与 `[1,13]`
- 产地卖出折扣 1/2、主角卖价加成
- 文化圈卖价回升门槛 15 与比例 2/10

改这些不必改货舱或战斗。新增港口状态只加规则行，不改代码。

## 8. 对外接口

```
PriceSnapshot = {
  town_id,
  item_id,
  kind_id,
  grade,              # 调用方给；买货栏用 grade_rank，卖货用 hold.grade
  kind_rate,          # 指数
  buy_gold,           # buy_quote
  sell_gold,          # sell_quote
}

Economy.quote(town_id, item_id, grade) -> PriceSnapshot
Economy.kind_rate(town_id, kind_id) -> int
Economy.culture_price(culture_id, item_id) -> int
Economy.set_kind_rate(town_id, kind_id, value)   # 仅系统内部 / 读档
Economy.on_kind_traded(town_id, kind_id, side, qty, avg_gold)   # Commit / NPC 后滑动
Economy.on_bulk_sold(town_id, item_id, qty)
Economy.town_state(town_id) -> town_state_key   # §2.9；倍率按 context（ship_buy / remodel / ship_sell / exclusive_tribute）查 town_state_price_rules.yaml
```

只传 ID 与整数。不传 `CTown*`、`World*`。

本系统查询：

```
Fashion.current(town_id) -> [ { trading_item_id, level } ]   # factions；0–2 项
Culture.of_town(town_id) -> culture_id | null                # factions
Modifiers.eval("trade.sell_price_pct", ctx) -> int      # 缺省 100
```

UI：`CMarketPriceRouteMap` 对选中 `town_id` 调 `quote`，画航路。

事件：

```
KindRateChanged { town_id, kind_id, old, new }
CulturePriceChanged { culture_id, item_id, old, new }
```

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| `MARKET_PRICE_MIN/MAX` 头文件整数值 | 只有符号；代码夹 25–500 |
| 港口状态 0–12 的枚举名 | 只有文案顺序与目标表佐证 `[推断]` |
| ~~`CTown+0x75`~~ | 已钉：状态持续月数（§2.3）；`fn_14000bcf8` 写 1、`fn_140002d1a` 加 1、`fn_14000c509` 清 0 |
| 港口状态由谁、按什么概率切换 | 月初三段已钉（§2.3 ①②③）；`fn_140007fbd(8)` 的 8、`fn_14000938b` / `fn_140009ae8` 两处复位点的触发条件未拆 |
| 流行度 `+0x71` 怎么涨落 | 本篇只读；`fn_14000dc6a` 一类写入在 factions |
| `fn_14000c658` 的玩法用途 | 只确认是文化圈价第二写入点 |
| ~~`fn_140010f46()==2` 是否就是剧本 2~~ | 已钉 `eScenario`（§2.5） |
| ~~`CTown+0x75` 谁清零~~ | 已钉：`fn_14000c509`（`0x140265270`）；月初 `0x1402670c6` / `0x140267184` / `0x14025dfe6`，投资 `07` §4.4 |
| 港口槽价运行时是否会变 | setter `fn_14000d7a1` 无直接调用者；未查间接调用 |
| ~~文化圈静态行 +8（0–6）~~ | 已钉：`major_sea_id`（→`CCulture+0x10`，`fn_140014e48` 读），经济里用于流行影响力（`03`）、投资影响力（`07`）、NPC 选港范围（`06`）；归 factions 注册 |
