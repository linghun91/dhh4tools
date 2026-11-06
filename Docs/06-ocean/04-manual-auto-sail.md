# 手动航行、自动航行与水手舱

大地图上玩家怎么把船从 A 开到 B。风速、洋流、帆种进 [F-OCN-01](../13-algorithms/01-formulas.md) 的式子，写在 [01-sailing.md](01-sailing.md) §4.3，本篇只引用，不重抄。航线边表属 world（[05-routes-navigation.md](../01-world/05-routes-navigation.md)）。本篇管：**三种航行模式**、舵与半开帆、水手/舱室怎么改航速、所谓「航行升级」到底是技能、改装帆还是开辟航线。

## 1. 职责边界

本系统管：

- 航行模式：`manual` / `auto_route` / `auto_town`
- 手动：舵轮输入、航向 `heading256`、半开/全开
- 自动：沿已建立 `route_id` 走一跳，或向目标 `town_id` 串多跳
- 水手人数、划桨手室、追加船舱/装甲对航速的**入口**（幅度进公式表，可空）
- 测量员缺席、航线未建立时的判定（只发事件，不内嵌汉字）
- 像素位移 vs 日历步：哪条路径走哪一种钟

不管：

- 顺风/逆风主公式、风力 CDF、无风气候 id（`01-sailing.md`，F-OCN-01 / F-OCN-02）
- 200 条航线的端点表、已建立状态的存档位（world）
- 海战里的驾驶术（战斗船对象初始化读过 `eSkill=2`，不在本 tick）
- 舱室怎么摆、谁坐哪间（`05-ships-fleet/03-cabins-crew-posts.md`）；本篇只订阅 `cabin_kind` 计数
- `CWheelView` / `CSpeedBarSprite` 的像素矩形（UI 投影）
- 疲劳每 tick 加减的整数（人物系统持有 `FATIGUE_*`；本篇只发事件）

场景切分：

- `COceanScene`（0x218）— 大地图：手动或沿当前航线跳像素移动
- `CSeaAreaScene`（`CRouteMap` 子类，0x128）— 海域图：点港口后的「自动移动」，按日历步走
- `CSelectTownRouteMap` — 选目标港；未选哨兵 98

## 2. 原版事实

### 2.1 tick 分叉 `[确认]`

`COceanScene::vfunc_02`（body `0x1401a5f90`，`srcdata/source/game/ocean/COceanScene.cpp`）每帧内层循环：`fn_140012625` 为真则 2 次，否则 1 次。该函数读场景 `+0x98`，`>0` 为真 `[推断]` 加速档，与 `01-sailing.md` 的 `ticks_per_frame_fast` 一致。

同一拍里，航速函数二选一（`COceanScene.cpp` `L_1401a6338`）：

```
obj = fn_140011630(scene)          # 16–45 字节包装，多半是当前舰队
scene_flag = fn_140006c12(obj)     # 16 字节 getter，反编译丢了返回值
auto_flag  = fn_140003689(obj)     # 同上

if scene_flag == 0 and auto_flag == 1:
    fn_140008814(scene)            # 自动：沿当前航线跳
else:
    fn_14001444d(scene)            # 手动：F-OCN-01
```

`fn_140008814` body `0x14018c420`（`srcdata/source/game/unassigned/fn_140188eb0.cpp`）。RTC：`vec`、`posFleet`、`posFlagShip`、`sLocus`、`iWindVelocity`、`cWindAngle`、`cShipAngle`。取风与手动相同（`fn_140014812`），**不**调用 `fn_14001444d`。

`fn_140006c12 == 1` 时 tick 另走 `fn_140003acb`（NPC `eMobileFleet` 1..3 的瞄向）并可能调 `fn_14000ee8a`。语义 `[未知]`，禁止当成第四种玩家航行模式。玩家三种模式见 §3。

### 2.2 手动：舵、帆、半开 `[确认]` 结构，`[推断]` 半开键

舵：

- `CWheelView` 体积 0x98。`vfunc_16`（`CWheelView.cpp`）：`fn_14000f26d(obj, 1)` 为真则**不**把输入交给舵；否则 `fn_14000dcb5`。后者 RTC `vector` / `ptCenter`，用指针位置相对轮心的向量改航向。
- `CWheelSprite` 体积 0xB0，轮面边 0x76；`+0x3c` 参与角计算 `[推断]` 当前航向投影。
- `01-sailing.md` 写「某标志为真时不把输入交给舵」：该标志就是 `fn_14000f26d` 的 **edx=1** 枝。

半开不是同一枝。`fn_14000f26d`（body `0x1401df420`）是：

```
fn_1401dd860(obj+8, k) > 0
```

`k` 是第二参。同一函数不同 `k`：

| `k` | 调用点 | 行为 |
| --- | --- | --- |
| 1 | `CWheelView::vfunc_16`、`fn_14001444d` 开头 | 锁舵；为真则跳过整段 F-OCN-01，只写速度条 |
| `0x11` | `fn_14001444d` 算出风帆速度之后 | `speed >>= 1` |
| 3 | `COceanScene::vfunc_05` | 选 SE 0x14 / 0x15 |

`01-sailing.md` 把所有 `fn_14000f26d` 收成一个 `half_speed_flag`。本篇拆开：`k=0x11` 才是半速；`k=1` 是「不在玩家操舵」。半开帆是否即 `k=0x11`：文案「减速时帆要“半开”。」（`mesfile_sc.txt` `0x16f03`）与 `>>= 1` 对得上 `[推断]`，未见到按钮写下标 0x11 的赋值，不升确认。

帆按钮：`fn_140002e1e`（`fn_140194480.cpp`）RTC `aeSailButton`、`aeOperateButton`、`eOSBI`。`aeSailButton = 6,7,8,9`，贴图 `OcFrm.DK4` 第 6–9 帧。循环 `eOSBI` 5..6 共 **2** 个帆按钮，每个 2 帧。`[推断]` 纵/横 × 开/半开的投影，不是第三种帆公式。操作按钮 `eOSBI` 7..0xA，帧号含 −1 空洞，名字 `[未知]`。

速度条：`CSpeedBarSprite` 从 `OcFrm.DK4` 取第 3 帧。`fn_140004250` 把速度写成 `sprite+0x38 = (speed + (edx&7)) >> 3` 再调 `vfunc+0x30`。`>> 3` 是显示标尺，不是玩法除数。

`COceanScene` 构造循环上界 5 / 2 / 4：5 船槽、2 帆按钮、4 操作按钮。`vfunc_14` RTC `aeOceanSceneButtonIndex` 字面量 0–4，五个主按钮中文名 `[未知]`。

### 2.3 自动航线跳 `[确认]`

`fn_140008814`：

1. 取旗舰 `cShipAngle`、气候风 `cWindAngle` / `iWindVelocity`
2. `fn_140001ece` 用风和船角（自动对准 `[推断]`，式子未还原）
3. `posFlagShip`、`posFleet`，差向量 `vec`，`sLocus` 一字节角
4. 速度条：某阈值内用已算长度，否则 `isqrt(vec²+…)`（`fn_14000baaf`）
5. 写 `COceanScene+0xe8`（速度条对象）

自动跳**仍然取风**，但不走 F-OCN-01 的横帆/纵帆分枝。重构不得在自动模式里偷偷改调 F-OCN-01，除非公式包显式写 `auto_uses_manual_speed: true`（原版包 false）。

航线对象：

- `CRouteNavigator` 体积 0x10。构造 `+0x8 = 0xc9`（201 = `NUM_ROUTE+1`）。`vfunc_01`：`+0x8 == 0xc9` 或 `+0xa != 7` 则否。`vfunc_03` 扫 `DAT_14429ac50`，循环 `eRoute < 0xc8`。
- 表步长 `[确认]` 0x1c：`fn_140029120` 调 `fn_140007dce(DAT_14429ac50, 0x1c, 0xc8, …)`（`fn_140027000.cpp`）。200 条 × 28 字节，在 exe 不在 World DLL。
- 选中一条边时 `fn_140002d9c`（`fn_1401f0ab0.cpp`）写入：

  | 偏移 | 正向匹配 | 反向匹配 |
  | --- | --- | --- |
  | `+0x8` | `route_id` | `route_id` |
  | `+0x9` | 0 | 5 |
  | `+0xa` | 表内一字节 | 表内一字节 |
  | `+0xb` | 5 | 0 |

  `+0xb == 0` 时 `vfunc_02` 走 `fn_140008882`，否则 `fn_1400123fa`：方向。`+0xa == 7` 是 `vfunc_01` 的「本跳可继续 / 已到位」条件 `[推断 进度字节]`。

### 2.4 自动到港 `[确认]` 对象，`[推断]` 与自动跳的关系

`CTownNavigator : CRouteNavigator`，体积 0x40。构造：`+0x10 = +0x11 = 0x62`（98，空港），并在 `+0x18` 构造 `CTownRoute`（析构尺寸 0x378，折线缓冲 `[未知]`）。

`CFleet` 构造在 `this+0x88` 嵌入 `CTownNavigator`（`CFleet.cpp` `fn_1401f34c0`）。存档 `vfunc_06/07` 把 `+0x88` 一并读写。自动到港状态挂在舰队上，不挂在场景指针上。

`fn_140002adb`（`fn_1401f0ab0.cpp`）：比较 `+0x10` 与 `+0x11` 决定方向，取下一 `route_id`；若不是 201，则 `CRouteNavigator::vfunc_02` 再 `fn_140002d9c` 装入下一跳。这是**多跳串接**。调用链：自动模式下补给检查通过后 `fn_14000e214` → `fn_140002adb`。像素位移仍是 `fn_140008814`。

`fn_1401f3aa0` RTC `eRoute`、`eTown`、`asConnect`（1552 字节）。先把每个港的 `asConnect` 写成 201、对端 98；再对 97 港 × 200 边做松弛。`s710 == 0x1a` 时跳过 `fn_14000a4fc`（已建立检查）。这是航线图上的最短路 `[推断]`，给自动到港填 hop 表。边是否已建立仍由 world 的 `established` 决定；本函数只读表。

选港 UI：`CSelectTownRouteMap` `+0x110 = 0x62`。点到没有边的港：只出文案，不移动。文案 key 见航线文档；同块还有「没有测量员。」（`mesfile_sc.txt` `0x5d0f`）。逻辑层两事件：`NavigatorMissing`、`RouteMissing`。不要把两句当成同一条件。

海域图快进：`CSeaAreaScene::vfunc_02` 的 `+0x8`：

| `+0x8` | 行为 |
| --- | --- |
| 1 | `fn_14000884b`；返回港 id ≠ 98 则切场景入港 |
| 2 | `fn_140004101`：读 `nMonth/nDay/nTime`，**仅当 `nTime == 0`** 重算补给 |

「终止自动移动吗？」（`0x3343`）是取消这条海域图自动移动。与大地图像素自动是同一套 `CTownNavigator` 状态、两种投影 `[推断]`。

### 2.5 水手舱、水兵室、划桨手室、SHIP_CREW

专家清单写「水手舱」。exe 没有这个四字舱名。要对上三件不同的东西：

| 东西 | 证据 | 改航速？ |
| --- | --- | --- |
| 水兵室 `cabin_kind` 7 | exe 舱室名第 8 项；`fn_140013b92` = `fn_140006b95(ship, 7)`；`CSelectShipDrawLine` 扫 7 | **否**。接舷容量，见战斗文档。最多 4 间（文案 `0xc503`） |
| 划桨手室 `cabin_kind` 11（0xb） | 名表第 12 项；`fn_140006b95` 按 `vfunc+0x38`（槽的种类）计数，edx 是种类不是槽号。`CTrade` 用 `0xa` 对货物仓库，种类下标与名表对齐 `[确认 对照]` | **是**。见下 |
| `SHIP_CREW_MIN/MAX` | `USHORT` 断言，十进制未进仓库 | 主公式里**没有**直接乘水手数。`CShip+0x4A/+0x4C` 候选船员或耐久，见 §9 |
| 追加船舱 / 装甲 | 「追加船舱速度会降低。」「追加装甲板……速度会降低。」（`0xc5d7` `0xc43f`） | 文案保证有罚；幅度 `[未知]` |
| 大鹫像 item 114 | 「船飞速前进」 | 船首像效果，属 items；幅度 `[未知]` |

`fn_140006b95`（body `0x14023e4c0`，RTC `eShipCabin`）：循环 31 槽，`vfunc+0x38(slot) == edx` 则 `count++`。edx=11 → 划桨手室间数。

划桨进入航速的两处（均在 `fn_14001444d` 之后段，即**手动** F-OCN-01）：

1. `fn_14000da9e`：`n = count(kind==11); n*n / rower_speed_div`（`idiv ecx`，除数丢失，YAML **null**）。当 `fn_140006c12==1` **或** `CFleet+0x71 < 0x32` 时 `speed +=` 该项。`+0x71` 是存档字节，语义 `[未知]`。
2. `fn_14000a466` 合成「额定」：船体类型 `+0x18` + 三根桅的加项（`fn_14000feac`，RTC `eShipMast`，循环 `< 3`）+ `count(kind==11)`（`fn_1400053ee`）。然后：

```
now = ship.vfunc+0x58()     # CShip::vfunc_11 → +0x4C
cap = fn_14000a466(ship)
if now < cap:
    speed = speed * (now + cap) / (2 * cap)    # 分母 2 已见；再无丢失 idiv
```

`01-sailing.md` 把 `now/cap` 写成耐久。战斗文档把 `vfunc+0x58` 写成水手人数。本篇**两边都不升确认**：`+0x4C` 是哪一个 `[未知]`。能确认的是 cap 含桅与划桨手室，不是单纯 `SHIP_DURABILITY_MAX`。

自动跳 `fn_140008814` **没有**调用 `fn_14000da9e` / `fn_14000a466`。划桨手室加成是手动公式的后处理。原版包：自动模式不吃这项，除非以后还原出自动内部等价式。

「有的船没有１名水手。要重新雇用水手或者重新编组。」（`0x3eb`）是出海检查 `crew_count`，不是航速。解雇全体「船就动不了」（`0x1323f`）同。

### 2.6 驾驶术、测量术 `[确认]` 岗位门闩，`[未知]` 天数曲线

十技能 id 见人物文档。本 tick **不**调用 `fn_14000e1ce(person, 2)` 或 `(person, 3)`。

驾驶术（`eSkill=2`，「驾驶的资质，航海术、造船能力。」）：

- 战斗船初始化 `fn_1400094d0` 把 `Ability(captain, 2)` 写进战斗对象 `+0xac`（同函数还写命中力、剑术）。那是海战，不是大地图。
- 人物 `ModifierSet.auto_sail_days` 是预留键。`COceanScene::vfunc_02` / `fn_140008814` / `fn_140002adb` 里没有「还能自动几天」的计数器。禁止编 `days = skill/10`。

测量术（`eSkill=3`）与测量员岗位：

- 无测量员：不能建航线。文案「没有测量员。」×8 +「测量员不在。」（`0x5d0f`）。这是**岗位空缺**，不是技能值。
- 有测量员但边未建立：「还没有建立到那座城市的航线。」及 7 条同义句（航线文档已收 key）。
- 手册阈值 120 进 `posts.yaml` 的 `required_skill`，标 `[推断]`，exe 立即数未钉死。
- `fn_14000e1ce(person, 3)` 出现在造船/城镇函数（`fn_140006cb7`、`fn_1400099ee`），不是开辟航线。

操帆手 / 操舵手：文案强调合作（`0x163ef` `0x164ab`）。F-OCN-01 是否乘进 `s58/s5C` `[未知]`。重构只读 `ModifierSet.sail_handling` / `helm`。

舰长室：海上命令必须从舰长室发（`0x5e4f`）。没有舰长室不能下自动/探索令，不改船速。

### 2.7 「航行升级」不是一棵树 `[确认]` 三件独立进度

原版没有名为「航行升级」的枚举或 View。玩家能变强的通道是三条，不要合成一条经验条：

1. **开辟航线**（world 状态）。发现/建立 `route_id` 之后，自动到港才能把该边当 hop。未建立 = `RouteMissing`。这是图的升级，不是速度升级。
2. **帆与改装**（ships）。船首帆/上帆/四角帆 → 横帆（顺风）；船尾帆/斜帆/三角帆 → 纵帆（逆风）。数值进 F-OCN-01 的 `square_sail` / `fore_sail` / extra。装甲、追加船舱降速（文案）。百分比 `[未知]`。
3. **技能与岗位**（characters）。驾驶术不进本 tick 航速。测量术只通过「有没有测量员」开门。操帆/操舵修正器键保留，曲线空。

人物等级 `isqrt(exp/30)` 不直接乘航速。

### 2.8 疲劳与两套钟 `[确认]` 入口，`[未知]` 增量

疲劳：符号 `FATIGUE_MIN/MAX` 约束 `UCHAR`。文案「疲劳度上升了%s。」「水手们好像很疲劳，不让他们休息一下吗？」。大厨「减轻疲劳。」温泉探索消疲劳。`COceanScene` / `CShip` / `fn_14001444d` / `fn_140008814` 均无疲劳写入。重构只发 `FatigueChanged`；`delta` 来自 `formulas/ocean.yaml`，原版包 `null`。

两套钟：

| 钟 | 谁走 | 证据 |
| --- | --- | --- |
| 像素 | `COceanScene` 每 tick 1 或 2 次，`fn_14001444d` 末尾用航向×速度加到坐标；自动用 `posFleet` 差向量 | 见 2.1–2.3 |
| 日历 | `nMonth/nDay/nTime`（`fn_140007e69`） | 海域图自动：`nTime==0` 才重算补给（`fn_140004101`）。`01-sailing.md`：补给按日历步，不按像素 |

大地图自动跳在补给通过后走 `fn_140002adb`（换 hop），**不是**在 `vfunc_02` 里直接 `nDay++`。谁在像素航行时推进 `nTime` `[未知]`，不要把「每 N 像素一天」写进原版包。

补给耗尽：`fn_140009f1b`（RTC `anSupplyAmount`）在 `auto_flag==1` 时调用；`fn_14000bcc1` 对 5 船两类补给求和，返回 3 `[推断]` 两类都有余。耗尽把 `COceanScene+0xa0` 写成 98。靠近港口且检查通过则 `+0xa0 = town_id` 并切场景。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key |
| --- | --- | --- | --- |
| 航行模式 | 无枚举；tick 分叉 + 海域图 `+0x8` | — | `sail_mode`：`manual` / `auto_route` / `auto_town` |
| 航线 | `eRoute` 200 | −1；运行时 201 | `route_id` |
| 港口 | `eTown` 97 | −1；Ocean 98 | `town_id` |
| 航向 | `cShipAngle` uint8 | — | `heading256` |
| 帆开度 | `fn_14000f26d` 的 `k` | — | `sail_opening`：`full` / `half` |
| 舱室种类 | `eCabinKind` | 0x17→0x18 别名 | `cabin_kind_id` |
| 划桨手室 | 名表下标 11 `[推断]` | 0 间合法 | `rower_room` |
| 水兵室 | 名表下标 7 `[推断]` | 最多 4 | `marine_room`（战斗订阅） |
| 水手人数 | `SHIP_CREW_*` USHORT | 0 不能开船 | `crew_count` |
| 测量员 | `ePost` surveyor | 空岗位 | `navigator_person_id` |
| 舰队导航器 | `CFleet+0x88` `CTownNavigator` | 港 98、线 201 | 不暴露指针 |

`sail_mode` 是运行时状态，进存档（原版嵌在 `CFleet+0x88` 与场景旗标里）。不要用 201 当「未建立航线」——那是「当前没选边」。未建立是 world 的布尔。

## 4. 算法 / 公式

魔数进 `config/formulas/ocean.yaml` 与本篇两个 YAML。代码只留参数名。F-OCN-01 的顺风/逆风/洋流/三处 `idiv` **全部**以 [01-sailing.md](01-sailing.md) §4.3 为准；`type_a_div` / `type_b_div` / `sea_area_speed_div` 继续 **null**。本篇只追加模式分派、半开键、划桨项。

### 4.1 模式分派

```
scene_flag = Fleet.scene_flag(fleet_id)      # 原版 fn_140006c12；1 时禁止当玩家自动
auto_on    = Fleet.auto_engaged(fleet_id)    # 原版 fn_140003689 == 1
dest       = Nav.destination(fleet_id)       # CTownNavigator +0x11；98 = 无
route      = Nav.current_route(fleet_id)     # +0x8；201 = 无

def tick_sail(fleet_id):
    if scene_flag == 1:
        speed = F-OCN-01(...)               # 仍算手动式；NPC 另走 fn_140003acb
        move_pixels(fleet_id, speed)
        return
    if auto_on and route != empty_route:
        auto_follow_route(fleet_id)         # fn_140008814
        if hop_complete:                    # +0xa 条件；原版 7
            Nav.advance_hop(fleet_id)       # fn_140002adb
        return
    speed = F-OCN-01(...)
    if helm_locked:                         # fn_14000f26d(k=1)
        write_speedbar(speed); return
    heading = Helm.heading256(fleet_id)     # CWheelView 本帧输入，LIVE
    move_pixels(fleet_id, heading, speed)
```

`auto_town` 不是第三套位移积分：它是「目标港 + hop 表」，每跳的像素仍是 `auto_follow_route`。海域图 `CSeaAreaScene+0x8 ∈ {1,2}` 是同一状态的日历投影：

```
on sea_area_auto_tick:
    if nTime == 0: drain_or_warn_supplies()
    if arrived(dest): emit ArrivedAtTown
```

取消：UI 问 `ui.ocean.cancel_auto_move`，确认后 `Nav.clear()`（港 98、线 201），回到 `manual`。

### 4.2 手动：F-OCN-01 + 半开 + 划桨

```
speed = sailing_speed(...)                  # F-OCN-01，含洋流 isqrt、三处 null idiv
if flag(k=0x11):                            # 半开 [推断]
    speed >>= half_speed_shift              # 原版 1
if scene_flag == 1 or fleet.byte_0x71 < low_stat_threshold:
    speed += rower_bonus(ship)
if now < cap:
    speed = speed * (now + cap) / (durability_denom_factor * cap)

rower_bonus(ship) = count(ship, rower_kind)^2 / rower_speed_div
# rower_speed_div: null。禁止猜 2 或 3。
# cap = hull_type_byte18 + mast_contrib(3) + count(rower_kind) + …
# now = CShip+0x4C；语义 [未知]
```

`count(ship, kind)` = 31 槽中种类相等的个数，上界 `caps.cabins_per_ship`。

操帆/操舵：`speed` 乘或不乘 `ModifierSet.sail_handling` / `helm` 由公式包 `apply_post_modifiers: false`（原版包，因为 F-OCN-01 体内没见到）控制。改 true 不改代码路径。

### 4.3 自动：跟线、不重算帆

```
wind = World.wind_at(x, y, calendar)        # 仍取
heading = align_to_locus(ship_angle, wind)  # fn_140001ece；式子 [未知]
delta = waypoint - fleet.pos                # posFlagShip / 当前 locator
speed_bar = pick(precomputed, isqrt(|delta|))
move toward waypoint
write_speedbar(speed_bar >> speedbar_shift) # 显示仍 >>3
```

自动到港：

```
def plot_auto_town(fleet_id, to_town_id) -> Result:
    if Characters.post(fleet_id, surveyor) is empty:
        emit NavigatorMissing { fleet_id }; return fail
    path = Routes.shortest(from_town(fleet_id), to_town_id, established_only=true)
    if path is empty:
        emit RouteMissing { fleet_id, from, to }; return fail
    Nav.set_hops(fleet_id, path)            # 写入 CTownNavigator / asConnect
    Fleet.set_auto(fleet_id, true)
    return ok

def advance_hop(fleet_id):
    nxt = Nav.next_route(fleet_id)          # 方向由 +0x10 ? +0x11
    if nxt == empty_route:
        # 已到目标港附近；入港仍看 arrival_radius（01 篇，值 null）
        return
    Nav.load_route(fleet_id, nxt)           # fn_140002d9c：写 +0x8/+0x9/+0xa/+0xb
```

`Routes.shortest` 的原版是 `fn_1401f3aa0` 的松弛。重构可用任何最短路，返回仍是 `route_id[]`。`established_only` 原版包 true；`s710==0x1a` 那条「跳过已建立检查」做成 `bypass_established_code: 26`，语义未还原，默认包不要用。

### 4.4 出海门闩（不是速度）

```
can_leave(fleet_id):
    if any_ship.crew_count == 0: refuse     # 「没有１名水手」
    return ok

can_plot(fleet_id, to):
    if no surveyor post: NavigatorMissing
    elif no established route: RouteMissing
    else ok
```

测量术技能值不出现在这两个谓词里。要加「技能 < 120 视同无测量员」，改 `posts.surveyor.required_skill`，Ocean 只问 `Characters.modifier_set.survey_unlock`。

### 4.5 日历 vs 像素

```
on ocean_pixel_tick:          # 1 或 2 次 / 帧
    tick_sail()
    # 不在这里减水粮、不加疲劳，除非公式包显式 drain_on_pixel: true（原版 false）

on calendar_step:             # 会话时钟；海域图 nTime==0 对齐
    drain_supplies()          # 01 篇 4.4，系数 null
    emit FatigueChanged { delta: formulas.ocean.fatigue_per_day }  # null
```

加速档只加倍**像素** tick，不加倍日历，除非以后还原出 `+0x98` 也推 `nTime`。

## 5. 耦合分析

原版硬编码：

- `COceanScene::vfunc_02` 内联手动/自动/补给/入港/遭遇
- `CFleet+0x88` 焊死 `CTownNavigator`；加一条航线要改 exe 静态表 `DAT_14429ac50`
- 测量员与「没有航线」挤在同一 mesfile 块
- 划桨手室种类下标 11 写在 `fn_14000da9e` 的立即数 0xb
- 半开与锁舵共用 `fn_14000f26d`，靠第二参区分
- 速度条 `>>3`、轮面 0x76、帆帧 6–9 写在 UI 构造
- `fn_140006c12==1` 把航行 tick 和疑似战斗函数拧在一起

重构切断：

- Ocean 只收 `SailingSnapshot` + `WindSnapshot` + `NavSnapshot`（当前 `route_id`、hop、from/to）
- 边表只问 `Routes.find` / `Routes.established`；最短路可替换
- 舱室间数问 `Ships.cabin_count(ship_id, kind_key)`，不读 `CShip*`
- 岗位问 `ModifierSet.survey_unlock`，不读六维
- 汉字全部 `ui/strings`
- `scene_flag==1` 的战斗叠层发 `FleetSpotted` / 交给 Combat，不在航行公式里调炮弹函数

## 6. 解耦后的配置

`config/ocean/sail_modes.yaml`：

```yaml
schema: dk4.ocean.sail_modes
version: 1
empty_town_sentinel: 98
empty_route_sentinel: 201
hop_done_locator: 7              # CRouteNavigator +0xa 比较值
bypass_established_code: 26      # fn_1401f3aa0 s710==0x1a；原版包禁用
speedbar_shift: 3                # 显示；不是 F-OCN-01 的 sail_shift
helm_lock_flag_k: 1              # fn_14000f26d edx
half_speed_flag_k: 17            # 0x11
wheel_face: 0x76                 # UI，可放 ui/scenes.yaml
modes:
  - key: manual
    name: 手动航行
    pixel_fn: F-OCN-01
    helm: true
    uses_rower_bonus: true
    auto_on: false
  - key: auto_route
    name: 沿航线自动
    pixel_fn: auto_follow_route    # fn_140008814
    helm: false
    uses_rower_bonus: false        # 原版自动跳未调划桨项
    needs: { current_route: true }
    auto_on: true
  - key: auto_town
    name: 向港口自动
    pixel_fn: auto_follow_route
    hop_fn: advance_hop            # fn_140002adb
    helm: false
    uses_rower_bonus: false
    needs: { surveyor: true, established_path: true, dest_town: true }
    sea_area_fast:
      scene_flag_values: [1, 2]    # CSeaAreaScene +0x8
      calendar_on_ntime0: true
    cancel_prompt: ui.ocean.cancel_auto_move
sail_opening:
  - key: full
    name: 全开
    half_speed: false
  - key: half
    name: 半开
    half_speed: true
    # 与 half_speed_flag_k 的对应 [推断]
```

`config/ocean/crew_sail.yaml`：

```yaml
schema: dk4.ocean.crew_sail
version: 1
# 舱室种类 id 与 exe 名表对齐；抽出器可改 id，key 稳定。
cabins:
  - key: marine_room
    cabin_kind_id: 7
    name: 水兵室
    max_per_ship: 4
    speed: { apply: false }
    notes: 接舷容量；Ocean 不读
  - key: cargo_room
    cabin_kind_id: 10
    name: 货物仓库
    speed: { apply: false }
  - key: rower_room
    cabin_kind_id: 11            # [推断] 划桨手室
    name: 划桨手室
    speed:
      apply: true
      modes: [manual]
      square_count: true         # n*n
      divisor: null              # rower_speed_div，idiv 丢失
      also_in_rating_cap: true   # fn_14000a466 加 n
crew_headcount:
  min: SHIP_CREW_MIN
  max: SHIP_CREW_MAX
  zero_blocks_departure: true
  in_F-OCN-01: unknown           # 不要写成耐久或水手直到 +0x4C 钉死
rating_cap:
  now_field: ship_u16_plus_0x4c  # vfunc_11
  includes: [hull_type_plus_0x18, masts_3, rower_room]
  scale_if_now_lt_cap: true
  denom_factor: 2
penalties:                       # 文案有、式子无
  extra_cabin_speed: null
  armor_speed: null
figureheads:
  - item_id: 114
    key: eagle
    name: 大鹫像
    speed_bonus: null            # 「飞速前进」
low_stat_threshold: 50           # CFleet+0x71；语义 [未知]
```

`config/formulas/ocean.yaml` 追加键（不删 01 篇已有项；三处原 idiv 保持 null）：

```yaml
# 下列为 04 篇追加。sailing_speed.type_*_div 仍 null。
half_speed_shift: 1
rower_speed_div: null
fatigue_per_day: null
drain_on_pixel: false
auto_uses_manual_speed: false
apply_post_modifiers: false
hop_done_locator: 7
```

`config/ui/strings.zh-CN.yaml` 片段：

```yaml
ui.ocean.cancel_auto_move: 终止自动移动吗？
ui.ocean.no_navigator: 没有测量员。
ui.ocean.route_not_established:
  - 还没有建立到那座城市的航线。
# 其余变体见 01-world/05-routes-navigation.md
```

岗位 / 技能阈值仍在 `config/characters/posts.yaml`（测量员 `required_skill: 120` `[推断]`）。Ocean 不复制。帆改装数值在 `config/ships/`。

## 7. 独立旋钮

改这些不应迫使贸易、战斗、人物改代码：

- 三种模式是否开放、自动是否改调 F-OCN-01
- 半开是否半速、锁舵用哪一个 flag k
- 划桨手室种类 id、`n²` 除数、是否计入 cap
- 水兵室是否开始影响航速（默认否）
- 出海是否检查 `crew_count==0`
- hop 完成字节、空港/空线哨兵（存档迁移表另写）
- 海域图自动是否跟 `nTime==0`
- 测量员缺席：禁命令 vs 只减精度（默认禁 `plot`）
- 加速档只加倍像素

改横帆纵帆数字走 `ships/`。改 200 条边走 `world/routes.yaml`。改驾驶术曲线走 `formulas/character.yaml` 的 `modifiers.auto_sail_days`（现 null）。

## 8. 对外接口

事件（本篇发出，01 篇已列的不重复定义）：

```
SailModeChanged { fleet_id, from, to }          # manual|auto_route|auto_town
HopAdvanced { fleet_id, route_id, dest_town_id }
AutoMoveCancelled { fleet_id }
NavigatorMissing { fleet_id }
RouteMissing { fleet_id, from_town_id, to_town_id }
ArrivedAtTown { fleet_id, town_id }             # 仍由 01 定义
FatigueChanged { fleet_id, delta }
```

查询：

```
Ocean.sail_mode(fleet_id) -> manual|auto_route|auto_town
Ocean.heading(fleet_id) -> heading256
Ocean.sail_opening(fleet_id) -> full|half
Ocean.nav(fleet_id) -> { from_town_id, to_town_id, route_id, hops: [route_id] }
Ocean.can_plot_route(fleet_id, to_town_id) -> { ok, reason }
Ocean.speed(fleet_id) -> int                    # 与 01 相同入口；内部按模式选函数
```

命令：

```
Ocean.helm { fleet_id, heading256 }             # manual；其它模式 no-op
Ocean.set_sail_opening { fleet_id, full|half }
Ocean.set_sail_mode { fleet_id, manual }        # 取消自动
Ocean.plot_route { fleet_id, route_id }         # auto_route；边须 established
Ocean.plot_town { fleet_id, to_town_id }        # auto_town；须测量员 + 路径
Ocean.advance_calendar_step {}                  # 会话时钟
```

`NavSnapshot`（Fleets 提供，Ocean 只读）：

```
{
  from_town_id,              # 98 = 无
  to_town_id,
  route_id,                  # 201 = 无
  locator,                   # +0xa
  direction_flag,            # +0xb
  hops: [route_id]
}
```

`SailingSnapshot` 仍按 01 篇；本篇要求其中 `crew` 与 `cabin_counts[kind_key]` 可用。禁止传 `CFleet*`、`CTownNavigator*`、`Location`。

UI 投影：

- `CWheelView` ← `heading256`；`helm_lock` 时忽略指针
- `CSpeedBarSprite` ← `Ocean.speed >> speedbar_shift`
- `CSelectTownRouteMap` ← 港列表 + `Routes.established`
- 海域图自动进度 ← `NavSnapshot` + 日历，不持有舰队对象

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| `fn_140006c12` / `fn_140003689` 读的字节 | 16 字节 getter，反编译空 | `[未知]` 语义；行为按比较值 0/1 钉死 |
| `fn_140006c12==1` 是否战斗叠层 | tick 内调 `fn_14000ee8a` | `[未知]` |
| `CRouteNavigator +0xa` 除「比较 7」以外的值域 | `fn_140002d9c` 从表写 | `[未知]` 进度还是 locator 种类 |
| `+0x9/+0xb` 的 0/5 | 方向 | `[推断]` 正向/反向 |
| `CTownRoute` 0x378 布局 | 析构尺寸 | `[未知]` |
| `asConnect` 1552 字节元素 | `fn_1401f3aa0` | `[未知]` 步长 |
| `s710==0x1a` 跳过已建立 | 立即数 26 | `[未知]` |
| `fn_14000f26d(k=0x11)` 是否半开按钮 | 文案 + `>>=1` | `[推断]` |
| 帆按钮 6–9 与纵/横/开/半的一一对应 | 2 按钮×2 帧 | `[推断]` |
| `aeOceanSceneButtonIndex` 0–4 名称 | `vfunc_14` | `[未知]` |
| `CShip+0x4A/+0x4C` 耐久还是水手 | 航行 vs 战斗文档冲突 | `[未知]` |
| `rower_speed_div` | `fn_14000da9e` `idiv` | `[未知]`，null |
| `CFleet+0x71` | 与 50 比较才加划桨项 | `[未知]` |
| 追加船舱 / 装甲降速幅度 | 仅文案 | `[未知]` |
| 大鹫像速度加成 | 仅文案 | `[未知]` |
| 驾驶术 → `auto_sail_days` 曲线 | ModifierSet 占位；本 tick 未读 skill 2 | `[未知]` |
| 测量员 120 | 手册 | `[推断]` |
| 操帆/操舵是否乘进 `s58/s5C` | 01 篇已标未知 | `[未知]` |
| 海上疲劳每 tick / 每日增量 | 仅文案 | `[未知]` |
| 像素航行谁推进 `nTime` | 不在 `vfunc_02` 体内 | `[未知]` |
| 入港半径 | 01 篇 | `[未知]` |
| F-OCN-01 三处 `idiv` | 01 篇 | `[未知]`，继续 null |
| `COceanScene+0x208` 声音 id | `vfunc_05` | `[未知]` |

`CLocator` 同时是港、村、结点的基类。hop 的途经点抽出前，`NavSnapshot.hops` 只存 `route_id`，折线可空。
