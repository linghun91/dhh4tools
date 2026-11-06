# 同伴与羁绊

开局带谁上船、谁锁在旗舰、月结时扫哪些在岗人、酒馆怎么打听位置、图鉴传记怎么挂 key。名册招入/解雇见 `04-recruitment-crew.md`；舱室几何见 `05-ships-fleet/03-cabins-crew-posts.md`；EV 何时招人见 `10-narrative`。

原版 RTC 把「同伴」用在两处，重构必须拆开：

| 原版名 | 实际含义 | 本文件 |
| --- | --- | --- |
| `asCompanion` / `aeCompanion` | 当前玩家舰队舱室里的在岗名单（工作数组） | 查询接口，不进内容表 |
| 剧本开局同伴 | 与主角同势力、或开场 EV 拉上来的具名航海士 | `companions.yaml` |
| `PLAYERCOMPANION_BITS` | `CPlayerFleetData` 里的位域类型，宽未知 | 人物封装，海洋禁止直接解析 |

没有独立的「好感度 / 结婚 / 配偶」状态机。誓言戒指与爱情灵药是道具目录里的条目，不在这里发明恋爱循环。

## 1. 职责边界

本系统管：

- 每个 `scenario_id` 开局要 `PersonRecruited` 的 `person_id` 列表
- 旗舰锁定：哪些人拒绝调到其它船（文案在，映射未钉死）
- 把 `PLAYERCOMPANION_BITS` 与在岗工作数组封装成 `Characters.companions()` / `Characters.aboard()`
- 图鉴长传记的文案 key（按 `person_id`）；点数经济仍走叙事画廊
- 酒馆「打听某人现在在哪」的**查询入口**（城镇发命令，人物回答 `town_id` / `fleet_id` 或不知道）

不管：

| 事项 | 归属 |
| --- | --- |
| 98 槽名册、雇佣水手人数、三熟练技能 | `04-recruitment-crew.md` |
| 舱室 31 槽、岗位、换旗舰解任 | `05-ships-fleet/03-cabins-crew-posts.md` |
| 委托人 `eRequestPerson`（公会找人/护送） | `08-town-life/02-requests-schemes.md` |
| EV opcode、事件旗、开场脚本 | `10-narrative/02-event-system.md` |
| 通关点数换简介/照片的价格 | `10-narrative/01-scenarios.md` 画廊段；专篇 `05-gallery-unlock.md` |
| 行走图 / 半身像 | `05-portraits-animation.md` |
| 道具标价、种类、装备槽 | `09-items` |
| 酒馆酒价、女招待表、划拳 | `08-town-life/01-facilities.md` |

叙事发 `StartScenario`；人物按本表执行 `PersonRecruited`，再按船侧模板把人绑到旗舰舱室。其它系统只拿 `person_id`，不读 bitfield，不持 `CPlayerFleetData*`。

## 2. 原版事实

### 2.1 在岗工作数组（`asCompanion` / `aeCompanion`）

- `[确认]` RTC 名 `asCompanion` 出现在 `fn_140005a15`（body `0x1400fa870`，4192 字节，补给/新舰队状态）和 `CFleetElement::vfunc_04`（月结）。栈上体积 **1880 字节**。来源：`rtc.json`、`unassigned_named.tsv`、`CFleetElement.cpp`。
- `[确认]` RTC 名 `aeCompanion` 出现在 `fn_1400044cb`（body `0x1401605f0`，770 字节）。栈上体积 **940 字节**。来源：同上。
- `[确认]` `1880 / 8 = 235 = NUM_PERSON`，`940 / 4 = 235`。`fn_140002e3c` 的 `aePerson` / `aeCabinKind` 同样各 940 字节。来源：`fn_1401db9d0.cpp` RTC。
- `[确认]` `fn_140002e3c` 先调 `fn_1400120ee` 填「人物 id 数组 + 舱室种类数组」，再按条打成 8 字节记录（`fn_140004561`）。循环跳过 `person_id == 0xEC`。来源：`fn_140002e3c`、`fn_1400120ee`。
- `[确认]` `fn_1400120ee` 扫 `ePerson >= 0xEB`（235）建 `abPersonFlag`（940 字节），再扫旗舰 31 舱（`eShipCabin >= 0x1F`）、另外 4 艘船（`eFleetShip >= 5`）。空人物写 `0xEC`，空种类写 `0x18`。来源：同文件。
- `[确认]` `fn_140009106` 只向 `fn_1400120ee` 要人物 id 列（`aeCompanion` 的填充路径）。`fn_1400044cb` 用它扫在岗人，对身心状态 2/3/4/5 做月结分支。来源：`fn_14015b600.cpp`。
- `[确认]` `CFleetElement::vfunc_04` 在日历条件命中后：`fn_140002e3c` → 按条读 `*(asCompanion+4)`；若种类 `== 0x11` 走另一条半值路径，否则按表 `DAT_140682bc0` 给该人加两段量。来源：`CFleetElement.cpp`。种类 0x11 的舱室名 `[未知]`。
- `[确认]` `fn_140005a15` 的 RTC 同时有 `asCompanion`、`aePerson`（124 字节）、`anSupply`、`anSupplyShortage`、`nShipTotal`、`nLatitude`/`nLongitude`。反编译在 `fn_1400f7c10.cpp` 失败（format 字符串），函数体未读懂。来源：`rtc.json`、该 cpp 注释。
- `[推断]` `asCompanion` = `{ person_id: u32, cabin_kind_id: u32 }[235]`，`aeCompanion` = `person_id[235]`，都是**栈上工作区**，有效长度由填充函数返回值给出，不是「235 个故事同伴」。依据：体积整除 `NUM_PERSON`、填充来自舱室绑定、空槽 `0xEC`。

这不是剧本开局表。海洋月结要「船上有谁」时走人物查询，禁止自己解析 1880 字节。

### 2.2 `CPlayerFleetData` 与 `PLAYERCOMPANION_BITS`

- `[确认]` `CPlayerFleetData : CSaveObject : CObject`。头文件并列三种位域类型：`FLAGSHIPSTATE_BITS`、`PLAYERSTATE_BITS`、`PLAYERCOMPANION_BITS`。来源：`PlayerFleetData.h`、`class_hierarchy.txt`、`constants.json`。
- `[确认]` 断言：`0 <= eData && eData < _GetBitTotal_BYTE2DWORD(sizeof(PLAYERCOMPANION_BITS))`。来源：`asserts.txt`。sizeof 未恢复，不能假设 98 位或 235 位。
- `[确认]` `vfunc_05` 初始化：`+0x0A` 起 31 字节写 `0xEC`；`+0x29=0x0B`、`+0x2C=-1`、`+0x30=0x1B`、`+0x34=0`；再 `4×31` 组各 2 字节；`+0x130` dword 写 0。`vfunc_06/07` 按同一布局读写。来源：`CPlayerFleetData.cpp`。
- `[推断]` `+0x0A` 的 31 字节是旗舰舱室人物（空=`0xEC`）；`+0x38` 起是另外 4 艘船的人物+种类。这是**舱室绑定**，见岗位文档，不是 `PLAYERCOMPANION_BITS` 本身。
- `[未知]` `+0x130` 那个 dword 是否就是 `PLAYERCOMPANION_BITS`（若 sizeof=4 则 32 位）。禁止把 31 个舱室字节当成同伴位域。

`0xEC=236`：`NUM_PERSON=235` 外侧，空人物哨兵 `[推断]`，与 `INVALID_PERSON=-1` 并存。重构用 `person.empty`，不要用 0（拉斐尔）或 -1 当舱室空槽。

### 2.3 开局同伴：世界表势力 + 传记

世界表人物行 `+0x2A` 一字节，与 `eForce` 对齐 `[推断]`。依据：

- 取值 0–21 落在 `NUM_FORCE=25` 内；**26（`0x1A`）** 与势力运行时空哨兵相同（`CForce::vfunc_05` 写 `0x1A`，`01-forces.md`）。
- 七个主角的值分别是 0、2、1、3、20、13、21，与「卡斯特罗商会 / 柏格斯通商会 / 阿歌特商会 / 李家 / 佐伯家 / 伍丁商会 / 昆廷韦拉斯」的 `force_id` 一致。来源：`Docs/_data/persons_world0.json` hex、`15-catalogs/forces.md`。
- 0–97 中与主角同值的第二人：

| force_id | 势力 | 主角 | 同势力其它人（0–97） |
| ---: | --- | --- | --- |
| 0 | 卡斯特罗商会 | 0 拉斐尔 | **8 库拉乌迪** |
| 1 | 阿歌特商会 | 2 丽璐 | **12 卡米尔** |
| 2 | 柏格斯通商会 | 1 赫德拉姆 | （无） |
| 3 | 李家 | 3 华梅 | **13 希恩** |
| 13 | 伍丁商会 | 5 亚伯拉罕 | **32 阿米娜、33 德尼雅、34 法娣玛、35 哈希姆、66 伊文** |
| 20 | 佐伯家 | 4 杏太郎 | **31 令** |
| 21 | 昆廷韦拉斯 | 6 蒂雅 | （无） |

同势力 ≠ 开局一定在船上。伊文传记是周游推销的传教士，不列入伍丁开局名单，除非 EV 证明。

传记与选人文案 `[确认]`（`mesfile_sc.txt`）：

| 剧本 | 文案 | 偏移 |
| --- | --- | --- |
| 拉斐尔 | 和库拉乌迪 10 岁就认识；库拉乌迪拉他去冒险 | `0x1ef2f`、`0x1fb04` |
| 丽璐 | 「和童年伙伴卡米尔一起梦想做一番大事业」 | `0xbab`、`0x1f2e6`、`0x202c2` |
| 华梅 | 从希恩那里继承「侠义」；希恩把她当孩子抚养 | `0x1f447`、`0x20406` |
| 蒂雅 | 塞维把她当姐姐；两人处境相似 | `0x1f943`、`0x2135f` |
| 杏太郎 | 令作为普雷依拉翻译上船，漂流后继续保护杏太郎 | `0x21c48` |
| 伍丁 | 阿米娜/德尼雅/法娣玛侍候；哈希姆是执事兼侍从 | `0x21e33`–`0x2215a` |
| 赫德拉姆 | 少年离开同伴投靠老师；传记未点开局航海士名 | `0x1f11b` |

甲板岗位对白会点名这些人 `[确认]`：`0x164e3`「希恩教过我掌舵的。」`0x167d7`「像库拉乌迪这样的人很容易受伤……。」`0x16923`「不愧是塞维的食谱……。」这是主角台词，不是入队表。

EV0–EV6 头与触发计数已还原（`02-event-system.md`），opcode 未译码。仓库里的 `savedatXX.dk4` 是后期/改档盘，名册几乎人人有六维，**不能**当开局快照。

因此开局名单标 `[推断]`：世界表同势力 + 传记交叉；赫德拉姆空表、蒂雅的塞维靠开场 EV。禁止把杰拿斯、格尔哈特、行久写成开局同伴——他们 force=`0x1A`，是后来招的。

### 2.4 旗舰锁定

- `[确认]` 调船被拒的台词（`0x810b`–`0x8323`）：「没有我跟着，你就什么也不行。」「我是你的监护人，不能离开这艘船。」「我答应辅助您。我不能离开这艘船。……不！不管您怎么说，我们也要留在船上。请您不要讲到其他船上去之类的话。」
- `[未知]` 触发这些句的是 `PLAYERCOMPANION_BITS`、开局名单、还是舱室种类。重构把「锁旗舰」做成 `companions.yaml` 的 `flagship_locked`，缺省只给开局名单；原版精确集合进未核实。

### 2.5 酒馆打听 ≠ 委托人

- `[确认]` 酒馆发招募：「有人要乘坐提督的船……让他上船吧？」（`0x7627`）。入队仍走 `PersonRecruited`。
- `[确认]` 位置类传闻模板：「这一带名叫%s的%s的%s很有名。」（`0x8357`）；「提督！我想试着找出%s。好像在%s。」（`0xbcd3`）；「%s的舰队正停泊在%s。」（`0x8743`）。阿科布有专句「好像在%s」（`0x728b`）。
- `[确认]` `CBar` 构造写 `+0x18=5`。来源：`CBar.cpp`。`[推断]` 同屏/菜单 5 项，不是打听表长。
- `[确认]` `eRequestPerson` 是**委托委托人**。`fn_14000e23c` 循环 `eRequestPerson >= 0x64`，命中写入 400 字节 `aeRequestPerson`，再随机一个写到委托记录 `+0x11`。来源：`RequestPerson.h`、`fn_1401eb2a0.cpp`。`[推断]` `NUM_REQUESTPERSON=100`。公会「寻找居住在%s某个地方的%s」（`0x152b7`）走这张表，**不是**同伴位。
- `[确认]` 符号 `NUM_PERSONKNOWN` / `INVALID_PERSONKNOWN` 存在，整型未写出。来源：`PersonknownID.h`、`asserts.txt`。是否「已经打听过这个人」`[未知]`，不要和 `PLAYERCOMPANION_BITS` 合并。
- 简中 mesfile **没有**「打听同伴」四字。`04-recruitment-crew.md` 把酒馆打听写成确认，依据是上列位置模板 + 酒馆菜单职责，不是菜单标题原文。

### 2.6 图鉴传记

- `[确认]` 选人界面通关提示（`0xed0`）：「如果在其他剧本中顺利过关，使用１点就能显示此人物的人物简介，要显示吗？（剩%s点）使用%s点显示额外的照片吗？（剩%s点）」
- `[确认]` UI：`CAcrossGameView`、`CAcrossGameCharacterPushButton`。画廊先扫剧本 0–3 再扫 4–6。来源：`CAcrossGameView.cpp`、`fn_1400089e5`、`10-narrative/01-scenarios.md`。
- `[确认]` 长传记从 `0x1ef2f` 拉斐尔起，接着赫德拉姆、丽璐、华梅、杏太郎、伍丁、蒂雅，其后按名册顺序排具名航海士（杰拿斯/库拉乌迪、弗利奥、克莉丝汀娜……直到宗九郎），抽出时多条粘在同一块。来源：`mesfile_sc.txt`。
- `[确认]` 人物信息窗 `CPersonInformationView` 是 5 装备槽 + 10 技能的状态卡，不是画廊。来源：`CPersonInformationView.cpp`、装备文档。
- 点数存在哪段存档、额外照片价 `[未知]`。本系统只持 `bio_key`；扣点、通关置位归叙事。

### 2.7 戒指、灵药、结婚 —— 不要发明恋爱

查过 mesfile「结婚 / 戒指 / 爱情灵药」和道具表：

| 对象 | 事实 | 等级 |
| --- | --- | --- |
| `eItem` 140「誓言戒指的剧本」 | 种类字节 7（赠礼），标价 8500，记录形态与其它 gift 相同 | `[确认]` 目录；`[推断]` kind |
| `eItem` 230「小瓶爱情灵药」 | 种类字节 4（人物用具），标价 198000 | `[确认]` 目录；`[推断]` kind |
| 灵药说明 | 「据说只要一滴，就能俘获心上人的魔药。不过只有用过的人才知道效果如何。」另句「完全不清楚这种液体会有怎样的效果。」 | `[确认]` `0x1ccc3`、`0x1ee63` |
| 灵药材料 | 发现物/药草文案，属种植 | `[确认]` 同段；玩法在 `09-items` |
| `eItem` 225「真红指环」 | 用具，不是结婚戒指 | `[确认]` |
| 酒馆赠礼 | 「难道……要给我？真的！？太开心了！」 | `[确认]` `0x84cf`，对象是女招待，不是航海士配偶 |
| 「结婚」 | 只出现在 NPC 家世（弗利奥、塞西莉雅父母、罗佩斯祖父） | `[确认]` 传记 |
| 塞西莉雅 | 「随着身心的发展会爱上杏太郎」 | `[确认]` `0x22327`，图鉴句子，没有配偶字段 |

没有 `eSpouse`、没有婚礼事件类、没有「戒指装备后改婚姻状态」的还原函数。重构**不准**加好感度条或结婚指令。赠礼走道具+城镇；灵药当普通用具，效果表保持 `unverified`。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空 / 非法 | 重构 |
| --- | --- | --- | --- | --- |
| 剧本 | `eScenario` | 7 | `INVALID_SCENARIO=-1` | `scenario_id` |
| 人物 | `ePerson` | 235 | `-1`；舱室空槽 `0xEC` | `person_id` |
| 开局同伴名单 | （无独立枚举） | 每剧本 0–4 人 `[推断]` | 空列表合法 | `companions.yaml` 行 |
| 在岗工作数组 | `asCompanion` / `aeCompanion` | 容量 235，有效长度运行时 | `person.empty` | 不进内容包 |
| 同伴位域 | `PLAYERCOMPANION_BITS` | 位宽 `[未知]` | 全 0 | 人物状态，不暴露布局 |
| 委托人 | `eRequestPerson` | 100 `[推断]` | `-1` | **另一系统** |
| 已知人物 | `ePERSONKNOWN` | `[未知]` | `-1` | 打听标记，未还原则不建表 |
| 图鉴传记 | mesfile 块 | 主角 7 + 随后具名者 | 无 bio 则无 key | `bio_key` |

`NUM_NOWPERSON=4` 是同时出场立绘槽（`05-portraits-animation.md`），不是同伴上限。

模组可加 `scenario_id` 行或把 `person_ids` 改成别人，引擎按表长，不写 `if scenario==3`。

## 4. 算法 / 公式

没有伤害式。开局是一次装填；在岗是舱室投影。

```
# 开局（StartScenario 之后，人物执行）
player_force = Forces.leader_force(protagonist_person_id)
for person_id in companions[scenario_id].person_ids:
    PersonRecruited { person_id, force_id: player_force }
    if companions[scenario_id].flagship_locked[person_id]:
        SetCompanion { person_id, present: true, flagship_locked: true }
# 船侧再按 start_fleet_template 绑舱室（本系统不写船）

# 在岗投影（月结 / 补给查询）
aboard(fleet_id) =
    [ binding.person_id
      for ship in fleet.ships
      for cabin in ship.cabins
      if binding.person_id != person.empty ]

# 旗舰锁定
can_reassign(person_id, to_ship):
    if companion.flagship_locked and to_ship != flagship:
        refuse   # 原文「不能离开这艘船」
    else:
        defer to 04-recruitment / 03-cabins
```

参数（进 `caps.yaml` / 本表，禁止写死在引擎）：

| 参数 | 原版包 | 落点 |
| --- | --- | --- |
| `person.empty` | 236（`0xEC`） | `caps.yaml` |
| `force.empty` | 26（`0x1A`） | `factions` / `caps.yaml` |
| `aboard_scratch_cap` | 235 | 运行时取 `len(persons)` |
| `gallery.bio_cost` | 1 | 叙事画廊 |
| `gallery.extra_photo_cost` | `%s` 未还原 | 叙事画廊 |

同势力启发式（**只给内容管线生成 YAML 初稿**，不是引擎语义）：

```
draft_start(scenario) =
    [ p.id for p in persons[0..97]
      if p.world_force_id == protagonist.world_force_id
      and p.id != protagonist.id
      and p.id not in companions.exclude ]
```

原版包 `exclude: [66]`（伊文）。赫德拉姆草稿为空；蒂雅草稿为空，塞维靠 `join: opening_event` 手工补。引擎只读写成的 `person_ids`，不再算 `world_force_id`。

打听：

```
Characters.locate(person_id) ->
    { town_id } | { fleet_id } | unknown
```

原版成功率、金价、是否消耗 `PERSONKNOWN` `[未知]`。城镇付费后调用；人物不读酒价。

## 5. 耦合分析

原版：

- 月结和补给直接在舰队对象上扫 `asCompanion`，人物/岗位/海洋搅在 `CFleetElement::vfunc_04`。
- 开局同伴写在世界表 `+0x2A`（势力）+ EV 脚本里，选人界面不知情。
- 酒馆菜单、委托人、阿科布行踪、舰队停泊传闻共用一批 `%s` 句。
- 画廊点数和选人 UI 焊在一起（`CAcrossGameView`）。
- `RequestPerson` 与 `ePerson` 是否同一 235 人表的子集 `[未知]`（委托文档已列）。

重构切断：

```
Narrative.StartScenario { scenario_id }
  → Characters.load_start_companions(scenario_id)
  → Ships.bind start_fleet_template
  → Narrative.load event_pack          # 开场 EV 可再发 PersonRecruited（蒂雅/塞维）

Ocean.MonthTick
  → Characters.aboard(player_fleet_id)     # 不读 bitfield
  → 对每人发 Fatigue / 技能月结（07-fatigue / 岗位效果）

Town(Bar).InquirePerson { person_id }
  → Characters.locate(person_id) → UI 只拿 string_key + town_id

Town(Bar).RecruitOfficer { person_id }
  → 04 的 PersonRecruited，不是本表

Town(Guild).AcceptRequest
  → RequestPerson 池，禁止问 Characters.companions()
```

禁止：经济/战斗读 `PLAYERCOMPANION_BITS`；海洋持有 `CPerson*`；把 `eRequestPerson` 当成同伴 id。

## 6. 解耦后的配置

落点：`config/characters/companions.yaml`（本系统）。`narrative/scenarios.yaml` **不复制** `person_ids`，只保留 `scenario_id` / `protagonist_person_id`。传记正文在 `ui/strings.*.yaml`。

```yaml
# config/characters/companions.yaml
schema: dk4.characters.companions
version: 1
pack: vanilla

empty_person: 236              # 原版 0xEC；舱室空槽，不是 INVALID_PERSON
scratch_cap: 235               # asCompanion 容量 = len(persons)；引擎读表长

# 内容管线启发式，不进运行时
generate:
  same_force_as_protagonist: true
  roster_only: true            # 只考虑 person_id < sailor_roster
  exclude_person_ids: [66]     # 伊文：同伍丁势力，传记不是开局随员

scenarios:
  - scenario_id: 0
    key: raphael
    person_ids: [8]            # 库拉乌迪
    flagship_locked: [8]       # [推断] 对应「不能离开这艘船」
    notes: "bio+force 0；EV 未译码"

  - scenario_id: 1
    key: hedram
    person_ids: []             # 世界表 force 2 无第二人
    flagship_locked: []
    ev_opening_unknown: true

  - scenario_id: 2
    key: liru
    person_ids: [12]           # 卡米尔
    flagship_locked: [12]

  - scenario_id: 3
    key: hua_mei
    person_ids: [13]           # 希恩
    flagship_locked: [13]

  - scenario_id: 4
    key: kyotaro
    person_ids: [31]           # 令；[推断] force 20 + 传记
    flagship_locked: [31]

  - scenario_id: 5
    key: woodin
    person_ids: [32, 33, 34, 35]  # 阿米娜 德尼雅 法娣玛 哈希姆
    flagship_locked: [32, 33, 34, 35]
    # 66 伊文故意不写

  - scenario_id: 6
    key: tia
    person_ids: [25]           # 塞维
    flagship_locked: [25]
    join: opening_event        # 世界表 force=26，不是开局隶属；靠 EV0 号包 EV6
```

图鉴 key（人物拥有，解锁规则在叙事）：

```yaml
# 同文件或 characters/bios.yaml —— 只挂 key，不写正文
bios:
  - person_id: 0
    bio_key: ui.bio.raphael          # mesfile 0x1ef2f
  - person_id: 8
    bio_key: ui.bio.claude           # 0x1fb04 后半
  - person_id: 12
    bio_key: ui.bio.camille          # 0x202c2
  # … 其余具名者按 mesfile 顺序生成；无传记则缺行
```

`flagship_locked` 原版集合未钉死。default 包先等于 `person_ids`；模组可把锁定改成空，让开局同伴能调船。

## 7. 独立旋钮

- 某剧本换开局名单（只动本表）
- 把塞维改成世界表开局而非 EV（改 `join`，叙事脚本另改）
- 关掉旗舰锁定
- 图鉴 bio 正文（只动 strings）
- 打听是否收费（城镇设施表；本系统仍只回答 locate）

不要把「华梅必须带希恩」写进人物 C++。不要把灵药做成结婚开关。

## 8. 对外接口

查询：

```
Characters.start_companions(scenario_id) -> [person_id]
Characters.companions() -> [person_id]          # 封装 PLAYERCOMPANION_BITS
Characters.aboard(fleet_id) -> [person_id]      # 舱室投影，月结用这个
Characters.flagship_locked(person_id) -> bool
Characters.locate(person_id) -> { town_id | fleet_id | unknown }
Characters.bio_key(person_id) -> string_key | none
```

命令：

```
LoadStartCompanions { scenario_id }             # StartScenario 的人物侧
SetCompanion        { person_id, present, flagship_locked? }
InquirePerson       { person_id }               # 酒馆；返回 locate + string_key
```

事件：`CompanionChanged`、`CompanionLocked`。`PersonRecruited` 仍由 04 发出；本系统订阅后可把开局名单里的人标 `present`。

海洋 / 补给 / 身心月结：`Ocean.MonthTick` → `Characters.aboard()`。禁止订阅方缓存 `person_id` 列表跨月；下一次 tick 再问。

画廊 UI：`Narrative.gallery_can_show_bio(person_id)` 为真时，向本系统取 `bio_key`。人物系统不读 `scenario_id` 通关位。

## 9. 未核实字段

- `PLAYERCOMPANION_BITS` 的 sizeof、存档偏移、与 `+0x130` dword 的关系。
- `fn_140005a15` 函数体（反编译失败）；补给短缺如何用到 `asCompanion`。
- 月结种类 `0x11` 是哪一间舱。
- EV 开场是否立刻 `PersonRecruited`（尤其赫德拉姆空表、蒂雅/塞维、杏太郎/令）。
- 伊文（66）同伍丁势力：开局在不在船上。
- 旗舰锁定句对应的 `person_id` 集合。
- 酒馆打听：菜单项、金价、失败句、是否写 `PERSONKNOWN`。
- `NUM_PERSONKNOWN` 整型。
- `RequestPerson` 与 `ePerson` 是否同一批 id。
- 长传记与 `person_id` 的 1:1 切分（mesfile 多条粘块）；安杰洛妹妹比安卡有传记但无 0–97 id。
- 画廊点数存档位置、额外照片价。
- 爱情灵药 / 誓言戒指剧本的使用效果。道具说明已写「不清楚效果」——效果表保持 `null`，禁止补结婚。
- `CBar+0x18=5` 是否含「打听」项。
- 世界表 `+0x2A` 升格为确认的 `force_id`：对照强，但 `01-person-model.md` 仍标未知；本文件按 `[推断]` 用，不改那一行的确认等级。
