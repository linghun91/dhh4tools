# 航行：tick、风、帆、补给

大地图出海。Ocean 只负责把舰队从一格挪到下一格：风向、船速、补给扣减、入港触发。舵轮、速度条、天空是投影。

## 1. 职责边界

本系统管：

- 大地图 tick（`COceanScene`）与海域图导航（`CSeaAreaScene` / `CRouteMap`）
- 当前风（方向、风力）如何改航速
- 纵帆 / 横帆 / 改装帆如何改顺风、逆风速度
- 潮流（current）与船速的矢量合成
- 水和粮食的消耗与耗尽告警
- 海上疲劳的**触发口**（涨跌公式未还原时只发事件）
- 入港、缺航线、无测量员的判定

不管：

- 港口内部、市价、投资、设施（Town / Economy）
- 海战与接舷结算（Combat）；本系统只发 `FleetSpotted`
- 村落 / 遗迹的坐标与发现状态表（World 拥有；Ocean 只做探索动作，见 `02-search-encounters.md`）
- 黄金航路进度与奖励（`03-gold-road.md`）
- 人物六维、技能公式（Characters 输出 `ModifierSet`）
- 船型耐久、帆值、补给舱定义（Ships）；本系统只读快照
- 气候场如何随月份生成（World / `climate.yaml`）；本系统读当前 `WindSnapshot`
- `CWheelView` / `CWheelSprite` / `CSpeedBarSprite` / `CSkySprite` / `CCloudData` 的绘制

场景切分（原版已有）：

- `COceanScene`（对象 0x218）— 大地图航行
- `CSeaAreaScene`（`CRouteMap` 子类，0x128）— 海域图：港、村、NPC 舰队图标
- `CGeneralCommandView` / `CNextAreaView` / `CForceSelectView` — 命令与对象选择，不写规则

## 2. 原版事实

### 场景与资源 `[确认]`

- `COceanScene` 构造 / 主循环：`srcdata/source/game/ocean/COceanScene.cpp`。ctor 循环上界 5 / 2 / 4，与 `NUM_FLEETSHIP=5`、帆按钮组、操作按钮组一致。`vfunc_02` 是 tick。`vfunc_14` 的 RTC 名 `aeOceanSceneButtonIndex`，字面量 0–4，五个主按钮。
- 舵轮：`CWheelView`（0x98）、`CWheelSprite`（0xB0，轮面边 0x76）。`CWheelView::vfunc_16` 在某标志为真时不把输入交给舵。
- 速度条：`CSpeedBarSprite` 从 `OcFrm.DK4` 取第 3 帧。
- 帆 / 操作按钮：`fn_140002e1e`（`srcdata/source/game/unassigned/fn_140194480.cpp`）RTC `aeSailButton`、`aeOperateButton`、`eOSBI`。帆按钮贴图索引写死为 OcFrm 6、7、8、9。
- 海面 / 天空 / 云 / 天气贴图：`COceanSurface`（`Chip.DK4`）、`CSkySprite`（`Sky.DK4`）、`CCloudData`（2 槽）、`Weather.dk4`。`CSkySprite` ctor 在某气候 / 海域 id `== 8` 时置内部标志。
- 航线资源：`Route.dk4`、`RouteFrm.dk4`、`RouteMap.dk4`、`RouteMkr.dk4`。
- `COceanMapData` 仅 0x10 字节，不是 `World.dat`。2500×2500 地图像素属 World。

### 风向与风力 `[确认]`

- 符号：`WIND_DIRECTION_ADJUST_MIN/MAX`、`WIND_VELOCITY_ADJUST_MIN/MAX`（`srcdata/catalog/constants.json`）。具体整数未从断言还原。
- RTC 名：`cWindAngle`（1 字节角）、`iWindVelocity`、`cCurrentAngle`、`iCurrentVelocity`、`sClimateStreamWind`。
- 气候在 `CWorld` / `CClimate`（`CWorldData` 基类链含 `CClimate`）。`CWorld::vfunc_04` 更新 `sClimateStreamWind`。
- 取风：`fn_140014812`（`srcdata/source/game/unassigned/fn_1402aad20.cpp`）。若某气候 id `== 0x18`，`iWindVelocity = 0`；否则 `iWindVelocity += 3`。同一分支出现在航速函数 `fn_14001444d`。
- 风力档：`fn_1402abb20` 先 `rand(0x64)` 再必要时 `rand(0x14)`，映射到 0–12 共 13 档。

### 航速 `[确认]` 结构，`[未知]` 除数与余弦表

主公式在未挂类函数 `fn_14001444d`（body `0x1401b0200`，`srcdata/source/game/unassigned/fn_1401ac810.cpp`），RTC：`cShipAngle`、`cSailAngle`、`cSailToWindAngle`、`iWindVelocity`、`cWindAngle`、`cCurrentAngle`、`iCurrentVelocity`、`cShipToCurrentAngle`、`eFleetShip`。由 `COceanScene::vfunc_02` 在非自动航线模式下调用。自动航线走 `fn_140008814`（同文件 `fn_140188eb0.cpp`），同样取风。

角是 256 分圆的 `uint8`（立即数 `0x20=32`、`0x2A=42`、`0xD6=214`）。`fn_140009e3a` 是该角的余弦（表未还原）。`fn_14000baaf` 是整数开方。

文案（`srcdata/catalog/mesfile_sc.txt`）：

- 「张挂船首帆能提高横帆的性能，加快顺风时的航海速度。」
- 「张挂船尾帆能提高纵帆的性能，加快逆风时的航海速度。」
- 「安装上帆，能提高横帆的性能，加快顺风时的航海速度。」
- 「安装斜帆能提高纵帆的性能，加快逆风时的航海速度。」
- 「四角帆在顺风时加快速度。但是不能张挂在三角帆之后。」
- 「三角帆在逆风时加快速度。但是不能张挂在四角帆之前。」
- 「四角帆能提高纵帆﹑横帆两种帆的性能。」
- 「减速时帆要“半开”。」
- 「要根据风力区分使用纵帆或横帆。」

`NUM_ADDPARTS=9`、`NUM_DECKADDPARTS=4`（`num_values.json`）。改装件与帆种的一一对应未从字段名还原。

### 补给 `[确认]`

- `eShipSupply` 循环上界为 2（`fn_14000bcc1`，`srcdata/source/game/unassigned/fn_1400f2f80.cpp`）。`[推断]` `NUM_SHIPSUPPLY=2`：水、粮食。文案「探险队发现了%s桶水﹑%s桶粮食。」
- `fn_14000bcc1` 对玩家 5 艘船把两类补给分别求和，再找第一个 `<= 0` 的种类。返回 3 时调用方跳过告警 `[推断]`「两类都有余」。
- 大地图告警：`fn_140009f1b`（RTC `anSupplyAmount`），`COceanScene::vfunc_02` 在内部状态 `== 1` 时调用。耗尽时把待入港 id 写成 `0x62`。
- 海域图告警：`fn_14000884b` / `fn_140004101`（`CSeaAreaScene` 地址段）。后者在 `nTime == 0` 时重算补给，RTC 含 `nMonth`、`nDay`、`nTime`。
- 文案：`mesfile_sc.txt` `0x537` 起「提督，这样下去物资就要没了……。」多档。
- `SUPPLYAMOUNT_MIN/MAX` 有符号，整数未还原。`CSupplyRateView` 是补给比例 UI，属投影。
- 大厨舱「能节约水和粮食，水手也减轻疲劳。」（mesfile `0xc187`）— 节约系数 `[未知]`。

### 入港与航线 `[确认]`

- `NUM_TOWN=97`。Ocean 用 `0x62=98` 当空港口哨兵（`CTownNavigator` ctor 写 `+0x10/+0x11 = 0x62`；补给耗尽同样写 0x62）。
- `NUM_ROUTE=200`，`INVALID_ROUTE=-1`。`CRouteNavigator` ctor 写 `+0x8 = 0xC9`（201）`[推断]` 空航线哨兵；`vfunc_03` 循环 `eRoute < 0xC8`。
- 无航线文案：「还没有建立到那座城市的航线。」（及 7 条同义句）。
- 无测量员：「没有测量员。」×8 +「测量员不在。」
- 测量是技能也是岗位。mesfile：「测量就是根据太阳的位置了解目前所在的位置。」「测量工作有了六分仪就能顺利进行。」岗位名「测量士」。技能条数 `NUM_SKILL=10`；哪一条是测量、`ePost` 哪一个是测量士，整数未还原。
- 入港后 Ocean 停手。原版 `COceanScene::vfunc_02` 在靠近港口且补给检查通过后把城镇 id 写入 `+0xA0` 并切场景。

### 疲劳 `[确认]` 有符号与文案，`[未知]` 每 tick 公式

- `FATIGUE_MIN/MAX`，断言宽度为 `UCHAR`。
- 文案：「疲劳度上升了%s。」「疲劳度恢复了%s。」「水手们好像很疲劳，不让他们休息一下吗？」探索温泉可消疲劳。
- 海上每 tick 如何加疲劳：未在 `COceanScene` / `CShip` 还原。重构只发事件，数值进 `formulas/ocean.yaml`。

### 坐标 `[确认]`

伦敦 `x1=20080 y1=4272`。这是原版坐标单位，**不要除 8**。换算到 `World.dat` 2500×2500 的公式未还原。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key | 备注 |
| --- | --- | --- | --- | --- |
| `eSeaArea` | 25 | `INVALID_SEAAREA=-1` | `sea_area_id` | 与 `NUM_NEXTBASETOWN=25` 同数。发现 / 访问用 `SEAAREA_BITS` |
| `eRoute` | 200 | `-1`；运行时另见 `0xC9` | `route_id` | 港到港导航航线，不是黄金航路 |
| `eJunction` | `[未知]` | `INVALID_JUNCTION` | `junction_id` | World 表 |
| `eTown` | 97 | `-1`；Ocean 空哨兵 `98` | `town_id` | 入港只传 id |
| `eFleet` / `eFleetShip` | 69 / 5 | `-1` | `fleet_id` / `ship_slot` | 玩家船位 5 |
| `eMobileFleet` | 4 | | `mobile_fleet_slot` | 海域上的活动舰队槽 |
| `eShipSupply` | `[推断]` 2 | `-1` | `supply_kind` | 0 水、1 粮（顺序 `[未知]`） |
| `eShipType` | 5 | `-1` | `ship_type_id` | 航速里对 type 0 与 7 有特殊枝，7 超出 0–4，可能是运行时种类 `[未知]` |
| `ePost` | `[未知]` | `INVALID_POST` | `post_id` | 测量士 / 操帆手 / 操舵手 |
| `eSkill` | 10 | `-1` | `skill_id` | 测量技能下标 `[未知]` |
| 风向角 | 0–255 | | `angle256` | 1 字节 |
| 风力档 | 0–12 | | `wind_level` | `fn_1402abb20` |

`SEAAREA_BITS` / `SEAAREA_ID` 是位域类型符号，宽度未还原。运行时以表长为准，禁止写死 `NUM_SEAAREA=25`。

## 4. 算法 / 公式

所有魔数进 `config/formulas/ocean.yaml` 与 `config/ocean/wind.yaml`。代码里只留参数名。

### 4.1 tick

```
each ocean_tick:
  wind = World.wind_at(fleet.x, fleet.y, calendar)
  snap = Fleets.sailing_snapshot(player_fleet_id)   # 帆、耐久、补给、岗位
  nav  = Characters.modifier_set(navigator_person_id)
  speed = sailing_speed(snap, wind, helm, sail_mode, nav)
  move fleet by speed along heading
  maybe drain_supplies(snap, elapsed)
  maybe emit fatigue event
  if overlaps town arrival radius: emit ArrivedAtTown
  if supplies empty: emit SuppliesDepleted
  if climate id == storm: emit StormHit   # 遭遇细节见 02
```

原版 `COceanScene::vfunc_02` 内层循环次数：`fn_140012625` 为真则 2 次，否则 1 次 `[推断]` 加速档。参数名 `ticks_per_frame_fast` / `ticks_per_frame_normal`。

### 4.2 风力档

`fn_1402abb20`，`rand100 = uniform(0, 99)`，`rand20 = uniform(0, 19)`：

| 条件 | `wind_level` |
| --- | --- |
| `rand100 < wind_cdf_0` (30) | 0 |
| `< wind_cdf_1` (50) | 1 |
| `< wind_cdf_2` (70) | 2 |
| `< wind_cdf_3` (80) | 3 |
| `< wind_cdf_4` (87) | 4 |
| `< wind_cdf_5` (96) | 5 |
| 否则 `rand20 < 4` | 6 |
| `< 7` | 7 |
| `< 10` | 8 |
| `< 13` | 9 |
| `< 16` | 10 |
| `< 18` | 11 |
| else | 12 |

无风：`climate_kind_id == calm_climate_id`（原版 `0x18`）时 `wind_velocity = 0`，否则 `wind_velocity = base + wind_velocity_bias`（原版 bias=3）。

方向：气候流 `sClimateStreamWind` + `WIND_DIRECTION_ADJUST`，合成到 `cWindAngle`。调整量上下限用符号名，整数待填。

### 4.3 航速

命名（原版局部名对照）：

| 参数 | 原版 | 说明 |
| --- | --- | --- |
| `square_sail` | `s58` | `[推断]` 横帆。顺风枝使用 |
| `fore_sail` | `s5C` | `[推断]` 纵帆。逆风枝使用 |
| `square_extra` | `s60` | 当船–风夹角 `< angle_blend` (32) 时被夹角缩小 |
| `fore_extra` | `s64` | 同上 |
| `wind_velocity` | `iWindVelocity` | |
| `sail_to_wind` | `cSailToWindAngle` | 帆向相对风向 |
| `downwind_cone` | `0x2A` / `0xD6` | `<=42` 或 `>=214` 走顺风枝 |
| `cos256(a)` | `fn_140009e3a` | 256 分圆余弦，值域与表 `[未知]` |
| `sail_shift` | `17` | 顺风 / 逆风枝最后 `>> 17` |
| `min_speed_floor` | `0x60` | 计算结果 `<=0` 时 |
| `speed_bias` | `0xC0` | 正速度时 `speed + fore_sail + 0xC0` |
| `half_speed_flag` | `fn_14000f26d` | 真则 `speed >>= 1`。半开帆或恶劣天气 `[未知]` |
| `durability_now` / `durability_max` | `s144` / `s148` | `now < max` 时 `speed *= (now+max) / (2*max)` |
| `low_stat_threshold` | `0x32` | 某统计 `< 50` 时加一项船值 |
| `type_a` / `type_b` | 船 `vfunc+0x48` 返回 0 或 7 | 各做一次 `idiv`，除数 `[未知]` |

顺风枝（`sail_to_wind` 落在锥内）：

```
square = square_sail + square_extra
fore   = fore_sail + fore_extra
term   = wind_velocity * (square * cos256(sail) + square * cos256(sail) * fore)
speed  = (term * cos256(sail_to_wind) + sail_to_wind_low_bits) >> sail_shift
```

逆风枝：

```
term   = wind_velocity * fore * cos256(sail)
speed  = (term * cos256(sail_to_wind) + 0) >> sail_shift
```

然后：

```
if speed <= 0: speed = min_speed_floor
else:          speed = speed + fore + speed_bias
if half_speed_flag: speed >>= 1
speed = isqrt( speed^2
             + ((speed * current_velocity * cos256(ship_to_current)) >> 8)
             + current_velocity^2 )
# 即 |v_sail + v_current|
if durability_now < durability_max:
    speed = speed * (durability_now + durability_max) / (2 * durability_max)
```

`idiv ecx` 三处（type 0 加倍后、type 7 左移 2 后、海域模式 `==1` 时）除数未反汇编出来，**禁止猜成 2 或 3**。参数名 `type_a_div`、`type_b_div`、`sea_area_speed_div`，默认包留空或标 `null` 直到还原。

操帆手 / 操舵手修正：Characters 的 `ModifierSet.sail_handling`、`ModifierSet.helm`。原版是否乘进 `s58/s5C` `[未知]`；重构只从修正器读，不读六维。

测量：无测量士时禁止「定位 / 开辟航线」命令，不改船速。

### 4.4 补给消耗

原版按日历步（`nTime == 0` 的那一拍）检查，不按像素。消耗量函数未还原。

```
need = crew_count * drain_per_crew_per_day * cook_saving_factor
for kind in (water, food):
    stock[kind] -= need * kind_weight[kind]
    clamp to [SUPPLYAMOUNT_MIN, SUPPLYAMOUNT_MAX]
if any stock == 0: emit SuppliesDepleted { fleet_id, kind }
```

`drain_per_crew_per_day`、`cook_saving_factor`、两种类权重全部进公式表。原版默认值 `[未知]`。告警文案分档（「就要没了 / 有点不放心 / 有点儿少」）用阈值 `supply_warn_high` / `supply_warn_low`，阈值 `[未知]`。

### 4.5 入港

```
if distance(fleet, town) <= arrival_radius
   and not in_combat_handoff:
    emit ArrivedAtTown { fleet_id, town_id }
    Ocean 停止该舰队的移动
```

`arrival_radius` `[未知]`。无航线：海域图点港口时若 `Route.exists(from, to)==false`，只提示文案，不移动。

## 5. 耦合分析

原版硬编码：

- `COceanScene` 直接调补给求和、风、船、气候、战斗切入（`fn_14000cd8d` 写 `sChangeBattle` / `sEventTrigger`）
- 航速读船对象虚函数，不经快照
- 测量员用岗位指针，缺人就弹 mesfile 固定句
- `NUM_FLEETSHIP=5`、`eShipSupply<2`、`eRoute<200` 写在循环里
- 帆按钮贴图索引 6–9 写在 UI 构造里
- 无风气候 id `0x18`、半速标志、船 type 0/7 写在公式里
- 入港空哨兵 98 与 `ITEM_EMPTY=106` 不是同一套，Ocean 自己用 98

重构切断：

- Ocean 只收 `SailingSnapshot` 与 `WindSnapshot`
- 入港只发 `ArrivedAtTown`，不 new 城镇设施
- 战斗只发 `FleetSpotted`，不调 `CCombatScreen`
- 气候场留在 World；Ocean 只订阅当前格的风
- UI 字符串全部走 `ui/strings.*.yaml`

## 6. 解耦后的配置

`config/formulas/ocean.yaml`：

```yaml
schema: dk4.formulas.ocean
version: 1
angle_modulus: 256
downwind_cone_low: 42          # 0x2A
downwind_cone_high: 214        # 0xD6
angle_blend: 32                # 0x20，小于此值缩小 extra 帆
sail_shift: 17
current_shift: 8
min_speed_floor: 96            # 0x60
speed_bias: 192                # 0xC0
half_speed_shift: 1
low_stat_threshold: 50         # 0x32
durability_denom_factor: 2     # (now+max) / (factor*max)
calm_climate_id: 24            # 0x18
wind_velocity_bias: 3
wind_level_max: 12
ticks_per_frame_normal: 1
ticks_per_frame_fast: 2
type_a_div: null               # [未知]
type_b_div: null
sea_area_speed_div: null
drain_per_crew_per_day: null   # [未知]
cook_saving_factor: 1.0
```

`config/ocean/sailing.yaml`：

```yaml
schema: dk4.ocean.sailing
version: 1
supply_kinds:
  - id: 0
    key: water
    name: 水
  - id: 1
    key: food
    name: 粮食
empty_town_sentinel: 98        # 原版 0x62，仅 Ocean 入港槽
empty_route_sentinel: 201      # 原版 0xC9
arrival_radius: null           # [未知] 原版坐标单位
sail_mode:
  - key: full
    name: 全开
    half_speed: false
  - key: half
    name: 半开
    half_speed: true
    # 原版「减速时帆要半开」；是否即 half_speed_flag [未知]
add_parts_effect:              # 指向 ships/ 的改装，不在此定义船体
  headsail: { square_sail_bonus: null, name: 船首帆 }
  spanker:  { fore_sail_bonus: null, name: 船尾帆 }
  topsail:  { square_sail_bonus: null, name: 上帆 }
  lateen:   { fore_sail_bonus: null, name: 斜帆 }
  square_cloth: { both: true, name: 四角帆 }
  triangle_cloth: { fore_only: true, name: 三角帆 }
required_posts:
  navigator: { post_key: navigator, fail_message: ui.ocean.no_navigator }
  sailmaster: { post_key: sailmaster }
  helmsman: { post_key: helmsman }
modifier_keys:
  - navigation          # 测量
  - sail_handling       # 操帆
  - helm                # 操舵
```

`config/ocean/wind.yaml`：

```yaml
schema: dk4.ocean.wind
version: 1
# 气候场本身在 config/world/climate.yaml。这里只放「风怎么作用在船上」。
velocity_cdf: [30, 50, 70, 80, 87, 96]
velocity_tail_cdf: [4, 7, 10, 13, 16, 18]
direction_adjust:
  min: WIND_DIRECTION_ADJUST_MIN    # 实现时换成整数
  max: WIND_DIRECTION_ADJUST_MAX
velocity_adjust:
  min: WIND_VELOCITY_ADJUST_MIN
  max: WIND_VELOCITY_ADJUST_MAX
calm:
  climate_kind_id: 24
  velocity: 0
```

内容包不复制公式。World0 / World1 共用同一套 sailing / wind，只换 `world/climate.yaml` 的流场。

## 7. 独立旋钮

改这些**不应**迫使贸易、战斗、人物改代码：

- 风力 CDF、无风 id、`wind_velocity_bias`
- 顺风锥角、`sail_shift`、速度地板与偏置
- 补给种类数、日耗、大厨系数（种类增加时只扩 `supply_kinds`）
- 半开是否半速
- 入港半径、空哨兵值（存档迁移表单独写）
- 每帧 tick 次数（加速档）
- 测量岗位缺员时是禁命令还是只减精度

改船帆数值走 `config/ships/`。改气候网格走 `config/world/climate.yaml`。

## 8. 对外接口

事件（Ocean 只发，不解释）：

```
ArrivedAtTown { fleet_id, town_id }
SuppliesDepleted { fleet_id, supply_kind, remaining: [int] }
StormHit { fleet_id, sea_area_id, climate_kind_id }   # 生成在遭遇表，见 02
RouteMissing { fleet_id, from_town_id, to_town_id }
NavigatorMissing { fleet_id }
FatigueChanged { fleet_id, delta }                    # 公式未还原前 delta 由公式包给
```

查询：

```
Ocean.wind_at(x, y) -> WindSnapshot { angle256, velocity, current_angle256, current_velocity, climate_kind_id }
Ocean.speed(fleet_id) -> int
Ocean.supplies(fleet_id) -> [ { kind, amount } ]
Ocean.can_plot_route(fleet_id, to_town_id) -> { ok, reason }
Ocean.position(fleet_id) -> { x, y, sea_area_id, heading256 }   # LIVE 坐标，不缓存 World*
```

命令：

```
Ocean.helm { fleet_id, heading256 }
Ocean.set_sail_mode { fleet_id, mode }          # full / half
Ocean.plot_route { fleet_id, to_town_id }       # 需测量士 + 已建立航线
Ocean.advance_calendar_step {}                  # 由会话时钟调用，扣补给
```

`SailingSnapshot`（Fleets 提供，Ocean 只读）：

```
{
  fleet_id,
  ships: [{ ship_id, square_sail, fore_sail, square_extra, fore_extra,
            durability_now, durability_max, type_code, crew }],
  supplies: [int],
  navigator_person_id,    # INVALID 则无测量士
  modifiers: ModifierSet
}
```

禁止传 `CShip*`、`World*`、`Location`。坐标用原版单位整数。

UI 投影（只读）：

- `CWheelView` / `CWheelSprite` ← `heading256`
- `CSpeedBarSprite` ← `Ocean.speed`
- `CSkySprite` / `Weather.dk4` ← `climate_kind_id`
- 海域图 `CSeaAreaTownView` / `CSeaAreaFleetView` ← id 列表，不持有实体

## 9. 未核实字段

| 项 | 依据 | 状态 |
| --- | --- | --- |
| `WIND_*_ADJUST` 整数 | 仅有 MIN/MAX 符号 | `[未知]` |
| `SUPPLYAMOUNT_MIN/MAX`、`FATIGUE_MIN/MAX` 整数 | 符号 + `UCHAR` 断言 | `[未知]` |
| `NUM_SHIPSUPPLY` 正式值 | 循环 `< 2` | `[推断]` 2 |
| 水 / 粮 与 `eShipSupply` 0、1 的对应 | 文案有两种桶 | `[未知]` |
| 每船员每日消耗、大厨系数、告警阈值 | 无公式体 | `[未知]` |
| 海上疲劳每 tick 增量 | 仅有事件文案 | `[未知]` |
| `s58/s5C/s60/s64` 与 `CShip+0x4A/+0x4C/+0x52/+0x53` 的对应 | 船有四个取值虚函数 | `[未知]` |
| `fn_140009e3a` 余弦表 | 只见调用 | `[未知]` |
| 三处 `idiv ecx` 除数 | 反编译丢掉除数 | `[未知]` |
| 船 type 0 与 7 的语义 | `NUM_SHIPTYPE=5` 与立即数 7 冲突 | `[未知]` |
| `half_speed_flag` 是否即半开帆 | `fn_14000f26d` 无名字 | `[未知]` |
| 无风气候 id `0x18` 的枚举名 | 立即数 | `[推断]` 无风 |
| `CSkySprite` 对 id `8` 的特殊枝 | 立即数 | `[未知]` |
| 入港半径、海域点击矩形（村落侧见 02，480×256 是显示空间） | | `[未知]` |
| `ePost` / 测量技能下标 | `NUM_POST` 未恢复 | `[未知]` |
| `SEAAREA_BITS` 位宽与「已发现 / 已航行」语义 | 仅有类型名 | `[未知]` |
| `COceanScene+0x208` 声音 id | `vfunc_05` | `[未知]` |
| `COceanScene::vfunc_13`（0x1401a6700） | 反编译失败，1828 字节 | `[未知]` 多半是绘制 |
| 原版坐标 → `World.dat` 像素 | 伦敦 20080,4272 | `[未知]` |
| `NUM_JUNCTION` | 头文件未写 | `[未知]` |
