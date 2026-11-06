# 炮种、炮弹与战斗炮位目录

本篇补齐造船厂 **6 种大炮** 的名称与表内数值、战斗里 **`eBattleGun`（场景炮对象池下标，循环上界 21）** 的含义、以及 **100 个 `eShell`** 到底是不是 100 种弹药。改装流程、天数、装甲仍见 [04-remodel-guns-supplies.md](04-remodel-guns-supplies.md)；射程、命中、沉船见 [07-combat/01-naval-battle.md](../07-combat/01-naval-battle.md)。攻城把军事打穿会让本表解锁集合变小，见 [07-combat/04-ship-vs-port.md](../07-combat/04-ship-vs-port.md)。

04 把六炮中文名写成「不在 mesfile、保持未知」。名称在 **exe `.data` 的 GBK 定长记录**里，不在 `mesfile_sc.txt`。本篇按记录抽出；缺的名字写 `null`，禁止用「卡隆 / 臼炮」去填。

## 1. 职责边界

本系统管：

- 炮种定义表 `eGunType`（6）：稳定 key、四语言名、威力、价基、射程基值、长射程标记、军事解锁门槛
- 战斗炮对象 `eBattleGun`（场景池 21；头文件「5」为抽取脚本误猜）
- 飞行炮弹 **实例池** `eShell`（100）的布局与空闲判定；**不是** 100 种弹药目录
- 图标包 `Game/Gun.dk4`（G1T，6 帧，下标 = `gun_type_id`）
- 炮价（`fn_140013593`）用到的价基与 `3^n` 乘数：换炮差价与船只估价 `outfit_value` 都调它（公式参数进 `formulas/ships.yaml`）

本系统不管：

- 船首炮 / 船尾炮 / 撞角（改装件 `eAddParts`，04）
- 炮手室缩短装填的 tick 数（战斗读舱室修正器）
- 炮弹打船体 / 水手 / 城镇的伤害闭式（07-combat；本表只提供 `power`）
- 船体货格、耐久、帆（01）
- 市价（经济）

一艘船上的「装备炮种」是 **一个** `gun_type`（`CShip` 元素 `+0x52`，主 vtbl+0x40），不是 5 门不同种类。初值来自船体表 `+0x2E` / 船模表 `+0x35`（`05` 篇旧稿误作「船体等级」）。原版空炮哨兵是 **7**（`NUM_GUNTYPE+1`），不是头文件的 `INVALID_GUNTYPE=-1`；重构运行时为 `null`，7 只在 `ids/legacy_codec.yaml`（D1）。

## 2. 原版事实

### 2.1 计数与对象

| 符号 | 值 | 对象 | 来源 |
| --- | --- | --- | --- |
| `eGunType` / `NUM_GUNTYPE` | 6 `[确认]` | 造船厂可选炮种 | 判界：`fn_140012760` `id >= 6` 返回 NULL（§2.2） |
| `INVALID_GUNTYPE` | -1 | 非法枚举 | 同上 |
| 装备空炮 | **7** | `ship.vfunc+0x40 == 7` 则不画炮、威力 0 | `CSelectShipDrawLine.cpp`；`fn_14000c6cb`（`0x14023eb60`） |
| `eBattleGun` / `NUM_BATTLEGUN` | **21** `[确认]` 池 | 场景炮对象下标 | 证据类型：循环上界（唯一带 RTC `eBattleGun` 的函数 `fn_14000ee8a`（`0x1401c3210`）`cmp eBattleGun, 0x15`）+ 构造计数 21。`BattleGunID.h` / `num_values.json` 的 5 是投票误猜（同函数里 `mov edx,5` 等），作废 |
| `INVALID_BATTLEGUN` | -1 | | `BattleGunID.h` |
| 场景 `CBattleGun` 池 | **21** | `CMobileShipElement+0x1038`，步长 `0x18` | 构造 `fn_140007dce(..., 0x18, 0x15)` |
| `eShell` / `NUM_SHELL` | 100 `[确认]` | 飞行炮弹实例 | 循环上界 `>= 0x64` + 构造计数 |
| `INVALID_SHELL` | -1 | | `ShellID.h` |
| 场景 `CShell` 池 | 100 | `CMobileShipElement+0x1230`，步长 `0x30` | 构造 `fn_140007dce(..., 0x30, 0x64)` |
| `CMobileShip` 池 | 20 | 同对象 `+8`，步长 `0x30` | `NUM_MOBILESHIP=20` |

`Gun.h` 只有 `CBattleGun : CSaveObject`，没有炮种结构体。炮种表是 exe 静态数组，不是 World DLL。

`num_from_rtc.json` 对 `eBattleGun` 记 5×3、21×1、20×1；对 `eShell` 记 100×4、4×3、65×2、20×1。这是「同函数里所有立即数」的统计，不是循环上界。逐条反汇编：`eBattleGun` 局部只在 `fn_14000ee8a` 出现，其循环上界是 `0x15`；20（`0x14`）是同函数 `eMobileShip` 的上界；5 不是任何 `eBattleGun` 比较。`eShell` 的循环上界是 `0x64`。

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
- `[确认]` 船 `vfunc+0x48==4` 时威力再 `+2`（`0x14023ebb0`）。`vfunc+0x48` 读 `CShip+0x53` 船首像，4 = 龙像「对敌炮击能力提高」`[推断]` 强（`04`「船首像」）。重构：`figureheads.yaml` 的 `combat.gun_power +2` 贡献，本表不写。

`+0x12`（价基）：

- `[确认]` `fn_1400144d9`（`0x14023f490`）读 `BYTE [record+0x12]`。
- `[确认]` `fn_140013593`（`0x14023ec30`）：

```
base  = gun_record[+0x12]          # 10,50,70,100,200,250
mult  = u16_table[hull_param]      # 0x140596d78 = 1,3,9,27,81
price = base * mult * 20
if half: price /= 2
```

`u16_table` 前五档是 `3^n`（n=0..4），第 6–8 个 u16 是 0（`[确认]` 读 `0x140596d78`），所以 5 间及以上价为 0。`hull_param` 来自 `fn_140006b95(ship, 8)`：扫 31 个 `eShipCabin`，计 `vfunc+0x38(slot)==8` 的间数。`[确认]` kind 8 = **「武装炮台」**：舱室种类表由 `0x1400256d0` 按行号建表，第 8 行名就是它（`03`）；「炮手」是坐在这间舱的岗位（`fn_140007e19(9) = 8`）。它同时决定门数（每间 24 门）和必要水手（每间 +20，`06`）。重构只调 `Ships.cabin_count(ship, gun_room)`。

`fn_140012760` 对 `id` 越界（含空炮 7）返回 NULL，`fn_1400144d9` **不判空**直接读 `[NULL+0x12]`。所以原版对无炮船调 `fn_140013593` 会崩；正常流程里被估价的船都有炮。重构：`gun_price(empty) = 0`。

调用点 `[确认]`：

| 调用者 | 实参 | 用途 |
| --- | --- | --- |
| `fn_14000623f`（`0x14022ff50`）← `CShip::vfunc_04`（`0x140230140`） | `(ship.vtbl40(), count(kind 8), half=1)` | 船只估价 `outfit_value` 的炮价项；买船 `<<1`、卖船原值（`05`） |
| `fn_140013d68`（`0x1400a9d20`）后半 | 新旧两船各算一次 | 改装 / 换炮差价（`04`） |

04 与 05 旧稿都把这一项叫「货格差价」。它是**炮价**：炮种价基 × `3^武装炮台间数` × 20（估价再 /2）。货格本身在估价里没有单独项 `[确认]`（`vfunc_04` 只有表价、桅、改装件、炮价四项）。

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

`[确认]` 实际写法：先数 `military >= 表[i]` 的个数 `n`（`jl` 跳过，有符号），再只铺 `id < n` 的炮按钮（`0x1400a36b1` `cmp eGunType, n; jge`）。门槛单调递增时等价于逐条比较；模组若给出非单调门槛，原版语义是「前缀」，重构按逐条比较并在 schema 里校验单调。单位与发展度 0–9999 同量级。来源：`fn_1400a1140.cpp` 的 `fn_140003bb1`；与 [04-ship-vs-port.md](../07-combat/04-ship-vs-port.md) §2.7 一致。04-remodel 里「商业 / 军事哪一条未知」以本篇为准。

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
| 头文件 `NUM_BATTLEGUN=5` | **作废** | 抽取脚本误猜；`eBattleGun` 循环上界是 `0x15`（见 §2.1） |
| 场景里实际构造 **21** 个 `CBattleGun`，每个 **0x18** 字节 | `[确认]` | `CMobileShipElement` 构造 / 析构循环 `>= 0x15` |
| 同对象上 20 艘 `CMobileShip` + 100 发 `CShell` | `[确认]` | 同上；`02-fleet-proxy.md` |
| 一艘船只有一个 `gun_type_id`（`vfunc+0x40`） | `[确认]` | 选炮、船信息、换炮差价都只读这一处 |
| 海战 tick 先扫 21 门炮再扫 100 发弹 | `[确认]` | `fn_14000ee8a`（`0x1401c3210`）RTC `eBattleGun` / `eShell` |
| 炮 `i`（0–19）↔ 移动船 `i`，炮 20 = 城炮 | `[确认]` | `fn_14000e6ba`（`0x14014a050`）下标 `>= 0x14` 返回城炮分支（`07-combat/01` §2.4）；与 `NUM_MOBILESHIP=20` 对齐 |
| `eBattleGun` 就是场景池下标 0–20 | `[确认]` | RTC 局部 `eBattleGun` 直接当 `fn_1400024be(mgr, i)` 的下标、上界 `0x15`；没有「每舰队 5 炮位」的证据 |

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
- `[确认]` 生成 `fn_1400052e0`（`0x140149580`）往 `+0x0C/+0x10/+0x14/+0x1C/+0x20` 和三个 u16 写值。字段与弹道（落点、速度、`T = isqrt(d2)/4` 飞行 tick、散布 `±T*192` 定点）已按原始指令还原在 `07-combat/01-naval-battle.md` §2.8，本篇不重复。
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
| 炮种 | `eGunType` | 6（0–5）`[确认]` 判界 | 非法 `-1`；装备空 **7**（legacy） | `gun_type` key | 表长；空 = `null` |
| 场景炮下标 | `eBattleGun` | `[推断]` 21（0–20） | `-1` | `scene_gun_id` | `caps.scene_gun_pool` |
| 场景炮对象 | `CBattleGun` | 21 | 下标越界不取 | `scene_gun_id` | `caps.scene_gun_pool` |
| 飞行炮弹 | `eShell` / `CShell` | 100 | `-1`；`+0x26==0` 空闲 | `shell_instance_id` | `caps.shell_pool` |
| 炮弹定义 | （无） | 0 | — | — | 不要为原版包造 100 行弹药 |

模组加炮种：追加 `guns.yaml` 行，主键是命名空间 key（`mymod:swivel_gun`），加载器分配下标（D1）。图标包按表长扩展，UI 不要写死 6 格。

「没装炮」运行时是 `gun_type = null`；原版 7 只在 legacy_codec 导入导出。

## 4. 算法 / 公式

全部进 `config/formulas/ships.yaml`（战斗乘威力的闭式仍归 `formulas/combat.yaml`，本篇只导出 `power`）。

```
# 解锁（fn_140003bb1）
gun_unlocked(gun_type, military) =
    military >= guns[gun_type].necessary_development
# necessary_development default [1, 1, 2000, 3500, 5000, 7000]
# military = Towns.develop(town_id, military)     # 原版 track 1

# 威力（表 +0x11；无炮 → 0）
gun_power(gun_type) = gun_type == null ? 0 : guns[gun_type].power
# 战斗侧再加 Modifiers.eval("combat.gun_power", shooter)（原版：船首像 4 龙像 +2，figureheads.yaml 注册）

# 射程基值（fn_140002d2e，0x140123290 switch）与长射程标记
range_base(gun_type) = gun_type == null ? 0 : guns[gun_type].range_base   # 70/98/70/70/70/98
long_range(gun_type) = guns[gun_type].long_range           # 原版 UI 写死 id {1,5}，恰是 range_base 98 的两行

# 换炮价（fn_140013593）
gun_price(gun_type, n_gun_rooms) =
    guns[gun_type].price_base * size_mult[n_gun_rooms] * price_scale
# size_mult default [1, 3, 9, 27, 81]     # 3^n，n=0..4
# price_scale default 20
# n_gun_rooms = Ships.cabin_count(ship, gun_room)            # 原版 fn_140006b95(ship, 8)
# n>=5 时原版下一档乘数是 0，价变 0；gun_room 行 max_per_ship: 4（03）挡住
# half=true 时整价 / 2（无符号）：outfit_value 路径（fn_14000623f r8=1），买卖都经它
# gun_type == null → 0（原版会解引用 NULL）

# 改装差价（fn_140013d68 后半，相对旧船 / 预览船）
# 炮种或 gun_room 间数变化才走；已还原，见 04「改装费」：同炮减间数退半、换炮旧炮折半

# 门数（fn_140004836）——显示「%s炮%s门」，也给舷侧火力读
doors(ship) =                                               # Ships 派生量（D6），进 Ships.derived().doors
    clamp(Ships.cabin_count(ship, gun_room) * doors_per_room, doors_min, doors_max)
    + Σ_{p installed} add_parts[p].door_bonus               # 04 add_parts.yaml：bow_gun 6、stern_gun 6
# doors_per_room=24, doors_min=2, doors_max=96
```

原版包默认值：

```yaml
# config/formulas/ships.yaml  片段
gun_price:                        # §4 换炮价（fn_140013593）；half / null / n>=5 分支见 §4
  impl: vanilla.gun_price
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }  # half=true 时整价 / 2（无符号）
  params:
    gun_price_scale: 20
  tables:
    gun_size_mult: [1, 3, 9, 27, 81]
doors:                            # §4 门数（fn_140004836）
  impl: expr
  params:
    doors_per_room: 24
    doors_min: 2
    doors_max: 96
  inputs: [gun_rooms, door_bonus_sum]   # gun_rooms = Ships.cabin_count(ship, gun_room)；door_bonus_sum = Σ add_parts[p].door_bonus
  expr: "clamp(gun_rooms * doors_per_room, doors_min, doors_max) + door_bonus_sum"
# 门槛 / 长射程 / 射程基值在 guns.yaml 每行；计数舱由 role: gun_room 决定；船首 / 尾 +6 在 add_parts.yaml；龙像 +2 在 figureheads.yaml
```

炮弹实例没有「种类伤害表」。命中时战斗读 `ShellSnapshot`（不透明），不要为 100 个 id 各编一个 dice。

## 5. 耦合分析

| 依赖 | 原版表现 | 重构 |
| --- | --- | --- |
| 港口军事发展度 | `fn_14000bc03(town, 1)` 滤上架 | 只收 `Towns.develop(town_id, military)` 标量 |
| 舱室种类 8 | 间数进价与门数 | `Ships.cabin_count(ship, gun_room)` |
| 改装件 6 / 7 | 船首 / 船尾弧 + 门数 +6 | `add_parts.yaml` 行的 `gun_arc` / `door_bonus` |
| 船首像 4 | 威力 +2 | `Modifiers.eval("combat.gun_power", ship)` |
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
# 主键 key；vanilla_index 只供导入。空炮运行时 null（原版 7 在 legacy_codec）。
# range_base = fn_140002d2e 的 switch（07-combat/01），不在 exe 表里，重构并进同一行。
types:
  - key: vanilla:saker
    vanilla_index: 0
    name: 霰弹
    name_i18n: { zh-CN: 霰弹, zh-TW: 散彈, ja: セーカー, ko: 세이커 }
    power: 1
    price_base: 10
    long_range: false
    range_base: 70                # fn_140002d2e
    necessary_development: 1
    icon_frame: 0                 # Gun.dk4 帧
  - key: vanilla:culverin
    vanilla_index: 1
    name: 曲射
    name_i18n: { zh-CN: 曲射, zh-TW: 曲射, ja: カルバリン, ko: 캘버린 }
    power: 2
    price_base: 50
    long_range: true
    range_base: 98                # fn_140002d2e
    necessary_development: 1
    icon_frame: 1
  - key: vanilla:pedrero
    vanilla_index: 2
    name: 加农曲射
    name_i18n: { zh-CN: 加农曲射, zh-TW: 加農曲射, ja: ペドロ, ko: 페드로 }
    power: 3
    price_base: 70
    long_range: false
    range_base: 70                # fn_140002d2e
    necessary_development: 2000
    icon_frame: 2
  - key: vanilla:cannon
    vanilla_index: 3
    name: 加农
    name_i18n: { zh-CN: 加农, zh-TW: 加農, ja: カノン, ko: 캐논 }
    power: 4
    price_base: 100
    long_range: false
    range_base: 70                # fn_140002d2e
    necessary_development: 3500
    icon_frame: 3
  - key: vanilla:heavy_cannon
    vanilla_index: 4
    name: 重加农
    name_i18n: { zh-CN: 重加农, zh-TW: 重加農, ja: 重カノン, ko: 중캐논 }
    power: 5
    price_base: 200
    long_range: false
    range_base: 70                # fn_140002d2e
    necessary_development: 5000
    icon_frame: 4
  - key: vanilla:carronade
    vanilla_index: 5
    name: 连射
    name_i18n: { zh-CN: 连射, zh-TW: 連射, ja: キャロネード, ko: 캐노네이드 }
    power: 5
    price_base: 250
    long_range: true
    range_base: 98                # fn_140002d2e
    necessary_development: 7000
    icon_frame: 5

# 场景炮对象池：eBattleGun = 池下标；不是「每船 5 种炮」
battle_guns:
  scene_pool: caps.scene_gun_pool # 21；4 舰队 × 5 + 1 城炮 [推断]
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
scene_gun_pool: 21                # eBattleGun 循环上界；guns.yaml battle_guns.scene_pool 引用此键
shell_pool: 100
# 炮种数读表长；武装炮台最多 4 间在 03 cabins.yaml max_per_ship（价表 5 档 0–4 间，第 5 间起原版价 0）
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
- `battle_guns.scene_pool`、`shell_pool`
- 哪种舱算炮舱：`cabins.yaml` 的 `role: gun_room`（原版种类 8）
- `Gun.dk4` 帧数随炮种表长

加第 7 种炮：追加 `types` 行，解锁门槛自己写，战斗按表长枚举。不要在 combat 里写死 `for i in 0..5`。

## 8. 对外接口

查询（只读，只传 ID 与值）：

```
Guns.get(gun_type) -> GunTypeRecord | none
Guns.available(town_id) -> [gun_type]        # military >= necessary_development
Guns.power(gun_type?) -> int                 # null → 0
Guns.rangeBase(gun_type?) -> int             # null → 0
Guns.price(gun_type, n_gun_rooms, half) -> gold
Guns.doors(ship_instance_id) -> int          # = Ships.derived().doors
Guns.equipped(ship_instance_id) -> gun_type?
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
| `CBattleGun+0x8` | 存档 u16，无名字 | 写成 ammo / 炮种 |
| `CBattleGun+0xA` | 非 0 才能开火 | 写成剩余弹药发数（也可能是装填完成旗） |
| `CBattleGun+0x10` | 与 `cAngle` 同一构造 | 当成欧拉角 API |
| 城炮固定在下标 20 | 已确认（`07-combat/01`），但重构仍按 `scene_gun_pool` 末位配置 | 写死 `scene_gun_id==20` |
| `NUM_BATTLEGUN` 头文件正式值 | 循环上界 21；头文件 5 为投票误猜 | 断言「每船五门」 |
| RTC `eShell >= 4` | 与 100 并存 | 造 4 个弹药中文名，或把 100 当种类数 |
| 装填间隔 tick | 文案「炮手缩短间隔」 | 按威力反推冷却 |
| 独立弹药仓 | 无 RTC 名 | 做第三种补给当原版行为 |
| 空炮 7 vs `INVALID_GUNTYPE=-1` 谁进存档 | 装备路径见 7 | 用 -1 当船上空炮字节 |
| `Gun.dk4` 每帧像素尺寸 | 只确认 6 帧 | 编 64×64 |
| 04 货格差价完整式 | 换炮差价已从 `fn_140013593` 剥离 | 继续把该 thunk 当货格 |

炮弹伤害闭式（舷侧 `×(3n+1)`、首尾弧不乘、命中半径、暴击、着火）已在 `07-combat/01` §2.8–2.10 还原；不要为本目录 100 行各填一个骰子。
