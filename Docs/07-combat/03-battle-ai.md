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

| 符号 | 值 | 来源 |
| --- | --- | --- |
| `NUM_MOBILEFLEET` | 4 | `num_values.json` |
| `NUM_MOBILESHIP` | 20 | 同上；`fn_14000ee8a` `eMobileShip >= 0x14` |
| `NUM_FLEET` | 69 | 世界表 |

`fn_14000cd8d`（`0x140192400`，RTC `sChangeBattle` / `eMobileFleet` / `sEventTrigger`）：

- 循环 `eMobileFleet = 1; ... >= 4`
- 状态 `== 0x46`（70）则 skip
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

1. 扫进攻侧 / 目标侧各 5 船，看 `vfunc+0x48 == 0xa` 的特殊船
2. 两侧舰长敏捷比先手（`01` §2.7）
3. 嵌套 `eAttackerFleetShip 0..4` × `eSide 0..1`，`s2B8` 初值 3 表示「战斗仍在进行」
4. `rand(500)` 与派生值比较决定是否出手；命中则按 `(factor + 500)` 打耐久/水手；船态 8 则 ×2
5. 水手或耐久 ≤ 30 → 该侧 `s2B8 = eSide`，循环结束
6. 再扫一轮 `rand(200)` 的分支（`0xc8`），处理擒获 / 沉没文案

这不是独立「AI 人格」，是同一套近战公式两边各跑。AI 的差异如果存在，只可能来自 `battle_algo_id` 改命令权重——目前没有读到权重表。

### 2.4 战斗中的命令扫描（`fn_14000cdc9`）

`COceanScene` 战斗 tick 第二步。`0x1401bcbd0`，RTC `eFleetShip`：

1. `fn_1401ad010` 得到 `s30`
2. 先按 `s30 ∈ {0,1,3,5,8}` 做一次粗分（3 调 `fn_14000219e(..., 20)`，5 调 `(..., 10)`）
3. 对玩家侧 5 船：
   - 内层 `slot = 0..5`（6 步，与 `NUM_GUNTYPE` 同宽 `[推断]`）
   - `fn_1400037a6(ship, slot)` 为假则 skip
   - 距离 `> range + 100` skip
   - 再 switch `s30`：

| `s30` | 可见副作用 |
| --- | --- |
| 2 | 船体 `+ rand(3) + 5`（`fn_14000c202`）；旗舰额外特效 |
| 3 | 船体 `+ rand(3) + 10`，水手 `+ rand(3) + 5` |
| 4 | `fn_14018f440`（特效 / 撞角类 `[未知]`） |
| 6 | 若 `fn_1400091e2(ship, 2)` 失败：船体按当前值右移 2 位（约 1/4） |
| 5 | 类似 6，查询参数是 8 |

`NUM_HAPPENING = 5`（`num_values.json`）。上面恰好 5 个有效命令号 2–6。是否 1:1 对应标 `[推断]`。不要把这些号写成「火攻 / 链弹」除非 mesfile 对上。

武装炮台着火（`0xf1eb`）是文案，可能挂在命令 5/6 或城防，未在本函数字符串里出现。

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
| `fn_14000c8c4` | 对指定目标算射界，不选目标 |
| `fn_14000ee8a` | 每发弹在 `shell_acquire_limit` 内找最近船或城镇 |
| `fn_14000cdc9` | 对每艘己船扫 6 个 slot，距离过滤 |
| 文案 `0x17f07` | 「瞄准敌人的旗舰」 |
| 文案 `0x18a23` | 「要是炮击战就用Ｔ字战法」——职文，不是算法名 |

「选旗舰 / 选最近 / 选最残」如果存在，藏在未恢复的 `battle_algo` 表里。当前实现应对 NPC 使用**可配置的目标策略表**，原版包先给 `nearest` 与 `flagship` 两个策略，未证实的不要当原版行为宣传。

### 2.7 势力月结里的战果

`CForceElement::vfunc_04`（`0x14010dc20`）：

- 扫 `eForce 0..24`
- 再扫 `eBattleResult 0..4`，对象在 `this+0x910`
- 匹配当前势力则 `fn_140011518`（与 `CBattleResult::vfunc_05` 同源）

NPC 舰队在大地图上的胜负进这 5 个 `CBattleResult` 槽，不是玩家 HUD。`fn_14015b600` 也有 `aeBattleResult[5]`。玩家战斗结束同样要写 `CBattleResult`（存档对象），再由势力系统读 ID。

### 2.8 支援与旁观

文案允许「正在交战，支援吗？」。这是 ocean 遭遇选项，AI 不自动支援除非 `battle_algo` 以后证明会。接口留 `Combat.join(battle_id, fleet_id)`。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 空值 |
| --- | --- | --- | --- |
| 战斗算法 | `battle_algo_id` | `[未知]` | `INVALID_BATTLEALGO=-1` |
| 选目标 | `select_target_id` | `[未知]` | `INVALID_SELECTTERGET=-1` |
| 海战 happening / 命令 | `happening_id` | 5（`NUM_HAPPENING`） | |
| 战果槽 | `battle_result_id` | 5 | `INVALID_BATTLERESULT=-1` |
| 场景舰队 | `mobile_fleet_id` | 4 | 0 玩家 `[推断]` |
| 场景船 | `mobile_ship_id` | 20 | |

运行时以表长为准。原版包可以只有 0 行 algo，NPC 用默认策略 `default_npc`。

## 4. 算法 / 公式

### 4.1 NPC 每 tick

```
algo = table.battle_algo[fleet.battle_algo_id] or default_npc
target = select_target(algo.select_target_id, snapshot)
cmd    = weighted_pick(algo.command_weights)   # 原版权重 [未知]
Combat.issue(battle_id, ship_id, cmd, target)
```

原版包在权重未还原前：

```yaml
default_npc:
  select_target: nearest_in_arc
  command_weights: { 2: 1, 3: 1, 4: 1, 5: 1, 6: 1 }   # 均匀；[未知] 是否原版
```

均匀分布必须标成占位，不能写成「已还原」。

### 4.2 目标策略（引擎提供，内容包点名）

```
flagship:        对方 fleet_ship_id == 0 且仍能打
nearest_in_arc:  最小 dist2 且 can_fire
nearest_any:     最小 dist2（接舷用）
random_alive:    均匀
```

`select_target_id` 一旦从 DLL 恢复，只改 YAML 映射，不改 C++。

### 4.3 happening 5 命令（原版包立即数）

来自 `fn_14000cdc9`，名字全是占位：

```yaml
happenings:
  - id: 2
    key: h2
    hull: { add: 5, rand: 3 }
  - id: 3
    key: h3
    hull: { add: 10, rand: 3 }
    crew: { add: 5, rand: 3 }
  - id: 4
    key: h4
    special: fn_14018f440
  - id: 5
    key: h5
    hull_quarter_if_missing_query: 8
  - id: 6
    key: h6
    hull_quarter_if_missing_query: 2
range_slack: 100          # 距离 > range+100 则跳过
```

`hull_quarter` 是「当前耐久 + 3 再 >>2」那种脏算术的命名，实现时按反编译逐步对照，不要改成「-25%」。

### 4.4 自动近战是否开打

```
if dist <= boarding_request_range and algo.allow_melee:
    emit BoardingRequested
```

贴舷距离原版未还原，用 `01` 的 `min_cannon_range`（900）作「炮打不了就尝试近战」的默认 `[推断]`，单独做成旋钮 `melee_engage_range`。

### 4.5 大地图 NPC 互打

```
if factions.may_attack(a, b) and same_sea_area:
    Combat.resolve_offscreen(fleet_a, fleet_b)    # 走 fn_14000d7ab 同源公式
    write BattleResult slot on both forces
```

不进 `CCombatScreen`。结果仍发 `BattleResolved`，`loot` 可空。

## 5. 耦合分析

| 原版 | 重构 |
| --- | --- |
| algo / target 写在世界舰队 blob | `fleets/templates.yaml` 外键 `battle_algo_id` |
| `COceanScene` 里直接调三个 fn | Ocean 只 `BattleTick`；AI 与 Combat 订阅 |
| `CForceElement` 扫 5 个战果 | factions 订阅 `BattleResolved` |
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
  - { id: 0, key: nearest_in_arc }
  - { id: 1, key: flagship }
  - { id: 2, key: nearest_any }
  - { id: 3, key: random_alive }
algos:
  - id: 0
    key: default_npc
    select_target: nearest_in_arc
    command_weights: { 2: 1, 3: 1, 4: 1, 5: 1, 6: 1 }
    allow_melee: true
    allow_town_fire: true
  # 原版 eBattleAlgo 行数未知：有对照后追加，id 与 DLL 一致
happenings:
  - { id: 2, key: h2 }
  - { id: 3, key: h3 }
  - { id: 4, key: h4 }
  - { id: 5, key: h5 }
  - { id: 6, key: h6 }
player_mobile_fleet_id: 0          # [推断]
npc_mobile_fleet_ids: [1, 2, 3]
skip_fleet_state: 70               # 0x46
```

### `config/formulas/combat.yaml`（AI 段）

```yaml
ai:
  melee_engage_range: 900          # [推断] 与 min_cannon_range 对齐
  happening_range_slack: 100
  offscreen_uses_same_melee: true
  happening_h2_hull: { base: 5, rand: 3 }
  happening_h3_hull: { base: 10, rand: 3 }
  happening_h3_crew: { base: 5, rand: 3 }
```

航海 20 槽随机表不放这里。

### `config/fleets/templates.yaml` 片段

```yaml
- id: 12
  key: some_npc_fleet
  force_id: 3
  battle_algo_id: 0
  select_target_id: 0
```

世界表 0x18 字节里哪一格是 algo：第 9 节。没对上之前全部 NPC 用 `default_npc`。

## 7. 独立旋钮

- 每个 algo 的命令权重、是否允许接舷 / 攻城
- happening 五条的数值
- `player_mobile_fleet_id`
- 大地图互打是否复用同一套近战公式
- 目标策略列表（加 `lowest_hull` 只改 YAML）

改这些不碰贸易、不碰人物成长。把 `NUM_HAPPENING` 从 5 加到 8 只加表行。

## 8. 对外接口

查询：

```
AI.plan(battle_id, fleet_id) -> [{ ship_id, command_id, target_id }]
Factions.may_attack(force_id_a, force_id_b) -> bool
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
HappeningFired     { battle_id, happening_id, ship_id }
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
| `fn_14018f440` | 命令 4 调用 | 撞角确认 |
| `fn_1400091e2(ship, 2/8)` | 命令 5/6 的守卫 | 具体改装件 |
| mobile fleet 0 = 玩家 | 循环从 1 起 | 确认
| 船态 `0x46` | skip 入战 | 「已溃逃」 |
| 命令权重 | 未找到表 | 原版 NPC 爱打旗舰 |
| `aeBattleResult` 与玩家 HUD 五结果是否同一枚举 | 都是 5 | 一定相同 |

职文「炮击战就用Ｔ字战法」（`0x18a23`）是角色说话，不是 `eBattleAlgo` 的一项。还原算法时对着 World DLL 断言和舰队 blob，不要按光荣其他作的阵型表来填。
