# 能力、技能、职业与岗位

六维、十技能、岗位定义。舱室在船上怎么摆见 `05-ships-fleet/03-cabins-crew-posts.md`；本文件只定义岗位本身，以及人物系统对外的 **ModifierSet**。

## 1. 职责边界

本系统管：

- 六维定义与上下限（成长帽、结算帽）
- 十技能定义、名称、与两维的配对、计算公式参数
- 岗位（`ePost`）名称、所需技能、岗位效果（作为修正器键）
- 把「六维 + 等级 + 装备 + 身心」收成 ModifierSet

不管：

- 某条船有几间测量室、房间坐标（船）
- `level = isqrt(exp/30)` 的实现（progression 拥有；本系统消费 `level`）
- 买货折扣、炮击命中、交涉成功率的业务公式（各业务系统读 ModifierSet 的键，自己算）
- 职业种类 `eJobKind` 的玩法含义（计数未还原，见第 9 节）

岗位与舱室的接口只有：`cabin_kind_id → post_id` 的对照表在船侧；人物侧不存房间。

## 2. 原版事实

### 六维

- `[确认]` `NUM_ABILITY = 6`，`INVALID_ABILITY = -1`。RTC `eAbility < 6`。来源：`AbilityID.h`。
- `[确认]` 顺序与名称：体力、敏捷、魅力、智力、精神、运气。来源：`Person.cpp`、`save_format.md`、exe 连续字串 `0x57eaf4` 起、`dk4save.c` `kStatName`。
- `[确认]` 基础成长帽 100、结算帽 500；硬顶 255 / 65535。来源：`exe_caps.md`、`id-registry.md`。
- `[确认]` 地方舰队买货用 `Ability(captain, 4)`（下标 4 = 精神）做 `cost * (1000 - a + rand(100)) / 1000`。同类 `500-a`（战斗）、`100-a/50`（道具价）。来源：`exe_caps.md`。这是原版「读生属性」的证据，重构后必须进 ModifierSet。

CGoodSkillMenuData 里混有能力说明口吻的字串，但绑定是 `index < 0x0A`（十技能）。六维没有单独的说明函数被还原；能力说明以技能 tooltip 中间接出现（生命力←体力+精神，等等）。

### 十技能

- `[确认]` `NUM_SKILL = 10`，`INVALID_SKILL = -1`。来源：`SkillID.h`。
- `[确认]` 技能不存档。公式：

  ```
  skill = (stat_a + stat_b) * (min(level, skill_cap) + 100) / 150
  ```

  原版 `skill_cap = 200`，硬顶「两段等级之和 510」。来源：`save_format.md`、`Save.cpp`、`exe_caps.md`、`dk4save.c` 注释。
- `[确认]` `CGoodSkillMenuData::vfunc_01` 按 `eSkill` 0–9 取说明（`index >= 10` 则空）。三条长说明在 rdata，七条内联。从 `DK4HD_sc.exe` 抽出的完整 GBK：

  | id | 说明原文 |
  | --- | --- |
  | 0 | `%6s:ＨＰ最大值，对疾病等的抵抗力。` |
  | 1 | `%6s:舰长、提督的资质，指挥能力。` |
  | 2 | `%6s:驾驶的资质，航海术、造船能力。` |
  | 3 | `%6s:方向感、土地感觉，绘制海图能力。` |
  | 4 | `%6s:鉴定商品，物价情报的判断力。` |
  | 5 | `%4s　:剑的资质，白刃战的指挥能力。` |
  | 6 | `%6s:炮击的资质，大炮的命中力。` |
  | 7 | `%6s:辩论的资质，外交谈判能力。` |
  | 8 | `%6s:策略的资质，谍报能力。` |
  | 9 | `%6s:发现的资质，回避危险能力。` |

- `[确认]` exe 8 字节槽连续技能名：生命力、驾驶术、测量术、会计术、命中力、说服力、谋略术、观察力（缺统率力、剑术两名，这两名分别出现在 `0x53b3f8`「统率力」与状态画面「剑术」）。与上表 10 条说明对齐后的 **eSkill 名称**：

  | id | key | 名称 | 说明摘要 |
  | --- | --- | --- | --- |
  | 0 | `vitality_skill` | 生命力 | HP 上限、抗病 |
  | 1 | `leadership` | 统率力 | 舰长 / 提督 |
  | 2 | `helmsmanship` | 驾驶术 | 航海术、造船 |
  | 3 | `surveying` | 测量术 | 海图、方向 |
  | 4 | `accounting` | 会计术 | 鉴定、物价 |
  | 5 | `fencing` | 剑术 | 白刃、指挥 |
  | 6 | `gunnery` | 命中力 | 炮击命中 |
  | 7 | `persuasion` | 说服力 | 外交谈判 |
  | 8 | `tactics` | 谋略术 | 策略、谍报 |
  | 9 | `observation` | 观察力 | 发现、避险 |

- `[确认]` 新游戏文案「请选择３种熟练的技能。」（mesfile `0xb63`），确认后「是%s﹑%s﹑%s吗？」（`0xb7b`）。UI 类 `CGoodSkillMenuData`。
- `[确认]` 符号 `NUM_SKILLABILITY` / `INVALID_SKILLABILITY` 存在（`SkillabilityID.h`），整数未恢复。技能由两维合成，此枚举即配对表的原版名。
- `[推断]` 公式里的 `level` 是**两段等级之和**（状态画面 `+0x70` 缓存的总等级）。依据：`dk4save.c` 写 `min(total level, skill cap)`；`save_format.md` 只说「等级」。单段 100 时总和最大 200，与原版 `skill_cap=200` 相切。未在反编译里看到取哪一段，不升为确认。
- `[推断]` 技能↔两维配对（玩家手册 / 攻略与说明文字一致，exe 内未找到 10×2 的立即数表，故为推断；**必须进 YAML**，禁止业务代码写死下标）：

  | 技能 | 维 A | 维 B |
  | --- | --- | --- |
  | 生命力 | 体力 | 精神 |
  | 统率力 | 魅力 | 精神 |
  | 驾驶术 | 敏捷 | 精神 |
  | 测量术 | 敏捷 | 智力 |
  | 会计术 | 智力 | 精神 |
  | 剑术 | 体力 | 敏捷 |
  | 命中力 | 敏捷 | 运气 |
  | 说服力 | 魅力 | 运气 |
  | 谋略术 | 智力 | 运气 |
  | 观察力 | 精神 | 运气 |

### 岗位与舱室

- `[确认]` `ePost` / `NUM_POST` 有符号，整数未恢复。`INVALID_POST = -1`。来源：`PostID.h`。
- `[确认]` `eCabinKind` RTC 立即数 23 与 24；赋值默认 `0x18`，`>= 0x17` 则跳过。来源：`num_from_rtc.json`、`fn_1401dde00`。`NUM_SHIPCABIN = 31` 是船上房间**槽位数**，不是岗位种数。来源：`ShipCabinID.h`。
- `[确认]` exe 连续舱室名（从「桅杆」到「无」共 24 个字串）：桅杆、了望台、甲板、测量、船舵、舰长室、副官室、水兵室、武装炮台、物资仓库、货物仓库、划桨手室、单人房、木材室、医务室、厨房、娱乐室、饲养室、礼拜室、参谋室、会计室、追加1、追加2、无。来源：`DK4HD_sc.exe` `0x52dfbc`。
- `[确认]` exe 连续岗位名（`0x58568c` 起）：副官、参谋、会计长、测量员、操帆手、舵手、了望手、冲锋队长、炮手、传教士、船医、大厨、船木工、饲养员、甲板主管。舰长不在这串里；「舰长室只能配置%s。」（`0x53b400`）把舰长单列，官方手册「舰长室（限主角）」。
- `[确认]` mesfile 岗位效果散句：无测量员则不能建航线；船木工航行中修船、造船厂折扣；船医治病；大厨省水和粮食、减疲劳；炮手缩短炮击间隔；冲锋队长提高肉搏；舰长/副官攻城；副官写交涉文书；会计长战斗中无事可做、平时杀价。来源：`mesfile_sc.txt` `0x5d0f` `0xc0b3` `0xc187` `0x160bf` `0x17a83` 等。
- `[确认]` `eJobKind` / `NUM_JOBKIND` 仅有符号。来源：`JobkindID.h`。世界表 `+0x27` 像国籍/文化而非这 15 个岗位（拉斐尔=4 与里斯本 region 4 偶合，赫德拉姆=5 却不是瑞典），不能当成 `ePost`。

### 身心与疲劳

- `[确认]` `FATIGUE_MIN` / `FATIGUE_MAX` 约束在 `UCHAR`。来源：`asserts.txt`。
- `[确认]` 文案「疲劳度上升了%s。疲劳度恢复了%s。」（mesfile `0x343f`）；exe 状态条「%4d疲劳」。旅馆「船员身体和精神都能够恢复」（`0xc0b3`）。
- `[确认]` exe 连续身体字串：健康、轻伤、重伤、濒死、疾病、死亡。连续精神字串：高亢、轻松、不高兴、混乱。来源：`0x57eb78` 起。
- `[确认]` `MENTALLEVEL_MIN/MAX`、`PHYSICALLEVEL_MIN/MAX` 约束在 `UCHAR`。来源：`asserts.txt`。`NUM_MENTALSTATE` / `NUM_PHYSICALSTATE` 整数未恢复。
- `[推断]` 疲劳是独立计量，不是身体枚举的一项（连续身体名里没有「疲劳」，但有独立 FATIGUE 符号与「疲劳度」文案）。官方手册把疲劳列为身体状态之一，与 exe 字串表不一致，以 exe 为准并把手册差记在未核实。

## 3. 实体与 ID

| 实体 | 枚举 | 原版计数 | 空值 | 拥有者 |
| --- | --- | --- | --- | --- |
| 能力 | `eAbility` | 6 | −1 | 人物 |
| 技能 | `eSkill` | 10 | −1 | 人物 |
| 技能维对 | `eSkillAbility` | `[未知]` | −1 | 人物（YAML） |
| 岗位 | `ePost` | `[未知]`（exe 至少 16 个名称：舰长+15） | −1 | 人物 |
| 舱室种类 | `eCabinKind` | RTC 23/24 | 无 / 0x18 `[推断]` | **船** |
| 船上房间槽 | `eShipCabin` | 31 | −1 | **船** |
| 装备槽 | `ePersonEquip` | 5 | −1 | 人物 |
| 性别 | `eSex` | `[推断]` 2（男/女） | −1 | 人物 |
| 精神状态 | `eMentalState` | `[未知]`（exe 见到 4 名） | | 人物 |
| 身体状态 | `ePhysicalState` | `[未知]`（exe 见到 6 名） | | 人物 |

原版内容包给岗位稳定 id（顺序按 exe 字串，舰长另置 0）：

| id | key | 名称 | 主要技能 `[推断]` |
| --- | --- | --- | --- |
| 0 | `captain` | 舰长 | 统率力 |
| 1 | `adjutant` | 副官 | 说服力 |
| 2 | `staff` | 参谋 | 谋略术 |
| 3 | `purser` | 会计长 | 会计术 |
| 4 | `surveyor` | 测量员 | 测量术 |
| 5 | `sail_master` | 操帆手 | 驾驶术 |
| 6 | `helmsman` | 舵手 | 驾驶术 |
| 7 | `lookout` | 了望手 | 观察力 |
| 8 | `boarding_chief` | 冲锋队长 | 剑术 |
| 9 | `gunner` | 炮手 | 命中力 |
| 10 | `missionary` | 传教士 | 说服力 |
| 11 | `surgeon` | 船医 | 观察力 / 生命力 |
| 12 | `cook` | 大厨 | 命中力 |
| 13 | `carpenter` | 船木工 | 测量术 |
| 14 | `keeper` | 饲养员 | 生命力 |
| 15 | `deck_chief` | 甲板主管 | 生命力 |

舱室种类 id 按 exe 字串顺序，**表在船包**，这里只引用。

## 4. 算法 / 公式

全部纯函数，常数进 `config/formulas/character.yaml`。

```
# 技能
effective_level = min(level_for_skill(exp_tracks), params.skill_level_cap)
raw = (ability[stat_a] + ability[stat_b]) * (effective_level + params.skill_level_bias) / params.skill_divisor
skill  = min(raw, params.skill_value_cap)     # 原版未另封技能值；skill_cap 作用在 level 上
```

原版内容包默认：

| 参数 | 原版 | 来源 |
| --- | --- | --- |
| `exp_divisor` | 30 | progression；人物不重复实现 |
| `skill_level_bias` | 100 | `save_format.md` |
| `skill_divisor` | 150 | 同上 |
| `skill_level_cap` | 200 | `VANILLA_SKILL_CAP` |
| `level_for_skill` | 两段之和 `[推断]` | `dk4save.c` |

装备：5 槽的 `item_id` 由物品系统提供效果描述（加减某技能、某修正器）。人物系统求和后写入 ModifierSet，不解释「这把剑为什么加剑术」。

身心：工作效果 × 身心系数。系数表未还原，YAML 留空档，缺省 1.0，禁止代码猜。

### ModifierSet（原则 6）

人物系统**唯一**向经济 / 战斗 / 航海 / 城镇暴露的能力出口：

```
ModifierSet {
  buy_cost_factor,          # 原版约 (1000 - settled_ability[精神] + rng) / 1000
  sell_price_factor,
  item_price_factor,        # 原版约 (100 - a/50) 一类
  battle_offense,           # 原版 500-a 一类的输入
  battle_hit,               # 命中力
  battle_boarding,          # 剑术 / 生命力
  fleet_morale,             # 统率力 → 哗变抗性
  auto_sail_days,           # 驾驶术
  survey_unlock,            # 测量术是否达到建航线阈值
  repair_at_sea,            # 船木工在岗
  food_water_factor,        # 大厨在岗
  fatigue_gain_factor,      # 舰长在岗减疲劳；甲板岗加疲劳
  diplomacy_ok,             # 副官在岗 → 可发交涉文书
  scheme_ok,                # 参谋在岗 → 酒馆计略
  discovery_bonus,          # 观察力 / 了望
  ...
}
```

具体从技能值到 factor 的曲线，原版散落在各系统。重构时：**曲线参数进 `formulas/character.yaml` 的 `modifiers:` 段**，由人物系统算出 factor；业务系统禁止再读六维下标。原版 `1000`、`500`、`50` 作为该段 default。

阈值类（测量员 120、会计长 100 等）来自官方手册而非 exe 立即数，进 YAML 并标 `[推断]`，直到反编译钉死。

## 5. 耦合分析

原版：

- 贸易、战斗、道具价直接取能力下标。
- 岗位效果写在设施对话和战斗 AI 里，没有岗位表。
- `CCrewView` 同时画人、技能、房间。
- 技能公式常数 100、150、200 写在 exe。

允许的依赖方向（原则 9）：`characters ← factions.force_id`；`ships/fleets ← characters.person_id`；`combat ← characters.modifiers`。禁止经济改经验、战斗写市价。

## 6. 解耦后的配置

```yaml
# config/characters/abilities.yaml
schema: dk4.characters.abilities
version: 1
- id: 0
  key: vitality
  name: 体力
- id: 1
  key: agility
  name: 敏捷
- id: 2
  key: charm
  name: 魅力
- id: 3
  key: intelligence
  name: 智力
- id: 4
  key: spirit
  name: 精神
- id: 5
  key: luck
  name: 运气

# config/characters/skills.yaml
schema: dk4.characters.skills
version: 1
- id: 0
  key: vitality_skill
  name: 生命力
  tooltip_key: ui.skill.vitality
  stat_a: vitality
  stat_b: spirit          # [推断] 配对
- id: 3
  key: surveying
  name: 测量术
  tooltip_key: ui.skill.surveying
  stat_a: agility
  stat_b: intelligence

# config/characters/posts.yaml          # 人物拥有岗位定义
schema: dk4.characters.posts
version: 1
- id: 0
  key: captain
  name: 舰长
  protagonist_only: true
  skill: leadership
  required_skill: 0
  modifiers: { fatigue_gain_factor: 0.8, auto_sail_days: true }   # 数值待还原
- id: 4
  key: surveyor
  name: 测量员
  skill: surveying
  required_skill: 120     # [推断] 手册
  modifiers: { survey_unlock: true }

# config/ships/cabins.yaml              # 船拥有；此处只演示对照
# - id: 3
#   key: survey_room
#   name: 测量
#   post: surveyor

# config/formulas/character.yaml
schema: dk4.formulas.character
version: 1
skill:
  level_bias: 100
  divisor: 150
  level_cap: 200
  level_source: total_tracks    # total_tracks | track_max；原版包用前者 [推断]
modifiers:
  buy_cost:
    ability: spirit
    base: 1000
    rand: 100
    # factor = (base - settled(ability) + rng(0..rand-1)) / base
  item_price:
    ability: spirit             # exe 未写清是哪一维，YAML 可改而不改代码
    base: 100
    divisor: 50
```

`config/caps.yaml`：`skill_cap: 200`、`ability_growth_cap: 100`、`ability_settled_cap: 500`、`fatigue` 用符号名直到断言给出整数。

## 7. 独立旋钮

- 十技能改名、改配对两维、改 `level_bias/divisor/cap`
- 岗位改所需技能与阈值
- ModifierSet 某键的曲线（买货从精神改到魅力：只改 YAML）
- 加第 11 个技能（模组）：`skills.yaml` 追加，`posts.yaml` 引用；引擎按表长

改六维**含义**（例如把下标 4 从精神改成别的）只允许改 `abilities.yaml` 的 key 与 `modifiers:` 引用，不允许经济 cpp 出现 `ability[4]`。

## 8. 对外接口

```
Characters.ability(person_id, ability_id) -> int          # 结算后，已封 cap
Characters.skill(person_id, skill_id) -> int              # 派生
Characters.modifiers(person_id) -> ModifierSet
Characters.modifiers_for_post(person_id, post_id) -> ModifierSet
    # 在岗才叠加岗位键；未达 required_skill 则岗位键为中性

# 船侧查询（人物不持有船）
Ships.cabin_layout(ship_id) -> [(cabin_kind_id, post_id, occupant_person_id|empty)]

Commands:
  AssignPost { person_id, fleet_id, ship_index, cabin_kind_id }
    → 人物问船：该船是否有此 cabin_kind
    → 人物检查 required_skill、protagonist_only
    → 成功则发 PostAssigned
```

事件：`PostAssigned` / `PostCleared` `{ person_id, post_id, fleet_id, ship_index }`。

战斗 / 经济订阅 ModifierSet，不订阅六维。

## 9. 未核实字段

- `NUM_POST`、`NUM_JOBKIND`、`NUM_SKILLABILITY`、`NUM_CABINKIND`、`NUM_SEX`、`NUM_MENTALSTATE`、`NUM_PHYSICALSTATE`：有符号无整数。
- 技能维对：exe 未找到 10×2 表，上表为 `[推断]`。
- `level_for_skill` 用总等级还是单段：`[推断]` 总等级。
- 岗位门槛（测量 120、驾驶 60/80 等）：官方手册 `[推断]`，不是 asserts。
- 舰长是否占用 `ePost=0`：名称表从副官起，舰长可能是特例而不是枚举 0。
- `eJobKind` 与世界表 `+0x27`/`+0x28` 的关系：`[未知]`。
- 身心 4/6 名 vs 官方手册 6/6（手册有疲劳、通常、元気）：以 exe 字串为准，手册差待对日文 mesfile。
- `FATIGUE_MIN/MAX` 具体整数：只有符号。
- 熟练三技能的机械效果（加点？阈值减免？）：`[未知]`，见 `04-recruitment-crew.md`。
- HP 从生命力怎么算、心情从精神状态怎么算：`Person.cpp` 只说 computed，公式未还原。
