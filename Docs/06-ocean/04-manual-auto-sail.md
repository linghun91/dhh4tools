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

- 顺风/逆风主公式、风力 CDF、无风结局 key（`01-sailing.md`，F-OCN-01 / F-OCN-02）
- 200 条航线的端点表、可见 / 可走谓词与多跳寻路（world，`01-world/05-routes-navigation.md`；原版没有「已建立」存档位）
- 海战里的驾驶术（战斗船对象初始化读过 `eSkill=2`，不在本 tick）
- 舱室怎么摆、谁坐哪间（`05-ships-fleet/03-cabins-crew-posts.md`）；本篇只订阅 `cabin_kind` 计数
- `CWheelView` / `CSpeedBarSprite` 的像素矩形（UI 投影）
- 疲劳日增量（`01-sailing.md` §2；状态归 `03-characters/07`）

场景切分：

- `COceanScene`（0x218）— 大地图：手动或沿当前航线跳像素移动
- `CSeaAreaScene`（`CRouteMap` 子类，0x128）— 海域图：点港口后的「自动移动」，按日历步走
- `CSelectTownRouteMap` — 选目标港；未选 = `null`（原版 98）

## 2. 原版事实

### 2.1 tick 分叉 `[确认]`

`COceanScene::vfunc_02`（body `0x1401a5f90`，`srcdata/source/game/ocean/COceanScene.cpp`）每帧内层循环：`fn_140012625(scene, 2)` 为真则 2 次，否则 1 次。该函数是 `byte[scene+0x98] & (1<<k)`（`fn_1401ab620`）：bit2 加速 `[确认]`、bit1 自动调帆、bit0 横帆枝音效门，与 `01-sailing.md` 的 `ticks_per_frame_fast` 一致。每次内层迭代在 `0x1401a62b7` 调 `fn_14000f871` 推进世界 1 拍 `[确认]`（海战态且 `fn_1400144ac` 为假时跳过），所以加速档像素与日历一起加倍。

同一拍里，航速函数二选一（`COceanScene.cpp` `L_1401a6338`）：

```
obj = fn_140011630(scene)          # = DAT_144296500+8，当前场景描述 SceneDesc
scene_flag = fn_140006c12(obj)     # *(int*)(obj+0xC)  [确认] 原始指令 0x140092d4b
auto_flag  = fn_140003689(obj)     # *(int*)(obj+0x10) [确认] 0x1400f351b

if scene_flag == 0 and auto_flag == 1:
    fn_140008814(scene)            # 自动：沿当前航线跳
else:
    fn_14001444d(scene)            # 手动：F-OCN-01
```

`fn_140008814` body `0x14018c420`（`srcdata/source/game/unassigned/fn_140188eb0.cpp`）。RTC：`vec`、`posFleet`、`posFlagShip`、`sLocus`、`iWindVelocity`、`cWindAngle`、`cShipAngle`。取风与手动相同（`fn_140014812`），**不**调用 `fn_14001444d`。

`fn_140006c12 == 1` 时 tick 另走 `fn_140003acb`（NPC `eMobileFleet` 1..3 的瞄向）、`fn_14000660e`、`fn_14000ee8a`，**不**调遭遇检测 `fn_14000cd8d`；航速 `/3`；划桨无条件生效。`[推断，强]` 大地图海战态（DK4 海战就在大地图上进行）。禁止当成第四种玩家航行模式。玩家三种模式见 §3。

### 2.2 手动：舵、帆、半开 `[确认]` 结构，`[推断]` 半开键

舵：

- `CWheelView` 体积 0x98。`vfunc_16`（`CWheelView.cpp`）：`fn_14000f26d(obj, 1)` 为真则**不**把输入交给舵；否则 `fn_14000dcb5`。后者 RTC `vector` / `ptCenter`，用指针位置相对轮心的向量改航向。
- `CWheelSprite` 体积 0xB0，轮面边 0x76；`+0x3c` 参与角计算 `[推断]` 当前航向投影。
- `01-sailing.md` 写「某标志为真时不把输入交给舵」：该标志就是 `fn_14000f26d` 的 **edx=1** 枝。

半开不是同一枝。`fn_14000f26d`（body `0x1401df420`）是：

```
(byte[obj+8] & (1 << k)) != 0      # fn_1401dd860 + fn_1401dd310(k)=1<<k
```

`obj = fn_1401ab9e0()`，按函数簇看是 `CPlayerFleetData`（`+8` 位旗、`+9` 帆角 `cSailAngle`）。`k` 是第二参。同一函数不同 `k`：

| `k` | 调用点 | 行为 |
| --- | --- | --- |
| 1 | `CWheelView::vfunc_16`、`fn_14001444d` 开头、`COceanScene::vfunc_02` 手动枝 | 锁舵；为真则跳过整段 F-OCN-01，只写速度条 |
| **0** | `fn_14001444d` 算出风帆速度之后（`xor edx,edx` @ `0x1401b061f`） | `speed /= 2`（带符号） |
| 3 | `COceanScene::vfunc_05` | 选 SE 0x14 / 0x15 |

**更正**：旧版把半速键写成 `0x11`，那是反编译把上一调用（`fn_14000cf1d(…, 0x11, 2, 1)`，顺风枝音效）的寄存器残留当成参数。原始指令是 `k=0`。半开帆是否即 bit0：文案「减速时帆要“半开”。」（`mesfile_sc.txt` `0x16f03`）与 `/2` 对得上 `[推断]`，写入者未找到，不升确认。

帆按钮：`fn_140002e1e`（`fn_140194480.cpp`）RTC `aeSailButton`、`aeOperateButton`、`eOSBI`。`aeSailButton = 6,7,8,9`，贴图 `OcFrm.DK4` 第 6–9 帧。循环 `eOSBI` 5..6 共 **2** 个帆按钮，每个 2 帧。`[推断]` 纵/横 × 开/半开的投影，不是第三种帆公式。操作按钮 `eOSBI` 7..0xA，帧号含 −1 空洞，名字 `[未知]`。

速度条：`CSpeedBarSprite` 从 `OcFrm.DK4` 取第 3 帧。`fn_140004250` 把速度写成 `sprite+0x38 = (speed + (edx&7)) >> 3` 再调 `vfunc+0x30`。`>> 3` 是显示标尺，不是玩法除数。

`COceanScene` 构造循环上界 5 / 2 / 4：5 船槽、2 帆按钮、4 操作按钮。`vfunc_14` RTC `aeOceanSceneButtonIndex` 字面量 0–4，五个主按钮中文名 `[未知]`。

### 2.3 自动航线跳 `[确认]`

`fn_140008814`：

1. 取旗舰 `cShipAngle`、气候风 `cWindAngle` / `iWindVelocity`
2. `fn_140001ece` 自动调帆 `[确认]`：`d = wind − ship`（mod 256），`d<=128` 则帆角 `= d/2`，否则 `256 − (256−d)/2`，写回 `CPlayerFleetData+9`。即帆取风与船的平分角。手动 F-OCN-01 在 `scene+0x98` bit1 时也调它
3. `posFlagShip`、`posFleet`，差向量 `vec`，`sLocus` 一字节角
4. 速度条：某阈值内用已算长度，否则 `isqrt(vec²+…)`（`fn_14000baaf`，`isqrt(n*k*k)`）
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

`fn_1401f3aa0`（RTC `eRoute`、`eTown`、`asConnect` 1552 字节 = 97 × 16）是港口图上按层松弛的多跳最短路 `[确认]`，权取 `SRoute+0x0`，比较为无符号；势力参数 `!= 0x1A` 时只走两端港该势力份额都 `> 0` 的边（`fn_14000a4fc`），`0x1A`=26=`NUM_FORCE+1` 即「不过滤」。规格与 YAML 归 `01-world/05-routes-navigation.md`，本篇只调 `Routes.path(a, b, force | null)`。原版**没有**「航线已建立」存档位：可见 = 两端港已知位（`fn_14001389f`），可走 = 寻路有解。

选港 UI：`CSelectTownRouteMap` `+0x110 = 0x62`（未选）。寻路无解：只出文案，不移动。文案 key 见航线文档；同块还有「没有测量员。」（`mesfile_sc.txt` `0x5d0f`）。逻辑层两事件：`NavigatorMissing`、`RouteMissing`。不要把两句当成同一条件。

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
| `SHIP_CREW_MIN/MAX` | `USHORT` 断言，十进制未进仓库 | **是**，经人手不足项。`CShip+0x4C` 与必要人手比较，见下 |
| 追加船舱 / 装甲 | 「追加船舱速度会降低。」「追加装甲板……速度会降低。」（`0xc5d7` `0xc43f`） | **是**。改装位 2/3 各罚 1、4/5 各罚 2，四个帆值都减（`fn_14001292c`）。见 01 §2 |
| 大鹫像 item 114 | 「船飞速前进」 | **是**。`CShip+0x53 == 0` 的船在队中 → `speed += speed/10` `[确认]` 数值，`[推断，强]` 0=大鹫。魔鬼像（7）`*4/5`。重构走 `fleet.speed` 修正器（01 §6） |

`fn_140006b95`（body `0x14023e4c0`，RTC `eShipCabin`）：循环 31 槽，`vfunc+0x38(slot) == edx` 则 `count++`。edx=11 → 划桨手室间数。

划桨进入航速的两处（均在 `fn_14001444d` 之后段，即**手动** F-OCN-01）。原始指令已钉死 `[确认]`：

1. `fn_14000da9e`（body `0x14023e1b0`）：`10 * count(kind==11) / size_base[size_class]`。`imul eax,eax,0xa` 后 `idiv`，除数是 `fn_14023e9c0(size_class)` 查 `.rdata 0x140596d98` 的 `{3,5,10}`，`size_class = CShipType+0x18`（`05-ships-fleet/06` 的 `size_base` 同表）。**不是** `n*n`。门闩：`fn_140006c12==1`（海战态）**或** `fn_14000ee12(fleet) < 0x32`。`fn_14000ee12` 读 `CFleet+0x71`，舰队情报面板把它画在 RTC `posTired` / `rcTiredTitle`（`fn_140005f51`），写入只经 `fn_140009467` 钳 `0..100` → `[推断，强]` 舰队疲劳。日步疲劳式里的 `fn_140006d9d`（疲劳 `< 50` 或另一场景谓词 `fn_14000ce19`，`[推断]` 同一门闩）判「正在划桨」并加划桨疲劳（01 §2）；接舷白刃用 `(200 − 疲劳)`（`07-combat/02-boarding.md`）。
2. `fn_14000a466`（body `0x14023f370`）合成必要人手 `need`（= `Crew.min`，公式归 `05-ships-fleet/06` §2.4）：`size_base[size_class]`（基数）+ `fn_14000feac`（每根有桅 `+size_base[size_class]`，桅位 bit3 `+1`、bit2 `+2`；改装 0 `+1`、改装 1 `+4`）+ `fn_1400053ee`（`40*count(kind==11)`）+ `fn_140001e2e`（`20*count(kind==8)` + 改装 6/7 各 `+2`）。然后：

```
now  = ship.vfunc+0x58()     # CShip::vfunc_11 → +0x4C
need = fn_14000a466(ship)
if now < need:
    speed = speed * (now + need) / (2 * need)    # 0x1401b07ae idiv，除数 2*need
```

`need` 每间划桨手室 +40、每间武装炮台 +20、每根桅按船格加——只有「必要水手数」说得通；`+0x4C` 同时是日耗的吃粮人数（F-ADJ-03）。结论：`now` = 当前水手，`need` = 必要水手 `[推断，强]`。01 篇旧稿的「耐久」已改。只算**旗舰**。

自动跳 `fn_140008814` **没有**调用 `fn_14000da9e` / `fn_14000a466`。划桨手室加成是手动公式的后处理。原版包：自动模式不吃这项，除非以后还原出自动内部等价式。

「有的船没有１名水手。要重新雇用水手或者重新编组。」（`0x3eb`）是出海检查 `crew_count`，不是航速。解雇全体「船就动不了」（`0x1323f`）同。

### 2.6 驾驶术、测量术 `[确认]` 岗位门闩，`[未知]` 天数曲线

十技能 id 见人物文档。本 tick **不**调用 `fn_14000e1ce(person, 2)` 或 `(person, 3)`。

驾驶术（`eSkill=2`，「驾驶的资质，航海术、造船能力。」）：

- 战斗船初始化 `fn_1400094d0` 把 `Ability(captain, 2)` 写进战斗对象 `+0xac`（同函数还写命中力、剑术）。那是海战，不是大地图。
- 修正器 `fleet.auto_sail_days` 是预留 key。`COceanScene::vfunc_02` / `fn_140008814` / `fn_140002adb` 里没有「还能自动几天」的计数器。禁止编 `days = skill/10`。

测量术（`eSkill=3`）与测量员岗位：

- 无测量员：不能建航线。文案「没有测量员。」×8 +「测量员不在。」（`0x5d0f`）。这是**岗位空缺**，不是技能值。
- 有测量员但边未建立：「还没有建立到那座城市的航线。」及 7 条同义句（航线文档已收 key）。
- 手册阈值 120 进 `posts.yaml` 的 `required_skill`，标 `[推断]`，exe 立即数未钉死。
- `fn_14000e1ce(person, 3)` 出现在造船/城镇函数（`fn_140006cb7`、`fn_1400099ee`），不是开辟航线。

操帆手 / 操舵手：文案强调合作（`0x163ef` `0x164ab`）。`[确认]` 不乘：`fn_14001444d` 与四个帆值函数（`fn_140006f64` / `fn_140002612` / `fn_1400036e8` / `fn_14001150e`）只读船体字节与改装位，不调人物 / 岗位函数。重构只读修正器 `fleet.sail_handling` / `fleet.helm`（原版包无贡献）。

舰长室：海上命令必须从舰长室发（`0x5e4f`）。没有舰长室不能下自动/探索令，不改船速。

### 2.7 「航行升级」不是一棵树 `[确认]` 三件独立进度

原版没有名为「航行升级」的枚举或 View。玩家能变强的通道是三条，不要合成一条经验条：

1. **航线可用**（world 派生量）。两端港已知（`fn_14001389f`）才画线；寻路按势力份额过滤边。无解 = `RouteMissing`。这是图的升级，不是速度升级；原版没有「建立航线」存档位。
2. **帆与改装**（ships）。船首帆/上帆/四角帆 → 横帆（顺风）；船尾帆/斜帆/三角帆 → 纵帆（逆风）。数值进 F-OCN-01 的 `square_sail` / `fore_sail` / extra。装甲、追加船舱从四个帆值里扣罚值（不是百分比），数值见 01 §2「船帆四值」 `[确认]`。
3. **技能与岗位**（characters）。驾驶术不进本 tick 航速。测量术只通过「有没有测量员」开门。操帆/操舵修正器键保留，曲线空。

人物等级 `isqrt(exp/30)` 不直接乘航速。

### 2.8 疲劳与钟 `[确认]`

疲劳：`CFleet+0x71`，日增量在舰队日步 `fn_140005a15`（`01-sailing.md` §2「舰队疲劳日增量」，原始指令已还原）。`COceanScene` / `CShip` / `fn_14001444d` / `fn_140008814` 均无疲劳写入。重构：Ocean 在 `DayStarted` 的 `daily` phase `fleets.fatigue` 步算 `delta`，调 `Fleets.add_fatigue`；状态归 fleets，`FatigueChanged` 由 fleets 发。

钟只有一套：

| 层 | 谁走 | 证据 |
| --- | --- | --- |
| 迭代 | `COceanScene::vfunc_02` 每帧 1 或 2 次；每次先位移（手动 `fn_14001444d` / 自动 `fn_140008814`），再 `fn_14000f871` 推进 1 拍 | `0x1401a62b7`；`01-world/07-calendar-and-coords.md` |
| 日 | 世界 tick 在日首（`nTime==0`）跑舰队日步：补给、疲劳、人物、happening | `fn_14000906b` 门 @ `0x1400ff991` |
| 海域图 | `CSeaAreaScene::vfunc_02` 同样调 `fn_14000f871`；`+0x8==2` 时 `nTime==0` 重算补给告警 | `0x140214213`；`fn_140004101` |

所以「每走多少像素算一天」= 每 48 次迭代一天（`ticks_per_day`），加速档每帧 2 次。旧稿「谁推进 `nTime` 未知」「加速只加倍像素」作废。

补给耗尽：`fn_140009f1b`（RTC `anSupplyAmount`）在 `auto_flag==1` 时调用；`fn_14000bcc1` 对 5 船两类补给求和，返回 3 `[推断]` 两类都有余。耗尽把 `COceanScene+0xa0` 写成 98。靠近港口且检查通过则 `+0xa0 = town_id` 并切场景。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key |
| --- | --- | --- | --- |
| 航行模式 | 无枚举；tick 分叉 + 海域图 `+0x8` | — | `sail_mode`：`manual` / `auto_route` / `auto_town` |
| 航线 | `eRoute` 200 | `null`（原版 201，仅 `legacy_codec`） | `route_id` |
| 港口 | `eTown` 97 | `null`（原版 98，仅 `legacy_codec`） | `town_id` |
| 航向 | `cShipAngle` uint8 | — | `heading256` |
| 帆开度 | `fn_14000f26d` 的 `k` | — | `sail_opening`：`full` / `half` |
| 舱室种类 | `eCabinKind` | 0x17→0x18 别名 | `cabin_kind` key + `role` |
| 划桨手室 | 名表下标 11 `[推断]` | 0 间合法 | role `rower_room` |
| 水兵室 | 名表下标 7 `[推断]` | 最多 4 | role `marine_room`（战斗订阅） |
| 水手人数 | `SHIP_CREW_*` USHORT | 0 不能开船 | `crew_count` |
| 测量员 | `ePost` 4 `surveyor` | 空岗位 | `Fleets.post_holder(fleet, surveyor)` |
| 舰队导航器 | `CFleet+0x88` `CTownNavigator` | 港 / 线 `null` | 不暴露指针 |

`sail_mode` 是运行时状态，进存档（原版嵌在 `CFleet+0x88` 与场景旗标里）。不要用 201 当「未建立航线」——那是「当前没选边」。航线可用是 world 的派生谓词，不存档。

## 4. 算法 / 公式

魔数进 `config/formulas/ocean.yaml` 与本篇两个 YAML。代码只留参数名。F-OCN-01 的顺风/逆风/洋流、划桨、人手、船首像、海战 `/3` **全部**以 [01-sailing.md](01-sailing.md) §4.3 为准（除数已钉死：10 / 5 / 3 / `2*need`）。本篇只追加模式分派、半开键与各模式是否吃这些项。

### 4.1 模式分派

```
scene_mode = Session.mode()                  # manual | auto | battle；原版 SceneDesc +0xC==1 → battle、+0x10==1 → auto
dest       = Nav.destination(fleet_id)       # CTownNavigator +0x11；null = 无
route      = Nav.current_route(fleet_id)     # +0x8；null = 无

def tick_sail(fleet_id):
    if scene_mode == battle:
        speed = F-OCN-01(...)               # 仍算手动式；NPC 另走 fn_140003acb
        move_pixels(fleet_id, speed)
        return
    if scene_mode == auto and route is not null:
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

取消：UI 问 `ui.ocean.cancel_auto_move`，确认后 `Nav.clear()`（港、线置 `null`），回到 `manual`。

### 4.2 手动：F-OCN-01 + 半开 + 划桨

```
sail  = sail_speed(...)                     # F-OCN-01 帆 + 洋流部分
if flag(k=0):                               # 半开 [推断]；/2 在洋流 isqrt 之前
    ...                                     # 顺序见 01 §4.3，本篇不重抄
if scene_mode == battle or crew_fatigue < rower.gate_fatigue_below:   # 50
    speed += rower.unit * rower_rooms / size_base[size_class]            # 10, {3,5,10}
if crew_now < crew_needed:                                              # Crew.min
    speed = speed * (crew_now + crew_needed) / (2 * crew_needed)
speed = Modifiers.eval("fleet.speed", ctx(base=speed))       # 船首像 add +1/10 → mul 4/5
show(speed)
speed = Modifiers.eval("fleet.speed_move", ctx(base=speed))  # 海战态 /3（scene.battle 贡献）
```

`rower_rooms` = `Ships.cabin_count(flagship, rower_room)`（原版 31 槽中种类 11 的个数），上界 `caps.cabins_per_ship`。

操帆/操舵：模组要让岗位影响航速，就给 `fleet.speed` 注册岗位来源的贡献（D2），不另开 `apply_post_modifiers` 开关。原版包无此贡献（F-OCN-01 体内不读人物）。

### 4.3 自动：跟线、不重算帆

```
wind = Climate.wind_at(x, y)                # 仍取
sail_angle = bisect(wind - ship_angle)      # fn_140001ece，§2.3 [确认]
delta = waypoint - fleet.pos                # posFlagShip / 当前 locator
speed_bar = pick(precomputed, isqrt(|delta|))
move toward waypoint
write_speedbar(speed_bar >> speedbar_shift) # 显示仍 >>3
```

自动到港：

```
def plot_auto_town(fleet_id, to_town_id) -> Result:
    if Fleets.post_holder(fleet_id, surveyor) is null:
        emit NavigatorMissing { fleet_id }; return fail
    path = Routes.path(from_town(fleet_id), to_town_id, force)   # force | null；01-world/05
    if path is null:
        emit RouteMissing { fleet_id, from, to, force }; return fail
    Nav.set_hops(fleet_id, path)            # 写入 CTownNavigator / asConnect
    Fleet.set_auto(fleet_id, true)
    return ok

def advance_hop(fleet_id):
    nxt = Nav.next_route(fleet_id)          # 方向由 +0x10 ? +0x11
    if nxt is null:
        # 已到目标港附近；入港仍看 arrival_radius（01 篇，值 null）
        return
    Nav.load_route(fleet_id, nxt)           # fn_140002d9c：写 +0x8/+0x9/+0xa/+0xb
```

`Routes.path` 的原版是 `fn_1401f3aa0` 的分层松弛（`01-world/05` §4 的 `path_search`）。重构可换任何最短路，返回仍是 `route_id[]`。旧稿的「`s710==0x1a` 跳过已建立检查」是误读：`0x1A`=26 是「不按势力过滤」，在 API 上就是 `force = null`。

### 4.4 出海门闩（不是速度）

```
can_leave(fleet_id):
    if any_ship.crew_count == 0: refuse     # 「没有１名水手」
    return ok

can_plot(fleet_id, to):
    if Fleets.post_holder(fleet, surveyor) is null: NavigatorMissing
    elif Routes.path(from, to, force) is null: RouteMissing
    else ok
```

测量术技能值不出现在这两个谓词里。要加「技能 < 120 视同无测量员」，改 `posts.yaml` 的 `enforce_threshold`（门槛取舱表 `survey.required_skill`），Ocean 只问 `Fleets.post_holder(fleet, surveyor) != null`（不是修正器）。

### 4.5 日历 vs 像素

```
on ocean_iteration:           # 1 或 2 次 / 帧（加速档）
    tick_sail()
    Clock.advance(1)          # 原版 fn_14000f871；海战态且 fn_1400144ac 假时跳过

on DayStarted:                # tick.yaml 的 daily phase，见 01 §4.4
    ocean.supply_drain → fleets.fatigue → … → ocean.happening_roll
```

加速档同时加倍像素与日历（每迭代 1 拍）。`ticks_per_day=48` 归 `caps.calendar_ticks_per_day`。

## 5. 耦合分析

原版硬编码：

- `COceanScene::vfunc_02` 内联手动/自动/补给/入港/遭遇
- `CFleet+0x88` 焊死 `CTownNavigator`；加一条航线要改 exe 静态表 `DAT_14429ac50`
- 测量员与「没有航线」挤在同一 mesfile 块
- 划桨手室种类下标 11 写在 `fn_14000da9e` 的立即数 0xb
- 半开与锁舵共用 `fn_14000f26d`，靠第二参区分
- 速度条 `>>3`、轮面 0x76、帆帧 6–9 写在 UI 构造
- `fn_140006c12==1`（海战态）把航行 tick 和战斗函数拧在一起
- 船首像 0 / 7、划桨室 11、炮台 8 的 id 直接写在航速相关函数里

重构切断：

- Ocean 只收 `Fleets.sailing_snapshot(fleet_id)`（含 `nav` 段：当前 `route_id`、hop、from/to）+ `WindSnapshot`
- 边表只问 `Routes.find_direct` / `Routes.visible` / `Routes.path`；最短路可替换
- 舱室间数问 `Ships.cabin_count(ship_id, role_key)`，不读 `CShip*`
- 岗位问 `Fleets.post_holder`（有无测量员 = `Fleets.post_holder(fleet, surveyor) != null`），不读六维
- 汉字全部 `ui/strings`
- `scene_flag==1` 的战斗叠层发 `FleetSpotted` / 交给 Combat，不在航行公式里调炮弹函数

## 6. 解耦后的配置

`config/ocean/sail_modes.yaml`：

```yaml
schema: dk4.ocean.sail_modes
version: 1
# 空港 98 / 空线 201 只在 ids/legacy_codec.yaml（D1）；运行时 null
hop_done_locator: 7              # CRouteNavigator +0xa 比较值
route_force_filter: player_force # Routes.path 的 force 参数；null = 不过滤（原版 0x1A）
speedbar_shift: 3                # 显示；不是 F-OCN-01 的 sail_shift
helm_lock_flag_k: 1              # fn_14000f26d edx
half_speed_flag_k: 0             # 原始指令 xor edx,edx；旧稿 0x11 是反编译残留
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
    needs: { surveyor: true, path_found: true, dest_town: true }
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

`config/ships/rig.yaml`（旧名 `ocean/crew_sail.yaml`，已并入）只在 `05-ships-fleet/04` 定义（词表 §14-13），本篇只引用：舱室按 role 计数（`Ships.cabin_count(ship, rower_room)`，原版 kind 11 划桨手室；`marine_room` / `cargo_room` 不进航速），必要水手走 `ships/crew.yaml#capacity`（`Crew.min`），装甲 / 追加船舱罚值与帆值都由 Ships 现算后进 `Fleets.sailing_snapshot`。划桨参数（单位 10、除数 `size_base`、疲劳门 50、海战态总划）在 01 §6 `formulas/ocean.yaml#sailing_speed.params.rower`。

`config/formulas/ocean.yaml` 追加键（不删 01 篇已有项）：

```yaml
# 下列为 04 篇追加（D3 形状，词表 §14-15）。
auto_sail:                     # 自动跳 fn_140008814 与手动共用的开关
  params:
    half_speed_shift: 1
    auto_uses_manual_speed: false
    hop_done_locator: 7
    rower_modes: [manual]      # 划桨只进手动 F-OCN-01；自动跳不吃
    crew_zero_blocks_departure: true   # 水手 0 不能出港；now<need 时按 sailing_speed.params.crew_shortfall_den_factor 缩放
# 疲劳日增量见 01 §6 crew_fatigue；旧键 fatigue_per_day / drain_on_pixel / apply_post_modifiers 删除
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
- 划桨手室角色、每间单位、按体型档除数、疲劳门闩（每间必要人手归 `ships/crew.yaml`）
- 水兵室是否开始影响航速（默认否）
- 出海是否检查 `crew_count==0`
- hop 完成字节（原版空港 / 空线哨兵只在 `legacy_codec`）
- 海域图自动是否跟 `nTime==0`
- 测量员缺席：禁命令 vs 只减精度（默认禁 `plot`）
- 加速档每帧迭代数（每迭代 1 拍）
- 寻路是否按势力过滤（`route_force_filter`）

改横帆纵帆数字走 `ships/`。改 200 条边走 `world/routes.yaml`。改驾驶术曲线 = 给修正器 `fleet.auto_sail_days` 注册贡献（原版包无）。

## 8. 对外接口

事件（本篇发出，01 篇已列的不重复定义）：

```
SailModeChanged { fleet_id, from, to }          # manual|auto_route|auto_town
HopAdvanced { fleet_id, route_id, dest_town_id }
AutoMoveCancelled { fleet_id }
NavigatorMissing { fleet_id }
RouteMissing { fleet_id, from_town_id, to_town_id, force_id | null }   # 与 01-world/05 同形
ArrivedAtTown { fleet_id, town_id }             # 仍由 01 定义
# 疲劳：命令 Fleets.add_fatigue；FatigueChanged { fleet_id, old, new } 由 fleets 发（01 §8）
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
Ocean.plot_route { fleet_id, route_id }         # auto_route；边须 Routes.visible
Ocean.plot_town { fleet_id, to_town_id }        # auto_town；须测量员 + 路径
```

`Fleets.sailing_snapshot(fleet_id)` 的 `nav` 段（旧名 `NavSnapshot`；Fleets 提供，Ocean 只读）：

```
{
  from_town_id,              # null = 无
  to_town_id,
  route_id,                  # null = 无
  locator,                   # +0xa
  direction_flag,            # +0xb
  hops: [route_id]
}
```

快照其余字段仍按 01 篇；本篇要求其中 `crew` 与 `cabin_counts[role_key]` 可用。禁止传 `CFleet*`、`CTownNavigator*`、`Location`。

UI 投影：

- `CWheelView` ← `heading256`；`helm_lock` 时忽略指针
- `CSpeedBarSprite` ← `Ocean.speed >> speedbar_shift`
- `CSelectTownRouteMap` ← 港列表 + `Routes.visible`
- 海域图自动进度 ← `Fleets.sailing_snapshot(fleet_id).nav` + 日历，不持有舰队对象

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| `fn_140006c12` / `fn_140003689` 读的字节 | 当前 `SceneDesc` 的 `+0xC` / `+0x10`（`DAT_144296500+8`） | `[确认]` 偏移；`+0x10==1` 自动 `[推断]` |
| `fn_140006c12==1` 是否海战态 | 调 NPC 瞄准 / 近战 / 炮弹、跳过遭遇检测、航速 /3 | `[推断，强]` |
| `CRouteNavigator +0xa` 除「比较 7」以外的值域 | `fn_140002d9c` 从表写 | `[未知]` 进度还是 locator 种类 |
| `+0x9/+0xb` 的 0/5 | 方向 | `[推断]` 正向/反向 |
| `CTownRoute` 0x378 布局 | 析构尺寸 | `[未知]` |
| `asConnect` 1552 字节元素 | `fn_1401f3aa0`：97 × 16 `{route, prev, hops, cost}` | `[确认]`（`01-world/05`） |
| `0x1A` 势力参数 | 26 = `NUM_FORCE+1` = 不按份额过滤 | `[确认]`（`01-world/05`） |
| `fn_14000f26d(k=0)`（`CPlayerFleetData+8` bit0）是否半开按钮 | 文案 + `/2`；写入者未找到 | `[推断]` |
| 帆按钮 6–9 与纵/横/开/半的一一对应 | 2 按钮×2 帧 | `[推断]` |
| `aeOceanSceneButtonIndex` 0–4 名称 | `vfunc_14` | `[未知]` |
| `CShip+0x4C` = 当前水手 | 与「必要人手」比较、日耗按它吃粮 | `[推断，强]`；`+0x4A` 仍 `[未知]` |
| 划桨除数 | `fn_14000da9e` → `{3,5,10}[size_class]` | `[确认]` |
| `CFleet+0x71` = 舰队疲劳 | 情报面板 `posTired` | `[推断，强]` |
| 改装位 2–5 的件名 | 罚值 1/1/2/2 已确认 | `[推断]` 2/3 装甲、4/5 追加船舱 |
| 船首像 id 与道具 114+ 的映射 | 0 加速、1 省粮、4 炮、7 减速、8 近战 | `[推断，强]` |
| 驾驶术 → `fleet.auto_sail_days` 曲线 | 修正器 key 占位；本 tick 未读 skill 2 | `[未知]` |
| 测量员 120 | 手册 | `[推断]` |
| 操帆/操舵是否乘进 `s58/s5C` | 帆值函数无人物调用 | `[确认]` 不乘 |
| 海上疲劳日增量 | `fn_140005a15` 原始指令 | `[确认]`（01 §2） |
| 像素航行谁推进 `nTime` | `vfunc_02` 每迭代 `fn_14000f871` @ `0x1401a62b7` | `[确认]` 1 拍 / 迭代 |
| 入港半径 | 01 篇 | `[未知]` |
| F-OCN-01 三处 `idiv` | 原始指令 10 / 5 / 3 | `[确认]`，见 01 |
| `COceanScene+0x208` 声音 id | `vfunc_05` | `[未知]` |

`CLocator` 同时是港、村、结点的基类。hop 的途经点抽出前，`nav.hops` 只存 `route_id`，折线可空。
