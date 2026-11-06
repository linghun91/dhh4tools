# 死亡、寿终、继承与 Game Over

本系统判定「这一局是否结束、结束时播哪段终章、能不能把同伴升成新主角」。身体状态枚举本身归人物；金币与船数归经济 / 舰队。这里只做**终局策略**。

经验、等级见 `01-level-exp-skills.md`。发现称号见 `02-fame-titles.md`。身心六字见 `03-characters/03-abilities-skills-jobs.md`。Game Over 画面是 UI 投影，见 `12-ui-save/01-scenes-flow.md`。

## 1. 职责边界

本系统管：

- 终局种类（`ending_id`）：寿终、主角身体死亡、全船不可航、资金穷尽等
- 寿终过场要播的文案 key 与顺序
- 继承策略：候选从哪来、没有候选时是否 Game Over、继承时移交哪些 ID
- 向 UI 发出 `GameOver` / `SuccessionOffered`；不自己画 still

不管：

- `ePhysicalState` 的名称表与系数（人物；本系统只认 key `death`）
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
| exe 连续身体字串：健康、轻伤、重伤、濒死、疾病、死亡 | `[确认]` | `DK4HD_sc.exe` `0x57eb78` 起，8 字节槽；`03-abilities-skills-jobs.md` |
| 紧接着精神字串：高亢、轻松、不高兴、混乱 | `[确认]` | 同上 `0x57eba8` 起 |
| `NUM_PHYSICALSTATE` / `INVALID_PHYSICALSTATE=-1` 有符号，整数未还原 | `[确认]` | `PhysicalstateID.h` `asserts.txt` |
| `PHYSICALLEVEL_MIN/MAX` 约束在 `UCHAR`，整数未还原 | `[确认]` | `asserts.txt` |
| 技能 / HP / 心情不存档，现场算 | `[确认]` | `Person.cpp` `Save.cpp` |
| `CPerson::vfunc_04`：`*(this+0x0D) == 6` 则返回 0，否则 1 | `[确认]` | `CPerson.cpp` body `0x1401ca620` |
| `CPlayerPerson::vfunc_04` 空实现（直接 return） | `[确认]` | `CPlayerPerson.cpp` body `0x1402aa3a0` |
| 6 既是 `NUM_ABILITY` 也可能是状态枚举 / 死亡哨兵 | `[未知]` | `01-person-model.md` 已禁止猜 |
| 名册 `0x19` 步长里身心字节 | `[未知]` | 名册 `+0x13..+0x18` 未命名 |

「死亡」是人物状态机上的一格，船员可以处于死亡。寿终过场是另一条、对着**当前主角**的终章，不要合成一个函数。

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
| 存档 `+0x20` 是当前主角名册 id，改主角必须连同名册行与 `+0x70` 一起写 | `[确认]` | `save_format.md` `Save.cpp` `01-person-model.md` |

`%s成为伙伴` 在字串表里紧挨寿终，但 JP 把它和「得到了 %s」捆在一起，语义更像入队 / 入手，不是「要不要以同伴继续」。

**能否以同伴继续玩：** 仓库**不能确认**。寿终 B–D 读起来是终章旁白（「迎向崭新的时代 / 次の新しき世代へと語り継がれていく」），没有选择 UI 类、没有 succession 符号。同时存在同伴位和可切换的主角 id。重构把继承做成显式旋钮，禁止把传闻写进 vanilla 默认事实。

### 2.4 Game Over 画面

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| RTC：`cGameOverStill` `cGameOverSurfaceSprite` `rcGameOver` `posGameOver` `sizeGameOver` | `[确认]` | `rtc_names.txt` `unassigned_rtc.md` |
| 函数 `fn_14000a894` body `0x14011fb70` size `0x424` | `[确认]` | `fn_140115330.cpp` `functions.tsv` |
| still 从 `TitleSC.dk4` **图 5** 加载；surface 从同包 **图 4** 加载 | `[确认]` | 同上，`fn_140006faa(..., 5)` / `(..., 4)` |
| 主表面尺寸立即数 `0x780` × `0x438`（1920×1080） | `[确认]` | `fn_140006596(0x780, 0x438, ...)` |
| 进入前 `dwCancel = 0xf`，并 `fn_14000cf1d(..., 0x39, 2, 1)` | `[确认]` | 同函数；`0x39` 语义 `[未知]`（BGM / 画面态） |
| 调用点 `fn_14000e8a4` body `0x140112820`：宿主 `+0x88 == 3` 走 Game Over | `[确认]` | `fn_14010be70.cpp` |
| 若 `+0x90 == 1`，先连续 `fn_14000fb14(0x170..0x173)` 四次，再进 still | `[确认]` | 同上 |
| `0x170..0x173` 不是 mesfile 顺序下标（下标 0x170 是广场「景气？」闲话） | `[确认]` | 对 `mesfile_sc.txt` 顺序索引；`fn_14000fb14` RTC 名 `vl` |
| 播完后若 `+0x88 != 5` 则写成 `0` | `[确认]` | `fn_14000e8a4` |
| `+0x88 == 4` 走另一支 `fn_14000b677`，不是 Game Over still | `[确认]` | 同函数 |
| 标题场景同样从 `TitleSC.dk4` 取图 2/3/6/7/8 | `[确认]` | `fn_1401b9940.cpp` `fn_1401200a0.cpp` |
| 场景表已把 `game_over` 登记为 overlay | `[确认]` | `12-ui-save/01-scenes-flow.md` |
| 无「ゲームオーバー / Game Over」mesfile 句 | `[确认]` | JP/SC 全文检索；画面是图，不是字 |

谁把 `+0x88` 写成 3：**未还原**。本系统不猜测「沉船写 3、寿终写 3」。重构用事件，不复刻这个字节。

### 2.5 沉船、无水手、重建——不是已经钉死的 Game Over

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| JP：`%sが沈没しました！` @ `0x4f93`；`%sの水夫が１人もいません！` @ `0x4fa7` | `[确认]` | JP mesfile |
| SC 粘成 `0x37e7`：`%s号沉没了。%s号连一个水手也没有了！得到了%s。` | `[确认]` | `mesfile_sc.txt`；「得到了」对应 JP 下一句「手に入れました」，是另一条 |
| 海战：旗舰被击、浸水停不住；水手全灭则「请到其他船上躲避」 | `[确认]` | SC `0xebb3`–`0xecfb`；JP `提督、旗艦がやられました` |
| 旗舰耐久 **或** 水手 `<= 30` 则该侧溃败 | `[确认]` | `07-combat/01-naval-battle.md` `fn_14000d7ab` |
| 「用%s枚金币重建%s舰队吗？」 | `[确认]` | SC `0x37a7`；JP `金貨%s枚で、\n%s艦隊を再建しますか？` @ `0x4f34` |
| 同粘连块里还有「%s解散了」「等级上升」 | `[确认]` | SC `0x37a7`；JP 拆开 `@0x4f58` |
| 拒绝重建 / 付不起之后是否进 `fn_14000a894` | `[未知]` | 调用链未接到沉船函数 |

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
| `ending_id` | 无枚举；重构稳定 id | `invalid` | **本系统** |
| `person_id` | 235 | −1 | 人物 |
| `physical_state_id` | `ePhysicalState`，计数 `[未知]`（exe 6 名） | −1 | 人物 |
| `protagonist_id` | 存档 `+0x20`，名册 0–97 | 非法则拒绝读档 | 人物 / 存档 |
| `companion` | `PLAYERCOMPANION_BITS`，宽 `[未知]` | 无单独空 id | 人物（封装 bit） |
| `fleet_id` / `fleet_ship_id` | 69 / 5 | −1；槽内空船 `0xD0` | 舰队 |
| `scene_id` | `game_over` overlay | — | UI |
| `string_key` | mesfile 句 | 缺 key = 加载失败 | 文案包 |

运行时按表长循环。原版没有 `eGameOver` / `NUM_ENDING`。重构 ending 用字符串 key，导入器不必发明原版整数。

exe 身体名顺序只作 vanilla 内容包的 **key 顺序**，整数下标在 `NUM_PHYSICALSTATE` 还原前标 `[推断]`：

| 顺序 | key | 名称 |
| --- | --- | --- |
| 0 | `healthy` | 健康 |
| 1 | `light_wound` | 轻伤 |
| 2 | `heavy_wound` | 重伤 |
| 3 | `dying` | 濒死 |
| 4 | `disease` | 疾病 |
| 5 | `death` | 死亡 |

本系统只消费 `death`（以及是否当前 `protagonist_id`）。禁止业务代码写 `== 5`。

建议的 `ending_id`（重构目录，不是原版枚举）：

| id | key | 原版对应 |
| --- | --- | --- |
| 0 | `natural_death` | 寿终四句 + Game Over still |
| 1 | `protagonist_physical_death` | 身体态 `death` 落在主角上；过场文案 `[未知]` |
| 2 | `all_ships_unseaworthy` | 沉船 / 无水手之后的合成条件；原版是否进 still `[未知]` |
| 3 | `bankrupt` | 无「破产」文案；默认 **关闭** |
| 4 | `manual` | 调试 / 剧本指令 |

## 4. 算法 / 公式

无原版可引用的岁数式、破产式。禁止把 `60`、`70` 写进引擎。常数进 YAML；缺字段 = 该触发关闭，不 fallback。

### 4.1 身体死亡（人物先判，本系统后收）

```
Characters.set_physical_state(person_id, death)
  → emit PersonPhysicalDied { person_id }

on PersonPhysicalDied:
  if person_id != protagonist_id:
    本系统 no-op          # 船员死亡不是终局
  else if succession.enabled and has_candidate():
    emit SuccessionOffered { reason: physical_death, candidates }
  else:
    BeginEnding { ending_id: protagonist_physical_death }
```

`CPerson+0x0D == 6` 在还原之前**不要**当「已死亡」用。人物系统用 `physical_state` key。

### 4.2 寿终

```
# 仅当 natural_death.auto_from_age == true 且 age_threshold 为整数
age = Calendar.age_years(person_id)      # 世界/叙事提供；公式未还原则该查询失败
on CalendarAdvanced:
  if auto_from_age and age >= age_threshold and person_id == protagonist_id:
    emit NaturalDeathDue { person_id }

on NaturalDeathDue / 命令 TriggerNaturalDeath:
  play cinematic keys [preamble, last_breath, era, legacy]   # 句 A–D
  if succession.enabled and has_candidate():
    emit SuccessionOffered { reason: natural_death, candidates }
  else:
    BeginEnding { ending_id: natural_death }
```

vanilla 包：`auto_from_age: false`（阈值未知）。寿终仍可通过叙事 / 调试命令触发，这样不靠猜岁数也能播四句旁白。

`Calendar.age_years` 失败（生日偏移未导入）→ 整段 no-op，不把日历年当成年龄。

### 4.3 继承

```
has_candidate() =
  succession.enabled
  AND len(candidates()) > 0

candidates() =
  for p in Characters.companions():          # 封装 PLAYERCOMPANION_BITS
    if require_in_party and not in_party(p): skip
    if Characters.physical_state(p) == death: skip
    yield p

SucceedAs { heir_id }:
  if heir_id not in candidates(): no-op
  old = protagonist_id
  Characters.set_physical_state(old, death)   # 寿终者退出可操作集
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

`SucceedAs` 与开局选主角共用 `Session.set_protagonist`。原版改 `+0x20` 必须同步名册与 `+0x70`；重构只写一处源。

### 4.4 全船不可航

```
on FleetWiped { fleet_id } / ShipsUnseaworthy { fleet_id }:
  if fleet_id != player_fleet: no-op
  n = Fleets.seaworthy_count(player_fleet)    # 船体耐久>0 且允许作旗舰
  if n > 0: no-op                             # 战斗「逃到其他船」
  if rebuild.enabled and Economy.gold() >= rebuild.cost:
    emit RebuildOffered { cost }              # 文案「用%s枚金币重建%s舰队吗？」
    # 接受 → Fleets.rebuild；本系统结束
    # 拒绝或付不起 → BeginEnding { all_ships_unseaworthy } 仅当该 ending.enabled
  else if endings.all_ships_unseaworthy.enabled:
    BeginEnding { all_ships_unseaworthy }
```

`rebuild.cost` 原版公式 `[未知]`。YAML 无整数则**不出**重建报价，避免引擎猜 100000（那是建地方舰队的文案常数）。

水手人数 = 0 但船还在：船侧发 `CrewEmpty`，不是本系统的 `FleetWiped`。

### 4.5 资金穷尽（默认关）

```
bankrupt_condition =
  endings.bankrupt.enabled
  AND Economy.gold() <= gold_floor
  AND Fleets.seaworthy_count(player_fleet) == 0
```

vanilla：`endings.bankrupt.enabled: false`。地方舰队赤字走舰队解散，永不进这条。

### 4.6 开画面

```
BeginEnding { ending_id }:
  emit GameOver { ending_id, string_keys[], still_key }
  UI 切 overlay game_over
  关闭后 Scene.Enter(title)     # 非法 scene 则进 title 并报错，与读档失败同策略
```

still 资源走语言包的 `title_texture` + `game_over.still_index`（原版 SC = 5）。文案不进逻辑 YAML。

## 5. 耦合分析

原版缠在一起的点：

- 寿终句、入伙句、沉船句、重建句、亲笔信句在 mesfile 里前后相连，抽出工具还把它们粘成一行
- Game Over still 复用标题图包，函数在 unassigned 里，靠宿主 `+0x88==3` 进入
- 主角 id、名册、总等级缓存三处；继承如果用原版写法会漏写
- `CPerson::vfunc_04` 与 `CPlayerPerson::vfunc_04` 行为不一致
- 地方舰队「钱不够就解散」和玩家终局共用「钱不够」口语
- 同伴位在 `CPlayerFleetData`，海洋月结直接扫 `asCompanion`
- 生日、星座、寿终可能根本不是同一条轴，却都从「请输入生日」进游戏

重构切断：

- 人物只报 `physical_state` 与 `companions()`
- 舰队只报 `seaworthy_count` / `RebuildOffered` 结果
- 经济只报 `gold`
- 日历只报 `age_years`（失败则空）
- UI 只订 `GameOver` / `SuccessionOffered`
- 叙事可发 `TriggerNaturalDeath` / `SucceedAs`，不可写 still

允许的依赖：`progression.gameover ← characters.person_id / physical_state / companions`；`← fleets.seaworthy_count`；`← economy.gold`；`← calendar.age_years`。禁止反向去改市价或经验。

## 6. 解耦后的配置

```yaml
# config/progression/gameover.yaml
schema: dk4.progression.gameover
version: 1

natural_death:
  auto_from_age: false          # 岁数阈值未还原；true 且缺 threshold 则启动失败
  age_threshold: null           # 整数或 null；禁止代码猜 60
  cinematic_keys:
    - ending.natural.preamble   # 多年来率领 {leader} 的 {subject}……。
    - ending.natural.last_breath
    - ending.natural.era
    - ending.natural.legacy

succession:
  enabled: false                # 原版能否继承 [未知]；vanilla 先关
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
  prompt_key: ending.rebuild.prompt

endings:
  - id: 0
    key: natural_death
    enabled: true
    still: game_over            # 语言包 title_texture 内的逻辑名
    after: title
  - id: 1
    key: protagonist_physical_death
    enabled: true
    still: game_over
    after: title
    cinematic_keys: []          # 原版无独立句；空则只出 still
  - id: 2
    key: all_ships_unseaworthy
    enabled: false              # 原版是否进 still [未知]
    still: game_over
    after: title
  - id: 3
    key: bankrupt
    enabled: false              # 无「破产」文案
    gold_floor: 0
    require_no_seaworthy_ship: true
    still: game_over
    after: title
  - id: 4
    key: manual
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
ending.ship_sunk: "{ship}号沉没了。"
ending.no_crew: "{ship}号连一个水手也没有了！"
person.physical.death: "死亡"
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
| `PHYSICALSTATE` 名称 | `characters` | 本系统只认 `death` key |

加第 6 种 ending：表后追加一行，引擎按表长。禁止在战斗 cpp 里 `if sunk: GameOver()`。

## 8. 对外接口

只传 ID 与值。取不到人 / 舰队：整段 no-op。

**查询**

```
GameOver.pending() -> { ending_id } | none
GameOver.succession_candidates() -> [person_id]
GameOver.would_end_on(kind) -> bool     # 读 YAML enabled + 当前快照，不写状态
```

人物 / 舰队 / 经济查询仍走原主：`Characters.physical_state`、`Characters.companions`、`Fleets.seaworthy_count`、`Economy.gold`、`Calendar.age_years`。

**命令**

```
TriggerNaturalDeath { person_id }                 # 叙事 / 调试；person 必须是主角否则 no-op
NotifyPhysicalDied { person_id }                  # 仅人物系统可发
NotifyFleetUnseaworthy { fleet_id }
OfferRebuildRejected { fleet_id, reason }         # cannot_afford | declined | cost_unspecified
SucceedAs { person_id }
BeginEnding { ending_id }                         # 跳过检定，仍校验 endings[].enabled
DismissGameOver {}                                # UI 关闭 still → title
```

**事件**

```
NaturalDeathDue { person_id }
PersonPhysicalDied { person_id }                  # 人物发出，本系统订阅
SuccessionOffered { reason, candidates: [person_id] }
Succeeded { old, new, reason }
RebuildOffered { fleet_id, cost }
GameOver { ending_id }
GameOverDismissed {}
```

`Session.set_protagonist` 是人物 / 会话的命令，本系统调用它，不直接 poke 存档。UI 不得发 `SucceedAs` 以外的写。

战斗：`BattleResolved` 可以带 `flagship_lost`、`player_ships_left`。本系统看 `player_ships_left == 0`，不看伤害公式。

## 9. 未核实字段

- 寿终是否自动发生、岁数阈值、是否只对主角
- 生日存档偏移；生日是喂寿终、喂星座，还是两者都喂
- `Calendar.age_years` 用年差还是「开局后经过年数」
- 寿终之后能否以同伴继续：无问句、无符号、无 cpp 调用链
- `PLAYERCOMPANION_BITS` 宽度与 `person_id` 映射（`aeCompanion` 未读懂）
- `CPerson+0x0D == 6` 是死亡、是 `NUM_ABILITY` 哨兵，还是别的
- `CPlayerPerson` 三段 17 字节、`+0x63..+0x7D` 是否含生日 / 继承人
- `CPlayerPerson::vfunc_05`（`0x1402ad690`）反编译失败
- 宿主 `+0x88` 的 0/3/4/5 枚举名；谁写成 3
- `fn_14000fb14(0x170..0x173)` 与 `fn_14000cf1d(..., 0x39)` 的语义
- 沉船 / 拒绝重建 / 金币 0 是否调用 `fn_14000a894`
- 重建报价公式
- 主角身体死亡（战斗、疾病、濒死）是否也走寿终四句，还是直接 still
- `NUM_PHYSICALSTATE` / `PHYSICALLEVEL_*` 整数
- 名册行里身心字节
- `+0x88 == 4` 那一支是通关、职员表，还是别的终章
- World1 是否改寿终句（mesfile 按语言不分世界包）

在上述项钉死之前：vanilla 只保证「能播寿终四句 + 能出 Game Over still + 继承默认关 + 破产 / 全船终局默认关」。打开旋钮必须改 YAML，不许在引擎里写隐式默认岁数或隐式继承。
