# 招募与编制

名册 98 槽、酒馆/旅馆/公会与人物的交界、船上岗位任命、开局三熟练技能、同伴位。水手**人数**（船上 headcount）属船；这里管有名字的航海士。

## 1. 职责边界

本系统管：

- 98 槽名册的招入 / 解雇 / 在队判定
- 把已招航海士任命到「某舰队某船的某舱室种类」（调用船侧布局）
- 开局选择 3 个熟练技能的存储
- `PLAYERCOMPANION_BITS` 在人物侧的语义（哪些人当前随行）

不管：

- 酒馆菜单、酒价、传闻 UI（`08-town-life` 的 `CBar`）
- 旅馆房价与恢复量曲线（城镇；人物只收「恢复疲劳/身心」命令）
- 公会交涉文书、计略（城镇 / 势力；人物只回答「有没有副官/参谋在岗」）
- 船上水手人数 `SHIP_CREW_MIN/MAX`、水兵室容量（船）
- 地方舰队编制、10 万金币建舰队（舰队）

设施「雇人」是城镇发命令，人物改名册。不要在 `CBar.cpp` 里直接写六维。

## 2. 原版事实

### 名册

- `[确认]` `SAVE_SAILOR_COUNT = 98`，存档 `+0x2C00` 步长 0x19。来源：`Constants.h`、`save_format.md`、`Save.cpp`、`dk4save.c`。
- `[确认]` 水手 id `i` = `person_id i`（0–97）。`kSailorName[98]` 从拉斐尔到宗九郎，与 `PersonID.h` 一致。
- `[确认]` 已招收：`sailor.faction == protagonist.faction`。在队：`sum(stats[6]) > 0`。来源：`Save.cpp` 注释、`save_format.md`。
- `[确认]` 主角另有状态画面体（`+0x35AA` 一组）和 `+0x20` 名册 id、`+0x70` 总等级缓存。两边不一致时画面与名单会分叉。
- `[确认]` 六维全 0 的具名者（95 爱斯法妮雅、96 特欧贝尔德、97 宗九郎）按规则「不在队」，但仍占着 0–97 的 id，可被事件改六维后入队。
- `[推断]` `person_id >= 98` 的职能 NPC / 事件人 / 首都女性永不进入这 98 槽。依据见 `01-person-model.md`。
- `[推断]` 167–177「属下」六维全 40，是雇佣无名航海士或 NPC 舰队军官的模板，不是名册里的 11 个固定人。

### 酒馆 / 旅馆 / 公会

- `[确认]` 设施类：`CBar`、`CInn`、`CGuild`（`class_files.tsv`）。构造函数几乎只有 vftable，逻辑在 unassigned。
- `[确认]` 雇佣失败文案（mesfile `0xa82f`–`0xa897`）：「不能再雇用」「船坐不下了。不能再雇用了。」同一带还有买卖对话，需靠场景区分是雇水手还是买货。
- `[确认]` 出海检查：「有的船没有１名水手。要重新雇用水手或者重新编组。」（`0x3eb`）。这是船上 **headcount**，不是名册槽。
- `[确认]` 解雇：「如果解雇全体水手，船就动不了」（`0x1323f`）。仍是 headcount。
- `[确认]` 旅馆：`0xbab7`「住１晚１枚金币，要住几天？」；`0xc0b3`「在这里船员身体和精神都能够恢复。」
- `[确认]` 公会：交涉文书、停战、策略（`0xad43`、`0xa82f` 一带「出谋划策」）。副官在岗才能发文书（岗位文档）。
- `[确认]` 酒馆还承担计略（参谋在岗，mesfile 策略句）和打听同伴。
- `[推断]` 具名航海士入队发生在酒馆对话（原作惯例 + 酒馆客人职能 NPC 9 个文化圈），不是旅馆睡觉。旅馆主职是恢复。公会主职是文书/委托。三者都可能间接触发 `PersonRecruited`，但雇 **水手人数** 与招 **航海士** 是两条命令。
- `[未知]` 酒馆招某 `person_id` 的出现条件、金价、失败检定：未从 `CBar` 还原。

### 船上任命

- `[确认]` `CCrewView` 是编制 UI（大量布局常数，feature `person`）。
- `[确认]` 任命路径带 `ePerson`、`eFleet`、`eFleetShip`、`eCabinKind`、`eShipCabin`、`abPersonFlag`（`fn_1401dde00` RTC 名）。
- `[确认]` 「请选择要更换舰长的船。」「将舰长委任给属下。」「任命谁为舰长？」（mesfile `0x2ed3`–`0x2eff`）。「提督不担任舰长怎么行？」（`0x80a7`）。「舰长室只能配置%s。」（exe）。
- `[确认]` 无测量员：不能建航线（`0x5d0f` 连打「没有测量员」）。
- `[确认]` 舱室种类名称与岗位名称见 `03-abilities-skills-jobs.md`。布局属船，任命属人物。

### 三熟练技能

- `[确认]` 「请选择３种熟练的技能。」（`0xb63`），确认句「是%s﹑%s﹑%s吗？」（`0xb7b`）。`CGoodSkillMenuData` 按 `eSkill < 10` 列说明。
- `[未知]` 选中之后改公式的哪一项（额外加成、阈值减免、还是仅标记）。存档位置也未还原。必须当人物状态字段存，读档后技能查询能看到。

### 同伴位

- `[确认]` 符号 `PLAYERCOMPANION_BITS`，与 `FLAGSHIPSTATE_BITS`、`PLAYERSTATE_BITS` 一同写在 `PlayerFleetData.h`。
- `[确认]` 断言：`0 <= eData && eData < _GetBitTotal_BYTE2DWORD(sizeof(PLAYERCOMPANION_BITS))`。来源：`asserts.txt`。
- `[确认]` RTC 局部名 `asCompanion`、`aeCompanion` 出现在舰队月结 `CFleetElement::vfunc_04` 与海洋补给函数 `fn_1400fa870`、`fn_1401605f0`。
- `[未知]` 位宽、哪一位对应哪个 `person_id`。不能假设 98 人人手一位（sizeof 未知）。叙事「某同伴是否在旗舰」应通过人物系统查询，不让海洋直接解析 bitfield。

## 3. 实体与 ID

| 实体 | 计数 | 说明 |
| --- | --- | --- |
| 名册槽 | 98（原版包） | `roster[i].person_id = i`（0–97） |
| 航海士 | 子集 | 隶属=玩家 |
| 在队航海士 | 子集 | 六维和 > 0 |
| 船上水手人数 | 每船一个整数 | **船** 的字段，不是 person |
| 岗位任命 | (fleet, ship, cabin_kind) → person_id\|empty | 人物写占用，船提供槽 |
| 熟练技能 | 每主角 3 个 `skill_id` | 开局写入 |
| 同伴位 | `PLAYERCOMPANION_BITS` | 位域，宽未知 |

空岗位：`person_id = invalid`。不要用 0（0 是拉斐尔）。

## 4. 算法 / 公式

### 在册 / 在队

```
recruited(p) = roster[p].affiliation == player_force_id
in_party(p)  = recruited(p) and sum(roster[p].abilities) > 0
```

`player_force_id` 来自势力系统；原版存档比较的是名册 `+0x03` 与主角体 `+0x35AA`。

解雇具名航海士：隶属改回中立/原势力，**不要**把六维清零（清零会让事件角色「从未存在」）。六维清零只留给「尚未登场」的 95–97 这类。

雇佣无名模板：从 `crew_template` 人物拷六维与姓名生成规则进一个空的名册槽。原版 98 槽已被 0–97 占满，模组加大 `sailor_roster` 后多出来的槽才给无名航海士。原版包可以规定「不能雇佣超出 98 个具名 id 的人」，用「不能再雇用」对应。

### 水手人数 vs 航海士

```
# 船
ship.crew_count ∈ [SHIP_CREW_MIN, SHIP_CREW_MAX]     # 城镇雇佣/解雇改变这个
# 人物
assign(person, fleet, ship, cabin_kind)               # 不改变 crew_count
```

出海检查「没有 1 名水手」看 `crew_count`，不看名册是否空。名册空但 crew_count>0 的船仍能开（原版属下水手）；测量/会计等岗位效果另算。

### 任命

```
can_assign(person, cabin_kind):
    post = Ships.post_of(cabin_kind)          # 船表
    if post.protagonist_only and person != protagonist: refuse
    if Characters.skill(person, post.skill) < post.required_skill: refuse
    if person not in_party: refuse
    if already assigned elsewhere: move or refuse（原版行为 [未知]，YAML 选 move）
```

原版阈值未从 exe 钉死，用 `posts.yaml` 的 `required_skill`，缺省 0 表示不拦。

### 熟练三技能

开局写入 `proficient_skill_ids: [s0,s1,s2]`，须两两不同、均 `< NUM_SKILL`。效果参数进 `formulas/character.yaml`：

```yaml
proficient_skills:
  pick: 3
  # 效果未还原；先留空操作。禁止代码发明 +10%。
  bonus_skill_points: 0
```

在公式还原前，查询接口仍要能返回这 3 个 id，供 UI 打标。

## 5. 耦合分析

原版：

- 酒馆、交易所文案挤在同一 mesfile 段，「不能再雇用」可能被误接到贸易。
- `CCrewView` 直接碰船舱与人物。
- 同伴位在 `CPlayerFleetData`，海洋月结直接扫 `asCompanion`。
- 名册 98 与人物表 235  silently 切开，代码却到处用 `ePerson`。

重构事件链：

```
Town(Bar).HireCrewCount { ship_id, delta }     → Ships
Town(Bar).RecruitOfficer { person_id }         → Characters.PersonRecruited
Town(Inn).Rest { days }                        → Characters.RecoverFatigue
Town(Guild).NeedAdjutant                       → Characters.modifiers().diplomacy_ok
Ocean.Tick                                     → 问 Characters 同伴列表，不读 bitfield
```

## 6. 解耦后的配置

```yaml
# config/characters/recruitment.yaml
schema: dk4.characters.recruitment
version: 1
roster_size: 98                  # 原版包；引擎读 caps.sailor_roster
in_party_stat_sum_min: 1         # 原版 > 0
generic_templates: [167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177]
bar:
  hire_crew_count: true
  recruit_named_officers: true
inn:
  rest_recovers_fatigue: true
  rest_recovers_mental_physical: true
  recruit_named_officers: false
guild:
  requires_post: adjutant        # 发文书
  scheme_requires_post: staff
proficient_skill_pick: 3

# 同伴位：叙事给名字，人物给位
# config/narrative/companions.yaml  （叙事拥有 person_id 列表）
# 人物只实现 bit 的读写封装
```

设施是否出现「雇用」按钮由 `town/facilities.yaml` 决定，本文件只声明人物接受哪些命令。

## 7. 独立旋钮

- 名册长度
- 哪些设施能招具名航海士
- 在队判定（六维和 > 0 可改成「隶属且未被停职」）
- 熟练技能个数
- 无名模板的 `person_id` 列表

不要把「船坐不下」做成名册上限——那是 `ship.crew_count` vs 船体容量。

## 8. 对外接口

查询：

```
Characters.roster() -> [person_id]
Characters.in_party() -> [person_id]
Characters.assignment(person_id) -> { fleet_id, ship_index, cabin_kind_id } | none
Characters.occupants(fleet_id, ship_index) -> [{ cabin_kind_id, post_id, person_id }]
Characters.proficient_skills(person_id) -> [skill_id]
Characters.companions() -> [person_id]     # 封装 PLAYERCOMPANION_BITS
```

命令：

```
PersonRecruited { person_id, force_id }
PersonDismissed { person_id }
AssignPost      { person_id, fleet_id, ship_index, cabin_kind_id }
ClearPost       { fleet_id, ship_index, cabin_kind_id }
RecoverFatigue  { person_id | all_in_party, amount }
SetCompanion    { person_id, present: bool }
HireCrewCount   { ship_id, delta }          # 转发给船，人物不存
```

事件：`RosterChanged`、`PostAssigned`、`CompanionChanged`。

海洋 / 战斗需要「现在船上有谁」时走 `occupants`，禁止持有 `CPerson*` 跨月结。

## 9. 未核实字段

- 名册 `+0x00..+0x02`、`+0x04`、`+0x13..+0x18`：可能含当前岗位、所在船、身心、熟练技能，未对上。
- 酒馆招人条件、费用、同时可招人数。
- 「船坐不下了」是 crew_count 上限还是名册/岗位上限。
- 解雇具名航海士是否清六维、是否留隶属。
- 熟练三技能的数值效果与存档位置。
- `PLAYERCOMPANION_BITS` 宽度与映射。`aeCompanion` 函数体未读懂。
- 开局各剧本自动入队的同伴名单（库拉乌迪等）完整表。
- 属下 167–177 是否真被雇佣流程实例化，还是仅 NPC 舰队使用。
- `CBar +0x18 = 5`（构造函数写死）：像菜单项数，未验证。
