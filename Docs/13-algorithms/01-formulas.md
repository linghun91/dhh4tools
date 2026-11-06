# 公式总表（Master Formula Catalog）

本文是重构引擎的**唯一公式目录**。系统文档（`Docs/01-world` … `Docs/12-ui-save`）若再写算法，只引用这里的公式名，不另发明一套符号。

初稿从 `srcdata/docs` 汇编。域文档成文后，下列公式以域文档为准并回链本表，禁止第三套符号：

- 航速 / 风力档：`Docs/06-ocean/01-sailing.md`（`fn_14001444d`；F-OCN-01 不再是空占位）
- 海战射程与溃败：`Docs/07-combat/01-naval-battle.md`
- 接舷 `500-a`：`Docs/07-combat/02-boarding.md`
- 市价报价：`Docs/04-economy/02-market-price.md`
- 技能名与两维配对：`Docs/03-characters/03-abilities-skills-jobs.md`

配置落点：`config/formulas/*.yaml`。上限落点：`config/caps.yaml`、`config/engine_limits.yaml`（见 [02-caps-ranges.md](02-caps-ranges.md)）。

## 职责边界

管：

- 命名纯函数：输入、参数、输出、溢出、所属系统、源路径
- 把原版魔数收成 `params`（`exp_divisor`、`ref_1000`、`ref_500`、`ref_100` …）
- 把人物六维/技能接到 `ModifierSet`，供经济、战斗、道具读取

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

原版把「魅力/智力/技能」直接塞进买货、战斗、道具价。重构后人物系统只暴露 `ModifierSet`，各系统不读六维原值。

## 实体与 ID

公式用稳定英文名，YAML key 同名。不分配整数 ID。

技能下标是 `eSkill`（`NUM_SKILL=10`）。原版 `Ability(person, k)`（`fn_14000e1ce` → `fn_1400135cf`）按 **技能下标** 取值，不是六维下标：买货/道具价用 `k=4`，接舷结算用 `k=1`。技能中文名未从头文件还原，配置里用 `skill_04` 这种 key，文案另表。

`fn_1400099e9` 检查 `skill ∈ [0,10)` 且第二下标 `∈ [0,2)`。`[推断]` 每项技能绑两个 `eAbility`。配对表本身 `[未知]`，属于 `config/characters/skills.yaml`。

## 溢出总则

| 原版 | 重构必须 |
| --- | --- |
| 单段等级 `> 255` 时 32 位 `isqrt` 溢出，画面等级变 0 | **饱和**到 `level_per_track`，禁止绕回 0 |
| `ability_settled_cap > 1000` 时 NPC 买货 `(1000-a)` 为负，无符号 `div` 记几百万 | 先 `clamp` 再减；用有符号饱和算术 |
| `a > 500` 时 `500-a`、`100-a/50` 同类变负 | 各公式自带 `*_clamp`，与结算上限解耦 |
| 经验写入 `min(xp, 30*track²)`，四处立即数 | 按 `exp_from_level(level_per_track)` 重算，不写死 `300000` |

`engine_limits.yaml` 给整数宽度；`caps.yaml` 必须 ≤ 硬顶。缺字段不回退代码默认值。

## 命名参数与 ModifierSet

原版魔数变成参数。基准常数用 `ref_*`，避免代码里出现裸 `1000`/`500`/`100`：

| 参数 | 原版 default | 用途 |
| --- | --- | --- |
| `exp_divisor` | 30 | 经验开方除数 |
| `level_bias` | 100 | 技能公式 `min(level,skill_cap)+bias` |
| `skill_divisor` | 150 | 技能公式分母 |
| `ref_1000` | 1000 | NPC 买货基准 |
| `rand_span` | 100 | `rand(rand_span)` → `[0, rand_span)` |
| `buy_ability_clamp` | 500 | 买货用能力夹紧，**独立于** `ability_settled_cap` |
| `ref_500` | 500 | 战斗结算基准 |
| `battle_ability_clamp` | 500 | 战斗用能力夹紧 |
| `ref_100` | 100 | 道具价 / 份额封顶基准 |
| `ability_div` | 50 | 道具价 `a / ability_div` |
| `item_price_scale` | `[推断]` 100 | 乘因子后的分母；反编译 `idiv` 丢失 |

人物系统输出（原则 6）：

```
ModifierSet = {
  buy_cost_factor,      # F-ECO-01
  item_price_factor,    # F-ECO-02
  battle_offense_factor,# F-CBT-01
  sell_price_factor,    # [未知] 占位
  sailing_speed_factor, # [未知] 占位
  ...
}
```

取值一律 `clamp` 后再进因子，禁止把未夹紧的技能原值交给贸易/战斗。

## 公式索引

| ID | 名称 | 系统 | 等级 | YAML |
| --- | --- | --- | --- | --- |
| F-PROG-01 | `level_from_exp` | progression | `[确认]` | `formulas/progression.yaml` |
| F-PROG-02 | `exp_from_level` | progression | `[确认]` | `formulas/progression.yaml` |
| F-PROG-03 | `track_level` | progression | `[确认]` | `formulas/progression.yaml` |
| F-PROG-04 | `total_level` | progression | `[确认]` | `formulas/progression.yaml` |
| F-PROG-05 | `skill_value` | progression / characters | `[确认]` | `formulas/progression.yaml` |
| F-PROG-06 | `clamp_settled` | characters | `[确认]` | `formulas/character.yaml` |
| F-PROG-07 | `person_present` | characters | `[确认]` | （谓词，无 YAML 魔数） |
| F-PROG-08 | `person_recruited` | characters | `[确认]` | （谓词） |
| F-ECO-01 | `npc_buy_cost` | economy | `[确认]` | `formulas/economy.yaml` |
| F-ECO-02 | `item_price_factor` | items / economy | `[确认]` 因子；整式 `[推断]` | `formulas/economy.yaml` |
| F-ECO-03 | `ledger_balance` | factions / economy | `[确认]` | `formulas/economy.yaml` |
| F-ECO-04 | `fashion_share_threshold` | economy | `[确认]` | `formulas/economy.yaml` |
| F-ECO-05 | `fashion_share_gain` | economy | `[确认]` | `formulas/economy.yaml` |
| F-ECO-06 | `fashion_letter` | economy | `[确认]` | `formulas/economy.yaml` |
| F-ECO-07 | `share_apply_cap` | economy | `[确认]` | `formulas/economy.yaml` |
| F-ECO-08 | `count_fleet_item` | economy | `[确认]` | `formulas/economy.yaml` |
| F-ECO-09 | `add_load` | economy | `[确认]` | `formulas/economy.yaml` |
| F-ECO-10 | `same_item_market_adjust` | economy | `[确认]` 门槛；式子 `[未知]` | `formulas/economy.yaml` |
| F-ECO-11 | `investment` | economy | `[未知]` 占位 | `formulas/economy.yaml` |
| F-ECO-12 | `tribute` | town-life | `[未知]` 占位 | `formulas/economy.yaml` |
| F-ECO-13 | `repair_cost` | ships / town-life | `[未知]` 占位 | `formulas/economy.yaml` |
| F-ECO-14 | `local_fleet_founding_gold` | fleets | `[确认]` 常数文案 | `formulas/economy.yaml` |
| F-CBT-01 | `battle_settlement_factor` | combat | `[确认]` 因子；整式 `[推断]` | `formulas/combat.yaml` |
| F-OCN-01 | `sailing_speed` | ocean | 分枝 `[确认]`，三处 `idiv` 除数 `[未知]` | `formulas/ocean.yaml` |
| F-OCN-02 | `wind_level_from_rng` | ocean | CDF `[确认]` | `formulas/ocean.yaml` |
| F-CBT-02 | `can_fire` | combat | 最小距 900、射界字节 `[确认]` | `formulas/combat.yaml` |
| F-CBT-03 | `fleet_defeated` | combat | 旗舰耐久或水手 ≤30 `[确认]` | `formulas/combat.yaml` |

---

## F-PROG-01 `level_from_exp`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `exp: uint32` 单段经验 |
| 参数 | `exp_divisor: int = 30` |
| 输出 | `level: int ≥ 0` |
| 所属 | progression |
| 源 | `srcdata/docs/save_format.md`；`srcdata/source/save/Save.cpp` `level_of_exp`；`srcdata/include/save.h`；`dhh4tools/dk4save.c` |

```
if exp < exp_divisor: return 0
q = floor(exp / exp_divisor)
level = isqrt(q)          # 最大整数 r 满足 r*r ≤ q
```

溢出：原版 MSVC 调试 32 位开方，`level > 255` 画面变 0。重构饱和到 `caps.level_per_track`（设计）且不超过 `engine_limits.level_per_track`（255）。禁止绕回。

`Save.cpp` 的演示循环用 `r < 400` 防跑飞，不是玩法上限。

ModifierSet：无。等级是派生缓存，经验是源。

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

原版写入经验时四处立即数 `30*track²`。`track=100` 时 `300000`（`0x493E0`，`fn_140006c67` 亦用此立即数夹经验）。重构禁止写死 `300000`，按本式从 `level_per_track` 重算。

溢出：`level > 255` 先夹到硬顶再乘；用 64 位中间值，再饱和到经验字段宽度。

## F-PROG-03 `track_level`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `exp` 单段 |
| 参数 | `exp_divisor`、`level_per_track`（原版设计 100，工具硬顶 255） |
| 输出 | 该段显示/规则等级 |
| 源 | `exe_caps.md`「单段等级」两处 `min(level, track cap)`；`dk4save.c` `track_level` |

```
level = min(level_from_exp(exp), level_per_track)
```

存档有两段经验：名册 `+0x0B` / `+0x0F`，主角体 `+0x35B2` / `+0x35B6`。

## F-PROG-04 `total_level`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `exp0`, `exp1` |
| 参数 | 同 F-PROG-03；`track_count = 2` |
| 输出 | 总等级 |
| 源 | `save_format.md` `+0x70` 缓存主角总等级；`dk4save.c` `total_level` |

```
total_level = track_level(exp0) + track_level(exp1)
```

`+0x70` 是缓存。重构：经验是源，等级可缓存但必须能从源重建。技能公式用的「等级」是这个总等级，不是单段。

溢出：两段各 255 时合计 510。原版技能再被 `skill_cap=200` 截断。

## F-PROG-05 `skill_value`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `stat_a`, `stat_b`（两项六维），`level`（总等级） |
| 参数 | `skill_cap=200`，`level_bias=100`，`skill_divisor=150` |
| 输出 | 未结算技能值（随后 F-PROG-06 夹紧） |
| 所属 | progression 算、characters 拥有配对 |
| 源 | `save_format.md`；`Save.cpp`；`dk4save.c` 1209 行 |

```
skill = (stat_a + stat_b) * (min(level, skill_cap) + level_bias) / skill_divisor
```

整数除法，向 0 截断（MSVC 对非负即向下）。

技能**不存档**。配对哪两个六维 `[未知]`，表在 `characters/skills.yaml`。

溢出：原版技能封顶 200 写在主程序两处相同立即数。硬顶注释「两段之和 510」。重构：`min(level, skill_cap)` 饱和；乘法用 64 位。

ModifierSet：本式是技能原值。买货/战斗/道具价读的是夹紧后的技能，再变成因子。

## F-PROG-06 `clamp_settled`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `value`（技能或结算属性） |
| 参数 | `ability_settled_cap=500` |
| 输出 | `[0, ability_settled_cap]` |
| 源 | `exe_caps.md` 属性结算；`fn_1401cbb40` 以立即数 `0x1F4=500` 调用 `fn_1400103d4` |

`Ability(person, k)` 的结果封在 `[0, 属性结算上限]`。NPC 买货、道具价、战斗结算都走这条。

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

名册 98 槽（`SAVE_SAILOR_COUNT`）与 235 人物表不是同一集合。

---

## F-ECO-01 `npc_buy_cost`

地方舰队 / NPC 舰队自动买货。

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `price`, `qty`；船长 `person_id` |
| 参数 | `ref_1000=1000`，`rand_span=100`，`buy_ability_clamp=500`，`buy_skill_index=4` |
| 输出 | `cost: int`（写入账本支出） |
| 所属 | economy（读 ModifierSet，不读六维） |
| 源 | `srcdata/docs/exe_caps.md` `fn_140010d25` / body `0x1400f6d20`；`dhh4tools/dk4save.c` `PROXY_BUY_BEGIN` |

原版：

```
a    = Ability(captain, buy_skill_index)     # 已按 ability_settled_cap 夹紧
cost = price * qty
cost = cost * (ref_1000 - a + rand(rand_span)) / ref_1000
record.expense += cost                       # 无账本则扣势力金币
```

`rand(rand_span)` 为 `[0, rand_span)`。32 位 `imul` + **无符号** `div`。

溢出（原版 bug）：`a > ref_1000` 时括号为负，无符号除法变成几百万。`ability_settled_cap > 1000` 即触发。月结 `CFleetElement::vfunc_04`（约 `0x140100180`）付不起的转入下月支出。

补丁：买货路径把 `a` 再夹到 500，不改结算上限。

重构：

```
a = ModifierSet.buy_skill                    # 已是 F-PROG-05 + F-PROG-06
a = clamp(a, 0, buy_ability_clamp)
factor = (ref_1000 - a + rand(0, rand_span-1)) / ref_1000   # 有符号，饱和 ≥ 0
cost = saturating_mul_div(price, qty, factor)
```

`buy_cost_factor` 即这里的 `factor`。禁止贸易代码读 `eAbility`。

## F-ECO-02 `item_price_factor`

道具价格相对人物能力的折扣。

| 项 | 内容 |
| --- | --- |
| 事实 | 因子 `[确认]`；乘回单价的分母 `[推断]` |
| 输入 | 基础价 `price`；人物 `person_id` |
| 参数 | `ref_100=100`，`ability_div=50`，`item_skill_index=4`，`item_price_scale` |
| 输出 | `item_price` |
| 所属 | items（读 ModifierSet） |
| 源 | `exe_caps.md` `fn_14000f4ed`（`100 - a/50`）；反编译 `srcdata/source/game/unassigned/fn_140154030.cpp`：`Ability(*, 4)` 后 `idiv` + `price * (0x64 - rax)` |

原版因子：

```
a = Ability(person, item_skill_index)
factor_term = ref_100 - floor(a / ability_div)
```

随后 `imul` + `idiv`/`div`，除数在反编译中丢失。若不再除一次，`a=0` 时价格会 ×100，故：

```
item_price = price * factor_term / item_price_scale
```

`item_price_scale` 原版 `[推断]=ref_100=100`。未核实前 YAML 仍写出该键，禁止在代码里写死。

溢出：`a/ability_div > ref_100` 时因子为负。重构：`a = clamp(a, 0, ability_div * ref_100)`，`factor_term ≥ 0`。

`ModifierSet.item_price_factor = factor_term / item_price_scale`。

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
| 输入 | 五船货舱格数之和 `hold_slots`（`6b95(ship, 0x0A)`，扫描 10 格） |
| 参数 | `fashion_threshold_factor`（工具 `fthr`，原版 1） |
| 输出 | 门槛 `threshold` |
| 源 | `srcdata/docs/trade_stack.md` DK4FSH1；`dk4save.c` CapValues |

原版：

```
threshold = ceil(hold_slots / 2)
```

补丁：

```
threshold = (hold_slots * fashion_threshold_factor + 1) >> 1
```

`fthr=1` 时与原版等价。重构只保留补丁式，default `fashion_threshold_factor=1`。

## F-ECO-05 `fashion_share_gain`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `x` = 本次卖出的两种流行品**件数**（STK3 已按件数计）；`town_id`；`force_id`；当前份额 |
| 参数 | `share_high=80`（`0x50`），`fashion_div=6`（工具 `fdiv`），`fashion_gain_add=1` |
| 输出 | 拟增加的份额点数 |
| 源 | `trade_stack.md` `CommitDeal` `0x140277090` |

份额 `< share_high` 时，每种流行品件数 `≥ threshold` 才加：

```
gain = floor(x / fashion_div) + fashion_gain_add
```

然后经 F-ECO-07 封顶。`122f6(town, force, gain, 1)`。

份额 `≥ share_high` 走介绍信/推荐信（F-ECO-06），不加这条点数。

## F-ECO-06 `fashion_letter`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `x` 流行品件数；当前信件状态 `letter_state` |
| 参数 | `letter_lo=10`（`fl1`），`letter_hi=16`（`fl2`） |
| 输出 | 信件升级事件 |
| 源 | `trade_stack.md`；四处 `cmp imm8` |

`bd98(s550)`：`2` = 无信，`0` = 有介绍信。`[推断]` 枚举值，字段名未恢复。

```
if share >= share_high:
  if letter_state == none and letter_lo <= x < letter_hi: grant 介绍信
  if letter_state == none and x >= letter_hi: grant 推荐信
  if letter_state == intro and x >= letter_hi: upgrade 推荐信
```

重构把 `letter_state` 做成正式枚举，不暴露 `0/2`。

## F-ECO-07 `share_apply_cap`

| 项 | 内容 |
| --- | --- |
| 事实 | `[确认]` |
| 输入 | `current_share`, `gain` |
| 参数 | `share_cap = 100`（`ref_100` 可复用） |
| 输出 | 新份额 |
| 源 | `trade_stack.md`：`54cf` 不吃 ecx；`0x1402635f6` 封到 `100 − 已占` |

```
applied = min(gain, share_cap - current_share)
new_share = current_share + max(applied, 0)
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
| 事实 | 门槛 `[确认]`；调整式 `[未知]` |
| 参数 | `same_item_adjust_threshold=15` |
| 源 | `trade_stack.md`：`0x1402777ae` 卖出同一商品 ≥15 时调整城市该商品数值（非流行品） |

占位：

```
if sold_qty_of_same_item >= same_item_adjust_threshold:
    market_adjust(town_id, trading_item_id, sold_qty)  # 式子未还原
```

## F-ECO-11 `investment`（占位）

| 项 | 内容 |
| --- | --- |
| 事实 | `[未知]` |
| 所属 | economy |
| 已知 | 账本 `+0x14` 投资；文案「多亏了巨额的商业投资」；配置将落 `economy/investment.yaml` |
| 参数占位 | `investment_gold_min`, `investment_gold_max`, `development_gain_curve` — **无原版 default，禁止填假数** |

改发展度时仍受 `town_develop` 上限（原版 9999，uint16 硬顶 65535）。

## F-ECO-12 `tribute`（占位）

| 项 | 内容 |
| --- | --- |
| 事实 | `[未知]` |
| 所属 | town-life（宫殿），读 economy 金币 |
| 已知 | 文案「交了金币%s枚」；`CPalace` 无公式注释 |
| 参数占位 | `tribute_gold` 曲线、友好度增益 — 无 default |

## F-ECO-13 `repair_cost`（占位）

| 项 | 内容 |
| --- | --- |
| 事实 | `[未知]` |
| 所属 | ships / town-life（造船厂） |
| 已知 | `CShipyard` 无公式注释；`SHIP_DURABILITY_MIN/MAX` 为 USHORT 符号 |
| 参数占位 | `repair_gold_per_durability` — 无 default |

## F-ECO-14 `local_fleet_founding_gold`

| 项 | 内容 |
| --- | --- |
| 事实 | 文案常数 `[确认]`；是否唯一计费点 `[未知]` |
| 参数 | `local_fleet_founding_gold=100000` |
| 源 | `mesfile_sc.txt`「建立地方舰队需要１０００００枚金币」 |

进 `formulas/economy.yaml`，禁止 UI 字符串里藏逻辑。

---

## F-CBT-01 `battle_settlement_factor`

接舷/战斗结算里与人物能力相关的项。

| 项 | 内容 |
| --- | --- |
| 事实 | 因子 `[确认]`；乘回伤害/结果的分母 `[推断]` |
| 输入 | 某基准量 `base`；人物 `person_id` |
| 参数 | `ref_500=500`，`battle_ability_clamp=500`，`battle_skill_index=1`，`battle_scale` |
| 所属 | combat |
| 源 | `exe_caps.md` `fn_14000660e`（`500 - a`）；反编译 `fn_1401b9940.cpp`：`Ability(person, 1)` 后 `idiv` + `base * (0x1F4 - rax)`，四处相同 |

```
a = Ability(person, battle_skill_index)
term = base * (ref_500 - a) / battle_scale
```

`battle_scale` `[未知]`（反编译丢失）。YAML 必须留键。

溢出：`a > ref_500` 因子为负。重构夹到 `battle_ability_clamp`，`ref_500 - a ≥ 0`。

`ModifierSet.battle_offense_factor = (ref_500 - clamp(a,0,battle_ability_clamp)) / ref_500`（把 `ref_500` 当尺度；若原版其实不除 `ref_500`，由 `battle_scale` 表达，代码只读参数）。

同函数另有 `Ability(*, 5)` 比较、`0x32/0x28/0x0A/0x64` 分支，语义 `[未知]`，不收入本条。查明后另开 `F-CBT-02+`。

---

## F-OCN-01 `sailing_speed`

完整分枝、移位、耐久衰减见 [06-ocean/01-sailing.md](../06-ocean/01-sailing.md) §4.3。主函数 `fn_14001444d`（body `0x1401b0200`）。

| 项 | 内容 |
| --- | --- |
| 事实 | 顺风/逆风分枝与 `>>17` `[确认]`；三处 `idiv` 除数 `[未知]` |
| 所属 | ocean |
| 参数 | `downwind_cone_lo=42`、`sail_shift=17`、`min_speed_floor=96`、`speed_bias=192`；`type_a_div` 等保持 null |
| YAML | `formulas/ocean.yaml` |

禁止把丢失的除数猜成 2 或 3。

## F-OCN-02 `wind_adjust`

| 项 | 内容 |
| --- | --- |
| 事实 | 符号 `[确认]`，式子 `[未知]` |
| 参数 | `WIND_DIRECTION_ADJUST_MIN/MAX`，`WIND_VELOCITY_ADJUST_MIN/MAX`（值未还原） |
| 源 | `constants.json` MAXMIN |

---

## 耦合分析

原版硬编码：

- 经验除数 30、技能 150/100/200、买货 1000、战斗 500、道具 100/50 散落立即数
- `Ability(person, 字面量下标)` 被贸易、战斗、道具直调
- 交易、委托、自动航海都假设 5 船 × 5 格 × 不堆叠
- 份额 80、信件 10/16、同品 15、地方舰队 100000 写在指令或文案里
- `NUM_TOWN=97` 等既是内容计数又是循环上界

重构后允许的依赖方向见 `03-decoupling-principles.md`。公式包版本进存档元数据；**不存公式原文**。读档用当前包重算派生值（等级缓存、技能）。

## 解耦后的配置

```
config/formulas/progression.yaml   # F-PROG-*
config/formulas/economy.yaml       # F-ECO-*
config/formulas/combat.yaml        # F-CBT-*
config/formulas/ocean.yaml         # F-OCN-*
config/formulas/character.yaml     # 夹紧、技能配对引用
config/caps.yaml
config/engine_limits.yaml
config/characters/skills.yaml      # stat_a/stat_b 配对（内容，不是公式）
```

示例见 `Docs/14-config-schemas/examples/formulas/`。

## 独立旋钮

改这些值不应迫使贸易/战斗改代码：

- `exp_divisor`、`level_per_track`、`skill_cap`、`level_bias`、`skill_divisor`
- `ref_1000` / `ref_500` / `ref_100`
- `buy_ability_clamp`（可在提高 `ability_settled_cap` 时单独留在 500）
- `cargo_stack`、`fleet_ships`、`hold_slots_per_ship`
- `fashion_div`、`letter_lo`、`letter_hi`、`share_high`
- `buy_skill_index` / `item_skill_index` / `battle_skill_index`

## 对外接口

```
Progression.level_from_exp(exp) -> int
Progression.skill_value(person_id, skill_id) -> int
Characters.modifiers(person_id) -> ModifierSet
Economy.npc_buy_cost(price, qty, modifiers, rng) -> gold
Economy.quote(town_id, trading_item_id) -> PriceSnapshot
Combat.settle(..., modifiers) -> BattleResult
Ocean.speed(fleet_id, wind_snapshot, modifiers) -> speed
```

只传 ID 与值，不传 `World*`、不传六维数组。

## 未核实

- 十项技能中文名与 `stat_a/stat_b` 配对
- `item_price_scale`、`battle_scale` 确切除数
- 市价弹性、特产品供给消耗、投资/进贡/修理/航速
- `letter_state` 的正式枚举名
- 伦敦坐标 `20080,4272` 与 `World.dat` 2500×2500 的换算（禁止擅自 `/8`）
- `SHARE_*` / `MARKET_PRICE_*` / `FATIGUE_*` 等 MIN/MAX 整数

查明后只改本表对应条目与 YAML default，不改调用方代码。
