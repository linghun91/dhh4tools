# 旅馆、教会、王宫、公会：设施内部规则

入口层见 [01-facilities.md](01-facilities.md)：这座港开哪些门、点击翻译成哪条命令。本篇管门**里面**的会话：旅馆单晚 / 长住（1 金 1 天一晚）并恢复身心、教会 / 遗址 / 城门的首访与按日期的仪式、王宫三槽合约与谒见投资 / 投降交涉 / 花信、公会委托看板。酒馆酒价 5/10/20 也收在这里（`CBar.cpp` 本身没有价表）。

委托 kind 与生成规则、计略、教会辨认棋规仍分属 [02-requests-schemes.md](02-requests-schemes.md)、[04-schemes-rhetoric.md](04-schemes-rhetoric.md)、[03-minigames.md](03-minigames.md)。本系统只编排设施会话，不重写那几张表。

配置落点：`config/town/facility_rules.yaml`。

## 1. 职责边界

本系统管：

- 设施内部**动作树**（稳定 `action_key`，不是原版枚举）
- 旅馆：每晚金币、可住天数区间、休息命令的参数（夜数）
- 教会 / 遗址 / 城门（原版同一 `CMpRemains` 入口）：首访（发影响力命令）、仪式日判定、确认、奖励子类型、走日
- 王宫 / 总督府：三槽合约会话、谒见投资、势力投降/归顺交涉、介绍信/推荐信当场花掉、珍品陈列、品牌奉献
- 公会：委托看板的列出 / 接 / 交 / 弃，以及停战入口
- 酒馆酒价（自饮 5/10、请客 10/20，按高档旗）——设施会话层持有，扣款发给 economy

本系统不管：

| 事项 | 归属 |
| --- | --- |
| 这座港有没有这家设施、广场点击号 | `01-facilities.md` / `facilities.yaml` |
| 委托 14 kind / 69+1 槽、计略 10 kind / 24 槽的规则与完成判定 | `02-requests-schemes.md` / `04-schemes-rhetoric.md` |
| 教会辨认小游戏的棋子 / 标准解（仪式时限不是小游戏规则，归本篇 2.3） | `03-minigames.md` |
| 身体/精神/疲劳的存储与派生 | characters（`Characters.apply_rest`） |
| 合约槽里的份额数字、独占、上缴公式 | factions |
| 投资扣款、账本 `investment`、发展度增量 | economy |
| 介绍信/推荐信的获得（卖流行品） | economy → factions / items |
| 日历怎么走一天 | world |
| 文案字符串 | ui |

原则：本系统报价、问夜数、问是否亮信、问看板点了哪条；**加减金币 / 份额 / 身心**一律发命令。禁止在旅馆场景里写 `person.body = 0`。

## 2. 原版事实

### 2.1 设施对象仍是空壳

`[确认]` `CInn` `CPalace` `CGuild` `CBar` 的还原 cpp 只有 ctor / dtor，没有菜单。来源：`srcdata/source/game/town/CInn.cpp` `CPalace.cpp` `CGuild.cpp` `CBar.cpp`。

| 类 | ctor 写入 | dtor 释放尺寸 | 含义 |
| --- | --- | --- | --- |
| `CInn` | 仅 vftable | `0x10` | 无字段 |
| `CPalace` | 仅 vftable | `0x10` | 无字段 |
| `CGuild` | `+0x1c = 0` | `0x20` | `[未知]` 这一字节 |
| `CBar` | `+0x14 = 0`，`+0x18 = 5` | `0x20` | `+0x10` 高档旗（入口写）；`+0x18=5` 喝酒时重写，语义 `[未知]` |

`[确认]` 没有 `CChurch` 类。`CTownScene::vfunc_02`（body `0x14026d160`）跳表 `0x14026d92c`：kind 9 遗址 / 10 教会 / 11 城门 三项都构造 `CMpRemains` 并调 `fn_14000f803(kind)`；教会是 `CMpRemains` 的一个分支（`01-facilities.md` 2.1a）。

`[确认]` `CChurchGameView`（ctor body `0x1400759b0` 大小 `0x7E8`；`+0x100=0x358`）**不由教会热点构造**。唯一构造路径：事件脚本解释器 `fn_1400020fe`（RTC `sScript`）→ `fn_14000508d` → 小游戏分派 `fn_14000847c`（body `0x1400e1970`）→ `fn_140005c13` 取脚本指令字低位作游戏号（不是随机数）→ 跳表 `0x1400e1f18` 第 5 项 → `fn_14000dd32`（body `0x140076a70`）→ ctor。`12-ui-save/01-scenes-flow.md` 把 `church_game` 的 parent 标成 `facility` 与此不符。

真正的内部规则在各设施入口函数（`01-facilities.md` 2.1a 表）、mesfile、`CTown` 三槽、`CRequestElement`、`CInvestmentPadView`。

### 2.2 旅馆：1 金 / 夜，恢复身体与精神

`[确认]` SC mesfile：

| offset | 文案 |
| --- | --- |
| `0xba97` | 「要１枚金币。好的，您请休息吧。」 |
| `0xbab7` | 「住１晚１枚金币，要住几天？」 |
| `0xbad3` | 「住%s天吗？」 |

`[确认]` 旅馆入口 `fn_14000b8e3`（body `0x140160270`，`fn_14015b600.cpp`）。`CInn.cpp` 本身**没有**这些数。菜单 3 项（全局 `DAT_14429a634`）：

| 项 | 函数 | 规则 |
| --- | --- | --- |
| 0 住一晚 | `fn_14000295a`（body `0x140160400`） | 金币 `< 1` → 拒（消息 `0x583`「钱不够啦。」）；确认后 `fn_14000af74(this, 1)` 一次 |
| 1 长住 | `fn_14000c9eb`（body `0x140160b70`） | 子菜单 4 项：**3 / 10 / 30 晚** / 取消。先要求金币 `>= 晚数`，再循环 `晚数` 次 `fn_14000af74(this, 0)`；每晚后若有事件（`fn_14000438b(&0x144294500,1)`）且玩家确认（消息 `0x588`「哎？即将出发？」），提前结束 |
| 2 离开 | — | |

`[确认]` 每晚 `fn_14000af74(this, single)`（body `0x140160ae0`）依次：同伴恢复 `fn_1400044cb`、水手疲劳 `fn_14000efde`、`fn_14001117b(gold, 1)`（金币加 `-1`）、`fn_14000b122(…, 1)`（走 1 天 = 循环 `0x30` 次 tick）。即 **1 金 / 晚、1 天 / 晚**，夜数不是自由输入。

`[确认]` 同伴恢复 `fn_1400044cb`（body `0x1401605f0`，RTC `aeCompanion`），对每名同伴（`0xEC` 跳过）：

- 数值：若 `cur != max`，`cur = min(cur + max/20, max)`（`idiv 0x14`；`min` 是 `fn_140011bd0`）。`[推断]` 是体力类数值，getter 名未还原
- 人物字节 `+0x0D`（getter `fn_14000f0fb`，setter `fn_14000ab0f`）：`1..2 → 0`；`3 → 2` 概率 1/3；`4 → 3` 概率 1/3；`5 → 0`：长住必然，单晚概率 1/3
- 人物字节 `+0x0E`（getter `fn_14000c4a5`，setter `fn_14000342c`）：`5 → 4` 必然；`3 → 2`、`4 → 3`：单晚 `rand%2==0`（1/2），长住 `rand%3>0`（2/3）

`[确认]` 水手疲劳 `fn_14000efde`（body `0x140160520`）：值 `>0` 时减 `min(d, 值)`，单晚 `d = rand%3 + 4`（4–6），长住 `d = rand%5 + 5`（5–9）。

`[推断]` 每晚还会叠加一次在港日步恢复：`fn_14000b122(…,1)` 跑满 48 拍，日首拍 `CFleetElement::vfunc_04` 经 `fn_14000641f` 对玩家舰队调 `fn_140005a15`（`0x1400ff991`–`0x1400ffa2a`），其在港分支 `d = -(rand(5)+2)`（`06-ocean/01-sailing.md` §2）。即每晚疲劳共降约 6–15（单晚）/ 7–18（长住）。未核：玩家舰队在港时 `vfunc+0x20` 门是否为真。

`[确认]` `+0x0D` = 身体态、`+0x0E` = 精神态（`03-characters/01-person-model.md` §2：世界行拷贝写 `+0x0D=0` 健康、`+0x0E=3` 普通，`vfunc_04` 以 `+0x0D == 6` 判死亡）。

`[确认]` 入店事件：本次进港标志为 0 时，`rand%3==0` 且 `fn_14000d14d(4)`、`fn_140160dc0(4)` 成立 → 标志置 1。`fn_140160dc0` 扫 `eItem < 0xE7`（231 = `NUM_ITEM`）收集满足两项条件的物品，随机挑一件，由人物 `0x57` 或 `0x58` 说消息 `0x589`「提督！我想试着找出%s。好像在%s。」。`[推断]` 老板娘透露宝物情报，每次进港至多一次。

`[确认]` 身心是人物状态，不是设施字段。exe 连续身体名：健康、轻伤、重伤、濒死、疾病、死亡；连续精神名：高亢、轻松、不高兴、混乱（`03-abilities-skills-jobs.md`，exe `0x57eb78`）。`PHYSICALLEVEL_*` / `MENTALLEVEL_*` / `FATIGUE_*` 约束在 `UCHAR`（`asserts.txt`）。`NUM_PHYSICALSTATE` / `NUM_MENTALSTATE` 头文件无整数。

`[确认]` 文案「在这里船员身体和精神都能够恢复。」`0xc0b3` 后面紧跟船木工 / 船医 / 大厨 / 饲养员 / 传教士 / 参谋说明（`0xc0b3`–`0xc237`），是**舱室**说明，不是旅馆句。旅馆效果以上面的代码为准。

`[确认]` 职能 NPC：旅馆老板娘 7 人，`person_id` 156–162，按文化圈（欧洲 / 非洲 / 阿拉伯 / 东南亚 / 中国 / 日本 / 中南美）。来源：`15-catalogs/persons.md`。只提供立绘与文案，不参与技能。

`[未知]` 长住中断事件 `fn_14000438b` 的具体条件；同伴数值的原名。夜数上限 = 30（菜单最大项），日历每晚推进 1 天，均已 `[确认]`。

### 2.3 遗址 / 教会 / 城门：首访与仪式日

> 旧稿把教会写成「休息 / 小游戏 / 捐献」三岔。代码不支持：三类热点都走 `CMpRemains`，没有休息，也不构造 `CChurchGameView`。

#### 入口：首访 vs 重访

`[确认]` `fn_14000f803(obj, scene, kind)`：`remain_id = STown[当前港]+0x26`，画 `Remains.dk4[remain_id]`。遗迹位 0 未置（`fn_14000c6f3()==0`）走首访 `fn_14000c4d2`，否则走仪式 `fn_14000eacf`。详见 `01-world/11-remains-puzzle-system.md` 2.11。

`[确认]` 首访 `fn_14000c4d2`（body `0x140173ee0`）：消息 `0x129`「发现了%s！」；置遗迹位 0；给玩家势力加七大海域影响力，表 `.data 0x140686650`（`int[3][4]`，列 = 本港 `major_sea` 分组）：遗址 3/5/10/15，教会 1/1/1/1，城门 5/10/15/20；`fn_14000f27c`（body `0x1401eefc0`）取表值、`game+0x7c==0` 时 ×2、加给玩家势力并返回该值；回到 `fn_14000c4d2` 后若 `fn_140010f46()==0` 再 `shl 1`（旧稿未解的 `<<1`），经 `fn_14000ae7a` **第二次**加到同一势力（上限 9999）；消息 `0x12a`「势力值上升了%s！」显示第二次的数。`fn_140010f46` 谓词 `[未知]`。重构后首访由 world 处理（`01-world/11` `on_enter_town_map_point`），影响力属 factions；本篇只订阅 `RemainsRevisited` 跑仪式。

#### 重访：按日期的仪式

`[确认]` `fn_14000eacf`（body `0x140173d80`，RTC `nMonth,nDay,nTime`）读日历，算 `day1 = nDay + 1`、`target = (fn_14000e68d() + 1) % 28 + 1`（`cdq; idiv 0x1c`，取余数），再按 kind 分派（`cmp 9/0xa/0xb`）。`[推断]` `fn_14000e68d` 返回当前 `town_id`（`CTownScene::vfunc_02` 开头同样调用并拿去查港口），所以**每港每月第 `(town_id+1)%28+1` 天**有仪式。

| kind | 分支 | 条件 | 问句 / 未开放 | 成功后 | 走日 |
| --- | --- | --- | --- | --- | --- |
| 9 遗址 | `fn_140008e9a`（body `0x140174900`） | `day1 == target` 且确认 | `0x58b`「哦？你是来参加仪式的人吗？」/ `0x45`「没事请马上离开。」 | 按 `(day1+1)` 奇偶：偶 → 跪拜（`0x58d` `0x58e` `0x591`）`fn_14000a8ee(this,0)`；奇 → 帮忙准备（`0x594` `0x59c`「喂，喂，别偷懒！」）`(this,1)` | 1 天 |
| 10 教会 | `fn_140002171`（body `0x140174230`） | `day1 == target` 且确认 | `0x58a`「今天是喜庆的日子，来，一起祈祷吧。」/ `0x59f`「神明时时刻刻关注着你的行为。」 | `0x58c`「来，唱一首欢快的歌吧。」；`fn_14000a8ee(this,2)` | 1 天 |
| 11 城门 | `fn_14000e840`（body `0x140174360`） | `target <= day1 < target + 3` 且确认 | `0x5a0`「你是来参加%s的仪式的吧？」（`%s` 取 `fn_140002450(town)`）/ `0x5a2`「来遗迹这里有事吗？如果没有，还是赶快离开吧。」 | 按 `(day1+1) % 5` 五选一，见下表 | **4 天** |

`[确认]` 走日：三支末尾都调 `fn_14000b122(…, n)`，遗址 `edx=1`（`0x140174ada`）、教会 `edx=1`（`0x14017430a`）、城门 `edx=4`（`0x1401747bb`）。城门不论哪个 variant 都走 4 天。旧稿城门 `days_cost: 1 [推断]` 错。

城门五个 variant（成功消息首句 → 奖励子类型）`[确认]`：

| `(day1+1)%5` | 消息 | 子类型 |
| --- | --- | --- |
| 0 | `0x5a3`「啊……！！！」`0x5a6`（不知所云） | 3 |
| 1 | `0x5a7`「是来巡礼的吧，站到那边，按顺序祈祷。」 | 4 |
| 2 | `0x5a8`「那好，一起跳舞好了。」`0x5ab` `0x5ac` | 5 |
| 3 | `0x5ad`「现在开始绝食，可以吗？」`0x5b5`「那么，请安安静静地祈祷吧。」 | 6 |
| 4 | `0x5bd`「为取圣水要登上灵峰，可以吗？」`0x5be` `0x5bf`「太阳下山前不回来就会被猛兽吃了」 | 7 |

上表消息号按 exe 消息号表（`.data 0x140683620` / `0x1406836c0`，见 `11-progression/01-level-exp-skills.md`）解出。旧稿按 mesfile 块偏移 `0xbcd3`–`0xc0b3` 给的 key 与分支对应有误（如「巡礼 / 跳舞 / 绝食 / 圣水」都属城门，不属教会）。

`[确认]` 奖励函数 `fn_14000a8ee`（body `0x1401754e0`，RTC `ePerson`）：选一名人物，按子类型 0–7（`<= 7` 跳表）给奖励，多处 `rand%5` / `rand%3` 决定大小档（如 `0x190` vs `0x32`、`0xc8` vs `0x64`、`0x1f4` vs `0x64`），大小档值经 `fn_140006e88`（= `fn_140011c34(p, 0, n, f)`，第 1 轨经验，见 04 篇）或 `fn_1400026d5` 发放；`fn_140011c34` RTC `nUpAbility,aeAbility`。`[推断]` 仪式奖励 = 执行人经验（如 400 vs 50、200 vs 100、500 vs 100），各子类型具体分支未逐一展开。

`[确认]` 「那么，请安安静静地祈祷吧。」（`0x5b5`）是城门绝食仪式里的一句，不是休息菜单。教会**没有**休息动作。

#### 教会小游戏不在热点上

`[确认]` `CChurchGameView` 是独立 View，不挂在教会热点上（见 2.1）。ctor：`+0xb0 = 5`，`+0xb4 = 0x11`（17）。规则（17 选 5 辨认）见 `03-minigames.md` 2.7。原版入口只有事件脚本指令 `0xC1` 号 5，本篇不发小游戏命令。

#### 捐献 / 花金币

SC mesfile **没有**「捐」「布施」「献金」作为教会专词。

`[确认]` 「需要%s枚金币，好像不够。」（消息 `0x5c0`）「要花掉%s枚金币，可以吗？」（`0x5c1`）是**参数化**通用句。调用点：`fn_1400048a9`（body `0x1400a1bb0`，两句都用）、`fn_140007531`（body `0x140175f60`，也是可变天数的 `fn_14000b122` 调用者）、`fn_140004138`、`~CChangeFormationView`。三支仪式代码均无扣金调用。原版包不开 `donate`。

职能 NPC `[确认]`：神父 155（欧洲）、阿訇 148/151、高僧 150/152/153、祈祷师 149/154。按港口文化选人，与设施对象无指针。

### 2.4 王宫 / 总督府

`CPalace.cpp` 无逻辑。会话全是文案 + `CTown` 三槽 + 投资垫板。

#### 合约，每港 3 槽

`[确认]` `CTown` 存档循环 `s24 >= 3`，每槽两字节（`+0x18/+0x19`）。初值由 `CTown::vfunc_05` 从 `GetFixTownProc` 记录拷入，源值 `<=0` 时写 `0x1A, 0`（`vfunc_04` 只是转调，不扫槽）。`[确认]` 槽 = (force_id, share)（`01-world/02-towns.md` §2；DLL `+0x4C`）：广场 `fn_140166580` 按 `eTownShare < 3` 累加份额与 100 比较；`0x1A` = `NUM_FORCE + 1` 空值。旧「流行槽」读法作废。`eTownShare >= 3` 的 UI/逻辑循环见 `02-factions/03-relations-influence.md`。

`[确认]` 宫殿文案（`0xb207`–`0xb53b`）：

- 最低合约费 `%s` 枚
- 「根据你支付的合约金来承认占有率，你要支付多少钱？」
- 钱不够拒签
- 已有独占约：「无法再与其他势力签合约」
- 三槽占满：「已经与很多势力签了合约，没有其他势力的份儿了」

合约金 → 份额已由 economy 还原：最低合约费 `floor((商业 + 军事) / 4)`，`share_gain = floor(gold_paid / min_fee)`，金额对半写两轨发展度（`04-economy/07-investment-tribute.md` 4.1–4.3）。本系统只收集 `gold_paid` 交给 `Invest.pay(…, reason: contract)`，返回的 `share_gain` 交给 factions 的 `Contract.sign`（内部调 `Share.add(reason: contract)`）；`InvestmentMade` 不带份额增量，任何订阅方不得再加一次（词表 §14.21）。

#### 谒见投资

`[确认]` 无占有率则交易所问「要投资吗……？真是奇怪的人，那么要投资多少呢？」（`0x7943` `0x7a63`）。投资垫板是 `CInvestmentPadView`（`trade/CInvestmentPadView.cpp`），ctor 一堆矩形，`+0xb4=0xffffffff`，**没有**金额公式。

`[确认]` `01-facilities.md` 把宫殿 commands 写成 `open_contract, open_investment, donate_brand`。本篇把 `open_investment` 定为王宫谒见的一条菜单：会话在宫殿，扣款与账本在 economy。交易所无份额时的「要投资吗」是**另一入口、同一命令**。

投资 → 发展度 / 「稍微优待」份额：economy 已还原（`04-economy/07-investment-tribute.md` 4.2–4.4、F-ECO-11）。本系统只调 `Invest.pay(town_id, force_id, gold, reason)`；`InvestmentMade` 由 economy 发。

#### 投降 / 归顺交涉

`[确认]` 与合约同一文案带（`0xb0ff`–`0xb207`）：

- 「%s是不会加入我们麾下的，可以吗？」
- 「我看%s是绝对不会投降的，这样也可以吗？」
- 「我不认为%s会进入我们的伞下。」

这是**势力级**归顺/投降问句，不是海战白旗。海战「城市投降需要舰长和副官」在 `0x18463`，属 combat。

`[未知]` 成功概率、是否必须先宣战、与友好度/协议旗的关系。本系统只发 `SurrenderTalk { town_id, target_force_id }` 给 factions。

#### 花介绍信 / 推荐信

`[确认]` 信在交易所卖流行品达到件数门槛后发放（`0x7be3`–`0x7d47`，`trade_stack.md`）。宫殿当场用：

- 「什么？你说带着交易所的推荐信，拿来我看看。」`0xb3d3`
- 「你说带来了交易所的介绍信？拿来看看。」`0xb3ff`
- 「拿着这个到小城市的王宫或者总督府去」——介绍信仅小城市；推荐信任何城市 `0x7d47`
- 「嗯，光凭介绍信有点困难，如果是推荐信的话还可以考虑……我答应让你在这个城市扩大合约的比例。」`0xb69f`

`[确认]` `letter_state` 运行时值：`bd98(s550)` `2`=无信，`0`=有介绍信、`1`=推荐信（`13-algorithms/01-formulas.md` F-ECO-06；`02-factions/04-letters-licenses.md` §2.4 宫殿菜单槽 2/3 的亮灯条件；`[推断]` 枚举名）。信**不是** `eItem` 0–230 里的一件。

本系统：查询信旗 → 亮菜单 → 确认后 `Letters.spendAtPalace(holder_id, town_id, force_id)`（factions）。增量 `[确认]`（`02-factions/04` §2.4）：己方份额 `<1` 加 1；否则推荐信 +10、介绍信 +5，再与剩余空间取 `min`；介绍信在首都被拒且不消耗（§2.5：小城市 = `is_capital == 0`）。

#### 珍品陈列与品牌奉献

`[确认]` 「在我的居城陈列你的珍品」「若是这件，每月支付 %s 枚金币」`0xb58f` `0xb5db`。出租费 `floor(item_price / 50)`（`TreasureRent.quote(item_id)`，`04-economy/07` 4.7），月末结算归 economy。

`[确认]` 「通过将品牌商品的%s奉献给城市的掌权人」`0xb75b` 一带。品牌判定属 economy（份额 ≥100 且槽 `grade_rank==1`）。本系统只发 `DonateBrand { town_id, trading_item_id }`。

`[确认]` 「交了金币%s枚……。」`0x11e7` 紧跟「与%s的友好程度提高了。」`0x11cf`。F-ECO-12 把它标成宫殿上缴/进贡占位，金额曲线无 default。

职能 NPC `[确认]`：国王 / 地方领主 / 藩主 / 县令 / 大名 / 总督（`person_id` 141–147），王宫 / 总督府由该港热点位域第 6 / 7 位决定（`01-facilities.md` 2.4；数据上互斥），不是首都标记。

### 2.5 公会：委托看板

`CGuild.cpp` 无看板逻辑。`+0x1c=0` 语义 `[未知]`。

`[确认]` 世界对象持有整张委托表：`CWorld` ctor `+0x19f48` 构造 `CRequestElement`（`world/CWorld.cpp` 调 `fn_1402a64d0`）。计略表在 `+0x182a0`，入口是酒馆不是公会。

`[确认]` `CRequestElement`：ctor `stride=0x18, count=0x46`（70）；`vfunc_04` 循环 `eRequest >= 0x45`（69）。`CRequest` kind 字节 `+0x09`：`<0` 或 `>=0x0E` 跳过，`==0x0F` 视为空。来源：`world/CRequest.cpp` `CRequestElement.cpp`。目录与 14 种模板见 `02-requests-schemes.md`。

`[确认]` 看板 UI：`CRequestInformationView`（`game/CRequestInformationView.cpp`），ctor `+0xb4=0x47`（71）。71 与 69 的差 `[未知]`。

`[确认]` 公会是交付点：「希望把此文书交给%s的同业公会。」消息 `0xb2e`；道具类同样送到同业公会 `0xb35`。放弃：「要交出违约金%s枚金币。」`0xb2c`。「钱不够呀。」`0xb2d`。（消息号按 exe 消息号表解；旧稿是 mesfile 块偏移 `0x15267` 等。）

`[确认]` 停战入口：「前去“同业公会”要求停战吗？」`0x18463`。计略在酒馆（参谋，`0xc237`），不要塞进公会菜单。

职能 NPC `[确认]`：行会总管 4 人，`person_id` 163–166（欧洲 / 阿拉伯 / 中国 / 日本）。

`[推断]` 同时只能持有 1 条：下标 `0x45`（第 70 槽）是玩家当前委托槽，接受时把公告槽 0x18 字节拷进去再清原槽（`fn_14015b600`，`0x14015bf8a`–`0x14015c0ce`；详见 `02-requests-schemes.md` §2.1）。看板按港过滤条件 `[未知]`。

### 2.6 酒馆酒价：自饮 5/10、请客 10/20（入口 `fn_1400035cb`，不在 `CBar.cpp`）

`[确认]` `CBar.cpp` 只写 `+0x14=0`、`+0x18=5`。**没有** 5/10/20 立即数。

`[确认]` mesfile：

- `0x733`「需要１０枚金币。5枚金币。」（同一记录里还有「用%s枚金币来补充吗？欢迎。想要点儿什么？」）
- `0x7bf`「喂，请再喝一杯。收您２０枚金币。10枚金币。」

`[确认]` 选档已还原。酒馆入口 `fn_1400035cb`（body `0x1401529d0`，`fn_14014c350.cpp`）进门时算「高档」旗 `CBar+0x10`：

```
high = (fn_1400023ab(player, fn_140014e48(town)) >= 3000) AND (develop(town) >= 3000)   # 影响力(玩家, 本港大海域)；develop 同广场招牌用的 fn_14000bc03
```

高档用消息 `0x37`「欢迎。请进。」，普通 `0x38`「欢迎。想要点儿什么？」。菜单 5 项：0–3 各一处理函数，4 离开。

`[确认]` 喝酒 `fn_14000aa3d`（body `0x14014ed20`）：先写 `CBar+0x18 = 5`；自己喝 **高档 10 / 普通 5**，钱不够说 `0x3ca`/`0x3cb`（「没有免费白喝的酒。」）；扣款 `fn_14001117b(gold, price)`。随后 `fn_14000448f(town_id)`（`eBarmaid` 循环 `< 0x15`）找本港女招待，有且金币 `>= 20` 才问请她喝，确认后 **高档 20 / 普通 10**（消息 `0x41`「收您２０枚金币。」/`0x42`「10枚金币。」）。所以 5/10/20 是「自己 × 档」+「请客 × 档」，不是三选一。

`[推断]` `CBar+0x18=5` 每次喝酒都重写，不是同屏人数；含义未知。

`[确认]` `fn_1400023ab(fn_140012bfc(), a)` 读 `CForce+0x10+a*2`，即玩家势力在大海域 `a` 的影响力 word（`02-requests-schemes.md` 2.5）；`fn_140014e48(town)` = 本港 `major_sea_id`（`01-world/08-sea-area-music-icons.md`）。所以高档 = 玩家在本港大海域影响力 ≥ 3000 且发展度 ≥ 3000。06 篇「寻求支持」用的是同一个量。

划拳、招募、计略仍走 01 / 02 / 03，本表只收 `buy_drink`。

## 3. 实体与 ID

原版无 `eFacilityAction`。重构用命名空间 `action_key`（D1），运行时空值是 `null`，不用整数哨兵。每个动作都是 D9 通用 `Action`（见 4.0）。

| 实体 | 原版 | 计数 | 空 | 备注 |
| --- | --- | --- | --- | --- |
| 设施 | 无 `eFacility` | 12 个热点 key → 9 个处理器 | — | key 与 `01-facilities.md` 对齐 |
| 内部动作 | 无 | 本表 | `null` | 不要用 0 当「没选」 |
| 合约槽 | `CTown` 每港 3 | `share_slots_per_town=3` | 原版写 `0x1A`/`0` `[推断]` 为空 force | 运行时 `force: null`；`0x1A` 只在 `ids/legacy_codec.yaml` |
| 信件状态 | `bd98` 立即数 | 至少 none / intro | none=`2` `[推断]` | 推荐信第三态 `[未知]` |
| 委托实例 | `eRequest` | 69 | kind `0x0F` | 目录不在本表 |
| 教会小游戏 | `CChurchGameView` | 1 个 View | — | `minigame: church`；事件脚本指令 `0xC1` 号 5，不属教会菜单 |
| 仪式奖励子类型 | `fn_14000a8ee` 参数 | 8（0–7） | — | 遗址 0/1、教会 2、城门 3–7 |
| 仪式日 | `(town_id+1)%28+1` | 每港 1 天 / 月（城门 3 天窗口） | — | `fn_14000eacf` |
| 首访影响力 | `.data 0x140686650` `int[3][4]` | 3 种 × 4 列 | — | `fn_14000f27c` |
| 身心 | `ePhysicalState` / `eMentalState` | 名 6 / 4；整型 `[未知]` | — | 人物系统存 |

设施 key 复用 01。本篇用到的内部动作（原版包；锚点里的 `0x` 小号是消息号，`0x5xxx` 以上是 mesfile 块偏移，待导出脚本换成消息号）：

| action_key | facility | 原版锚点 |
| --- | --- | --- |
| `inn.rest_single` | inn | `fn_14000295a`；`0xba97` |
| `inn.rest_long` | inn | `fn_14000c9eb`；`0xbab7`；3/10/30 |
| `rite.first_visit` | ruins / church / gate | `fn_14000c4d2`；消息 `0x129` `0x12a` |
| `church.rite` | church | `fn_140002171`；子类型 2；消息 `0x58a` |
| `ruins.rite` | ruins | `fn_140008e9a`；子类型 0/1；消息 `0x58b` |
| `gate.rite` | gate | `fn_14000e840`；子类型 3–7；消息 `0x5a0` |
| `palace.contract` | palace | `0xb2f3` 三槽 |
| `palace.invest` | palace | 谒见 + `CInvestmentPadView` |
| `palace.surrender_talk` | palace | `0xb1df` |
| `palace.spend_letter` | palace | `0xb3d3` `0xb69f` |
| `palace.display_treasure` | palace | `0xb5db` |
| `palace.donate_brand` | palace | `0xb75b` |
| `palace.tribute` | palace | `0x11e7` |
| `guild.list_requests` | guild | `CRequestInformationView` |
| `guild.deliver_request` | guild | 消息 `0xb2e`+kind |
| `guild.abandon_request` | guild | 消息 `0xb2c` |
| `guild.ask_ceasefire` | guild | `0x18463` |
| `bar.buy_drink` | bar | `0x733` `0x7bf` |

会话状态：`current_action_key: null`，不要拿 `inn.rest_single` 的下标 0 当空。

## 4. 算法 / 公式

全部命名参数进 `config/town/facility_rules.yaml`（动作树）与 `config/formulas/town.yaml`（数字）。禁止场景里写 `gold -= 1`。

### 4.0 通用动作（D9）

旅馆、仪式、寻求支持（06 篇）、海上 happening、叙事命令共用一个执行器：

```
Action { cost{gold, days}, guard[predicate], roll: formula_ref, effects[command], repeat{n, interruptible} }
```

- `cost.days` → `Clock.advance_days(n)`。原版 1 天 = 48 tick，`fn_14000b122` 逐拍跑 48·n 次完整世界调度（`01-world/07-calendar-and-coords.md` 2.4）；是否逐拍由 `add_days_mode` 决定，本系统不管。旅馆每晚推进日首时，tick 的 `daily` phase 照常触发其**在港分支**（原版 `fn_140005a15` 在港只做水手疲劳恢复，归 fleets；人物两步跳过），与本系统的 `Characters.apply_rest` 是两笔，不互相替代（词表 §14.18、`03-characters/07`）。
- `repeat` 每轮都重新扣 `cost` 并跑 `effects`；`interruptible: true` 时每轮后问 `Clock.interrupt_pending()`，玩家确认即停。
- 时间相关 guard 只用 `Clock` 查询（`day_of_month`），不读 `SCalendar` 字段。

### 4.1 旅馆

```
inn.rest_single: Action
  cost:    { gold: inn.gold_per_night, days: inn.days_per_night }     # 1, 1
  guard:   [ has_gold(inn.gold_per_night) ]                           # 否则消息 0x583
  effects: [ Characters.apply_rest { fleet_id, source: inn_single } ]
  repeat:  { n: 1 }

inn.rest_long(nights ∈ inn.long_stay_options): Action                 # vanilla [3, 10, 30]
  cost:    { gold: inn.gold_per_night, days: inn.days_per_night }     # 每晚扣一次，不是一次扣
  guard:   [ has_gold(inn.gold_per_night * nights) ]                  # 长住先整体校验
  effects: [ Characters.apply_rest { fleet_id, source: inn_long } ]
  repeat:  { n: nights, interruptible: true }                         # 中途事件 + 确认（0x588）则停
then emit InnRested { town_id, mode, nights_done, gold_paid }
```

characters 的 `rest.tables.inn_single` / `rest.tables.inn_long`（`formulas/vitals.yaml`，原版值）：

```
stat_regen      = min(cur + max / stat_regen_div, max)   # div = 20
physical (+0x0D) : {1,2}→0 必然; 3→2 p; 4→3 p; 5→0 p5    # single p=1/3, p5=1/3; long p=1/3, p5=1
mental   (+0x0E) : 5→4 必然; 3→2 q; 4→3 q                # single q=1/2; long q=2/3
sailor_fatigue -= min(fatigue, rand(span) + base)        # single 4 + rand%3; long 5 + rand%5
```

本系统只传 `source`（`inn_single` / `inn_long`）；概率与数值属 characters（`formulas/vitals.yaml` 的 `rest`，`03-characters/07`）。

### 4.2 教会 / 遗址 / 城门：首访与仪式

```
# 首访归 world：进 kind 9/10/11 时 world 判遗迹位（原版 fn_14000c6f3），未发现则
# World.mark_remains_discovered + 两次 Influence.add（01-world/11 on_enter_town_map_point）；本系统不重复
on RemainsRevisited { remains_id, town_id, kind }:      # world 发；原版已发现 → fn_14000eacf
  run site(kind).rite

<site>.rite: Action
  guard:   [ rite_day_open(town_id, site) , player.confirms(site.msg_ask) ]
           # rite_day_open = target <= Clock.day_of_month() < target + site.window_days
           # target = formula rite_day(town_id)；原版 (town_id + 1) % 28 + 1
  roll:    rite_variant         # 原版 site.variants[(day1 + 1) % len]，确定性，不掷骰
  effects: [ Rites.grant { town_id, reward_type: variant.reward_type } ]   # 原版 fn_14000a8ee
  cost:    { days: site.days_cost }                    # 遗址 1、教会 1、城门 4
```

原版参数：`rite_day_offset=1`、`rite_day_mod=28`；遗址 `window_days=1`、2 个 variant（奖励 0/1）、走 1 天；教会 `window_days=1`、1 个 variant（奖励 2）、走 1 天；城门 `window_days=3`、5 个 variant（奖励 3–7）、走 4 天。`rite_day(town_id)` 依赖港口整数顺序，模组新增港时内容包应能按港显式填 `rite_day`，公式只作缺省。奖励表 `rewards[0..7]` 的字段 `[未知]`，只知含 `rand%5` / `rand%3` 大小档和能力成长调用。

`CChurchGameView+0xb0` → `option_count`（默认 5）；`+0xb4` → `select_button_count`（默认 17）。这是小游戏参数，进 `minigames.yaml`，不属教会设施。

### 4.3 王宫

```
palace.contract:
  slots = Share.slots(town_id)                 # 3
  if Exclusive(town_id) and not our_slot: reject_exclusive
  if no empty slot and not our_slot: reject_full
  fee_min = Invest.min_fee(town_id, contract)  # floor((商业+军事)/4)，04-economy/07 4.1
  ask gold_paid >= fee_min
  { share_gain } = Invest.pay(town_id, force_id, gold_paid, reason: contract)
  Contract.sign(town_id, force_id, share_gain)   # factions 执行，内部 Share.add(reason: contract)；合约份额只此一条路（词表 §14.21）

palace.invest:
  Invest.pay(town_id, force_id, gold, reason: military)   # 垫板确认后；王宫谒见投资 = 军事加投资（04-economy/07 4.2）
  # economy 收金额、记账、emit InvestmentMade（不带 share_gain）；已有份额时的优待份额由 economy 调 Share.add(reason: investment)

palace.surrender_talk:
  emit SurrenderTalk { town_id, target_force_id }
  # factions 判定；本系统不掷骰

palace.spend_letter:
  if letter(holder) == null: hide
  Letters.spendAtPalace(holder_id, town_id, force_id)   # factions：小城市校验、+1 / +5 / +10、消耗信

palace.display_treasure:
  item_id = pick unique item
  emit TreasureDisplayed { town_id, item_id, rent_per_month }  # rent = TreasureRent.quote(item_id)，economy

palace.donate_brand:
  item_id in Economy.list_brands(force_id)
  emit BrandOffered { town_id, item_id }

palace.tribute:
  gold = Tribute.pending or player input       # 公式 null
  emit TributePaid { town_id, gold }
```

小城市 `[确认]` = 非首都（`is_capital == 0`，`fn_14000c71b` 读 `STown+0x24`；`02-factions/04-letters-licenses.md` §2.5），判定与增量都在 factions `letters.yaml`（`small_city.predicate: not_capital`），本系统不判。**禁止**代码猜 `develop < 1000`。

### 4.4 公会看板

```
guild.list_requests:
  ids = Requests.listAt(town_id)               # 过滤未还原则返回全表非空 kind
  show CRequestInformationView

guild.accept(request_id):
  Requests.AcceptRequest { request_id, town_id }

guild.deliver(request_id):
  Requests.DeliverRequest { request_id }

guild.abandon(request_id):
  gold = request.penalty_gold
  Requests.AbandonRequest { request_id }

guild.ask_ceasefire:
  Factions.ceasefire_offer { from, to }        # 成功式 [未知]
```

同时可接条数 `max_active_requests`：原版包填 1（`0x45` 单槽，`[推断]`），引擎读表，不写死。

### 4.5 酒馆酒

```
on BarEntered:
  tier = high if Factions.influence(player_force, MajorSea.of_town(town)) >= bar.high_tier.influence_min
                 and develop(town) >= bar.high_tier.develop_min else normal
bar.buy_drink:
  price = bar.drink_price[tier]                # normal 5 / high 10
  if gold < price: reject
  debit price
  if barmaid(town_id) and gold >= bar.treat_gate:          # 20
     if confirms: debit bar.treat_price[tier]               # normal 10 / high 20
  emit BarDrinkPaid { town_id, gold_paid, tier }
```

## 5. 耦合分析

原版：

- 旅馆直接改人物 `+0x0D/+0x0E`、扣金、走日；仪式日公式写死 `%28`；仪式文案与舱室说明挤在同一 mesfile 块
- 宫殿直接改 `CTown` 三槽份额、读信件状态、打开 `CInvestmentPadView`
- 公会直接扫 `CWorld+0x19f48` 的 69 条
- 酒价与补给句挤在 `0x733`
- `NUM_REQUEST=69`、合约槽 3、教会 `+0xb0=5` 写死在 ctor
- `CTownScene` 宿主名单没有教会对象

重构后只允许：

```
town-life/facility_rules
  → Characters.apply_rest（经 Actions.run）
  → Force.add_gold / Invest.pay / Invest.min_fee / TreasureRent.quote / list_brands
  → Contract.sign / Letters.spendAtPalace / ceasefire_offer / SurrenderTalk
  → Requests.*（首访影响力与遗迹位归 world，本系统只订阅 RemainsRevisited）
  → Clock.advance_days(n)           # 只经 Action.cost.days
```

禁止：本表写市价、写六维、写 `share += gold/N`、给教会编捐献 / 休息效果。

`01-facilities.md` 的 `inn_gold_per_night` / `bar_drink_price` / `bar_treat_price` 与本表对应键必须相等；加载时校验，只保留一份运行时值（本表为内部规则源，01 的 `formulas/town.yaml` 作公式解释器别名，构建管线拷贝）。D3 形状下公式侧多一层：`facility_rules.inn.gold_per_night` ↔ `formulas/town.yaml` 的 `inn_cost.params.inn_gold_per_night`；`bar_drink_price` ↔ `bar_drink_cost.tables.bar_drink_price`；`bar_treat_price` ↔ `bar_treat_cost.tables.bar_treat_price`；`long_stay_options` ↔ `inn_cost.tables.inn_long_stay_options`。校验按「公式名.params|tables.键」路径比对，不按裸键名。

## 6. 解耦后的配置

`config/town/facility_rules.yaml` — 本系统唯一内部规则表。

```yaml
schema: dk4.town.facility_rules
version: 1

# 与 01-facilities / formulas/town.yaml 对齐（公式侧路径带 <公式名>.params. / .tables. 一层，见上）
inn:
  gold_per_night: 1                # 每晚扣一次
  days_per_night: 1
  single: { nights: 1 }
  long_stay_options: [3, 10, 30]   # fn_14000c9eb 子菜单
  long_stay_interruptible: true
  restore: { single: inn_single, long: inn_long }   # characters rest 源 key；量在 formulas/vitals.yaml
  entry_rumor:                     # 入店宝物情报
    chance_den: 3                  # rand%3 == 0
    once_per_visit: true
  npc_role: innkeeper              # person 表按 culture 选 156–162

rite_calendar:
  formula: rite_day                # 缺省 (town_id + day_offset) % day_mod + 1；内容包可按港覆盖
  day_offset: 1
  day_mod: 28
  per_town_override: {}            # town key → 日；原版为空

rite_sites:                        # 原版 CMpRemains kind 9/10/11；键 = 01 的设施 key
  vanilla:remains:
    window_days: 1
    days_cost: 1
    msg_ask: mes.0x58b
    msg_closed: mes.0x45
    variants: [{reward_type: 0}, {reward_type: 1}]
  vanilla:church:
    window_days: 1
    days_cost: 1
    msg_ask: mes.0x58a
    msg_closed: mes.0x59f
    variants: [{reward_type: 2}]
    npc_role: clergy               # 148–155
  vanilla:gate:
    window_days: 3
    days_cost: 4                   # [确认] fn_14000b122(…,4)
    msg_ask: mes.0x5a0
    msg_closed: mes.0x5a2
    variants: [{reward_type: 3}, {reward_type: 4}, {reward_type: 5}, {reward_type: 6}, {reward_type: 7}]

# 奖励子类型 0–7 的内容 [未知]；放 config/town/rite_rewards.yaml，由 characters 执行
rite_rewards: null

# 首访（fn_14000c4d2）归 world：数值表与 apply_twice 在 01-world/11 的 influence_reward，本表不存

palace:
  original_class: CPalace
  # 称号按设施 key（palace / governor），见 01 的 facilities.yaml；不按首都标记
  contract:
    slots: 3
    fee_min: { owner: economy, call: "Invest.min_fee(town_id, contract)" }     # floor((商业+军事)/4)
    share_from_gold: { owner: economy, formula: "floor(gold_paid / min_fee)" }
  invest:
    pad: CInvestmentPadView
    source_tag: palace             # 与交易所入口区分
  surrender_talk:
    enabled: true
    formula: null
  letters:
    intro_small_town_only: true
    owner: factions                # letters.yaml：small_city.predicate: not_capital；增量 +1 / +5 / +10
  treasure_display:
    rent_per_month: { owner: economy, call: "TreasureRent.quote(item_id)" }   # floor(item_price / 50)
  donate_brand: true
  tribute:
    formula: null                  # 0x11e7
  npc_role: ruler                  # 141–147

guild:
  original_class: CGuild
  board:
    view: CRequestInformationView
    view_b4: 0x47                  # 71；语义未核实
    generation: request_generation # 见 02 篇
    max_active: 1                  # 原版 0x45 单槽 [推断]
  ceasefire: true
  schemes: false                   # 计略在酒馆
  npc_role: guildmaster            # 163–166

bar:
  original_class: CBar
  high_tier:
    influence_min: 3000            # 玩家势力在本港大海域的影响力（fn_1400023ab）
    develop_min: 3000
  drink_price: { normal: 5, high: 10 }
  treat_price: { normal: 10, high: 20 }
  treat_gold_gate: 20              # 原版写死 0x14，与 treat_price 独立
  # CBar+0x18=5 每次喝酒重写，语义 [未知]，不入表
  # recruit / scheme / rps 入口仍在 facilities.yaml
```

`config/formulas/town.yaml` 增段（与 01 已有键合并，不另发明）：

```yaml
schema: dk4.formulas.town
version: 1

inn_cost:                          # inn_cost = inn_gold_per_night * nights（01 篇 §4）
  impl: expr
  params: { inn_gold_per_night: 1 }
  tables: { inn_long_stay_options: [3, 10, 30] }
  inputs: [nights]
  expr: "inn_gold_per_night * nights"

bar_drink_cost:                    # bar_drink_cost = bar_drink_price[tier]
  impl: expr
  tables: { bar_drink_price: { normal: 5, high: 10 } }
  inputs: [tier]
  expr: "bar_drink_price[tier]"

bar_treat_cost:
  impl: expr
  tables: { bar_treat_price: { normal: 10, high: 20 } }
  inputs: [tier]
  expr: "bar_treat_price[tier]"

palace_contract:
  impl: vanilla.palace_contract
  params: { palace_contract_slots: 3 }
```

`config/caps.yaml`：

```yaml
share_slots_per_town: 3            # 已在 factions 文档
```

身心恢复曲线不在本文件。characters 的 `config/formulas/vitals.yaml`（规范形状见 `03-characters/07` §6 `rest:`；下面只是原版转移表对照）：

```yaml
rest:                                        # D3 形状：数值在 rest.tables.<source>；规范列名见 03-characters/07 §6
  tables:                                    # stat_regen = max/20（规范表 hp_frac: 1/20），封顶 max
    inn_single:
      physical: { "1": {to: 0, p: 1}, "2": {to: 0, p: 1}, "3": {to: 2, p: 1/3}, "4": {to: 3, p: 1/3}, "5": {to: 0, p: 1/3} }
      mental: { "5": {to: 4, p: 1}, "3": {to: 2, p: 1/2}, "4": {to: 3, p: 1/2} }
      sailor_fatigue: { base: 4, rand: 3 }   # 4–6
    inn_long:
      physical: { "1": {to: 0, p: 1}, "2": {to: 0, p: 1}, "3": {to: 2, p: 1/3}, "4": {to: 3, p: 1/3}, "5": {to: 0, p: 1} }
      mental: { "5": {to: 4, p: 1}, "3": {to: 2, p: 2/3}, "4": {to: 3, p: 2/3} }
      sailor_fatigue: { base: 5, rand: 5 }   # 5–9
```

`physical` / `mental` = 人物 `+0x0D` / `+0x0E`（`03-characters/01` 已钉死）。教会无休息条目。

## 7. 独立旋钮

改这些不应迫使经济 / 战斗 / 人物公式改代码：

- 每晚金币、夜数上下限
- 仪式日公式参数、各仪式点窗口天数与 variant 列表
- `option_count` / `select_button_count`
- 仪式列表
- 合约槽数（与 factions `share_slots_per_town` 一起改表，不改战斗）
- 介绍信是否仅小城市（factions `letters.yaml` 的 `small_city`）
- 酒价表与选档器
- 公会是否提供停战；同时可接委托数
- 首都「王宫」/ 非首都「总督府」文案 key

会碰到边界、但仍不改外系统代码：

- 休息恢复量 → 只动 characters `rest`（`formulas/vitals.yaml`）
- 合约金→份额 → 只动 economy `investment.yaml`（已还原）
- 投资金额曲线 → economy `investment.yaml`

## 8. 对外接口

事件：

```
InnRested { town_id, mode, nights_done, gold_paid }
RiteJoined { town_id, site, reward_type }
PalaceContractOpened { town_id }
SurrenderTalk { town_id, target_force_id }
LetterSpent { force_id, kind, town_id }
TreasureDisplayed { town_id, item_id }
BrandOffered { town_id, trading_item_id }
TributePaid { town_id, gold }
GuildBoardOpened { town_id }
BarDrinkPaid { town_id, gold_paid, tier }
```

查询：

```
FacilityRules.actions(facility_key, town_id) -> [action_key]
FacilityRules.innQuote(mode, nights) -> gold
FacilityRules.drinkPrices() -> [gold]
FacilityRules.riteOpen(town_id, site) -> { first_visit, open, variant }   # 日期取 Clock
FacilityRules.canSpendLetter(town_id, force_id) -> { allowed, need_kind }
```

命令（本系统收点击，转发时只带 ID 与标量）：

```
Actions.run(inn.rest_single | inn.rest_long, { town_id, fleet_id, nights })
Actions.run(<site>.rite, { town_id, site })   # 遗址 / 教会 / 城门仪式
SignContract { town_id, gold_paid }          # → Invest.pay(contract) + Contract.sign
Invest.pay(town_id, force_id, gold, reason)  # 王宫 military / 交易所 trade；InvestmentMade 由 economy 发
StartSurrenderTalk { town_id, target_force_id }
SpendLetter { town_id, kind }
DisplayTreasure { town_id, item_id }
DonateBrand { town_id, trading_item_id }
PayTribute { town_id, gold }
ListRequests { town_id }
DeliverRequest { request_id }
AbandonRequest { request_id }
AskCeasefire { town_id, target_force_id }
BuyDrink { town_id, tier }
```

存档：设施定义不存。可变状态：

- 进行中的长住（`repeat` 剩余晚数；原版在同一帧内跑完，无需存）
- 已陈列珍品 `item_id`（若确认进 `CTown`）
- `letter_state` 不在本系统存（factions / 玩家状态）
- 当前已接委托在 requests 存档

「今晚是否已休息」原版无此状态。遗迹已发现位归 world / remains 存档。

## 9. 未核实字段

- `CInn`/`CPalace` 无字段；菜单状态机在 unassigned，未挂到类上。
- `CGuild+0x1c`、`CBar+0x14`。
- 消息 `0x5c2`「在这里船员身体和精神都能够恢复」对应哪个舱室（`03-cabins-crew-posts.md` `rest_room`）；已排除旅馆 / 教会。
- 旅馆：被 `/20` 恢复的数值原名；在港日步是否每晚叠加（玩家舰队 `vfunc+0x20` 门）；长住中断条件 `fn_14000438b`；入店情报两项物品条件。
- `fn_140007531`（body `0x140175f60`，用 `0x5c0`、可变走日）是哪个设施动作。
- `alIdentify` 136 字节内容；17 个 `CSelectPushButton` 里几个是选项、几个是框。
- 仪式奖励子类型 0–7 的具体字段与数额。首访支线已还原（2.3）；`fn_140010f46` 谓词仍 `[未知]`。
- `fn_14000e68d` 是否就是当前 `town_id`（仪式日公式依赖它）。
- 独占：单槽 100 还是仅一槽非空。
- （已结）介绍信「小城市」= 非首都，见 `02-factions/04` §2.5。
- 推荐信的 `letter_state` 第三值；花信后份额增量。
- 珍品月租；`0x11e7` 是否必走宫殿。
- 投降交涉成功率。
- 投资垫板从宫殿打开还是仅交易所；两者是否共用 `CInvestmentPadView`。
- 看板按港过滤的条件。
- `CRequestInformationView+0xb4=0x47`。
- 酒馆菜单项 1–2（`fn_14000aadd` / `fn_14000736a`）各是什么；项 3 `fn_1400110bd` 是计谋写槽（04 篇 2.6）。高档判定量已还原（2.6）。
- `CDatePadView`（生日/日期垫板，`trade/`）是否也用于旅馆夜数；ctor `+0xbc=5` 与酒价/夜数无关的像素立即数居多，未核实前不要当夜数上限。
- `CLetterSelectView` 是 CJK 选字（姓名），**不是**介绍信 UI。
- 休息是否清疾病/死亡；船医治病与旅馆休息是否互斥。

## 附：mesfile 块偏移 → exe 消息号

按 D10，文案 key 用 exe 消息号，不用 mesfile 字节偏移。正文里 `0x` 开头、落在 mesfile 的数多为 `catalog/mesfile_sc.txt` 的 NUL 块偏移；一块常含多条消息。下表由脚本按 exe 消息号表（`.data 0x140683620` 37 块 `{first_id,size}` + `0x1406836c0` 块内偏移；`.data` 文件偏移 = VA − `0x140001600`）切句，再按正文引号句匹配生成。只给一条的是已匹配到句；给区间的需导出时按句再选。正文里已标「消息」的号（如 `0x58a`、`0xb2e`）本来就是消息号，不在此表。

| 块偏移 | 消息号 | 首句 |
| --- | --- | --- |
| `0x5ab` | `0x2a`（块内多条，按句选） | 提督，这样下去物资储备就没了。 |
| `0x733` | `0x36–0x3a`（块内多条：补充句、`0x39` 需要１０枚金币…） | 用%s枚金币来补充吗？ |
| `0x7bf` | `0x41` | 收您２０枚金币。 |
| `0x11cf` | `0x92` | 与%s的友好程度提高了。 |
| `0x11e7` | `0x93` | 交了金币%s枚……。 |
| `0x7943` | `0x38a` | 没有占有率，交易也不能进行，要投资吗……？真是奇 |
| `0x7a63` | `0x38b` | 要投资吗？与别人是有些不同，那么投资多少呢？ |
| `0x7be3` | `0x393–0x397`（块内多条，按句选） | 战争如果不早日结束，生意很难做。 |
| `0x7d47` | `0x39b` | 拿着这个到小城市的王宫或者总督府去，占有率的事会 |
| `0xb0ff` | `0x53a–0x53b`（块内多条，按句选） | %s是不会加入我们麾下的，可以吗？ |
| `0xb1df` | `0x540`（块内多条，按句选） | 我看%s是绝对不会投降的，这样也可以吗？ |
| `0xb207` | `0x541–0x543`（块内多条，按句选） | 我看%s不会加入我们帐下，可以吗？ |
| `0xb2f3` | `0x546–0x549`（块内多条，按句选） | 想在这个城市做生意吗？好吧，这个城市的最低合约费 |
| `0xb3d3` | `0x54a` | 什么？你说带着交易所的推荐信，拿来我看看。 |
| `0xb3ff` | `0x54c` | 什么？你说带来了交易所的介绍信？拿来看看。 |
| `0xb53b` | `0x553`（块内多条，按句选） | 那点钱不够合约费，改天再来吧。 |
| `0xb58f` | `0x555` | 在我的居城陈列你的珍品，要借给我哪个？ |
| `0xb5db` | `0x557` | 是啊，……若是这件，每月支付%s枚金币，可以吗？ |
| `0xb69f` | `0x55f` | 嗯，光凭介绍信有点困难，如果是推荐信的话还可以考 |
| `0xb75b` | `0x562` | 通过将品牌商品的%s奉献给城市的掌权人，交涉一下 |
| `0xba97` | `0x572` | 好的，您请休息吧。 |
| `0xbab7` | `0x573` | 住１晚１枚金币，要住几天？ |
| `0xbad3` | `0x574` | 住%s天吗？ |
| `0xbcd3` | `0x589–0x58a`（块内多条，按句选） | 提督！我想试着找出%s。好像在%s。 |
| `0xc0b3` | `0x5bf–0x5c4`（`0x5bf` 圣水时限、`0x5c2` 身体精神恢复…） | 太阳下山前不回来就会被猛兽吃了 |
| `0xc237` | `0x5c9–0x5ca`（块内多条，按句选） | 如果参谋在，可在酒馆里拟定策略。 |
| `0x15267` | `0xb2e` | 希望把此文书交给%s的同业公会。愿意接受吗？ |
| `0x18463` | `0xd08` | 要想让城市投降，需要舰长和副官。 |
