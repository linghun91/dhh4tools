# 付费查阅市场行情

玩家在**本港交易所**花钱看**另一座港**此刻的市价板。每次点选一座港扣一次金币，看完可以再点下一座，取消才退出。本篇是这条会话的规格。报价公式仍是 `02.quote()`；本港买卖会话见 `03-trade-loop.md`。

## 1. 职责边界

本系统管：

- 交易所菜单里「查阅行情」这条命令
- 每看一座目标港收多少金币、从哪只钱包扣
- 哪些港可以点（可见性谓词）
- 选港航路图只作为**投影**：不存市价表
- 打开目标港的只读市价板（原版 `CPriceInformationView`）

不管：

- 市价指数怎么滑、金币报价怎么乘（`02`）
- 本港买卖、货舱、堆叠（`03`）
- 特产品供给、投资垫板（`04`）
- 份额槽谁拥有（`02-factions`；本系统只读 `player_share(target) > 0`）
- 港口「已知」位谁置位（world / 入港；本系统只查询）
- 人物六维与会计术原值（只读 `ModifierSet`；原版扣费函数**不读**技能）
- 航线是否已建立（`01-world/05-routes-navigation.md`）
- 广场听流行、酒馆打听（`08-town-life`）
- 文案、贴图、按钮像素（`12-ui-save`）

交易所入口由城镇设施发出 `OpenExchange`。行情是交易所菜单里的一条命令，不是独立设施。

## 2. 原版事实

### 2.1 入口在交易所菜单第 3 项（下标 2）

`fn_14000e0e3`（`srcdata/source/game/unassigned/fn_1402755b0.cpp`）是交易所主循环。`fn_14027a640` 弹出 4 项 `sMenuOption`。返回值：

| 下标 | 动作 | 标记 |
| --- | --- | --- |
| 0 | 本港买卖（`UnpackFromShip` / `CTradingView`） | [确认] |
| 1 | 投资（`fn_14000b2fd`） | [确认] |
| 2 | **查阅行情** `fn_1400115ea` | [确认] |
| 3 | 关闭，写 `DAT_1442a7140 = 0` | [确认] |

没有占有率时买卖会被挡（文案「没有占有率，交易也不能进行，要投资吗」`mesfile_sc.txt` `0x7943`）。行情项不走这条门。

### 2.2 文案：每看一次缴费一次

SC `mesfile_sc.txt` `0x2ef` 同一记录里拼了多句：

> 查阅市场行情每看一次缴费一次。要看哪个城市的市场行情呢？金额方面能不能再给我们想想办法。金额方面能不能帮帮忙。

后两句与 `0x35f` / `0x383` 的杀价套话挤在相邻块里。`[确认]` 前两句是本功能对白。`[推断]` 「金额方面……」属于交易所杀价（`fn_1400137b4`），不是行情折扣；依据：扣费函数 `fn_140003918` / `fn_14027a310` 不读人物、不进讨价还价分支。

开图前 `fn_1400115ea` 调 `fn_140004b3d(dialog, 0x13)`。钱不够调 `fn_140004b3d(dialog, 0x38c)`。`0x13` / `0x38c` 经 `fn_1400012da` → mesfile。`[推断]` `0x13` 对应「要看哪个城市的市场行情呢？」；`0x38c` 对应同菜单块里「钱不够，我这里不是慈善机构，没有钱是不能给你看的。」（`0x7a63`）。禁止在逻辑里写死汉字。

旁支剧情句「要调查各港口的市场行情」（`0x16ce7`）不是这条命令。

### 2.3 选港航路图不持有市价

`CMarketPriceRouteMap` → `CSelectTownRouteMap` → `CRouteMap` → `CView`。

- `[确认]` 构造加载 `RouteMkr.dk4`（`CMarketPriceRouteMap.cpp` body `0x1401f62d0`）。
- `[确认]` `CSelectTownRouteMap` 体积 `0x118`，`+0x110 = 0x62`（未选哨兵 98 = `NUM_TOWN+1`）。
- `[确认]` 选港：`fn_140012238` 读按钮号。`0x2F ≤ id < 0x90` 则 `town_id = id - 0x2F`（0–96），否则返回 `0x62`。
- `[确认]` 虚函数只做选港 / 画标记。`vfunc_46` 用 `RouteMkr` 槽画点；成功看完一座后 `fn_140010ca8` → `fn_140014763` 刷 `RouteFrm.dk4`。
- `[确认]` 悬停 `vfunc_45`：标记类型 `== 5` 时算一次 `fn_140003918`，把金币数交给绘制。地图不缓存报价。

重构后这张图是 UI，只问 `fee(from, to)` 和 `town_visible(to)`，报价仍 `Economy.quote(to, item)`。

### 2.4 金币：0 / 50 / 200，看目标港有没有我的份额

`fn_140003918(target, current)` body `0x140275250`：

```
fee = 200                    # 0xC8
if target == current:
    fee = 0
else:
    town = GetTown(target)   # fn_1400103bb
    if player_share(town) > 0:
        fee = 50             # 0x32
return fee
```

`player_share` = `fn_1400088f0(town)`：取玩家势力，在该港 3 个份额槽 `CTown+0x18+i*2` 里找匹配势力字节，命中则返回邻字节份额（`fn_1400092af` + `fn_140265ed0` 的 `movzx`）。找不到或份额为 0 → 走 200。空槽势力哨兵 `0x1A`。

`current` = `fn_14000e68d`：玩家舰队所在港；取不到返回 `0x62`。

扣费 `fn_14027a310(target, dialog)` body `0x14027a310`：

1. `GetTown(target)` 为空则失败，不扣钱。
2. `cost = fn_140003918(target, current)`。
3. 钱包 `fn_140012bfc` → `fn_140002725` 读对象 `+0xc`（与本港买卖同一只金币）。
4. 无符号比较 `gold < cost`：`jb`。不够则消息 `0x38c`，返回 0。
5. 够则 `fn_14001117b(wallet, cost)` = `AddGold(-cost)`，返回 1，再 `new` 市价板。

`[确认]` 本港（`target == current`）免费仍打开市价板。`[确认]` 无份额的已知港收 200，不是拒绝。`[确认]` 这条路径**没有**账本 `expense` 调用，只扣钱包。

只有两处调用 `fn_140003918`：扣费、地图悬停。都没有会计术下标。

### 2.5 哪些港能点：`CTown+0x10` 的 bit 1

`CMarketPriceRouteMap::vfunc_42`：`GetTown(id)` 后 `fn_14000c65d`。`fn_14000c65d` 测 `*(uint16*)(town+0x10) & (1<<1)`（`fn_140262570` + `fn_140266770`）。未置位则返回 0，点选失败。

`CTown+0x10` 是存档 WORD（`CTown::vfunc_07` 第一个字段）。`vfunc_05` 初值来自世界表 `STown+0x46`（伦敦 `rest_hex` 头两字节 `01 00` = bit 0，**不是** bit 1）。

置 bit 1 的包装是 `fn_140265820`（thunk `0x14000f2e0`，`enable` 在 `edx`）：

| 调用点 | 做什么 | 标记 |
| --- | --- | --- |
| `0x1400eaa30` | 入港：`GetTown(当前港)` 后 `SetBit1(1)` | [确认] 访问过的港可查 |
| `0x1402a8e10` | 开局：按主角/剧本分支对若干 `town_id` `SetBit1(1)` | [确认] 有 switch；哪条分支对应哪个提督 `[未知]` |
| `0x1401b5610` | 另一处置位 | [确认] 有调用；触发条件 `[未知]` |

开局函数里出现过的 `town_id` 并集（跨全部分支，不是一次全亮）：1, 2, 3, 5–12, 38–41, 48, 49, 52, 54–60, 65–68, 70, 85, 93。伦敦 0 不在这份并集里——未访问前默认不可点。

bit 0、bit 2 各有独立 setter（`fn_140265780` / `fn_1402659d0`）。行情点选只用 bit 1。`eTOWNACCESS` 符号存在，与这三位是否同一枚举 `[未知]`。

航线底图仍可能画出未置位的港。`[确认]` 的门是「点选」；未置位港是否画灰点属 UI，不进本规则。

### 2.6 会话：循环点选，看一次扣一次

`fn_1400115ea` body `0x140276ae0`：

```
show mes 0x13
map = CMarketPriceRouteMap()
loop:
    if 输入结束: break
    t = map.selected_town()          # 0x62 = 取消
    if t == 0x62: break
    if pay_and_open(t):              # fn_14027a310
        map.redraw()                 # fn_140010ca8
destroy map
```

`[确认]` 扣费发生在市价板打开**之前**。打开后立刻关掉也已经扣了——对得上「每看一次缴费一次」。钱不够不离开地图，可改点或取消。

### 2.7 市价板是只读交易所皮

`CPriceInformationView` 体积 `0x8e8`（dtor `fn_14000750e(..., 0x8e8)`）。继承链接到 `CTrade`（`class_hierarchy.txt`）。

构造 body `0x14027ad30`：

- 资源 `TradeWin.dk4`
- 循环 `eTownFashion >= 2` 清 `+0x748` 两槽
- 循环 `eTradingItemKind >= 0x13` 清 `+0x758` 共 19 槽
- 循环 11 次（`0xB`）做两套 sprite（`+0x5a8` / `+0x600`）
- 绑定：`fn_14000a907(view, town_id, …)` 把目标港写进 `+0x590 / +0x594`，再加载 `InfoBox.dk4`

附属 UI：`CPriceInfoTradingItemKindButton`、`CPriceInformationTradingItemView`。种类按钮筛 19 类；板子展示目标港特产品 / 种类市价 / 当前两格流行。数值全部是 `quote(target, …)` 与 factions 流行快照，本会话不改指数、不改供给。

### 2.8 会计术：原版扣费不用它

技能说明「鉴定商品，物价情报的判断力。」（`03-characters/03-abilities-skills-jobs.md`，`eSkill` 4 = `accounting`）。那是人物系统的文案。

`[确认]` `fn_140003918` 与 `fn_14027a310` 不调用 `fn_1400135cf` / `fn_14000e1ce`。会计术真正进交易所的是成交杀价 `fn_1400137b4`（`CTrade::CommitDeal` 调），与本篇无关。

重构仍预留 `ModifierSet.price_intel_fee_factor`。原版包必须是恒等（不打折）。禁止在引擎里写死「会计越高越便宜」却标成原版确认。

## 3. 实体与 ID

| 实体 | 键 | 范围 / 哨兵 | 存哪 |
| --- | --- | --- | --- |
| 行情会话 | 不持久 | 一次交易所菜单 | 不存 |
| 目标港 | `town_id` | 0–表长；未选 `town.none=98` | 命令参数 |
| 已知可查 | `(town_id, flag known_for_intel)` | bit 1 | 存档 `CTown+0x10`（world 状态） |
| 份额门 | `Factions.player_share(town_id)` | 0–100；>0 走低价 | 存档份额槽 |
| 金币报价板 | 纯查询 | 见 `02` | 不存 |
| 本笔扣费 | `gold` | 原版包 0 / 50 / 200 | 不存公式 |

没有「已付费港」缓存。下次再点同一座港再扣一次。

## 4. 算法 / 公式

参数进 `config/formulas/economy.yaml` 的 `price_intel:` 与 `config/economy/price_intel.yaml`。代码禁止出现 50、200、`0x2F`、`0x62`。

### 4.1 点选映射

```
town_none            = 98          # vanilla 0x62
button_id_base       = 47          # vanilla 0x2F
button_id_end        = 144         # vanilla 0x90，开区间

selected_town(button_id):
    if button_id_base <= button_id < button_id_end:
        return button_id - button_id_base
    return town_none
```

运行时用 `len(towns)` 代替 `button_id_end - button_id_base`；原版包 97 与 `0x90-0x2F` 相切。

### 4.2 可见

```
visible(town_id) =
    World.town_flag(town_id, flag_known_for_intel)   # 原版 bit 1
```

经济不写该位。入港 / 开局 / 其它发现路径由 world 发 `TownKnown { town_id }`。

### 4.3 扣费 `price_intel_fee`

```
share = Factions.player_share(town_id)               # 玩家势力在该港 3 槽中的份额；没有槽 = 0

raw =
    fee_same_town          if town_id == current_town_id     # vanilla 0
    fee_has_share          if share > share_positive_min     # vanilla 50, min 0
    fee_unknown_relation   otherwise                         # vanilla 200

fee = clamp(raw * Characters.modifier(price_intel_fee_factor),
            gold_fee_min, gold_fee_max)
```

原版包：

```
price_intel_fee_factor = 1          # 会计术不参与
share_positive_min     = 0          # 比较是 > 0
gold_fee_min           = 0
gold_fee_max           = gold_quote_max   # 与 02 同一金币夹值；本功能观察值远小于它
```

`target == current` 的判断在份额之前。本港即使份额为 0 也是 0 金。

钱包不够：不扣、不打开、会话留在地图。够则先扣后开。扣 0 金也算一次成功查看。

### 4.4 打开市价板

```
on confirm(town_id):
    if town_id == town_none: close session
    if not visible(town_id): ignore click
    town = World.town(town_id)
    if town is None: ignore
    cost = price_intel_fee(current_town_id, town_id)
    gold = Wallet.gold()                   # 与 03 CommitDeal 同一 fn_140012bfc / +0xc
    if gold < cost:
        emit PriceIntelRejected { town_id, reason: not_enough_gold }
        return
    Wallet.add_gold(-cost)                 # 原版 fn_14001117b，无月账 expense 调用
    emit PriceIntelViewed { from_town_id, town_id, gold: cost }
    ui.open PriceBoard {
        town_id,
        kinds: Economy.kind_rates(town_id),          # 19
        specials: Economy.specials(town_id),         # 最多 5
        fashion: Factions.current_fashion(town_id),  # 2 格
        quotes: Economy.quote(town_id, item_id)      # 按种类筛
    }
```

市价板只读。关闭板子回到地图，不退款。

## 5. 耦合分析

原版：

- 行情函数写在 `CTrade` 同文件家族（`fn_1402755b0.cpp`），和买卖、投资抢同一个菜单。
- `CPriceInformationView` 继承 `CTrade`，体积几乎是交易窗，只是不 `CommitDeal`。
- `CMarketPriceRouteMap` 继承航路选港图，资源 `RouteMkr.dk4` / `RouteFrm.dk4`。
- 扣费直接改玩家金币对象 `+0xc`。
- 低价门直接扫 `CTown` 份额槽。
- 可见性直接测 `CTown+0x10` bit 1。
- 按钮号 `0x2F` 写死在 `fn_140012238`。
- 会计术说明写了「物价情报」，扣费没读。

重构：

```
town-life  OpenExchange { town_id }
    → economy 开买卖 / 投资 / 本篇 OpenPriceIntel

economy.price_intel
    → World.town_flag(known_for_intel)     只读
    → Factions.player_share                只读
    → Wallet.gold / add_gold               与 03 同一玩家金库；只发 delta
    → Characters.modifier_set              只读 factor（原版 1）
    → Economy.quote / kind_rate            同系统只读
    → ui 投影航路图与市价板
```

禁止：UI 自己算 50/200；经济写 `CTown+0x10`；行情会话改指数。

## 6. 解耦后的配置

```yaml
# config/economy/price_intel.yaml
schema: dk4.economy.price_intel
version: 1

entry:
  facility_key: exchange          # 不是独立设施
  menu_index: 2                   # 原版 sMenuOption 下标；禁止代码写死 if (i==2)
  pay_per_view: true              # false 则一次会话只扣第一座（原版包必须 true）

selection:
  town_none: 98
  button_id_base: 47              # 0x2F；仅对照原版 UI。运行时按表长生成按钮
  map_marker_kind: 5              # vfunc_45 的类型号；UI 配置可改

visibility:
  flag: known_for_intel           # World.town_flag 的名字
  original_bit: 1                 # CTown+0x10；对照用，运行时读 flag 名
  reveal_on_visit: true           # 对应入港 SetBit1
  # 开局揭示表属于 world 内容包（按主角），本文件不列 97 行

ui:
  route_map_class: CMarketPriceRouteMap
  board_class: CPriceInformationView
  resources: [RouteMkr.dk4, RouteFrm.dk4, TradeWin.dk4, InfoBox.dk4]
  kind_slots: 19                  # 与 NUM_TRADINGITEMKIND 对齐，运行时读种类表长
  fashion_slots: 2
  item_row_slots: 11              # 构造循环 0xB；布局不是规则
```

```yaml
# config/formulas/economy.yaml  （本篇新增段）
price_intel:
  fee_same_town: 0
  fee_has_share: 50
  fee_unknown_relation: 200
  share_positive_min: 0           # 比较用 > 此值
  gold_fee_min: 0
  gold_fee_max: null              # 缺省用 quote.gold_max
  accounting:
    enabled: false                # 原版不打折。true 时乘 factor
    skill_key: accounting         # eSkill 4；只给人物系统映射 ModifierSet
    # price_intel_fee_factor 由人物系统输出，缺省 1
    # 禁止在本表编造「(100-skill)/100」充原版
```

```yaml
# config/caps.yaml（本篇相关）
price_intel_fee_max: 200          # 原版观察顶；模组可加大，仍 ≤ gold_quote_max
```

```yaml
# config/ui/strings.zh-CN.yaml  （文案，不进逻辑）
price_intel.prompt: "要看哪个城市的市场行情呢？"
price_intel.pay_each_view: "查阅市场行情每看一次缴费一次。"
price_intel.not_enough_gold: "钱不够，我这里不是慈善机构，没有钱是不能给你看的。"
```

存档：不存会话。要存的是 world 的 `known_for_intel` 位和 factions 的金币 / 份额——本篇不另开表。

## 7. 独立旋钮

改这些不应迫使货舱、战斗或航线代码重写：

- `fee_same_town` / `fee_has_share` / `fee_unknown_relation`
- `share_positive_min`（改成「份额 ≥ 10 才算熟客」只动本公式）
- `pay_per_view`（一次会话通票）
- `accounting.enabled` 与人物系统的 `price_intel_fee_factor` 曲线
- 开局揭示哪些港（world 内容包）
- 市价板展示哪些列（UI）

把「熟客价」改成按距离，只换 `price_intel_fee` 纯函数，不要让航路图去算金币。

## 8. 对外接口

```
PriceIntelQuote = {
  from_town_id,
  to_town_id,
  visible: bool,
  fee: int,
  share: int,
}

Economy.priceIntelFee(from_town_id, to_town_id) -> PriceIntelQuote
Economy.openPriceIntel(from_town_id)            # 开地图循环；UI 调
Economy.confirmPriceIntel(from_town_id, to_town_id)
    -> { ok, fee, reason? }                     # 扣钱 + 允许打开板
Economy.priceBoard(town_id) -> {
  town_id,
  kind_rates: [{ kind_id, kind_rate }],
  specials: [{ slot, item_id, buy_gold, sell_gold }],
  fashion: [fashion_id, fashion_id],            # 空哨兵按 factions
}
```

只传 ID 与整数。不传 `CTown*`、`CTrade*`、`World*`。

命令（谁可以发）：

```
OpenPriceIntel { town_id }           # town-life / 交易所菜单
ConfirmPriceIntel { from, to }       # UI 选港后
Wallet.add_gold { delta }            # 本系统发；与 Trade.commit 同一接收方
```

事件：

```
PriceIntelViewed   { from_town_id, to_town_id, gold }
PriceIntelRejected { to_town_id, reason }   # not_enough_gold | not_visible | no_town
TownKnown          { town_id }              # world 置位后发；本系统刷新可见集
```

UI：`CMarketPriceRouteMap` 对悬停港调 `priceIntelFee` 画数字；确认后调 `confirmPriceIntel`，成功再向 `priceBoard` 要快照。

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| mes 下标 `0x13` / `0x38c` 与 `mesfile_sc.txt` 偏移的一一对应 | 只确认调用了 `fn_1400012da`；句文是推断 |
| `0x2ef` 里「金额方面能不能再给我们想想办法」是否曾用于行情还价 | 扣费路径没有还价；当杀价套话直到找到独立 xref |
| 开局 `0x1402a8e10` 的 jump table → 提督 / 剧本 | 有置位调用；分支与 `ePerson` 0–6 未钉 |
| `0x1401b5610` 置 bit 1 的玩法触发 | 有 call，条件未读完 |
| bit 0 / bit 2 / `eTOWNACCESS` 与 bit 1 的关系 | 独立 setter；行情只用 bit 1 |
| 未置位港在航路图上是否仍绘制 | 只确认点选被拒 |
| `CPriceInformationView` 11 行 sprite 对应特产品还是种类分页 | 构造循环 `0xB`，填充函数未完整抬码 |
| 市价板是否显示买入列、卖出列还是指数 | 走 `quote` 家族；列开关 `[未知]` |
| 扣费是否写入月账 `expense` | `fn_14027a310` 只 `AddGold(-cost)` |
| 会计术折扣曲线 | 原版无；YAML `enabled: false`。不要把攻略口说写进 default |
| 悬停 `vfunc_45` 两个城镇参数顺序与扣费是否始终一致 | 扣费路径已钉 `fee(target, current)`；绘制路径动态类型转换未逐项对完 |
| `map_marker_kind == 5` 的枚举名 | 立即数 |
| 钱包对象 `+0xc` 的类型名 | 与买卖共用，字段正式名未恢复 |
