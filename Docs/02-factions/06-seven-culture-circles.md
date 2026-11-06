# 七大海域与 21 文化圈（更正）

上一版把 `CForce+0x10` 的 7×uint16 写成「文化圈」。大佬看完后纠正：**七大海域，21 个文化圈。**

本篇只写源码对得上的计数与名字。没有从 exe 抽出的 21 个文化圈中文名，这里不编。

## 1. 职责边界

管：

- **7 大海域**的 ID 空间、已抽出的显示名、势力影响力数组下标
- **21 文化圈**的 ID 空间（`CCulture` 槽数、与港口 `region` 同计数）
- 这两轴和 **25 `eSeaArea`**、**9 张 NPC 脸**、**`eCountry`** 的分界

不管：

- 25 航海分区的网格/邻港（`01-world/01-map-and-sea-areas.md`）
- 流行 26 档、市价公式
- 9 脸换装（仍在 `02-countries-culture-fashion.md`）

## 2. 原版事实

### 2.1 四套 ID，计数不同

| 轴 | 计数 | 源码落点 | 已抽出的名字 |
| --- | ---: | --- | --- |
| **七大海域** | 7 | `CForce` 存盘循环 `s20 >= 7`，读写 `this+0x10+i*2`；RTC 局部名 **`eSeaArea` 上界 7** | exe `0x58ab4c`：北海、地中海、非洲、印度洋、东南亚、东亚、新大陆；其后一条「全世界」 |
| **21 文化圈** | 21 | `CCultureElement` 构造 `fn_140007dce(..., stride=0xE8, count=0x15)`；析构/存盘同样 `>= 0x15` | **未抽出 21 名表** |
| **25 航海分区** | 25 | `SeaAreaID.h` `NUM_SEAAREA=25`；`NUM_NEXTBASETOWN=25` | 25 条显示名未抽出 |
| **9 NPC 脸** | 9 文案 | 人物 `family`：欧洲、非洲、阿拉伯、印度、东南亚、中国、韩国、日本、中南美 | 这 9 个词 |

`eCulture` / `eCountry` 头文件只有符号，`NUM_CULTURE` / `NUM_COUNTRY` 整数未写进 `*ID.h`。21 来自 `CCultureElement` 的 `0x15`，不是头文件常量。

`eCountry` 仍然存在（`CountryID.h`、`asserts.txt`）。源码里**没有**把 `NUM_COUNTRY` 钉成 21。港口 `region` 0–20 与 21 文化圈同计数，**字段名不是 `eCountry`**。

### 2.2 七大海域 `[确认]` 布局与名字

- `[确认]` `CForce::vfunc_06/07`：`s20` 从 0 加到 `>= 7`，每次 `fn_14000a19b` / `fn_140004421` 读写 `&self + s20*2 + 0x10` 一个 word。来源：`srcdata/source/game/world/CForce.cpp`。
- `[确认]` 宽度与 `FORCE_INFLUENCE`（USHORT）一致。MIN/MAX 整数未还原。来源：`Force.h` 注释、`constants.json`。
- `[确认]` 下列函数的 RTC 变量名是 **`eSeaArea`**，比较上界是 **7**：
  - `fn_140008b6b` body `0x14010a6c0`（`fn_140107150.cpp`）
  - `fn_14010be70`（`if (eSeaArea >= 7)`）
- `[确认]` 不能因此改 `NUM_SEAAREA=7`。`SeaAreaID.h` 仍是 `NUM_SEAAREA=25`。7 与 25 是两套计数，源码里同时存在。
- `[确认]` SC exe 文件偏移 `0x58ab4c` 连续 GBK C 字符串：

| id | 短名 | 长名（`SeaAreaMessage` / 北海海域…） |
| ---: | --- | --- |
| 0 | 北海 | 北海海域 / 北海海域周边 |
| 1 | 地中海 | 地中海海域 / 地中海海域周边 |
| 2 | 非洲 | 非洲海域 |
| 3 | 印度洋 | 印度洋海域 |
| 4 | 东南亚 | 东南亚海域 / 东南亚周边海域 |
| 5 | 东亚 | 东亚海域 |
| 6 | 新大陆 | 新大陆海域 |
| （7） | 全世界 | 影响力数组长度是 7，**没有**第 8 格 |

长名来源：`fn_140009750` RTC `SeaAreaMessage`；旁白同序见 `Docs/01-world/08-sea-area-music-icons.md` §2.2。

霸者之证 mesfile `0x1e247` 起七句「这是%s的霸者之证」顺序与上表 0–6 相同。那是**同一套 7 个名字**被叙事/证物引用，不是另一套枚举。

World0 势力 `+0x10` 活跃者恰好一格非 0，格号与母港所在大区一致（里斯本→格 1 地中海，杭州→格 5 东亚）。布局 `[确认]`；「格号 = 七大海域 id」与 RTC 名、短名表、母港对照一致。

### 2.3 21 文化圈 `[确认]` 槽数 / 名字未还原

- `[确认]` `CCulture : CSaveObject`。`CCultureElement` 分配 **21** 个 `CCulture`，stride `0xE8`。来源：`CCultureElement.cpp` 构造参数 `0x15`。
- `[确认]` 每个 `CCulture` 存盘扫 `eTradingItem` 0–0x68（105）个 word，基址 `+0x14`。另有 `+0x10` 一字节、`+0x11` 写两轮。来源：`CCulture.cpp` `vfunc_05/06/07`。
- `[确认]` 没有 `GetFixCultureProc`。文化块不在 World DLL。来源：`DllMain.cpp`。
- `[确认]` 港口表 `+0x25` 取值 0–20，共 21 档；21 座 `is_capital==1` 港各占一档。来源：`Docs/_data/towns_world0.json`。
- `[确认]` 港口情报 UI 有 `posCulture`（`fn_140005f51` RTC）。
- **没有**从 exe 抽出 21 条「文化圈」名称表。`0x5b656c` 起 13 词（佛兰德…墨西哥）后面直接接「小麦」交易品表，**对不上 21**。`0x538e8c` 起 19 个国名（英吉利…印加）是另一串，**对不上 21**，也不要当成 `eCulture` 名。

因此：21 个文化圈用 `culture_id` 0–20。显示名 `null`，直到抽出名字。首都港只作对照，不是圈名。

| culture_id | 该档首都港（World0，`region` 字节 = 此 id） |
| ---: | --- |
| 0 | 伦敦 |
| 1 | 阿姆斯特丹 |
| 2 | 汉堡 |
| 3 | 斯德哥尔摩 |
| 4 | 里斯本 |
| 5 | 塞维利亚 |
| 6 | 热那亚 |
| 7 | 雅典 |
| 8 | 伊斯坦布尔 |
| 9 | 亚历山大 |
| 10 | 圣乔治 |
| 11 | 索法拉 |
| 12 | 巴士拉 |
| 13 | 卡利卡特 |
| 14 | 马六甲 |
| 15 | 雅加达 |
| 16 | 杭州 |
| 17 | 汉城 |
| 18 | 大阪 |
| 19 | 哈瓦那 |
| 20 | 委拉克鲁斯 |

`region` 字节是否在原版源码里就叫 `eCulture`：反编译 `CTown` **没有** RTC 名 `eCulture`。同计数 + `CCulture`×21 + 大佬「21 个文化圈」一致，字段名仍不升格。

### 2.4 不要再混的几件事

| 说法 | 源码 |
| --- | --- |
| 把 7 格叫文化圈 | 错。RTC 名是 `eSeaArea`，短名是海域。 |
| 把 `NUM_SEAAREA` 改成 7 | 错。头文件是 25。 |
| 把 9 张 NPC 脸当 21 文化圈 | 错。9 是人物 `family` 文案。 |
| 给 21 圈填「英格兰文化」等 | 源码没有这 21 个名字。 |
| 用 13 词或 19 国名凑 21 | 表长对不上。 |

## 3. 实体与 ID

| 实体 | 原版计数 | 空值 | 配置 key |
| --- | ---: | --- | --- |
| 七大海域 | 7 | 无第 8 格；「全世界」只是文案 | `major_sea_id` |
| 21 文化圈 | 21 | `INVALID_CULTURE=-1` | `culture_id` |
| 25 航海分区 | 25 | `INVALID_SEAAREA=-1` | `sea_area_id` |
| 9 NPC 脸 | 9 | 无枚举文件 | `npc_face_id`（人物文档） |

影响力：`Influence[force_id][major_sea_id] : ushort`。

## 4. 算法 / 公式

无新公式。势力影响力读写按下标 `0..6`。文化块 105 word 按 `culture_id` 取整块。

原版包：`major_sea_count = 7`，`culture_count = 21`。运行时按表长，不写死。

## 5. 耦合分析

原版把 7 格焊在 `CForce` 结构体里，把 21 个 `CCulture` 焊在 `CWorld` 的 `CCultureElement` 上。港口 `+0x25` 与 21 槽同计数。

重构：

- 七大海域表独立；势力只存 `major_sea_id → 影响力`
- 文化圈表独立；港口存 `culture_id`；`CCulture` 的 105 word 属文化圈，不属海域
- 25 `sea_area_id` 仍属 world 航海

## 6. 解耦后的配置

```yaml
# config/world/major_seas.yaml
schema: dk4.world.major_seas
version: 1
- { id: 0, key: north_sea, name: 北海 }
- { id: 1, key: mediterranean, name: 地中海 }
- { id: 2, key: africa, name: 非洲 }
- { id: 3, key: indian_ocean, name: 印度洋 }
- { id: 4, key: southeast_asia, name: 东南亚 }
- { id: 5, key: east_asia, name: 东亚 }
- { id: 6, key: new_world, name: 新大陆 }

# config/factions/cultures.yaml
schema: dk4.factions.cultures
version: 1
# 21 行。name 未从 exe 抽出则 null。首都港只对照。
- { id: 0, key: culture_00, name: null, capital_town_id: 0 }
# ... id 1–20
```

势力：

```yaml
influence:
  - { major_sea_id: 1, value: 122 }   # 卡斯特罗 World0
```

港口：`culture_id`（0–20）。不要写 `culture_circle_id` 去指 7 格。

霸者之证 / kind 8 的 `+0x15`（0–6）：外键 **`major_sea_id`**，与七大海域同空间。

## 7. 独立旋钮

- 7 个海域名、影响力初值
- 21 个文化圈行（可加长；原版包 21）
- 25 航海分区表长不动本轴

## 8. 对外接口

```
MajorSea.list() -> [major_sea_id]
MajorSea.name(id) -> string
Culture.list() -> [culture_id]
Culture.ofTown(town_id) -> culture_id | invalid   # 读港口 culture_id
Influence.get(force_id, major_sea_id) -> int
```

事件：`InfluenceChanged { force_id, major_sea_id, old, new }`

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| 21 个文化圈中文名 | 未抽出 |
| 港口 `+0x25` 的原版类型名是不是 `eCulture` | 无 RTC 名 |
| `NUM_COUNTRY` 整数 | 未还原；不要写成 21 |
| `NUM_SEAAREA=25` 与七大海域的从属表 | 未见 25→7 的对照数组 |
| 「全世界」是否运行时 id 7 | 影响力循环到 7 为止，没有第 8 格 |
| `0x5b656c` 13 词、`0x538e8c` 19 国名 | 表长对不上 21，用途未钉 |

上一版 YAML 里的 `culture_circle_id` 指 0–6：作废。改用 `major_sea_id`。
