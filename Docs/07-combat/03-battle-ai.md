# 战斗 AI、目标选择与 happening

玩家海战指令在 `01-naval-battle.md`，接舷在 `02-boarding.md`。这一篇管 **NPC 舰队怎么打、打谁、战斗中的随机事件**。原版 `eBattleAlgo` / `eSELECTTERGET` 只有 World DLL 断言，没有恢复出枚举值和公式表——配置先把槽位留出来，禁止用「会 T 字战法」这种文案当算法。

## 1. 职责边界

本系统管：

- NPC 参战舰队的战术槽 `battle_algo_id`、目标策略 `select_target_id`
- 战斗 tick 里按船扫描的命令 / happening
- 大地图上地方舰队互相开打（`CForceElement` 月结扫 `CBattleResult`）
- 警戒、放哨、主动进攻如何变成 `BattleStart`（只发命令，遭遇检测属 ocean）

本系统不管：

- 炮弹物理、射界、船体公式（`01`）
- 白刃 / 单挑内部（`02`）
- 舰队巡逻路线、委任贸易（`fleets/`、`ocean/`）
- 势力友好度为什么开战（`factions` 提供 `may_attack(force_a, force_b)`）
- 文案（「舰队向我们进攻了」只挂 key）

AI 只输出 `Combat.issue(command_id, target_id)`，不自己改耐久。

## 2. 原版事实

### 2.1 `eBattleAlgo` 与 `eSELECTTERGET`

头文件是从 World DLL `_wassert` 捞回来的：

```
( 0 <= eData && eData < NUM_BATTLEALGO ) || eData == INVALID_BATTLEALGO
( 0 <= eData && eData < NUM_SELECTTERGET ) || eData == INVALID_SELECTTERGET
```

`BattlealgoID.h` / `SelecttergetID.h` 里 **`NUM_*` 没有整数**。`constants.json` 有 `BATTLEALGO_ID`、`SELECTTERGET_ID` 符号。主程序 cpp 里没有以这些枚举为 RTC 名的循环。

含义 `[推断]`：地方舰队 / 世界表上的「战斗算法」和「选目标」ID。`SELECTTERGET` 是原拼写（Target）。World0 舰队记录步长 0x18（`World0_fleet.c`），字段未命名，不能指着某个 byte 说「这就是 algo」。

重构仍然给这两个 ID 留表：内容包可以先只有 `invalid`，有对照后再填。

### 2.2 场景里的 NPC 舰队

| 符号 | 值 | 证据类型 / 来源 |
| --- | --- | --- |
| `NUM_MOBILEFLEET` | 4 | 循环上界：`fn_14000cd8d` `eMobileFleet >= 4`（`num_values.json` 只是投票） |
| `NUM_MOBILESHIP` | 20 | 循环上界：`fn_14000ee8a` `eMobileShip >= 0x14` |
| `NUM_FLEET` | 69 | 世界表行数 |

`fn_14000cd8d`（`0x140192400`，RTC `sChangeBattle` / `eMobileFleet` / `sEventTrigger`）：

- 循环 `eMobileFleet = 1; ... >= 4`
- 某字段 `== 0x46`（70）则 skip：`0x46` 是原版越界哨兵（叙事侧同样以 `0x46` 表示「无对手 person」，`10-narrative/03`），按 D1 只进 `ids/legacy_codec.yaml`，运行时为 `null`
- 通过一串位置 / 关系检查后改 `sSceneDesc`，把该 mobile fleet 推进战斗

下标 0 不进循环 `[推断]`：0 是玩家。`COceanScene` 在非 `fn_140006c12==1` 的分支调用它（`COceanScene.cpp` 约 `0x1401a6301`）。

开战文案：

- `%s上的%s的舰队向我们进攻了！`（`0x5d0f`）
- `在%s发现%s的舰队！进入战斗状态！` / `继续警戒吗？` / `继续放哨吗？`（`0x5aff`）
- `提督！%s来进攻了！`（`0xc99f`）
- `提督！%s的舰队正在交战！支援吗？`（`0xc883`）

警戒 / 放哨是 ocean 遭遇模式，成功后才 `Combat.start`。AI 模块不实现视野圆。

### 2.3 舰队近战 AI（自动结算）

`fn_14000d7ab`（`0x140123d30`）是无画面近战，NPC 月结和贴舷都会走到：

1. 扫进攻侧 / 目标侧各 5 船，某侧有船首像（`CShip+0x53`）`== 0xa` 的船则对侧每船水手 `-= crew/10`（`01` §2.9 船首像表）
2. 两侧提督**统率力**（eSkill 1）比先手，平手归进攻侧（`01` §2.7）
3. 嵌套 `eAttackerFleetShip 0..4` × `eSide 0..1`，`s2B8` 初值 3 表示「战斗仍在进行」
4. 炮击段：`rand(500) <= 命中力 − 目标槽.c − 对方驾驶/5` 才命中；伤害 `炮威力*(3n+1)*(运气+500)/500`
5. 接舷段：`rand(200) <= 槽.c + 驾驶/5 − 目标槽.c/2 − 对方驾驶/10` 才接舷；双方按剑术互扣水手，各自船首像 `== 8`（王者像）则该方 ×1.2
6. 打沉 / 擒获对方**旗舰（槽 0）**立即 `s2B8 = 行动方`；本方旗舰水手打光 → 对方胜
7. 回合末：旗舰耐久或水手 ≤ 30 → 对方胜并记「退走」

完整伪码与常数见 `02-boarding.md` §2.7 `[确认]`。目标 = `fn_14001484e` 在对方存活槽里**均匀随机**挑一个（不是最近、不是旗舰优先）。这不是独立「AI 人格」，是同一套公式两边各跑，没有 `battle_algo` 分支。

### 2.4 战斗中的命令扫描（`fn_14000cdc9`）

`COceanScene` 战斗 tick 第二步。`0x1401bcbd0`，RTC `eFleetShip`：

`[确认]`（原始指令）：

1. `s30 = fn_1401ad010(fn_14000b87a(玩家), 0)`——由玩家当前位置/海域决定的**一个**值，不是每船命令
2. 先按 `s30 ∈ {0,1,3,5,8}` 做一次粗分（3 调 `fn_14000219e(..., 20)`，5 调 `(..., 10)`）
3. 对**玩家舰队** 5 船（`fn_14000afb5(玩家, i)`）：
   - 内层 `slot = 0..5`：场景里 6 个 happening 点（`fn_1400037a6(scene, slot)` 是否激活，`fn_1400090a7` 取坐标）
   - `dist2(船, 点) > hit_r2(船) + 100` skip（`hit_r2` 即 `01` 的 81/100/121）
   - 再 switch `s30`（`fn_14000c202` 是**扣**耐久，`fn_140014943` 是**扣**水手）：

| `s30` | 效果 |
| --- | --- |
| 2 | 耐久 −(rand(3)+5) |
| 3 | 耐久 −(rand(3)+10)，水手 −(rand(3)+5) |
| 4 | 只播特效 `fn_14018f440` / `fn_14018bca0` |
| 6 | 有改装 2（装甲）则只弹文案 `0x140560480`；否则耐久 −`maxhull/4`（`fn_1400084db`） |
| 5 | 同 6，免疫改装查 8 |

每次触发对 `fn_1400026d5(…, 100)` 记一笔（经验 `[推断]`）；旗舰（`i == 0`）额外播 `fn_14018b940(…, 100)`。

旧文把 2/3 写成「加」是符号错误。这是**场景障碍物 / 海域灾害**（`s30` 随海域变，6 个点有坐标，用船的碰撞半径判定）`[推断]`，不是 NPC 命令，也不碰敌船。`NUM_HAPPENING = 5` 与 2–6 五个分支同数，1:1 对应标 `[推断]`。

武装炮台着火（`0xf1eb`）`[确认]` 不在这里：它是被炮弹命中后的 `(n_battery+1)%` 判定，写在 `fn_14000ee8a`（见 `01` §2.9）。

### 2.5 另一处 `eHappening`

`fn_1400f2d90`（`0x1400f2d90`）：

```
flag = (arg >= 30)
roll = rand(200)
for eHappening in 0..19:
    if roll < table[eHappening]: break
```

循环 20，不是 5。RTC 名仍叫 `eHappening`。所在文件 `fn_1400ed7c0.cpp` 与舰队元素相邻，更像**航海随机事件表**，不是海战五命令。`num_from_rtc.json` 对 `eHappening` 的比较上界有 5、34、20。

重构：海战 happening 进 `combat/ai.yaml`；航海随机事件进 `ocean/encounters.yaml`。不要共用一张 20 行表当 5 个战斗命令。

### 2.6 目标选择（代码里实际在做的）

没有 `eSELECTTERGET` 的 switch。能看到的选目标行为：

| 地点 | 行为 |
| --- | --- |
| `fn_1400013ac`（`0x1401a7260`）`[确认]` | **实时海战的选目标**。每个就绪的炮控制器：① 有战斗城镇且本船舰队 kind 字节（`fn_1400115a9`：舰队 `+1` ∈ {0,1}）→ 先瞄城中心；② 否则 `eMobileShip 0..19` 依次找**第一艘**「存活 + `fn_140005cc2` 判敌对 + `fn_14000c8c4` 允许开火」的船；③ 城炮（无船）只打 kind ∈ {0,1} 的舰队船（`fn_140005b37`）。下标序，不是最近、不是旗舰 |
| `fn_14000c8c4` | 对指定目标算射程射界，不选目标 |
| `fn_14000ee8a` | 弹落点判定：覆盖落点、命中半径最小的船（不分敌我），否则 576 内的城 |
| `fn_14001484e` | 无画面结算：对方存活槽均匀随机 |
| `fn_14000cdc9` | 玩家船 × 6 个障碍点，距离过滤（不是选目标） |
| 文案 `0x17f07` | 「瞄准敌人的旗舰」 |
| 文案 `0x18a23` | 「要是炮击战就用Ｔ字战法」——职文，不是算法名 |

「选旗舰 / 选最近 / 选最残」在原版代码里**没有**。原版包的目标策略应写成 `first_hostile_in_range`（实时）和 `uniform_random_alive`（无画面），`nearest` / `flagship` 只能作为模组策略。玩家侧舰队有城必先打城。

### 2.7 势力月结里的战果

`[确认]`（`02-factions/05-war-failure-absorb.md` §2.4 / §2.8）：

- 战后 `fn_1401200a0` 用 `fn_14000d508` 取 `CForceElement+0x910` 的第一个空槽（`+8 == 0x1A`），按三种形状写 {势力 A, 势力 B, 港口（`0x62` = 无）, 旗 0/1}，`+0xC` = 当前月
- 月结 `CForceElement::vfunc_04`（`0x14010dc20`）扫 `eBattleResult 0..4`：`(当前月 + 9) % 12 == 槽.+0xC` 的槽由 `fn_140011518` 清空 → 战果保留 3 个月
- 「匹配当前势力」是旧稿误读：清除条件是月龄，不是势力

所以 `eBattleResult` 是**战果记录槽下标**，不是战斗结局枚举。槽由 factions 拥有；战斗只发 `BattleResolved { force_a, force_b, town_id, winner_side }`，factions 订阅后写槽。读这些槽的 AI 逻辑属 factions。

### 2.8 支援与旁观

文案允许「正在交战，支援吗？」。这是 ocean 遭遇选项，AI 不自动支援除非 `battle_algo` 以后证明会。接口留 `Combat.join(battle_id, fleet_id)`。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 空值 |
| --- | --- | --- | --- |
| 战斗算法 | `battle_algo_id` | `[未知]` | `INVALID_BATTLEALGO=-1` |
| 选目标 | `select_target_id` | `[未知]` | `INVALID_SELECTTERGET=-1` |
| 海战 happening | `happening` key | 5（`NUM_HAPPENING`，投票值；2–6 五个分支 `[推断]`） | `null` |
| 战果记录槽 | 属 factions | 5 | — |
| 场景舰队 | `mobile_fleet_id` | 4 | 0 玩家 `[推断]` |
| 场景船 | `mobile_ship_id` | 20 | |

运行时以表长为准。原版包可以只有 0 行 algo，NPC 用默认策略 `default_npc`。

## 4. 算法 / 公式

### 4.1 NPC 每 tick

```
algo = table.battle_algo[fleet.battle_algo_id] or default_npc
on gun_ready(gun):                                 # 原版：每个炮控制器就绪时
    target = select_target(algo.select_target_id, gun, snapshot)
    if target: Combat.fire(battle_id, gun, target)
```

原版包 `[确认]`：NPC 与玩家用**同一个**选目标函数 `fn_1400013ac`，没有命令权重：

```yaml
default_npc:
  select_target: first_hostile_in_range
  prefer_town_if_side_kind: [0, 1]                 # 舰队 +1 字节
```

### 4.2 目标策略（引擎提供，内容包点名）

```
first_hostile_in_range: mobile_ship 下标升序第一个 hostile 且 can_fire   # 原版实时
uniform_random_alive:   对方存活槽均匀                                   # 原版无画面
flagship:        对方 fleet_ship_id == 0 且仍能打                         # 模组
nearest_in_arc:  最小 dist2 且 can_fire                                  # 模组
```

`select_target_id` 一旦从 DLL 恢复，只改 YAML 映射，不改 C++。

### 4.3 场景 happening（原版包立即数）

来自 `fn_14000cdc9` `[确认]`，只作用于玩家舰队，按海域选一种：

数值表唯一出处是 `05-formation-happening.md` §4.3 的 `combat/happenings.yaml`（h2…h6，`[确认]`），本篇不重复。

`maxhull/4` 是 `(x + ((x>>31)&3)) >> 2`（向零取整的有符号 /4），即 `-25% 最大耐久`，不是当前耐久。

### 4.4 自动近战是否开打

```
if dist <= boarding_request_range and algo.allow_melee:
    emit BoardingRequested
```

贴舷距离原版未还原，用 `01` 的 `min_cannon_range`（900）作「炮打不了就尝试近战」的默认 `[推断]`，单独做成旋钮 `melee_engage_range`。

### 4.5 大地图 NPC 互打

```
if Relation.may_attack(a, b) and same_sea_area:
    Combat.resolve_offscreen(fleet_a, fleet_b)    # 走 fn_14000d7ab 同源公式
    emit BattleResolved { force_a, force_b, town_id: null, winner_side }   # factions 写战果槽
```

不进 `CCombatScreen`。结果仍发 `BattleResolved`，`loot` 可空。

## 5. 耦合分析

| 原版 | 重构 |
| --- | --- |
| algo / target 写在世界舰队 blob | `fleets/templates.yaml` 外键 `battle_algo_id` |
| `COceanScene` 里直接调三个 fn | Ocean 只 `BattleTick`；AI 与 Combat 订阅 |
| 战后直接写 `CForceElement+0x910`；月结按月龄清 | factions 订阅 `BattleResolved` 写槽、按 `retention_months` 清 |
| happening 与航海事件同名 RTC | 两张表 |
| 先手、500、30 溃败 | 全部用 `formulas/combat.yaml`，AI 不复制一份 |
| 「T 字战法」职文 | `ui/strings`，不是 algo 名 |

AI 禁止写市价、禁止直接 `Person.ability +=`。

## 6. 解耦后的配置

### `config/combat/ai.yaml`

```yaml
schema: dk4.combat.ai
version: 1
select_targets:
  - { key: first_hostile_in_range, vanilla_index: 0 }   # 原版实时
  - { key: uniform_random_alive, vanilla_index: 1 }     # 原版无画面
  - { key: flagship }                                   # 模组；无原版值
  - { key: nearest_in_arc }                             # 模组
algos:
  - key: vanilla:default_npc
    vanilla_index: 0
    select_target: first_hostile_in_range
    prefer_town_if_side_kind: [0, 1]
    allow_melee: true
    allow_town_fire: true
  # 原版 eBattleAlgo 行数未知：有对照后追加，vanilla_index 与 DLL 一致
# happenings 属场景，不属 algo：见 §4.3，放 config/combat/happenings.yaml
player_mobile_fleet_id: 0          # [推断]
npc_mobile_fleet_ids: [1, 2, 3]
skip_fleet_when: empty             # 原版 0x46 哨兵，见 ids/legacy_codec.yaml
```

### `config/formulas/combat.yaml`（AI 段）

```yaml
ai:
  impl: vanilla.ai
  params:
    melee_engage_range: 900          # [推断] 与 min_cannon_range 对齐
    offscreen_uses_same_melee: true   # 常数在 formulas/combat.yaml offscreen:
```

航海 20 槽随机表不放这里。

### `config/fleets/templates.yaml` 片段

```yaml
- key: vanilla:some_npc_fleet
  vanilla_index: 12
  force: vanilla:li
  battle_algo: vanilla:default_npc
  select_target: first_hostile_in_range
```

世界表 0x18 字节里哪一格是 algo：第 9 节。没对上之前全部 NPC 用 `default_npc`。

## 7. 独立旋钮

- 每个 algo 的目标策略、是否允许接舷 / 攻城（原版无命令权重）
- happening 五条的数值
- `player_mobile_fleet_id`
- 大地图互打是否复用同一套近战公式
- 目标策略列表（加 `lowest_hull` 只改 YAML）

改这些不碰贸易、不碰人物成长。把 `NUM_HAPPENING` 从 5 加到 8 只加表行。

## 8. 对外接口

查询：

```
AI.select_target(battle_id, gun_id) -> ship_id | town_id | none   # 原版粒度：每个炮控制器就绪时问一次
AI.plan(battle_id, fleet_id) -> [{ ship_id, command_id, target_id }]   # 模组扩展（移动/接舷意图），原版无
Relation.may_attack(force_id_a, force_id_b) -> bool   # factions（02-factions/05）
Ocean.fleets_in_area(sea_area_id) -> [fleet_id]
```

命令：

```
Combat.start(...)                  # ocean 警戒成功后
Combat.join(battle_id, fleet_id)   # 支援
Combat.resolve_offscreen(a, b)     # 月结
```

事件：

```
NpcBattleIntended  { fleet_id, target_fleet_id, sea_area_id }
HappeningFired     { battle_id, happening, ship_id }
BattleResolved     { ... }         # 与 01 同一事件
```

AI 模块可以对 `BattleTick` 调用 `AI.plan` 再 `Combat.issue`。不要让 AI 持有 `CBattleShip*`。

## 9. 未核实字段

| 项 | 事实 | 不要写成 |
| --- | --- | --- |
| `NUM_BATTLEALGO` | 断言存在，无数 | 5 种战术、T 字/横阵/追击 |
| `NUM_SELECTTERGET` | 同上 | 旗舰/最近/随机 已是原版枚举 |
| 舰队 raw[24] 哪一字节是 algo | 未命名 | 指着 `+0x12` 当战术 |
| happening 2–6 的玩法名 | 只有数值副作用 | 火攻、链弹、抓钩 |
| `fn_1400f2d90` 的 20 槽 | 与 `NUM_HAPPENING=5` 冲突 | 同一张 happening 表 |
| `fn_14018f440` | happening 4 只播特效 | 撞角确认 |
| `fn_1400091e2(ship, 2/8)` | happening 6/5 的免疫改装（2 = 装甲 `[推断]`） | 8 的具体改装件 |
| mobile fleet 0 = 玩家 | 循环从 1 起 | 确认
| `0x46` skip | 越界哨兵，字段语义 `[未知]` | 「已溃逃」 |
| 命令权重 | 未找到表 | 原版 NPC 爱打旗舰 |
| `aeBattleResult[5]`（`fn_14015b600`） | 战果记录槽下标（factions） | 结局枚举 |

职文「炮击战就用Ｔ字战法」（`0x18a23`）是角色说话，不是 `eBattleAlgo` 的一项。还原算法时对着 World DLL 断言和舰队 blob，不要按光荣其他作的阵型表来填。
