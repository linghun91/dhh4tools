# 等级、经验、技能

经验是人物成长的唯一存档源。等级与技能值一律派生，禁止再当独立状态写回。

## 1. 职责边界

本系统管：

- 人物两条经验轨道的增减、封顶、溢出语义
- 由经验派生单轨等级、总等级
- 由六维 + 总等级派生 10 项技能值
- 开局「3 种熟练技能」的选择接口（只记账，不解释战斗/贸易怎么用）
- `caps.yaml` / `formulas/progression.yaml` 里与成长有关的参数

不管：

- 六维基础值从哪张人物表来（`characters`）
- 装备、舱室岗位如何改结算属性（`items` / `ships`，只读 `ModifierSet`）
- 何种玩法给哪一条轨道加经验（航海、战斗、委托……调用本系统的 `GrantExp`，本系统不反查来源）
- 技能值如何变成买货折扣、炮术命中（各系统读 `ModifierSet` 或 `skill_value`，不读经验原值）
- 名声、发现称号（`02-fame-titles.md`）
- UI 怎么画六边形与技能条（`12-ui-save`，只读投影）

## 2. 原版事实

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 名册 98 条，步长 `0x19`，基址 `0x2C00` | `[确认]` | `srcdata/include/save.h` `srcdata/docs/save_format.md` `Constants.h` `SAVE_SAILOR_COUNT=98` |
| 名册 `+0x05` 六维各 1 字节：体力 敏捷 魅力 智力 精神 运气 | `[确认]` | `save.h` `AbilityID.h` `Person.cpp` |
| 名册 `+0x0B` / `+0x0F` 两条 `uint32` 经验 | `[确认]` | `save_format.md` `Save.cpp` |
| 主角体另有一份：派系 `0x35AA`、六维 `0x35AC`、经验 `0x35B2` / `0x35B6` | `[确认]` | 同上；状态画面读这份 |
| `save+0x70` 缓存主角**总等级**（两轨之和） | `[确认]` | `save_format.md` `dk4save.c` `write_cached_level` |
| `save+0x20` 主角名册 id（0 拉斐尔 … 5 亚伯拉罕） | `[确认]` | `save.h`；改主角时名册行与 `+0x70` 必须一起写 |
| 已招收：水手派系 == 主角派系；在队：六维之和 > 0 | `[确认]` | `Save.cpp` `dk4save.c` |
| 单轨 `level = isqrt(exp / 30)`，`exp = 30 * level * level` | `[确认]` | `Save.cpp` `level_of_exp` / `exp_of_level` |
| 设计单轨上限 100；工具/补丁硬顶 255 | `[确认]` | `exe_caps.md` `VANILLA_TRACK_LEVEL` `HARD_TRACK_LEVEL` |
| 原版 32 位 `isqrt` 在单轨 > 255 时溢出，画面等级变 0 | `[确认]` | `exe_caps.md` `Save.cpp` 注释；实测存档 255 轨经验为 `30*255*255=1950750` |
| 技能、健康、心情不存档，现场算 | `[确认]` | `Save.cpp` `Person.cpp` |
| `skill = (statA + statB) * (min(level, skill_cap) + 100) / 150`，原版 `skill_cap=200` | `[确认]` | `Save.cpp`；`dk4save.c` 写明 `level` 是**总等级** |
| 技能封顶的实现硬顶是「两段之和 510」 | `[确认]` | `exe_caps.md` `04-id-registry.md` |
| 基础属性成长上限 100；属性结算上限 500 | `[确认]` | `exe_caps.md`（补丁点各一处 / 三处） |
| 存档六维是 `uint8`，物理范围 0–255 | `[确认]` | 名册 6 字节；`Person.h` `LEVEL_MIN/MAX` 挂在 `UCHAR` |
| `NUM_ABILITY=6` `INVALID_ABILITY=-1` | `[确认]` | `AbilityID.h` |
| `NUM_SKILL=10` `INVALID_SKILL=-1` | `[确认]` | `SkillID.h`；`CPersonInformationView` / `CGoodSkillMenuData` 循环 `>= 0xa` |
| 符号 `NUM_SKILLABILITY` 存在，计数值未还原 | `[未知]` | `SkillabilityID.h` `constants.json` |
| `CGoodSkillMenuData` 按 `eSkill` 0–9 取 10 条说明，开局文案「请选择３种熟练的技能」 | `[确认]` | `CGoodSkillMenuData.cpp` `mesfile_sc.txt` `0xb63` |
| 10 条说明在 SC exe `.rdata` 连续存放 | `[确认]` | `DK4HD_sc.exe` 文件偏移 `0x5910c0` 起，见下表 |
| 两轨各自对应航海 / 战斗（或其它玩法） | `[未知]` | 仓库无轨名符号；禁止写成确认 |
| 熟练技能的数值效果（成长加速？初始加成？） | `[未知]` | 只有选 3 个的 UI 与确认句 `是%s﹑%s﹑%s吗？` |

`CGoodSkillMenuData::vfunc_01` 的 10 条说明（`eSkill` 下标 = 字符串顺序）：

| id | 原文 | 短名 `[推断]` | key `[推断]` |
| --- | --- | --- | --- |
| 0 | ＨＰ最大值，对疾病等的抵抗力。 | 医疗 | `medicine` |
| 1 | 舰长、提督的资质，指挥能力。 | 统率 | `leadership` |
| 2 | 驾驶的资质，航海术、造船能力。 | 航海 | `sailing` |
| 3 | 方向感、土地感觉，绘制海图能力。 | 测量 | `survey` |
| 4 | 鉴定商品，物价情报的判断力。 | 会计 | `accounting` |
| 5 | 剑的资质，白刃战的指挥能力。 | 剑术 | `sword` |
| 6 | 炮击的资质，大炮的命中力。 | 炮术 | `gunnery` |
| 7 | 辩论的资质，外交谈判能力。 | 交涉 | `negotiate` |
| 8 | 策略的资质，谍报能力。 | 策略 | `scheme` |
| 9 | 发现的资质，回避危险能力。 | 发现 | `discovery` |

短名与 key 只来自说明文本，**不是**还原出的枚举成员。`stat_a` / `stat_b` 配对未还原。

`CHexagonSprite` 画六维雷达；`CSkillView` / `CSkillValView` 画技能。二者都是投影。

## 3. 实体与 ID

| ID | 原版计数 | 空/非法 | 备注 |
| --- | --- | --- | --- |
| `person_id` | 235（世界表） | `invalid` = -1 | 定义在 `characters`；本系统只认已注册 id |
| `sailor_slot` | 98 | 无「空槽 id」，用六维全 0 表示不在队 | 存档名册下标，与 235 人不是同一集合 |
| `ability_id` | 6 | `invalid` = -1 | 0–5：体力 敏捷 魅力 智力 精神 运气 |
| `skill_id` | 10 | `invalid` = -1 | 0–9 见上表 |
| `exp_track_id` | 2 | 无非法值，只有 0 和 1 | 轨语义未命名 |
| `protagonist_id` | 剧本 0–6 | 存档 `+0x20` 必须 < 98 | 与 `eScenario` 对齐的是人物表 0–6，不是名册长度 |

哨兵：技能/能力查询失败返回 0 并记日志，不用 -1 当技能值（-1 是枚举非法）。

运行时循环读表长，禁止写死 `10` / `6` / `98`。原版计数只作为 vanilla 内容包实例数。

## 4. 算法 / 公式

全部常数进 `config/formulas/progression.yaml` 与 `config/caps.yaml`。代码里不得出现裸 `30`、`100`、`150`、`200`。

### 4.1 单轨等级

```
q            = floor(exp / exp_divisor)          # exp_divisor=30
track_level  = isqrt(q)                          # 整数平方根，向下取整
track_level  = saturate(track_level, 0, level_per_track)
```

`exp < exp_divisor` → 等级 0。

逆变换（写经验、工具、读档校验）：

```
exp_of_level(level) = exp_divisor * level * level
```

原版内容包：`level_per_track=100` 时满级经验 `300000`。硬顶 255 时 `1950750`。

**溢出语义（重构必须与原版分清）：**

| 实现 | `track_level` 超过 255 / 超过设计上限 |
| --- | --- |
| 原版 exe 32 位 `isqrt` | 绕回 **0**（画面级） |
| 重构引擎 | **饱和**到 `min(level_per_track, engine_level_hard)`，禁止绕回 0 |

`engine_level_hard` 默认 255（`UCHAR` / 历史硬顶）。若模组把 `level_per_track` 调过硬顶，启动校验失败，不静默截断。

### 4.2 总等级

```
total_level = track_level(track_0) + track_level(track_1)
```

主角 `save+0x70` 是这份值的缓存。重构：**经验是源**；缓存可有，读档必须能从两条经验重建。UI 与技能公式都读重建后的 `total_level`，不信缓存。

### 4.3 技能值

```
level_used   = min(total_level, skill_cap)       # skill_cap=200
skill_value  = (stat_a + stat_b) * (level_used + skill_level_bias) / skill_divisor
             # skill_level_bias=100, skill_divisor=150
             # 整数除法，与原版一致
```

`stat_a` / `stat_b` 来自 `config/characters/skills.yaml` 的配对（原版 `NUM_SKILLABILITY` 表，字段未还原）。取的是**结算后**属性，已封在 `ability_settled_cap`。

技能输出本身不再另做一层 200 封顶，除非内容包显式加 `skill_value_cap`。原版「技能封顶 200」作用在公式的等级项上。

### 4.4 属性成长与结算

```
grown    = min(base + delta, ability_growth_cap)     # 100
stored   = min(grown, 255)                           # 存档 uint8；重构可把存档加宽
settled  = min(stored + modifiers, ability_settled_cap)  # 500
```

`ability_settled_cap` 被贸易、战斗、道具价共用（`1000-a`、`500-a`、`100-a/50`）。本系统只提供 `settled` 与 `ModifierSet`；各系统不得自己再读六维。

### 4.5 加经验

```
GrantExp(person_id, track_id, amount, source_tag)
  exp' = saturate(exp + amount, 0, exp_of_level(level_per_track))
  重建 track_level / total_level
  失效该人物技能缓存
```

`source_tag` 只用于日志与成就，不进公式。禁止按 `source_tag` 在代码里 `if combat`。哪条轨道由**调用方配置**（`formulas/progression.yaml` 的 `grant_routes`）决定。

原版「经验写入四处相同立即数 `30*cap^2`」见 `exe_caps.md`。重构只保留一处饱和。

## 5. 耦合分析

原版缠在一起的点：

- 技能公式直接吃六维原值 + 总等级，贸易/战斗再吃技能或能力（`exe_caps.md` 的 `Ability(captain, 4)`）
- 主角经验写两份（名册行 + `0x35AA` 体）再加 `+0x70` 缓存，状态画面与名单会不一致
- `NUM_SKILL=10`、`NUM_ABILITY=6`、名册 98 写死在循环和断言
- 等级上限、技能封顶、属性结算上限散落在 exe 立即数，改一处要搜字节
- 健康 / 心情与技能同一套「不存、现算」，但公式未还原
- 开局选技能的文案在 `mesfile`，逻辑在 `CGoodSkillMenuData`，说明字符串在 exe `.rdata` 而不是 mesfile

重构切断方式：人物系统只暴露 `ModifierSet` + `SkillSnapshot`；经验模块不引用 `World*`、不引用港口/舰队指针。

## 6. 解耦后的配置

```yaml
# config/caps.yaml  （片段，vanilla 默认）
schema: dk4.caps
version: 1
level_per_track: 100
skill_cap: 200
ability_growth_cap: 100
ability_settled_cap: 500
sailor_roster: 98
stat_count: 6
skill_count: 10
exp_track_count: 2
```

```yaml
# config/engine_limits.yaml  （片段）
schema: dk4.engine_limits
version: 1
level_hard: 255          # 超过则启动失败，不绕回 0
ability_stored_hard: 255 # 原版 uint8；加宽存档时可升
ability_settled_hard: 65535
exp_hard: 4294967295     # uint32
skill_value_hard: 65535
```

```yaml
# config/formulas/progression.yaml
schema: dk4.formulas.progression
version: 1
level:
  exp_divisor: 30
  overflow: saturate          # 禁止 wrap_to_zero
skill:
  level_bias: 100
  divisor: 150
  level_input: total          # 总等级，不是单轨
grant_routes:                 # 调用方按事件选轨；轨名可配，缺省 0/1
  - event: BattleResolved
    track: 1
    formula: battle_exp       # 具体加多少由战斗系统算完再 GrantExp
  - event: ArrivedAtTown
    track: 0
    formula: sail_exp
# grant_routes 的 event→track 映射在原版未还原前不要当作事实，只是接口形状
```

```yaml
# config/progression/level.yaml
schema: dk4.progression.level
version: 1
tracks:
  - id: 0
    key: track_0
    name: 经验轨 0            # 轨名 [未知]
  - id: 1
    key: track_1
    name: 经验轨 1
starting_good_skills:
  pick: 3                     # mesfile「请选择３种熟练的技能」
  pool: all                   # 从 skill_id 0–9 选
```

```yaml
# config/characters/skills.yaml  （本系统消费，表权属 characters）
schema: dk4.characters.skills
version: 1
- id: 0
  key: medicine               # 短名 [推断]
  name: 医疗
  description_key: skill.0.desc
  stat_a: [未知]
  stat_b: [未知]
# … id 1–9
```

World0 / World1 共用同一套公式；人物初始六维在内容包，不在本文件。

## 7. 独立旋钮

改这些值不应迫使贸易、战斗、UI 改代码：

| 旋钮 | 文件 | 影响 |
| --- | --- | --- |
| `exp_divisor` | `formulas/progression.yaml` | 升级曲线 |
| `level_per_track` | `caps.yaml` | 满级经验、总等级上限 |
| `skill_cap` / `level_bias` / `divisor` | 公式 + caps | 技能数值尺度 |
| `ability_growth_cap` / `ability_settled_cap` | `caps.yaml` | 全游戏能力尺度 |
| `overflow: saturate` | 公式 | 高经验存档行为 |
| `starting_good_skills.pick` | `level.yaml` | 开局选几个 |
| 技能短名 / 说明 | `skills.yaml` + 文案包 | 只影响显示 |
| `grant_routes` | 公式 | 哪类事件进哪条轨 |

显示层自己处理「等级数字几位」，不要用 UI 宽度倒逼 `level_per_track`。

## 8. 对外接口

只传 ID 与值。

**查询**

```
Progression.track_exp(person_id, track_id) -> u64
Progression.track_level(person_id, track_id) -> u32
Progression.total_level(person_id) -> u32
Progression.skill_value(person_id, skill_id) -> u32
Progression.skill_snapshot(person_id) -> { skill_id -> u32 }
Progression.ability_settled(person_id, ability_id) -> u32
Progression.good_skills(person_id) -> [skill_id]   # 开局 3 个，可空
```

取不到人：整体 no-op / 返回 0，不抛进 UI。

**命令**

```
GrantExp { person_id, track_id, amount, source_tag }
SetGoodSkills { person_id, skill_ids[pick] }      # 仅开局/编辑器
RebuildDerived { person_id }                      # 读档、改 caps 后
```

**事件（本系统发出）**

```
LevelChanged { person_id, track_id, old, new, total }
SkillChanged { person_id }                        # 派生值已脏，订阅者重拉 snapshot
```

禁止：UI 写经验；经济写经验；战斗直接改 `save+0x70`。

## 9. 未核实字段

- 两轨的玩法语义（航海 / 战斗 / 其它）与「谁在什么时候加哪一轨」
- 每条技能的 `stat_a` / `stat_b`（`NUM_SKILLABILITY` 表）
- 技能短名是否就是说明里的「医疗、统率、航海…」（说明已确认，枚举名未确认）
- 开局 3 个熟练技能的数值效果
- 健康 / 心情公式（与技能一样不存，公式未还原）
- 名册 `+0x00..+0x02`、`+0x04`、`+0x13..+0x18`（步长 `0x19` 里除派系/六维/两轨经验外的字节）
- 主角体 `0x35AA` 之前、`0x35B6` 之后到名字块 `0x35C3` 之间的字节
- `+0x70` 缓存是否还被除状态画面外的系统信任
- `CPlayerPerson` 运行时对象 `0x30` 与名册 `0x19` 的字段对应（工具注释提到状态画面 0x30 对象）
- `CPlayerPerson::vfunc_05`（`0x1402ad690`，RTC `eAbility`）反编译失败，属性结算细节以 `exe_caps.md` 为准
- `LEVEL_MIN` / `LEVEL_MAX` 的整数（断言只证明挂在 `UCHAR`）
- World 人物行 `stride 0x38` 里除姓名指针和 `+0x20` 六维外，是否还有初始经验 / 熟练技能位
