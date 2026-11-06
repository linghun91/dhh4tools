# 招募与编制

名册（已招航海士集合）、酒馆/旅馆/公会与人物的交界、船上岗位任命、开局三熟练技能、同伴位。水手**人数**（船上 headcount）属船；这里管有名字的航海士。

## 1. 职责边界

本系统管：

- 名册的招入 / 解雇 / 在队判定（名册 = 隶属 == 玩家势力的人物集合，不是定长数组）
- 任命的人物侧校验（`protagonist_only`、门槛、在队）；占用由船侧 `Cabins.assign` 写（06 所有权）
- 开局选择 3 个熟练技能的存储
- `PLAYERCOMPANION_BITS` 在人物侧的语义（哪些人当前随行）

不管：

- 酒馆菜单、酒价、传闻 UI（`08-town-life` 的 `CBar`）
- 旅馆房价与菜单（城镇；人物只收 `Characters.apply_rest`，水手疲劳归舰队 `CFleet+0x71`）
- 公会交涉文书、计略（城镇 / 势力；人物只回答「有没有副官/参谋在岗」）
- 船上水手人数 `SHIP_CREW_MIN/MAX`、水兵室容量（船）
- 地方舰队编制、10 万金币建舰队（舰队）

设施「雇人」是城镇发命令，人物改名册。不要在 `CBar.cpp` 里直接写六维。

## 2. 原版事实

### 名册

- `[确认]` 原版没有独立的「名册数组」。存档里是 235 条 `CPerson` 记录按 `person_id` 排（0–86 完整 25 字节，87–234 只有隶属 + 1 字节），再接主角 `CPlayerPerson`；`SAVE_SAILOR_COUNT = 98`、`+0x2C00` 步长 0x19 是 `dk4save` 起点差 3 字节的视图（字段偏移相抵，ids 0–86 读写正确；行 87–97 会写坏 2 字节区里别人的隶属 / `+0x22`）。详见 `01-person-model.md`。
- `[确认]` 已招收：`person.affiliation(+0x0C) == 主角隶属`。来源：`Save.cpp` 注释、`save_format.md`；引擎 `fn_14000e1ce` 也按「隶属 == 玩家势力」区分。
- `[推断]` 在队：`sum(stats[6]) > 0` 是 `dk4save` 的判据；引擎的在队 / 在船判据未还原（在船走舱室绑定，见 `06`）。
- `[确认]` 主角另有状态画面体（`+0x35AA` 一组）和 `+0x20` 名册 id、`+0x70` 总等级缓存。两边不一致时画面与名单会分叉。
- `[确认]` 95 爱斯法妮雅、96 特欧贝尔德、97 宗九郎世界六维全 0。`[推断]` 他们由事件登场；且 87–234 的六维不进存档，事件若改其六维，读档后会回到世界值（原版行为，需在 `10-narrative` 核对）。
- `[推断]` `person_id >= 98` 的职能 NPC / 事件人 / 首都女性不会被招为航海士。依据：除 199–201（安吉鲁 / 斯耐克 / 丹第，丹第有六维）等少数事件人外六维全 0；大多数无行走图（`05`）。引擎侧未找到显式的 98 上限检查（`CPerson::vfunc_02` 有 `id < 0x62` 分界，用途未读）。
- `[推断]` 167–177「属下」六维全 40，是雇佣无名航海士或 NPC 舰队军官的模板，不是名册里的 11 个固定人。

### 酒馆 / 旅馆 / 公会

- `[确认]` 设施类：`CBar`、`CInn`、`CGuild`（`class_files.tsv`）。构造函数几乎只有 vftable，逻辑在 unassigned。
- `[确认]` 雇佣失败文案（mesfile `0xa82f`–`0xa897`）：「不能再雇用」「船坐不下了。不能再雇用了。」同一带还有买卖对话，需靠场景区分是雇水手还是买货。
- `[确认]` 出海检查：「有的船没有１名水手。要重新雇用水手或者重新编组。」（`0x3eb`）。这是船上 **headcount**，不是名册槽。
- `[确认]` 解雇：「如果解雇全体水手，船就动不了」（`0x1323f`）。仍是 headcount。
- `[确认]` 旅馆：`0xbab7`「住１晚１枚金币，要住几天？」。菜单「住一晚 / 长住 3·10·30 晚」，每晚 `fn_14000af74` = 人物休养 `fn_1400044cb(flag)` + 水手疲劳 `fn_14000efde(flag)` + 走 1 天 + 扣 1 金（`08-town-life/05`、`07-fatigue-health-mood.md` §2.12）。`0xc0b3`「在这里船员身体和精神都能够恢复。」是**舱室**说明，不是旅馆句。
- `[确认]` 公会：交涉文书、停战、策略（`0xad43`、`0xa82f` 一带「出谋划策」）。副官在岗才能发文书（岗位文档）。
- `[确认]` 酒馆还承担计略（参谋在岗，mesfile 策略句）和打听同伴。
- `[推断]` 具名航海士入队发生在酒馆对话（原作惯例 + 酒馆客人职能 NPC 9 个文化圈），不是旅馆睡觉。旅馆主职是恢复。公会主职是文书/委托。三者都可能间接触发 `Characters.recruit`，但雇 **水手人数** 与招 **航海士** 是两条命令。
- `[未知]` 酒馆招某 `person_id` 的出现条件、金价、失败检定：未从 `CBar` 还原。

### 船上任命

- `[确认]` `CCrewView` 是编制 UI（大量布局常数，feature `person`）。
- `[确认]` 任命路径带 `ePerson`、`eFleet`、`eFleetShip`、`eCabinKind`、`eShipCabin`、`abPersonFlag`（`fn_1401dde00` RTC 名）。
- `[确认]` 「请选择要更换舰长的船。」「将舰长委任给属下。」「任命谁为舰长？」（mesfile `0x2ed3`–`0x2eff`）。「提督不担任舰长怎么行？」（`0x80a7`）。「舰长室只能配置%s。」（exe）。
- `[确认]` 无测量员：不能建航线（`0x5d0f` 连打「没有测量员」）。
- `[确认]` 舱室种类名称与岗位名称见 `03-abilities-skills-jobs.md`。布局属船，任命属人物。

### 三熟练技能

- `[确认]` 「请选择３种熟练的技能。」（`0xb63`），确认句「是%s﹑%s﹑%s吗？」（`0xb7b`）。`CGoodSkillMenuData` 按 `eSkill < 10` 列说明。
- `[未知]` 选中之后改公式的哪一项（额外加成、阈值减免、还是仅标记）。技能公式链 `fn_14000e1ce → fn_1400135cf → fn_140009214` 里**没有**读任何「熟练」字段 `[确认]`，所以它不直接加技能值。`CPerson` / `CPlayerPerson` 串行化里也没有 3 个 skill_id 的字段，存档位置未还原（可能在 `CPlayerFleetData` 或成长函数侧）。必须当人物状态字段存，读档后能查到。

### 同伴位

- `[确认]` 符号 `PLAYERCOMPANION_BITS`，与 `FLAGSHIPSTATE_BITS`、`PLAYERSTATE_BITS` 一同写在 `PlayerFleetData.h`。
- `[确认]` 断言：`0 <= eData && eData < _GetBitTotal_BYTE2DWORD(sizeof(PLAYERCOMPANION_BITS))`。来源：`asserts.txt`。
- `[确认]` RTC 局部名 `asCompanion`、`aeCompanion` 出现在舰队每拍调度 `CFleetElement::vfunc_04`（3 日在岗经验，`06` §2.1）与舰队日步 / 休养函数 `fn_1400fa870`、`fn_1401605f0`。
- `[未知]` 位宽、哪一位对应哪个 `person_id`。不能假设 98 人人手一位（sizeof 未知）。叙事「某同伴是否在旗舰」应通过人物系统查询，不让海洋直接解析 bitfield。

## 3. 实体与 ID

| 实体 | 计数 | 说明 |
| --- | --- | --- |
| 名册 | 动态集合 | 隶属 == 玩家势力的 `person_id`；原版无定长槽 |
| 航海士 | 子集 | 隶属=玩家 |
| 在队航海士 | 子集 | 六维和 > 0 |
| 船上水手人数 | 每船一个整数 | **船** 的字段，不是 person |
| 岗位任命 | (ship, cabin) → person_id\|empty | 船 `Cabins` 写占用；人物只校验 |
| 熟练技能 | 每主角 3 个 `skill_id` | 开局写入 |
| 同伴位 | `PLAYERCOMPANION_BITS` | 位域，宽未知 |

空岗位：`null`（`Option<PersonId>`）。不要用 0（0 是拉斐尔）。原版舱室绑定数组的空人物 `0xEC`（236）、空舱种 `0x18`、非岗位舱 `0x11`（`fn_140012dd2`，`03`）都只在 `ids/legacy_codec.yaml` 出现（D1）。

## 4. 算法 / 公式

### 在册 / 在队

```
recruited(p) = roster[p].affiliation == player_force_id
in_party(p)  = recruited(p) and sum(roster[p].abilities) > 0
```

`player_force_id` 来自势力系统；原版存档比较的是各 `CPerson` 记录首字节（dk4save 视图的「名册 `+0x03`」）与主角 `+0x35AA`。

解雇具名航海士：隶属改回中立/原势力，**不要**把六维清零（清零会让事件角色「从未存在」）。六维清零只留给「尚未登场」的 95–97 这类。

雇佣无名模板：从 `crew_template` 人物拷六维与姓名生成规则，生成运行时人物实例（key 用 `session:` 命名空间，随存档 `id_map` 保存，D1/D7）。原版是否真的实例化 167–177 未还原；原版包可以关闭此功能。名册上限若需要，进 `recruitment.yaml` 的 `max_officers`，不与人物表长绑定。

### 水手人数 vs 航海士

```
# 船
ship.crew_count ∈ [SHIP_CREW_MIN, SHIP_CREW_MAX]     # 城镇雇佣/解雇改变这个
# 人物
Cabins.assign(ship_id, cabin_id, person_id)          # 不改变 crew_count
```

出海检查「没有 1 名水手」看 `crew_count`，不看名册是否空。名册空但 crew_count>0 的船仍能开（原版属下水手）；测量/会计等岗位效果另算。

### 任命

```
can_assign(person, cabin_kind):
    post = Characters.post_of_room(cabin_kind)    # posts.yaml 的 room 反查；null = 非岗位舱
    if post and post.protagonist_only and not Characters.is_protagonist(person): refuse
    if post and post.enforce_threshold and Characters.skill(person, post.skill) < post.threshold: refuse
    if person not in_party: refuse
    if already assigned elsewhere: move or refuse（原版行为 [未知]，YAML 选 move）
```

岗位门槛值来自舱室种类表 `+0x12`（测量 120、会计 100…，`03` §3）`[确认]` 数值；原版任命时是否拿它拦人 `[未知]`，故 `enforce_threshold` 缺省 false。

### 熟练三技能

开局写入 `proficient_skills: [k0,k1,k2]`（技能 key），须两两不同、均在 `skills.yaml` 中。效果参数进 `formulas/character.yaml`：

```yaml
# config/formulas/character.yaml  片段（D3 形状）
proficient_skills:
  impl: vanilla.proficient_skills   # 效果未还原；先留空操作。禁止代码发明 +10%。
  params:
    pick: 3
    bonus_skill_points: 0
```

在公式还原前，查询接口仍要能返回这 3 个 id，供 UI 打标。

## 5. 耦合分析

原版：

- 酒馆、交易所文案挤在同一 mesfile 段，「不能再雇用」可能被误接到贸易。
- `CCrewView` 直接碰船舱与人物。
- 同伴位在 `CPlayerFleetData`，舰队日步与在岗经验直接扫 `asCompanion`。
- 「完整存档」只覆盖 id < 87（`CPerson::vfunc_06/07` 写死 `0x57`），工具又按 98 行解析，三个数（87 / 98 / 235）互不相干。

重构事件链：

```
Town(Bar) 雇水手                               → Crew.hire(ship_id, n, reason)（Ships）
Town(Bar).RecruitOfficer { person_id }         → Characters.recruit（成功后发 PersonRecruited）
Town(Inn) Actions.run(inn_*, ctx)              → Characters.apply_rest(fleet, source: inn_single | inn_long, nights)
                                               → Fleets.add_fatigue(fleet, delta, reason)（水手疲劳，07 §2.12）
Town(Guild).NeedAdjutant                       → Fleets.post_holder(fleet, adjutant) != null
tick.yaml 阶段（D4）                            → 问 Characters.aboard()，不读 bitfield
```

## 6. 解耦后的配置

```yaml
# config/characters/recruitment.yaml
schema: dk4.characters.recruitment
version: 1
max_officers: null               # 原版未见上限；「不能再雇用」的条件 [未知]
in_party_rule: stat_sum_gt_zero   # dk4save 判据 [推断]；引擎判据未还原
generic_templates: { by_category: crew_template }   # persons.yaml 显式字段；原版 167–177 只是数据观察（D1）
bar:
  hire_crew_count: true
  recruit_named_officers: true
inn:
  rest_source: [inn_single, inn_long]   # 07 vitals.yaml rest 表
  recruit_named_officers: false
guild:
  requires_post: vanilla:adjutant        # 发文书；Fleets.post_holder 判在岗
  scheme_requires_post: vanilla:staff
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
Characters.assignment(person_id) -> { fleet_id, ship_index, cabin_kind_id } | none   # 只读投影；源在 ships Cabins
Ships.occupants(ship_id, role_key) -> [person_id]            # ships 拥有；按岗位取人用 Fleets.post_holder
Characters.proficient_skills(person_id) -> [skill_key]
Characters.companions() -> [person_id]     # 封装 PLAYERCOMPANION_BITS
```

命令：

```
Characters.recruit(person_id, force_id) / dismiss(person_id)
Cabins.assign(ship_id, cabin_id, person_id) / unassign   # ships 执行；人物校验见 §4
SetCompanion    { person_id, present: bool }
Crew.hire / set / lose(ship_id, n, reason)               # 船拥有，人物不存
```

事件：`PersonRecruited` / `PersonDismissed`、`RosterChanged`、`CompanionChanged`（人物发）；`CabinBound` / `CabinUnbound`（船发，人物订阅）。

海洋 / 战斗需要「现在船上有谁」时走 `Ships.occupants` / `Fleets.post_holder`，禁止持有 `CPerson*` 跨日。

## 9. 未核实字段

- （已解决）dk4save 视图的 `+0x00..+0x02` / `+0x04` / `+0x13..+0x18` 是上一条装备、`+0x22` 字节、HP、身体、精神、本条装备 0–1。不含岗位 / 船 / 熟练技能。
- 酒馆招人条件、费用、同时可招人数。
- 「船坐不下了」是 crew_count 上限还是名册/岗位上限。
- 解雇具名航海士是否清六维、是否留隶属。
- 熟练三技能的数值效果与存档位置。
- `PLAYERCOMPANION_BITS` 宽度与映射。`aeCompanion` 函数体未读懂。
- 开局各剧本自动入队的同伴名单（库拉乌迪等）完整表。
- 属下 167–177 是否真被雇佣流程实例化，还是仅 NPC 舰队使用。
- `CBar +0x18 = 5`（构造函数写死）：像菜单项数，未验证。
