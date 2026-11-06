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
| **21 文化圈** | 21 | `CCultureElement` 构造 `fn_140007dce(..., stride=0xE8, count=0x15)` | 对象码表 21 名（`dh4/2.png`）；SC 无连续 21 名表 |
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
- `[确认]` SC exe **没有** 21 条连续的文化圈名表。`0x5b656c` 起 13 词（佛兰德、北欧、意大利、希腊、土耳其、埃及、西非、东非、印度支那、印尼、朝鲜、加勒比海、墨西哥）后面紧接交易品「小麦」。`0x538e8c` 起 19 词是国名（英吉利、法兰西、荷兰…印加），表长 19，不是 21。
- 大佬对象码表（`/home/claude/data/config/dh4/2.png`）给出 21 个圈名，与 21 档首都一一对应。其中 13 个能在 `0x5b656c` 对上（用词「佛兰德」对表里「法兰德斯」）。「英国 / 德国 / 法兰德斯」三词 **不在** SC 连续名表里（国名表是「英吉利 / 德意志」）。圈名采用对象码表，SC 未找到的条目不升格为 exe 字符串。

| id | 对象码表圈名 | 首都港 | SC exe 字符串 |
| ---: | --- | --- | --- |
| 0 | 英国 | 伦敦 | 无连续表；国名表有「英吉利」 |
| 1 | 法兰德斯 | 阿姆斯特丹 | `0x5b656c`「佛兰德」 |
| 2 | 德国 | 汉堡 | 无连续表；国名表有「德意志」 |
| 3 | 北欧 | 斯德哥尔摩 | `0x5b6574`「北欧」 |
| 4 | 葡萄牙 | 里斯本 | 国名表 `0x538eac`「葡萄牙」 |
| 5 | 西班牙 | 塞维利亚 | 国名表「西班牙」 |
| 6 | 意大利 | 热那亚 | `0x5b657c`「意大利」 |
| 7 | 希腊 | 雅典 | `0x5b6584`「希腊」 |
| 8 | 土耳其 | 伊斯坦布尔 | `0x5b658c`「土耳其」 |
| 9 | 埃及 | 亚历山大 | `0x5b6594`「埃及」 |
| 10 | 西非 | 圣乔治 | `0x5b659c`「西非」 |
| 11 | 东非 | 索法拉 | `0x5b65a4`「东非」 |
| 12 | 阿拉伯 | 巴士拉 | 国名表「阿拉伯」 |
| 13 | 印度 | 卡利卡特 | 国名表「印度」 |
| 14 | 印度支那 | 马六甲 | `0x5b65b0`「印度支那」 |
| 15 | 印尼 | 雅加达 | `0x5b65bc`「印尼」 |
| 16 | 中国 | 杭州 | 多处「中国」 |
| 17 | 朝鲜 | 汉城 | `0x5b65c4`「朝鲜」 |
| 18 | 日本 | 大阪 | 国名表「日本」 |
| 19 | 加勒比海 | 哈瓦那 | `0x5b65d0`「加勒比海」 |
| 20 | 墨西哥 | 委拉克鲁斯 | `0x5b65dc`「墨西哥」 |

`region` 字节在 `CTown` 上没有 RTC 名 `eCulture`。同计数 + 21 槽 + 对象码表，配置 key 用 `culture_id` 0–20。

运行时 `CCulture` 对象 stride：**SC 调试版** `CCultureElement` 构造参数 `0xE8`（`CCultureElement.cpp`）。大佬 ES.exe 笔记静态表寻址 `imul …, 0xE0`、基址 `+5B6160`（`dh4/3.png`、`dh4/6.png`）。两套步长不要合成一个数。SC 文件偏移 `0x5B6160` 起按 `0xE0` 切 21 块时，只有第 0 块前 105 个 u16 像物价（4000、2200…），其余块不是干净的 105 价表。静态初值表布局 **未钉死**。

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
# 21 行。name 来自大佬对象码表（dh4/2.png）。SC 无连续 21 名表。
- { id: 0,  key: england,     name: 英国,     capital_town_id: 0 }
- { id: 1,  key: flanders,    name: 法兰德斯, capital_town_id: 2 }
- { id: 2,  key: germany,     name: 德国,     capital_town_id: 5 }
- { id: 3,  key: norse,       name: 北欧,     capital_town_id: 7 }
- { id: 4,  key: portugal,    name: 葡萄牙,   capital_town_id: 11 }
- { id: 5,  key: spain,       name: 西班牙,   capital_town_id: 13 }
- { id: 6,  key: italy,       name: 意大利,   capital_town_id: 15 }
- { id: 7,  key: greece,      name: 希腊,     capital_town_id: 19 }
- { id: 8,  key: turkey,      name: 土耳其,   capital_town_id: 22 }
- { id: 9,  key: egypt,       name: 埃及,     capital_town_id: 25 }
- { id: 10, key: west_africa, name: 西非,     capital_town_id: 29 }
- { id: 11, key: east_africa, name: 东非,     capital_town_id: 34 }
- { id: 12, key: arabia,      name: 阿拉伯,   capital_town_id: 38 }
- { id: 13, key: india,       name: 印度,     capital_town_id: 42 }
- { id: 14, key: indochina,   name: 印度支那, capital_town_id: 47 }
- { id: 15, key: indonesia,   name: 印尼,     capital_town_id: 50 }
- { id: 16, key: china,       name: 中国,     capital_town_id: 54 }
- { id: 17, key: korea,       name: 朝鲜,     capital_town_id: 57 }
- { id: 18, key: japan,       name: 日本,     capital_town_id: 59 }
- { id: 19, key: caribbean,   name: 加勒比海, capital_town_id: 61 }
- { id: 20, key: mexico,      name: 墨西哥,   capital_town_id: 65 }
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
| 21 圈名在 SC 的连续表 | 无；对象码表 21 名与首都档对齐；13 词在 `0x5b656c` |
| SC `CCulture` stride `0xE8` vs ES 笔记 `0xE0` | 两套步长，未合成 |
| `0x5B6160` 是否 21×0xE0 初值价表 | 仅第 0 块像 105 个物价 |
| 港口 `+0x25` 的原版类型名是不是 `eCulture` | 无 RTC 名 |
| `NUM_COUNTRY` 整数 | 未还原；不要写成 21 |
| `NUM_SEAAREA=25` 与七大海域的从属表 | 未见 25→7 的对照数组 |
| 「全世界」是否运行时 id 7 | 影响力循环到 7 为止，没有第 8 格 |
| `0x5b656c` 13 词、`0x538e8c` 19 国名 | 表长对不上 21，用途未钉 |

上一版 YAML 里的 `culture_circle_id` 指 0–6：作废。改用 `major_sea_id`。
