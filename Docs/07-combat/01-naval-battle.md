# 海战（炮击、机动、船体）

原版把「海战画面」和「海战模拟」拆在两处：画面是 `CCombatScreen` 家族，模拟在 `COceanScene` 的战斗 tick 里。重构后这一篇只定义**舰队炮击战**的规则与表；接舷/单挑见 `02-boarding.md`，NPC 选目标与 happening 见 `03-battle-ai.md`。

## 1. 职责边界

本系统管：

- 进入 / 离开海战会话（双方舰队、海域坐标、风向只读）
- 每艘参战船的机动、朝向、炮击弧、炮弹飞行与命中
- 船体耐久与水手人数在炮击下的增减
- 港口武装炮台对舰队开火、舰队炮击港口（只发事件，不改市价）
- 海战结束时发出 `BattleResolved`（战利品、被擒船、是否占港只作为载荷）

本系统不管：

- 接舷画面、甲板/箱庭白刃、单挑决斗（`02-boarding.md`）
- NPC 选哪艘船、用哪套战术、随机 happening 表（`03-battle-ai.md`）
- 造船厂改炮种、船首炮/船尾炮/撞角的安装（`ships/`，本系统只读 `gun_type_id` / `add_part_id`）
- 市价、特产品、投资（禁止战斗写经济表）
- 经验、名声、技能成长（`progression` 订阅 `BattleResolved` 再算）
- 文案、像素坐标、炮弹动画（`ui/`）

进入战斗前的「警戒 / 放哨 / 发现舰队」属于航海遭遇（`ocean/encounters.yaml`），本系统只接收已经成立的战斗会话。

## 2. 原版事实

### 2.1 三套战斗画面，不是一种战斗

| 画面类 | 基类 | 用途 | 来源 |
| --- | --- | --- | --- |
| `CCombatScreen` | `CSurfaceSprite` | 海战 HUD 基类，ctor 把表面写成 0x780×0x438（1920×1080） | `combat/CCombatScreen.cpp` `0x140085d10` |
| `CCombatCrewScreen` | `CCombatScreen` | 接舷 / 白刃 | `CCombatCrewScreen.cpp`，体积 0x210 |
| `CCombatSingleScreen` | `CCombatScreen` | 提督单挑 | `CCombatSingleScreen.cpp`，体积 0x348 |

`class_hierarchy.txt`：`CCombatCrew` / `CCombatCrewDeck` / `CCombatCrewBox` 走 `CView`，不是 `CCombatScreen`。海战模拟**不**在这些 View 的 vfunc 里。

### 2.2 模拟在海洋场景 tick

`COceanScene` 在 `fn_140006c12 == 1`（战斗中）时顺序调用：

1. `fn_14000660e`（`0x1401bb5b0`）— 接舷/船员结算，RTC 名 `sCombatCrewResult` / `sPlayer` / `sEnemy`
2. `fn_14000cdc9`（`0x1401bcbd0`）— 按船位扫命令 / happening
3. `fn_14000ee8a`（`0x1401c3210`）— 火炮与炮弹

来源：`ocean/COceanScene.cpp` 约 `0x1401a63a8`。任一函数认为战斗已结束就会提前 return，后面的炮/弹不再 tick。

非战斗分支里 `fn_14000cd8d`（RTC `sChangeBattle`）会让 `eMobileFleet` 1..3 切入战斗。玩家舰队是 mobile fleet 0 的推断见 `03-battle-ai.md`。

### 2.3 一侧最多五艘

| 符号 | 值 | 来源 |
| --- | --- | --- |
| `NUM_FLEETSHIP` | 5 | `FleetShipID.h` / `num_values.json` |
| `NUM_ATTACKERFLEETSHIP` | 5 | RTC，`fn_14000d7ab` |
| `NUM_INVADEFLEETSHIP` | 5 | 同上 |
| `NUM_TARGETFLEETSHIP` | 5 | 同上 |
| `eSide` | 循环上界 2 | `fn_14000d7ab` |

`COceanScene` ctor 在 `+0xf8` 清 5 个指针。`eCaputureShip` 同样 `>= 5` 结束（擒获槽）。

`CBattleShip` 继承 `CShipObject → CShipBase`，体积约 0x20，save 类型字节 `+0x8 == 0xd0`。`vfunc_14` / `vfunc_16` 在船体或水手计数掉到 ≤0 且船态为 7 或 0xb 时把对应计数抬回 1（防 0 夹死）。船态枚举本身未恢复。

### 2.4 炮种 6、炮槽 5、炮弹 100

| 符号 | 值 | 对象 | 来源 |
| --- | --- | --- | --- |
| `eGunType` / `NUM_GUNTYPE` | 6 | 造船厂可选炮种 | `GunTypeID.h` RTC `cmp < 6` |
| `eBattleGun` / `NUM_BATTLEGUN` | 5 | 战斗火炮槽 | `BattleGunID.h` |
| `eShell` / `NUM_SHELL` | 100 | 炮弹对象 | `ShellID.h`；`fn_14000ee8a` 循环 `eShell >= 0x64` |

`CBattleGun` / `CShell` 都是 `CSaveObject`。`CShellInfo` 不是存档对象，ctor 把 `+0xc` 写成 `0xffffffff`（空哨兵）。

战斗场景里 `eBattleGun` 还出现过 `>= 0x15`（21）和 `>= 20` 的比较（`num_from_rtc.json`）。21 ≠ 5，不能把「每船五门炮」写成确认。较干净的读法：5 是每舰队船位，21 是场景实例池（4 支 `NUM_MOBILEFLEET` × 5 船 + 1 门城炮）——标 `[推断]`。

### 2.5 炮种解锁看港口军事发展度

`fn_140003bb1`（`0x1400a3290`，RTC `eGunType` / `anNecessaryDevelopment`，字符串 `InfoBox.dk4`）给 6 个炮种写死发展度门槛：

| `gun_type_id` | `anNecessaryDevelopment` | 立即数 |
| --- | --- | --- |
| 0 | 1 | |
| 1 | 1 | |
| 2 | 2000 | `0x7d0` |
| 3 | 3500 | `0xdac` |
| 4 | 5000 | `0x1388` |
| 5 | 7000 | `0x1b58` |

当前船的「必要发展」来自 `fn_14000bc03`。门槛本身属于 `ships/guns.yaml`，战斗只读已装炮种。六种炮的中文名在 `mesfile_sc.txt` 里没有单独条目，禁止按「卡隆 / 加农」去猜。

### 2.6 船首炮、船尾炮、撞角、炮手室

`mesfile_sc.txt` `0xc43f`–`0xc503`：

- 船首炮：可向前打，威力小
- 船尾炮：可向后打，命中率低
- 撞角：撞敌舰侧面，进入碰撞 / 肉搏
- 冲锋队长：提高肉搏攻击；水兵室最多 4
- 炮手：缩短炮击间隔；炮手室最多 4

这些是改装件，不是 `eGunType`。瞄准函数用 `fn_1400091e2(ship, 6)` / `(ship, 7)` 判断能否打船首弧 / 船尾弧（`fn_14000c8c4`）。6/7 是否等于 `eAddParts` 下标标 `[推断]`。

### 2.7 先手：双方舰长敏捷

`fn_14000d7ab`（`0x140123d30`，紧挨 `CBattleShip`）：

```
a_invade = Ability(invade_captain, 1)   # fn_14000e1ce，敏捷
a_target = Ability(target_captain, 1)
s234 = (a_invade >= a_target) ? 1 : 0   # 1 = 进攻侧先手
```

`eAbility` 顺序是体力、敏捷、魅力、智力、精神、运气（`AbilityID.h` 注释）。重构后战斗不得再读六维原值，改读 `ModifierSet.battle_initiative`。

### 2.8 射程与射界

`fn_14000c8c4`（`0x1401bd330`，RTC `cTergetAngle` / `vecTerget` / `posNow`）：

1. 取本船位置与目标向量，距离 `s74`
2. `s80 = fn_140014c7c(ship, 9, 6)` — 与炮术/射程相关的派生值，语义 `[未知]`
3. `s80 <= 100` 时把有效半径减 12；否则把 `s80` 加进半径
4. **最小距离** `0x384 = 900`：更近则 return（对得上「靠得太近炮就无法射击了」`0x17f07`）
5. **最大距离** `s74 <= s7C * s7C`，否则 return
6. 相对角 `< 0x20` 或 `> 0xe0`：必须有船首炮改装
7. 相对角在 `(0x60, 0xa0)`：必须有船尾炮改装
8. 其余角度：乘 `fn_14000bb5e`（舷侧门数 / 火力，`[未知]`）
9. 再 `rand(3)` 做一次平均，调用 `fn_1400052e0` 生成炮弹

角度单位是原版朝向字节（0–255 一圈）`[推断]`，不要换成度数写进代码常数。

### 2.9 炮弹命中（船体 / 水手 / 城市）

`fn_14000ee8a` 先扫火炮实例，再扫 `eShell 0..99`：

- 每发活弹在距离上限 `0x240 = 576` 内找最近目标
- 目标可以是城镇（局部名 `posTown`）或 `eMobileShip 0..19`
- 打到船上：用 `ship->vfunc+0x50` 当船体耐久，经 `fn_140012b34(shell)` 与 `fn_140011bd0` 合成后 `fn_14000c202` 写回。耐久到 0 走沉没 `fn_14000c8dd`
- 水手：`vfunc+0x58` 减 1 后再掺 `rand(6)`，`fn_140014943` 写回
- 某派生值 `>= 100` 时走投降 / 擒获检查（`fn_14000118b` / `fn_140014bd2`）
- 打到城镇：`fn_140004f84(town, damage)`，不碰市价

`vfunc+0x50` = 耐久、`vfunc+0x58` = 水手人数是从「沉了 / 一个水兵都没有」（`0x1421f`）和 `SHIP_DURABILITY_*` / `SHIP_CREW_*` 符号对上的 `[推断]`。寄存器里 `idiv` 的除数经常丢了，**禁止**把这里补成完整伤害表。

旗舰耐久 **或** 水手 `<= 0x1e`（30）则该侧判定溃败（`fn_14000d7ab` 尾部）。

船态 `vfunc+0x48 == 8` 时近战伤害翻倍（`fn_14000d7ab`、`fn_14000660e` 都有）。8 的名字 `[未知]`，不要写成「撞角状态」除非改装查询也对上。

### 2.10 弹药

没有名为 ammo / powder / 炮弹数的 RTC 局部。`CBattleGun` 存档只序列化 `+0x8`、`+0xa` 两个 uint16（`vfunc_06/07`）。可能是炮种 + 冷却，也可能含弹药。文案有「装上炮弹」，不能当成独立弹药池。标 `[未知]`。

炮击间隔由炮手舱缩短（文案），具体 tick 数未从 cpp 读出。

### 2.11 士气

战斗 RTC / 头文件里没有 morale 枚举。`mesfile` 的「士气」出现在大厨、项羽头盔、舰长职文，是人物/道具效果，不是海战条。水手「不满 / 疲劳」是航海状态，开战前由 ocean 结算。海战过程**不要**另做一条士气条，除非以后从存档位恢复。

### 2.12 战果与文案

| 符号 | 值 | 来源 |
| --- | --- | --- |
| `eBattleResult` / `NUM_BATTLERESULT` | 5 | `BattleResultID.h`；多处 `>= 5` |
| `BATTLERESULTSTATE_BITS` | 符号存在 | `constants.json` |
| `CBattleResult` | 5 个连续 byte（`+8..+c`） | `vfunc_06/07` |

五个取值的名字未恢复。文案侧能见到的结局包括：胜利、逃走、沉没、擒获、占领、投降、城市投降、战利品金币（`mesfile_sc.txt` `0x13a93`–`0x13cbb`、`0xf1eb`、`0xf4af`）。`CForceElement::vfunc_04` 按势力扫 5 个 `CBattleResult`（NPC 月结，见 `03-battle-ai.md`）。

金币战利品：`从擒获的船上得到%s金币`（`0x3363`）、`获得了%s枚金币的战利品`（`0x13bfb`）。`fn_140005fb0` 对状态==4 的船累加 `fn_140006b95(ship, 10)`，再打开 `CCapturedLoadSprite` 处理货舱。战斗产出的是物品/金币/船 ID，不是价格。

### 2.13 港口炮击与占领

文案：

- `%s城正在炮击%s舰队`（`0xac7`）
- `%s城市炮击我们` / `向我们开炮`（`0x13af3`）
- `开始攻击%s城市`（`0x13cbb`）
- `%s城市投降了`（`0x13a93`）；职文「要想让城市投降，需要舰长和副官」（`0x18463`）
- 武装炮台增减要钱（`0x32b7`）；战斗中炮台火药着火（`0xf1eb`）

占领港是势力状态，不是海战写 `town.market_price`。海战只许发 `town_occupied`。

### 2.14 `500-a` 不是炮击公式

`srcdata/docs/exe_caps.md`：战斗结算 `fn_14000660e` 使用 `500 - a`，`a` 来自 `fn_14000e1ce`，原版假设结算属性 ≤ 500（`ability_settled_cap`）。这条公式用在接舷/船员结算和舰队近战，**不是**炮弹打船体的主路径。原版包把 `500` 写成公式参数 `ability_ref`，见 `02-boarding.md`。

## 3. 实体与 ID

| 实体 | 稳定 ID | 原版计数 | 空值 | 运行时 |
| --- | --- | --- | --- | --- |
| 炮种 | `gun_type_id` | 6 | `INVALID_GUNTYPE=-1` | 表长 |
| 战斗炮槽 | `battle_gun_id` | 5 | `INVALID_BATTLEGUN=-1` | 表长 |
| 炮弹实例 | `shell_id` | 100 | `INVALID_SHELL=-1` | 池长 |
| 战果槽 | `battle_result_id` | 5 | `INVALID_BATTLERESULT=-1` | 表长 |
| 参战船位 | `fleet_ship_id` | 5 / 侧 | `INVALID_FLEETSHIP=-1` | `caps.fleet_ships` |
| 场景船 | `mobile_ship_id` | 20 | `INVALID_MOBILESHIP=-1` | `caps.mobile_ships` |
| 场景舰队 | `mobile_fleet_id` | 4 | | `caps.mobile_fleets` |
| 海战会话 | `battle_id` | 运行时 | | 不进原版枚举 |

原版兼容包：`fleet_ships: 5`、`sides: 2`。引擎按表长循环，禁止写死 `for i in 0..4`。

参战船在会话里只持：`fleet_id`、`fleet_ship_id`、`ship_type_id`、`gun_type_id`、当前耐久、当前水手、朝向、坐标。不持 `Player*`、不持 `World*`。

## 4. 算法 / 公式

全部立即数进 `config/formulas/combat.yaml`。代码只调纯函数。人物侧只消费 `ModifierSet`。

### 4.1 先手

```
initiative(side) = modifier.battle_initiative(flagship_captain_id)
first_side = invade if initiative(invade) >= initiative(target) else target
```

原版包：`battle_initiative` 映射自敏捷（ability index 1）。

### 4.2 炮击许可

```
can_fire(shooter, target):
  dist = distance2(shooter.pos, target.pos)
  if dist < min_cannon_range: return false          # vanilla 900
  radius = aim_radius(shooter)                      # 含炮术派生 s80
  if dist > radius * radius: return false
  bearing = relative_heading(shooter, target)       # 0..255
  if bearing in bow_arc:    return has_add_part(shooter, bow_gun)
  if bearing in stern_arc:  return has_add_part(shooter, stern_gun)
  return true                                       # 舷侧
```

原版包：

```yaml
min_cannon_range: 900
bow_arc: { lt: 32, gt: 224 }      # 0x20 / 0xe0
stern_arc: { gt: 96, lt: 160 }    # 0x60 / 0xa0
aim_skill_cut: 100                # s80 <= 100 时半径 -12
aim_skill_penalty: 12
shell_acquire_limit: 576          # 0x240
```

`aim_radius` 的闭式 `[未知]`，实现时按 `fn_14000c8c4` 的调用图还原，不要编一张「每炮种射程格」。

### 4.3 炮弹命中（命名、不编表）

```
hull' = apply_hull_hit(hull, shell_power, modifiers)   # fn_140012b34 + fn_140011bd0 + vfunc+0x50
crew' = apply_crew_splash(crew, shell_power, rand)     # vfunc+0x58, rand(crew_splash_sides)
if hull' <= 0: emit ShipSunk
if hull' <= defeat_hull_or_crew or crew' <= defeat_hull_or_crew:
  if is_flagship: emit SideBroken
```

```yaml
defeat_hull_or_crew: 30          # 0x1e
crew_splash_sides: 6             # rand(6)
melee_double_ship_state: 8       # vfunc+0x48；语义未知
```

`shell_power` 从 `CShell` 存档字段来，六 dword + 三 word 的名字见第 9 节。未还原前按不透明 `ShellSnapshot` 传递。

### 4.4 城炮 / 攻城

同一套炮弹池。目标 `town_id` 时调用城镇驻军接口 `Town.receive_bombardment(town_id, shell_power)`（城镇系统实现），战斗只传 ID 与数值。城市开火方向相反：`Town.try_fire(town_id) -> optional ShellRequest`。

### 4.5 结束

```
if no_ships_able_to_fight(side): resolve(opposite)
if flagship_fled(side): resolve(flee)
# 接舷导致的投降 / 擒获由 boarding 回调 Combat.apply_capture
```

战利品、拖船、货舱转移在结算阶段用 `CaptureSession` 做，不在炮弹 tick 里写货格。

## 5. 耦合分析

原版硬编码依赖：

| 依赖 | 表现 | 重构 |
| --- | --- | --- |
| 人物六维 | `fn_14000e1ce(person, 1/5)` 直接进先手和近战 | `ModifierSet` |
| 结算属性上限 500 | `500-a` 在属性 >500 时翻转 | `ability_ref` 参数 + 饱和 |
| 船 vfunc 表 | `+0x48` 态、`+0x50` 耐久、`+0x58` 水手 | 战斗会话快照字段 |
| 造船改装 | 炮种、船首/尾炮、撞角、炮手室 | 只读 `ships` 查询 |
| 海洋场景 | 战斗 tick 塞在 `COceanScene` | `Ocean` 发 `BattleTick`，战斗自己跑 |
| 城镇发展度 | 解锁炮种、城防开火 | `town.military` 只读 |
| 势力 | `CForceElement` 月结扫 `CBattleResult` | 事件，不回写势力表结构 |
| 文案 / 像素 | 屏幕 ctor 一堆坐标 | `ui/scenes.yaml` |
| 存档 | `CBattleGun` `CShell` `CBattleResult` `CBattleShip` 都是 `CSaveObject` | 会话态进存档 schema，定义不进存档 |

禁止：combat → economy 写价格；combat → 人物写经验；UI 写存档。

## 6. 解耦后的配置

### `config/formulas/combat.yaml`

```yaml
schema: dk4.formulas.combat
version: 1
naval:
  ability_ref: 500                 # 近战用，炮击不用；见 boarding
  min_cannon_range: 900
  shell_acquire_limit: 576
  defeat_hull_or_crew: 30
  aim_skill_cut: 100
  aim_skill_penalty: 12
  bow_arc_lt: 32
  bow_arc_gt: 224
  stern_arc_gt: 96
  stern_arc_lt: 160
  heading_mod: 256                 # 朝向一圈
  crew_splash_sides: 6
  melee_double_ship_state: 8
```

### `config/combat/naval.yaml`

```yaml
schema: dk4.combat.naval
version: 1
sides: 2
ships_per_side: 5                  # 原版包 = caps.fleet_ships
scene_gun_pool: 21                 # 原版场景循环；运行时按表
shell_pool: 100
mobile_ship_cap: 20
commands:                          # 玩家海战指令，名字未恢复，只留 id
  - { id: 2, key: cmd_2 }
  - { id: 3, key: cmd_3 }
  - { id: 4, key: cmd_4 }
  - { id: 5, key: cmd_5 }
  - { id: 6, key: cmd_6 }
add_parts_used_in_aiming:
  bow_gun_query: 6                 # fn_1400091e2 第二参，[推断]
  stern_gun_query: 7
outcomes_ui_keys:                  # 只引用文案 key，不写中文进逻辑
  - battle.town_surrendered
  - battle.town_fires
  - battle.ship_sunk
  - battle.ship_captured
  - battle.loot_gold
```

### `config/ships/guns.yaml`（战斗只读）

```yaml
schema: dk4.ships.guns
version: 1
types:
  - { id: 0, key: gun_0, military_unlock: 1 }
  - { id: 1, key: gun_1, military_unlock: 1 }
  - { id: 2, key: gun_2, military_unlock: 2000 }
  - { id: 3, key: gun_3, military_unlock: 3500 }
  - { id: 4, key: gun_4, military_unlock: 5000 }
  - { id: 5, key: gun_5, military_unlock: 7000 }
```

`name_i18n` 等文案抽出后再填。不要在本表写伤害骰。

### `config/ships/shells.yaml`

炮弹**定义**（若与 100 实例池分离）和战斗池分开。原版 `NUM_SHELL=100` 更像实例池（循环 + `CSaveObject`），类型数可能更少（RTC 另有 `eShell >= 4`）。未分开前，池长进 `caps.yaml`，单发字段进不透明 blob。

### `config/caps.yaml` 片段

```yaml
fleet_ships: 5
gun_types: 6
battle_guns: 5
shell_pool: 100
mobile_ships: 20
mobile_fleets: 4
ability_settled_cap: 500
ship_crew: { min: SHIP_CREW_MIN, max: SHIP_CREW_MAX }
ship_durability: { min: SHIP_DURABILITY_MIN, max: SHIP_DURABILITY_MAX }
```

MIN/MAX 整数以 RTC / 断言补齐，文档保持符号名。

## 7. 独立旋钮

改这些值不应迫使贸易、人物、城镇改代码：

- `ships_per_side` / `shell_pool` / `scene_gun_pool`
- `min_cannon_range`、射界字节、`defeat_hull_or_crew`
- 六炮种的 `military_unlock`
- `ability_ref`（近战，见 boarding）
- 是否允许攻城、城炮是否与舰炮同一炮弹池

加新炮种：只追加 `ships/guns.yaml` 行，战斗按表长枚举。显示宽度由 UI 自己处理。

## 8. 对外接口

查询（只读）：

```
Combat.can_engage(fleet_id_a, fleet_id_b) -> bool
Combat.snapshot(battle_id) -> BattleSnapshot      # ID 与数值，无指针
Ships.gun(ship_id) -> GunTypeRecord
Characters.modifiers(person_id) -> ModifierSet    # battle_initiative, battle_gunnery, ...
Town.military(town_id) -> int
```

命令：

```
Combat.start(BattleStart { attacker_fleet_id, defender_fleet_id, sea_area_id, optional town_id })
Combat.issue(battle_id, fleet_ship_id, command_id, target_id)
Combat.cancel(battle_id, reason)                  # 逃走
```

事件（载荷只有 ID 与值）：

```
BattleStarted     { battle_id, attacker_fleet_id, defender_fleet_id, sea_area_id }
ShipHullChanged   { battle_id, fleet_id, fleet_ship_id, hull }
ShipCrewChanged   { battle_id, fleet_id, fleet_ship_id, crew }
ShipSunk          { battle_id, fleet_id, fleet_ship_id }
TownBombarded     { battle_id, town_id, damage }
BattleResolved    {
  battle_id,
  result_id,                 # 0..n-1，名字未恢复前用整数
  loot: [{ item_id | gold, qty }],
  captured_ships: [ship_id],
  town_occupied: town_id | null
}
```

`BattleResolved` 由 economy / factions / progression / town-life 订阅。战斗模块发完即丢会话，不写市场。

## 9. 未核实字段

| 位置 | 现状 | 禁止的做法 |
| --- | --- | --- |
| `CBattleGun +0x8,+0xa` 两 uint16 | 存档有，无名字 | 写成 ammo / 炮种而不标推断 |
| `CBattleGun` 5 vs 场景循环 21 | 两套上界 | 断言「每船五门」 |
| `CShell +0xc..+0x28` 六 dword + 三 word | 存档布局已知 | 编成 x/y/vx/vy/damage 表 |
| RTC `eShell >= 4` | 与 100 并存 | 把 100 当炮种数 |
| `vfunc+0x48` 船态 7 / 8 / 0xa / 0xb | 多处分支 | 用 D&D 状态名 |
| `fn_140014c7c(ship, 9, 6)` | 进射程 | 叫「炮术技能」当确认 |
| `fn_14000bb5e` 舷侧乘数 | 只知相乘 | 编门数表 |
| 六个 `gun_type` 的名称与伤害 | 只有解锁门槛 | 写成卡隆/加农/臼炮 |
| `eBattleResult` 五值 | 计数 5，有 `BATTLERESULTSTATE_BITS` | 写成 win/lose/draw/flee/capture 定论 |
| 弹药、冷却、炮击间隔 tick | 文案有间隔，代码未还原 | 做独立弹药仓当原版行为 |
| 海战士气条 | 未找到 | 从航海不满抄一条进战斗 |
| `CBattleShip +0x10,+0x14` | 低值抬回 1 | 当最大耐久 |
| 炮弹池是否进长期存档 | 类是 `CSaveObject` | 假设战役之间还留着 100 发弹 |

炮击伤害在 `fn_14000ee8a` / `fn_14000c8c4` 的调用图已定位；`idiv` 操作数在反编译里经常是脏寄存器。还原公式时对着这两处下断点，不要从别的 SRPG 抄伤害表。
