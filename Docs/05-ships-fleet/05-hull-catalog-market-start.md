# 船体目录、船模、造船厂与开局船

舱室 / 帆 / 炮 / 装甲 / 船首像见 `03-cabins-crew-posts.md`、`04-remodel-guns-supplies.md`。舰队槽与地方舰队见 `02-fleet-proxy.md`。船体实例布局见 `01-ship-model.md`。本篇只补：**107 条船体定义名录**、**船模（`eShipType`，39）**、**造船厂买卖**、**剧本初始船与事件赠船**。

术语（全目录统一）：`hull`（船体定义，107 行，exe `0x1405b42e0`）、`ship_model`（船模 = `CShipType` = `eShipType`，39 行）、`ship_instance`（`eShip` 是 207 个**实例槽**的下标，不是目录下标）。偏移一律按 `01` 的元素相对写法。

exe 里**没有**「轻帆船」「轻快帆船」「盖伦」「克拉克」。SC 船型名是「小型汉萨式帆船」「战列舰」这类，从 `Game/DK4HD_sc.exe` `.rdata` 抽出。禁止用常识译名填回原版包。

## 1. 职责边界

本系统管：

- 船体定义 107 行（名字、默认舱室、默认桅、默认改装件、默认炮种、绑定舰长）
- 船模 `eShipType` = `CShipType` 0–38（贴图、基础耐久、表价、体型档、上架门槛、海域位图）
- 造船厂上架、买、改名、放船坞；卖船入口
- 剧本开局船与事件赠船（`GiveShip`）相对造船厂买船（`BuyShip`）

本系统不管：

- 舱室种类与人事绑定（`03`）
- 改装件、火炮、补给、修理差价的细项（`04`；买价会**调用**那套加算）
- 舰队 5 槽、委任、阵型（`02`）
- 海战伤害（`07-combat`）
- 事件 opcode 译码（`10-narrative`；本篇只规定命令 `GiveShip`）

## 2. 原版事实

### 三套计数不要混

| 层 | 原版 | 计数 | 是什么 |
| --- | --- | --- | --- |
| 船体定义 | （无独立枚举）| 107 `[确认]` 表长 | exe 表 107 条。`eShip` 是实例槽下标（`01`）：头文件 `NUM_SHIP=100` 是 RTC 投票，100 只是动态段 / 模板段分界，不是目录长 |
| 船种枚举 | `eShipType` | `[推断] 39` | **就是船模下标**。RTC 局部 `eShipType` 的循环上界全是 `0x27`（`fn_14000d9a9`、`fn_140006cb7`、`fn_14000c8fb`）或 `0x22`（`fn_1400f41e0`）。`ShipTypeID.h` 的 `NUM_SHIPTYPE=5` 是 `tools/fill_gaps.py` 按「cmp 立即数众数」猜的（`num_from_rtc.json`：`eShipType` 5×5、97×4、39×3、34×2），**不可信** |
| 普通船种 | `eNORMALSHIPTYPE` | `[推断] 34` | 断言 `eShipType < NUM_NORMALSHIPTYPE`（World DLL 字符串，无代码引用）。`StlShip.dk4` 34 张；`fn_140011d2e` `cmp type, 0x22`；NPC 选船 `fn_1400f41e0` `rand(0x22)` |
| 船模对象池 | `CShipType` ×39 | 39（`0x27`） | 管理器（RTTI 名 `CShipTypeElement`）`0x1402a67d0` 用 vector ctor 建 39 个 `CShipType`（`0x50`）。34 艘可买船模 + 4 海怪 + 1 幽灵船 |
| 玩家事件船 | `ePLAYEREVENTSHIP` | `[未知]` | `PlayereventshipID.h` 只有符号。World0.dll UTF-16 断言有 `NUM_PLAYEREVENTSHIP`，但 DLL 代码不引用该串，整数无处可读 |

来源：`srcdata/source/ShipID.h`、`ShipTypeID.h`、`NormalshiptypeID.h`、`PlayereventshipID.h`、`catalog/num_values.json`、`catalog/num_from_rtc.json`、`tools/fill_gaps.py:280-298`、`CShipTypeElement.cpp`、`CShipSurfaceManager.cpp`（同样循环 `>= 0x27`）、`Game/StlShip.dk4`。

`0x28`（40）是「空槽船模」哨兵：`CShip::vfunc_05` 给 id < 100 的槽写元素 `+0x51=0x28`，`fn_140006cb7` 有 `cmp type, 0x28`。不是 `INVALID_SHIPTYPE`；重构只在 `ids/legacy_codec.yaml`。

汇编（`01` 已按此更正）：

```
; CShip::vfunc_05  0x1402adee0
call fn_1400026e9          ; 槽下标 = (this - pool) >> 7，sizeof(CShip)=0x80
cmp eax, 0x64
jge init_from_table        ; id >= 100 → 目录 (id-100)
mov [this+0x49], 0x28      ; id < 100 → 空实例（元素 +0x51）
```

`fn_140001eab`（`0x140230430`）：`sar rax, 7`。来源：反汇编，不是反编译的颠倒 `jge`。

`vfunc_05` 的 `this` 是池元素 `+8`（次基类；主 vtbl `0x14053e920`，次 vtbl `0x14053e9d8`）。下文「拷到」列已换成**元素相对**（`CShip*`，与 `01` 一致）。

因此 207 个 `CShip` 槽（`0xCF`）的用法 `[确认]`：

| 槽 | id | 初始化 |
| --- | --- | --- |
| 0–99 | < 100 | 空（元素 `+0x51=0x28`），玩家活船从 8–99 分配 |
| 100–193 | 100–193 | 船体目录 0–93（通用船体；NPC 随机挑变体的池，见下） |
| 194–199 | 194–199 | 目录 94–99（主角 0–4 的具名船） |
| 200–206 | 200–206 | 目录 **100–106**。exe 表在 99 之后还有 7 条，`0x1405b55a0` 起；`0x1405b56f0`（第 107 条）起全 0 |
| 208 | `0xD0` | 空船哨兵（在 207 外侧） |

`[确认]` 目录 100–106（`eShip` 之外，exe 同表续写）：

| 目录 | 槽 | name | 舰长 `+0x08` | type_id | 改装位图 `+0x0A` | 炮种 `+0x2E` |
| --- | --- | --- | --- | --- | --- | --- |
| 100 | 200 | 安娜希莉雅 | 5（伍丁） | 26 | 51 | 1 |
| 101 | 201 | 拉乌卡 | 6（蒂雅） | 3 | 0 | 1 |
| 102 | 202 | 怪鱼 | 197 | 34 | 0 | 0 |
| 103 | 203 | 大墨鱼 | 197 | 35 | 0 | 0 |
| 104 | 204 | 鲨鱼 | 197 | 36 | 0 | 0 |
| 105 | 205 | 白鲸 | 197 | 37 | 0 | 0 |
| 106 | 206 | 幽灵船 | 197 | 38 | 241 | 1 |

舰队模板 62（两包同）首字节 `0xC5`=197、船 `0xCA`=202 → 怪鱼。海怪 / 幽灵船遭遇就是挂这些槽。

`[确认]` NPC 选船变体：`fn_1400f4080`（body 同址）扫槽 `0x64..0xC1`（100–193），收集 `type == 参数` 的槽，`rand(n)` 取一个；没有则 `0xD0`。具名船 94–106 不进这个池。调用者 `fn_1400f7c10.cpp:1374`。

造船厂买列表写 `eShipType + 0x64`（`fn_14000d9a9`），就是槽 100+type = 该船模的模板实例。

### 船体定义表（107 × 0x30）

`fn_14000cff9`（`0x1402ac5e0`）：

```
eax = id - 0x64
rax *= 0x30
return &table[rax]
```

表 VA `0x1405b42e0`（`lea rcx, [rip+0x307ce7]`），文件偏移 `0x5b30e0`，步长 **48**。名称指针在 `+0x00`，GBK C 字符串在邻近 `.rdata`。`CShip::vfunc_05` `lstrcpyA(ship+8, *def)`。

| 偏移 | 宽 | 拷到 `CShip*`（元素相对） | 含义 |
| --- | --- | --- | --- |
| `+0x00` | ptr | `+0x10` 起 19 B 名 | 目录名 |
| `+0x08` | u8 | `+0x50` | 默认舰长；`0xEC` 空；94–97、99 为主角 0–4；100/101 为 5/6；海怪 197 |
| `+0x09` | u8 | `+0x51` | `CShipType` 下标（船模） |
| `+0x0A` | u16 | `+0x4E` | **默认改装件位图** `[确认]`：`CShip` vtbl+0x30（`0x1400a77c0`）读元素 `+0x4E` 并 `btr 15`；`fn_1400091e2(ship, part)` 测 `1<<part`。船体 0–33 与船模 `+0x14` 同值。商用常 17/51/241，武装常 196/212/244/246 |
| `+0x0C` | 3 B | `+0x23` | 三根桅默认 |
| `+0x0F` | 31 B | `+0x26` | 默认舱室种类（`0x17`→`0x18`） |
| `+0x2E` | u8 | `+0x52` | **默认炮种** `eGunType` `[确认]`：`CShip` 主 vtbl+0x40（`0x1400a7bd0`）读元素 `+0x52`，`fn_140013593` / `fn_14000c6cb` 把它当炮种查 `0x140682e10` 炮表。出现 0–4（霰弹…重加农）。旧稿「船体等级」作废 |
| `+0x2F` | u8 | — | 抽出全 0 |

100 个名字（加 100–106 共 107 个）全部在 exe 里，**无 null**。同名两条：id 16 与 18 共用指针 `0x1405b5b28`「北海三桅三角帆船」（桅布局不同）。

| id | name | type_id | 默认炮种 | 备注 |
| --- | --- | --- | --- | --- |
| 0 | 小型汉萨式帆船 | 0 | 0 | 船坞可买模板 |
| 1 | 小型三角帆船 | 1 | 0 | |
| 2 | 小型方形帆船 | 2 | 0 | |
| 3 | 中型双桅三角帆船 | 3 | 1 | |
| 4 | 宽大型三桅方形帆船 | 4 | 1 | |
| 5 | 中型西班牙方形帆船 | 5 | 2 | |
| 6 | 大型西班牙方形帆船 | 6 | 3 | |
| 7 | 中型北海三桅方帆船 | 7 | 2 | |
| 8 | 大型北海三桅方帆船 | 8 | 3 | |
| 9 | 小型英国方形帆船 | 9 | 1 | |
| 10 | 英国双桅三角帆船 | 10 | 2 | |
| 11 | 中型大桅横帆船 | 11 | 2 | |
| 12 | 中型大桅方形帆船 | 12 | 2 | |
| 13 | 小型护航艇 | 13 | 1 | |
| 14 | 护航舰 | 14 | 2 | |
| 15 | 战列舰 | 15 | 4 | |
| 16 | 北海三桅三角帆船 | 16 | 4 | 与 18 同名 |
| 17 | 北海三桅方形横帆船 | 17 | 4 | |
| 18 | 北海三桅三角帆船 | 18 | 4 | 与 16 同名 |
| 19 | 北海多桅方形横帆船 | 19 | 4 | |
| 20 | 小型桨帆并用船 | 20 | 1 | |
| 21 | 中型桨帆并用船 | 21 | 2 | |
| 22 | 大型桨帆并用船 | 22 | 3 | |
| 23 | 巨型桨帆并用船 | 23 | 4 | |
| 24 | 小型阿拉伯船 | 24 | 0 | |
| 25 | 中型阿拉伯快船 | 25 | 1 | 船模名「中型阿拉伯轻快船」 |
| 26 | 巨型阿拉伯船 | 26 | 2 | |
| 27 | 小型中国帆船 | 27 | 0 | |
| 28 | 中国帆船 | 28 | 1 | |
| 29 | 大型中国帆船 | 29 | 3 | |
| 30 | 小型日本桨船 | 30 | 0 | |
| 31 | 中型日本桨船 | 31 | 1 | |
| 32 | 大型日本桨帆并用船 | 32 | 2 | |
| 33 | 铁甲船 | 33 | 4 | |
| 34 | 商用三桅方形帆宽船 | 4 | 1 | 商用变体，非船坞 id |
| 35 | 西班牙方形帆商船 | 5 | 2 | |
| 36 | 西班牙方形帆大商船 | 6 | 3 | |
| 37 | 北海三桅方帆大商船 | 8 | 3 | |
| 38 | 中型日本商用桨船 | 31 | 1 | |
| 39 | 大型日本商用桨帆船 | 32 | 2 | |
| 40 | 中型双桅方形帆商船 | 12 | 2 | |
| 41 | 中型双桅横方帆商船 | 11 | 2 | |
| 42 | 英国双桅三角帆商船 | 10 | 2 | |
| 43 | 小型日本武装桨船 | 30 | 0 | 武装变体 |
| 44 | 中型日本武装桨船 | 31 | 1 | |
| 45 | 大型日本武装桨帆船 | 32 | 2 | |
| 46 | 军用铁甲船 | 33 | 4 | |
| 47 | 北海三桅方形帆商船 | 7 | 2 | |
| 48 | 中型双桅三角帆商船 | 3 | 1 | |
| 49 | 运输合约用护航艇 | 13 | 1 | |
| 50 | 运输合约用护航舰 | 14 | 2 | |
| 51 | 运输合约用战列舰 | 15 | 4 | |
| 52 | 北海多桅三角帆商船 | 16 | 4 | |
| 53 | 北海三桅大方帆商船 | 17 | 4 | |
| 54 | 北海三桅三角帆商船 | 18 | 4 | |
| 55 | 北海多桅大方帆商船 | 19 | 4 | |
| 56 | 中型运输用桨帆船 | 21 | 2 | |
| 57 | 大型运输用桨帆船 | 22 | 3 | |
| 58 | 巨型运输用桨帆船 | 23 | 4 | |
| 59 | 中型阿拉伯轻快商船 | 25 | 1 | 伍丁开局用 |
| 60 | 阿拉伯三角帆商船 | 26 | 2 | |
| 61 | 中国商用帆船 | 28 | 1 | |
| 62 | 大型中国商用帆船 | 29 | 3 | |
| 63 | 运输用铁甲船 | 33 | 4 | |
| 64 | 小型汉萨式武装帆船 | 0 | 0 | 武装 0–33 |
| 65 | 小型三角帆武装帆船 | 1 | 0 | |
| 66 | 小型方形帆武装帆船 | 2 | 0 | |
| 67 | 双桅三角帆武装帆船 | 3 | 1 | |
| 68 | 三桅方形帆武装帆船 | 4 | 1 | |
| 69 | 中型西班牙方帆军船 | 5 | 2 | |
| 70 | 大型西班牙方帆军船 | 6 | 3 | |
| 71 | 中型北海三桅武装船 | 7 | 2 | |
| 72 | 大型北海三桅武装船 | 8 | 3 | |
| 73 | 小型英国方帆武装船 | 9 | 1 | |
| 74 | 英双桅三角帆武装船 | 10 | 2 | |
| 75 | 双桅横方形帆武装船 | 11 | 2 | |
| 76 | 双桅方形帆武装船 | 12 | 2 | |
| 77 | 小型军用护航艇 | 13 | 1 | |
| 78 | 军用护航舰 | 14 | 2 | |
| 79 | 军用战列舰 | 15 | 4 | |
| 80 | 多桅三角帆武装船 | 16 | 4 | |
| 81 | 北海三桅横帆武装船 | 17 | 4 | |
| 82 | 三桅三角帆武装船 | 18 | 4 | |
| 83 | 北海多桅横帆武装船 | 19 | 4 | |
| 84 | 小型桨帆并用武装船 | 20 | 1 | |
| 85 | 中型桨帆并用武装船 | 21 | 2 | |
| 86 | 大型桨帆并用武装船 | 22 | 3 | |
| 87 | 巨型军用桨帆并用船 | 23 | 4 | |
| 88 | 小型阿拉伯武装船 | 24 | 0 | |
| 89 | 阿拉伯武装轻快船 | 25 | 1 | 伍丁开局用 |
| 90 | 阿拉伯三角帆武装船 | 26 | 2 | 伍丁开局用 |
| 91 | 小型中国武装帆船 | 27 | 0 | |
| 92 | 中国武装帆船 | 28 | 1 | |
| 93 | 大型中国武装帆船 | 29 | 3 | |
| 94 | 卡鲁提拉 | 1 | 1 | 拉斐尔；`person=0` |
| 95 | 维斯泰洛斯 | 11 | 1 | 赫德拉姆；`person=1` |
| 96 | 阿伦海姆 | 1 | 0 | 丽璐；`person=2` |
| 97 | 春申 | 28 | 1 | 华梅；`person=3` |
| 98 | 华昌 | 28 | 1 | 华梅第二艘；`person=0xEC` |
| 99 | 埃斯佩兰莎 | 3 | 3 | 杏太郎；`person=4` |

目录分段 `[确认 范围，名称分组]`：0–33 船坞模板（与船模 0–33 同名或极近）；34–63 商用 / 运输合约 / 一部分武装（43–46 已是日本武装与军用铁甲）；64–93 武装变体（type_id 0–29）；94–99 具名开局船；100–101 伍丁 / 蒂雅具名船；102–106 海怪与幽灵船。不要把 34–63 写成「全是商用」。变体与模板的真正差别在 `+0x0A` 改装位图和舱室，名称只是标签。

### 船模表（39 × 0x40）

`fn_14001372d`（`0x1402ac610`）：`id * 0x40 + table`。表 VA `0x1405b3460`。`CShipType::vfunc_05`（`0x1402ae1f0`）填充运行时对象（元素 `0x50`）。

`vfunc_05` 在**次 vtbl**（`0x1405af0d0`，元素 `+8`）上，`this = 元素+8`；索引由 `fn_140013f20` 算 `(this-8 - (mgr+8)) / 0x50`。`fn_14000affb(type)` 返回的是**元素基址**。所以下表给两列：`this` 相对（vfunc_05 写入处）与元素相对（其他 getter 读取处）。来源：反汇编 `0x1402ae1f0`、`0x140240330`、`0x1400bc2d0`、ctor `0x1402a6750`。

| 偏移 | 宽 | this / 元素 | 含义 |
| --- | --- | --- | --- |
| `+0x00` | ptr | `+0x08` / `+0x10` | 名称 |
| `+0x08` | u8 | `+0x10` / `+0x18` | 体型档 0/1/2（= `eShipGrade` `[推断]`；`fn_14000d152` 取，无船模时 4）`[确认 取值]`。上架谓词 `fn_14000e093`、装甲 / 帆罚 / 必要水手三张按档表都读它 |
| `+0x0A` | u16 | `+0x12` / `+0x1A` | **基础耐久** `[推断]` 强（25–100；海怪 500–999、幽灵 400）：`fn_14000a2ae` 读元素 `+0x1A`，进最大耐久 `fn_1400084db`（`01`）。`CShip::vfunc_05` 写元素 `+0x4A` 的是 `fn_1400084db` 的结果（`0x1402ae127..0x1402ae13f`），不是直接拷贝。旧稿「水手定额」作废 |
| `+0x0C` | u32 | `+0x14` / `+0x1C` | **表价** 1200–160000；海怪 / 幽灵为 0。`[确认]` 主 vtbl+0x20（`0x1402a87d0`，`mov eax,[rcx+0x1c]`）返回它，**买价就从它起算**（见「买价」） |
| `+0x10` | 3 B | `+0x18` / `+0x20` | 默认桅（位域：bit0 有桅，bit2 / bit3 计价） |
| `+0x14` | u16 | `+0x1C` / `+0x24` | **默认改装件位图** `[确认]`：主 vtbl+0x30（`0x1402a78a0`）返回它。战列舰 66、铁甲 196、幽灵船 241 |
| `+0x16` | 31 B | `+0x1E` / `+0x26` | 默认舱室（`0x17`→`0x18`） |
| `+0x35` | u8 | `+0x3D` / `+0x45` | **默认炮种** `[确认]`：主 vtbl+0x40（`0x1402a7c90`）读元素 `+0x45`。与船体 0–33 `+0x2E` 同值 |
| `+0x36` | u16 | `+0x3E` / `+0x46` | **上架发展门槛** `[确认]`（100–3000）：`fn_140002e9b` 读元素 `+0x46`，`fn_14000d9a9` 比 `门槛 > 阈值 → 不上架` |
| `+0x38` | u8 | `+0x40` / `+0x48` | **七大海域位图** `[确认]`：`fn_1400013bb(model, area)` = `byte & (1<<area)`；`area = fn_140014e48(town)` = 港口 `+0x25` 文化圈 → `CCulture+0x10` `major_sea_id`（`fn_140013a02` → `fn_14000aa0b`，文化 `0x16` 时取 0）。海怪 / 幽灵为 0，所以永不上架。也是 `CShipType` 唯一存档字段（`01`）→ 运行时可变 `[推断]` |
| `+0x39` | 7 B | — | 抽出全 0 |

船模 0–33 名称与船体 0–33 对应（个别用词不同，如船模 4「大型三桅方帆宽身船」vs 船体 4「宽大型三桅方形帆船」；船模 11「中型大桅横方形帆船」；船模 16–19「北海多桅三角帆大船 / 北海三桅横方帆大船 / 北海三桅三角帆大船 / 北海多桅横方帆大船」）。**34–38 在船体 0–99 里没有同号**，但在目录 102–106 有对应行：

| type_id | name | table_u32 | 基础耐久 `+0x0A` | 目录 |
| --- | --- | --- | --- | --- |
| 34 | 怪鱼 | 0 | 800 | 102 |
| 35 | 大墨鱼 | 0 | 600 | 103 |
| 36 | 鲨鱼 | 0 | 500 | 104 |
| 37 | 白鲸 | 0 | 999 | 105 |
| 38 | 幽灵船 | 0 | 400 | 106 |

`[推断]` `eShipType` 就是这 39 个船模的枚举（证据类型：循环上界 `0x27`；见上文计数表）；`NUM_SHIPTYPE=5` 是投票误猜，不存在「5 个粗船种」。按 type_id 粗分欧式帆 / 桨帆 / 阿拉伯 / 中国 / 日本是 `[推断 分组]`，没有单独字段；最接近的数据是 `+0x38` 海域位图（只用到 bit0–6 = `major_sea_id` 0–6；阿拉伯型多含 bit3，中日型多为 bit5 东亚。各位海域名以 `02-factions` 的七大海域注册为准）。

### 船模资源

全部 `GT1G4600`（`formats.md`）。

| 文件 | 张数 | 用法 |
| --- | --- | --- |
| `Game/StlShip.dk4` | **34** | 静画。`fn_140011d2e`（body `0x14023ff70`）：`type < 0x22` → `fn_140010e6f(..., "StlShip.dk4", type)`；`>= 0x22` 只建同类精灵并 `fn_14000d5e9(obj,0,0)`，**不加载任何 dk4**（`[推断]` 空图）。rdata 里 `StlShip.dk4` 紧挨 `eShipType` |
| `Game/ship.dk4` | **72** | 海图 / 海战船图。`CShipSurfaceManager` 为 39 个 type 各挂表面；72 与 39 的下标式未钉死。`eCloseCombatTypeToIndex` 也在加载此文件（见 `07-combat/02`） |
| `Game/DeckShip.dk4` | **17** | 甲板俯视船身。与 34/39 都对不上，按体型 / 等级切分 `[未知]` |
| `Game/DeckScrn.dk4` | **50** | 甲板 UI（舱室按钮），不是船体目录 |

映射规则 `[确认]`：`type_id < 34` → `StlShip.dk4` 下标 = `type_id`；`>= 34` 无静画。张数由 G1T 头 `+0x10` 计数读出（34 / 72 / 17 / 50）。船体的 `model_asset` 跟 `type_id`，不跟 `hull_id`（商用 / 武装变体共用船模）。

### 造船厂

`CShipyard` 只有构造 / 析构，尺寸 `0x10`，没有买卖逻辑。来源：`game/town/CShipyard.cpp`。

实际会话在 `fn_140004fd9`（`0x1401765f0`）菜单：

| 选项 | 函数 | 角色 |
| --- | --- | --- |
| 0 | `fn_1400068c0`（`0x140176320`） | 买船坞上架的船模 |
| 1 | `fn_140007531`（`0x140175f60`） | 买「与已改造好的船完全一样的船」 |

上架 `[确认]`：`fn_14000d9a9`（`0x1402613e0`，`fn_14000d9a9(town, out[], size_filter)`）。反汇编：

```
area      = fn_140014e48(town)                       # 港口文化圈的 major_sea_id（0–6）
commerce  = fn_14000bc03(town, 0)                    # town+0x1E
military  = fn_14000bc03(town, 1)                    # town+0x20
threshold = (commerce + military / 2) / 5            # 两次都是有符号除（cdq/sar、idiv 5），向零截断
          + 30 * (calendar_packed / 17520)           # fn_140002068：年；imul 0x1e
for type in 0 .. 0x26:
    m = fn_14000affb(type)                           # 元素基址；不会为空
    if not (m.area_mask & (1 << area)): continue     # fn_1400013bb，元素 +0x48
    if m.develop_need > threshold: continue          # fn_140002e9b，元素 +0x46；jg 有符号
    if size_filter != 4 and m.size_class != size_filter: continue   # fn_14000e093，元素 +0x18
    out[n++] = type + 100
return n
```

四个调用点（`fn_1400068c0`、`fn_1400086b6`、`fn_1400f7c10.cpp:1618`、`fn_1400e7660.cpp:1061`）都传 `size_filter = 4`，原版体型过滤从未生效。年项让门槛逐年放宽：开局一年后每座港多 30 点。`calendar_packed` 的纪元见 `01-world/07-calendar-and-coords.md`（纪元年 `[未知]`，若打包从 0 起则年项初值 0）。`17520 = 365×48`：日历一天 48 拍（D4）。

`aeBuyShip` / `aeMyShip` / `aeDockShip` / `aeShipTable` RTC 都是 **828 字节 = 207×4**，下标是船实例槽。

选船 UI：`CSelectShipView`（尺寸 `0x290`）、`CSelectShipScrollView`、`CSelectShipFrameView`、`CSelectShipDrawLine`（画静画，调 `StlShip.dk4`）。换船：`CShipChangeView`（尺寸 `0x758`）把 5 个舰队槽填 `0xD0`，再扫 `eShip` 0–99 的 100 个标志。

文案（`mesfile_sc.txt`）：

| 偏移 | 文本 |
| --- | --- |
| `0x2e4f` | 持有的船过多了！这个城市没有可以制造的船！ |
| `0x2e7b` | 钱不够！购买这艘船吗？ |
| `0x2e93` | 请给船取个名字！不能再把船处置掉了！还有剩余货物，这样可以吗？ |
| `0x7df7` | 一只舰队最多可以有五艘船，新船放在船坞里。 |
| `0x7ea3` | 购买与已改造好的船完全一样的船？好了，那就先存放在船坞吧。 |
| `0x7edf` | 卖哪艘船？ |
| `0x8053` | …和%s是同一规格吗？准备就需要%s天的时间，买吗？（同规格路径 `0x3b4`） |
| `0x30a3` | 将各艘船放在船坞里。 |
| `0x343f` | 把拖来的船停泊在船坞里。 |

`fn_14000de13`（`0x140178a20`）RTC `szShipName` 19 字节、`aeDummyShip`：从模板拷名 → 输入（上限 `0x12`）→ 空名消息 `0x102` → 舰队未满则进 5 槽，否则消息 `0x3a3` 进船坞。

mesfile 里「赠」命中是广场免费样品，**不是**送船。送船走事件脚本，opcode 未译码（`02-event-system.md`）。

### 买价、卖价与天数

`CShip` 主 vtbl+0x20 = `vfunc_04` body `0x140230140`（RTC `eShipMast`），下称 `outfit_value`。反汇编：

```
value  = model.vtbl20() / 2               # fn_140005321 取船模元素；vtbl+0x20 = 0x1402a87d0 读元素 +0x1C
                                          #   = 定义 +0x0C 的 u32 表价；div 2 无符号
for mast in 0..2:                         # fn_140007ce3 / fn_14000a970：mast 字节 bit2、bit3 各算一次
    value += 100 * popcount(mast_byte & 0b1100)
for part in 0..8:                         # fn_1400091e2：改装位图 1<<part
    if installed: value += part_fee[part] # 跳表 0x1402302ac
value += fn_14000623f(ship)               # = fn_140013593(gun_type, count(cabin kind 8 武装炮台), half=1)
                                          #   炮价 / 2 = 价基 × 3^武装炮台间数 × 20 / 2，见 08
```

`part_fee` `[确认]` = `[500, 400, 1000, 1000, 1000, 1000, 400, 400, 400]`（part 0 → `0x1F4`；1、6、7、8 → `0x190`；2–5 → `0x3E8`）。

买（`fn_1400068c0` 与 `fn_140007531` 同式）`[确认]`：

```
price = outfit_value(template_or_source_ship) << 1
price = fn_14000643d(town, price)          # 港口景气状态修正，见下
if gold < price: 消息 0x100 / 0x5c0
```

所以对未改装的模板槽，买价 = `表价 + 2×(桅 + 默认改装件) + 炮价`（再乘港口倍率）。例：战列舰模板 = `160000 + 2×(500 + 800) + 200×27×20 = 270600`（桅 `[15,15,5]` 共 5 个计价位；改装 66 = part 1、6；重加农、3 间武装炮台）。旧稿把炮价项当「货格差价」，也已更正。旧稿说「买价不读 `+0x0C` 表价」是把 this / 元素偏移错位所致，**已更正**：vtbl+0x20 读的就是表价，`+0x14` u16 走的是 vtbl+0x30（改装位图）。

港口景气修正 `fn_14000643d`（body `0x140261a60`）`[确认]`，`state = fn_14001244f(town)` = `town+0x74` 字节（economy 的港口景气状态，`04-economy/02` §2.3；旧稿称「港口类别 eTownKind」）。买船、卖船、改装三处共用（改装调用点 `fn_1400a1140.cpp:2285`，`04`），重构只留一张表：economy 的 `economy/town_state_price_rules.yaml` `contexts` 段（`04-economy/02` §6），按 context 取；状态 key 0 = `war`、3 = `boom`、4 = `slump`：

| 状态 | 买价（`ship_buy`） | 卖价（`ship_sell`，`fn_14000bb95` 内联） |
| --- | --- | --- |
| 0 `war` | `×2`（`shl 1`） | 不变 |
| 3 `boom` | `×12/10`（`imul 0xC; div 0xA`，无符号） | `×12/10` |
| 4 `slump` | `×8/10`（`shl 3; div 0xA`） | `×8/10` |
| 其他 | 不变 | 不变 |

改装价原版走买价同一函数（`fn_14000643d`）；重构单列 `context: remodel`，原版值同 `ship_buy`（词表 §14.14）。

卖 `[确认]`：`fn_14000bb95`（body `0x140177f60`，RTC `cSelectShipPalam`、`aeShipTable`）。列表 `fn_140177030`，空则消息 `0x103`；提示 `0x3a4`「卖哪艘船？」→ `price = outfit_value(ship)`（**不左移**）→ 上表卖价修正 → 确认 `0x3a5` → `fn_140002cb6` 加金 → `fn_1401767e0` 把船从舰队 / 船坞摘掉（货物处理在其中）。普通港卖价 = 买价的一半；kind 0 港买价再翻倍而卖价不翻。

天数：

- 买同规格改造船：`fn_14000c63f`（body `0x140176c70`）比的是**所选己船 vs 该船模默认**（`fn_14000affb(ship.type)`），不是新旧两船：
  - 桅 0..2：己船该桅存在（bit0）且整字节 ≠ 船模 → `+2` 并停止（只加一次）
  - 改装件 0..8：己船有、船模默认没有 → 每件 `+1`
  - 舱室 0..30：己船舱室种类在 `[7,12)` 且 ≠ 船模 → 置标志；在 `[12,0x15)` 且不同 → 另一标志。每个标志各 `+2`（整船最多 +4）
  - 炮种（vtbl+0x40）与船模默认炮种不同 → `+1`
  - 结果 > 0：消息 `0x3b4`「和%s是同一规格吗？准备就需要%s天的时间，买吗？」；= 0：`0x3b5`。确认后扣钱、`fn_14000b122(player, days)` 推进 `days×48` 拍。
- 买目录船 `fn_1400068c0` `[确认]` **不调用** `fn_14000b122`，当场交船，准备天数 = 0。「%s天」文案只属于同规格路径。

### 开局船

`CFleet::vfunc_05` 从 `GetFixFleetProc` 拷 24 字节，五艘船引用 `== 0xD0` 则跳过，否则 `fn_140007153(..., id)` 把该 **CShip 槽** 挂上。来源：`CFleet.cpp`。`02-fleet-proxy.md` 说这些字节「常 ≥ 100 故不是 eShip」——现在知道 **100–206 就是目录模板槽**（目录 0–106）。

World0 / World1 舰队 0–6。剧本 0–3 读 World0、4–6 读 World1（`fn_14000ff60`，见 `10-narrative/04-world0-world1-diff.md`）。**开局队以剧本所用的那份包为准**；另一份包里同号舰队是该主角作为 NPC 时的舰队。

| scenario | 所用包 | 舰队槽字节 | 目录 id（字节−100） | 船名 |
| --- | --- | --- | --- | --- |
| 0 拉斐尔 | W0 | `C2 D0×4` | 94 | 卡鲁提拉 |
| 1 赫德拉姆 | W0 | `C3 D0×4` | 95 | 维斯泰洛斯 |
| 2 丽璐 | W0 | `C4 D0×4` | 96 | 阿伦海姆 |
| 3 华梅 | W0 | `C5 C6 D0×3` | 97, 98 | 春申、华昌 |
| 4 杏太郎 | W1（W0 同字节） | `C7 D0×4` | 99 | 埃斯佩兰莎 |
| 5 伍丁 | W1 | `C8 7E BD D0 D0` | 100, 26, 89 | 安娜希莉雅、巨型阿拉伯船、阿拉伯武装轻快船 |
| 6 蒂雅 | W1（W0 同字节） | `C9 D0×4` | 101 | 拉乌卡 |

W0 的舰队 5（NPC 伍丁）是 `BE BE 9F BD D0` → 90, 90, 59, 89（阿拉伯三角帆武装船 ×2、中型阿拉伯轻快商船、阿拉伯武装轻快船）。来源：`catalog/world_tables.json` fleet 0–6。

`def+0x08` 人物字节与主角对齐：94–97、99 = 0–4，100 = 5，101 = 6；华昌 98 为 `0xEC`。七位主角全有具名船体。

`NUM_PLAYEREVENTSHIP` 仍无整数。候选：主角具名船 94–101（8 条），或模板槽 194–206。禁止当成已确认。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空 | 重构 key |
| --- | --- | --- | --- | --- |
| 船体定义 | 静态表（无独立枚举） | 107 `[确认]` 表长 | — | `hull` key |
| 船模 | `eShipType` | 39 `[推断]` 循环上界（头文件 5 为投票误猜） | `INVALID_SHIPTYPE=-1`；实例空 `0x28` | `ship_model` key |
| 普通船模 | `eNORMALSHIPTYPE` | 34 `[推断]` 循环上界 / `rand(0x22)` | `INVALID_NORMALSHIPTYPE=-1` | `models.yard != null` |
| 船模池 | `CShipType` | 39 `[确认]` 构造计数 | — | 同船模 |
| 玩家事件船 | `ePLAYEREVENTSHIP` | `[未知]` | `INVALID_PLAYEREVENTSHIP=-1` | `player_event_ship` key |
| 默认炮种 | 定义 `+0x2E` / 船模 `+0x35` | 0–4 出现 | 空炮 7（见 `08`） | `default_gun_type` |
| 模板槽 | `CShip` id | 100–206 | 0–99 空；`0xD0` 未挂 | 仅 legacy_codec |
| 船坞列表 | `aeDockShip` | 207 | `0xD0` | 实例 id 列表 |

约定：

- 主键是命名空间 key（`vanilla:carutira`），`vanilla_index` = 原版行号 0–106 / 0–38，只供导入（D1）。系统之间传 `hull` / `ship_model` key 或活实例 `ship_instance_id`。
- 原版模板实例 id = `vanilla_index(hull) + 100`。这只是 `ids/legacy_codec.yaml` 的换算（舰队模板字节、旧存档），配置里写 hull key，不写 194。
- 船模 = 船体 `+0x09`；原版 `StlShip` 下标 = 船模 `vanilla_index`（< 34 时），重构写成 `still` 资源键。
- 运行时空值一律 `null`；`0x28`、`0xD0`、`7` 只在 legacy_codec。
- 模组加船：加 `hulls.yaml` 行 + 可选新船模行 + 贴图。运行时按表长，禁止写死 100 / 107 / 34 / 39 / 194。
- 「NPC 可随机抽到的变体」用 `hulls.yaml` 的 `npc_pool: true` 显式标，不要靠原版「槽 < 194」的位置约定。

## 4. 算法 / 公式

参数进 `config/formulas/ships.yaml`。禁止在代码里写死 `100`、`0x64`、`500`、`0x22`。

```
# 槽 ↔ 目录：只在 legacy_codec 导入舰队模板 / 旧存档时换算（template_base 100；0xD0 = null）

# 船坞上架（fn_14000d9a9）
yard_threshold(town) = f.yard_threshold(commerce = Towns.develop(town, commerce),
                                         military = Towns.develop(town, military),
                                         years    = Calendar.years_elapsed())   # (c + m/2)/5 + 30*年
listed(model, town):
    model.yard != null
    and MajorSea.of_town(town) in model.yard.seas                    # fn_1400013bb；原版位图

    and model.develop_need <= yard_threshold(town)                   # 原版 jg 跳过 → <= 上架
    and (size_filter == any or model.size_class == size_filter)      # 原版恒 any（实参 4）
buy_list = [model.yard_hull for model in models if listed(model, town)]   # 原版写 id+100 模板槽

# NPC 随机变体（fn_1400f4080）
npc_pick_hull(model) = rand_choice([h for h in hulls if h.npc_pool and h.model == model]) or null

# 静画
still(model) = model.still                                          # 原版 type < 34 才有；null 不加载

# 估价（CShip vfunc_04）
outfit_value(ship) =
    ship.model.list_price // f.outfit_base_div                       # 2，无符号
    + rig.mast_addon_outfit_fee * count(addons on ship.masts)       # 100；每桅 square_addon / fore_addon 各算一次（04 rig.yaml）
    + sum(add_parts[p].outfit_fee for p in ship.parts)                # 04 add_parts.yaml 每行一格
    + Guns.price(ship.gun_type, Ships.cabin_count(ship, gun_room), half=true)   # fn_140013593，见 08

town_state_price(town, price, context) =                           # 表归 economy：town_state_price_rules.yaml contexts 段
    r = rules.contexts[context].get(Economy.town_state(town), rules.default); udiv(price * r.num, r.den)
quoted_buy_gold  = town_state_price(town, outfit_value(src) * f.buy_price_mul, ship_buy)    # 2
quoted_sell_gold = town_state_price(town, outfit_value(ship), ship_sell)

# 同规格改造船准备天数（fn_14000c63f）：src = 己船，ref = 该船模默认
days = 0
if any(m exists in src and src.mast[m] != ref.mast[m]): days += f.copy_mast_days     # 2，只加一次
days += f.copy_part_days * count(p in src.parts and p not in ref.parts)              # 1
if any(cabin differs and cabins[cabin].copy_group == lo): days += f.copy_cabin_lo_days   # 2；原版种类 [7,12)
if any(cabin differs and cabins[cabin].copy_group == hi): days += f.copy_cabin_hi_days   # 2；原版种类 [12,21)
if src.gun_type != ref.default_gun_type: days += f.copy_gun_days                     # 1
catalog_build_days = 0                                                                # 原版当场交船

# 改名
name_bytes <= caps.ship_name_bytes                                  # 19；输入循环 0x12
```

上式全部 `[确认]`（反汇编见第 2 节）。`MajorSea.of_town(town)`（旧稿 `Towns.theater`）= 该港文化圈的 `major_sea_id` `[确认]`：`fn_140014e48` → `fn_140013a02`（港 `+0x25` 文化圈）→ `fn_14000aa0b`（`CCulture+0x10`），与 `02-factions` 卖货 `fn_140010d25` 用的同一条链；`town+0x74` 是 economy 的港口景气状态（`04-economy/02` §2.3：0 `war`、3 `boom`、4 `slump`）。原版按 id 区间判舱室组 `[7,12)` / `[12,21)`，重构改成 `cabins.yaml` 的 `copy_group` 列（D3）。

## 5. 耦合分析

原版：

- 船体定义夹带 31 舱室 + 3 桅；实例再被 `04` 改装改写。
- 买价调用改装加算和炮价（`08` 的 `fn_140013593`）。本系统报价，经济只扣金币。
- 上架读港口商业 / 军事发展度、港口文化圈 → 大海域（世界 / 势力）和日历年。本系统收标量，不持 `CTown*`。
- 买卖价读 `town+0x74` 港口景气状态做倍率。倍率表归 economy（`economy/town_state_price_rules.yaml` 的 `contexts`），状态由 `Economy.town_state(town_id)` 给出；本系统只选 context。
- 开局船在世界舰队模板里，是 `vanilla_index+100` 槽号，不是另一张 World 表；重构开局克隆实例，不挂模板。World0 / World1 共用这 107 条船体（exe 一份）。
- NPC 舰队生成按「槽 100–193 同船模」随机抽变体（`fn_1400f4080`）——位置即语义，重构改成显式标志。
- 具名船 `+0x08` 写死主角 id。叙事不要在贸易代码里 `if scenario==3`。
- 海怪 / 幽灵船是船模 34–38 / 目录 102–106，遭遇战用（舰队模板 62）。不上船坞是因为海域位图 `+0x38 = 0`，不是因为表价 0。接舷函数里 `0x22–0x26` 的海怪冲撞 / 幽灵瞬移（`07-combat/02`）是**船模行为**，重构进 `models.yaml` 的 `combat` 列（`on_contact: ram | teleport`、`ram_crit_rate`，字段定义归 `07-combat/02` §6），不在战斗代码比船模 id（D3）。
- `CShipyard` 空壳；逻辑在未挂类的 `fn_*` 与 `CSelectShipView`。重构后设施只发命令。

禁止：经济持有 `CShip*`；战斗写 `hulls.yaml`；UI 用列宽倒逼 100 条目录。

## 6. 解耦后的配置

```yaml
# config/caps.yaml（本篇增补；行数一律读表长，这里不放 107 / 39 / 34）
ship_name_bytes: 19
# 原版资源张数（StlShip 34 / ship 72 / DeckShip 17）是资源包元数据，不是上限；模板槽基址 100、实例池 207 只在 legacy_codec（01）
```

```yaml
# config/formulas/ships.yaml（续 01/04）
schema: dk4.formulas.ships
version: 1
buy_price:                      # §4 quoted_buy_gold 进 town_state_price 前
  impl: expr
  params:
    buy_price_mul: 2            # ulPrice = vfunc_04 << 1
  inputs: [outfit_value]
  expr: "outfit_value * buy_price_mul"
outfit_value:                   # §4 估价（CShip vfunc_04）
  impl: vanilla.outfit_value
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }
  params:
    outfit_base_div: 2          # 表价 / 2（无符号）
# 桅加帆件 100 / 改装件逐件估价：见 04 rig.yaml mast_addon_outfit_fee、add_parts.yaml outfit_fee（与改装差价 add_parts_price 不是同一份）
# 港口景气倍率不在本文件：economy/town_state_price_rules.yaml 的 contexts（ship_buy 买，remodel 改装，ship_sell 卖；04-economy/02 §6）
yard_threshold:
  impl: expr
  int_mode: vanilla_i32         # 两次有符号除，向零截断
  params: { develop_div: 5, military_div: 2, year_bonus: 30 }
  inputs: [commerce, military, years]
  expr: "(commerce + military / military_div) / develop_div + year_bonus * years"
build_days:                     # §4 同规格改造船准备天数（fn_14000c63f）+ 目录船工期
  impl: vanilla.build_days
  params:
    copy_mast_days: 2
    copy_part_days: 1
    copy_cabin_lo_days: 2           # cabins.yaml copy_group: lo（原版种类 [7,12)）
    copy_cabin_hi_days: 2           # copy_group: hi（原版种类 [12,21)）
    copy_gun_days: 1                # 炮种与船模默认不同
    catalog_build_days: 0           # 原版当场交船
```

```yaml
# config/ships/models.yaml         # 39 个 CShipType = eShipType
schema: dk4.ships.models
version: 1
models:
  - key: vanilla:small_hansa
    vanilla_index: 0
    name: 小型汉萨式帆船
    size_class: 0                   # +0x08
    base_durability: 30             # +0x0A，进 durability_max（01）
    list_price: 1300                # +0x0C；outfit_value 的起点
    default_masts: [0, 3, 0]
    default_parts: []               # +0x14 位图展开成 add_part key
    default_gun_type: vanilla:saker # +0x35
    yard_hull: vanilla:small_hansa  # 船坞代表船体（原版 = 同行号 hull）
    yard:                           # 有此节点才可能上架；海怪 / 幽灵船省略
      develop_need: 100             # +0x36
      seas: [vanilla:north_sea]             # +0x38 位图展开（major_sea key，world/major_seas.yaml；bit0 = north_sea）
    still: { pack: StlShip.dk4, index: 0 }
  - key: vanilla:ship_of_the_line
    vanilla_index: 15
    name: 战列舰
    size_class: 2
    base_durability: 90
    list_price: 160000
    default_masts: [15, 15, 5]
    default_parts: [vanilla:spanker, vanilla:bow_gun]   # 66 = 位 1、6
    default_gun_type: vanilla:heavy_cannon
    yard: { develop_need: 2500, seas: [vanilla:north_sea] }
    still: { pack: StlShip.dk4, index: 15 }
  - key: vanilla:monster_fish
    vanilla_index: 34
    name: 怪鱼
    size_class: 2
    base_durability: 800
    list_price: 0
    default_gun_type: vanilla:saker
    yard: null                      # 原版海域位图 = 0
    still: null                     # type >= 34 不加载静画
    combat: { on_contact: ram, ram_crit_rate: 50 }   # 07-combat/02；原版接舷函数比船模 0x22
  - key: vanilla:ghost_ship
    vanilla_index: 38
    name: 幽灵船
    size_class: 2
    base_durability: 400
    list_price: 0
    default_masts: [15, 7, 5]
    default_parts: [vanilla:headsail, vanilla:extra_cabin_1, vanilla:extra_cabin_2, vanilla:bow_gun, vanilla:stern_gun]  # 241
    default_gun_type: vanilla:culverin
    yard: null
    still: null
    combat: { on_contact: teleport }   # 原版比船模 0x26
```

```yaml
# config/ships/hulls.yaml
schema: dk4.ships.hulls
version: 1
# 107 行由 exe 0x1405b42e0 生成。name 已全部抽出，禁止再写 null。
# 价格只在 models.yaml；船体不另存 base_price。原版模板槽 = 100 + vanilla_index，只在 legacy_codec。
hulls:
  - key: vanilla:small_hansa
    vanilla_index: 0
    name: 小型汉萨式帆船
    model: vanilla:small_hansa      # type_id 0
    role: yard_template             # yard_template | variant | named | encounter
    npc_pool: true                  # 原版：槽 100–193
    default_parts: []               # +0x0A
    default_gun_type: vanilla:saker
    bound_person: null              # +0x08 → 实例舰长；原版 0xEC
  - key: vanilla:arab_lateen_armed
    vanilla_index: 90
    name: 阿拉伯三角帆武装船
    model: vanilla:giant_arab       # type_id 26
    role: variant
    npc_pool: true
    default_parts: [vanilla:armor_1, vanilla:extra_cabin_1, vanilla:extra_cabin_2, vanilla:bow_gun, vanilla:stern_gun]  # 244
    default_gun_type: vanilla:pedrero
  - key: vanilla:carutira
    vanilla_index: 94
    name: 卡鲁提拉
    model: vanilla:small_lateen     # type_id 1
    role: named
    npc_pool: false
    default_gun_type: vanilla:culverin
    bound_person: vanilla:raphael_castro
  - key: vanilla:anahelia
    vanilla_index: 100
    name: 安娜希莉雅
    model: vanilla:giant_arab       # type_id 26
    role: named
    npc_pool: false
    default_parts: [vanilla:headsail, vanilla:spanker, vanilla:extra_cabin_1, vanilla:extra_cabin_2]   # 51
    default_gun_type: vanilla:culverin
    bound_person: vanilla:abraham_woodin   # 伍丁；人物 key 以 03-characters 为准
  - key: vanilla:monster_fish
    vanilla_index: 102
    name: 怪鱼
    model: vanilla:monster_fish     # type_id 34
    role: encounter
    npc_pool: false
    default_gun_type: vanilla:saker
    bound_person: vanilla:person_197   # 原版字节 0xC5，与舰队模板 62 首字节同；人物 key 待定
```

船坞「哪条船体代表该船模」原版是 `model_id + 100`，即 `role: yard_template` 且行号相同的那行。重构用 `models.yaml` 的 `yard_hull: <hull key>` 显式指向，不靠下标相等。

```yaml
# config/narrative/starting_fleets.yaml
schema: dk4.narrative.starting_fleets
version: 1
# 值是 hull key。开局一律克隆成新实例，不挂模板实例。原版舰队字节 = vanilla_index + 100。
# 只列剧本自己的包；另一包的同号舰队是 NPC 舰队，归 fleets/templates。
starting_fleets:
  - { scenario: vanilla:raphael, pack: vanilla-world0, fleet: vanilla:fleet_000, hulls: [vanilla:carutira] }
  - { scenario: vanilla:helmut,  pack: vanilla-world0, fleet: vanilla:fleet_001, hulls: [vanilla:vestellos] }
  - { scenario: vanilla:lulu,    pack: vanilla-world0, fleet: vanilla:fleet_002, hulls: [vanilla:arnheim] }
  - { scenario: vanilla:hua_mei, pack: vanilla-world0, fleet: vanilla:fleet_003, hulls: [vanilla:chunshen, vanilla:huachang] }
  - { scenario: vanilla:kyotaro, pack: vanilla-world1, fleet: vanilla:fleet_004, hulls: [vanilla:esperanza] }
  - { scenario: vanilla:woodin,  pack: vanilla-world1, fleet: vanilla:fleet_005, hulls: [vanilla:anahelia, vanilla:giant_arab, vanilla:arab_armed_light] }   # 100, 26, 89
  - { scenario: vanilla:tia,     pack: vanilla-world1, fleet: vanilla:fleet_006, hulls: [vanilla:rauka] }
  # hull key 是示例拼写，以生成的 hulls.yaml 为准；驻港 town 属于 fleets/templates（见 02），这里不重复。
```

World0 / World1 **共用** `hulls.yaml` / `models.yaml`。开局差异只在 `starting_fleets.yaml` 的 pack 行。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗改代码：

- 船体行数、船模行数、StlShip 张数
- `role` / `npc_pool` / `models.yard`（是否可买、是否进 NPC 抽选）
- `list_price`、`buy_price_mul`、`outfit_base_div`；桅 / 改装估价（`04` 的 `rig.yaml` / `add_parts.yaml`）
- 港口景气倍率（economy `town_state_price_rules.yaml` 的 `ship_buy` / `remodel` / `ship_sell`；本系统只选 context）
- `yard.develop_need`、`yard.seas`、`yard_threshold` 公式参数
- `catalog_build_days` / 同规格拷贝天数；舱室 `copy_group`（`03`）
- `bound_person`（开局 / 赠船绑定）
- `combat`（海怪冲撞 / 瞬移，`07-combat`）
- `still`（换图只动资源键）
- 某剧本 `hulls` 列表

加一条可买船：`models.yaml` 一行（带 `yard` 节点）+ `hulls.yaml` 一行 `role: yard_template`。加一条只送不卖的剧情船：`role: named`，由 `GiveShip` 发，不进上架与 NPC 池。加一种遭遇怪：`models.yaml` 无 `yard` + `hulls.yaml` `role: encounter`。

## 8. 对外接口

事件：

- `YardListed { town_id, hull_ids }`
- `ShipQuoted { hull_id | ship_instance_id, gold, days }`
- `ShipBought { ship_instance_id, hull_id, gold, days, docked }`
- `ShipSold { ship_instance_id, gold }`
- `ShipNamed { ship_instance_id, name }`
- `ShipDocked { ship_instance_id, town_id }`
- `ShipGifted { ship_instance_id, hull_id, person_id, reason }`   # 事件 / 开局

查询：

- `Hulls.get(hull_id) -> HullDef`
- `Hulls.model(hull_id) -> ModelDef`
- `Hulls.still(hull_id) -> { pack, index } | none`
- `Yard.list(town_id) -> [hull_id]`
- `Yard.quoteBuy(town_id, hull_id | instance_id) -> { gold, days }`
- `Yard.quoteSell(town_id, instance_id) -> { gold }`
- `Start.fleet(scenario_id, pack) -> [hull_id]`

命令：

- `Yard.BuyShip { town_id, hull_id, name } -> ship_instance_id`   # 总是新建实例，不复用模板实例
- `Yard.BuyCopy { town_id, source_instance_id, name } -> ship_instance_id`   # 同规格
- `Yard.SellShip { instance_id }`
- `Yard.Rename { instance_id, name }`
- `Narrative.GiveShip { hull_id, fleet_id | dock, name?, person_id? } -> ship_instance_id`

`GiveShip` 不走金币、不上架过滤、不强制 19 字节改名（开局用目录名）。`BuyShip` 必须过上架、钱、船坞 / 五槽、改名。事件解释器只发 `GiveShip`，禁止自己 `lstrcpyA` 进 `CShip*`。

付钱：`Force.add_gold(force_id, delta, reason)`（买负、卖正；factions）。上架输入：`Towns.develop(town_id, track)`、`MajorSea.of_town(town_id)`（= 文化圈的 `major_sea_id`，`02-factions` 注册）、`Calendar.yearsElapsed()`。价格倍率输入：`Economy.town_state(town_id)`。五槽满了：`Fleets.dock(instance_id)`。同规格准备：`Calendar.advanceDays(days)`。

## 9. 未核实字段

- `NUM_SHIPTYPE` 正式值（循环上界 39 为推断；头文件 5 为脚本误猜）
- `NUM_NORMALSHIPTYPE` 正式值（34 为推断；World DLL 断言串无代码引用）
- `NUM_PLAYEREVENTSHIP` 正式值；与具名船 94–101 / 槽 194–206 的关系
- `NUM_SHIPGRADE` 正式值与它挂在哪个字段：`+0x2E` / `+0x35` 已证明是炮种；`[推断]` 等级就是船模 `+0x08` 体型档 0/1/2（`fn_14000d152` 取它、无船模时回 4，`06-ocean/01-sailing.md` 也把这条叫 `eShipGrade`）
- 船体 `+0x2F`（全 0）；船模 `+0x39..0x3F`（全 0）
- 船模 `+0x38` 海域位图运行时被谁改写（它是 `CShipType` 唯一存档字段）
- ~~`town+0x74` 港口类别 0 / 3 / 4 的名字~~：已定为 economy 景气状态 `war` / `boom` / `slump`（`04-economy/02` §2.3）
- 日历打包纪元：上架年项 `30×年` 开局时是否为 0
- 改装位 0、1、4、5、8 的名字（作用已在 `04` 确认）
- `ship.dk4` 72 张 ↔ 39 船模的下标；`DeckShip.dk4` 17 张的切分
- 事件送船 opcode；mesfile「赠」与船无关
- `CSelectShipView+0x138` 起那批立即数（UI 矩形）
- `CShipChangeView+0x4d0` 的 100 个 dword 是拥有掩码还是别的

抽出名字这件事已经做完。以后只许改 key / 补字段，不许把「盖伦」写进 `name`。
