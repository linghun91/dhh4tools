# 全局 key 词表

第三轮定稿。各篇对同一概念用了不同的 key：修正器、技能 / 六维、岗位、舱室种类与 role、caps、配置文件路径、海域、特产品槽字段、tick 阶段、接口名。本篇给每类 key 定**唯一规范名**，附录 A 列「别名 → 规范名」及每个别名出现的 `文件:行`（行号以第三轮开始时为准）。

与其它总览篇的关系：

- 事件 / 查询 / 命令的规范名以 `06-system-interfaces.md` 为准，本篇只补别名出现处（§11、附录 A.10）。
- 配置文件路径以 `05-config-architecture.md` 目录树为准（§9）。
- 数值与原版整数以拥有方文档为准：六维 / 技能 / 岗位 `03-characters/03`，舱室 `05-ships-fleet/03`，身心态 `03-characters/07`，七大海域 `02-factions/06`，caps `13-algorithms/02`。
- 决策依据见 `09-architecture-decisions.md`（D1–D10）。

## 0. 命名规则

| 规则 | 说明 |
| --- | --- |
| 形式 | YAML key、内容 key、修正器量名一律英文蛇形 `[a-z0-9_]+`；可扩展内容表行的 key 在定义与引用处一律写全命名空间 `<pack>:<key>`（§14-4 及其澄清；原「包内可省前缀」作废）；引擎词汇不带命名空间（D1） |
| 一个 key 只属一个 ID 域 | 同一字符串不在两个域里复用。岗位 `lookout` 与舱室「了望台」原来同名，舱室改 `crows_nest`；`staff` / `purser` 同理（§4） |
| 原版整数 | 字段名一律 `vanilla_index`，只供导入 / 导出（`legacy_index`、`id:` 作废）；运行时空值 `null`，哨兵只在 `ids/legacy_codec.yaml` |
| 文案 key | 由内容 key 派生：`<domain>.<key>.name` / `.desc`（`skill.accounting.desc`），不用下标 |
| 修正器 key | `<域>.<量>`。域 = 被修正的量所属的业务域（`skill` `ability` `person` `fleet` `supply` `ship` `combat` `trade` `invest` `request` `factions`）；量名带单位后缀时用 `_pct`（百分数）、`_div`（除数加值）、`_add`（加值） |
| 消费方定名 | 一个量有多个提供方时，key 名以**消费方公式**里的名字为准；提供方只在内容表 `effects` 里引用 |
| 开关 vs 乘数 | 布尔开关走剧本旗标 `Scenario.flag(key)` / `enabled_by: scenario_flag.<key>`；乘除差异走修正器（D5） |

## 1. 六维（`characters/abilities.yaml`）

| vanilla_index | key | 中文 | 修正器 key |
| --- | --- | --- | --- |
| 0 | `stamina` | 体力 | `ability.stamina` / `ability_raw.stamina` |
| 1 | `agility` | 敏捷 | `ability.agility` |
| 2 | `charm` | 魅力 | `ability.charm` |
| 3 | `intelligence` | 智力 | `ability.intelligence` |
| 4 | `spirit` | 精神 | `ability.spirit` |
| 5 | `luck` | 运气 | `ability.luck` |

六维上限 `caps.ability_cap`（100）。`vitality` 是技能 0，不是六维；把 `vitality` 当体力的写法是别名（附录 A.2）。

## 2. 技能（`characters/skills.yaml`）

| vanilla_index (`eSkill`) | key | 中文 | stat_a + stat_b |
| --- | --- | --- | --- |
| 0 | `vitality` | 生命力 | stamina + spirit |
| 1 | `leadership` | 统率力 | charm + spirit |
| 2 | `helmsmanship` | 驾驶术 | agility + spirit |
| 3 | `surveying` | 测量术 | agility + intelligence |
| 4 | `accounting` | 会计术 | intelligence + spirit |
| 5 | `fencing` | 剑术 | stamina + agility |
| 6 | `gunnery` | 命中力 | agility + luck |
| 7 | `persuasion` | 说服力 | charm + intelligence |
| 8 | `tactics` | 谋略术 | intelligence + luck |
| 9 | `observation` | 观察力 | spirit + luck |

技能值 0–`caps.skill_value_cap`（500），修正器 key `skill.<key>`。别名：`vitality_skill`→`vitality`、`seamanship`→`helmsmanship`、`survey`（技能）→`surveying`、`swordsmanship`→`fencing`、`strategy`→`tactics`。

## 3. 岗位（`characters/posts.yaml`，03-characters 拥有）

| vanilla_index (`ePost`) | key | 中文 | room（舱室 key，§4） | skill | threshold（舱表 `+0x12`） | 多人 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | `captain` | 舰长 | `captain_room` | leadership | 30 | 否；限主角 |
| 1 | `adjutant` | 副官 | `adjutant_room` | persuasion | 100 | |
| 2 | `staff` | 参谋 | `staff_room` | tactics | 120 | |
| 3 | `purser` | 会计长 | `purser_room` | accounting | 100 | |
| 4 | `surveyor` | 测量员 | `survey` | surveying | 120 | |
| 5 | `sail_master` | 操帆手 | `mast` | helmsmanship | 80 | |
| 6 | `helmsman` | 舵手 | `helm` | helmsmanship | 60 | |
| 7 | `lookout` | 了望手 | `crows_nest` | observation | 70 | |
| 8 | `boarding_chief` | 冲锋队长 | `marines` | fencing | 90 | 是（接舷收全部） |
| 9 | `gunner` | 炮手 | `gun_deck` | gunnery | 90 | |
| 10 | `missionary` | 传教士 | `chapel` | persuasion | 160 | |
| 11 | `surgeon` | 船医 | `infirmary` | observation | 150 | |
| 12 | `cook` | 大厨 | `galley` | gunnery | 70 | |
| 13 | `carpenter` | 船木工 | `carpentry` | surveying | 130 | |
| 14 | `keeper` | 饲养员 | `livestock` | vitality | 130 | |
| 15 | `deck_chief` | 甲板主管 | `deck` | vitality | 30 | |

原版无岗位 `0x11` 只在 `legacy_codec`。别名（05-ships-fleet/03 用过）：`staff_officer`→`staff`、`sailhand`→`sail_master`、`assault_leader`→`boarding_chief`、`doctor`→`surgeon`。

## 4. 舱室种类与 role（`ships/cabins.yaml`，ships 拥有）

主键 = 舱室种类 key；`role` 是给 `Ships.cabin_count(ship_id, role_key)` 计数用的稳定 key（D2）。

| vanilla_index (`eCabinKind`) | key | 中文 | skill（`+0x10`） | required_skill（`+0x12`） | role | 岗位 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | `mast` | 桅杆 | helmsmanship | 80 | — | sail_master |
| 1 | `crows_nest` | 了望台 | observation | 70 | — | lookout |
| 2 | `deck` | 甲板 | vitality | 30 | — | deck_chief |
| 3 | `survey` | 测量 | surveying | 120 | — | surveyor |
| 4 | `helm` | 船舵 | helmsmanship | 60 | — | helmsman |
| 5 | `captain_room` | 舰长室 | leadership | 30 | — | captain |
| 6 | `adjutant_room` | 副官室 | persuasion | 100 | — | adjutant |
| 7 | `marines` | 水兵室 | fencing | 90 | `marine_room` | boarding_chief |
| 8 | `gun_deck` | 武装炮台 | gunnery | 90 | `gun_room` | gunner |
| 9 | `stores` | 物资仓库 | null | 0 | `supply_room` | — |
| 10 | `cargo` | 货物仓库 | null | 0 | `cargo_room` | — |
| 11 | `rowers` | 划桨手室 | null | 0 | `rower_room` | — |
| 12 | `cabin_single` | 单人房 | null | 0 | — | — |
| 13 | `carpentry` | 木材室 | surveying | 130 | — | carpenter |
| 14 | `infirmary` | 医务室 | observation | 150 | — | surgeon |
| 15 | `galley` | 厨房 | gunnery | 70 | — | cook |
| 16 | `lounge` | 娱乐室 | null | 0 | `recreation_room`（本轮新增） | — |
| 17 | `livestock` | 饲养室 | vitality | 130 | — | keeper |
| 18 | `chapel` | 礼拜室 | persuasion | 160 | — | missionary |
| 19 | `staff_room` | 参谋室 | tactics | 120 | — | staff |
| 20 | `purser_room` | 会计室 | accounting | 100 | — | purser |
| 21 | `extra_1` | 追加1 | null | 0 | — | — |
| 22 | `extra_2` | 追加2 | null | 0 | — | — |

- 合法 23 种（判界 `<0x17`）。原版第 23 行「无」不进表：运行时空槽 `null`，`0x17` / `0x18` 只在 `legacy_codec`。
- 本轮改名：`lookout`→`crows_nest`、`staff`→`staff_room`、`purser`→`purser_room`（与岗位 key 撞名）；03-characters 写的 `vanilla:captain`（舱室）→`captain_room`。
- role 规范集：`marine_room` `gun_room` `supply_room` `cargo_room` `rower_room` `recreation_room`。别名：`oar_room`→`rower_room`、`recreation`→`recreation_room`、`cargo` / `rower`（简写）→`cargo_room` / `rower_room`、`kitchen`→舱室 `galley`。
- 技能求值上下文 `Modifiers.eval("skill.<key>", { person, cabin? })` 里的 `cabin` 是**舱室种类 key**，不是 role；`cabin_role:` 作 ctx 字段是别名。公式 `refs.cabin_role` 只用于计数。

## 5. 身心状态（`characters/vitals.yaml`，03-characters/07 拥有）

| vanilla_index | 身体态 key | 中文 | | vanilla_index | 精神态 key | 中文 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | `healthy` | 健康 | | 0 | `elated` | 高亢 |
| 1 | `fatigued` | 疲劳 | | 1 | `spirited` | 精神 |
| 2 | `light_wound` | 轻伤 | | 2 | `relaxed` | 轻松 |
| 3 | `heavy_wound` | 重伤 | | 3 | `normal` | 普通（初值） |
| 4 | `dying` | 濒死 | | 4 | `unhappy` | 不高兴 |
| 5 | `disease` | 疾病 | | 5 | `confused` | 混乱 |
| 6 | `dead` | 死亡 | | | | |

全仓已一致，本轮无别名。

## 6. 修正器 key（`config/modifiers.yaml`）

### 6.1 贡献写法

```yaml
- { key: fleet.speed, stage: mul, value: { num: 11, den: 10 }, order: 10, scope: fleet_any, stack: once }
```

| 字段 | 规范值 | 别名 |
| --- | --- | --- |
| `key` | 已注册的修正器 key | `mod:`（07-combat） |
| `stage` | `base` / `add` / `mul` / `clamp` / `override` | — |
| `value` | 整数，或 `{ num, den }`（先乘后除，按 key 的 `int_mode` 截断），或 `formula_ref` | `op: frac_of_base / add_frac / mul_frac / frac / sub_frac` + 顶层 `num/den` 或 `value: [n, d]`。`s + s/10` 写 `mul {11,10}`，`x + x/5` 写 `mul {6,5}`（非负整数截断结果相同） |
| `order` | 同 stage 内先后（航速 10 → 20 → 900） | — |
| `scope` | `ship` / `fleet` / `fleet_any` / `person` | `once_per_fleet` → `scope: fleet_any, stack: once` |
| `stack` | `sum`（默认）/ `once` | — |
| `when` | 谓词 | — |
| `source` | 加载器填 `{kind, key}` | — |

提供方可写通配 `skill.*`（身体态乘子）。

### 6.2 规范 key 表

单位「技能值」= 0–`skill_value_cap`。提供方列写内容表；消费方列写读它的公式。

| key | 类型 / 单位 | stage（原版贡献） | 提供方 | 消费方 | owner_system |
| --- | --- | --- | --- | --- | --- |
| `skill.<skill_key>`（10） | int，技能值 | base = `vanilla.skill_v1`；add = 装备（`active_when`）；mul = 身体态；clamp `[0, skill_value_cap]` | characters（base）、`items/effects.yaml`、`characters/vitals.yaml` | 全部业务公式 | characters |
| `ability.<ability_key>`（6） | int，0–`ability_cap` | base = 人物状态；mul = 精神态 | characters、`characters/vitals.yaml` | 技能 base 段 | characters |
| `ability_raw.<ability_key>` | int | 只有 base，禁止挂贡献 | characters | `person.hp_max`、`combat.crit_luck` base | characters |
| `person.hp_max` | int，HP | base = `skill.vitality` 基段（用 `ability_raw`） | characters | characters vitals | characters |
| `fleet.speed` | int，航速（速度条显示值） | mul order 10 `{11,10}` 大鹫像；mul order 20 `{4,5}` 魔鬼像 | `ships/figureheads.yaml` | ocean F-OCN-01 | ocean |
| `fleet.speed_move` | int，实际移动航速 | mul order 900 `{1,3}` 海战态（写速度条之后） | Session 场景态 `scene.battle` | ocean 移动 | ocean |
| `fleet.search_radius_add` | int，航行坐标，default 0 | add 16 海豚像（`stack: once`） | `ships/figureheads.yaml` | ocean F-ADJ-01（派生量 `Ocean.search_radius` 唯一计算方） | ocean |
| `fleet.happening_block` | set\<happening key\> | add `[scurvy, plague]` 圣母像 | `ships/figureheads.yaml` | ocean happening 掷骰 | ocean |
| `fleet.rat_auto_clear` | bool | override true 金黄色的猫 | `items/effects.yaml` | ocean happening `rats` | ocean |
| `fleet.fatigue_days_randomized` | bool | override true 处女像（船首像 6 `[推断]`） | `ships/figureheads.yaml` | ocean 疲劳日增量 | ocean |
| `fleet.sail_handling` / `fleet.helm` / `fleet.navigation` | int，预留 | 原版无贡献 | — | ocean | ocean |
| `supply.food_div` | int，粮除数加值，default 0 | add 2 小猪像（`stack: once`） | `ships/figureheads.yaml` | ocean F-ADJ-03 | ocean |
| `supply.food_stock_pct` | int，%，default 100 | override 95 结局 `rats` | `ocean/happenings.yaml` outcomes.effects | ocean F-ADJ-03 | ocean |
| `supply.water_div` | int，水除数加值，default 0 | add 10 结局 `storm/blizzard/rain/snow` | `ocean/happenings.yaml` | ocean F-ADJ-03 | ocean |
| `ship.durability_max` | int，耐久 | add 10 白鲸像 | `ships/figureheads.yaml` | ships 派生（`05-ships-fleet/01`）；combat 开战快照 | ships |
| `ship.repair_cost` | int，金币 | base = 修理报价；mul = 船木工岗位 + 等级（F-ADJ-04） | characters（`post_holder(carpenter)`） | ships `Yard` 修理、town-life 造船厂 | ships |
| `combat.gun_power` | int，炮威力 | add 2 龙像 | `ships/figureheads.yaml` | combat 炮击 | combat |
| `combat.melee_damage` | int，白刃伤害 | mul `{6,5}` 王者像（只加己方） | `ships/figureheads.yaml` | combat F-CBT-04 | combat |
| `combat.engage_fear_crew_div` | int，除数，default 0 = 无 | override 10（船首像 0xa） | `ships/figureheads.yaml` | combat 开场：对侧每船 `crew -= crew/div`。触发点两处：实时海战入场 `fn_14000974b`（`0x14018ed91`，另把对侧提督精神态置 5）+ 无画面开场 `fn_14000d7ab`（判定 `0x140123df6` / `0x140123e6c`）；接舷不触发（`07-combat/02` §2.7、`07-combat/01` 船首像效果表） | combat |
| `combat.hull_floor_at_zero` / `combat.crew_floor_at_zero` | bool | override 1（魔鬼像：耐久；提督像：耐久 + 水手） | `ships/figureheads.yaml` | combat `CBattleShip` | combat |
| `combat.initiative` | int，技能值 | base = 提督 `skill.leadership` | combat 公式 | combat 无画面结算 | combat |
| `combat.crit_luck` | int，六维值 | base = `ability_raw.luck` | combat 公式 | combat 暴击 | combat |
| `combat.level_bonus` | int | F-ADJ-05 `anBonus` | progression | combat | combat |
| `trade.npc_buy_cost` | int，金币 | base = F-ECO-01（提督会计术） | — | economy NPC / 地方舰队买货 | economy |
| `trade.item_shop_price` | int，金币 | base = F-ECO-02 | — | items 道具店还价 | items |
| `trade.sell_price_pct` | int，%，default 100 | mul `{105,100}` 剧本 2 | `narrative/scenarios.yaml` | economy F-ECO-16 报价 | economy |
| `trade.price_intel_fee_pct` | int，%，default 100 | 原版无贡献 | — | economy 行情费 | economy |
| `factions.influence_gain` | int，影响力增量；ctx `{ force, major_sea, source: fashion_sale \| investment \| remains }` | mul 2：剧本 2 × `source: fashion_sale`；剧本 0 × `source: remains`（首访） | `narrative/scenarios.yaml` | factions `Influence.add` 之前 | factions |
| `invest.military_min_fee_pct` | int，%，default 100 | mul `{8,10}` 剧本 0 | `narrative/scenarios.yaml` | economy 军事投资底价 | economy |
| `request.reward_gold` | int，金币 | mul `{3,2}` 剧本 0（无符号截断） | `narrative/scenarios.yaml` | town-life 委托报酬 | town |

不是修正器的写法：

- `fleet.survey_unlock` → 「有没有测量员」= `Fleets.post_holder(fleet, surveyor) != null`；门槛走 `posts.yaml` 的 `enforce_threshold`。
- `combat.prize_ship_value_bounty` → 布尔开关，`Scenario.flag("prize_ship_value_bounty")`（07-combat/06 已有 `flags`）。
- `military_invest_discount`（剧本旗标）→ 八折是乘数，走 `invest.military_min_fee_pct`（D5）。
- 派生量 `Ocean.search_radius(fleet_id)` 是查询，不是 `fleet.search_radius` 修正器。
- `progression.level_for_skill`（13-algorithms/03 注册表片段）→ 技能公式内 `min(L0+L1, $caps.skill_level_cap)`，由 characters 公式算，不注册为修正器。
- `ships.repair_discount_pct` → 并入 `ship.repair_cost`：船木工按等级登记 `mul (100 - discount)/100` 贡献。

06-system-interfaces「修正器」表（`:251`–`:257`）的 `trade.buy_cost`、`trade.sell_price`、`trade.fashion_influence`、`invest.military_cost`、`request.reward`、`fleet.food_drain`、`fleet.search_radius`、`battle.melee` 都按上表改。

## 7. caps 键（`config/caps.yaml`）

| key | 原版 | 说明 | 别名 |
| --- | --- | --- | --- |
| `level_per_track` | 100 | 单段等级上限 | `track_level_cap` |
| `track_count` | 2 | 经验段数 | — |
| `exp_track_cap` | null（= `exp_of_level(level_per_track)`；原版 300000） | 单段经验上限 | `exp_cap`、`exp_per_track_at_cap` |
| `ability_cap` | 100 | 六维上限 | `ability_growth_cap` |
| `skill_value_cap` | 500 | 技能值上限 | `ability_settled_cap` |
| `skill_level_cap` | 200 | 技能式等级项 `min(L0+L1, ·)` | `skill_cap`；`formulas/character.yaml` 的 `skill.level_cap` 改为引用 `$caps.skill_level_cap` |
| `buy_ability_clamp` / `battle_ability_clamp` | 500 / 500 | 独立于 `skill_value_cap` | — |
| `town_develop` | 9999 | 商业 / 军事发展度上限 | `town_develop_cap`、`town.develop_cap` |
| `share_cap` | 100 | 份额上限 | — |
| `cargo_stack` | 1 | 每格件数 | — |
| `fleet_ships` | 5 | 舰队船位 | — |
| `hold_slots_per_ship` | 5 | 每船交易货格 | `hold_records_per_ship` |
| `cabins_per_ship` | 31 | 每船舱室槽 | — |
| `calendar_ticks_per_day` | 48 | 每日拍数（D4） | `ticks_per_day`（作配置定义时）、`Clock.ticks_per_day`；式内局部变量可留，但注明取自 `$caps.calendar_ticks_per_day` |
| `owned_ships_max` | 92 `[推断]` | 持有船上限（`05-ships-fleet/07`；本轮补入 caps.yaml） | — |
| `gold_max` | 99,999,999 | 势力金币上限（`fn_140002cb6`；本轮补入 caps.yaml） | — |

删除：`sailor_roster`（98 是 dk4save 工具值；原版完整人物记录 87 条只是存档 schema，不进 caps）、`hold_scan`（误读，实为 role `cargo_room`）。`quote.gold_max`（报价夹紧）是公式参数，不是 caps。

## 8. 七大海域（`world/major_seas.yaml`；ID 域由 factions 登记）

| vanilla_index (`major_sea_id`) | key | 中文 |
| --- | --- | --- |
| 0 | `north_sea` | 北海 |
| 1 | `mediterranean` | 地中海 |
| 2 | `africa` | 非洲 |
| 3 | `indian_ocean` | 印度洋 |
| 4 | `se_asia` | 东南亚 |
| 5 | `east_asia` | 东亚 |
| 6 | `new_world` | 新大陆 |

- 全世界视图 `whole_world` 不是海域行（地图帧 7 / 原版哨兵 8）。
- 文件按 05 目录树放 `world/major_seas.yaml`，schema `dk4.world.major_seas`；02-factions/06 写的 `factions/major_seas.yaml`、`dk4.factions.major_seas` 是别名。ID 域登记与 `MajorSea.*` 查询仍归 factions（06「状态所有权」）。
- 别名：`southeast_asia`→`se_asia`；`Factions.major_sea(town)` / `Towns.major_sea(town)`→`MajorSea.of_town(town_id)`。
- NPC 脸分组 `europe` / `africa` / `se_asia`（`factions/npc_faces`）是另一个 ID 域，不是海域。
- 细分海域（原版包为空表）：ID 域 `sea_area`，文件 `world/sea_areas.yaml`，服务 `SeaArea`（06 已登记）。01-world 写的 `sea_zone` / `sea_zones.yaml` / `SeaZone.*` / `EnteredSeaZone` 是别名。事件里的 `sea_area_id` 在原版包恒为 `null`；要七大海域用 `major_sea_id`。

## 9. 配置文件路径与通用字段

路径以 `05-config-architecture.md` 目录树为准。别名：

| 别名 | 规范路径 |
| --- | --- |
| `characters/zodiacs.yaml` | `characters/zodiac.yaml` |
| `characters/companions.yaml` | `narrative/companions.yaml` |
| `characters/modifier_keys.yaml` | `modifiers.yaml` |
| `characters/protagonist_modifiers.yaml`、`factions/scenarios.yaml` | `narrative/scenarios.yaml` 的 `modifiers` / `player_force`（§14-1；`protagonist_force` 作废） |
| `characters/protagonists.yaml` | `narrative/scenarios.yaml` 的 `protagonist`（人物侧不设此表，`03-characters/02`） |
| `world/unlock_maps.yaml` | `discovery/old_maps.yaml`（§14-10） |
| `formulas/economy.yaml` 里的 `fashion_*`、`share_letter_cut`、`letter_*` | `formulas/factions.yaml`（§14-9，消费方 factions） |
| `world/mappoint_kinds.yaml` | `world/map_point_kinds.yaml` |
| `world/sea_zones.yaml` | `world/sea_areas.yaml` |
| `world/adman.yaml` | `town/plaza.yaml` |
| `factions/major_seas.yaml` | `world/major_seas.yaml` |
| `factions/culture_prices.yaml` | `economy/culture_prices.yaml` |
| `factions/town_shares.yaml` | `factions/town_shares_initial.yaml` |
| `economy/town_state_price_mods.yaml` | `economy/town_state_price_rules.yaml` |
| `economy/price_curves.yaml` | `economy/culture_prices.yaml` + `economy/town_state_price_rules.yaml` |
| `items/figureheads.yaml`、`config/figureheads.yaml` | `ships/figureheads.yaml` |
| `ships/types.yaml`、`ships/ship_types.yaml` | `ships/models.yaml` |
| `ships/sails.yaml`、`ocean/crew_sail.yaml` | `ships/rig.yaml` |
| `formulas/fleet.yaml` | `formulas/fleets.yaml` |
| `formulas/adjustment.yaml` | 拆入 `formulas/vision.yaml` / `formulas/supply.yaml` |
| `town/town_facilities.yaml` | `town/town_map_points.yaml` |
| `import/vanilla.yaml` | `ids/legacy_codec.yaml` |
| `ships/crew.yaml#capacity.size_base` | `ships/rig.yaml#crew_per_mast_by_grade`（第五轮；`size_base` 只作公式局部名） |
| 字段 `size_tier`（06-ocean、05/01、05/03） | 船模字段 `models.size_class`（第五轮；`CShipType+0x18`） |
| `rig_penalty_by_size` | `ships/rig.yaml` 的 `rig_penalty_by_grade`（第五轮） |
| caps `person_equip_slots`、`formulas/character.yaml` 的 `ability.params.equip_slots` | `items/effects.yaml` 的 `slot_groups` 槽总数（第五轮；原版 5） |
| 配置键 `modifier_keys` | D3 出口列表 `modifiers: [...]`（第五轮） |
| `unlock_tables` 行 key `vanilla_0..3` | `vanilla:map_point_unlock_0..3`（第五轮） |
| 公式 `refs: { holder: fleet.admiral }` | `inputs` 由调用方取 `Fleets.admiral(fleet_id)`；`refs` 只放内容 key（第五轮） |

第三轮补登进 05 目录树的文件（不是别名；第四轮已全部在树里）：

| 文件 | 出现处 | 落点 |
| --- | --- | --- |
| `town/schemes.yaml` | `08-town-life/04`:596；`14-config-schemas/README.md`:382；`README.md`:128 | 补入 05 树（24 行计略槽定义窗） |
| `ui/sea_map.yaml` | `01-world/08`:352 | 补入 05 树（海域图图标） |
| `fleets/post_rules.yaml` | `05-ships-fleet/02`:164 | 补入 05 树：`holder_rule`（玩家旗舰取岗位 / 他船取船长 / 其它取提督）全局共享规则 |
| `economy/town_develop_initial.yaml` | `01-world/02`:181 | 发展度归 world `Towns`（06），改放 `world/towns.yaml` 的 `develop_initial` 列 |
| `economy/town_state_initial.yaml` | `01-world/02`:187 | 景气状态归 economy（06），补入 05 树 |

通用字段：

| 规范名 | 别名 | 说明 |
| --- | --- | --- |
| `vanilla_index` | `legacy_index`、`id:` | 原版枚举值，只供导入 / 导出（D1） |
| 剧本 → 玩家势力 `player_force` | `protagonist_force` | §14-1 |
| 交易品外键 `unlock_item` | `plant_item` | 解锁该交易品的发现物（道具）；§14-2 |
| 剧本字段 `map_point_unlock_table` | `base_milestone`、`vanilla:milestones.rowN` | 港口地图点解锁表（`01-world/10` `unlock_tables`，行 key `vanilla:map_point_unlock_0..3`，旧裸 key `vanilla_0..3` 作废；行形状 `{need: [商业, 军事], kind}`）；先过据点港门再查表；§14-11 |
| `orphan_policy: drop \| replace_with: <key> \| refuse_load` | `on_missing`、`keep_orphan` | 读档找不到 key 时的处理；`keep_orphan` 不是合法值 |
| 港口 YAML 外键 `culture` / 运行时 `culture_id` | `region` | 港口 `+0x25` |
| 港口 YAML 外键 `remains` / 运行时 `remains_id` | `b38` | 港口 `+0x26`，原版 26 = 无 |
| `byte_0x27` | `b39` | 港口 `+0x27`，`[未知]` |

## 10. 特产品槽字段（`economy/special_products.yaml`，economy 拥有）

| 字段 | 原版位置 | 含义 | 别名 |
| --- | --- | --- | --- |
| `town` | — | 港口 key | — |
| `slot` | 0–4 | 槽号 | — |
| `trading_item` | 定义段 `+0x28+slot*6 +0` u8 | 交易品 key（不用 `item`，避免与道具域混淆） | `item` |
| `listing_threshold` | 定义段 `+2` u16 | 商业发展度 ≥ 门槛才上架 | `unlock_commerce`、`supply_or_price`（JSON 旧键） |
| `supply_base` | 定义段 `+4` u8 | 月配额基数：`supply_base × (商业/50+100)/100` | `base_qty`、`base_supply`、JSON `grade` |
| `data_price` | 初值段 `+0x60+slot*6 +2` u16 | 槽底价（伦敦煤 550）；与交易品目录的 `data_price` 是两张表 | — |
| `initial_supply` | 初值段 `+4` | 开局供给 | — |
| `plant_lock` | 初值段 `+0` bit0 | 待种锁 | `awaiting_plant` |
| 运行态 `supply` / `month_bought` | `CTown+0x2c` 段 | 当前供给 / 本月已购 | — |

`Docs/_data/towns_world0.json` 已用 `listing_threshold` / `supply_base`；04-economy 第二轮报告建议的 `unlock_commerce` / `base_qty` 不采用（`_data`、14、15、10 已按前者写）。

## 11. 时钟门与 tick 阶段（`engine/tick.yaml`）

`schedule` 只取五个门：`tick` `day_start` `month_start` `month_end` `year_start`（D4）。附加修饰：`every_n_days: <n>`、`when: at_sea`（只用于 `duty_exp`）、步骤粒度 `per: town | per: phase`、按舰队位置分支 `steps: { in_port: […], at_sea: […] }`（只用于 `daily`）。别名：`gate:`→`schedule:`、`on: day_start`→`schedule: [day_start]`、`every_3_days_at_sea` / `schedule: every 3 days`→`schedule: [day_start]` + `every_n_days: 3` + `when: at_sea`、`interleave: per_town`→`per: town`。

phase id（顺序见 `14-config-schemas/examples/engine/tick.yaml`）：

| phase id | 原版 | schedule | 步骤 key |
| --- | --- | --- | --- |
| `climate` | `CWorld::vfunc_04` | day_start | — |
| `npc_fleets` | `CFleetElement` | tick | — |
| `items` | `CItemElement::vfunc_04` | month_end | `economy.treasure_rent`（出租费；早于 `towns` 月末，§14-17） |
| `daily` | 玩家舰队日步 `fn_140005a15` | day_start（不限海上，§14-18） | `in_port`：`fleets.fatigue`（只恢复疲劳；旅馆每夜同样触发）；`at_sea`：`ocean.supply_drain` → `fleets.fatigue` → `characters.vitals` → `ocean.happening_roll` → `characters.mental_transition` / `ocean.happening_persist` |
| `duty_exp` | `fn_140009b24` | day_start，`every_n_days: 3`，`when: at_sea` | `progression.duty_exp` |
| `persons` | `CPersonElement` | day_start | — |
| `towns` | `CTownElement::vfunc_04` | month_start / month_end | 月初：`economy.town_state_update` → `economy.price_drift` → `{ per: town, steps: [economy.investment_convert, economy.restock] }`；月末：`economy.exclusive_tribute`（出租费不在此，见 `items`） |
| `forces` | `CForceElement` | month_start | `factions.suzerain_tribute` |
| `schemes` | — | day_start | — |
| `requests` | — | month_start | — |
| `star_goods` | 年初星品 | year_start | `economy.star_goods` |
| `gold_road_bonus` | `fn_1400108fc` | month_start | system `ocean.gold_road` |
| `player_fleet` | `fn_140011414` | tick | — |

- 05-config-architecture 的 phase `fleets`（把 DailyTick 挂在 `CFleetElement` 下）拆成 `npc_fleets` + `daily`；`yearly_brand` → `star_goods`。14 示例第三轮已补 `persons` phase。
- `characters.mental_transition`（事件引起的精神转移，人物拥有）与 `ocean.happening_persist`（持续态）分开两步；两者先后以 06-ocean/01 的反汇编顺序为准。
- 步骤别名：`progression.post_exp`→`progression.duty_exp`、`economy.convert_pending`→`economy.investment_convert`、`economy.convert_and_restock`→`{ per: town, steps: [economy.investment_convert, economy.restock] }`、`economy.brand_and_star`→`economy.star_goods`、`economy.golden_route_bonus`→phase `gold_road_bonus`、`economy.suzerain_tribute`→`factions.suzerain_tribute`、`town.state_update`→`economy.town_state_update`。
- `DailyTick` 是 `daily` phase 的名字，不是事件。

## 12. 事件 / 查询 / 命令

规范名见 `06-system-interfaces.md`，各别名的出现处见附录 A.10。本轮补充的别名（06 表里没有，按同一规则归并）：

| 别名 | 规范名 |
| --- | --- |
| `Town.develop` | `Towns.develop(town_id, track)` |
| `Town.remains_of` | `Remains.at_town` |
| `Factions.share_slots` | `Share.slots(town_id)` |
| `Factions.add_influence` | `Influence.add(force_id, major_sea_id, delta, reason)` |
| `Characters.skill_value` | `Characters.skill(person_id, skill_key)` |
| `Session.scene_mode` | `Session.mode()` |
| `MajorSea.ofTown`、`Factions.major_sea`、`Towns.major_sea` | `MajorSea.of_town(town_id)` |
| `Economy.town_price_mod`、`Towns.kind` | `Economy.town_state(town_id)` + `economy/town_state_price_rules.yaml` 的 `context` |
| `count_cabin_kind`、`fleet_cabin_count`、`cabin_count(…, kind_key)` | `Ships.cabin_count(ship_id, role_key)` |
| `person_in_post` | `Fleets.post_holder(fleet_id, post_key)` |
| `Fleets.addFatigue` | `Fleets.add_fatigue` |
| `GrantGold`、`GoldGained`、`RewardGold` | `Force.add_gold(force_id, delta, reason)`（命令；成功后发 `ForceGoldChanged`） |
| `ExpGranted`、`CrewExpGranted` | `Progression.grant_exp(…)`（全员广播由 progression 展开） |
| `LevelUp` | `LevelChanged` |
| `FleetEliminated`、`FleetDisbanded`、`FleetWiped` | `FleetDestroyed { fleet_id, cause }` |
| `CrewEmpty` | `CrewWiped`（`05-ships-fleet/06`） |
| `SeaZone.*`、`EnteredSeaZone` | `SeaArea.*`、`EnteredSeaArea` |
| `Clock.ticks_per_day` | `$caps.calendar_ticks_per_day` |
| `ProtagonistDied` | `PersonDied { person_id, is_protagonist, cause }`（§14-5） |
| `UnlockRemain`、`UnlockVillage`（items 事件） | 命令 `World.mark_remains_discovered / mark_village_discovered(id, via: item, item_id)`，world 发 `RemainsDiscovered` / `VillageDiscovered`（§14-23） |
| `Influence.get` | `Factions.influence(force_id, major_sea_id)`（§14-26） |
| `Characters.applyRest` | `Characters.apply_rest`（§14-5） |
| `ScenarioCleared { scenario }` 等不带 `_id` 的载荷 | `ScenarioCleared { scenario_id }` / `ScenarioFailed { scenario_id, cause, end_code? }` / `ScenarioSelected { scenario_id, …, protagonist_id }`（§14-3） |

06 登记状态（第四轮）：第三轮列的待登记项 `EnteredMajorSea` / `EnteredSeaArea`、`NpcTradeCommitted` / `NpcPurchaseFailed`、`TradeOpened`、`RemainsRevisited`、`ProofDiscovered` / `ItemBargained`、`SurgeonWorked`、`CrewWiped`、`SaveWriting`、`TownStateChanged { town_id, old, new, flag }`、`ItemFound` 已全部在 06；§14-26 另补命令 `Fleets.destroy(fleet_id, cause)`、查询 `Factions.influence(force_id, major_sea_id)`、`Remains.town(remains_id)`、`Economy.settle_special(…, counts_toward_quota)`，`Influence.add` 返回修正后、夹紧前的加值。`PlantSpecialProduct` 是命令、`SpecialPlanted { town_id, slot, trading_item, fleet_id, item_id }` 由 economy 发（§13-3、§14-25）。`TickAdvanced`（01-world/07）只给 UI，系统用 `schedule: tick`。第五轮补登：事件 `HappeningPersist { fleet_id, outcome }`、`CrewAttrition { fleet_id, keep_pct }`，`HappeningRolled.happening_id` = 槽 key；命令 `Profile.set_cleared(scenario_id)`（仅 profile 与导入器）；`ContractSigned { town_id, force_id, share_gain, share }`（与 `Contract.sign(town_id, force_id, share_gain)` 对齐，不带金额）；`PlantSpecialProduct` 四字段（含 `slot`）在 `04-economy/04` 同步。

## 13. 总控裁定（已定）

1. 舱室种类 `crows_nest` / `staff_room` / `purser_room`：**采用**。05-ships-fleet/03 改 YAML，09-items/02 改 `cabin:`。
2. `FleetSpotted` / `StormHit` 等载荷：`major_sea_id` 必填；`sea_area` 可选、默认 `null`（原版无细分区，模组可用）。两者并存，不互相替代。
3. `PlantSpecialProduct` 是**命令**（items / narrative → economy）；`SpecialPlanted` 是**事件**，由特产品槽所有者 economy 唯一发出。两者都保留。
4. `combat.engage_fear_crew_div`：**采用** 07-combat 写法；05-ships-fleet/04 改写。
5. 剧本 2 流行品 ×2 与剧本 0 遗迹首访 ×2 统一走 `factions.influence_gain` 修正器（`ctx.source` 区分）；04-economy/03 等公式里不得再乘一次。

---

## 14. 第三轮冲突裁定（总控，已定）

命名
1. 剧本→玩家势力字段统一 `player_force`（多数写法）；§9 的 `protagonist_force` 作废，列为别名。
2. 交易品「解锁它的发现物」外键统一 `unlock_item`；`plant_item` 作别名。
3. 事件载荷引用实体一律 `<entity>_id`（与 `town_id`、`fleet_id` 一致）：`ScenarioCleared { scenario_id }`、`ScenarioFailed { scenario_id, cause, end_code? }`。`cause` 为语义枚举，`end_code` 仅原版导入用。
4. 内容 key 在 YAML 引用处一律写全命名空间（`vanilla:white_whale`），定义行的 `key:` 也写全。剧本 key 以 `10-narrative/01-scenarios.md` 为准（`vanilla:raphael` / `vanilla:helmut` / `vanilla:lulu` / `vanilla:tia` …）。
   澄清（总控）：凡是模组可扩展的内容表行（人物、港口、道具、剧本、船首像、岗位、技能、能力、舱室种类、happening 结局、地图点 kind 等）在定义与引用处一律写全命名空间 key；引擎词汇（stage 名、schedule gate、修正器 key、事件/命令名、枚举语义值）不带命名空间。
5. `ProtagonistDied` → `PersonDied { person_id, is_protagonist, cause }`；`Characters.applyRest` 按 06 规范名。
6. 叙事触发 kind 5 / 6 的事件源名用 06 的 `TownTick` / `SeaTick`；A.10 `CalendarStep → schedule: tick` 仅指调度 gate。
7. caps：`exp_track_cap`；删除 `sailor_roster`（存档完整记录数 87 只在 `legacy_codec`）。
8. 舱室种类 23 种（「无」不进表）；05 目录树注释改 23。

文件归属
9. 流行品 / 发信参数（`fashion_*`、`share_letter_cut`、`letter_*`）归 `formulas/factions.yaml`（消费方 factions）。
10. 遗迹/旧地图解锁表只在 `config/discovery/old_maps.yaml`；05 目录树删除 `world/unlock_maps.yaml`。
11. 港口地图点解锁表名 `map_point_unlock_table`；`unlock_by_development` 必须先过据点港门（`fn_140009b42`，`0x14025f867`）再查表。
12. 开局舰队只在 `narrative/starting_fleets.yaml`；`scenarios.yaml` 只以 key 引用。
13. `figureheads.yaml` / `rig.yaml` 只在 05-ships-fleet 定义；06-ocean 只引用。
14. 价格情境 `town_kind_price_mods` 保留 `remodel` 行（原版值同 `ship_buy`），便于模组单调。
15. 公式 YAML 一律用 14-config-schemas README 的 D3 形状（`impl / int_mode / quirks / params / tables / golden`）；平铺参数改成该形状。
16. `+0x7E` 写作「两字节：u8 trading_item + u8 等级 [确认]；原版按 word 拷贝」，legacy_codec 同步。

所有权与顺序
17. tick 顺序以原版为准：出租费 `economy.treasure_rent` 在 items 阶段，排在 towns 阶段（独占上缴）之前。
18. `daily` 阶段不限 `at_sea`：在港分支只做疲劳恢复（原版 `fn_140005a15`）；海上分支走 补给 → 疲劳 → 人物 → 掷骰。旅馆每夜同样触发在港分支。
19. 疲劳状态归 fleets；人物 HP / 身心归 characters。
20. 宗主上缴执行与 `SuzerainTributePaid` 归 factions；economy 只给金额式。
21. 合约份额只走一条路：`Contract.sign` 由 factions 执行并调 `Share.add(reason: contract)`；`InvestmentMade` 不再带 `share_gain`。
22. 遗迹首访 / 重访由 world 在 `on_enter_town_map_point`（kind 9/10/11）内处理并发 `RemainsDiscovered` / `RemainsRevisited`；06 的 `FacilityEntered` 订阅方加 world。
23. 道具触发的村落/遗迹发现：items 发命令 `World.mark_village_discovered / mark_remains_discovered(id, via: item, item_id)`，world 发事件；`UnlockRemain/UnlockVillage` 作废。
24. 通关位：profile 订阅 `ScenarioCleared` 写位；画廊只读 `Profile.cleared`。
25. `SpecialPlanted { town_id, slot, trading_item, fleet_id, item_id }`（带 `fleet_id`）。
26. 06 补登记：`NpcTradeCommitted`、`NpcPurchaseFailed`、`TradeOpened`、`TownStateChanged { town_id, old, new, flag }`、`ItemFound`、命令 `Fleets.destroy(fleet_id, cause)`、查询 `Factions.influence(force, major_sea)`、`Remains.town(remains_id)`；`Influence.add` 返回「修正后、夹紧前」的加值；`Economy.settle_special(town_id, slot, qty, counts_toward_quota)`。
27. 怪鱼像入口以 07-combat/02 源码核对结果为准（接舷开场 vs 无画面开场），05 与词表 §6.2 跟随。

---

## 附录 A 别名 → 规范名（文件:行）

自动扫描 `Docs/**/*.md|yaml`（不含 `_data/` 与本篇）。`*` 标记的行是说明 / 对照句（「旧名」「别名」「→」「作废」等），可保留；其余行下一轮按本表替换。同一数据按目标文件分组的领取表随第三轮任务分发。

### A.1 修正器 key 与贡献写法

| 别名 | 规范名 | 出现处（`*` = 说明 / 对照句，可保留） |
| --- | --- | --- |
| `trade.buy_cost` | trade.npc_buy_cost | `00-overview/03-decoupling-principles.md`:103；`00-overview/06-system-interfaces.md`:251；`00-overview/09-architecture-decisions.md`:54*；`03-characters/03-abilities-skills-jobs.md`:273 |
| `trade.sell_price` | trade.sell_price_pct | `00-overview/06-system-interfaces.md`:251；`10-narrative/01-scenarios.md`:294 |
| `trade.fashion_influence` | factions.influence_gain（ctx.source: fashion_sale） | `00-overview/06-system-interfaces.md`:252；`13-algorithms/01-formulas.md`:108,572；`14-config-schemas/examples/formulas/economy.yaml`:42；`14-config-schemas/examples/modifiers.yaml`:22；`14-config-schemas/examples/narrative/scenarios.yaml`:24 |
| `trade.fashion_influence_mult` | factions.influence_gain（ctx.source: fashion_sale） | `04-economy/02-market-price.md`:365,491；`04-economy/03-trade-loop.md`:280,392 |
| `influence.fashion_sale` | factions.influence_gain（ctx.source: fashion_sale） | `10-narrative/01-scenarios.md`:295 |
| `remains.first_visit_influence_second_add` | factions.influence_gain（ctx.source: remains） | `10-narrative/01-scenarios.md`:273 |
| `invest.military_cost` | invest.military_min_fee_pct | `00-overview/06-system-interfaces.md`:253 |
| `invest.military_min_cost` | invest.military_min_fee_pct | `10-narrative/01-scenarios.md`:272 |
| `invest.military_min` | invest.military_min_fee_pct | `10-narrative/01-scenarios.md`:222 |
| `military_invest_discount（剧本旗标）` | invest.military_min_fee_pct 修正器（D5：乘除差异走修正器） | `00-overview/05-config-architecture.md`:139；`10-narrative/01-scenarios.md`:269,272；`14-config-schemas/examples/narrative/scenarios.yaml`:14 |
| `request.reward` | request.reward_gold | `00-overview/05-config-architecture.md`:141；`00-overview/06-system-interfaces.md`:254 |
| `fleet.search_radius` | fleet.search_radius_add（贡献）／Ocean.search_radius(fleet_id)（派生量） | `00-overview/06-system-interfaces.md`:256；`06-ocean/02-search-encounters.md`:149,265,402 |
| `fleet.food_drain` | supply.food_div | `00-overview/06-system-interfaces.md`:256 |
| `fleet.food_drain_div_add` | supply.food_div | `06-ocean/01-sailing.md`:360,369,504；`06-ocean/05-sea-happenings.md`:404 |
| `battle.melee` | combat.melee_damage | `00-overview/06-system-interfaces.md`:257 |
| `combat.max_hull` | ship.durability_max | `07-combat/01-naval-battle.md`:483,534 |
| `combat.enemy_crew_on_engage` | combat.engage_fear_crew_div | `05-ships-fleet/04-remodel-guns-supplies.md`:468 |
| `ships.repair_discount_pct` | ship.repair_cost | `13-algorithms/03-adjustment-factors.md`:282,387 |
| `fleet.survey_unlock` | （非修正器）Fleets.post_holder(fleet, surveyor) != null | `06-ocean/04-manual-auto-sail.md`:332,365 |
| `贡献字段 mod:` | key: | `07-combat/01-naval-battle.md`:534,535,536,537；`07-combat/06-prizes-capture.md`:352 |
| `op: frac_of_base / add_frac / mul_frac / frac / sub_frac + num/den` | value: { num, den }（stage 决定 add/mul；+s/10 写 mul {11,10}，x+x/5 写 mul {6,5}） | `05-ships-fleet/04-remodel-guns-supplies.md`:418,453,459,468；`06-ocean/01-sailing.md`:501,510,513 |
| `scope: once_per_fleet` | scope: fleet_any, stack: once | `06-ocean/01-sailing.md`:501,504,507,510；`06-ocean/05-sea-happenings.md`:517 |
| `combat.prize_ship_value_bounty` | Scenario.flag("prize_ship_value_bounty")（剧本旗标，非修正器；07-combat/06:350 已有 flags） | `07-combat/06-prizes-capture.md`:324,352 |
| `progression.level_for_skill` | （非修正器）技能公式内 min(L0+L1, $caps.skill_level_cap) | `13-algorithms/03-adjustment-factors.md`:139,386 |

### A.2 六维 / 技能 key

| 别名 | 规范名 | 出现处（`*` = 说明 / 对照句，可保留） |
| --- | --- | --- |
| `vitality（作六维）` | stamina | `11-progression/01-level-exp-skills.md`:93,211,347,363；`12-ui-save/02-save-session.md`:299；`12-ui-save/03-localization.md`:238 |
| `vitality_skill` | vitality（技能） | `06-ocean/01-sailing.md`:359；`11-progression/01-level-exp-skills.md`:82 |
| `seamanship` | helmsmanship | `12-ui-save/02-save-session.md`:269；`12-ui-save/03-localization.md`:230 |
| `swordsmanship` | fencing | `12-ui-save/03-localization.md`:233 |
| `skill.survey` | surveying | `12-ui-save/03-localization.md`:231 |
| `strategy（技能）` | tactics | `12-ui-save/03-localization.md`:236；`13-algorithms/01-formulas.md`:871 |
| `skill.<数字>.<desc\|name>` | skill.<skill_key>.desc / .name | `03-characters/03-abilities-skills-jobs.md`:96 |
| `skill_id: <int>` | skill: <skill_key> | `14-config-schemas/examples/ocean/search.yaml`:11* |

### A.3 岗位 key

| 别名 | 规范名 | 出现处（`*` = 说明 / 对照句，可保留） |
| --- | --- | --- |
| `staff_officer` | staff | `05-ships-fleet/03-cabins-crew-posts.md`:262,267；`14-config-schemas/examples/town/plots.yaml`:5 |
| `sailhand` | sail_master | `05-ships-fleet/03-cabins-crew-posts.md`:263,267 |
| `assault_leader` | boarding_chief | `05-ships-fleet/03-cabins-crew-posts.md`:206,263,267 |
| `doctor` | surgeon | `05-ships-fleet/03-cabins-crew-posts.md`:206,264,267 |

### A.4 舱室种类 / role key

| 别名 | 规范名 | 出现处（`*` = 说明 / 对照句，可保留） |
| --- | --- | --- |
| `舱室 lookout（了望台）` | crows_nest | `05-ships-fleet/03-cabins-crew-posts.md`:238,263；`07-combat/02-boarding.md`:283；`09-items/02-equipment-effects.md`:364；`13-algorithms/03-adjustment-factors.md`:122 |
| `舱室 staff（参谋室）` | staff_room | `05-ships-fleet/03-cabins-crew-posts.md`:256,262；`14-config-schemas/examples/town/plots.yaml`:5 |
| `舱室 purser（会计室）` | purser_room | `05-ships-fleet/03-cabins-crew-posts.md`:257,262 |
| `舱室 vanilla:captain` | captain_room | `03-characters/03-abilities-skills-jobs.md`:335；`03-characters/07-fatigue-health-mood.md`:486 |
| `kitchen` | galley | `13-algorithms/03-adjustment-factors.md`:122 |
| `oar_room` | rower_room | `06-ocean/01-sailing.md`:317,436,626；`06-ocean/04-manual-auto-sail.md`:220,281,425 |
| `recreation（role）` | recreation_room（lounge 行新增 role） | `06-ocean/01-sailing.md`:185,464 |
| `cabin_count(ship, cargo) / cabin_count(rower)` | cargo_room / rower_room | `13-algorithms/01-formulas.md`:543,857 |
| `per_cabin_kind` | per_cabin_role | `14-config-schemas/examples/ocean/search.yaml`:15* |
| `skill ctx 里的 cabin_role` | cabin: <cabin_kind key>（由 posts.room 得出） | `07-combat/01-naval-battle.md`:334；`07-combat/02-boarding.md`:283 |
| `Ships.cabin_count(…, kind_key)` | Ships.cabin_count(ship_id, role_key) | `06-ocean/04-manual-auto-sail.md`:364 |

### A.5 caps 键

| 别名 | 规范名 | 出现处（`*` = 说明 / 对照句，可保留） |
| --- | --- | --- |
| `ability_settled_cap` | skill_value_cap | `00-overview/04-id-registry.md`:132*；`03-characters/01-person-model.md`:195*；`03-characters/03-abilities-skills-jobs.md`:31,391*；`07-combat/01-naval-battle.md`:551*；`11-progression/01-level-exp-skills.md`:221,288*；`13-algorithms/01-formulas.md`:332*；`13-algorithms/02-caps-ranges.md`:48*；`14-config-schemas/README.md`:46*；`14-config-schemas/examples/caps.yaml`:11* |
| `ability_growth_cap` | ability_cap | `00-overview/04-id-registry.md`:131*；`11-progression/01-level-exp-skills.md`:289*；`13-algorithms/02-caps-ranges.md`:47*；`14-config-schemas/README.md`:46*；`14-config-schemas/examples/caps.yaml`:10* |
| `skill_cap` | skill_level_cap | `00-overview/04-id-registry.md`:133*；`03-characters/03-abilities-skills-jobs.md`:95；`08-town-life/04-schemes-rhetoric.md`:192,576；`11-progression/01-level-exp-skills.md`:287*；`13-algorithms/02-caps-ranges.md`:51*；`14-config-schemas/README.md`:46*；`14-config-schemas/examples/caps.yaml`:12* |
| `track_level_cap` | level_per_track | `03-characters/01-person-model.md`:197；`03-characters/03-abilities-skills-jobs.md`:230,247,391* |
| `exp_cap` | exp_track_cap | `14-config-schemas/README.md`:44；`14-config-schemas/examples/caps.yaml`:9 |
| `exp_per_track_at_cap` | exp_track_cap | `13-algorithms/02-caps-ranges.md`:46,287 |
| `sailor_roster` | （删除；不进 caps，见 persisted_full_person 说明） | `00-overview/04-id-registry.md`:137*；`07-combat/06-prizes-capture.md`:275,509,545；`11-progression/01-level-exp-skills.md`:291；`13-algorithms/02-caps-ranges.md`:58*；`14-config-schemas/README.md`:46*；`14-config-schemas/examples/caps.yaml`:22* |
| `hold_scan` | （删除；用 role cargo_room） | `04-economy/03-trade-loop.md`:309；`04-economy/06-npc-fleet-trade.md`:519；`13-algorithms/02-caps-ranges.md`:57* |
| `town_develop_cap` | town_develop | `04-economy/07-investment-tribute.md`:251,391,471,561；`13-algorithms/01-formulas.md`:687 |
| `hold_records_per_ship` | hold_slots_per_ship | `05-ships-fleet/01-ship-model.md`:193,194,240,297,324 |
| `town.develop_cap` | town_develop | `01-world/02-towns.md`:114 |
| `skill.level_cap（formulas/character.yaml）` | $caps.skill_level_cap | `03-characters/03-abilities-skills-jobs.md`:372 |
| `ticks_per_day（定义）` | calendar_ticks_per_day（caps；公式里写 $caps.calendar_ticks_per_day） | `01-world/07-calendar-and-coords.md`:60,243,322,421；`08-town-life/04-schemes-rhetoric.md`:336,649 |
| `gold_max（未入 caps.yaml）` | gold_max（caps，补登；原版 99,999,999） | `04-economy/03-trade-loop.md`:311 |

### A.6 配置文件路径与 ID 字段

| 别名 | 规范名 | 出现处（`*` = 说明 / 对照句，可保留） |
| --- | --- | --- |
| `characters/companions.yaml` | narrative/companions.yaml | `00-overview/05-config-architecture.md`:102*；`03-characters/02-protagonists-scenarios.md`:63,138；`03-characters/06-companions-bonds.md`:261,264；`14-config-schemas/README.md`:292* |
| `characters/zodiacs.yaml` | characters/zodiac.yaml | `00-overview/05-config-architecture.md`:102*；`11-progression/01-level-exp-skills.md`:125,212,358；`14-config-schemas/README.md`:292* |
| `config/figureheads.yaml` | ships/figureheads.yaml | `07-combat/01-naval-battle.md`:530 |
| `items/figureheads.yaml` | ships/figureheads.yaml | `00-overview/05-config-architecture.md`:102*；`14-config-schemas/README.md`:328* |
| `economy/town_state_price_mods.yaml` | economy/town_state_price_rules.yaml | `04-economy/02-market-price.md`:466 |
| `factions/culture_prices.yaml` | economy/culture_prices.yaml | `00-overview/05-config-architecture.md`:102*；`14-config-schemas/README.md`:273* |
| `factions/major_seas.yaml` | world/major_seas.yaml | `01-world/09-remains-oldmaps-catalog.md`:432；`01-world/11-remains-puzzle-system.md`:17*,74,546；`02-factions/03-relations-influence.md`:243*；`02-factions/06-seven-culture-circles.md`:13*,190 |
| `factions/town_shares.yaml` | factions/town_shares_initial.yaml | `01-world/02-towns.md`:66*,178；`02-factions/03-relations-influence.md`:263 |
| `formulas/fleet.yaml` | formulas/fleets.yaml | `03-characters/07-fatigue-health-mood.md`:495 |
| `import/vanilla.yaml` | ids/legacy_codec.yaml | `00-overview/05-config-architecture.md`:102*；`09-items/01-item-catalog.md`:62,533；`09-items/02-equipment-effects.md`:283,386；`14-config-schemas/README.md`:230* |
| `ocean/crew_sail.yaml` | ships/rig.yaml | `00-overview/05-config-architecture.md`:102*；`06-ocean/04-manual-auto-sail.md`:418；`14-config-schemas/README.md`:328* |
| `ships/sails.yaml` | ships/rig.yaml | `00-overview/05-config-architecture.md`:102*；`05-ships-fleet/04-remodel-guns-supplies.md`:393；`14-config-schemas/README.md`:328* |
| `ships/ship_types.yaml` | ships/models.yaml | `07-combat/02-boarding.md`:396 |
| `ships/types.yaml` | ships/models.yaml | `00-overview/05-config-architecture.md`:102*；`14-config-schemas/README.md`:328* |
| `world/adman.yaml` | town/plaza.yaml | `00-overview/05-config-architecture.md`:102*；`08-town-life/06-plaza-intel-adman.md`:363,431；`14-config-schemas/README.md`:253* |
| `world/mappoint_kinds.yaml` | world/map_point_kinds.yaml | `00-overview/05-config-architecture.md`:102*；`14-config-schemas/README.md`:253* |
| `world/sea_zones.yaml` | world/sea_areas.yaml | `01-world/01-map-and-sea-areas.md`:67,154 |
| `characters/modifier_keys.yaml` | modifiers.yaml | `00-overview/05-config-architecture.md`:102*；`14-config-schemas/README.md`:69*,292* |
| `characters/protagonist_modifiers.yaml` | narrative/scenarios.yaml#modifiers | `00-overview/05-config-architecture.md`:102*；`14-config-schemas/README.md`:292* |
| `town/town_facilities.yaml` | town/town_map_points.yaml | `00-overview/05-config-architecture.md`:102*；`14-config-schemas/README.md`:385* |
| `formulas/adjustment.yaml` | formulas/vision.yaml + formulas/supply.yaml | `14-config-schemas/README.md`:119* |
| `economy/price_curves.yaml` | economy/culture_prices.yaml + economy/town_state_price_rules.yaml | `14-config-schemas/README.md`:310* |
| `town_state_price_mods.yaml` | economy/town_state_price_rules.yaml | `04-economy/02-market-price.md`:232,366,467；`04-economy/07-investment-tribute.md`:339,404,510 |
| `sea_zones.yaml（裸名）` | sea_areas.yaml | `01-world/08-sea-area-music-icons.md`:207,366 |
| `price_curves.yaml（裸名）` | culture_prices.yaml + town_state_price_rules.yaml | `README.md`:81 |
| `schema dk4.factions.major_seas` | dk4.world.major_seas | `02-factions/06-seven-culture-circles.md`:191 |
| `schema dk4.world.sea_zones` | dk4.world.sea_areas | `01-world/01-map-and-sea-areas.md`:157 |
| `legacy_index` | vanilla_index | `00-overview/09-architecture-decisions.md`:39*；`06-ocean/03-gold-road.md`:172,173,174,175,176；`08-town-life/02-requests-schemes.md`:432；`10-narrative/01-scenarios.md`:181,189,249,276,286,298；`10-narrative/04-world0-world1-diff.md`:767,768,769,770,771,772,773,786；`10-narrative/05-gallery-unlock.md`:154,252,268,269,270,271,272,307；`14-config-schemas/README.md`:23* |
| `keep_orphan / on_missing` | orphan_policy: drop \| replace_with: <key> \| refuse_load | `12-ui-save/02-save-session.md`:191,246,248,249 |
| `protagonist_force`（§14-1，第四轮补） | player_force | `00-overview/05-config-architecture.md`:106*；`14-config-schemas/README.md`:407*；`14-config-schemas/examples/narrative/scenarios.yaml`:6 |
| `plant_item`（§14-2，第四轮补） | unlock_item | `04-economy/05-goods-art-spawn-display.md`:168,428；`09-items/01-item-catalog.md`:383,480；`09-items/02-equipment-effects.md`:376；`09-items/03-discovery-planting.md`:112,309,321,322,325,391,430,494,517,546；`14-config-schemas/README.md`:301 |
| `world/unlock_maps.yaml`（§14-10，第四轮补） | discovery/old_maps.yaml | `00-overview/05-config-architecture.md`:106*；`01-world/09-remains-oldmaps-catalog.md`:436；`01-world/11-remains-puzzle-system.md`:530；`09-items/04-ancient-treasures-hd.md`:207；`14-config-schemas/README.md`:253 |
| `characters/protagonists.yaml`（第四轮补） | narrative/scenarios.yaml#protagonist | `00-overview/05-config-architecture.md`:44*,106*；`03-characters/02-protagonists-scenarios.md`:128*；`10-narrative/01-scenarios.md`:3；`14-config-schemas/README.md`:288 |

### A.7 海域 key

| 别名 | 规范名 | 出现处（`*` = 说明 / 对照句，可保留） |
| --- | --- | --- |
| `southeast_asia` | se_asia | `09-items/03-discovery-planting.md`:416,434；`09-items/04-ancient-treasures-hd.md`:487,489 |
| `Factions.major_sea / Towns.major_sea` | MajorSea.of_town(town_id) | `01-world/01-map-and-sea-areas.md`:102,119,201；`01-world/08-sea-area-music-icons.md`:234；`01-world/09-remains-oldmaps-catalog.md`:355；`01-world/11-remains-puzzle-system.md`:386；`02-factions/03-relations-influence.md`:309*；`02-factions/06-seven-culture-circles.md`:271*；`05-ships-fleet/05-hull-catalog-market-start.md`:418,454*,683 |
| `sea_zone（ID 域）` | sea_area | `01-world/01-map-and-sea-areas.md`:11,157,163,173,180,213；`01-world/08-sea-area-music-icons.md`:12 |
| `EnteredSeaZone` | EnteredSeaArea | `01-world/01-map-and-sea-areas.md`:213 |

### A.8 特产品槽 / 港口记录字段

| 别名 | 规范名 | 出现处（`*` = 说明 / 对照句，可保留） |
| --- | --- | --- |
| `unlock_commerce` | listing_threshold | `01-world/02-towns.md`:58*；`04-economy/04-special-products-investment.md`:67*,193,379,384,385,386,387,540；`04-economy/05-goods-art-spawn-display.md`:76*,351,402 |
| `base_qty` | supply_base | `01-world/02-towns.md`:58*；`04-economy/04-special-products-investment.md`:67*,85,100,103,105,260,267,270,380,384,385,386,387,540；`04-economy/05-goods-art-spawn-display.md`:206*,402；`04-economy/06-npc-fleet-trade.md`:374,475；`04-economy/07-investment-tribute.md`:290* |
| `base_supply` | supply_base | `13-algorithms/01-formulas.md`:730 |
| `awaiting_plant` | plant_lock | `09-items/03-discovery-planting.md`:310,320 |
| `supply_or_price（JSON 旧键）` | listing_threshold | `01-world/02-towns.md`:58*；`04-economy/04-special-products-investment.md`:67*,540；`04-economy/05-goods-art-spawn-display.md`:76*；`10-narrative/04-world0-world1-diff.md`:286*；`14-config-schemas/examples/economy/special_products.yaml`:6；`15-catalogs/towns.md`:5*,41 |
| `JSON `grade`（槽 +4）` | supply_base | `04-economy/05-goods-art-spawn-display.md`:82,96,206*,448；`04-economy/06-npc-fleet-trade.md`:153；`04-economy/07-investment-tribute.md`:285,290*,621 |
| `b38` | remains_id（YAML 外键 remains） | `01-world/02-towns.md`:43,73,230；`10-narrative/04-world0-world1-diff.md`:286*；`15-catalogs/towns.md`:5*,18* |
| `b39` | byte_0x27 | `01-world/02-towns.md`:44,168,231；`10-narrative/04-world0-world1-diff.md`:286*；`15-catalogs/towns.md`:5*,19 |
| `region（港口 +0x25）` | culture_id（YAML 外键 culture） | `01-world/02-towns.md`:10*,42*,85*,131*；`01-world/08-sea-area-music-icons.md`:32*；`01-world/10-mappoint-kinds.md`:102*；`02-factions/02-countries-culture-fashion.md`:3,10,31,204*；`02-factions/06-seven-culture-circles.md`:95；`04-economy/04-special-products-investment.md`:19；`04-economy/07-investment-tribute.md`:23；`08-town-life/01-facilities.md`:28；`09-items/03-discovery-planting.md`:546；`10-narrative/04-world0-world1-diff.md`:286*；`15-catalogs/towns.md`:5*,17；`_meta/source-map.md`:56 |
| `town.region` | Towns.def(town_id).culture | `09-items/03-discovery-planting.md`:546 |
| `特产品槽 item:` | trading_item: | `04-economy/04-special-products-investment.md`:378,384,385,386,387；`14-config-schemas/README.md`:302；`14-config-schemas/examples/economy/special_products.yaml`:11,12,13,14,15 |

### A.9 schedule / tick 阶段名

| 别名 | 规范名 | 出现处（`*` = 说明 / 对照句，可保留） |
| --- | --- | --- |
| `every_3_days_at_sea` | phase duty_exp: schedule [day_start] + every_n_days: 3 + when: at_sea | `00-overview/05-config-architecture.md`:238 |
| `schedule: every 3 days` | schedule: [day_start] + every_n_days: 3 + when: at_sea | `03-characters/06-companions-bonds.md`:244 |
| `gate:（tick 片段）` | schedule: | `04-economy/04-special-products-investment.md`:458,464,467,470,473 |
| `on: day_start` | schedule: [day_start] | `06-ocean/01-sailing.md`:557 |
| `progression.post_exp` | progression.duty_exp | `00-overview/05-config-architecture.md`:238；`03-characters/06-companions-bonds.md`:244 |
| `economy.convert_and_restock` | { per: town, steps: [economy.investment_convert, economy.restock] } | `04-economy/04-special-products-investment.md`:462 |
| `economy.convert_pending` | economy.investment_convert | `04-economy/07-investment-tribute.md`:433,465 |
| `economy.brand_and_star / yearly_brand` | economy.star_goods（phase star_goods） | `00-overview/05-config-architecture.md`:251；`04-economy/04-special-products-investment.md`:471 |
| `economy.golden_route_bonus` | ocean.gold_road（phase gold_road_bonus） | `04-economy/04-special-products-investment.md`:474 |
| `economy.suzerain_tribute` | factions.suzerain_tribute（forces phase） | `04-economy/04-special-products-investment.md`:468 |
| `phase fleets（含 DailyTick 子步）` | phase npc_fleets [tick] + phase daily [day_start] | `00-overview/05-config-architecture.md`:231 |
| `interleave: per_town` | per: town（tick.yaml 步骤粒度） | `04-economy/04-special-products-investment.md`:153,305,440 |
| `town.state_update` | economy.town_state_update（景气状态归 economy，06） | `04-economy/04-special-products-investment.md`:460 |

### A.10 事件 / 查询 / 命令名（06 别名）

| 别名 | 规范名 | 出现处（`*` = 说明 / 对照句，可保留） |
| --- | --- | --- |
| `Town.develop` | Towns.develop(town_id, track) | `02-factions/03-relations-influence.md`:21,277；`04-economy/04-special-products-investment.md`:523；`04-economy/05-goods-art-spawn-display.md`:442；`05-ships-fleet/08-gun-shell-catalog.md`:216 |
| `Town.remains_of` | Remains.at_town | `01-world/02-towns.md`:215；`01-world/09-remains-oldmaps-catalog.md`:352；`01-world/11-remains-puzzle-system.md`:380,768,804 |
| `Factions.share_slots` | Share.slots(town_id) | `04-economy/07-investment-tribute.md`:598 |
| `SeaZone.*` | SeaArea.* | `01-world/01-map-and-sea-areas.md`:103,119,198,199,200；`01-world/08-sea-area-music-icons.md`:232 |
| `Clock.ticks_per_day` | $caps.calendar_ticks_per_day | `08-town-life/04-schemes-rhetoric.md`:336 |
| `ModifierSet` | Modifiers.eval(key, ctx)（注册表，非结构体） | `00-overview/09-architecture-decisions.md`:54*；`03-characters/03-abilities-skills-jobs.md`:259*,269；`09-items/02-equipment-effects.md`:410*；`13-algorithms/01-formulas.md`:89*；`13-algorithms/03-adjustment-factors.md`:447；`14-config-schemas/README.md`:162*；`_meta/writing-guide.md`:51 |
| `Narrative.current_scenario` | Scenario.flag(key)（系统侧） | `03-characters/02-protagonists-scenarios.md`:158 |
| `in_battle` | Session.mode() == battle | `06-ocean/06-npc-fleet-ai.md`:550；`13-algorithms/01-formulas.md`:829,834,837*；`14-config-schemas/README.md`:188 |
| `Session.scene_mode` | Session.mode() | `06-ocean/04-manual-auto-sail.md`:235；`06-ocean/05-sea-happenings.md`:278；`06-ocean/06-npc-fleet-ai.md`:529 |
| `Town.def` | Towns.def(town_id) | `01-world/01-map-and-sea-areas.md`:119,169；`01-world/02-towns.md`:144,208；`01-world/08-sea-area-music-icons.md`:253,399 |
| `World.town` | Towns.def(town_id) | `04-economy/08-price-intel.md`:244；`10-narrative/04-world0-world1-diff.md`:848 |
| `World.town_exists` | Towns.def(town_id) | `10-narrative/02-event-system.md`:420；`10-narrative/03-event-commands.md`:685 |
| `Develop.get` | Towns.develop(town_id, track) | `04-economy/07-investment-tribute.md`:578 |
| `Town.military` | Towns.develop(town_id, track) | `05-ships-fleet/08-gun-shell-catalog.md`:264；`07-combat/01-naval-battle.md`:591；`07-combat/04-ship-vs-port.md`:336,349,372,429,447,527,560 |
| `Town.known` | Towns.is_known(town_id, force_id) | `01-world/02-towns.md`:211；`01-world/05-routes-navigation.md`:147 |
| `World.climate_at` | Climate.weather() | `06-ocean/05-sea-happenings.md`:583 |
| `Map.class_at` | Map.tile_class(...) | `01-world/01-map-and-sea-areas.md`:196；`01-world/06-three-maps.md`:515 |
| `World.village` | Village.def / discovered | `06-ocean/02-search-encounters.md`:400 |
| `World.remains` | Remains.def / discovered / at_town | `06-ocean/02-search-encounters.md`:401 |
| `Factions.share` | Share.of / Share.slots / Share.exclusive_owner | `00-overview/03-decoupling-principles.md`:183；`01-world/03-villages-discovery.md`:97；`02-factions/03-relations-influence.md`:309*；`04-economy/03-trade-loop.md`:394；`04-economy/04-special-products-investment.md`:330,506,523；`04-economy/05-goods-art-spawn-display.md`:442；`04-economy/06-npc-fleet-trade.md`:572；`04-economy/07-investment-tribute.md`:598 |
| `query_share` | Share.of / Share.slots / Share.exclusive_owner | `02-factions/03-relations-influence.md`:309* |
| `Factions.force_of_town` | Share.of / Share.slots / Share.exclusive_owner | `10-narrative/03-event-commands.md`:684 |
| `Factions.relation` | Relation.get(a, b) | `02-factions/03-relations-influence.md`:309*；`06-ocean/06-npc-fleet-ai.md`:385,516；`07-combat/04-ship-vs-port.md`:426,564 |
| `Factions.may_attack` | Relation.may_attack(a, b) | `07-combat/03-battle-ai.md`:300；`07-combat/04-ship-vs-port.md`:565 |
| `World.culture_of` | Culture.of_town(town_id) | `02-factions/02-countries-culture-fashion.md`:296；`02-factions/06-seven-culture-circles.md`:263；`04-economy/02-market-price.md`:545 |
| `Towns.theater` | MajorSea.of_town(town_id) | `05-ships-fleet/05-hull-catalog-market-start.md`:454* |
| `MajorSea.ofTown` | MajorSea.of_town(town_id) | `02-factions/03-relations-influence.md`:309*；`02-factions/06-seven-culture-circles.md`:261,271* |
| `Factions.fashion_slots` | Fashion.current(town_id) | `02-factions/02-countries-culture-fashion.md`:299；`04-economy/02-market-price.md`:544 |
| `TownLife.plazaFashion` | Fashion.current(town_id) | `08-town-life/01-facilities.md`:397 |
| `Factions.force` | Force.get(force_id) | `02-factions/01-forces.md`:326；`10-narrative/04-world0-world1-diff.md`:850 |
| `Economy.gold` | Force.gold(force_id) | `11-progression/03-gameover-succession.md`:274,290,465 |
| `Progression.skill_value` | Characters.skill(person_id, skill_key) | `13-algorithms/01-formulas.md`:949 |
| `Characters.skill_value` | Characters.skill(person_id, skill_key) | `06-ocean/01-sailing.md`:369 |
| `Town.state` | Economy.town_state(town_id) | `04-economy/02-market-price.md`:543 |
| `Towns.kind` | Economy.town_state(town_id) | `05-ships-fleet/04-remodel-guns-supplies.md`:283,325,550；`05-ships-fleet/05-hull-catalog-market-start.md`:438,439,683 |
| `Economy.town_price_mod` | Economy.town_state(town_id) | `04-economy/02-market-price.md`:484,535；`04-economy/07-investment-tribute.md`:330 |
| `Specials.*` | Economy.special_offer / Economy.specials | `04-economy/03-trade-loop.md`:395,396；`04-economy/04-special-products-investment.md`:279,292,478*,498,501,502,503,504,505,506,525；`04-economy/05-goods-art-spawn-display.md`:168,285,381；`04-economy/06-npc-fleet-trade.md`:21,388,597,598 |
| `Economy.special_slot` | Economy.special_offer(town_id, slot, force_id) | `10-narrative/04-world0-world1-diff.md`:853 |
| `Specials.quota` | Economy.buy_cap(town_id, slot, force_id, channel) | `04-economy/04-special-products-investment.md`:499 |
| `Specials.buy_cap` | Economy.buy_cap(town_id, slot, force_id, channel) | `04-economy/04-special-products-investment.md`:109,500,525；`04-economy/05-goods-art-spawn-display.md`:247；`04-economy/06-npc-fleet-trade.md`:21,165,374,460,596 |
| `Economy.count_fleet_item` | Ships.count_item(fleet_id, item_id) | `10-narrative/02-event-system.md`:279,418；`10-narrative/03-event-commands.md`:464,681 |
| `Ships.cargo_view` | Ships.holds(ship_id) | `04-economy/06-npc-fleet-trade.md`:577 |
| `count_cabin_kind` | Ships.cabin_count(ship_id, role_key) | `05-ships-fleet/03-cabins-crew-posts.md`:215 |
| `fleet_cabin_count` | Ships.cabin_count(ship_id, role_key) | `06-ocean/01-sailing.md`:185 |
| `Force.isPlayer` | Fleets.is_player_fleet(fleet_id)（判舰队时） | `02-factions/05-war-failure-absorb.md`:473 |
| `Characters.occupants` | Ships.occupants(ship_id, role_key) / Fleets.post_holder | `03-characters/04-recruitment-crew.md`:199 |
| `Town.has_map_point` | TownLife.has_map_point(town_id, kind_key) | `08-town-life/02-requests-schemes.md`:179 |
| `person_in_post` | Fleets.post_holder(fleet_id, post_key) | `04-economy/03-trade-loop.md`:146 |
| `SailingSnapshot` | Fleets.sailing_snapshot(fleet_id) | `06-ocean/01-sailing.md`:397,471*,619；`06-ocean/04-manual-auto-sail.md`:362,529；`13-algorithms/01-formulas.md`:957；`13-algorithms/03-adjustment-factors.md`:280 |
| `NavSnapshot` | Fleets.sailing_snapshot(fleet_id) | `06-ocean/04-manual-auto-sail.md`:362,516,536,566 |
| `Fleets.town` | Fleets.location(fleet_id) | `06-ocean/06-npc-fleet-ai.md`:339,515 |
| `Fleets.current_town` | Fleets.location(fleet_id) | `04-economy/06-npc-fleet-trade.md`:297,574 |
| `Ocean.position` | Fleets.location(fleet_id) | `06-ocean/01-sailing.md`:604 |
| `TownLife.facilities` | TownLife.map_points(town_id) | `08-town-life/01-facilities.md`:394 |
| `Town.map_points` | TownLife.map_points(town_id) | `01-world/10-mappoint-kinds.md`:403 |
| `Characters.has_item` | Items.owned(fleet_id, item_id) | `10-narrative/02-event-system.md`:419；`10-narrative/03-event-commands.md`:682 |
| `Gallery.cleared` | Profile.cleared(scenario_id)（profile 订阅 ScenarioCleared 写位，§14-24） | `10-narrative/01-scenarios.md`:355；`10-narrative/05-gallery-unlock.md`:345 |
| `Calendar.add_days` | Clock.advance_days(n) | `00-overview/05-config-architecture.md`:262；`00-overview/09-architecture-decisions.md`:132 |
| `Ocean.advance_calendar_step` | Clock.advance_days(n) | `13-algorithms/03-adjustment-factors.md`:432 |
| `RestAtInn` | Actions.run(action_key, ctx) | `03-characters/07-fatigue-health-mood.md`:23；`08-town-life/01-facilities.md`:408；`08-town-life/05-inn-church-palace.md`:655 |
| `JoinRite` | Actions.run(action_key, ctx) | `08-town-life/05-inn-church-palace.md`:656 |
| `SeekSupport` | Actions.run(action_key, ctx) | `08-town-life/06-plaza-intel-adman.md`:375,538 |
| `BeginEnding` | Session.end_scenario(outcome, cause) | `10-narrative/01-scenarios.md`:338；`10-narrative/03-event-commands.md`:461 |
| `LaunchMinigame` | Minigames.run(id) -> int | `08-town-life/03-minigames.md`:511 |
| `MinigameStart` | Minigames.run(id) -> int | `06-ocean/05-sea-happenings.md`:18,246,314,379,563 |
| `Town.apply_develop` | Towns.apply_develop(town_id, track, delta, reason) | `07-combat/04-ship-vs-port.md`:339,574 |
| `World.mark_town_known` | Towns.mark_known(town_id, force_id) | `01-world/02-towns.md`:220；`01-world/05-routes-navigation.md`:261 |
| `World.mark_discovered` | World.mark_village_discovered / World.mark_remains_discovered | `06-ocean/02-search-encounters.md`:419 |
| `Gold.debit` | Force.add_gold(force_id, delta, reason) | `05-ships-fleet/04-remodel-guns-supplies.md`:550；`05-ships-fleet/05-hull-catalog-market-start.md`:683 |
| `Gold.credit` | Force.add_gold(force_id, delta, reason) | `05-ships-fleet/05-hull-catalog-market-start.md`:683 |
| `Economy.credit` | Force.add_gold(force_id, delta, reason) | `07-combat/06-prizes-capture.md`:333,346,579 |
| `Wallet.add_gold` | Force.add_gold(force_id, delta, reason) | `04-economy/08-price-intel.md`:251,395 |
| `Force.setGold` | Force.add_gold(force_id, delta, reason) | `02-factions/01-forces.md`:311 |
| `GrantGold` | Force.add_gold(force_id, delta, reason) | `01-world/11-remains-puzzle-system.md`:467 |
| `GoldGained` | Force.add_gold(force_id, delta, reason) | `06-ocean/02-search-encounters.md`:393 |
| `RewardGold` | Force.add_gold(force_id, delta, reason) | `02-factions/04-letters-licenses.md`:271；`08-town-life/02-requests-schemes.md`:269 |
| `Factions.add_influence` | Influence.add(force_id, major_sea_id, delta, reason) | `01-world/09-remains-oldmaps-catalog.md`:359,362；`01-world/11-remains-puzzle-system.md`:388,390；`02-factions/03-relations-influence.md`:309*；`02-factions/06-seven-culture-circles.md`:271* |
| `OpenInvestment` | Invest.pay(town_id, force_id, gold, reason) | `04-economy/07-investment-tribute.md`:600；`08-town-life/01-facilities.md`:260,410；`08-town-life/05-inn-church-palace.md`:373,456,658 |
| `Economy.add_load` | Ships.set_hold / add_load(fleet_id, item_id, delta) | `10-narrative/02-event-system.md`:279,427；`10-narrative/03-event-commands.md`:464,692 |
| `AddLoad` | Ships.set_hold / add_load(fleet_id, item_id, delta) | `04-economy/06-npc-fleet-trade.md`:25；`10-narrative/02-event-system.md`:55,279；`10-narrative/03-event-commands.md`:464,653,708 |
| `AssignPost` | Cabins.assign / unassign | `03-characters/03-abilities-skills-jobs.md`:426；`03-characters/04-recruitment-crew.md`:209 |
| `ClearPost` | Cabins.assign / unassign | `03-characters/04-recruitment-crew.md`:210 |
| `HireCrewCount` | Crew.hire / set / lose(ship_id, n, reason) | `03-characters/04-recruitment-crew.md`:212；`05-ships-fleet/06-crew-headcount.md`:327,486* |
| `Crew.lost` | Crew.hire / set / lose(ship_id, n, reason) | `05-ships-fleet/06-crew-headcount.md`:476 |
| `Ships.addSupplies` | Supplies.consume / add | `07-combat/06-prizes-capture.md`:305,314,580 |
| `Fleets.refill_supplies` | Supplies.consume / add | `13-algorithms/03-adjustment-factors.md`:434 |
| `EquipFigurehead` | Yard.*（船首像） | `09-items/02-equipment-effects.md`:416 |
| `Fleets.found_proxy` | Fleets.create_proxy(gold, …) | `04-economy/06-npc-fleet-trade.md`:433 |
| `Fleets.addFatigue` | Fleets.add_fatigue | `03-characters/04-recruitment-crew.md`:149；`03-characters/07-fatigue-health-mood.md`:29,332,394,541 |
| `applyRest` | Characters.apply_rest / add_hp / set_physical / set_mental | `03-characters/04-recruitment-crew.md`:17；`03-characters/07-fatigue-health-mood.md`:541；`08-town-life/05-inn-church-palace.md`:27 |
| `applyHappening` | Characters.apply_rest / add_hp / set_physical / set_mental | `03-characters/07-fatigue-health-mood.md`:540 |
| `GrantExp` | Progression.grant_exp(person_id, add{track: n}, roll_mode, source) | `00-overview/03-decoupling-principles.md`:186；`05-ships-fleet/02-fleet-proxy.md`:143；`11-progression/01-level-exp-skills.md`:23,228,240*,408,422；`13-algorithms/01-formulas.md`:447,491 |
| `ExperienceGranted` | Progression.grant_exp(person_id, add{track: n}, roll_mode, source) | `03-characters/01-person-model.md`:242 |
| `ExpGranted` | Progression.grant_exp(person_id, add{track: n}, roll_mode, source) | `13-algorithms/01-formulas.md`:464 |
| `CrewExpGranted` | Progression.grant_exp(person_id, add{track: n}, roll_mode, source) | `06-ocean/02-search-encounters.md`:391；`06-ocean/05-sea-happenings.md`:571 |
| `EquipOn` | Equip.on / off(person_id, slot, item_id) | `09-items/02-equipment-effects.md`:414 |
| `EquipOff` | Equip.on / off(person_id, slot, item_id) | `09-items/02-equipment-effects.md`:415 |
| `AcquireItem` | Items.acquire / discard / use | `01-world/11-remains-puzzle-system.md`:212,442,464；`09-items/01-item-catalog.md`:557 |
| `GrantItem` | Items.acquire / discard / use | `06-ocean/05-sea-happenings.md`:357,568 |
| `Items.give` | Items.acquire / discard / use | `10-narrative/02-event-system.md`:428；`10-narrative/03-event-commands.md`:693 |
| `DayAdvanced` | DayStarted | `01-world/07-calendar-and-coords.md`:493* |
| `MonthEnded` | MonthEnding | `01-world/07-calendar-and-coords.md`:493* |
| `HalfYearStart` | MonthStarted（按月过滤） | `01-world/07-calendar-and-coords.md`:493* |
| `CalendarStep` | 调度门 schedule: tick（仅 UI 可用 TickAdvanced）；叙事 kind 5 / 6 事件源是 TownTick / SeaTick（§14-6） | `01-world/07-calendar-and-coords.md`:493*；`10-narrative/02-event-system.md`:407；`10-narrative/03-event-commands.md`:505,667 |
| `DailyTick` | tick.yaml day_start 子阶段（不是事件） | `00-overview/05-config-architecture.md`:234,259；`00-overview/09-architecture-decisions.md`:129；`01-world/07-calendar-and-coords.md`:308；`11-progression/01-level-exp-skills.md`:261；`13-algorithms/03-adjustment-factors.md`:350,432；`14-config-schemas/README.md`:203 |
| `StartScenario` | ScenarioSelected | `03-characters/02-protagonists-scenarios.md`:164；`03-characters/06-companions-bonds.md`:38,170,348 |
| `NewGameSeeded` | ScenarioSelected | `10-narrative/01-scenarios.md`:336 |
| `GameStateChanged` | ScenarioCleared / ScenarioFailed（GameStateChanged 只给 ui） | `12-ui-save/01-scenes-flow.md`:366 |
| `TownEntered` | ArrivedAtTown | `08-town-life/01-facilities.md`:380* |
| `FleetArrived` | ArrivedAtTown | `05-ships-fleet/02-fleet-proxy.md`:274 |
| `FacilityOpened` | FacilityEntered | `08-town-life/01-facilities.md`:385 |
| `EnterFacility` | FacilityEntered | `12-ui-save/01-scenes-flow.md`:272,346 |
| `MapPointClicked` | FacilityEntered | `01-world/10-mappoint-kinds.md`:223,247,411；`01-world/11-remains-puzzle-system.md`:804 |
| `SearchHit` | DiscoveryFound | `09-items/04-ancient-treasures-hd.md`:603 |
| `TownKnown` | TownBecameKnown | `04-economy/08-price-intel.md`:209,403 |
| `CargoCommitted` | TradeCommitted | `04-economy/01-trading-items.md`:319；`06-ocean/03-gold-road.md`:124 |
| `InvestmentAudience` | InvestmentMade | `08-town-life/05-inn-church-palace.md`:169,632 |
| `ExclusiveTribute` | ExclusiveTributePaid | `02-factions/03-relations-influence.md`:340；`04-economy/07-investment-tribute.md`:335,592 |
| `SuzerainTributeDue` | SuzerainTributePaid | `02-factions/03-relations-influence.md`:340；`04-economy/07-investment-tribute.md`:353,593 |
| `LedgerCredited` | ForceGoldChanged（账本事件另列） | `04-economy/06-npc-fleet-trade.md`:609 |
| `LedgerDebited` | ForceGoldChanged（账本事件另列） | `04-economy/06-npc-fleet-trade.md`:610 |
| `TownCaptured` | TownOccupied | `07-combat/04-ship-vs-port.md`:595 |
| `ProtagonistDied` | PersonDied { person_id, is_protagonist, cause } | `03-characters/07-fatigue-health-mood.md`:164,358,395,553,558；`11-progression/03-gameover-succession.md`:19,173,194,197,324,331,448,477 |
| `OfficerDied` | PersonDied { person_id, is_protagonist, cause } | `03-characters/07-fatigue-health-mood.md`:359,554；`11-progression/03-gameover-succession.md`:195 |
| `PersonEquipSet` | PersonEquipped / PersonUnequipped | `03-characters/01-person-model.md`:243 |
| `PostAssigned` | CabinBound / CabinUnbound | `03-characters/03-abilities-skills-jobs.md`:429,432；`03-characters/04-recruitment-crew.md`:215 |
| `PostCleared` | CabinBound / CabinUnbound | `03-characters/03-abilities-skills-jobs.md`:432 |
| `CrewLost` | CrewChanged { reason } | `05-ships-fleet/06-crew-headcount.md`:143,278,294,297,318,448；`06-ocean/02-search-encounters.md`:198,201,390；`07-combat/02-boarding.md`:437 |
| `FleetEliminated` | FleetDestroyed { fleet_id, cause } | `11-progression/01-level-exp-skills.md`:255；`11-progression/03-gameover-succession.md`:272,325,477 |
| `FleetDisbanded` | FleetDestroyed { fleet_id, cause } | `11-progression/03-gameover-succession.md`:272,325,477 |
| `FleetWiped` | FleetDestroyed { fleet_id, cause } | `11-progression/03-gameover-succession.md`:283 |
| `UnlockRemain` / `UnlockVillage`（§14-23，第四轮补） | World.mark_remains_discovered / mark_village_discovered(id, via: item, item_id) | `09-items/01-item-catalog.md`:391,565；`09-items/02-equipment-effects.md`:273；`01-world/03-villages-discovery.md`:235*；`01-world/11-remains-puzzle-system.md`:807* |
| `Influence.get`（§14-26，第四轮补） | Factions.influence(force_id, major_sea_id) | `02-factions/03-relations-influence.md`:324*,334 |
| `LevelUp` | LevelChanged | `03-characters/07-fatigue-health-mood.md`:286 |
| `SpecialPlanted`（items 发出的旧写法） | 命令 `PlantSpecialProduct { fleet_id, town_id, slot, item_id }` → economy 唯一发 `SpecialPlanted { town_id, slot, trading_item, fleet_id, item_id }`（§13-3、§14-25，已定） | `04-economy/04-special-products-investment.md`:520 |
| `CrewEmpty` | CrewWiped（05-ships-fleet/06；06 已登记） | `11-progression/03-gameover-succession.md`:283 |

---

## 15. 待定（交实现前裁定）

第五轮收尾时仍未裁定的项。正文**未按任一方案改**，下列位置保持现状；裁定后按所选方案批量改，并在附录 A 补别名。

### 15-1 引擎提供的策略 / 动作 key 是否算可扩展内容、是否加命名空间

§14-4 澄清把「模组可扩展的内容表行」定为全命名空间，把「引擎词汇」定为裸 key。下列 key 介于两者之间：key 由引擎实现（代码里有对应处理器），但表是数据、模组可能想加行。

| 位置 | 现写法 | 说明 |
| --- | --- | --- |
| `07-combat/03` §6 `combat/ai.yaml` `select_targets`（及 `algos[*].select_target` 引用） | 裸 `first_hostile_in_range` / `uniform_random_alive` / `flagship` / `nearest_in_arc` | 同表 `algos` 行已是 `vanilla:default_npc`；表内已注明「模组」行 |
| `05-ships-fleet/02` §6 `fleets/proxy_rules.yaml` `orders` | 裸 `buy_warehouse` / `deliver` / `seek_person` / `escort` / `collect_debt` | 代理舰队指令 |
| `05-ships-fleet/06` §6 `crew.yaml` `hire.sites[*].key`；§4 `Crew.redistribute` 策略键 | 裸 `inn` / `port`；`fill_min_then_even` | `facility` 列已是 `vanilla:inn` / `vanilla:dock` |
| `08-town-life/01` §6 `facilities` 各行 `actions` | 裸 `open_exchange`、`buy_ship`、`rest_single` … | 设施动作 key；`08-town-life/06` 用 `town.plaza.seek_support` 点分写法，两种并存 |
| `08-town-life/06` §6 `town/plaza.yaml` `clicks[*].key`、`crowd.outcomes[*].key`、`rumors[*].key` | 裸 `seek_support` / `shop_sign_0` / `minigame` / `price_drop` / `remains_hint` … | rumors 带 `i18n` 派生文案 key |
| `06-ocean/05` §6 `ocean/happenings.yaml` `slots[*].key` / `outcome`、`outcomes` 映射键 | 裸 `scurvy` / `rats` / `storm` … | §14-4 澄清已把「happening 结局」列为内容；但引用方不一致：`05-ships-fleet/04` 圣母像已写 `[vanilla:scurvy, vanilla:plague]`，`03-characters/07` 正文、词表 §6.2 `fleet.happening_block` 行仍是裸 key；`06-ocean/05` §7 又写「结局表 key 是命名空间字符串，模组可加 `mymod:kraken`」 |
| 小游戏 key：`08-town-life/03` §6 定义 `vanilla:rat`；`06-ocean/05`（L18、L246、L262、L314、L380、L473）引用 `Minigames.run(rat)` / `{ minigame: rat }` | 定义带前缀、引用裸 | 已是事实上的不一致 |

| 方案 | 做法 | 影响 |
| --- | --- | --- |
| A：算内容，全加前缀 | 上表全部定义与引用改 `vanilla:<key>`；处理器按 `impl` / `handler` 字段绑定而不是按 key 名 | 改动约 7 个文件、百余处；`06-ocean/05` 结局键与 `05-ships-fleet/04`、`03-characters/07`、词表 §6.2 一次对齐；模组可加 `mymod:` 行而不撞名；需在每张表加 `handler:`（或 `impl:`）列，否则新行没有实现 |
| B：算引擎词汇，全裸 | 只允许引擎注册的 key，模组加行须同时注册处理器（插件），key 取 `<plugin>.<name>` 点分 | 需把已带前缀的 `vanilla:rat`、圣母像 `vanilla:scurvy` 回退为裸 key；§14-4 澄清里「happening 结局」要移出内容清单；`06-ocean/05` §7 的 `mymod:kraken` 说法要改 |

折中可选：happening 槽 / 结局与小游戏走 A（已有一半带前缀、且澄清已点名），其余策略 / 动作 / 点击 / 传闻 key 走 B。

### 15-2 同串 key 跨 ID 域

词表 §0「一个 key 只属一个 ID 域」目前有两处违例：

| 串 | 域 1 | 域 2 |
| --- | --- | --- |
| `vanilla:north_sea` 等七个 | 七大海域 `world/major_seas.yaml`（`02-factions/06` §6、`14-config-schemas/examples/world/major_seas.yaml`） | 霸者之证槽 `09-items/04` §6 `slots[*].key`（同行 `major_sea:` 又引用同串） |
| `vanilla:woodin` | 势力 `02-factions/01`（L279 `player_force: vanilla:woodin`） | 剧本 `10-narrative/04` L772、`03-characters/02` L48、`03-characters/06` L298、`05-ships-fleet/05` L635、`10-narrative/05` L257/266、`12-ui-save/02` L304–379（存档 JSON 的 `force` 与 `scenario` 两张映射同时出现该串） |

建议方案：**按域加 key 前缀词**，不改命名空间规则。

- 霸者证槽改 `vanilla:proof_<sea>`（如 `vanilla:proof_north_sea`），`major_sea:` 列保持引用海域 key。影响：只 `09-items/04` §6 一张表 7 行与其文案派生 key（`proof.identify.*` 已独立，不变）。
- 剧本 key 维持 `10-narrative/01` 现名（§14-4 已定剧本 key 以它为准），势力侧凡与剧本同串的改 `vanilla:force_<name>` 或改用家族名（`vanilla:woodin_family`）。影响：`02-factions/01` 势力表与全部 `player_force:` / `force:` 引用、`12-ui-save/02` 存档示例、`15-catalogs` 势力目录；需先列出所有与剧本同串的势力 key。

备选：加载器按「ID 域 + key」做复合主键，允许同串。代价是所有跨域引用字段必须带域（不能只凭字段名推断），且存档 / 调试输出要写 `force:vanilla:woodin` 形式，词表 §0 规则作废。

### 15-3 `npc_loop.params.fashion_npc` 的文件归属

`04-economy/06` §6 `formulas/economy.yaml` 的 `npc_loop.params.fashion_npc`（`share_cut` / `gain` / `capacity_*` / `grant_letters`）是 NPC 卖流行品后的份额规则，消费方是 factions（订阅 `NpcFashionSold`，`00-overview/06`）。按 §14-9「流行品 / 发信参数归 `formulas/factions.yaml`（消费方 factions）」应迁到 `formulas/factions.yaml`（如 `fashion_npc_share_gain` 一条 D3 公式）；但它与 NPC 买卖循环同一函数（原版内联在 NPC 交易里），留在 economy 可少一次跨文件引用。待定：迁 factions（与 §14-9 一致）或在 §14-9 加例外。

### 15-4 `ocean.happening_upkeep` 是否作为 tick 步骤

`06-ocean/01` §4.4 第 3 步 `ocean.happening_upkeep`（happening 态续期等，`0x1400fae0f`–`0x1400fb1a6`）`[未知]`，未逐条还原。现状：`06-ocean/01` L529 的 `steps` 与 `14-config-schemas/examples/engine/tick.yaml` L16 已写入该步；词表 §11 `daily` 行、`00-overview/05-config-architecture` 的 tick 片段（`at_sea` 分支）与 `00-overview/06` 的 `DailyTick` 说明未列。待定：

- 列入：§11 `at_sea` 分支改为 `ocean.supply_drain → fleets.fatigue → ocean.happening_upkeep → characters.vitals → ocean.happening_roll → …`，05 树片段与 06 同步；
- 不列：从 `06-ocean/01` 与 14 示例删除，等反汇编确认有独立效果再加。
