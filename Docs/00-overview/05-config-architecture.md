# 配置架构

## 目录

运行时只读 `config/`。本 Docs 里的 YAML 是规格示例，真正的表由内容管线从原版 DLL / mesfile 生成后再人工补名字。

```
config/
  caps.yaml                 # 设计上限（技能值 500、六维 100、等级 100 …）
  engine_limits.yaml        # 实现硬顶，模组不可改
  modifiers.yaml            # 修正器 key 注册 + 阶段序（原则 6，D2）
  engine/
    tick.yaml               # 阶段与系统顺序（见「tick 顺序」，D4）
    rng.yaml                # 随机数流（见「随机数」）
  formulas/                 # 只放算法与参数（D3）
    progression.yaml  character.yaml  vitals.yaml
    economy.yaml  investment.yaml  tribute.yaml
    factions.yaml           # 含流行品 / 发信参数 fashion_*、share_letter_cut、letter_*（消费方 factions）
    ships.yaml  fleets.yaml  supply.yaml
    ocean.yaml  vision.yaml  discovery.yaml  climate.yaml  routes.yaml  world.yaml
    combat.yaml  town.yaml  rumors.yaml  items.yaml
  ids/
    generated.json          # 构建时从各表收集（key ↔ vanilla_index）
    legacy_codec.yaml       # 原版哨兵与存储宽度，仅导入 / 导出（D1）
  world/
    map.yaml                # World.dat 引用、坐标系（chip_unit 16、2500×1250 u16）
    maps.yaml               # 航海图 / 航线图 / 黑块图
    calendar.yaml  coords.yaml
    sea_areas.yaml          # 细分航海区；原版为空（25 分区是投票误采），只作模组扩展
    major_seas.yaml         # 七大海域；ID 域登记在 02-factions/06
    towns.yaml              # 港口定义（坐标、culture、remains、首都）+ develop_initial 列（发展度初值，归 world Towns）
    villages.yaml  remains.yaml        # 遗迹 / 旧地图解锁表只在 discovery/old_maps.yaml
    routes.yaml  junctions.yaml
    climate.yaml            # 风场、天气 CDF
    map_point_kinds.yaml    # 12 种地图点
  factions/
    forces.yaml  relations.yaml  war.yaml  letters.yaml
    cultures.yaml           # 21 文化圈行（名、major_sea、npc_face）
    countries.yaml          # 19 国名 [推断]
    fashion.yaml  fashion_initial.yaml
    town_shares_initial.yaml
  characters/
    abilities.yaml  skills.yaml  posts.yaml  jobs.yaml
    persons.yaml  recruitment.yaml     # 可当主角的人物由 narrative/scenarios.yaml 的 protagonist 指定，不设 protagonists.yaml
    vitals.yaml             # 身体 7 态 / 精神 6 态：系数、恢复、转移
    zodiac.yaml             # 12 星座 × 6 成长权重
    presentation.yaml  bios.yaml  npc_faces.yaml
  economy/
    trading_item_kinds.yaml  trading_items.yaml
    special_products.yaml   restock_calendar.yaml
    town_state_initial.yaml # 港口景气状态初值（+0x82；状态归 economy）
    culture_prices.yaml     # 21×105 卖价底（数值列归 economy，D10）
    town_state_price_rules.yaml
    investment.yaml  npc_trade.yaml  price_intel.yaml
  ships/
    hulls.yaml              # 107 船体定义（槽 100–206）
    models.yaml             # 39 船模（替代旧 types.yaml）
    rig.yaml                # 桅 / 帆 / 必要人手贡献（派生量归 Ships，D6；只在 05-ships-fleet 定义，ocean 只引用）
    cabins.yaml             # 23 舱室种类（原版「无」不进表）+ role + 技能
    add_parts.yaml  guns.yaml  shells.yaml
    figureheads.yaml        # 船首像修正器贡献（只在 05-ships-fleet 定义，ocean / combat 只引用）
    supplies.yaml  crew.yaml  dock.yaml
  fleets/
    templates.yaml  proxy_rules.yaml  formations.yaml
    post_rules.yaml         # holder_rule：玩家旗舰取岗位 / 他船取船长 / 其它取提督（Fleets.post_holder）
  combat/
    naval.yaml  boarding.yaml  ai.yaml  happenings.yaml
    formations.yaml  port_assault.yaml  prizes.yaml
  ocean/
    sailing.yaml  sail_modes.yaml  wind.yaml
    search.yaml  encounters.yaml  happenings.yaml
    npc_ai.yaml  gold_road.yaml
  town/
    facilities.yaml         # kind → 处理器 + 参数（D9）
    facility_rules.yaml     # Action 定义：旅馆 / 仪式 / 酒馆 …
    town_map_points.yaml    # 每港有哪些地图点（初值来自港口 +0x48）
    rite_rewards.yaml  plaza.yaml
    request_kinds.yaml  request_generation.yaml  request_persons.yaml
    scheme_kinds.yaml  schemes.yaml  plots.yaml  rhetoric.yaml   # schemes = 24 行计略槽定义
    minigames.yaml
  items/
    kinds.yaml  items.yaml  effects.yaml
    planting.yaml  recipes.yaml  hunting.yaml  ancient_treasures.yaml
  discovery/
    old_maps.yaml  puzzles.yaml  hegemony_proofs.yaml
  narrative/
    scenarios.yaml          # 世界包、事件包、player_force、flags、modifiers（D5）；不写开局舰队
    starting_fleets.yaml    # 开局舰队唯一定义处（按剧本 key 查行 → 舰队 + 船体）
    companions.yaml
    events.yaml  triggers.yaml  trigger_kinds.yaml
    opcodes.yaml            # 仅 EV 导入器用（D8）
    stills.yaml  gallery.yaml
  progression/
    level.yaml  gameover.yaml
    fame.yaml               # 原版无名声；默认 enabled: false
  ui/
    scenes.yaml  bgm.yaml  sea_map.yaml   # sea_map = 海域图图标
    save.yaml               # 槽数、checksum、坏档策略、orphan_policy、legacy_codec、unknown_sections、rng_state、profile 节
    anim/gold_road.yaml
    strings.zh-CN.yaml      # 只文案
  localization/
    languages.yaml          # language_id ↔ 存档目录（ja / ko）↔ 源编码
    messages.<lang>.map.yaml   # exe 消息号 → string_key（D10）
```

同名异路以本树为准，旧写法视为别名：`characters/zodiacs.yaml` → `characters/zodiac.yaml`；`world/mappoint_kinds.yaml` → `world/map_point_kinds.yaml`；`factions/culture_prices.yaml` → `economy/culture_prices.yaml`；`items/figureheads.yaml` → `ships/figureheads.yaml`；`characters/modifier_keys.yaml` → `modifiers.yaml`；`ocean/crew_sail.yaml`、`ships/sails.yaml` → `ships/rig.yaml`；`ships/types.yaml` → `ships/models.yaml`；`import/vanilla.yaml` → `ids/legacy_codec.yaml`；`characters/protagonist_modifiers.yaml`、`factions/scenarios.yaml` → `narrative/scenarios.yaml` 的 `modifiers` / `player_force`（旧字段名 `protagonist_force` 作别名）；`characters/protagonists.yaml` → `narrative/scenarios.yaml` 的 `protagonist`；`world/unlock_maps.yaml` → `discovery/old_maps.yaml`；`town/town_facilities.yaml` → `town/town_map_points.yaml`；`world/adman.yaml` → `town/plaza.yaml`；`characters/companions.yaml` → `narrative/companions.yaml`。完整别名表（含 `world/sea_zones.yaml`、`economy/price_curves.yaml`、`factions/major_seas.yaml` 等）见 `10-key-glossary.md` §9。

## 加载顺序

1. `engine_limits.yaml` — 实现硬顶（只有引擎自带，模组不可改）
2. 选定本次运行的**包集合**：剧本记录的 `world_pack` 字段决定世界包（原版 `fn_14000ff60` 写死 `scenario_id >= 4` → World1；重构不比较剧本号），再加 `vanilla-core`、语言包、启用的模组
3. 按依赖拓扑排序包（见「包清单」），同层按清单里的 `load_order` 再按包 id 字典序，保证确定
4. 依次叠加每个包的各表（见「覆盖与合并」），得到合并后的定义
5. 建 ID 注册表：给每个命名空间 key 分配运行时整数（见「ID 与命名空间」）
6. 校验：外键、`caps ≤ engine_limits`、公式 `impl` 存在且 `golden` 通过、修正器 key 已注册、按 key 矩阵覆盖率、各语言 key 集合相等、公式里无剧本号比较
7. 读档：按存档里的 key↔int 表重映射，跑迁移链，再覆盖状态字段

校验失败则拒绝启动。缺字段不回退到代码默认值（原版「双轨 fallback」禁止）。caps 必须在模组叠加**之后**校验，因为模组可以改 caps。

## 内容包

```
packs/
  vanilla-core/       # exe 内表：交易品、船型、村落、遗迹、航线、公式默认参数
  vanilla-world0/     # 对应 World0.dll（五张表）
  vanilla-world1/     # 对应 World1.dll
  vanilla-sc-text/    # mesfile SC
```

剧本记录：

```yaml
# config/narrative/scenarios.yaml
- key: vanilla:raphael
  vanilla_index: 0              # 原版 eScenario；仅导入用（不是运行时下标）
  protagonist: vanilla:raphael_castro
  player_force: …               # force key；原版 fn_140009df9 表 {0,2,1,3,20,13,21}，剧本 0 → force 0
  # 不写舰队 / 船体：开局舰队只在 narrative/starting_fleets.yaml，按本剧本 key 查行
  world_pack: vanilla-world0
  event_pack: EV0
  scene_pack: Sc0
  animation_pack: EVA
  flags: {}                     # D5：只放布尔开关，公式里写 enabled_by: scenario_flag.<key>（如剧本 6 的宗主上缴）
  modifiers:                    # D5：乘除差异走修正器，开局注入注册表，source = { kind: scenario, key: vanilla:raphael }
    - { key: request.reward_gold,          stage: mul, value: { num: 3, den: 2 } }    # 委托报酬 ×1.5
    - { key: invest.military_min_fee_pct,  stage: mul, value: { num: 8, den: 10 } }   # 军事投资八折
```

完整字段见 `10-narrative/01-scenarios.md`。

## 包清单

每个包根目录一个 `pack.yaml`：

```yaml
id: mymod                  # 命名空间，[a-z0-9_]+，全局唯一
version: 1.2.0
requires: [{ id: vanilla-core, version: ">=1" }]
load_after: [vanilla-world0]
conflicts: []
schema_versions: { dk4.world.towns: 2 }
```

`requires` / `load_after` 决定拓扑序；有环或缺依赖则拒绝启动。

## ID 与命名空间

- 主键是 `<pack>:<key>` 字符串（`vanilla:london`）。包内引用可省前缀，默认本包，其次 `vanilla`。
- 运行时整数是加载后分配的紧凑下标，类型 `u32`，只在进程内用，不写进 YAML，不跨存档假定不变。原版条目另带 `vanilla_index`（原枚举值，伦敦 = 0），只供导入 / 导出原版 DLL 与存档。模组不写任何整数 ID。
- 存档写一张 `id_map: { "vanilla:london": 0, "mymod:new_port": 97, … }`。读档时按 key 重映射；某 key 已不存在 → 走该 ID 域的 `orphan_policy`（`drop` / `replace_with: <key>` / `refuse_load`），单条引用可用同名字段覆盖。
- 运行时空值一律 `null` / `Option<Id>`，**不由计数推导**。原版哨兵（`0x6A`、`0x62`、`0xE8`、`0xEC`、`0x46`、`0xC9`、`0x16`、`0x1A` …）只在 `ids/legacy_codec.yaml` 里按「枚举 × 字段」声明，导入时翻成 `null`，导出原版格式时 id 越界则拒绝导出（不截断）。原因：哨兵 = NUM 或 NUM+1，正落在模组顺序分配的区间里（第 2 个模组港 = 98 = 原版「无港」）。见 `04-id-registry.md`「遗留哨兵与存储宽度」、`09-architecture-decisions.md` D1。
- 原版靠 id 区间表达的语义（主角 0–6、港口女性 214–234、船体槽 `id-100`、槽 < 194 进 NPC 池）改成显式字段；按下标排的矩阵（文化圈 21×105 卖价、`CTown+0x4A` 19 种类）改成按 key 稀疏映射 + 默认值 + 加载期覆盖率校验。
- World0 / World1 共用 `vanilla:*` key：`vanilla-world1` 对同一批 key `$op: patch`，不新建。

```yaml
- key: london
  name_key: town.london.name
  # id 不写；原版包由导入器写 vanilla_index: 0
```

## 覆盖与合并

后加载的包对同一张表的同一 key 有四种写法，必须显式：

| 写法 | 语义 |
| --- | --- |
| 新 key | 追加 |
| `$op: replace` | 整条替换 |
| `$op: patch`（默认） | 深合并：map 递归合并，标量后者覆盖，列表**整体替换** |
| `$op: delete` | 删除；任何外键仍指向它则校验失败 |

列表要增量改时用显式操作：`$append`、`$remove: [<key>]`、`$insert_after: <key>`。带 `key` 字段的列表按 key 合并，不按下标；无 `key` 的列表用 `$merge_by: <field>` 指定合并字段，否则只能整体替换。

caps 与 formulas 不得重复同一参数；公式里引用上限写 `$caps.<key>`，加载期解析。

```yaml
# packs/mymod/world/towns.yaml
- key: vanilla:london
  $op: patch
  special_products:
    $append: [{ trading_item: mymod:tea_brick }]
```

冲突：两个包 `patch` 同一字段 → 加载序靠后者胜，启动日志列出 `(表, key, 字段, 胜者, 败者)`。`replace` / `delete` 与他包的 `patch` 冲突 → 默认报错，除非后者在 `load_after` 里显式声明前者。公式参数、caps 同样按此规则合并。

## Schema 版本与迁移

每个 YAML 头：

```yaml
schema: dk4.world.towns
version: 1
```

- 引擎按 `schema` 选解析器。加可选字段升小版本（旧包照读，缺省值写在 schema 里，不在代码里）；删 / 改语义升大版本，旧包要经内容迁移脚本或拒绝加载。
- 存档单独有 `save_version` 与各系统状态段版本。读档按 `vN → vN+1` 逐步迁移，每步是纯函数并有测试样本；不支持跳步。细节见 `12-ui-save/02-save-session.md`。

## 本地化 key

- 定义表只存 `name_key` / `desc_key`，不存文本。key 用命名空间字符串，不用整数：`town.london.name`、模组 `mymod:town.new_port.name`。原版 `skill.<id>.desc` 一类以下标为 key 的写法改成 `skill.accounting.desc`。
- 语言包是普通包，可被模组覆盖（同一合并规则）。缺 key 校验失败（与 `12-ui-save/03-localization.md` 一致）。可选开关 `allow_mod_language_fallback`（默认关）让只带一种语言的模组能跑，原版包不得依赖它。

## tick 顺序

原版顺序是一个函数写死的（`02-original-architecture.md`「世界对象与原版 tick 顺序」）。重构把它变成数据（D4）：

```yaml
# config/engine/tick.yaml
schema: dk4.engine.tick
version: 2
# 每日拍数不在这里定义：caps.calendar_ticks_per_day（原版 48 = 0x30）
# schedule 由 Clock 提供：tick | day_start | month_start | month_end | year_start
# 原版「半年」门（fn_14001374b）是 month_start 的子集，不单列；需要时订阅方按月份过滤
phases:                                   # 每拍按序执行；注释是原版世界 tick 的序号
  - { id: climate,      system: world.climate,  schedule: [day_start] }       # 1 CWorld::vfunc_04
  - { id: npc_fleets,   system: fleets,         schedule: [tick] }            # 2 CFleetElement
  - id: items                                   # 3 CItemElement::vfunc_04（仅月末最后一拍）
    system: items
    schedule: [month_end]
    steps: [economy.treasure_rent]              # 出租费（economy 执行）；早于 towns 的独占上缴
  - id: daily                                   # 2 内的玩家舰队日步 fn_140005a15（原名 DailyTick）；在港 / 海上都跑
    schedule: [day_start]
    steps:                                      # 按舰队港槽分支（原版同一函数两条路）
      in_port:                                  # 港槽非空；旅馆每夜快进同样走这里
        - fleets.fatigue                        # 只恢复疲劳 -(rand(5)+2)
      at_sea:                                   # 港槽为空
        - ocean.supply_drain                    # 补给日耗 fn_140005aa6
        - fleets.fatigue                        # 舰队疲劳日增量
        - characters.vitals                     # 人物 HP / 身心（读掷骰前的 happening 态）
        - ocean.happening_roll                  # 海上事件掷骰 fn_1400f2d90
        - characters.mental_transition          # 事件引起的精神转移 fn_1400ec260
        - ocean.happening_persist               # 持续态 fn_1400f0450
  - id: duty_exp                                # 在岗经验：packed % 0x90 == 0 且在海上（fn_140009b24）
    system: progression.duty_exp
    schedule: [day_start]
    every_n_days: 3
    when: at_sea
  - { id: persons,      system: characters,     schedule: [day_start] }       # 4 CPersonElement
  - id: towns                                                                  # 5 CTownElement（原版一个函数）
    schedule: [month_start, month_end]
    steps:
      month_start:
        - economy.town_state_update                # 景气状态切换（CTownElement::vfunc_04 前半，未拆；与漂移的先后 [推断]）
        - economy.price_drift                      # 先全体市价漂移
        - { per: town, steps: [economy.investment_convert, economy.restock] }   # 再逐港交错：转化 → 补货
      month_end:   [economy.exclusive_tribute]                                 # 月末最后一拍（出租费已在 items 阶段）
  - id: forces                                                                 # 6 CForceElement（AI、宗主上缴）
    system: factions
    schedule: [month_start]
    steps: [factions.suzerain_tribute]
  - { id: schemes,      system: town.schemes,   schedule: [day_start] }       # 7
  - { id: requests,     system: town.requests,  schedule: [month_start] }     # 8
  - { id: star_goods,   system: economy.star_goods, schedule: [year_start] }  # 9 星品（年初 = 0 月月初第一拍，在当月补货之后）
  - { id: gold_road_bonus, system: ocean.gold_road, schedule: [month_start] } # 10 fn_1400108fc
  - { id: player_fleet, system: ocean,          schedule: [tick] }            # fn_140011414
```

- 每个 phase / step 结束是一个事件派发边界（原则 5）。`steps` 写死同一日历点上的先后，不靠订阅注册顺序。`steps` 是 map 时，键要么是 schedule 门（`towns` 的 `month_start` / `month_end`），要么是舰队位置分支（`daily` 的 `in_port` / `at_sea`）。
- `towns` 子步骤按原版实测（`04-economy` 第二轮核对）：月初先市价漂移（景气状态切换在同一函数前半，先后 `[推断]`），再在同一逐港循环里「投资转化（`0x1402679cf`）→ 补货（`0x1402679f8`）」；独占上缴在月末最后一拍（独占门 `0x140267a0a`）；出租费 `economy.treasure_rent` 在第 3 行 `items` 阶段（`CItemElement::vfunc_04`），先于 towns 的独占上缴；星品 / 品牌在年初，晚于当月补货。`per: town` 表示逐实体交错：后序港当月转化解锁的货，前序港当月不补——这是原版怪癖，改成 `per: phase`（先全部转化再全部补货）会改变数值。tick 配置因此支持 `per: town | phase` 粒度。
- 第 10 步 `fn_1400108fc`（body `0x140291650`）`[确认]`：扫 105 种交易品，对 `fn_1400040ca` 返回 4 且满足到货谓词 `fn_140004eee` 的品种累加 `fn_140001384` 奖金，金币经 `fn_140002cb6`（封 99,999,999）入账，文案「从公会获得了报告黄金航路的奖金，金币%d枚。」。
- `daily`（旧称 DailyTick，是阶段不是事件）不限海上：在港分支只恢复疲劳（旅馆每夜快进同样触发）；海上分支按原版 `fn_140005a15` 调用序：补给日耗 → 舰队疲劳 → 人物 HP / 身心 → 海上事件掷骰 → 事件精神转移 → 持续态；人物 HP 用掷骰前（前一日）的事件态（`03-characters/07` §2.11、`06-ocean/01` §4.4）。在岗经验 `duty_exp` 每 3 天（`0x90` 拍）、仅在海上结算，按出港天数算，不是月结（`11-progression/01`）。改成「先事件后人物」是合法的模组配置，但数值会变。
- 叙事不占 tick 阶段：按 D8 订阅入港、港内、航海日推进等事件，在派发边界扫描触发器。
- 系统只订阅 `DayStarted` / `MonthStarted` / `MonthEnding` / `YearStarted`，不读 `SCalendar+0x1c` 一类字段。
- 「过 n 天」用 `Clock.advance_days(n)`；`add_days_mode: simulate`（原版：`fn_14000b122` 逐拍跑 48·n 次完整调度）| `jump`。
- 模组插入新系统用 `insert_after: <id>`，不改原版条目顺序。
- 原版包的顺序与门照抄上表；改顺序会改变数值结果（后跑的系统能看到先跑的写入），因此 `tick.yaml` 进存档的「规则包版本」。

## 随机数

`[确认]` 原版只有一个玩法随机源：`fn_140005272(n)`（body `0x140140a70`），525 个调用点。

```
if (first_call_flag) { srand(time(NULL)); first_call_flag = 0; }   # DAT_14068357c，只在此处读写
return n > 0 ? rand() % n : 0;
```

`rand` 是 MSVC CRT 每线程 LCG：`state = state * 214013 + 2531011; return (state >> 16) & 0x7FFF`（body `0x1404abbc4`）。来源：反汇编 `0x140140a70`、`0x1404abbc4`、`0x1404abbf8`（`srand`）、`0x1404abb64`（`_time64`）。

推论：

- 原版随机不可重现：种子是进程启动后首次掷骰时的时间；读档不会重新播种，序列与存档前无关。`[确认]`（`srand` 全 exe 只有这一个调用点，标志 `DAT_14068357c` 只在此函数读写）
- `rand() % n` 有取模偏差；`n > 32768` 时高位永远取不到。原版 `n` 都远小于此。
- 所有系统共用一条序列，任何一处多掷一次都会改变后面所有结果。
- 开局输入生日的 `SGameSeed`（`10-narrative/01-scenarios.md`）**不是** RNG 种子。
- exe 里另有 64 位 `*0x343FD + 0x269EC3` 的 LCG（`0x14031d069`、`0x140322d9d` 一带），`[推断]` 属 KTGL 表现层，不进玩法。

重构合同：

```yaml
# config/engine/rng.yaml
schema: dk4.engine.rng
version: 1
algorithm: pcg32                # vanilla_msvc 仅供对拍测试
seed_source: new_game           # 开局生成，写进存档
streams:                        # 每系统独立流，互不扰动
  [climate, ocean, encounters, combat, economy, characters, narrative, ai]
save_state: true
```

- 每系统一条命名流，派生自主种子 + 流名哈希。模组新系统加新流，不扰动原版流。
- 流状态进存档；读档后继续同一序列。
- 公式通过参数拿流句柄（原则 3），不得自取全局随机。
- `algorithm: vanilla_msvc` + 单一共享流 + `rand() % n` 只用于和原版逐调用对拍；正常运行用分流。

## 生成管线（建议）

```
World0/1.dll ─┐
DK4HD_sc.exe ─┤  (.rdata / .data 表 → vanilla-core)
mesfile.dk4 ──┼─→ tools/extract_content.py → packs/vanilla-*/ ─→ 人工补 key/注释
EV/Sc/EVA ────┤
World.dat ────┘
```

- 提取器只输出「确认」字段。hex 残余进 `unknown_bytes` 数组，等字段命名后再升级 schema。
- exe 地址按 PE 段换文件偏移（`.text` −`0x140000A00`、`.rdata` −`0x140001200`、`.data` −`0x140001600`），见 `_meta/source-map.md`。exe 内容表（船体、船模、炮、舱室、岗位、文化圈、技能配对、交易品目录、村落、航线、折点、风场、星座、消息号表）一律由脚本导出，不手抄（D10）。
- 一条 DLL 港口记录按所有权拆成四张初值表：world（坐标、culture、remains、首都）/ town-life（地图点位域 `+0x48`）/ factions（份额槽 `+0x4C`、流行 `+0x7E`）/ economy（特产品槽 `+0x28`、槽底价 `+0x60` 段、景气状态 `+0x82`）。同一行跨系统：一个系统拥有 id 与外键，另一个拥有数值列（文化圈行归 factions，105 卖价列归 economy）。字段表见 `15-catalogs/towns.md`「记录布局」。
- 文案 key 用 exe 消息号表（`.data 0x140683620` 37 块 `{first_id, size}` + `0x1406836c0` 每条 u16 偏移）切句，不用 mesfile 字节偏移。
- `NUM_*` 计数要带证据类型（判界 / 循环上界 / RTC 投票）；投票值只能作 `[推断]`。

## 与存档的关系

存档存（D7）：

- 启用的包列表（id + version）+ 各表 schema version
- `id_map`（命名空间 key ↔ 运行时整数）
- RNG 主种子与各流状态
- 各系统状态，**分节**、每节自带版本、按节注册迁移链；列表一律按 id / key 存，不按下标；跨系统引用由 Save 层统一 remap（见 `12-ui-save/02-save-session.md`）
- 模组状态在 `mod:<id>` 节原样保留：模组暂未启用也不丢
- 不存公式原文（避免旧存档锁死数值）；存「公式包版本」，读档时用当前包重算派生值（技能、等级缓存、必要水手等，D6）

跨存档数据（通关位、画廊点数、博物馆 / 图鉴位，原版 `savedata.dk4`）是独立 `profile`，不进槽。槽数、校验、坏档策略、各 ID 域 `orphan_policy` 进 `ui/save.yaml`。

原版存档是无标签字段流 + 全局格式版本 + 剧本 EV 版本锁（头 `+0x24`），任何内容变化都会拒档；不要照搬。原版存档人物段从 `0x2C03` 起 235 条变长记录（id < 87 每条 25 字节，其余 2 字节），导入器按此成帧，不用 dk4save 的「98 行 × `0x19`」（见 `12-ui-save/02-save-session.md`）。

原版技能不存、等级有缓存（`save+0x70`）。重构：经验是源，等级/技能是派生，缓存可有但必须能从源重建。
