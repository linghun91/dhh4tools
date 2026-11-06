# 国家、文化、流行

港口上的 `culture_id`（港口 `+0x25`；YAML 外键 `culture`）、21 个 `CCulture` 槽的**行定义**、9 个 NPC 脸文案、以及每港 2 格流行，都挂在势力域的**内容表**里。设施只消费这些 ID，不拥有表。文化圈行上的数值列（105 卖价、两字节流行推力）归 economy（`config/economy/culture_prices.yaml`），本域只拥有行 id 与外键（DECISIONS D10）。

## 1. 职责边界

**管：**

- `eCountry` / `eCulture` / `eTownFashion` / `eFashionSpeed` 的 ID 空间
- 21 档港口 `culture_id`（港口 `+0x25`，0–20）与首都对照（文化圈名字/海域见 06）
- 文化圈行：id、名字、`major_sea_id` 外键（06）；105 卖价与流行推力是同一行上的 economy 数值列
- 9 张 NPC 脸（人物 `family` 文案）到设施形象的映射
- 每港当前 2 格流行的**定义**（槽 = `{trading_item, level}`）；`eFashionSpeed` 枚举的 ID

**不管：**

- 港口坐标、设施开没开 → `world` / `town-life`
- 流行品如何推种类市价 → `economy`（02 §2.3，读本系统的槽）；卖流行品加份额 / 发信 / 加影响力 → 本目录 03 / 04（订阅 economy 的 `SoldFashionGoods`）
- 港口状态字节 `CTown+0x74/+0x75`（战争、瘟疫、景气…，0–12）→ town 状态（`04-economy/02-market-price.md` §2.3 读它）
- 阿科布流浪、发放品牌商品的事件脚本 → `narrative` / `town-life`
- `CFashionSprite` 等绘制 → `ui`

流行状态存在港口对象上（原版 `CTown +0x70` 两格），但**目录和规则**属于本域，避免城镇设施文档夹一份商品 ID 表。

## 2. 原版事实

### 国家

- `[确认]` `eCountry` 存在，`INVALID_COUNTRY=-1`，`NUM_COUNTRY` 整数未还原。来源：`CountryID.h`、`asserts.txt`。
- `[确认]` 工程里**没有** `CCountry` / `CCountryElement`。国家不是 World DLL 表。来源：`class_hierarchy.txt`、`DllMain.cpp` 导出只有 Town/Person/Item/Force/Fleet。
- `[确认]` 港口表 `+0x24` 为首都标记（1/0），`+0x25` 为文化圈字节（`culture_id`），取值 0–20 共 21 档。来源：`Docs/_data/towns_world0.json`。
- `[确认]` `+0x25` 就是 `culture_id`：`fn_14001055a` 读 `GetFixTown+0x25`，`fn_140013a02` 拿它当 `CCultureElement` 下标（空值 `0x16`）。见 [06](06-seven-culture-circles.md) §2.3。不要把 21 写成 `NUM_COUNTRY`。
- `[确认]` 另有 19 条国名指针表（`.data` VA `0x140682780`：英吉利、法兰西、荷兰…阿兹特克、印加），访问器 `fn_14000ddeb`，未见直接调用。`[推断]` 这才是 `eCountry` 候选，19 ≠ 21。见 06 §2.4。

21 档首都（`is_capital=1` 且 `culture_id=N` 的港口）：

| culture_id | 首都 | 同档其它港（World0） |
| --- | --- | --- |
| 0 | 伦敦 | 布里斯托尔 |
| 1 | 阿姆斯特丹 | 布鲁日、南特、雷利史塔特、圣马罗 |
| 2 | 汉堡 | 吕贝克 |
| 3 | 斯德哥尔摩 | 奥斯陆、哥本哈根、里加 |
| 4 | 里斯本 | 休达、马德拉 |
| 5 | 塞维利亚 | 瓦伦西亚、拉斯帕马斯 |
| 6 | 热那亚 | 马赛、锡拉库萨、威尼斯、拿坡里 |
| 7 | 雅典 | 克里特、塞浦路斯 |
| 8 | 伊斯坦布尔 | 拉古札、贝鲁特、阿布哈兹 |
| 9 | 亚历山大 | 的黎波里、阿尔及尔、突尼斯、班加西 |
| 10 | 圣乔治 | 绿角、罗安达、塞拉利昂、圣多美 |
| 11 | 索法拉 | 开普敦、莫桑比克、摩加迪沙、马达加斯加、蒙巴萨 |
| 12 | 巴士拉 | 亚丁、马斯喀特、霍尔木兹、索哥德拉 |
| 13 | 卡利卡特 | 果阿、锡兰、加尔各答、安曼、卡拉奇、马德拉斯、马斯利巴丹 |
| 14 | 马六甲 | 文莱、马尼拉、吉阿丁 |
| 15 | 雅加达 | 巴邻旁、德尔纳特、安波那、亚齐、班贾尔马辛、望加锡、泗水、马纳多 |
| 16 | 杭州 | 泉州、澳门、沂州、淡水 |
| 17 | 汉城 | 釜山 |
| 18 | 大阪 | 长崎、那霸、江户 |
| 19 | 哈瓦那 | 圣多明尼各、圣约翰、牙买加、朋沙科拉 |
| 20 | 委拉克鲁斯 | 梅里达、波托韦洛、马拉开波、伯南布哥、特鲁希略、卡宴、卡拉卡斯 |

国名与港口的对照原版未见（19 国名表无调用者）。UI 港口情报有 `posKind` / `rcKindTitle`（`fn_140005f51`），文案 key 另表。

### 文化

- `[确认]` `eCulture` 存在，`INVALID_CULTURE=-1`，`NUM_CULTURE` 整数未还原。来源：`CultureID.h`。
- `[确认]` `CCulture : CSaveObject`。`CCultureElement` 构造 **21** 个（`0x15`）对象，stride **0xE8**（= 8 字节 vptr + `0xE0` 载荷）。析构 / 存盘同样循环 `>= 0x15`。来源：`CCultureElement.cpp`。
- `[确认]` `CCulture` 存盘（`vfunc_06/07`）**只**遍历 `eTradingItem` 0–0x68（105）：每个 word 在 `+0x14 + i*2`。`+0x08`（名字指针）、`+0x10`（`major_sea_id`）、`+0x11/+0x12` 只在 `vfunc_05` 开局写，不存盘。来源：`CCulture.cpp`、objdump `0x1402ac830`。
- `[确认]` 没有 `GetFixCultureProc`。开局由 `vfunc_05` 从 exe `.rdata` 静态表（VA `0x1405b6160`，21×`0xE0`，访问器 `fn_140010f73`）拷贝。World0/World1 共用。来源：`DllMain.cpp`、objdump。
- `[确认]` 21 名在静态表里（英吉利、佛兰德、德意志、北欧…加勒比海、墨西哥），每条还带 `major_sea_id`。全表见 [06-seven-culture-circles.md](06-seven-culture-circles.md) §2.3。
- `[确认]` 港口 `+0x25` 直接当 `CCultureElement` 下标（06 §2.3）。`CTown` 上虽无 RTC 名 `eCulture`，取值语义已钉死。
- `[确认]` 人物表从 id 98 起，`family` 字段出现 9 个脸部分组文案：**欧洲、非洲、阿拉伯、印度、东南亚、中国、韩国、日本、中南美**。广场店主把欧洲拆成「欧洲男子 / 欧洲女子」，故店主 10 人、组仍是 9。来源：`Docs/_data/persons.tsv`、`PersonID` 记录。这 9 个不是 21 文化圈。
- `[确认]` 同 9 词还用在酒馆掌柜/客人、交易所掌柜、造船老板、码头、卫兵、国王/藩主/县令/大名/总督、宗教人士、旅馆老板娘、行会总管、属下等职能 NPC 上。来源：同上。
- `[推断]` 9 圈是设施 NPC 换装 / 对白的粗分组，**不是** `eCulture` 的计数。`NUM_KIND=9`（`num_values.json`）可能相关，未证明。

### 流行

- `[确认]` `eTownFashion` 符号存在，`INVALID_TOWNFASHION=-1`。`TownFashionID.h` 的 `NUM_TOWNFASHION = 26` 是 **RTC 投票误采**：`catalog/num_from_rtc.json` 里 `eTownFashion` 的比较立即数是 `{2: 19 次, 3: 4, 26: 2, 97: 1}`，`fill_gaps.py` 只收 `>= 4` 的立即数，所以 2 被丢、取了 26（且扫描窗口固定 `0x2000` 会越过函数尾）。`[推断]` `NUM_TOWNFASHION = 2`：港内流行槽数。
- `[确认]` `NUM_FASHIONSPEED` 只有符号。来源：`FashionspeedID.h`、`Fashion.h` 注释。
- `[确认]` 玩法循环几乎都是 `eTownFashion >= 2` 就结束：每港**当前流行 2 格**，不是 26 格全亮。来源：`CPriceInformationView.cpp`、`fn_14025ae30.cpp`、`fn_1402645a0.cpp`、`fn_140260c10.cpp` RTC。
- `[确认]` `CTown` 存盘：`i=0,1` 各写 `+0x70+2i`、`+0x71+2i` 两字节（2 槽 × 2 字节），随后 `+0x74`、`+0x75` 各一字节。来源：`CTown.cpp` `vfunc_06/07`。`+0x74` 是**港口状态**（0–12，12 = 平常；月初漂移按它查跳表 `0x140267cc8`），`+0x75` 是本状态已持续月数——都不是流行字段，见 `04-economy/02-market-price.md` §2.3。
- `[确认]` 开局 `CTown::vfunc_05` 把 FixTown `+0x7E`、`+0x80` 两个 word 拷到 `+0x70`、`+0x72`，`+0x82` 一个 word 拷到 `+0x74`。World0 97 港全是 `0x006A, 0x006A, 0x000C`。`0x6A = 106` 即 `ITEM_EMPTY`（`num_values.json`）。`+0x7E` 每格两字节：u8 trading_item + u8 等级 `[确认]`；原版按 word 拷贝（低字节 `0x6A` = 交易品空，高字节 0 = 等级）。来源：objdump `0x1402ae65f`、`World0_town.c`。
- `[确认]` 槽 = `{u8 trading_item, u8 level}`（空 item = 106）。月初漂移对每格：`level < 120` 时该商品种类市价上推，否则下拉（`0x14026765e`–`0x140267762`，见 economy 02 §2.3）。`level` 如何随时间变化、何时清槽 `[未知]`；`fn_14000dc6a(town, slot, 件数)` 在每笔卖出后调用（economy 03 §2.6 第 16 步），`[推断]` 是推进 level 的入口。
- `[确认]` 交易所卖出当前流行品会计入 `anFashionSellAmount`，再改占有率或发介绍信 / 推荐信。来源：`srcdata/docs/trade_stack.md`（`CommitDeal 0x140277090`）。份额公式在 03，本篇只承认「流行品是份额的输入」。
- `[确认]` `fn_1402767f0` 扫 97 港，建两张 388 字节候选表（`97×4`）：`eTownShareTable`、`eTownNotFashionTable`。无流行的港可被抽中开新流行，并 `fn_14000fb14(0xfd, …)`。来源：`fn_1402755b0.cpp`。
- `[确认]` 广场文案：选零售店招牌听流行情报；「据说在 %s 将流行 %s」「听说 %s 正流行 %s」「这城市好像正在流行 %s」。来源：`mesfile_sc.txt` `0x1143`、`0x8653`、`0x86fb`、`0x8727`。
- `[确认]` 流行生命周期文案三句：「今后 %s 的某一个物品一定会流行起来」「好像还要继续流行下去」「%s 的流行也该结束了吧」。来源：`0x6ebb`、`0x6ef7`、`0x6f0f`。
- `[确认]` 阿科布要把「新的流行趋势带到世界的所有城市」；发放品牌商品 `%s` 做宣传。来源：`0x6fa3`、`0x728b`。
- `[确认]` 唯一流行类 RTTI 是 `CFashionSprite`（绘制），没有 `CFashion` 存档对象。流行状态住在港口上。来源：`class_hierarchy.txt`、`Fashion.h`。
- `[推断]` 上一版说「流行空格更像 `0x1A=26`」与开局数据矛盾：槽初值是 `0x6A`（交易品空）。流行目录就是交易品表本身，「26 条流行目录」不存在。`00-overview/04-id-registry.md` 把港口表 `+0x4C` 当 `eTownFashion`（空 = 26）是误读：那是份额槽（03 §2）。
- `[推断]` `NUM_FASHIONSPEED ≥ 3`，对应「将流行 / 继续 / 结束」三句。整数未知。与槽 `level` 的关系未钉。

## 3. 实体与 ID

| 枚举 | 原版计数 | 空 / 非法 | 备注 |
| --- | --- | --- | --- |
| `eCountry` | `[推断]` 19（国名表）；**不是**港口 `culture_id` | `-1` | 无存档对象 |
| `eCulture`（21 文化圈） | 21（容器计数 `0x15`、静态表 21 条） | `-1`；港口字节空值 `0x16`（只进 legacy_codec） | 名字见 06 |
| NPC 脸 | 9（文案） | 无 | 人物 `family`；不是这 21 槽 |
| `eTownFashion` | 头文件 26 = 投票误采；循环上界 **2** | `-1` | `[推断]` 槽下标；槽值是 `trading_item`，空 = `null`（原版 106） |
| `eFashionSpeed` | `[未知]` | `-1` | |

`CCulture` 运行时布局 `[确认，vfunc_05 汇编]`：

| 偏移 | 宽 | 来自静态表 | 用途 |
| --- | --- | --- | --- |
| +0x00 | ptr | — | vtable / `CSaveObject` |
| +0x08 | 8 | `+0x00` | 名字 `char*` |
| +0x10 | 1 | `+0x08` | `major_sea_id`（`fn_14000aa0b` 读） |
| +0x11 | 2×1 | `+0x09` | 流行推力 上推 / 下拉（`fn_1400077bb`，economy 02 §2.3）；值 0–7；无文化圈时读 7 |
| +0x14 | 105×2 | `+0x0C` | 每交易品一个 uint16 卖价底，存盘 |
| … | 至 0xE8 | | stride 尾（静态记录尾 2 字节恒 0） |

105 个 word：`[确认]` 文化圈卖价底（`fn_140010a23` 读；胡椒欧洲 7500–8850、印尼 1300）。`+0x11/+0x12`：`[确认]` 月初流行推力（economy 02 §2.3）。两者是同一行上的**数值列**，表 `config/economy/culture_prices.yaml` 归 economy，按 `culture` key 关联；本域只拥有行 id、名字与 `major_sea` 外键（06 `cultures.yaml`）。

### 9 圈与 21 文化圈的粗映射 `[推断]`

仅供设施选 NPC 形象。未在 `CTown` 字段名上证实。欧洲殖民港（culture 19 的总督等）人物表仍标「欧洲」。

| 脸 key | 文案 | culture_id（推断） |
| --- | --- | --- |
| europe | 欧洲 | 0–8，另加部分 19 |
| africa | 非洲 | 9–11 |
| arabia | 阿拉伯 | 12 |
| india | 印度 | 13 |
| se_asia | 东南亚 | 14–15 |
| china | 中国 | 16 |
| korea | 韩国 | 17 |
| japan | 日本 | 18 |
| mesoamerica | 中南美 | 19–20 |

广场店主 person_id：98 欧洲男子、99 欧洲女子、100 非洲 … 107 中南美。

## 4. 算法 / 公式

本篇几乎无数值公式。运行时规则：

```
# 每港当前流行
town.fashion_slots: [ { trading_item | null, level } ] × fashion_slots_per_town
fashion_slots_per_town = 2          # 原版循环上界；进 caps
# 空 = null。原版 FixTown 初值 0x6A（ITEM_EMPTY）只在 legacy_codec 出现 [确认]
# 可流行的商品集合 [未知]；在钉死前 = 全部交易品

# 开新流行（原版 fn_1402767f0 的结构，抽选函数未还原）
candidates_no_fashion = towns where fashion_slots 皆空 且 港口有效
candidates_with_share = towns where 任一合约份额 > 0
pick = random(candidates_no_fashion) or fallback random(candidates_with_share)
# 另有 rax != 3 则放弃；3 的语义 [未知]
```

`level` 如何随日 / 随卖出前进、一格流行何时被替换：`[未知]`，只把 `eFashionSpeed` 当枚举旋钮。`level` 的 120 分界属 economy 的市价推力参数（`formulas/economy.yaml` `fashion_push.level_cut`），不在本表重复。阿科布 / 品牌发放是叙事命令 `Fashion.seed(town_id, trading_item_id)`，不在本公式里写死人物。

文化 105 word 如何进市价：不在本篇写。Economy 读自己的 `Economy.culture_price(culture_id, item_id)`；本域只提供行与外键。

## 5. 耦合分析

原版：

- `culture_id` 塞在港口 packed 表 `+0x25`，代码直接拿它当 `CCultureElement` 下标；NPC 脸是否也由它派生未证
- `CCulture` 直接按 `eTradingItem` 105 循环，文化内容与交易品表长绑死；初值是 exe 常量，World DLL 改不到
- 流行 2 格写在 `CTown` 上，交易所、广场、月结扫港函数都直接读偏移
- 广场情报是 mesfile 字符串，逻辑和文案搅在一起
- 无 World DLL 文化/流行表，开局填充路径在 exe

重构后：

- `world.towns.yaml` 只留 `culture` 外键（原版 `+0x25`，已确认）；国家若要挂港口，另开 `country` 列，原版包留空
- `factions/countries.yaml`、`cultures.yaml`（06 定义）、`fashion.yaml` 各一张；文化圈数值列在 `economy/culture_prices.yaml`
- 文化圈 × 交易品不再是 21×105 下标矩阵：economy 表按 `trading_item` key 稀疏映射 + 默认 + 加载期覆盖率校验（D1）
- 人物职能 NPC 持 `npc_face_id`，由城镇查询「此港用哪张脸」再选 person 模板（不要叫 `culture_circle_id`，那个名字已废）
- 城镇设施、交易所只问 `Fashion.current(town_id) -> [{ trading_item, level }]`

## 6. 解耦后的配置

```yaml
# config/factions/countries.yaml
schema: dk4.factions.countries
version: 1
# [推断] eCountry 候选：.data 0x140682780 的 19 个国名（fn_14000ddeb）。
# 原版未见国家 ↔ 港口 / 文化圈的对照，故不给 capital_town，也不与 culture 同号。
# vanilla_index = 国名指针表下标（候选 eCountry）。
countries:
  - { key: vanilla:england, vanilla_index: 0, name: 英吉利 }
  - { key: vanilla:france, vanilla_index: 1, name: 法兰西 }
  - { key: vanilla:netherlands, vanilla_index: 2, name: 荷兰 }
  - { key: vanilla:spain, vanilla_index: 3, name: 西班牙 }
  - { key: vanilla:portugal, vanilla_index: 4, name: 葡萄牙 }
  - { key: vanilla:sweden, vanilla_index: 5, name: 瑞典 }
  - { key: vanilla:venice, vanilla_index: 6, name: 威尼斯 }
  - { key: vanilla:genoa, vanilla_index: 7, name: 热那亚 }
  - { key: vanilla:germany, vanilla_index: 8, name: 德意志 }
  - { key: vanilla:russia, vanilla_index: 9, name: 俄国 }
  - { key: vanilla:denmark, vanilla_index: 10, name: 丹麦 }
  - { key: vanilla:poland, vanilla_index: 11, name: 波兰 }
  - { key: vanilla:ottoman, vanilla_index: 12, name: 奥斯曼帝国 }
  - { key: vanilla:arabia, vanilla_index: 13, name: 阿拉伯 }
  - { key: vanilla:india, vanilla_index: 14, name: 印度 }
  - { key: vanilla:ming, vanilla_index: 15, name: 明朝 }
  - { key: vanilla:japan, vanilla_index: 16, name: 日本 }
  - { key: vanilla:aztec, vanilla_index: 17, name: 阿兹特克 }
  - { key: vanilla:inca, vanilla_index: 18, name: 印加 }
```

上一版把 21 档 region 当国家、起了「尼德兰 / 汉萨 / 新西班牙」等别名：作废。21 档是文化圈，名字、`major_sea_id`、首都见 06 的 `cultures.yaml`。

```yaml
# config/characters/npc_faces.yaml   （9 张 NPC 脸；与 21 文化圈不同轴）
schema: dk4.characters.npc_faces
version: 1
npc_faces:                    # 原版无枚举，只有 9 个 family 文案；无 vanilla_index
  - { key: vanilla:europe, name: 欧洲 }
  - { key: vanilla:africa, name: 非洲 }
  - { key: vanilla:arabia, name: 阿拉伯 }
  - { key: vanilla:india, name: 印度 }
  - { key: vanilla:se_asia, name: 东南亚 }
  - { key: vanilla:china, name: 中国 }
  - { key: vanilla:korea, name: 韩国 }
  - { key: vanilla:japan, name: 日本 }
  - { key: vanilla:mesoamerica, name: 中南美 }

# culture → npc_face [推断]，见 §3 粗映射；原版无此表，内容包显式给。按 key，不按下标
culture_faces:
  vanilla:europe:      [vanilla:england, vanilla:flanders, vanilla:germany, vanilla:norse, vanilla:portugal, vanilla:spain, vanilla:italy, vanilla:greece, vanilla:turkey]
  vanilla:africa:      [vanilla:egypt, vanilla:west_africa, vanilla:east_africa]
  vanilla:arabia:      [vanilla:arabia]
  vanilla:india:       [vanilla:india]
  vanilla:se_asia:     [vanilla:indochina, vanilla:indonesia]
  vanilla:china:       [vanilla:china]
  vanilla:korea:       [vanilla:korea]
  vanilla:japan:       [vanilla:japan]
  vanilla:mesoamerica: [vanilla:caribbean, vanilla:mexico]
# 加载期校验：每个 culture 恰好出现一次；缺的走 default_face
default_face: vanilla:europe

plaza_shopkeepers:            # person 外键；原版包由导入器写 vanilla_index
  - { person: { vanilla_index: 98 },  npc_face: vanilla:europe, variant: male }
  - { person: { vanilla_index: 99 },  npc_face: vanilla:europe, variant: female }
  - { person: { vanilla_index: 100 }, npc_face: vanilla:africa }
  - { person: { vanilla_index: 101 }, npc_face: vanilla:arabia }
  - { person: { vanilla_index: 102 }, npc_face: vanilla:india }
  - { person: { vanilla_index: 103 }, npc_face: vanilla:se_asia }
  - { person: { vanilla_index: 104 }, npc_face: vanilla:china }
  - { person: { vanilla_index: 105 }, npc_face: vanilla:korea }
  - { person: { vanilla_index: 106 }, npc_face: vanilla:japan }
  - { person: { vanilla_index: 107 }, npc_face: vanilla:mesoamerica }
```

```yaml
# config/factions/fashion.yaml
schema: dk4.factions.fashion
version: 1

slots_per_town: 2            # 原版循环上界 [确认]；空槽 = null（原版 0x6A 只在 legacy_codec）
# NUM_FASHIONSPEED 未还原；文案至少三态。与槽 level 的对应 [未知]
speeds:
  - { key: vanilla:starting, name_i18n: fashion.speed.starting }  # 将流行 [推断]
  - { key: vanilla:ongoing,  name_i18n: fashion.speed.ongoing }
  - { key: vanilla:ending,   name_i18n: fashion.speed.ending }

# 可流行商品集合。原版槽直接存 trading_item；不存在 26 条子集
eligible_items: all          # 或显式 [trading_item key, ...]

# 状态不进本表。存档 town 节：每港 fashion[] = { trading_item | null, level }
# CTown+0x74/+0x75 是港口状态，不属本表
```

```yaml
# config/ids/legacy_codec.yaml 片段（只给原版导入导出）
enums:
  culture: { vanilla_count: 21, invalid: -1, empty: 22 }    # 港口字节 0x16
fields:
  - { record: world.town, offset: 0x25, enum: culture, width: u8 }
  - { record: world.town, offset: 0x7E, stride: 2, count: 2, layout: [{ name: trading_item, enum: trading_item, width: u8 }, { name: level, width: u8 }], copy: word }  # 两字节：u8 trading_item + u8 等级 [确认]；原版按 word 拷贝；item 空 106
# 计数（文化圈 21、海域 7、脸 9、国名 19）= 各表长，不进 caps
```

文案（招牌、三句生命周期、阿科布）进 `config/ui/strings.zh-CN.yaml`，key 如 `plaza.fashion_intel`，不进逻辑表。

## 7. 独立旋钮

- 国家条数、国家 ↔ 港口是否挂钩（原版包不挂）
- 文化圈条数与 `major_sea` 外键（数值列在 economy `culture_prices.yaml`，按交易品 key 存）
- 9 张脸增减、`culture_faces` 映射（只影响 NPC 换装表）
- 可流行商品集合、每港槽数
- 速度枚举
- 广场情报是否要「先选招牌」（场景流，`ui/scenes.yaml`）

改「卖流行品加多少份额」走 03 / economy，不改 `fashion.yaml` schema。

## 8. 对外接口

查询：

```
Country.ofTown(town_id) -> country_id | null      # 原版包恒 null
Culture.of_town(town_id) -> culture_id | null     # 读 world/towns.yaml 的 culture 列
Culture.majorSea(culture_id) -> major_sea_id     # 06
Culture.npcFace(culture_id) -> npc_face_id
Fashion.current(town_id) -> [{ trading_item | null, level }]  # 长度 = slots；外部一律调此名
Fashion.isFashioned(town_id, trading_item_id) -> bool
Fashion.plazaShopkeeper(town_id) -> person_id
```

命令：

```
Fashion.set_slot(town_id, slot, trading_item_id, level)
Fashion.clear(town_id, slot)
Fashion.onSold(town_id, slot, qty)   # 原版 fn_14000dc6a；level 推进式 [未知]，先 no-op + 事件
```

文化圈卖价不在本域查：用 economy 的 `Economy.culture_price(culture_id, item_id)`；`CulturePriceChanged` 由 economy 发。

事件：

| 事件 | 载荷 | 谁关心 |
| --- | --- | --- |
| `FashionChanged` | `{ town_id, slot, old_item, new_item, level }` | economy、town-life、ui |
| `FashionEnded` | `{ town_id, trading_item_id }` | economy |

广场：玩家点招牌 → UI 调 `Fashion.current` + 字符串 `plaza.fashion_intel`。不在本系统里画按钮。

不暴露 `CCulture*`、不暴露 0xE8 布局。

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| `NUM_COUNTRY` | 只有符号；19 国名表是候选 | `= 21`；`= 19` 作为确认 |
| 国名表 `fn_14000ddeb` 调用者 | 直接调用扫描为 0 | 「港口所属国」 |
| `NUM_CULTURE = 21` | 容器 `0x15`、静态表 21 条 | 头文件已恢复 |
| `CCulture +0x11/+0x12` | 开局拷贝、不存盘，值 0–7；economy 已钉为流行推力 | 本域字段 |
| `CCulture +0x14` 105 word | 卖价底已确认；写价路径见 economy | 「能否流行」 |
| 9 脸 ↔ culture 表 | 地理常识 | 确认映射 |
| `NUM_KIND = 9` | 符号 | 就是文化圈 |
| `NUM_FASHIONSPEED` | 符号；文案 3 句 | `= 3` |
| 是否存在独立 `fashion_id` 空间 | 槽初值是交易品空 106 | 26 条流行目录已还原 |
| `CTown +0x70+2i` / `+0x71+2i` | item（初值 106）/ level；level 推进式未还原 | `level` = `eFashionSpeed` |
| `CTown +0x74/+0x75` | 港口状态 0–12 / 状态 0 附属旗（economy 02） | 流行字段 |
| `NUM_TOWNFASHION = 26` | RTC 投票误采；循环都是 2 | 26 条流行目录 |
| `fn_1402767f0` 里 `rax != 3` | 放弃开流行 | 「必须 3 个份额槽满」 |
| 港口字节空值 `0x16` | `fn_140013a02`；与 98/106/232/236 同为「计数+1」惯例（观察） | `NUM_CULTURE = 22`；运行时空值 |

`CCulture` 0xE8 里除 `+0x08/+0x10/+0x14` 外的残余，一律 `unknown_bytes`。
