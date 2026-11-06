# 航行：tick、风、帆、补给

大地图出海。Ocean 只负责把舰队从一格挪到下一格：风向、船速、补给扣减、入港触发。舵轮、速度条、天空是投影。

## 1. 职责边界

本系统管：

- 大地图 tick（`COceanScene`）与海域图导航（`CSeaAreaScene` / `CRouteMap`）
- 当前风（方向、风力）如何改航速
- 纵帆 / 横帆 / 改装帆如何改顺风、逆风速度
- 潮流（current）与船速的矢量合成
- 水和粮食的消耗与耗尽告警
- 舰队疲劳的**日增量计算**（`fn_140005a15` 日步，§2「疲劳」；海上枝 + 在港恢复枝，§14-18）；疲劳状态本身归 fleets（`Fleets.fatigue(fleet_id)`），Ocean 只调 `Fleets.add_fatigue`，`FatigueChanged` 由 fleets 发
- 入港、缺航线、无测量员的判定

不管：

- 港口内部、市价、投资、设施（Town / Economy）
- 海战与接舷结算（Combat）；本系统只发 `FleetSpotted`
- 村落 / 遗迹的坐标与发现状态表（World 拥有；Ocean 只做探索动作，见 `02-search-encounters.md`）
- 黄金航路进度与奖励（`03-gold-road.md`）
- 人物六维、技能公式（Characters 向修正器注册表 `Modifiers` 注册贡献；Ocean 只按 key 取值）
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
- `COceanMapData` 仅 0x10 字节，不是 `World.dat`。`World.dat` 是 2500×1250 `uint16` LE 贴图编号，属 World（`01-world/01-map-and-sea-areas.md`）。

### 风向与风力 `[确认]`

- 符号：`WIND_DIRECTION_ADJUST_MIN/MAX`、`WIND_VELOCITY_ADJUST_MIN/MAX`（`srcdata/catalog/constants.json`）。具体整数未从断言还原。
- RTC 名：`cWindAngle`（1 字节角）、`iWindVelocity`、`cCurrentAngle`、`iCurrentVelocity`、`sClimateStreamWind`。
- 气候在 `CWorld` / `CClimate`（`CWorldData` 基类链含 `CClimate`）。`CWorld::vfunc_04` 更新 `sClimateStreamWind`。
- 取风：`fn_140014812`（body `0x1402abc90`，`srcdata/source/game/unassigned/fn_1402aad20.cpp`）。`cWindAngle = 流场角 + fn_140003e3b()`（方向调整）。玩家舰队 happening 态 `CPlayerFleetData+0x30`（`fn_14000b87a(fn_1401ab9e0())`）`== 0x18` 时 `iWindVelocity = 0`；否则 `iWindVelocity = 流场速 + fn_140003139() + 3`（`lea eax,[rcx+rax+3]` @ `0x1402abd57`）。
- `0x18` **不是气候 id**：`+0x30` 是 happening 结局态（初值 `0x1B`=27 空闲，见 `05-sea-happenings.md` §2.3），24 = 槽 18 的结局 → `[推断，强]` 无风事件。旧稿写成「无风气候 id」是错的。
- 航速函数 `fn_14001444d` 取完风后**再做一次**同样判断：`!= 0x18` 则 `iWindVelocity += 3`（`0x1401b0357`），否则置 0。手动航速里风速偏置实际是 **+6**（两次 +3）。反编译把第一处写成 `+= 3`，丢了流场与调整项。
- 风力档：`fn_1402abb20` 先 `rand(0x64)` 再必要时 `rand(0x14)`，映射到 0–12 共 13 档。唯一调用者 `CWorld::vfunc_04`：玩家 happening 态 `== 0x1B`（空闲），或 `== 4` 且 `rand(5)==0`，且世界风计时已到期时，连同时长 `(rand(5)+10)*48` 拍（`0x1402ac245 add eax,0xa` / `0x1402ac248 imul eax,eax,0x30`，即 10–14 天；48 = `$caps.calendar_ticks_per_day`）交给 `fn_14000592a`（写一字节档位 + 到期时刻）。该档位是否经 `fn_140003139`（读另一对象 `+0x1D`）进入 `iWindVelocity` `[未知]`；它**不是**每 tick 直接掷的风速。

### 航速 `[确认]`（原始反汇编逐条核过）

主公式在未挂类函数 `fn_14001444d`（body `0x1401b0200`–`0x1401b0bfa`，`srcdata/source/game/unassigned/fn_1401ac810.cpp`），RTC：`cShipAngle`、`cSailAngle`、`cSailToWindAngle`、`iWindVelocity`、`cWindAngle`、`cCurrentAngle`、`iCurrentVelocity`、`cShipToCurrentAngle`、`eFleetShip`。由 `COceanScene::vfunc_02` 在非自动航线模式下调用。自动航线走 `fn_140008814`（同文件 `fn_140188eb0.cpp`），同样取风。

反编译在此函数丢了不少东西，以下按 `Game/DK4HD_sc.exe` 原始指令（文件偏移 = VA − 0x140000A00）为准：

- 三处 `idiv ecx` 除数**已钉死**：`mov ecx,0xA`（`0x1401b088f`）、`mov ecx,5`（`0x1401b08bc`）、`mov ecx,3`（`0x1401b08fe`）。第四处 `0x1401b07ae` 除数是 `2*need`（见下）。
- 顺风枝是 `square*cos(sail) + fore*0x80`（`imul ecx,[rsp+0xac],0x80` @ `0x1401b04d0`）。反编译写成 `square*cos + square*cos*fore`，是错的。
- 顺风锥判定的是 **`cSailAngle`（帆相对船体）**，不是 `cSailToWindAngle`（`lea rcx,[rsp+0xb4]` @ `0x1401b048d`）。
- 所有 `>> n` 前都有 `cdq; and edx,2^n-1; add`：这是**带符号向零取整的除以 2^n**，不是加「低位」。
- 只算**旗舰**（`fn_14000afb5(fleet, 0)`）的帆与人手；船首像检查扫全部 5 船。

角是 256 分圆的 `uint8`。`fn_140009e3a(a)`（body `0x140140980`）= `sin256(a + 0x40)`，`sin256` 查 65 项 `uint16` 四分之一表（`.rdata` VA `0x140683580`），幅值 **256**：

```
0,6,13,19,25,31,38,44,50,56,62,68,74,80,86,92,98,104,109,115,121,126,132,137,142,
147,152,157,162,167,172,177,181,185,190,194,198,202,206,209,213,216,220,223,226,229,
231,234,237,239,241,243,245,247,248,250,251,252,253,254,255,255,256,256,256
```

`a<=64` 取表；`<=128` 取 `t[128-a]`；`<=192` 取 `-t[a-128]`；否则 `-t[256-a]`（`fn_140009b60` body `0x140140c50`）。`[确认]`

`fn_14000baaf(n, k)`（body `0x140140dd0`）：无符号牛顿迭代 `isqrt(n*k*k)`，`n<=1` 直接返回 `n`。航速里 `k=1`。`[确认]`

`fn_140001f8c` = `abs(int)`。`fn_14000939f(a,b)` = 有符号 `max`。`[确认]`

### 船帆四值与必要人手 `[确认]` 公式，`[推断]` 位名

四个帆值与「必要人手」都由旗舰 `CShip` 现算，不存档（Ships 是唯一计算方，D6）。`size_class = CShipType+0x18`（`fn_14000d152`，查不到船模时 4；`05-ships-fleet/01` 称体型档）。两张按体型档的表（`.rdata`）：

| 表 | VA | 值 size_class 0/1/2 |
| --- | --- | --- |
| `rig_penalty_by_grade` | `0x140596d88` | 0, 4, 10（不在 0..2 时按 0） |
| `crew_per_mast_by_grade`（`ships/rig.yaml`；公式里简写 `size_base`，`05-ships-fleet/06` 同一张表） | `0x140596d98` | 3, 5, 10 |

桅字节 `CShip+0x23+m`（`m=0..2`，`CShip::vfunc_05`）：bit0 有桅，bit1 横帆（四角帆）/否则纵帆（三角帆），bit2 加横帆件，bit3 加纵帆件。改装位 `CShip+0x4E` `u16`（`CShip::vfunc_06`，屏蔽 bit15），`fn_1400091e2(ship,k)` 测 bit k。

```
sq_mast(m)   = (has_mast(m) && is_square(m) ? 7 : 0) + (bit2(m) ? 3 : 0)   # fn_140006cb2
fa_mast(m)   = (has_mast(m) && !is_square(m) ? 8 : 0) + (bit3(m) ? 4 : 0)  # fn_140001410
penalty      = rig_penalty_by_grade[t] + part2 + part3 + 2*part4 + 2*part5   # fn_14001292c
square_sail  = max(0, 8*(sq_mast(1) + 2*part0) - penalty)   # s58  fn_140006f64
fore_sail    = max(0, 8*(fa_mast(1) + 3*part1) - penalty)   # s5C  fn_140002612
square_extra = max(0, 5*(sq_mast(0) + sq_mast(2)) - penalty) # s60 fn_1400036e8
fore_extra   = max(0, 6*(fa_mast(0) + fa_mast(2)) - penalty) # s64 fn_14001150e
need = size_base[t]                                           # fn_14000a466 = Crew.min（05-ships-fleet/06 §2.4）
     + Σm has_mast(m) ? (size_base[t] + bit3(m)*1 + bit2(m)*2) : 0   # fn_14000feac
     + part0*1 + part1*4
     + 40*count(cabin_kind==11)                              # fn_1400053ee
     + 20*count(cabin_kind==8) + 2*part6 + 2*part7           # fn_140001e2e
```

`[推断]` 位名：part0 船首帆（横帆，「提高横帆的性能」）、part1 船尾帆（纵帆）、part2/3 装甲（「追加装甲板……速度会降低」，与 `anAddArmorSizeTable` 一致）、part4/5 追加船舱（「追加船舱速度会降低」）、part6/7 船首/船尾炮（战斗瞄准同参）。bit2 上帆、bit3 斜帆。cabin_kind 11 = 划桨手室、8 = 武装炮台（舱室名表顺序）。

`need` 与 `CShip+0x4C`（`vfunc+0x58`，`CShip::vfunc_11`）比较。`need` 按桅、划桨室 40、炮台 20 累加，是**必要水手数** `[推断，强]`，`+0x4C` 是当前水手数。原文写成「耐久」是错的。

### 划桨、船首像、战斗态 `[确认]` 数值，`[推断]` 语义

- 划桨 `fn_14000da9e`（body `0x14023e1b0`）：`10*count(cabin_kind==11) / size_base[size_class]`（`imul eax,eax,0xa` 后 `idiv`，除数来自同一张 3/5/10 表）。
- 门闩：`fn_140006c12(SceneDesc) == 1` **或** `fn_14000ee12(fleet) < 0x32`。前者读当前场景描述 `DAT_144296500+8` 的 `+0xC`，`[推断]` 大地图战斗态（同一标志下 tick 调 NPC 瞄准与炮弹）。后者读 `CFleet+0x71`，舰队情报面板把它画在 `posTired` / `rcTiredTitle`（`fn_140005f51`）→ `[推断，强]` 舰队疲劳度。即「疲劳 < 50 才划桨，战斗中总是划」。
- 船首像：`vfunc+0x48` = `CShip::vfunc_09` = `CShip+0x53`（初值 `0x0D`）。5 船任一为 0 → `+speed/10`；任一为 7 → `*4/5`。对照道具 114 大鹫像「船飞速前进」、121 魔鬼像「不祥」→ `[推断，强]` `figurehead_id = item_id − 114`，0 大鹫、7 魔鬼。
- 战斗态 `/3` 在**写速度条之后**（`fn_140004250` @ `0x1401b08da`），所以战斗中速度条仍显示未除 3 的值。

### 撞岸 `[确认]` 结构

位移后 `fn_14000a9e8(scene+0x88, pos)` 非 0 视为受阻。受阻且船角 `== scene+0xA8`（上次航向）时，`k=1..255` 交替调 `fn_1400052b3` / `fn_140010a78` 改船角重试，找到第一个不受阻角即 `fn_140009fd4` 定向，本拍速度写 0。航向已变则本拍不动。转向方向与步长语义 `[推断]` 交替左右逐步扩大。

### 自动调帆 `[确认]`

`scene+0x98` bit1 为真时调 `fn_140001ece`：`d = wind − ship`（mod 256）；`d<=128` 则帆角 `d/2`，否则 `256 − (256−d)/2`；写回 `CPlayerFleetData+9`（`cSailAngle`）。即帆取风与船夹角的**平分角**。bit1 由谁置位 `[未知]`（像帆按钮的自动档）。

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
- 调度 `[确认]`：舰队日步只在**日首**跑。`CFleetElement::vfunc_04` 在 `fn_14000906b`（`nTime==0`，`0x1400ff991`）为真时扫 69 支舰队，逐支 `fn_14000641f`（`0x1400ffa2a`），玩家舰队再进 `fn_140005a15`（`0x1400eef21`）。
- `SUPPLYAMOUNT_MIN/MAX` 有符号，断言宽度 `USHORT`（`asserts.txt` 310–311），整数未还原。库存在 `CShip+0x46+kind*2`（u16）。`CSupplyRateView` 是补给比例 UI，属投影。
- 日耗 `[确认]`：`fn_140005aa6`（body `0x1401dc830`），由舰队日步 `fn_140005a15`（body `0x1400fa870`，反编译失败）在舰队不在港（港槽 `== 0x62`）时调用。本篇已对原始指令复核：基础除数 `0xA`（`0x1401dcb70`）；`rand(0x1F4) <` 大厨值（`fn_14000da26(fleet, 0xC, 6)`）则 `+2`；粮食另有 `rand(0x1F4) <` 饲养员值（`(0xE, 0)`）则 `+8`；任一船 `vfunc+0x48 == 1` 则粮 `+2`；水在 `fn_14000ce78` 为真（happening 态 ∈ [2,5] = 暴风雨 / 暴风雪 / 雨 / 雪）时 `+10`；happening 态 `== 1`（有老鼠）时粮库存先 `*0x5F/0x64`。两处读的都是 `this+0x30`（`fn_14000b87a`），与无风同一字段，**不是气候**。结局名表 `.data 0x140686550`（`fn_140014803`，26 项）与「雨天省水、老鼠吃粮」互证 `[推断，强]`。消耗 `= crew*10/divisor`，余数 `rand(10) < need%10` 再 `+10`，最后 `/10`。完整式见 `13-algorithms/03-adjustment-factors.md` F-ADJ-03。
- `vfunc+0x48 == 1` 即船首像 1 = 小猪像（item 115「减少饭量」），与航速的 0 / 7 同一字段 → 船首像映射多一条旁证。
- 大厨舱「能节约水和粮食，水手也减轻疲劳。」（mesfile `0xc187`）：原版实现是「岗位技能掷骰加除数」，不是固定系数。

### 入港与航线 `[确认]`

- `NUM_TOWN=97`。Ocean 用 `0x62=98` 当空港口哨兵（`CTownNavigator` ctor 写 `+0x10/+0x11 = 0x62`；补给耗尽同样写 0x62）。
- `NUM_ROUTE=200`，`INVALID_ROUTE=-1`。`CRouteNavigator` ctor 写 `+0x8 = 0xC9`（201）`[推断]` 空航线哨兵；`vfunc_03` 循环 `eRoute < 0xC8`。
- 无航线文案：「还没有建立到那座城市的航线。」（及 7 条同义句）。
- 无测量员：「没有测量员。」×8 +「测量员不在。」
- 测量是技能也是岗位。mesfile：「测量就是根据太阳的位置了解目前所在的位置。」「测量工作有了六分仪就能顺利进行。」岗位名「测量士」。技能条数 `NUM_SKILL=10`；哪一条是测量、`ePost` 哪一个是测量士，整数未还原。
- 入港后 Ocean 停手。原版 `COceanScene::vfunc_02` 在靠近港口且补给检查通过后把城镇 id 写入 `+0xA0` 并切场景。

### 舰队疲劳日增量 `[确认]`（原始指令 `0x1400fa8b2`–`0x1400fae0a`）

`CFleet+0x71`（`fn_14000ee12` 读，`fn_140009467(fleet,d)` 写并钳 `0..100`）是舰队疲劳 `[推断，强]`（情报面板 `posTired`；`FATIGUE_*` 断言宽 `UCHAR`）。状态归 fleets（`Fleets.fatigue` / `Fleets.add_fatigue`，06「状态所有权」）；本篇只给日增量。舰队日步 `fn_140005a15` 扣完补给（`fn_140005aa6`，缺口写 `short[2]`）后立即算：

```
if 在港（fn_14000c144 != 0x62）:
    d = -(rand(5) + 2)                                         # 0x1400fadf1；每日恢复 2–6
else:
    days  = days_at_sea                                        # fn_140012e36
    tired = any_figurehead(6) ? rand(10) < rand(days)          # fn_140003f99(fleet,6)；[推断] 处女像
                              : rand(10) < days
    if tired and rand(500) < ps(captain,leadership)/2 + ps(adjutant,persuasion)/2:
        tired = false
    d = 0
    if tired: d += (rand(500) < ps(missionary,persuasion)) ? 1 : rand(2) + 1
    if rand(500) < ps(surgeon,observation)/2 + ps(cook,gunnery)/2: d -= 1
    d += (short[0] > 0) + (short[1] > 0) + (short[0] > 0 && short[1] > 0)
    d += hap_fatigue[happening_state]                          # 跳表 0x1400fb8b8，态 > 23 为 0
    if pos.y <= 2777 or pos.y >= 17222: d += 1                 # 0xAD9 / 0x4346；[推断] 高纬（约 |lat| >= 65）
    if rowing_active(fleet):                                   # fn_140006d9d = 疲劳 < 50 或 fn_14000ce19(fn_140011c89())（[推断] 海战态，同划桨门闩）
        n = 船数; m = 有划桨手室的船数                          # fn_140012634
        if n > 0 and m > 0: d += (rand(m*35/n) + 5) / 10
    if sum(Ships.cabin_count(s, recreation_room) for s in fleet) > 0: d -= rand(2)   # fn_14000dbac(…, 0x10) 娱乐室
fatigue = clamp(fatigue + d, 0, 100)
if 海上 and fatigue >= 100:                                    # 0x1400fad09
    k = ps(surgeon,observation)/50 + 90                        # fn_1400f3e60(0xB, 9)
    if k < 100: 每船 crew = crew * k / 100                       # vfunc+0x58 读、+0x80 写
```

`ps(post, skill)` = `fn_14000da26(fleet, post, skill, 0)`：岗位持有人技能值，空岗 0。岗位 / 技能下标：舰长 0·统率 1、副官 1·说服 7、传教士 10·说服 7、船医 11·观察 9、大厨 12·命中 6（`03-characters/03` 岗位表）。所有 `/2` 为带符号向零。

`hap_fatigue`（按 happening 结局 id，名表见 `05-sea-happenings.md` §2.6）：坏血病 0、奇怪的歌声 13、传染病 23 → `+2`；暴风雨 2、暴风雪 3、神秘泡泡 15、龙卷风 16、大漩涡 17 → `+1`；人鱼 14 → `−1`；其余（含空闲 27）`0`。

疲劳刚从 `<100` 升到 `>=100` 时还调 `fn_14000bcc1`，两项缓冲都 `<= 0` 才播消息 `0x29c`（语义 `[未知]`）。

文案：「疲劳度上升了%s。」「疲劳度恢复了%s。」「水手们好像很疲劳，不让他们休息一下吗？」。探索 / 温泉 / 漏水另经 `fn_140009467` 改疲劳（02、05 篇）。

### 坐标 `[确认]`

伦敦 `x1=20080 y1=4272`。这是原版坐标单位，**不要除 8**。到 `World.dat`（2500×1250 `uint16`）贴图格是带符号向零 `/16`（`fn_14000a9e8` / `fn_140014a74`，`cdq; and 0xf; sar 4`），撞岸判定 `fn_14000a9e8(scene+0x88,pos)` 走的就是它。Ocean 不自写除数，调 `Map.chip_of` / `Map.tile_class`（`01-world/01-map-and-sea-areas.md`）。与港口坐标同空间 `[推断，强]`（40000/16=2500）。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key | 备注 |
| --- | --- | --- | --- | --- |
| `eSeaArea` | 25 | `INVALID_SEAAREA=-1` | `sea_area_id` | 与 `NUM_NEXTBASETOWN=25` 同数。发现 / 访问用 `SEAAREA_BITS` |
| `eRoute` | 200 | `-1`；原版 `0xC9` 只在 `legacy_codec` | `route_id` | 港到港导航航线，不是黄金航路 |
| `eJunction` | 164 | `INVALID_JUNCTION` | `junction_id` | World 表 |
| `eTown` | 97 | `-1`；原版空哨兵 `98` 只在 `legacy_codec` | `town_id` | 入港只传 id；运行时空 = `null` |
| `eFleet` / `eFleetShip` | 69 / 5 | `-1` | `fleet_id` / `ship_slot` | 玩家船位 5 |
| `eMobileFleet` | 4 | | `mobile_fleet_slot` | 海域上的活动舰队槽 |
| `eShipSupply` | `[推断]` 2 | `-1` | `supply_kind` | 0 水、1 粮 `[推断，强]`（态 1 老鼠乘粮、态 2–5 雨雪加水除数） |
| `eShipType` | 见 `05-ships-fleet/01`（RTC 投票值不作数） | `-1` | `ship_type_id` | 航速**不**读船种。原写「type 0 / 7」实为船首像 id（`CShip+0x53`） |
| `eFigureHead` | 道具 114–123 共 10 件 | 初值 `0x0D` `[推断]` 空 | `figurehead` key | `figurehead = item − 114` `[推断，强]`。本篇读 0 大鹫 / 7 魔鬼（航速）、1 小猪（粮）、6 处女（疲劳）、9 圣母（05 篇疾病）；运行时走 `effects`，不比 id |
| 体型档 `size_class` | 0–2（表长 3） | 查不到船模时 4 | `size_class` | `CShipType+0x18`；即 `05-ships-fleet` 的 `models.size_class`；`rig.yaml` 各 `*_by_grade` 表按它下标 |
| `ePost` | 16（`03-characters/03`） | `INVALID_POST`；无岗 `0x11` | `post` key | 测量员 4 / 操帆手 5 / 舵手 6；疲劳式用 0、1、10、11、12 |
| `eSkill` | 10 | `-1` | `skill` key | 测量术 = 3（`surveying`） |
| 风向角 | 0–255 | | `angle256` | 1 字节 |
| 风力档 | 0–12 | | `wind_level` | `fn_1402abb20` |

`SEAAREA_BITS` / `SEAAREA_ID` 是位域类型符号，宽度未还原。运行时以表长为准，禁止写死 `NUM_SEAAREA=25`。

主键一律命名空间 key（`vanilla:london`）；原版哨兵 98 / 201 / 0x0D / 0xD0 / 0x46 只出现在 `ids/legacy_codec.yaml` 的导入导出层，运行时空值用 `null`（`00-overview/09-architecture-decisions.md` D1）。

## 4. 算法 / 公式

所有魔数进 `config/formulas/ocean.yaml` 与 `config/ocean/wind.yaml`。代码里只留参数名。

### 4.1 tick

```
each ocean_iteration:                                # 每帧 1 或 2 次
  wind = Climate.wind_at(fleet.x, fleet.y)          # 当前格；无风结局由 happening_state 判
  snap = Fleets.sailing_snapshot(player_fleet_id)   # 只读快照，派生量由 Ships / Characters 现算（D6）
  speed = F-OCN-01(snap, wind, sail_opening)        # 修正器只经 Modifiers.eval("fleet.speed", ctx)
  move fleet by speed along heading
  if overlaps town arrival radius: emit ArrivedAtTown
  Clock.advance(1)                                   # 原版每迭代 1 拍（fn_14000f871 @ 0x1401a62b7）
  # 补给日耗、疲劳、happening 都在 DayStarted 的舰队日步里（§4.4），不在像素 tick 里
```

原版 `COceanScene::vfunc_02` 内层循环次数：`fn_140012625(scene, 2)`（`scene+0x98` bit2）为真则 2 次，否则 1 次 `[确认]`。每次迭代都在 `0x1401a62b7` 调 `fn_14000f871`（世界前进 1 拍，`01-world/07-calendar-and-coords.md`），所以加速档 = 每帧 2 拍，**日历同样加倍**。唯一跳过：海战态（`fn_140006c12==1`）且 `fn_1400144ac` 为假。同一字节 bit1 = 自动调帆，bit0 = 横帆枝音效门（SE `0x11`，`fn_14000cf1d`）。参数名 `ticks_per_frame_fast` / `ticks_per_frame_normal`。

### 4.2 风力档

`fn_1402abb20`（`[确认]` 原始指令），`rand100 = uniform(0, 99)`，`rand20 = uniform(0, 19)`。注意：这张表在世界风计时到期时重掷（§2），不是每 tick 掷，也不直接等于航速里的 `wind_velocity`：

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

无风：玩家舰队 happening 态 `== calm_outcome_id`（原版 `0x18`=24）时 `wind_velocity = 0`，否则 `wind_velocity = stream_velocity + velocity_adjust + wind_velocity_bias`（原版 bias=3，`fn_140014812`）。手动航速函数再加一次 `manual_extra_bias`（原版 3）。两处分开配，别合并成 6。

方向：气候流 `sClimateStreamWind` + `WIND_DIRECTION_ADJUST`，合成到 `cWindAngle`。调整量上下限用符号名，整数待填。

### 4.3 航速

全部 `[确认]`（`fn_14001444d` 原始指令）。整数运算全程 `int32`，`/` 为带符号向零截断。命名（原版局部名对照）：

| 参数 | 原版 | 原版值 / 说明 |
| --- | --- | --- |
| `square_sail` / `fore_sail` | `s58` / `s5C` | 主桅（`m=1`）横 / 纵帆值，见 §2「船帆四值」 |
| `square_extra` / `fore_extra` | `s60` / `s64` | 前后桅（`m=0,2`）横 / 纵帆值 |
| `blanket_angle` | `0x20` | `abs(int(ship) − int(wind)) < 32` 时 extra 按比例缩小。**不按 256 回绕**（`fn_140001f8c` 只是 `abs`） |
| `blanket_shift` | `5` | `extra = extra * d / 32` |
| `wind_velocity` | `iWindVelocity` | 已含两次 +3，见 §2 |
| `sail_angle` | `cSailAngle` | 帆相对船体，`CPlayerFleetData+9` |
| `sail_to_wind` | `cSailToWindAngle` | `(sail_angle + ship) − wind`，mod 256 |
| `square_cone` | `0x2A` / `0xD6` | `sail_angle <= 42` 或 `>= 214` 走横帆枝 |
| `fore_fixed_cos` | `0x80` | 横帆枝里纵帆按固定 128 计，不乘 `cos(sail_angle)` |
| `cos256(a)` | `fn_140009e3a` | 幅值 256，表见 §2 |
| `sail_shift` | `17` | 两枝最后 `/ 2^17` |
| `min_speed_floor` | `0x60`=96 | 帆速 `<= 0` 时 |
| `speed_bias` | `0xC0`=192 | 帆速 `> 0` 时 `+ fore + 192` |
| `half_sail_flag` | `CPlayerFleetData+8` bit0 | `fn_14000f26d(obj, 0)`；真则 `/2`。`[推断]` 半开帆 |
| `current_shift` | `8` | 洋流交叉项 `/256` |
| `rower_gate_fatigue` | `0x32`=50 | 舰队 `+0x71 < 50` 才划桨 |
| `rower_unit` | `0xA`=10 | 每间划桨手室 |
| `rower_divisor` | `size_base[size_class]` = 3 / 5 / 10 | 划桨除数；与必要水手基数同一张表（`ships/rig.yaml#crew_per_mast_by_grade`） |
| `crew_shortfall_den_factor` | `2` | `(now+need) / (2*need)` |
| 船首像加速 | `fleet.speed` 贡献：`stage: add, order: 10`，`x/10` | 原版船首像 0 大鹫；`speed += speed/10` |
| 船首像减速 | `fleet.speed` 贡献：`stage: mul, order: 10`，`4/5` | 原版船首像 7 魔鬼；`speed = speed*4/5`（先乘后除） |
| `battle_speed_div` | `3` | `fleet.speed_move` 贡献，来源 `scene.battle`；场景描述 `+0xC == 1` 时 `/3`，在显示之后 |

```
# 0. 锁舵：CPlayerFleetData+8 bit1 为真 → 只刷新显示，整段跳过
square = square_sail + square_extra'      # extra' = 经 blanket 缩小后
fore   = fore_sail   + fore_extra'
d = abs(int(ship_angle) - int(wind_angle))
if d < blanket_angle:
    square_extra' = square_extra * d / 32 ;  fore_extra' = fore_extra * d / 32

if sail_angle <= 42 or sail_angle >= 214:            # 横帆枝
    term  = wind * (square * cos256(sail_angle) + fore * 128)
else:                                                 # 纵帆枝
    term  = wind * fore * cos256(sail_angle)
speed = term * cos256(sail_to_wind) / 2^17

speed = speed <= 0 ? 96 : speed + fore + 192
if half_sail_flag: speed = speed / 2
speed = isqrt( speed^2 + speed*cur_v*cos256(cur_angle - ship_angle) / 256 + cur_v^2 )
if battle_flag or fleet_fatigue < 50:
    speed += 10 * rower_rooms / size_base[size_class]   # rower_rooms = Ships.cabin_count(flagship, rower_room)
if crew_now < need:                                    # need = Crew.min(flagship)
    speed = speed * (crew_now + need) / (2 * need)
speed = Modifiers.eval("fleet.speed", ctx(base=speed))  # 原版：任一船像 0 → ×11/10（mul order 10）；任一船像 7 → ×4/5（mul order 20）
show(speed)                      # 速度条 fn_140004250：speed/8（带符号）
speed = Modifiers.eval("fleet.speed_move", ctx(base=speed))  # 原版：海战态 /3（scene.battle 贡献）
move(heading=ship_angle, length=speed)
```

要点：

- 洋流交叉项系数是 **1** 不是 2，所以不是精确的 `|v_sail + v_cur|`。`[确认]`
- 横帆枝（帆接近与船体垂直）里纵帆贡献固定 `fore*128`。纵帆枝里横帆完全不计。`[确认]`
- 正速度时 `+ fore + 192`：纵帆值越大底速越高。这是原版手感的一部分，别「修」。
- 中间值可能溢出 `int32`（`wind*square*256*256`）。原版就是 32 位回绕；重构默认用 64 位，`overflow_mode: wrap32` 留给比对测试。

操帆手 / 操舵手：本函数**不读**任何人物能力或技能。`fleet.sail_handling` / `fleet.helm` 是重构预留修正器 key，原版包无贡献。

测量：无测量士时禁止「定位 / 开辟航线」命令，不改船速。

### 4.4 舰队日步：补给消耗与疲劳

`[确认]` 按舰队日步（`fn_140005a15`，每个日首都跑，不限海上：在港只走疲劳恢复枝，海上走 补给 → 疲劳 → 人物 → 掷骰；词表 §14-18），不按像素。海域图 `nTime == 0` 的 `fn_140004101` 只重算总和做告警，不扣库存。原版日步内顺序（`tick.yaml` 的 `daily` phase 步骤照抄，改序即改数值）：

| 序 | 子阶段 | 原版位置 | 归属 |
| ---: | --- | --- | --- |
| 1 | `ocean.supply_drain` | `fn_140005aa6` @ `0x1400fa8e0` | Ocean 调 F-ADJ-03 |
| 2 | `fleets.fatigue` | `0x1400fa8e5`–`0x1400fae0a`（§2） | Ocean 算 `d`，Fleets 存（`Fleets.add_fatigue`） |
| 3 | `ocean.happening_upkeep`（happening 态续期等，`0x1400fae0f`–`0x1400fb1a6`） | 未逐条还原 `[未知]` | Ocean |
| 4 | `characters.vitals` | `0x1400fb3aa`–`0x1400fb5fc` | `03-characters/07` §2.11 |
| 5 | `ocean.happening_roll` | `fn_1400f2d90` @ `0x1400fb76d` | `05-sea-happenings.md` |
| 6 | `characters.mental_transition` | `fn_1400ec260` @ `0x1400fb841`（随后 `fn_14000a8fd` 写态） | `03-characters/07` §2.11 |
| 7 | `ocean.happening_persist` | `fn_1400f0450` @ `0x1400fb879` | `05-sea-happenings.md` |

注意：人物日结读的是**前一日**留下的 happening 态，当日掷骰在其后。

在港（港槽 `!= 0x62`）时同一函数只走第 2 步的在港枝 `-(rand(5)+2)`：`0x1400fa8cd` 跳过补给与海上疲劳，`0x1400fb392` 跳过第 3–7 步 `[确认]`。重构：`daily` phase 不挂 `when: at_sea`，步骤内按 `Fleets.location(fleet_id).town_id` 分枝；旅馆每夜推进日首时同样触发此在港枝（`08-town-life/05`）。

日耗公式本体归 `13-algorithms/03-adjustment-factors.md` F-ADJ-03，Ocean 只调用：

```
for ship in fleet, kind in supply_kinds:          # 原版 5 船 × 2 类
    div = drain_div_base                           # 10
    if rand(skill_roll) < ps(cook, gunnery):  div += cook_div_bonus           # 500 / +2
    if kind == food:
        if rand(skill_roll) < ps(keeper, vitality): div += keeper_food_bonus  # +8
        div += Modifiers.eval("supply.food_div")             # 原版小猪像 +2
        stock = stock * Modifiers.eval("supply.food_stock_pct") / 100   # 原版态 1 有老鼠 → 95
    if kind == water: div += Modifiers.eval("supply.water_div")       # 原版态 2–5 暴风雨/暴风雪/雨/雪 → +10
    need  = crew * 10 / div
    need += (rand(10) < need % 10) ? 10 : 0
    stock = max(0, stock - need / 10)              # 不足部分记 shortage[kind]，供疲劳式用
if any kind sum <= 0: emit SuppliesDepleted { fleet_id, kind }
```

`ps(post, skill)` = `Characters.skill(Fleets.post_holder(fleet, post), skill)`，空岗 0（D2）。船首像加成不在公式里比较 id 1：由 `ships/figureheads.yaml` 的 `effects` 向 `supply.food_div` 注册贡献。happening 态改补给也不在公式里比较态号：`ocean/happenings.yaml` 结局 `rats` 的 `effects` 登记 `supply.food_stock_pct override 95`，`storm` / `blizzard` / `rain` / `snow` 登记 `supply.water_div add 10`（态持续期间生效）。告警文案分档（「就要没了 / 有点不放心 / 有点儿少」）的阈值 `[未知]`；原版 `fn_140009f1b` 只在某类总和 `<= 0` 时触发并改入港槽。

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
- 无风结局 id `0x18`、半帆位、船首像 id 0/1/6/7、划桨室种类 11、娱乐室 16、岗位 0/1/10/11/12、体型档表写在公式里
- 航速只看旗舰的帆与人手，船首像却扫全队
- 入港空哨兵 98 与 `ITEM_EMPTY=106` 不是同一套，Ocean 自己用 98（重构：运行时 `null`，98 只在 `legacy_codec`）

重构切断：

- Ocean 只收 `Fleets.sailing_snapshot(fleet_id)` 与 `WindSnapshot`
- 入港只发 `ArrivedAtTown`，不 new 城镇设施
- 战斗只发 `FleetSpotted`，不调 `CCombatScreen`
- 气候场留在 World；Ocean 只订阅当前格的风
- UI 字符串全部走 `ui/strings.*.yaml`

## 6. 解耦后的配置

`config/formulas/ocean.yaml`（D3 形状 `impl / int_mode / quirks / params / tables / refs / golden`，词表 §14-15）：

```yaml
schema: dk4.formulas.ocean
version: 4
trig:                          # 共用常量，不是公式
  params:
    angle_modulus: 256
    amplitude: 256
  tables:
    quarter_table_ref: ids/sin256_quarter.yaml   # 65 项，幅值 256，原版 .rdata 0x140683580（按段导出，不手抄）
sailing_speed:                 # F-OCN-01，全部 [确认]
  impl: vanilla.sailing_speed  # 控制流重，走 ③；模组可换 <pack>.<name>
  int_mode: { div: trunc, width: int64 }   # 对拍时切 vanilla_i32
  quirks:
    blanket_abs_no_wrap: true  # abs(ship-wind) 不按 256 回绕
    current_cross_coef_1: true # 洋流交叉项系数 1，非精确 |v+c|
    display_before_move_mods: true   # 速度条取 fleet.speed，不含 fleet.speed_move
  params:
    blanket_angle: 32          # 0x20
    blanket_shift: 5
    square_cone_low: 42        # 0x2A，比较的是 sail_angle
    square_cone_high: 214      # 0xD6
    fore_fixed_cos: 128        # 横帆枝里纵帆系数
    sail_shift: 17
    min_speed_floor: 96        # 0x60
    speed_bias: 192            # 0xC0，另加 fore
    half_sail_div: 2
    current_cross_shift: 8
    rower:
      gate_fatigue_below: 50   # Fleets.fatigue
      gate_in_battle: true
      unit: 10                 # 每间划桨手室
      divisor_ref: ships/rig.yaml#crew_per_mast_by_grade
    crew_shortfall_den_factor: 2   # (now+need)/(2*need)
  refs: { cabin_role: rower_room }   # Ships.cabin_count(ship, role)，只用于计数
  modifiers: { display: fleet.speed, move: fleet.speed_move }   # 船首像等 mul order 10 → 20；海战态 /3 等在显示之后
  golden: tests/ocean/sailing_speed.golden.yaml   # 原版逐位对拍
wind:                          # F-OCN-01 的输入偏置（§2「无风」），不是独立公式
  params:
    wind_velocity_bias: 3      # fn_140014812
    manual_extra_bias: 3       # fn_14001444d 再加一次
  refs: { calm_outcome: calm } # 结局 key；原版 outcome 24（0x18），不是气候
iteration:                     # 每次迭代 Clock.advance(1)
  params: { ticks_per_frame_normal: 1, ticks_per_frame_fast: 2 }
supply_drain_ref: formulas/supply.yaml#supply_drain   # F-ADJ-03 / F-OCN-05（13-algorithms/03 §6），Ocean 不重复定义
crew_fatigue:                  # §2「舰队疲劳日增量」[确认]
  impl: vanilla.crew_fatigue_daily
  int_mode: { div: trunc, width: int64 }
  params:
    in_port: { base: -2, rand: 5 }            # -(rand(5)+2)；daily 在港枝（词表 §14-18）
    tired_roll: 10                            # rand(10) < days
    tired_days_randomized_key: fleet.fatigue_days_randomized   # bool；原版船首像 6
    resist_roll: 500
    shortage: { per_kind: 1, both_extra: 1 }
    high_lat: { y_le: 2777, y_ge: 17222, add: 1 }   # 原版坐标；[推断] 高纬
    rowing: { mul: 35, add: 5, div: 10 }      # (rand(m*35/n)+5)/10
    recreation_rand: 2
    clamp: [0, 100]
    exhausted:                                # 疲劳 >= 100
      at: 100
      keep_pct: { base: 90, post: surgeon, skill: observation, div: 50, cap: 100 }
  tables:
    resist: [ { post: captain, skill: leadership, div: 2 }, { post: adjutant, skill: persuasion, div: 2 } ]
    calm_post: { post: missionary, skill: persuasion }          # 过则 +1，否则 +rand(2)+1
    relief: [ { post: surgeon, skill: observation, div: 2 }, { post: cook, skill: gunnery, div: 2 } ]
    by_happening_ref: ocean/happenings.yaml#outcomes.*.daily.fatigue   # 原版：坏血病/歌声/传染病 +2，暴风雨/暴风雪/泡泡/龙卷风/漩涡 +1，人鱼 −1
  refs: { cabin_role: recreation_room }     # 娱乐室计数
```

船体侧（帆值、必要水手）是 Ships 的派生量（D6）：必要水手 = `Crew.min(ship)`（`05-ships-fleet/06-crew-headcount.md` §2.4，`ships/crew.yaml#capacity`），本篇不再重复人手参数。四个帆值的参数表是 `config/ships/rig.yaml`，只在 `05-ships-fleet/04` 定义一次（本篇 §2 的乘数、罚值、改装件加成已并入该表）；Ships 现算后放进 `Fleets.sailing_snapshot(fleet_id)` 的 `flagship` 段，Ocean 不读 rig.yaml。

船首像效果属 `config/ships/figureheads.yaml`，只在 `05-ships-fleet/04` 定义（词表 §14-13），以修正器贡献给出（D2，`stage` / `order` 复刻原版取整；`scope: fleet_any, stack: once` = 原版「任一船装了即生效、不叠加」）。本篇只按 key 读：`fleet.speed`（`eagle` 原版 figurehead 0 `s + s/10`、`devil` figurehead 7 `*4/5`）、`supply.food_div`（`piglets` figurehead 1 `+2`）、`fleet.fatigue_days_randomized`（`maiden` figurehead 6 `[推断]`），不复制该表。

海战态 `/3` 由 Session 场景态注册：`{ key: fleet.speed_move, stage: mul, order: 900, value: { num: 1, den: 3 }, source: scene.battle }`。

`config/ocean/sailing.yaml`：

```yaml
schema: dk4.ocean.sailing
version: 2
supply_kinds:
  - { key: water, name_key: ui.supply.water }   # 原版 0
  - { key: food,  name_key: ui.supply.food }    # 原版 1
# 结局态改补给不在这里：ocean/happenings.yaml#outcomes.*.effects 登记 supply.food_stock_pct（rats）/ supply.water_div（storm/blizzard/rain/snow）
arrival_radius: null           # [未知] 原版坐标单位
sail_opening:
  - { key: full, half_speed: false }
  - { key: half, half_speed: true }   # 原版 CPlayerFleetData+8 bit0 [推断]
required_posts:
  navigator: { post: surveyor, fail_message: ui.ocean.no_navigator }
modifiers: [fleet.navigation, fleet.sail_handling, fleet.helm]   # D3 出口列表（同 07-combat/01 写法）；原版包无贡献
# 空港 98 / 空线 201 不在这里：见 ids/legacy_codec.yaml（D1）
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
  happening_outcome: calm       # 原版 outcome 24；按 key 引用 05 篇结局表
  velocity: 0
```

调度进 `config/engine/tick.yaml`（D4）：

```yaml
- { id: player_fleet, system: ocean, schedule: [tick] }   # 由 COceanScene 迭代驱动（fn_140011414）
- id: daily                    # 原版 fn_14000906b 门 → 每支舰队 fn_14000641f → 玩家 fn_140005a15
  schedule: [day_start]
  # 不挂 when: at_sea：在港也跑 fleets.fatigue 的在港枝 -(rand(5)+2)，其余步骤在港跳过（§4.4）
  steps: [ocean.supply_drain, fleets.fatigue, ocean.happening_upkeep, characters.vitals, ocean.happening_roll, characters.mental_transition, ocean.happening_persist]
```

内容包不复制公式。World0 / World1 共用同一套 sailing / wind，只换 `world/climate.yaml` 的流场。

## 7. 独立旋钮

改这些**不应**迫使贸易、战斗、人物改代码：

- 风力 CDF、无风结局 key、`wind_velocity_bias`
- 顺风锥角、`sail_shift`、速度地板与偏置
- 补给种类数、日耗、大厨系数（种类增加时只扩 `supply_kinds`）
- 半开是否半速、战斗减速除数、船首像修正（修正器 `stage` / `order`，不是 id 判断）
- 划桨门闩（疲劳阈值、战斗是否总划）
- 疲劳日增量各项（岗位抵抗、缺粮、happening 表、高纬、划桨、娱乐室、≥100 损员）
- 两处风速偏置分开配
- 入港半径；原版空哨兵只在 `legacy_codec`
- 每帧迭代次数（加速档；每迭代 1 拍）
- `daily` phase 步骤顺序（`tick.yaml`，属规则包版本）
- 测量岗位缺员时是禁命令还是只减精度

改船帆数值走 `config/ships/`。改气候网格走 `config/world/climate.yaml`。

## 8. 对外接口

事件（Ocean 只发，不解释）：

```
ArrivedAtTown { fleet_id, town_id }
SuppliesDepleted { fleet_id, kind }                   # kind = supply_kinds key；载荷按 06
StormHit { fleet_id, major_sea_id, sea_area_id?, kind }   # kind = happening 结局 key，见 05；载荷按 06；major_sea_id 必填，sea_area_id 可选、默认 null（§13-2）
RouteMissing { fleet_id, from_town_id, to_town_id, force_id | null }   # 与 01-world/05 同形
NavigatorMissing { fleet_id }
CrewAttrition { fleet_id, keep_pct }                  # 疲劳 >= 100；Ships 按比例减水手
```

查询：

```
Climate.wind_at(x, y) -> WindSnapshot { angle256, velocity, current_angle256, current_velocity, climate_kind }   # World 提供
Ocean.speed(fleet_id) -> int                    # fleet.speed（显示值）
Ocean.supplies(fleet_id) -> [ { kind, amount } ]
Ocean.can_plot_route(fleet_id, to_town_id) -> { ok, reason }
Fleets.location(fleet_id) -> { town_id | x, y }   # fleets 提供；LIVE 坐标，不缓存 World*；航向 heading256 是 Ocean 运行态
```

命令：

```
Ocean.helm { fleet_id, heading256 }
Ocean.set_sail_opening { fleet_id, full|half }
Ocean.plot_town { fleet_id, to_town_id }        # 需测量员 + Routes.path 有解（04 篇）
```

命令（发往 fleets）：`Fleets.add_fatigue(fleet_id, delta, reason)`，reason = 日步 / 探索 / happening；fleets 钳 0..100 并发 `FatigueChanged { fleet_id, old, new }`。

订阅（D4）：`DayStarted` → `daily` phase 步骤（§4.4）；Ocean 不自建计时器。

叙事触发源（`10-narrative/03-event-commands.md`）：航海场景 tick 问 kind 6（航海中），探索命中道具问 kind 0，登陆村落问 kind 8，舰队接触问 kind 3。Ocean 只在这些点发场景切换事件，不解释条件。

`Fleets.sailing_snapshot(fleet_id)`（Fleets 汇总，派生量各有唯一计算方，D6；Ocean 只读，不存档）：

```
{
  fleet_id,
  flagship: { ship_id, square_sail, fore_sail, square_extra, fore_extra,   # Ships 按 rig.yaml 现算
              crew_now, crew_needed, size_class },             # crew_needed = Crew.min
  rower_rooms,            # Ships.cabin_count(flagship, rower_room)
  crew_fatigue,           # Fleets.fatigue(fleet_id)；CFleet+0x71 [推断，强]
  scene_mode,             # Session.mode()：manual | auto | battle（SceneDesc +0xC / +0x10）
  happening_state,        # 结局 key | null（05 篇）
  supplies: { water, food },
  navigator,              # Fleets.post_holder(fleet, surveyor) | null
}
# 修正器不进快照：消费方按 key 调 Modifiers.eval（D2）
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
| 水 / 粮 与 `eShipSupply` 0、1 的对应 | 态 1（有老鼠）只乘 kind 1；态 2–5（雨雪）只加 kind 0 的除数 | `[推断，强]` 0 水 1 粮 |
| 日耗公式 | `fn_140005aa6` 原始指令 | `[确认]`（见 §4.4） |
| 告警分档阈值 | `fn_140009f1b` 只判 `<= 0` | `[未知]` |
| 海上疲劳日增量 | `fn_140005a15` `0x1400fa8e5`–`0x1400fae0a` 原始指令 | `[确认]`（§2）；高纬阈值语义、消息 `0x29c` `[推断]/[未知]` |
| 日步 `0x1400fae0f`–`0x1400fb1a6` 段（态续期、冰盖、`fn_14000a8fd`） | 只列了调用 | `[未知]` |
| 舰队日步调度 | `fn_14000906b` 门 @ `0x1400ff991` → `fn_14000641f` → `fn_140005a15` | `[确认]` 日首 |
| `s58/s5C/s60/s64` 来源 | 由桅字节 `+0x23..0x25` 与改装位 `+0x4E` 现算，见 §2 | `[确认]` 公式；位名 `[推断]` |
| `fn_140009e3a` 余弦表 | `.rdata 0x140683580` 65 项 | `[确认]` |
| 三处 `idiv ecx` 除数 | 原始指令 `0xA` / `5` / `3` | `[确认]` |
| `CShip+0x53`（`vfunc+0x48`）= 船首像 id | 0 加速 / 1 省粮 / 6 疲劳 / 7 减速 / 9 防病；道具 114+ 文案 | `[推断，强]` |
| `CShip+0x4C` = 当前水手数、`fn_14000a466` = 必要水手数 | 累加项是桅 / 划桨室 40 / 炮台 20 | `[推断，强]` |
| `CFleet+0x71` = 舰队疲劳 | 情报面板画在 `posTired` | `[推断，强]` |
| `SceneDesc+0xC == 1` = 大地图战斗态 | 同标志下 tick 调 NPC 瞄准、炮弹 | `[推断]` |
| 半开帆位 `CPlayerFleetData+8` bit0 的写入者 | 只见读 | `[未知]` |
| 自动调帆位 `scene+0x98` bit1 的写入者 | 只见读 | `[未知]` |
| 帆角 `cSailAngle`（`CPlayerFleetData+9`）玩家怎么改 | `fn_1401b9940.cpp` 有写入 | `[未知]` |
| happening 结局 24 = 无风 | 风速置 0 的唯一条件 | `[推断，强]` |
| `CSkySprite` 对 id `8` 的特殊枝 | 立即数 | `[未知]` |
| 入港半径、海域点击矩形（村落侧见 02，480×256 是显示空间） | | `[未知]` |
| `ePost` / 测量技能下标 | 岗位名表 16 项、技能名表 10 项（`03-characters/03`） | `[确认]` 测量员 4、测量术 3 |
| `SEAAREA_BITS` 位宽与「已发现 / 已航行」语义 | 仅有类型名 | `[未知]` |
| `COceanScene+0x208` 声音 id | 每帧 `fn_1400083e6` 写：七大海域 0..6 → BGM 4..10，否则 `0x1B`（`01-world/08`） | `[确认]` |
| `COceanScene::vfunc_13`（0x1401a6700） | 反编译失败，1828 字节 | `[未知]` 多半是绘制 |
| 原版坐标 → `World.dat` 贴图格 | `/16` 向零（`fn_14000a9e8` / `fn_140014a74`） | `[确认]` 除数；与港口同空间 `[推断，强]` |
| `NUM_JUNCTION` | exe 折点表 164 条（`01-world/05`） | `[确认]` 164 |
