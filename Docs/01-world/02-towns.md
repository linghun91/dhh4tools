# 港口

港口是世界图上的节点。world 只存**定义**：ID、名字、原版坐标、是否首都、region、特产品槽的交易品外键。市价、供给、发展度、份额、驻军、设施玩法全部用同一个 `town_id` 挂到别的系统。

## 1. 职责边界

管：

- `eTown` 97 条定义（World0 / World1 各一份内容包）
- 名字、`x1,y1,x2,y2`、`is_capital`、`region`
- 每港 5 个特产品**槽位**（`slot_index` + `trading_item_id`，空槽哨兵 106）
- 与势力/经济的 ID 关联（不存关系数值）

不管：

- 特产品价格、供给、品级公式（`config/economy/special_products.yaml`）
- 市场价、投资、发展度（economy；上限 `town_develop` 在 `caps.yaml`）
- 份额 `eTownShare`（factions / economy，原版与港口 1:1）
- 流行 `eTownFashion`（factions）
- 设施里具体干什么（`config/town/facilities.yaml`：酒馆、造船、公会、宫殿、广场…）
- 入港场景 `CTownScene`（ui / town-life）
- 航线是否已开通（routes）
- 舰队停靠、补给消耗（ocean / fleets）

设施**标志位**在港口 DLL 记录里没有恢复出字段名。`CBar` `CInn` `CGuild` `CPalace` `CPort` `CShipyard` `CPlazaView` 是城镇活动类，不在本表。

## 2. 原版事实

- `[确认]` `enum eTown`：0 伦敦 … 96 卡拉卡斯，`NUM_TOWN = 97`，`INVALID_TOWN = -1`。来源：`srcdata/source/TownID.h`。
- `[确认]` `GetFixTownProc` 断言 `0 <= eTown && eTown < NUM_TOWN`，返回 `&s_aTown[id]`。World0 表 VA `0x1800e7b30`。来源：`srcdata/source/world/DllMain.cpp`。
- `[确认]` 记录步长 **0x84 = 132**。结构：`name[20]` GBK + `rest[0x70]`。来源：`DllMain.cpp` `STown`，`srcdata/include/world.h`，`srcdata/source/world/World0_town.c`。
- `[确认]` 抽出字段见 `Docs/_data/towns_world0.json` / `towns.tsv`：

| 偏移 | 类型 | 字段 | 伦敦 |
| --- | --- | --- | --- |
| +0x00 | char[20] | GBK 名 | 伦敦 |
| +0x14 | uint32 LE | x1 | 20080 = `0x4e70` |
| +0x18 | uint32 LE | y1 | 4272 = `0x10b0` |
| +0x1c | uint32 LE | x2 | 20136 |
| +0x20 | uint32 LE | y2 | 4280 |
| +0x24 | uint8 | is_capital | 1 |
| +0x25 | uint8 | region | 0 |
| +0x26 | uint8 | JSON `b38` | 0 |
| +0x27 | uint8 | JSON `b39` | 0（全表恒 0） |
| +0x28 | 5×(u16,u16,u16) | 特产品槽 | 见下 |
| +0x46 | 62 字节 | 未命名 | `rest_hex` 124 个 hex 字符 |

- `[确认]` 特产品槽：`item_id`、`supply_or_price`、`grade`；空槽 `item_id = 0x6A = 106`（`ITEM_EMPTY`，不是 -1，也不是 0）。来源：`towns_world0.json`，`srcdata/source/Constants.h`，`SpecialProductID.h` 每港 5 槽，`num_values.json` `NUM_SPECIALPRODUCT = 5`。
- `[确认]` 这 5 槽的价/量/品级属于经济。本系统只保留 `trading_item_id` 外键。
- `[确认]` `CTown` 基类 `CSaveObject -> CObject -> CLocator`。来源：`class_hierarchy.txt`。`CTown::vfunc_05` RTC `r_sFixTown` 132 字节，从 `GetFixTownProc` `rep movsb` 拷进运行时对象。来源：`srcdata/source/game/town/CTown.cpp`。
- `[确认]` `CTownElement` 循环 `eTown >= 0x61`（97）通知每个港口对象。来源：`srcdata/source/game/world/CTownElement.cpp`。`CWorldData::vfunc_05` 同样 `GetFixTownProc` 循环 97。来源：`CWorldData.cpp`。
- `[确认]` `CTown` 存档/读档（`vfunc_07` / `vfunc_06`）按偏移扫：`+0x10,+0x14`；2 次 `+0x1e/+0x24`；3 次 `+0x18/+0x19`；5 次特产品 `+0x2c…+0x31`；**19** 次 `+0x4a`；2 次 `+0x70/+0x71`；再 `+0x74,+0x75`。来源：`CTown.cpp`，`catalog/fields.tsv` 行 `CTown`。19 与 `NUM_TRADINGITEMKIND = 19` 同数，`[推断]` 是按交易种类挂的经济状态，不是设施标志。
- `[确认]` 21 个 `is_capital==1` 的港，region 0–20 各一。名单与 `Docs/00-overview/04-id-registry.md` 一致。
- `[推断]` `region` 字节是否等于 `eCountry`：值域 0–20 共 21 档，`CountryID.h` 的 `NUM_COUNTRY` 整数未恢复。依据：值域与首都一一对应。未在 `CTown` 字段名确认。
- `[确认]` `eTownFashion = 26`。`b38` 取值 0,2–24,26，从不见 1 和 25；26 = `NUM_TOWNFASHION`。`[推断]` 可能是流行 ID，26 当空哨兵。不得写成确认。
- `[确认]` World1 港口名与 World0 前部对照相同（至少 0–9 同名）。来源：`Docs/_data/world1_names.json`。同一 schema，两份包。
- `[确认]` 坐标不是 `World.dat` 下标。见 `01-map-and-sea-areas.md`。`x1/x2`、`y1/y2` 差的绝对值 0–56。

## 3. 实体与 ID

| 项 | 值 |
| --- | --- |
| ID | `town_id` / `eTown` 0–96 |
| 非法 | -1 |
| 原版条数 | 97（内容包实例计数） |
| 空特产品 | `trading_item.empty` = 106 |
| 每港特产品槽 | 5（`eSpecialProduct`） |
| region | 0–20，21 档 |
| 首都 | 21 座，每 region 一座 |
| 别名 | 稳定 key（`london` …），整数 ID 保持原版 |

`eTownShare` 97 与港口 1:1，份额表不属于 world。

哨兵对照（禁止用 0 当空槽：0 是交易品表第一项）：

| 用途 | 原版 | 重构 |
| --- | --- | --- |
| 非法港口 | -1 | `invalid` |
| 空特产品 | 106 | `trading_item.empty` |
| 若干 UI 选港未选 | 0x62 = 98 = NUM_TOWN+1 | `town.none`（见航线文档的 `CSelectTownRouteMap +0x110`） |

## 4. 算法 / 公式

港口定义无伤害/价格公式。

原版立刻数（进 caps / 内容包，不进代码）：

```
town.count_original          = 97
town.record_stride           = 0x84
town.name_bytes              = 20
town.special_product_slots   = 5
town.region_count_original   = 21   # region 0..20
town.develop_cap             = 9999 # 经济状态，见 caps.yaml town_develop
empty_trading_item_id        = 106
```

首都判定：内容表 `is_capital: bool`，不要写 `if region==0 -> 伦敦`。

特产品槽：

```
for slot in 0..slots-1:
    id = town.special_product_slots[slot].trading_item_id
    if id == empty_trading_item_id: skip
    else: Economy.bind_special(town_id, slot, id)
```

价、供给、品级由 economy 读自己的表。world 不提供 `supply_or_price`。

## 5. 耦合分析

原版：

- `CTown` 是 `CSaveObject`，定义和运行时状态打在一个 对象上。
- `vfunc_05` 每次从 DLL 拷 132 字节再映射到运行时偏移，改一个港要碰 World0.dll。
- 循环上界 `0x61`、`NUM_TOWN` 写死在 `CTownElement` / `CWorldData`。
- 特产品 6 字节里混了 `trading_item_id`（world/economy 外键）和价量（economy）。
- `region` 被当国家用，但字段名不是 `eCountry`。
- 入港：Ocean 直接进 `CTownScene`，场景再找 `CBar` 等设施。

重构：

```
Ocean.ArrivedAtTown { fleet_id, town_id }
  -> TownLife.open(town_id)
  -> Economy.refresh(town_id)
  -> Narrative.check(town_id)
```

world 只回答 `Town.def(town_id)`。

## 6. 解耦后的配置

`config/world/towns.yaml`：

```yaml
schema: dk4.world.towns
version: 1
pack: vanilla-world0
empty_trading_item_id: 106
towns:
  - id: 0
    key: london
    name: 伦敦
    coords:
      space: original
      x1: 20080
      y1: 4272
      x2: 20136
      y2: 4280
    is_capital: true
    region_id: 0          # 是否 eCountry：未确认
    country_id: null      # 待与 factions/countries.yaml 对齐
    culture_id: null
    force_id: null        # 初始所属势力，factions 表填
    special_product_slots:
      - slot: 0
        trading_item_id: 99
      - slot: 1
        trading_item_id: 47
      - slot: 2
        trading_item_id: 51
      - slot: 3
        trading_item_id: 90
      - slot: 4
        trading_item_id: 91
    unknown:
      b38: 0
      b39: 0
      rest_hex: "01007f01000004501a001a00f811301100000000000000000000000026020f000000590119000000e501190000007e040c00000060090c006a006a000c00"
```

经济侧（对照，不在本文件落地）：

```yaml
# config/economy/special_products.yaml  — 不要抄进 towns.yaml
- town_id: 0
  slot: 0
  trading_item_id: 99
  supply_or_price: 4500   # 原版字段名未恢复，JSON 暂用此名
  grade: 15
```

`region` 内容包默认 0–20 与下表首都对应。模组可加 region，但 `is_capital` 必须显式写。

## 7. 独立旋钮

- 增删港口（表长；ID ≥ 1000 给模组）
- 坐标、第二锚点
- 哪座是首都、region 划分
- 每港槽数（原版 5；改槽数只动 economy schema version）
- 空槽哨兵（必须 ≠ 0 且 ≠ -1 混用）
- 名字 / key / i18n（文案在 `ui/strings`，表内 `name` 仅调试对照）

改市价曲线、发展度上限、设施列表不必改本表 schema。

## 8. 对外接口

```
Town.def(town_id) -> {
  key, name_key, x1, y1, x2, y2, is_capital, region_id,
  special_slots: [{slot, trading_item_id}]
}
Town.by_key(key) -> town_id
Town.capitals() -> [town_id]
Town.in_region(region_id) -> [town_id]
Town.special_item(town_id, slot) -> trading_item_id | empty
```

命令：无。world 港口定义只读。状态写入走 economy/factions/town-life。

事件：不由 world 发「入港」。Ocean 发 `ArrivedAtTown { fleet_id, town_id }`。

## 9. 未核实字段

DLL 记录 `+0x46 .. +0x83`（62 字节）整段进 `unknown.rest_hex`。伦敦这段以 `01 00 7f 01 00 00 04 50 1a 00 1a 00 … 6a 00 6a 00 0c 00` 结尾，后部再次出现 106 哨兵，`[推断]` 仍与交易品有关，未命名。

| 偏移/符号 | 观察 | 等级 |
| --- | --- | --- |
| +0x26 `b38` | 0 或 2–24 或 26；26=NUM_TOWNFASHION | `[推断]` 流行？ |
| +0x27 `b39` | 全 0 | `[未知]` |
| +0x28 槽内 u16×2 价/量、品级 | JSON 已抽出 | 经济字段，world 不解释 |
| 运行时 +0x18 循环 3 | 存档 | `[未知]` |
| 运行时 +0x1e/+0x24 循环 2 | 存档 | `[未知]` |
| 运行时 +0x4a 循环 19 | 与交易种类数相同 | `[推断]` 经济 |
| 运行时 +0x70 循环 2、+0x74/+0x75 | 存档 | `[未知]` |
| `eTownKind` `eTownAccess` `eTownStateKind` `eTownDevelop` | 头文件计数未恢复 | `[未知]` |
| 设施开关位 | 无字段名 | 未找到；不要把 rest_hex 猜成「有无造船厂」 |
| `x2,y2` 语义 | 相对 x1,y1 偏移很小 | `[推断]` 第二地图锚 |

`CTown` 运行时对象还有 Locator 基类坐标，与 DLL 四整数的对应关系未逐字节对完。

---

## 原版 97 港（World0）

坐标为原版单位，来源 `Docs/_data/towns_world0.json`。首都与 region 见上。

| id | key | 名 | region | 首都 | x1 | y1 | x2 | y2 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | london | 伦敦 | 0 | 是 | 20080 | 4272 | 20136 | 4280 |
| 1 | bristol | 布里斯托尔 | 0 | 否 | 19632 | 4304 | 19624 | 4312 |
| 2 | amsterdam | 阿姆斯特丹 | 1 | 是 | 20624 | 4112 | 20680 | 4120 |
| 3 | bruges | 布鲁日 | 1 | 否 | 20224 | 4320 | 20248 | 4312 |
| 4 | nantes | 南特 | 1 | 否 | 19872 | 4912 | 19864 | 4936 |
| 5 | hamburg | 汉堡 | 2 | 是 | 20976 | 4064 | 21016 | 4056 |
| 6 | lubeck | 吕贝克 | 2 | 否 | 21216 | 4000 | 21240 | 3992 |
| 7 | stockholm | 斯德哥尔摩 | 3 | 是 | 22032 | 3376 | 22072 | 3432 |
| 8 | oslo | 奥斯陆 | 3 | 否 | 21152 | 3376 | 21176 | 3416 |
| 9 | copenhagen | 哥本哈根 | 3 | 否 | 21360 | 3856 | 21400 | 3880 |
| 10 | riga | 里加 | 3 | 否 | 22656 | 3648 | 22648 | 3656 |
| 11 | lisbon | 里斯本 | 4 | 是 | 19024 | 5712 | 19016 | 5752 |
| 12 | ceuta | 休达 | 4 | 否 | 19376 | 6048 | 19384 | 6040 |
| 13 | seville | 塞维利亚 | 5 | 是 | 19312 | 5936 | 19304 | 5976 |
| 14 | valencia | 瓦伦西亚 | 5 | 否 | 19952 | 5648 | 19992 | 5672 |
| 15 | genoa | 热那亚 | 6 | 是 | 20992 | 5040 | 21016 | 5096 |
| 16 | marseille | 马赛 | 6 | 否 | 20512 | 5152 | 20536 | 5192 |
| 17 | syracuse | 锡拉库萨 | 6 | 否 | 21664 | 5856 | 21704 | 5880 |
| 18 | venice | 威尼斯 | 6 | 否 | 21376 | 4912 | 21400 | 4952 |
| 19 | athens | 雅典 | 7 | 是 | 22576 | 5744 | 22600 | 5800 |
| 20 | crete | 克里特 | 7 | 否 | 22800 | 6064 | 22808 | 6056 |
| 21 | cyprus | 塞浦路斯 | 7 | 否 | 23696 | 6064 | 23720 | 6056 |
| 22 | istanbul | 伊斯坦布尔 | 8 | 是 | 23184 | 5408 | 23192 | 5464 |
| 23 | ragusa | 拉古札 | 8 | 否 | 21872 | 5184 | 21880 | 5224 |
| 24 | beirut | 贝鲁特 | 8 | 否 | 23984 | 6112 | 23976 | 6120 |
| 25 | alexandria | 亚历山大 | 9 | 是 | 23424 | 6480 | 23448 | 6472 |
| 26 | tripoli | 的黎波里 | 9 | 否 | 21472 | 6336 | 21496 | 6328 |
| 27 | algiers | 阿尔及尔 | 9 | 否 | 20320 | 5904 | 20328 | 5896 |
| 28 | tunis | 突尼斯 | 9 | 否 | 21104 | 5872 | 21144 | 5880 |
| 29 | saint_george | 圣乔治 | 10 | 是 | 20416 | 9248 | 20440 | 9304 |
| 30 | madeira | 马德拉 | 4 | 否 | 18112 | 6352 | 18152 | 6376 |
| 31 | las_palmas | 拉斯帕马斯 | 5 | 否 | 18144 | 6832 | 18152 | 6824 |
| 32 | verde | 绿角 | 10 | 否 | 18064 | 8352 | 18056 | 8376 |
| 33 | luanda | 罗安达 | 10 | 否 | 21456 | 11040 | 21448 | 11064 |
| 34 | sofala | 索法拉 | 11 | 是 | 23872 | 12688 | 23896 | 12744 |
| 35 | cape_town | 开普敦 | 11 | 否 | 22080 | 13792 | 22088 | 13832 |
| 36 | mozambique | 莫桑比克 | 11 | 否 | 24512 | 11616 | 24552 | 11640 |
| 37 | mogadishu | 摩加迪沙 | 11 | 否 | 24992 | 9760 | 25016 | 9800 |
| 38 | basra | 巴士拉 | 12 | 是 | 25328 | 6656 | 25368 | 6712 |
| 39 | aden | 亚丁 | 12 | 否 | 25104 | 8480 | 25128 | 8520 |
| 40 | muscat | 马斯喀特 | 12 | 否 | 26496 | 7376 | 26520 | 7368 |
| 41 | hormuz | 霍尔木兹 | 12 | 否 | 26240 | 6960 | 26264 | 7000 |
| 42 | calicut | 卡利卡特 | 13 | 是 | 28448 | 8816 | 28440 | 8856 |
| 43 | goa | 果阿 | 13 | 否 | 28256 | 8400 | 28248 | 8424 |
| 44 | ceylon | 锡兰 | 13 | 否 | 28864 | 9232 | 28856 | 9256 |
| 45 | calcutta | 加尔各答 | 13 | 否 | 29760 | 7552 | 29768 | 7592 |
| 46 | anman | 安曼 | 13 | 否 | 30704 | 8144 | 30712 | 8184 |
| 47 | malacca | 马六甲 | 14 | 是 | 31328 | 9696 | 31320 | 9736 |
| 48 | brunei | 文莱 | 14 | 否 | 32432 | 9664 | 32440 | 9656 |
| 49 | manila | 马尼拉 | 14 | 否 | 33424 | 8368 | 33416 | 8392 |
| 50 | jakarta | 雅加达 | 15 | 是 | 31936 | 10688 | 31960 | 10680 |
| 51 | palembang | 巴邻旁 | 15 | 否 | 31584 | 10160 | 31608 | 10152 |
| 52 | ternate | 德尔纳特 | 15 | 否 | 34160 | 9824 | 34152 | 9848 |
| 53 | ambon | 安波那 | 15 | 否 | 34208 | 10400 | 34200 | 10424 |
| 54 | hangzhou | 杭州 | 16 | 是 | 33440 | 6640 | 33480 | 6632 |
| 55 | quanzhou | 泉州 | 16 | 否 | 33280 | 7152 | 33320 | 7176 |
| 56 | macau | 澳门 | 16 | 否 | 32656 | 7504 | 32680 | 7544 |
| 57 | seoul | 汉城 | 17 | 是 | 34064 | 5984 | 34056 | 6008 |
| 58 | nagasaki | 长崎 | 18 | 否 | 34384 | 6272 | 34376 | 6280 |
| 59 | osaka | 大阪 | 18 | 是 | 35040 | 6144 | 35032 | 6168 |
| 60 | naha | 那霸 | 18 | 否 | 34208 | 7024 | 34200 | 7048 |
| 61 | havana | 哈瓦那 | 19 | 是 | 10832 | 7424 | 10856 | 7416 |
| 62 | santo_domingo | 圣多明尼各 | 19 | 否 | 12272 | 7936 | 12296 | 7976 |
| 63 | saint_john | 圣约翰 | 19 | 否 | 12640 | 7936 | 12664 | 7928 |
| 64 | jamaica | 牙买加 | 19 | 否 | 11456 | 7984 | 11480 | 8024 |
| 65 | veracruz | 委拉克鲁斯 | 20 | 是 | 9248 | 7808 | 9304 | 7832 |
| 66 | merida | 梅里达 | 20 | 否 | 10064 | 7616 | 10072 | 7608 |
| 67 | portobelo | 波托韦洛 | 20 | 否 | 11104 | 8960 | 11112 | 8952 |
| 68 | maracaibo | 马拉开波 | 20 | 否 | 12048 | 8768 | 12072 | 8760 |
| 69 | pernambuco | 伯南布哥 | 20 | 否 | 16112 | 10864 | 16152 | 10872 |
| 70 | trujillo | 特鲁希略 | 20 | 否 | 10448 | 8224 | 10472 | 8216 |
| 71 | cayenne | 卡宴 | 20 | 否 | 14080 | 9392 | 14104 | 9384 |
| 72 | sierra_leone | 塞拉利昂 | 10 | 否 | 18480 | 8928 | 18488 | 8968 |
| 73 | sao_tome | 圣多美 | 10 | 否 | 20704 | 9968 | 20744 | 9976 |
| 74 | madagascar | 马达加斯加 | 11 | 否 | 25040 | 11760 | 25048 | 11752 |
| 75 | mombasa | 蒙巴萨 | 11 | 否 | 24384 | 10432 | 24424 | 10456 |
| 76 | socotra | 索哥德拉 | 12 | 否 | 26000 | 8592 | 26024 | 8584 |
| 77 | karachi | 卡拉奇 | 13 | 否 | 27872 | 7664 | 27880 | 7704 |
| 78 | madras | 马德拉斯 | 13 | 否 | 28896 | 8480 | 28936 | 8504 |
| 79 | masulipatam | 马斯利巴丹 | 13 | 否 | 29120 | 8112 | 29160 | 8136 |
| 80 | aceh | 亚齐 | 15 | 否 | 30784 | 9408 | 30792 | 9400 |
| 81 | gia_dinh | 吉阿丁 | 14 | 否 | 31904 | 8816 | 31944 | 8840 |
| 82 | banjarmasin | 班贾尔马辛 | 15 | 否 | 32688 | 10352 | 32696 | 10392 |
| 83 | makassar | 望加锡 | 15 | 否 | 33264 | 10544 | 33256 | 10552 |
| 84 | surabaya | 泗水 | 15 | 否 | 32512 | 10816 | 32552 | 10840 |
| 85 | yizhou | 沂州 | 16 | 否 | 33280 | 6160 | 33304 | 6152 |
| 86 | pensacola | 朋沙科拉 | 19 | 否 | 10304 | 6608 | 10312 | 6648 |
| 87 | abkhazia | 阿布哈兹 | 8 | 否 | 24280 | 5344 | 24280 | 5384 |
| 88 | relistad | 雷利史塔特 | 1 | 否 | 20720 | 4128 | 20712 | 4120 |
| 89 | tamsui | 淡水 | 16 | 否 | 33424 | 7232 | 33416 | 7224 |
| 90 | saint_malo | 圣马罗 | 1 | 否 | 19712 | 4592 | 19720 | 4574 |
| 91 | naples | 拿坡里 | 6 | 否 | 21616 | 5456 | 21624 | 5496 |
| 92 | benghazi | 班加西 | 9 | 否 | 22400 | 6336 | 22408 | 6320 |
| 93 | manado | 马纳多 | 15 | 否 | 33872 | 9808 | 33888 | 9808 |
| 94 | busan | 釜山 | 17 | 否 | 34272 | 6112 | 34272 | 6160 |
| 95 | edo | 江户 | 18 | 否 | 35520 | 6048 | 35536 | 6096 |
| 96 | caracas | 卡拉卡斯 | 20 | 否 | 12624 | 8816 | 12636 | 8808 |

region → 首都：

| region | 首都 town_id | 名 |
| --- | --- | --- |
| 0 | 0 | 伦敦 |
| 1 | 2 | 阿姆斯特丹 |
| 2 | 5 | 汉堡 |
| 3 | 7 | 斯德哥尔摩 |
| 4 | 11 | 里斯本 |
| 5 | 13 | 塞维利亚 |
| 6 | 15 | 热那亚 |
| 7 | 19 | 雅典 |
| 8 | 22 | 伊斯坦布尔 |
| 9 | 25 | 亚历山大 |
| 10 | 29 | 圣乔治 |
| 11 | 34 | 索法拉 |
| 12 | 38 | 巴士拉 |
| 13 | 42 | 卡利卡特 |
| 14 | 47 | 马六甲 |
| 15 | 50 | 雅加达 |
| 16 | 54 | 杭州 |
| 17 | 57 | 汉城 |
| 18 | 59 | 大阪 |
| 19 | 61 | 哈瓦那 |
| 20 | 65 | 委拉克鲁斯 |
