# 大地图 NPC 舰队 AI

世界表里的 69 支 `eFleet` 多数时候只是「驻港 / 有目的地」的记录。玩家在大地图上看到的移动船，是另外 4 个 `eMobileFleet` 槽和 20 艘 `eMobileShip`。本篇管 **这些槽怎么占用、怎么转向、怎么追上玩家、怎么回港**，以及 **海域图名单和大地图船体为什么不是同一份列表**。

地方舰队买货走 `04-economy/03-trade-loop.md`。海战里 NPC 打谁、happening 五命令走 `07-combat/03-battle-ai.md`。本系统只把舰队放到坐标上，并发 `FleetSpotted`。

## 1. 职责边界

本系统管：

- 大地图 tick 里给 NPC 占用 / 释放 `mobile_fleet_slot`
- 槽上的种类 `mobile_fleet_kind_id`、目标种类 `fleet_aim_kind_id`、分带距离
- 出航（离开 `town_id`）与远距回槽（入港 / 从大地图消失）
- 非战斗时跟旗舰走；战斗时按 aim 算 `heading256`
- 接触玩家后只发遭遇事件，不结算炮弹

不管：

- 玩家自己操舵、航速、补给（`01-sailing.md`）
- 探索圈、海豚 / 老鼠 / 风暴（`02-search-encounters.md`）；本系统只提供「附近有哪支舰队」
- 黄金航路（`03-gold-road.md`）
- 地方舰队编制、账本、自动买货（`05-ships-fleet/02-fleet-proxy.md`、`04-economy/03-trade-loop.md`）
- 海战 AI、选目标、happening 2–6（`07-combat/03-battle-ai.md`）
- 月结地方舰队互打、上缴、买船（`CFleetElement::vfunc_04` / `CForceElement::vfunc_04` 属 fleets / factions / combat）
- 广场情报文案「%s的舰队正驶向%s」（town-life 投影；本系统只保证有 `docked_town_id` / `dest_town_id` 可查）
- `CDrawShipManager` / `CSeaAreaFleetView` 贴图

场景切分（原版已有）：

- `COceanScene` — 大地图。最多 4 支活动舰队、20 艘可见船
- `CSeaAreaScene` — 海域图。图标来自 **69 条世界舰队**，不读那 4 个槽

## 2. 原版事实

### 2.1 两套对象 `[确认]`

| 层 | 符号 | 计数 | 对象 | 来源 |
| --- | --- | --- | --- | --- |
| 世界舰队 | `eFleet` | 69 | `CFleet` 步长 `0xC8`，挂在 `CFleetElement+0x8` | `FleetID.h`、`CFleetElement.cpp` |
| 活动舰队槽 | `eMobileFleet` | 4 | `CFleetElement+0x35f0` 起 4 条 4 字节 | `MobileFleetID.h`、`num_values.json` |
| 活动船 | `eMobileShip` | 20 | `CMobileShip` 步长 `0x30`，`CMobileShipElement+0x18` | `MobileShipID.h`、`CMobileShip.cpp` |
| 炮弹池 | `eShell` | 100 | 挂在 `CMobileShipElement+0x1230` | 战斗弹药，不是本系统 |

`CMobileShip` 继承 `CShipObject -> CShipBase -> CLocator`。析构按 `0x30` 释放。`+0x18 == 0xD0` 视为空船（与 `CFleetShip` 空槽同一哨兵）。

`CDrawShipManager` ctor 在 `+0x3b0` 建 **20** 个步长 `0x60` 的绘制对象，与 `NUM_MOBILESHIP` 同数。这是投影。

`COceanScene` ctor 对 `+0x130` 循环上界 4，给 4 个活动舰队 UI 指针槽。

### 2.2 活动槽 4 字节 `[确认]` 布局，`[推断]` 字段名

`CFleetElement` 存档对 `idx < 4` 各写 `+0x35f0+idx*4` 的 4 个字节。访问函数：

| 偏移 | 读写 | 事实 |
| --- | --- | --- |
| `+0` | `fn_14000d9c2` 读；`fn_140014542` 写 | `fleet_id`。`fn_14000a407`：值 `== 0x46` 则空 |
| `+1` | `fn_140001947` 写；`fn_1400068cf` 把指针 +1 再读 | `mobile_fleet_kind`。代码里出现 0、1、2、3、5 |
| `+2` | `fn_140002f81` 读 `this+2` | 战斗转向 switch 0/1/2/3，对应 `eFleetAimKind` `[推断]` |
| `+3` | `fn_14000f268` 读 `this+3` | 选目标分支 0 / 1 / 其它 |

`0x46 = 70`。`NUM_FLEET = 69`，70 是舰队 id 越界哨兵，**不是** `eFleetState`。`fn_14000cd8d` 里 `fleet_id == 0x46` 则 skip，含义是空槽。已有遭遇文档把 `0x46` 写成「船态」，本篇以槽字节为准。

`NUM_MOBILEFLEETKIND`、`NUM_FLEETAIMKIND`、`NUM_FLEETSTATE`、`NUM_MOBILESHIPSTATE` 只有 World DLL 断言，整数未恢复。`FleetaimkindID.h` / `MobilefleetkindID.h` 枚举体是空的。

释放槽 `fn_1400038c3`（`0x1401018c0`）：对 5 艘船 `fn_14000cb71`（解绑并 `+0x18=0xD0`），再写 `fleet_id=0x46`、`kind=5`。

占用槽 `fn_140002f18`（`0x1400eb1e0`）：参数含 `fleet_id` 与 kind。若该舰队当前 `town_id != 98` 且 kind `!= 5`，先 `fn_14000fd5d` 离港；再写入槽，并 `fn_14000feed` 把船填进 20 艘池。

### 2.3 `CMobileShip` 存档字段 `[确认]`

`vfunc_07` / `vfunc_06` 成对读写：

| 偏移 | 宽 |
| --- | --- |
| `+0x8` | dword |
| `+0xC` | dword |
| `+0x10` | dword |
| `+0x14` | dword |
| `+0x16` | u16 |
| `+0x18` | u16（空=`0xD0`） |
| `+0x1A` | u8 |
| `+0x1B` | u8 |

运行时另有 `+0x26`、`+0x28`（`vfunc_14` / `vfunc_16` 按船 type 7 / `0xB` 填）。语义未命名，禁止写成帆角确认。

`CMobileShipElement` 另外：

- `+0x3c8`：4 × **65** 条轨迹。每条 `sLocus` 12 字节（1+4+4，存档循环 `s24 < 0x41`）
- `+0x1038`：21 × `0x18` `[未知]`
- `+0x1230`：100 个炮弹
- `+0x1030` 初值 `0x46`；`+0xff8` 初值 `0x62`

`fn_140008643` 拷 65 条轨迹（占槽时）。`fn_140007914` 把 64 条往前挪再写入最新点（环缓冲）。`fn_14000bf37` 按下标取 `+0x3c8` 那条。

### 2.4 世界舰队上与 AI 有关的字节 `[确认]` 偏移，`[推断]` 名

`CFleet` 存档含 `+0x70..+0x75`、`+0x7C`、`+0x80`。构造：`+0x70..+0x74=0`，`+0x75=5`，`+0x7c=1`。

| 偏移 | 访问 | 事实 |
| --- | --- | --- |
| `+0x70` | `fn_14000c144` 回退；`fn_14000bf6e` | 驻港 `town_id`。98=`0x62` 表示不在港 |
| `+0x80` | `fn_14000c144` 优先；`fn_14000db66` 非空则真 | 目的地 / 第三字节城镇 `[推断]` |
| `+0x72` | `fn_14000aeac` | 状态。`== 6` 时大地图不刷、海域图默认不画 |
| `+0x10` | `fn_1400075cc`：`== 0xC5` | World0 舰队 62 模板首字节是 `0xC5`。海域图跳过这一支；远距释放时多一次 `fn_140007117` |
| `+0x68` | `fn_140009188` 从港口坐标写入 | 无旗舰时 `CFleet::vfunc_02` 用 `+0x60` 当点 |

离港 `fn_14000fd5d`：写 `+0x80`，若在港则把 `+0x70` 拷走再清。入港判定本身是「`town_id != 98`」。

### 2.5 距离分带 `[确认]` 阈值，`[推断]` 返回值

`fn_140008530`（`0x14013f970`，RTC `delta`）：

1. `fn_14000b5ff` 得 `dx, dy`（原版坐标单位）
2. 可选写下 `isqrt(dx²+dy²)`
3. 四段比较，反编译丢掉了 `return` 立即数。调用方只用 0、1、2、3：

| 条件 | 调用方当作 |
| --- | --- |
| `\|dx\| < 0x60` 且 `\|dy\| < 0x60`（96） | 0 最近 |
| 否则 `\|dx\| < 0xC0` 且 `\|dy\| < 0x80`（192 / 128） | 1 |
| 否则 `\|dx\| < 0xC0` 且 `\|dy\| < 0xC0`（192） | 2 |
| 否则 | 3 最远 |

哪一段精确对应 1 还是 2：第二段的 y 阈值是 `0x80` 不是 `0xC0`，带是扁的。返回值编号标 `[推断]`，阈值标 `[确认]`。

### 2.6 何时出现在大地图 `[确认]`

`COceanScene::vfunc_02` 在 `fn_140006c12 != 1`（非战斗）时：

1. `fn_140013d95`：活动槽 1..3，相对玩家分带 `== 3` → `fn_1400038c3` 释放
2. `fn_1400100b9`：扫 69 支世界舰队，跳过玩家自己、未激活、`fn_14000c1d0`、状态 `== 6`。分带 0 或 1–2（与第二参数有关）则 `fn_14000916f(..., kind=5)` → `fn_140014cf4` 占槽
3. `fn_14000cd8d`：近距离开战检查

槽下标 **从 1 起**。0 槽不进这些循环 `[推断]` 玩家。`fn_140014cf4` 分配 1..3；槽 3 对某些状态（0 / 7–8）有额外拒绝 `[未知]`。

`fn_140012a99`（`0x1400eb370`）是另一套占槽：扫 1..3，按是否已有 kind=2、以及 `fn_140012dc8` 关系值选 kind ∈ {1,2,3}，再 `fn_140008643` 填轨迹。反编译目录里未见其它 cpp 直接 call，可能是函数指针 / 未挂名调用。关系值：无 kind=2 则派 2；关系 `== 2` 派 3；否则派 1。

战斗中 `fn_14000974b` 也会占用 1..3，并把 20 艘 `CMobileShip` 绑到场景（含 type `== 0xA` 的特殊船）。开战流程属遭遇，见 `02`。

### 2.7 何时出现在海域图名单 `[确认]`

`fn_14020bbd0.cpp` 从 `eFleet = 0x44`（68）递减到 0，对每支 `CFleet`：

- 未激活 → 跳过
- `CFleet+0x10 == 0xC5` → 跳过
- 状态 `== 6` 且（不在玩家同港、且另外两个「在海」查询为假）→ 不画
- 与玩家同 `town_id`（且玩家不在 98）或 `fn_14000db66`（`+0x80` 有值）或 `fn_1400040c0` → 可画
- 再与海域归属 `s78`、可见旗 `s74` 合取
- 通过则 `new CSeaAreaFleetView`（`0x14020c7d0`），对象 `0x150`，挂在场景 `+0x110+slot*8`

这是 **69 选 N 的图标列表**，不是 4 个活动槽的投影。同一支舰队可以在海域图上有图标，但尚未进入大地图 4 槽（玩家还没驶近）。反过来，大地图 4 槽里的船离开分带 3 后图标仍可留在海域图——只要世界舰队记录还在该海域 / 该港。

`CSeaAreaFleetView::vfunc_01` 只 `fn_1400041a6` 再 `fn_14000279d` 取关联舰队，不持有 `CFleet*` 以外的玩法状态。

### 2.8 大地图怎么动 `[确认]`

`COceanScene::vfunc_02` 在航速之后：

| `fn_140006c12` | 调用 | 作用 |
| --- | --- | --- |
| `== 1` 战斗 | `fn_140003acb` | 槽 1..3：按 `+3` 选目标，按 `+2` 算 `cAngle`，`fn_14000c44b` 用风 / 流转向，写轨迹与船位 |
| 非 1 | `fn_14000d855` | 槽 1..3：旗舰跟 `CFleet` 定位点，写轨迹，不走 aim switch |
| 总是 | `fn_1400122bf` | **四槽都扫**，船位 1..4 跟旗舰。战斗中特殊船可单独取角 |

`fn_140003acb` 目标（`+3`）：

| 值 | 函数 | 能看到的行为 |
| --- | --- | --- |
| 0 | `fn_140006eba` | 一种目标 |
| 1 | `fn_140004f66` | 扫对方 5 船，取 `vfunc+0x50` 最小者 `[推断]` 最近 |
| 其它 | `fn_14000fe3e` | 第三种 |
| 已有 `fn_1400149ac` | `fn_140004e6c` | 直接对准玩家定位点 |

然后 `fn_140002f81`（`+2`，aim）→ `cAngle`：

| 值 | 函数 | 能看到的几何 |
| --- | --- | --- |
| 0 | `fn_14000f1b9` | 远则对目标角偏 `0x40`（256 分圆的 90°）。相对角 `> 0x80` 加，否则减 |
| 1 | `fn_1400103c0` | 目标向量，无偏置 |
| 2 | `fn_140007513` | 远：同 90° 偏置；近：再偏 `0x80`（180°） |
| 3 | `fn_140007987` | 偏 `0x80`，并 `fn_140008de1(..., 3)` |
| 其它 | `fn_1400129b8` | 偏 `0x80`，并 `fn_140008de1(..., 4)` |

不要把这些 id 写成「巡逻 / 追击 / 逃跑」枚举名。几何是确认的；玩法名是 `[推断]` 占位 key。

`fn_14000c44b` 的 RTC 含 `cWindAngle`、`iWindVelocity`、`cCurrentAngle`、`iCurrentVelocity`。战斗中 NPC 转向走与玩家类似的风一流合成，参数仍在 `formulas/ocean.yaml`，本系统不复制一份航速公式。

非战斗的 `fn_14000d855` **不**调用 aim switch。世界舰队的目的地推进如果存在，不在这条大地图 tick 里被还原——月结 `CFleetElement::vfunc_04` 扫的是互打和账本，不是轨迹。广场文案「正驶向%s / 停泊在%s」证明运行时有目的地与驻港两态，推进公式 `[未知]`。

### 2.9 追玩家与接触 `[确认]`

战斗 tick 末 `fn_14000601e`（RTC `posTergetFleet`）：

- 先取一个目标点（锁定船或对方旗舰）
- 扫 **0..3 全部槽**（含玩家）
- 每船相对目标 `fn_140008530`
- 返回 `== 3`：按 kind 0/1/2 分支（`fn_1400080df` / `fn_1400041f1` / `fn_14000ec6e`），可结束本次扫描
- 否则若某查询为真：对非旗舰发 mesfile 指针，并 `fn_14000118b(ship, 4)` 改船态
- 跳过 `eFleet == 0x3E`（62）

接触用的「3」和远距释放用的「3」是同一个分带函数。同一数字在追击里表示「够着了」、在刷槽里表示「太远了」，不要合成一个叫 `spot_range` 的像素半径。

### 2.10 开战移交 `[确认]`

`fn_14000cd8d`（`0x140192400`）：槽 1..3，空槽（`0x46`）跳过，定位重叠且 `fn_1400040c0` 为假则组 `sChangeBattle` / `sSceneDesc` / `sEventTrigger`。

`fn_14000d599` 取势力。`== 0x1A`（26）走「海盗」文案枝：26 在势力系统里是空势力哨兵 `[确认]` `01-forces.md`，本处当「无所属 / 海盗」`[推断]`。状态 0 或 8–9 才继续。

`fn_140012dc8` 关系：1 尝试 `fn_14000f79f`；2 用 `DAT_14055e950`；0 加额外条件用 `DAT_14055e978`。随后 `fn_14000974b` 把双方绑进战斗场景。

文案（`mesfile_sc.txt`）：

- `0x5aff` 起：「在%s发现%s的舰队！进入战斗状态！」；「继续警戒吗？」「继续放哨吗？」
- `0x5c47` / `0x5d0f`：对方进攻
- `0x133cf`：「发现了%s的舰队！攻击吧！」（玩家主动）
- 海盗句在遭遇文档已列，本系统不拥有字符串

警戒 / 放哨是 ocean 遭遇模式，成功后 `Combat.start`。本系统不实现视野圆。

### 2.11 月结不是大地图 AI `[确认]`

`CFleetElement::vfunc_04`（`0x1400ff2c0`，约 0x19F1 字节）RTC 含 `eTargetFleet`、`asCompanion`、`sCalendar`：

- 扫 69 支，跳过 `eFleet == 0x3E`（62）
- 两两 `fn_1400ec540` 关系，`== 3` 跳过；否则 `fn_14000d7ab` 无画面近战（公式在 combat）
- 其后同伴、补给、金币、地方舰队 100000、换旗舰

`CForceElement::vfunc_04` 扫 25 势力战果槽，再扫 69 支给势力买船（`aeShip` / `aeShipType`），跳过势力哨兵 26 与状态 6。

这两处都 **不写** `+0x3c8` 轨迹，也不占 4 槽。禁止把月结买货 / 互打塞进 `npc_ai.yaml`。

### 2.12 情报文案（只作状态旁证）

`mesfile_sc.txt`：

- `0x8743`「%s的舰队正停泊在%s」
- `0x8b67` / `0x8b93`「正驶向%s」
- `0x9f33`「身份不明的舰队停泊 / 驶向」
- `0x14d33`「敌人商船舰队出现」

这些是广场 / 委托台词，证明有停泊与驶向两态。播放权在 town-life。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key | 拥有者 |
| --- | --- | --- | --- | --- |
| 世界舰队 | `eFleet` 69 | `INVALID_FLEET=-1` | `fleet_id` | fleets |
| 活动槽 | `eMobileFleet` 4 | `INVALID_MOBILEFLEET=-1`；槽内空 `fleet_id=70` | `mobile_fleet_slot` | ocean 运行时 |
| 活动船 | `eMobileShip` 20 | `INVALID_MOBILESHIP=-1`；`+0x18=0xD0` | `mobile_ship_id` | ocean 运行时 |
| 槽种类 | `eMobileFleetKind` | `INVALID_MOBILEFLEETKIND=-1` | `mobile_fleet_kind_id` | 本表 |
| 目标种类 | `eFleetAimKind` | `INVALID_FLEETAIMKIND=-1` | `fleet_aim_kind_id` | 本表 |
| 舰队状态 | `eFleetState` | `INVALID_FLEETSTATE=-1` | `fleet_state_id` | fleets；本系统只读 `==6` 不刷 |
| 船状态 | `eMobileShipState` | `INVALID_MOBILESHIPSTATE=-1` | `mobile_ship_state_id` | `[未知]` 计数 |
| 分带 | 0–3 | | `dist_band` | 本系统 |
| 轨迹点 | 65 / 槽 | | `locus_index` | 本系统 |

运行时以表长为准。原版包：槽 4、船 20、世界舰队 69、空槽 70、空港 98。

禁止把 `mobile_fleet_slot` 当成持久 `fleet_id` 存档主键。存档存 69 支世界舰队 + 4 条 4 字节槽 + 20 艘 `CMobileShip`。

## 4. 算法 / 公式

魔数进 `config/formulas/ocean.yaml` 的 `npc_ai` 段与 `config/ocean/npc_ai.yaml`。

### 4.1 分带

```
band(dx, dy):
  if abs(dx) < band_0 and abs(dy) < band_0:           return 0   # 96
  if abs(dx) < band_far_x and abs(dy) < band_1_y:     return 1   # 192, 128
  if abs(dx) < band_far_x and abs(dy) < band_far_y:   return 2   # 192, 192
  return 3
```

编号与原版 return 的对应标 `[推断]`，改阈值只改 YAML。

### 4.2 每 tick（非战斗）

```
# COceanScene::vfunc_02, fn_140006c12 != 1
for slot in 1 .. mobile_slots-1:
  rec = mobile[slot]
  if rec.fleet_id == empty_mobile_fleet: continue
  if band(player, rec) == despawn_band:          # 3
      Ocean.release_slot(slot)                   # fleet_id=70, kind=5, 清 5 船

for fleet in world_fleets:
  if fleet == player or not active: continue
  if fleet.state_id == skip_spawn_state: continue  # 6
  if band(player, fleet) in spawn_bands:
      Ocean.occupy_slot(fleet_id, kind=spawn_kind) # 原版 kind=5

for slot in 1 .. mobile_slots-1:
  follow_flagship_and_push_locus(slot)           # fn_14000d855

for slot in 0 .. mobile_slots-1:
  ships[1..4] follow flagship                    # fn_1400122bf

maybe emit FleetSpotted                          # fn_14000cd8d；细节见 02
```

`spawn_kind` 原版这条路径写 5。关系派种 1/2/3 走 `fn_140012a99`，调用点未挂名，配置里两套都留，默认包只启用「近距 kind=5」。

### 4.3 每 tick（战斗）

```
for slot in 1 .. mobile_slots-1:
  if empty or no flagship: continue
  target = pick_target(slot.byte3)               # 0/1/else
  aim    = table.aim[slot.byte2] or aim_fallback
  heading256 = aim.steer(self, target)           # 见 2.8
  apply_wind_current(heading256)                 # 复用 sailing 公式，不复制
  push_locus(slot)
  write_ship_pos

for slot in 0 .. mobile_slots-1:
  ships[1..4] follow flagship

Ocean.contact_scan(target_pos)                   # fn_14000601e
```

### 4.4 转向（原版包）

```
rel = angle256(target) - heading256
lead = aim.lead_angle          # 0x40 或 0x80
if dist2 >= range_scale^2:
    heading256 += (rel > 128) ? +lead : -lead
else if aim.reverse_when_close:
    heading256 += 128
```

`range_scale` 来自对方 `vfunc+0x40` 再 `fn_140002d2e`，闭合式 `[未知]`，参数名 `aim_range_scale`。

### 4.5 占槽 / 离港 / 入港

```
occupy(slot, fleet_id, kind):
  if Fleets.town(fleet_id) != empty_town and kind != leftover_kind:
      Fleets.leave_town(fleet_id)                # 清 +0x80，拷 +0x70
  if slot occupied by other: release(slot)
  slot.fleet_id = fleet_id
  slot.kind = kind
  bind up to 5 ships into mobile_ship pool
  init locus[65] from current locator

release(slot):
  unbind 5 ships                               # +0x18 = 0xD0
  slot.fleet_id = empty_mobile_fleet           # 70
  slot.kind = leftover_kind                    # 5
  # 世界舰队的 town_id 由 Fleets 在「入港」命令里写；本系统只丢槽
```

玩家入港是 `fn_14000b79e`（扫 97 港，半径 0x20 / 0x30），属 `01-sailing.md` 的 `ArrivedAtTown`。NPC 回港没有单独还原的半径函数；远距释放不等于写驻港。配置里把「远距丢槽」和「写回 town_id」分成两步，后一步默认包可关，直到 `fn_14000cb71` 之后的写港被钉死。

### 4.6 海域图名单

```
visible_on_sea_area(fleet, player, sea_area_id):
  if not active: false
  if fleet.byte10 == sea_area_skip_person_byte: false   # 0xC5
  in_same_town = player.town != empty_town and fleet.town == player.town
  at_sea_flag  = has_dest(fleet) or extra_query(fleet)
  if fleet.state == skip_spawn_state and not in_same_town and not at_sea_flag:
      false
  return sea_area_match and (in_same_town or at_sea_flag or already_known)
```

名单按 `fleet_id` 出图标，不创建 `mobile_ship`。点击图标若要切大地图，由 UI 发 `Ocean.focus(fleet_id)`，本系统再决定是否占槽。

## 5. 耦合分析

原版硬编码：

- `COceanScene` 直接调占槽、转向、开战组包
- 4 槽写在 `CFleetElement` 里，20 船写在 `CMobileShipElement` 里，100 炮弹同对象
- 空槽 70、空港 98、状态 6、势力哨兵 26、舰队 62 特殊字节 `0xC5` 全是立即数
- 海域图 new `CSeaAreaFleetView`，大地图用 `CDrawShipManager` 20 槽
- 月结互打与大地图 AI 共享 `eFleet`，但函数完全分开

重构切断：

- Ocean 只持有 `mobile_fleet_slot -> fleet_id` 映射和轨迹
- 世界舰队坐标 / 驻港 / 目的地由 Fleets 提供快照，Ocean 不写 `CFleet*`
- 关系 `Factions.relation(force_a, force_b)`，Ocean 不读友好度表
- 开战只 `FleetSpotted` / `NpcBattleIntended`；Combat 订阅
- 自动贸易只读 `Hold` / 账本，永不读 4 槽
- 战斗 AI 只在 `BattleTick` 里 `AI.plan`，不改 heading256
- UI 字符串全部 `ui/strings`

禁止：Ocean 改金币、改货舱、改六维、new `CCombatScreen`。

## 6. 解耦后的配置

### `config/ocean/npc_ai.yaml`

```yaml
schema: dk4.ocean.npc_ai
version: 1
player_mobile_slot: 0          # [推断]
npc_mobile_slots: [1, 2, 3]
empty_mobile_fleet_id: 70      # 0x46
empty_town_id: 98              # 0x62，与 sailing.empty_town_sentinel 同一数字
skip_spawn_state: 6            # CFleet+0x72
leftover_kind: 5               # 释放槽时写入
spawn_kind: 5                  # fn_1400100b9 路径
skip_world_fleet_ids: [62]     # 0x3E；月结互打也跳过
sea_area_skip_person_byte: 0xC5
pirate_force_id: 26            # 空势力哨兵当海盗枝 [推断]
locus_points: 65
locus_shift: 64
mobile_ships: 20
mobile_fleets: 4

mobile_fleet_kinds:
  - { id: 0, key: k0 }
  - { id: 1, key: k1 }
  - { id: 2, key: k2 }
  - { id: 3, key: k3 }
  - { id: 5, key: leftover }
  # NUM_MOBILEFLEETKIND 未知：有对照后按 DLL 补行，禁止把 5 写成「已还原的种类数」

aim_kinds:
  - id: 0
    key: intercept_lead
    lead_angle: 64             # 0x40
    reverse_when_close: false
  - id: 1
    key: direct
    lead_angle: 0
    reverse_when_close: false
  - id: 2
    key: lead_or_reverse
    lead_angle: 64
    reverse_when_close: true
    reverse_angle: 128         # 0x80
  - id: 3
    key: reverse_a
    lead_angle: 128
    extra: 3                   # fn_140008de1 第三参
  - id: 4
    key: reverse_b             # switch 落 default
    lead_angle: 128
    extra: 4

target_picks:
  - { id: 0, key: pick0 }
  - { id: 1, key: nearest_enemy_ship }
  - { id: 2, key: pick_other }

relation_kind_map:             # fn_140012a99；默认包可不启用
  no_kind2_occupied: 2
  relation_eq_2: 3
  otherwise: 1

write_town_on_despawn: false   # 原版是否写回驻港 [未知]
```

种类 / aim 的 `key` 是占位。有 World DLL 枚举后再改 YAML，不改 C++。

### `config/formulas/ocean.yaml`（追加）

```yaml
npc_ai:
  band_0: 96                   # 0x60
  band_1_y: 128                # 0x80
  band_far_x: 192              # 0xC0
  band_far_y: 192
  spawn_bands: [0, 1, 2]       # 与 fn_1400100b9 第二参的精确组合 [推断]
  despawn_band: 3
  contact_band: 3              # 追击里「够着了」；与 despawn 同函数不同语义
  lead_angle_90: 64
  reverse_angle: 128
  town_hit_small: 32           # 0x20，玩家入港枝；NPC 未证实复用
  town_hit_large: 48           # 0x30
  aim_range_scale: null        # [未知]
```

分带阈值只出现在这里。`01-sailing.md` 的航速段不引用它们。

### `config/caps.yaml`（本系统只读）

```yaml
mobile_fleets: 4
mobile_ships: 20
world_fleets: 69
empty_mobile_fleet: 70
```

World0 / World1 共用同一套 npc_ai。内容包只换 `fleets/templates.yaml`。

## 7. 独立旋钮

改这些不应迫使贸易、海战 AI、人物改代码：

- 槽数、活动船上限（引擎按表长；原版包 4 / 20）
- 空槽哨兵、跳过状态 6、舰队 62 例外
- 分带三个阈值、spawn / despawn / contact 各用哪一档
- aim 表行数、偏置角、近距是否 180°
- kind 表行数、关系→kind 映射
- 远距丢槽要不要写回 `town_id`
- 海域图是否显示状态 6、是否跳过 `0xC5`
- 战斗中 NPC 是否复用玩家航速公式（关了只转朝向、不乘风）

加一种 aim = YAML 加一行。把 4 槽改成 8 只改 caps 与存档宽度，名单仍按 `fleet_id` 画海域图。

## 8. 对外接口

查询：

```
Ocean.mobile_slots() -> [{ slot, fleet_id, kind_id, aim_id, x, y, heading256 }]
Ocean.mobile_ships(slot) -> [mobile_ship_id]      # LIVE；槽已空则 []
Ocean.band(fleet_id_a, fleet_id_b) -> 0..3
Ocean.sea_area_list(sea_area_id, player_fleet_id) -> [fleet_id]
Fleets.location(fleet_id) -> { town_id | x, y, state_id, dest_town_id }
Fleets.town(fleet_id) -> town_id                  # 98 = 不在港
Factions.relation(force_a, force_b) -> int        # 原版 0/1/2/3
Factions.force_of(fleet_id) -> force_id           # 26 = 海盗枝
```

坐标只传整数，不传 `CLocator*` / `World*`。槽不存在或 `fleet_id==70`：查询返回空，命令 no-op。

命令：

```
Ocean.occupy { slot?, fleet_id, kind_id }         # slot 省略则按 1..3 分配
Ocean.release { slot }
Ocean.set_aim { slot, aim_id }
Ocean.set_kind { slot, kind_id }
Ocean.npc_tick { in_battle: bool }                # 由场景时钟调用
```

Fleets 命令（Ocean 不直接改世界表，由总线转）：

```
Fleets.leave_town { fleet_id }
Fleets.enter_town { fleet_id, town_id }           # 仅当 write_town_on_despawn
```

事件：

```
NpcFleetSpawned    { fleet_id, slot, kind_id, x, y }
NpcFleetDespawned  { fleet_id, slot, reason: far|replace|battle_bind }
NpcLeftTown        { fleet_id, town_id }
NpcChaseIntended   { fleet_id, target_fleet_id, band }
FleetSpotted       { fleet_id, other_fleet_id, sea_area_id, aggressive }
  # 与 02 同一事件；本系统是生产者之一
```

`FleetSpotted.aggressive` 对应「进入战斗状态」vs「向我们进攻了」。Combat 订阅后 `Combat.start`。Ocean 在战斗中仍可 `npc_tick(in_battle=true)` 只转向，不发第二份 `FleetSpotted`。

UI 投影：

- `CDrawShipManager` ← `Ocean.mobile_ships`（最多 20）
- `CSeaAreaFleetView` ← `Ocean.sea_area_list`（按 `fleet_id`，不持实体）
- 文案 key：`ui.ocean.fleet_spotted` / `ui.ocean.fleet_attacks` / `ui.ocean.pirate_spotted`

## 9. 未核实字段

| 项 | 依据 | 不要写成 |
| --- | --- | --- |
| `NUM_FLEETAIMKIND` | 仅断言 | 5 种战术、T 字 |
| `NUM_MOBILEFLEETKIND` | 仅断言；代码见 0–3 与 5 | 一共 6 种已还原 |
| 槽 `+2` = aim、`+3` = 选目标 | 读取点与 switch 宽度 | 字段名已在头文件 |
| aim 0–3 的玩法名 | 只有偏置角 | 「追击 / 巡逻 / 逃跑」已是原版枚举 |
| kind 0–3 的玩法名 | 关系派种 + 接触分支 | 「商船 / 海军 / 海盗」 |
| 分带 return 0/1/2/3 与三档 if 的一一对应 | 反编译丢 return | 像素半径 96 就是遭遇半径 |
| `fn_140012a99` 的调用点 | 无其它 cpp call | 每 tick 一定跑关系派种 |
| 非战斗是否朝 `dest_town_id` 挪世界坐标 | `fn_14000d855` 不读 aim | NPC 在大地图上按航线巡航已还原 |
| 远距 `release` 是否写回驻港 | `fn_14000cb71` 只清船 | 出分带 3 一定入最近港 |
| `CFleet+0x72` 除 6 以外 | 海盗枝见 0、≤7、≤9 | 6 = 「已溃逃」 |
| `CFleet+0x80` 三字节 | `fn_14000c144` 优先于 `+0x70` | 一定是 `dest_town_id` |
| `CFleet+0x10 == 0xC5` | 仅舰队 62 模板 | 所有地方舰队 |
| `0x46` | 空槽 `fleet_id` | `eFleetState` 已溃逃 |
| `CMobileShip+0x8..+0x1B` 除空槽外 | 成对存档 | 帆角、耐久 |
| `CMobileShipElement+0x1038` 21×`0x18` | 只见构造 | 第 21 艘船 |
| `fn_14000c44b` 与玩家航速是否同一套除数 | 同风 / 流 RTC | NPC 用另一张速度表 |
| `fn_140008de1` 第三参 3 / 4 | aim 3 / default | 接舷确认 |
| 接触 `band==3` 与释放 `band==3` | 同一函数 | 同一语义 |
| `fn_14000601e` 的 mesfile 指针 | 只见 DAT_ | 具体哪一句「发现%s的舰队」 |
| World1 槽规则 | 未对照 | 一定仍是 4×20 |

职文和广场情报不是算法名。还原 `eFleetAimKind` 对着 World DLL 断言和槽 `+2` 的 switch，不要按光荣其它作的「巡逻 / 护卫 / 贸易」表来填。
