# 疲劳、健康与精神

具名航海士的疲劳计量、身体状态、精神状态，以及无名水手的疲劳 / 伤病入口。旅馆只报价；船医和大厨只提供在岗开关；海战只报告谁受伤。主角进入「死亡」之后交给 `11-progression/03-gameover-succession.md`，本系统不播 GameOver 图。

## 1. 职责边界

本系统管：

- 每名在队人物的 `FATIGUE`（独立 UCHAR 计量）
- `ePhysicalState`：健康 / 轻伤 / 重伤 / 濒死 / 疾病 / 死亡
- `eMentalState`：高亢 / 轻松 / 不高兴 / 混乱
- `PHYSICALLEVEL` / `MENTALLEVEL`（UCHAR 计量；与状态枚举的换算未还原）
- 状态画面上的派生「健康 / 心情 / HP」（不进存档）
- 舰队无名水手的疲劳计量，以及坏血病 / 传染病落到人物状态上的**应用**
- 把旅馆、教会、舱室、温泉、船医、大厨、道具的恢复 / 抗病折成命令
- 主角或在队人物进入「死亡」时发事件，不解释继承

不管：

| 事项 | 归属 |
| --- | --- |
| 旅馆房价、住几晚的 UI | town-life `01-facilities.md`（`CInn` 只发 `RestAtInn`） |
| 教会仪式规则 | town-life `03-minigames.md`；休息仍走本系统 |
| 舱室几何、医务室 / 厨房 / 娱乐室坐标 | ships `03-cabins-crew-posts.md` |
| 海上每 tick 何时涨疲劳、坏血病掷骰 | ocean `01-sailing.md` / `02-search-encounters.md`；本系统收事件 |
| 接舷伤害公式、单挑 | combat；本系统收 `ApplyInjury` |
| 水手**人数**、哗变武力镇压 | ships / ocean；不满计量若独立，见第 9 节 |
| 生命力技能怎么从六维算 | `03-abilities-skills-jobs.md`；本系统只读 `vitality_skill` |
| 寿终日历、继承、GameOver still | `11-progression/03-gameover-succession.md` |
| 文案字符串 | ui / localization |

人物对外数值仍只出 `ModifierSet`。伤病通过修正器键进入战斗 / 航海，禁止业务系统读 `ePhysicalState` 下标。

## 2. 原版事实

### 2.1 符号与断言

`[确认]` World DLL 断言（`srcdata/catalog/asserts.txt`）：

| 符号 | 断言 | 宽度 |
| --- | --- | --- |
| `eData` / `idData` / `eVal` ∈ `eMentalState` | `0 <= x < NUM_MENTALSTATE` 或 `INVALID_MENTALSTATE` | `MENTALSTATE_ID` |
| 同上 `ePhysicalState` | `0 <= x < NUM_PHYSICALSTATE` 或 `INVALID_PHYSICALSTATE` | `PHYSICALSTATE_ID` |
| `FATIGUE_MIN <= data <= FATIGUE_MAX` | `FATIGUE_MAX <= sizeof(UCHAR)` | UCHAR |
| `MENTALLEVEL_MIN <= data <= MENTALLEVEL_MAX` | MAX 约束在 UCHAR | UCHAR |
| `PHYSICALLEVEL_MIN <= data <= PHYSICALLEVEL_MAX` | MAX 约束在 UCHAR | UCHAR |

`NUM_MENTALSTATE` / `NUM_PHYSICALSTATE` / 四个 MIN/MAX 的整数均未写进头文件（`MentalstateID.h` `PhysicalstateID.h` 只有 `NUM_*` 与 `INVALID_*=-1`）。`constants.json` 有符号无名。

### 2.2 状态画面字串表

`[确认]` `DK4HD_sc.exe` 文件偏移 `0x57eaf4` 起 8 字节槽连续：

| 偏移 | 字串 |
| --- | --- |
| `0x57eaf4`–`0x57eb1c` | 体力 敏捷 魅力 智力 精神 运气 |
| `0x57eb24`–`0x57eb5c` | 生命力 驾驶术 测量术 会计术 命中力 说服力 谋略术 观察力（缺统率力、剑术，见技能文档） |
| `0x57eb70` / `0x57eb74` | 女 / 男 |
| `0x57eb78`–`0x57eba0` | **健康、轻伤、重伤、濒死、疾病、死亡** |
| `0x57eba8`–`0x57ebc0` | **高亢、轻松、不高兴、混乱** |
| `0x57ebc8` 起 | 总司令、`%s舰长`、`%s号舰` |

六条身体名、四条精神名与 `ePhysicalState` / `eMentalState` 一一对应 `[推断]`（表就在六维 / 技能名后面，槽宽与能力名相同）。身体名里**没有**「疲劳」。

`[确认]` 大地图 HUD 把疲劳画成数字：同文件 `0x57d7dc`「水手」、`0x57d7e4`「`%4d`」、`0x57d7e8`「疲劳」。情报面板 RTC `rcTiredTitle` / `posTired`（`fn_140005f51`，`fn_1402645a0.cpp`）是这块字的排版，不是公式。

### 2.3 存档：健康 / 心情不存

`[确认]` `Person.cpp`：「Skill / HP / mood are computed from stats + level, not stored。」`Save.cpp`：「Derived, not stored: 健康 / 心情 / 技能」。`dk4save.c` 帮助文本重复这一点。

`[确认]` 名册一行 0x19 **已命名**的只有隶属、六维、两段经验（`save_format.md`）。`+0x00..+0x02`、`+0x04`、`+0x13..+0x18` 未命名，**可能**含疲劳 / 身心，未从 `CPerson` 对上（`01-person-model.md` 第 9 节）。

`[确认]` `CPerson::vfunc_06/07` 成对读写：`+0x0C`、`+0x22`、六维 `+0x0F`、两段经验、`+0x20`、`+0x0D`、`+0x0E`、五装备槽 `+0x23`。`vfunc_04` 比较 `+0x0D == 6` 后返回 0/1。6 既是 `NUM_ABILITY` 也可能是 `NUM_PHYSICALSTATE`，不猜。

因此重构把「健康 / 心情 / HP」当派生；把 `FATIGUE` / `PHYSICALLEVEL` / `MENTALLEVEL` 当**可能进存档的计量**，导入原版槽时未命名字节进 `unknown_bytes`，禁止把派生值写回当源。

### 2.4 疲劳文案

SC `mesfile_sc.txt`：

| 偏移 | 句 | 用法 |
| --- | --- | --- |
| `0x343f` | 「疲劳度上升了%s。」「疲劳度恢复了%s。」 | 与温泉经验、拖船同一长块 |
| `0x81cb` | 「水手们好像很疲劳，不让他们休息一下吗？」 | 码头补给 |
| `0xc187` | 「如果有大厨，能节约水和粮食，水手也减轻疲劳。」 | 厨房岗位 |
| `0xc187` | 「船员进到这里能恢复精神，水手也能减少疲劳。」 | 娱乐室 |
| `0x10c9b`–`0x10ceb` | 温泉「解除旅途的疲劳」 | 探索 |
| `0x1dd13` | 某船首像「消除疲劳」 | 道具 |
| `0xcf63`–`0xd0c7` | 「太累了……在“娱乐室”里……。」「过于疲劳了吧？」 | 航海士台词 |

`[推断]` HUD `%4d疲劳` 跟在「水手」后面，是**舰队无名水手**的疲劳；「疲劳度上升了%s」带数量，人物与舰队可能共用 `FATIGUE` 类型。具名航海士另有累/伤/病/死台词，见 2.8。

### 2.5 旅馆 `CInn`

`[确认]` `CInn` 还原 cpp 只有 ctor / dtor，写 vftable，无菜单逻辑。来源：`srcdata/source/game/town/CInn.cpp`（body `0x14015ea70` / `0x14015eaa0` / dtor `0x14015eac0`）。玩法在 `CTownScene` 宿主里（`CTownScene::vfunc_02` RTC 含 `cInn`）。

`[确认]` 资费文案：`0xba97`「要１枚金币。好的，您请休息吧。」`0xbab7`「住１晚１枚金币，要住几天？」`0xbad3`「住%s天吗？」。单价与天数属 town-life；恢复效果属本系统。

`[确认]` `0xc0b3`「在这里船员身体和精神都能够恢复。」同一长块后面才是船木工 / 船医说明，所以这句话是**舱室**（单人房 / 休息向）而不是旅馆专属句。旅馆与该舱室共用「恢复身体 + 精神」出口，恢复量是否相同 `[未知]`。

教会「安安静静地祈祷」（`0xc01f`）也可当休息点（`03-minigames.md`），同样只发本系统的 rest 命令。

### 2.6 船医、大厨、舱室

`[确认]` 岗位名 exe `0x58568c` 含「船医」「大厨」。舱室名 exe `0x52dfbc` 含「医务室」「厨房」「娱乐室」「单人房」。

| 岗位 / 舱 | 文案保证 | 来源 |
| --- | --- | --- |
| 船医 | 「当船员和水手的身体有病时能够治病」 | `0xc0b3` |
| 船医 | 「要是受伤了，我来缠绷带」「打起仗来，船医特别忙」 | `0x18ecb` `0x18077` |
| 大厨 | 节水粮，水手减疲劳 | `0xc187` |
| 单人房类 | 船员身体和精神都能恢复 | `0xc0b3` |
| 娱乐室 | 船员恢复精神，水手减少疲劳 | `0xc187` |
| 传教士 | 消除水手的不满 | `0xc1fb` |

幅度（每晚恢复几点、船医几天治愈、大厨减疲劳系数）全部 `[未知]`。节水粮系数归 ocean 公式包；本系统只认 `ModifierSet.fatigue_gain_factor`。

### 2.7 战斗受伤

`[确认]` 航海士伤台词 `0xd0f3`–`0xd26b`：「这点儿伤……没关系」「血止不住了」「由于这伤，力气……。」濒死 / 死亡 `0xd453`–`0xd5bf`：「身体不能动了……我不行了吗？」「我要死了吗？」

`[确认]` 接舷公式里有 `Ability(*, 5) * (200 - fn_14000ee12)`（`07-combat/02-boarding.md`）。`fn_14000ee12` 本体只是读对象 `+0x71` 的小包装（`fn_1400c4f20.cpp`），情报面板也拿它画 `posTired`——**不能**当成疲劳 getter。战斗是否改 `ePhysicalState` 未从 cpp 还原。

海战水手人数打到 0 是「水手全被杀死了，提督逃到其他船」（`0xec53`），不是人物「死亡」枚举。旗舰进水 / 俘船属 combat。

### 2.8 海上疾病

`[确认]` 遭遇文案（ocean 掷骰，本系统应用）：

| 事件 | 文案 | 效果（文案能看见的） |
| --- | --- | --- |
| 坏血病 | `0x3c2b`–`0x3d6f`「水手们好像染上了坏血病！不赶快靠岸，大家的身体……。」 | 需入港；「全都完了」 |
| 传染病 | `0x481f`–`0x48e3`「发生了传染病！不赶快靠岸就会全死光的！」 | 需入港 |
| 瘟疫（人物） | `0xd29b`「好像染上了什么瘟疫。身体发烧……。」及感冒 / 流行病变体 `0xd2bf`–`0xd3e3` | 身体状态「疾病」`[推断]` |

`[确认]` 生命力说明：「ＨＰ最大值，对疾病等的抵抗力。」（技能 id 0，exe `0x5910c4`）。抗病曲线未还原。

`[确认]` 抗病 / 治疗道具（效果进 items，本系统读修正器）：

| id | 名 | 文案 |
| --- | --- | --- |
| 21 | 莱姆的滴汁 | 「航海中可以为海员治疗坏血病」（`0x19c6f`）；「为防备航海时出现坏血病」（`0x1d08f`） |
| 23 | 华陀的中药 | voyage_charm |
| 99 | 赫洛菲洛斯的医书 | 岗位用具分组 14 `[推断]` |
| 104 | 带纱布的口罩 | 分组 17 `[推断]` |
| 123 | 圣母像 | 「抵御瘟疫」（`04-remodel-guns-supplies.md`） |
| — | 某船首像 | 「好像不会得病」（`0x1dd83`） |

莱姆是消耗护佑，圣母像是船首像，不进人物槽。

### 2.9 死亡与 GameOver

`[确认]` 身体枚举最后一项是「死亡」。人物台词有临终句（2.7）。

`[确认]` mesfile `0x383b`「寿终正寝，结束了波澜壮阔的一生。……留下了一个个的传说，迎向崭新的时代。」与亲笔信句挤在同一长块。这是**剧本结束 / 寿终**文案，日历怎么判寿终、谁能继承，不在本文件。

`[确认]` GameOver 画面：`fn_14000a894`（`0x14011fb70`）RTC `cGameOverStill` `cGameOverSurfaceSprite` `posGameOver` `sizeGameOver` `rcGameOver`，从 `TitleSC.dk4` 取 still。场景表 overlay `game_over`（`12-ui-save/01-scenes-flow.md`）。本系统**不**加载这张图。

主角死亡 → 发 `ProtagonistDied` → 寿终 / 继承 / GameOver 文档接收。属下死亡是否移出名册 `[未知]`。

### 2.10 手册差

`[推断]` 疲劳是独立计量，不是 `ePhysicalState` 的一项：连续身体名无「疲劳」，却有 `FATIGUE_*` 与「疲劳度」文案。官方手册若把疲劳列为身体状态、把精神写成「通常 / 元気」六档，以 exe 字串为准。

## 3. 实体与 ID

| 实体 | 原版 | 原版计数 | 空 / 非法 | 重构 key | 拥有者 |
| --- | --- | --- | --- | --- | --- |
| 疲劳计量 | `FATIGUE_*` | UCHAR 宽 | — | `fatigue` | 人物；舰队水手另见 `crew_fatigue` |
| 身体状态 | `ePhysicalState` | `[推断]` 6 | `INVALID_PHYSICALSTATE=-1` | `physical_state_id` | 人物 |
| 精神状态 | `eMentalState` | `[推断]` 4 | `INVALID_MENTALSTATE=-1` | `mental_state_id` | 人物 |
| 身体计量 | `PHYSICALLEVEL_*` | UCHAR 宽 | — | `physical_level` | 人物 |
| 精神计量 | `MENTALLEVEL_*` | UCHAR 宽 | — | `mental_level` | 人物 |
| HP / 心情 | 无枚举 | 派生 | — | `hp` / `mood` | 运行时，不存 |
| 人物 | `ePerson` | 235 | -1；名册空见 `01` | `person_id` | 人物 |
| 舰队水手疲劳 | HUD `%4d疲劳` | `[未知]` 与人物是否同一上限 | — | `fleet_id` + `crew_fatigue` | 本系统状态，ocean 触发 |

原版包身体状态（顺序 = exe 字串，id 是重构编号）：

| id | key | 名称 |
| --- | --- | --- |
| 0 | `healthy` | 健康 |
| 1 | `light_wound` | 轻伤 |
| 2 | `heavy_wound` | 重伤 |
| 3 | `dying` | 濒死 |
| 4 | `disease` | 疾病 |
| 5 | `dead` | 死亡 |

精神状态：

| id | key | 名称 |
| --- | --- | --- |
| 0 | `elated` | 高亢 |
| 1 | `relaxed` | 轻松 |
| 2 | `unhappy` | 不高兴 |
| 3 | `confused` | 混乱 |

运行时以表长循环，禁止写死 `6` / `4`。`CPerson+0x0D==6` 在整数钉死前不得当 `dead` 哨兵。

恢复来源（不是枚举，是命令 `source`）：

| key | 谁发命令 | 文案保证 |
| --- | --- | --- |
| `inn` | town `RestAtInn` | 住店；身体 + 精神 |
| `church` | town 教会 | 祈祷；是否等同旅馆 `[未知]` |
| `cabin_rest` | ships 单人房占用 tick | 身体 + 精神 |
| `cabin_morale` | ships 娱乐室 | 精神 + 水手疲劳 |
| `hot_spring` | ocean 探索 | 清疲劳；经验另走 progression |
| `surgeon` | 在岗船医 + 日历步 | 治病；战斗中包扎 |
| `cook` | 在岗大厨 + 日历步 | 减水手疲劳（不直接改身体枚举） |
| `item` | items 消耗莱姆 / 中药 | 坏血病 / 疾病 |

## 4. 算法 / 公式

全部进 `config/formulas/vitals.yaml`。代码只留参数名。原版闭合式未还原的项 default `null`，禁止猜 10 / 20 / 100。

### 4.1 派生 HP / 心情

```
hp_max = f_hp(Characters.skill(person_id, vitality_skill), params.hp)
hp     = f_hp_current(hp_max, physical_level, physical_state)   # [未知] 是否即 PHYSICALLEVEL
mood   = f_mood(mental_level, mental_state)                     # [未知]
```

`f_hp` / `f_mood` 未从 cpp 抽出。生命力说明只保证「HP 最大值」与抗病有关。缺省实现：对外查询可以返回 `null` 的 hp，UI 只画已确认的状态名；**禁止**用 `vitality_skill` 直接当 HP。

### 4.2 状态与计量

```
# 计量夹紧
fatigue         = clamp(fatigue, FATIGUE_MIN, FATIGUE_MAX)
physical_level  = clamp(physical_level, PHYSICALLEVEL_MIN, PHYSICALLEVEL_MAX)
mental_level    = clamp(mental_level, MENTALLEVEL_MIN, MENTALLEVEL_MAX)
crew_fatigue    = clamp(crew_fatigue, FATIGUE_MIN, FATIGUE_MAX)

# 枚举是否由计量阈值推出：[未知]
# 若日后还原，用 bands: 表驱动，禁止 if (hp < 30) dead
physical_state = bands.physical.lookup(physical_level)  or  stored enum
mental_state   = bands.mental.lookup(mental_level)      or  stored enum
```

疾病与轻伤同属 `ePhysicalState`，不能用一条 HP 轴表示「既轻伤又生病」。YAML 允许 `disease` **覆盖**伤口，或伤口与疾病分槽；原版是一个枚举还是位域 `[未知]`，默认包按**单枚举**（exe 六名互斥）。

### 4.3 休息

town-life 只传 `nights` 与 `source`：

```
Characters.applyRest { person_ids, source, nights }:
  for p in person_ids:          # 缺省 = 在队
    fatigue        -= rest[source].fatigue * nights * modifiers.fatigue_gain_factor
    physical_level += rest[source].physical * nights
    mental_level   += rest[source].mental * nights
    if rest[source].clear_disease: maybe_clear(physical_state == disease)
    if rest[source].clear_wounds:  maybe_set(physical_state, healthy)
    clamp
  emit VitalsChanged { person_ids }
```

旅馆：`inn_gold_per_night` 在 `formulas/town.yaml`（已有，默认 1）。本系统不扣金币。

`nights_max`、每夜恢复量 `[未知]` → YAML `null`。

### 4.4 海上疲劳与疾病

ocean 每日历步：

```
on Ocean.FatigueTick { fleet_id, base_delta }:
  crew_fatigue += base_delta * cook_fatigue_factor * captain_fatigue_factor
  for officer in occupants(fleet_id):
      officer.fatigue += base_delta * officer.modifiers.fatigue_gain_factor
  if crew_fatigue >= crew_fatigue_warn: emit CrewFatigueWarning

on Ocean.Encounter { kind: scurvy | plague }:
  if modifiers.scurvy_immune: skip          # 莱姆
  if modifiers.plague_immune: skip          # 圣母像等
  resist = Characters.skill(captain, vitality_skill)
  if rand() < disease_chance(resist):       # 式子 [未知]
      apply_disease(fleet_id)               # 水手伤亡归 ships；军官 physical_state=disease
```

船医日历步：`disease → healthy` 的天数 / 概率 `[未知]`，参数名 `surgeon_cure_days`。

无船医时疾病是否自行恶化到死亡 `[未知]`，参数名 `untreated_disease_to_dead_days`，default `null`。

### 4.5 战斗受伤

```
on Combat.ApplyInjury { person_id, grade }:
  # grade 由 combat 按自己的公式给出：light | heavy | dying | none
  # 本系统只做枚举跃迁，不回写伤害数字
  physical_state = max_severity(physical_state, grade)   # 健康<轻伤<重伤<濒死
  emit VitalsChanged { person_id }
  if physical_state == dead:
      emit PersonDied { person_id, cause: combat }
```

`max_severity` 的全序是表，不是代码。疾病与伤口冲突时的规则进 YAML `injury_vs_disease`，原版值 `null`。

船医在战斗中「缠绷带」：是否把 `dying` 拉回 `heavy_wound` `[未知]`。

### 4.6 死亡出口

```
on PersonDied { person_id, cause }:
  physical_state = dead
  if Characters.is_protagonist(person_id):
      emit ProtagonistDied { person_id, cause }
      # 停手。GameOver / 寿终 / 继承不在本系统
  else:
      emit OfficerDied { person_id, cause }
      # 是否 ClearPost + 移出名册：[未知]，YAML officer_death_policy
```

`cause` ∈ `combat | disease | scurvy | plague | succession | other`。`succession` 只允许 progression 回写（寿终），本系统不读日历年龄。

### 4.7 ModifierSet 键（本系统贡献）

人物系统汇总时并入（原则 6）：

```
disease_resist          # 生命力 → 抗病；曲线 [未知]
scurvy_immune           # 莱姆等
plague_immune           # 圣母像等
fatigue_gain_factor     # 大厨 / 舰长 / 甲板；默认 1.0
surgeon_ok              # 船医在岗
rest_cabin_ok           # 单人房 / 娱乐室占用
battle_wounded_factor   # 轻伤/重伤/濒死打战斗；原版乘数 [未知]，缺省 1.0
```

业务系统禁止再读 `physical_state_id`。

## 5. 耦合分析

原版硬编码：

- 旅馆 / 教会 / 舱室说明句直接改「身体 / 精神」，没有 rest 表
- 大地图 HUD 把水手人数和 `%4d疲劳` 画在一起
- 坏血病 / 传染病是 ocean happening 文案，治愈看船医岗位和莱姆道具
- 接舷可能读某个 `+0x71` 字节，语义未钉
- `NUM_PHYSICALSTATE` 写在断言循环，整数未恢复却与六条字串相切
- 主角死亡切 `TitleSC.dk4` GameOver still，与人物对象同一条调用链

允许的依赖方向：

```
town-life  → Characters.applyRest
ocean      → Characters.FatigueTick / ApplyDisease   （事件）
combat     → Characters.ApplyInjury                  （事件）
items      → ModifierSet.scurvy_immune / plague_immune
ships      → 查询 has_surgeon / has_cook；不写疲劳
characters → ProtagonistDied → progression / ui
```

禁止：ocean 改 `CPerson+0x0D`；combat 加载 GameOver；town 自己清疾病；ui 写存档疲劳。

## 6. 解耦后的配置

`config/characters/vitals.yaml` — 状态目录，本系统拥有。

```yaml
schema: dk4.characters.vitals
version: 1

physical_states:
  - id: 0
    key: healthy
    name: 健康
    name_i18n: ui.physical.healthy
  - id: 1
    key: light_wound
    name: 轻伤
  - id: 2
    key: heavy_wound
    name: 重伤
  - id: 3
    key: dying
    name: 濒死
  - id: 4
    key: disease
    name: 疾病
  - id: 5
    key: dead
    name: 死亡
    terminal: true

mental_states:
  - id: 0
    key: elated
    name: 高亢
  - id: 1
    key: relaxed
    name: 轻松
  - id: 2
    key: unhappy
    name: 不高兴
  - id: 3
    key: confused
    name: 混乱

# 单枚举：疾病与伤口互斥。若日后还原为位域，改 mode。
physical_mode: enum          # enum | flags
mental_mode: enum

injury_order: [healthy, light_wound, heavy_wound, dying, dead]
# disease 不参与这条全序

rest_sources:
  inn:          { fatigue: true, physical: true, mental: true, clear_disease: null, clear_wounds: null }
  church:       { fatigue: true, physical: true, mental: true, clear_disease: null, clear_wounds: null }
  cabin_rest:   { fatigue: true, physical: true, mental: true }
  cabin_morale: { fatigue: crew, physical: false, mental: true }
  hot_spring:   { fatigue: true, physical: null, mental: null, grant_exp: true }
  surgeon:      { clear_disease: true, bandage: true }
  cook:         { fatigue: crew }
  item:         { by_item_effect: true }

officer_death_policy: null   # drop_roster | keep_dead | unknown
disease_kinds:
  - key: scurvy
    name: 坏血病
    targets: crew            # crew | officers | both
    immune_modifier: scurvy_immune
  - key: plague
    name: 传染病
    targets: both
    immune_modifier: plague_immune
  - key: illness
    name: 疾病
    targets: officers        # 人物台词「瘟疫 / 感冒」
```

`config/formulas/vitals.yaml` — 只放数。整数未还原保持 `null`。

```yaml
schema: dk4.formulas.vitals
version: 1

fatigue:
  min: null                  # FATIGUE_MIN
  max: null                  # FATIGUE_MAX；硬顶 UCHAR 255 在 engine_limits
crew_fatigue:
  min: null
  max: null
  warn_threshold: null       # 「水手们好像很疲劳」
physical_level:
  min: null                  # PHYSICALLEVEL_MIN
  max: null
mental_level:
  min: null
  max: null

hp:
  skill: vitality_skill
  formula: null              # [未知] 禁止用 skill 值冒充
mood:
  formula: null

rest:
  inn:
    fatigue_per_night: null
    physical_per_night: null
    mental_per_night: null
    nights_min: 1            # 文案「住几天」；上限 [未知]
    nights_max: null
  church:
    fatigue: null
    physical: null
    mental: null
  cabin_rest_per_day: { fatigue: null, physical: null, mental: null }
  cabin_morale_per_day: { crew_fatigue: null, mental: null }
  hot_spring_fatigue: null   # 文案像清零；量 [未知]

sea:
  fatigue_per_day: null
  cook_fatigue_factor: null  # 大厨在岗 < 1；无则 1.0
  captain_fatigue_factor: null
  surgeon_cure_days: null
  untreated_disease_to_dead_days: null
  scurvy_chance: null
  plague_chance: null

injury:
  battle_wounded_factor:
    healthy: 1.0
    light_wound: null
    heavy_wound: null
    dying: null
    disease: null
  bandage_improves: null     # dying -> heavy_wound ?
  injury_vs_disease: null    # keep_injury | overwrite_disease | unknown

bands:
  physical: null             # 若计量→枚举，填 [{max: n, state: key}, ...]
  mental: null
```

`config/caps.yaml` 片段（符号名直到断言给出整数）：

```yaml
fatigue_min: FATIGUE_MIN
fatigue_max: FATIGUE_MAX
physical_level_min: PHYSICALLEVEL_MIN
physical_level_max: PHYSICALLEVEL_MAX
mental_level_min: MENTALLEVEL_MIN
mental_level_max: MENTALLEVEL_MAX
# 硬顶
# engine_limits: uchar_max: 255
```

World0 / World1 共用同一套 vitals / 公式，不按剧本复制。

## 7. 独立旋钮

改这些不应迫使贸易 / 海战 / 航行改代码：

- 增减身体 / 精神档（表长驱动；UI 自己画名）
- 旅馆 / 教会 / 舱室每夜恢复量
- 大厨减疲劳系数、船医治愈日
- 坏血病 / 传染病几率、莱姆 / 圣母像改为哪条 `immune_modifier`
- 伤势对 `battle_wounded_factor` 的乘数
- 属下死亡是留尸还是移出名册
- HUD 疲劳用 3 位还是 4 位（`%4d` 只是原版投影）

会碰到边界、但仍不改外系统代码：

- HP 公式一旦还原 → 只动 `formulas/vitals.yaml`
- 计量改存档 → 只动 save schema 人物节
- 主角死亡演出 → 只动 gameover 文档 / `ui/scenes.yaml` overlay

改生命力技能配对仍走 `skills.yaml`，不在本文件写六维下标。

## 8. 对外接口

查询：

```
Characters.vitals(person_id) -> {
  fatigue, physical_state_id, mental_state_id,
  physical_level, mental_level,
  hp, hp_max, mood          # 未还原公式时 hp/mood 可为 null
}
Characters.crew_fatigue(fleet_id) -> int
Characters.can_act(person_id) -> bool
    # dead / dying 是否禁岗位：YAML；原版 [未知]
Characters.modifiers(person_id) -> ModifierSet
    # 含 disease_resist, fatigue_gain_factor, surgeon_ok, battle_wounded_factor
```

命令（只收 ID 与标量）：

```
applyRest      { person_ids | all_in_party, source, nights }
applyFatigue   { person_id | fleet_id, delta }      # ocean / 温泉
applyDisease   { fleet_id, kind }                   # scurvy | plague | illness
applyInjury    { person_id, grade }                 # combat
clearDisease   { person_id | fleet_id, reason }     # surgeon | inn | item
setPhysical    { person_id, physical_state_id }     # 仅 progression 寿终回写 dead
```

事件：

```
VitalsChanged     { person_id, fatigue, physical_state_id, mental_state_id }
CrewFatigueWarning{ fleet_id, crew_fatigue }
DiseaseStarted    { fleet_id, kind }
DiseaseCured      { fleet_id | person_id, kind, reason }
PersonDied        { person_id, cause }
ProtagonistDied   { person_id, cause }              # progression / ui 订阅
OfficerDied       { person_id, cause }
```

`ProtagonistDied` 的订阅方：

- `11-progression/03-gameover-succession.md` — 寿终、继承、是否允许换提督
- `12-ui-save/01-scenes-flow.md` — overlay `game_over`（`TitleSC.dk4` still）

本系统不 `new` GameOver 精灵，不读 `SGameSeed` 生日。

存档：派生 HP / 心情 / 技能不写。疲劳与身心计量若在原版未命名 6 字节里，导入器原样放 `unknown_bytes`；新存档按本表具名字段写。设施「今晚是否已休息」属 town，不在人物节。

## 9. 未核实字段

| 项 | 依据 | 状态 |
| --- | --- | --- |
| `NUM_PHYSICALSTATE` 正式值 | 六条 8 字节字串 | `[推断]` 6 |
| `NUM_MENTALSTATE` 正式值 | 四条字串 | `[推断]` 4 |
| `FATIGUE_MIN/MAX` 整数 | 仅 UCHAR 宽 | `[未知]` |
| `PHYSICALLEVEL_MIN/MAX` `MENTALLEVEL_MIN/MAX` 整数 | 仅 UCHAR 宽 | `[未知]` |
| 计量与枚举是否双射 | 同时存在 LEVEL 与 STATE | `[未知]` |
| HP / 心情闭合式 | `Person.cpp` 只说 computed | `[未知]` |
| 名册 `+0x13..+0x18`、`CPerson+0x0C/+0x0D/+0x0E/+0x22` | 序列化有、未命名 | `[未知]` 是否即疲劳/身心 |
| `vfunc_04` 的 `+0x0D==6` | 立即数 6 | `[未知]` |
| 人物疲劳 vs 水手 `%4d疲劳` 是否同一上限 | 同符号、两处 UI | `[推断]` 同类型不同实例 |
| 水手「不满」是否独立于疲劳 | 传教士消不满；另有哗变句 | `[未知]`；不与疲劳合并 |
| 旅馆每夜恢复量、`nights` 上限 | 只有 1 金币 / 晚 | `[未知]` |
| 教会休息是否等于旅馆 | 同出口、文案不同 | `[未知]` |
| 单人房 vs 旅馆恢复是否同参 | 同一句「身体和精神」 | `[未知]` |
| 大厨 / 舰长减疲劳系数 | 只有文案 | `[未知]` |
| 船医治愈天数、战斗包扎效果 | 只有文案 | `[未知]` |
| 坏血病 / 传染病掷骰、生命力抗病曲线 | happening 20 槽未抽出 | `[未知]` |
| 无船医是否病死 | 「全死光」是水手人数还是人物死亡 | `[未知]` |
| 接舷是否写 `ePhysicalState` | 伤台词有、公式无 | `[未知]` |
| `fn_14000ee12` | 画 `posTired` 的包装 | **不是**疲劳公式 |
| 属下死亡后名册 | 临终台词有 | `[未知]` |
| 寿终判据（年龄 / 日历 / 生日） | 「寿终正寝」文案 | 归 gameover 文档 |
| 手册 疲劳∈身体、精神 6 档 | 与 exe 4+6 名冲突 | 以 exe 为准 |
| `PLAYERSTATE_BITS` 是否含伤病 | 与同伴位同头文件 | `[未知]`，见名声文档 |
