# 船体模型

船的**定义**（船模 / 船体目录）和**实例**（某艘正在用的船）分开。本篇只管实例：槽位分段、实例布局、耐久、水手、名称、货格。船模表与 107 条船体定义的逐行名录见 `05-hull-catalog-market-start.md`；舱室岗位见 `03-cabins-crew-posts.md`；改装 / 炮 / 补给见 `04-remodel-guns-supplies.md`、`08-gun-shell-catalog.md`；舰队编成见 `02-fleet-proxy.md`；停泊港见 `07-dock-warehouse.md`。

## 1. 职责边界

本系统管：

- 207 个船实例槽 `eShip` 的分段与生命周期
- 船模 `CShipType`（39）与船体定义（107）**被实例引用的方式**（逐行内容在 `05`）
- 船的派生量（最大耐久、必要 / 上限水手、可用货格、帆值、炮门数）的**唯一计算方**；其它系统只读快照，不存档（D6）
- 单船实例的耐久、水手、名称、货格、改装位、炮种字段
- 货格所有权：货格属于船；贸易会话只是借来读写

本系统不管：

- 人物属性、招募、疲劳（`03-characters`）
- 舱室种类与 `person_id` 绑定（本目录 `03`）
- 帆、船首像、火炮、补给消耗（本目录 `04` / `08`）
- 舰队编成、委任、阵型、地方舰队（本目录 `02`）
- 市价与买卖会话（`04-economy`）
- 海战伤害结算（`07-combat`）

## 2. 原版事实

### 枚举计数：头文件值多半是启发式

`srcdata/source/*ID.h` 里 `NUM_SHIP=100`、`NUM_SHIPTYPE=5`、`NUM_SHIPMAST=100` 都来自 `catalog/num_values.json`，而它是 `tools/fill_gaps.py::collect_num_from_rtc` 按「RTC 局部名 vs 立即数」投票出来的众数，不是原版头文件。投票明细 `catalog/num_from_rtc.json`：

| 局部名 | 立即数（次数） | 实际用法 |
| --- | --- | --- |
| `eShip` | 100（21）、5、8、3、194（3）、207（3）、208（1） | 循环 `0..0x64`、`0x64..0xC2`、`0..0xCF`；见下 |
| `eShipType` | 5（5）、97（4）、39（3）、34（2） | 内存比较只见 `0x22`、`0x27`（`0x1400f41e0`、`0x1400fc690`、`0x140240240`、`0x1402613e0`） |
| `eShipMast` | 3（21）、100（4） | 十余个函数里只与 `3` 比（如 `0x1400a9d20`、`0x14023f5a0`） |

- `[确认]` `eShip` 是 **207 个实例槽的下标**，不是「100 条目录」。`CShipElement` 构造 `fn_1400017df(..., 0x80, 0xcf)`：207 个 `CShip`，每个 `0x80`。来源：`game/ship/CShipElement.cpp`。
- `[推断]` 原版 `NUM_SHIP` 是 100 还是 207 不能定。100 是动态段 / 预置段的分界（下表），断言 `eData < NUM_SHIP` 若取 100，开局舰队里的 194 会过不了，故 207 更像。依据：上表投票、`catalog/asserts.txt` L48。
- `[推断]` `eShipType` 实为船模下标 0–38（`0x27=39`），`0x22=34` 是可买船模（`NUM_NORMALSHIPTYPE`，断言 `eShipType < NUM_NORMALSHIPTYPE`）。头文件 `NUM_SHIPTYPE=5` 不可信。
- `[推断]` `eShipMast` 是桅下标 0–2，`NUM_SHIPMAST` 应为 3，不是 100。

证据类型（`NUM_*` 一律注明）：207 = 构造计数 + 循环上界 `0..0xCF`；39 = 构造计数 + 循环上界 `0x27`；34 = 循环上界 / `rand(0x22)`；3 = 比较立即数；`NUM_SHIP=100`、`NUM_SHIPTYPE=5`、`NUM_SHIPMAST=100` = RTC 投票，**不得**标 `[确认]`。

### 207 槽分段

`[确认]` 来源均为反汇编（`srcdata/tools/fn.sh`）：

| 槽 | 用途 | 证据 |
| --- | --- | --- |
| 0–7 | 不参与动态分配 | `fn_1400018e8`（`0x14022fc30`）找空槽从 **8** 起到 `<0x64` |
| 8–99 | 动态船（买 / 造 / 拿到的船） | 同上；`fn_140004854`（`0x14022fb30`）按 `+0x54` 停泊港列 0–99 |
| 100–206 | 预置船：按 `id-100` 从静态定义表初始化 | `CShip::vfunc_05`（`0x1402adee0`） |
| `0xD0` | 空 / 查无 | `fn_14000ceaa`（`0x14022fdf0`）扫 `0..0xCF` 查无返回 `0xD0` |

0–7 的用途 `[未知]`。

`CShip::vfunc_05`（`0x1402adee0`，经 `this+8` 子对象调用）原始指令：

```
mov byte [this8+0x4b], 0x0d          ; CShip+0x53
call fn_1400026e9 (CShip*)           ; 槽下标 = (this - pool) >> 7
cmp eax, 0x64
jge init_from_def                    ; id >= 100 → 按定义初始化
mov byte [this8+0x49], 0x28          ; id < 100 → CShip+0x51 = 0x28（空）
ret
```

反编译 `CShip.cpp` 把 `jge` 写成了反向的分支；以汇编为准。

- `[确认]` 静态船体定义表：`fn_14000cff9`（`0x1402ac5e0`）`= 0x1405b42e0 + (id-100)*0x30`。表里**107** 条有效（槽 100–206），第 107 条起全 0。100–193 为船坞模板 / 商用 / 武装变体，194–201 为主角船（`+0x08` 人物 0–6，198 为 `0xEC`），202–205 为怪鱼 / 大墨鱼 / 鲨鱼 / 白鲸，206 为幽灵船。名称全部是 `.rdata` GBK 串。来源：`Game/DK4HD_sc.exe` 直读。
- `[确认]` 船模表：`fn_14001372d`（`0x1402ac610`）`= 0x1405b3460 + type*0x40`，39 条，名称同样在 `.rdata`。`CShipTypeElement` 构造 `fn_1400017df(..., 0x50, 0x27)`：39 个 `CShipType`，每个 `0x50`。来源：`game/ship/CShipTypeElement.cpp`、exe。
- `[确认]` 船体 / 船模的**中文名都在 exe**，不在 `mesfile_sc.txt` 或 World DLL。逐行见 `05`。
- `[确认]` `fn_14000a984`（`0x14022fd00`）扫 `0x64..0xC1`，按 `vfunc+0x18`（船模下标）收集：船坞买船用的是 100–193 的模板实例。
- `[确认]` `CShipElement+0x6788` 起 8 字节（紧接 207×0x80 之后）：`~CShipElement` 写成 `0xD0`，`vfunc_02` 逐字节存档。语义 `[未知]`，像 8 个 `eShip` 引用。来源：`CShipElement.cpp`。
- `[确认]` 空船哨兵 `0xD0=208`：`CFleetShip` 空构造写 `+0x8=0xD0`，`CBattleShip` 构造写 `+0x8=0xD0` 且 `vfunc_01` 以 `==0xD0` 判空，阵型 / 委任 UI 把未放的船位填 `0xD0`。208 在 207 槽之外。来源：`game/fleet/CFleetShip.cpp`、`CBattleShip.cpp`。

### 玩家可见上限

- `[确认]` 「一只舰队最多可以有五艘船，新船放在船坞里。」来源：`mesfile_sc.txt` `0x7df7`（与流行品句同串）。
- `[确认]` 「持有的船过多了！这个城市没有可以制造的船！」「请给船取个名字！不能再把船处置掉了！还有剩余货物，这样可以吗？」来源：`mesfile_sc.txt` `0x2e4f` / `0x2e93`。持船上限即动态段 8–99 的 92 槽 `[推断]`。
- `[确认]` 玩家舰队船位 `NUM_FLEETSHIP=5`。证据类型：构造计数 + 循环上界（`02`）；`eFleetShip` 与 5 比较 236 次是投票旁证。
- `[确认]` 每船货格记录 5 条（`+0x58` 起 5×8，`vfunc_05` / 存档循环 `<5`）。`NUM_SHIPLOAD=5`（证据类型：循环上界）、`NUM_SHIPLOAD_EXCH=5`（头文件 / 投票）。来源：`CShip.cpp`、`Constants.h`、`include/trade.h`。
- `[确认]` 货格空商品 `ITEM_EMPTY=0x6A=106`。`ClearHoldRecord`（`0x1400fce20`）写 `+0=0x6A`、`+1=fn_1400ff280(1)`、`+2=0x62`、`+4=0`。禁止用 0 当空格（0 是南瓜种子）。

### 可用货格数 = 货舱舱室数

- `[确认]` 旧称 `CShip::CountHolds(10)` 的 `fn_140006b95`（body `0x14023e4c0`）是 **`CountCabinKind(ship, kind)`**：扫 31 个 `eShipCabin`，数 `vfunc+0x38(slot) == kind` 的间数。`10` 是舱室种类，不是「扫 10 格」。同一函数还以 `7`、`8`、`0xb` 调用（`0x14023e9e0`、`0x14023ed40`、`0x14023f4b0`）。
- `[确认]` `PackToShip`（`0x140279750`）：`n = CountCabinKind(ship, 10)`；对 `eShipLoad` 0–4，`k >= n` 即停。可用货格 = `min(种类 10 间数, 5)`。来源：`game/trade/Trade.cpp` 注释 + 上面反汇编。
- `[确认]` 39 个船模默认布局里种类 10 最多 5 间（北海大船 16–19、日本 32），与 5 条记录对齐。来源：exe 船模表 `+0x16` 31 字节统计。
- `[确认]` 种类 10 在 exe 舱室种类表里名为「货物仓库」（`03`）。`Constants.h` 的 `NUM_HOLD_SCAN=10` 与 `Ship.h` 注释是误读。

### 耐久与水手

- `[确认]` `SHIP_DURABILITY_MIN/MAX`、`SHIP_CREW_MIN/MAX` 都是 `USHORT`，断言 `<= _GetDataSizeMax_BYTE2DWORD(sizeof(USHORT))`，即硬顶 65535。来源：`ShipBase.h`、`catalog/asserts.txt` L302–305。
- `[未知]` 这两个 MIN/MAX 的十进制原版值。`vfunc_14` / `vfunc_16` 直接写字段，getter `0x1400aa600..0x1400aa660` 只是 `movzx word`，都**没有**夹取。
- `[确认]` 新船初始耐久 `+0x4A = fn_1400084db(ship)`（`0x14023f270`）：

```
armor  = has_part(2) + has_part(3)                 # +0x4E 位 2、3
tier   = ship_type.size_class  (CShipType+0x18；无船模时 4)
dur    = armor * T_ARMOR[tier]                      # .data 0x1406876d0 = {6, 8, 10}
       + (vfunc+0x48(ship) == 3 ? 10 : 0)           # +0x53 船首像 3 = 白鲸像「不易沉」
       + ship_type.base_durability (CShipType+0x1A)
```

  依据：`fn_14000d152`→`fn_14000e093` 读 `+0x18`；`fn_14000e129`→`fn_14000a2ae` 读 `+0x1A`；`fn_1400091e2` 是 `(vfunc+0x30 & (1<<n)) > 0`。语义「最大耐久」`[推断]`：`07-combat` 把 `vfunc+0x50`（读 `+0x4A`）当船体 HP；船模 `+0x0A` 取值 25–100，海怪 500–999。
- `[确认]` 新船初始水手 `+0x4C = (u8)((fn_14000a466(ship) + fn_140012a26(ship)) / 2)`：`cdq; sub; sar 1; movzx eax, al`，**截到 8 位**再存 u16。`fn_140012a26 = 5*T_TIER[tier] + 60*CountCabinKind(7) + fn_14000a466`，`T_TIER`（`.rdata 0x140596d98`）= `{3, 5, 10}`。`fn_14000a466 = T_TIER[tier] + 桅加项(fn_14000feac，含改装位 0/1) + 40*CountCabinKind(0xb) + fn_140001e2e`（展开见 `06` §2.4）。语义：`fn_140012a26` = 水手上限、`fn_14000a466` = 必要水手 `[推断]` 强（出港 `crew < fn_14000a466` 警告「水手不足」；`nAddCrew = vfunc+0x58 − fn_140012a26`）。
- `[确认]`（按 exe 静态表逐行计算）初值截 u8 在原版数据里**真的发生**：107 条船体里 6 条模板的 `(min+max)/2 > 255`——46 军用铁甲船 299→43、70 大型西班牙方帆军船 276→20、86 大型桨帆并用武装船 309→53、87 巨型军用桨帆并用船 349→93、90 阿拉伯三角帆武装船 269→13、106 幽灵船 277→21。90 号（槽 190 = `0xBE`）被大量 NPC 舰队模板引用（W0 舰队 5、14、18、32、44、57、60、61），这些 NPC 船开局水手只有 13。之后是否有别处重写模板水手 `[未知]`。
- `[确认]` 改造文案把船分成「中型以上」「大型」：中型以上可加船首帆，大型可加船尾帆。来源：`mesfile_sc.txt` `0xc5d7` / `0xc663`。对应船模 `+0x08` 体型档 0/1/2 `[推断]`（取值见 `05`）。
- `[确认]` 改装计价 `fn_140013d68`（`0x1400a9d20`）里的 `4000` / `7000` 比的是**当前港口商业发展度** `fn_14000bc03(town, 0)`（`town` 来自 `fn_1400a7820`），不是船的耐久，见 `04`。船的体型档直接读船模 `+0x08`，不经阈值。旧稿把它写成「耐久体型档」是误读。

### `CShip` 布局（相对 `CShip*`）

继承：`CShip → CShipObject → CShipBase → CSaveObject → CObject`。构造在 `this+8` 再挂一份 `CSaveObject` 虚表。析构尺寸 `0x80`。

存档 `vfunc_07`（body `0x140230ac0`）/ 读档 `vfunc_06`（`0x140231300`）与初始化 `vfunc_05` 都以 `this+8` 为基址，下表已加 8。存档顺序：名 19 B → `+0x50` → `+0x51` → `+0x23..25` → `+0x4E` → 舱室 31 B → `+0x52` → `+0x53` → `+0x4A` → `+0x46` `+0x48` → 5 条货格 → `+0x4C` → `+0x54`。

| 偏移 | 宽 | 存档 | 初值（预置船） | 含义 | 等级 |
| --- | --- | --- | --- | --- | --- |
| `+0x00` | 8 | 否 | — | 虚表 | `[确认]` |
| `+0x08` | 8 | 否 | — | 内嵌 `CSaveObject` 虚表 | `[确认]` |
| `+0x10` | 19 B | 是 | `lstrcpyA(def+0x00 名)` | GBK 船名（循环 `0..0x12`） | `[确认]` |
| `+0x23` | 3 B | 是 | 定义 `+0x0C..0x0E` | 三根桅（`eShipMast` 0–2），每桅一字节：bit0 有桅、bit1 横帆（否则纵帆）、bit2 加横帆件、bit3 加纵帆件（帆值 / 必要水手公式见 `06-ocean/01-sailing.md` §2、`06`） | `[确认]` 拷贝与位测试；位名 `[推断]` |
| `+0x26` | 31 B | 是 | 定义 `+0x0F..0x2D`，`0x17→0x18` | `eShipCabin` 槽，每槽一个舱室种类；`vfunc_07` 读 | `[确认]` |
| `+0x46` / `+0x48` | 2×u16 | 是 | 0 | 未命名 | `[未知]` |
| `+0x4A` | u16 | 是 | `fn_1400084db` | 当前耐久；`vfunc_10` 读 / `vfunc_14` 写；装甲改装后 `clamp(1, +0x4A+Δ, fn_1400084db)` 写回（`0x1400a0420`） | `[推断]` 强 |
| `+0x4C` | u16 | 是 | 见上「水手」 | 当前水手；`vfunc_11` 读 / `vfunc_16` 写。航速拿它比必要水手（`06-ocean/01`），海战槽初始化把它存作 crew（`07-combat/01` `fn_140011603`） | `[推断]` 强 |
| `+0x4E` | u16 | 是 | 定义 `+0x0A` | 改装件位域；`vfunc_06` 返回时清位 15 | `[确认]` 位读法；位名见 `04` |
| `+0x50` | u8 | 是 | 定义 `+0x08` | 舰长 `person_id`。`fn_1400056cd`（`0x14022fac0`）读它，`0xEC` 时改取 `fn_140230100()` 给的默认人物；卖船列表用它比提督（`07`）。初值：主角船 0–6、海怪 197、其它 `0xEC` | `[推断]` 强 |
| `+0x51` | u8 | 是 | 定义 `+0x09`；空槽 `0x28` | 船模下标 0–38；`vfunc_03` 读；`vfunc_01` 以 `!= 0x28` 判有效 | `[确认]` |
| `+0x52` | u8 | 是 | 定义 `+0x2E` | 炮种 `eGunType`；`vfunc_08` 读；`7` = 无炮（`08`） | `[确认]` 读写；语义见 `08` |
| `+0x53` | u8 | 是 | `0x0D` | **船首像** `eFigureHead`；`vfunc_09` 读。`CShipObject::vfunc_09`（`0x14023e7f0`，海战 `CBattleShip` 走它）查不到船时也返回 `0x0D`，故 `0x0D` = 无。各值的比较点见 `04`「船首像」 | `[推断]` 强（9 处效果与道具文案逐一吻合） |
| `+0x54` | u8 | 是 | `0x62` | 停泊港 `town_id`，`0x62` = 在海 / 未入坞（`07`）；`fn_14000175d` 读 | `[推断]` |
| `+0x58` | 5×8 | 是 | `ClearHoldRecord` | 货格。每条：`+0` 商品、`+1`、`+2`（空时 `0x62`，疑为购入港）、`+4` dword 买入价。`+3` 不存 | `[确认]` 布局；`+1/+2` 语义 `[推断]` |

`0x28=40`：船模池 39 条之外的值，做「无船模」哨兵 `[推断]`。

贸易开市 `UnpackFromShip` 从这 5 格生成交易槽并清空船货；确认后 `PackToShip` 按「货舱间数」写回。来源：`srcdata/docs/trade_stack.md`、`game/trade/Trade.cpp`。

### `CShipType` 布局

尺寸 `0x50`。`vfunc_05`（`0x1402ae1f0`）同样经 `this+8` 调用；`CShipType.cpp` 反编译里的偏移要再加 8。下表已换成 `CShipType*` 偏移；源列是船模静态表（步长 `0x40`）。

| 偏移 | 宽 | 源 | 含义 | 等级 |
| --- | --- | --- | --- | --- |
| `+0x10` | ptr | `+0x00` | 名称指针 | `[确认]` |
| `+0x18` | u8 | `+0x08` | 体型档 0/1/2（海怪 / 幽灵 2） | `[确认]` 取值；`fn_14000e093` 读 |
| `+0x1A` | u16 | `+0x0A` | 基础耐久（25–100；海怪 500–999） | `[推断]`；`fn_14000a2ae` 读，进耐久式 |
| `+0x1C` | u32 | `+0x0C` | 表价（1200–160000；海怪 0）；主 vtbl+0x20（`0x1402a87d0`）返回，买价起点（`05`） | `[确认]` |
| `+0x20` | 3 B | `+0x10` | 默认三桅 | `[推断]` |
| `+0x24` | u16 | `+0x14` | 默认改装位（战列舰 `0x42`、铁甲船 `0xC4`）；主 vtbl+0x30（`0x1402a78a0`）返回 | `[确认]` 与 `CShip+0x4E` 同形 |
| `+0x26` | 31 B | `+0x16` | 默认舱室种类；`== 0x17` 改写成 `0x18` | `[确认]` 循环；改写原因见 `03` |
| `+0x45` | u8 | `+0x35` | 默认炮种 `eGunType`；主 vtbl+0x40（`0x1402a7c90`）读，同规格天数比它（`05`） | `[确认]` |
| `+0x46` | u16 | `+0x36` | 船坞上架发展门槛（100–3000）；`fn_140002e9b` 读 | `[确认]`（`05`） |
| `+0x48` | u8 | `+0x38` | 地域位图（`fn_1400013bb` 测 `1<<area`，`05`）；也是唯一存档字段（`vfunc_07` `0x140240460`）→ 运行时可变 `[推断]`，写入者未查 | `[确认]` 位测试 |

`CShip` 与 `CShipType` 的舱室都落在 `+0x26`，桅分别在 `+0x23` / `+0x20`。

**偏移约定（全目录统一）**：本目录各篇的 `CShip` / `CShipType` 偏移一律是**元素相对**（`CShip*` / `CShipType*`，即池基址 + i×步长）。`vfunc_05/06/07` 经次基类 `this = 元素+8` 调用，反编译里的 `this+X` 要加 8 才是这里的 `+X`；主 vtbl getter（`+0x20/+0x30/+0x40/+0x48/+0x50/+0x58`）读的都是元素相对偏移。

### 类与设施

- `[确认]` 造船厂对象 `CShipyard` 只有构造 / 析构，尺寸 `0x10`，逻辑在未挂类的 `fn_*` 与甲板 View 里。来源：`game/town/CShipyard.cpp`。
- `[确认]` 选船 / 换船 / 甲板：`CSelectShipView`、`CShipChangeView`、`CDeckShipView`、`CShipArrangeView`。来源：`class_hierarchy.txt`。
- `[确认]` 海战用 `CBattleShip`（尺寸 `0x20`），不是 `CShip`。

## 3. 实体与 ID

| 实体 | 原版 | 原版计数 | 空 / 非法 | 重构 key |
| --- | --- | --- | --- | --- |
| 船模 | `CShipType` / `eShipType` | 39（可买 34）`[推断]` 循环上界 | 实例内 `0x28` | `ship_model_id` |
| 船体定义 | 静态表 `0x1405b42e0` | 107 `[确认]` 表长 | — | `hull_id`（原版 = 槽 − 100） |
| 船实例槽 | `eShip` | 207 `[确认]` 构造计数 | `0xD0`；头文件 `INVALID_SHIP=-1` | `ship_instance_id` |
| 体型档 | `eShipGrade` `[推断]` = 船模 `+0x18` | 3（三张按档表的表长） | 无船模时 4 | `models.size_class` |
| 船首像 | `eFigureHead` | 12 `[推断]`（道具 kind 5 共 12 件；`0x0D` 像「计数+1」哨兵） | `0x0D` | `figurehead_id`（`04`） |
| 玩家船位 | `eFleetShip` | 5 | `INVALID_FLEETSHIP=-1` | `fleet_ship_slot` |
| 货格 | `eShipLoad` | 5 | `INVALID_SHIPLOAD=-1` | `hold_slot` |
| 空货格商品 | — | — | `0x6A` | `trading_item.empty` |
| 停泊港空 | — | — | `0x62` | `dock_town.at_sea`（`07`） |

重构约定：

- `ship_model_id`、`hull_id` 是定义；`ship_instance_id` 是活船。系统之间只传这三个 ID，不传 `CShip*`。三者主键都是命名空间 key（`vanilla:ship_of_the_line`），`vanilla_index` 只供导入对照；运行时整数由加载器紧凑分配（D1）。
- 原版「预置船 = 槽 100+hull_id」是**存储布局**，不是规则。重构里预置船在开局时由 `hull_id` 生成实例，实例 ID 与 `hull_id` 解耦。
- 模组可以加船模 / 船体；运行时按表长循环，禁止写死 `39`、`100`、`107`、`207`。
- 运行时空值一律 `Option<Id>`（`null`）。原版 `0xD0`、`0x28`、`0x0D`、`7`、`0x6A`、`0x62`、`0xEC` 都是「计数+1 / 越界常数」，会和模组顺序分配撞号，只出现在 `ids/legacy_codec.yaml`（见 §6），导出原版格式时越界拒绝。

## 4. 算法 / 公式

全部参数进 `config/caps.yaml` / `config/formulas/ships.yaml`，代码里不准出现裸 `5`、`8`、`10`、`100`、`0x28`、`0xD0`，也不准 `== 字面量` 比改装件 / 舱室种类 / 船首像 id（D3）。

```
# 货格
usable_holds(ship) = min(Ships.cabin_count(ship, cargo_room), caps.hold_slots_per_ship)
# cargo_room 是 03 cabins.yaml 的 role（原版种类 10）；hold_slots_per_ship 原版 5

# 槽分段（原版布局，只在 legacy_codec 导入 / 导出存档时用）
# 原版：动态段 [8, 100)，模板段 [100, 207)；运行时实例池无分段、无上限
instance_valid(ship)  = ship.model != null                      # 原版 +0x51 != 0x28

# 新船初值（vfunc_05；预置船与新造船共用派生式）
durability_max(ship) = model.base_durability
                     + count(p in ship.parts where add_parts[p].armor) * formulas.armor_by_tier[model.size_class]
                     + Modifiers.eval("ship.durability_max", ship)     # 原版：船首像 3 白鲸像 +10，由 figureheads.yaml 注册
# armor_by_tier [6,8,10]；哪些改装件算装甲由 04 add_parts.yaml 的 armor: true 标出（原版位 2、3）
crew_init(ship) = (crew_required(ship) + crew_capacity(ship)) / 2
# 原版再截到 u8；重构默认不截（formulas.crew_init_truncate_u8: false 可切回原版）
# crew_required / crew_capacity 的展开在 06-crew-headcount

# 水手 / 耐久夹取
crew        = clamp(crew,        caps.ship_crew_min,        caps.ship_crew_max)
durability  = clamp(durability,  caps.ship_durability_min,  caps.ship_durability_max)
```

体型档由船模给（`model.size_class`），不再从耐久推。改装计价的商业发展度三档见 `04`。

`durability_max`、`crew_required` / `crew_capacity`、`usable_holds`、帆值（`06-ocean/01` §2）、炮门数（`08`）都是**派生量**：只由 Ships 按当前桅 / 改装件 / 舱室 / 船首像现算，放进 `Ships.derived()` 快照；Ocean / Combat / Economy 只读快照，不自己再算，不进存档（D6）。改装、舱室变更、换船首像只发事件触发重算。

`SHIP_CREW_*` / `SHIP_DURABILITY_*` 的原版十进制未进仓库，`caps.yaml` 先留符号，抽出后再填 default。

## 5. 耦合分析

原版硬编码：

- 贸易、委托、自动买卖都假设「5 船 × 5 格、商品不堆叠、空格 `0x6A`」。`CTrade` 直接扫 `CShip+0x58`，可用格数借舱室种类 10 的间数。货格系统和舱室系统在这里绑死。
- 非玩家势力不走真实货格，走 `CFleet+0x74` 一条「商品 + 数量」（`trade_stack.md`）。船体系统被舰队系统的紧凑货记绕开。
- 实例 ID 兼作定义 ID：槽 100–206 = 定义行 + 100，买船列表直接写 `type + 0x64`（`05`）。
- `CShip::vfunc_05` 初始化 31 个舱室槽，并把 `0x17` 改成 `0x18`——船体定义夹带舱室布局。
- 初始耐久 / 水手由舱室间数、改装位、船模体型档现算，三张表（`.data 0x1406876d0`、`.rdata 0x140596d98`）写死在 exe。
- 停泊港 `+0x54` 存在船实例上，船坞没有独立表（`07`）。
- 船名存在实例上（可在造船厂改名），定义名在 exe 静态表，World DLL 不管船。
- 海战用 `CBattleShip`（尺寸 `0x20`，空船同样 `0xD0`），与 `CShip` 不是同一对象。

重构后允许的依赖：`ships → characters.person_id`（本篇不写人物）；`ships → cabins.count_kind`（查询，不读布局）；`economy` **查询** `Ships.holds(instance_id)`，不拥有货格。

## 6. 解耦后的配置

```yaml
# config/caps.yaml（只摘本系统）
fleet_ships: 5
hold_slots_per_ship: 5
ship_instance_pool: null         # 原版 207；重构运行时无上限（null）。原版槽分段只在 legacy_codec
ship_crew_min: SHIP_CREW_MIN          # 抽出后替换
ship_crew_max: SHIP_CREW_MAX
ship_durability_min: SHIP_DURABILITY_MIN
ship_durability_max: SHIP_DURABILITY_MAX
```

```yaml
# config/ids/legacy_codec.yaml（只摘船系统；仅导入 / 导出原版存档用，运行时一律 null）
enums:
  ship_instance: { vanilla_count: 207, invalid: -1, empty: 0xD0 }   # 208
  ship_model:    { vanilla_count: 39,  invalid: -1, empty: 0x28 }   # 40；实例 +0x51
  gun_type:      { vanilla_count: 6,   invalid: -1, empty: 7 }      # 实例 +0x52
  figurehead:    { vanilla_count: 12,  invalid: -1, empty: 0x0D }   # 实例 +0x53；计数 [推断]
  cabin_kind:    { vanilla_count: 23,  invalid: -1, empty: 0x18, data_alias: { 0x17: 0x18 } }
fields:
  - { record: save.ship, offset: 0x50, enum: person,       width: u8 }   # 空 0xEC
  - { record: save.ship, offset: 0x54, enum: town,         width: u8 }   # 空 0x62 = 在海
  - { record: save.ship, offset: 0x58, stride: 8, count: 5, enum: trading_item, width: u8 }   # 空 0x6A
# 原版槽分段：动态 [8,100)、模板 [100,207)（模板槽 = vanilla_index(hull) + 100）
ship_slot_bands: { dynamic: [8, 100], template_base: 100 }
on_export_overflow: refuse
```

```yaml
# config/formulas/ships.yaml
schema: dk4.formulas.ships
version: 1
durability_max:                  # 新船初值（vfunc_05）；§4 durability_max(ship)
  impl: expr
  tables:
    armor_by_tier: [6, 8, 10]    # .data 0x1406876d0；按 models.size_class；装甲件见 04 add_parts.yaml armor: true
  inputs: [base_durability, armor_parts, size_class]   # armor_parts = 已装 armor: true 的改装件数
  expr: "base_durability + armor_parts * armor_by_tier[size_class]"
  modifiers: ship.durability_max
  # 船首像 3 的 +10 不在这里：figureheads.yaml effects → ship.durability_max
crew_init:                       # crew_init: average = (required + capacity) / 2
  impl: expr
  quirks: { crew_init_truncate_u8: false }   # true = 原版行为（quirk）
  inputs: [crew_required, crew_capacity]
  expr: "(crew_required + crew_capacity) / 2"
```

```yaml
# config/ships/hulls.yaml（05 拥有 schema 与逐行；本篇只列实例化读的列）
schema: dk4.ships.hulls
version: 1
hulls:
  - key: vanilla:small_hansa
    vanilla_index: 0               # 原版模板槽 = 100 + vanilla_index，只在 legacy_codec 用
    model: vanilla:small_hansa     # ship_model key
    default_masts: [0, 3, 0]       # 定义 +0x0C
    default_parts: []              # 定义 +0x0A 位图展开成 add_part key 列表
    default_cabins: [...]          # 定义 +0x0F，31 槽，值是 cabin_kind key（03）
    default_gun_type: vanilla:saker   # 定义 +0x2E
    bound_person: null             # 定义 +0x08 → 实例 +0x50 舰长；原版 0xEC
    role: yard_template            # 05：yard_template | variant | named | encounter
```

船模表与 107 行船体由 exe 导出，World0 / World1 **共用**同一份；两个 World DLL 都不带船表。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗改代码（它们只读 caps 与查询接口）：

- 船模行数、船体行数（模组加行即可）
- `hold_slots_per_ship`；哪种舱室算货舱（`03` 的 `role: cargo_room`）
- `fleet_ships`、实例池大小（原版动态段起止只影响存档导入导出）
- 水手 / 耐久上下限（须 ≤ `engine_limits` 的 u16 硬顶）
- 装甲加算表、状态加成、初始水手取法
- 船名长度（原版 19 字节 GBK；显示层自己裁）

显示层不得用「价格列放得下几位」倒逼货格规则。

## 8. 对外接口

事件：

- `ShipCreated { ship_instance_id, hull_id, ship_model_id }`
- `ShipScrapped { ship_instance_id }`
- `ShipRenamed { ship_instance_id }`
- `HoldsChanged { ship_instance_id }`
- `DurabilityChanged { ship_instance_id, durability }`
- `CrewChanged { ship_instance_id, crew }`

查询（只返回 ID 与标量）：

- `Ships.model(ship_model_id) -> ShipModelDef`
- `Ships.hull(hull_id) -> HullDef`
- `Ships.instance(ship_instance_id) -> { hull_id?, ship_model_id, name, durability, crew, gun_type_id?, figurehead_id?, captain_person_id?, add_parts, dock_town_id? }`
- `Ships.derived(ship_instance_id) -> { durability_max, crew_required, crew_capacity, usable_holds, sail{square, fore, square_extra, fore_extra}, rower_rooms, doors }`  （D6 唯一计算方；不存档）
- `Ships.cabin_count(ship_instance_id, role) -> int`  （定义在 `03`）
- `Ships.holds(ship_instance_id) -> [{ slot, trading_item_id, unit_price }]`
- `Ships.usableHolds(ship_instance_id) -> int`  （= `min(货舱间数, hold_slots_per_ship)`）
- `Ships.durabilityMax(ship_instance_id) -> int`  （= `derived().durability_max`）
- `Ships.sizeClass(ship_instance_id) -> int`

命令：

- `Ships.create(hull_id | ship_model_id) -> ship_instance_id`  （总是新建实例；不复用模板实例）
- `Ships.setHold(instance_id, slot, trading_item_id, unit_price)`
- `Ships.clearHold(instance_id, slot)`  （写成 `trading_item.empty`）
- `Ships.rename(instance_id, name)`

`CTrade` 开市 / 关市改为：借 `holds()` 快照，提交时发 `Ships.setHold`。禁止贸易模块持有 `CShip*` 或自己数舱室。

## 9. 未核实字段

必须单列，禁止猜成确定语义。

### 船体定义表（`0x1405b42e0`，107×0x30）

已用字段：`+0x00` 名、`+0x08` 人物、`+0x09` 船模、`+0x0A` u16 改装位、`+0x0C` 3 桅、`+0x0F` 31 舱室、`+0x2E` 炮种。`+0x2F` 未读 `[未知]`。

### `CShip` 未命名

- `+0x46`、`+0x48` 两个 u16（初值 0，存档）
- `+0x53` 船首像：2 虎鲸的效果未找到代码（6 处女 / 9 圣母见 `04` 表）；写入点未穷尽；`0x0C` 是否合法值
- 货格记录 `+1`（`fn_1400ff280(1)` 的结果）、`+2`（空 `0x62`）
- `+0x4E` 位 15（getter 清掉）

### `CShipType` 未命名

`+0x48` 地域位图是唯一存档字节：谁在运行时改它 `[未知]`。

### 计数未还原

- `NUM_SHIP` 是 100 还是 207；`NUM_SHIPTYPE` 是否 39；`NUM_SHIPGRADE`；`NUM_FIGUREHEAD` 是否 12
- 槽 0–7 不参与分配的原因
- `CShipElement+0x6788` 的 8 个 `0xD0`
- 6 条回绕模板（见 §2「耐久与水手」）开局后是否被别的函数重写水手
