# 日期与经纬度

本系统拥有两件正交的世界标尺：**时间坐标**（打包日历、日步、事件用的 `SCalendar` 快照）和 **经纬度显示**（原版定位单位 → 整数度）。两者都从世界对象上读，都不拥有港口表、网格通行、风向场。

叙事只查询 `Calendar.snapshot()`，不自己算月末。航海只在日历步上扣补给。经纬度是投影，不是 `World.dat` 下标。

配置落点：`config/world/calendar.yaml`、`config/world/coords.yaml`；立刻数进 `config/formulas/world.yaml` 与 `config/caps.yaml`。

## 1. 职责边界

管：

- 打包日期的编码 / 解码（年、月、日、日内拍 `nTime`）
- 月长表、无闰年、日内拍数
- `SCalendar` 快照（含 `+0x1c` 三态）以及事件触发里的时间通配
- 原版定位单位 → 显示用整数经纬度
- 经纬度 UI 格式与半球文案 key
- 开局生日（月日）作为人物状态的写入入口；黄道只**查询**生日，算法在气候文档

不管：

- `World.dat` 2500 网格与原版单位的换算（`01-map-and-sea-areas.md`；禁止擅自 `/8`）
- 三种地图怎么画（`06-three-maps.md`）。该篇里的 `40000` / `20000` 分母与本篇原点是同一组立刻数，本篇只拥有度换算
- 风向场、天气档、`Zodiac.dk4` 资源（`04-climate-weather-zodiac.md`）
- 旅馆收几金、修船等几天（town-life 发 `Calendar.add_days`；是否真推进日历见第 9 节）
- 水粮 / 疲劳怎么扣（ocean 订阅 `CalendarStep`）
- 月结上缴金额、品牌「今年」（economy；本系统只发 `MonthStarted` / `YearStarted`）
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
ticks_per_day = 48          # 立即数 0x30
days_per_year = 365         # 立即数 0x16D
nTime = packed % 48
doy   = (packed / 48) % 365
year  = packed / (365 * 48)
```

- `[确认]` 年可以单独取：`fn_140002068`（body `0x1400f5440`）对对象 `+0x10` 做 `div 0x4470`。`0x4470 = 17520 = 365×48`。
- `[确认]` 月长表 `DAT_1405af730` 十二个 `int32`：`31,28,31,30,31,30,31,31,30,31,30,31`。二月恒 28。来源：SC exe `.rdata`。
- `[确认]` `fn_1400059bb`（body `0x1402ab960`，16 字节）返回 `days_in_month[month]`。
- `[确认]` `fn_1402abe70` 在 `CWorldData` 构造里调用，填两张运行时表：`DAT_1442c7590[month] = 月首 doy`，`DAT_1442c75c0[doy] = month`。来源：`CWorldData.cpp`、`fn_1402aad20.cpp`。
- `[确认]` 月首 doy 因此是 `0,31,59,90,120,151,181,212,243,273,304,334`。`day = doy - month_start[month]`，0-based。
- `[确认]` **没有闰年**。打包与月长都不看 4 的倍数。
- `[确认]` 世界当前打包从 `fn_140012c79`（body `0x1402a8cf0`，11 字节 `lea` 全局）返回的对象 `+0x10` 读出。`fn_14000da58` 用它构造 `SCalendar`；`fn_140005736` 构造 `SDate`；`fn_140007e69` 拆成 `nMonth,nDay,nTime`。
- `[未知]` 开局打包的非零立即数。二进制里**没有** `1522×365×48`。算法纪元年是 0。rdata 有一组被 `0xFFFFFFFF` 切开的历史年（含 1521、1522…），**未**接到 `SDate` 构造，禁止当成开局年。
- `[未知]` 该 `+0x10` 与气候文档里 `CWorldData+0x10` 是否同一字段。重构把日历打包放进自己的 `CalendarState`，不和气候计数器共用。

### 2.3 `SCalendar+0x1c` 三态（不是十日旬）

构造 `0x1402ab2f0`：

1. 先按打包拆 `SDate`，把打包写入 `+0x18`。
2. `+0x1c = 1`。
3. 若 `nTime==0` **且** `day==0`：`+0x1c = 0`。
4. 否则若 `nTime==0x2F`（47）**且** `day == days_in_month[month]-1`：`+0x1c = 2`。

- `[确认]` 事件文档写的「日 `== 0x2F`」是 **`nTime`（`SDate+0x14`）**，不是月内日（`+0x10`）。47 是一天的最后一拍。
- `[确认]` SC `mesfile_sc.txt` **没有**「旬 / 上旬 / 中旬 / 下旬」。三态是月初第一拍 / 其它 / 月末最后一拍，不是十日一段。
- `[确认]` `fn_14001374b`（body `0x1400f8ca0`）：仅当 `+0x1c==0` 且 `month % 6 == 0` 返回真。即 0 月与 6 月（内部）的月初。`CFleetElement::vfunc_04` 在 `packed += 1` 之后用它。用途（半年结算 / 品牌 / 上缴）`[未知]`，本系统只提供谓词。

### 2.4 日历怎么走

- `[确认]` 打包 `+1` = 前进 1 拍 `nTime`。`nTime` 从 47 再加会进一天（算术进位，没有单独的 wrap 函数）。
- `[确认]` `CFleetElement::vfunc_04` 有 `*(obj+0x18) += 1` 然后 `SCalendar(packed)`。这是世界模拟里见到的明确 `+1`。该 `obj` 是否就是 `fn_140012c79` 单例 `[推断]`，未在同一条指令钉死。
- `[确认]` 海域图补给：`fn_140004101`（body `0x1402158b0`）先 `fn_140012c79` + `fn_140007e69`，**仅当 `nTime==0`** 才重算补给。来源：该函数 RTC `nTime,nDay,nMonth,anSupplyAmount`；`06-ocean/01-sailing.md`。
- `[确认]` 航海场景每帧 1 或 2 次像素循环（`COceanScene::vfunc_02` 里 `fn_140012625` 为真则 2）。来源：`04-manual-auto-sail.md`。**未**在 `COceanScene` 里看到 `packed += 1`。加速档是否推日历 `[未知]`；ocean 文档默认不加倍日历。
- `[确认]` 旅馆文案「住１晚１枚金币，要住几天？」（`0xbab7`）、「住%s天吗？」（`0xbad3`）。修理「还得等一天」、改造「要等２天」。`CInn` 是空壳。夜数是否 `packed += 48*n` **未还原**。见 `08-town-life/05-inn-church-palace.md`：`advance_calendar: null`。
- `[确认]` HUD `CRightFrameSprite::vfunc_04` 读单例拆月日，缓存 `+0x60/+0x64`，变化才重画。画之前对月、日各 `+= 1` 再交给 `fn_140013acf`。内部 0-based，画面 1-based。
- `[确认]` 月日文案槽 `mesfile_sc.txt` `0xa43`：`%s月　%s日`。HUD 另有内嵌格式串（反编译成 UTF-16 乱码，逻辑是两个 `%2d` + 月/日）。**画面不显示年**。

### 2.5 生日

- `[确认]` 开局「请输入生日。」`mesfile_sc.txt` `0xbab`。视图 `CDatePadView`（`trade/CDatePadView.cpp`，体积 `0x250`）。`fn_140008067` 创建它；`fn_14000be97` 把一对指针的值写进视图 `+0x144/+0x148`，确认后再写回。
- `[确认]` `CPlayerStatusView::vfunc_13` 分支 `0xa`：从视图 `+0x158/+0x15c` 取出月日，打开同一块日期垫板。来源：`CPlayerStatusView.cpp` RTC `nMonth,nDay`。
- `[确认]` `CPlayerPerson+0x63`、`+0x64` 各一字节，随 `vfunc_07` 存档。`fn_1400022b6` = `movzx eax, [rcx+0x63]`；`fn_140003e18` = `movzx eax, [rcx+0x64]`。
- `[确认]` 时间触发里若流中的月 `== 12`，改写成 `fn_1400022b6`（人物 `+0x63`）；若日 `== 31`，改写成 `fn_140003e18`（`+0x64`）。来源：`fn_14000fe52`。故这两字节进入与世界日历相同的 `month/day` 比较。`[推断]` 与 `SDate` 一样是 0-based 月日，即生日。
- `[确认]` `CPlayerPerson::vfunc_11` 把 `+0x63/+0x64` 交给 `fn_1400034b3`。该函数按月查 12 槽表，日大于阈值则月+1，月 `>= 11` 则回到 0。这是生日 → 黄道档，**不是**航海日历在走黄道。cusp 表在 `.data`，映像里为 0，运行时才填，**阈值未抽出**。黄道枚举与 `Zodiac.dk4` 仍归气候文档。
- `[推断]` `SGameSeed+0x58/+0x5c` 是开局过程中的月日（复制构造会拷这两槽）。种子其它字节见剧本文档。

### 2.6 经纬度

转换函数 **`fn_14000a6fa`**（body `0x14013fe70`）。输入 `int32 x,y`（Locator 两点：`[pos], [pos+4]`），输出 `nLongitude, nLatitude`。SC 字节：

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

- `[确认]` `fn_140010b22`（body `0x14013ffc0`）同一转换、同一四个字，但 0 度用 `<= 0`：经度 0 走西、纬度 0 走北。与城镇情报 `>= 0` → 东/南 **不一致**。重构默认跟城镇情报（0 度标东、南），把另一套标成 `zero_sign: inclusive_negative` 可配，不要两套写死在 C++。
- `[确认]` `%03d` 是整数度，不是度×N 再除。伦敦 `(20080,4272)` → 北 051、东 000。杭州 `(33440,6640)` → 北 030、东 120。委拉克鲁斯 `(9248,7808)` → 北 019、西 096。江户 `(35520,6048)` → 北 035、东 139。罗安达 `(21456,11040)` → 南 009、东 013。来源：公式 + `Docs/_data/towns_world0.json`。
- `[确认]` `fn_14000f218`、`fn_140009aca` 把 `nLatitude/nLongitude` 与立刻数矩形比较（15/30/45/60/90/105/120/150…）。这是**度空间里的 AABB 消费者**，区域名未还原，不进本系统内容表。
- `[确认]` `fn_1400f55d0`（2196 字节，RTC `nLatitude,nLongitude`）内部调用 `fn_14000a6fa`；第一参与 `0x13` 比较后进跳表。反编译因格式串失败。本篇只认它是经纬消费者，不给它玩法名。

### 2.7 事件用的时间坐标

- `[确认]` `fn_140013c32`：`fn_14000da58` 取当前 `SCalendar`，按 `CEvent+0x88` 的 kind 扫触发器，`fn_140001b09(event, trigger, calendar)`。来源：`10-narrative/02-event-system.md`。
- `[确认]` 时间条件解码 `fn_14000fe52`（body `0x1400c8140`，RTC `sWorldDate,sCheckDate`）：从触发流抽 year/month/day/nTime，做成 `SDate`，再与世界 `SDate` 比。
- `[确认]` 通配：`year >= 99` → `0xFF`；`month > 12` → `0xFF`；`day > 31` → `0xFF`；`nTime >= 48` → `0xFF`。`0xFF` 在相等 / 排序里跳过该字段。
- `[确认]` 哨兵：`month == 12` → 主角生日月；`day == 31` → 主角生日日。不是「任意月/日」（任意是 `>12` / `>31` → `0xFF`）。
- `[确认]` `fn_1400c49c0`：四字段相等（`0xFF` 通配）。`fn_14000fe52` 命中返回 1。
- `[确认]` `fn_1400c85e0`：字典序 `world < check`（同样跳过 `0xFF`）。命中返回 2。否则 0。重构接口暴露 `match / before / after`，不要把 1/2 写进内容包。

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
| 黄道 | `eZodiac` | `INVALID_ZODIAC=-1` | 气候系统；本系统 `Zodiac.from_birthday` 只当查询 |

没有 `eYear` 枚举。运行时不要写死 `NUM_MONTH=12`，读表长。

## 4. 算法 / 公式

全部立刻数进 `config/formulas/world.yaml`。禁止在引擎里写 `48`、`365`、`10000`。

```
# 打包
packed = ((year * days_per_year + doy) * ticks_per_day) + time_of_day
year         = packed / (days_per_year * ticks_per_day)
time_of_day  = packed % ticks_per_day
doy          = (packed / ticks_per_day) % days_per_year
month        = doy_to_month[doy]
day          = doy - month_start[month]

# 前进
add_ticks(n) = packed + n
add_days(n)  = packed + n * ticks_per_day

# 月界（构造 SCalendar 时）
month_phase =
  start  if time_of_day==0 and day==0
  end    if time_of_day==time_of_day_max and day==month_length[month]-1
  mid    otherwise

half_year_start = (month_phase==start) and (month % half_year_months==0)

# 显示月日
ui_month = month + ui_month_bias    # 原版 1
ui_day   = day + ui_day_bias        # 原版 1

# 经纬（向 0 截断的整数除）
lat_deg = (y - origin_y) * lat_num / lat_den
lon_deg = (x - origin_x) * lon_num / lon_den
# 显示绝对值 + 半球：lat_deg<0 北，>0 南；lon_deg<0 西，>0 东
# 0 度默认：东、南（城镇情报 >= 0）

# 触发
decode_field(v, sentinel_birthday, wildcard_min):
  if v == sentinel_birthday: return player_birthday_field
  if v >= wildcard_min:      return ANY
  return v
```

| 参数 | 原版 | 落点 |
| --- | --- | --- |
| `ticks_per_day` | 48 | `formulas/world.yaml` |
| `days_per_year` | 365 | 同上 |
| `month_lengths` | 31,28,31,30,31,30,31,31,30,31,30,31 | `calendar.yaml` |
| `leap_years` | false | `calendar.yaml` |
| `time_of_day_max` | 47 | 派生 `ticks_per_day-1` |
| `ui_month_bias` / `ui_day_bias` | 1 | `formulas/world.yaml` |
| `origin_x` / `origin_y` | 20000 / 10000 | `coords.yaml` |
| `lat_num` / `lat_den` | 9 / 1000 | `coords.yaml` |
| `lon_num` / `lon_den` | 18 / 2000 | `coords.yaml`（与 9/1000 相等，仍分写，禁止实现里合并后改一边） |
| `trigger_year_wildcard_min` | 99 | `calendar.yaml` |
| `trigger_month_birthday` | 12 | 同上 |
| `trigger_day_birthday` | 31 | 同上 |
| `trigger_any` | 255 | 同上 |
| `half_year_months` | 6 | 同上 |
| `coord_format` | `(%s%03d %s%03d)` | `ui` 包；逻辑只出四个值 |

本系统没有玩法随机。黄道 cusp 表未抽出，`Zodiac.from_birthday` 的阈值进气候包，缺表则返回 `invalid`，禁止猜 12 宫日期。

### 已还原 / 未还原

**已还原**

- 打包 ↔ 年/doy/`nTime`
- 月长、月首、doy→月，无闰年
- `SCalendar+0x1c` 谓词；`0x2F` = 最后一拍
- 内部月日 0-based、HUD +1
- 经纬公式、原点、半球字、`%03d`、`abs`
- 触发通配、生日哨兵 12/31、相等 / 早于
- 生日两字节在 `CPlayerPerson` 上并参与触发与黄道函数
- 海域图 `nTime==0` 才重算补给
- 年 = `packed / 17520`

**未还原**

- 画面上的「1522 年」类纪元偏移（若有）
- 打包在 `savedatXX.dk4` 的文件偏移
- 出海每像素 tick 是否 `packed += 1`；加速档是否加倍日历
- 旅馆夜数 / 修理 1 日 / 改造 2 日是否加 `48*n`
- `CFleetElement` 那个 `+0x18` 与单例 `+0x10` 是否同一打包
- `fn_14000f218` / `fn_140009aca` / `fn_1400f55d0` 的区域名
- 黄道 cusp 12 个阈值
- `CDatePadView` 对日是否按月长夹紧（构造里只有像素立刻数）
- 原点的地理名称（**不要猜**）

## 5. 耦合分析

原版焊点：

- `CWorldData` 构造顺手初始化月表；气候 tick 与日历打包可能共享 `+0x10`（未钉）。
- 事件扫描把 `SCalendar` 整对象推进求值函数。
- 城镇情报、HUD、海域补给各自再拆一遍打包 / 经纬。
- 时间触发直接读主角 `+0x63/+0x64`。
- 黄道函数与气候随机、`Zodiac.dk4` 落在同一片 unassigned 编译单元。
- `CDatePadView` 放在 `trade/` 只因原版工程切法，不是交易公式。
- `CFleetElement::vfunc_04` 既推打包又做势力/舰队月逻辑（上缴文案「本月」）。

重构切断：

- world 只出 `Calendar` 与 `Coords` 两个无指针接口。
- narrative 订阅 `CalendarStep` / `MonthStarted`，LIVE 查快照。
- ocean 订阅 `CalendarStep` 扣补给；像素 tick 不默认推日。
- town-life 发 `Calendar.add_days(n)`，不自己改 packed。
- characters 拥有生日字节；本系统只定义编码，并在触发解码时 **查询** `Characters.birthday(person_id)`。
- 气候拥有黄道资源；本系统不存 `zodiac_id`。

## 6. 解耦后的配置

`config/world/calendar.yaml`：

```yaml
schema: dk4.world.calendar
version: 1
pack: vanilla-world0
ticks_per_day: 48
days_per_year: 365
leap_years: false
months:
  - { id: 0,  key: jan, length: 31 }
  - { id: 1,  key: feb, length: 28 }
  - { id: 2,  key: mar, length: 31 }
  - { id: 3,  key: apr, length: 30 }
  - { id: 4,  key: may, length: 31 }
  - { id: 5,  key: jun, length: 30 }
  - { id: 6,  key: jul, length: 31 }
  - { id: 7,  key: aug, length: 31 }
  - { id: 8,  key: sep, length: 30 }
  - { id: 9,  key: oct, length: 31 }
  - { id: 10, key: nov, length: 30 }
  - { id: 11, key: dec, length: 31 }
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
trigger:
  any: 255
  year_wildcard_min: 99
  month_birthday_sentinel: 12
  day_birthday_sentinel: 31
  result: { match: 1, before: 2, after: 0 }   # 原版返回值，内容包不要依赖
```

`config/world/coords.yaml`：

```yaml
schema: dk4.world.coords
version: 1
pack: vanilla-world0
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
  zero_sign: nonnegative_east_south   # fn_140005f51；另一套见 unknown
  hemi:
    east:  hemi.e               # SC「东」
    west:  hemi.w
    south: hemi.s
    north: hemi.n
span_hint:
  full_lon_units: 40000         # 与黑块 MulDiv 0x9c40 相同立刻数，不是网格宽
  origin_y_units: 10000
example:
  town_id: 0
  x: 20080
  y: 4272
  lat_deg: -51                  # 北 051
  lon_deg: 0                    # 东 000
```

`config/formulas/world.yaml`：

```yaml
schema: dk4.formulas.world
version: 1
calendar:
  ticks_per_day: 48
  days_per_year: 365
  ui_month_bias: 1
  ui_day_bias: 1
coords:
  origin_x: 20000
  origin_y: 10000
  lat_num: 9
  lat_den: 1000
  lon_num: 18
  lon_den: 2000
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
- 触发通配与生日哨兵
- `half_year_months`

改补给扣在哪一拍 → 只动 ocean（它订阅 `time_of_day==0`）。改旅馆是否推日 → 只动 town-life 发不发 `add_days`。改黄道阈值 → 只动气候包。

## 8. 对外接口

查询（LIVE；取不到则调用方 no-op）：

```
Calendar.snapshot() -> {
  packed, year, month, day, time_of_day, day_of_year,
  month_phase, ui_month, ui_day
}
Calendar.packed() -> u32
Coords.to_deg(x_original, y_original) -> { lat_deg, lon_deg }
Coords.format(lat_deg, lon_deg) -> { hemi_lat, abs_lat, hemi_lon, abs_lon }
Characters.birthday(person_id) -> { month, day } | none
Zodiac.from_birthday(month, day) -> zodiac_id | invalid   # 实现在气候；此处只声明
```

命令：

```
Calendar.set_packed(u32)          # 读档 / 调试；玩法走 add_*
Calendar.add_ticks(n)
Calendar.add_days(n)              # n * ticks_per_day
```

事件：

```
CalendarStep { packed, time_of_day, month, day }
DayStarted { packed, month, day }          # time_of_day 从 max 进到 0
MonthStarted { year, month }               # phase==start
MonthEnded { year, month }                 # phase==end
YearStarted { year }
HalfYearStart { year, month }              # fn_14001374b 为真时
```

`DayAdvanced`（事件文档已写）与 `DayStarted` 同义；实现只发一个，叙事订阅名在 `triggers.yaml` 里别名。

禁止把 `SCalendar*` / `CWorld*` 传出。存档：`packed` 一个 u32 + 人物生日两字节（人物节）。月表、原点不存。

触发解码（叙事调用，日历提供纯函数）：

```
Calendar.decode_trigger(year, month, day, time, birthday) -> SDate-like
Calendar.compare(world, check) -> match | before | after
```

## 9. 未核实字段

| 项 | 位置 | 现状 | 禁止 |
| --- | --- | --- | --- |
| 开局年 / 画面年 | 无 `1522×365×48` | `[未知]` | 不要写死 1522 |
| rdata 1444–1560 年表 | `0x14055f1xx` | 被 −1 切开，未接 `SDate` | 不要当开局年 |
| 打包存档文件偏移 | `CWorldData+0x10` 候选 | 与气候字段可能重叠 | 不要两家各写一份 |
| `CFleetElement+?+0x18` | `vfunc_04` `packed += 1` | 是否全局钟 `[推断]` | 不要再复制一个 NPC 钟 |
| 出海 tick → 拍 | `COceanScene` | 未见 `packed +=` | 不要默认 1 像素 = 1 拍 |
| 旅馆/修理/改造推日 | `CInn` 空；mesfile 有「天」 | `[未知]` | YAML 已留 `advance_calendar: null` |
| 旬 | mesfile 无此字 | 三态是月界拍 | 不要写成十日旬 |
| `fn_14000f218` / `fn_140009aca` 矩形名 | 整数度 AABB | `[未知]` | 不要标成海域 / 黄金航路 |
| `fn_1400f55d0` | 2196 B，调 `fn_14000a6fa` | 反编译失败 | 不要当第二套转换公式 |
| 0 度东 vs 西 | 两个 UI 函数符号相反 | 默认跟城镇情报 | 不要各画各的 |
| 黄道 cusp 12 值 | `.data` 映像为 0 | `[未知]` | 不要抄西洋占星日期 |
| `CDatePadView` 一串 `0x1fe,5,0x2b1…` | 构造 | UI 像素 | 不进玩法表 |
| `SGameSeed` 除 `+0x58/+0x5c` | `0x78` | 剧本文档 | 不要把整个种子当生日 |
| 原点地名 | 无字符串 | — | **不要写格林尼治** |
| 原版单位 → 2500 网格 | 地图文档 | `[未知]` | 禁止 `/8`；与经纬公式无关 |

提取器：月长表、原点、两个分数、四个半球字、格式串进 YAML；cusp、开局年、存档偏移进 `unknown`。
