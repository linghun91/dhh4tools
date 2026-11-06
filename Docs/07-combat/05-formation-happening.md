# 海战阵型、命令与 happening

炮击物理在 `01-naval-battle.md`，接舷在 `02-boarding.md`，NPC 何时出招在 `03-battle-ai.md`。这一篇管 **五艘船怎么排、战斗 tick 里那条场景 happening 扫描写什么、`eSELECTTERGET` / `eBATTLEALGO` 的占位**。

原版没有 `NUM_FORMATION`，mesfile 也没有「鹤翼 / 鱼鳞 / 横阵」这种阵名。阵型就是 `eFleetShip` 0–4 的排列；槽 0 在战斗里当旗舰。`fn_14000cdc9` 的 2–6 是**由位置码决定的场景 happening**，不是命令号（§2.3 `[确认]`）。禁止用职文「Ｔ字战法」给算法起名。

## 1. 职责边界

本系统管：

- 舰队五槽排列（`CChangeFormationView` 写槽，战斗读槽）
- 战斗 tick 里按位置码触发的场景 happening 2–6（只作用于玩家舰队）
- `select_target_id`（`eSELECTTERGET`）与 `battle_algo_id`（`eBATTLEALGO`）的占位（策略本体在 `03`）

本系统不管：

- 炮弹飞行、射界、船体主伤害（`01`）
- 白刃 / 单挑（`02`）
- NPC 选目标、大地图互打、月结 `CBattleResult`（`03`）
- 航海 20 槽随机表（风暴 / 鼠 / 鲨，`ocean/encounters.yaml`）
- 地方舰队买货、委任命令（`fleets/`；那是「运到某港」，不是海战命令）
- 像素坐标、确认框文案（`ui/`）

玩家点「换阵」只发 `Fleets.setFormation`。战斗不 new `CChangeFormationView`。选目标属 `03`。

## 2. 原版事实

### 2.1 阵型是五槽排列，不是命名阵

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 无 `NUM_FORMATION` / `eFormation` 符号 | `[确认]` | `constants.json`、`*ID.h` 全表 |
| `CChangeFormationView` → `CShipArrangeView` → `CSurfaceSpriteView` | `[确认]` | `class_hierarchy.txt` |
| 同族还有 `CChangeProxyFleetView`、`CShipChangeView` | `[确认]` | 同上；都按 `eFleetShip` 排五艘 |
| 体积 `0x5c0` | `[确认]` | dtor `fn_14000750e(..., 0x5c0)` |
| 空船哨兵 `0xD0=208`（= 船实例池长 208，越界哨兵；D1 只进 legacy codec） | `[确认]` | ctor 写 `+0x4c4 = 0xd0`，再循环 `eFleetShip` 0–4 写 `+0x4b0 + i*4 = 0xd0` |
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
- 人物空槽 `0xec=236`（全仓统一的空人物哨兵 `[确认]`，`03-characters`；运行时 `null`）

战斗 cpp 里没有构造 `CChangeFormationView`。开战前（港口 / 甲板）改槽，战斗只读 `fleet_ship_id`。战斗中能否再打开该 View：`[未知]`，接口仍走 `Fleets.setFormation`，由场景图决定是否允许。

### 2.3 战斗 tick 第二条：场景 happening 扫描

`COceanScene` 在 `fn_140006c12 == 1` 时顺序：

1. `fn_14000660e` 接舷
2. **`fn_14000cdc9`（`0x1401bcbd0`）场景 happening**
3. `fn_14000ee8a` 炮弹

任一认为战斗结束就提前 return。来源：`COceanScene.cpp` 约 `0x1401a63a8`。

`fn_14000cdc9` RTC `eFleetShip`，`[确认]`（原始指令）：

```
code = fn_1401dd360(player + 0x30)            # 经 fn_14000b87a，玩家所在处的地形/海域码 [推断]
s30  = fn_1401ad010(code, 0)                  # 0xc→0 0xe→1 0x10→3 0xf→2 0x11→4 0x12→6 0x13→5 其它→8
if s30 in {0, 1, 8} or s30 < 0: return        # 无 happening
if s30 == 3: fn_14000219e(scene, 20, 0)
if s30 == 5: fn_14000219e(scene, 10, …)       # 场景特效 [推断]
for i in 0..4:                                # 玩家舰队 eFleetShip（fn_14000afb5(玩家, i)）
  ship = 该槽; if !ship or !ship.vfunc8: continue
  for slot in 0..5:                           # 场景里 6 个 happening 点
    if not fn_1400037a6(scene, slot): continue
    p = fn_1400090a7(scene, slot)             # 点坐标
    if dist2(ship, p) > hit_r2(ship) + 100: continue    # hit_r2 = 81/100/121（01 §2.9）
    switch s30: 2..6 → 见 §2.4
```

`s30` **不是命令号**：它由玩家所在位置的码查表得到，整场 tick 一个值；6 个点挂在场景上，不在船上；只扫玩家舰队。所以这是「场景障碍 / 海域灾害」，不是玩家或 NPC 下的命令 `[推断]`（码 0xc–0x13 的地形名 `[未知]`）。不要把海洋 HUD `aeOceanSceneButtonIndex` 0–4 写成 2–6。

### 2.4 happening 2–6 的副作用

`fn_14000c202(ship, x)` = `fn_140005731(ship, -x)`，`fn_140014943` 同理：**都是扣减** `[确认]`（炮弹命中也走 `fn_14000c202`，`01` §2.9）。旧文「SET 还是 ADD 未证明」「hull +=」作废。

| `s30` | 效果 `[确认]` | 旗舰（`i == 0`） | 非旗舰 |
| --- | --- | --- | --- |
| 2 | 耐久 −(rand(3)+5) | `fn_14018b940(…,100)` 两次 | `fn_140013f2f` 过了再 `fn_1400026d5(…,100)` |
| 3 | 耐久 −(rand(3)+10)；水手 −(rand(3)+5) | 同 2 | 同 2 |
| 4 | 无数值 | `fn_14018bca0(fn_14018f440)` | `fn_14018f440` |
| 6 | 音效 0x1b；有改装 2 则只弹文案 `0x140560480`；否则耐久 −`maxhull/4` | `fn_14018bca0(fn_14018f3d0)` | 同 2 |
| 5 | 同 6，免疫改装换成 **8** | 同 6 | 同 2 |

`maxhull` = `fn_1400084db(ship)`（`02` §2.7，最大耐久 `[推断]`）。`(x + ((x>>31)&3)) >> 2` 是编译器的有符号 /4，不是「+3 偏置」。

`fn_14018f440`（`0x14018f440`，0x6e 字节）：`fn_14000f0fb` 为假则 `fn_14000ab0f(..., 1)`；再取一个值，若 ∈ `[0,4]` 则 `fn_14000342c(..., 5)`。按 `03-characters` / `08-town-life/05` 的 setter 归属，前者写身体状态 1（疲劳）、后者写精神状态 5（混乱）`[推断]`——h4 是「人物身心」类 happening，不是撞角。

`fn_1400091e2(ship, n)` 是改装位图测试（`01` §2.6）。位 2 = 装甲 `[确认]`（`05-ships-fleet/04` 改装位域）；位 8 未对上 `[未知]`。

「武装炮台着火」（`0xf1eb`）`[确认]` 不在这里，在炮弹命中 `fn_14000ee8a`：`rand(100) <= 武装炮台数` 时船态写 0（`01` §2.9）。

### 2.5 `NUM_HAPPENING=5` 与航海 20 槽不是一张表

| 符号 / 循环 | 值 | 地点 | 归属 |
| --- | --- | --- | --- |
| `NUM_HAPPENING` | 5 | `num_values.json`（RTC 立即数 5 出现 6 次） | 本文件：场景 happening 2–6 `[推断]` |
| `eHappening >= 5` | 6 次 | `num_from_rtc.json` | 同上 `[推断]` |
| `eHappening >= 20` | 1 次 | `fn_1400f2d90` | **航海**遭遇 |
| `eHappening >= 34` | 2 次 | 同上文件一带 | `[未知]`，不并进战斗 |

`fn_1400f2d90`：`arg >= 30` 选表 1 否则表 0，`rand(200)`，`eHappening` 0..19 与表项比较。文案是暴风雨 / 鼠 / 鲨 / 海豚（`06-ocean/02-search-encounters.md`）。重构：海战 5 条进 `combat/happenings.yaml`；航海 20 条进 `ocean/encounters.yaml`。

`[确认]` `s30` 只来自位置码，**没有**玩家 / NPC 命令进入这个 switch。旧文「命令与 happening 同一表、`Combat.issue(2)` 走同一段」没有代码依据，作废。原版炮击路径里玩家和 NPC 都没有「命令号」：开火由炮控制器计时 + `fn_1400013ac` 自动选目标（`03` §2.6）。玩家输入（转向、是否接舷）走哪条函数本次未追 `[未知]`。

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
| `fn_1400013ac` | **实际选目标**：有城先打城，否则下标序第一个敌对且可开火的船（`03` §2.6） |
| `fn_14000cdc9` | 玩家船 × 6 个场景点，距离过滤（不是选目标） |
| 文案 `0x17f07` | 「瞄准敌人的旗舰」——职文 |
| 文案 `0x18a23` | 「要是炮击战就用Ｔ字战法」——职文，不是枚举项 |

原版包策略 `first_hostile_in_range`（实时）/ `uniform_random_alive`（无画面），其余是模组策略；`select_target_id` 从 DLL 恢复后只改 YAML 映射。

`eFleetAimKind` 是另一套「舰队目标种类」（委任去某港），不要和 `eSELECTTERGET` 合并。

### 2.7 旗舰槽与命令的耦合

happening 2–6 对 `eFleetShip == 0` 走旗舰特效，其它槽走 `fn_140013f2f` 守卫。无画面结算里打沉 / 擒获槽 0 即分胜负，回合末也读旗舰耐久 / 水手 ≤ 30（`02` §2.7）。**把哪艘船放到槽 0，就是改旗舰**。`CChangeFormationView` 确认后发的是槽排列；换旗舰清舱室担任者仍由 `fleets` 发 `FlagshipChanged`，人事系统执行（`02-fleet-proxy.md`）。

### 2.8 文案：有「下命令」，没有命令名录

| 偏移 | 文本 | 用途 |
| --- | --- | --- |
| `0x2fe3` | 必须决定下给舰队的命令和命令中的目标城镇 | **委任**，不是海战 |
| `0x5e4f` 起 | 提督，请从舰长室下命令 | 航海 |
| `0x17f07` | 瞄准敌人的旗舰 / 装上炮弹 | 战斗职文 |
| `0x1851b` | 在海战中不能使用「策略」 | 计略不出海战 |
| `0x18a23` | 要是炮击战就用Ｔ字战法 | 职文 |
| `0x191ff` | 如果要进入肉搏战，请您下命令 | 玩家可发接舷，输入路径 `[未知]` |
| `0xf1eb` | 武装炮台的火药着火 | `[确认]` 炮弹命中后的着火判定（`01` §2.9），与 happening 无关 |

没有一条 mesfile 把 happening 2–6 写成火攻 / 链弹 / 抓钩。名字保持 `h2` … `h6`。

`CIntoBattleView`（体积 `0x90`）是进入战斗确认，不是换阵、不是下命令。`CCombatScreen` 是海战 HUD 基类，ctor 写 1920×1080，不解释命令枚举。

## 3. 实体与 ID

| 实体 | ID | 原版 | 空值 |
| --- | --- | --- | --- |
| 阵型槽排列 | `formation_id` 运行时；内容包无目录 | 5 槽置换 | 槽内空船 `null`（原版 `0xD0`） |
| 场景 happening | `happening` key（h2…h6） | 有效 2–6；`NUM_HAPPENING=5`（投票值） | `null`（原版 0/1/8，`fn_1401ad010` 映射 `[确认]`） |
| 位置码 → happening | `terrain_code` | 0xc–0x13 七个码 | 其它 → 8 |
| 战斗算法 | `battle_algo_id` | `[未知]` 行数 | `INVALID_BATTLEALGO=-1` |
| 选目标 | `select_target_id` | `[未知]` 行数 | `INVALID_SELECTTERGET=-1` |
| 引擎目标策略 | `target_strategy_key` | 本文件命名，不是原枚举 | |
| 参战船位 | `fleet_ship_id` | 5 / 侧 | `INVALID_FLEETSHIP=-1`；槽 0 旗舰 |
| 改装查询 | `add_part_query` | `fn_1400091e2` 第二参 | |

运行时以表长为准。原版包：`happenings` 含 id 2–6；`battle_algo` 可以 0 行，NPC 用 `default_npc`。

`formation_id` 不要做成「鹤翼=1」。原版包可以只有一种 `slot_order`：长度为 `caps.fleet_ships` 的 `fleet_ship_id` 序列。

## 4. 算法 / 公式

全部立即数进 `config/formulas/combat.yaml` 的 `formation` 段；happening 数值进 `combat/happenings.yaml`。

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
formation:
  impl: vanilla.formation
  int_mode: vanilla_i32
  params:
    slots: 5
    lerp_steps: 8
    lerp_denom: 7                   # 传给 fn_140008e9f 的第四参
    flagship_slot: 0
```

`layout_stride` / `layout_base` 未还原成逻辑单位前，UI 自己播动画；逻辑层只提交槽置换。

### 4.2 happening 是否作用到该船

```
kind = terrain_to_happening[terrain_code(player_pos)]   # 原版 fn_1401ad010
if kind in none_ids: return
for ship in player_fleet:
  for point in scene.happening_points (len = points_per_scene):
    if not point.active: continue
    if dist2(ship, point) > hit_r2(ship) + range_slack2: continue
    apply(kind, ship, is_flagship = ship.slot == flagship_slot)
```

### 4.3 五种 happening（原版包）

名字全是占位，数值 `[确认]`。

```yaml
# config/combat/happenings.yaml
# 位置码属 ocean（原版整数码）；happening 用 key，「无」= null（原版 0/1/8）
terrain_to_happening: { 0x0c: null, 0x0e: null, 0x10: vanilla:h3, 0x0f: vanilla:h2, 0x11: vanilla:h4, 0x12: vanilla:h6, 0x13: vanilla:h5, default: null }
points_per_scene: 6
range_slack2: 100
pre_fx: { vanilla:h3: 20, vanilla:h5: 10 }      # fn_14000219e 第二参
happenings:
  - { key: vanilla:h2, vanilla_code: 2, hull_loss: { base: 5, rand: 3 } }
  - { key: vanilla:h3, vanilla_code: 3, hull_loss: { base: 10, rand: 3 }, crew_loss: { base: 5, rand: 3 } }
  - { key: vanilla:h4, vanilla_code: 4, person_state: { physical: vanilla:fatigued, mental_if_in: [0, 4], mental: vanilla:confused } }   # 原版身体 1 / 精神 5；setter 归属 [推断]
  - { key: vanilla:h5, vanilla_code: 5, sfx_id: 27, immune_add_part: vanilla:add_part_8, hull_loss_div_of_max: 4 }   # 位 8 名 [未知]
  - { key: vanilla:h6, vanilla_code: 6, sfx_id: 27, immune_add_part: vanilla:armor, hull_loss_div_of_max: 4 }
flagship_fx_arg: 100
exp_per_trigger: 100                 # [推断] 经验
```

实现：`hull_loss_div_of_max` 读 `Ships.max_hull`（派生量，D6），有符号除法；`immune_add_part` 是改装件 key，经 `Ships.has_add_part` 查，命中则 **不扣**，只发文案。

### 4.4 原版没有「海战命令」

`[确认]` 炮击路径没有命令号：开火由炮控制器计时（`01` §2.10），目标由 `fn_1400013ac` 自动选（`03` §2.6），接舷由船接触触发（`02` §2.2）。若新引擎要加玩家命令（集火、接舷、撤退），作为模组层的 `command_id` 设计，原版包为空表。

### 4.5 接舷

原版接舷由 `fn_1400052ef` 的接触检测触发（`02` §2.2），文案「如果要进入肉搏战，请您下命令」（`0x191ff`）的输入路径 `[未知]`。接口：

```
on contact(ship_a, ship_b): emit BoardingRequested
```

## 5. 耦合分析

| 原版纠缠 | 重构 |
| --- | --- |
| `CChangeFormationView` 直接改 `CFleetShip` | UI 发 `Fleets.setFormation`；战斗只读快照 |
| 槽 0 = 旗舰，换阵可能换旗舰 | `FlagshipChanged` → 人事清舱室 |
| `COceanScene` 里直接 `fn_14000cdc9` | Ocean 发 `BattleTick`；Combat 扫 happening 表 |
| 位置码 0xc–0x13 写死在 `fn_1401ad010` | `terrain_to_happening` 表，地形码属 `ocean/` |
| `fn_1400091e2(ship, 2/8)` | `Ships.has_add_part(ship_id, add_part_id)` |
| `fn_14000c202` 炮弹与 happening 共用 | 同一 `Combat.damage_hull`，happening 表只提供操作数 |
| algo / target 写在 0x18 舰队 blob | `fleets/templates.yaml` 外键；没对上之前全员 `default_npc` |
| `eHappening` RTC 名复用 | 两张表，禁止 20 行当 5 命令 |
| 「Ｔ字战法」职文 | `ui/strings` |
| `aeOceanSceneButtonIndex` 0–4 | `ui/scenes.yaml`，不是 `command_id` |
| 委任「给舰队下命令」 | `fleets/proxy_rules.yaml` 的 `orders` |

禁止：本系统写市价、写六维、在 View 里改存档船指针。

与 `03` 的分工：`03` 管选目标；本文件管阵型槽与场景 happening。两者不共享数值表。

与 `fleets/formations.yaml` 的分工：那份管港口排船 UI（`kinds: []`，lerp 步数）；本文件的 `combat/formations.yaml` 管战斗读槽、旗舰槽、空哨兵。lerp 参数两份必须同值，源在 `formulas/combat.yaml` `formation:`，舰队文档引用，不另写一份 8。

## 6. 解耦后的配置

### `config/combat/formations.yaml`

```yaml
schema: dk4.combat.formations
version: 1
slots: 5
flagship_slot: 0
empty_ship: null                  # 原版 0xD0 只在 ids/legacy_codec.yaml
# 原版没有命名阵。kinds 只描述「这 5 个槽的默认排列」
kinds:
  - key: vanilla:slot_order       # 原版无阵型枚举，不设 vanilla_index
    name_i18n: ui.formation.slot_order
    slot_map: [0, 1, 2, 3, 4]     # 身份；玩家改的是运行时状态
ui:
  view: CChangeFormationView
  arrange_base: CShipArrangeView
  confirm_menu: [0x108, 0x109]    # CMenuData id，不是 mesfile
  lerp_steps_ref: formulas.combat.formation.params.lerp_steps
```

加一种「预设站位」（例如内容包后来对上了 DLL）只追加 `kinds[]`。引擎按 `slot_map` 把 `ship_instance_id` 写进战斗快照。

### `config/combat/happenings.yaml`

见 §4.3。`name_i18n` 不进本表。

### `config/formulas/combat.yaml`（本篇追加段）

```yaml
formation:
  impl: vanilla.formation
  int_mode: vanilla_i32
  params:
    slots: $caps.fleet_ships
    lerp_steps: 8
    lerp_denom: 7
    flagship_slot: 0
```

### `config/combat/ai.yaml`

只在 `03-battle-ai.md` 定义，本篇不复制。

### `config/fleets/templates.yaml` 片段

```yaml
- key: vanilla:some_npc_fleet
  vanilla_index: 12
  force: vanilla:li
  battle_algo: vanilla:default_npc
  select_target: first_hostile_in_range
  # formation 运行时状态：五槽 ship_instance_id，不进模板 hex
```

## 7. 独立旋钮

改这些值不应迫使贸易、人物、炮弹物理改代码：

- `slots` / `flagship_slot` / `lerp_steps`
- happening 2–6 的 `base` / `rand` / `hull_loss_div_of_max` / `immune_add_part`
- `terrain_to_happening`、`range_slack2`、`points_per_scene`

加 happening：只在 `happenings.yaml` 追加行（新 key）并在 `terrain_to_happening` 指过去；数值逻辑走 D9 `Action { guard, roll, effects }` 形状，与 town / 叙事共用执行器。命名阵：只加 `formations.kinds[]`。

## 8. 对外接口

查询：

```
Fleets.formation(fleet_id) -> [ship_instance_id | null]    # 长度 = slots
Fleets.flagship(fleet_id) -> ship_instance_id
Ocean.terrain_code(pos) -> int                              # happening 选择
Ships.max_hull(ship_id) -> int
Ships.has_add_part(ship_id, add_part_id) -> bool
Combat.can_fire(shooter, target) -> bool                   # 01
```

命令：

```
Fleets.setFormation(fleet_id, slot_map)    # UI；可能间接触发 FlagshipChanged
Combat.tick_happenings(battle_id)          # 原版 fn_14000cdc9
```

事件：

```
FormationChanged  { fleet_id, slot_map, flagship_ship_id }
HappeningFired    { battle_id, happening, ship_id, hull_loss, crew_loss }   # 与 03 同一事件
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
| happening 2–6 的玩法名 | 数值已确认，名字无 | 火攻 / 链弹 / 抓钩 |
| `fn_1400091e2(ship, 2)` | 改装位 2 = 装甲 `[确认]`（`05-ships-fleet/04`） | — |
| `fn_1400091e2(ship, 8)` | happening 5 免疫件 | 船首像 / 某帆 |
| `fn_14018f440` | 值 ∈[0,4] 时调 `fn_14000342c(..., 5)` | 撞角 / 接舷确认 |
| `fn_14018f3d0` | 5/6 失败分支 | 进水动画确认 |
| `NUM_HAPPENING=5` 与 2–6 | 同宽 | 已证明 1:1 |
| `fn_1400f2d90` 的 20 / 34 | 另一套 RTC 名 | 同一 happening 表 |
| `NUM_BATTLEALGO` / `NUM_SELECTTERGET` | 断言存在，无数 | 5 种战术、旗舰/最近/随机已是原版枚举 |
| 舰队 raw[24] 哪一字节是 algo | 未命名 | `+0x12` 当战术 |
| 位置码 `0xc..0x13` 的地形名 | 映射已确认，码名未知 | 「暗礁 / 冰山」定论 |
| `aeOceanSceneButtonIndex` 0–4 | 大地图按钮 | 海战五命令 |
| 内层 slot 6 | 场景 6 个 happening 点 | 每船六门炮 |
| 接舷的玩家输入 | 文案有「下命令」 | 已是 happening 4 |

还原顺序：`fn_1401dd360` 的位置码来源（地图点种类？）；对着 World DLL 断言恢复 `NUM_BATTLEALGO` / `NUM_SELECTTERGET` 整数；舰队 0x18 blob 对上后再填 `templates.yaml` 外键。不要从光荣其它作的阵型表回填。
