# 旅馆、教会、王宫、公会：设施内部规则

入口层见 [01-facilities.md](01-facilities.md)：这座港开哪些门、点击翻译成哪条命令。本篇管门**里面**的会话：旅馆按夜收费并恢复身心、教会小游戏 / 休息 / 捐献三岔、王宫三槽合约与谒见投资 / 投降交涉 / 花信、公会委托看板。酒馆酒价 5/10/20 也收在这里（`CBar.cpp` 本身没有价表）。

委托目录、计略目录、教会辨认棋规仍分属 [02-requests-schemes.md](02-requests-schemes.md)、[03-minigames.md](03-minigames.md)。本系统只编排设施会话，不重写那三张目录。

配置落点：`config/town/facility_rules.yaml`。

## 1. 职责边界

本系统管：

- 设施内部**动作树**（稳定 `action_key`，不是原版枚举）
- 旅馆：每晚金币、可住天数区间、休息命令的参数（夜数）
- 教会：小游戏 / 安静祈祷 / 捐献 三条互斥入口，以及仪式文案对应的 `rite_key`
- 王宫 / 总督府：三槽合约会话、谒见投资、势力投降/归顺交涉、介绍信/推荐信当场花掉、珍品陈列、品牌奉献
- 公会：委托看板的列出 / 接 / 交 / 弃，以及停战入口
- 酒馆酒价表（5 / 10 / 20）——设施会话层持有，扣款发给 economy

本系统不管：

| 事项 | 归属 |
| --- | --- |
| 这座港有没有这家设施、广场点击号 | `01-facilities.md` / `facilities.yaml` |
| 69 条委托、24 条计略的 kind 与完成判定 | `02-requests-schemes.md` |
| 教会辨认的棋子/标准解、仪式时限规则 | `03-minigames.md` |
| 身体/精神/疲劳的存储与派生 | characters（`applyRest`） |
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
| `CBar` | `+0x14 = 0`，`+0x18 = 5` | `0x20` | `+0x18=5` `[推断]` 同屏人数，不是酒价 |

`[确认]` 没有 `CChurch` 类。`CTownScene::vfunc_02` 宿主 RTC 是 `cShipyard,cRemains,cGuild,cInn,cPalace,cBar,cPort,cTrade,cPlazaView`（`functions.tsv` body `0x14026d160`），名单里没有教会对象。教会进港后走 `CChurchGameView`（`town/CChurchGameView.cpp`，体积 `0x7E8`，dtor 分配 `0x358`）。场景流把 `church_game` 的 parent 标成 `facility`（`12-ui-save/01-scenes-flow.md`）。

真正的内部规则在 mesfile、`CTown` 三槽、`CRequestElement`、`CInvestmentPadView`、`CChurchGameView` 绘制函数。

### 2.2 旅馆：1 金 / 夜，恢复身体与精神

`[确认]` SC mesfile：

| offset | 文案 |
| --- | --- |
| `0xba97` | 「要１枚金币。好的，您请休息吧。」 |
| `0xbab7` | 「住１晚１枚金币，要住几天？」 |
| `0xbad3` | 「住%s天吗？」 |

`[确认]` 单价写死为 **1 枚 / 晚**，夜数是玩家输入的 `%s`。`CInn.cpp` **没有** 这个 1。

`[确认]` 身心是人物状态，不是设施字段。exe 连续身体名：健康、轻伤、重伤、濒死、疾病、死亡；连续精神名：高亢、轻松、不高兴、混乱（`03-abilities-skills-jobs.md`，exe `0x57eb78`）。`PHYSICALLEVEL_*` / `MENTALLEVEL_*` / `FATIGUE_*` 约束在 `UCHAR`（`asserts.txt`）。`NUM_PHYSICALSTATE` / `NUM_MENTALSTATE` 头文件无整数。

`[确认]` 文案「在这里船员身体和精神都能够恢复。」`0xc0b3`。`01-facilities.md` 与 `04-recruitment-crew.md` 把它当作旅馆休息效果。同块还拼了取圣水时限、`需要%s枚金币`、船木工/船医岗位句（见 2.3），**哪一句绑旅馆**不能从 cpp 再钉一次，见第 9 节。

`[确认]` 职能 NPC：旅馆老板娘 7 人，`person_id` 156–162，按文化圈（欧洲 / 非洲 / 阿拉伯 / 东南亚 / 中国 / 日本 / 中南美）。来源：`15-catalogs/persons.md`。只提供立绘与文案，不参与技能。

`[未知]` 夜数上限；每夜身体/精神/疲劳各恢复多少；是否推进日历；同一晚能否连住两次。

### 2.3 教会：小游戏 vs 休息 vs 金币

三条是设施菜单上的**不同动作**，不要合成一个 `pray`。

#### 休息

`[确认]` 「那么，请安安静静地祈祷吧。」`0xc01f`。无「住几天」、无「1 枚金币」。

`[推断]` 教会休息走与旅馆同一 `Characters.applyRest`，夜数默认 1、金币 0。依据：同设施「船员身体和精神」句 + 小游戏文档已把教会标成休息点。恢复量仍 `[未知]`。

#### 小游戏 / 仪式

`[确认]` `CChurchGameView` 是独立 View，不是空壳设施。ctor：`+0xb0 = 5`，`+0xb4 = 0x11`（17），其余大量坐标立即数（属 ui）。来源：`CChurchGameView.cpp`。

`[确认]` 绘制 `fn_1400066cc`（body `0x140076cf0`，`fn_14006f9e0.cpp`）：

- RTC `alIdentify` 136 字节、`rcMaru`、`rcTarget`
- 资源 `InfoBox2.dk4` `EvItem.dk4` `InfoBox.dk4`
- 循环 `s100 >= *(obj+0xb0)` → 用 `+0xb0=5` 当选项数
- 循环 `s234 >= *(obj+0xb4)` → 用 `+0xb4=17` 造 `CSelectPushButton`（`fn_1400763a0` 即其 ctor）

棋规（辨认、对错圈）属 `03-minigames.md` 的 `minigame_id: church`。本篇只负责 `LaunchMinigame { church, source: facility_church }`。

`[确认]` 仪式文案（可能与 View 不是同一条代码路径）：

| key | 文案 | offset |
| --- | --- | --- |
| `feast_pray` | 「今天是喜庆的日子，来，一起祈祷吧。」 | `0xbcd3` |
| `rite_enter` | 「哦？你是来参加仪式的人吗？」 | `0xbd17` |
| `song` | 「来，唱一首欢快的歌吧。」 | `0xbd33` |
| `pilgrim` | 「是来巡礼的吧，站到那边，按顺序祈祷。」 | `0xbefb` |
| `dance` | 「那好，一起跳舞好了。」 | `0xbefb` |
| `fast` | 「现在开始绝食，可以吗？」 | `0xbfa7` |
| `holy_water` | 「为取圣水要登上灵峰」；「太阳下山前不回来就会被猛兽吃了」 | `0xc06f` `0xc0b3` |

仪式若证实不是 `CChurchGameView`，仍用同一 `LaunchMinigame`，`minigame_id` 换成 `church_rite`（小游戏文档已留口）。

#### 捐献 / 花金币

SC mesfile **没有**「捐」「布施」「献金」作为教会专词。

`[确认]` 与教会休息/圣水打在同一记录里的金币句：`0xc0b3`「需要%s枚金币，好像不够。要花掉%s枚金币，可以吗？」这是**参数化**金额，和旅馆写死的「１枚」不是同一句。

`[推断]` 教会有一条「花 `%s` 金币」的动作（捐献或收费休息）。依据：参数化金额紧挨安静祈祷/圣水；旅馆单价已有独立固定句。效果（友好度、名声、份额）cpp 未还原，标 `[未知]`，禁止编「捐 X 金 → 友好 +Y」。

职能 NPC `[确认]`：神父 155（欧洲）、阿訇 148/151、高僧 150/152/153、祈祷师 149/154。按港口文化选人，与设施对象无指针。

### 2.4 王宫 / 总督府

`CPalace.cpp` 无逻辑。会话全是文案 + `CTown` 三槽 + 投资垫板。

#### 合约，每港 3 槽

`[确认]` `CTown` 存档循环 `s24 >= 3`：每槽两字节，空槽 `force=0x1A`、`share=0`。ctor 同样扫 3 槽写 `0x1A`/`0`。来源：`town/CTown.cpp` `vfunc_05`/`vfunc_06`/`vfunc_04`。`eTownShare >= 3` 的 UI/逻辑循环见 `02-factions/03-relations-influence.md`。

`[确认]` 宫殿文案（`0xb207`–`0xb53b`）：

- 最低合约费 `%s` 枚
- 「根据你支付的合约金来承认占有率，你要支付多少钱？」
- 钱不够拒签
- 已有独占约：「无法再与其他势力签合约」
- 三槽占满：「已经与很多势力签了合约，没有其他势力的份儿了」

合约金 → 份额公式 `[未知]`。本系统只收集 `gold_paid`，交给 `Factions.Contract.sign`。

#### 谒见投资

`[确认]` 无占有率则交易所问「要投资吗……？真是奇怪的人，那么要投资多少呢？」（`0x7943` `0x7a63`）。投资垫板是 `CInvestmentPadView`（`trade/CInvestmentPadView.cpp`），ctor 一堆矩形，`+0xb4=0xffffffff`，**没有**金额公式。

`[确认]` `01-facilities.md` 把宫殿 commands 写成 `open_contract, open_investment, donate_brand`。本篇把 `open_investment` 定为王宫谒见的一条菜单：会话在宫殿，扣款与账本在 economy。交易所无份额时的「要投资吗」是**另一入口、同一命令**。

投资 → 发展度 / 「稍微优待」份额：公式未还原（`04-special-products-investment.md`、F-ECO-11）。本系统 `emit InvestmentAudience { town_id, gold }`。

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

`[确认]` `letter_state` 运行时值：`bd98(s550)` `2`=无信，`0`=有介绍信（`13-algorithms/01-formulas.md` F-ECO-06，`[推断]` 枚举名）。推荐信的第三态未从立即数钉死。信**不是** `eItem` 0–230 里的一件。

本系统：查询 `HasLetter` → 亮菜单 → 确认后 `SpendLetter { kind }` → factions 改份额。放大多少 `[未知]`。

#### 珍品陈列与品牌奉献

`[确认]` 「在我的居城陈列你的珍品」「若是这件，每月支付 %s 枚金币」`0xb58f` `0xb5db`。出租费公式 `[未知]`，月结走 factions/economy。

`[确认]` 「通过将品牌商品的%s奉献给城市的掌权人」`0xb75b` 一带。品牌判定属 economy（份额 ≥100 且槽 `grade_rank==1`）。本系统只发 `DonateBrand { town_id, trading_item_id }`。

`[确认]` 「交了金币%s枚……。」`0x11e7` 紧跟「与%s的友好程度提高了。」`0x11cf`。F-ECO-12 把它标成宫殿上缴/进贡占位，金额曲线无 default。

职能 NPC `[确认]`：国王 / 地方领主 / 藩主 / 县令 / 大名 / 总督（`person_id` 141–147），首都用「王宫」文案、非首都用「总督府」（`01-facilities.md` `name_variants`）。

### 2.5 公会：委托看板

`CGuild.cpp` 无看板逻辑。`+0x1c=0` 语义 `[未知]`。

`[确认]` 世界对象持有整张委托表：`CWorld` ctor `+0x19f48` 构造 `CRequestElement`（`world/CWorld.cpp` 调 `fn_1402a64d0`）。计略表在 `+0x182a0`，入口是酒馆不是公会。

`[确认]` `CRequestElement`：ctor `stride=0x18, count=0x46`（70）；`vfunc_04` 循环 `eRequest >= 0x45`（69）。`CRequest` kind 字节 `+0x09`：`<0` 或 `>=0x0E` 跳过，`==0x0F` 视为空。来源：`world/CRequest.cpp` `CRequestElement.cpp`。目录与 14 种模板见 `02-requests-schemes.md`。

`[确认]` 看板 UI：`CRequestInformationView`（`game/CRequestInformationView.cpp`），ctor `+0xb4=0x47`（71）。71 与 69 的差 `[未知]`。

`[确认]` 公会是交付点：「希望把此文书交给%s的同业公会。」`0x15267`；道具类同样送到同业公会 `0x15383`。放弃：「要交出违约金%s枚金币。」`0x15243`。「钱不够呀。」`0x1525b`。

`[确认]` 停战入口：「前去“同业公会”要求停战吗？」`0x18463`。计略在酒馆（参谋，`0xc237`），不要塞进公会菜单。

职能 NPC `[确认]`：行会总管 4 人，`person_id` 163–166（欧洲 / 阿拉伯 / 中国 / 日本）。

`[未知]` 同时可接几条；看板是否按 `town_id` 过滤、还是扫完全表再按 kind 参数筛。

### 2.6 酒馆酒价：文案 5 / 10 / 20，不在 `CBar.cpp`

`[确认]` `CBar.cpp` 只写 `+0x14=0`、`+0x18=5`。**没有** 5/10/20 立即数。

`[确认]` mesfile：

- `0x733`「需要１０枚金币。5枚金币。」（同一记录里还有「用%s枚金币来补充吗？欢迎。想要点儿什么？」）
- `0x7bf`「喂，请再喝一杯。收您２０枚金币。10枚金币。」

三档 **5 / 10 / 20** 出现。如何选档（第一杯 vs 续杯、人物魅力、随机）cpp 未还原。`+0x18=5` 不要当成酒价；`01-facilities.md` 已推断为同屏最多 5 人（`eBarmaid` 循环 `0x15`=21）。

划拳、招募、计略仍走 01 / 02 / 03，本表只收 `buy_drink`。

## 3. 实体与 ID

原版无 `eFacilityAction`。重构用 `action_key`；整数只在原版包里给对照号，运行时按表长。

| 实体 | 原版 | 计数 | 空 | 备注 |
| --- | --- | --- | --- | --- |
| 设施 | 无 `eFacility` | 内容包 9 个入口 | — | id 与 `01-facilities.md` 对齐 |
| 内部动作 | 无 | 本表 | `invalid` | 不要用 0 当「没选」——0 会占 rest |
| 合约槽 | `CTown` 每港 3 | `share_slots_per_town=3` | `force_id=26`（`0x1A`），`share=0` | 禁止 `force_id=0` 当空 |
| 信件状态 | `bd98` 立即数 | 至少 none / intro | none=`2` `[推断]` | 推荐信第三态 `[未知]` |
| 委托实例 | `eRequest` | 69 | kind `0x0F` | 目录不在本表 |
| 教会小游戏 | `CChurchGameView` | 1 个 View | — | `minigame_id: church` |
| 身心 | `ePhysicalState` / `eMentalState` | 名 6 / 4；整型 `[未知]` | — | 人物系统存 |

设施 id 复用 01 的 0–8。本篇用到的内部动作（原版包）：

| action_key | facility | 原版锚点 |
| --- | --- | --- |
| `inn.rest` | inn | `0xbab7` |
| `church.rest` | church | `0xc01f` |
| `church.minigame` | church | `CChurchGameView` |
| `church.rite` | church | `0xbcd3`–`0xc0b3` |
| `church.donate` | church | `0xc0b3` `%s枚金币` `[推断]` |
| `palace.contract` | palace | `0xb2f3` 三槽 |
| `palace.invest` | palace | 谒见 + `CInvestmentPadView` |
| `palace.surrender_talk` | palace | `0xb1df` |
| `palace.spend_letter` | palace | `0xb3d3` `0xb69f` |
| `palace.display_treasure` | palace | `0xb5db` |
| `palace.donate_brand` | palace | `0xb75b` |
| `palace.tribute` | palace | `0x11e7` |
| `guild.list_requests` | guild | `CRequestInformationView` |
| `guild.deliver_request` | guild | `0x15267` |
| `guild.abandon_request` | guild | `0x15243` |
| `guild.ask_ceasefire` | guild | `0x18463` |
| `bar.buy_drink` | bar | `0x733` `0x7bf` |

会话状态：`current_action_key: invalid`，不要拿 `inn.rest` 的下标 0 当空。

## 4. 算法 / 公式

全部命名参数进 `config/town/facility_rules.yaml`（动作树）与 `config/formulas/town.yaml`（数字）。禁止场景里写 `gold -= 1`。

### 4.1 旅馆

```
nights ∈ [inn_nights_min, inn_nights_max]
inn_cost = inn_gold_per_night * nights          # vanilla: 1 * nights
if player.gold < inn_cost: reject               # 文案「钱不够」
debit inn_cost                                  # economy
emit RestAtInn { town_id, nights, gold_paid }
Characters.applyRest { person_ids, nights, source: inn }
# Calendar.advance { days: nights }             # [未知] 是否走日
```

`inn_gold_per_night = 1`，`inn_nights_min = 1`，`inn_nights_max = null`（上限未还原则拒绝「不限」实现，UI 必须有夹紧值；缺省夹紧进 caps，禁止代码猜 7 或 30）。

`applyRest` 的每夜身体/精神/疲劳增量属 characters，本表只传 `source` 与 `nights`。characters 用 `rest_tables[source]` 查量；原版量 `null` 时**不要**写成「满恢复」。

### 4.2 教会三岔

```
on ChurchOpened:
  menu = church.actions                        # rest | minigame | rite | donate

church.rest:
  gold = church_rest_gold                      # vanilla default 0；[推断]
  nights = church_rest_nights                  # vanilla default 1；[未知]
  Characters.applyRest { source: church, nights }

church.minigame:
  LaunchMinigame { minigame_id: church, source: facility_church }

church.rite(rite_key):
  LaunchMinigame { minigame_id: church_rite, rite: rite_key }
  # 或同一 church 引擎读 rites[]

church.donate(gold):
  gold ∈ [church_donate_min, player.gold]      # min [未知] → 缺省则拒绝启动
  debit gold
  emit ChurchDonated { town_id, gold }
  # 效果表 effect: null，禁止发明友好度
```

`CChurchGameView+0xb0` → `option_count`（默认 5）；`+0xb4` → `select_button_count`（默认 17）。棋规仍在 `minigames.yaml`。

### 4.3 王宫

```
palace.contract:
  slots = Factions.Share.slots(town_id)        # 3
  if Exclusive(town_id) and not our_slot: reject_exclusive
  if no empty slot and not our_slot: reject_full
  fee_min = ContractFeeMin(town_id)            # 公式 null → 内容表或拒绝
  ask gold_paid >= fee_min
  Factions.Contract.sign { town_id, force_id, gold_paid }

palace.invest:
  OpenInvestment { town_id, force_id, source: palace }
  # economy 收金额、记账、emit InvestmentMade

palace.surrender_talk:
  emit SurrenderTalk { town_id, target_force_id }
  # factions 判定；本系统不掷骰

palace.spend_letter:
  st = HasLetter(force_id)
  if st == none: hide
  if st == intro and not IsSmallTown(town_id): reject_need_recommend
  SpendLetter { kind: st }
  Factions.Share.add { reason: letter }        # 增量 null

palace.display_treasure:
  item_id = pick unique item
  emit TreasureDisplayed { town_id, item_id, rent_per_month }  # rent null

palace.donate_brand:
  item_id in Economy.list_brands(force_id)
  emit BrandOffered { town_id, item_id }

palace.tribute:
  gold = Tribute.pending or player input       # 公式 null
  emit TributePaid { town_id, gold }
```

`IsSmallTown`：文案「小城市」。判定（发展度 < K、非首都、…）`[未知]`，配置里 `small_town_rule: null`，未填则介绍信菜单对所有港都走「仅推荐信可用」的保守路径，**禁止**代码猜 `develop < 1000`。

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

同时可接条数 `max_active_requests: null`。null 时引擎按「表说了才限」，不要写死 1。

### 4.5 酒馆酒

```
bar.buy_drink:
  price ∈ bar_drink_prices                     # [5, 10, 20]
  tier = pick_drink_tier(...)                  # [未知] → 必须由表的 picker 给出
  debit price
  emit BarDrinkPaid { town_id, gold_paid, tier }
```

`picker: unknown` 时原版包**不要**默认第一档 5。实现：要么还原选档，要么 UI 让玩家三档里选，禁止静默 `prices[0]`。

## 5. 耦合分析

原版：

- 旅馆直接改人物身心字节；教会 View 与休息对话、岗位说明挤在同一 mesfile 块
- 宫殿直接改 `CTown` 三槽份额、读信件状态、打开 `CInvestmentPadView`
- 公会直接扫 `CWorld+0x19f48` 的 69 条
- 酒价与补给句挤在 `0x733`
- `NUM_REQUEST=69`、合约槽 3、教会 `+0xb0=5` 写死在 ctor
- `CTownScene` 宿主名单没有教会对象

重构后只允许：

```
town-life/facility_rules
  → Characters.applyRest / HasLetter（查询）
  → Economy.debit / OpenInvestment / list_brands
  → Factions.Contract.sign / Share.add / ceasefire_offer / SurrenderTalk
  → Requests.* / Minigames.Launch
  → World.Calendar.advance          # 仅当 nights 确认走日
```

禁止：本表写市价、写六维、写 `share += gold/N`、教会捐献写友好度公式。

`01-facilities.md` 的 `inn_gold_per_night` / `bar_drink_prices` 与本表同名键必须相等；加载时校验，只保留一份运行时值（本表为内部规则源，01 的 `formulas/town.yaml` 作公式解释器别名，构建管线拷贝）。

## 6. 解耦后的配置

`config/town/facility_rules.yaml` — 本系统唯一内部规则表。

```yaml
schema: dk4.town.facility_rules
version: 1

# 与 01-facilities / formulas/town.yaml 同名键对齐
inn:
  gold_per_night: 1
  nights_min: 1
  nights_max: null                 # [未知]；UI 夹紧见 caps.inn_nights_ui_max
  restore:
    source: inn
    targets: [body, mind]          # 疲劳是否一起清：[未知] 不写 true
    amount_per_night: null
  advance_calendar: null           # true/false 未还原
  npc_role: innkeeper              # person 表按 culture 选 156–162

church:
  original_class: CChurchGameView  # 无 CChurch
  hosted_by_town_scene: false
  option_count: 5                  # +0xb0
  select_button_count: 17          # +0xb4
  actions:
    - key: rest
      gold: 0                      # [推断] 安静祈祷免费
      nights: 1
      restore: { source: church, targets: [body, mind], amount_per_night: null }
    - key: minigame
      minigame_id: church
    - key: rite
      rites: [feast_pray, song, pilgrim, dance, fast, holy_water]
    - key: donate
      gold_min: null               # 缺省拒绝启动
      effect: null                 # 禁止填假友好度
  npc_role: clergy                 # 148–155

palace:
  original_class: CPalace
  title_by_capital:
    true: ui.facility.palace
    false: ui.facility.governor
  contract:
    slots: 3
    empty_force_id: 26
    fee_min_formula: null
    share_from_gold: null
  invest:
    pad: CInvestmentPadView
    source_tag: palace             # 与交易所入口区分
  surrender_talk:
    enabled: true
    formula: null
  letters:
    intro_small_town_only: true
    small_town_rule: null          # [未知]
    share_bonus: null
  treasure_display:
    rent_per_month: null
  donate_brand: true
  tribute:
    formula: null                  # 0x11e7
  npc_role: ruler                  # 141–147

guild:
  original_class: CGuild
  board:
    view: CRequestInformationView
    view_b4: 0x47                  # 71；语义未核实
    catalog: requests              # 外键到 requests.yaml
    max_active: null
  ceasefire: true
  schemes: false                   # 计略在酒馆
  npc_role: guildmaster            # 163–166

bar:
  original_class: CBar
  drink_prices: [5, 10, 20]
  drink_tier_picker: unknown       # 禁止 fallback 成 prices[0]
  on_screen_slots: 5               # CBar+0x18；[推断] 不是酒价
  # recruit / scheme / rps 入口仍在 facilities.yaml
```

`config/formulas/town.yaml` 增段（与 01 已有键合并，不另发明）：

```yaml
schema: dk4.formulas.town
version: 1
inn_gold_per_night: 1
inn_nights_min: 1
inn_nights_max: null
church_rest_gold: 0
church_rest_nights: 1
church_donate_min: null
bar_drink_prices: [5, 10, 20]
palace_contract_slots: 3
```

`config/caps.yaml`：

```yaml
inn_nights_ui_max: null            # 仅 UI 夹紧；null 则输入控件必须显式上限
share_slots_per_town: 3            # 已在 factions 文档
```

身心恢复曲线不在本文件。characters 的 `config/formulas/character.yaml`：

```yaml
rest_tables:
  inn:    { body: null, mind: null, fatigue: null }
  church: { body: null, mind: null, fatigue: null }
```

null 表示原版量未还原：命令仍发，人物系统 no-op 或只打事件，禁止用满血当 default。

## 7. 独立旋钮

改这些不应迫使经济 / 战斗 / 人物公式改代码：

- 每晚金币、夜数上下限
- 教会四条动作的开关、捐献下限
- `option_count` / `select_button_count`
- 仪式列表
- 合约槽数（与 factions `share_slots_per_town` 一起改表，不改战斗）
- 介绍信是否仅小城市；小城市规则一旦还原只改 `small_town_rule`
- 酒价表与选档器
- 公会是否提供停战；同时可接委托数
- 首都「王宫」/ 非首都「总督府」文案 key

会碰到边界、但仍不改外系统代码：

- 休息恢复量 → 只动 `rest_tables`
- 合约金→份额 → 只动 factions 公式（本表继续 `null` 直到还原）
- 投资金额曲线 → economy `investment.yaml`

## 8. 对外接口

事件：

```
InnRested { town_id, nights, gold_paid }
ChurchRested { town_id, nights }
ChurchDonated { town_id, gold }
ChurchRiteStarted { town_id, rite_key }
PalaceContractOpened { town_id }
InvestmentAudience { town_id, force_id, gold, source }   # source=palace|exchange
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
FacilityRules.actions(facility_id, town_id) -> [action_key]
FacilityRules.innQuote(nights) -> gold
FacilityRules.drinkPrices() -> [gold]
FacilityRules.churchMenu() -> [rest, minigame, rite, donate]
FacilityRules.canSpendLetter(town_id, force_id) -> { allowed, need_kind }
```

命令（本系统收点击，转发时只带 ID 与标量）：

```
RestAtInn { town_id, nights }
RestAtChurch { town_id }
DonateAtChurch { town_id, gold }
LaunchMinigame { minigame_id, source, rite_key? }
SignContract { town_id, gold_paid }
OpenInvestment { town_id, source }
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

- 进行中的休息（若跨 tick）
- 已陈列珍品 `item_id`（若确认进 `CTown`）
- `letter_state` 不在本系统存（factions / 玩家状态）
- 当前已接委托在 requests 存档

「今晚是否已休息」`[未知]`；有则进 save schema。

## 9. 未核实字段

- `CInn`/`CPalace` 无字段；菜单状态机在 unassigned，未挂到类上。
- `CGuild+0x1c`、`CBar+0x14`。
- `0xc0b3`「在这里船员身体和精神都能够恢复」与旅馆 / 教会 / 船上礼拜室（`03-cabins-crew-posts.md` `rest_room`）的一一对应。打包块还含圣水、岗位说明。
- 旅馆 `nights` 上限；是否推进 `SCalendar`；每夜身体/精神/疲劳增量。
- 教会「`%s` 枚金币」是捐献、收费休息、还是别的设施串进来的通用句。
- 捐献效果；安静祈祷是否收费。
- `alIdentify` 136 字节内容；17 个 `CSelectPushButton` 里几个是选项、几个是框。
- 仪式文案是否与 `CChurchGameView` 同路径。
- 合约金 → 份额；最低合约费与发展度。
- 独占：单槽 100 还是仅一槽非空。
- 介绍信「小城市」判定。
- 推荐信的 `letter_state` 第三值；花信后份额增量。
- 珍品月租；`0x11e7` 是否必走宫殿。
- 投降交涉成功率。
- 投资垫板从宫殿打开还是仅交易所；两者是否共用 `CInvestmentPadView`。
- 同时可接委托数；看板按港过滤的条件。
- `CRequestInformationView+0xb4=0x47`。
- 酒价 5/10/20 选档；`0x733` 里「用%s枚金币来补充吗」是否补给句混排。
- `CDatePadView`（生日/日期垫板，`trade/`）是否也用于旅馆夜数；ctor `+0xbc=5` 与酒价/夜数无关的像素立即数居多，未核实前不要当夜数上限。
- `CLetterSelectView` 是 CJK 选字（姓名），**不是**介绍信 UI。
- 休息是否清疾病/死亡；船医治病与旅馆休息是否互斥。
