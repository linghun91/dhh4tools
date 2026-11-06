# 船体目录、船模、造船厂与开局船

舱室 / 帆 / 炮 / 装甲 / 船首像见 `03-cabins-crew-posts.md`、`04-remodel-guns-supplies.md`。舰队槽与地方舰队见 `02-fleet-proxy.md`。船体实例布局见 `01-ship-model.md`。本篇只补：**100 条船型名录**、**船种 / 船模**、**造船厂买卖**、**剧本初始船与事件赠船**。

exe 里**没有**「轻帆船」「轻快帆船」「盖伦」「克拉克」。SC 船型名是「小型汉萨式帆船」「战列舰」这类，从 `Game/DK4HD_sc.exe` `.rdata` 抽出。禁止用常识译名填回原版包。

## 1. 职责边界

本系统管：

- 船体目录 `eShip` 0–99（名字、默认舱室、默认桅、等级、绑定人物）
- 船模 / 船种对象 `CShipType` 0–38（贴图、水手、表价、体型档）
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
| 船型目录 | `eShip` | 100 | 可命名的船体定义。头文件 `ShipID.h` `NUM_SHIP=100` |
| 船种枚举 | `eShipType` | 5 | `ShipTypeID.h` `NUM_SHIPTYPE=5`。**五条中文名未抽出** |
| 普通船种 | `eNORMALSHIPTYPE` | `[推断] 34` | 断言 `eShipType < NUM_NORMALSHIPTYPE`。`StlShip.dk4` 34 张；`fn_140011d2e` `cmp type, 0x22` |
| 船模对象池 | `CShipTypeElement` | 39（`0x27`） | `fn_1400017df(..., 0x50, 0x27)`。34 艘可买船模 + 4 海怪 + 1 幽灵船 |
| 玩家事件船 | `ePLAYEREVENTSHIP` | `[未知]` | `PlayereventshipID.h` 只有符号。World0.dll UTF-16 断言有 `NUM_PLAYEREVENTSHIP`，整数未写进头 |

来源：`srcdata/source/ShipID.h`、`ShipTypeID.h`、`NormalshiptypeID.h`、`PlayereventshipID.h`、`catalog/num_values.json`、`CShipTypeElement.cpp`、`CShipSurfaceManager.cpp`（同样循环 `>= 0x27`）、`Game/StlShip.dk4`。

`01-ship-model.md` 把「id ≥ 100 → 空船」写反了。汇编是：

```
; CShip::vfunc_05  0x1402adee0
call fn_1400026e9          ; 槽下标 = (this - pool) >> 7，sizeof(CShip)=0x80
cmp eax, 0x64
jge init_from_table        ; id >= 100 → 目录 (id-100)
mov [ship+0x49], 0x28      ; id < 100 → 空实例
```

`fn_140001eab`（`0x140230430`）：`sar rax, 7`。来源：反汇编，不是反编译的颠倒 `jge`。

因此 207 个 `CShip` 槽（`0xCF`）的用法 `[确认 分段]`：

| 槽 | id | 初始化 |
| --- | --- | --- |
| 0–99 | < 100 | 空（`+0x49=0x28`） |
| 100–199 | 100–199 | 船体目录 0–99 |
| 200–206 | 200–206 | 仍在 207 池内。`vfunc_05` 会按 `id-100` 取表，**越出 0–99**。开局舰队里出现 `0xC8=200`、`0xC9=201`，语义 `[未知]` |
| 208 | `0xD0` | 空船哨兵（在 207 外侧） |

造船厂买列表写 `eShipType + 0x64`（`fn_14000d9a9`），就是槽 100+type = 该船模的模板实例。

### 船体定义表（100 × 0x30）

`fn_14000cff9`（`0x1402ac5e0`）：

```
eax = id - 0x64
rax *= 0x30
return &table[rax]
```

表 VA `0x1405b42e0`（`lea rcx, [rip+0x307ce7]`），文件偏移 `0x5b30e0`，步长 **48**。名称指针在 `+0x00`，GBK C 字符串在邻近 `.rdata`。`CShip::vfunc_05` `lstrcpyA(ship+8, *def)`。

| 偏移 | 宽 | 拷到 `CShip*` | 含义 |
| --- | --- | --- | --- |
| `+0x00` | ptr | `+0x10` 起 19 B 名 | 目录名 |
| `+0x08` | u8 | `+0x48` | 默认人物；`0xEC` 空；94–97、99 为主角 0–4 |
| `+0x09` | u8 | `+0x49` | `CShipType` 下标（船模） |
| `+0x0A` | u16 | `+0x46` | `[未知]`（武装/商用变体常非 0） |
| `+0x0C` | 3 B | `+0x1B` | 三根桅默认 |
| `+0x0F` | 31 B | `+0x1E` | 默认舱室种类（`0x17`→`0x18`） |
| `+0x2E` | u8 | `+0x4A` | 船体等级 0–4 `[确认 取值]` |
| `+0x2F` | u8 | — | 抽出全 0 |

100 个名字全部在 exe 里，**无 null**。同名两条：id 16 与 18 共用指针 `0x1405b5b28`「北海三桅三角帆船」（桅布局不同）。

| id | name | type_id | grade | 备注 |
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

目录分段 `[确认 范围，名称分组]`：0–33 船坞模板（与船模 0–33 同名或极近）；34–63 商用 / 运输合约 / 一部分武装（43–46 已是日本武装与军用铁甲）；64–93 武装变体（type_id 0–29）；94–99 具名开局船。不要把 34–63 写成「全是商用」。

### 船模表（39 × 0x40）

`fn_14001372d`（`0x1402ac610`）：`id * 0x40 + table`。表 VA `0x1405b3460`。`CShipType::vfunc_05`（`0x1402ae1f0`）填充运行时对象（尺寸 `0x50`）。

| 偏移 | 宽 | 对象 | 含义 |
| --- | --- | --- | --- |
| `+0x00` | ptr | `+0x08` | 名称 |
| `+0x08` | u8 | `+0x10` | 体型档 0/1/2（对应 `01` 的 4000/7000 三档）`[确认 取值]` |
| `+0x0A` | u16 | `+0x12` | 水手定额 `[推断]`（25–100；海怪 400–999） |
| `+0x0C` | u32 | `+0x14` | 表价 1300–160000；海怪 / 幽灵为 0 `[确认 数值]`。**不是** 下面买价 vfunc 读的字段 |
| `+0x10` | 3 B | `+0x18` | 默认桅 |
| `+0x14` | u16 | `+0x1C` | `CShipType` vtbl+0x20 返回值（多半 0；战列舰 66、铁甲 196） |
| `+0x16` | 31 B | `+0x1E` | 默认舱室 |
| `+0x35` | u8 | `+0x3D` | 与船体 `grade` 同档 0–4 |
| `+0x36` | u16 | `+0x3E` | 上架门槛候选（100–3000，与发展度同量级）`[推断]` |
| `+0x38` | u8 | `+0x40` | 地域 / 文化过滤候选 `[未知]` |

船模 0–33 名称与船体 0–33 对应（个别用词不同，如船模 4「大型三桅方帆宽身船」vs 船体 4「宽大型三桅方形帆船」）。**34–38 只存在于船模表，没有同号船体：**

| type_id | name | table_u32 | crew |
| --- | --- | --- | --- |
| 34 | 怪鱼 | 0 | 800 |
| 35 | 大墨鱼 | 0 | 600 |
| 36 | 鲨鱼 | 0 | 500 |
| 37 | 白鲸 | 0 | 999 |
| 38 | 幽灵船 | 0 | 400 |

`NUM_SHIPTYPE=5` 的五条名字仍 `[未知]`。不要把这 39 个船模名当成那 5 个枚举。按 type_id 粗分欧式帆 / 桨帆 / 阿拉伯 / 中国 / 日本是 `[推断 分组]`，没有单独字段。

### 船模资源

全部 `GT1G4600`（`formats.md`）。

| 文件 | 张数 | 用法 |
| --- | --- | --- |
| `Game/StlShip.dk4` | **34** | 静画。`fn_140011d2e`：`type >= 0x22` 走另一路，否则 `fn_140010e6f(..., "StlShip.dk4", type)`。rdata 里 `StlShip.dk4` 紧挨 `eShipType` |
| `Game/ship.dk4` | **72** | 海图 / 海战船图。`CShipSurfaceManager` 为 39 个 type 各挂表面；72 与 39 的下标式未钉死。`eCloseCombatTypeToIndex` 也在加载此文件（见 `07-combat/02`） |
| `Game/DeckShip.dk4` | **17** | 甲板俯视船身。与 34/39 都对不上，按体型 / 等级切分 `[未知]` |
| `Game/DeckScrn.dk4` | **50** | 甲板 UI（舱室按钮），不是船体目录 |

映射规则 `[确认]`：`type_id < 34` → `StlShip.dk4` 下标 = `type_id`。船体的 `model_asset` 跟 `type_id`，不跟 `hull_id`（商用 / 武装变体共用船模）。

### 造船厂

`CShipyard` 只有构造 / 析构，尺寸 `0x10`，没有买卖逻辑。来源：`game/town/CShipyard.cpp`。

实际会话在 `fn_140004fd9`（`0x1401765f0`）菜单：

| 选项 | 函数 | 角色 |
| --- | --- | --- |
| 0 | `fn_1400068c0`（`0x140176320`） | 买船坞上架的船模 |
| 1 | `fn_140007531`（`0x140175f60`） | 买「与已改造好的船完全一样的船」 |

上架：`fn_14000d9a9`（`0x1402613e0`）扫 `eShipType` 0–38。过滤：`fn_1400013bb`（船模 vs 港口）、`fn_140002e9b` 读船模 `+0x46` 与港口发展度合成值比大小、可选 `fn_14000e093` 相等（实参 `== 4` 则跳过）。命中则写入 `type_id + 100`。

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
| `0x8053` | …准备就需要%s天的时间，买吗？ |
| `0x30a3` | 将各艘船放在船坞里。 |
| `0x343f` | 把拖来的船停泊在船坞里。 |

`fn_14000de13`（`0x140178a20`）RTC `szShipName` 19 字节、`aeDummyShip`：从模板拷名 → 输入（上限 `0x12`）→ 空名消息 `0x102` → 舰队未满则进 5 槽，否则消息 `0x3a3` 进船坞。

mesfile 里「赠」命中是广场免费样品，**不是**送船。送船走事件脚本，opcode 未译码（`02-event-system.md`）。

### 买价与天数

`CShip` vtbl+0x20 = `vfunc_04` body `0x140230140`（RTC `eShipMast`）。组成：

```
value  = CShipType.vfunc_20() / 2          # 读对象 +0x1C = 定义 +0x14 那个 u16
value += 100 * mast_feature_count          # 三根桅，每根两个布尔
for part in 0..8:                          # 已装改装件
    value += {500 | 400 | 1000}            # 0x1F4 / 0x190 / 0x3E8，按 part 跳表
value += fn_140013593(holds)               # 货格差价，见 04
```

买船两处都做 `ulPrice = value << 1`。所以实付是这套「当前配置估价」的两倍，**不是** 船模表 `+0x0C` 那个 u32。u32（1300–160000）看起来像标价，但买价 vfunc 不读它 `[确认 买价路径；表价用途未知]`。

卖：只有文案「卖哪艘船？」。未找到与买对称、且 RTC 名为卖的函数。`vfunc_04` 已 `/2`，卖是否直接用它 `[推断 未证实]`。

天数：

- 买同规格改造船：`fn_14000c63f`。桅种类不同 `+2`（遇第一根就停）；新改装件每件 `+1`；舱室种类落在 `[7,12)` 有差 `+2`，落在 `[12,0x15)` 有差 `+2`；`vfunc+0x40` 不同 `+1`。结果当「准备天数」塞进消息 `0x3b4` / 为 0 走 `0x3b5`。
- 买目录船 `fn_1400068c0` **不算** 这个函数。文案仍有「%s天」，按船体表的天数 `[未知]`。

### 开局船

`CFleet::vfunc_05` 从 `GetFixFleetProc` 拷 24 字节，五艘船引用 `== 0xD0` 则跳过，否则 `fn_140007153(..., id)` 把该 **CShip 槽** 挂上。来源：`CFleet.cpp`。`02-fleet-proxy.md` 说这些字节「常 ≥ 100 故不是 eShip」——现在知道 **100–199 就是目录模板槽**。

World0 / World1 舰队 0–6（剧本 0–6 的开局队，驻港与 `02` 一致）：

| scenario | 舰队槽字节 | hull_id（字节−100，仅 100–199） | 船名 |
| --- | --- | --- | --- |
| 0 拉斐尔 | `C2 D0×4` | 94 | 卡鲁提拉 |
| 1 赫德拉姆 | `C3 D0×4` | 95 | 维斯泰洛斯 |
| 2 丽璐 | `C4 D0×4` | 96 | 阿伦海姆 |
| 3 华梅 | `C5 C6 D0×3` | 97, 98 | 春申、华昌 |
| 4 杏太郎 | `C7 D0×4` | 99 | 埃斯佩兰莎 |
| 5 伍丁 W0 | `BE BE 9F BD D0` | 90, 90, 59, 89 | 阿拉伯武装三角帆 ×2、轻快商船、武装轻快船 |
| 5 伍丁 W1 | `C8 7E BD D0 D0` | **200 非目录**、26、89 | 巨型阿拉伯船、武装轻快船；首槽 `[未知]` |
| 6 蒂雅 | `C9 D0×4` | **201 非目录** | `[未知]` 不在 94–99 具名船里 |

94–99 的 `def+0x08` 人物字节与主角 0–4 对齐（华昌除外）。伍丁 / 蒂雅没有专名船体。

`NUM_PLAYEREVENTSHIP` 仍无整数。候选：6 条具名船，或槽 200–206 共 7 格。禁止当成已确认。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空 | 重构 key |
| --- | --- | --- | --- | --- |
| 船体目录 | `eShip` | 100 | `INVALID_SHIP=-1` | `hull_id` |
| 船种（粗） | `eShipType` | 5 | `INVALID_SHIPTYPE=-1` | `ship_kind_id` |
| 普通船模 | `eNORMALSHIPTYPE` | `[推断] 34` | `INVALID_NORMALSHIPTYPE=-1` | `ship_model_id` 0–33 |
| 船模池 | `CShipType` | 39 | — | `ship_model_id` 0–38 |
| 玩家事件船 | `ePLAYEREVENTSHIP` | `[未知]` | `INVALID_PLAYEREVENTSHIP=-1` | `player_event_ship_id` |
| 船体等级 | 定义 `+0x2E` | 0–4 出现 | `INVALID_SHIPGRADE=-1` | `ship_grade_id` |
| 模板槽 | `CShip` id | 100–199 | 0–99 空；`0xD0` 未挂 | `ship_instance_id` |
| 船坞列表 | `aeDockShip` | 207 | `0xD0` | 同实例 id |

约定：

- `hull_id` 永远 0–99。系统之间传它，或传活实例 `ship_instance_id`。
- 模板实例 id = `hull_id + 100`。这是原版买列表 / 开局舰队用的数，配置里不要把 194 写成 hull 194。
- `ship_model_id` = 船体 `+0x09` = `StlShip` 下标（< 34 时）。
- 模组加船：加 `hulls.yaml` 行 + 可选新船模行 + 贴图。运行时按表长，禁止写死 100 / 34 / 39。

## 4. 算法 / 公式

参数进 `config/formulas/ships.yaml`。禁止在代码里写死 `100`、`0x64`、`500`、`0x22`。

```
# 槽 → 目录
is_template(instance_id) = (instance_id >= caps.hull_template_base)          # 100
                          and (instance_id < caps.hull_template_base + len(hulls))
hull_id_of(instance_id)  = instance_id - caps.hull_template_base             # 仅模板槽
is_empty_player_slot(id) = (id < caps.hull_template_base)                    # 0–99 经 vfunc_05 标 0x28
is_empty_ref(id)         = (id == sentinels.ship_instance_empty)             # 0xD0

# 船坞上架（fn_14000d9a9）
listed(model_id, town):
    model_id in 0 .. len(models)-1
    and culture_allows(model, town)                 # fn_1400013bb，公式未知
    and model.develop_need <= town_develop_score    # fn_140002e9b vs 商业+(军事-1)/2 [推断]
    and (mode == any or model.region_key == mode)   # 实参 4 跳过
buy_list_ids = [model_id + caps.hull_template_base for model in listed]

# 静画
still_index(model_id) = model_id if model_id < caps.stlship_count else special_still  # 34

# 买价（确认路径）
outfit_value(instance) =
    models[instance.type].word_14 // 2
    + formulas.mast_feature_fee * count_mast_features(instance)     # 100
    + sum(add_part_buy_fee[part] for part installed)                # 500/400/1000
    + hold_delta_fee(instance)                                      # fn_140013593
quoted_buy_gold = outfit_value(instance) * formulas.buy_price_shift # 2
# type.table_u32 不进这条式子

# 同规格改造船准备天数（fn_14000c63f）
days = 0
if first_mast_kind_differs: days += formulas.copy_mast_days         # 2
days += formulas.copy_part_days * count_new_parts                   # 1
if cabin_kind_diff_in_[7,12): days += formulas.copy_cabin_lo_days   # 2
if cabin_kind_diff_in_[12,21): days += formulas.copy_cabin_hi_days  # 2
if vfunc40_differs: days += formulas.copy_misc_days                 # 1

# 改名
name_bytes <= caps.ship_name_bytes                                  # 19；输入循环 0x12
```

卖价、目录船准备天数、发展度合成的精确式，参数先留 `null` / `unverified`。

## 5. 耦合分析

原版：

- 船体定义夹带 31 舱室 + 3 桅；实例再被 `04` 改装改写。
- 买价调用改装加算和货格差价（`04` 的 `fn_140013593`）。本系统报价，经济只扣金币。
- 上架读港口发展度（世界）和地域字节。本系统收标量，不持 `CTown*`。
- 开局船在世界舰队模板里，是 `hull_id+100` 槽号，不是另一张 World 表。World0 / World1 共用这 100 条船体（exe 一份）。
- 具名船 `+0x08` 写死主角 id。叙事不要在贸易代码里 `if scenario==3`。
- 海怪 / 幽灵船是船模 34–38，遭遇战用，不上船坞（表价 0）。
- `CShipyard` 空壳；逻辑在未挂类的 `fn_*` 与 `CSelectShipView`。重构后设施只发命令。

禁止：经济持有 `CShip*`；战斗写 `hulls.yaml`；UI 用列宽倒逼 100 条目录。

## 6. 解耦后的配置

```yaml
# config/caps.yaml（本篇增补）
ship_hulls: 100
ship_kinds: 5
ship_models: 39                 # CShipType 池；运行时读表长
ship_models_normal: 34          # StlShip 张数；[推断] NUM_NORMALSHIPTYPE
hull_template_base: 100
ship_instance_pool: 207
stlship_count: 34
ship_dk4_count: 72
deckship_count: 17
ship_name_bytes: 19
ship_grade_max: 4               # 定义 +0x2E 出现 0–4
```

```yaml
# config/formulas/ships.yaml（续 01/04）
schema: dk4.formulas.ships
version: 1
buy_price_shift: 2              # ulPrice = vfunc_04 << 1
mast_feature_fee: 100           # 0x64
add_part_buy_fee:               # vfunc_04 跳表；与 04 改装差价表不是同一份
  500: 0x1F4
  400: 0x190
  1000: 0x3E8
copy_mast_days: 2
copy_part_days: 1
copy_cabin_lo_days: 2
copy_cabin_hi_days: 2
copy_misc_days: 1
sell_uses_outfit_value: null    # 未证实
catalog_build_days: null
```

```yaml
# config/ships/types.yaml          # 5 个 eShipType，名未抽出
schema: dk4.ships.kinds
version: 1
- id: 0
  key: ship_kind_0
  name: null
- id: 1
  key: ship_kind_1
  name: null
- id: 2
  key: ship_kind_2
  name: null
- id: 3
  key: ship_kind_3
  name: null
- id: 4
  key: ship_kind_4
  name: null
```

```yaml
# config/ships/models.yaml         # 39 个 CShipType
schema: dk4.ships.models
version: 1
- id: 0
  key: small_hansa
  name: 小型汉萨式帆船
  size_class: 0
  crew: 30
  table_u32: 1300                 # 定义 +0x0C；买价 vfunc 不读
  word_14: 0
  grade: 0
  develop_need: 100               # [推断] +0x36
  still: { pack: StlShip.dk4, index: 0 }
  buyable: true
- id: 15
  key: ship_of_the_line
  name: 战列舰
  size_class: 2
  crew: 90
  table_u32: 160000
  word_14: 66
  grade: 4
  develop_need: 2500
  still: { pack: StlShip.dk4, index: 15 }
  buyable: true
- id: 34
  key: monster_fish
  name: 怪鱼
  size_class: 2
  crew: 800
  table_u32: 0
  still: null                     # type >= 34 不走 StlShip
  buyable: false
  encounter_only: true
- id: 38
  key: ghost_ship
  name: 幽灵船
  table_u32: 0
  buyable: false
  encounter_only: true
```

```yaml
# config/ships/hulls.yaml
schema: dk4.ships.hulls
version: 1
# 100 行由 exe 0x1405b42e0 生成。name 已全部抽出，禁止再写 null。
- id: 0
  key: small_hansa
  name: 小型汉萨式帆船
  type_id: 0
  model_asset: StlShip.dk4:0
  buyable: true
  gift_only: false
  base_price: 1300                # 船模 table_u32；实付见 outfit_value<<1
  build_days: null
  grade_id: 0
  bound_person_id: null
- id: 33
  key: ironclad
  name: 铁甲船
  type_id: 33
  model_asset: StlShip.dk4:33
  buyable: true
  gift_only: false
  base_price: 150000
  build_days: null
  grade_id: 4
- id: 90
  key: arab_lateen_armed
  name: 阿拉伯三角帆武装船
  type_id: 26
  model_asset: StlShip.dk4:26
  buyable: false                  # 武装变体；船坞卖的是 type 26 的模板 id 26
  gift_only: false
  base_price: 74000
  build_days: null
- id: 94
  key: carutira
  name: 卡鲁提拉
  type_id: 1
  model_asset: StlShip.dk4:1
  buyable: false
  gift_only: true
  base_price: 2400
  build_days: null
  bound_person_id: 0              # 拉斐尔
- id: 99
  key: esperanza
  name: 埃斯佩兰莎
  type_id: 3
  model_asset: StlShip.dk4:3
  buyable: false
  gift_only: true
  base_price: 12000
  build_days: null
  bound_person_id: 4              # 杏太郎
```

```yaml
# config/narrative/starting_fleets.yaml
schema: dk4.narrative.starting_fleets
version: 1
# 值是 hull_id。原版舰队字节 = hull_id + 100（仅当字节在 100–199）。
- scenario_id: 0
  key: raphael
  pack: vanilla-world0
  fleet_id: 0
  town_id: 11                     # 里斯本 [推断]，见 02
  hull_ids: [94]
- scenario_id: 1
  key: helmut
  pack: vanilla-world0
  fleet_id: 1
  town_id: 7
  hull_ids: [95]
- scenario_id: 2
  key: lulu
  pack: vanilla-world0
  fleet_id: 2
  town_id: 2
  hull_ids: [96]
- scenario_id: 3
  key: hua_mei
  pack: vanilla-world0
  fleet_id: 3
  town_id: 54
  hull_ids: [97, 98]
- scenario_id: 4
  key: kyotaro
  pack: vanilla-world1
  fleet_id: 4
  town_id: 49
  hull_ids: [99]
- scenario_id: 5
  key: woodin
  pack: vanilla-world0
  fleet_id: 5
  town_id: 38
  hull_ids: [90, 90, 59, 89]
- scenario_id: 5
  key: woodin
  pack: vanilla-world1
  fleet_id: 5
  town_id: 38
  hull_ids: [26, 89]              # 模板首字节 0xC8=200 不是目录；不编造 hull 100
  unknown_slots: [200]
- scenario_id: 6
  key: tia
  pack: vanilla-world1
  fleet_id: 6
  town_id: 67
  hull_ids: []
  unknown_slots: [201]
```

World0 / World1 **共用** `hulls.yaml` / `models.yaml`。开局差异只在 `starting_fleets.yaml` 的 pack 行。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗改代码：

- 船体行数、船模行数、StlShip 张数
- `buyable` / `gift_only` / `encounter_only`
- `base_price`（表价）与 `buy_price_shift`、桅 / 改装加算
- `develop_need`（哪座港上架）
- `build_days` / 同规格拷贝天数
- `bound_person_id`（开局 / 赠船绑定）
- `model_asset`（换图只动资源键）
- 某剧本 `hull_ids` 列表

加一条可买船：`models.yaml` + `hulls.yaml` 各一行，`buyable: true`。加一条只送不卖的剧情船：`gift_only: true`，由 `GiveShip` 发，不上 `fn_14000d9a9`。

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
- `Yard.quoteBuy(hull_id | instance_id) -> { gold, days }`
- `Yard.quoteSell(instance_id) -> { gold } | unverified`
- `Start.fleet(scenario_id, pack) -> [hull_id]`

命令：

- `Yard.BuyShip { town_id, hull_id | template_instance_id, name } -> ship_instance_id`
- `Yard.BuyCopy { town_id, source_instance_id, name } -> ship_instance_id`   # 同规格
- `Yard.SellShip { instance_id }`
- `Yard.Rename { instance_id, name }`
- `Narrative.GiveShip { hull_id, fleet_id | dock, name?, person_id? } -> ship_instance_id`

`GiveShip` 不走金币、不上架过滤、不强制 19 字节改名（开局用目录名）。`BuyShip` 必须过上架、钱、船坞 / 五槽、改名。事件解释器只发 `GiveShip`，禁止自己 `lstrcpyA` 进 `CShip*`。

付钱：`Gold.debit`。发展度：`Towns.develop(town_id)`。五槽满了：`Fleets.dock(instance_id)`。

## 9. 未核实字段

- `NUM_SHIPTYPE=5` 的中文名与字段
- `NUM_NORMALSHIPTYPE` 正式值（34 为推断）
- `NUM_PLAYEREVENTSHIP` 正式值；与 94–99 / 槽 200–206 的关系
- `NUM_SHIPGRADE` 正式值（0–4 已出现）
- 船体 `+0x0A` u16、`+0x2F`
- 船模 `+0x0C` u32（表价）谁读；与买价 vfunc 的关系
- 船模 `+0x36` 是否就是发展度门槛；`+0x38` 是否文化
- `fn_1400013bb` / `fn_14000e093` 完整谓词
- 目录船准备天数；卖价是否 = `vfunc_04`（已 /2）
- `add_part_buy_fee` 与 `eAddParts` 0–8 的一一对应
- `ship.dk4` 72 张 ↔ 39 船模的下标；`DeckShip.dk4` 17 张的切分
- type ≥ 34 的静画走哪份 dk4
- 伍丁 World1 槽 200、蒂雅槽 201 对应哪条 `hull_id`
- 事件送船 opcode；mesfile「赠」与船无关
- `CSelectShipView+0x138` 起那批立即数（UI 矩形）
- `CShipChangeView+0x4d0` 的 100 个 dword 是拥有掩码还是别的

抽出名字这件事已经做完。以后只许改 key / 补字段，不许把「盖伦」写进 `name`。
