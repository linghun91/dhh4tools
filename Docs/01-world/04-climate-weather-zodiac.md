# 气候、天气、星座

`CClimate` 是 `CWorldData` 的 RTTI 基类，气候状态活在世界对象里。风场表、天气贴图是**内容**；帆如何吃风是 ocean。星座**不是**气候：它是人物属性表（12 宫 × 6 项数值），本篇只因原版把它和气候函数编进同一单元而顺带记录，所有权交 characters。禁止把 `30`、`10`、`20`、`48` 只写在代码里。

## 1. 职责边界

管：

- 气候场（原版 `sClimateStream` / `sClimateStreamWind`）的数据形状与采样入口
- 世界存档里气候相关字节（`CWorldData +0x10…+0x1e`）及其每 tick 更新规则
- 「天气事件」档位随机表与持续时长
- 风向/风速修正的上下限符号
- 天气资源引用（`Weather.dk4`）

不管：

- 顺风/逆风如何改航速（`config/ocean/sailing.yaml`；mesfile 只说明帆的玩法）
- 暴风雨是否打断航行的判定细节未还原部分（ocean 订阅气候快照）
- 天空盒网格（`Sky.DK4`、`CSkySprite` 是 ocean/ui）
- 星座表与 `Zodiac.dk4`（characters；本篇 §2 只留事实，§6 给移交 schema）
- 港口本地气候对作物的闲聊文案（mesfile，不进逻辑表）

## 2. 原版事实

类与对象：

- `[确认]` `CWorldData -> CSaveObject -> CObject -> CWorldElement -> CClimate`。仓库里**没有**独立 `CClimate.cpp`，气候方法落在 `CWorld` / `CWorldData` 与 unassigned 函数。来源：`srcdata/source/WorldData.h`，`class_hierarchy.txt`，`catalog/rtti_names.txt`。
- `[确认]` `CWorldData::vfunc_05`（body `0x1402ae900`）初始化 `+0x10=0`、`+0x14=0`、`+0x18=0`、`+0x1c=0x0a`、`+0x1d=0`；`+0x1e` = **开局所在七大海域**：港号 `!= 0x62` 时取该港 `culture_id`（STown `+0x25`），`fn_140010f73`（body `0x1402ac510`）= 静态文化表 VA `0x1405b6160 + culture*0xE0`，取其 `+0x08` 字节即 `major_sea_id`（反汇编 `0x1402aeb4a`–`0x1402aeb6b`）；不在港（`0x62`）时按舰队位置走 `fn_140009fbb`/`fn_1402aec90` 另一支。海域图 BGM 在区号 8 时读它（`fn_14000a5b0`，见 `08`）。来源：`srcdata/source/game/world/CWorldData.cpp` + 反汇编。
- `[确认]` `CWorldData::vfunc_07` 按 `+0x10`（4 字节写函数）、`+0x14`（1 字节）、`+0x18`（4 字节）、`+0x1c`、`+0x1d`、`+0x1e`（各 1 字节）存档。`+0x15` **不存档**。来源：同文件。

每 tick 更新 `CWorld::vfunc_04`（body `0x1402ac1d0`，RTC `sClimateStreamWind`）。`this` 是 `CWorldData` 基址 `+8` 的子对象，下面一律换算成基址偏移 `base`（反汇编核对，反编译漏了乘数）：

- `[确认]` `arg` 是 `const SCalendar*`：`arg+0x18` = 当前打包时刻，`arg+0xc` = 月（`07-calendar-and-coords.md` §2.1）。取 `s = fn_14000b87a(fn_1402abae0())`：`fn_14000b87a`（body `0x1401dd8f0`）读玩家舰队对象 `+0x30`，即 **海上 happening 结局态**（`06-ocean/05-sea-happenings.md`；初值 27 = `0x1b` 无事）。仅当 `s == 0x1b`，或 `s == 4 且 rand(5) == 0`，并且 `base+0x18 < arg+0x18`（**无符号** `jae` 跳过）时，抽新天气：`dur = (rand(5) + 10) * 0x30`（480–672），`type = fn_1402abb20()`，调 `fn_14000592a(base, type, dur)`。
  反编译写成 `(rax+0xa)*(rax+0xa)` 是错的；指令是 `add eax,0xa; imul eax,eax,0x30`。
- `[确认]` `fn_14000592a(base, type, dur)`（body `0x1402ac140`）：`base+0x14 = (u8)type`；`dur == -1` 时同样取 `(rand(5)+10)*0x30`；`base+0x18 = base+0x10 + dur`。
- `[确认]` 随后若 `fn_140011d92(...) == 0`：在玩家舰队位置采样风场得 `{dir, w}`（`w` 为强度，`rsp+0x3c`），
  - 若 `rand(10) < w`：`v = base+0x1c + rand(w+1) - rand(w+1)`，`base+0x1c = clamp(v, 0, 20)`（`fn_1400103d4(0, v, 0x14)`）。
  - 若 `base+0x1d != 0`：`w < rand(10)` 时置 0；否则（为 0）：`rand(20) < w` 时置 1。
- `[确认]` `rand(n)` = `fn_140005272`：`n <= 0` 返回 0，否则取随机数对 `n` 取余（`cdq; idiv`）。
- `[推断]`（强）`0x18`（24）= 无风事件结局（`06-ocean/05-sea-happenings.md` 结局表）；`0x1b`（27）= `HAPPENING_IDLE`；`4` 的事件名未钉。它们是 happening 结局态，**不是**气候 id，也不是场景号。

风场采样：

- `[确认]` `fn_1400122c4`（body `0x1400786a0`）：`cx = x*50/40000`、`cy = y*25/20000`（`imul 0x32; idiv 0x9c40` / `imul 0x19; idiv 0x4e20`）；半年号 `h = fn_140078640(m)`，`0 <= m <= 5` 得 0，否则 1；`m` 来自 `vfunc_04` 的 `arg+0xc`（`SCalendar` 的 `month` 0–11）。取 `cell = table[h][cy][cx]`（`u16`，表 VA `0x140533890`，`.rdata`；行步长 `0x64`，半年步长 `0x9c4`）；`dir = (cell & 0xf) << 4`（经 `fn_140012d78`），`strength = (cell & 0xf0) >> 4`。
- `[确认]` 表形状：2 × 25 × 50 个 `u16` = 5000 字节。World0 实测：强度 0–6；方向 16 档全用到；高字节 0–12，两半年相同，纬向成带。高字节在这两个采样函数里**不读**。
- `[确认]` `m` 是月份 0–11（上半年 / 下半年两张风场）：`arg` 是 `SCalendar*`，`+0xc` 即 `SDate.month`。
- `[确认]` `fn_14000fe7f`（body `0x1400784f0`）用同一表、同一除数，不经 `fn_140078640`。
- `[确认]` 风输出 `fn_140014812`（body `0x1402abc90` 起）：`out_dir = (u8)(stream.dir + base+0x1c)`（`fn_140003e3b` 读 `+0x1c`，`fn_140013101` 相加）；`out_speed = (s == 0x18) ? 0 : stream.strength + base+0x1d + 3`（`lea eax,[rcx+rax+3]`）。
- `[推断]` `base+0x1c` 是全局风向偏移（初值 10 居中，随机游走 0–20），`base+0x1d` 是阵风位（+1 风速）。名字未恢复。

天气档随机 `fn_1402abb20`：

- `[确认]` `r = rand(100)`：`<30→0`、`<50→1`、`<70→2`、`<80→3`、`<87→4`、`<96→5`；否则 `q = rand(20)`：`<4→6`、`<7→7`、`<10→8`、`<13→9`、`<16→10`、`<18→11`、否则 12。输出 0–12 共 13 档。来源：反汇编 `0x1402abb20`。结果写 `base+0x14`。
- `[未知]` 13 档的名字与读取方。风场表高字节恰好也是 0–12，两者是否同一枚举未证。

风符号：

- `[确认]` `constants.json` 仅有：`WIND_DIRECTION_ADJUST_MAX`、`WIND_DIRECTION_ADJUST_MIN`、`WIND_VELOCITY_ADJUST_MAX`、`WIND_VELOCITY_ADJUST_MIN`。
- `[确认]` 断言：MAX 适合 `UCHAR` 宽度；`WIND_*_MIN <= data <= WIND_*_MAX`。断言字串在 `Game/SC/World0.dll` `.rdata`（UTF-16，`0xced30` / `0xd0710`），未找到直接引用它的代码。来源：`catalog/asserts.txt`。**具体整数未恢复**。
- `[推断]` `WIND_DIRECTION_ADJUST_*` 很可能就是 `base+0x1c` 的夹紧区间 0 / 20（UCHAR、初值居中 10）。只作候选，不得写成确认。

天气资源：

- `[确认]` `fn_14000e28c`（`0x140194130`）循环 `s30 >= 2`，两次加载 `"Weather.dk4"` 到对象 `+0x10` 起的指针槽。来源：`fn_140188eb0.cpp`。
- `[确认]` `CCloudData` 体积 `0x20`，构造把 `+0x10`、`+0x18` 两个指针置 0（循环 `>= 2`）。来源：`srcdata/source/game/ocean/CCloudData.cpp`。`[推断]` 这两槽就是 Weather.dk4 的两份贴图对象。
- `[确认]` 另有函数带 `Weather.dk4` 字符串：`unassigned_named.tsv` `fn_140008832`。
- `[确认]` `CSkySprite` 构造若某查询 `!= 8` 则 `+0x38=0` 否则 1；写 `+0x40=0x40`、`+0x44=0xf`。来源：`CSkySprite.cpp`。资源 `Sky.DK4`。
- `[确认]` mesfile 暴风雨：`0x3efb`「提督，是暴风雨！！天气突然就变了……。」及 `0x3f9b` `0x3feb`。来源：`mesfile_sc.txt`。判定公式未还原。
- `[确认]` 帆的文案区分顺风/逆风（`0xc3c3` `0xc40b` 等），那是造船/航海，不在气候表。

星座（事实留档，所有权在 characters）：

- `[确认]` `eZodiac`：头文件未写整数，`INVALID_ZODIAC = -1`，类型符号 `ZODIAC_ID`。来源：`ZodiacID.h`，`constants.json`，`asserts.txt`。
- `[确认]` `NUM_ZODIAC = 12`。`fn_14000b852`（body `0x1402aeef0`）：`0 <= i < 0xc` 返回 `&table[i]`，表 VA `0x140688970`，步长 `0x20`；第 13 槽全 0。每条 `{ char* name; i32 v[6]; }`。名字依次：摩羯座、水瓶座、双鱼座、牧羊座、金牛座、双子座、巨蟹座、狮子座、处女座、天秤座、天蝎座、射手座。`v[]` 每项 0–2，**每行和恰为 6**（如摩羯 `0,2,1,2,0,1`）。
- `[确认]` `v[6]` 是人物六维（体力、敏捷、魅力、智力、精神、运气，`SPerson+0x20` 起同序）的**升级成长 `apt`**：`fn_1400065fa`（body `0x1401ca9c0`）升级时取 `apt` 掷 `rand(2) + rand(apt+3)/2`（`03-characters/03-abilities-skills-jobs.md`）。人物星座字段 = `CPerson+0x0B`，`CPerson::vfunc_11`（vtable `+0x58`）返回；`0x1401ca330` 调 `[vtbl+0x58]` 后直接进 `fn_14000b852`。
- `[确认]` 调用方：`fn_140001d61` 取某对象虚函数 `+0x58` 的返回值作下标；`fn_140001a78`（`InfoBox2.dk4`）与 `fn_14000dd69`（人物状态画面，RTC `eSkill` `szBufHexagon`，加载 `Zodiac.dk4`）。没有任何气候函数读它。
- `[确认]` `fn_140006361`（`0x1402aef30`）分配 `0x90` 对象并加载 `"Zodiac.dk4"`。`Game/Zodiac.dk4` 存在。

## 3. 实体与 ID

| 实体 | ID / 键 | 计数 | 非法 |
| --- | --- | --- | --- |
| 气候状态 | 挂在 world 单例，无独立枚举 | 1 | — |
| 风场格 | `(half, cy, cx)` | 2 × 25 × 50 | 越界无检查 |
| 风向 | `dir` 16 档 × 16 = 0–240 | 16 | — |
| 风强 | 0–15 位宽，原版用到 0–6 | — | — |
| 天气档 | `weather_kind` 0–12 | 13 | — |
| 风向修正 | `WIND_DIRECTION_ADJUST_*` | UCHAR 范围 | 断言夹紧 |
| 风速修正 | `WIND_VELOCITY_ADJUST_*` | UCHAR 范围 | 断言夹紧 |
| 星座 | `zodiac_id` / `eZodiac` | 12 | -1（归 characters） |

没有恢复出 `WIND_N` / `WEATHER_STORM` 这种枚举名。配置用整数档 + 待补 `key`。

`CWorldData +0x1c` 初值 `0x0a`：是风向偏移的居中值，不是「十月」。

气候状态里的「下次换天气时刻」`+0x18` 与日历打包 `+0x10` 同单位（拍）。打包 `+0x10` 本身归日历（`07`），气候只读。

## 4. 算法 / 公式

风场采样（`config/formulas/climate.yaml`）：

```
climate_grid: { cols: 50, rows: 25, world_w: 40000, world_h: 20000 }
cx = x * cols / world_w            # 整数除，原版不夹紧
cy = y * rows / world_h
half = 0 if 0 <= month <= season_split else 1     # season_split = 5
cell = stream[half][cy][cx]
dir      = (cell & 0x0f) << 4
strength = (cell & 0xf0) >> 4
```

每 tick（`climate_tick`）：

```
weather_roll:
  outcomes_always: [idle]          # 原版 happening 态 0x1b（27）；ocean 映射
  outcomes_chance: { happening_4: 1/5 }   # 原版态 4，rand(5)==0
  duration: (rand(5) + 10) * 48    # 480..672；单位同 base+0x10
weather_kind_table:                # fn_1402abb20
  first:  { die: 100, bins: [[30,0],[50,1],[70,2],[80,3],[87,4],[96,5]] }
  second: { die: 20,  bins: [[4,6],[7,7],[10,8],[13,9],[16,10],[18,11]], else: 12 }
wind_dir_offset:                   # base+0x1c
  init: 10
  step_chance: "rand(10) < w"
  step: "rand(w+1) - rand(w+1)"
  clamp: [0, 20]
gust:                              # base+0x1d
  set_chance:   "rand(20) < w"     # 当前为 0 时
  clear_chance: "w < rand(10)"     # 当前为 1 时
```

风输出：

```
out_dir   = (stream.dir + wind_dir_offset) & 0xff
out_speed = 0 if ctx.suppress_wind else stream.strength + gust + speed_base   # 原版 happening 态 0x18；speed_base = 3
```

在公式名字还原前，实现必须把这些数放 YAML，禁止在 C++ 里写 `0x1b`、`48`、`3`。`48` 由 `caps.calendar_ticks_per_day` 给，气候公式写 `days(10)+rand_days(5)` 一类单位，不写拍数。

风向应用（ocean，本文件只声明接口）：

```
Ocean.sail_speed = sail_formula(wind_dir, wind_speed, rig, heading, params)
```

`params` 来自 `formulas/ocean.yaml`。world 只给当前 `WindSnapshot`。

## 5. 耦合分析

原版：

- 气候是 `CWorld` 的基类，tick 写在 `CWorld::vfunc_04`，和城镇加载、舰队表同一对象。
- tick 直接读玩家舰队的 happening 结局态决定要不要抽天气、要不要给风速；气候依赖了 ocean 的事件机。
- 风场表编译进 exe `.rdata`，World0/World1 共用；换剧本不能换风。
- 采样直接读玩家舰队位置（`fn_1402abaa0`），气候只为玩家一处取样。
- 风场格 50×25 写死在两个立即数对里，和 `World.dat` 的 2500×1250 不同比例。
- `CCloudData` / `CSkySprite` 在 ocean 场景里直接加载 dk4。
- 星座表与气候随机函数挤在同一 unassigned 编译单元，名字也被放进本篇；实际只被人物画面读。

重构：

- `config/world/climate.yaml`：风场、半年切分、tick 参数、天气档表。
- `config/ocean/wind.yaml`：如何把快照变成航速修正。
- `config/characters/zodiac.yaml`：星座表（不在 world）。
- UI 贴图路径在 `config/ui/`。
- tick 不读 happening 态：ocean 暴露 `Ocean.current_happening(fleet)`，由 ocean 配置把结局态映射成 `ClimateTickContext { allow_weather_roll, roll_chance, suppress_wind }` 交给气候（原版：27 无事 → 允许；4 → 1/5；24 无风 → 风速 0）。
- 气候 tick 由引擎 `Clock` 按 `config/engine/tick.yaml` 调度（原版 `schedule: day_start`，见 `07` §2.4），不自建计时循环。

## 6. 解耦后的配置

`config/world/climate.yaml`：

```yaml
schema: dk4.world.climate
version: 2
pack: vanilla-core               # 风场表在主 exe，World0/World1 共用
schedule: day_start              # 原版 fn_14000277f 一拍调度：日首（07 §2.4）
stream:
  table_file: packs/vanilla-core/climate_stream.bin   # exe .rdata 0x140533890，5000 字节
  layout: { halves: 2, rows: 25, cols: 50, cell: u16le }
  world_units: { width: 40000, height: 20000 }
  season_split_month: 5          # m <= 5 用第 0 张
  decode:
    dir:      { mask: 0x000f, shift_left: 4 }
    strength: { mask: 0x00f0, shift_right: 4 }
    zone:     { mask: 0xff00, shift_right: 8, used: false }   # 0..12，用途未知
tick:
  weather_roll:
    duration_days: { base: 10, rand: 5 }   # 原版 (rand(5)+10)*48 拍；拍数由 Clock 换算
    kind_table: weather_kind_table_vanilla
  wind_dir_offset: { init: 10, min: 0, max: 20 }
  gust: { set_die: 20, clear_die: 10 }
  wind_speed_base: 3
weather_kind_table_vanilla:
  - { die: 100, bins: [[30,0],[50,1],[70,2],[80,3],[87,4],[96,5]], fallthrough: next }
  - { die: 20,  bins: [[4,6],[7,7],[10,8],[13,9],[16,10],[18,11]], else: 12 }
weather_kinds:                   # 13 档，名字待补；key 是主键
  - { key: vanilla:weather_00 }          # 原版 0
  # ... weather_12
wind_adjust:
  direction_min: WIND_DIRECTION_ADJUST_MIN   # 候选 0
  direction_max: WIND_DIRECTION_ADJUST_MAX   # 候选 20
  velocity_min:  WIND_VELOCITY_ADJUST_MIN
  velocity_max:  WIND_VELOCITY_ADJUST_MAX
state_fields:                    # CWorldData 偏移，供存档对照
  # +0x10 是日历打包，归 07 calendar，不在气候状态里重复
  weather_kind:    { off: 0x14, type: u8,  init: 0 }
  weather_until:   { off: 0x18, type: u32, init: 0 }
  wind_dir_offset: { off: 0x1c, type: u8,  init: 10 }
  gust:            { off: 0x1d, type: u8,  init: 0 }
  start_major_sea: { off: 0x1e, type: u8,  init: derived }   # 开局港 culture → major_sea（不是 region）
weather:
  asset: Weather.dk4
  cloud_slots: 2
  storm_message_keys: [mes_0x3efb, mes_0x3f9b, mes_0x3feb]
```

`config/characters/zodiac.yaml`（移交，示例）：

```yaml
schema: dk4.characters.zodiac
version: 1
signs:
  - { key: vanilla:capricorn, vanilla_index: 0, name_key: zodiac.capricorn, growth: { vanilla:stamina: 0, vanilla:agility: 2, vanilla:charm: 1, vanilla:intelligence: 2, vanilla:spirit: 0, vanilla:luck: 1 } }
  - { key: vanilla:aquarius,  vanilla_index: 1, name_key: zodiac.aquarius,  growth: { vanilla:stamina: 1, vanilla:agility: 1, vanilla:charm: 0, vanilla:intelligence: 1, vanilla:spirit: 2, vanilla:luck: 1 } }
  # ... 共 12；growth 按六维 key（升级成长 apt，F-PROG-10 的 g），schema 以 14-config-schemas/examples/characters/zodiac.yaml 为准
asset: Zodiac.dk4
invariants: { growth_sum: 6 }   # 原版每行和为 6，校验器可选
```

`config/caps.yaml` 片段：

```yaml
wind_dir_adjust: { min: WIND_DIRECTION_ADJUST_MIN, max: WIND_DIRECTION_ADJUST_MAX }
wind_vel_adjust: { min: WIND_VELOCITY_ADJUST_MIN,  max: WIND_VELOCITY_ADJUST_MAX }
```

整数从断言补齐后替换符号，schema 升 version。

## 7. 独立旋钮

- 风场表（换一张世界风场）、风场分辨率、半年切分月
- 天气档权重表、持续时长三元组
- 风向偏移初值/区间、阵风开关概率、风速底数 3
- 风向/风速修正上下限
- 天气资源、云槽数量
- 哪些场景允许抽天气 / 屏蔽风速（在 ocean/ui 的场景表，不在气候代码）

改横帆/纵帆系数只动 ocean。改暴风雨文案只动 `ui/strings`。改星座只动 characters。

## 8. 对外接口

```
Climate.wind_at(x_original, y_original, month) -> WindSnapshot
  # { dir: u8, strength: u8 }，纯查表，不含全局偏移
Climate.wind_effective(x, y, month, ctx) -> { dir, speed }
  # 叠加 wind_dir_offset / gust / speed_base；ctx.suppress_wind 时 speed=0
Climate.on_day_start(calendar_snapshot, player_pos, ctx: ClimateTickContext) -> void   # Clock 调度
Climate.weather() -> { kind: weather_kind_id, until: u32 }
Weather.visual() -> { asset_key, slot }   # 给 ui，不给战斗
```

事件：

```
WeatherRolled { kind, until }
GustChanged { on: bool }
StormStarted { }          # 13 档中哪几档算暴风雨还原后再发；在此之前 ocean 不得假装有公式
StormEnded { }
```

星座查询在 characters：`Person.zodiac(person_id) -> zodiac_id`、`Zodiac.def(zodiac_id)`。world 不再暴露 `Zodiac.current()`，也不发 `ZodiacChanged`。

ocean 读快照。combat 不读气候，除非以后明确做「暴风雨影响炮术」并单独开公式文件。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| 风场表高字节 0–12 | `0x140533890` | 值 `[确认]`，用途 `[未知]` |
| `m` 是否月份 | `arg+0xc` = `SCalendar.month` | `[确认]` |
| `vfunc_04` 的 `s`（`0x1b`、`4`、`0x18`） | `fn_14000b87a` = 舰队 `+0x30` happening 结局态 | 来源 `[确认]`；4 的事件名 `[未知]` |
| `arg+0x18` 是否当前时刻 | `SCalendar+0x18` = 打包 | `[确认]` |
| `base+0x10` 由谁推进 | `fn_140012049` `world+0x10 += 1`（`07` §2.4） | `[确认]` |
| 天气档 0–12 的名字与读取方 | 仅见写 `base+0x14` | `[未知]` |
| `base+0x1e` 的来源函数 `fn_140010f73` 语义 | 静态文化表项指针，取 `+8` major_sea | `[确认]`；不在港分支 `fn_1402aec90` `[未知]` |
| `WIND_*` 具体整数 | 仅断言字串 | `[未知]`（方向候选 0/20） |
| `fn_140011d92` 为真时跳过风更新的条件 | `vfunc_04` | `[未知]` |
| 星座 `v[6]` 是否就是六能力加成 | `fn_1400065fa` | `[确认]` 升级成长 `apt` |
| 人物星座字段在哪 | 虚函数 `+0x58` | `[确认]` `CPerson+0x0B`（`vfunc_11`） |
| `CSkySprite +0x38` 与「8」 | 天空 | ui，非玩法 |
| 暴风雨触发条件 | 仅文案 | `[未知]` |
| `CAdman` 写 `0x62/0x6a` | `CAdman.cpp` | 与气候无关，勿混入 |
