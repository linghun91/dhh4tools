# 日期与经纬度

本系统拥有两件正交的世界标尺：**时间坐标**（打包日历、日步、事件用的 `SCalendar` 快照）和 **经纬度显示**（原版定位单位 → 整数度）。两者都从世界对象上读，都不拥有港口表、网格通行、风向场。

引擎统一由 `Clock` 推进时间：它持有打包时刻、按 `config/engine/tick.yaml` 的阶段序列逐拍调度各系统，并在日首 / 月首 / 月末 / 年首发 time 事件。本篇定义 `Clock` 用的日历编码与谓词（`Calendar` 纯函数）。叙事只查询快照或订阅事件，不自己算月末；任何系统不得读 `SCalendar+0x1c` 之类原版字段。经纬度是投影，不是 `World.dat` 下标。

配置落点：`config/world/calendar.yaml`、`config/world/coords.yaml`；立刻数进 `config/formulas/world.yaml` 与 `config/caps.yaml`。

## 1. 职责边界

管：

- 打包日期的编码 / 解码（年、月、日、日内拍 `nTime`）
- 月长表、无闰年、日内拍数
- `SCalendar` 快照（含 `+0x1c` 三态）以及事件触发里的时间通配
- 原版定位单位 → 显示用整数经纬度
- 经纬度 UI 格式与半球文案 key
- 开局生日（月日）的编码；生日是人物状态（characters），黄道也归 characters（`04-climate-weather-zodiac.md` 只留事实）

不管：

- `World.dat` 2500 网格与原版单位的换算（`01-map-and-sea-areas.md`：`/16`；禁止 `/8`）
- 三种地图怎么画（`06-three-maps.md`）。该篇里的 `40000` / `20000` 分母与本篇原点是同一组立刻数，本篇只拥有度换算
- 风向场、天气档（`04-climate-weather-zodiac.md`）；星座表与 `Zodiac.dk4`（characters）
- 旅馆收几金、修船等几天（town-life 发 `Clock.advance_days(n)`）
- 水粮 / 疲劳怎么扣（ocean 在 `tick.yaml` 里按 `schedule` 订阅）
- 月结上缴金额、品牌「今年」（economy；本系统只给 `MonthStarted` / `MonthEnding` / `YearStarted`）
- mesfile 正文（只持 `string_key`）

## 2. 原版事实

### 2.1 类型与体积

- `[确认]` `SCalendar -> SDate`。来源：`srcdata/catalog/class_hierarchy.txt`。
- `[确认]` `SDate` 析构按 `0x18` 释放，`SCalendar` 按 `0x28` 释放。来源：`SDate.cpp` `SCalendar.cpp`。事件扫描栈上 `sCalendar` RTC 标 40 字节，与 `0x28` 一致。来源：`fn_140013c32` RTC。
- `[确认]` `fields.tsv`：`SDate` 访问 `+0x8 ×2, +0xc ×2, +0x10 ×2, +0x14 ×2`；`SCalendar` 访问 `+0x1c ×3, +0x14 ×2, +0x10 ×2, +0x18 ×1, +0xc ×1`。与下面构造函数写入一一对应。

`SDate` 布局（构造 `0x1402ab440` 直接填字段；`0x1402ab4b0` 从打包整数拆）：

| 偏移 | 宽度 | 内部名 | 值域 |
| --- | --- | --- | --- |
| `+0x00` | ptr | vptr | |
| `+0x08` | i32 | `year` | 打包纪元起的整年，0-based |
| `+0x0c` | i32 | `month` | 0–11 |
| `+0x10` | i32 | `day` | 0-based 月内日 |
| `+0x14` | i32 | `nTime` | 0–47 |

`SCalendar` 在 `SDate` 之后：

| 偏移 | 宽度 | 含义 |
| --- | --- | --- |
| `+0x18` | i32 | 构造时拷贝进来的**打包整数** |
| `+0x1c` | i32 | 三态 0 / 1 / 2，见 2.3 |

### 2.2 打包整数

- `[确认]` 拆包（`SDate` 构造 `0x1402ab4b0`，UI `fn_140007e69` body `0x1402ab9c0`）：

```
calendar_ticks_per_day = 48 # 立即数 0x30（caps）
days_per_year = 365         # 立即数 0x16D
day_index = packed / 48     # div（无符号）
year  = day_index / 365     # idiv（有符号；day_index < 2^31 时等价）
nTime = packed - day_index * 48
doy   = day_index % 365
```

- `[确认]` 年可以单独取：`fn_140002068`（body `0x1400f5440`）对对象 `+0x10` 做 `div 0x4470`。`0x4470 = 17520 = 365×48`。
- `[确认]` 月长表 `DAT_1405af730` 十二个 `int32`：`31,28,31,30,31,30,31,31,30,31,30,31`。二月恒 28。来源：SC exe `.rdata`。
- `[确认]` `fn_1400059bb`（body `0x1402ab960`，22 字节）返回 `days_in_month[month]`。
- `[确认]` `fn_1402abe70` 在 `CWorldData` 构造里调用，填两张运行时表：`DAT_1442c7590[month] = 月首 doy`，`DAT_1442c75c0[doy] = month`。来源：`CWorldData.cpp`、`fn_1402aad20.cpp`。
- `[确认]` 月首 doy 因此是 `0,31,59,90,120,151,181,212,243,273,304,334`。`day = doy - month_start[month]`，0-based。
- `[确认]` **没有闰年**。打包与月长都不看 4 的倍数。
- `[确认]` 世界当前打包从 `fn_140012c79`（body `0x1402a8cf0`，11 字节 `lea` 全局）返回的对象 `+0x10` 读出。`fn_14000da58` 用它构造 `SCalendar`；`fn_140005736` 构造 `SDate`；`fn_140007e69` 拆成 `nMonth,nDay,nTime`。
- `[未知]` 开局打包的非零立即数。二进制里**没有** `1522×365×48`。算法纪元年是 0。rdata 有一组被 `0xFFFFFFFF` 切开的历史年（含 1521、1522…），**未**接到 `SDate` 构造，禁止当成开局年。
- `[确认]` 该 `+0x10` **就是** `CWorldData+0x10`。单例 `0x1442a7b40` 由静态初始化 `0x14002ac85` 调 `CWorld` 构造（body `0x1402a6af0`），后者先调 `CWorldData` 构造（`0x1402ab260`），即 `CWorld : CWorldData`，打包在基类 `+0x10`。`CWorldData::vfunc_07`（body `0x1402abf30`）序列化顺序：`+0x10` u32、`+0x14` u8、`+0x18` u32、`+0x1c/+0x1d/+0x1e` u8。故打包是存档里 `CWorldData` 段的第一个 u32。
- `[确认]` 气候计时器与日历同一时间单位：`fn_14000592a`（body `0x1402ac140`）写 `+0x14` 后，`+0x18 = +0x10 + d`，`d` 缺省（传 `-1`）为 `(rand(5)+10)*48`，即 10–14 天后的打包时刻。该计时器归气候文档；重构里日历只提供 `packed`，气候自存「下次换天气的 tick」。

### 2.3 `SCalendar+0x1c` 三态（不是十日旬）

构造 `0x1402ab2f0`：

1. 先按打包拆 `SDate`，把打包写入 `+0x18`。
2. `+0x1c = 1`。
3. 若 `nTime==0` **且** `day==0`：`+0x1c = 0`。
4. 否则若 `nTime==0x2F`（47）**且** `day == days_in_month[month]-1`：`+0x1c = 2`。

- `[确认]` 事件文档写的「日 `== 0x2F`」是 **`nTime`（`SDate+0x14`）**，不是月内日（`+0x10`）。47 是一天的最后一拍。
- `[确认]` SC `mesfile_sc.txt` **没有**「旬 / 上旬 / 中旬 / 下旬」。三态是月初第一拍 / 其它 / 月末最后一拍，不是十日一段。
- `[确认]` `fn_14001374b`（body `0x1400f8ca0`）：仅当 `+0x1c==0` 且 `month % 6 == 0` 返回真。即 0 月与 6 月（内部）的月初，是月首的子集。两个用处：① 一拍调度里与 `fn_14000e9da`（月首）并列作 `CForceElement::vfunc_04` 的门，因是子集而冗余（§2.4 表第 6 行）；② `CFleetElement::vfunc_04` 用「下一拍」快照问它（`packed+1` 只是局部前瞻，不写回，见 §2.4）。②的玩法用途 `[未知]`。
- `[确认]` 另两个调度谓词：`fn_14000906b` = `nTime == 0`（日首拍）；`fn_140014dd0` = `phase == 0 && month % 12 == 0`（年首）。`fn_14000f8b7` = `phase == 2`（月末）、`fn_14000e9da` = `phase == 0`（月首）。

### 2.4 日历怎么走

- `[确认]` 打包 `+1` = 前进 1 拍 `nTime`。`nTime` 从 47 再加会进一天（算术进位，没有单独的 wrap 函数）。
- `[确认]` **唯一的推进点**：`fn_140012049`（body `0x1402a7780`）。若 `fn_140004ead(0x144296500)`（`[+0x88]==1`，游戏运行态门）为真：`world+0x10 += 1`（`inc eax` 回写），随即调 `fn_14000277f(world)` 做全世界一拍调度，再 `fn_140011414(fn_140002900())`（RTC `eFleetShip,eFleet`，舰队收尾）。`fn_14000f871`（body `0x1402a7750`）是对单例的包装。
- `[确认]` `fn_140006555`（body `0x1402ab210`）：仅当 `packed==0` 时调一次 `fn_14000277f`，**不**加 1（开局第 0 拍的调度）。
- `[确认]` 世界一拍调度 `fn_14000277f`（body `0x1402aad80`）：`SCalendar(packed)` 拷到全局 `0x1442a7b10`，然后按谓词分发到各 Element 的 `vfunc_04`：

| 顺序 | 条件 | 目标（`world+偏移`） | 类 |
| --- | --- | --- | --- |
| 1 | 日首 `nTime==0`（`fn_14000906b`） | `+0x08` | `CWorld::vfunc_04`（气候 `sClimateStreamWind`） |
| 2 | 每拍 | `+0x20` | `CFleetElement::vfunc_04` |
| 3 | 月末 `phase==2`（`fn_14000f8b7`） | `+0x5cd8` | `CItemElement::vfunc_04` |
| 4 | 日首 | `+0x96a8` | `CPersonElement::vfunc_04` |
| 5 | 日首 **或** 月末 | `+0xc340` | `CTownElement::vfunc_04` |
| 6 | 月首 `phase==0`（`fn_14000e9da`）或半年首（`fn_14001374b`，是月首子集） | `+0xf268` | `CForceElement::vfunc_04` |
| 7 | 日首 | `+0x182a0` | `CSchemeElement::vfunc_04` |
| 8 | 月首 | `+0x19f48` | `CRequestElement::vfunc_04` |
| 9 | 年首 `phase==0 && month%12==0`（`fn_140014dd0`） | `+0x184e8` | `CTradingItemElement::vfunc_04` |
| 10 | 月首 | `+0x184e8` | `fn_1400108fc`（同对象月处理） |

- `[确认]` `CFleetElement::vfunc_04(this, const SCalendar* cal)`：第二参是日历快照。它**不**写回打包；`0x140100080` 处是 `SCalendar tmp(cal->packed + 1)`（局部前瞻一拍）再问 `fn_14001374b`。旧稿「`*(obj+0x18) += 1`」是反编译把读 `+0x18` 加 1 传参误显示成自增，已更正。
- `[确认]` 海域图补给：`fn_140004101`（body `0x1402158b0`）先 `fn_140012c79` + `fn_140007e69`，**仅当 `nTime==0`** 才重算补给。来源：该函数 RTC `nTime,nDay,nMonth,anSupplyAmount`；`06-ocean/01-sailing.md`。
- `[确认]` 航海推日历：`COceanScene::vfunc_02`（body `0x1401a5f90`）内层循环 1 或 2 次（`fn_140012625` 为真则 2）；每次迭代在 `0x1401a62b7` 调 `fn_14000f871`，即 **每迭代 1 拍**。跳过条件：`fn_140006c12(...)==1` 且 `fn_1400144ac` 为假。故加速档 = 每帧 2 拍，日历同样加倍。`CSeaAreaScene::vfunc_02`（`0x140214213`）也调 `fn_14000f871`。
- `[确认]` 「过 n 天」原语 `fn_14000b122`（body `0x14026b8a0`）：外层 `n` 次 × 内层 `0x30` 次 `fn_14000f871`，每天后刷新一次界面。即 `add_days(n)` 是 **48n 次完整世界调度**，不是直接 `packed += 48n`。调用者 16 处，`n` 立即数：`CTrade::CommitDeal` 1、修理 `fn_14000c847`（RTC `ulRepairTotalCost`）1、`fn_140011cac`（`ulRepairCost`）1、`fn_14000e840`（城门仪式分支，`10-mappoint-kinds.md` §2.3）4、`fn_140007531`（`ulPrice,cSelectShipPalam`）取变量 `[rsp+0x404]`，其余（投资 `eTownShare`、`aeForce`、旅馆每晚 `fn_14000af74` 等）均为 1。`CDeckRemodelView::vfunc_13` 另经 `fn_1400103b6` 调 `fn_14000f871`。
- `[确认]` 旅馆文案「住１晚１枚金币，要住几天？」（`0xbab7`）、「住%s天吗？」（`0xbad3`）。修理「还得等一天」、改造「要等２天」。`CInn` 是空壳。修理 / 交易确实走 `fn_14000b122`（见上）。旅馆 `[确认]`：每晚 `fn_14000af74`（body `0x140160ae0`）调 `fn_14000b122(…, 1)`，长住 3/10/30 晚就循环这么多次（`08-town-life/05-inn-church-palace.md`）。
- `[确认]` HUD `CRightFrameSprite::vfunc_04` 读单例拆月日，缓存 `+0x60/+0x64`，变化才重画。画之前对**缓存字段本身**各 `+= 1` 再交给 `fn_140013acf`。内部 0-based，画面 1-based。副作用：缓存变成 1-based 后下帧必不等，HUD 实际每帧重画（原版小缺陷，重构用独立显示值，不改缓存）。
- `[确认]` 月日文案槽 `mesfile_sc.txt` `0xa43`：`%s月　%s日`。HUD 另有内嵌格式串（反编译成 UTF-16 乱码，逻辑是两个 `%2d` + 月/日）。**画面不显示年**。

### 2.5 生日

- `[确认]` 开局「请输入生日。」`mesfile_sc.txt` `0xbab`。视图 `CDatePadView`（`trade/CDatePadView.cpp`，体积 `0x250`）。`fn_140008067` 创建它；`fn_14000be97` 把一对指针的值写进视图 `+0x144/+0x148`，确认后再写回。
- `[确认]` `CPlayerStatusView::vfunc_13` 分支 `0xa`：从视图 `+0x158/+0x15c` 取出月日，打开同一块日期垫板。来源：`CPlayerStatusView.cpp` RTC `nMonth,nDay`。
- `[确认]` `CPlayerPerson+0x63`、`+0x64` 各一字节，随 `vfunc_07` 存档。`fn_1400022b6` = `movzx eax, [rcx+0x63]`；`fn_140003e18` = `movzx eax, [rcx+0x64]`。
- `[确认]` 时间触发里若流中的月 `== 12`，改写成 `fn_1400022b6`（人物 `+0x63`）；若日 `== 31`，改写成 `fn_140003e18`（`+0x64`）。来源：`fn_14000fe52`。故这两字节进入与世界日历相同的 `month/day` 比较。`[推断]` 与 `SDate` 一样是 0-based 月日，即生日。
- `[确认]` `CPlayerPerson::vfunc_11` 把 `+0x63/+0x64` 交给 `fn_1400034b3`。该函数按月查 12 槽表，日大于阈值则月+1，月 `>= 11` 则回到 0。这是生日 → 黄道档，**不是**航海日历在走黄道。cusp 表在 `.data`，映像里为 0，运行时才填，**阈值未抽出**。黄道枚举、十二宫表（VA `0x140688970`）与 `Zodiac.dk4` 归 characters（事实留在 `04-climate-weather-zodiac.md`）。
- `[推断]` `SGameSeed+0x58/+0x5c` 是开局过程中的月日（复制构造会拷这两槽）。种子其它字节见剧本文档。

### 2.6 经纬度

转换函数 **`fn_14000a6fa(pos, &nLongitude, &nLatitude)`**（body `0x14013fe70`，258 字节）。输入 `int32 x,y`（Locator 两点：`[pos], [pos+4]`）；第二参（`rdx`）收经度，第三参（`r8`）收纬度。SC 字节：

```
lat_abs = |y - 10000| * 9  / 1000     # 0x2710, imul 9, idiv 1000
lon_abs = |x - 20000| * 18 / 2000     # 0x4E20, imul 18, idiv 2000
nLatitude  = (y < 10000) ? -lat_abs : lat_abs
nLongitude = (x < 20000) ? -lon_abs : lon_abs
```

`idiv` 向 0 截断。`y==10000` / `x==20000` 为 0，不走取负支。

化简（同尺度 `9/1000`）：

```
nLatitude  = (y - 10000) * 9 / 1000    # 南为正
nLongitude = (x - 20000) * 9 / 1000    # 东为正
```

- `[确认]` 原点世界坐标 `(20000, 10000)` 显示为纬度 0、经度 0。x 增大 → 东；y 增大 → 南（屏幕/地图向下）。**禁止把这个原点叫做格林尼治**；代码没有子午线名字。
- `[确认]` 黑块投影分母 `40000` 与 `20000`（`06-three-maps.md` `MulDiv(..., 0x9c40)` / `0x4e20`）与本公式的全经度 360°×(1000/9)=40000、纬度原点 10000 是同一组立刻数。那是贴图比例尺，不是 2500 网格宽。
- `[确认]` 包装 `fn_14000625d`：对对象取点（城镇情报传 `obj+8`，即 Locator）再调 `fn_14000a6fa`。
- `[确认]` 城镇情报 `fn_140005f51`（body `0x140269810`）RTC `rcLongitudeLatitude,nLatitude,nLongitude`。格式串 **`(%s%03d %s%03d)`**。`fn_140001f8c`（12 字节）是 `abs`。参数顺序：纬度半球、纬度绝对值、经度半球、经度绝对值。
- `[确认]` SC `.rdata` `DAT_14055cfc4` 起四个 GBK 字（后跟 00 00）：

| 符号 | GBK | 字 | 使用 |
| --- | --- | --- | --- |
| `DAT_14055cfc4` | `B6 AB` | 东 | `nLongitude >= 0`（`fn_140005f51`） |
| `DAT_14055cfc8` | `CE F7` | 西 | `nLongitude < 0` |
| `DAT_14055cfcc` | `C4 CF` | 南 | `nLatitude >= 0` |
| `DAT_14055cfd0` | `B1 B1` | 北 | `nLatitude < 0` |

- `[确认]` `fn_140010b22`（body `0x14013ffc0`）同一转换、同一四个字，但判 `> 0` 才取东/南：经度 0 走西、纬度 0 走北。与城镇情报 `>= 0` → 东/南 **不一致**。重构默认跟城镇情报（0 度标东、南），把另一套标成 `zero_sign: inclusive_negative` 可配，不要两套写死在 C++。
- `[确认]` `fn_140010b22` 用**另一个格式串** `DAT_14055cfd8` = GBK `%s纬%3d°%s经%3d°`（空格补位，不是 `%03d`），参数同序（纬半球、|纬|、经半球、|经|），写入全局缓冲 `0x14429d680`。经 `fn_14000bf55`（body `0x140150dc0`，取对象坐标后转调）被 5 处使用：发现物/情报位置文案 `fn_1400092fa`（RTC `sItemPos,…,as_szEastWestName,as_szNorthSouthName`）、村落位置 `fn_14000bcf3`（`sVillagePos`）、`CRouteMap::vfunc_45`、`fn_140160dc0`（`eItem`）、`fn_140014763`。故原版有两套经纬显示：城镇情报 `(北051 东000)`，其余 `北纬 51°东经  0°`。
- `[确认]` `%03d` 是整数度，不是度×N 再除。伦敦 `(20080,4272)` → 北 051、东 000。杭州 `(33440,6640)` → 北 030、东 120。委拉克鲁斯 `(9248,7808)` → 北 019、西 096。江户 `(35520,6048)` → 北 035、东 139。罗安达 `(21456,11040)` → 南 009、东 013。来源：公式 + `Docs/_data/towns_world0.json`。
- `[确认]` `fn_14000f218`（13 个矩形）、`fn_140009aca`（8 个矩形）把 `nLatitude/nLongitude` 与立刻数矩形比较（15/30/45/60/90/105/120/150/180…，闭区间）。唯一调用者 `fn_14001266b`（body `0x1401b4d20`，RTC `apfnSearchEvent,cptItem,eItem,posPlayerFleet`），即海上「搜索」事件表的区域判定。这是**度空间里的 AABB 消费者**，区域名未还原，不进本系统内容表；重构由搜索系统持有自己的 `lat/lon` 矩形表，调 `Coords.to_deg`。
- `[确认]` `fn_1400f55d0`（2196 字节，RTC `nLatitude,nLongitude`）内部调用 `fn_14000a6fa`；第一参（happening 编号）`> 0x13` 即失败返回，否则进 20 槽跳表（`0x1400f567c cmp …,0x13`），即海上 happening 映射函数（`06-ocean/05-sea-happenings.md`）。唯一调用点 `0x1400fb7bc` 在 `fn_140005a15`（body `0x1400fa870`，RTC `eNewFleetState,…,nLatitude,nLongitude,anSupply`）里；`fn_140005a15` 是**玩家舰队日步**（经 `fn_14000641f` 调用；`tick.yaml` 的 `daily` phase），不是 NPC 舰队状态机（旧稿误称）。本篇只认它是经纬消费者。

### 2.7 事件用的时间坐标

- `[确认]` `fn_140013c32`：`fn_14000da58` 取当前 `SCalendar`，按 `CEvent+0x88` 的 kind 扫触发器，`fn_140001b09(event, trigger, calendar)`。来源：`10-narrative/02-event-system.md`。
- `[确认]` 时间条件解码 `fn_14000fe52`（body `0x1400c8140`，RTC `sWorldDate,sCheckDate`）：从触发流抽 year/month/day/nTime，做成 `SDate`，再与世界 `SDate` 比。
- `[确认]` 通配：`year >= 99` → `0xFF`；`month > 12` → `0xFF`；`day > 31` → `0xFF`；`nTime >= 48` → `0xFF`。`0xFF` 在相等 / 排序里跳过该字段。
- `[确认]` 哨兵：`month == 12` → 主角生日月；`day == 31` → 主角生日日。不是「任意月/日」（任意是 `>12` / `>31` → `0xFF`）。
- `[确认]` `fn_1400c49c0`：四字段相等（`0xFF` 通配）。`fn_14000fe52` 命中返回 1。
- `[确认]` `fn_1400c85e0(world, check)`：按 年→月→日→拍 逐字段，任一方 `0xFF` 跳过；首个不等字段 `world > check` 返 1、`world < check` 返 0；全等返 1。即 **`world >= check`（已到或已过）**。`fn_14000fe52` 在相等失败后用它：真 → 返回 **2（已过）**，否则 0（未到）。旧稿写成 `world < check` 方向反了。重构接口暴露 `match / passed / pending`，不要把 1/2 写进内容包。

kind 0–8 与 `SEventTriggerTime` 的槽位号仍 `[未知]`，见事件文档。本系统不解释 kind。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 |
| --- | --- | --- | --- |
| 打包时刻 | `uint32 packed` | 0 = 纪元年 1 月 1 日第 0 拍（算法上合法） | `tick` |
| 快照 | `SCalendar` / `SDate` | 无 INVALID 符号 | `CalendarSnapshot` |
| 年内日 | `doy` 0–364 | — | `day_of_year` |
| 月 | 0–11 | 触发 12=生日月，>12=通配 | `month` |
| 日 | 0 .. month_length-1 | 触发 31=生日日，>31=通配 | `day` |
| 日内拍 | `nTime` 0–47 | ≥48 在触发里 = 通配 | `time_of_day` |
| 月界三态 | `SCalendar+0x1c` 0/1/2 | — | `month_phase`：`start` / `mid` / `end` |
| 触发通配 | `0xFF` | — | `any` |
| 经纬度 | `nLatitude` `nLongitude` 有符号整数度 | — | `lat_deg` `lon_deg` |
| 半球 | 东/西/南/北 四字 | — | `hemi.e/w/s/n` 文案 key |
| 生日 | `CPlayerPerson+0x63/+0x64` | 未输入 `[未知]` | `birthday.month/day`（人物状态，本系统只定义编码） |
| 黄道 | `eZodiac` | `null`（原版 -1） | characters；本系统不持有 |

没有 `eYear` 枚举。运行时不要写死 `NUM_MONTH=12`，读表长。

## 4. 算法 / 公式

全部立刻数进 `config/formulas/world.yaml`（每日拍数进 `caps.yaml`）。禁止在引擎里写 `48`、`365`、`10000`。

```
# ticks_per_day 是式内局部名，取自 $caps.calendar_ticks_per_day
# 打包
packed = ((year * days_per_year + doy) * ticks_per_day) + time_of_day
year         = packed / (days_per_year * ticks_per_day)
time_of_day  = packed % ticks_per_day
doy          = (packed / ticks_per_day) % days_per_year
month        = doy_to_month[doy]
day          = doy - month_start[month]

# 前进（Clock 执行；add_days_mode 决定是否逐拍跑调度）
advance_ticks(n) = packed + n
advance_days(n)  = packed + n * ticks_per_day

# 月界（构造 SCalendar 时）
month_phase =
  start  if time_of_day==0 and day==0
  end    if time_of_day==time_of_day_max and day==month_length[month]-1
  mid    otherwise

half_year_start = (month_phase==start) and (month % half_year_months==0)   # 派生谓词，不单独发事件

# 显示月日
ui_month = month + ui_month_bias    # 原版 1
ui_day   = day + ui_day_bias        # 原版 1

# 经纬（向 0 截断的整数除）
lat_deg = (y - origin_y) * lat_num / lat_den
lon_deg = (x - origin_x) * lon_num / lon_den
# 显示绝对值 + 半球：lat_deg<0 北，>0 南；lon_deg<0 西，>0 东
# 0 度默认：东、南（城镇情报 >= 0）

# 触发（只在 EV 字节码导入器里用；规范形式见下）
decode_field(v, sentinel_birthday, wildcard_min):
  if v == sentinel_birthday: return BIRTHDAY
  if v >= wildcard_min:      return ANY
  return v
```

| 参数 | 原版 | 落点 |
| --- | --- | --- |
| `calendar_ticks_per_day` | 48 | `caps.yaml`（式内写 `$caps.calendar_ticks_per_day`） |
| `days_per_year` | 365 | `formulas/world.yaml` |
| `month_lengths` | 31,28,31,30,31,30,31,31,30,31,30,31 | `calendar.yaml` |
| `leap_years` | false | `calendar.yaml` |
| `time_of_day_max` | 47 | 派生 `calendar_ticks_per_day-1` |
| `ui_month_bias` / `ui_day_bias` | 1 | `formulas/world.yaml` |
| `origin_x` / `origin_y` | 20000 / 10000 | `coords.yaml` |
| `lat_num` / `lat_den` | 9 / 1000 | `coords.yaml` |
| `lon_num` / `lon_den` | 18 / 2000 | `coords.yaml`（与 9/1000 相等，仍分写，禁止实现里合并后改一边） |
| `trigger_year_wildcard_min` | 99 | `legacy_codec`（EV 导入） |
| `trigger_month_birthday` | 12 | 同上 |
| `trigger_day_birthday` | 31 | 同上 |
| `trigger_any` | 255 | 同上 |
| `half_year_months` | 6 | `calendar.yaml` |
| `coord_format` | `(%s%03d %s%03d)`（城镇情报）/ `%s纬%3d°%s经%3d°`（发现物、村落、航路图） | `ui` 包；逻辑只出四个值 |

本系统没有玩法随机。黄道 cusp 表未抽出，`Zodiac.from_birthday` 归 characters，缺表则返回 `null`，禁止猜 12 宫日期。

### 已还原 / 未还原

**已还原**

- 打包 ↔ 年/doy/`nTime`
- 月长、月首、doy→月，无闰年
- `SCalendar+0x1c` 谓词；`0x2F` = 最后一拍
- 内部月日 0-based、HUD +1
- 经纬公式、原点、半球字、`%03d`、`abs`
- 触发通配、生日哨兵 12/31、相等 / 已过 / 未到
- 推进点 `fn_140012049` 与一拍调度表（第 2.4 节）
- 出海每迭代 1 拍、加速档 2 拍；`add_days(n)` = 48n 次调度；旅馆每晚 1 天
- 打包 = `CWorldData+0x10`，存档段首 u32
- 生日两字节在 `CPlayerPerson` 上并参与触发与黄道函数
- 海域图 `nTime==0` 才重算补给
- 年 = `packed / 17520`

**未还原**

- 画面上的「1522 年」类纪元偏移（若有）
- 打包在 `savedatXX.dk4` 的绝对文件偏移（段内顺序已知）
- 改造「２天」对应的 `n`
- 开局把 `packed` 设成非零的写入点（`CWorldData::vfunc_05` 未见写 `+0x10`）
- `fn_14000f218` / `fn_140009aca` / `fn_1400f55d0` 的区域名
- 黄道 cusp 12 个阈值
- `CDatePadView` 对日是否按月长夹紧（构造里只有像素立刻数）
- 原点的地理名称（**不要猜**）

## 5. 耦合分析

原版焊点：

- `CWorldData` 构造顺手初始化月表；气候计时器 `+0x18` 直接用打包时刻（`+0x10 + 天数*48`）。
- 一拍调度 `fn_14000277f` 把「日首 / 月首 / 月末 / 年首」写死成 if 链，按 `world+偏移` 直接调各 Element。
- 事件扫描把 `SCalendar` 整对象推进求值函数。
- 城镇情报、HUD、海域补给各自再拆一遍打包 / 经纬。
- 时间触发直接读主角 `+0x63/+0x64`。
- 黄道函数与气候随机、`Zodiac.dk4` 落在同一片 unassigned 编译单元（所有权实为 characters）。
- `CDatePadView` 放在 `trade/` 只因原版工程切法，不是交易公式。
- `CFleetElement::vfunc_04` 拿日历快照做舰队/势力逻辑，并自建「下一拍」快照问半年首。
- 城镇操作用 `fn_14000b122` 跑 48n 次完整世界调度来「过天」，所以交易/修理会触发 NPC、市价、事件。

重构切断：

- 引擎 `Clock` 拥有打包时刻与推进；world 只出 `Calendar`（编码 / 谓词纯函数）与 `Coords` 两个无指针接口。
- 原版写死的 if 链改成 `config/engine/tick.yaml` 的阶段序列：每个系统声明 `schedule: tick | day_start | month_start | month_end | year_start`，阶段顺序写在表里，不靠订阅注册顺序（`00-overview/05-config-architecture.md`「tick 顺序」）。原版顺序见 §2.4 表。
- 同一门内多系统的先后（例：月初 town 状态 → 市价漂移 → 逐港〔投资转化 → 补货〕；月末先 items 阶段出租费、再 towns 阶段独占上缴，词表 §11 / §14-17）也写进 `tick.yaml`，由各系统文档给出原版顺序。
- 日首的 `daily` phase（`tick.yaml`：`schedule: [day_start]`，**不限** `at_sea`；旧名 `DailyTick`，不是事件）按原版 `fn_140005a15` 分两支（词表 §14-18）：在港分支只做疲劳恢复（旅馆每夜同样触发）；海上分支步骤顺序以词表 §11 与 `06-ocean/01` 为准：补给 → 疲劳 → 人物 HP / 身心 → 海上事件。经验在其后的 `duty_exp` phase（`every_n_days: 3`，`when: at_sea`）。
- narrative 订阅 `DayStarted` / `MonthStarted` 等事件，按需查快照；不读 `month_phase` 字段做判断。
- ocean 每个航行迭代调 `Clock.advance_ticks(1)`（原版行为）；补给按 `schedule: day_start`。
- town-life 调 `Clock.advance_days(n)`，不自己改 packed。
- characters 拥有生日字节与黄道；本系统只定义编码，触发解码时 **查询** `Characters.birthday(person_id)`。

## 6. 解耦后的配置

`config/world/calendar.yaml`：

```yaml
schema: dk4.world.calendar
version: 2
pack: vanilla-core
# 每日拍数不在本表：取 $caps.calendar_ticks_per_day（48）
days_per_year: 365
leap_years: false
months:                        # 顺序即月序（内部 0-based）
  - { key: vanilla:jan, length: 31 }
  - { key: vanilla:feb, length: 28 }
  - { key: vanilla:mar, length: 31 }
  - { key: vanilla:apr, length: 30 }
  - { key: vanilla:may, length: 31 }
  - { key: vanilla:jun, length: 30 }
  - { key: vanilla:jul, length: 31 }
  - { key: vanilla:aug, length: 31 }
  - { key: vanilla:sep, length: 30 }
  - { key: vanilla:oct, length: 31 }
  - { key: vanilla:nov, length: 30 }
  - { key: vanilla:dec, length: 31 }
epoch:
  year0_display: null          # 画面不显示年；禁止填 1522 冒充已还原
  packed_at_new_game: 0        # 未找到非零立即数；0 是算法起点不是「公元 0 年」
ui:
  month_bias: 1
  day_bias: 1
  date_string_key: mes_0xa43   # "%s月　%s日"
  show_year: false
phase:
  start: { time_of_day: 0, day: 0 }
  end:   { time_of_day: 47, day: last }
  mid:   other
half_year_months: 6
add_days_mode: simulate_each_tick   # 原版 fn_14000b122：48n 次完整调度；可选 jump（只改 packed）
```

调度表不在本文件：阶段序列在 `config/engine/tick.yaml`（`00-overview/05-config-architecture.md`），原版包照抄 §2.4 表。日历只给门谓词 `tick / day_start / month_start / month_end / year_start`（`half_year_start` 是 `month_start` 的派生子集，系统在自己的处理里判，不另设门）。

时间触发的规范形式（叙事内容包用；`any` / `birthday` 是枚举，不是数值）：

```yaml
# 例：narrative 触发器里的时间条件
when: { year: any, month: birthday, day: birthday, time_of_day: any, compare: reached }   # reached = 原版 passed|match
```

原版 EV 字节码里的数值哨兵只在导入器（`config/ids/legacy_codec.yaml` 的 `ev_time` 段）里出现：

```yaml
ev_time:
  any: 255                       # 0xFF；year>=99 / month>12 / day>31 / nTime>=48 解码成 any
  year_wildcard_min: 99
  month_birthday_sentinel: 12
  day_birthday_sentinel: 31
  compare_result: { match: 1, passed: 2, pending: 0 }   # fn_14000fe52 返回值
```

`config/world/coords.yaml`：

```yaml
schema: dk4.world.coords
version: 1
pack: vanilla-core
space: original                 # 与 map.yaml 同一空间
origin_x: 20000
origin_y: 10000
latitude:
  num: 9
  den: 1000
  positive: south               # y 增大 → 南
longitude:
  num: 18
  den: 2000
  positive: east                # x 增大 → 东
name_origin: null               # 禁止填格林尼治 / Greenwich
display:
  format_key: ui.coord.latlon   # "(%s%03d %s%03d)"
  width: 3
  zero_sign: nonnegative_east_south   # fn_140005f51；fn_140010b22 是 positive_only_east_south
  styles:                       # 原版两套显示，按调用点选
    town_info: { format_key: ui.coord.latlon_short, pad: zero, zero_sign: nonnegative_east_south }  # (%s%03d %s%03d)
    report:    { format_key: ui.coord.latlon_long,  pad: space, zero_sign: positive_only_east_south } # %s纬%3d°%s经%3d°
  hemi:
    east:  hemi.e               # SC「东」
    west:  hemi.w
    south: hemi.s
    north: hemi.n
span_hint:
  full_lon_units: 40000         # 与黑块 MulDiv 0x9c40 相同立刻数，不是网格宽
  origin_y_units: 10000
example:
  town: vanilla:london
  x: 20080
  y: 4272
  lat_deg: -51                  # 北 051
  lon_deg: 0                    # 东 000
```

`config/formulas/world.yaml`：

```yaml
schema: dk4.formulas.world
version: 2
# D3 形状（14-config-schemas README「公式层」）
calendar:                       # 打包 / 拆包 / 显示月日（§4）
  impl: vanilla.calendar_pack
  int_mode: vanilla_i32
  quirks: {}
  params:
    ticks_per_day: $caps.calendar_ticks_per_day   # 48；只引用，不另定义
    days_per_year: 365
    ui_month_bias: 1
    ui_day_bias: 1
  inputs: [year, doy, time_of_day]
  golden: [{ year: 1, doy: 0, time_of_day: 0, out: 17520 }]   # packed = 365 * 48
coord_lat:                      # 向 0 截断的整数除
  impl: expr
  int_mode: vanilla_i32
  params: { origin_y: 10000, lat_num: 9, lat_den: 1000 }
  inputs: [y]
  expr: "(y - origin_y) * lat_num / lat_den"
  golden: [{ y: 4272, out: -51 }]     # 伦敦 → 北 051
coord_lon:
  impl: expr
  int_mode: vanilla_i32
  params: { origin_x: 20000, lon_num: 18, lon_den: 2000 }
  inputs: [x]
  expr: "(x - origin_x) * lon_num / lon_den"
  golden: [{ x: 20080, out: 0 }]      # 伦敦 → 东 000
```

`config/caps.yaml` 片段：

```yaml
calendar_ticks_per_day: 48
calendar_days_per_year: 365
calendar_months: 12
coord_deg_abs_max: 180          # 显示用；原版 %03d 放得下 180
```

生日月日是人物状态，定义不进本表。内容包可给默认生日，开局被垫板覆盖。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗改代码：

- 月长、是否闰年、每年天数、每日拍数
- HUD 是否 +1、是否显示年、纪元显示偏移（一旦还原）
- 经纬原点与两个分数；0 度算东还是西
- 半球文案
- 时间触发的 `any` / `birthday` 语义（原版数值哨兵只在导入器）
- `half_year_months`

改补给扣在哪一拍 → 只动 `tick.yaml` 里 ocean 的 `schedule`。改「过天」是否逐拍模拟 → 只动 `add_days_mode`。改某系统的结算频率或先后 → 只动 `tick.yaml`。改旅馆是否推日 → 只动 town-life 发不发 `advance_days`。改黄道阈值 → 只动 characters 包。

## 8. 对外接口

查询（LIVE；取不到则调用方 no-op）：

```
Clock.now() -> packed u32
Calendar.snapshot(packed = Clock.now()) -> {
  packed, year, month, day, time_of_day, day_of_year, ui_month, ui_day
}
Calendar.is(gate, packed) -> bool     # gate: day_start | month_start | month_end | year_start；Clock 调度用
Coords.to_deg(x_original, y_original) -> { lat_deg, lon_deg }
Coords.format(lat_deg, lon_deg, style) -> { hemi_lat, abs_lat, hemi_lon, abs_lon }
Characters.birthday(person_id) -> { month, day } | null
```

`month_phase` 只是 `Calendar` 内部求门的中间量，不对外暴露；系统要「月初做事」就在 `tick.yaml` 里声明 `schedule: month_start`。

命令（`Clock`）：

```
Clock.set(u32)                    # 读档 / 调试；玩法走 advance_*
Clock.advance_ticks(n)
Clock.advance_days(n)             # n * ticks_per_day；add_days_mode 决定是否逐拍调度
```

事件（`Clock` 在阶段边界发）：

```
TickAdvanced { packed }                    # 只给 UI；系统按 schedule: tick 跑
DayStarted { packed, month, day }          # time_of_day==0
MonthStarted { year, month }               # phase==start
MonthEnding { year, month }                # phase==end（该月最后一拍）
YearStarted { year }
```

旧名 `CalendarStep` → 系统侧是调度门 `schedule: tick`、UI 侧是 `TickAdvanced`（叙事触发 kind 5 / 6 的事件源是 06 的 `TownTick` / `SeaTick`，词表 §14-6），`MonthEnded` → `MonthEnding`，`DayAdvanced` → `DayStarted`，`HalfYearStart` 取消（订阅 `MonthStarted` 后判 `month % half_year_months == 0`）。

禁止把 `SCalendar*` / `CWorld*` 传出。存档：`Clock` 的 `packed` 一个 u32（原版 `CWorldData+0x10`）+ 人物生日两字节（人物节）。月表、原点不存。

触发比较（叙事调用，日历提供纯函数）：

```
Calendar.compare(now, when) -> match | passed | pending   # 字段为 any 时跳过；birthday 先代入主角生日
```

## 9. 未核实字段

| 项 | 位置 | 现状 | 禁止 |
| --- | --- | --- | --- |
| 开局年 / 画面年 | 无 `1522×365×48` | `[未知]` | 不要写死 1522 |
| rdata 1444–1560 年表 | `0x14055f1xx` | 被 −1 切开，未接 `SDate` | 不要当开局年 |
| 打包存档文件偏移 | `CWorldData` 段首 u32 `[确认]` | 绝对文件偏移未算 | 不要两家各写一份 |
| 旅馆推日 | 旅馆走 `fn_14000b122`（`08-town-life/05-inn-church-palace.md`：`0x14026b8a0` n×0x30） | 修理/交易/旅馆已钉 | 不要绕过 `Clock.advance_days` |
| 旬 | mesfile 无此字 | 三态是月界拍 | 不要写成十日旬 |
| `fn_14000f218` / `fn_140009aca` 矩形名 | 整数度 AABB | `[未知]` | 不要标成海域 / 黄金航路 |
| `fn_1400f55d0` | 2196 B，调 `fn_14000a6fa` | 反编译失败 | 不要当第二套转换公式 |
| 0 度东 vs 西 | 两个 UI 函数符号相反、格式串也不同 | `display.styles` 分开配 | 不要各画各的 |
| 黄道 cusp 12 值 | `.data` 映像为 0 | `[未知]` | 不要抄西洋占星日期 |
| `CDatePadView` 一串 `0x1fe,5,0x2b1…` | 构造 | UI 像素 | 不进玩法表 |
| `SGameSeed` 除 `+0x58/+0x5c` | `0x78` | 剧本文档 | 不要把整个种子当生日 |
| 原点地名 | 无字符串 | — | **不要写格林尼治** |
| 原版单位 → 2500 网格 | `/16` 向零截断（`01`） | 代码 `[确认]`；与港口同空间 `[推断]` | 禁止 `/8`；与经纬公式无关 |

提取器：月长表、原点、两个分数、四个半球字、格式串进 YAML；cusp、开局年、存档偏移进 `unknown`。
