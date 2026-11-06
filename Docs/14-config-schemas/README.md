# 配置 Schema 总表

运行时只读 `config/`。这里是规格：每个文件的 schema 名、所属文档、主键、外键、定义 vs 状态。真正的表由内容管线从 World DLL / exe 段表 / mesfile 生成后再补 key（D10）。

目录树与别名以 [05-config-architecture.md](../00-overview/05-config-architecture.md)「目录」为准；全局 key 规范名（六维、技能、岗位、舱室、修正器、caps、海域、特产品槽字段、tick 阶段）以 [10-key-glossary.md](../00-overview/10-key-glossary.md) 为准；架构决策见 [09-architecture-decisions.md](../00-overview/09-architecture-decisions.md) D1–D10。公式见 [13-algorithms/01-formulas.md](../13-algorithms/01-formulas.md)。上限见 [13-algorithms/02-caps-ranges.md](../13-algorithms/02-caps-ranges.md)。ID 计数见 [04-id-registry.md](../00-overview/04-id-registry.md)。

## 通用规则

每个 YAML 头：

```yaml
schema: dk4.<domain>.<file>
version: 1
```

引擎按 `schema` 选解析器。加可选字段升小版本，删 / 改语义升大版本。缺字段不回退代码默认值。

**定义** = 只读内容包。**状态** = 存档覆盖。禁止把定义拷进存档再改（原版部分表运行时会被改写，重构必须标出可变字段）。

**ID（D1）**

- 主键是 `key: string`，全局为 `<pack>:<key>`。YAML 里定义行的 `key:` 与所有引用处一律写全命名空间（`key: vanilla:white_whale`、`trading_item: vanilla:coal`，词表 §14-4），不靠加载器补前缀。不是内容引用的串写裸名：修正器 key 里的段（`skill.accounting`、`ability.stamina`）、role key（`cargo_room`）、轨 key（`track_a`）、枚举值（`category: officer`）、文案 key。
- 原版条目另写 `vanilla_index: int`（= 原版枚举，导入器生成，只用于对照 DLL / 旧存档）。旧稿的 `id:` / `legacy_index:` 都改名 `vanilla_index`。模组**不写任何整数**。
- 外键写**目标 key**（`trading_item: vanilla:coal`），不写整数、不传指针。下表「主键」列里的 `(eTown)` 只说明原版对应哪个枚举。
- 空值写 `null`（运行时 `Option<Id>`）。原版哨兵（106、98、232、236、26、22、70、201 …）**只**出现在 `ids/legacy_codec.yaml`，内容表与公式里不得出现。
- 原版靠 id 区间表达的语义写成显式字段（`recruitable_roster`、`category`、`home_town`、`npc_pool`、`dock_template_of`）；按下标排的矩阵写成按 key 稀疏映射 + `default` + 加载期覆盖率校验。
- 代码与公式里不得出现 `== 字面量 id`（船首像、岗位、技能、舱室种类、剧本号）。用 `refs`（key）、`role`（舱室角色）、`effects`（修正器贡献）、`scenario_flag.*`。

示例目录：`Docs/14-config-schemas/examples/`（不是运行时路径；运行时在 `config/`）。全部示例经 YAML 解析器逐文件校验（js-yaml 4）。

---

## 全局

### `config/caps.yaml`

| 项 | 值 |
| --- | --- |
| schema | `dk4.caps` |
| 所属文档 | [13-algorithms/02-caps-ranges.md](../13-algorithms/02-caps-ranges.md) |
| 主键 | 无行主键；键即上限名 |
| 定义 / 状态 | **定义**。存档只存规则包版本 |

字段：`level_per_track`、`track_count`、`ability_cap`（六维 100）、`skill_value_cap`（技能值 / Ability 500）、`skill_level_cap`（技能式等级项 200）、`buy_ability_clamp`、`battle_ability_clamp`、`town_develop`、`cargo_stack`、`fleet_ships`、`hold_slots_per_ship`、`cabins_per_ship`、`share_cap`、`calendar_ticks_per_day`（48，D4）、`exp_track_cap`（`null` = 由公式推导）、`owned_ships_max`（92 `[推断]`）、`gold_max`（99,999,999）。必须 ≤ `engine_limits`。键表见 10-key-glossary §7。

旧名作废：`ability_growth_cap`→`ability_cap`，`ability_settled_cap`→`skill_value_cap`，`skill_cap`→`skill_level_cap`，`exp_cap` / `exp_per_track_at_cap`→`exp_track_cap`，`track_level_cap`→`level_per_track`，`town_develop_cap`→`town_develop`，`hold_scan`（误读，实为 role `cargo_room`），`sailor_roster`（98 是 dk4save 工具值；原版存档完整人物记录 87 条，见 04-id-registry，不进 caps）。

**只放上限。** 门槛、除数、基准（`share_letter_cut`、`letter_*`、`ref_*`、`fashion_share_div`、`exp_divisor` …）是公式参数，只在 `formulas/` 出现一次。公式要用上限时写 `$caps.<key>`，不抄数字。

### `config/engine_limits.yaml`

| 项 | 值 |
| --- | --- |
| schema | `dk4.engine_limits` |
| 所属文档 | 同上；[11-progression/01](../11-progression/01-level-exp-skills.md) §6 |
| 定义 / 状态 | **定义**（引擎自带，内容包 / 模组不可改） |

字段：`overflow: saturate`、整数宽度表、`level_hard`、`ability_stored_hard`、`skill_value_hard`、`exp_hard`、`town_develop_hard`、`cargo_stack_hard`、`gold_hard`。超过硬顶则启动失败，不回绕。

### `config/modifiers.yaml`（D2）

| 项 | 值 |
| --- | --- |
| schema | `dk4.modifiers` |
| 所属文档 | [03-decoupling-principles.md](../00-overview/03-decoupling-principles.md) 原则 6；[01-formulas.md](../13-algorithms/01-formulas.md)「命名参数与修正器」 |
| 主键 | `key`（命名空间字符串：`skill.accounting`、`fleet.speed`、`supply.food_div`） |
| 定义 / 状态 | **定义**。贡献的当前集合是运行时派生，不存档 |

见下文「修正器注册」；规范 key 表 = 10-key-glossary §6.2。旧名 `characters/modifier_keys.yaml` 是别名。

### `config/engine/tick.yaml`（D4）

| 项 | 值 |
| --- | --- |
| schema | `dk4.engine.tick` |
| 所属文档 | [05-config-architecture.md](../00-overview/05-config-architecture.md)「tick 顺序」；[01-world/07](../01-world/07-calendar-and-coords.md) |
| 主键 | `phases[].id` |
| 定义 / 状态 | **定义**，属规则包版本（改顺序会改数值，版本进存档） |

见下文「时间与阶段」。

### `config/engine/rng.yaml`（D7）

| 项 | 值 |
| --- | --- |
| schema | `dk4.engine.rng` |
| 所属文档 | 05-config-architecture「随机数」 |
| 主键 | `streams[]`（流名） |
| 定义 / 状态 | 定义；种子与各流状态进存档 |

公式里 `rand(n)` 取调用方传入的命名流（`rng_stream: ocean.supply`），不得取全局随机。`algorithm: vanilla_msvc` + 单流只用于对拍。

---

## `config/formulas/`

全部 **定义**。一条公式一个顶层 key（公式名）。技能 / 岗位 / 舱室等引用写在 `refs`（key），不写下标。

| 文件 | schema | 所属文档 | 公式 |
| --- | --- | --- | --- |
| `progression.yaml` | `dk4.formulas.progression` | [11-progression/01](../11-progression/01-level-exp-skills.md)；F-PROG-01..04/10 | 等级、成长、经验授予（`grant_routes`） |
| `character.yaml` | `dk4.formulas.character` | [03-characters/03](../03-characters/03-abilities-skills-jobs.md)；F-PROG-05/06/09 | `skill_value`、`ability`（技能值 + 装备 → 身体乘子 → 夹） |
| `vitals.yaml` | `dk4.formulas.vitals` | [03-characters/07](../03-characters/07-fatigue-health-mood.md) | HP / 身心恢复掷骰 |
| `economy.yaml` | `dk4.formulas.economy` | [04-economy/02](../04-economy/02-market-price.md)、03、04、05、06、08；F-ECO-01、03、07..10、13..18 | 买货、报价、份额封顶、同品调整、特产品供给 |
| `investment.yaml` / `tribute.yaml` | `dk4.formulas.investment` / `.tribute` | [04-economy/07](../04-economy/07-investment-tribute.md)；F-ECO-11/12/19 | 投资→发展度、独占上缴、附庸税、出租费 |
| `factions.yaml` | `dk4.formulas.factions` | [02-factions/03](../02-factions/03-relations-influence.md)、04、05；F-ECO-04/05/06 | 关系、影响力、战争；流行品份额与发信（`fashion_*`、`share_letter_cut`、`letter_*` 只在本文件，消费方 factions，词表 §14-9） |
| `ships.yaml` | `dk4.formulas.ships` | [05-ships-fleet/01](../05-ships-fleet/01-ship-model.md)、04、05、08；F-SHP-01 | 耐久、估价、造船厂上架 |
| `fleets.yaml` | `dk4.formulas.fleets` | [05-ships-fleet/02](../05-ships-fleet/02-fleet-proxy.md) | 地方舰队 |
| `supply.yaml` | `dk4.formulas.supply` | [13-algorithms/03](../13-algorithms/03-adjustment-factors.md)；F-ADJ-03 / F-OCN-05 | 补给日耗 |
| `vision.yaml` | `dk4.formulas.vision` | 同上；F-ADJ-01/02 | 搜索半径、发现判定 |
| `ocean.yaml` | `dk4.formulas.ocean` | [06-ocean/01](../06-ocean/01-sailing.md)；F-OCN-01..03 | 航速、风力档、划桨 |
| `discovery.yaml` | `dk4.formulas.discovery` | [01-world/11](../01-world/11-remains-puzzle-system.md) | 遗迹 / 古地图 |
| `climate.yaml` / `routes.yaml` / `world.yaml` | `dk4.formulas.climate` / `.routes` / `.world` | [01-world/04](../01-world/04-climate-weather-zodiac.md)、05、07 | 风场采样、航线可见、日历 |
| `combat.yaml` | `dk4.formulas.combat` | [07-combat/01](../07-combat/01-naval-battle.md)、02、04、05；F-CBT-* | 射程、装填、白刃、单挑、投降、阵型 |
| `town.yaml` | `dk4.formulas.town` | [08-town-life/01](../08-town-life/01-facilities.md)、04、05；F-TWN-* | 旅馆恢复、仪式日、计谋对抗 |
| `rumors.yaml` | `dk4.formulas.rumors` | [01-world/03](../01-world/03-villages-discovery.md)（归 town-life） | 近港村落传闻 |
| `items.yaml` | `dk4.formulas.items` | [09-items/02](../09-items/02-equipment-effects.md)；F-ECO-02 | 道具店还价 `item_bargain`（旧名 `item_price_factor`，原在 economy）、装备加成 |

别名：`formulas/adjustment.yaml` → 拆入 `vision.yaml` / `supply.yaml`。

### 公式层（D3）

「固定算法 + YAML 参数」**不够**：原版公式里结构本身就是玩法（无符号 `div`、先乘后除、`q==1` 特例、按段写死的成长维）。统一形状：

```yaml
<formula_name>:
  impl: expr                       # expr | vanilla.<name> | <pack>.<name>（插件）
  int_mode: { div: trunc, width: wrap32 }   # div: trunc|floor；width: wrap32|int64|sat64；预设 vanilla_i32 = {trunc, wrap32}
  quirks: { unsigned_div: true }   # 原版怪癖显式开关：unsigned_div、q65536_div_zero、u8_truncate、wrap_mul16 …（带数值的怪癖如 quirk_min_q 放 params，expr 才能读）
  params: { ... }                  # 标量；可写 $caps.<key>
  tables: { ... }                  # 分段 / cdf / 查表（按 key 的稀疏映射 + default）
  refs: { skill: vanilla:accounting, post: vanilla:purser }   # 内容 key，加载期校验外键
  inputs: [price, qty, ability]    # 调用方传入的值；不给系统句柄
  rng_stream: economy              # rand(n) 用的命名流
  enabled_by: scenario_flag.<key>  # 可选：剧本旗标开关（D5）
  schedule: month_start            # 可选：周期性公式由哪个时钟点触发（D4）
  expr: null                       # impl: expr 时必填
  golden: [{ ... , out: N }]       # 回归用例，加载时自检；原版包 expr 版须与 vanilla.* 逐位一致
  modifiers: <output_key>          # 输出再过修正器注册表的这一路（可省）
```

| 档 | 用在哪 | 模组怎么改 |
| --- | --- | --- |
| ① `params` / `tables` | 全部公式 | `$op: patch` 改数 |
| ② `impl: expr` | 单表达式公式：F-PROG-01/02/05、F-ECO-01/02/10、F-CBT-01、F-ADJ-01/04 | 整条替换 `expr` |
| ③ `impl: vanilla.*` / `<pack>.*` | 控制流重的：F-OCN-01、F-ECO-11、F-ADJ-03、F-PROG-10、F-CBT-04、市价循环 | 只能改参数，或换 `impl` |
| ④ `modifiers` | 公式输出的统一出口 | 加修正器贡献（船首像、道具、剧本），不碰公式 |

`expr` 规则（沙箱，不是脚本）：

- 只读 `inputs`、`params`、`tables`、`$caps.*`；可调用其他公式名（无环，加载时拓扑检查）。
- 整数运算，算子显式：`/` 向零截断，`udiv` 无符号（复刻原版），`div_floor`；`min max clamp abs isqrt`、`? :`、`table[key]`；`rand(n)` 取 `rng_stream`，`n<=0` 返回 0。
- 无循环、无状态、无 I/O。`width: wrap32` 回绕（仅对拍），`sat64` 饱和（新包默认）。
- 不得出现 `scenario_id`、字面量内容 id。校验器拦截。

脚本钩子只开在叙事事件边界（D8），**不**进公式。

---

## 修正器注册（D2）

不是写死字段的 `ModifierSet` 结构体。消费方 `Modifiers.eval(key, ctx) -> value`；提供方在内容表里登记**带来源的贡献**。

`config/modifiers.yaml`（key 注册）：

```yaml
keys:
  fleet.speed:            { type: int, int_mode: vanilla_i32, owner_system: ocean }
  supply.food_div:        { type: int, default: 0, owner_system: ocean }
  skill.accounting:       { type: int, clamp: [0, $caps.skill_value_cap], owner_system: characters }
  factions.influence_gain: { type: int, owner_system: factions, ctx: [force, major_sea, source] }
stages: [base, add, mul, clamp, override]     # 固定；同 stage 内按 order，再按 source
```

贡献（写在各内容表的 `effects` / `modifiers` 字段，不集中）：

```yaml
- { key: fleet.speed, stage: mul, value: { num: 11, den: 10 }, order: 10, scope: fleet_any, stack: once }
```

| 字段 | 含义 |
| --- | --- |
| `key` | 已在 `modifiers.yaml` 注册的 key；未注册则加载失败 |
| `stage` | `base / add / mul / clamp / override` |
| `value` | 整数，或 `{num, den}`（先乘后除，按 key 的 `int_mode` 截断），或 `formula_ref` |
| `order` | 同 stage 内先后；复刻原版取整（航速：船首像 `+s/10` order 10 → `×4/5` order 20 → 海战 `/3` order 900） |
| `scope` | `ship` / `fleet_any`（任一船有即生效）/ `person` / `fleet` |
| `stack` | `sum`（默认）/ `once`（同 key 同来源只算一次，复刻「第一艘即停」） |
| `when` | 可选谓词（`scenario_flag.x`、`Session.mode() == battle`、`ctx.source`） |
| `source` | 加载器自动填 `{kind, key}`（`figurehead:vanilla:eagle`、`scenario:vanilla:raphael`） |

提供方表：`ships/figureheads.yaml`、`ships/cabins.yaml`、`items/effects.yaml`、`characters/vitals.yaml`、`ocean/happenings.yaml`（结局 effects）、`narrative/scenarios.yaml`（`modifiers`）、Session 场景态（`scene.battle`）。

一级查询（不是修正器）：`Fleets.post_holder(fleet_id, post_key) -> Option<PersonId>`、`Ships.cabin_count(ship_id, role_key)`、派生量 `Ocean.search_radius(fleet_id)`。替代原版「玩家 / 提督 / 船长」三分支与舱室种类字面量 7/8/9/10/11。

业务公式的技能输入单位是**技能值**（0–`skill_value_cap`），key `skill.<skill_key>`；不读六维原值。

---

## 时间与阶段（D4）

- 每日拍数只在 `caps.calendar_ticks_per_day`（原版 48）。
- 系统订阅 `schedule: tick | day_start | month_start | month_end | year_start`；只收事件 `DayStarted / MonthStarted / MonthEnding / YearStarted`，不读日历内部字段。
- 同一日历点上多系统的先后写进 `engine/tick.yaml` 的 `phases[].steps`，不靠订阅注册顺序。原版顺序（`fn_14000277f` body `0x1402aad80` 世界 tick）：月初先景气状态切换与市价漂移，再逐港「投资转化 → 补货」；`forces` 月初宗主上缴（factions 执行并发 `SuzerainTributePaid`，economy 只给金额式）；月末最后一拍先 `items` 阶段收出租费 `economy.treasure_rent`，再 `towns` 阶段独占上缴；年初星品。`daily` phase（旧称 DailyTick，是阶段不是事件）按 `fn_140005a15`，不挂 `when: at_sea`：在港分支只做舰队疲劳恢复（旅馆每夜同样触发），海上分支 补给日耗 → 舰队疲劳 → happening 续期 → 人物 HP / 身心 → 海上事件掷骰 → 事件精神转移 → 持续态；疲劳状态归 fleets，人物 HP / 身心归 characters；在岗经验是独立 phase `duty_exp`（每 3 天、仅海上）。phase / 步骤 key 见 10-key-glossary §11。
- 模组插入新阶段用 `insert_after: <id>`。

示例：[examples/engine/tick.yaml](examples/engine/tick.yaml)。

---

## 剧本差异（D5）

公式与引擎不比较剧本号。剧本记录（`narrative/scenarios.yaml`）带：

- `flags: { <key>: bool }` → 只放布尔开关；公式写 `enabled_by: scenario_flag.<key>`，查询 `Scenario.flag(key)`。
- `modifiers: [贡献]` → 乘除差异一律走这里（不用旗标）；`ScenarioSelected` 时注入注册表，`source = scenario:<key>`。
- 剧本专属内容表（`base_towns`、`initial_known_towns`、`player_force`、港口地图点解锁表 `map_point_unlock_table`）挂在剧本记录下；开局舰队只在 `narrative/starting_fleets.yaml`，剧本记录只以 key 关联。

原版已知：剧本 0 委托 `request.reward_gold` ×3/2、军事投资 `invest.military_min_fee_pct` ×8/10、遗迹首访 `factions.influence_gain`（`source: remains`）×2；剧本 2 卖价 `trade.sell_price_pct` ×105/100、流行影响力 `factions.influence_gain`（`source: fashion_sale`）×2；剧本 6 旗标 `pay_suzerain_tribute`。示例：[examples/narrative/scenarios.yaml](examples/narrative/scenarios.yaml)。

---

## `config/ids/`

| 文件 | schema | 所属文档 | 主键 | 定义 / 状态 |
| --- | --- | --- | --- | --- |
| `generated.json` | `dk4.ids` | [04-id-registry.md](../00-overview/04-id-registry.md)、05-config-architecture | `<table>.<pack>:<key>` → 运行时 int | **构建产物**，不手写；存档 `id_map` 的来源 |
| `legacy_codec.yaml` | `dk4.ids.legacy_codec` | 04-id-registry「遗留哨兵与存储宽度」 | `enums.<domain>`、`fields[]`（记录 × 偏移） | **定义**，只给原版存档 / DLL 导入导出用；`on_export_overflow: refuse` |
| `sin256_quarter.yaml` | `dk4.ids.sin256_quarter` | [06-ocean/01](../06-ocean/01-sailing.md) | 无 | 定义：65 项 1/4 正弦表（`.rdata 0x140683580`，导出脚本生成）。**05 目录树未列，待补登** |

别名：`import/vanilla.yaml`（`item_empty_byte`）→ `legacy_codec.yaml`。`generated.json` 不含哨兵；`null` 不占整数。

---

## `config/world/`

所属：[01-world](../01-world/)。

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态（存档） |
| --- | --- | --- | --- | --- | --- | --- |
| `map.yaml` | `dk4.world.map` | `01-world/01-map-and-sea-areas.md` | 无 | 资源 `World.dat` | 2500×1250 u16 贴图编号、`chip_unit: 16` | 无。迷雾另表 |
| `maps.yaml` | `dk4.world.maps` | `01-world/06-three-maps.md` | 坐标空间 key | 无 | 航海图 / 航线图 / 黑块图投影 | 无 |
| `calendar.yaml` / `coords.yaml` | `dk4.world.calendar` / `.coords` | `01-world/07-calendar-and-coords.md` | 无 | 无 | 历法、经纬换算 | 当前日期 |
| `sea_areas.yaml` | `dk4.world.sea_areas` | `01-world/01`、`08` | `key`（`eSeaArea`，投票 25） | `major_sea` | 海域、BGM / 图标绑定 | 无 |
| `major_seas.yaml` | `dk4.world.major_seas` | `02-factions/06-seven-culture-circles.md` | `key`（7 + 全世界） | 无 | 七大海域名、矩形 | 无 |
| `towns.yaml` | `dk4.world.towns` | `01-world/02-towns.md` | `key`（`eTown`） | `culture` → `factions/cultures`；`remains` → `world/remains`（`null` = 无） | 坐标 `x1,y1,x2,y2`、`is_capital`、`known_at_start`、`develop_initial`（`+0x52`，发展度初值）、`unknown` | 已知位、发展度（world `Towns` 运行态，唯一发 `TownDeveloped`）。份额、流行、特产品都**不在本表** |
| `villages.yaml` | `dk4.world.villages` | `01-world/03-villages-discovery.md` | `key`（`eVillage`） | | 村落定义 | 发现位 |
| `remains.yaml` | `dk4.world.remains` | `01-world/03`、`09`、`11` | `key`（`eRemains`） | 所在港由 `towns.remains` 反查 | 遗迹定义 | `REMAINSSTATE_BITS` |
| `routes.yaml` / `junctions.yaml` | `dk4.world.routes` / `.junctions` | `01-world/05-routes-navigation.md` | `key`（`eRoute` 200 / `eJunction` 164） | `town` 端点 | 航线、折点 | 无（可见性派生，D6） |
| `climate.yaml` | `dk4.world.climate` | `01-world/04-climate-weather-zodiac.md` | 气候带 | `sea_area` | 风场 2×25×50、天气 CDF | 当前天气 |
| `map_point_kinds.yaml` | `dk4.world.map_point_kinds` | `01-world/10-mappoint-kinds.md` | `key`（12 种） | 处理器 key（D9） | 地图点种类 | 无 |

别名：`world/mappoint_kinds.yaml` → `map_point_kinds.yaml`；`world/adman.yaml` → `town/plaza.yaml`；`world/unlock_maps.yaml`（旧稿）→ `discovery/old_maps.yaml`（遗迹 / 旧地图解锁表只在那里，词表 §14-10）。

港口一条 DLL 记录按所有权拆四张初值表（D10）：world（本表）/ town-life（`town_map_points`，`+0x48`）/ factions（`town_shares_initial` `+0x4C`、`fashion_initial` `+0x7E`）/ economy（`special_products` `+0x28` 与 `+0x60` 段、`+0x82` 景气）。字段表见 [15-catalogs/towns.md](../15-catalogs/towns.md)。

---

## `config/factions/`

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `forces.yaml` | `dk4.factions.forces` | `02-factions/01-forces.md` | `key`（`eForce`） | `capital`、`major_sea` | 势力名、色、`is_private_fleet` | 金币、账本、关系 |
| `relations.yaml` | `dk4.factions.relations` | `02-factions/03-relations-influence.md` | 无序对 `(a, b)`，`a < b`（D10） | `force` ×2 | 默认友好度 | 当前关系 |
| `war.yaml` | `dk4.factions.war` | `02-factions/05-war-failure-absorb.md` | 规则 key | `force` | 宣战 / 吸收规则 | 战争状态 |
| `letters.yaml` | `dk4.factions.letters` | `02-factions/04-letters-licenses.md` | 信件 key | `force`、`town` | 介绍 / 推荐信、许可 | 持有 |
| `cultures.yaml` | `dk4.factions.cultures` | `02-factions/06-seven-culture-circles.md` | `key`（`eCulture` 21） | `major_sea`、`npc_face` | 文化圈行（名、大海域） | 无 |
| `countries.yaml` | `dk4.factions.countries` | `02-factions/02-countries-culture-fashion.md` | `key`（`eCountry`，计数 `[未知]`） | | 国名 | 无 |
| `fashion.yaml` | `dk4.factions.fashion` | 同上 | 规则 key | `trading_item` | 流行规则（每港 2 槽，槽值是交易品） | 各港当前流行 |
| `fashion_initial.yaml` | `dk4.factions.fashion_initial` | `01-world/02-towns.md` | `town` | `trading_item`（`null` = 无，原版 106） | DLL `+0x7E` 初值 | — |
| `town_shares_initial.yaml` | `dk4.factions.town_shares_initial` | 同上 | `(town, force)` | `force` | DLL `+0x4C` 3 槽初值（值 0 的槽丢弃，原版写 26） | 份额 |

别名：`factions/culture_prices.yaml` → `economy/culture_prices.yaml`；`factions/scenarios.yaml` → `narrative/scenarios.yaml` 的 `player_force`。

---

## `config/characters/`

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `abilities.yaml` | `dk4.characters.abilities` | `03-characters/03-abilities-skills-jobs.md` | `key`（`stamina agility charm intelligence spirit luck`） | | 六维名 | 无 |
| `skills.yaml` | `dk4.characters.skills` | 同上 | `key`（`vitality leadership helmsmanship surveying accounting fencing gunnery persuasion tactics observation`） | `stat_a`, `stat_b` → ability（`.data 0x1406864a0`） | 技能名、配对 | 无（技能是派生量） |
| `posts.yaml` | `dk4.characters.posts` | 同上；`05-ships-fleet/03` | `key`（16 岗位） | `room` → `ships/cabins` key；`skill` | 岗位定义、门槛 | 任命（由舱室绑定反查） |
| `jobs.yaml` | `dk4.characters.jobs` | 同上 | `key`（`eJobKind`） | | 职业 | 当前职业 |
| `zodiac.yaml` | `dk4.characters.zodiac` | `03-characters/03`；`11-progression/01` 只读 | `key`（12 星座；`.data 0x140688970`） | | `growth{六维 key}` | 无 |
| `vitals.yaml` | `dk4.characters.vitals` | `03-characters/07-fatigue-health-mood.md` | `physical_states[].key`（7）、`mental_states[].key`（6） | ability key | 乘子（修正器贡献）、恢复 / 恶化、转移表（按 key） | HP、身体态、精神态进存档 |
| `persons.yaml` | `dk4.characters.persons` | `03-characters/01-person-model.md` | `key`（`ePerson`） | `init_force`、`zodiac`、`home_town` | 姓名、`category`、初始六维、两段经验、`recruitable_roster`、`unknown_bytes` | 经验、六维、隶属、装备、HP、身心 |
| `recruitment.yaml` | `dk4.characters.recruitment` | `03-characters/04-recruitment-crew.md` | 规则 key | | 招募规则 | 无 |
| `presentation.yaml` / `bios.yaml` / `npc_faces.yaml` | `dk4.characters.presentation` / `.bios` / `.npc_faces` | `03-characters/05`、`06`；`02-factions/02` | `person` / 脸 key | | 头像、立绘、简介 key | 无 |

别名：`characters/protagonists.yaml` → `narrative/scenarios.yaml` 的 `protagonist`（不设单独表，05 目录树）；`characters/zodiacs.yaml` → `zodiac.yaml`；`characters/companions.yaml` → `narrative/companions.yaml`；`characters/protagonist_modifiers.yaml` → `narrative/scenarios.yaml` 的 `modifiers`；`characters/modifier_keys.yaml` → `modifiers.yaml`。

---

## `config/economy/`

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `trading_item_kinds.yaml` | `dk4.economy.trading_item_kinds` | `04-economy/01-trading-items.md` | `key`（19 类） | | 类名 | 无 |
| `trading_items.yaml` | `dk4.economy.trading_items` | 同上 | `key`（`eTradingItem` 105） | `kind`、`unlock_item`（解锁它的发现物；`plant_item` 是别名，§14-2） | `data_price`、`grade_step`、`initially_discovered`。**空货不是一行**（`null`） | 无 |
| `special_products.yaml` | `dk4.economy.special_products` | `04-economy/04-special-products-investment.md` | `(town, slot)` | `town`、`trading_item` | 每港槽（原版 5）：`trading_item`（交易品）、`listing_threshold`（+2 上架门槛）、`supply_base`（+4 供给基数）、`data_price`（`+0x60` 段槽底价）。**槽→交易品只在这里** | 余量、本月已购、待种锁 |
| `restock_calendar.yaml` | `dk4.economy.restock_calendar` | 同上 | 规则 key | | 补货日历 | — |
| `town_state_initial.yaml` | `dk4.economy.town_state_initial` | `04-economy/02-market-price.md` | `town` | `town` | DLL `+0x82` 景气状态初值（`default` + 只列非默认港） | 港口景气（economy 拥有） |
| `culture_prices.yaml` | `dk4.economy.culture_prices` | `04-economy/02-market-price.md` | `culture` | `culture`、`trading_item`（稀疏映射 + `default`） | 21 圈 × 交易品卖价底（数值列归 economy，D10） | 无 |
| `town_state_price_rules.yaml` | `dk4.economy.town_state_price_rules` | 同上 | 景气状态 key | | 状态价格系数；`lifecycle:`（月初结束 / 蔓延 / 新发三段，`04-economy/02` §2.3）；`contexts:`（含 `remodel` 行，原版值同 `ship_buy`，§14-14） | 港口景气 |
| `investment.yaml` | `dk4.economy.investment` | `04-economy/07-investment-tribute.md` | 规则 key | | 投资表 | 待转化投资、发展度 |
| `npc_trade.yaml` | `dk4.economy.npc_trade` | `04-economy/06-npc-fleet-trade.md` | 规则 key | `force` | NPC 舰队贸易 | — |
| `price_intel.yaml` | `dk4.economy.price_intel` | `04-economy/08-price-intel.md` | 规则 key | | 价格情报 | 已知价格 |

别名：`economy/price_curves.yaml`（旧稿）→ `culture_prices.yaml` + `town_state_price_rules.yaml`。交易会话（5×5 槽、卖出列表）是运行时状态，进存档船货。

---

## `config/ships/`

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `models.yaml` | `dk4.ships.models` | `05-ships-fleet/05-hull-catalog-market-start.md` | `key`（船模 39，`eShipType`） | | 船模、`size_class`、可买 | 无 |
| `hulls.yaml` | `dk4.ships.hulls` | `05-ships-fleet/01`、`05`、`06` | `key`（107 船体定义） | `model`、`npc_pool`、`dock_template_of` | 船体 | 无（实例在船状态） |
| `rig.yaml` | `dk4.ships.rig` | `05-ships-fleet/04-remodel-guns-supplies.md`（Ships 拥有；`06-ocean/01` 只读） | 无 | `add_parts` key | 帆值 / 必要水手贡献、grade 表 `{0,4,10}` `{3,5,10}` | 无（派生，D6） |
| `cabins.yaml` | `dk4.ships.cabins` | `05-ships-fleet/03-cabins-crew-posts.md` | `key`（23 种） | `skill` | 舱室种类 + `role`（`supply_room`、`cargo_room`、`gun_room`、`marine_room`、`rower_room` …） | 31 槽安装与乘员 |
| `add_parts.yaml` | `dk4.ships.add_parts` | `05-ships-fleet/04` | `key`（9） | | 改装件 | 已装 |
| `guns.yaml` / `shells.yaml` | `dk4.ships.guns` / `.shells` | `05-ships-fleet/08-gun-shell-catalog.md` | `key`（炮种 6） | | 炮、弹 | 炮位 |
| `figureheads.yaml` | `dk4.ships.figureheads` | `05-ships-fleet/04` | `key`（12） | `item`（道具 114–123、199、200） | `effects`（修正器贡献） | 安装 |
| `supplies.yaml` | `dk4.ships.supplies` | `05-ships-fleet/04` | `key`（水 / 粮） | | 补给种类 | 船上余量 |
| `crew.yaml` / `dock.yaml` | `dk4.ships.crew` / `.dock` | `05-ships-fleet/06`、`07` | 规则 key | | 水手上下限、停泊 | 停泊港 |

别名：`ships/types.yaml` → `models.yaml`；`ships/sails.yaml`、`ocean/crew_sail.yaml` → `rig.yaml`；`items/figureheads.yaml` → `ships/figureheads.yaml`。船首像原版效果（`effects`）：`fleet.speed` ×11/10、`supply.food_div` +2、`ship.durability_max` +10、`combat.gun_power` +2、`fleet.search_radius_add` +16、`fleet.speed` ×4/5、`combat.melee_damage` ×6/5。

---

## `config/fleets/`

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `templates.yaml` | `dk4.fleets.templates` | `05-ships-fleet/02-fleet-proxy.md` | `key`（`eFleet` 69） | `force`、`hull`、`person` | World 舰队模板 | 位置、编制、委任 |
| `proxy_rules.yaml` | `dk4.fleets.proxy_rules` | 同上 | 规则 key | `force` | 地方舰队规则 | 存活 |
| `formations.yaml` | `dk4.fleets.formations` | 同上 | 阵型 key | | 编队阵型 | 当前阵型 |
| `post_rules.yaml` | `dk4.fleets.post_rules` | 同上 | 无 | `post` | `holder_rule`：玩家旗舰取岗位 / 他船取船长 / 其它取提督（`Fleets.post_holder`） | — |

---

## `config/combat/`

| 文件 | schema | 所属文档 | 主键 | 定义 |
| --- | --- | --- | --- | --- |
| `naval.yaml` | `dk4.combat.naval` | `07-combat/01-naval-battle.md` | 无 | 海战参数、`modifiers`（D3 出口列表，旧 `modifier_keys` 作废） |
| `boarding.yaml` | `dk4.combat.boarding` | `07-combat/02-boarding.md` | 无 | 接舷参数 |
| `ai.yaml` | `dk4.combat.ai` | `07-combat/03-battle-ai.md` | `key`（`eBattleAlgo`） | AI 算法 |
| `happenings.yaml` | `dk4.combat.happenings` | `07-combat/05-formation-happening.md` | `key` | 海战场景 happening（不与海上 20 槽共 id） |
| `formations.yaml` | `dk4.combat.formations` | 同上 | `key` | 海战阵型 |
| `port_assault.yaml` / `prizes.yaml` | `dk4.combat.port_assault` / `.prizes` | `07-combat/04`、`06` | 规则 key | 攻港、战利品 |

战斗会话不进内容包。

---

## `config/ocean/`

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `sailing.yaml` | `dk4.ocean.sailing` | `06-ocean/01-sailing.md` | 无 | | 航行旋钮（公式在 `formulas/ocean.yaml`） | 位置、航向 |
| `sail_modes.yaml` | `dk4.ocean.sail_modes` | `06-ocean/04-manual-auto-sail.md` | 模式 key | | 手动 / 自动 | 当前模式 |
| `wind.yaml` | `dk4.ocean.wind` | `06-ocean/01` | 无 | `sea_area` | 风场采样 | 当前风 |
| `search.yaml` | `dk4.ocean.search` | `06-ocean/02-search-encounters.md` | 无 | `skill`、`post`、cabin `role` | 探索流程、富度 / 地区矩形表、地区事件 | 已发现 |
| `encounters.yaml` | `dk4.ocean.encounters` | 同上 | 遭遇 key | `fleet` 模板 | 遭遇表 | 无 |
| `happenings.yaml` | `dk4.ocean.happenings` | `06-ocean/05-sea-happenings.md` | 槽 key（20）、结局 key | | CDF、门闩、结局 `effects`（修正器贡献，如补给 95% / +10） | 舰队结局态（随舰队存档） |
| `npc_ai.yaml` | `dk4.ocean.npc_ai` | `06-ocean/06-npc-fleet-ai.md` | 规则 key | | NPC 舰队航行 | — |
| `gold_road.yaml` | `dk4.ocean.gold_road` | `06-ocean/03-gold-road.md` | 航路 key | `town`、`route` | 黄金航路 | 进度 |

---

## `config/town/`

| 文件 | schema | 所属文档 | 主键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- |
| `facilities.yaml` | `dk4.town.facilities` | `08-town-life/01-facilities.md` | 设施 kind | kind → 处理器 + 参数（D9） | 开闭 |
| `facility_rules.yaml` | `dk4.town.facility_rules` | `08-town-life/05-inn-church-palace.md` | 动作 key | `Action { cost, guard, roll, effects, repeat }` | — |
| `town_map_points.yaml` | `dk4.town.town_map_points` | `01-world/10-mappoint-kinds.md` | `town` | 每港地图点（初值 `+0x48`） | 可变 |
| `rite_rewards.yaml` | （在 `facility_rules` schema 内） | `08-town-life/05` | 仪式 key | 奖励 | 已领 |
| `plaza.yaml` | `dk4.town.plaza` | `08-town-life/06-plaza-intel-adman.md` | 无 | 广场、情报、`CAdman` | CAdman 状态 |
| `request_kinds.yaml` / `request_generation.yaml` / `request_persons.yaml` | `dk4.town.request_*` | `08-town-life/02-requests-schemes.md` | 种类 key / 规则 / 委托人 | 委托 | 公告槽、接取 |
| `scheme_kinds.yaml` / `plots.yaml` / `rhetoric.yaml` / `schemes.yaml` | `dk4.town.scheme_kinds` / `.plots` / `.rhetoric` / `.schemes` | `08-town-life/04-schemes-rhetoric.md` | key | 计谋入口（`post`、`skill` 用 key）、对抗、话术 | 冷却 / 结果 |
| `minigames.yaml` | `dk4.town.minigames` | `08-town-life/03-minigames.md` | 小游戏 key | `Minigames.run(id) -> int`（D8） | 无 |

别名：`town/town_facilities.yaml` → `town_map_points.yaml`。

---

## `config/items/` 与 `config/discovery/`

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `items/kinds.yaml` | `dk4.items.kinds` | `09-items/01-item-catalog.md` | `key`（13） | | 分类 | 无 |
| `items/items.yaml` | `dk4.items.items` | 同上 | `key`（`eItem` 231） | `kind` | 道具 | 持有、发现 |
| `items/effects.yaml` | `dk4.items.effects` | `09-items/02-equipment-effects.md` | `(item, slot)` | `item`、`skill` key | 装备贡献（`skill.<key>` `stage: add`） | 装备在人身上 |
| `items/planting.yaml` / `hunting.yaml` / `recipes.yaml` / `ancient_treasures.yaml` | `dk4.items.*` | `09-items/03`、`01`、`04` | key | | 种植、狩猎、配方、古代宝物 | 进度 |
| `discovery/old_maps.yaml` / `puzzles.yaml` / `hegemony_proofs.yaml` | `dk4.discovery.*` | `01-world/11`、`09-items/04` | key | `item`、`LocatorRef` | 古地图（遗迹 / 旧地图解锁表的唯一位置）、谜题、霸者之证 | 已用、进度 |

---

## `config/narrative/`

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `scenarios.yaml` | `dk4.narrative.scenarios` | `10-narrative/01-scenarios.md` | `key`（`eScenario` 7） | `protagonist`、`player_force`（`protagonist_force` 是作废别名，§14-1）、`world_pack`、`event_pack`、`map_point_unlock_table` | 剧本选包、`flags`（布尔）、`modifiers`、剧本专属表（D5） | 当前剧本 |
| `starting_fleets.yaml` | `dk4.narrative.starting_fleets` | `05-ships-fleet/05` | `(scenario, pack)` | `scenario`、`fleet`、`hulls` | 开局舰队（唯一登记处；`scenarios.yaml` 只以 key 关联，§14-12） | — |
| `companions.yaml` | `dk4.narrative.companions` | `03-characters/04`、`06` | `person` | | 同伴羁绊 | 好感 |
| `events.yaml` / `triggers.yaml` / `trigger_kinds.yaml` | `dk4.narrative.*` | `10-narrative/02`、`03` | 事件 / 触发器 key | 多（只读查询） | 数据触发器 + 结构化命令树（D8） | 已触发、旗标 |
| `opcodes.yaml` | `dk4.narrative.opcodes` | `10-narrative/03-event-commands.md` | opcode | | **仅 EV 导入器用** | — |
| `stills.yaml` / `gallery.yaml` | `dk4.narrative.stills` / `.gallery` | `10-narrative/02`、`05` | key | | 剧照、画廊 | 画廊点数在 profile |

---

## `config/progression/`

| 文件 | schema | 所属文档 | 定义 | 状态 |
| --- | --- | --- | --- | --- |
| `level.yaml` | `dk4.progression.level` | `11-progression/01-level-exp-skills.md` | 轨名、显示；**魔数在 `formulas/progression.yaml`** | 经验在人物状态 |
| `gameover.yaml` | `dk4.progression.gameover` | `11-progression/03-gameover-succession.md` | 终局条件 → `Session.end_scenario(outcome)` | — |
| `fame.yaml` | `dk4.progression.fame` | `11-progression/02-fame-titles.md` | 原版无名声；默认 `enabled: false` | — |

---

## `config/ui/` 与 `localization/`

| 文件 | schema | 所属文档 | 主键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- |
| `ui/scenes.yaml` | `dk4.ui.scenes` | `12-ui-save/01-scenes-flow.md` | 场景 key | 场景流 | 当前场景 |
| `ui/bgm.yaml` | `dk4.ui.bgm` | `01-world/08-sea-area-music-icons.md` | 绑定 key | 海域 / 港口 BGM | — |
| `ui/sea_map.yaml` | `dk4.ui.sea_map` | `01-world/08-sea-area-music-icons.md` | 图标 key | 海域图图标 | — |
| `ui/save.yaml` | `dk4.save` | `12-ui-save/02-save-session.md` | 无 | 槽数、校验、坏档策略、各 ID 域 `orphan_policy`、`legacy_codec` 引用、`unknown_sections: keep`、`rng_state` | — |
| `ui/anim/gold_road.yaml` | `dk4.ui.anim` | `06-ocean/03-gold-road.md` | 无 | 动画时间轴 | — |
| `ui/strings.zh-CN.yaml`（每语言一份 `strings.<lang>.yaml`） | `dk4.ui.strings` | `12-ui-save/03-localization.md` | 文案 key（由 exe 消息号表切句，D10） | **只文案** | 无 |
| `localization/languages.yaml` | `dk4.localization.languages` | 同上 | 语言码 | zh-CN / zh-TW / ja / ko ↔ 存档目录 ↔ 源编码 | 玩家语言 |
| `localization/messages.<lang>.map.yaml` | `dk4.localization.messages_map` | 同上 | exe 消息号 | 消息号 → `string_key`（D10） | 无 |

禁止把 UI 字符串写进逻辑配置。

---

## 存档（非 `config/`，字段分类对照，D7）

所属：`12-ui-save/02-save-session.md`。

存：启用包列表 + 版本、各表 schema version、`id_map`、RNG 主种子与各流状态、规则包版本（含 `tick.yaml`）、各系统**状态**节（每节自带版本，按节迁移链；列表按 key 存）、`mod:<id>` 节原样保留。跨存档数据（原版 `savedata.dk4`）是独立 `profile`。

不存：公式原文、技能值、等级（派生，可缓存但须能从经验重建）、派生量（D6）。

原版存档人物段（`0x2C03` 起 235 条，id<87 每条 25 B、其余 2 B）只在 `legacy_codec` 导入器里解释，见 04-id-registry。

---

## 校验规则

1. `engine_limits` → 合并后的 `caps`（caps ≤ 硬顶）→ `modifiers.yaml`（key 注册）→ `formulas`（`impl` 已注册、`expr` 可解析且无环、`golden` 通过、无剧本号比较、无字面量内容 id）→ 内容表（外键、`effects.key` 已注册、按 key 矩阵覆盖率）→ `narrative/scenarios.yaml` 选包 → 存档覆盖状态。
2. 外键 ∈ 注册表 ∪ `{null}`。`$op: delete` 后仍被引用 → 拒绝启动并列出引用方。
3. 内容表里出现原版哨兵整数（106、98、232、236 …）作外键 → 拒绝；它们只在 `legacy_codec`。
4. `unknown_bytes` 原样保存，禁止猜成确定字段后仍留在「已命名」区。
5. World0 / World1 同一 schema，`vanilla-world1` 对同一批 `vanilla:*` key `$op: patch`，不新建 key。
6. 同一数字只能有一个来源：`caps` 与 `formulas` 不得重复同名键；公式引用上限写 `$caps.<key>`；同一参数不在两个公式文件各写一份（例：`fashion_*`、`share_letter_cut`、`letter_*` 只在 `formulas/factions.yaml`）。
7. `tick.yaml` 的 `steps` 里每个系统名必须已注册；`schedule` 只能取五个时钟点。

## 模组扩展

包、命名空间、合并、存档 id 重映射的规则在 [05-config-architecture.md](../00-overview/05-config-architecture.md)。最小模组示例：

| 需求 | 写法 | 示例（`examples/mods/mymod/`） |
| --- | --- | --- |
| 新 ID | 新 `key`，不写整数 | `economy/trading_items.yaml` 加 `tea_brick`；`world/towns.yaml` 加 `new_port` |
| 覆盖 | `$op: patch` 深合并；无 key 列表用 `$merge_by` | `economy/special_products.yaml` 改伦敦 3 号槽；`formulas/economy.yaml` 改 `rand_span` |
| 删除 | `$op: delete`；所有引用同包先改掉；存档按 `orphan_policy` | `world/towns.yaml` 删 `vanilla:bristol` |
| 换公式 | `$op: replace` + `impl: expr` + `golden` | `formulas/items.yaml` 把道具店还价 `item_bargain` 改成线性折扣 |
| 加效果 | 新修正器贡献，不动公式 | `ships/figureheads.yaml` 新船首像 `fleet.search_radius_add` +8 |
| 加阶段 | `engine/tick.yaml` `$append` + `insert_after` | （见 tick 示例注释） |

## 示例文件

| 路径 | 覆盖 |
| --- | --- |
| [examples/caps.yaml](examples/caps.yaml) | 设计上限 |
| [examples/engine_limits.yaml](examples/engine_limits.yaml) | 硬顶 |
| [examples/modifiers.yaml](examples/modifiers.yaml) | 修正器 key 注册与阶段序（D2）；全部规范 key（10-key-glossary §6.2） |
| [examples/engine/tick.yaml](examples/engine/tick.yaml) | 阶段序列（D4，原版月初 / 月末 / 年初顺序） |
| [examples/ids/legacy_codec.yaml](examples/ids/legacy_codec.yaml) | 原版哨兵与存储宽度（D1） |
| [examples/ids/generated.snippet.yaml](examples/ids/generated.snippet.yaml) | 构建产物片段：key→int（无哨兵） |
| [examples/formulas/progression.yaml](examples/formulas/progression.yaml) | F-PROG-* |
| [examples/formulas/character.yaml](examples/formulas/character.yaml) | F-PROG-05 技能值、F-PROG-09 修正器堆叠 |
| [examples/formulas/items.yaml](examples/formulas/items.yaml) | F-ECO-02 `item_bargain`、装备加成参数 |
| [examples/formulas/economy.yaml](examples/formulas/economy.yaml) | F-ECO-01/07..10/13/14/16/18 |
| [examples/formulas/factions.yaml](examples/formulas/factions.yaml) | F-ECO-04/05/06 流行品份额与发信（片段） |
| [examples/formulas/combat.yaml](examples/formulas/combat.yaml) | F-CBT-01..04 |
| [examples/formulas/ocean.yaml](examples/formulas/ocean.yaml) | F-OCN-01..03 |
| [examples/characters/abilities.yaml](examples/characters/abilities.yaml) | 六维 6 行（词表 §1） |
| [examples/characters/skills.yaml](examples/characters/skills.yaml) | 十技能与两维配对（词表 §2） |
| [examples/characters/posts.yaml](examples/characters/posts.yaml) | 16 岗位 → 舱室种类 key（词表 §3） |
| [examples/characters/persons.yaml](examples/characters/persons.yaml) | 拉斐尔 |
| [examples/characters/vitals.yaml](examples/characters/vitals.yaml) | 身体 7 态 / 精神 6 态（片段） |
| [examples/characters/zodiac.yaml](examples/characters/zodiac.yaml) | 星座成长（片段） |
| [examples/world/towns.yaml](examples/world/towns.yaml) | 伦敦、布里斯托尔（只放 world 字段，含 `develop_initial`） |
| [examples/world/major_seas.yaml](examples/world/major_seas.yaml) | 七大海域（词表 §8） |
| [examples/world/map_point_kinds.yaml](examples/world/map_point_kinds.yaml) | 12 种地图点 kind |
| [examples/economy/trading_items.yaml](examples/economy/trading_items.yaml) | 煤 |
| [examples/economy/special_products.yaml](examples/economy/special_products.yaml) | 伦敦 5 槽（槽→交易品的唯一所有者；字段见词表 §10） |
| [examples/economy/culture_prices.yaml](examples/economy/culture_prices.yaml) | 文化圈卖价底（数值列归 economy，片段） |
| [examples/ships/rig.yaml](examples/ships/rig.yaml) | 帆值 / 必要水手参数 |
| [examples/ships/cabins.yaml](examples/ships/cabins.yaml) | 23 种舱室种类 + role（词表 §4） |
| [examples/ships/figureheads.yaml](examples/ships/figureheads.yaml) | 船首像修正器贡献（片段） |
| [examples/ocean/search.yaml](examples/ocean/search.yaml) | 探索补给检定（片段） |
| [examples/town/plots.yaml](examples/town/plots.yaml) | 酒馆计谋入口 |
| [examples/narrative/scenarios.yaml](examples/narrative/scenarios.yaml) | 剧本 flags / modifiers（D5） |
| [examples/ui/save.yaml](examples/ui/save.yaml) | 存档旋钮、`orphan_policy` |
| [examples/mods/mymod/](examples/mods/mymod/) | 模组：新 ID、覆盖、删除、换公式、加修正器 |

系统文档 01–12 若改文件名，只改本表路径与 05-config-architecture 的别名表。
