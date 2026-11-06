# 国家、文化、流行

港口上的 `region` 字节、21 个 `CCulture` 槽、9 个 NPC 文化圈文案、以及每港 2 格流行，都挂在势力域的**内容表**里。设施只消费这些 ID，不拥有表。

## 1. 职责边界

**管：**

- `eCountry` / `eCulture` / `eTownFashion` / `eFashionSpeed` 的 ID 空间
- 21 档港口 `region`（0–20）与首都对照
- 文化作为可存档块：`CCulture` ×21，每块带 105 个交易品 word
- 9 个 NPC 文化圈（人物 `family` 文案）到设施形象的映射
- 流行目录（26）与每港当前 2 格流行的**定义**；速度枚举的 ID

**不管：**

- 港口坐标、设施开没开 → `world` / `town-life`
- 流行品卖出如何改份额、如何改市价 → `economy`（本系统只发 `FashionChanged`，份额见 03）
- 阿科布流浪、发放品牌商品的事件脚本 → `narrative` / `town-life`
- `CFashionSprite` 等绘制 → `ui`

流行状态存在港口对象上（原版 `CTown +0x70` 两格），但**目录和规则**属于本域，避免城镇设施文档夹一份商品 ID 表。

## 2. 原版事实

### 国家

- `[确认]` `eCountry` 存在，`INVALID_COUNTRY=-1`，`NUM_COUNTRY` 整数未还原。来源：`CountryID.h`、`asserts.txt`。
- `[确认]` 工程里**没有** `CCountry` / `CCountryElement`。国家不是 World DLL 表。来源：`class_hierarchy.txt`、`DllMain.cpp` 导出只有 Town/Person/Item/Force/Fleet。
- `[确认]` 港口表 `+0x24` 为首都标记（1/0），`+0x25` 为 `region` 字节，取值 0–20 共 21 档。来源：`Docs/00-overview/04-id-registry.md`、`Docs/_data/towns_world0.json`。
- `[推断]` `region` 即 `eCountry`。依据：只有这一档 21 值的地理字段；无独立国家表。未在 `CTown` 反编译里看到 `eCountry` 局部名，故不升格。

21 档首都（`is_capital=1` 且 `region=N` 的港口）：

| region | 首都 | 同档其它港（World0） |
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

国家**显示名**原版未从枚举还原。UI 港口情报有 `posKind` / `rcKindTitle`（`fn_140005f51`），文案 key 另表。

### 文化

- `[确认]` `eCulture` 存在，`INVALID_CULTURE=-1`，`NUM_CULTURE` 整数未还原。来源：`CultureID.h`。
- `[确认]` `CCulture : CSaveObject`。`CCultureElement` 构造 **21** 个（`0x15`）对象，stride **0xE8**。析构 / 存盘同样循环 `>= 0x15`。来源：`CCultureElement.cpp`。
- `[确认]` `CCulture` 存盘遍历 `eTradingItem` 0–0x68（105）：每个 word 在 `+0x14 + i*2`。另有 1 字节 `+0x10`、循环 2 次写 `+0x11`。来源：`CCulture.cpp` `vfunc_05/06/07`。
- `[确认]` 没有 `GetFixCultureProc`。文化块不是 World DLL 表，开局由 `vfunc_05` 从别的对象填。来源：`DllMain.cpp`、`CCulture::vfunc_05`。
- `[推断]` `NUM_CULTURE = 21`，下标与 `town.region` 对齐。依据：容器长度恰好 21，且港口情报 UI 有 `posCulture`。
- `[确认]` 人物表从 id 98 起，`family` 字段出现 9 个文化圈文案：**欧洲、非洲、阿拉伯、印度、东南亚、中国、韩国、日本、中南美**。广场店主把欧洲拆成「欧洲男子 / 欧洲女子」，故店主 10 人、圈仍是 9。来源：`Docs/_data/persons.tsv`、`PersonID` 记录。
- `[确认]` 同 9 词还用在酒馆掌柜/客人、交易所掌柜、造船老板、码头、卫兵、国王/藩主/县令/大名/总督、宗教人士、旅馆老板娘、行会总管、属下等职能 NPC 上。来源：同上。
- `[推断]` 9 圈是设施 NPC 换装 / 对白的粗分组，**不是** `eCulture` 的计数。`NUM_KIND=9`（`num_values.json`）可能相关，未证明。

### 流行

- `[确认]` `eTownFashion`，`NUM_TOWNFASHION = 26`，`INVALID_TOWNFASHION=-1`。来源：`TownFashionID.h`、`num_values.json`。
- `[确认]` `NUM_FASHIONSPEED` 只有符号。来源：`FashionspeedID.h`、`Fashion.h` 注释。
- `[确认]` 玩法循环几乎都是 `eTownFashion >= 2` 就结束：每港**当前流行 2 格**，不是 26 格全亮。来源：`CPriceInformationView.cpp`、`fn_14025ae30.cpp`、`fn_1402645a0.cpp`、`fn_140260c10.cpp` RTC。
- `[确认]` `CTown` 存盘在 `+0x70 / +0x71` 各写一字节两次，随后 `+0x74 / +0x75` 各一字节。与「2 格流行 + 2 个附属字节」同形。来源：`CTown.cpp` `vfunc_06/07`。
- `[确认]` 交易所卖出当前流行品会计入 `anFashionSellAmount`，再改占有率或发介绍信 / 推荐信。来源：`srcdata/docs/trade_stack.md`（`CommitDeal 0x140277090`）。份额公式在 03，本篇只承认「流行品是份额的输入」。
- `[确认]` `fn_1402767f0` 扫 97 港，建两张 388 字节候选表（`97×4`）：`eTownShareTable`、`eTownNotFashionTable`。无流行的港可被抽中开新流行，并 `fn_14000fb14(0xfd, …)`。来源：`fn_1402755b0.cpp`。
- `[确认]` 广场文案：选零售店招牌听流行情报；「据说在 %s 将流行 %s」「听说 %s 正流行 %s」「这城市好像正在流行 %s」。来源：`mesfile_sc.txt` `0x1143`、`0x8653`、`0x86fb`、`0x8727`。
- `[确认]` 流行生命周期文案三句：「今后 %s 的某一个物品一定会流行起来」「好像还要继续流行下去」「%s 的流行也该结束了吧」。来源：`0x6ebb`、`0x6ef7`、`0x6f0f`。
- `[确认]` 阿科布要把「新的流行趋势带到世界的所有城市」；发放品牌商品 `%s` 做宣传。来源：`0x6fa3`、`0x728b`。
- `[确认]` 唯一流行类 RTTI 是 `CFashionSprite`（绘制），没有 `CFashion` 存档对象。流行状态住在港口上。来源：`class_hierarchy.txt`、`Fashion.h`。
- `[推断]` 26 个 `fashion_id` 与 105 个 `trading_item_id` 是不同空间：交易品空格是 `0x6A=106`，流行空格更像 `0x1A=26`（`NUM_TOWNFASHION`）。对照表未还原。
- `[推断]` `NUM_FASHIONSPEED ≥ 3`，对应「将流行 / 继续 / 结束」三句。整数未知。

## 3. 实体与 ID

| 枚举 | 原版计数 | 空 / 非法 | 备注 |
| --- | --- | --- | --- |
| `eCountry` | `[未知]`；港口 region 21 档 | `-1` | 无存档对象 |
| `eCulture` | `[推断]` 21 | `-1` | `CCultureElement` 循环 `0x15` |
| NPC 文化圈 | 9（文案） | 无 | 人物 `family` 字符串，不是枚举文件 |
| `eTownFashion` | 26 | `-1`；运行时或用 `26` | 目录；每港只用 2 槽 |
| `eFashionSpeed` | `[未知]` | `-1` | |

`CCulture` 运行时布局 `[推断，由存盘循环反推]`：

| 偏移 | 宽 | 用途 |
| --- | --- | --- |
| +0x00 | ptr | vtable / `CSaveObject` |
| +0x08 | 4? | 从源对象拷来 |
| +0x10 | 1 | 未命名 |
| +0x11 | 2×1 | 循环 2 |
| +0x14 | 105×2 | 每交易品一个 uint16 |
| … | 至 0xE8 | stride 尾 |

105 个 word 的语义（需求、加成、能否流行……）`[未知]`，实现时当不透明数组进内容包，解释权留给 economy 的查询接口。

### 9 圈与 21 region 的粗映射 `[推断]`

仅供设施选 NPC 形象。未在 `CTown` 字段名上证实。欧洲殖民港（region 19 的总督等）人物表仍标「欧洲」。

| 圈 key | 文案 | region（推断） |
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
town.fashion_slots: [fashion_id | empty] × fashion_slots_per_town
fashion_slots_per_town = 2          # 原版循环上界；进 caps
fashion_catalog_size   = 26         # 原版 NUM_TOWNFASHION；运行时读表长
fashion_empty_id       = 26         # [推断] 与 0x1A 哨兵一致

# 开新流行（原版 fn_1402767f0 的结构，抽选函数未还原）
candidates_no_fashion = towns where fashion_slots 皆空 且 港口有效
candidates_with_share = towns where 任一合约份额 > 0
pick = random(candidates_no_fashion) or fallback random(candidates_with_share)
# 另有 rax != 3 则放弃；3 的语义 [未知]
```

速度如何随日前进、一格流行何时被替换：`[未知]`，只把 `eFashionSpeed` 当枚举旋钮。阿科布 / 品牌发放是叙事命令 `Fashion.seed(town_id, fashion_id)`，不在本公式里写死人物。

文化 105 word 如何进市价：不在本篇写。Economy 通过 `Culture.itemWeight(culture_id, trading_item_id) -> int` 查询。

## 5. 耦合分析

原版：

- `region` 塞在港口 packed 表 `+0x25`，国家 / 文化 / NPC 圈三套概念共用同一字节的嫌疑很大
- `CCulture` 直接按 `eTradingItem` 105 循环，文化内容与交易品表长绑死
- 流行 2 格写在 `CTown` 上，交易所、广场、月结扫港函数都直接读偏移
- 广场情报是 mesfile 字符串，逻辑和文案搅在一起
- 无 World DLL 文化/流行表，开局填充路径在 exe

重构后：

- `world.towns.yaml` 只留 `region_id`（或改名 `country_id`，但要等确认）
- `factions/countries.yaml`、`cultures.yaml`、`fashion.yaml` 各一张
- 人物职能 NPC 持 `culture_circle_id`，由城镇查询「此港用哪一圈」再选 person 模板
- 城镇设施、交易所只问 `Fashion.current(town_id) -> [item_id, item_id]`

## 6. 解耦后的配置

```yaml
# config/factions/countries.yaml
schema: dk4.factions.countries
version: 1
# NUM_COUNTRY 未还原。原版包按 21 档 region 实例化，id 与 town.region 相同 [推断]
countries:
  - { id: 0,  key: england,           name: 英格兰,     capital_town_id: 0 }   # 伦敦
  - { id: 1,  key: netherlands,       name: 尼德兰,     capital_town_id: 2 }
  - { id: 2,  key: hansa,             name: 汉萨,       capital_town_id: 5 }
  - { id: 3,  key: sweden,            name: 瑞典,       capital_town_id: 7 }
  - { id: 4,  key: portugal,          name: 葡萄牙,     capital_town_id: 11 }
  - { id: 5,  key: spain,             name: 西班牙,     capital_town_id: 13 }
  - { id: 6,  key: italy,             name: 意大利,     capital_town_id: 15 }
  - { id: 7,  key: greece,            name: 希腊,       capital_town_id: 19 }
  - { id: 8,  key: ottoman,           name: 奥斯曼,     capital_town_id: 22 }
  - { id: 9,  key: egypt,             name: 埃及,       capital_town_id: 25 }
  - { id: 10, key: west_africa,       name: 西非,       capital_town_id: 29 }
  - { id: 11, key: east_africa,       name: 东非,       capital_town_id: 34 }
  - { id: 12, key: arabia,            name: 阿拉伯,     capital_town_id: 38 }
  - { id: 13, key: india,             name: 印度,       capital_town_id: 42 }
  - { id: 14, key: malacca_region,    name: 马六甲一带, capital_town_id: 47 }
  - { id: 15, key: indonesia,         name: 爪哇一带,   capital_town_id: 50 }
  - { id: 16, key: china,             name: 中国,       capital_town_id: 54 }
  - { id: 17, key: korea,             name: 朝鲜,       capital_town_id: 57 }
  - { id: 18, key: japan,             name: 日本,       capital_town_id: 59 }
  - { id: 19, key: caribbean,         name: 加勒比,     capital_town_id: 61 }
  - { id: 20, key: new_spain,         name: 新西班牙,   capital_town_id: 65 }
```

上表 `name` / `key` 是给原版包起的可读别名，**不是** mesfile 还原的官方国名。校验只要求 `capital_town_id` 指向 `is_capital` 港且 `town.region == id`。

```yaml
# config/factions/cultures.yaml
schema: dk4.factions.cultures
version: 1

# 粗圈：设施 NPC
circles:
  - { id: 0, key: europe,       name: 欧洲 }
  - { id: 1, key: africa,       name: 非洲 }
  - { id: 2, key: arabia,       name: 阿拉伯 }
  - { id: 3, key: india,        name: 印度 }
  - { id: 4, key: se_asia,      name: 东南亚 }
  - { id: 5, key: china,        name: 中国 }
  - { id: 6, key: korea,        name: 韩国 }
  - { id: 7, key: japan,        name: 日本 }
  - { id: 8, key: mesoamerica,  name: 中南美 }

# eCulture 实例。原版 21 槽 [推断] 与 country/region 同 id
cultures:
  - id: 0
    key: england
    country_id: 0
    circle_id: 0
    item_words: []    # 105 个 uint16；生成管线从存档/运行时快照抽出，语义 [未知]
    unknown_b10: 0
    unknown_b11: [0, 0]

plaza_shopkeepers:
  - { person_id: 98, circle_id: 0, variant: male }
  - { person_id: 99, circle_id: 0, variant: female }
  - { person_id: 100, circle_id: 1 }
  - { person_id: 101, circle_id: 2 }
  - { person_id: 102, circle_id: 3 }
  - { person_id: 103, circle_id: 4 }
  - { person_id: 104, circle_id: 5 }
  - { person_id: 105, circle_id: 6 }
  - { person_id: 106, circle_id: 7 }
  - { person_id: 107, circle_id: 8 }
```

```yaml
# config/factions/fashion.yaml
schema: dk4.factions.fashion
version: 1

slots_per_town: 2
empty_id: 26                 # [推断]
# NUM_FASHIONSPEED 未还原；文案至少三态
speeds:
  - { id: 0, key: starting, name_i18n: fashion.speed.starting }  # 将流行 [推断]
  - { id: 1, key: ongoing,  name_i18n: fashion.speed.ongoing }
  - { id: 2, key: ending,   name_i18n: fashion.speed.ending }

# 26 条目录。原版未还原名字与 trading_item 对照
items:
  - { id: 0,  key: fashion_00, trading_item_id: null }  # 对照 [未知]
  - { id: 1,  key: fashion_01, trading_item_id: null }
  # … 2-25 由管线列 id，禁止手编不存在的商品

# 状态不进本表。存档：每港 fashion_slots[2]、fashion_speed[2]
```

```yaml
# config/caps.yaml 片段
fashion_slots_per_town: 2
fashion_catalog_vanilla: 26
culture_count_vanilla: 21
culture_circle_count: 9
country_count_vanilla: 21
```

文案（招牌、三句生命周期、阿科布）进 `config/ui/strings.zh-CN.yaml`，key 如 `plaza.fashion_intel`，不进逻辑表。

## 7. 独立旋钮

- 国家条数、首都、region 与 country 是否 1:1
- 文化条数、每文化 105 word 换成「表长 = 交易品表长」
- 9 圈增减（只影响 NPC 换装表）
- 流行目录长度、每港槽数、空哨兵
- 速度枚举
- 广场情报是否要「先选招牌」（场景流，`ui/scenes.yaml`）

改「卖流行品加多少份额」走 03 / economy，不改 `fashion.yaml` schema。

## 8. 对外接口

查询：

```
Country.ofTown(town_id) -> country_id | invalid
Country.capital(country_id) -> town_id
Culture.ofTown(town_id) -> culture_id | invalid
Culture.circle(culture_id) -> circle_id
Culture.itemWord(culture_id, trading_item_id) -> u16
Fashion.current(town_id) -> [{ fashion_id, trading_item_id, speed_id }]  # 长度 = slots
Fashion.isFashioned(town_id, trading_item_id) -> bool
Fashion.plazaShopkeeper(town_id) -> person_id
```

命令：

```
Fashion.setSlot(town_id, slot, fashion_id, speed_id)
Fashion.clear(town_id, slot)
Fashion.tickDay()     # 内部按 speed 表推进；公式未还原前保持 no-op + 事件钩子
```

事件：

| 事件 | 载荷 | 谁关心 |
| --- | --- | --- |
| `FashionChanged` | `{ town_id, slot, old_id, new_id, speed_id }` | economy、town-life、ui |
| `FashionEnded` | `{ town_id, fashion_id }` | economy |
| `CultureItemWordChanged` | `{ culture_id, trading_item_id, value }` | economy |

广场：玩家点招牌 → UI 调 `Fashion.current` + 字符串 `plaza.fashion_intel`。不在本系统里画按钮。

不暴露 `CCulture*`、不暴露 0xE8 布局。

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| `NUM_COUNTRY` | 只有符号 | `= 21` 作为确认 |
| `region == eCountry` | 21 档对 21 首都 | 确认的字段名 `country` |
| `NUM_CULTURE = 21` | 容器 `0x15` | 头文件已恢复 |
| `culture_id == region` | 同数 | 确认双射 |
| `CCulture +0x08/+0x10/+0x11` | 存盘有 | 任何玩法名 |
| `CCulture +0x14` 105 word | 与交易品同数 | 「物价倍率」或「能否流行」 |
| 9 圈 ↔ region 表 | 地理常识 | 确认映射 |
| `NUM_KIND = 9` | 符号 | 就是文化圈 |
| `NUM_FASHIONSPEED` | 符号；文案 3 句 | `= 3` |
| `fashion_id → trading_item_id` | 无表 | 1:1 或「fashion_id 就是商品 id」 |
| `CTown +0x70` 两字节 | 与 2 槽同形 | 确认字段名 `fashion` |
| `CTown +0x74/+0x75` | 紧跟两槽 | 确认是 `FashionSpeed` |
| `fn_1402767f0` 里 `rax != 3` | 放弃开流行 | 「必须 3 个份额槽满」 |
| 国家中文名 | 本文 YAML 别名 | mesfile 官方译名 |

`CCulture` 0xE8 里除 `+0x14` 起 210 字节外的残余，一律 `unknown_bytes`。
