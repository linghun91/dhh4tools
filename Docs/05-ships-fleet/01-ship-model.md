# 船体模型

船的**定义**（船体目录）和**实例**（某艘正在用的船）分开。本篇只管船体：类型、等级、耐久、水手上下限、货格。舱室岗位见 `03-cabins-crew-posts.md`，改装 / 炮 / 补给见 `04-remodel-guns-supplies.md`，舰队编成见 `02-fleet-proxy.md`。

## 1. 职责边界

本系统管：

- 船体目录 `eShip` 与船种 `eShipType`
- 船体等级 / 体型档（原版 `eShipGrade`，计数未还原）
- 单船实例的耐久、水手、名称、货格
- 货格所有权：货格属于船；贸易会话只是借来读写

本系统不管：

- 人物属性、招募、疲劳（`03-characters`）
- 舱室种类与 `person_id` 绑定（本目录 `03`）
- 帆、船首像、火炮、补给消耗（本目录 `04`）
- 舰队编成、委任、阵型、地方舰队（本目录 `02`）
- 市价与买卖会话（`04-economy`）
- 海战伤害结算（`07-combat`）

## 2. 原版事实

### 目录与池

- `[确认]` `eShip` 0–99，`NUM_SHIP=100`，`INVALID_SHIP=-1`。来源：`srcdata/source/ShipID.h`、`catalog/num_values.json`。
- `[确认]` `eShipType` 0–4，`NUM_SHIPTYPE=5`，`INVALID_SHIPTYPE=-1`。来源：`ShipTypeID.h`。
- `[确认]` 船体**中文名不在** `mesfile_sc.txt`，也不在 World DLL。`CShip::vfunc_05` 用 `lstrcpyA` 从船体定义指针拷名称。仓库未抽出这 100 个名字。文档里名称写 `[未知]`，只保留 id。
- `[确认]` 运行时实例池是 `CShipElement`：构造函数 `fn_1400017df(..., 0x80, 0xcf)`，**207** 个 `CShip`，每个 **0x80** 字节。析构把 `+0x6788` 起 8 个字节写成 `0xD0`。来源：`game/ship/CShipElement.cpp`。
- `[确认]` 船种对象池是 `CShipTypeElement`：`fn_1400017df(..., 0x50, 0x27)`，**39** 个 `CShipType`，每个 **0x50** 字节。与 `NUM_SHIPTYPE=5` 的关系未还原。来源：`game/ship/CShipTypeElement.cpp`。
- `[确认]` 空船哨兵 `0xD0=208`：`CFleetShip` 构造写 `+0x8=0xD0`，`CBattleShip` 同，阵型 / 委任 UI 把未放的船位填 `0xD0`。208 刚好在 207 槽之外。
- `[确认]` `CShip::vfunc_05`：若船体 id `>= 0x64`（`NUM_SHIP`），把空标记写成 `0x28` 后返回，不当成有效船体初始化。

### 玩家可见上限

- `[确认]` 「一只舰队最多可以有五艘船，新船放在船坞里。」来源：`mesfile_sc.txt` `0x7df7`。
- `[确认]` 「持有的船过多了！这个城市没有可以制造的船！」「请给船取个名字！不能再把船处置掉了！还有剩余货物，这样可以吗？」来源：`mesfile_sc.txt` `0x2e4f` / `0x2e93`。
- `[确认]` 玩家舰队船位 `NUM_FLEETSHIP=5`。来源：`FleetShipID.h`、`Constants.h`。
- `[确认]` 每船交易货格 `NUM_SHIPLOAD=5`，交易所行 `NUM_SHIPLOAD_EXCH=5`。来源：`ShipLoadID.h`、`Constants.h`、`include/trade.h`。
- `[确认]` `CShip::CountHolds` 实参 **10**（`NUM_HOLD_SCAN` / `HOLD_SLOTS`）。来源：`Ship.h` 注释、`Constants.h`、`game/trade/Trade.cpp`。
- `[确认]` 货格空商品 `ITEM_EMPTY=0x6A=106`。来源：`Constants.h`、`trade.h`。禁止用 0 当空格（0 是南瓜种子）。

### 耐久与水手

- `[确认]` `SHIP_DURABILITY_MIN/MAX`、`SHIP_CREW_MIN/MAX` 都是 `USHORT`，断言 `<= _GetDataSizeMax_BYTE2DWORD(sizeof(USHORT))`，即硬顶 65535。来源：`ShipBase.h`、`catalog/asserts.txt`、`catalog/constants.json`。
- `[未知]` 这两个 MIN/MAX 的**十进制原版值**没有写进头文件。实现从断言 / RTC 再填，文档保持符号名。
- `[确认]` 改造文案把船分成「中型以上」「大型」：中型以上可加船首帆，大型可加船尾帆。来源：`mesfile_sc.txt` `0xc5d7` / `0xc663`。
- `[推断]` 改装计价把耐久与两档阈值比较：`4000`（`0xFA0`）、`7000`（`0x1B58`），得到 3 个体型档，用来索引 `aulAddPartsPrice`。依据：`fn_140013d68`（`0x1400a9d20`）RTC `aulAddPartsPrice`。体型档是否等于 `eShipGrade` 未证明。

### `CShip` 布局（相对 `CShip*`）

继承：`CShip → CShipObject → CShipBase → CSaveObject → CObject`。构造在 `this+8` 再挂一份 `CSaveObject` 虚表。析构尺寸 `0x80`。

存档 `vfunc_07` / 读档 `vfunc_06`（body `0x140230ac0` / `0x140231300`）以 `this+8` 为基址写字段；贸易手写还原按 `CShip*` 计偏移。两者差 8 字节，对上之后：

| 偏移（CShip*） | 宽 | 存档 | 含义 | 等级 |
| --- | --- | --- | --- | --- |
| `+0x00` | 8 | 否 | 虚表 | `[确认]` |
| `+0x08` | 8 | 否 | 内嵌 `CSaveObject` 虚表 | `[确认]` |
| `+0x10` | 19 B | 是 | GBK 船名（循环 `0..0x12`） | `[确认]` |
| `+0x23` | 3 B | 是 | 从船体定义 `+0x09` 拷 3 字节，与桅相关 `[推断]` | `[推断]` |
| `+0x26` | 31 B | 是 | `eShipCabin` 槽，每槽一个舱室种类 | `[确认]` |
| `+0x4A` | u16 | 是 | `vfunc_10/14` 读写；初始化来自船体定义。候选：当前耐久或当前水手 | `[未知]` |
| `+0x4C` | u16 | 是 | `vfunc_11/16` 读写 | `[未知]` |
| `+0x51` | u8 | 是 | 空船标记：有效初始化前为 `0x28`；`vfunc_01` 相等则视为空 | `[确认]` |
| `+0x53` | u8 | 是 | 初始化写成 `0x0D` | `[未知]` |
| `+0x54` | u8 | 是 | 初始化写成 `0x62=98` | `[未知]` |
| `+0x58` | 5×8 | 是 | 货格。每条：`+0` 商品、`+1/+2` 字节、`+4` dword 买入价。原版 `+3` 不存 | `[确认]` |

货格扫描 10、对象只有 5 条记录（`0x58+5×8=0x80`）。多出来的 5 次扫描语义未还原，可能是历史上限或战利品 UI（存在 `CCapturedLoadSprite`），**不要**当成「每船 10 个可配置货格」。

贸易开市 `UnpackFromShip` 从这 5 格生成交易槽并清空船货；确认后 `PackToShip` 再写回。来源：`srcdata/docs/trade_stack.md`、`game/trade/Trade.cpp`。

### `CShipType` 布局

尺寸 `0x50`。`vfunc_05`（`0x1402ae1f0`）从静态定义填充：

| 偏移 | 宽 | 含义 | 等级 |
| --- | --- | --- | --- |
| `+0x08` | ptr/拷 | 名称来源 | `[确认]` 有拷贝 |
| `+0x10` / `+0x12` / `+0x14` | 1 / 2 / 4 | 未命名 | `[未知]` |
| `+0x18` | 3 B | 与桅 / 帆相关 `[推断]` | `[推断]` |
| `+0x1C` | u16 | 未命名 | `[未知]` |
| `+0x1E` | 31 B | 默认舱室种类；若值 `== 0x17` 改写成 `0x18` | `[确认]` 循环；改写原因 `[未知]` |
| `+0x3D` / `+0x3E` / `+0x40` | 1 / 2 / 1 | 未命名 | `[未知]` |

### 类与设施

- `[确认]` 造船厂对象 `CShipyard` 只有构造 / 析构，尺寸 `0x10`，逻辑在未挂类的 `fn_*` 与甲板 View 里。来源：`game/town/CShipyard.cpp`。
- `[确认]` 选船 / 换船 / 甲板：`CSelectShipView`、`CShipChangeView`、`CDeckShipView`、`CShipArrangeView`。来源：`class_hierarchy.txt`。

## 3. 实体与 ID

| 实体 | 原版枚举 | 原版计数 | 空 / 非法 | 重构 key |
| --- | --- | --- | --- | --- |
| 船体定义 | `eShip` | 100 | `INVALID_SHIP=-1` | `ship_id` |
| 船种 | `eShipType` | 5 | `INVALID_SHIPTYPE=-1` | `ship_type_id` |
| 船体等级 | `eShipGrade` | `[未知]` | `INVALID_SHIPGRADE=-1` | `ship_grade_id` |
| 「普通船种」 | `eNORMALSHIPTYPE` | `[未知]` | `INVALID_NORMALSHIPTYPE=-1` | 暂不单列 |
| 船实例槽 | （无独立枚举） | 207 | `0xD0` | `ship_instance_id` |
| 玩家船位 | `eFleetShip` | 5 | `INVALID_FLEETSHIP=-1` | `fleet_ship_slot` |
| 货格 | `eShipLoad` | 5 | `INVALID_SHIPLOAD=-1` | `hold_slot` |
| 空货格商品 | — | — | `0x6A` | `trading_item.empty` |

重构约定：

- `ship_id` 只指目录 0–99，永远是定义。
- `ship_instance_id` 指 207 槽里的活船。系统之间传这两个 ID，不传 `CShip*`。
- 模组可以加船体；运行时按表长循环，禁止写死 `100` / `207`。
- 原版 `0xD0` 映射为 `ship_instance.empty`，不要复用 `-1`（与 `INVALID_SHIP` 撞车）。

## 4. 算法 / 公式

全部参数进 `config/caps.yaml` / `config/formulas/ships.yaml`，代码里不准出现裸 `5`、`10`、`100`、`4000`、`7000`。

```
# 货格
hold_count(ship) = min(occupied_holds, caps.hold_scan)
trade_slots      = caps.hold_slots_per_ship          # 原版 5
hold_scan        = caps.hold_scan                    # 原版 10

# 空船
is_empty_instance(id) = (id == sentinels.ship_instance_empty)   # 0xD0
is_empty_hull(ship_id) = (ship_id >= len(hulls)) or (ship.flag_empty == hull_empty_marker)  # 0x28

# 体型档（改装计价用，原版立即数）
# [推断] 来自 fn_140013d68
size_class(durability):
  if durability >= formulas.size_large_min:      # 7000
      return size.large                          # 原版分支写成 1
  if durability <  formulas.size_small_max:      # 4000
      return size.small                          # 原版分支写成 2
  return size.medium                             # 0

# 水手 / 耐久夹取
crew        = clamp(crew,        caps.ship_crew_min,        caps.ship_crew_max)
durability  = clamp(durability,  caps.ship_durability_min,  caps.ship_durability_max)
```

买船准备天数、修理 / 改造天数见 `04-remodel-guns-supplies.md`。本篇只提供 `size_class(durability)` 给改装计价查询。

`SHIP_CREW_*` / `SHIP_DURABILITY_*` 的原版十进制未进仓库，`caps.yaml` 先留符号，抽出后再填 default。

## 5. 耦合分析

原版硬编码：

- 贸易、委托、自动买卖都假设「5 船 × 5 格、商品不堆叠、空格 `0x6A`」。`CTrade` 直接扫 `CShip+0x58`。
- 非玩家势力不走真实货格，走 `CFleet+0x74` 一条「商品 + 数量」（`trade_stack.md`）。船体系统被舰队系统的紧凑货记绕开。
- `CShip::vfunc_05` 初始化 31 个舱室槽，并把 `0x17` 改成 `0x18`——船体定义夹带舱室布局。
- 耐久立即数 `4000` / `7000` 写在改装计价函数里，不是船种表字段。
- 船名存在实例上（可在造船厂改名），定义名在 exe 静态表，World DLL 不管船。
- 海战用 `CBattleShip`（尺寸 `0x20`，空船同样 `0xD0`），与 `CShip` 不是同一对象。

重构后允许的依赖：`ships → characters.person_id`（本篇不写人物）；`economy` **查询** `Ships.holds(instance_id)`，不拥有货格。

## 6. 解耦后的配置

```yaml
# config/caps.yaml（只摘本系统）
fleet_ships: 5
hold_slots_per_ship: 5
hold_scan: 10
ship_hulls: 100          # 原版内容包实例数；运行时读表长
ship_instance_pool: 207
ship_types: 5
ship_crew_min: SHIP_CREW_MIN          # 抽出后替换
ship_crew_max: SHIP_CREW_MAX
ship_durability_min: SHIP_DURABILITY_MIN
ship_durability_max: SHIP_DURABILITY_MAX
```

```yaml
# config/ids/ 生成
sentinels:
  ship_invalid: -1
  ship_instance_empty: 0xD0
  hull_uninitialized: 0x28
  trading_item_empty: 0x6A
```

```yaml
# config/ships/types.yaml
schema: dk4.ships.types
version: 1
types:
  - id: 0
    key: ship_type_0
    name: "[未知]"
  - id: 1
    key: ship_type_1
    name: "[未知]"
  - id: 2
    key: ship_type_2
    name: "[未知]"
  - id: 3
    key: ship_type_3
    name: "[未知]"
  - id: 4
    key: ship_type_4
    name: "[未知]"
```

```yaml
# config/ships/hulls.yaml
schema: dk4.ships.hulls
version: 1
# 100 行由 exe 船体表生成。名字未抽出则 name 为 null。
- id: 0
  key: hull_000
  name: null
  ship_type_id: null          # 定义表字段未命名前保持 null
  grade_id: null
  durability_max: null
  crew_min: null
  crew_max: null
  default_cabins: []          # 31 槽，见 03
  default_masts: []           # 3 字节，见 04
  unknown_bytes: []           # 定义表残余
```

```yaml
# config/formulas/ships.yaml
schema: dk4.formulas.ships
version: 1
size_small_max: 4000
size_large_min: 7000
cabin_kind_alias:
  from: 0x17
  to: 0x18
```

World0 / World1 **没有** 船体目录；两份内容包共用同一份 `hulls.yaml`，除非以后从 exe 拆出两套表。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗改代码（它们只读 caps 与查询接口）：

- 船体行数、船种行数
- `hold_slots_per_ship`、`hold_scan`
- `fleet_ships`、`ship_instance_pool`
- 水手 / 耐久上下限（须 ≤ `engine_limits` 的 u16 硬顶）
- 体型档阈值 `size_small_max` / `size_large_min`
- 船名长度（原版 19 字节 GBK；显示层自己裁）

显示层不得用「价格列放得下几位」倒逼货格规则。

## 8. 对外接口

事件：

- `ShipCreated { ship_instance_id, ship_id }`
- `ShipScrapped { ship_instance_id }`
- `ShipRenamed { ship_instance_id }`
- `HoldsChanged { ship_instance_id }`
- `DurabilityChanged { ship_instance_id, durability }`
- `CrewChanged { ship_instance_id, crew }`

查询（只返回 ID 与标量）：

- `Ships.hull(ship_id) -> HullDef`
- `Ships.instance(ship_instance_id) -> { ship_id, name, durability, crew, empty }`
- `Ships.holds(ship_instance_id) -> [{ slot, trading_item_id, unit_price }]`
- `Ships.sizeClass(ship_instance_id) -> size_id`
- `Ships.countHolds(ship_instance_id) -> int`  （≤ `hold_scan`）

命令：

- `Ships.create(ship_id) -> ship_instance_id`
- `Ships.setHold(instance_id, slot, trading_item_id, unit_price)`
- `Ships.clearHold(instance_id, slot)`  （写成 `trading_item.empty`）
- `Ships.rename(instance_id, name)`

`CTrade` 开市 / 关市改为：借 `holds()` 快照，提交时发 `Ships.setHold`。禁止贸易模块持有 `CShip*`。

## 9. 未核实字段

必须单列，禁止猜成确定语义。

### 船体定义表（exe 静态，未抽出）

整表未进 `Docs/_data/`。已知运行时从定义拷了：名称指针、3 字节（`+0x09`）、31 个舱室种类（`+0x0A/+0x0F` 一带）、若干 u16。其余字节全部 `[未知]`。

### `CShip` 未命名字节

相对 `CShip*`：

- `+0x23..+0x25` 3 字节（与桅一起初始化）
- `+0x45`、`+0x46..+0x49`、`+0x4A` u16、`+0x4C` u16、`+0x4E` u16
- `+0x50`、`+0x52`、`+0x53=0x0D`、`+0x54=0x62`
- 货格记录 `+1/+2`（原版拷贝函数 `110b8` 会拷，语义未命名）

`+0x4A` / `+0x4C` 极可能是当前耐久与当前水手（都是 `USHORT`），但没有字段名，保持 `[未知]`。

### `CShipType` 未命名

`+0x10`、`+0x12`、`+0x14`、`+0x1C`、`+0x3D`、`+0x3E`、`+0x40`。

### 计数未还原

- `NUM_SHIPGRADE`
- `NUM_NORMALSHIPTYPE`
- 39 个 `CShipType` 槽与 5 个 `eShipType` 的对应
- `CountHolds(10)` 多扫的 5 格
- `CShipElement+0x6788` 的 8 个 `0xD0`

### 名称

100 艘船体、5 个船种的玩家可见名称：仓库里没有。禁止用「轻快帆船 / 盖伦」等常识名填进原版包。抽出 exe 字符串后再补 `name` / `key`。
