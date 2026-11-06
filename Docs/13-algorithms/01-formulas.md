# 公式总表（Master Formula Catalog）

本文是重构引擎的**唯一公式目录**。系统文档（`Docs/01-world` … `Docs/12-ui-save`）若再写算法，只引用这里的公式名，不另发明一套符号。

初稿从 `srcdata/docs` 汇编。域文档成文后，下列公式以域文档为准并回链本表，禁止第三套符号：

- 航速 / 风力档 / 划桨：`Docs/06-ocean/01-sailing.md`（`fn_14001444d`）；帆值与必要水手归 Ships（`05-ships-fleet/06-crew-headcount.md` §4.1）
- 海战射程、装填与溃败：`Docs/07-combat/01-naval-battle.md`
- 接舷 `500-a`、白刃、无画面结算：`Docs/07-combat/02-boarding.md`；投降：`07-combat/04-ship-vs-port.md`
- 市价报价、指数漂移、文化圈卖价：`Docs/04-economy/02-market-price.md`
- 流行品份额与信件：流程 `Docs/04-economy/03-trade-loop.md`，参数与执行 `Docs/02-factions/03`、`04`（`formulas/factions.yaml`，词表 §14-9）；会计长杀价：`Docs/04-economy/03-trade-loop.md`
- 特产品供给：`Docs/04-economy/04-special-products-investment.md`
- 技能名：`Docs/03-characters/03-abilities-skills-jobs.md`；两维配对以本表 F-PROG-05（exe `.data` 表）为准
- 等级 / 经验 / 成长：`Docs/11-progression/01-level-exp-skills.md`
- 投资 / 上缴 / 出租：`Docs/04-economy/07-investment-tribute.md`（F-ECO-11 / F-ECO-12）
- 补给日耗、视野、木工折扣：[03-adjustment-factors.md](03-adjustment-factors.md)（F-ADJ-*）

本表与域文档冲突时，以原始指令为准，两边一起改；本表只登记域文档已 `[确认]` 的式子，不另造数。

配置落点：`config/formulas/*.yaml`。上限落点：`config/caps.yaml`、`config/engine_limits.yaml`（见 [02-caps-ranges.md](02-caps-ranges.md)）。

## 职责边界

管：

- 命名纯函数：输入、参数、输出、溢出、所属系统、源路径
- 把原版魔数收成 `params`（`exp_divisor`、`ref_1000`、`ref_500`、`ref_100` …）
- 每条公式的整数语义（`int_mode`）与原版怪癖（`quirks`）
- 公式输出接入修正器注册表的 key（`Modifiers.eval(key, ctx)`）

不管：

- 内容实例（港口、人物、商品行）
- UI 排版（价格列 `%6d` 宽度不是公式）
- 存档字节布局（见 `12-ui-save` / `srcdata/docs/save_format.md`）

## 原版事实

| 标记 | 含义 |
| --- | --- |
| `[确认]` | 有公式、有立即数、有补丁注释或有手写还原 |
| `[推断]` | 从布局/调用/反编译残式推出，除数或语义未钉死 |
| `[未知]` | 有玩法入口或符号，式子未还原 |

原版把技能值直接塞进买货、战斗、道具价（`fn_14000e1ce(person, k)` 的 `k` 是 `eSkill`）。重构后业务公式的输入单位是**技能值**（0–`skill_value_cap`=500），由 Progression 算、经修正器注册表交出；各系统不读六维原值（上限 `ability_cap`=100），也不比较字面量 id。

## 实体与 ID

公式用稳定英文名，YAML key 同名。不分配整数 ID。

技能下标是 `eSkill`（`NUM_SKILL=10`）。原版 `Ability(person, k)`（`fn_14000e1ce` → `fn_1400135cf`）按 **技能下标** 取值，不是六维下标：买货/道具价用 `k=4`（会计术），接舷结算用 `k=1`（统率力），白刃用 `k=5`（剑术）。`[确认]` 依据：`k` 一路传到 `fn_1400099e9(k, j)` 查技能→两维表（见 F-PROG-05）。技能名见 `03-characters/03-abilities-skills-jobs.md`。

`fn_1400099e9`（body `0x1401ca010`）检查 `skill ∈ [0,10)` 且 `j ∈ [0,2)`，返回 `.data 0x1406864a0` 处 `int32[10][2]` 的 `eAbility`；越界返回 `7`。`[确认]` 配对表见 F-PROG-05。`NUM_SKILLABILITY` `[推断]` 即第二维 2。内容落 `config/characters/skills.yaml`。

## 溢出总则

| 原版 | 重构必须 |
| --- | --- |
| 单段 `q=exp/30 ≥ 65536`（等级 ≥ 256）时 `isqrt` 内 `q*0x10000` 溢出 32 位：结果错乱，`q` 为 65536 整倍数时 `div` 除零 | **饱和**到 `level_per_track`，禁止绕回 |
| `skill_value_cap > 1000`（补丁）时 NPC 买货 `(1000-a)` 为负，无符号 `div` 记几百万；原版 500 碰不到 | 先 `clamp` 再减；用有符号饱和算术 |
| `a > 500` 时 `500-a`、`100-a/50` 同类变负 | 各公式自带 `*_clamp`，与结算上限解耦 |
| 经验写入 `min(xp, 30*track²)`，四处立即数 | 按 `exp_from_level(level_per_track)` 重算，不写死 `300000` |

`engine_limits.yaml` 给整数宽度；`caps.yaml` 必须 ≤ 硬顶。缺字段不回退代码默认值。

## 命名参数与修正器

原版魔数变成参数。基准常数用 `ref_*`，避免代码里出现裸 `1000`/`500`/`100`：

| 参数 | 原版 default | 用途 |
| --- | --- | --- |
| `exp_divisor` | 30 | 经验开方除数 |
| `level_bias` | 100 | 技能公式 `min(level, skill_level_cap) + level_bias` |
| `skill_divisor` | 150 | 技能公式分母 |
| `ref_1000` | 1000 | NPC 买货基准 |
| `rand_span` | 100 | `rand(rand_span)` → `[0, rand_span)`；原版 `fn_140005272` = `n>0 ? rand()%n : 0`（取模偏差照旧） |
| `buy_ability_clamp` | 500 | 买货用技能值夹紧，**独立于** `skill_value_cap` |
| `ref_500` | 500 | 接舷结算基准 |
| `battle_ability_clamp` | 500 | 接舷用技能值夹紧 |
| `ref_100` | 100 | 道具价 / 份额封顶基准 |
| `ability_div` | 50 | 道具价 `a / ability_div` |
| `item_price_scale` | 100 | 乘因子后的分母。`[确认]` `0x14015657f mov ecx,0x64; div` |
| `battle_scale` | 500 | 接舷 `base*(500-a)/battle_scale`。`[确认]` `idiv 0x1F4` 四处 |

`buy_ability_clamp` / `battle_ability_clamp` 名字沿用，量纲是**技能值**（不是六维）。

### 修正器注册表（统一出口）

不是写死字段的 `ModifierSet` 结构体。消费方按字符串 key 查：

```
Modifiers.eval(key, ctx) -> int          # ctx = { person?, fleet?, ship?, town?, base }
```

- 提供方注册「带来源的贡献」：`{ key, stage, order, source, value | num/den | formula_ref }`。来源：装备、舱室、身心状态、船首像、剧本旗标、主角修正器…
- 堆叠顺序固定：`base → add → mul → clamp → override`；同 stage 内按 `order` 升序，每步按公式的 `int_mode` 截断。每条贡献带 `source`，可追溯。
- 消费方只认 key，不认来源。代码里不得出现 `== 字面量 id`（船首像 0/1/5/7/8、岗位 3/7/9、技能 1/4/5/6、舱室种类 8/10/11）；这些写成内容表 `effects` / `role` / `refs`。
- 「岗位持有人」用 `Fleets.post_holder(fleet, post_key)`，「舱室计数」用 `Ships.cabin_count(ship, role)`，替代原版玩家/提督/船长三分支与舱室种类字面量。

本表用到的 key（原版包的贡献者在括号里）：

| key | 消费公式 | 原版贡献 |
| --- | --- | --- |
| `skill.<skill_key>` | F-PROG-09 输出；F-ECO-01/02/15、F-CBT-*、F-OCN-06 输入 | 装备（`stage: add`）、身体状态（`mul`）、`skill_value_cap`（`clamp`） |
| `trade.npc_buy_cost` | F-ECO-01 | 无（`base` 即公式） |
| `trade.item_shop_price` | F-ECO-02 | 无 |
| `trade.sell_price_pct` | F-ECO-16 | 剧本 2 主角 `mul 105/100`（原版比剧本号，见 F-ECO-16） |
| `factions.influence_gain` | F-ECO-05 旁路（`ctx.source: fashion_sale`）；遗迹首访（`source: remains`） | 剧本 2 × `fashion_sale` `mul 2/1`；剧本 0 × `remains` `mul 2/1` |
| `fleet.speed` | F-OCN-01 速度条 | 船首像大鹫 `mul 11/10`、魔鬼 `mul 4/5`（order 10/20） |
| `fleet.speed_move` | F-OCN-01 实际移动 | 海战态 `mul 1/3`（order 900，写速度条之后；提供方 Session 场景态 `scene.battle`） |
| `supply.food_div` | F-ADJ-03 | 船首像小猪 `add 2`（粮除数；`scope: fleet_any, stack: once`） |
| `supply.food_stock_pct` / `supply.water_div` | F-ADJ-03 | happening 结局态 1 → `override 95`；结局态 2–5 → `add 10`（`ocean/happenings.yaml` outcome effects，不是气候 id） |
| `fleet.search_radius_add` | F-ADJ-01 | 船首像海豚 `add 16`（`stack: once`） |
| `combat.melee_damage` | F-CBT-04 | 船首像王者 `mul 6/5`（`x + x/5`） |

`mul n/d` 对非负整数与原版 `x + x/10`、`x*4/5`、`x + x/5` 逐位相同。全部规范 key 见 `00-overview/10-key-glossary.md` §6.2 与 `14-config-schemas/examples/modifiers.yaml`，加载期校验「有消费方的 key 必须有定义」。F-CBT-01 `duel_crew_loss` 是公式名，不是修正器 key。

## 公式索引

`int` 列：原版整数语义，即 vanilla 包的 `int_mode` + `quirks`。`i32` = 32 位回绕、`/` 向零截断；`udiv` = 无符号除；`sar` = 有符号右移（向零修正后）。

| ID | 名称 | 系统 | 等级 | int / quirks | YAML / 正文 |
| --- | --- | --- | --- | --- | --- |
| F-PROG-01 | `level_from_exp` | progression | `[确认]` | u32 `udiv`；`quirk_min_q`、`q*0x10000` 回绕 | `formulas/progression.yaml` |
| F-PROG-02 | `exp_from_level` | progression | `[确认]` | u32；写入夹 `[0,300000]` 无符号 | `formulas/progression.yaml` |
| F-PROG-03 | `track_level` | progression | `[确认]` | 无符号夹 100 | `formulas/progression.yaml` |
| F-PROG-04 | `total_level` | progression | `[确认]` | — | `formulas/progression.yaml` |
| F-PROG-05 | `skill_value` | characters | `[确认]` | i32 `idiv 150`；等级项有符号比 200 | `formulas/character.yaml` |
| F-PROG-06 | `clamp_settled` | characters | `[确认]` | 有符号夹 `[0,500]` | `formulas/character.yaml` |
| F-PROG-07 | `person_present` | characters | `[确认]`（工具判据） | — | （谓词，无 YAML 魔数） |
| F-PROG-08 | `person_recruited` | characters | `[确认]`（工具判据） | — | （谓词） |
| F-PROG-09 | `ability` | characters | `[确认]` | i32 `idiv`；重伤 `sar 1` | `formulas/character.yaml` |
| F-PROG-10 | `stat_growth` | progression | `[确认]` | i32 有符号 `/2` | `formulas/progression.yaml` |
| F-ECO-01 | `npc_buy_cost` | economy | `[确认]` | i32 `imul` + `udiv`（`a>1000` 爆） | `formulas/economy.yaml` |
| F-ECO-02 | `item_bargain`（旧名 `item_price_factor`） | items | `[确认]` | `idiv 50` + `udiv 100` | `formulas/items.yaml`（`09-items/02`） |
| F-ECO-03 | `ledger_balance` | factions / economy | `[确认]` | i32 | `formulas/economy.yaml` |
| F-ECO-04 | `fashion_share_threshold` | factions | `[确认]` | 向上取整 | `formulas/factions.yaml`；`04-economy/03` §4.5、`02-factions/03` |
| F-ECO-05 | `fashion_share_gain` | factions | `[确认]` | i32 `idiv 6` | `formulas/factions.yaml`；`04-economy/03` §4.5、`02-factions/03` |
| F-ECO-06 | `fashion_letter` | factions | `[确认]` | 有符号 `cmp` | `formulas/factions.yaml`；`02-factions/04` |
| F-ECO-07 | `share_apply_cap` | economy / factions | `[确认]` | 有符号 `min` | `formulas/economy.yaml` |
| F-ECO-08 | `count_fleet_item` | economy | `[确认]` | — | `formulas/economy.yaml` |
| F-ECO-09 | `add_load` | economy | `[确认]` | — | `formulas/economy.yaml` |
| F-ECO-10 | `same_item_market_adjust` | economy | `[确认]` | u32 比较与乘除 | `formulas/economy.yaml`；`04-economy/02` §4.6 |
| F-ECO-11 | `investment` | economy | `[确认]`，正文 `04-economy/07` §4.4 | `udiv`；`max_s(1,·)` 透支 | `formulas/investment.yaml` |
| F-ECO-12 | `tribute` | economy / town-life | `[确认]`，正文同篇 §4.5 / §4.6 | `idiv 10`；`×3/2` 无符号 | `formulas/tribute.yaml` |
| F-ECO-13 | `repair_cost` | ships / town-life | 底价 `[未知]`；船木工折扣见 F-ADJ-04 | — | `formulas/economy.yaml` |
| F-ECO-14 | `local_fleet_founding_gold` | fleets | `[确认]` 常数文案 | — | `formulas/economy.yaml` |
| F-ECO-15 | `bargain` | economy | `[确认]`，正文 `04-economy/03` §2.7 / §4.3 | `idiv`；改价 `udiv 100` | `formulas/economy.yaml` |
| F-ECO-16 | `quote_gold` / `sell_quote` | economy | `[确认]`，正文 `04-economy/02` §2.5 / §4.4–§4.5 | 两次 `udiv`（100、10000）；结果夹 `[1,999999]` | `formulas/economy.yaml` |
| F-ECO-17 | `kind_rate_drift` / `kind_rate_slide` | economy | `[确认]`，正文 `04-economy/02` §4.2–§4.3 | `idiv 10`；滑动夹 `[1,13]` | `formulas/economy.yaml` |
| F-ECO-18 | `special_supply` | economy | `[确认]`，正文 `04-economy/04` §4.4 | i32 `idiv` | `formulas/economy.yaml` |
| F-ECO-19 | `treasure_rent` | economy | `[确认]`，正文 `04-economy/07` §4.7 | `/50` | `formulas/tribute.yaml` |
| F-CBT-01 | `duel_crew_loss` | combat | `[确认]` | i32 `idiv 0x1F4` | `formulas/combat.yaml` |
| F-CBT-02 | `can_fire` | combat | `[确认]`，正文 `07-combat/01` §2.8 / §4.3 | `d2 < 900` 无符号 `jb` | `formulas/combat.yaml` |
| F-CBT-03 | `fleet_defeated` | combat | `[确认]`（仅无画面结算） | 有符号 `<= 30` | `formulas/combat.yaml` |
| F-CBT-04 | `boarding_fencing` | combat | `[确认]` 式；`fn_14000ee12`、`fn_14001399e` 语义 `[推断]` | i32 `idiv 2000`；`/2` = `sar` | `formulas/combat.yaml` |
| F-CBT-05 | `melee_strike` | combat | `[确认]`，正文 `07-combat/02` §2.4 / §4.1 | `rand(70)` 无符号比较 | `formulas/combat.yaml` |
| F-CBT-06 | `reload_period` | combat | `[确认]`，正文 `07-combat/01` §2.10 / §4.2 | `idiv 10` | `formulas/combat.yaml` |
| F-CBT-07 | `offscreen_battle` | combat | `[确认]` 式，`slot.c` 四项 `[未知]`，正文 `07-combat/02` §2.7 / §4.4 | i32 | `formulas/combat.yaml` |
| F-CBT-08 | `surrender_roll` | combat | `[确认]`，正文 `07-combat/04` §2.5 | `/20`；`roll <= rand(100)` 才降 | `formulas/combat.yaml` |
| F-OCN-01 | `sailing_speed` | ocean | `[确认]`（四处 `idiv` 全钉：`/10`、`*4/5`、`/3`、`/(2*need)`） | i32；`>>n` = 向零 `sar`；中间值 wrap32 | `formulas/ocean.yaml` |
| F-OCN-02 | `wind_level_from_rng` | ocean | CDF `[确认]`；`WIND_*_ADJUST` 整数 `[未知]` | — | `formulas/ocean.yaml` |
| F-OCN-03 | `rowing_bonus` | ocean / ships | `[确认]`，正文 `06-ocean/01` §2 | i32 `idiv` | `formulas/ocean.yaml` |
| F-OCN-04 | `sail_values` / `crew_needed` | ships（派生，Ocean 只读快照） | `[确认]` 式，位名 `[推断]` | `max(0,·)` | `ships/rig.yaml`；`05-ships-fleet/06` §4.1 |
| F-OCN-05 | `supply_drain` | ocean | 见 F-ADJ-03 | — | `formulas/supply.yaml` |
| F-OCN-06 | `explore_supply_roll` | ocean | `[确认]`，正文 `06-ocean/02` §2 探索流程 | `idiv 0x32` | `ocean/search.yaml` |
| F-SHP-01 | `shipyard_listing_threshold` | ships | `[确认]`，正文 `05-ships-fleet/05` 造船厂 | 两次有符号除 | `formulas/ships.yaml` |
| F-TWN-01 | `inn_recover` | town-life | `[确认]` 式，`[推断]` 字段，正文 `08-town-life/05` | `idiv 0x14` | `formulas/town.yaml` |
| F-TWN-02 | `rite_day` | town-life | `[确认]` 式，`town_id` 来源 `[推断]`，正文 `08-town-life/05` | `idiv 0x1C` | `formulas/town.yaml` |
| F-TWN-03 | `scheme_contest` | town-life | `[确认]`，正文 `08-town-life/04` §2.7 / §4 | `sar 1` | `formulas/town.yaml` |

F-ECO-15 起与 F-SHP / F-TWN 只登记名字、等级与整数语义，式子与参数以正文为准，本表不重抄（避免第三套符号）。

---

## F-PROG-01 `level_from_exp`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `exp: uint32` 单段经验 |
| 参数 | `exp_divisor: int = 30` |
| 输出 | `level: int ≥ 0` |
| 所属 | progression |
| 源 | exe：`fn_1400146c3`（body `0x1401c97a0`，读 `CPerson+0x18`）/ `fn_140012085`（`0x1401c92b0`，读 `+0x1C`）；开方 `fn_14000819d`（`0x140140d30`）。旁证 `save_format.md`、`Save.cpp`、`dk4save.c` |

原版指令（两段同构）：

```
q = exp / 30                       # 无符号 div
r = isqrt16(q)                     # q<=1 时原样返回 q；否则 n=q*0x10000（32 位回绕），牛顿迭代
level = r / 0x100                  # 无符号 div
level = clamp_u(level, 0, 100)     # fn_14001377d，100 即 level_per_track
```

`isqrt16` 返回 `floor(256*sqrt(q))`，所以 `level = floor(sqrt(q))`。**例外**：`q==1`（`exp ∈ [30,60)`）时函数直接返回 1，`1/256=0`，等级为 0 而不是 1。`[确认]`

重构式：

```
q = floor(exp / exp_divisor)
level = (q < quirk_min_q) ? 0 : isqrt(q)       # quirk_min_q=2 复刻原版（q==1 得 0）；设 1 则为数学 isqrt
level = min(level, level_per_track)
```

参数名与 `11-progression/01-level-exp-skills.md` §6 一致：`exp_divisor`、`fx_shift`（16）、`quirk_min_q`（旧名 `q1_quirk` 布尔开关作废）。

溢出：`q ≥ 65536`（等级 ≥ 256）时 `n = q*0x10000` 回绕成 `(q mod 65536)<<16`，等级变成 `isqrt(q mod 65536)`；`q` 是 65536 整倍数时 `n=0`，迭代首步 `div [x]`（`x=n=0`，`0x140140d82`）除零崩溃——不是「得 0」。vanilla 被 300000 夹住碰不到。重构饱和到 `caps.level_per_track` 且不超过 `engine_limits.level_hard`（255）。禁止绕回。

`Save.cpp` 的演示循环用 `r < 400` 防跑飞，不是玩法上限。

修正器：无。等级是派生缓存，经验是源。

## F-PROG-02 `exp_from_level`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `level: int` |
| 参数 | `exp_divisor: int = 30` |
| 输出 | `exp: uint32` |
| 所属 | progression |
| 源 | 同上；`dhh4tools/dk4save.c` `exp_of_level` |

```
exp = exp_divisor * level * level
```

原版写入经验时四处立即数 `0x493E0=300000`（`0x1401c8076` / `0x1401c80df` 在 `fn_140011c34`，`0x1401c82fb` 在 `fn_140006c67`，`0x1401c84cb` 在 `fn_140008387`），全部是 `exp = clamp_u(exp + add, 0, 300000)`。`[确认]` 重构禁止写死 `300000`，按本式从 `level_per_track` 重算。

加经验入口 `fn_140011c34(person, add0, add1, show)`：两段各加、各夹，段等级上升则触发 F-PROG-10。

溢出：`level > 255` 先夹到硬顶再乘；用 64 位中间值，再饱和到经验字段宽度。

## F-PROG-03 `track_level`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `exp` 单段 |
| 参数 | `exp_divisor`、`level_per_track`（原版设计 100，`engine_limits.level_hard` 255） |
| 输出 | 该段显示/规则等级 |
| 源 | `exe_caps.md`「单段等级」两处：`0x1401c97e5` / `0x1401c92f5` `mov r8d,0x64` → `fn_14001377d` 无符号夹紧 |

```
level = min(level_from_exp(exp), level_per_track)
```

存档有两段经验：人物记录 `+0x08` / `+0x0C`（= dk4save 行偏移 `+0x0B` / `+0x0F`，起点差 3 字节），仅 id < 87；主角体 `+0x35B2` / `+0x35B6`；运行时 `CPerson+0x18` / `+0x1C`。

## F-PROG-04 `total_level`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `exp0`, `exp1` |
| 参数 | 同 F-PROG-03；`track_count = 2` |
| 输出 | 总等级 |
| 源 | exe `fn_1400123dc`（body `0x1401c9230`）= `fn_1400146c3 + fn_140012085`；`save_format.md` `+0x70` 缓存主角总等级 |

```
total_level = track_level(exp0) + track_level(exp1)
```

`+0x70` 是缓存。重构：经验是源，等级可缓存但必须能从源重建。技能公式用的「等级」是这个总等级，不是单段。

溢出：两段各 255 时合计 510。原版技能公式再把等级项夹到 `skill_level_cap=200`。

## F-PROG-05 `skill_value`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `person`，`skill k`，`use_bias_stats`（布尔） |
| 参数 | `skill_level_cap=200`，`level_bias=100`，`skill_divisor=150`，`skill_value_cap=500`（即 F-PROG-06） |
| 输出 | 技能值 `[0, 500]` |
| 所属 | characters（公式与配对；`formulas/character.yaml`）；等级输入来自 progression |
| 源 | exe `fn_140009214`（body `0x1401c9d50`）；配对表 `fn_1400099e9` / `.data 0x1406864a0`；旁证 `Save.cpp`、`dk4save.c` |

原版指令：

```
L = total_level(person)                    # F-PROG-04
if L > 200: L = 200                        # 有符号 cmp；即 skill_level_cap
a, b = SKILL_PAIR[k]                       # 0x1406864a0
if use_bias_stats:                         # Ability 路径传 1；0x14008c70e 处传 0（用途 `[未知]`）
    s = biased_stat(person, a) + biased_stat(person, b)   # fn_14000e098
else:
    s = stat(person, a) + stat(person, b)                 # fn_1400090ca = CPerson+0x0F+i
skill = s * (L + 100) / 150                # 有符号 idiv
return clamp(skill, 0, 500)                # fn_1401cbb40
```

`SKILL_PAIR`（`[确认]`，`.data` 初值；六维 0–5 = 体力 敏捷 魅力 智力 精神 运气）：

| k | 技能 | a | b |
| ---: | --- | --- | --- |
| 0 | 生命力 | 体力 | 精神 |
| 1 | 统率力 | 魅力 | 精神 |
| 2 | 驾驶术 | 敏捷 | 精神 |
| 3 | 测量术 | 敏捷 | 智力 |
| 4 | 会计术 | 智力 | 精神 |
| 5 | 剑术 | 体力 | 敏捷 |
| 6 | 命中力 | 敏捷 | 运气 |
| 7 | 说服力 | 魅力 | **智力** |
| 8 | 谋略术 | 智力 | 运气 |
| 9 | 观察力 | 精神 | 运气 |

`biased_stat`（`fn_14000e098`）：按人物 `+0x0E` 精神状态 `m` 对单维乘除，`m ∈ {0,1,2,4,5}` 有表，其余原值（魅力、运气从不改）：

| m | 精神状态 | 体力 | 敏捷 | 智力 | 精神 |
| ---: | --- | --- | --- | --- | --- |
| 0 | 高亢 | ×11/10 | ×105/100 | ×95/100 | ×105/100 |
| 1 | 精神 | ×105/100 | — | — | ×105/100 |
| 2 | 轻松 | — | — | ×105/100 | — |
| 3 | 普通（世界表初值） | — | — | — | — |
| 4 | 不高兴 | — | — | — | ×95/100 |
| 5 | 混乱 | ×95/100 | — | ×9/10 | — |

状态名与 `+0x0E` 的归属见 `03-characters/01-person-model.md`。表进 `formulas/character.yaml` 的 `tables.mental_adjust`，键为精神状态 key。

技能**不存档**。重构：`min(level, skill_level_cap)` 饱和；乘法用 64 位；`skill_value_cap` 独立成参。

本式是**基础技能值**。买货/战斗/道具价读 F-PROG-09（修正器 key `skill.<skill_key>`），不是本式。`Base(p,0,0)`（生命力、不计精神）另作 HP 上限（`fn_14000cf22`，见 `03-characters/07-fatigue-health-mood.md`）。

## F-PROG-06 `clamp_settled`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `value`（技能值或 `Ability`） |
| 参数 | `skill_value_cap=500`（旧名 `ability_settled_cap` 作废） |
| 输出 | `[0, skill_value_cap]`（有符号 clamp） |
| 源 | `exe_caps.md` 属性结算；`fn_1401cbb40` 以立即数 `0x1F4=500` 调用 `fn_1400103d4(lo=0, v, hi)` |

`fn_1401cbb40` 全 exe 只有两个调用点：`fn_140009214` 末尾（技能值）与 `fn_1400135cf` 末尾（Ability）。`[确认]` 所以这个 500 夹的是**技能 / Ability 输出**，不是六维；六维只受 F-PROG-10 的 `ability_cap`=100 成长帽。键名统一为 `skill_value_cap`（与 `11-progression`、`03-characters` 一致）。

重构：贸易/战斗 **不要** 依赖「结算上限恰好 ≤ 1000」。另设 `buy_ability_clamp` / `battle_ability_clamp`。原版两值都是 500。

## F-PROG-07 `person_present`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `stats[6]` |
| 输出 | bool |
| 源 | `Save.cpp`；`save_format.md` |

```
present = (stats[0]+…+stats[5]) > 0
```

在队：六维之和 > 0。无额外参数。

## F-PROG-08 `person_recruited`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `sailor.force_id`, `protagonist.force_id`, `present` |
| 输出 | bool |
| 源 | 同上 |

```
recruited = present AND sailor.force_id == protagonist.force_id
```

F-PROG-07/08 是存档工具（`Save.cpp`）的判据，不是 exe 运行时谓词。存档记录 i = `ePerson` i，仅 id < 87 有完整状态（87 条，不是 98），见 `00-overview/04-id-registry.md`。

## F-PROG-09 `ability`

业务系统读的「能力」= 技能值 + 装备 − 身心折扣。买货、道具价、接舷都调它。

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `person`，`skill k` |
| 参数 | `condition_mul[s]`（见下表），`skill_value_cap=500` |
| 源 | `fn_14000e1ce`（body `0x1401c9f60`）→ `fn_1400135cf`（`0x1401ca250`）；装备 `fn_14000e0b6`（`0x1401c8ff0`，5 槽）；身心 `fn_1401c87a0` |

```
post = (person.force == player_force) ? post_of(person) : 0x18   # 0x18 = 无岗位
v  = skill_value(person, k, use_bias_stats=1)                    # F-PROG-05，已夹 500
v += sum(equip_bonus(person, k, post, slot) for slot in 0..4)    # 空槽 item==0xE8 计 0
v  = v * condition_mul[person.+0x0D]                             # 有符号整除
return clamp(v, 0, 500)                                          # F-PROG-06
```

| `+0x0D` 身体状态 | 乘子 | 指令 |
| ---: | --- | --- |
| 0 健康、6 死亡（及其他） | 1 | — |
| 1 疲劳、2 轻伤 | 19/20 | `imul 0x13; idiv 0x14` |
| 3 重伤 | 1/2 | `sar 1`（向 0） |
| 4 濒死 | 1/10 | `idiv 0xA` |
| 5 疾病 | 3/5 | `imul 3; idiv 5` |

状态名见 `03-characters/01-person-model.md`。`post` 实为人物所在**舱室种类**（`fn_14000b9fb`，`0x18`=无）。装备求和细节（剑术对道具 kind 2/3 恒加；其余要求道具绑定舱室的技能 == k 且持有者在该舱或舰长室）见 `03-characters/03-abilities-skills-jobs.md`，效果量归 `09-items`。

重构：本式就是修正器 key `skill.<skill_key>` 的原版堆叠：`base` = F-PROG-05；装备 `Items.equip_skill_bonus` 以 `stage: add` 贡献；`condition_mul` 以 `stage: mul` 贡献（落点 `characters/vitals.yaml` 的 `physical_states[*].skill_mult` 状态行，`03-characters/07` §6；不进 `formulas/character.yaml`）；`skill_value_cap` 为 `stage: clamp`。原版「玩家势力才查舱室」改成求值上下文 `Modifiers.eval("skill.<key>", { person, cabin? })`：`cabin` 是舱室种类 key（`null` = 不在舱），查舱范围由 `formulas/character.yaml` 的 `cabin_lookup_scope` 决定，不比 `0x18`。

## F-PROG-10 `stat_growth`

段等级上升时六维成长。

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 参数 | `track_stats`（段→三维），`growth_rand_a=2`，`growth_rand_bias=3`，`roll_mode_default`，`ability_cap=100`；随机流 `progression.growth` |
| 源 | `fn_1400065fa`（body `0x1401ca9c0`）；表 `.data 0x1406863f0` = `{0,1,2}`、`0x140686400` = `{3,4,5}`；写回 `fn_1400062da`（`0x1401c7f20`，`mov r8d,0x64`） |

```
track_stats = { track_a: [stamina, agility, charm], track_b: [intelligence, spirit, luck] }   # 原版 {0,1,2} / {3,4,5}
for each roll:                              # 次数见下
  for stat in track_stats[t]:
    g = zodiac(person).growth[stat]         # 原版表 0x140688970，12 星座 × 6；fn_140001d61 → fn_140001b8b
    d = rand(growth_rand_a) + rand(g + growth_rand_bias) / 2   # 2 / 3；有符号 /2；流 progression.growth
    if d > 0: stat = clamp(stat + d, 0, $caps.ability_cap)     # 100
```

滚动次数 `[确认]`（`11-progression/01` §2）：`fn_140011c34` 一次加两段经验时，某段只要升级就把该段三维加入列表，**只掷一轮**；`fn_140008387`（只加段 0）/ `fn_140006c67`（只加段 1）按**总等级**升了几级就掷几轮。重构用 `roll_mode_default: once_per_call | per_level` 一个旋钮（名随 `11-progression/01` §6）。

成长倾向按**星座**取，不按人（星座见 `03-characters/01-person-model.md`）。重构：表进 `characters/zodiac.yaml` 的 `growth`（按六维 key 的映射，不是 `[6]` 数组）；人物只引用 `zodiac`。

---

## F-ECO-01 `npc_buy_cost`

地方舰队 / NPC 舰队自动买货。

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `price`, `qty`；舰队领队人（提督）`person_id` |
| 参数 | `ref_1000=1000`，`rand_span=100`，`buy_ability_clamp=500`；`refs.skill: accounting`（原版 eSkill 4 会计术） |
| 输出 | `cost: int`（写入账本支出） |
| 所属 | economy（读修正器 `skill.accounting`，不读六维） |
| 源 | `srcdata/docs/exe_caps.md` `fn_140010d25` / body `0x1400f6d20`；`dhh4tools/dk4save.c` `PROXY_BUY_BEGIN` |

原版：

```
cost = price * qty                           # 0x1400f73eb
captain = fn_1400078a1(fleet)                # body 0x1400f0ad0：fn_14001460f 取领队 id，0xEC → null
if captain != null:                          # 无领队则不打折、不给经验
    a    = Ability(captain, 4)               # eSkill 4 会计术；F-PROG-09，已夹 500
    cost = cost * (ref_1000 - a + rand(rand_span)) / ref_1000
    Progression.grant_exp(captain, add{track_b: 10})   # fn_140006e88(captain,10,1)：段 1（+0x1C）+10
record.expense += cost                       # 无账本则扣势力金币
```

`rand(n)`（`fn_140005272`）= `n>0 ? rand()%n : 0`，即 `[0, n)`。32 位 `imul` + **无符号** `div`。`[确认]` 指令 `0x1400f742c`–`0x1400f7480`。

溢出（原版 bug）：`a > ref_1000` 时括号为负，无符号除法变成几百万。`skill_value_cap > 1000`（补丁）即触发。月结 `CFleetElement::vfunc_04`（约 `0x140100180`）付不起的转入下月支出。

补丁：买货路径把 `a` 再夹到 500，不改结算上限。

重构：

```
a = Modifiers.eval("skill.accounting", {person: leader})   # F-PROG-09，已夹 skill_value_cap
a = clamp(a, 0, buy_ability_clamp)
cost = saturating(price * qty * max(0, ref_1000 - a + rand(rand_span)) / ref_1000)   # 64 位
cost = Modifiers.eval("trade.npc_buy_cost", {fleet, base: cost})
emit NpcTradeCommitted { fleet_id, admiral_id: leader, … }   # progression 按 grant_routes 调 Progression.grant_exp(leader, add{track_b: 10})；经济不改经验（04-economy/06）
```

禁止贸易代码读 `eAbility` 或写死技能下标。

## F-ECO-02 `item_bargain`（旧名 `item_price_factor`）

道具店**买入**时会计长还价（不是卖出）。`[确认]` 消息 `0x4f1` 买不起、`0x4f2` 要买吗、`0x4f3` 谢谢、`0x4fd` 还价成交（`09-items/02-equipment-effects.md` §2）。

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | 基础价 `price`（`CItem+0x2C`）；舰队会计长岗位上的人 |
| 参数 | `ref_100=100`，`ability_div=50`，`item_price_scale=100`；`refs.skill: accounting`、`refs.post: purser`（原版 eSkill 4、岗位 3） |
| 输出 | `item_price` |
| 所属 | items（读修正器 `skill.accounting`） |
| 源 | `fn_14000f4ed`（body `0x140156360`）指令 `0x14015654e`–`0x140156586` |

原版：

```
p = first_person_in_post(fleet, 3)          # fn_140010cee；无人返回 0xEC
if p == 0xEC: 不打折（消息 0x4f3）
else:
    a = Ability(p, 4)                       # eSkill 4 会计术
    item_price = price * (ref_100 - a / ability_div) / item_price_scale
                 # a/50 有符号 idiv；乘后 div 0x64 无符号
    Progression.grant_exp(p, add{track_b: 10})   # 段 1（+0x1C）+10
```

注意：付款前的「钱够不够」比较用的是**折前价**（`0x1401564da`）。

技能值封 500 → 原版最多打 9 折（`500/50 = 10`）。

溢出：`a/ability_div > ref_100` 时因子为负。重构：`a = clamp(a, 0, ability_div * ref_100)`，`factor_term ≥ 0`。结果再过 `Modifiers.eval("trade.item_shop_price", …)`；会计长由 `Fleets.post_holder(fleet, purser)` 取，不比 `0xEC`。

## F-ECO-03 `ledger_balance`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | 账本一行：收入、支出、投资 |
| 输出 | 余额 |
| 源 | `exe_caps.md` 账本 `mgr + 0x3738 + idx*0x18` |

```
# +0x00..0x0B 上月
# +0x0C 支出  +0x10 收入  +0x14 投资
balance = income - expense - investment
```

参数：无魔数。字段名进存档 schema，不进 `formulas/`。

溢出：原版 32 位有符号。重构饱和到金币字段宽度，禁止绕回。

## F-ECO-04 `fashion_share_threshold`

卖流行品加份额的件数门槛。

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | 五船**货物仓库舱室**个数之和 `hold_slots`：`fn_140006b95(ship, 0x0A)` 扫 31 个舱室槽（`eShipCabin < 0x1F`），数 `cabin_kind == 10`（货物仓库）。`0x0A` 是舱室种类，不是「扫描 10 格」 |
| 参数 | `fashion_threshold_numer=1`、`fashion_threshold_denom=2`（`formulas/factions.yaml`，词表 §14-9；工具 `fthr` = numer） |
| 输出 | 门槛 `threshold` |
| 源 | `srcdata/docs/trade_stack.md` DK4FSH1；`dk4save.c` CapValues；`0x1402776ea` |

原版：

```
threshold = ceil(hold_slots / 2)      # 0x1402776ea：奇数 s/2+1，偶数 s/2
```

补丁：

```
threshold = (hold_slots * fashion_threshold_factor + 1) >> 1
```

`fthr=1` 时与原版等价。重构写成 `ceil(hold_slots * numer / denom)`（与 `04-economy/03` §4.5 同式），default 1/2。舱室计数用 `Ships.cabin_count(ship, cargo_room)`，不比种类 10。

## F-ECO-05 `fashion_share_gain`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `x0`/`x1` = 本次卖出的两种流行品**件数**（STK3 已按件数计）；`town_id`；`force_id`；当前份额 |
| 参数 | `share_letter_cut=80`（`0x50`），`fashion_share_div=6`（工具 `fdiv`），`fashion_gain_add=1`；全部在 `formulas/factions.yaml`（消费方 factions 订阅 `SoldFashionGoods`） |
| 输出 | 拟增加的份额点数 |
| 源 | `CommitDeal` `0x140277b6b`–`0x140277e0f`（原始指令） |

份额 `< share_letter_cut`（`fn_1400088f0(town) < 0x50`）时：

```
if 两个流行槽都非空:                                  # 0x6A = 空
    pass = x0 >= threshold and x1 >= threshold       # 两种都要达标（0x140277b9a–0x140277bbc 两个 jl）
    x    = x0 + x1
elif 只有一个非空:
    pass = (该种件数 >= threshold)
    x    = 该种件数
if pass:
    gain = x / fashion_share_div + fashion_gain_add  # cdq; idiv 6; inc（0x140277dda），有符号
    fn_1400122f6(town, force, gain, 1)               # F-ECO-07 封顶
elif x0 > 0 or x1 > 0: 只出「件数不够」提示
```

旧稿「有一种达标就把达标那几种相加」错：两槽都有流行品时必须**两种都达标**。

份额 `≥ share_letter_cut` 走介绍信/推荐信（F-ECO-06），不加这条点数。影响力另加 `x0 + x1`，原版剧本 2 翻倍——重构走修正器 `factions.influence_gain`（`ctx.source: fashion_sale`，剧本记录 `modifiers` 贡献），公式里不比剧本号，也不再乘一次。

## F-ECO-06 `fashion_letter`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `x = x0 + x1` 流行品件数（不看 `threshold`）；当前信件状态 `letter_state` |
| 参数 | `letter_intro_min=10`、`letter_intro_max=15`、`letter_recommend_min=16`、`letter_upgrade_min=16`（`formulas/factions.yaml`，与 `02-factions/03`、`04` 同键）。原版四处立即数 `0x0A`/`0x0F`/`0x10`/`0x10`（`0x140277e63`、`0x140277e71`、`0x14027805f`、`0x14027829d`） |
| 输出 | 信件升级事件 |
| 源 | `CommitDeal` `0x140277e3a` 起 |

`bd98(s550)`：`2` = 无信，`0` = 有介绍信。`[推断]` 枚举值，字段名未恢复。

```
if share >= share_letter_cut:
  if letter_state == none  and letter_intro_min <= x <= letter_intro_max: grant 介绍信
  if letter_state == none  and x >= letter_recommend_min:                 grant 推荐信
  if letter_state == intro and x >= letter_upgrade_min:                   upgrade 推荐信
```

重构把 `letter_state` 做成正式枚举，不暴露 `0/2`。

## F-ECO-07 `share_apply_cap`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `current_share`（本势力）, `gain` |
| 参数 | `share_cap = 100`，`share_slots = 3` |
| 输出 | 新份额 |
| 源 | `fn_1400122f6`（body `0x14025e500` 起，RTC `asShareData`）；`0x1402635f6` `mov eax,0x64; sub eax,[本势力份额]` → `min_s` |

每港 3 个份额槽（`town+0x18+i*2`：势力字节 + 份额字节；空势力 `0x1A`）。

```
applied = min(gain, share_cap - current_share)   # 有符号 min
if applied <= 0: return 0
if share_cap - sum(all_slots) >= applied: 直接加到本势力槽
else: 从他势力槽挤出（细节归 02-factions/03-relations-influence.md）
```

`SHARE_MIN/MAX` 符号在 `constants.json`，整数值 `[未知]`。实现时 `share_cap` 必须 ≤ `SHARE_MAX`（一旦还原）。

## F-ECO-08 `count_fleet_item`

委托/剧情查舰队某商品件数。

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `fleet_id`, `trading_item_id` |
| 参数 | `fleet_ships=5`，`hold_slots_per_ship=5`，`cargo_stack`（原版 1） |
| 输出 | `count` |
| 源 | `trade_stack.md` thunk `5c86` body `0x1400f5060` |

玩家势力（真实舱位）：

```
# 原版：5 船 × 5 格，商品相同则 count++（数格）
# 堆叠补丁：count += 该格件数
```

其他势力：`fleet+0x74` 一条记录 + 数量，本来就是数量，不改。

重构只保留「按件数」语义；`cargo_stack=1` 时与原版数格等价。循环上界读表长，不写死 5。

## F-ECO-09 `add_load`

剧情/委托增减货物。`n>0` 加，`n<0` 扣。

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 参数 | `max_new_slots=25`（原版 `mov ecx,0x19`），`fleet_ships`，`hold_slots_per_ship`，`cargo_stack` |
| 源 | `trade_stack.md` thunk `13f8e` body `0x1400edc40` |

原版玩家分支：

```
if n > 0: 新建 min(max_new_slots, n) 格，每格 1 件
if n < 0: 取 min(max_new_slots, -n)，每碰到一格同商品整格清掉、欠数--
```

补丁：扣货按件数；一格件数 ≤ 欠数则整格清，否则改件数。`max_new_slots` 改为 `25 * cargo_stack`。`n>0` 仍一格一件。

重构：`max_new_slots` 由 `fleet_ships * hold_slots_per_ship` 推导，或显式配置；禁止写死 25。

## F-ECO-10 `same_item_market_adjust`

| 项 | 内容 |
| --- | --- |
| 事实 | 式与两字段 `[确认]`（`04-economy/02-market-price.md` §2.6 / §4.6） |
| 参数 | `culture_recover.bulk_qty=15`，`num=2`，`den=10`（键名随 `04-economy/02` §6） |
| 源 | `CommitDeal` `0x1402777ae`–`0x140277896`；循环 `item < 0x69`（`NUM_TRADINGITEM`） |

```
for item in trading_items:                       # 原版 0..104
    if sold[item] < same_item_adjust_threshold: continue      # cmp 0xF; jl
    ref = catalog[item].data_price               # 交易品目录 +0x10，fn_140007022
    cur = culture_price[culture(town)][item]     # 文化圈卖价 CCulture+0x14+item*2，fn_140004c7d
    if ref > 0 and ref > cur:                    # 无符号比较
        cur = min(ref, cur + (ref - cur) * 2 / 10)   # 无符号
        culture_price[culture(town)][item] = cur # fn_140006ae1
```

整片文化圈的该货卖价向目录价收拢 20%，只升不降。另一写入点 `fn_14000c658`：`cp += (base - cp) * qty / 50` 再 `min(·, base)`（玩法 `[未知]`，见 `04-economy/02` §2.6）。原版 `sold[item]` 是格数，堆叠补丁后为件数。

## F-ECO-11 `investment`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 所属 | economy |
| 正文 | `04-economy/07-investment-tribute.md` §4.1–§4.4（`fn_140009ae8` body `0x14025f3c0`） |
| 参数 | `invest_min_floor=1`、`invest_max_factor=20`、`pending_gold_cap=9999999`、`develop_bracket=1000`、`normal_cost_unit=20`、`exclusive_cost_unit=10`、`influence_div=2000`；上限引用 `$caps.town_develop`（9999） |
| YAML | `formulas/investment.yaml`（所属 04-economy） |

摘要：每 1000 发展度一档，档内一点价 `(档+1)*20`（独占 `*10`）；余金强买 1 点后清零；结果 `min(dev, 9999)` 写 `town+0x1e+track*2`（`0x14025f577 mov ecx,0x270F`）。本表不重抄，只登记名字与参数。

## F-ECO-12 `tribute`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 所属 | economy（金额式）；附庸税的执行与 `SuzerainTributePaid` 归 factions（`forces` 阶段 `factions.suzerain_tribute`，词表 §14-20）；宫殿文案属 town-life |
| 正文 | `04-economy/07-investment-tribute.md` §4.5 独占上缴、§4.6 附庸上缴 |
| 参数 | `exclusive_div=10`、`kind_bonus ×3/2`、`kind_penalty ×1/2`、`suzerain_div=100` |
| 剧本开关 | 附庸税原版只在 `eScenario == 6` 收（`DAT_144296500+0x7c`）。重构写成剧本包 `scenario.flags.pay_suzerain_tribute: true`（公式 `enabled_by: scenario_flag.pay_suzerain_tribute`，与 `04-economy/07`、`10-narrative/01` 同名），**不是**公式参数；公式文件不比剧本号 |
| 时点 | 独占上缴：月末最后一拍；附庸税：月初第一拍、开局第 0 拍不收（`schedule: month_end` / `month_start`） |
| YAML | `formulas/tribute.yaml`（所属 04-economy） |

## F-ECO-13 `repair_cost`（底价占位）

| 项 | 内容 |
| --- | --- |
| 事实 | 底价 `[未知]`；船木工折扣 `[确认]`（F-ADJ-04） |
| 所属 | ships / town-life（造船厂） |
| 已知 | 「修理要花%s枚金币」；折扣 `pct = total_level/5`，`total_level ≥ 25` 才生效（`fn_140175b00`） |
| 参数占位 | `repair_gold_per_durability` — 无 default，禁止填假数 |

## F-ECO-14 `local_fleet_founding_gold`

| 项 | 内容 |
| --- | --- |
| 事实 | 文案常数 `[确认]`；是否唯一计费点 `[未知]` |
| 参数 | `local_fleet_founding_gold=100000` |
| 源 | `mesfile_sc.txt` `0x2f33`「建立地方舰队需要１０００００枚金币」 |

进 `formulas/economy.yaml`，禁止 UI 字符串里藏逻辑。NPC 买货函数里另一个 `0x186A0` 是有账本时的买货预算（`100000 / 单价`），与建队金分成两键（`04-economy/06` §2.8）。

## F-ECO-15 … F-ECO-19（只登记）

| ID | 原版式（摘要） | 关键地址 | 重构注意 |
| --- | --- | --- | --- |
| F-ECO-15 `bargain` | `s = skill(会计长, 会计术)`；`chance = min(100, (s-60)/2)`；`rand(100) < chance` 成功；`bonus = min(15, (share+6)/7)`；收入 `×(100+bonus)/100`、支出 `×(100-bonus)/100`（无符号）；每会话一次 | `fn_1400137b4` body `0x140276c50` | `s` 读 `skill.accounting`；会计长用 `post_holder` |
| F-ECO-16 `quote_gold` / `sell_quote` | `clamp(base * grade_pct * rate/100 * fashion_pct/10000, 1, 999999)`；产地卖 = 买价 /2；原版剧本 2 卖价 `×105/100` | `fn_140010492` `0x140262be5`/`0x140262bf3`；`fn_140001b04` body `0x1402758a0` | 剧本 2 加成走 `trade.sell_price_pct` 主角修正器，公式不读 `fn_140010f46()` |
| F-ECO-17 `kind_rate_drift` / `kind_rate_slide` | 月初 `rate += (d±bias)/10 ± rand(2)`；成交 `delta = clamp(n*avg/ref, 1, 13)` | `0x14025dd4c`；body `0x140263ea0`/`0x140264010` | `schedule: month_start`，顺序进 `tick.yaml` |
| F-ECO-18 `special_supply` | `base * (commerce/50 + 100) / 100`（`base` = 特产品槽 `+4` u8） | `fn_140012da0` body `0x140261b20` | 键名 `supply_base`；上架门槛槽 `+2` 是 `listing_threshold` |
| F-ECO-19 `treasure_rent` | `item_price / 50`，月末最后一拍收 | `fn_140003afd`；`CItemElement::vfunc_04` | 循环上界读表长，不写死 231；tick 在 `items` 阶段，先于 `towns` 阶段独占上缴（词表 §14-17） |

---

## F-CBT-01 `duel_crew_loss`

接舷里**单挑败北**一方的水手损失；统率力越高损失越小。不是进攻加成（旧名 `battle_settlement_factor` / `battle_offense_factor` 作废）。正文 [07-combat/02-boarding.md](../07-combat/02-boarding.md) §2.5。

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `crew`：败方船 `vfunc+0x58`（水手数，`07-combat/01` §2.9 已确认）；败方提督 `person_id` |
| 参数 | `ref_500=500`，`battle_ability_clamp=500`，`battle_scale=500`；`refs.skill: leadership`（原版 eSkill 1 统率力） |
| 所属 | combat |
| 源 | `fn_14000660e`（body `0x1401bb5b0`）四处：`0x1401bbdd6` / `0x1401bbeb6` / `0x1401bc113` / `0x1401bc17f`（`mov edx,1; call fn_14000e1ce` → `imul` → `idiv 0x1F4`） |

```
a    = Skill(loser_admiral, leadership)             # F-PROG-09，已夹 500
term = crew * (ref_500 - a) / battle_scale          # imul + 有符号 idiv 0x1F4
# 实时单挑（路径 a，前两处）：loss = min_s(crew, term)（fn_140011bd0）后写回
# 白刃画面里发生单挑（路径 b，后两处）：loss += term，之后各自 min(loss, crew)
```

溢出：`a > ref_500` 时 `term` 为负。原版 `a ≤ 500` 碰不到；重构先夹 `battle_ability_clamp`，`ref_500 - a ≥ 0`。结果过 `Modifiers.eval("combat.duel_crew_loss", …)`。

## F-CBT-02 `can_fire`

正文 [07-combat/01-naval-battle.md](../07-combat/01-naval-battle.md) §2.8 / §4.3（`fn_14000c8c4` body `0x1401bd330`）。摘要：

```
d2 = (dx/128)^2 + (dy/128)^2                  # 先截断再平方
if d2 < min_cannon_range2: no                 # 900，无符号 jb
r  = range_base[gun] + range_bonus(skill(gunner, gunnery))   # <=100: -12；<=300: +g/75；else +g/30
if d2 > r*r: no
bow / stern 弧要求对应改装件，舷侧 power *= 3*n_battery + 1
power = (power + rand(3)) / 2
```

射手岗位、炮手加成、船首像「龙像 +2」（`vfunc+0x48 == 4`）在重构里都走 `post_holder` 与修正器，不比字面量。

## F-CBT-03 `fleet_defeated`

`[确认]` 只在**无画面结算** `fn_14000d7ab`（`0x140123d30`）每回合末尾检查：后手旗舰耐久或水手 `<= 30` → 先手胜、退走标记；否则查先手旗舰（`0x140124abf`）。实时海战没有这条阈值（`07-combat/01` §2.9）。参数 `defeat_hull_le=30`、`defeat_crew_le=30`。

## F-CBT-04 `boarding_fencing`

同函数里剑术相关的两条。细节与命名归 [07-combat/02-boarding.md](../07-combat/02-boarding.md) §2.5。

| 项 | 内容 |
| --- | --- |
| 事实 | 常数 `[确认]`；`fn_14000ee12` = 舰队 `+0x71` 疲劳 `[推断，强]`（`06-ocean/01` §2）；`fn_14001399e` 体力类 `[推断]` |
| 参数 | `duel_gate=30`，`duel_roll=10`，`duel_diff_div=100`，`fencing_ref=200`，`fencing_div=2000`，`chief_best_div=1200`，`fencing_rand=3`，`melee_halve=2`；`refs.skill: fencing`（eSkill 5 剑术）、`refs.cabin_role: marine_room`（`fn_1401ab6a0` → `fn_14000dbac(ship, 7)` 扫 31 槽取舱室种类 7 水兵室的全部乘员 `[确认]`；对应岗位 8 冲锋队长） |

```
# 单挑触发（0x1401bbce0 起）
if f399e(A) > 30 and f399e(B) > 30:
    d = |Skill(A,fencing) - Skill(B,fencing)| / 100   # 无符号 div
    if rand(10 - d) == 0: 进入单挑 → F-CBT-01       # rand(n<=0)=0
# 自动肉搏（0x1401bc2ad 起；不对称）
own = player ? fn_1401ab6a0(...)                     # 水兵室（舱室种类 7）有人：b1*(200-fat)/1200 + rand(3)+1 + (b2+f)*(200-fat)/2000
             : Skill(own_admiral,fencing) * (200 - fat(own)) / 2000 + rand(3) + 1
if own_ship.vfunc_48 == 8: own += own / 5            # 只加己方（0x1401bc37e）；对方伤害不查
foe = Skill(foe_admiral,fencing) * (200 - fat(foe)) / 2000 + rand(3) + 1
own /= 2; foe /= 2                                   # cdq; sub; sar 1
```

旧稿「两侧对称」错：×1.2 只加己方，玩家侧走水兵室乘员（舱室种类 7，岗位 8 冲锋队长）的 `/1200` 式。`vfunc_48` 的接收者是 `fn_1400042b4(x)`（body `0x14023f6f0`）取回的对象：它调 `x->vfunc+0x88` 得船 id，`== 0xD0`（208 = 船实例 207 槽 +1）时返回空，否则 `fn_14000a89e`（body `0x1400a7f40`，`id*0x80+8`，`CShip` 池步长 `0x80`）取 **CShip** `[确认]`；所以与航速的 `CShip+0x53` 是同一字段，值 8 = 王者像「鼓舞水手」`[推断，强]`。重构：`Modifiers.eval("combat.melee_damage", {ship})`，船首像以 `mul 6/5` 贡献。

另：特殊擒获（岗位 7 `lookout` 了望手的 `skill.gunnery` `> 150` 且 `rand(2000) <= x/10`）见 02-boarding §2.6。

## F-CBT-05 … F-CBT-08（只登记）

| ID | 原版式（摘要） | 关键地址 | 正文 |
| --- | --- | --- | --- |
| F-CBT-05 `melee_strike` | `s = Skill(p,fencing)*gauge/100`；对手非当前槽 `×8/10`；`|Δ| > rand(70)`（无符号）分胜负，否则未决 | `fn_14000827e` `0x1400814a0` | `07-combat/02` §2.4 |
| F-CBT-06 `reload_period` | 玩家本舰队 `(1100 - g)/10`、他队 `(550 - g)/10`、城炮 `(550 - (军事/20+1))/10`；`g` = 岗位 9 命中力 | `07-combat/01` §2.10 | `07-combat/01` §4.2 |
| F-CBT-07 `offscreen_battle` | 命中 `rand(500) <= max(0, 命中 - c - 驾驶/5)`；炮伤 `power*(3n+1)*(luck+500)/500`；肉搏 `(剑术/8 - fat/10)*(luck+500)/500`，船首像 8 `+x/5` | `fn_14000d7ab` `0x140123d30` | `07-combat/02` §2.7 |
| F-CBT-08 `surrender_roll` | `roll = (Skill(岗位1,说服) + Skill(岗位0,说服))/20`；`roll <= rand(100)` 才投降 | `fn_14000601e` | `07-combat/04` §2.5 |

---

## F-OCN-01 `sailing_speed`

完整分枝、移位、洋流见 [06-ocean/01-sailing.md](../06-ocean/01-sailing.md) §2「航速」/ §4.3。主函数 `fn_14001444d`（body `0x1401b0200`）。只算**旗舰**的帆与人手；船首像扫全部 5 船。

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]`（原始指令逐条） |
| 所属 | ocean（帆值、必要水手由 Ships 供快照，F-OCN-04） |
| 参数 | `square_cone_low=42`、`square_cone_high=214`、`fore_fixed_cos=128`、`blanket_angle=32`、`sail_shift=17`、`min_speed_floor=96`、`speed_bias=192`、`current_shift=8`、`rower_gate_fatigue=50`、`crew_den_factor=2`、`battle_speed_div=3`（键名随 06-ocean §4.3） |
| YAML | `formulas/ocean.yaml` |

```
term  = 横帆枝 ? wind*(square*cos256(sail) + fore*128) : wind*fore*cos256(sail)
speed = term * cos256(sail_to_wind) / 2^17           # >>17 带符号向零
speed = speed <= 0 ? 96 : speed + fore + 192
if half_sail: speed /= 2
speed = isqrt(speed^2 + speed*cur_v*cos256(cur-ship)/256 + cur_v^2)   # 交叉项系数 1
if Session.mode() == battle or fleet_fatigue < 50: speed += F-OCN-03
if crew_now < need: speed = speed * (crew_now + need) / (2*need)       # 第四处 idiv（0x1401b07ae）
if 任一船船首像 0: speed += speed / 10               # mov ecx,0xA（0x1401b088f）
if 任一船船首像 7: speed = speed * 4 / 5             # shl 2; mov ecx,5（0x1401b08bc）
show(speed / 8)                                      # 速度条
if Session.mode() == battle: speed /= 3             # mov ecx,3（0x1401b08fe），在写速度条之后
```

`Session.mode() == battle` 即原版 `fn_140006c12(SceneDesc) == 1`（`SceneDesc+0xC`，海战态 `[推断，强]`）。旧稿「只有三处 idiv」漏了 `2*need`。

船首像 0 / 7 与目录 114 大鹫像「飞速前进」、121 魔鬼像「不祥」对得上（另 1=小猪像 F-ADJ-03、3=白鲸像最大耐久 +10、4=龙像炮威 +2、5=海豚像 F-ADJ-01、8=王者像 F-CBT-04）。`[推断，强]` `CShip+0x53` = 船首像 = `item_id − 114`。重构：两条船首像修正是 `fleet.speed` 的 `mul` 贡献（order 10/20）；海战 `/3` 是 `fleet.speed_move` 的 `mul` 贡献（order 900，写速度条之后）。公式不比 id。原版本函数**不读**任何人物技能。

## F-OCN-02 `wind_level_from_rng`

| 项 | 内容 |
| --- | --- |
| 事实 | CDF `[确认]`（`fn_1402abb20`，见 06-ocean §4.2）；`WIND_DIRECTION_ADJUST_*` / `WIND_VELOCITY_ADJUST_*` 整数 `[未知]` |
| 参数 | `cdf=[30,50,70,80,87,96]`，`high_band_rand=20`，`high_band_cuts=[4,7,10,13,16,18]` |
| 源 | 06-ocean §2「风向与风力」/ §4.2；`constants.json` MAXMIN |

不是每 tick 掷：唯一调用者 `CWorld::vfunc_04` 在世界风计时到期、且玩家 happening 态 `== 0x1B`（空闲）或 `== 4 且 rand(5)==0` 时重掷，时长 `(rand(5)+10) * 48` 拍（`0x1402ac245 add eax,0xa` / `0x1402ac248 imul eax,eax,0x30`，即 10–14 天；48 = `$caps.calendar_ticks_per_day`）交 `fn_14000592a`。旧稿 `^2` 错。档位是否进入 `iWindVelocity` `[未知]`。

无风：`iWindVelocity = 0` 的条件是 happening 结局态 `CPlayerFleetData+0x30 == 0x18`，**不是气候 id**；否则 `流场速 + 调整 + 3`，航速函数里再 `+3`（共 +6，两处分开配）。

## F-OCN-03 … F-OCN-06（只登记）

| ID | 原版式（摘要） | 关键地址 | 正文 |
| --- | --- | --- | --- |
| F-OCN-03 `rowing_bonus` | `10 * cabin_count(rower_room) / crew_per_mast_by_grade[grade]`，表 `{3,5,10}`（`.rdata 0x140596d98`） | `fn_14000da9e` body `0x14023e1b0` | `06-ocean/01` §2 |
| F-OCN-04 `sail_values` / `crew_needed` | 四帆值 `max(0, k*(mast项 + 改装项) - penalty)`，`penalty = {0,4,10}[grade] + 改装罚`；`need = base[grade] + Σ桅 + 改装 + 40*划桨室 + 20*炮台`；容量 `= 5*base + 60*水兵室 + need` | `fn_14000a466`、`fn_140012a26`、`fn_14001292c` | `06-ocean/01` §2、`05-ships-fleet/06` §4.1 |
| F-OCN-05 `supply_drain` | 见 F-ADJ-03（除数 10；厨师、饲养员掷骰加除数；小猪像 +2；happening 态 1 粮 ×95/100、态 2–5 水 +10） | `fn_140005aa6` body `0x1401dc830` | `03-adjustment-factors.md` §4.5 |
| F-OCN-06 `explore_supply_roll` | `rand(100) <= 10*rich + Skill(提督, observation)/50` | `idiv 0x32` @ `0x1401b5201` | `06-ocean/02` §2 探索流程 |

`{3,5,10}` 同时是划桨除数与必要水手基数：一张 `crew_per_mast_by_grade` 表，归 `ships/rig.yaml`，Ocean 与 Ships 都引用它，不各存一份。

## F-SHP / F-TWN（只登记）

| ID | 原版式（摘要） | 关键地址 | 正文 |
| --- | --- | --- | --- |
| F-SHP-01 `shipyard_listing_threshold` | `(commerce + military/2)/5 + 30*year`；船模 `develop_need > threshold` 不上架 | `fn_14000d9a9` `0x1402613e0` | `05-ships-fleet/05` 造船厂 |
| F-TWN-01 `inn_recover` | `cur = min(cur + max/20, max)` | `idiv 0x14` | `08-town-life/05` |
| F-TWN-02 `rite_day` | `(town_id + 1) % 28 + 1` | `fn_14000eacf` body `0x140173d80` | `08-town-life/05` |
| F-TWN-03 `scheme_contest` | `Skill(agent, tactics) - Skill(target_leader, tactics)/2` | `cdq; sub; sar 1` | `08-town-life/04` §2.7 / §4 |

F-TWN-02 依赖 `town_id` 的数值顺序：重构改成 `rite_day(town_key)` 可替换公式，vanilla 包用 `vanilla_index` 复刻。

---

## 耦合分析

原版硬编码：

- 经验除数 30、技能 150/100/200、买货 1000、接舷 500、道具 100/50 散落立即数
- `fn_14000e1ce(person, 字面量技能下标)` 被贸易、战斗、道具直调；技能↔两维写在 `.data` 表；六维成长按段写死 `{0,1,2}` / `{3,4,5}`
- 同一个 `CShip+0x53`（船首像）被航速（0、7）、视野（5）、补给（1）、最大耐久（3）、炮威（4）、白刃（8）各自 `== 字面量` 判断
- 岗位（会计长 3、炮手 9）与舱室种类（水兵室 7、炮台 8、货舱 10、划桨室 11）字面量
- 剧本号比较：剧本 2 卖价 ×105/100、流行影响力 ×2；剧本 6 附庸税；剧本 0 委托 ×1.5、军事八折（`04-economy/07` §4.8）
- 交易、委托、自动航海都假设 5 船 × 5 格 × 不堆叠
- 份额 80、信件 10/15/16、同品 15、地方舰队 100000 写在指令或文案里
- `NUM_TOWN=97` 等既是内容计数又是循环上界

重构后：字面量 id 进内容表 `effects` / `role` / `refs`；剧本差异进剧本包 `scenario.flags` 与主角修正器，由叙事开局注入修正器注册表；公式文件里不比剧本号。依赖方向见 `03-decoupling-principles.md`。公式包版本进存档元数据；**不存公式原文**。读档用当前包重算派生值（等级缓存、技能）。随机数按系统用命名流（`rand(n)` 取调用方传入的流），种子与流状态进存档；原版只有一个以 `time()` 播种、不存档的 CRT `rand()`。

## 解耦后的配置

```
config/formulas/progression.yaml   # F-PROG-01..04、10；经验授予 grant_routes
config/formulas/character.yaml     # F-PROG-05、06、09：技能值、夹紧（系数在 characters/vitals.yaml）
config/formulas/items.yaml         # F-ECO-02 item_bargain、装备加成（09-items 所有）
config/formulas/economy.yaml       # F-ECO-01、03、07..10、13..18
config/formulas/factions.yaml      # F-ECO-04..06 流行品份额与发信（02-factions 所有，词表 §14-9）
config/formulas/investment.yaml    # F-ECO-11（04-economy 所有）
config/formulas/tribute.yaml       # F-ECO-12、19（04-economy 所有）
config/formulas/combat.yaml        # F-CBT-*
config/formulas/ocean.yaml         # F-OCN-01..03
config/formulas/supply.yaml        # F-ADJ-03 / F-OCN-05
config/formulas/vision.yaml        # F-ADJ-01/02
config/formulas/ships.yaml         # F-SHP-*
config/formulas/town.yaml          # F-TWN-*
config/ships/rig.yaml              # F-OCN-04 帆值 / 必要水手表（Ships 所有）
config/ocean/search.yaml           # F-OCN-06
config/caps.yaml
config/engine_limits.yaml
config/characters/skills.yaml      # stat_a/stat_b 配对（内容，不是公式）
config/characters/zodiac.yaml      # 星座成长倾向（F-PROG-10）
```

示例见 `Docs/14-config-schemas/examples/formulas/`。

### 公式怎么替换

每条公式在 YAML 里是 `{impl, int_mode, quirks, params, tables, refs, inputs, expr, golden, modifiers}`（规格见 [14-config-schemas/README.md](../14-config-schemas/README.md#公式层d3)）。四档：

| 档 | 用于 | 例 |
| --- | --- | --- |
| ① `params` / `tables` / 分段 | 全部公式 | 改除数、改 CDF、改 grade 表 |
| ② `impl: expr` | 单个整数表达式（沙箱；`/` 向零截断、`udiv`、`rand(n)` 命名流），模组整条替换 | F-PROG-01/02/05、F-ECO-01/02/05/10/15、F-CBT-01/04、F-OCN-03、F-TWN-* |
| ③ `impl: vanilla.<name>` / `<pack>.<name>` | 控制流重的原版实现或插件，签名同上 | F-OCN-01、F-ECO-11/16/17、F-ADJ-03、F-PROG-10、F-CBT-07 |
| ④ `modifiers` | 公式输出的统一出口，按 key 加贡献 | 船首像、装备、主角、剧本旗标 |

- `int_mode`：vanilla 包统一 `vanilla_i32`（32 位回绕、`/` 向零截断）；新内容包可用 `i64_sat`。逐条差异见索引 `int` 列。
- `quirks`：原版怪癖显式开关，不悄悄修。本表已登记：`quirk_min_q`（F-PROG-01，带数值，放 `params`：2 复刻原版，1 = 数学 isqrt）、`q65536_div_zero`（F-PROG-01：`q` 为 65536 整倍数时原版 `div` 除零崩溃；重构默认饱和、此开关只供对拍）、`unsigned_div`（F-ECO-01/02/10/15/16）、`wrap_mul16`（F-PROG-01 大等级）、`u8_initial_crew`（F-OCN-04 初始水手截 8 位）、`old_zero_flourish`（F-ECO-11 `old==0` 每月发文案）。
- `golden`：本表每条「原版」块就是 `vanilla.*` 的规格，也是 `expr` 版逐位对拍原版的回归基准。
- 通用脚本只在叙事事件边界，不进公式。

## 独立旋钮

改这些值不应迫使贸易/战斗改代码：

- `exp_divisor`、`level_per_track`、`skill_level_cap`、`level_bias`、`skill_divisor`、`skill_value_cap`
- `ref_1000` / `ref_500` / `ref_100`
- `buy_ability_clamp` / `battle_ability_clamp`（可在提高 `skill_value_cap` 时单独留在 500）
- `cargo_stack`、`fleet_ships`、`hold_slots_per_ship`
- `fashion_share_div`、`fashion_threshold_numer/denom`、`letter_intro_min/max`、`letter_recommend_min`、`letter_upgrade_min`、`share_letter_cut`（都在 `formulas/factions.yaml`）
- 各公式的 `refs.skill` / `refs.post` / `refs.cabin_role`（替代旧 `*_skill_index`、`item_post`）
- `condition_mul[*]`、`mental_adjust[*]`、`track_stats`、`growth_rand_*`、`roll_mode_default`
- `quirk_min_q`（复刻原版「30–59 经验仍 0 级」与否）

## 对外接口

```
Progression.level_from_exp(exp) -> int
Characters.skill(person_id, skill_key) -> int                # 技能值（F-PROG-05 base → F-PROG-09 堆叠）
Modifiers.eval(key, ctx) -> int                               # 统一出口，如 skill.accounting、fleet.speed
Modifiers.contribute(key, {stage, order, source, value})      # 提供方注册
Fleets.post_holder(fleet_id, post_key) -> Option<PersonId>
Ships.cabin_count(ship_id, role) -> int
Economy.npc_buy_cost(price, qty, leader_id, rng) -> gold
Economy.quote(town_id, trading_item_id, grade) -> PriceSnapshot
Combat.settle(...) -> BattleResult
Ocean.speed(fleet_id, wind_snapshot, sailing_snapshot) -> speed   # sailing_snapshot = Fleets.sailing_snapshot(fleet_id)，数据由 Ships 供（F-OCN-04）
```

只传 ID 与值，不传 `World*`、不传六维数组。

## 未核实

- `CShip+0x53` 是否必为 `eFigureHead`（七处效果与 114–123 顺序吻合；`vfunc_09`（`0x14023e7f0`）查不到船回退 `0x0D` = 无；比较值 10 / 11 对应道具 199 怪鱼像 / 200 提督像，见 `05-ships-fleet/04`；写入点未穷尽）
- 特产品消耗、修理底价（F-ECO-13）
- `letter_state` 的正式枚举名
- `fn_14000ee12`（舰队 `+0x71`，疲劳 `[推断，强]`）、`fn_14001399e`（单挑门槛量）、`fn_1401a8730(2,5)` 的语义
- `0x14008c70e` 处 `skill_value(..., use_bias_stats=0)` 的玩法用途
- `SHARE_*` / `MARKET_PRICE_*` / `FATIGUE_*` / `WIND_*_ADJUST_*` 等 MIN/MAX 整数

已解决（本轮移出）：F-ECO-10 两字段名（目录价 / 文化圈卖价）；伦敦坐标换算（`World.dat` = 2500×1250 `uint16`，航行坐标 `/16`（向零）→ chip 下标 `[确认]`，港口 `x1/y1` 同一空间 `[推断]`，见 `01-world/01-map-and-sea-areas.md` §2）。

查明后只改本表对应条目与 YAML default，不改调用方代码。
