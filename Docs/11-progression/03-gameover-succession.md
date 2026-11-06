# 死亡、寿终、继承与 Game Over

本系统判定「这一局是否结束、结束时播哪段终章、能不能把同伴升成新主角」。身体状态枚举本身归人物；金币与船数归经济 / 舰队。这里只做**终局策略**。

经验、等级见 `01-level-exp-skills.md`。发现称号见 `02-fame-titles.md`。身心六字见 `03-characters/03-abilities-skills-jobs.md`。Game Over 画面是 UI 投影，见 `12-ui-save/01-scenes-flow.md`。

## 1. 职责边界

本系统管：

- 终局种类（`ending` key）：寿终、主角身体死亡、全船不可航、资金穷尽等
- 寿终过场要播的文案 key 与顺序
- 继承策略：候选从哪来、没有候选时是否 Game Over、继承时移交哪些 ID
- 终局入口统一是会话层 `Session.end_scenario(outcome, cause)`（D8，`10-narrative/03`）；本系统是会话层查的**终局策略表**：给定 `cause`，决定 ending、要播的文案 key、是否先提供继承
- 向 UI 发出 `SuccessionOffered`；Game Over 事件 `ScenarioFailed { scenario_id, cause, end_code? }` 由会话层发（`00-overview/06`；`cause` 是语义枚举，`end_code` 只供原版导入，词表 §14-3），本系统经 `GameOver.pending()` 给出 ending、文案 key、still；不自己画 still

不管：

- `ePhysicalState` 的名称表与系数（人物；本系统只订 `PersonDied`（`is_protagonist`），不比较状态值）
- 疲劳、轻伤、疾病怎么从航海 / 战斗写进人物（人物收命令）
- 海战「旗舰溃败 / 水手全灭、逃到其他船」（战斗）；本系统只在「已经没有可航船」时被通知
- 地方舰队赤字、借款、解散确认（舰队；那不是玩家 Game Over）
- 交易「钱不够」拒绝买入（经济）
- 生日怎么进 `SGameSeed`、日历怎么 tick（叙事 / 世界）
- `TitleSC.dk4` 第几张图怎么解码（UI / 语言包只提供资源 id）
- 船员 headcount 为 0 但船体还在（船；文案「连一个水手也没有了」由船侧发出）

禁止：UI 直接改 `save+0x20` 主角 id；战斗把「旗舰沉了」写成 Game Over；经济把「钱不够」写成破产终局。

## 2. 原版事实

### 2.1 身体状态「死亡」不是寿终过场

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 身体态名表是 exe 指针表 `.data 0x140686410`，**7 项**：0 健康 1 疲劳 2 轻伤 3 重伤 4 濒死 5 疾病 6 死亡 | `[确认]` | 取名函数 `fn_140010e6a`（body `0x1401c9870`）：读 `person+0x0D`，`< 7` 才查表；「疲劳」串不与其它五个相邻，按地址扫会漏掉 |
| 精神态名表 `0x140686458`，**6 项**：0 高亢 1 精神 2 轻松 3 普通 4 不高兴 5 混乱 | `[确认]` | `fn_1400056c3`（body `0x1401c9380`）：读 `person+0x0E`，`< 6` |
| `NUM_PHYSICALSTATE` = 7、`NUM_MENTALSTATE` = 6（取名越界检查） | `[确认]` | 同上；头文件仍未写整数 |
| 身体态改技能：疲劳/轻伤 ×19/20，重伤 ÷2，濒死 ÷10，疾病 ×3/5 | `[确认]` | `fn_1401c87a0`；见 `01-level-exp-skills.md` 4.3 |
| `PHYSICALLEVEL_MIN/MAX` 约束在 `UCHAR`，整数未还原 | `[确认]` | `asserts.txt` |
| 技能不存档；HP、身体态、精神态进存档 | `[确认]` | `03-characters/07` 2.3；旧稿「HP / 心情不存档」错 |
| `CPerson::vfunc_04`：`*(this+0x0D) == 6` 则返回 0，否则 1 → 即「身体态 ≠ 死亡」 | `[确认]` | `CPerson.cpp` body `0x1401ca620`；`+0x0D` 是身体态字节（上条），6 = 死亡 |
| `CPlayerPerson::vfunc_04` 空实现（直接 return） | `[确认]` | `CPlayerPerson.cpp` body `0x1402aa3a0` |
| 存档人物记录（id < 87）`+0x12` 身体态、`+0x13` 精神态（对象 `+0x0D` / `+0x0E`；dk4save 行 `+0x15` / `+0x16`） | `[确认]` | `CPerson::vfunc_06` `0x1401cb574` / `0x1401cb58d`；`03-characters/01` §2 |

「死亡」是身体态 6，船员可以处于死亡。寿终过场是另一条、对着**当前主角**的终章，不要合成一个函数。

### 2.2 寿终文案（终章旁白）

SC `mesfile.dk4` 把相邻 UI 句挤进同一条 NUL 串，抽出文本会粘在一起。JP 同段有 NUL 切开。以 JP 为句边界，SC 对译如下。

| 顺序 | JP（`Game/JP/mesfile.dk4`） | SC 对译 | SC 粘连位置 |
| --- | --- | --- | --- |
| A | `長年に渡り、%sを率いてきた\n%sは…` @ `0x4ff0` | `多年来率领%s的%s……。` | 与「成为伙伴」粘在 `0x3817` |
| B | `天寿をまっとうし、\nその波乱に満ちた生涯を閉じた。` @ `0x5012` | `寿终正寝，结束了波澜壮阔的一生。` | `0x383b` 起 |
| C | `中世から近世にかけて\n世界に多大なる変革をもたらした\n大航海時代は…` @ `0x5046` | `从中世纪到近代，给世界带来重大变革的大航海时代……。` | 同上 |
| D | `数々の伝説を残しつつ、\n次の新しき世代へと\n語り継がれていくのである――` @ `0x508b` | `留下了一个个的传说，迎向崭新的时代。` | 同上；SC 无「――」 |

`0x383b` 末尾还粘了 `%s拒绝接受亲笔信。`。JP 下一句是独立的 ` %sは\n親書の受け取りを拒否しました。` @ `0x50d3`，属于公会文书，**不是**寿终句。来源：`mesfile_sc.txt`；JP 原文直接从 `mesfile.dk4` 按 NUL 切开。

句 A 的两个 `%s`：JP 语序是「多年来率领 X 的 Y 是…」。重构按 `leader_name`、`force_or_fleet_name` 两个槽，禁止在代码里写死「先人名后势力」。

### 2.3 「成为伙伴」不是继承问句

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| SC `0x3817`：`%s成为伙伴！多年来率领%s的%s……。` | `[确认]` | `mesfile_sc.txt` |
| JP 把「入手」和「入伙」放在寿终 A 句**之前**一条：`%sを手に入れました！%sが\n仲間になりました！` @ `0x4fc3` | `[确认]` | JP mesfile |
| 仓库无「选继承者 / 後を継 / 跡取り / この人を後継」问句 | `[确认]` | JP/SC mesfile 全文检索 |
| `PLAYERCOMPANION_BITS` 与 `FLAGSHIPSTATE_BITS`、`PLAYERSTATE_BITS` 同在 `CPlayerFleetData` | `[确认]` | `PlayerFleetData.h` `asserts.txt` |
| RTC `asCompanion` / `aeCompanion` 出现在舰队月结与海洋补给 | `[确认]` | `CFleetElement.cpp` `fn_14015b600.cpp`；见 `04-recruitment-crew.md` |
| 存档 `+0x20` 是当前主角 `person_id`，改主角必须连同人物记录（`0x2C03` 段与 `0x35AA` 记录）与 `+0x70` 一起写 | `[确认]` | `save_format.md` `Save.cpp` `01-person-model.md` |

`%s成为伙伴` 在字串表里紧挨寿终，但 JP 把它和「得到了 %s」捆在一起，语义更像入队 / 入手，不是「要不要以同伴继续」。

**能否以同伴继续玩：** 原版**不能** `[确认]`。寿终四句 `0x170..0x173` 只在 Game Over 分支里播（`+0x90 == 1`），播完直接进 still，`fn_14000e8a4` 之后无选人 UI、无写主角 id。重构把继承做成显式旋钮（默认关），属于新玩法。

### 2.4 Game Over 画面

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| RTC：`cGameOverStill` `cGameOverSurfaceSprite` `rcGameOver` `posGameOver` `sizeGameOver` | `[确认]` | `rtc_names.txt` `unassigned_rtc.md` |
| 函数 `fn_14000a894` body `0x14011fb70` size `0x424` | `[确认]` | `fn_140115330.cpp` `functions.tsv` |
| 宿主是全局 `0x144296500`；**唯一**写 `+0x88 = 3` 的是 `fn_140010578(host, natural)`（body `0x1400c42d0`）：`+0x88 = 3; +0x90 = natural` | `[确认]` | 全 exe 搜 `c7 8x 88 00 00 00 03 00 00 00` 只此一处 |
| `natural = 1` 的入口①：事件执行器 `fn_1400050fb`（RTC `sScriptTable` `sExecute`）遇事件号 `0x3fff` | `[确认]` | `0x1400c39a1` `cmp ..,0x3fff` → `fn_140010578(host,1)` |
| `natural = 1` 的入口②：事件脚本 opcode `0xC5`（`end_scenario`，`fn_14000847c` body `0x1400e1970` 的 class 6 分支）：`fn_140008b98` 取 imm8；`0` → `fn_14000210d` 写 `+0x88 = 4`（剧本通关）；`1` → `fn_140010578(host,1)`；其它 → `fn_140007b58` 写 `+0x88 = 0`（回标题） | `[确认]` | `0x1400e1d80..0x1400e1dd2`；opcode 归属见 `10-narrative/03-event-commands.md` |
| `natural = 0` 的入口①：`fn_14000a9ed`（舰队覆灭处理，被海战 `fn_14000d7ab`、战后俘获 `fn_14000ec6e`、`CWorld::vfunc_05` 等调用）：被灭舰队属玩家势力且是玩家本队 | `[确认]` | `0x1400ef115..0x1400ef13e`：`fn_14000e638`（势力 == 玩家势力）且 `fn_14000188e` |
| `natural = 0` 的入口②：`fn_14000a8c1`（舰队解散，msg `0x16b`「%s解散了」）解散的是玩家舰队 | `[确认]` | `0x14010d01f..0x14010d039` `fn_140005b19` |
| `+0x88 = 5` = **退出**：`fn_140014ab0`（body `0x1400d6b40`）只写 5，调用点都在各消息循环 `cmp msg, 0x12`（`WM_QUIT`）之后（`0x1400d57ac` 等 6 处）及 Game Over 收尾；Game Over 播完不把 5 清零 | `[确认]` | 指令；与 `12-ui-save/01` 流程态表一致。旧稿「事件动画态」错 |
| still 从 `TitleSC.dk4` **图 5** 加载；surface 从同包 **图 4** 加载 | `[确认]` | 同上，`fn_140006faa(..., 5)` / `(..., 4)` |
| 主表面尺寸立即数 `0x780` × `0x438`（1920×1080） | `[确认]` | `fn_140006596(0x780, 0x438, ...)` |
| 进入前 `dwCancel = 0xf`，并 `fn_14000cf1d(..., 0x39, 2, 1)` | `[确认]` | 同函数；`0x39` 语义 `[未知]`（BGM / 画面态） |
| 调用点 `fn_14000e8a4` body `0x140112820`：宿主 `+0x88 == 3` 走 Game Over | `[确认]` | `fn_14010be70.cpp` |
| 若 `+0x90 == 1`（寿终），先连续 `fn_14000fb14(0x170..0x173)` 四次，再进 still；`+0x90 == 0` 直接 still | `[确认]` | 同上 |
| 消息 `0x170..0x173` **就是寿终四句 A–D**；`0x16f`「%s成为伙伴！」、`0x174`「%s拒绝接受亲笔信。」各是独立消息 | `[确认]` | 消息号不是按 NUL 切的序号：exe `.data 0x140683620`（37 块 `{u16 first_id,u16 size}`）+ `0x1406836c0`（每条 `u16` 块内偏移）。按表解 SC：`0x170` 多年来率领%s的%s……。`0x171` 寿终正寝… `0x172` 从中世纪到近代… `0x173` 留下了一个个的传说… |
| 播完后若 `+0x88 != 5` 则写成 `0` | `[确认]` | `fn_14000e8a4` |
| `+0x88 == 4` = 剧本通关：走 `fn_14000b677`（`cEventAnimOP` 片尾 + sidecar 记账），不是 Game Over still | `[确认]` | 同函数；`10-narrative/05` |
| 宿主 `+0x88` 流程态全表：0 标题、1 进行中、2 读档、3 Game Over（`+0x90` 子码）、4 剧本通关、5 退出 | `[确认]` | `10-narrative/03` 流程态、`12-ui-save/01` |
| 标题场景同样从 `TitleSC.dk4` 取图 2/3/6/7/8 | `[确认]` | `fn_1401b9940.cpp` `fn_1401200a0.cpp` |
| 场景表已把 `game_over` 登记为 overlay | `[确认]` | `12-ui-save/01-scenes-flow.md` |
| 无「ゲームオーバー / Game Over」mesfile 句 | `[确认]` | JP/SC 全文检索；画面是图，不是字 |

谁把 `+0x88` 写成 3：已还原为四个入口（上表）。「寿终」（`+0x90=1`，播四句）= 事件脚本的剧情 Game Over：execute id `0x3fff` 或 opcode `0xC5` imm 1；非寿终（`+0x90=0`）= 玩家本队覆灭或解散。重构用 `Session.end_scenario(game_over, cause)`，不复刻这个字节。

### 2.5 沉船、无水手、重建——不是已经钉死的 Game Over

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| JP：`%sが沈没しました！` @ `0x4f93`；`%sの水夫が１人もいません！` @ `0x4fa7` | `[确认]` | JP mesfile |
| SC 粘成 `0x37e7`：`%s号沉没了。%s号连一个水手也没有了！得到了%s。` | `[确认]` | `mesfile_sc.txt`；「得到了」对应 JP 下一句「手に入れました」，是另一条 |
| 海战：旗舰被击、浸水停不住；水手全灭则「请到其他船上躲避」 | `[确认]` | SC `0xebb3`–`0xecfb`；JP `提督、旗艦がやられました` |
| 旗舰耐久 **或** 水手 `<= 30` 则该侧溃败 | `[确认]` | `07-combat/01-naval-battle.md` `fn_14000d7ab` |
| 「用%s枚金币重建%s舰队吗？」 | `[确认]` | SC `0x37a7`；JP `金貨%s枚で、\n%s艦隊を再建しますか？` @ `0x4f34` |
| 同粘连块里还有「%s解散了」「等级上升」 | `[确认]` | SC `0x37a7`；JP 拆开 `@0x4f58` |
| 玩家本队覆灭 / 玩家舰队解散 → `fn_140010578(host, 0)` → 不播寿终句的 Game Over still | `[确认]` | 见 2.4 |
| 重建问句 `0x168` 与上述覆灭判定的先后（拒绝重建后是否才判覆灭） | `[未知]` | 未追到 `0x168` 的调用点 |

沉船与无水手是**船侧事件**。Game Over 只在「玩家侧已经没有可航船、且重建失败」这条合成条件成立时才该登场——原版有没有走这条，cpp 没钉死。

### 2.6 「破产」二字不存在

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| JP/SC mesfile 无「破産 / 破产」 | `[确认]` | 全文检索 |
| 「钱不够！购买这艘船吗？」 | `[确认]` | SC `0x2e7c`；JP `所持金が足りません！この船を購入しますか？` |
| 「其舰队的收支是赤字。要偿还舰队的借款，带的钱不够了。」 | `[确认]` | SC `0x3057`；`02-fleet-proxy.md` 已收为**地方舰队**账本 |
| 同段有「解散这只舰队，可以吗？」 | `[确认]` | SC `0x3057` 粘连 |
| 酒馆 / 投资 / 炮台「钱不够」都是交易拒绝 | `[确认]` | 设施 / 经济文案；不是终局 |

玩家主舰队「金币 = 0 且无船」是否 Game Over：**未还原**。不要把地方舰队解散确认升级成主角破产。

### 2.7 主角对象与生日

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `CPlayerPerson → CPerson → CSaveObject → CObject` | `[确认]` | `PlayerPerson.h` `class_hierarchy.txt` |
| `CPlayerPerson` 在 `CPerson` 之上再串行化：三段 17 字节（`+0x30` `+0x41` `+0x52`）、`+0x63/+0x64`、19 字节 `+0x65`、`uint32 +0x78`、`+0x7C/+0x7D` | `[确认]` | `CPlayerPerson.cpp` `vfunc_06/07` |
| 名字块字段宽 17 字节（名 / 姓 / 中间名） | `[确认]` | `dk4save.c` `NAME_FIELD=17` `02-save-session.md` |
| 开局「请输入生日」，UI `CDatePadView` | `[确认]` | mesfile `0xbab`；`CDatePadView.cpp`；`SGameSeed` 体积 `0x78` |
| `SCalendar : SDate`；构造把日 `== 0x2F` 当特殊月末 | `[确认]` | `SCalendar.cpp` `10-narrative/02-event-system.md` |
| 生日存档偏移、年龄公式、寿终岁数阈值 | `[未知]` | `01-person-model.md` 第 9 节；世界表 0x38 行无已命名年龄字节 |
| `rcLeaderButton` 在人物 InfoBox（`InfoBox.dk4`），与 Game Over 函数不是同一条 | `[确认]` | `fn_140014c3b` RTC；属编制 / 人物信息，不是继承对话框 |

三段 17 字节与姓名槽同宽，**[推断]** 是玩家自定姓名，不是继承人名单。年龄是否只给主角算、生日是否其实只喂星座：都未还原。

## 3. 实体与 ID

| ID | 原版 | 空 | 拥有者 |
| --- | --- | --- | --- |
| `ending` | 无枚举；重构用命名空间 key | `null` | **本系统** |
| `person_id` | 235 | −1 | 人物 |
| `physical_state_id` | `ePhysicalState`，7 `[确认]` | −1 | 人物 |
| `protagonist_id` | 存档 `+0x20` 存 `person_id`（剧本主角 0–6） | 非法则拒绝读档 | 人物 / 存档 |
| `companion` | `PLAYERCOMPANION_BITS`，宽 `[未知]` | 无单独空 id | 人物（封装 bit） |
| `fleet_id` / `fleet_ship_id` | 69 / 5 | 运行时 `null`；原版 −1、槽内空船 `0xD0` 只在 `legacy_codec` | 舰队 |
| `scene_id` | `game_over` overlay | — | UI |
| `string_key` | 原版按 exe 消息号（`0x170..0x173` 等）导入 | 缺 key = 加载失败 | 文案包 |

运行时按表长循环。原版没有 `eGameOver` / `NUM_ENDING`。重构 ending 用字符串 key，导入器不必发明原版整数。

身体态整数已由 exe 名表与取名越界检查钉死 `[确认]`（`fn_140010e6a`，`0x140686410`）：

| id | key | 名称 |
| --- | --- | --- |
| 0 | `healthy` | 健康 |
| 1 | `fatigued` | 疲劳 |
| 2 | `light_wound` | 轻伤 |
| 3 | `heavy_wound` | 重伤 |
| 4 | `dying` | 濒死 |
| 5 | `disease` | 疾病 |
| 6 | `dead` | 死亡 |

key 与 `characters/vitals.yaml`（`03-characters/07`）一致。本系统不读状态值，只订人物系统发的 `PersonDied`（`is_protagonist: true`）。禁止业务代码写 `== 6`。

建议的 ending（重构目录，不是原版枚举；`cause` 是 `Session.end_scenario` 的第二参）：

| key | cause | 原版对应 |
| --- | --- | --- |
| `natural_death` | `scripted` | `+0x90=1`：脚本 `0x3fff` / `0xC5` imm 1 → 寿终四句 `0x170..0x173` + Game Over still `[确认]` |
| `protagonist_physical_death` | `protagonist_died` | 身体态 `dead` 落在主角上；原版是否触发 `fn_140010578` `[未知]`（未见身体态写 6 的调用链连到它） |
| `player_fleet_lost` | `fleet_lost` | `+0x90=0`：玩家本队覆灭（`fn_14000a9ed`）或玩家舰队解散（`fn_14000a8c1`）→ 直接 still `[确认]` |
| `bankrupt` | `bankrupt` | 无「破产」文案；默认 **关闭** |
| `manual` | `manual` | 调试 |

通关（`0xC5` imm 0，`+0x88=4`）与回标题（其它 imm）不是 Game Over，归会话层 / 叙事（`10-narrative/01`），不在本表。

## 4. 算法 / 公式

无原版可引用的岁数式、破产式。禁止把 `60`、`70` 写进引擎。常数进 YAML；缺字段 = 该触发关闭，不 fallback。

### 4.1 身体死亡（人物先判，本系统后收）

```
# characters（03-characters/07 §4.6）：身体态进 dead → emit PersonDied { person_id, cause, is_protagonist }
#                                     is_protagonist == false（船员）本系统忽略

on PersonDied { person_id, is_protagonist: true }:
  if succession.enabled and has_candidate():
    emit SuccessionOffered { reason: physical_death, candidates }
  else:
    Session.end_scenario(game_over, cause: protagonist_died)
```

`CPerson+0x0D == 6` 就是身体态死亡（`CPerson::vfunc_04` 用它判存活）。重构里人物系统用 `physical_state` key，不比较整数；本系统连 key 也不读，只订事件。

### 4.2 寿终

```
# 仅当 natural_death.auto_from_age == true 且 age_threshold 为整数
age = Calendar.age_years(person_id)      # 世界/叙事提供；公式未还原则该查询失败
on DayStarted:                           # D4 时间事件
  if auto_from_age and age >= age_threshold and person_id == protagonist_id:
    Session.end_scenario(game_over, cause: scripted)   # 与脚本寿终同一出口

# 会话层收到 end_scenario(game_over, cause) 后查本表：
resolve(cause = scripted):               # 原版 0x3fff / 0xC5 imm 1
  if succession.enabled and has_candidate():
    emit SuccessionOffered { reason: natural_death, candidates }
  else:
    ending = natural_death               # 先播 cinematic keys [preamble, last_breath, era, legacy]（句 A–D）
```

原版寿终在 exe 里没有岁数判定：入口是事件执行器收到保留 execute id `0x3fff`（`fn_1400050fb`），或脚本 opcode `0xC5` imm 1 `[确认]`。哪条触发在什么条件下走到它们，在事件脚本（`EV*.dk4`）里，未解 `[未知]`。所以 vanilla 包：`auto_from_age: false`；叙事导入器把 `0x3fff` 与 `0xC5 imm 1` 都译成命令 `end_scenario { outcome: game_over }`（`10-narrative/03`），会话层带 `cause: scripted` 来查本表。

原版 Game Over 后没有继承：`fn_14000e8a4` 播完 still 只把 `+0x88` 清 0，然后照常回到宿主循环（标题）；全函数无选人 UI、无改 `save+0x20` 的调用 `[确认]`（反编译 `fn_14010be70.cpp`）。`succession.enabled: false` 是**原版行为**，不只是保守默认。

`Calendar.age_years` 失败（生日偏移未导入）→ 整段 no-op，不把日历年当成年龄。

### 4.3 继承

```
has_candidate() =
  succession.enabled
  AND len(candidates()) > 0

candidates() =
  for p in Characters.companions():          # 封装 PLAYERCOMPANION_BITS
    if require_in_party and not in_party(p): skip
    if not Characters.can_act(p): skip         # 原版只有 dead 判否
    yield p

SucceedAs { heir_id }:
  if heir_id not in candidates(): no-op
  old = protagonist_id
  Characters.setPhysical(old, dead)           # 寿终者退出可操作集（人物命令）
  Session.set_protagonist(heir_id)            # 唯一写主角 id 的入口
  Progression.RebuildDerived(heir_id)
  emit Succeeded { old, new: heir_id, reason }
```

移交清单只传 ID：

| 移交 | 谁执行 | 默认 vanilla |
| --- | --- | --- |
| 主角 id | 人物 / 会话 | 是 |
| 玩家舰队 `fleet_id` | 舰队 | 是（改旗舰舰长，不清船） |
| 金币 | 经济 | 是 |
| 名册隶属 | 人物 | 是（已招的人仍隶属同一势力） |
| 死者装备 | 物品 | 掉进旗舰仓库；规则在 `items`，本系统只发事件 |
| 剧本事件旗 | 叙事 | 不重置 |
| 死者经验 | 人物 | 不转移 |

没有候选：`if_empty: game_over`（默认）或 `if_empty: title`。禁止第三种「悄悄换一个职能 NPC」。

`SucceedAs` 与开局选主角共用 `Session.set_protagonist`。原版改 `+0x20` 必须同步人物记录与 `+0x70`；重构只写一处源。

### 4.4 玩家舰队覆灭 / 解散

原版 `[确认]`：舰队覆灭处理 `fn_14000a9ed` 若被灭的是玩家本队，或舰队解散 `fn_14000a8c1` 解散的是玩家舰队，直接 `fn_140010578(host, 0)` → still（不播寿终句）。

```
on FleetDestroyed { fleet_id, cause }:             # cause：eliminated（fn_14000a9ed）| disbanded（fn_14000a8c1）
  if fleet_id != player_fleet: no-op            # 其它玩家势力舰队覆灭：经验由 progression grant_routes 处理（01 §4.5）
  if rebuild.enabled and Force.gold(player_force) >= rebuild.cost:
    emit RebuildOffered { cost }                # 文案 msg 0x168「用%s枚金币重建%s舰队吗？」；原版先后 [未知]
    # 接受 → Fleets.rebuild；本系统结束
  if endings.player_fleet_lost.enabled:
    Session.end_scenario(game_over, cause: fleet_lost)
```

`rebuild.cost` 原版公式 `[未知]`。YAML 无整数则**不出**重建报价，避免引擎猜 100000（那是建地方舰队的文案常数）。

水手人数 = 0 但船还在：船侧发 `CrewWiped`（`05-ships-fleet/06`），不是 `FleetDestroyed`。

### 4.5 资金穷尽（默认关）

```
bankrupt_condition =
  endings.bankrupt.enabled
  AND Force.gold(player_force) <= gold_floor
  AND Fleets.seaworthy_count(player_fleet) == 0
# 成立 → Session.end_scenario(game_over, cause: bankrupt)
```

vanilla：`endings.bankrupt.enabled: false`。地方舰队赤字走舰队解散，永不进这条。

### 4.6 开画面

```
Session.end_scenario(game_over, cause):          # 会话层（12-ui-save）
  ending = GameOver.resolve(cause)                 # 本系统：查 endings[] 的 cause 列；继承优先
  if ending is none or not ending.enabled: 回到游戏（不结束）并记日志
  emit ScenarioFailed { scenario_id, cause, end_code? }   # 会话层发；end_code 仅原版导入（+0x90 子码）；UI 用 GameOver.pending() 取 ending / string_keys / still_key
  UI 切 overlay game_over
  关闭后 Scene.Enter(title)     # 非法 scene 则进 title 并报错，与读档失败同策略
```

still 资源走语言包的 `title_texture` + `game_over.still_index`（原版 SC = 5）。文案不进逻辑 YAML。

## 5. 耦合分析

原版缠在一起的点：

- 寿终句、入伙句、沉船句、重建句、亲笔信句在 mesfile 里前后相连，抽出工具还把它们粘成一行
- Game Over still 复用标题图包，函数在 unassigned 里，靠宿主 `+0x88==3` 进入
- 主角 id、人物记录、总等级缓存三处；继承如果用原版写法会漏写
- `CPerson::vfunc_04` 与 `CPlayerPerson::vfunc_04` 行为不一致
- 地方舰队「钱不够就解散」和玩家终局共用「钱不够」口语
- 同伴位在 `CPlayerFleetData`，海洋月结直接扫 `asCompanion`
- 生日、星座、寿终可能根本不是同一条轴，却都从「请输入生日」进游戏

重构切断：

- 人物只报 `PersonDied` 与 `companions()`
- 舰队只报 `FleetDestroyed` / `seaworthy_count` / `RebuildOffered` 结果
- 势力只报 `Force.gold`
- 日历只报 `age_years`（失败则空）
- UI 只订 `ScenarioFailed` / `SuccessionOffered`
- 叙事只发 `end_scenario { outcome: game_over }` / `SucceedAs`，不可写 still

允许的依赖：`progression.gameover ← characters.companions / can_act`（事件 `PersonDied`）；`← fleets.seaworthy_count`；`← Force.gold`；`← calendar.age_years`。禁止反向去改市价或经验。

## 6. 解耦后的配置

```yaml
# config/progression/gameover.yaml
schema: dk4.progression.gameover
version: 1

natural_death:
  auto_from_age: false          # 岁数阈值未还原；true 且缺 threshold 则启动失败
  age_threshold: null           # 整数或 null；禁止代码猜 60
  cinematic_keys:               # 文案 key；原版按 exe 消息号导入（D10）
    - { key: ending.natural.preamble,    vanilla_msg: 0x170 }   # 多年来率领 {subject} 的 {leader}……。
    - { key: ending.natural.last_breath, vanilla_msg: 0x171 }
    - { key: ending.natural.era,         vanilla_msg: 0x172 }
    - { key: ending.natural.legacy,      vanilla_msg: 0x173 }

succession:
  enabled: false                # 原版无继承 [确认]：Game Over 后直接回标题
  require_in_party: true
  source: companions            # companions | in_party | roster
  if_empty: game_over           # game_over | title
  transfer:
    - protagonist_id
    - player_fleet
    - gold
    - roster_affiliation
  # 装备掉落规则在 items，不在此重复

rebuild:
  enabled: true
  cost: null                    # 未还原；null = 不出「重建吗？」报价
  prompt_key: { key: ending.rebuild.prompt, vanilla_msg: 0x168 }

endings:                        # 主键 key；会话层按 cause 查行
  - key: vanilla:natural_death
    cause: scripted             # 原版 0x3fff / 0xC5 imm 1
    enabled: true
    cinematic: natural_death.cinematic_keys
    still: game_over            # 语言包 title_texture 内的逻辑名
    after: title
  - key: vanilla:protagonist_physical_death
    cause: protagonist_died
    enabled: true
    still: game_over
    after: title
    cinematic_keys: []          # 原版无独立句；空则只出 still
  - key: vanilla:player_fleet_lost
    cause: fleet_lost
    enabled: true               # 原版：本队覆灭 / 玩家舰队解散 → still，不播寿终句
    still: game_over
    after: title
  - key: vanilla:bankrupt
    cause: bankrupt
    enabled: false              # 无「破产」文案
    gold_floor: 0
    require_no_seaworthy_ship: true
    still: game_over
    after: title
  - key: vanilla:manual
    cause: manual
    enabled: true
    still: game_over
    after: title

screen:
  original_fn: fn_14000a894
  title_image_still_index: 5    # TitleSC.dk4；语言包可改
  title_image_surface_index: 4
  size: { w: 1920, h: 1080 }    # 原版 0x780×0x438
```

```yaml
# config/formulas/progression.yaml  片段
# 寿终不新增除法常数。若日后还原 age 式，只加命名参数：
# age:
#   unit: years
#   from: birthday_and_calendar
```

```yaml
# config/ui/strings.zh-CN.yaml  片段（文案包，不进 gameover.yaml）
ending.natural.preamble: "多年来率领{subject}的{leader}……。"
ending.natural.last_breath: "寿终正寝，结束了波澜壮阔的一生。"
ending.natural.era: "从中世纪到近代，给世界带来重大变革的大航海时代……。"
ending.natural.legacy: "留下了一个个的传说，迎向崭新的时代。"
ending.rebuild.prompt: "用{gold}枚金币重建{fleet}舰队吗？"
ending.ship_sunk: "{ship}号沉没了。"            # msg 0x16c
ending.no_crew: "{ship}号连一个水手也没有了！"  # msg 0x16d
person.physical.dead: "死亡"
person.physical.dying: "濒死"
```

JP 句 D 的「――」与 SC「迎向崭新的时代。」不是同一标点；四语言必须同一组 key，译文可以不同。亲笔信句不进 `ending.*`。

`config/ui/scenes.yaml` 已有 overlay `game_over`。补边（实现时写进场景表，不在引擎里再写一套）：

```yaml
overlays:
  - game_over
# ocean / town / combat 的 overlays_allowed 加上 game_over
# game_over 无玩法边；关闭 → title
```

World0 / World1 共用同一份终局策略。剧本若要「某事件强制寿终」，走叙事命令，不复制 YAML。

## 7. 独立旋钮

| 旋钮 | 文件 | 说明 |
| --- | --- | --- |
| `natural_death.auto_from_age` / `age_threshold` | `gameover.yaml` | 关则只靠命令播寿终 |
| `succession.enabled` / `source` / `if_empty` | 同上 | 开继承不必改人物 / 存档布局 |
| `endings[].enabled` | 同上 | 全船不可航、破产默认关 |
| `rebuild.cost` | 同上 | `null` 不出报价 |
| `screen.title_image_*_index` | 同上 | 换 still 不换逻辑 |
| 四句寿终 / 重建问句 | 文案包 | 不进逻辑 YAML |
| `PHYSICALSTATE` 名称 | `characters` | 本系统只订 `PersonDied`，不读名称 |

加第 6 种 ending：表后追加一行（新 `cause` 由发起方传给 `Session.end_scenario`），引擎按表长。禁止在战斗 cpp 里 `if sunk: GameOver()`。

## 8. 对外接口

只传 ID 与值。取不到人 / 舰队：整段 no-op。

**查询**

```
GameOver.resolve(cause) -> ending | none    # 会话层在 end_scenario(game_over, cause) 时调用
GameOver.pending() -> { ending, string_keys[], still_key } | none
GameOver.succession_candidates() -> [person_id]
GameOver.would_end_on(cause) -> bool     # 读 YAML enabled + 当前快照，不写状态
```

人物 / 舰队 / 经济查询仍走原主：`Characters.physical_state`、`Characters.companions`、`Fleets.seaworthy_count`、`Force.gold(force_id)`、`Calendar.age_years`。

**命令**

```
OfferRebuildRejected { fleet_id, reason }         # cannot_afford | declined | cost_unspecified
SucceedAs { person_id }
DismissGameOver {}                                # UI 关闭 still → title
```

**事件**

订阅（别人发）：`PersonDied { person_id, cause, is_protagonist }`（characters，只看主角）、`FleetDestroyed { fleet_id, cause }`（fleets）、`DayStarted`（仅 `auto_from_age`）。

终局入口不是本系统的命令：叙事、人物、舰队一律走 `Session.end_scenario(game_over, cause)`，会话层再调 `GameOver.resolve`。

```
SuccessionOffered { reason, candidates: [person_id] }
Succeeded { old, new, reason }
RebuildOffered { fleet_id, cost }
GameOverDismissed {}
```

`Session.set_protagonist` 是人物 / 会话的命令，本系统调用它，不直接 poke 存档。UI 不得发 `SucceedAs` 以外的写。

战斗：`BattleResolved` 可以带 `flagship_lost`、`player_ships_left`。本系统看 `player_ships_left == 0`，不看伤害公式。

## 9. 未核实字段

- execute id `0x3fff` / opcode `0xC5` imm 1 由哪条事件脚本、在什么条件（岁数？年份？）下发出
- 生日存档偏移；生日是喂寿终、喂星座，还是两者都喂
- `Calendar.age_years` 用年差还是「开局后经过年数」
- `PLAYERCOMPANION_BITS` 宽度与 `person_id` 映射（`aeCompanion` 未读懂）
- `CPlayerPerson` 三段 17 字节、`+0x63..+0x7D` 是否含生日 / 继承人
- `CPlayerPerson::vfunc_05`（`0x1402ad690`）反编译失败
- `fn_14000cf1d(..., 0x39)` 的语义
- 拒绝重建 / 金币 0 与「玩家舰队解散」的先后
- 重建报价公式
- 主角身体态变成死亡（6）是否会触发 `fn_140010578`（未见调用链）
- `PHYSICALLEVEL_*` 整数（`NUM_PHYSICALSTATE=7` 已由取名越界检查钉死）
- World1 是否改寿终句（mesfile 按语言不分世界包）

vanilla 保证：「寿终（`0x3fff` 映射的命令）→ 四句 + still」「玩家本队覆灭 / 玩家舰队解散 → 直接 still」「无继承」「破产终局关」。打开旋钮必须改 YAML，不许在引擎里写隐式默认岁数或隐式继承。
