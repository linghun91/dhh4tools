# 七大海域与 21 文化圈（更正）

上一版把 `CForce+0x10` 的 7×uint16 写成「文化圈」。大佬看完后纠正：**七大海域，21 个文化圈。**

本篇只写源码对得上的计数与名字。21 个文化圈在 SC exe 里**有**静态表（名字指针 + 所属海域 + 105 个物价 word），见 §2.3。

> 二次核查（本版）：上一版说「SC 无 21 名表」「SC 0xE8 与 ES 0xE0 两套步长」「未见 25→7 对照」，都是把 `.rdata` 的 VA 当成文件偏移算错了位置（`.rdata` 文件偏移 = VA − `0x140001200`，不是 `.text` 的 VA − `0x140000A00`）。已按汇编改正。

## 1. 职责边界

管：

- **七大海域注册表**（文件按 05 目录树放 `config/world/major_seas.yaml`；ID 域登记与 `MajorSea.*` 查询归本域）：全仓唯一的 `major_sea` 定义。被势力影响力、`CForceFailure` 槽、BGM（world 08）、遗迹奖励与霸者之证（world 09/11）共用，别处只引用 key，不再各自列表
- **21 文化圈**的 ID 空间、显示名、所属大海域（行与外键）。同一行上的 105 卖价与流行推力是 economy 的数值列（`config/economy/culture_prices.yaml`，D10）
- 这两轴和航海分区（`sea_area`，world）、**9 张 NPC 脸**、**`eCountry`** 的分界

不管：

- 航海分区的网格/邻港（`01-world/01-map-and-sea-areas.md`）
- 文化圈卖价、流行推力的数值与公式（economy 02）
- 9 脸换装（仍在 `02-countries-culture-fashion.md`）

## 2. 原版事实

### 2.1 四套 ID，计数不同

| 轴 | 计数 | 源码落点 | 已抽出的名字 |
| --- | ---: | --- | --- |
| **七大海域** | 7 | `CForce` 存盘循环 `s20 >= 7`，读写 `this+0x10+i*2`；RTC 局部名 **`eSeaArea` 上界 7**；海域表 VA `0x14058bb30` 8 条（含「全世界」） | 北海、地中海、非洲、印度洋、东南亚、东亚、新大陆；第 8 条「全世界」 |
| **21 文化圈** | 21 | `CCultureElement` 构造 `fn_140007dce(..., stride=0xE8, count=0x15)`；静态表 VA `0x1405b6160` 21×`0xE0` | 21 名，见 §2.3 |
| ~~25 航海分区~~ | `[推断]` 不存在 | `SeaAreaID.h` 的 `NUM_SEAAREA=25` 是 RTC 投票误采（见 §2.2）；没有 25 条的表或判定函数（world 报告） | — |
| **9 NPC 脸** | 9 文案 | 人物 `family`：欧洲、非洲、阿拉伯、印度、东南亚、中国、韩国、日本、中南美 | 这 9 个词 |

`eCulture` / `eCountry` 头文件只有符号，`NUM_CULTURE` / `NUM_COUNTRY` 整数未写进 `*ID.h`。21 来自 `CCultureElement` 的 `0x15` 与静态表 21 条，不是头文件常量。

`NUM_*` 证据类型：七大海域 7 = 存盘循环上界 + RTC `eSeaArea` 函数内上界；文化圈 21 = 容器计数 `0x15` + 静态表行数；两者都不是头文件常量。

`eCountry` 仍然存在（`CountryID.h`、`asserts.txt`）。源码里**没有**把 `NUM_COUNTRY` 钉成 21。港口 `+0x25`（0–20）就是 `culture_id`（§2.3 有直接证据），**不是 `eCountry`**。

### 2.2 七大海域 `[确认]` 布局与名字

- `[确认]` `CForce::vfunc_06/07`：`s20` 从 0 加到 `>= 7`，每次 `fn_14000a19b` / `fn_140004421` 读写 `&self + s20*2 + 0x10` 一个 word。来源：`srcdata/source/game/world/CForce.cpp`。
- `[确认]` 宽度与 `FORCE_INFLUENCE`（USHORT）一致。加减 `fn_14000ae7a`（body `0x140106540`）：`clamp(0, cur + d, 0x270F)`，即**每格封 9999**。来源：`Force.h` 注释、objdump。
- `[确认]` 下列函数的 RTC 变量名是 **`eSeaArea`**，比较上界是 **7**：
  - `fn_140008b6b` body `0x14010a6c0`（`fn_140107150.cpp`）：对 7 格 `fn_1400023ab`（读 `this+0x10+i*2`）求和 = **势力总影响力**
  - `fn_14000a8c1` body `0x14010cf80`（文件 `fn_14010be70.cpp` 第 455 行 `if (eSeaArea >= 7)`）：势力灭亡时按 `eSeaArea` 0–6 扫 `CForceFailure` 槽（`fn_140006573`）
- `[确认]` `fn_140006573`（body `0x1400bb9c0` → `0x1400bba00`）：`CForceElement + 0x8A0 + i*0x10`。即 `CForceFailure` 7 槽**按七大海域下标**。来源：objdump。
- `[推断]` `eSeaArea` 就是七大海域，`NUM_SEAAREA` 原值应为 7。`SeaAreaID.h` 的 25 由 `srcdata/tools/fill_gaps.py` 生成：它对每个带 RTC 名的函数从入口固定扫 `0x2000` 字节、取 `cmp [rsp+var], imm` 的众数，会越过函数尾。`num_from_rtc.json` 里 `eSeaArea` 的票是 `{25: 7, 7: 2}`；7 张 25 票全部来自 `fn_140008b6b`（体长 `0x7A`）之后的邻函数（`cmp [rsp+0x34], 0x19`），两张 7 票正是上面两个函数体内的上界。所以头文件 25 不能当证据，「25 航海分区」不要再建表。
- `[确认]` 海域表：`.rdata` VA `0x14058bb30`（文件 `0x58a930`），**8 条 × `0x38`**：`+0` 名字 `char*`，`+8` 矩形 `x1,y1,x2,y2`（4×int），`+0x18` 8 个 int 邻接（`8` = 无）。访问器 `fn_140004403`（body `0x14020c380`，`0 <= i < 7`）与 `fn_140011423`（body `0x14020c3c0`，`0 <= i < 8`）。
- `[确认]` 名字串在文件偏移 `0x58ab4c`（VA `0x14058bd4c`）起连续 GBK：

| id | 短名 | 海域表矩形 (x1,y1,x2,y2) |
| ---: | --- | --- |
| 0 | 北海 | 19200,2720,22880,5120 |
| 1 | 地中海 | 17760,3840,24320,8160 |
| 2 | 非洲 | 16640,8000,26368,14400 |
| 3 | 印度洋 | 24320,5920,31360,10560 |
| 4 | 东南亚 | 30560,8000,35200,11200 |
| 5 | 东亚 | 31360,5056,35840,8000 |
| 6 | 新大陆 | 7040,5200,16640,11520 |
| 7 | 全世界 | 0,0,40000,20000 |

「全世界」是海域**表**的第 8 条（地图视图用，`fn_140011423` 允许 7）。影响力数组与 `CForceFailure` 只有 7 格，**没有**第 8 格。

长名（「北海海域」「北海海域周边」「东南亚周边海域」…）出现在 `fn_140009750`（RTC `SeaAreaMessage`，160 字节 = **20 个**指针）。唯一调用点在古地图信息框 `fn_140002e50`（`OldMap.dk4`），下标 0–19 **不是** `major_sea_id`。长名只当文案池用，不当本轴的名字表。

霸者之证 mesfile `0x1e247` 起七句「这是%s的霸者之证」顺序与上表 0–6 相同。那是**同一套 7 个名字**被叙事/证物引用，不是另一套枚举。

World0 势力 `+0x10` 活跃者恰好一格非 0，格号与母港所属文化圈的 `major_sea_id` 一致（里斯本→葡萄牙圈→格 1 地中海，杭州→中国圈→格 5 东亚）。`[确认]`：World0 19 条活跃记录、World1 新出场的图鲁维（里加→北欧圈→格 0）/ 朝鲜水师（汉城→朝鲜圈→格 5）逐条对过。

### 2.3 21 文化圈 `[确认]` 静态表、名字、所属海域

- `[确认]` `CCulture : CSaveObject`。`CCultureElement` 分配 **21** 个 `CCulture`，对象 stride `0xE8`。来源：`CCultureElement.cpp` 构造参数 `0x15`；析构/存读盘循环 `>= 0x15`。
- `[确认]` 静态初值表在 SC exe `.rdata` **VA `0x1405b6160`**（文件偏移 `0x5b4f60`），**21 条 × `0xE0`**。访问器 `fn_140010f73` body `0x1402ac510`：`imul rax,rax,0xE0; lea rcx,[0x1405b6160]`。大佬 ES 截图（`dh4/3.png`）的 `ES+2AC510` / `ES+5B6160` 是**同一 RVA、同一张表**。
- `[确认]` `0xE8` 与 `0xE0` 不是两套步长：运行时对象 = 8 字节 vptr + `0xE0` 载荷。`CCulture::vfunc_05`（body `0x1402ac830`）逐字段拷贝：

| 静态表偏移 | 宽 | → `CCulture` 偏移 | 内容 | 存盘 |
| --- | --- | --- | --- | --- |
| `+0x00` | 8 | `+0x08` | 名字 `char*` | 否 |
| `+0x08` | 1 | `+0x10` | **`major_sea_id`**（0–6） | 否 |
| `+0x09` | 2×1 | `+0x11` | 月初流行推力 上推 / 下拉，值 0–7（`fn_1400077bb`；`04-economy/02-market-price.md` §2.3 `[确认]`）；economy 数值列 | 否 |
| `+0x0C` | 105×2 | `+0x14` | 每交易品一个 u16 卖价底；economy 数值列 | **是**（`vfunc_06/07` 只存这 105 word） |

- `[确认]` 港口 → 文化圈 → 大海域链（objdump）：
  - `fn_14001055a`（body `0x140260370`）：`GetFixTown(town)+0x25` 字节
  - `fn_140013a02`（body `0x140260310`）：上值 `== 0x16` 返回空，否则 `fn_1400130d9` → `fn_140005b0a`：`CCultureElement + 8 + i*0xE8`
  - `fn_14000aa0b`（body `0x1400c61e0`）：读 `CCulture+0x10` 字节
  - `fn_140014e48`（body `0x140261330`）= 上面三步串起来：**town → major_sea_id**
  - 用处：NPC 舰队交易 `fn_140010d25`（body `0x1400f6d20`）用它取 `CForceFailure[major_sea_id]`；投资转影响力用它选格（`04-economy/07-investment-tribute.md`）
- 所以港口表 `+0x25` 就是 `culture_id`（升格 `[确认]`）。空值字节是 `0x16`（22），不是 `0x15`：与 98/106/232/236/70/201 同为「计数 + 1」惯例（观察，不是证明）。只进 `ids/legacy_codec.yaml`。
- `[确认]` 没有 `GetFixCultureProc`。文化块不在 World DLL，World0/World1 共用 exe 这张表。来源：`DllMain.cpp`。
- `[确认]` 港口表 `+0x25` 取值 0–20，共 21 档；21 座 `is_capital==1` 港各占一档。来源：`Docs/_data/towns_world0.json`。
- `[确认]` 港口情报 UI 有 `posCulture`（`fn_140005f51` RTC）。

21 条（SC exe 名字指针解出；首都按港口表 `culture_id==id && is_capital==1`）：

| id | SC exe 名 | 对象码表名（`dh4/2.png`） | major_sea_id | 首都港 | `+0x09/+0x0A` |
| ---: | --- | --- | ---: | --- | --- |
| 0 | 英吉利 | 英国 | 0 北海 | 0 伦敦 | 7, 4 |
| 1 | 佛兰德 | 法兰德斯 | 0 北海 | 2 阿姆斯特丹 | 5, 7 |
| 2 | 德意志 | 德国 | 0 北海 | 5 汉堡 | 1, 2 |
| 3 | 北欧 | 北欧 | 0 北海 | 7 斯德哥尔摩 | 2, 1 |
| 4 | 葡萄牙 | 葡萄牙 | 1 地中海 | 11 里斯本 | 3, 3 |
| 5 | 西班牙 | 西班牙 | 1 地中海 | 13 塞维利亚 | 2, 1 |
| 6 | 意大利 | 意大利 | 1 地中海 | 15 热那亚 | 6, 6 |
| 7 | 希腊 | 希腊 | 1 地中海 | 19 雅典 | 2, 4 |
| 8 | 土耳其 | 土耳其 | 1 地中海 | 22 伊斯坦布尔 | 4, 3 |
| 9 | 埃及 | 埃及 | 1 地中海 | 25 亚历山大 | 3, 2 |
| 10 | 西非 | 西非 | 2 非洲 | 29 圣乔治 | 2, 1 |
| 11 | 东非 | 东非 | 2 非洲 | 34 索法拉 | 3, 2 |
| 12 | 阿拉伯 | 阿拉伯 | 3 印度洋 | 38 巴士拉 | 1, 3 |
| 13 | 印度 | 印度 | 3 印度洋 | 42 卡利卡特 | 4, 1 |
| 14 | 印度支那 | 印度支那 | 4 东南亚 | 47 马六甲 | 2, 0 |
| 15 | 印尼 | 印尼 | 4 东南亚 | 50 雅加达 | 0, 2 |
| 16 | 中国 | 中国 | 5 东亚 | 54 杭州 | 2, 1 |
| 17 | 朝鲜 | 朝鲜 | 5 东亚 | 57 汉城 | 3, 2 |
| 18 | 日本 | 日本 | 5 东亚 | 59 大阪 | 4, 5 |
| 19 | 加勒比海 | 加勒比海 | 6 新大陆 | 61 哈瓦那 | 2, 2 |
| 20 | 墨西哥 | 墨西哥 | 6 新大陆 | 65 委拉克鲁斯 | 0, 1 |

名字串的来源：13 条是专用串（文件 `0x5b656c` 起：佛兰德、北欧、意大利、希腊、土耳其、埃及、西非、东非、印度支那、印尼、朝鲜、加勒比海、墨西哥）；7 条**复用**国名串池（文件 `0x538e8c` 起：英吉利、德意志、葡萄牙、西班牙、阿拉伯、印度、日本）；「中国」指向 VA `0x1405b2414`。上一版「13 词 / 19 国名凑不出 21」只看了串池，没看指针表。

显示名按 `designer-spreadsheets.md` 的规矩**用 exe**（英吉利 / 佛兰德 / 德意志），对象码表名进 `alias`。

105 个 word：`[确认]` **文化圈卖价底**。`fn_140010a23`（body `0x140169f00`）按 `CCulture+0x14+item*2` 取值；economy 已追到卖价路径与两处运行时写价，见 `04-economy/02-market-price.md` §2.6。数值 50–9600；胡椒（`trading_item 0x16`）英吉利 8750、意大利 7500、印度 1650、印尼 1300、加勒比海 7700——欧洲贵、产地便宜。表与存档归 economy（`culture_prices.yaml`，按 `culture` key + `trading_item` key 稀疏映射）。

### 2.4 国名表（`eCountry` 候选，不是文化圈）

- `[确认]` `.data` VA `0x140682780` 起 **19** 个 `char*`：英吉利、法兰西、荷兰、西班牙、葡萄牙、瑞典、威尼斯、热那亚、德意志、俄国、丹麦、波兰、奥斯曼帝国、阿拉伯、印度、明朝、日本、阿兹特克、印加；第 20 格为 0。访问器 `fn_14000ddeb` body `0x140091840`（无界检查，紧挨 `CCulture` 代码）。
- `[未知]` `fn_14000ddeb` 在 `.text` 里没有 `call/jmp` 直接引用（`xref.py` 只找到 thunk 自身）。用途未钉。
- `[推断]` `NUM_COUNTRY` 候选 = 19。与 21 文化圈**不是**同一轴。不要升格。

### 2.5 不要再混的几件事

| 说法 | 源码 |
| --- | --- |
| 把 7 格叫文化圈 | 错。RTC 名是 `eSeaArea`，短名是海域。 |
| 把头文件 `NUM_SEAAREA=25` 当证据 | 错。25 是投票误采；`eSeaArea` 函数体内上界都是 7。 |
| 把 9 张 NPC 脸当 21 文化圈 | 错。9 是人物 `family` 文案。 |
| 「SC 没有 21 圈名」 | 错。静态表 `0x1405b6160` 每条首 8 字节是名字指针。 |
| 「`0xE8` 与 `0xE0` 两套步长」 | 错。`0xE0` 静态记录，`0xE8` = vptr + 记录。 |
| 「未见 21→7 / 25→7 对照」 | 21→7 有：静态表 `+0x08`。25→7 不存在，因为 25 轴本身没有证据。 |
| 把 19 个国名当 21 文化圈 | 错。19 国名是另一张表（§2.4）。 |
| 港口 `+0x25` 字节 = `eCountry` | 错。它直接当 `CCultureElement` 下标（= `culture_id`）。 |

## 3. 实体与 ID

| 实体 | 原版计数 | 空值 | 配置 key |
| --- | ---: | --- | --- |
| 七大海域 | 7（表 8 条，第 8 条「全世界」只给地图） | 影响力无第 8 格；运行时 `null` | `major_sea` |
| 21 文化圈 | 21 | 运行时 `null`；原版港口字节 `0x16` 只在 legacy_codec | `culture` |
| 细分海域（重构扩展） | 原版 0 条（`eSeaArea` 即七大海域，25 是投票误采，01-world/01 §2） | 运行时 `null` | `sea_area`（world，`sea_areas.yaml` 原版空表） |
| 9 NPC 脸 | 9 | 无枚举文件 | `npc_face_id`（人物文档） |
| 国名 | 19（`[推断]`） | `INVALID_COUNTRY=-1` | `country_id` |

影响力：`Influence[force_id][major_sea_id] : ushort`。按 major_sea 表长，不按 7。

外键链：`town.culture → culture.major_sea`（键，不是下标）。

## 4. 算法 / 公式

```
major_sea_of_town(town) := culture[town.culture_id].major_sea_id   # 原版 fn_140014e48
total_influence(force)  := Σ_{s in major_seas} Influence[force][s] # 原版 fn_140008b6b
```

势力影响力读写按 major_sea 表长。文化圈数值列（105 卖价、推力）由 economy 按 `culture` key 存取，开局从静态表导出，之后进 economy 存档节。

原版包：`major_sea_count = 7`，`culture_count = 21`。运行时按表长，不写死。

## 5. 耦合分析

原版：

- 7 格焊在 `CForce` 结构体里；`CForceFailure` 7 槽焊在 `CForceElement+0x8A0`
- 21 个 `CCulture` 焊在 `CWorld` 的 `CCultureElement` 上；初值是 exe `.rdata` 常量，模组改不了
- `CCulture` 的 105 循环与交易品表长绑死（`0x69`）
- 港口 `+0x25` 直接当文化圈数组下标

重构：

- 七大海域表独立；势力只存 `major_sea_id → 影响力`
- 文化圈表独立，持 `major_sea_id` 外键；港口存 `culture_id`
- `CCulture` 的 105 word 与推力是 economy 数值列，按 `trading_item` key 稀疏存，加载期校验覆盖率（缺项走 economy 默认）
- `sea_area` 属 world，是重构扩展的细分海域；原版包为空表（01-world/01 已判 `eSeaArea` = 七大海域，`NUM_SEAAREA = 7` `[推断]`）

## 6. 解耦后的配置

```yaml
# config/world/major_seas.yaml   全仓唯一的七大海域注册表（文件在 world 目录；ID 域由 factions 登记）
schema: dk4.world.major_seas
version: 1
# 表长 = 影响力格数 = CForceFailure 槽数 = 霸者之证 / BGM / 遗迹奖励列的键空间
major_seas:
  - { key: vanilla:north_sea,     vanilla_index: 0, name_key: major_sea.north_sea,     rect: [19200, 2720, 22880, 5120] }
  - { key: vanilla:mediterranean, vanilla_index: 1, name_key: major_sea.mediterranean, rect: [17760, 3840, 24320, 8160] }
  - { key: vanilla:africa,        vanilla_index: 2, name_key: major_sea.africa,        rect: [16640, 8000, 26368, 14400] }
  - { key: vanilla:indian_ocean,  vanilla_index: 3, name_key: major_sea.indian_ocean,  rect: [24320, 5920, 31360, 10560] }
  - { key: vanilla:se_asia,       vanilla_index: 4, name_key: major_sea.se_asia,       rect: [30560, 8000, 35200, 11200] }
  - { key: vanilla:east_asia,     vanilla_index: 5, name_key: major_sea.east_asia,     rect: [31360, 5056, 35840, 8000] }
  - { key: vanilla:new_world,     vanilla_index: 6, name_key: major_sea.new_world,     rect: [7040, 5200, 16640, 11520] }
world_view: { key: vanilla:whole_world, name_key: major_sea.whole_world, rect: [0, 0, 40000, 20000] }  # 不是 major_sea 行
# 邻接（原版 +0x18 的 8 个 int，8=无）由提取器生成，语义（方向？）[未知]
# 显示名「北海…新大陆」进 strings（exe 消息号表），不写在本表
```

```yaml
# config/factions/cultures.yaml   文化圈「行」：id 与外键归 factions
schema: dk4.factions.cultures
version: 1
# 21 行。name 来自 SC exe 静态表 0x1405b6160；alias 是对象码表（dh4/2.png）
# 首都不在本表：world/towns.yaml 的 is_capital + culture 派生，避免两处定义
cultures:
  - { key: vanilla:england,     vanilla_index: 0,  name: 英吉利,   alias: 英国,     major_sea: vanilla:north_sea }
  - { key: vanilla:flanders,    vanilla_index: 1,  name: 佛兰德,   alias: 法兰德斯, major_sea: vanilla:north_sea }
  - { key: vanilla:germany,     vanilla_index: 2,  name: 德意志,   alias: 德国,     major_sea: vanilla:north_sea }
  - { key: vanilla:norse,       vanilla_index: 3,  name: 北欧,     major_sea: vanilla:north_sea }
  - { key: vanilla:portugal,    vanilla_index: 4,  name: 葡萄牙,   major_sea: vanilla:mediterranean }
  - { key: vanilla:spain,       vanilla_index: 5,  name: 西班牙,   major_sea: vanilla:mediterranean }
  - { key: vanilla:italy,       vanilla_index: 6,  name: 意大利,   major_sea: vanilla:mediterranean }
  - { key: vanilla:greece,      vanilla_index: 7,  name: 希腊,     major_sea: vanilla:mediterranean }
  - { key: vanilla:turkey,      vanilla_index: 8,  name: 土耳其,   major_sea: vanilla:mediterranean }
  - { key: vanilla:egypt,       vanilla_index: 9,  name: 埃及,     major_sea: vanilla:mediterranean }
  - { key: vanilla:west_africa, vanilla_index: 10, name: 西非,     major_sea: vanilla:africa }
  - { key: vanilla:east_africa, vanilla_index: 11, name: 东非,     major_sea: vanilla:africa }
  - { key: vanilla:arabia,      vanilla_index: 12, name: 阿拉伯,   major_sea: vanilla:indian_ocean }
  - { key: vanilla:india,       vanilla_index: 13, name: 印度,     major_sea: vanilla:indian_ocean }
  - { key: vanilla:indochina,   vanilla_index: 14, name: 印度支那, major_sea: vanilla:se_asia }
  - { key: vanilla:indonesia,   vanilla_index: 15, name: 印尼,     major_sea: vanilla:se_asia }
  - { key: vanilla:china,       vanilla_index: 16, name: 中国,     major_sea: vanilla:east_asia }
  - { key: vanilla:korea,       vanilla_index: 17, name: 朝鲜,     major_sea: vanilla:east_asia }
  - { key: vanilla:japan,       vanilla_index: 18, name: 日本,     major_sea: vanilla:east_asia }
  - { key: vanilla:caribbean,   vanilla_index: 19, name: 加勒比海, major_sea: vanilla:new_world }
  - { key: vanilla:mexico,      vanilla_index: 20, name: 墨西哥,   major_sea: vanilla:new_world }
```

同一静态行的数值列（`+0x09/+0x0A` 推力、`+0x0C` 105 卖价）**不在本域**：`config/economy/culture_prices.yaml`（`04-economy/02-market-price.md` §6），以 `culture` key 关联。提取器读同一行，按所有权分发到两张表（D10）。本域不再定义 `culture_prices`。

势力影响力初值（01 `forces.yaml`）：

```yaml
major_sea_influence: { vanilla:mediterranean: 122 }   # 卡斯特罗 World0；缺省 0
```

港口：`culture` 外键（原版 0–20）。不要写 `culture_circle_id` 去指 7 格。

霸者之证 / kind 8 的 `+0x15`（0–6）、BGM `major_sea_bgm`、遗迹 `influence_reward.col_by_major_sea`：外键都是本表 **`major_sea`** key。`01-world/11-remains-puzzle-system.md` 里自带的 `major_seas:` 列表应删去、改引用本表。

## 7. 独立旋钮

- 7 个海域名、矩形、影响力初值
- 21 个文化圈行（可加长；原版包 21）；每行的 `major_sea_id`
- 每文化圈的基准物价 → economy `culture_prices.yaml`
- 航海分区表长不动本轴

## 8. 对外接口

```
MajorSea.list() -> [major_sea_id]
MajorSea.name(id) -> string
MajorSea.of_town(town_id) -> major_sea_id | null  # 经 culture
Culture.list() -> [culture_id]
Culture.of_town(town_id) -> culture_id | null   # 读 world/towns.yaml 的 culture 列
Culture.majorSea(culture_id) -> major_sea_id
Factions.influence(force_id, major_sea_id) -> int   # 06 规范名（03 §8）
Influence.total(force_id) -> int
```

事件：`InfluenceChanged { force_id, major_sea_id, old, new, source }`。`CulturePriceChanged` 由 economy 发；卖价查询用 `Economy.culture_price(culture_id, item_id)`。

外部别名：`Factions.major_sea(town_id)` = `MajorSea.of_town`；`Factions.add_influence(force, sea, v, cap)` = 03 `Influence.add`（`cap` 参数废弃，上限读 caps）。

## 9. 未核实字段

| 项 | 现状 |
| --- | --- |
| 静态表 `+0x09/+0x0A`（→ `CCulture+0x11/+0x12`） | 已由 economy 钉为流行推力（`fn_1400077bb`）；本篇未独立复核 |
| 105 word 运行时写价的全部路径 | economy 02 §2.6 列了两处；本篇未复核 |
| 港口字节空值为何是 `0x16` 不是 `0x15` | 只见 `fn_140013a02` 比较 `0x16` |
| `NUM_COUNTRY` 整数 | 19 国名表是候选；未见断言常量 |
| 19 国名表 `fn_14000ddeb` 的调用者 | 直接 `call/jmp` 扫描为 0 |
| `NUM_SEAAREA=25` | 投票误采（§2.2）；01-world/01 §2 已核：`eSeaArea` 循环上界都是 7，即七大海域，`NUM_SEAAREA = 7` `[推断]`，无 25 轴 |
| 海域表 `+0x18` 8 个 int | 像邻接（8=无），方向/语义未钉 |
| `SeaAreaMessage` 20 格的下标 | 古地图信息框用，键未钉 |

上一版 YAML 里的 `culture_circle_id` 指 0–6：作废。改用 `major_sea_id`。
