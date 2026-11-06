# 炮种、炮弹与战斗炮位目录

本篇补齐造船厂 **6 种大炮** 的名称与表内数值、战斗里 **5 个 `eBattleGun` 槽** 的含义、以及 **100 个 `eShell`** 到底是不是 100 种弹药。改装流程、天数、装甲仍见 [04-remodel-guns-supplies.md](04-remodel-guns-supplies.md)；射程、命中、沉船见 [07-combat/01-naval-battle.md](../07-combat/01-naval-battle.md)。攻城把军事打穿会让本表解锁集合变小，见 [07-combat/04-ship-vs-port.md](../07-combat/04-ship-vs-port.md)。

04 把六炮中文名写成「不在 mesfile、保持未知」。名称在 **exe `.data` 的 GBK 定长记录**里，不在 `mesfile_sc.txt`。本篇按记录抽出；缺的名字写 `null`，禁止用「卡隆 / 臼炮」去填。

## 1. 职责边界

本系统管：

- 炮种定义表 `eGunType`（6）：稳定 ID、key、四语言名、威力、价基、长射程标记、军事解锁门槛
- 战斗炮位 `eBattleGun`（5）与场景炮对象池（原版 21）的对应关系
- 飞行炮弹 **实例池** `eShell`（100）的布局与空闲判定；**不是** 100 种弹药目录
- 图标包 `Game/Gun.dk4`（G1T，6 帧，下标 = `gun_type_id`）
- 换炮差价用到的价基与 `3^n` 乘数（公式参数进 `formulas/ships.yaml`）

本系统不管：

- 船首炮 / 船尾炮 / 撞角（改装件 `eAddParts`，04）
- 炮手室缩短装填的 tick 数（战斗读舱室修正器）
- 炮弹打船体 / 水手 / 城镇的伤害闭式（07-combat；本表只提供 `power`）
- 船体货格、耐久、帆（01）
- 市价（经济）

一艘船上的「装备炮种」是 **一个** `gun_type_id`，不是 5 门不同种类。空炮运行时哨兵是 **7**（`NUM_GUNTYPE+1`），不是头文件的 `INVALID_GUNTYPE=-1`。

## 2. 原版事实

### 2.1 计数与对象

| 符号 | 值 | 对象 | 来源 |
| --- | --- | --- | --- |
| `eGunType` / `NUM_GUNTYPE` | 6 | 造船厂可选炮种 | `GunTypeID.h`（RTC `cmp < 6`） |
| `INVALID_GUNTYPE` | -1 | 非法枚举 | 同上 |
| 装备空炮 | **7** | `ship.vfunc+0x40 == 7` 则不画炮、威力 0 | `CSelectShipDrawLine.cpp`；`fn_14000c6cb`（`0x14023eb60`） |
| `eBattleGun` / `NUM_BATTLEGUN` | 5 | 战斗炮位 ID | `BattleGunID.h`、`num_values.json` |
| `INVALID_BATTLEGUN` | -1 | | `BattleGunID.h` |
| 场景 `CBattleGun` 池 | **21** | `CMobileShipElement+0x1038`，步长 `0x18` | 构造 `fn_140007dce(..., 0x18, 0x15)` |
| `eShell` / `NUM_SHELL` | 100 | 飞行炮弹实例 | `ShellID.h`；循环 `>= 0x64` |
| `INVALID_SHELL` | -1 | | `ShellID.h` |
| 场景 `CShell` 池 | 100 | `CMobileShipElement+0x1230`，步长 `0x30` | 构造 `fn_140007dce(..., 0x30, 0x64)` |
| `CMobileShip` 池 | 20 | 同对象 `+8`，步长 `0x30` | `NUM_MOBILESHIP=20` |

`Gun.h` 只有 `CBattleGun : CSaveObject`，没有炮种结构体。炮种表是 exe 静态数组，不是 World DLL。

RTC 对 `eBattleGun` 的上界：5 出现 3 次、21 一次、20 一次；对 `eShell`：100 四次、4 三次、65 两次、20 一次（`num_from_rtc.json`）。21 ≠ 5，不能写成「每船五门不同的炮」。

### 2.2 六炮名称在 exe，不在 mesfile

`fn_140012760`（body `0x140123300`）：

```
if (id < 0 || id >= 6) return NULL;
return 0x140682e10 + id * 0x13;   # 19 字节 / 条
```

SC 文件偏移 `0x681810`。每条：`name[17]` GBK 以 0 结尾，`+0x11` 一字节，`+0x12` 一字节。调用方把返回指针当 C 字符串画名称，所以 `+0x11/+0x12` 在名字的 NUL 之后，UI 看不见。

同一套尾字节 `01 0a 02 32 03 46 04 64 05 c8 05 fa` 在四语言 exe 各有一份（JP 文件 `0x683120`，TC `0x681a40`，KR `0x68edc0`），名称编码随语言，数值字节相同。

| id | key | SC（GBK） | TC（Big5） | JP（SJIS） | KR（CP949） | `+0x11` 威力 | `+0x12` 价基 | 军事解锁 |
| ---: | --- | --- | --- | --- | --- | ---: | ---: | ---: |
| 0 | `saker` | 霰弹 | 散彈 | セーカー | 세이커 | 1 | 10 | 1 |
| 1 | `culverin` | 曲射 | 曲射 | カルバリン | 캘버린 | 2 | 50 | 1 |
| 2 | `pedrero` | 加农曲射 | 加農曲射 | ペドロ | 페드로 | 3 | 70 | 2000 |
| 3 | `cannon` | 加农 | 加農 | カノン | 캐논 | 4 | 100 | 3500 |
| 4 | `heavy_cannon` | 重加农 | 重加農 | 重カノン | 중캐논 | 5 | 200 | 5000 |
| 5 | `carronade` | 连射 | 連射 | キャロネード | 캐노네이드 | 5 | 250 | 7000 |

key 取日文原名的稳定英文（セーカー=saker，カルバリン=culverin，ペドロ=pedrero，カノン=cannon，キャロネード=carronade）。SC「霰弹 / 连射」是本地化，不是另一种炮。

`[确认]` 这 6 个中文短名 **不在** `mesfile_sc.txt`。mesfile 造船段只有「可以更改船上装备的大炮种类」（`0xc6af`）。

### 2.3 表内两字节的用法

`+0x11`（威力）：

- `[确认]` `fn_140012adf`（`0x1400a7ed0`）读 `BYTE [record+0x11]`。
- `[确认]` 造船厂 `fn_140003bb1` 把该值格式化进 rdata 串 `威力%s`（`0x14053eeac`）。
- `[确认]` `fn_14000c6cb`（`0x14023eb60`）：`gun_type==7` 则 0，否则读 `+0x11`。战斗瞄准路径会乘这个值（`0x1401be77a` 一带）。
- `[确认]` 船 `vfunc+0x48==4` 时威力再 `+2`（`0x14023ebb0`）。态 4 的名字未恢复，不要写成「撞角」。

`+0x12`（价基）：

- `[确认]` `fn_1400144d9`（`0x14023f490`）读 `BYTE [record+0x12]`。
- `[确认]` `fn_140013593`（`0x14023ec30`）：

```
base  = gun_record[+0x12]          # 10,50,70,100,200,250
mult  = u16_table[hull_param]      # 0x140596d78 = 1,3,9,27,81
price = base * mult * 20
if half: price /= 2
```

`u16_table` 前五档是 `3^n`（n=0..4）。`hull_param` 来自 `fn_140006b95(ship, 8)`：扫 31 个 `eShipCabin`，计 `vfunc+0x38(slot)==8` 的间数。文案炮手室最多 4，与 5 档价表对齐，**种类 8 是否就是炮手**标 `[推断]`（`CabinKindID.h` 无逐项名）。

04 把 `fn_140013593` 写成货格差价。对照 `fn_140013d68`（`0x1400a9d20`）后半：`vfunc+0x40` 取的是炮种，第二参是舱室种类 8 的间数，算的是 **换炮 / 改炮手室间数的差价**。货格差价仍可能在别处。

长射程：

- `[确认]` `fn_140003bb1` 仅当 `eGunType==1` 或 `==5`（曲射、连射）再画 rdata 串 `长射程`（`0x14053eeb4`）。不是 `+0x12` 的函数，是硬编码 id 集合。

### 2.4 军事解锁，不是商业

`fn_14000bc03(town, track)` 读 `uint16` 于 `town+0x1E+track*2`。港口情报 UI（`fn_140005f51` / `0x140269810`）：

- 商业数字：`xor edx,edx` → track **0**
- 军事数字：`mov edx,1` → track **1**

选炮 UI `fn_140003bb1` 同样 `mov edx,1` 再比 `anNecessaryDevelopment`。门槛是栈上 6 个 int，不是 .data 表：

```
1, 1, 2000, 3500, 5000, 7000
```

`s70 < 表[id]` 则该炮不上架。单位与发展度 0–9999 同量级。来源：`fn_1400a1140.cpp` 的 `fn_140003bb1`；与 [04-ship-vs-port.md](../07-combat/04-ship-vs-port.md) §2.7 一致。04-remodel 里「商业 / 军事哪一条未知」以本篇为准。

### 2.5 造船厂 UI 与 `Gun.dk4`

- `[确认]` `CSelectGunTypeView`：选炮种 View。来源：`game/ship/CSelectGunTypeView.cpp`。
- `[确认]` 真正铺 6 个按钮、滤发展度、画「威力%s / 长射程」的是 `fn_140003bb1`（RTC `eGunType`、`anNecessaryDevelopment`、`rcGun`、`posGun`）。字符串 `InfoBox2.dk4`（框 0x42）、`InfoBox.dk4`。
- `[确认]` `fn_14001366a`（`0x140123340`）按 `eGunType` 调 `fn_140006faa(sprite, "Gun.dk4", gun_type, 0)`。`Game/Gun.dk4` 头 `GT1G0600`（G1T 一变体；`formats.md` 常见的是 `GT1G4600`），文件长 24772，`uint32` 贴图数在 `+0x10`，值为 **6**。帧下标 = `gun_type_id`。
- `[确认]` 船信息行格式 `\%s炮%s门`（`0x140593ca0`）：第一段炮种名，第二段门数。
- `[确认]` 门数 `fn_140004836`（`0x14023edf0`）：

```
n = fn_140006b95(ship, 8) * 24
n = clamp(n, 2, 96)                 # fn_1400103d4(2, n, 0x60)
if fn_1400091e2(ship, 7): n += 6    # 船尾炮改装
if fn_1400091e2(ship, 6): n += 6    # 船首炮改装
```

`fn_1400103d4` 是三元夹紧：`x<=2 → 2`，`x>=96 → 96`，否则 `x`。0 间仍至少 2 门；4 间封顶 96（=4×24）。船首 / 船尾各 +6，与 01-naval-battle 的弧判定同一对 `fn_1400091e2` 实参。

- `[确认]` 无炮（类型 7）时战斗 HUD 改画 `0x14057e9d0` 的短串 `" - "`，不调用名称表。

其它 rdata：`购买大炮`、`船首炮`、`船尾炮`、`武装炮台`、`大炮`、`炮手`。后四者不是 `eGunType`。

### 2.6 五个战斗炮位是什么

**不是** 一艘船上的 5 种炮，也不是 5 种弹药。

| 事实 | 等级 | 依据 |
| --- | --- | --- |
| 枚举 `NUM_BATTLEGUN=5`，与 `NUM_FLEETSHIP=5` 同数 | `[确认]` | `BattleGunID.h`、`FleetShipID.h` |
| 场景里实际构造 **21** 个 `CBattleGun`，每个 **0x18** 字节 | `[确认]` | `CMobileShipElement` 构造 / 析构循环 `>= 0x15` |
| 同对象上 20 艘 `CMobileShip` + 100 发 `CShell` | `[确认]` | 同上；`02-fleet-proxy.md` |
| 一艘船只有一个 `gun_type_id`（`vfunc+0x40`） | `[确认]` | 选炮、船信息、换炮差价都只读这一处 |
| 海战 tick 先扫 21 门炮再扫 100 发弹 | `[确认]` | `fn_14000ee8a`（`0x1401c3210`）RTC `eBattleGun` / `eShell` |
| 21 = 4 支 `NUM_MOBILEFLEET` × 5 船 + 1 门城炮 | `[推断]` | 池长算术；索引 20 是否固定城炮未在字段名钉死 |
| `eBattleGun` 0–4 = 一支舰队里按船位各一门 `CBattleGun` | `[推断]` | 枚举 5 与每舰队 5 船对齐；场景下标 0–20 是展开后的池下标，RTC 仍把计数器叫 `eBattleGun` |

`CBattleGun` 存档（`vfunc_06/07`）：

| 偏移 | 宽 | 含义 | 等级 |
| --- | --- | --- | --- |
| `+0x00` | 8 | 虚表 | `[确认]` |
| `+0x08` | u16 | 存档有；语义未还原 | `[未知]` |
| `+0x0A` | u16 | **非 0 才能走开火分支** | `[确认]` `fn_14000b389`（`0x14014ada0`）`WORD[this+0xA] != 0` |
| `+0x0C` | | `vfunc_05` 清 0，再 `fn_140008693` | `[未知]` 重置块 |
| `+0x10` | | 构造调 `fn_140010e42`，其它 RTC 把同一函数当 `cAngle` | `[推断]` 朝向 |

禁止把 `+0x8/+0xA` 写成「弹药仓」：没有名为 ammo 的 RTC；文案「装上炮弹」只证明有装填动作。`+0xA==0` 当本 tick 不能射。

### 2.7 100 发炮弹没有 100 个名字

exe `.rdata` / `.data` 里 **没有** 100 条炮弹 GBK 名。`NUM_SHELL=100` 是飞行实例池：

- `[确认]` `fn_14000bed8` 从 0 扫到 99，`fn_140001a9b==0` 的槽可分配。
- `[确认]` `fn_140001a9b`：`WORD[shell+0x26] > 0` 视为在飞。
- `[确认]` 生成 `fn_1400052e0`（`0x140149580`）往 `+0x0C/+0x10/+0x14/+0x1C/+0x20` 和三个 u16 写值；`idiv` 操作数在反编译里经常是脏寄存器，**禁止**补成完整弹道式。
- `[确认]` 存档 `vfunc_06/07` 序列化六个 dword（`+0x0C..+0x20`）+ 三个 u16（`+0x24/+0x26/+0x28`）。来源：`CShell.cpp`。
- `[确认]` `CShellInfo` 不是 `CSaveObject`；构造把 `+0x0C` 写成 `0xFFFFFFFF`。析构尺寸 `0x28`。不是 100 种弹药定义。
- RTC 另有 `eShell >= 4`（3 次）。与 100 并存，更像状态 / 图种上界，**没有**对应的 4 个名称。

所以 `config/ships/shells.yaml` 的 `name` 对 id 0–99 一律 `null`。不要把实例池当成弹药图鉴。

### 2.8 头文件与断言

- `Gun.h`：`CBattleGun`；注释提到 `NUM_GUNTYPE / NUM_BATTLEGUN / NUM_SHELL`。
- `GunTypeID.h` / `BattleGunID.h` / `ShellID.h`：只有计数与 `INVALID_*`。
- `asserts.txt`：`0 <= eData < NUM_GUNTYPE \|\| == INVALID_GUNTYPE`（炮种）；`BATTLEGUN` 同形。运行时空炮 7 不走这条断言（7 不是 `INVALID`，也不在 0–5）。

## 3. 实体与 ID

| 实体 | 原版 | 原版计数 | 空 | 重构 key | 运行时 |
| --- | --- | --- | --- | --- | --- |
| 炮种 | `eGunType` | 6（0–5） | 非法 `-1`；装备空 **7** | `gun_type_id` | 表长 |
| 战斗炮位 | `eBattleGun` | 5（0–4） | `-1` | `battle_gun_slot` | 表长；场景池另计 |
| 场景炮对象 | `CBattleGun` | 21 | 下标越界不取 | `scene_gun_id` | `caps.scene_gun_pool` |
| 飞行炮弹 | `eShell` / `CShell` | 100 | `-1`；`+0x26==0` 空闲 | `shell_instance_id` | `caps.shell_pool` |
| 炮弹定义 | （无） | 0 | — | — | 不要为原版包造 100 行弹药 |

模组加炮种：追加 `guns.yaml` 行，`id >= 1000` 或独立命名空间，不覆盖 0–5。图标包按表长扩展，UI 不要写死 6 格。

`gun_type_id` 7 只用于「这艘船没装炮」，不进内容表。内容表非法值仍是 `-1`。

## 4. 算法 / 公式

全部进 `config/formulas/ships.yaml`（战斗乘威力的闭式仍归 `formulas/combat.yaml`，本篇只导出 `power`）。

```
# 解锁（fn_140003bb1）
gun_unlocked(gun_type_id, military) =
    0 <= gun_type_id < n_types
    and military >= necessary_development[gun_type_id]
# necessary_development default [1, 1, 2000, 3500, 5000, 7000]
# military = Town.develop(town_id, track=military)     # track 1

# 威力（表 +0x11；空炮 7 → 0）
gun_power(gun_type_id) =
    0 if gun_type_id == empty_gun_type else types[gun_type_id].power
# ship_state==4 时战斗侧再 +2，本表不写死态名

# 长射程标记
long_range(gun_type_id) = gun_type_id in long_range_ids    # default [1, 5]

# 换炮价（fn_140013593）
gun_price(gun_type_id, kind8_count) =
    types[gun_type_id].price_base * size_mult[kind8_count] * price_scale
# size_mult default [1, 3, 9, 27, 81]     # 3^n，n=0..4
# price_scale default 20
# kind8_count = count cabins whose kind == gun_room_cabin_kind   # 原版 8
# kind8_count>=5 时原版下一档乘数是 0，价变 0；用 caps 卡在 0..4
# half=true 时整价 / 2（卖出路径 r8=1）

# 改装差价（fn_140013d68 后半，相对旧船 / 预览船）
# 炮种或 kind8_count 变化才走；具体加减与 >>1 分支按该函数还原，不要抄成货格公式

# 门数（fn_140004836）——显示「%s炮%s门」，也给舷侧火力读
doors(ship) =
    clamp(kind8_count(ship) * doors_per_room, doors_min, doors_max)
    + (bow_door_bonus  if has_add_part(ship, bow_gun_query)  else 0)
    + (stern_door_bonus if has_add_part(ship, stern_gun_query) else 0)
# doors_per_room=24, doors_min=2, doors_max=96
# bow_gun_query=6, stern_gun_query=7, bow/stern_door_bonus=6
```

原版包默认值：

```yaml
# config/formulas/ships.yaml  片段
gun_necessary_development: [1, 1, 2000, 3500, 5000, 7000]
gun_price_scale: 20
gun_size_mult: [1, 3, 9, 27, 81]
gun_room_cabin_kind: 8          # [推断] 炮手室；只当整数 id
gun_empty_type: 7
doors_per_room: 24
doors_min: 2
doors_max: 96
bow_door_bonus: 6
stern_door_bonus: 6
bow_gun_query: 6
stern_gun_query: 7
long_range_ids: [1, 5]
gun_power_ship_state_bonus: { state: 4, add: 2 }   # 态名未知
```

炮弹实例没有「种类伤害表」。命中时战斗读 `ShellSnapshot`（不透明），不要为 100 个 id 各编一个 dice。

## 5. 耦合分析

| 依赖 | 原版表现 | 重构 |
| --- | --- | --- |
| 港口军事发展度 | `fn_14000bc03(town, 1)` 滤上架 | 只收 `Town.military(town_id)` 标量 |
| 舱室种类 8 | 间数进价与门数 | `Cabins.count(ship, cabin_kind_id)` |
| 改装件 6 / 7 | 船首 / 船尾弧 + 门数 +6 | `Ships.has_add_part`；id 可配 |
| 船体 `vfunc+0x40` | 当前炮种 | 会话快照字段 `gun_type_id` |
| 海战 tick | 21 门炮 + 100 弹挂在 `CMobileShipElement` | 炮弹池归 combat；本表只给定义 |
| 城炮 | 池 21 的余数 | combat 向 `Town.try_fire` 要弹，不在本表造第 7 种炮 |
| 文案 / 像素 | `威力%s`、`长射程`、`%s炮%s门`、View 坐标 | `ui/strings` + `Gun.dk4` 帧 |
| 存档 | `CBattleGun` `CShell` 是 `CSaveObject` | 会话态；定义不进存档 |

禁止：ships → economy 写市价；本表结算炮弹伤害；UI 写存档。

04 的换炮命令仍有效：本表提供 `Yard.gunTypesAvailable` 的数据。`fn_140013593` 从 04 的「货格差价」更正为本表的换炮价。

## 6. 解耦后的配置

### `config/ships/guns.yaml`

```yaml
schema: dk4.ships.guns
version: 1
empty_equipped: 7                 # 运行时空炮；非法枚举仍是 -1
gun_room_cabin_kind: 8            # fn_140006b95 第二参；[推断] 炮手室
types:
  - id: 0
    key: saker
    name: 霰弹
    name_i18n: { zh-CN: 霰弹, zh-TW: 散彈, ja: セーカー, ko: 세이커 }
    power: 1
    price_base: 10
    long_range: false
    necessary_development: 1
    icon_frame: 0                 # Gun.dk4 帧
  - id: 1
    key: culverin
    name: 曲射
    name_i18n: { zh-CN: 曲射, zh-TW: 曲射, ja: カルバリン, ko: 캘버린 }
    power: 2
    price_base: 50
    long_range: true
    necessary_development: 1
    icon_frame: 1
  - id: 2
    key: pedrero
    name: 加农曲射
    name_i18n: { zh-CN: 加农曲射, zh-TW: 加農曲射, ja: ペドロ, ko: 페드로 }
    power: 3
    price_base: 70
    long_range: false
    necessary_development: 2000
    icon_frame: 2
  - id: 3
    key: cannon
    name: 加农
    name_i18n: { zh-CN: 加农, zh-TW: 加農, ja: カノン, ko: 캐논 }
    power: 4
    price_base: 100
    long_range: false
    necessary_development: 3500
    icon_frame: 3
  - id: 4
    key: heavy_cannon
    name: 重加农
    name_i18n: { zh-CN: 重加农, zh-TW: 重加農, ja: 重カノン, ko: 중캐논 }
    power: 5
    price_base: 200
    long_range: false
    necessary_development: 5000
    icon_frame: 4
  - id: 5
    key: carronade
    name: 连射
    name_i18n: { zh-CN: 连射, zh-TW: 連射, ja: キャロネード, ko: 캐노네이드 }
    power: 5
    price_base: 250
    long_range: true
    necessary_development: 7000
    icon_frame: 5

# 战斗炮位：每舰队 5 个船位各一门对象，不是 5 种炮
battle_slots:
  count: 5
  meaning: per_fleet_ship         # [推断] 与 eFleetShip 0–4 对齐
  scene_pool: 21                  # 4 舰队 × 5 + 1 城炮 [推断]
  object: CBattleGun
  stride: 0x18
  save:
    - { off: 0x8, width: u16, name: null }
    - { off: 0xA, width: u16, name: can_fire_word }   # 非 0 才能开火
  icon_pack: Gun.dk4              # 定义侧图标，不是每槽一张
```

`name` / `name_i18n` 是文案。逻辑只读 `id`、`power`、`price_base`、`long_range`、`necessary_development`。

### `config/ships/shells.yaml`

```yaml
schema: dk4.ships.shells
version: 1
# 飞行实例池。原版没有 100 种弹药名。
kind: instance_pool
count: 100
name: null
object: CShell
stride: 0x30
live_field: { off: 0x26, width: u16, live_if: ">0" }
save:
  - { off: 0x0C, width: u32, name: null }   # 生成函数写入；坐标候选 [推断]
  - { off: 0x10, width: u32, name: null }
  - { off: 0x14, width: u32, name: null }
  - { off: 0x18, width: u32, name: null }
  - { off: 0x1C, width: u32, name: null }
  - { off: 0x20, width: u32, name: null }
  - { off: 0x24, width: u16, name: null }
  - { off: 0x26, width: u16, name: remaining }  # >0 在飞 [确认]
  - { off: 0x28, width: u16, name: null }
# 若引擎必须显式行：id 0–99 全部 name: null，禁止手填「葡萄弹」之类
info_object:
  class: CShellInfo
  empty_at: { off: 0x0C, value: 0xFFFFFFFF }
  size: 0x28
```

不要写 `gun_type_id` 外键：原版弹实例与炮种没有 100:6 的定义表。战斗生成弹时把当前船的 `gun_type_id` / `power` 拷进快照即可。

### `config/caps.yaml` 片段

```yaml
gun_types: 6
gun_empty_equipped: 7
battle_guns: 5
scene_gun_pool: 21
shell_pool: 100
gun_rooms_max: 4                  # 文案「最多 4 个室」+ 价表 5 档
```

### 图标

```yaml
# config/ui/ 或 ships/guns.yaml 同文件
gun_icons:
  pack: Gun.dk4
  magic: GT1G0600
  frames: 6                       # 运行时读表长
```

## 7. 独立旋钮

改这些值不应迫使贸易、人物、城镇改代码：

- 六行的 `name` / `name_i18n` / `key`（只动本表 + 文案包）
- `necessary_development`（「7000 军事才能买连射」只改这一格）
- `power`、`price_base`、`long_range`
- `gun_size_mult`、`gun_price_scale`
- `doors_per_room` / 门数夹紧 / 船首尾 +6
- `battle_slots.count`、`scene_gun_pool`、`shell_pool`
- `gun_room_cabin_kind`（原版 8；模组可改指向别的舱室种类）
- `Gun.dk4` 帧数随炮种表长

加第 7 种炮：追加 `types` 行，解锁门槛自己写，战斗按表长枚举。不要在 combat 里写死 `for i in 0..5`。

## 8. 对外接口

查询（只读，只传 ID 与值）：

```
Guns.get(gun_type_id) -> GunTypeRecord | none
Guns.available(town_id) -> [gun_type_id]     # military >= necessary_development
Guns.power(gun_type_id) -> int               # 空炮 7 → 0
Guns.price(gun_type_id, kind8_count, half) -> gold
Guns.doors(ship_instance_id) -> int
Guns.equipped(ship_instance_id) -> gun_type_id | empty
BattleGuns.of_scene(battle_id) -> [scene_gun_id]
Shells.alive(battle_id) -> [shell_instance_id]   # +0x26 > 0
```

命令：

```
Yard.setGunType(ship_instance_id, gun_type_id)   # 仍由 04 的造船会话付钱
Combat.spawnShell(battle_id, shooter, target)    # 从池里找 +0x26==0 的槽
```

事件：

```
GunTypeChanged { ship_instance_id, gun_type_id }
ShellSpawned   { battle_id, shell_instance_id, gun_type_id, power }
ShellExpired   { battle_id, shell_instance_id }   # remaining 落到 0
```

伤害数字由 combat 订阅 `ShellSpawned` 之后自己算。本表不发 `ShipHullChanged`。

## 9. 未核实字段

| 位置 | 现状 | 禁止的做法 |
| --- | --- | --- |
| `eCabinKind==8` 的中文名 | 间数进价与门数；文案炮手室最多 4 | 写成确认的「炮手室」而不标推断 |
| `CBattleGun+0x8` | 存档 u16，无名字 | 写成 ammo / 炮种 |
| `CBattleGun+0xA` | 非 0 才能开火 | 写成剩余弹药发数（也可能是装填完成旗） |
| `CBattleGun+0x10` | 与 `cAngle` 同一构造 | 当成欧拉角 API |
| 场景下标 20 是不是城炮 | 21=4×5+1 的算术 | 写死 `scene_gun_id==20` |
| RTC `eBattleGun >= 20` | 与 21 并存 | 断言「每船五门」 |
| `CShell+0x0C..+0x28` 除 `+0x26` | 生成函数有写；`idiv` 脏 | 编成 x/y/vx/vy/damage 定论 |
| RTC `eShell >= 4` | 与 100 并存 | 造 4 个弹药中文名，或把 100 当种类数 |
| 装填间隔 tick | 文案「炮手缩短间隔」 | 按威力反推冷却 |
| 独立弹药仓 | 无 RTC 名 | 做第三种补给当原版行为 |
| 空炮 7 vs `INVALID_GUNTYPE=-1` 谁进存档 | 装备路径见 7 | 用 -1 当船上空炮字节 |
| `Gun.dk4` 每帧像素尺寸 | 只确认 6 帧 | 编 64×64 |
| 战斗乘威力的完整式 | 已定位 `fn_14000c6cb` 与 `* (3n+1)` | 从别的 SRPG 抄伤害表 |
| 04 货格差价完整式 | 换炮差价已从 `fn_140013593` 剥离 | 继续把该 thunk 当货格 |

炮弹伤害继续对着 `fn_14000ee8a` / `fn_14000c8c4` / `fn_1400052e0` 下断点还原，不要为本目录 100 行各填一个骰子。
