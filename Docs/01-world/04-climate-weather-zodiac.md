# 气候、天气、星座

`CClimate` 是 `CWorldData` 的 RTTI 基类，气候状态活在世界对象里。风向表、天气贴图、星座贴图是**内容**；帆如何吃风是 ocean。本文件把原版符号和未还原公式摊开，禁止把 `30`、`10`、`20` 只写在代码里。

## 1. 职责边界

管：

- 气候场（原版 `sClimateStream` / `sClimateStreamWind`）的数据形状与采样入口
- 风向/风速修正的上下限符号
- 天气资源引用（`Weather.dk4`）
- 星座枚举与 `Zodiac.dk4`
- 世界存档里气候相关字节（`CWorldData +0x10…+0x1e`）

不管：

- 顺风/逆风如何改航速（`config/ocean/sailing.yaml`；mesfile 只说明帆的玩法）
- 暴风雨是否打断航行的判定细节未还原部分（ocean 订阅气候快照）
- 天空盒网格（`Sky.DK4`、`CSkySprite` 是 ocean/ui）
- 黄道用于占卜/事件的剧情（narrative 可读 `zodiac_id`）
- 港口本地气候对作物的闲聊文案（mesfile，不进逻辑表）

## 2. 原版事实

类与对象：

- `[确认]` `CWorldData -> CSaveObject -> CObject -> CWorldElement -> CClimate`。仓库里**没有**独立 `CClimate.cpp`，气候方法落在 `CWorld` / `CWorldData` 与 unassigned 函数。来源：`srcdata/source/WorldData.h`，`class_hierarchy.txt`，`catalog/rtti_names.txt`。
- `[确认]` `CWorldData` 字段访问：`+0x8`、`+0xc`、`+0x10`、`+0x14`、`+0x18`、`+0x1c`、`+0x1d`、`+0x1e`。来源：`catalog/fields.tsv`。
- `[确认]` `CWorldData::vfunc_05` 初始化 `+0x10=0`、`+0x14=0`、`+0x18=0`、`+0x1c=0x0a`、`+0x1d=0`，再写 `+0x1e`。来源：`srcdata/source/game/world/CWorldData.cpp`。
- `[确认]` `CWorldData::vfunc_07` 按同样偏移存档。来源：同文件。
- `[确认]` `CWorld::vfunc_04` RTC `sClimateStreamWind`（body `0x1402ac1d0`）。取一个值 `s20`，若 `== 0x1b` 或（`== 4` 且附加检查）则走一支；比较 `+0x10` 与 `+0x18`；`fn_140005272(5)` 再 `(rax+0xa)*(rax+0xa)`；调用 `fn_1400122c4` 填 `sClimateStreamWind`；`fn_140005272(0xa)` 与计数器比较后写 `+0x14`；再根据 `fn_140005272(0xa)` / `(0x14)` 写 `+0x15` 为 0 或 1。来源：`srcdata/source/game/world/CWorld.cpp`。

气候场采样：

- `[确认]` `fn_14000fe7f`（`0x1400784f0`）、`fn_1400122c4`（`0x1400786a0`）RTC `sClimateStream` + `pos`。对坐标做 `idiv`，从巨大静态表取 packed 半字节：`& 0xf` 左移 4，或 `& 0xf0` 右移 4。来源：`srcdata/source/game/unassigned/fn_14006f9e0.cpp`。表字面量反编译成 `\u0a3b\u0a3a…` 串，**网格含义未还原**。
- `[确认]` `fn_140078640` 把输入限制在 `0..5`（闭区间），越界返回 1。来源：同文件。`[推断]` 与风向离散档有关，不是确认的枚举名。
- `[确认]` `fn_140007ef5` / `fn_140014812`（`fn_1402aad20.cpp`）再包一层 `sClimateStreamWind`。`fn_140014812` 若某值 `== 0x18` 则输出 0。

风符号：

- `[确认]` `constants.json` 仅有：`WIND_DIRECTION_ADJUST_MAX`、`WIND_DIRECTION_ADJUST_MIN`、`WIND_VELOCITY_ADJUST_MAX`、`WIND_VELOCITY_ADJUST_MIN`。
- `[确认]` 断言：MIN/MAX 适合 `UCHAR` 宽度；`WIND_*_MIN <= data <= WIND_*_MAX`。来源：`catalog/asserts.txt`。**具体整数未在头文件写出**，文档保留符号名，实现从断言/RTC 再填。
- `[确认]` `id-registry.md` 已把这四项列入 caps。没有 `WIND_NORTH` 这类方位枚举被恢复。

随机档 `fn_1402abb20`：

- `[确认]` 先 `fn_140005272(0x64)`（100）：`<30→0` `<50→1` `<70→2` `<80→3` `<87→4` `<96→5`；否则再 `fn_140005272(0x14)`（20）分成 6–12。输出 0–12 共 13 档。来源：`fn_1402aad20.cpp`。用途 `[未知]`（天气类型或风向档都可能）。不得当成星座表。

天气资源：

- `[确认]` `fn_14000e28c`（`0x140194130`）循环 `s30 >= 2`，两次加载 `"Weather.dk4"` 到对象 `+0x10` 起的指针槽。来源：`fn_140188eb0.cpp`。
- `[确认]` `CCloudData` 体积 `0x20`，构造把 `+0x10`、`+0x18` 两个指针置 0（循环 `>= 2`）。来源：`srcdata/source/game/ocean/CCloudData.cpp`。`[推断]` 这两槽就是 Weather.dk4 的两份贴图对象。
- `[确认]` 另有函数带 `Weather.dk4` 字符串：`unassigned_named.tsv` `fn_140008832`。
- `[确认]` `CSkySprite` 构造若某查询 `!= 8` 则 `+0x38=0` 否则 1；写 `+0x40=0x40`、`+0x44=0xf`。来源：`CSkySprite.cpp`。资源 `Sky.DK4`。
- `[确认]` mesfile 暴风雨：`0x3efb`「提督，是暴风雨！！天气突然就变了……。」及 `0x3f9b` `0x3feb`。来源：`mesfile_sc.txt`。判定公式未还原。
- `[确认]` 帆的文案区分顺风/逆风（`0xc3c3` `0xc40b` 等），那是造船/航海，不在气候表。

星座：

- `[确认]` `eZodiac`：`NUM_ZODIAC` 整数未恢复，`INVALID_ZODIAC = -1`，类型符号 `ZODIAC_ID`。来源：`ZodiacID.h`，`constants.json`，`asserts.txt`。
- `[确认]` `fn_140006361`（紧挨着）加载 `"Zodiac.dk4"`，分配 `0x90` 字节对象。来源：`fn_1402aad20.cpp`。
- `[推断]` `NUM_ZODIAC = 12`。依据：同文件 `fn_14000b852` 在 Zodiac 加载**正上方**，把下标限制为 `0 <= x < 0xc`。西方黄道 12 宫与此相符，仍不是头文件确认。
- `[确认]` `Game/Zodiac.dk4` 存在。

## 3. 实体与 ID

| 实体 | ID / 键 | 计数 | 非法 |
| --- | --- | --- | --- |
| 气候状态 | 挂在 world 单例，无独立枚举 | 1 | — |
| 风向修正 | `WIND_DIRECTION_ADJUST_*` | UCHAR 范围 | 断言夹紧 |
| 风速修正 | `WIND_VELOCITY_ADJUST_*` | UCHAR 范围 | 断言夹紧 |
| 星座 | `zodiac_id` / `eZodiac` | `[推断]` 12 | -1 |
| 天气档 | `[未知]` 0–12 随机输出？ | 13 档或更少 | — |

没有恢复出 `WIND_N` / `WEATHER_STORM` 这种枚举名。配置用整数档 + 待补 `key`。

`CWorldData +0x1c` 初值 `0x0a`：`[未知]`，可能是计数器周期，不要写成「十月」。

## 4. 算法 / 公式

气候场采样——完整式子未还原。已看见的结构：

```
# config/formulas/ocean.yaml  /  climate 段
climate_stream:
  nibble_low_mask:  0x0f
  nibble_high_mask: 0xf0
  # pos 经 idiv 后索引静态表；表本体进内容包二进制，不手抄进 YAML
wind_dir_adjust: { min: WIND_DIRECTION_ADJUST_MIN, max: WIND_DIRECTION_ADJUST_MAX }
wind_vel_adjust: { min: WIND_VELOCITY_ADJUST_MIN,  max: WIND_VELOCITY_ADJUST_MAX }
```

`CWorld::vfunc_04` 里出现的立刻数，先当命名参数（语义未钉）：

```
climate_tick:
  branch_id_a: 0x1b          # s20 == 27
  branch_id_b: 4
  rand_span_a: 5
  square_bias: 10            # (r+10)^2
  rand_span_b: 10
  flag_rand_lo: 10
  flag_rand_hi: 20           # 写 +0x15
init_field_0x1c: 10          # CWorldData 初值 0x0a
```

`fn_1402abb20` 权重（若证实是天气/风向随机，原样进表）：

```
weighted_bins_d100:
  - { max_exclusive: 30, value: 0 }
  - { max_exclusive: 50, value: 1 }
  - { max_exclusive: 70, value: 2 }
  - { max_exclusive: 80, value: 3 }
  - { max_exclusive: 87, value: 4 }
  - { max_exclusive: 96, value: 5 }
# 其余走 d20 -> 6..12
```

在公式还原前，实现必须把这些数放 YAML，禁止在 C++ 里写 `0x1b`。

风向应用（ocean，本文件只声明接口）：

```
Ocean.sail_speed = sail_formula(wind_dir, wind_vel, rig, heading, params)
```

`params` 来自 `formulas/ocean.yaml`。world 只给当前 `WindSnapshot`。

## 5. 耦合分析

原版：

- 气候是 `CWorld` 的基类，tick 写在 `CWorld::vfunc_04`，和城镇加载、舰队表同一对象。
- 采样函数对着编译进 exe 的静态表，按坐标半字节打包。
- `CCloudData` / `CSkySprite` 在 ocean 场景里直接加载 dk4。
- 帆的顺风逆风文案在造船，数值在航海。
- 星座资源加载与气候随机函数挤在同一 unassigned 编译单元。

重构：

- `config/world/climate.yaml`：场、档位、星座枚举。
- `config/ocean/wind.yaml`：如何把快照变成航速修正。
- UI 贴图路径在 `config/ui/`。
- narrative 可 `query Zodiac.current()`，不得改风速。

## 6. 解耦后的配置

`config/world/climate.yaml`：

```yaml
schema: dk4.world.climate
version: 1
pack: vanilla-world0
stream:
  table_file: packs/vanilla-world0/climate_stream.bin   # 从 exe 抽出，未命名
  sample: nibble_packed
wind_adjust:
  direction_min: WIND_DIRECTION_ADJUST_MIN
  direction_max: WIND_DIRECTION_ADJUST_MAX
  velocity_min:  WIND_VELOCITY_ADJUST_MIN
  velocity_max:  WIND_VELOCITY_ADJUST_MAX
state_fields:                  # 对应 CWorldData 偏移，名字待定
  +0x10: { type: u32, init: 0 }
  +0x14: { type: u8,  init: 0 }   # vfunc_04 写入
  +0x15: { type: u8,  init: 0 }   # 0 或 1
  +0x18: { type: u32, init: 0 }
  +0x1c: { type: u8,  init: 10 }
  +0x1d: { type: u8,  init: 0 }
  +0x1e: { type: u8,  init: derived }
weather:
  asset: Weather.dk4
  cloud_slots: 2
  storm_message_keys: [mes_0x3efb, mes_0x3f9b, mes_0x3feb]
  bins: unknown                # 勿把 0..12 随机表直接标成天气
zodiac:
  count_inferred: 12
  invalid: -1
  asset: Zodiac.dk4
  names: null                  # 未抽出
```

`config/caps.yaml` 片段：

```yaml
wind_dir_adjust: { min: WIND_DIRECTION_ADJUST_MIN, max: WIND_DIRECTION_ADJUST_MAX }
wind_vel_adjust: { min: WIND_VELOCITY_ADJUST_MIN,  max: WIND_VELOCITY_ADJUST_MAX }
```

整数从 RTC 补齐后替换符号，schema 升 version。

## 7. 独立旋钮

- 气候场表（换一张世界风场）
- 风向/风速修正上下限
- 天气资源、云槽数量
- 星座个数与资源
- tick 立刻数（`branch_id_*`、`rand_span_*`）

改横帆/纵帆系数只动 ocean。改暴风雨文案只动 `ui/strings`。

## 8. 对外接口

```
Climate.wind_at(x_original, y_original) -> WindSnapshot
  # { dir_adjust: u8, vel_adjust: u8, flags }
Climate.tick(world_time) -> void     # 纯写气候状态
Zodiac.current() -> zodiac_id | invalid
Weather.visual() -> { asset_key, slot }   # 给 ui，不给战斗
```

事件：

```
WindChanged { dir_adjust, vel_adjust }
StormStarted { }          # 判定还原后再发；在此之前 ocean 不得假装有公式
StormEnded { }
ZodiacChanged { zodiac_id }
```

ocean 读快照。combat 不读气候，除非以后明确做「暴风雨影响炮术」并单独开公式文件。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| `sClimateStream` 静态表每个半字节 | `fn_14000fe7f` | `[未知]` |
| 坐标 idiv 的除数 | 同左，反编译损坏 | `[未知]` |
| `CWorldData +0x10/+0x14/+0x18/+0x1c/+0x1d/+0x1e` 字段名 | fields.tsv | `[未知]` |
| `vfunc_04` 的 `s20` 是什么枚举 | 比较 0x1b 与 4 | `[未知]` |
| `+0x15` 布尔 | 0/1 | `[未知]` |
| `fn_1402abb20` 0–12 | 权重随机 | `[未知]` 天气或风档 |
| `fn_140078640` 0–5 | 越界检查 | `[推断]` 风档 |
| `WIND_*` 具体整数 | 仅符号 | `[未知]` |
| `NUM_ZODIAC` | 仅 `< 0xc` 旁 Zodiac.dk4 | `[推断]` 12 |
| 12 星座名称与起始日 | 无表 | `[未知]` |
| `CSkySprite +0x38` 与「8」 | 天空 | ui，非玩法 |
| 暴风雨触发条件 | 仅文案 | `[未知]` |
| `CAdman` 写 `0x62/0x6a` | `CAdman.cpp` | 与气候无关，勿混入 |
