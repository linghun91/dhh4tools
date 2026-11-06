# 接舷、白刃与单挑

海战里船贴在一起之后，原版切到另一套 View：船员白刃（`CCombatCrew*`）或提督单挑（`CCombatSingle*`）。这一篇只定义近战规则。炮击与船体见 `01-naval-battle.md`。人物六维不进本系统，只消费 `ModifierSet`。

## 1. 职责边界

本系统管：

- 接舷会话：谁登谁的船、两侧参战人物、两侧「战力条」
- 甲板白刃与箱庭白刃两种布局
- 提督单挑（决斗）的触发与结算
- 近战导致的水手增减、投降、擒获船、拖船与货舱分配（经事件交出）
- 接舷演出用的 Sylphid 动画句柄（只发「播哪一段」，不解释 dk4 资源）

本系统不管：

- 炮弹、射界、城炮（`01-naval-battle.md`）
- NPC 何时选择接舷而不是开炮（`03-battle-ai.md`）
- 冲锋队长 / 水兵室 / 撞角的安装（`ships/` 只读）
- 把俘虏写进名册的规则细节（`characters` 订阅 `CrewCaptured`）
- 市价、经验、UI 文案

撞角命中（「撞角撞上了敌舰」`0xf677`）是海战命令的结果，本系统接收 `BoardingRequested`，不自己找碰撞几何。

## 2. 原版事实

### 2.1 类与画面

| 类 | 继承 | 体积（dtor `fn_14000750e`） | 作用 |
| --- | --- | --- | --- |
| `CCombatCrew` | `CView` | 0x4c8 | 白刃会话基类 |
| `CCombatCrewBox` | `CCombatCrew` | 0x4f0 | 箱庭布局；ctor 写 `+0x4d0 = 2` |
| `CCombatCrewDeck` | `CCombatCrew` | 0x6f8 | 甲板布局；另有 20+20 个指针槽 |
| `CCombatCrewScreen` | `CCombatScreen` | 0x210 | 接舷 HUD |
| `CCombatCrewSylphid` | `CSylphid` | 0x20 | 接舷角色动画 |
| `CCombatSingle` | `CView` | 0xc8 | 单挑会话 |
| `CCombatSingleScreen` | `CCombatScreen` | 0x348 | 单挑 HUD |
| `CCombatMesSprite` | `CSurfaceSprite` | 0x148 | 战斗气泡字 |

来源：`class_hierarchy.txt`、`combat/*.cpp`。`CSylphid` 是引擎 2D 角色播放器（广场也有 `CPlazaSylphid`），不是玩法对象。

### 2.2 从海战 tick 切进来

`fn_14000660e`（`0x1401bb5b0`，`exe_caps.md` 写明的战斗结算）RTC：`sCombatCrewResult` 12 字节、`sPlayer` / `sEnemy` 各 24 字节、`cpt`、`pos`、`posPlayerFleet`。

双方船指针都有效之后：

1. 船态 `vfunc+0x18 == 0x26`：走一段特殊结算（`fn_140005272(4/8)` + 写 `cpt`），然后 return
2. 否则若 `fn_140009df4` 为真：按船态 0x22 / 0x23 / 0x24 取系数 50 / 40 / 10，默认 100；再和 `rand(100)` 比较，可能放大伤害并直接打 `vfunc+0x50`（船体）——这是接舷前的碰撞 / 威慑，不是白刃画面
3. 否则 `fn_1400085a3` 失败则整段 return
4. 取两船中点，`fn_1400078a1` 取敌方人物 `sF8`；取不到则 `fn_14000ec6e` 结束战斗
5. **布局选择**（`s98 == 2 && s9C == 0` → `s108 = 1`）：
   - `s108 != 0`：`fn_140004a57` 构造 `CCombatCrewBox`（`0x140081790`）
   - 否则：`fn_14000166d` 构造 `CCombatCrewDeck`（`0x1400818b0`）
6. 另有单挑分支：`fn_14000e0a7` 在栈上构造 `CCombatSingle`（`0x14008ec40`）

`s98` / `s9C` 的语义未恢复。不要把 Box 写成「室内」、Deck 写成「甲板」当确认——类名是 `Box` / `Deck`，文案没有对照表。

`fn_1400096ab`（`0x14008ecf0`）`malloc(0x348)` 后调 `CCombatSingleScreen` ctor，这是打开单挑画面。

### 2.3 白刃会话内存

`CCombatCrew` ctor（`0x14007ecf0`）：

- 一串 HUD 坐标（属 UI，不进逻辑表）
- `+0x3f8 = 0x64`、`+0x3fc = 0x64`（两侧基准 100）
- `+0x4b8 = 0xec`
- 循环 `i = 0..19`：`+0x104 + i*4 = 0xec`，`+0x410 + i*8 = 0`（20 个角色槽）

`0xec = 236`。`NUM_PERSON = 235`（合法 id 0–234），236 是「空槽」哨兵 `[推断]`，与 `INVALID_PERSON = -1` 不是同一个值。重构用 `person.empty`，不要用 -1 和 0 混用（0 是拉斐尔）。

`CCombatCrewDeck` 再在 `+0x4c8`、`+0x568` 清 20 个指针。`CCombatCrewBox` 额外 `+0x4d0 = 2`。20 与 `NUM_MOBILESHIP` 同数，但是否共用上限标 `[未知]`。

### 2.4 白刃一击（运气 × 战力条）

`fn_14000827e`（`0x1400814a0`，与 `CCombatCrew` 同一带）：

```
person = lookup(slot[+0x104])           # fn_140001861
luck   = Ability(person, 5)             # fn_14000e1ce，运气
s28    = luck * *(crew + 0x3f8)         # 己侧条，ctor=100
s2C    = *(crew + 0xfc) * *(crew + 0x3fc)
if *(crew + 0xf8) != arg:
    s2C <<= 3                           # ×8
diff   = |s28 - s2C|                    # fn_140008dff
if diff > rand(70):                     # 0x46
    winner = (s28 > s2C) ? 0 : 1
```

`idiv` 操作数在反编译里被吃掉，乘法本身看得到。`+0xfc` / `+0xf8` 名字 `[未知]`。`rand(70)` 是命中阈，不是 D&D AC。

### 2.5 `500 - a`（船员结算）

同一函数后段（及 `fn_14000d7ab` 舰队近战）用结算属性做放大。`exe_caps.md`：

```
a = Ability(person, index)              # 封在 [0, ability_settled_cap]，原版 500
term = crew_or_stat * (500 - a)
```

`fn_14000660e` 里能读到的立即数：

| 立即数 | 用法 |
| --- | --- |
| `0x1f4 = 500` | `vfunc+0x58 * (500 - Ability(x, 1))`，敏捷进近战放大 |
| `0xc8 = 200` | 另一支 `Ability(*, 5) * (200 - fn_14000ee12)` |
| `0x96 = 150` | 单挑门槛，见下 |
| `0x7d0 = 2000` | 单挑 `rand` |
| `0xa = 10` | 运气差 `rand(10 - d)` |

`fn_14000ee12` 的语义 `[未知]`（疲劳？防御派生？）。属性超过 500 时 `500-a` 为负，原版当无符号除法——这是 bug，不是设计。重构：先把 `a` 饱和到 `ability_ref`，再减。

**禁止**战斗模块直接读六维。原版包把

- `battle_melee_offense` ← 敏捷（index 1）配合 `ability_ref=500`
- `battle_duel` / 白刃击发 ← 运气（index 5）

写进 `ModifierSet`。改六维含义不必改本文件代码。

### 2.6 单挑触发

`fn_14000660e` 在 `s108 != 0` 的 Box 路径上：

```
p = 某人物
x = fn_1400135cf(p, 6)           # 与 Ability 同源包装，第二参是 6
if x <= 150: skip
if rand(2000) > x: skip
# 之后切单挑：fn_14000cf1d(..., 4, ...)，并把敌船标记 4
```

`fn_1400135cf(person, 6)`：六维只有 0–5，6 越界。可能是 1-based 运气，也可能是技能槽。标 `[未知]`，原版包用独立 modifier `battle_duel_score`，vanilla 映射待对照。

另有运气差分支：两侧 `Ability(*, 5)` 相减，再 `rand(10 - d)`，为 0 则进入 `fn_14000e0a7`（构造 `CCombatSingle`）。两边船长 `fn_14001399e` 都 `> 30` 才继续（与海战溃败阈值 30 同源 `[推断]`）。

文案：`我们来单挑决斗！` 等（`mesfile_sc.txt` 约 L907）。`CCombatSingle` ctor 把 `+0xb0 = 0x38`，`+0xa0/+0xa8` 在析构里当两个子对象释放——两侧决斗者 `[推断]`。

`fn_14000e0a7` 只构造、调用 `fn_14000283d`、析构，返回值是单挑结果。`fn_14000283d` 本体未在本任务里展开，单挑伤害表 `[未知]`。

### 2.7 舰队级近战（无画面）

`fn_14000d7ab`（`0x140123d30`）对 `eInvadeFleetShip` / `eTargetFleetShip` / `eAttackerFleetShip` 各 0..4、`eSide` 0..1 做自动近战：

- 先手见 `01-naval-battle.md` §2.7（敏捷）
- `rand(500)` 与一组 `fn_14000e318` / `fn_14000faba` 派生比较，决定是否出手
- 伤害含 `(factor + 500)`；船态 8 则 ×2
- 水手或耐久 ≤ 30 判该侧溃
- 沉船 / 擒获走 `fn_14000ca4a` 文案

这是 NPC 对打和玩家贴舷时的自动结算，不一定弹出 `CCombatCrewScreen`。玩家可见白刃是 `fn_14000660e` 那条。

### 2.8 投降、擒获、占领

文案（`mesfile_sc.txt`）：

- 船：俘虏 / 夺取 / 劫获 / 举白旗 / 占领敌人的 `%s` / `%s号被占领了`
- 人：大量「我们投降吧」提督对白（`0xed47`–`0xefbf`）
- 城：`%s城市投降了`（占城，不是接舷）

`fn_140005fb0`（`0x1401e2e70`，RTC `eCaputureShip` / `asShipLoad` / `apInTowShip`）：

- 槽 0..4，船状态 `== 4` 视为可拖
- 金币 `+= fn_140006b95(ship, 10)`
- 货舱拷进 `asShipLoad`（200 字节 = 5 船 × 5 货格量级 `[推断]`）
- 拖船指针进 `apInTowShip`
- 打开 `CCapturedLoadSprite`（`ship/CCapturedLoadSprite.cpp`）
- 消息 id `0x143`

「从擒获的船上得到%s金币」（`0x3363`）是这条 UI 的文案。战斗交付的是 `gold` + `cargo slots` + `ship_id`，不是改市价。

有没有「把敌方水手写进 98 人名簿」的代码，本批 cpp 没读到。职文只强调冲锋队长加肉搏、水兵室最多 4。俘虏船员标 `[未知]`，接口先留 `CrewCaptured`，人物系统可以 no-op。

### 2.9 岗位与改装对近战的加成

只作为查询，不在本系统改船舱：

| 来源 | 效果（文案） |
| --- | --- |
| 冲锋队长 | 提高肉搏攻击 |
| 水兵室 ×≤4 | 水手容量 |
| 撞角 | 允许碰撞进攻，切接舷 |
| 项羽头盔等道具 | 「提高本队士气」——人物 modifier，不是接舷条 |

### 2.10 演出

`CCombatCrewSylphid` dtor 里有引用计数 `+0x18/+0x1c`，播完调 `fn_140004539`。逻辑层只发 `PlayBoardingAnim { clip_id, person_id }`。`CDamageDisplay` / `CDamageAnimeSprite` 是数字弹出，体积 0x10 / 0x100，无公式。

## 3. 实体与 ID

| 实体 | ID | 原版 | 空值 |
| --- | --- | --- | --- |
| 接舷布局 | `boarding_layout_id` | Box / Deck 两套 ctor | |
| 单挑会话 | `duel_id` | 运行时 | |
| 白刃槽 | `boarding_slot` | 20 | `person.empty`（原版 0xec） |
| 擒获槽 | `capture_ship_id` | 5 | 与 `fleet_ship_id` 同宽 |
| 战果 | `battle_result_id` | 5 | `INVALID_BATTLERESULT` |

`eCloseCombatTypeToIndex` 出现在 `fn_14001478b`（`0x1401a8e40`），但该函数在加载 `ship.dk4` 贴图，是近战船图索引，不是规则枚举。不要当成第三种白刃。

## 4. 算法 / 公式

### 4.1 白刃一击

```
power_self  = modifiers.battle_luck(person_id) * side_gauge_self
power_foe   = slot_weight_foe * side_gauge_foe
if layout_flag: power_foe *= gauge_flag_mul          # vanilla 8
delta = abs(power_self - power_foe)
if delta > rand(hit_threshold):
    winner = self if power_self > power_foe else foe
```

```yaml
# formulas/combat.yaml  boarding:
side_gauge_init: 100
hit_threshold: 70
gauge_flag_mul: 8
empty_person_sentinel: 236       # 原版 0xec；重构用 person.empty
boarding_slots: 20
```

`battle_luck` 原版包 ← 运气。`idiv` 未还原前不要再乘一层「等级」。

### 4.2 船员结算放大（原版 `500-a`）

```
a = saturate(modifiers.battle_melee_offense(person_id), 0, ability_ref)
term = crew_count * (ability_ref - a)
# 写入对方水手或用于投降判定，具体写回点见 fn_14000660e 调用图
```

```yaml
ability_ref: 500                 # vanilla；与 caps.ability_settled_cap 对齐
ability_ref_alt: 200             # 0xc8 那一支
melee_ability_index_vanilla: 1   # 敏捷；只写在原版映射，不写进引擎
luck_ability_index_vanilla: 5
```

`a > ability_ref` 时饱和，禁止无符号回绕。

### 4.3 单挑是否开

```
score = modifiers.battle_duel_score(person_id)
if score <= duel_stat_min: return false
if rand(duel_roll) > score: return false
if min(captain_hp(self), captain_hp(foe)) <= duel_min_alive: return false
return true
```

```yaml
duel_stat_min: 150
duel_roll: 2000
duel_luck_spread: 10
duel_min_alive: 30
```

单挑内部伤害 `[未知]`：实现先做成「调用 `Duel.resolve(modifiers_a, modifiers_b) -> side`」，vanilla 函数体对照 `fn_14000283d` 再填。

### 4.4 投降 / 擒获

```
if crew <= 0 or hull <= 0: mark_ship(state_captured_or_sunk)
if captured:
    gold += loot_gold(ship_id)          # fn_140006b95(..., 10)
    emit ShipCaptured { ship_id, cargo, gold }
```

船态 4 = 可拖 `[推断，来自 fn_140005fb0 cmp == 4]`。名字未恢复前用整数状态机，状态表进 `combat/boarding.yaml`。

## 5. 耦合分析

| 原版纠缠 | 重构 |
| --- | --- |
| `Ability(person, 1/5)`、`fn_1400135cf(person, 6)` | `ModifierSet` |
| `500-a` 与属性结算上限 | `ability_ref` + 饱和 |
| 船 `vfunc+0x50/0x58/0x48` | 会话快照 `hull/crew/state` |
| `COceanScene` 直接 new 接舷 View | Ocean 发 `BoardingRequested`；UI 订 `BoardingOpened` |
| `CCapturedLoadSprite` 在 ship 功能目录 | 战斗发事件，货舱 UI 属 ships/ui |
| 98 人名簿 | 本系统不写；最多发 `CrewCaptured { person_id }` |
| Sylphid / 坐标 | `ui/` |

接舷不得调用 `Economy.quote` / 写 `market_price`。

## 6. 解耦后的配置

### `config/combat/boarding.yaml`

```yaml
schema: dk4.combat.boarding
version: 1
layouts:
  - id: 0
    key: box
    ctor: CCombatCrewBox
    extra: { field_4d0: 2 }
  - id: 1
    key: deck
    ctor: CCombatCrewDeck
slots: 20
side_gauge_init: 100
empty_person: person.empty
capture_slots: 5
ship_state_towable: 4              # [推断]
duel:
  screen: CCombatSingleScreen
  session: CCombatSingle
```

### `config/formulas/combat.yaml`（接舷段）

```yaml
boarding:
  ability_ref: 500
  ability_ref_alt: 200
  hit_threshold: 70
  gauge_flag_mul: 8
  duel_stat_min: 150
  duel_roll: 2000
  duel_luck_spread: 10
  duel_min_alive: 30
vanilla_modifier_map:
  battle_melee_offense: { ability: agility }
  battle_luck: { ability: luck }
  battle_duel_score: { source: fn_1400135cf_arg6, unknown: true }
```

`vanilla_modifier_map` 只存在于原版内容包，引擎不 if 能力名。

## 7. 独立旋钮

- `slots`、`side_gauge_init`、`hit_threshold`、`gauge_flag_mul`
- `ability_ref`（与 `caps.ability_settled_cap` 一起改）
- 是否允许单挑、`duel_roll`
- 擒获是否自动拖船、货舱是否可分到已有五艘以外

加一种白刃布局：只加 `layouts[]` 和 UI 场景，不改炮击。

## 8. 对外接口

```
Combat.request_boarding(battle_id, from_ship, to_ship, layout_id)
Boarding.tick(boarding_id) -> { winner_slot | continue | duel }
Duel.resolve(person_id_a, person_id_b) -> side
```

事件：

```
BoardingOpened    { battle_id, layout_id, side_a: [person_id], side_b: [person_id] }
BoardingHit       { boarding_id, slot, damage_or_flag }
DuelOpened        { battle_id, person_id_a, person_id_b }
DuelResolved      { battle_id, winner_person_id }
ShipCaptured      { battle_id, ship_id, gold, cargo: [{trading_item_id, qty}] }
CrewCaptured      { battle_id, person_id }          # 人物系统可忽略
BoardingResolved  { battle_id, result_id }          # 再汇总进 BattleResolved
```

查询：`Characters.modifiers`、`Ships.has_add_part(ram)`、`Ships.crew(ship_id)`。

## 9. 未核实字段

| 项 | 事实 | 不要写成 |
| --- | --- | --- |
| Box vs Deck 的玩法差 | 两套 ctor；`s98==2 && s9C==0` 选 Box | 「舱内战 / 甲板战」定论 |
| `CCombatCrew +0xf8,+0xfc,+0x100` | 参与 ×8 分支 | 士气 |
| `fn_1400135cf(..., 6)` | 单挑门槛 | 第六维或「剑术」 |
| `fn_14000ee12` | 从 200 里减 | 疲劳 |
| `fn_14000283d` 单挑公式 | 只见调用 | 回合制伤害表 |
| `eCloseCombatTypeToIndex` | 加载 `ship.dk4` | 第三种近战 |
| 俘虏水手入队 | 无代码 | 自动招人 |
| 20 槽是否等于 20 个 mobile ship | 同数 | 同一上限 |
| 船态 0x22/23/24/26 与 4/7/8/0xb | 多处立即数 | 随意命名 |
| `sCombatCrewResult` 12 字节 | RTC 大小 | 三场比分 |

`fn_14000660e` 全长 0x11ab，大量脏寄存器。已确认的是调用图、`500-a`、运气乘 100 条、Box/Deck/Single 三套画面。其余算术标 `[未知]`，对着 `0x1401bb5b0` / `0x1400814a0` 补，不从别的游戏抄白刃表。
