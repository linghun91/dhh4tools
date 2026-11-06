# 投资、发展度与上缴

特产品文档把「投资金额 → 发展度 / 份额」和「上缴金额」标成 `[未知]`。本篇从垫板、宫殿、月结三条链把式子钉死。份额加减的**效果**仍属 factions；本系统算钱、算发展度、发事件。

`CTown.cpp` / `CForce.cpp` 是存档拷贝，没有公式。公式在未挂名的 `fn_140009ae8`、`fn_140004b8d`、`CTownElement::vfunc_04`、`CForceElement::vfunc_04`、`CItemElement::vfunc_04`。

## 1. 职责边界

本系统管：

- 投资垫板输入的金币如何扣、如何写入港口「待转化投资」
- 待转化投资 → 商业 / 军事发展度（含 9999 封顶、独占半价）
- 投资成功时拟增加的份额点数（交给 factions 封顶）
- 独占港月结上缴、珍品出租费
- 附庸 1% 上缴的**金额式**（`tribute.yaml#suzerain`）；执行步 `factions.suzerain_tribute`、扣款与 `SuzerainTributePaid` 归 factions（`forces` phase，`00-overview/06`、`02-factions/03`）
- 发展度涨完后按份额给势力影响力（不是金币）

不管：

- 垫板矩形、按钮皮肤（`CInvestmentPadView` 是 View）
- 份额槽怎么占、合约拒签文案的 UI（factions / town-life 发命令）
- 舰队代理账本买货亏损（`CFleetElement` 账本；见 `exe_caps.md`）
- 市价、特产品供给数量（`02` / `04`）
- 港口坐标、文化圈 `culture`（world；运行时 `culture_id`）
- 发展度的存储、饱和与 `TownDeveloped` 事件（world `Towns`，`00-overview/06`）；本系统只算增量并发命令 `Towns.apply_develop`

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

世界 tick 分发 fn_1402aad80（全局 SCalendar 0x1442a7b10）
  day   = fn_14000906b  nTime(+0x14)==0          每日第一拍
  mEnd  = fn_14000f8b7  +0x1c==2                 月末最后一拍
  mBeg  = fn_14000e9da  +0x1c==0                 月初第一拍
  CItemElement::vfunc_04          0x140129170   仅 mEnd
        出租费 Σ price/50，消息 0x5B
  CTownElement::vfunc_04          0x1402668f0   day 或 mEnd；反编译失败，objdump 可读
        体内 mBeg：每座活港 → fn_140009ae8  0x14025f3c0   待转化 → 发展度
        体内 mEnd：aulExclusiveProfit[25]  独占上缴 → AddGold，消息 0x5A
  CForceElement::vfunc_04         0x14010dc20   mBeg（或 mBeg 且 month%6==0，后者是前者子集）
        体内再判 mBeg；eScenario==6 且世界打包时间>0 且玩家附庸位 != 0x1A
              tribute = gold / 100，消息 0x167
  CFleetElement::vfunc_04         0x1400ff2c0   每拍无条件
        账本付不起转入下月；不是上缴公式
```

`[确认]` 上表门控全部来自 `0x1402aae14`–`0x1402aaf64` 的直接 `call` 与各谓词 body（`0x14010c000` / `0x1400f89d0` / `0x1400f87a0`）。`CTownElement::vfunc_04` 的 RTC 名 `aulExclusiveProfit`（100 字节 = 25×uint32）就是独占上缴的按势力累加桶。

`[确认]` `DAT_144296500+0x7c` 是 **`eScenario`**（0–6）：`fn_140004381`（玩家势力）= `fn_140009df9(DAT+0x7c)`，后者用 7 元素局部表 `{0,2,1,3,20,13,21}` 下标取势力；`NUM_SCENARIO=7`；`fn_14000864d` 以 `>=4` 判 World1。下文凡「`DAT+0x7c`」都按剧本号读。

### 2.2 港口上的数

`CTown` 存档 `vfunc_07` / `vfunc_06` `[确认]` `CTown.cpp`：

| 偏移 | 宽 | 循环 | 含义 |
| --- | --- | --- | --- |
| `+0x18` / `+0x19` | u8+u8 | 3 | 合约槽：势力 id、份额。空槽 `force=0x1A`、`share=0` |
| `+0x1e` | u16 | 2 | 商业 / 军事发展度。`eTownDevelop` 与 2 比 |
| `+0x24` | u32 | 2 | **待转化投资金币**，与发展度一一对应 |
| `+0x74` | 字节 | 1 | 港口状态（`02` §2.3 统一 key：0 `war` … 3 `boom`、4 `slump`、12 `normal`）。本篇读 3 / 4 / 12；写入口 `fn_14000bcf8` |

关系文档把 `+0x24` 两个 dword 标成未核实。本篇钉成待转化投资：`fn_1400117ca` 原样读 `*(town + track*4 + 0x24)`；转化后写 0。

发展度上限 **9999**（`0x270F`）在转化函数里 `min(9999, dev)`，与 `exe_caps.md` 港口商业/军事补丁点同一立即数。USHORT 硬顶 65535。

### 2.3 势力上的数

`CForce` 存档 `[确认]` `CForce.cpp`；金币读写是独立小函数：

| 偏移 | 宽 | 含义 |
| --- | --- | --- |
| `+0x0C` | u32 | 金币。`fn_140002725` 读；`fn_140002cb6` 加（封 **99 999 999** = `0x5F5E0FF`）；`fn_14001117b` 是取负再加 |
| `+0x10` | u16 × 7 | 影响力，按七大海域 `major_sea_id`。`fn_14000ae7a(force, sea, d)` 加完封 `[0, 9999]` |
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
| 合约槽 | `(town_id, slot 0–2)` | 3 | `force_id = null`（原版 26 = `0x1A`，只在 `legacy_codec`） |
| 独占港 | 派生：槽 0 份额 ≥ `share_cap` | — | 非独占 |
| 附庸 | `CForce+0x1E` | 每势力 1 | `null`（原版 `0x1A`） |
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

宫殿**军事加投资** `fn_14001077b`：先 `min_invest(town, military)`。若 `eScenario == 0`（`DAT_144296500+0x7c`，拉斐尔剧本）：

```
min_fee = max_u(1, floor(min_fee * 8 / 10))   # 八折；shl 3 后无符号 div 10
```

`[确认]` 立即数与比较对象；「只有剧本 0 打八折」是剧本内容，不是公式常量。

垫板上限 `[确认]`：交易所 = `min(min_invest*20, 玩家金币)`；宫殿新合约与军事加投资 = **玩家全部金币**（`fn_14001019a(min, gold, 0x191)`，没有 ×20）。玩家金币 `< min`（无符号）直接拒绝：交易所消息 `0x37a`，新合约 `0x546`+`0x553`，军事 `0x552`。

### 4.2 写入待转化

`fn_140004b8d(town, track, investor_force, gold)`：

```
if block_if_here(town) and investor_force != player_force: return
pending[track] = min_u(pending_gold_cap, pending[track] + gold)
# pending_gold_cap = 9_999_999 (0x98967F)
```

`block_if_here` = `fn_140009b42` → `fn_14000d6c0(town_id, eScenario)` `[确认]`：判断该港是不是**本剧本的据点港**。不是「玩家正在这座港」。

| eScenario | 据点港（`town_id`） |
| --- | --- |
| 0 | 86 朋沙科拉 |
| 1 | 87 阿布哈兹 |
| 2 | 88 雷利史塔特 |
| 3 | 89 淡水 |
| 4 / 5 | 无 |
| 6 | 89、90、92、94、95、96 |

据点港只有玩家势力能往待转化里加钱。`fn_1400072cf`（NPC 判份额余量）对据点港同样直接返回 0。表应归 narrative 剧本包（`scenario.base_towns`），经济只查询。

三条入口怎么分轨：

| 入口 | 扣金币 | 写入 |
| --- | --- | --- |
| 交易所 `fn_14000b2fd` | 全额 | 只 **商业** `track=0` |
| 宫殿新合约 | 全额 | **对半**：`half = gold/2`（无符号）各写一轨。奇数余 1 丢弃 |
| 宫殿军事加投资 | 全额 | 只 **军事** `track=1` |

### 4.3 份额优待

已有份额（或新合约）时：

```
share_gain = floor(gold_paid / min_fee)     # 无符号 div；原版 fn_1400122f6 → fn_14000957a
# 重构（词表 §14.21，只走一条路）：
#   reason: contract        → Invest.pay 只返回 share_gain；town-life 交给 factions Contract.sign，由它内部调 Share.add(reason: contract)
#   reason: trade / military → economy 调命令 Share.add(town_id, force_id, share_gain, reason: investment)（已有份额才走）
```

交易所与军事加投资：仅当玩家**已经**有份额（`fn_1400092af > 0`）才走这条 `[确认]`。新合约无条件走。这就是文案「稍微优待」——优待量是「付了几个最低额」，不是固定 +5。`gold >= min_fee` 已先保证，所以 `share_gain >= 1`。

调用形如 `fn_1400122f6(town, player, share_gain, 0, 1)`。`fn_1400122f6`：`amount==0` 直接返回；`>0` 走 `fn_14000957a`，`<=0` 走 `fn_140002356(town, force, -amount)`；之后对份额变动的其它势力逐个 `fn_140008279`（关系侧，属 factions）。

`fn_14000957a` 本体 1844 字节（RTC `nShareAmount` / `anProtectShare`），封顶仍是既有的 `100 − 已占`。内部保护槽细节不在本篇展开。

`fn_14000f759(town)` = `CTown+0x10` WORD 的 **bit 2** `[确认]`（`fn_140262570(town+0x10, 2)`，掩码 `1<<n`）。与行情可点的 bit 1 同一字（见 `08`）。用途：

- 新合约：只选台词，真 `0x554` / 假 `0x550`
- 军事加投资：真且 `share_gain >= rand(100)`（无符号）时 `fn_14000e90d(town, 0)` + 消息 `0x55e`

bit 2 的玩法名与 `fn_14000e90d` 效果 `[未知]`，不要写成介绍信确认式。

新合约成功后若玩家势力 `CForce+0xA == 0x62`（无本据港），`fn_1400092ff` 把本港写进 `CForce+0xA` `[确认]`；`fn_1400122f6` 内对任何势力也有同一判断。`+0xA` 的名字「本据港」`[推断]`，写入归 factions。

### 4.4 待转化 → 发展度（F-ECO-11）

`fn_140009ae8`，由 `CTownElement::vfunc_04` 在**月初第一拍**（`fn_14000e9da`，`SCalendar+0x1c==0`）对 97 座活港各调一次 `[确认]` `0x140267774`–`0x1402679cf`。本函数内没有日历立即数。没投资的港也照跑：特产品解锁、据点里程碑每月都会检查。

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
    develop[track] = min_s($caps.town_develop, dev)         # 9999
    pending[track] = 0                                      # 余金清掉
```

重构：economy 只算出两轨增量，发命令 `Towns.apply_develop(town_id, track, delta, investment)`；写入、饱和到 `caps.town_develop`、发 `TownDeveloped` 都由 world `Towns` 做（`00-overview/06`）。

含义：

- 每 1000 发展度一档，档内一点的价钱是 `(档+1)*20`（独占 `*10`）。0→999 一点 20；1000→1999 一点 40。
- `max(1, can_buy)`：桶里只要还剩正数，就再买 1 点（透支该桶，然后清零）。不是「余金结转下月」。
- 先买到 1000 边界再换档价，避免一笔钱按旧单价连跨两档。

商业发展度相对转化**前**的旧值：

```
if new_commerce >= old_commerce * 3 / 2:          # 有符号 *3/2
    if town_state == slump:                       # 4
        fn_14000c509(town)                        # +0x74 = 12 normal，+0x75 = 0
        emit 景气恢复          # 消息 0x80
    else if town_state == normal                  # 0xC
         and new_commerce >= old_commerce * 2:
        emit 越来越繁荣        # 消息 0x81
        fn_14000bcf8(town, 3, 1)                  # 切到 boom，+0x75=1（02 §2.3）
```

`town_state` 来自 `town+0x74`（`fn_14001244f`）。key 名按 `02` §2.3（枚举名未恢复，中文名 `[推断]`）。`fn_14000bcf8(town, state, flag)` `[确认]`（body `0x140265d40`）：写 `+0x74=state`、`+0x75=flag`，`state != 12` 时再调 `fn_14000db4d`。「景气恢复」分支先调 `fn_14000c509`（body `0x140265270`）直接把状态写成 12 `normal`、`+0x75` 清 0 `[确认]`（旧稿漏了这一步），再发 `0x80`。重构：景气状态归 economy（`00-overview/06`），两条分支直接改本系统的 `town_state` 并发 `TownStateChanged { town_id, old, new, flag }`。

边界 `[确认]`：比较用转化**前**旧值，无投资时 `new==old`；只有 `old==0` 才满足 `0*3/2 <= 0`，此时 kind 4 每月都会发 `0x80`。实现时加 `old > 0` 守卫要显式开关，原版不守。

特产品解锁（5 槽，`i=0..4`）`[确认]`：

```
rec = GetFixTown(town).special[i]          # fn_140014b55 = 静态表 +0x28+i*6，不是运行时 CTown
item = BYTE(rec+0)
if item == 106: skip
if discovered(item): skip                  # fn_14000f9e3：CTradingItem+0x16 bit0
if WORD(rec+2) > commerce(new): skip       # listing_threshold，300–4500
if bit0(CTown+0x2c+i*6): skip              # 运行时槽字节
set_discovered(item)                       # fn_14000513c：CTradingItem+0x16 bit0 = 1
```

`WORD(rec+2)` 是商业门槛（伦敦 4500/2000/2000/4000/3500）；`rec+4` 的 5–25 是旧 JSON `grade`，现名 `supply_base`（每月基准量，`04` §2.1）。解锁写的是**全局**交易品发现位，不是这座港的槽。

转化若使发展度增加 `total_gain`（两轨 `封顶后新值 − 旧值` 之和）：

```
for each occupied share slot:
    d = floor(total_gain * slot.share / influence_div)   # 2000
    Influence.add(slot.force, MajorSea.of_town(town), d, investment)  # 再封 9999
```

`MajorSea.of_town` = 原版 `fn_140014e48(town)`：town→culture→`CCulture+0x10` 的七大海域下标 `[确认]`（`02-factions` 统一注册 `major_sea_id`）。这是影响力，不是金币。`d` 是有符号 `idiv 2000`；`d==0` 时 `fn_14000ae7a` 直接返回，否则 `clamp(0, cur+d, 9999)`。

据点里程碑 `[确认]`：`fn_140009b42(town)` 为真（本剧本据点港）时调 `fn_140008fbc(town)`：

```
row = World1 ? 2 : eScenario            # fn_14000864d；表 0x1405a0490，步长 0x48
for tier in 0..5:                        # 6 × {commerce, military, kind}（12 字节 3 × i32）
    if commerce < row[tier].c or military < row[tier].m: break
    fn_14000e142(town, row[tier].kind, 1) # 写 CTown+0x14 的地图点 kind 位
```

| row | tier0 | tier1 | tier2 | tier3 | tier4 | tier5 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 1500/0→7 | 2500/500→0 | 4000/1000→8 | 4500/2000→4 | 5000/5000→2 | 9999/9999→5 |
| 1 | 0/500→1 | 1000/1000→0 | 2000/2000→8 | 2500/3000→4 | 4000/6000→2 | 9999/9999→5 |
| 2 | 0/500→1 | 1000/1000→0 | 2000/1500→4 | 3500/2000→8 | 6000/4000→2 | 9999/9999→5 |
| 3 | 1000/0→7 | 2000/2000→0 | 3000/3000→8 | 4000/4000→4 | 6000/6000→2 | 9999/9999→5 |

第三列是地图点 kind `[确认]`（`fn_14000e142` → `CTown+0x14` 位域，`01-world/10` §2.5）：7 总督府、0 广场、8 旅馆、4 酒馆、2 造船厂、5 行会、1 交易所。这张表属内容包（`town/town_map_points.yaml` 的 `unlock_tables`，键 `vanilla:map_point_unlock_0..3`，剧本以 `map_point_unlock_table` 选行），不进本公式包。注意剧本 4/5/6 都落到 row 2（`World1 ? 2`），剧本 4/5 又没有据点港，实际只有剧本 6 用 row 2。

### 4.5 独占上缴（F-ECO-12 的城市侧）

`CTownElement::vfunc_04` 末段，仅当 `fn_14000f8b7`：`SCalendar+0x1c == 2` = **月末最后一拍**（`nTime==47` 且月内最后一天；见 `01-world/07-calendar-and-coords.md` §2.3）。不是「旬」。

```
profit[force] = 0  for 25 势力          # aulExclusiveProfit
for town in alive_towns:
    owner = exclusive_owner(town)       # §2.6
    if owner == empty: continue
    p = floor(develop(town, commerce) / exclusive_div)   # 10，有符号 idiv
    (n, d) = rules.contexts.exclusive_tribute[Economy.town_state(town)]   # 02 §2.9 表
    p = floor(p * n / d)        # 原版 boom(3)：*3/2 无符号；slump(4)：/2
    profit[owner] += p
for force in 0..24:
    if profit[force] == 0: continue
    if force == player: emit ExclusiveTributePaid { force_id: force, gold: profit[force], town_ids }  # 0x5A
    Force.add_gold(force, profit[force], exclusive_tribute)
```

`slump`（4）在繁荣文案里是「景气恢复」的对象，在上缴里是 **×½**；`boom`（3）上缴 ×3/2、造船价 ×12/10。同一状态字节被多系统读，倍率统一放 `02` 的 `economy/town_state_price_rules.yaml`（`contexts` 段）。

### 4.6 附庸上缴（F-ECO-12 的势力侧）

执行方是 factions（`forces` phase 步 `factions.suzerain_tribute`，`02-factions/03` §2）；本篇只给金额式（词表 §14.20）。原版 `CForceElement::vfunc_04` 的门控与金额：

```
# 门控（原版；重构由 factions 步骤判断，见 02-factions/03 §2）
#   month_start(cal)                 fn_14000e9da，+0x1c==0
#   eScenario == 6                   DAT_144296500+0x7c；重构 = 剧本开关 pay_suzerain_tribute
#   world_packed_time > 0            fn_1400104b5(fn_140012c79()) 读 +0x10，无符号
#   player_force.overlord != empty   +0x1E
Tribute.suzerain_amount(force_id) = floor(gold(force_id) / suzerain_div)   # 100，无符号；0 则 factions 不执行
# 之后的扣款 / 入账 / SuzerainTributePaid（消息 0x167「请缴纳」）全部归 factions，本篇不写
```

这是**当前金币的 1%**，不读发展度、不读份额。与独占港上缴是两条。`[确认]` 月初第一拍、剧本 6、开局第 0 拍不收。「只在剧本 6 收」是剧本内容（蒂雅「我一定要独立」），重构写成剧本开关，不是公式常量。原版扣款 = `fn_140002cb6(-x)`：不足时夹到 0（无符号比较），供 factions 实现参考。重构：步骤 `factions.suzerain_tribute`（`forces` phase）由 factions 执行，调 `Tribute.suzerain_amount`；公式参数仍在本篇 `tribute.yaml#suzerain`。

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
    Force.add_gold(player_force, total, treasure_rent)
```

`[确认]` 同一套状态：陈列走 `fn_140010393(item, SItemOwner{kind=1}, 6, 0)`；跳表 case 1 调 `fn_14000e809(item, 1)`，置 `item+0x3C` 的 **bit 4**。`6` 是第三参（位置号），不是位。`fn_140010e5b`（回收）把 bit 2 / bit 4 都清掉。所以 `rent_bit` = 「物主类型 1（借出陈列）」。

`[确认]` 本函数无日历门，但世界 tick 只在**月末最后一拍**调它（`0x1402aae1b` `fn_14000f8b7`）。循环上界 `0xE7`（231）写死。

### 4.8 立即数总表

| 参数 | 原版 | 出现 |
| --- | --- | --- |
| `town_develop`（caps） | 9999 | `fn_140009ae8` `min_s`；影响力加值同样 9999 |
| `develop_bracket` | 1000 | `idiv 0x3E8` |
| `normal_cost_unit` | 20 | 非独占 |
| `exclusive_cost_unit` | 10 | 槽 0 份额 100 |
| `invest_min_floor` | 1 | `max_s(1, dev/2)` |
| `invest_max_factor` | 20 | 交易所最高 = 20×最低 |
| `contract_fee_div` | 4 | `(商+军)/4` |
| `pending_gold_cap` | 9 999 999 | 待转化 |
| `gold_max`（caps） | 99 999 999 | 势力金币 |
| `share_cap` | 100 | 独占、优待、封顶 |
| `share_from_gold_div` | min_fee | `floor(gold/min_fee)` |
| `influence_div` | 2000 | `gain*share/2000` |
| `exclusive_div` | 10 | 商业发展度 / 10 |
| 上缴状态倍率 | `boom` ×3/2、`slump` ×1/2 | `02` `town_state_price_rules.contexts.exclusive_tribute` |
| 繁荣文案状态 | `slump`（恢复）/ `normal`（繁荣，切 `boom`） | `0x80` / `0x81` |
| `suzerain_div` | 100 | 附庸税 |
| 附庸税剧本 | `eScenario==6` | `DAT+0x7c`；剧本开关，不是公式键 |
| 军事八折剧本 | `eScenario==0` | 同上 |
| `military_discount` | 8/10，下限 1 | `shl 3; div 10; max_u(1,·)` |
| 发展转化时点 | 月初第一拍 | `SCalendar+0x1c==0` |
| 独占上缴时点 | 月末最后一拍 | `SCalendar+0x1c==2` |
| 附庸税时点 | 月初第一拍 | 同转化 |
| 出租费时点 | 月末最后一拍 | 世界 tick 门 |
| `treasure_rent_div` | 50 | 报价与月结同一除数 |
| `treasure_price_hi` | 100000 | 陈列加句 |
| `pad_arg_2` | 401 | 垫板第三参，非金币 |

未出现在反汇编里的键保持 `null`。

## 5. 耦合分析

原版：

- 一次「投资」改港口 `+0x24`、势力金币、份额槽、稍后才改 `+0x1e`
- 转化函数自己读份额槽 0 决定单价，自己写影响力，自己解锁特产品
- 独占上缴、附庸税、出租费分三个 Element 的 `vfunc_04`，文案却写在同一句里（重构：附庸税执行归 factions）
- `CInvestmentPadView` 与公式零耦合

重构：

```
Invest.pay(town_id, force_id, gold, reason)     # 扣金、记 pending
time.MonthStarted            →  economy.investment_convert（逐港，与 04 补货交错）、factions.suzerain_tribute
                                （先后写在 tick.yaml，见 04 §6；DECISIONS D4）
time.MonthEnding             →  economy.treasure_rent（items phase，先）→ economy.exclusive_tribute（towns phase，economy 执行；factions 只读）
                                （原版 CItemElement 先于 CTownElement，§2.1；只在金币触顶时影响结果）
合约份额：town-life 把 Invest.pay 返回的 share_gain 交给 factions Contract.sign（内部 Share.add(reason: contract)）；交易所 / 军事加投资优待：economy 调 Share.add(reason: investment)
InvestmentMade 不带份额增量，订阅方不得据它再加份额
economy 发 Towns.apply_develop(town_id, track, delta, investment)；world Towns 写两轨并唯一发 TownDeveloped
items 听 TreasureDisplayed；月结出租费由 items 报金额、economy 入账
```

World0 / World1 只换港口内容，不换这些除数。

## 6. 解耦后的配置

```yaml
# config/formulas/investment.yaml
schema: dk4.formulas.investment
version: 1
# F-ECO-11。所属：本篇。

invest_pending:                # §4.2 写入待转化，fn_140004b8d
  impl: vanilla.invest_pending
  params:
    pending_gold_cap: 9999999  # min_u(pending_gold_cap, pending + gold)

min_invest:                    # §4.1 最低 / 最高投资额，fn_14000ba41 / fn_14000bc80 / fn_140013d1d
  impl: vanilla.min_invest
  int_mode: vanilla_i32        # dev/2、(商+军)/4 均为向 0 的有符号除
  params:
    invest_min_floor: 1
    invest_max_factor: 20      # 交易所最高 = 最低 × 此值
    contract_fee_div: 4        # (commerce + military) / 4

military_min_fee:              # §4.1 军事加投资最低额，fn_14001077b
  impl: vanilla.military_min_fee
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }               # 原 int_mode: { div: udiv }；原版 shl 3 后无符号 div 10
  modifiers: invest.military_min_fee_pct       # 原 modifier_key；缺省 100；原版剧本 0 剧本包贡献 80（DECISIONS D5）
  params:
    floor: 1                                    # max_u(1, ·)
  # 本文件不出现剧本号；哪个剧本打折写在剧本包

pad_max:
  impl: vanilla.pad_max
  params:
    trade: min_times_factor_capped_by_gold   # min*invest_max_factor 与金币取小
    contract: payer_gold
    military: payer_gold

investment_convert:            # §4.4 待转化 → 发展度（F-ECO-11），fn_140009ae8
  impl: vanilla.investment_convert
  schedule: month_start        # 原 schedule.investment_convert；tick.yaml towns phase，per: town；原版 SCalendar+0x1c==0
  int_mode: vanilla_i32        # dev / develop_bracket 有符号；min_s / max_s
  quirks: { unsigned_div: true }   # can_buy = gold / cost 无符号
  params:
    develop_bracket: 1000
    normal_cost_unit: 20
    exclusive_cost_unit: 10
    exclusive_share: 100       # 槽 0
    # 发展度上限用 $caps.town_develop（9999），由 world Towns 饱和；本文件不重复
    remainder_buys_one: true   # max(1, gold/cost)

flourish:                      # 状态 key 由 economy 拥有（02 §2.3）
  impl: vanilla.flourish
  int_mode: vanilla_i32        # 有符号 *3/2
  params:
    recover_factor_num: 3
    recover_factor_den: 2      # new >= old * 3/2
    recover_from: slump        # 原版 4 → 切 normal（fn_14000c509），消息 0x80
    recover_to: normal
    flourish_from: normal      # 原版 12
    flourish_factor: 2         # new >= old * 2
    flourish_to: boom          # fn_14000bcf8(town, 3, 1)，消息 0x81
    flourish_flag: true        # +0x75 = 1：状态持续月数从 1 起计（02 §2.3）
    guard_old_zero: false      # 原版不守 old==0（§4.4 边界）

share_from_gold:               # §4.3；原 formula: floor(gold_paid / min_fee)
  impl: expr
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }   # 无符号 div
  params:
    only_if_already_has_share_on_trade: true
  inputs: [gold_paid, min_fee]
  expr: "gold_paid udiv min_fee"

influence_from_gain:           # §4.4 末；原 formula: floor(total_gain * slot_share / influence_div)
  impl: expr
  int_mode: vanilla_i32        # 有符号 idiv 2000
  params:
    influence_div: 2000
    cap: 9999
  inputs: [total_gain, slot_share]
  expr: "total_gain * slot_share / influence_div"

pad:
  impl: vanilla.pad
  params:
    arg2: 401                  # 非金币上限
```

```yaml
# config/formulas/tribute.yaml
schema: dk4.formulas.tribute
version: 1
# F-ECO-12。所属：本篇。

exclusive:                     # §4.5 独占上缴；tick 步 economy.exclusive_tribute
  impl: vanilla.exclusive_tribute
  schedule: month_end          # 订阅 MonthEnding；原版 SCalendar+0x1c==2 月末最后一拍
  int_mode: vanilla_i32        # commerce / 10 有符号 idiv；状态倍率 *3/2 无符号
  params:
    base: floor(commerce / exclusive_div)
    exclusive_div: 10
    owner: share_slot_0_if_share_ge
    exclusive_share: 100       # 与 factions / combat 同键（02-factions/03）
    state_mod_context: exclusive_tribute   # 倍率表在 02 economy/town_state_price_rules.yaml（boom 3/2、slump 1/2）

suzerain:                      # 只给金额式；执行步 factions.suzerain_tribute（forces phase，month_start）归 factions
  impl: expr                   # 原 formula: floor(gold / suzerain_div)
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }   # §4.6 无符号
  enabled_by: scenario_flag.pay_suzerain_tribute   # 原版只有 eScenario 6 为真
  params:
    skip_at_time_zero: true    # 世界打包时间 == 0 不收
    # 无宗主 = null（原版 0x1A）
    suzerain_div: 100
    payer: player_force        # 原版这条只处理玩家势力；overlord 读 factions
  inputs: [gold]
  expr: "gold udiv suzerain_div"

treasure_rent:                 # §4.7；tick 步 economy.treasure_rent（items phase）
  impl: vanilla.treasure_rent
  schedule: month_end
  params:
    quote: floor(item_price / treasure_rent_div)
    treasure_rent_div: 50
    price_hi: 100000
    collect_when: owner_kind == lent_display   # 原版 item+0x3C bit 4，陈列 SItemOwner kind 1 置位
```

```yaml
# config/economy/investment.yaml   （内容/开关，不是除数）
schema: dk4.economy.investment
version: 1
brand_grade_rank: 1            # 仍见 04；本篇只提供发展度
tracks: [commerce, military]
share_slots_per_town: 3
```

```yaml
# narrative/scenarios.yaml 片段（narrative 拥有，字段名以 10-narrative/01 v2 为准；本篇只读这些开关）
- key: vanilla:raphael            # 原版 eScenario 0
  modifiers:
    - { key: invest.military_min_fee_pct, stage: mul, value: { num: 8, den: 10 } }
  base_towns: [vanilla:pensacola]  # 原版下标 86；town key 以 01-world 为准
  map_point_unlock_table: vanilla:map_point_unlock_0   # §14.4 全命名空间；据点里程碑表 0x1405a0490 行 0（01-world/10 unlock_tables）
- key: vanilla:tia                # 原版 eScenario 6（World1）
  flags: { pay_suzerain_tribute: true }
  base_towns: [vanilla:tamsui, vanilla:saint_malo, vanilla:benghazi, vanilla:busan, vanilla:edo, vanilla:caracas]   # 原版下标 89 90 92 94 95 96
  map_point_unlock_table: vanilla:map_point_unlock_2   # World1 ? 2
# 剧本 2 的卖价 / 流行影响力修正器见 02 §6；里程碑表每行 6 档 {need: [commerce, military], kind}（第三列 = 地图点 kind，exe 复核 [确认]），归 01-world/10
```

`caps.yaml` 已有 `town_develop: 9999`。势力金币硬顶是 `caps.gold_max`（99,999,999，本轮补登 caps.yaml），不要把 9999999 的待转化帽当成势力金库帽。

## 7. 独立旋钮

改这些值不应迫使贸易 / 战斗改代码：

- `develop_bracket`、`normal_cost_unit`、`exclusive_cost_unit`
- `invest_min_floor`、`invest_max_factor`、`contract_fee_div`
- `caps.town_develop`（须 ≤ USHORT 硬顶）
- `remainder_buys_one`（关掉则余金结转，须同时改 pending 清零）
- `exclusive_div`、`suzerain_div`、`treasure_rent_div`
- `influence_div`
- 各 `schedule`（月初 / 月末；改成季结只动这里）
- 剧本包修正器 `invest.military_min_fee_pct`、剧本开关 `pay_suzerain_tribute`、`base_towns`（narrative 包）
- 每港合约槽数（与 factions 同一键）

改「伦敦初始发展度」只动内容包。改「槽 0 才叫独占」只动 `owner` 策略。

## 8. 对外接口

```
Invest.min_fee(town_id, reason) -> gold
    reason: trade | contract | military
Invest.max_fee(town_id, reason, payer_gold) -> gold
Invest.pay(town_id, force_id, gold, reason) -> { pending, share_gain }   # share_gain 只是算好的数；contract 由 Contract.sign 落账，trade/military 由本系统调 Share.add(reason: investment)
Towns.develop(town_id, track) -> int                          # world 拥有，本篇只读
economy.investment_convert(town_id) -> { old, new, total_gain }   # tick 步骤（月初，per: town）；结果经 Towns.apply_develop 写入
Tribute.exclusive(town_id) -> { force_id | null, gold }
Tribute.exclusive_month(force_id) -> gold                      # 扫全部独占港
Tribute.suzerain_amount(force_id) -> gold                      # 只算金额；factions.suzerain_tribute 调用并执行扣款、发事件
TreasureRent.quote(item_id) -> gold
TreasureRent.month(force_id) -> gold
```

事件：

```
InvestmentMade { town_id, force_id, gold, tracks[] }   # 不带 share_gain（词表 §14.21）
ExclusiveTributePaid { force_id, gold, town_ids[] }
TownStateChanged { town_id, old, new, flag }        # §4.4 景气恢复 / 繁荣
ToolRentIncome { force_id, gold, item_ids[] }
SpecialUnlocked { town_id, slot, item_id }
```

查询 factions：`Share.of(town_id, force_id)`、`Share.slots(town_id)`。经济不写份额槽；份额只经 factions 命令 `Share.add` / `Contract.sign` 变动。

不由本篇发：`TownDeveloped`（world `Towns` 唯一发出）、`SuzerainTributePaid`（factions 发）。

`03.Trade` 无份额时只引导到投资入口（`Invest.pay(town_id, force_id, gold, reason)`），不自己改发展度。

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| ~~`DAT_144296500+0x7c`~~ | 已钉：`eScenario`（§2.1） | — |
| ~~`SCalendar+0x1c` 的 0/1/2~~ | 已钉：月初第一拍 / 其它 / 月末最后一拍 | 「旬」 |
| ~~`CTownElement::vfunc_04` 的调用频率~~ | 已钉：每日第一拍或月末最后一拍调；转化在月初、独占在月末 | — |
| `town+0x74` 的 3 / 4 / 12 | 统一 key `boom` / `slump` / `normal`（`02` §2.3，中文名 `[推断]`）；写入口 `fn_14000bcf8` / `fn_14000c509` 已钉 | 原版枚举名 |
| ~~`fn_14000bcf8(town, 3, 1)`~~ | 已钉：切 `boom`、`+0x75=1`；`state!=12` 时调 `fn_14000db4d`（效果未读） | — |
| ~~`fn_140009b42`~~ | 已钉：本剧本据点港（§4.2 表） | 「玩家在港」 |
| ~~`fn_140008fbc` 的 `arg` → `CTown+0x14`~~ | 已钉：第三列 = 地图点 kind（`01-world/10` §2.5） | — |
| `fn_14000957a` 保护槽 | 1844 字节、`anProtectShare` | 优待公式本身（优待量已钉） |
| `CTown+0x10` bit 2 / `fn_14000e90d` | 谓词已钉成位测；位的玩法名与效果未知 | 介绍信 |
| `fn_1400104b5(0x1442a7b40)` | 读世界打包时间 `+0x10`，`>0` | 「许可」 |
| ~~出租 `bit 4` vs 陈列写入 `6`~~ | 已钉：同一状态，`6` 是位置参数 | — |
| `CItem+0x2C` | 出租用的「价」 | 与目录 DATAPRICE 同一列 |
| 消息 `0x5A`/`0x5B`/`0x167` 与 mesfile 偏移 | 不是同一空间 | 「0x9e7 就是 0x5A」 |
| 账本 6 行 | 仍是存档循环 6 | 本篇改判定 |
| `pad_arg_2=401` | 垫板第三参 | 金币上限 |
| ~~世界表 JSON `grade` 与槽 `+0x2`~~ | 已钉：`+0x2` 是商业门槛 `listing_threshold`；旧 `grade`（`+0x4`）= `supply_base` | 「15 级商品」 |
| `abSpCharge`（RTC 76 字节） | 同在 `CTownElement::vfunc_04` | 上缴公式的一项 |

`CTownElement::vfunc_04` 前半还有流行、连接港（RTC `abSpCharge` / `aeConnectTown` / `eTownFashion`），保持 `[未知]`。月初补货已钉：同一循环里转化之后紧接 `fn_140010d7a(town, kind_mask)`，规则见 `04` §2.5（文案对照 `05` §2.8）。都不是投资/上缴，不要塞进本公式包。
