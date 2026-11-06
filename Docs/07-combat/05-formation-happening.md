# 海战阵型、命令与 happening

炮击物理在 `01-naval-battle.md`，接舷在 `02-boarding.md`，NPC 何时出招在 `03-battle-ai.md`。这一篇管 **五艘船怎么排、战斗 tick 里那条命令 / happening 扫描写什么、`eSELECTTERGET` / `eBATTLEALGO` 怎么接到命令表**。

原版没有 `NUM_FORMATION`，mesfile 也没有「鹤翼 / 鱼鳞 / 横阵」这种阵名。阵型就是 `eFleetShip` 0–4 的排列；槽 0 在战斗里当旗舰。命令号 2–6 与 `NUM_HAPPENING=5` 同宽，是否 1:1 标 `[推断]`。禁止用职文「Ｔ字战法」给算法起名。

## 1. 职责边界

本系统管：

- 舰队五槽排列（`CChangeFormationView` 写槽，战斗读槽）
- 玩家与 NPC 共用的海战命令表 `command_id` 2–6
- 战斗 tick 里按船扫描的 happening 副作用（同一套 2–6）
- `select_target_id`（`eSELECTTERGET`）与 `battle_algo_id`（`eBATTLEALGO`）到命令表的外键

本系统不管：

- 炮弹飞行、射界、船体主伤害（`01`）
- 白刃 / 单挑（`02`）
- NPC 权重怎么抽命令、大地图互打、月结 `CBattleResult`（`03`）
- 航海 20 槽随机表（风暴 / 鼠 / 鲨，`ocean/encounters.yaml`）
- 地方舰队买货、委任命令（`fleets/`；那是「运到某港」，不是海战命令）
- 像素坐标、确认框文案（`ui/`）

玩家点「换阵」只发 `Fleets.setFormation`。战斗不 new `CChangeFormationView`。NPC 只 `Combat.issue(command_id, target_id)`，不自己改耐久。

## 2. 原版事实

### 2.1 阵型是五槽排列，不是命名阵

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 无 `NUM_FORMATION` / `eFormation` 符号 | `[确认]` | `constants.json`、`*ID.h` 全表 |
| `CChangeFormationView` → `CShipArrangeView` → `CSurfaceSpriteView` | `[确认]` | `class_hierarchy.txt` |
| 同族还有 `CChangeProxyFleetView`、`CShipChangeView` | `[确认]` | 同上；都按 `eFleetShip` 排五艘 |
| 体积 `0x5c0` | `[确认]` | dtor `fn_14000750e(..., 0x5c0)` |
| 空船哨兵 `0xD0=208` | `[确认]` | ctor 写 `+0x4c4 = 0xd0`，再循环 `eFleetShip` 0–4 写 `+0x4b0 + i*4 = 0xd0` |
| 确认插值 8 步 | `[确认]` | `vfunc_11`：`s64 > 7` 结束；`fn_140008e9f(start, end, s64, 7)` |
| mesfile 无「阵型 / 队形」玩法条目 | `[确认]` | `mesfile_sc.txt`。「编组」是地方舰队编制；「Ｔ字战法」是职文 `0x18a23` |

`CShipArrangeView` ctor（`0x140232280`，RTC `eFleetShip` / `eShip`）还清：

- `+0x414[i] = i`（i = 0..4）身份排列
- `+0x428[i] = i+5`（第二排槽，10 个指针槽里的后 5 个 `[推断]`）
- `+0x270[5] = 0xD0`
- `+0x284[100] = 0xD0`（`eShip >= 0x64`，船坞 / 仓库侧池）
- 一长串屏幕坐标（`+0xb0` 起）

坐标属 UI，不进逻辑表。`CShipMoveButton`（体积 `0x200`）是拖船控件，无公式。

### 2.2 `CChangeFormationView` 放船 / 收回

`vfunc_11`（`0x140234510`，RTC `pos` / `posStart` / `posEnd`）：

1. `s20 = fn_140005867(...)` — 点击下标
2. **`s20 < 5`**：从阵上收回。`+0x264 -= 1`，`+0x268 += 1`
3. **`s20 >= 5`**：若 `+0x264 < 5`，取 `posStart`，终点

```
posEnd = table[+0x248][ (+0x244) * (+0x264) + (+0x258) ]
```

`+0x264` 是正在放的槽，`+0x244` 步长，`+0x258` 基址，`+0x248` 表指针。步长 / 基址是像素还是逻辑格 `[未知]`。

4. `s64 = 0..7` 共 8 帧：`fn_140008e9f(&posStart, &posEnd, s64, 7)` 线性插值（`denom <= 0` 则跳过）
5. 写回 `+0x414` / `+0x428`，`+0x264 += 1`，`+0x268 -= 1`
6. `+0x268 == 0` 则完成标志 `s98 = 1`，交给 `+0x490` 上的按钮

`vfunc_12`（`0x1402391e0`）确认：

- 当前选择与 `+0x4c4` 不同则弹菜单 `0x108` / `0x109`（`fn_14000fb14` / `fn_140009b1f`）。`CShipChangeView::vfunc_12` 用同一对 id，是 **CMenuData 号**，不是 mesfile `0x10843`（探险队遇野兽）
- 过了确认：`eFleetShip` 0..4 调 `fn_140014b50` / `fn_14000dad0` 把船写回舰队
- 人物空槽 `0xec=236`（与名册哨兵相同 `[推断]`）

战斗 cpp 里没有构造 `CChangeFormationView`。开战前（港口 / 甲板）改槽，战斗只读 `fleet_ship_id`。战斗中能否再打开该 View：`[未知]`，接口仍走 `Fleets.setFormation`，由场景图决定是否允许。

### 2.3 战斗 tick 第二条：命令扫描

`COceanScene` 在 `fn_140006c12 == 1` 时顺序：

1. `fn_14000660e` 接舷
2. **`fn_14000cdc9`（`0x1401bcbd0`）命令 / happening**
3. `fn_14000ee8a` 炮弹

任一认为战斗结束就提前 return。来源：`COceanScene.cpp` 约 `0x1401a63a8`。

`fn_14000cdc9` RTC `eFleetShip`：

```
s30 = fn_1401ad010(...)          # 当前命令号
# 粗分
if s30 in {0, 1, 8}: return      # 整段不扫船
if s30 == 3: fn_14000219e(..., 20)
if s30 == 5: fn_14000219e(..., 10)
# 然后无论 2–6 都进船循环（3/5 多一段特效）
for eFleetShip in 0..4:
  ship = 该槽
  if ship 无效: continue
  for slot in 0..5:              # 6 步，与 NUM_GUNTYPE 同宽 [推断]
    if not fn_1400037a6(ship, slot): continue
    dist = ...
    if dist > range + 100: continue   # 0x64
    switch s30:
      2, 3, 4, 5, 6 → 见下
      其它 → 只收尾
```

`s30 ∈ {0,1,8}` 不是 happening，是「本 tick 无命令」类状态。不要把海洋 HUD `aeOceanSceneButtonIndex` 0–4 写成命令 2–6：`COceanScene::vfunc_14` 那五个下标是大地图按钮，含义未恢复。

`fn_1401ad010`（`0x1401ad010`）按第二参 1/2 和一组立即数 `{0xc,0xe,0x10}` / `{0xf,0x11,0x12,0x13}` 分支，**返回值被反编译吃掉**。禁止把这些十六进制当成命令 2–6。

### 2.4 命令 2–6 的可见副作用

`fn_14000c202` 是写船体的同一入口，炮弹命中也走它（`01` §2.9）。happening 路径把一个立即数当第二参交进去。命令 5/6 的参数是「当前值右移 2 位」，只像 **SET 成约 1/4**；命令 2/3 的参数是 `rand(3)+base`，是 SET 成 5–12 还是 ADD，未证明。`03` 曾写成「hull +=」。本文件把 2/3 标 `[推断]` 增量，5/6 标 `[推断]` 四分之一，两边都进第 9 节。

| `s30` | 船体 / 水手 | 旗舰（`eFleetShip == 0`） | 其它 |
| --- | --- | --- | --- |
| 2 | `fn_14000c202(ship, rand(3)+5)` | `fn_14018b940(4, 100)` 两次 | 非旗舰：`fn_140013f2f` 过了再 `fn_1400026d5` |
| 3 | 船体 `rand(3)+10`；水手 `fn_140014943(ship, rand(3)+5)` | 同 2 的旗舰特效 | 粗分阶段已 `fn_14000219e(..., 20)` |
| 4 | 无直接耐久式 | 旗舰走 `fn_14018bca0(fn_14018f440, ...)` | 非旗舰直接 `fn_14018f440` |
| 6 | 先 `fn_14000cf1d(..., 0x1b, 2, 1)`（音效 / 特效 id 0x1b `[推断]`）。`fn_1400091e2(ship, 2)` **成功则只发文案**；失败则 `hull' = (hull + 3) >> 2` | 失败分支同样有 `fn_14018bca0(fn_14018f3d0)` | 查询 2 = 装甲 `[推断，见 ships/04]` |
| 5 | 同 6，查询参数换成 **8**；粗分已 `fn_14000219e(..., 10)` | 同 6 | `eAddParts` 8 的名字 `[未知]` |

`fn_14018f440`（`0x14018f440`，0x6e 字节）：`fn_14000f0fb` 为假则 `fn_14000ab0f(..., 1)`；再取一个值，若 ∈ `[0,4]` 则 `fn_14000342c(..., 5)`。5 与 `NUM_FLEETSHIP` 同宽 `[推断]`，不要写成撞角确认。

`fn_1400091e2(ship, n)` 在改装 UI 里按 `eAddParts` 查件（`fn_1400a1140.cpp`）。瞄准用 n=6/7 当船首炮 / 船尾炮（`01`）。本函数 n=2/8。2、3 在 `anAddArmorSizeTable` 循环里，2 更像装甲 `[推断]`。8 未对上。

每条分支开头 `fn_14000ca4a(..., DAT_140560xxx)` 是气泡文案。DAT 未抽成 mesfile 偏移，**不要**把「武装炮台着火」（`0xf1eb`）钉死在 5/6 上。

### 2.5 `NUM_HAPPENING=5` 与航海 20 槽不是一张表

| 符号 / 循环 | 值 | 地点 | 归属 |
| --- | --- | --- | --- |
| `NUM_HAPPENING` | 5 | `num_values.json`（RTC 立即数 5 出现 6 次） | 本文件：海战命令 2–6 |
| `eHappening >= 5` | 6 次 | `num_from_rtc.json` | 同上 `[推断]` |
| `eHappening >= 20` | 1 次 | `fn_1400f2d90` | **航海**遭遇 |
| `eHappening >= 34` | 2 次 | 同上文件一带 | `[未知]`，不并进战斗 |

`fn_1400f2d90`：`arg >= 30` 选表 1 否则表 0，`rand(200)`，`eHappening` 0..19 与表项比较。文案是暴风雨 / 鼠 / 鲨 / 海豚（`06-ocean/02-search-encounters.md`）。重构：海战 5 条进 `combat/commands.yaml`；航海 20 条进 `ocean/encounters.yaml`。

「命令」和「happening」在原版是同一 `s30` switch。玩家 `Combat.issue(2)` 与 NPC 抽到 2、以及随机 happening 抽到 2，走同一段副作用。配置用一张 `commands[]`，`happening: true` 只表示「也可被随机表抽中」，不要做两套数值。

### 2.6 `eSELECTTERGET` / `eBATTLEALGO`

World DLL `_wassert`（`asserts.txt`）：

```
( 0 <= eData && eData < NUM_BATTLEALGO ) || eData == INVALID_BATTLEALGO
( 0 <= eData && eData < NUM_SELECTTERGET ) || eData == INVALID_SELECTTERGET
```

`BattlealgoID.h` / `SelecttergetID.h` 的 `NUM_*` **没有整数**。`constants.json` 有 `BATTLEALGO_ID`、`SELECTTERGET_ID`。主程序 cpp 没有以这些枚举为 RTC 名的循环。

`SELECTTERGET` 是原拼写（Target）。含义 `[推断]`：世界舰队模板上的「怎么打」和「打谁」。World0 舰队步长 `0x18`，字段未命名（`05-ships-fleet/02-fleet-proxy.md` §9），**不能**指着 `+0x12` 说这就是 algo。

主程序能看到的选目标行为（没有 `eSELECTTERGET` switch）：

| 地点 | 行为 |
| --- | --- |
| `fn_14000c8c4` | 对**已经指定**的目标算射界 |
| `fn_14000ee8a` | 每发弹在 `shell_acquire_limit` 内找最近船或城镇 |
| `fn_14000cdc9` | 对每艘己船扫 6 个 slot，距离过滤 |
| 文案 `0x17f07` | 「瞄准敌人的旗舰」——职文 |
| 文案 `0x18a23` | 「要是炮击战就用Ｔ字战法」——职文，不是枚举项 |

原版包先提供引擎策略 `flagship` / `nearest_in_arc` / `nearest_any` / `random_alive`，`select_target_id` 从 DLL 恢复后只改 YAML 映射。

`eFleetAimKind` 是另一套「舰队目标种类」（委任去某港），不要和 `eSELECTTERGET` 合并。

### 2.7 旗舰槽与命令的耦合

命令 2/3/4 对 `eFleetShip == 0` 走旗舰特效，其它槽走 `fn_140013f2f` 守卫。溃败判定也读旗舰耐久 / 水手 ≤ 30（`01` §2.9）。**把哪艘船放到槽 0，就是改旗舰**。`CChangeFormationView` 确认后发的是槽排列；换旗舰清舱室担任者仍由 `fleets` 发 `FlagshipChanged`，人事系统执行（`02-fleet-proxy.md`）。

### 2.8 文案：有「下命令」，没有命令名录

| 偏移 | 文本 | 用途 |
| --- | --- | --- |
| `0x2fe3` | 必须决定下给舰队的命令和命令中的目标城镇 | **委任**，不是海战 |
| `0x5e4f` 起 | 提督，请从舰长室下命令 | 航海 |
| `0x17f07` | 瞄准敌人的旗舰 / 装上炮弹 | 战斗职文 |
| `0x1851b` | 在海战中不能使用「策略」 | 计略不出海战 |
| `0x18a23` | 要是炮击战就用Ｔ字战法 | 职文 |
| `0x191ff` | 如果要进入肉搏战，请您下命令 | 玩家可发接舷，命令号未对上 |
| `0xf1eb` | 武装炮台的火药着火 | 可能挂 5/6 或城防，未在 `fn_14000cdc9` 字符串里 |

没有一条 mesfile 把「命令二」写成火攻 / 链弹 / 抓钩。名字保持 `cmd_2` … `cmd_6`。

`CIntoBattleView`（体积 `0x90`）是进入战斗确认，不是换阵、不是下命令。`CCombatScreen` 是海战 HUD 基类，ctor 写 1920×1080，不解释命令枚举。

## 3. 实体与 ID

| 实体 | ID | 原版 | 空值 |
| --- | --- | --- | --- |
| 阵型槽排列 | `formation_id` 运行时；内容包无目录 | 5 槽置换 | 槽内空船 `ship.empty`（原版 `0xD0`） |
| 海战命令 / happening | `command_id` | 有效 2–6；`NUM_HAPPENING=5` | 0/1/8 = 本 tick 无命令 `[推断]` |
| 战斗算法 | `battle_algo_id` | `[未知]` 行数 | `INVALID_BATTLEALGO=-1` |
| 选目标 | `select_target_id` | `[未知]` 行数 | `INVALID_SELECTTERGET=-1` |
| 引擎目标策略 | `target_strategy_key` | 本文件命名，不是原枚举 | |
| 参战船位 | `fleet_ship_id` | 5 / 侧 | `INVALID_FLEETSHIP=-1`；槽 0 旗舰 |
| 改装查询 | `add_part_query` | `fn_1400091e2` 第二参 | |

运行时以表长为准。原版包：`commands` 至少含 id 2–6；`battle_algo` 可以 0 行，NPC 用 `default_npc`。

`formation_id` 不要做成「鹤翼=1」。原版包可以只有一种 `slot_order`：长度为 `caps.fleet_ships` 的 `fleet_ship_id` 序列。

## 4. 算法 / 公式

全部立即数进 `config/formulas/combat.yaml` 的 `formation` / `commands` 段。

### 4.1 阵型插值

```
# CChangeFormationView::vfunc_11
if click_index < slots:
    placed -= 1
    remaining += 1
else if placed < slots:
    posEnd = layout_base + layout_stride * placed
    for t in 0 .. lerp_steps-1:
        pos = lerp(posStart, posEnd, t, lerp_denom)   # denom = steps-1
    slot_map[placed] = chosen_ship
    placed += 1
    remaining -= 1
done = (remaining == 0)
```

```yaml
# formulas/combat.yaml  formation:
slots: 5
empty_ship: 208                 # 0xD0；重构用 ship.empty
lerp_steps: 8
lerp_denom: 7                   # 传给 fn_140008e9f 的第四参
flagship_slot: 0
```

`layout_stride` / `layout_base` 未还原成逻辑单位前，UI 自己播动画；逻辑层只提交槽置换。

### 4.2 命令是否作用到该船

```
can_apply(ship, command_id, slot, dist, range):
  if command_id in skip_ids: return false          # vanilla {0,1,8}
  if not gun_slot_ready(ship, slot): return false  # fn_1400037a6
  if dist > range + range_slack: return false      # slack=100
  return true
```

`slot` 循环 0..`gun_slots-1`（原版 6）。这不是「每船六门炮」的确认（`01` 里 5 与 21 并存），只是本扫描的内层上界。

### 4.3 五条命令（原版包）

名字全是占位。`hull_write` 的语义见第 9 节。

```yaml
# formulas/combat.yaml  commands:
range_slack: 100
rand_span_3: 3
skip_ids: [0, 1, 8]
pre_fx:
  3: { fn_14000219e_arg: 20 }
  5: { fn_14000219e_arg: 10 }
flagship_fx: { id: 4, arg: 100, times: 2 }
cmd_2:
  hull_write: { base: 5, rand: 3 }     # fn_14000c202(rand(3)+5)
cmd_3:
  hull_write: { base: 10, rand: 3 }
  crew_write: { base: 5, rand: 3 }
cmd_4:
  special: fn_14018f440
cmd_5:
  sfx: 0x1b
  skip_if_add_part: 8
  hull_shift: 2                        # (hull + hull_shift_bias) >> 2
  hull_shift_bias: 3
cmd_6:
  sfx: 0x1b
  skip_if_add_part: 2                  # [推断] 装甲
  hull_shift: 2
  hull_shift_bias: 3
```

实现：`hull_shift` 先读当前耐久，饱和后再右移，禁止无符号回绕。`skip_if_add_part` 成功则 **不写耐久**，只发文案。

### 4.4 NPC 怎么挑命令（本文件只定接口）

```
algo = table.battle_algo[fleet.battle_algo_id] or default_npc
target = select_target(algo.select_target_id, snapshot)
cmd = weighted_pick(algo.command_weights)          # 权重 [未知]
Combat.issue(battle_id, ship_id, cmd, target)
```

权重未还原前原版包均匀占位，必须标 `unverified: true`。策略表与 `03` 同一份，不在本文件复制近战公式。

```
flagship:        fleet_ship_id == flagship_slot 且仍能打
nearest_in_arc:  最小 dist2 且 Combat.can_fire
nearest_any:     最小 dist2（接舷）
random_alive:    均匀
```

### 4.5 接舷命令

文案允许玩家「下命令」进肉搏。命令号未对上。接口：

```
if command_id == cmd_board or (dist <= melee_engage_range and algo.allow_melee):
    emit BoardingRequested
```

`cmd_board` 在原版包先空着，对上 `s30` 再填。贴舷距离默认跟 `01` 的 `min_cannon_range`（900）走，旋钮名 `melee_engage_range`，属 `03`。

## 5. 耦合分析

| 原版纠缠 | 重构 |
| --- | --- |
| `CChangeFormationView` 直接改 `CFleetShip` | UI 发 `Fleets.setFormation`；战斗只读快照 |
| 槽 0 = 旗舰，换阵可能换旗舰 | `FlagshipChanged` → 人事清舱室 |
| `COceanScene` 里直接 `fn_14000cdc9` | Ocean 发 `BattleTick`；Combat 扫命令表 |
| `fn_1400091e2(ship, 2/8)` | `Ships.has_add_part(ship_id, add_part_id)` |
| `fn_14000c202` 炮弹与 happening 共用 | 同一 `Combat.write_hull`，命令表只提供操作数 |
| algo / target 写在 0x18 舰队 blob | `fleets/templates.yaml` 外键；没对上之前全员 `default_npc` |
| `eHappening` RTC 名复用 | 两张表，禁止 20 行当 5 命令 |
| 「Ｔ字战法」职文 | `ui/strings` |
| `aeOceanSceneButtonIndex` 0–4 | `ui/scenes.yaml`，不是 `command_id` |
| 委任「给舰队下命令」 | `fleets/proxy_rules.yaml` 的 `orders` |

禁止：本系统写市价、写六维、在 View 里改存档船指针。

与 `03` 的分工：`03` 决定 **谁在何时** `issue`；本文件决定 **issue 之后写什么**。`ai.yaml` 的 `command_weights` 只引用 `command_id`，不复制 `hull_write`。

与 `fleets/formations.yaml` 的分工：那份管港口排船 UI（`kinds: []`，lerp 步数）；本文件的 `combat/formations.yaml` 管战斗读槽、旗舰槽、空哨兵。lerp 参数两份必须同值，源在 `formulas/combat.yaml` `formation:`，舰队文档引用，不另写一份 8。

## 6. 解耦后的配置

### `config/combat/formations.yaml`

```yaml
schema: dk4.combat.formations
version: 1
slots: 5
flagship_slot: 0
empty_ship: ship.empty            # vanilla 0xD0=208
# 原版没有命名阵。kinds 只描述「这 5 个槽的默认排列」
kinds:
  - id: 0
    key: slot_order
    name_i18n: ui.formation.slot_order
    slot_map: [0, 1, 2, 3, 4]     # 身份；玩家改的是运行时状态
ui:
  view: CChangeFormationView
  arrange_base: CShipArrangeView
  confirm_menu: [0x108, 0x109]    # CMenuData id，不是 mesfile
  lerp_steps_ref: formulas.combat.formation.lerp_steps
```

加一种「预设站位」（例如内容包后来对上了 DLL）只追加 `kinds[]`。引擎按 `slot_map` 把 `ship_instance_id` 写进战斗快照。

### `config/combat/commands.yaml`

```yaml
schema: dk4.combat.commands
version: 1
skip_ids: [0, 1, 8]
gun_scan_slots: 6                 # 内层循环；[推断] 非每船炮数
commands:
  - id: 2
    key: cmd_2
    happening: true
    hull_write: { base: 5, rand: 3 }
    flagship_extra_fx: true
  - id: 3
    key: cmd_3
    happening: true
    hull_write: { base: 10, rand: 3 }
    crew_write: { base: 5, rand: 3 }
    pre_fx_arg: 20
    flagship_extra_fx: true
  - id: 4
    key: cmd_4
    happening: true
    special: ram_or_unknown       # 只占位；实现调 Ships/Boarding 查询
    flagship_extra_fx: true
  - id: 5
    key: cmd_5
    happening: true
    sfx_id: 27                    # 0x1b
    skip_if_add_part: 8           # 名字未恢复，保留整数
    hull_quarter: true
    pre_fx_arg: 10
  - id: 6
    key: cmd_6
    happening: true
    sfx_id: 27
    skip_if_add_part: 2           # [推断] 装甲
    hull_quarter: true
# 接舷命令号未对上：先不占 2–6
# board: { id: null, key: cmd_board }
```

`name_i18n` 不进本表。随机 happening 与玩家命令共用这些行。

### `config/formulas/combat.yaml`（本篇追加段）

```yaml
formation:
  slots: 5
  empty_ship_sentinel: 208
  lerp_steps: 8
  lerp_denom: 7
  flagship_slot: 0
commands:
  range_slack: 100
  rand_span_3: 3
  skip_ids: [0, 1, 8]
  hull_shift: 2
  hull_shift_bias: 3
  flagship_fx_arg: 100
  sfx_1b: 27
  add_part_cmd6: 2
  add_part_cmd5: 8
```

### `config/combat/ai.yaml`（只引用，不复制数值）

```yaml
# 详见 03-battle-ai.md。此处只强调外键：
select_targets:
  - { id: 0, key: nearest_in_arc }
  - { id: 1, key: flagship }
  - { id: 2, key: nearest_any }
  - { id: 3, key: random_alive }
algos:
  - id: 0
    key: default_npc
    select_target: nearest_in_arc
    command_weights: { 2: 1, 3: 1, 4: 1, 5: 1, 6: 1 }   # unverified
    allow_melee: true
```

`command_weights` 的键必须存在于 `commands.yaml`。

### `config/fleets/templates.yaml` 片段

```yaml
- id: 12
  key: some_npc_fleet
  force_id: 3
  battle_algo_id: 0
  select_target_id: 0
  # formation 运行时状态：五槽 ship_instance_id，不进模板 hex
```

## 7. 独立旋钮

改这些值不应迫使贸易、人物、炮弹物理改代码：

- `slots` / `flagship_slot` / `lerp_steps`
- 命令 2–6 的 `base` / `rand` / `hull_shift` / `skip_if_add_part`
- `range_slack`、`gun_scan_slots`
- `skip_ids`
- 目标策略列表（加 `lowest_hull` 只改 YAML）
- 每个 algo 的 `command_weights`

把 `NUM_HAPPENING` 从 5 加到 8：只在 `commands.yaml` 追加行，扫描按表长。命名阵：只加 `formations.kinds[]`，不要改 `fn_14000cdc9` 的命令号。

## 8. 对外接口

查询：

```
Fleets.formation(fleet_id) -> [ship_instance_id | empty]   # 长度 = slots
Fleets.flagship(fleet_id) -> ship_instance_id
Combat.command(command_id) -> CommandRecord
AI.plan(battle_id, fleet_id) -> [{ ship_id, command_id, target_id }]   # 03
Ships.has_add_part(ship_id, add_part_id) -> bool
Combat.can_fire(shooter, target) -> bool                   # 01
```

命令：

```
Fleets.setFormation(fleet_id, slot_map)    # UI；可能间接触发 FlagshipChanged
Combat.issue(battle_id, fleet_ship_id, command_id, target_id)
Combat.tick_commands(battle_id)            # 原版 fn_14000cdc9
```

事件：

```
FormationChanged  { fleet_id, slot_map, flagship_ship_id }
CommandIssued     { battle_id, fleet_id, fleet_ship_id, command_id, target_id }
HappeningFired    { battle_id, command_id, ship_id }      # 与 03 同一事件
ShipHullChanged   { ... }                                  # 01 已定义
BoardingRequested { ... }                                  # 02
```

载荷只有 ID 与值。禁止把 `CChangeFormationView*` 或 `CBattleShip*` 传出战斗模块。

## 9. 未核实字段

| 项 | 事实 | 不要写成 |
| --- | --- | --- |
| 命名阵种类 | 无符号、无文案 | 鹤翼 / 鱼鳞 / 横阵 / Ｔ字 |
| `CChangeFormationView` 能否在战斗中打开 | 战斗 tick 不构造它 | 「海战中换阵是原版操作」 |
| `+0x244` / `+0x258` / `+0x248` | 进终点坐标 | 海里间距、炮术格 |
| `+0x428[i] = i+5` | 10 槽里的后 5 | 「敌方五艘」 |
| 菜单 `0x108` / `0x109` | 与换船 UI 相同 | mesfile 探险遇兽 |
| `fn_14000c202` 对命令 2/3 | 第二参 `rand(3)+base` | 已还原的修理或秒杀 |
| 命令 5/6 的 `>>2` | 当前值右移 | 「-25%」或「着火」 |
| `fn_1400091e2(ship, 2)` | 改装查询；2 像装甲 | 确认是装甲板 |
| `fn_1400091e2(ship, 8)` | 命令 5 守卫 | 船首像 / 某帆 |
| `fn_14018f440` | 值 ∈[0,4] 时调 `fn_14000342c(..., 5)` | 撞角 / 接舷确认 |
| `fn_14018f3d0` | 5/6 失败分支 | 进水动画确认 |
| `NUM_HAPPENING=5` 与 2–6 | 同宽 | 已证明 1:1 |
| `fn_1400f2d90` 的 20 / 34 | 另一套 RTC 名 | 同一 happening 表 |
| `NUM_BATTLEALGO` / `NUM_SELECTTERGET` | 断言存在，无数 | 5 种战术、旗舰/最近/随机已是原版枚举 |
| 舰队 raw[24] 哪一字节是 algo | 未命名 | `+0x12` 当战术 |
| `fn_1401ad010` 的 `0xc..0x13` | 分支在，返回值丢失 | 就是命令 2–6 |
| `aeOceanSceneButtonIndex` 0–4 | 大地图按钮 | 海战五命令 |
| 命令权重 | 未找到表 | 原版 NPC 爱打旗舰 |
| 内层 slot 6 | 与 `NUM_GUNTYPE` 同宽 | 每船六门炮 |
| `s30 ∈ {0,1,8}` | 整段 return | 「待机 / 逃走 / 防御」定论 |
| 接舷的 `command_id` | 文案有「下命令」 | 已是命令 4 |

还原顺序：对着 `0x1401bcbd0` 下断点看 `s30` 从哪写入；对着 World DLL 断言恢复 `NUM_BATTLEALGO` / `NUM_SELECTTERGET` 整数；舰队 0x18 blob 对上后再填 `templates.yaml` 外键。不要从光荣其它作的阵型表回填。
