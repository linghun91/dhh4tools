# 广场情报与广告人

入港广场是城镇生活的情报层：人群事件、零售店招牌上的流行、世界单例 `CAdman` 的广告、缴费查阅行情的**入口**、以及听闻/散布谣言的表现。市价怎么算、扣多少金币、面板上画出哪 19 条种类，一律交给 economy（`04-economy/02-market-price.md`，行情专篇 `04-economy/08-price-intel.md`）。本系统只拥有广场侧的呈现、触发与 `CAdman` 两个存档字节。

设施目录仍在 `01-facilities.md`。本篇不新增 `facility_id`，只把广场内部点了什么说清楚。

## 1. 职责边界

本系统管：

- 广场场景内的点击翻译（招牌 / 寻求支持 / 发放交易品 / 离开）
- 按港口发展度决定亮几块零售店招牌，以及每块招牌挂哪件交易品（挑选函数未还原的部分标 `[未知]`）
- 点招牌后**展示**流行情报（目录在 factions）
- 人群聚集事件的触发与派发（打开小游戏 / NPC / 广告人，具体集合未还原）
- `CAdman` 单例状态：当前 `town_id` + 广告中的 `trading_item_id`
- 「查阅市场行情」对话的打开与选港；**不**算费、不持有报价
- 广场上听到的谣言种类与文案入口；计略「制造谣言」的目录在 `04-schemes-rhetoric.md`

本系统不管：

| 事项 | 归属 |
| --- | --- |
| 市价指数、金币报价、`CPriceInformationView` 画哪些列 | economy |
| 缴费金额、会计长杀价公式 | economy（`08-price-intel`）；本系统只传 `paid_gold` |
| `eTownFashion` 26 条目录、每港 2 槽、速度枚举 | factions（`02-countries-culture-fashion.md`） |
| 小游戏规则与奖金 | `03-minigames.md`（本系统只发 `source: plaza_crowd`） |
| 计略「制造谣言」是否成功、改友好度 | `04-schemes-rhetoric` → factions |
| 阿科布人物六维、能否入队 | characters（`person_id` 89） |
| 港口坐标、发展度怎么涨 | world / factions |
| `Plaza.dk4` 像素、`CPlazaSylphid` 走位 | ui |
| 文案字符串 | ui / localization |

入口：`ArrivedAtTown` 之后打开 `CPlazaView`。交易所仍走 `OpenExchange`，不经本篇。

## 2. 原版事实

### 2.1 广场对象与点击号

`[确认]` `CPlazaView : CView, …, CPlazaScreen`。主构造 `0x1401636d0` 体积在析构里按 `0x148` 释放。`CPlazaScreen` 嵌在 `+0x80`，自身体积 `0x68`。来源：`srcdata/source/game/town/CPlazaView.cpp`、`CPlazaScreen.cpp`。

`[确认]` 构造写：

| 偏移 | 初值 | 随后谁写 |
| --- | --- | --- |
| `+0xe8` | 0 | |
| `+0xf0` 起 5 个指针 | 0 | `fn_14000aa79` 填 `CShopButton*` |
| `+0x118` | 1 | `fn_14000aa79` 改成招牌数 1–5 |
| `+0x120` | 0 | 条件创建「寻求支持」`CPlazaButton*` |
| `+0x128` | 0 | 条件创建「发放交易品」`CPlazaButton*` |

`[确认]` `CPlazaView::vfunc_13`（`CPlazaView.cpp`）：

| 点击号 | 分支 | 对象 |
| --- | --- | --- |
| `4 <= id < 9` | `slot = id - 4`（`sub rax,4`）；先 `fn_140004ba1(CPlazaScreen, slot)`，再 `fn_14000d01c(+0xf0[slot])` | 最多 5 块 `CShopButton` |
| `id == 1` | `fn_140014d58` | 离开 |
| `id == 2` | 执行 `fn_14001361a`（寻求支持本体）；返回非 0 则 `fn_14000daf8`；随后若 `fn_140166580()==0`，对 `+0x120` 按钮调 `fn_140014083`（`[推断]` 置灰/隐藏） | 寻求支持 |
| `id == 3` | 执行 `fn_140011e19`（发放本体）；返回非 0 则 `fn_14000daf8`，再若 `fn_1401667b0()==0`，对 `+0x128` 调 `fn_140014083` | 发放交易品 |
| `id == 0xA` | `fn_140003d37` | `[未知]` |

`[确认]` 点击 4–8 对应招牌槽 0–4。原始指令 `0x1401685a2 sub rax,4` 后再按 `slot*8+0xf0` 取指针，反编译丢了这一步。

`[确认]` `fn_140166580` / `fn_1401667b0` 不是「为真才继续」的前置门，而是**动作之后**重算按钮可见性；按钮初次是否创建也由它们决定（`fn_14000aa79` 末尾调 `fn_140166580`，为真才 `new CPlazaButton`）。

`01-facilities.md` 把 1/2/3/4–8/`0xA` 标成未还原。本篇把 1–3 的标签和 4–8 的「是招牌」升格；`0xA` 仍未知。

`[确认]` 三颗 `CPlazaButton` 的 GBK 标签（`fn_14000aa79` 调 `CPlazaButton` ctor 的 `lstrcpyA` 实参，UTF-16 转义按 LE 解 GBK）：

| 槽 | 点击号 | 标签 | `Plaza.dk4` 帧（构造实参） |
| --- | --- | --- | --- |
| 离开 | 1 | 离开广场 | 0 / 1 |
| `+0x120` | 2 | 寻求支持 | 2 / 3 |
| `+0x128` | 3 | 发放交易品 | 4 / 5 |

`[确认]` `CPlazaScreen` ctor 写死四对整数 `(0x96,0xDA)` `(0x8A,0xBA)` `(0x3AB,0x287)` `(0x3C0,0x316)`。`[推断]` 广场底板坐标，不是玩法规则。

### 2.2 发展度决定招牌数

`[确认]` `fn_14000aa79`（`fn_140163ac0.cpp`，RTC `anDevelopment` 16 字节）读港口一个标量（`fn_140008f1c` → `fn_14000bc03`），再与四档阈值比较：

```
anDevelopment = [2000, 3000, 5000, 7000]   # 0x7d0, 0xbb8, 0x1388, 0x1b58
shop_count = 1 + count(develop >= threshold[i] for i in 0..3)
# 结果 1..5，写入 CPlazaView+0x118
```

随后按 `shop_count` 循环 `new CShopButton`（ctor `0x140163810`，体积 `0x1c0`），指针进 `+0xf0`。

`[推断]` 这个标量是商业发展度（`town_develop` 双轨之一）。依据：阈值落在 0–9999 的发展度值域内（`caps.yaml` `town_develop: 9999`），不是份额 0–100。未在局部名看到 `nCommerce`。

`[确认]` 招牌 X 表 RTC `anShopX`（20 字节 = 5 个 int，`fn_14000ba1e`）：`0x21A, 0xDA, 0x2BA, 0x35A, 0x17A`；Y 立即数 `0x1B1`。属 ui 像素，原版包可抄进表，代码禁止再写这组数。

### 2.3 零售店招牌与流行情报

`[确认]` `CShopButton` ctor 把 `+0xa8` 写成 `0x6A`（`ITEM_EMPTY`）。来源：`CShopButton.cpp`。`01-facilities.md` 曾问这是不是误用空货格哨兵——不是误用，这是招牌上的**交易品槽**，空=106。

`[确认]` 每块招牌创建后调 `fn_1401696e0`（RTC `nMonth` `nDay` `nTime`）得到一个 `trading_item_id`，再交给 `fn_14000c89c`：若 `== 0x6A` 跳过画图，否则从 `Plaza.dk4` 取帧。`fn_1401696e0` 读该港两格流行（`fn_14000b4dd` 两次）和日历。精确抽选 `[未知]`。

`[确认]` 文案「请选择零售店的招牌，就可以听取流行情报。」mesfile 偏移 `0x1143`。同块还拼了「据说在 %s 将流行 %s」「%s 的市场份额上涨了」等，抽取器按块拼接，**不能**当成一句。

`[确认]` 点招牌后的流行句（factions 已引用，本篇只负责触发）：

| 偏移 | 句 |
| --- | --- |
| `0x8653` | 「这城市好像正在流行 %s」；同块混有埋藏物传闻 |
| `0x86fb` | 「听说 %s 正流行 %s」 |
| `0x8727` | 「听说 %s 正流行 %s，颇受好评」 |

`[确认]` 店主形象：`person_id` 98–107，九个文化圈（欧洲拆男女）。目录在 `config/factions/cultures.yaml` 的 `plaza_shopkeepers`。本系统只问 `Fashion.plazaShopkeeper(town_id)`。

`[确认]` 「我们是零售店。」在 `0x5f13` 长块里，与舰长室指令、畅销推销句拼在一起。零售店自称 `[确认]` 文本；它是否从这块偏移直接取出 `[未知]`。

### 2.4 人群事件

`[确认]` 入港/广场台词「提督。好像广场上聚集了一群人。」多主角变体，mesfile `0x12c`–`0x217`。`12-ui-save/03-localization.md` 已收 `mes_0x12c`。

`[确认]` `CPlazaSylphid` 构造清 10 槽（`>= 0xA`），体积 `0x380`，状态字节出现 0–4，坐标步进 ±2。来源：`CPlazaSylphid.cpp`。`[推断]` 广场行人动画，不是人群事件的胜负。

`[确认]` `fn_14000b53c` 在广场函数族里：先过若干守卫（含 `fn_140005272(2)`、某对象 `vfunc` 非 0、返回值 `== 3`），再 `new` 一段从 `start` 走到 `end` 的演出，类型立即数 `0x33` / `0x34` 二选一。`[未知]` 这是不是「聚集了一群人」的生成器；`0x33/0x34` 不要当成 mesfile 下标（下标 51/52 是主角简介）。

`[未知]` 原版触发率。`01-facilities.md` 的 `plaza_crowd_chance` 仍为 `null`。

`[确认]` 原版广场代码**不**直接启动小游戏：7 个小游戏只经事件脚本指令（`fn_1400020fe` → `fn_14000508d` → `fn_14000847c`）按游戏号启动（`03-minigames.md` §2.0）。「聚集了一群人」若引出小游戏，也是剧本事件，不是广场自带抽签。`plaza_crowd` 只作模组挂钩，原版包不填。

### 2.5 `CAdman` 世界单例

`[确认]` `CAdman : CSaveObject : CObject`。RTTI 8 个虚函数。来源：`class_hierarchy.txt`、`rtti_classes.h`、`srcdata/source/game/world/CAdman.cpp`。

`[确认]` **只有一个实例**，嵌在 `CTownElement+0x2d80`。`CTownElement` ctor：

```
fn_140007dce(this+8, stride=0x78, count=0x61)  # 97 个 CTown
fn_1402a59c0(this+0x2d80, ...)                 # CAdman::CAdman
```

`97 * 0x78 = 0x2D78`，再加 8 字节头 = `0x2D80`。析构 / 存档循环先扫 97 港，再单独调 `+0x2d80` 的虚函数。来源：`town/CTownElement.cpp`、`world/CTownElement.cpp`。

`[确认]` `CWorld` ctor 在 `+0xc340` 构造 `CTownElement`（`fn_1402a68f0`）。因此广告人在 `CWorld+0xF0C0`。下一对象在 `+0xf0d0`，体积观察为 `0x10`。

`[确认]` 存档字段只有两字节（`vfunc_05` 初始化，`vfunc_06/07` 各调一次字节读写）：

| 偏移 | 初值 | `vfunc_04` |
| --- | --- | --- |
| `+0x08` | `0x62`（98） | 不看 |
| `+0x09` | `0x6A`（106） | `== 0x6A` 返回 0，否则 1 |

`fields.tsv`：`CAdman 0x9×2, 0x8×1`。与 `CRequest`/`CScheme` 一样，`vfunc_04==0` 表示空/未启用。

`[确认]` `0x6A` 是交易品空哨兵（`ITEM_EMPTY`，`04-id-registry.md`）。`[推断]` `+0x09` 是正在广告的 `trading_item_id`。

`[推断]` `+0x08` 是广告人所在 `town_id`，原版空哨兵 `98 = NUM_TOWN+1`（重构运行时为 `null`，98 只在 legacy codec）（`01-world/02-towns.md` 已用于选港 UI）。依据：对象跟在 97 港数组后面；`fn_140168770` 把读到的值与 `0x62` 比较走「无港」分支。禁止把 98 写成「欧洲男子店主」——那是 `person_id` 98，另一套空间。

`[确认]` 广场布局末尾 `fn_14016a130`：读广告人两字节，若商品不是 `0x6A` 且与本港某槽相等，则在 `CPlazaView+0xd8` 再挂一个精灵。`[推断]` 「广告人此刻在这座广场」。

`[确认]` `fn_140168770` RTC `aeAdmanMessage` 36 字节 = 9 个 dword：

```
0x343, 0x344, 0x345, 0x346, 0x347, 0x348, 0x371, 0x372, 0x373
```

按本港 / 广告人字节选一句，经 `fn_140004b3d` 播出。这 9 个整数是 **exe 消息号**，不是 mesfile 文件偏移，也不是按 NUL 切的序号。`[确认]` 按 exe 消息号表（`.data 0x140683620` 37 块 `{first_id,size}` + `0x1406836c0` 每条块内偏移，`11-progression/01-level-exp-skills.md`）解 SC：

| 消息号 | 句 |
| --- | --- |
| `0x343` | 阿科布啊，好像在%s。 |
| `0x344` | 阿科布啊，我想在%s。 |
| `0x345` | 阿科布呀，我想在%s。 |
| `0x346` | 阿科布去追求新流行的人们所在的城市了。 |
| `0x347` | 阿科布为了寻找传播新流行的城市，去流浪了。 |
| `0x348` | 不知道阿科布去哪里了，他在寻找掀起新流行的城市。 |
| `0x371` | 阿科布啊，刚才还在这里呢。 |
| `0x372` | 阿科布啊，在那里见到了。 |
| `0x373` | 阿科布呀，不知道。 |

即前 3 句报告广告人所在港（`%s` `[推断]` 为 `+0x08` 港名），中 3 句「去向不明」，后 3 句「刚在本港」。九句的主语全是阿科布。

`[确认]` 气候文档已声明：`CAdman` 写 `0x62/0x6A` 与气候无关，勿混入 `04-climate-weather-zodiac.md`。

### 2.6 查阅市场行情（触发，扣费属经济）

`[确认]` mesfile `0x2ef`：「查阅市场行情每看一次缴费一次。要看哪个城市的市场行情呢？」同块拼了杀价句「金额方面能不能再给我们想想办法。」后续 `0x35f` `0x383` 仍是金额通融。

`[确认]` 船员对白 `0x16ce7`：「要调查各港口的市场行情。」`0x16d33`：「市场行情吗……嗯，不十分了解。」

`[推断]` 入口在广场店主对话（`0x2ef` 紧挨店主寒暄 `0x257`），舰长室那句是催促而不是第二套 UI。未在 `CPlazaView::vfunc_13` 看到独立点击号，故不写成「第 N 个按钮」。

`[确认]` 真正的行情面板是 `CPriceInformationView : … : CTrade`，体积 `0x8E8`，资源 `TradeWin.dk4`。构造 RTC：

- `eTownFashion >= 2` 清 2 个 `+0x748` 指针
- `eTradingItemKind >= 0x13`（19）清 `+0x758`
- 再循环 `>= 0xB`（11）对 `TradeWin.dk4` 精灵

来源：`srcdata/source/game/trade/CPriceInformationView.cpp`。行项目 View：`CPriceInformationTradingItemView`。选港投影：`CMarketPriceRouteMap`（economy `02-market-price.md` 已说明它不存市价表）。

本系统**不**实现该 View，只发 `OpenPriceIntel { town_id }`。扣费、报价、2 格流行怎么画，见 economy。

缴费数字 `[未知]`。杀价句存在，是否走修正器 key（如 `trade.intel_fee`）/ 会计术 `[未知]`，禁止在本系统读六维或技能值。

### 2.7 谣言：广场听闻 vs 计略制造

两套不要并成一张表。

**A. 广场听闻（本系统）** `[确认]` 文本 `0xbadf` 长块：

- 「%s 的 %s 价格大幅滑落」
- 「传闻 %s 损失颇大」
- 「好像 %s 囤积着 %s」
- 「买错了 %s，非常后悔」
- 「提督，得到了 %s 的情报」

`[未知]` 听完是否解锁该港行情（免一次缴费），还是纯对白。实现缺省：只展示，不写市价缓存。

**B. 计略「制造谣言」（`04-schemes-rhetoric.md`）** `[确认]` 文本：

- 选目标港：「在哪个城市散布谣言？」`0x912f` `0x919b`
- 结果：「在 %s，流传着 %s 的坏话。」「这传闻没有被相信。」`0x3ad7`
- NPC 预警：「%s 要在 %s 制造谣言」`0x150af` 起

本系统若在广场播「城里在传某势力的坏话」，只消费 `Schemes.lastRumor(town_id)` 的只读快照，不掷成功骰。

`0x8653` 里混有埋藏物方向传闻，发现物属 world/items，广场只当 `rumor.kind = remains_hint` 展示。

### 2.8 阿科布与 `CAdman`

`[确认]` 阿科布·达迈 `person_id` 89（`PersonID.h` `ePerson_089`，`Docs/_data/persons.tsv`）。文案：要把「新的流行趋势带到世界的所有城市」；请求把阿科布带到别的广场；发放品牌商品 `%s` 做宣传。`0x6fa3` `0x728b` `0x72ef`（mesfile 块偏移）。

`[确认]` `CAdman` 没有 `person_id` 字段，但它的九句消息（2.5）主语全是阿科布。`[推断]` `CAdman` 就是「阿科布行踪 + 他在推广的商品」的状态块，人物身份写死在文案里而不在数据里。

`[推断]` 阿科布抵达某港、或发放品牌时，叙事发 `Adman.set(town_id, item_id)` 写入单例。cpp 未见 `person==89` 与 `CAdman+0x8` 的赋值。重构把广告人做成内容字段 `adman.person: vanilla:person_089`，文案 `%s` 取该人物名；禁止代码里写死 89。

### 2.9 资源

`[确认]` 广场图包 `Plaza.dk4`（`fn_14000aa79`、`fn_14000c89c`、`fn_14016cc40.cpp` 多处 `fn_140010e6f(..., "Plaza.dk4", frame, ...)`）。帧号进 ui，不进玩法公式。

`[确认]` 店主寒暄 `0x257`「有什么事吗？谢谢。今后还请多关照。」杀价 `0x27b` 起。传教士在队时可散发交易品（`01-facilities.md` `0xc1fb`），对应本篇点击 3。

## 3. 实体与 ID

| 实体 | 原版符号 | 计数 | 空 / 非法 | 来源 |
| --- | --- | --- | --- | --- |
| 广告人 | `CAdman`（无 `eAdman`） | 1 | 原版 `item == 106` 未启用；运行时 `item: null` | `CAdman.cpp` |
| 广告人所在港 | 无独立枚举 | 与 `eTown` 同空间 | 原版 `0x62` = 98 `[推断]` 无港；运行时 `null` | 初值 + 选港哨兵 |
| 广告商品 | `eTradingItem` | 表长 | `0x6A` = 106 | `+0x09` |
| 招牌槽 | 无 | 每港 1–5 | 槽内 `106` 不画 | `+0xf0`、`+0x118` |
| 广场点击 | 无 `ePlazaClick` | 原版号 1–8、`0xA` | — | `vfunc_13` |
| 店主 | `ePerson` 98–107 | 10 | — | factions 圈表 |
| 阿科布 | `ePerson` 89 | 1 | — | 人物表 |
| 人群事件 | 无 | 重构目录 | `null` | 本篇 key |
| 谣言种类 | 无 | 重构目录 | `null` | 文案 |

原版没有 `eFacility` 子枚举「情报」。重构 key 只在本系统内用（命名空间 key，D1）。

哨兵碰撞：`98` 既是原版「无港」，又是欧洲男店主 `person_id`；`106` 既是空货，又是日本店主 `person_id`。这正是 D1 的理由：运行时一律 `null`，原版哨兵只在 `ids/legacy_codec.yaml` 导入导出时翻译。

## 4. 算法 / 公式

全部进 `config/formulas/town.yaml`。场景代码禁止出现 `2000`、`0x6A`、`if click==4`。

```
# 招牌数。develop 来自 Towns.develop(town_id, commerce)  [推断 商业轨]
shop_count(develop) =
    1 + count(develop >= t for t in shop_develop_thresholds)
shop_count ∈ [shop_count_min, shop_count_max]
shop_develop_thresholds = [2000, 3000, 5000, 7000]
shop_count_min = 1
shop_count_max = 5

# 招牌商品。pick 原版 fn_1401696e0，未还原前原版包标 unverified
sign_item(town_id, slot) =
    PlazaSignPick(town_id, slot, Fashion.current(town_id), Clock.now())
    # 返回 trading_item_id | null

# 广告人是否启用
adman_active =
    adman.item != null
adman_present(town_id) =
    adman_active AND adman.town_id == town_id

# 寻求支持按钮。原版 fn_140166580（body 0x140166580）逐条 return 0，只有末尾一处 return 1 [确认]
can_seek_support(town_id, fleet_id) =
    probe_item != null                                              # fn_140168a50 != 0x6A（交易品）
    AND post_holder(fleet, support_post) != null                    # fn_140010cee(fleet, 3) != 0xEC；岗位 3 = 会计长 purser
    AND Skill(holder, support_skill) >= support_skill_min           # fn_14000e1ce(p, 4) >= 100（jge）；eSkill 4 = 会计术 accounting
    AND fn_14001423b()                                              # [未知] 布尔
    AND NOT (share_guard AND share_sum == share_cap)                # 三槽份额累加 == 100 且守卫成立
    AND special_product_type(town_id, probe_item) == support_special_type   # fn_14000680c == 6
    AND town_kind_value(town_id, kind(probe_item)) >= support_rate_min      # fn_1400095ed >= 100
    AND town_other_value(town_id) < support_other_max                       # fn_1400088f0 < 100
support_post          = purser       # 原版岗位下标 3 [确认]（03-characters 岗位表，fn_140012dd2）
support_skill         = accounting   # 原版 eSkill 4 [确认]；fn_14000e1ce 第二参是技能，不是六维
support_skill_min     = 100          # 技能值（0–500 量纲）
share_cap             = 100
support_special_type  = 6          # 类型名 [未知]
support_rate_min      = 100
support_other_max     = 100

# 寻求支持动作。原版 fn_14001361a（body 0x1401651d0）[确认]；写成 D9 Action
seek_support: Action
  guard:  [ can_seek_support(town_id, fleet_id), player.confirms() ]
  roll:   seek_support_roll          # rand(1000) < th，命名随机流 town.plaza
  #   th = (town_kind_value(town_id, kind) - 100) * support_rate_mul        # *10，原始 imul 0xA
  #      + Skill(post_holder(fleet, purser), accounting)                    # fn_14000f2d6(fleet,3) → fn_14000e1ce(p,4)
  #      + Factions.influence(player_force, MajorSea.of_town(town_id)) / support_influence_div   # /10（cdq; idiv 0xA）
  effects:
    success: [ Share.add(town_id, player_force, support_share_gain, reason: support) ]   # [确认] 份额 +1：0x1401653ad–0x1401653cd `fn_1400122f6(town, fn_1400054cf()=玩家势力, 1, 1, 1)`，与卖流行品同形；消息 0x341「噢，太棒了！只有%s才会如此！」+ 0x8f「%s的市场份额上涨了。」
    else_if influence >= support_influence_b_min:                      # 1000
             [ fn_140012a21 / fn_140004435(…,1) ]                     # 消息 0x342「嗯，是%s吗？它可能会畅销。」；字段 [未知]
    else:    []                                                       # 仅音效 0x90
  cost:   { days: support_days_cost }  # fn_14000b122(…, 1)，三支都走 1 天

# 发放交易品：岗位 + 船上有货
can_distribute(town_id, fleet_id) =
    Ships.hasCargo(fleet_id)                                  # fn_1401667b0：fn_140007266(asShipLoad) != 0
    AND post_holder(fleet, distribute_post) != null           # fn_140010cee(fleet, 0xA) != 0xEC
distribute_post = missionary   # 原版岗位 10 = 传教士 [确认]（岗位表）；mesfile 0xc1fb「如果有传教士…在广场上还能散发交易品」

# 发放动作 fn_140011e19（body 0x1401680b0）[确认]：确认后读货舱 nLoadTotal，<=0 退出；
# 消息 0x2ee「嗯！如果是免费赠送就试试！」；fn_1400026d5(post_holder(fleet, missionary), 50, 1)（[推断] 给传教士加 50 经验）；
# 具体扣货与效果在 fn_14000938b / fn_14000e79b / fn_14000c658，属 economy [未知]

# 人群。触发率未还原
on PlazaOpened:
    if rand() < plaza_crowd_chance:          # null = 原版未核实，禁止填假数
        emit PlazaCrowdGathered { town_id }
        # 订阅方：narrative / 本系统 Adman 表现；小游戏只能经叙事 Minigames.run

# 查阅行情：本系统只做入口
OpenPriceIntel(town_id):
    fee = Economy.priceIntelFee(from_town_id, to_town_id)   # 金额 [未知]，属经济
    # 杀价若有，走 Modifiers.eval(key)，不读六维 / 技能值
    emit PriceIntelRequested { from_town_id, to_town_id, fee }
    # economy 扣费失败则本系统不打开选港
```

`support_share_gain = 1`（原版立即数，`formulas/town.yaml`）。招牌抽选、人群概率、行情费、寻求支持结果 B 的效果，全部 `[未知]` 的保持 `null` / `unverified: true`。缺省禁止在代码里猜。

## 5. 耦合分析

原版硬编码：

- `CPlazaView` 直接 `new CShopButton` / `CPlazaButton`，标签写在构造实参里
- 招牌数阈值、`anShopX`、`Plaza.dk4` 帧号写在 `fn_14000aa79`
- `CAdman` 焊在 `CTownElement` 尾部，和 97 港同一存档虚函数链
- `CPriceInformationView` 继承 `CTrade`，行情 UI 与买卖会话同族
- 流行 2 槽、种类 19、空货格 106 在广场、交易所、广告人三处各写一遍
- `aeAdmanMessage` 九个立即数
- 人群文案按主角变体散落 mesfile
- `0x62` 同时当「无港」和「店主 person 98」

重构后允许的依赖：

```
town-life/plaza → world.town_id
town-life/plaza → factions.Fashion.current / plazaShopkeeper / townDevelop
town-life/plaza → economy.OpenPriceIntel / DistributeGoods / kind_rate（只读）
town-life/plaza → fleets.post_holder(fleet, post_key) / characters.Skill(person, skill_key)（只读）
town-life/plaza → ships.hasCargo（只读）
town-life/plaza → narrative（阿科布事件、Adman.set）
```

禁止：economy 改招牌表；factions 写 `CAdman` 布局；ui 存广告人状态；本系统调用 `quote()`。

`CAdman` 配置与广场呈现都落在 `config/town/plaza.yaml`（`adman:` 段；状态仍是世界存档块，不是设施菜单）。

## 6. 解耦后的配置

`config/town/plaza.yaml` — 广场呈现与触发。

```yaml
schema: dk4.town.plaza
version: 1

clicks:
  - { click_id: 1, key: leave,          command: LeavePlaza }
  - { click_id: 2, key: seek_support,   command: "Actions.run(town.plaza.seek_support)", visible: can_seek_support }
  - { click_id: 3, key: distribute,     command: DistributeGoods, visible: can_distribute }
  - { click_id: 4, key: shop_sign_0,    command: HearFashionIntel, slot: 0 }
  - { click_id: 5, key: shop_sign_1,    command: HearFashionIntel, slot: 1 }
  - { click_id: 6, key: shop_sign_2,    command: HearFashionIntel, slot: 2 }
  - { click_id: 7, key: shop_sign_3,    command: HearFashionIntel, slot: 3 }
  - { click_id: 8, key: shop_sign_4,    command: HearFashionIntel, slot: 4 }
  - { click_id: 10, key: unknown_0a,    command: null }    # 原版 0xA [未知]

signs:
  slot_min: 1
  slot_max: 5
  # 坐标属原版包像素，ui 可覆盖；逻辑只认 slot
  layout_vanilla:
    y: 433                    # 0x1B1
    x: [538, 218, 698, 858, 378]   # anShopX

crowd:
  chance: null                # [未知]
  message_i18n: plaza.crowd.gather
  outcomes:                   # 原版集合未还原；空 = 只播文案
    - { key: minigame, via: Minigames.run, source: plaza_crowd }    # 模组挂钩；原版无，原版小游戏只由剧本启动
    - { key: adman,    via: PresentAdman }
    - { key: fashion_npc, via: PresentPerson, person_id: 89 }   # 阿科布；不写死为唯一广告人
    - { key: talk,     via: PresentRumor }

rumors:                       # 广场听闻目录。计略谣言不进这张表
  - { key: price_drop,    i18n: plaza.rumor.price_drop,    args: [town_id, trading_item_id] }
  - { key: force_loss,    i18n: plaza.rumor.force_loss,    args: [force_id] }
  - { key: hoarding,      i18n: plaza.rumor.hoarding,      args: [town_id, trading_item_id] }
  - { key: bad_buy,       i18n: plaza.rumor.bad_buy,       args: [trading_item_id] }
  - { key: got_intel,     i18n: plaza.rumor.got_intel,     args: [label] }
  - { key: remains_hint,  i18n: plaza.rumor.remains_hint,  args: [item_id, town_id, dir] }
  - { key: fashion_here,  i18n: plaza.fashion.here,        args: [trading_item_id] }
  - { key: fashion_there, i18n: plaza.fashion.there,       args: [town_id, trading_item_id] }
  - { key: scheme_slander, i18n: plaza.rumor.scheme,       args: [force_id, town_id] }  # 只读 schemes

price_intel:
  prompt_i18n: plaza.price_intel.prompt
  pick_town_i18n: plaza.price_intel.pick_town
  # 费、杀价、面板：economy。这里只声明会发的命令
  command: OpenPriceIntel
  fee: { owner: economy, key: price_intel_fee }

seek_support:                 # D9 Action；guard / roll / cost 见 §4，数值在 formulas/town.yaml
  action: town.plaza.seek_support
  outcome_a: { msg: [mes.0x341, mes.0x8f], effect: { command: Share.add, delta: 1, reason: support } }   # [确认] fn_1400122f6(town, 玩家势力, 1,1,1)，份额 +1
  outcome_b: { msg: [mes.0x342], effect: null }             # fn_140012a21 / fn_140004435；字段 [未知]

distribute_goods:
  action: town.plaza.distribute_goods
  guard_post: vanilla:missionary
  msg: mes.0x2ee
  exp_to_holder: 50             # fn_1400026d5(p, 50, 1) [推断] 经验
```

`config/town/plaza.yaml` 的 `adman:` 段 — 世界单例定义（状态仍进存档）。

```yaml
# plaza.yaml 续（adman 段内容，缩进省略）

# 原版无 eAdman。内容包只声明哨兵与文案槽
instance_count: 1
person: vanilla:person_089    # 文案主语；原版写死在九句消息里
active_if_item_not_null: true
# 原版哨兵（+0x08=0x62 无港、+0x09=0x6A 空货）只在 ids/legacy_codec.yaml；运行时 town / item 为 null

messages:                     # aeAdmanMessage 九槽 = exe 消息号（2.5 已解出中文）
  located:   [mes.0x343, mes.0x344, mes.0x345]   # 广告人在某港（%s）
  wandering: [mes.0x346, mes.0x347, mes.0x348]   # 去向不明
  here:      [mes.0x371, mes.0x372, mes.0x373]   # 刚在本港
  # 三组内选哪句的规则在 fn_140168770 [未知]

# 谁可以写这个单例。默认叙事 + 品牌发放，不写死 person 89
writers:
  - narrative.AdmanSet
  - economy.BrandPromoted     # 发放品牌商品时 [推断]
```

存档状态（`12-ui-save/02-save-session.md` 一节，不进上面两份定义表）：

```yaml
adman:
  town: null                  # 原版 0x62
  item: null                  # 原版 0x6A
plaza_session:                # 可不进存档
  signs: []                   # 入港时按公式重建
```

`config/formulas/town.yaml` 增补（与 `01-facilities.md` 已有键并存，不另起文件）：

```yaml
# 下列键并入既有 dk4.formulas.town
shop_count:                     # 招牌数 = 1 + count(develop >= t)，夹 [min, max]
  impl: vanilla.shop_count
  params:
    shop_count_min: 1
    shop_count_max: 5
  tables:
    shop_develop_thresholds: [2000, 3000, 5000, 7000]
  inputs: [develop]             # Towns.develop(town_id, commerce) [推断 商业轨]

can_seek_support:               # fn_140166580；含 [未知] 布尔 fn_14001423b，控制流重
  impl: vanilla.can_seek_support
  params:
    support_skill_min: 100
    support_special_type: 6
    support_rate_min: 100
    support_other_max: 100
  refs:
    support_post: vanilla:purser          # 原版岗位 3
    support_skill: vanilla:accounting     # 原版 eSkill 4

seek_support_roll:              # rand(1000) < th
  impl: expr
  int_mode: vanilla_i32         # /10 为 cdq; idiv 0xA
  params:
    support_roll_den: 1000
    support_rate_mul: 10
    support_influence_div: 10   # 玩家在本港大海域的影响力 /10
  inputs: [kind_value, accounting, influence]   # town_kind_value / purser 的 accounting / Factions.influence
  rng_stream: town.plaza
  expr: "rand(support_roll_den) < (kind_value - 100) * support_rate_mul + accounting + influence / support_influence_div"

seek_support_effects:           # 结果 A / B / 空 与走日
  impl: vanilla.seek_support_effects
  params:
    support_influence_b_min: 1000
    support_share_gain: 1       # 结果 A 份额增量 [确认]（fn_1400122f6 第三参）
    support_days_cost: 1

plaza_crowd:
  impl: null
  unverified: true
  params:
    plaza_crowd_chance: null    # 已有；仍未知

price_intel_fee:                # 金额属经济公式包；本键只作跨表声明
  impl: null
  unverified: true
  params: { price_intel_fee: null }
```

`config/caps.yaml` 片段：

```yaml
plaza_shop_slots: 5
adman_count: 1
```

文案进 `config/ui/strings.zh-CN.yaml`，key 如 `plaza.crowd.gather`、`plaza.fashion.intel`、`plaza.price_intel.prompt`、`plaza.adman.0`。禁止把中文写进 `plaza.yaml`。

招牌抽选函数未还原前，`sign_pick: { unverified: true }`。启动校验：`unverified` 公式不得被内容包当成已实现。

## 7. 独立旋钮

改这些不应迫使其他系统改代码：

- 招牌阈值、最少/最多块数
- 点击号 → 命令
- 人群是否派发小游戏 / 广告人 / 阿科布
- 广场听闻谣言种类增减
- `aeAdmanMessage` 九句换成别的 key
- 广告人对应人物（`adman.person`）与九句消息 key
- 是否显示「寻求支持」「发放交易品」

改这些走边界，但仍不改贸易/战斗代码：

- 点招牌读哪两格流行 → 只动 `Fashion.current`
- 行情费、杀价、`CPriceInformationView` 列 → economy
- 散发交易品扣货、改市价 → economy `DistributeGoods`
- 阿科布出现条件 → narrative
- 行人 10 槽动画 → ui

## 8. 对外接口

查询（只读，只传 ID）：

```
Plaza.signs(town_id) -> [{ slot, trading_item_id, person_id }]
Plaza.shopCount(town_id) -> int
Plaza.canSeekSupport(town_id, fleet_id) -> bool
Plaza.canDistribute(town_id, fleet_id) -> bool
Adman.get() -> { town_id, item_id } | inactive
Adman.present(town_id) -> bool
Plaza.rumors(town_id) -> [{ kind, args }]     # 含 schemes 的只读投影
```

命令：

```
LeavePlaza { }
HearFashionIntel { town_id, slot }
  → ui.play(Fashion.current + plaza.fashion.*)
Actions.run(town.plaza.seek_support, { town_id, fleet_id })   # D9 Action；结果 A = Share.add +1 [确认]，B 效果未还原
DistributeGoods { town_id, fleet_id, item_id } → economy
OpenPriceIntel { from_town_id, to_town_id }  → economy
Adman.set { town_id, item_id }               # narrative / 品牌
Adman.clear { }
PresentRumor { town_id, kind, args }
LaunchCrowd { town_id }                      # 内部再 emit
```

事件：

| 事件 | 载荷 | 谁关心 |
| --- | --- | --- |
| `PlazaOpened` | `{ town_id, fleet_id }` | ui、narrative |
| `PlazaCrowdGathered` | `{ town_id }` | minigames、narrative、ui |
| `FashionIntelHeard` | `{ town_id, slot, fashion_id }` | ui、factions（只读） |
| `PriceIntelRequested` | `{ from_town_id, to_town_id, fee }` | economy |
| `AdmanChanged` | `{ town_id, item_id }` | ui、plaza |
| `PlazaRumorHeard` | `{ town_id, kind }` | ui、narrative |
| `GoodsDistributed` | `{ town_id, item_id }` | economy 已处理后回抛 |

不暴露 `CAdman*`、`CPlazaView*`、`CPriceInformationView*`、`0x2d80` 布局。

`OpenPriceIntel` 的金币结算合同在 economy：本系统在事件里带 `fee` 快照，经济拒绝则发 `PriceIntelRejected`，广场不打开面板。

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| `CPlazaView` 点击 `0xA` | `fn_140003d37` | 「情报按钮」或「人群」 |
| 点击 4–8 → 槽 0–4 | 已确认 `id-4`（`0x1401685a2`） | `signs[id]` |
| `+0x08 == town_id` | 初值 `0x62`、与 `town.none` 同形 | 确认字段名 `town` |
| `+0x08` 与 `person_id` 98 | 同值 98 | 广告人就是欧洲男店主 |
| `aeAdmanMessage` 三组内选句规则 | 九句已按消息号表解出（2.5） | 按 mesfile 偏移取句 |
| `fn_1401696e0` 抽选 | 读流行 2 槽与日历 | 确认公式 |
| 发展度标量是商业轨 | 阈值 2000…7000 | 确认 `nCommerce` |
| 人群触发率 | 文案有，cpp 无立即数 | `chance: 1.0` 或「每次入港」 |
| 人群打开哪些小游戏 | 原版只由剧本启动 | 写死 coin/water |
| `fn_14000b53c` 的 `0x33/0x34` | 二分类型 | mesfile 下标 |
| 「寻求支持」结果 B | A 已确认份额 +1（`fn_1400122f6(town, 玩家势力, 1, 1, 1)`）；B `fn_140004435` 改什么 | 投资 / 合约 |
| 支持判定里两个港口量 | `fn_1400095ed`、`fn_1400088f0`（第三个 `fn_1400023ab` 已知是玩家在本港大海域的影响力） | 确认是份额 / 友好 / 发展度 |
| `fn_140166580` 的类型 `6` | 立即数；岗位 3 = 会计长、`eSkill` 4 = 会计术已钉死 | 确认名字 |
| 行情费 | 文案「缴费一次」 | 任何金币默认值 |
| 行情杀价是否会计长 | 通融句与酒馆/交易所同型 | 确认走某修正器 key |
| 行情入口点击号 | 无独立 `vfunc_13` 分支 | `click_id: 10` |
| 听闻谣言是否解锁行情 | 「得到了情报」 | 免缴费缓存 |
| 阿科布写入 `CAdman` | 九句消息主语都是阿科布；无赋值点 | 代码里 `person_id == 89` |
| `CPlazaSylphid` 10 槽 | 状态 0–4 | 人群事件状态机 |
| `CPlazaView+0xd8` 精灵 | 广告人在场时创建 | 确认类名 |
| `CPriceInformationView` 11 行 | 循环 `>= 0xB` | 「11 种特产品」或「11 港」 |
| `fn_14000e732(0x12c)` | 与人群文案偏移相同 | 确认就是播那句（其它 View 的 `+0x12c` 是布局常数） |

`CAdman` 除 `+0x08/+0x09` 以外若有填充字节，一律 `unknown_bytes`。世界表 hex 与本对象无关：广告人不是 DLL 行。

## 附：mesfile 块偏移 → exe 消息号

按 D10，文案 key 用 exe 消息号，不用 mesfile 字节偏移。正文里 `0x` 开头、落在 mesfile 的数多为 `catalog/mesfile_sc.txt` 的 NUL 块偏移；一块常含多条消息。下表由脚本按 exe 消息号表（`.data 0x140683620` 37 块 `{first_id,size}` + `0x1406836c0` 块内偏移；`.data` 文件偏移 = VA − `0x140001600`）切句，再按正文引号句匹配生成。只给一条的是已匹配到句；给区间的需导出时按句再选。正文里已标「消息」的号（如 `0x58a`、`0xb2e`）本来就是消息号，不在此表。

| 块偏移 | 消息号 | 首句 |
| --- | --- | --- |
| `0x12c` | `0x0` | 提督。好像广场上聚集了一群人。 |
| `0x217` | `0x8` | 提督。好像广场上聚集了一群人。 |
| `0x257` | `0xa` | 谢谢。今后还请多关照。 |
| `0x27b` | `0xb–0xd`（块内多条，按句选） | 真没办法，再给你算公道点儿。 |
| `0x2ef` | `0x13` | 查阅市场行情每看一次缴费一次。要看哪个城市的市场 |
| `0x35f` | `0x16` | 金额方面能不能再给我们公道一些呢？ |
| `0x383` | `0x18` | 金额方面能不能再帮帮忙。 |
| `0x1143` | `0x8e` | 请选择零售店的招牌，就可以听取流行情报。 |
| `0x3ad7` | `0x190` | 这传闻没有被相信。 |
| `0x5f13` | `0x2da` | 我们是零售店。 |
| `0x6fa3` | `0x330` | 真的吗？谢谢！啊，我要把新的流行趋势带到世界的所 |
| `0x728b` | `0x343–0x346`（块内多条，按句选） | 阿科布啊，好像在%s。 |
| `0x72ef` | `0x347`（块内多条，按句选） | 阿科布为了寻找传播新流行的城市，去流浪了。 |
| `0x8653` | `0x3ed` | 这城市好像正在流行%s，也许是发大财的机会呢？ |
| `0x86fb` | `0x3ef` | 喂，你听我说，有大新闻，听说%s正流行%s呢。 |
| `0x8727` | `0x3f0` | 听说%s正流行%s，颇受好评。 |
| `0x912f` | `0x44b` | 想在哪个城市散布谣言？ |
| `0x919b` | `0x44f` | 在哪个城市散布谣言好呢？ |
| `0xbadf` | `0x575–0x57a`（块内多条，按句选） | 请随意。 |
| `0xc1fb` | `0x5c8`（块内多条，按句选） | 如果有传教士，能够消除水手的不满。在广场上还能散 |
| `0x150af` | `0xb23` | 好像%s要在%s制造谣言，没关系，解决方法万无一 |
| `0x16ce7` | `0xc22` | 要调查各港口的市场行情。 |
| `0x16d33` | `0xc26` | 市场行情吗……嗯，不十分了解。 |
