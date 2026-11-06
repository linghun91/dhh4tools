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

| 符号 | 值 | 证据类型 / 来源 |
| --- | --- | --- |
| `NUM_FLEETSHIP` | 5 | 循环上界：`fn_14000cdc9`、`CChangeFormationView` ctor `eFleetShip` 0–4（头文件值本身是投票） |
| `NUM_ATTACKERFLEETSHIP` | 5 | 循环上界：`fn_14000d7ab` `cmp …, 5` |
| `NUM_INVADEFLEETSHIP` | 5 | 同上 |
| `NUM_TARGETFLEETSHIP` | 5 | 同上 |
| `eSide` | 2 | 循环上界：`fn_14000d7ab` |

`COceanScene` ctor 在 `+0xf8` 清 5 个指针。`eCaputureShip` 同样 `>= 5` 结束（擒获槽）。

`CBattleShip` 继承 `CShipObject → CShipBase`，体积约 0x20，save 类型字节 `+0x8 == 0xd0`。`[确认]` 无画面结算 `fn_1400094d0` 用 `fn_1400017df(this, 0x20, 5, ...)` 每侧建 5 个 0x20 槽：`+0x8` 船 id（`0xd0` = 空，跳过）、`+0xc` = `fn_140004610(ship)/8`、`+0x10` 耐久、`+0x14` 水手、`+0x18` 退场标记。`vfunc_14`（`0x140126e50`）写耐久后若 ≤0 且 `vfunc+0x48` 为 7 或 0xb 则抬回 1；`vfunc_16`（`0x140126dd0`）写水手后若 ≤0 且 `vfunc+0x48 == 0xb` 则抬回 1 `[确认]`。`vfunc+0x48` 是**船首像**（§2.9），不是船态：7 = 魔鬼像、0xb = 提督像 `[推断，强]`（`05-ships-fleet/04`）。重构为修正器 `combat.hull_floor_at_zero` / `combat.crew_floor_at_zero`。

### 2.4 炮种 6、场景炮控制器 21、炮弹 100

| 符号 | 值 | 对象 | 来源 |
| --- | --- | --- | --- |
| `eGunType` / `NUM_GUNTYPE` | 6（判界：`fn_140012760` `id >= 6` 返回空） | 造船厂可选炮种 | `GunTypeID.h`；body `0x140123300` |
| `eBattleGun` / `NUM_BATTLEGUN` | **21**（循环上界 `0x15`；`BattleGunID.h` 的 5 是 RTC 立即数投票误采） | 场景炮控制器池下标 | `fn_14000ee8a` `0x1401c3248`；池构造 `fn_140007dce(..., 0x18, 0x15)` |
| `eShell` / `NUM_SHELL` | 100（循环上界 `0x64`） | 炮弹对象 | `ShellID.h`；`fn_14000ee8a` 循环 `eShell >= 0x64` |

`CBattleGun` / `CShell` 都是 `CSaveObject`。`CShellInfo` 不是存档对象，ctor 把 `+0xc` 写成 `0xffffffff`（空哨兵）。

`num_from_rtc.json` 对 `eBattleGun` 记的 5×3 是同函数里其它立即数（`mov edx,5` 等），不是比较上界；`>= 20` 是同函数 `eMobileShip` 的上界。「每船五门炮 / 每舰队 5 炮位」作废（与 `05-ships-fleet/08` 一致）。

`[确认]` 21 是场景炮控制器池：`fn_14000ee8a` 第一段循环 `cmp [eBattleGun], 0x15`（`0x1401c3248`）。`fn_14000e6ba(gun)`（`0x14014a050`）取该 gun 在池里的下标，`< 0x14` 原样返回、否则返回 `0x15`；`fn_14000bce9(gun)` 再用这个下标取 `eMobileShip`。即 **gun i ↔ 移动船 i（0..19），gun 20 = 城炮（无船）**。城炮分支见 §2.10。每个控制器只代表「这艘船的整舷火力」，不是一门炮。重构：池长 = `caps.mobile_ships + 场景城数`（原版 20 + 1），不读头文件的 5。

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

当前船的「必要发展」来自 `fn_14000bc03`。门槛本身属于 `ships/guns.yaml`，战斗只读已装炮种。

`[确认]` 炮种名与威力不在 mesfile，在 exe `.data` 表 `0x140682e10`（`fn_140012760`，19 字节/条，`id >= 6` 返回空）。射程基值是另一个硬编码 switch `fn_140002d2e`（`0x140123290`）：

| `gun_type_id` | key（`05-ships-fleet/08`） | 名（`+0x00` GBK） | 威力 `+0x11` | 价基 `+0x12` | 射程基值 `fn_140002d2e` |
| --- | --- | --- | --- | --- | --- |
| 0 | `saker` | 霰弹 | 1 | 10 | 70（`0x46`） |
| 1 | `culverin` | 曲射 | 2 | 50 | 98（`0x62`） |
| 2 | `pedrero` | 加农曲射 | 3 | 70 | 70 |
| 3 | `cannon` | 加农 | 4 | 100 | 70 |
| 4 | `heavy_cannon` | 重加农 | 5 | 200 | 70 |
| 5 | `carronade` | 连射 | 5 | 250 | 98 |

炮种目录（key、四语名、威力、价基、解锁门槛）归 `05-ships-fleet/08-gun-shell-catalog.md`，本篇只读。`+0x12` 是**价基**（换炮价 / 估价 `fn_140013593` = 价基 × 3^武装炮台间数 × 20，见 `08`），战斗不用。射程 98 的两种正好是 UI 画「长射程」的 1 / 5。其它 id（含空炮 7）射程 0、威力 0（`fn_14000c6cb` 对 7 返回 0）。

`[确认]` 战斗读的炮种是 `CShip+0x52`（主 vtbl+0x40，`0x1400a7bd0`）。它的初值来自船体定义 `+0x2E` = **默认炮种**（`05-ships-fleet/05` 的 `default_gun_type`，旧稿误称「船体等级」），造船厂换炮后改写。战斗不读船体定义。

### 2.6 船首炮、船尾炮、撞角、武装炮台

`mesfile_sc.txt` `0xc43f`–`0xc503`：

- 船首炮：可向前打，威力小
- 船尾炮：可向后打，命中率低
- 撞角：撞敌舰侧面，进入碰撞 / 肉搏
- 冲锋队长：提高肉搏攻击；水兵室最多 4
- 炮手：缩短炮击间隔；「最多可以设４个室」（炮手所在的武装炮台，`0xc503`）

这些是改装件，不是 `eGunType`。瞄准函数用 `fn_1400091e2(ship, 6)` / `(ship, 7)` 判断能否打船首弧 / 船尾弧（`fn_14000c8c4`）。`[确认]` `fn_1400091e2` 取船 `vfunc+0x30` 的 u16 改装位图，与 `fn_14023df80(idx)` 的位相与；同一函数在 `fn_1400084db` 里以 2 / 3 调用（装甲，见 `05-ships-fleet/04`）。所以 6 / 7 是 `eAddParts` 下标；6 = 船首炮、7 = 船尾炮是按射界推出的 `[推断，强]`（`05-ships-fleet/04` 同结论）。

`[确认]` 舱室种类 8 计数：`fn_140006b95(ship, 8)`（`0x14023e4c0`）数 31 个舱位里 `vfunc+0x38(i) == 8` 的个数，记 `n_battery`。它同时进三处：舷侧倍率 `3*n+1`（`fn_14023eaf0`）、装填周期 `5-n`（§2.10）、被弹着火概率（§2.9）；船侧另用它算炮门 24/间、必要水手 +20/间、炮价 3^n（`05-ships-fleet/03`、`06`、`08`）。种类 8 = 「武装炮台」：exe 舱室名表第 8 条 + 岗位映射 `fn_140007e19(9 炮手) = 8` + 文案 `0x18cef` / `0xf1eb`，`[推断，强]`。重构只认舱室 `role: gun_room`（`05-ships-fleet/03` 的 `cabins.yaml`），查询 `Ships.cabin_count(ship, gun_room)`，不传整数 8。

### 2.7 先手：双方舰长统率（只在无画面结算）

`fn_14000d7ab`（`0x140123d30`，紧挨 `CBattleShip`）是**无画面舰队对舰队结算**（详见 `02-boarding.md` §2.7），实时海战没有先手概念——21 个炮控制器每 tick 各自计时。

```
a_invade = Skill(invade_captain, 1)     # fn_14000e1ce，eSkill 1 = 统率力
a_target = Skill(target_captain, 1)
s234 = (a_invade >= a_target) ? 1 : 0   # 1 = 进攻侧先行动；平手归进攻侧
```

`[确认]` `fn_14000e1ce(person, i)` 是**技能值**，不是六维：`fn_140009214` 用 `.data` 表 `0x1406864a0[i]`（10×2 int）取两维下标，`(ab[a]+ab[b]) * (min(level,200)+100) / 150`，加装备（`fn_14000e0b6`），再按身体状态 `person+0xd` 乘 19/20（1、2）、1/2（3）、1/10（4）、3/5（5）（`fn_1401c87a0`），最后夹到 `[0, 500]`（`fn_1401cbb40`）。六维原值是另一个函数 `fn_1400090ca(person, i)` = `byte(person+0xf+i)`。重构后战斗不读人物字段：人选走 `Fleets.post_holder`，数值走 `Modifiers.eval("skill.leadership", {person})`（技能值单位，0–`skill_value_cap`）。

### 2.8 射程与射界

`fn_14000c8c4`（`0x1401bd330`，RTC `cTergetAngle` / `vecTerget` / `posNow`）`[确认]`，逐条对过原始指令：

1. `fn_14000bed8` 取空闲炮弹槽；池满（100 发都在飞）直接 return 0
2. `vec = target_pos - ship_pos`；`d2 = fn_1400062e4(vec)` = `(x/128)² + (y/128)²`（坐标是 1/128 定点，先截断再平方，**是距离平方**）
3. `r = range_base[gun_type]`（§2.5 表，`vfunc+0x40` 取炮种）
4. `g = fn_140014c7c(ship, 9, 6)`（body `0x14014a5d0`）= **命中力技能**（eSkill 6）。人选三分支 `[确认]`：
   - 船所属舰队 `fn_14000188e(fleet)` 为真（玩家舰队 `[推断]`）且本船 = 该舰队槽 0（`ship.vfunc+0x88 == fn_14000dd05(fleet)`，旗舰）：岗位 9 上的人（`fn_14014a580(9,6)` → 玩家对象 `+0x3600` → `fn_14000da26`：`fn_140007e19(9) = 8` 换成舱室种类，`fn_140012e22` 取该舱的人，`fn_1400135cf(p, 6, 8)` 带舱室参数算技能）；**岗位无人返回 0**
   - 玩家舰队的非旗舰：本船船长（`fn_14000980e(fn_1400042b4(ship))`，`CShip+0x50`）
   - 其它舰队：舰队提督（`fn_1400078a1(fleet)`）
5. 射程修正：`g <= 100` → `r -= 12`；`100 < g <= 300` → `r += g/75`；`g > 300` → `r += g/30`（`0x4b` / `0x1e` / `0x12c`）
6. `d2 < 900`（无符号 `jb`，即距离 < 30）不开火——对得上「靠得太近炮就无法射击了」`0x17f07`
7. `d2 > r*r` 不开火
8. `rel = byte(angle(vec) - heading(ship))`；`power = fn_140004e85(ship)`（`0x14023ebb0`）= 炮种威力（§2.5）+（`vfunc+0x48 == 4` 时 +2）。`vfunc+0x48` 是 `CShip+0x53` 船首像字节（`0x1400a7a10` `[确认]`）；4 = 龙像「对敌炮击提高」`[推断，强]`，见 §2.9 末
9. `rel < 0x20` 或 `rel > 0xe0`：没有改装 6（船首炮）则不开火；有则**威力不乘**
10. `0x60 < rel < 0xa0`：没有改装 7（船尾炮）则不开火；有则**威力不乘**
11. 其余（舷侧）：`power *= 3*n_battery + 1`（`fn_14000bb5e`）
12. `power = (power + rand(3)) / 2`，`fn_1400052e0` 生成炮弹

岗位 9 = 炮手 `[确认]`：岗位名表 `.rdata 0x1405867f0` 第 9 项（`03-characters/03` §2）+ `fn_140007e19(9) = 8`（武装炮台）。角度单位是原版朝向字节（0–255 一圈），不要换成度数写进代码常数。

**炮弹生成** `fn_1400052e0`（`0x140149580`）`[确认]`：

```
shell.owner_gun = gun_index                    # +0x08 byte
shell.age       = 0                            # +0x24 u16
shell.power     = power                        # +0x28 u16
shell.pos       = ship_pos                     # +0x14，当前位置
T = isqrt(d2) / 4                              # +0x26 u16，飞行 tick；fn_14000baaf 牛顿开方
spread = T * 384                               # 定点 = T*3 个坐标单位
shell.aim.x = target.x + rand(spread) - spread/2   # +0x0c
shell.aim.y = target.y + rand(spread) - spread/2   # +0x10
shell.vel   = (shell.aim - shell.pos) / T          # +0x1c/+0x20
```

散布与距离成正比（每轴 ±0.375×距离），命中力技能只影响射程和装填，不影响散布。

### 2.9 炮弹命中（船体 / 水手 / 城市）

`fn_14000ee8a`（body `0x1401c3210`）`[确认]`，已逐条对原始指令。**调用链里的 `idiv` 全部能从原始指令读出**：旧文「除数丢失」那处在 `fn_140012b34`，立即数是 `/10`（暴击率 = 运气/10 %）。

炮控制器段（gun 0..20）：`+0xa != 0` 视为启用；`+0x8 == +0xa` 时调 `fn_1400013ac` 选目标开火，成功则 `+0x8 = 0` 并重算周期（§2.10）、播音效 5；否则 `+0x8 += 1`（`fn_14000edb3`）。

炮弹段（`eShell 0..99`）：

```
shell.age += 1; pos += vel                          # fn_14000efca
if shell.age != shell.T: continue                   # fn_1400076df，只在落点判定一次
best = 576                                          # 0x240，距离平方
town = 战斗城镇 or null                              # fn_140001258
if town:
    c = town.pos + size/2   (size = 0x30 大城 / 0x20，town 记录 +0x24 字节非 0 为大)
    d2 = dist2(shell.pos, c)
    if d2 > best: town = null  else best = d2
hit_ship = null
for i in 0..19:                                     # 不分敌我
    s = mobile_ship[i]; if !s or !alive(s): continue    # alive: vfunc+8 且 +0x2a 不在 4..6
    R = hit_r2[size_class(s)]                       # fn_1401aabc0: 0→81 1→100 其它→121
    if dist2(shell.pos, s.pos) <= R and R <= best:
        best = R; hit_ship = s; town = null
```

注意第二个比较用的是**命中半径平方 R** 而不是距离；多船同时覆盖落点时取 R 最小者，平手取下标大者。友军同样会被打到（对得上「别瞎打，会射中自己人的」`0x18cef`）。

**有效威力** `fn_140012b34`（`0x1401a9920`）：

```
p = shell.power                                     # +0x28
shooter = mobile_ship[shell.owner_gun]              # 城炮为 null → 无暴击
if shooter and alive(shooter):
    luck = fn_14000b267(shooter, 9, 5)              # 六维 5 = 运气原值，人选同 §2.8 第 4 步
    if luck / 10 > rand(100): p += p / 2            # 暴击 ×1.5；玩家舰队另播 fn_1400126f7
```

**打船**（`hit_ship`）：

```
if hull > 0:
    dmg = min(p, hull)                              # fn_140011bd0 = min
    hull -= dmg                                     # fn_14000c202 → fn_140005731(ship, -dmg)
    if hull == 0:
        if fn_14000c8dd(scene, ship, shooter): return   # 沉没；返回非 0 = 战斗结束
    elif fn_140002d15(ship):                        # 船型条件，语义 [未知]
        if crew > 0:
            loss = min(rand(6), crew - 1)           # 0..5，炮击永远不打死最后一人
            crew -= loss                            # fn_140014943
        if rand(100) <= n_battery(ship):            # 概率 (n+1)/100；无武装炮台也有 1%
            ship.state(+0x2a) = 0                   # fn_14000118b；玩家舰队弹「火药着火」对白
```

**打城**：`v = town.develop[1]`（`fn_14000bc03(town,1)` = word `town+0x20`）；`v > 0` 时 `dmg = min(p, v)`，`fn_140004f84(town, dmg)` 把该项减 `dmg`。不碰市价。

**落空**：播音效 0x18。飞出场景矩形（`scene+0x88`）的弹由 `fn_140013c73` 回收。

`vfunc+0x50` = 耐久、`vfunc+0x58` = 水手：`[确认]`——战斗槽初始化 `fn_140011603` 把两者分别存为 `+0x10 hull` / `+0x14 crew`，移动船初始化 `fn_1400033eb`（`0x140148ec0`，RTC `eShip`）把源船的 `vfunc+0x50` / `vfunc+0x58` 写进移动船 `+0x26` / `+0x28` 两个 u16，并把 `+0x2a` 设 8、`+0x2b` 设 0。

**船态字节 `+0x2a`** `[确认]`：getter `fn_1400106a4`；初值 8（`0x140148ffd`、`fn_14000f079`）；水手打光写 4（`fn_14000660e`）；`fn_140005713` 视 4..6 为退出战斗；被弹着火写 0。0/4/5/6/8 的名字 `[未知]`（0 = 着火、4 = 被擒为 `[推断]`）。

旗舰耐久 **或** 水手 `<= 0x1e`（30）则该侧判定退走：只在无画面结算 `fn_14000d7ab` 每回合末尾检查，先查第二行动方（`0x140124abf`）。实时海战没有这条阈值。

近战加成 `vfunc+0x48 == 8` 是 **×1.2**（`x + x/5`），不是翻倍（`fn_14000d7ab` `0x140124709`、`fn_14000660e` `0x1401bc38d`）。

**`vfunc+0x48` = 船首像** `[确认 字段]`：`fn_1400042b4(mobile_ship)`（`0x14023f6f0`）取 `vfunc+0x88` 船号再查船池，返回的是 **`CShip`**（不是舰队）；`CShip` 主 vtbl+0x48（`0x1400a7a10`）读 `+0x53`，即船首像字节（`06-ocean`、`05-ships-fleet/04`）。战斗里出现的四个比较值与船首像文案逐一对上 `[推断，强]`：

| 值 | 位置 | 效果 | 船首像（道具 114 起） |
| --- | --- | --- | --- |
| 3 | `fn_1400084db` 最大耐久 | +10 | 白鲸像「不易沉」 |
| 4 | `fn_140004e85` 炮威力 | +2 | 龙像「对敌炮击提高」 |
| 8 | `fn_14000660e` / `fn_14000d7ab` 肉搏伤害 | ×6/5 | 王者像「鼓舞水手」 |
| 10 | `fn_14000974b` 实时入场（`0x14018ed91`）+ `fn_14000d7ab` 无画面开场（`02` §2.7） | 对侧每船水手 −crew/10、对侧提督精神 5；接舷不查 | 第 11 个船首像（道具 199 怪鱼像？`[推断，弱]`） |
| 7 | `CBattleShip::vfunc_14` | 耐久写到 ≤0 抬回 1 | 魔鬼像（`05-ships-fleet/04`） |
| 0xb | `CBattleShip::vfunc_14` / `vfunc_16` | 耐久 / 水手写到 ≤0 抬回 1 | 提督像（道具 200） |

重构：这些是 `figureheads.yaml` 的 `effects`，向修正器注册表贡献 `combat.*` 键与 `ship.durability_max`（§6），战斗代码里不出现 `== 3/4/7/8/10/0xb`。

### 2.10 弹药

没有名为 ammo / powder / 炮弹数的 RTC 局部。`CBattleGun` 存档只序列化 `+0x8`、`+0xa` 两个 uint16（`vfunc_06/07`）。`[确认]` 两者是计时器：`+0x8` 每 tick +1（`fn_14000edb3`），等于 `+0xa` 时可开火（`fn_140010767`），开火后清 0（`fn_14000d512`）；`+0xa` 为 0 表示停用（`fn_14000b389`）。**没有弹药池**，火力只受装填周期和 100 发在飞上限约束。

装填周期 `fn_140003d5f`（`0x14014c850`）`[确认]`：

```
if gun 有船 且 船有效:
    base = 5 - n_battery                            # 可为负
    if 船 == 玩家舰队旗舰:  t = (1100 - g_post9) / 10   # 0x44c；fn_14014a270() 玩家舰队 → fn_14000dd05 槽 0；g = 炮手命中力，无人 0
    else:                  t = (550  - g_cmd)  / 10    # 0x226；fn_140014c7c(ship, 9, 6)：玩家非旗舰取船长，其它舰队取提督
elif gun 有船 但无效: base = 1, t = 1
else (城炮 gun 20):
    base = 0
    t = (550 - (town.develop[1] / 20 + 1)) / 10
period = base + max(t, 1)                           # 写 +0xa
```

只有**玩家旗舰**用 1100，玩家其余船与所有 NPC 船用 550：同技能下玩家旗舰更慢（`t` 项：技能 0 时 110 vs 55，技能 500 时 60 vs 5）——这是原版数值，不是笔误；重构保留为两个独立旋钮 `reload.ref_player_flagship` / `ref_other`，不在引擎里「修正」。「炮手缩短炮击间隔」的文案对应 `g_post9`（岗位 9）。

### 2.11 士气

战斗 RTC / 头文件里没有 morale 枚举。`mesfile` 的「士气」出现在大厨、项羽头盔、舰长职文，是人物/道具效果，不是海战条。水手「不满 / 疲劳」是航海状态，开战前由 ocean 结算。海战过程**不要**另做一条士气条，除非以后从存档位恢复。

### 2.12 战果与文案

| 符号 | 值 | 来源 |
| --- | --- | --- |
| `eBattleResult` / `NUM_BATTLERESULT` | 5 = **槽下标**（循环上界 `>= 5`） | `BattleResultID.h`；`CForceElement` ctor `+0x910` 建 5 槽 |
| `BATTLERESULTSTATE_BITS` | 符号存在 | `constants.json` |
| `CBattleResult` | 5 字节 `+8..+0xC`：势力 A、势力 B、港口、旗（0/1）、写入月 | `vfunc_06/07`；写入函数见 `02-factions/05` §2.4 |

`[确认]` `eBattleResult` **不是**「胜/败/逃/擒/占」五种结局的枚举，而是挂在 `CForceElement+0x910` 的 5 个**战果记录槽**（`02-factions/05-war-failure-absorb.md` §2.4 / §2.9）：战后 `fn_1401200a0` 用 `fn_14000d508` 取第一个空槽（`+8 == 0x1A`），按三种形状写 {A, B, 港, 旗}，`+0xC` = 当前月；月结在写入 3 个月后清除。槽与哨兵归 **factions**；本系统只在 `BattleResolved` 里给出参战势力、港口与胜负旗。

文案侧能见到的结局包括：胜利、逃走、沉没、擒获、占领、投降、城市投降、战利品金币（`mesfile_sc.txt` `0x13a93`–`0x13cbb`、`0xf1eb`、`0xf4af`）。这些是会话结局，与槽数 5 无关。

金币战利品：`从擒获的船上得到%s金币`（`0x3363`）、`获得了%s枚金币的战利品`（`0x13bfb`）。`fn_140005fb0` 对船态 `+0x2a == 4`（`fn_1400106a4`）的船累加 `fn_140006b95(ship, 10)`——`[确认]` 这是「舱室种类 10 的个数」（31 舱位计数，见 §2.6），**不是金币**；合计值作为第 3 参传给分货窗口 `fn_1400034ae`（种类 10 = 货舱，见 `06` §2.3）。金币数的来源未在此函数内。战斗产出的是物品/金币/船 ID，不是价格。

### 2.13 港口炮击与占领

文案：

- `%s城正在炮击%s舰队`（`0xac7`）
- `%s城市炮击我们` / `向我们开炮`（`0x13af3`）
- `开始攻击%s城市`（`0x13cbb`）
- `%s城市投降了`（`0x13a93`）；职文「要想让城市投降，需要舰长和副官」（`0x18463`）
- 武装炮台增减要钱（`0x32b7`）；战斗中炮台火药着火（`0xf1eb`）

占领港是势力状态，不是海战写 `town.market_price`。海战只许发 `town_occupied`。

### 2.14 `500-a` 不是炮击公式

`srcdata/docs/exe_caps.md`：战斗结算 `fn_14000660e` 使用 `500 - a`，`a` 来自 `fn_14000e1ce`（技能值，自身已夹在 `[0,500]`，所以 `500-a` 不会为负）。`[确认]` 它只出现在接舷单挑的败方水手损失 `crew*(500-统率)/500`；无画面结算用的是另一种形式 `x*(运气+500)/500`。炮弹路径（§2.8–2.10）完全不含 500。见 `02-boarding.md`。

### 2.15 战斗收尾的外部钩子

- `[确认]` 舰队覆灭处理 `fn_14000a9ed`（被 `fn_14000d7ab`、战后 `fn_14000ec6e` 等调用）：被灭的是玩家本队时调 `fn_140010578(DAT_144296500, 0)` → 流程态 3 = Game Over（子码 0）；其它分支给经验。来源：`11-progression/03-gameover-succession.md`、`12-ui-save/01-scenes-flow.md`。重构：战斗只调 `Fleets.destroy(fleet_id, cause)`，由 fleets 发 `FleetDestroyed { fleet_id, cause }`；是否终局由 `Session.end_scenario(outcome)` 的规则决定（D8），战斗不调流程机。
- `[确认]` 叙事触发：进入海战（场景子态 1）以 kind 1 发事件，载荷对手 `person_id`（`0x46` = 无对手不发）；kind 2 / 3 = 海战结束 / 舰队接触。来源：`10-narrative/02-event-system.md`、`03-event-commands.md`。重构：`BattleStarted` / `BattleResolved` / `FleetContact` 载荷带 `opponent_person_id: Option<PersonId>`，由叙事层订阅。

## 3. 实体与 ID

| 实体 | 稳定 ID | 原版计数 | 空值 | 运行时 |
| --- | --- | --- | --- | --- |
| 炮种 | `gun_type` key（`vanilla:saker`…，`05-ships-fleet/08`） | 6 | `null`（原版装备空炮 7） | 表长 |
| 场景炮控制器 | `scene_gun` 运行时下标 | 21 = 20 船 + 1 城 | `null` | 船数 + 城数 |
| 炮弹实例 | `shell` 运行时下标 | 100 | `null` | 池长 |
| 战果记录槽 | 属 factions（`02-factions/05`） | 5 | `null` | factions 配置 |
| 参战船位 | `fleet_ship_id` | 5 / 侧 | `INVALID_FLEETSHIP=-1` | `caps.fleet_ships` |
| 场景船 | `mobile_ship_id` | 20 | `INVALID_MOBILESHIP=-1` | `caps.mobile_ships` |
| 场景舰队 | `mobile_fleet_id` | 4 | | `caps.mobile_fleets` |
| 海战会话 | `battle_id` | 运行时 | | 不进原版枚举 |

原版兼容包：`fleet_ships: 5`、`sides: 2`。引擎按表长循环，禁止写死 `for i in 0..4`。内容表主键是命名空间 key（D1），运行时整数只在会话内部用；原版哨兵（空炮 7、空人物 `0xEC`、空船 `0xD0`、空势力 `0x1A`、无港 `0x62`）只出现在 `ids/legacy_codec.yaml`，会话快照里一律 `null`。

参战船在会话里只持：`fleet_id`、`fleet_ship_id`、`ship_type_id`、`gun_type`、`figurehead`（可空）、当前耐久、当前水手、朝向、坐标，以及开战时取的派生量快照（`max_hull`、`crew_needed`、`n_battery`，唯一计算方 Ships，D6）。不持 `Player*`、不持 `World*`。

## 4. 算法 / 公式

全部立即数进 `config/formulas/combat.yaml`（D3：每条公式声明 `int_mode: trunc/wrap32` 与 `quirks`，带 `golden` 对拍用例）。代码只调纯函数。人物数值一律经修正器注册表 `Modifiers.eval(key, ctx)`（D2），人选一律经 `Fleets.post_holder` + 数据化的 `holder_rule`：

```
holder(ship, rule) =                          # 原版 fn_140014c7c / fn_14000b267 的三分支，写成数据
  if Fleets.is_player_fleet(ship.fleet) and ship == Fleets.flagship(ship.fleet):
      Fleets.post_holder(ship.fleet, rule.post)       # 原版 gunner；无人 → None → 技能按 0
  elif Fleets.is_player_fleet(ship.fleet): Ships.captain(ship)
  else: Fleets.admiral(ship.fleet)
skill_of(ship, rule) = holder ? Modifiers.eval("skill." + key_name(rule.skill), {person: holder, cabin: posts[rule.post].room}) : 0   # cabin = 舱室种类 key（原版 gun_deck），不是 role
# key_name 去包前缀（vanilla:gunnery → gunnery）；修正器 key 形如 skill.gunnery（14 README：refs 写 vanilla:accounting，eval 用 skill.accounting）
```

### 4.1 先手（仅无画面结算）

```
initiative(side) = Modifiers.eval("combat.initiative", {person: Fleets.admiral(side.fleet)})   # 原版 base = skill.leadership
first_side = invade if initiative(invade) >= initiative(target) else target
```

原版包：`combat.initiative` 的 base 贡献 = 统率力技能值（eSkill 1，`fn_14000e1ce`）。实时海战不用。

### 4.2 装填（每个炮控制器）

```
period(gun):
  if gun.ship:
    base = reload_base - per_battery_reload * Ships.cabin_count(ship, gun_room)
    ref  = reload_ref_player_flagship if holder_branch(ship) == post else reload_ref_other
    t    = (ref - skill_of(ship, gunnery_rule)) / reload_div
  else:                                              # 城炮
    base = 0
    t    = (reload_ref_other - (town_def(town) / town_def_div + 1)) / reload_div
  return base + max(t, reload_min)
tick: counter += 1；counter == period 时尝试开火，成功则 counter = 0
```

### 4.3 开火许可与出膛威力

```
can_fire(shooter, target_pos):
  d2 = dist2(shooter.pos, target_pos)                # 先把定点坐标 /128 再平方
  if d2 < min_cannon_range2: return false            # vanilla 900（=30²）
  r = guns[gun_type].range_base + range_bonus(skill_of(shooter, gunnery_rule))
  if d2 > r*r: return false
  rel = byte(bearing(shooter, target_pos) - shooter.heading)
  if rel in bow_arc:   return has_part(bow_gun)   -> mult = 1
  if rel in stern_arc: return has_part(stern_gun) -> mult = 1
  mult = broadside_mul_k * Ships.cabin_count(shooter, gun_room) + broadside_mul_c
power = Modifiers.eval("combat.gun_power", {ship: shooter, base: guns[gun_type].power})   # 原版龙像贡献 add +2
power = (power * mult + rand(power_jitter)) / 2

range_bonus(g) = -range_low_penalty          if g <= range_tier1
               = g / range_div1              if g <= range_tier2
               = g / range_div2
```

### 4.4 弹道、落点与伤害

```
T      = isqrt(d2) / shell_speed_div                 # 飞行 tick
spread = T * spread_per_tick                         # 定点
aim    = target_pos + uniform(-spread/2, spread/2)   # 每轴独立
vel    = (aim - start) / T
on age == T:
  candidates = ships(alive) where dist2(aim, ship) <= hit_r2[size_class(ship)]
  pick min hit_r2，且须 <= town_d2（若有城）否则打城（town_d2 <= shell_acquire_limit）
eff = power
if shooter and Modifiers.eval("combat.crit_luck", {person: holder(shooter, gunnery_rule)}) / crit_luck_div > rand(100):
    eff += eff * crit_bonus_pct / 100            # 原版 p + p/2；crit_luck base = 运气六维原值（不是技能值）
ship: dmg = min(eff, hull); hull -= dmg
      if hull == 0: sink
      elif Ships.takes_splash(ship):              # 原版 fn_140002d15，语义 [未知]
            if crew > 0: crew -= min(rand(crew_splash_sides), crew - 1)
            if rand(100) <= Ships.cabin_count(ship, gun_room) * fire_per_battery: set_state(fire)
town: v = town_def(town); if v > 0: town_def -= min(eff, v)
```

原版包数值全部 `[确认]`（地址见 §2.8–2.10），唯一出处是 §6 `formulas/combat.yaml` 的 `naval:` 段；射程基值 / 威力在 `ships/guns.yaml` 行内；龙像 +2 在 `figureheads.yaml` 的 `effects`。本节不再重复列值，避免两处漂移。

### 4.5 城炮 / 攻城

同一套炮弹池。目标 `town_id` 时发命令 `Towns.apply_develop(town_id, military, -damage, bombard)`（world `Towns` 唯一写方并发 `TownDeveloped`，原版扣 `develop[1]`），战斗只传 ID 与数值（`04` §4.2）。城市开火方向相反：战斗内 `try_town_fire(town_id, target) -> optional ShellRequest`（`04` §4.3，读 `Towns.develop(town_id, military)`），原版城炮只打 fleet kind ∈ {0,1} 的船（`fn_1400013ac` → `fn_140005b37`）。

### 4.6 结束

```
if no_ships_able_to_fight(side): resolve(opposite)
if flagship_fled(side): resolve(flee)
# 接舷导致的投降 / 擒获由 boarding 回调 Combat.apply_capture
# 沉没后的胜负判定 fn_14000c8dd 函数体未展开 [未知]：上两行是重构语义，不是原版逐条
if fleet_wiped(fleet): Fleets.destroy(fleet_id, cause=battle)   # 原版 fn_14000a9ed；fleets 发 FleetDestroyed，终局与否交给 Session 规则
```

战利品、拖船、货舱转移在结算阶段用 `CaptureSession` 做，不在炮弹 tick 里写货格。

## 5. 耦合分析

原版硬编码依赖：

| 依赖 | 表现 | 重构 |
| --- | --- | --- |
| 人物技能 / 六维 | 技能 `fn_14000e1ce(person, 1/2/5/6)`（统率/驾驶/剑术/命中）、六维 `fn_1400090ca(person, 5)`（运气）直接进射程、装填、暴击、先手 | `Modifiers.eval("skill.<key>" / "combat.<key>", ctx)` 注册表（D2） |
| 岗位硬编码 | `fn_140014c7c(ship, 9, 6)`：岗位 9 + 「玩家旗舰 / 玩家他船 / NPC」三分支 | `Fleets.post_holder(fleet, post_key)` + `holder_rule` 数据 |
| 舱室硬编码 | 舱室种类 8 计数进倍率/装填/着火 | `Ships.cabin_count(ship, role)`，role = `gun_room` |
| 船首像字面量 | `CShip+0x53 == 3/4/7/8/10/0xb` 散在六个函数 | `figureheads.yaml` `effects` → `combat.*` 修正器键 |
| 技能值上限 500 | `fn_1401cbb40` 夹 `[0,500]`，`500-a` 不会为负 | `caps.skill_value_cap` |
| 船 vfunc 表 | `+0x48` 船首像、`+0x50` 耐久、`+0x58` 水手、`+0x2a` 船态字节 | 战斗会话快照字段 |
| 造船改装 | 炮种、船首/尾炮、撞角、武装炮台 | 只读 `ships` 查询 |
| 海洋场景 | 战斗 tick 塞在 `COceanScene` | `Ocean` 发 `BattleTick`，战斗自己跑 |
| 城镇发展度 | 解锁炮种、城防开火 | `town.military` 只读 |
| 势力 | 战后 `fn_1401200a0` 直接写 `CForceElement+0x910` 战果槽 | 发 `BattleResolved`，factions 自己写槽 |
| 文案 / 像素 | 屏幕 ctor 一堆坐标 | `ui/scenes.yaml` |
| 存档 | `CBattleGun` `CShell` `CBattleResult` `CBattleShip` 都是 `CSaveObject` | 会话态进存档 schema，定义不进存档 |

禁止：combat → economy 写价格；combat → 人物写经验；UI 写存档。

## 6. 解耦后的配置

### `config/formulas/combat.yaml`

```yaml
schema: dk4.formulas.combat
version: 1
naval:
  impl: vanilla.naval
  int_mode: vanilla_i32            # D3（= { div: trunc, width: wrap32 }，原为文件级）；原版全部有符号 idiv，除非 quirks 另注
  quirks:
    splash_never_kills_last: true  # min(rand6, crew-1)
    friendly_fire: true            # 落点判定不分敌我
    pick_smallest_hit_r2: true     # 比 R 不比距离；平手取下标大者
  params:
    heading_mod: 256                 # 朝向一圈
    # 以下键全部 [确认]，地址见 §2.8–2.10
    min_cannon_range2: 900           # 无符号 jb
    range_tier1: 100
    range_low_penalty: 12
    range_tier2: 300
    range_div1: 75
    range_div2: 30
    bow_arc:   { lt: 32, gt: 224 }   # 0x20 / 0xe0
    stern_arc: { gt: 96, lt: 160 }   # 0x60 / 0xa0
    broadside_mul_k: 3
    broadside_mul_c: 1
    power_jitter: 3
    shell_speed_div: 4
    spread_per_tick: 384             # 1/128 定点
    shell_acquire_limit: 576         # 0x240，城镇落点距离平方上限
    town_center_offset: { large: 24, small: 16 }
    crit_luck_div: 10
    crit_bonus_pct: 50
    crew_splash_sides: 6
    fire_per_battery: 1              # rand(100) <= n → (n+1)%
    reload: { base: 5, per_battery: 1, ref_player_flagship: 1100, ref_other: 550, div: 10, min: 1, town_def_div: 20 }
  tables:
    hit_r2_by_size_class: { 0: 81, 1: 100, default: 121 }
    # 人选规则（原版 fn_140014c7c / fn_14000b267）：数据，不是 if 玩家
    holder_rules:
      gunnery_rule:   { post: vanilla:gunner, skill: vanilla:gunnery,
                        branches: [player_flagship: post, player_other: captain, other: admiral] }
      crit_luck_rule: { same_as: gunnery_rule, ability: vanilla:luck }   # 原版读运气六维原值，不是技能值
  # 修正器出口：引擎只 eval 这些键
  modifiers: [combat.initiative, combat.gun_power, combat.crit_luck, combat.hull_floor_at_zero, combat.crew_floor_at_zero]   # 耐久上限 ship.durability_max 归 ships 派生，战斗只取 Ships.max_hull 快照
offscreen:                         # fn_14000d7ab，见 02-boarding §2.7
  impl: vanilla.offscreen
  int_mode: vanilla_i32
  params:
    defeat_hull_or_crew: 30
```

`gunnery_rule.branches` 是原版三分支的数据化；模组可以改成「所有舰队都用炮手」而不改引擎。`ref_player_flagship` 与 `ref_other` 的不对称是原版事实，保留为两个旋钮。

### `config/combat/naval.yaml`

```yaml
schema: dk4.combat.naval
version: 1
sides: 2
ships_per_side: $caps.fleet_ships
scene_gun_pool: derived            # 运行时 = 场景船数 + 城数；原版上限 21 [确认]
shell_pool: $caps.shell_pool
mobile_ship_cap: $caps.mobile_ships
add_parts_used_in_aiming:          # Ships.has_add_part 的 key，不传位号
  bow_gun_part: vanilla:bow_gun    # 原版位 6 [推断，强]
  stern_gun_part: vanilla:stern_gun  # 原版位 7
cabin_roles_used: [gun_room]       # 原版种类 8；计数进倍率/装填/着火
outcomes_ui_keys:                  # 只引用文案 key，不写中文进逻辑
  - battle.town_surrendered
  - battle.town_fires
  - battle.ship_sunk
  - battle.ship_captured
  - battle.loot_gold
```

旧稿的 `commands: [cmd_2..cmd_6]` 删除：原版炮击没有命令号（`05` §2.5）；2–6 是场景 happening，在 `combat/happenings.yaml`。

### `config/ships/guns.yaml`（战斗只读，所有者 `05-ships-fleet/08`）

```yaml
# 只列战斗读的列；完整行（name_i18n、price_base、icon_frame）见 08
types:
  - { key: vanilla:saker,        power: 1, range_base: 70, necessary_development: 1 }
  - { key: vanilla:culverin,     power: 2, range_base: 98, necessary_development: 1 }
  - { key: vanilla:pedrero,      power: 3, range_base: 70, necessary_development: 2000 }
  - { key: vanilla:cannon,       power: 4, range_base: 70, necessary_development: 3500 }
  - { key: vanilla:heavy_cannon, power: 5, range_base: 70, necessary_development: 5000 }
  - { key: vanilla:carronade,    power: 5, range_base: 98, necessary_development: 7000 }
# 空炮：装备槽为 null；原版 7 只在 ids/legacy_codec.yaml
```

`range_base` 原版是 `fn_140002d2e` 的 switch，不在 `.data` 炮表里；重构把它并进 08 的同一行（需 08 加列，见报告）。旧稿 key（`grapeshot` / `howitzer` / `rapid_fire`）作废，统一用 08 的 key。

### `ships/figureheads.yaml` 的战斗效果（所有者 `05-ships-fleet/04`）

```yaml
# 船首像向修正器注册表贡献的战斗键；原版取值 [推断，强]，依据见 §2.9
- { key: vanilla:white_whale, effects: [{ key: ship.durability_max, stage: add, value: 10 }] }   # ships 派生耐久上限；战斗开局快照
- { key: vanilla:dragon,      effects: [{ key: combat.gun_power,  stage: add, value: 2 }] }
- { key: vanilla:king,        effects: [{ key: combat.melee_damage, stage: mul, value: { num: 6, den: 5 } }] }   # 原版 x + x/5；非负整数时与 6x/5 截断相等
- { key: vanilla:monster_fish, effects: [{ key: combat.engage_fear_crew_div, stage: override, value: 10 }] }  # 原版开战时（实时入场 fn_14000974b + 无画面开场 fn_14000d7ab，02 §2.7）：对侧每船 crew -= crew/10；非接舷；id 对应 [推断，弱]
- { key: vanilla:devil,       effects: [{ key: combat.hull_floor_at_zero, stage: override, value: 1, scope: ship }] }   # CBattleShip::vfunc_14；航速效果见 05-ships-fleet/04
- { key: vanilla:admiral,     effects: [{ key: combat.hull_floor_at_zero, stage: override, value: 1, scope: ship }, { key: combat.crew_floor_at_zero, stage: override, value: 1, scope: ship }] }   # vfunc_14 / vfunc_16
```

### `config/ships/shells.yaml`

炮弹**定义**（若与 100 实例池分离）和战斗池分开。原版 `NUM_SHELL=100` 更像实例池（循环 + `CSaveObject`），类型数可能更少（RTC 另有 `eShell >= 4`）。未分开前，池长进 `caps.yaml`，单发字段进不透明 blob。

### `config/caps.yaml` 片段

```yaml
fleet_ships: 5
shell_pool: 100
mobile_ships: 20
mobile_fleets: 4
skill_value_cap: 500               # 旧名 ability_settled_cap；技能值上限，不是六维（100）
# 炮种数、场景炮池不进 caps：按 guns.yaml 表长 / 场景实体数派生
ship_crew: { min: SHIP_CREW_MIN, max: SHIP_CREW_MAX }
ship_durability: { min: SHIP_DURABILITY_MIN, max: SHIP_DURABILITY_MAX }
```

MIN/MAX 整数以 RTC / 断言补齐，文档保持符号名。

## 7. 独立旋钮

改这些值不应迫使贸易、人物、城镇改代码：

- `ships_per_side` / `shell_pool` / `scene_gun_pool`
- `min_cannon_range2`、射界字节、`range_*`、`spread_per_tick`、`hit_r2_by_size_class`
- `crit_*`、`fire_per_battery`、`reload.*`（玩家旗舰 1100 / 其它 550 的不对称是原版旋钮，可单独抹平）
- `holder_rules.*.branches`（谁的技能进射程 / 装填 / 暴击）
- `quirks.*`（友军误伤、最后一名水手不死、取最小命中圆）
- 六炮种的 `power` / `range_base` / `necessary_development`（`ships/guns.yaml`）
- 船首像战斗效果（`figureheads.yaml` `effects`）
- `offscreen.defeat_hull_or_crew`（无画面结算，见 boarding）
- 是否允许攻城、城炮是否与舰炮同一炮弹池

加新炮种：只追加 `ships/guns.yaml` 行，战斗按表长枚举。显示宽度由 UI 自己处理。

## 8. 对外接口

查询（只读）：

```
Combat.can_engage(fleet_id_a, fleet_id_b) -> bool
Combat.snapshot(battle_id) -> BattleSnapshot      # ID 与数值，无指针
Ships.gun(ship_id) -> GunTypeRecord | null
Ships.cabin_count(ship_id, role) -> int           # role = gun_room（原版种类 8）
Ships.has_add_part(ship_id, add_part_key) -> bool
Ships.size_class(ship_id) -> int                  # 命中半径
Ships.max_hull(ship_id) / crew_needed(ship_id)    # 派生量，唯一计算方 Ships（D6）
Ships.captain(ship_id) -> Option<person_id>
Fleets.post_holder(fleet_id, post_key) -> Option<person_id>
Fleets.admiral(fleet_id) / flagship(fleet_id) / is_player_fleet(fleet_id)
Modifiers.eval(key, ctx) -> int                   # skill.gunnery / skill.leadership / combat.* …
Towns.develop(town_id, track) -> int             # 城防读 military 轨（world Towns）
```

命令：

```
Combat.start(BattleStart { attacker_fleet_id, defender_fleet_id, sea_area_id, optional town_id })
Combat.issue(battle_id, fleet_ship_id, command_key, target)   # 模组扩展；原版炮击无命令号
Combat.cancel(battle_id, reason)                  # 逃走
```

事件（载荷只有 ID 与值）：

```
BattleStarted     { battle_id, attacker_fleet_id, defender_fleet_id, sea_area_id, opponent_person_id: Option }   # 叙事 kind 1
ShipHullChanged   { battle_id, fleet_id, fleet_ship_id, hull }
ShipCrewChanged   { battle_id, fleet_id, fleet_ship_id, crew }
ShipSunk          { battle_id, fleet_id, fleet_ship_id }
ShipStateChanged  { battle_id, fleet_id, fleet_ship_id, state_id }   # 原版着火写 0
ShellLanded       { battle_id, shell_id, target: ship|town|none, damage, crit }  # UI 订阅
TownBombarded     { battle_id, town_id, damage }
BattleResolved    {
  battle_id,
  winner_side: 0 | 1 | null,
  outcome: win | flee | sunk | captured | surrendered,   # 会话结局 key；不是 eBattleResult 槽号
  force_a, force_b, town_id: Option,                     # factions 据此写战果记录槽
  loot: [{ item_id | gold, qty }],
  captured_ships: [ship_id],
  town_occupied: town_id | null
}
```

`BattleResolved` 由 economy / factions / progression / town-life / narrative 订阅（完整载荷见 `06` §8）。战斗模块发完即丢会话，不写市场、不写势力战果槽。

## 9. 未核实字段

| 位置 | 现状 | 禁止的做法 |
| --- | --- | --- |
| `CShell +0x0c..+0x28` | `[确认]` +0x08 发射炮下标、+0x0c/+0x10 落点、+0x14 当前位置（初值=起点，`fn_14000efca` 每 tick 加速度）、+0x1c/+0x20 速度、+0x24 已飞 tick、+0x26 总 tick、+0x28 威力 | — |
| RTC `eShell >= 4` | 与 100 并存，未定位用途 | 把 100 当炮种数 |
| 船态 `+0x2a` 取值 0/4/5/6/8 | 写入点已知，名字未恢复 | 用 D&D 状态名 |
| `vfunc+0x48` 3/4/7/8/10/0xb | `[确认]` = `CShip+0x53` 船首像字节；3/4/7/8/0xb ↔ 白鲸/龙/魔鬼/王者/提督 `[推断，强]`；10 ↔ 道具 199 `[推断，弱]` | 写成船态、撞角 |
| `fn_140002d15(ship)` | 决定被弹后是否掉水手/着火（`vfunc+0x18` 船模 → `fn_14014ad50`），重构为 `Ships.takes_splash`，宜做成船模表字段 | 当「有水手」 |
| 岗位 9 / 舱室种类 8 / 10 的名字 | 岗位 9 = 炮手 `[确认]`（名表 + `fn_140007e19`）；种类 8 = 武装炮台、10 = 货物仓库 `[推断，强]` | 在代码里写整数 |
| `fn_140007e19` 16 路 post→cabin 映射 | `[确认]`（跳表 `0x14006902c` 已解码）0→5,1→6,2→19,3→20,4→3,5→0,6→4,7→1,8→7,9→8,10→18,11→14,12→15,13→13,14→17,15→2，其余 24。即舰长→舰长室、炮手→武装炮台、冲锋队长→水兵室、了望手→了望台 | 自行重排 |
| `fn_14000c8dd` 沉没后战斗结束条件 | 返回非 0 即结束，函数体未展开 | 自编胜负表 |
| `eBattleResult` | `[确认]` 5 个战果记录槽的下标（factions），不是结局枚举 | 写成 win/lose/draw/flee/capture |
| 海战士气条 | 炮击全链无士气项 `[确认]` | 从航海不满抄一条进战斗 |
| `CBattleShip +0x10,+0x14` | `[确认]` = 初始化时的耐久/水手快照（`fn_140011603`），`+0x0c` = 四项船属性和 /8，`+0x18` 退场标记 | 当最大耐久 |
| 炮弹池是否进长期存档 | 类是 `CSaveObject` | 假设战役之间还留着 100 发弹 |

炮击伤害已从 `fn_14000c8c4` / `fn_1400052e0` / `fn_14000ee8a` / `fn_140012b34` / `fn_140003d5f` 原始指令完整还原（§2.8–2.10、§4.2–4.4），没有脏 `idiv`。
