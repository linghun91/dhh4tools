# 港口

港口是世界图上的节点。world 存**定义**：key、名字、原版坐标、是否首都、文化圈、挂港遗迹；运行态只存已知位与商业 / 军事发展度（06「状态所有权」）。特产品、市价、供给、份额、驻军、设施玩法全部用同一个 town key 挂到别的系统。

## 1. 职责边界

管：

- `eTown` 97 条定义（World0 / World1 各一份内容包）
- 名字、`x1,y1,x2,y2`、`is_capital`、`culture`（原 `region`）、`remains`
- 与势力/经济的 ID 关联（不存关系数值）
- 开局「已知」位（DLL `+0x46` bit0；航线与海域图的可见前提）
- 商业 / 军事发展度运行态（`CTown+0x1e/+0x20`；写入走 `Towns.apply_develop`，唯一发 `TownDeveloped`；上限 `caps.town_develop`）

不管：

- 特产品槽整行：商品外键、门槛、基准量、槽底价、供给（`config/economy/special_products.yaml`；原版空槽 106 只在 `legacy_codec`）
- 市场价、投资（economy；投资转化只发命令 `Towns.apply_develop`）
- 份额 `eTownShare`（factions `Share.*`；economy 只发 `Share.add`，原版与港口 1:1）
- 流行 `eTownFashion`（factions）
- 设施里具体干什么（`config/town/facilities.yaml`：酒馆、造船、公会、宫殿、广场…）
- 入港场景 `CTownScene`（ui / town-life）
- 航线是否已开通（routes）
- 舰队停靠、补给消耗（ocean / fleets）

设施开关位已钉到 DLL `+0x48`（= 地图点种类掩码，见 §2），但它的**所有权在 town-life**：本表只在提取时转交到 `config/town/town_map_points.yaml`，不在 `towns.yaml` 存。`CBar` `CInn` `CGuild` `CPalace` `CPort` `CShipyard` `CPlazaView` 是城镇活动类，不在本表。

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
| +0x25 | uint8 | `culture_id`（旧称 region；CCulture 下标） | 0 |
| +0x26 | uint8 | `remains_id`（YAML 外键 `remains`；旧 JSON 名 b38）= 附近遗迹，26=无 | 0（巨石阵） |
| +0x27 | uint8 | `byte_0x27`（旧 JSON 名 b39） | 0（全表恒 0） |
| +0x28 | 5×6 字节 | 特产品槽（定义）：`+0` u8 商品 id、`+2` u16 上架门槛、`+4` u8 供给基数 | `(99, 4500, 15)` |
| +0x46 | u16 | → `CTown+0x10` 位掩码（bit0 已知） | 1 |
| +0x48 | u32 | → `CTown+0x14` 地图点种类掩码 | `0x17f` |
| +0x4c | 3×(u8,u8) | → `CTown+0x18` 合约/份额槽 `(force_id, 份额)` | `(4,80)(26,0)(26,0)` |
| +0x52 | 2×u16 | → `CTown+0x1e` 商业 / 军事发展度 | 4600, 4400 |
| +0x56 | 2 字节 | 对齐填充（未被拷贝） | 0 |
| +0x58 | 2×u32 | → `CTown+0x24` | 0, 0（全表恒 0） |
| +0x60 | 5×6 字节 | → `CTown+0x2c` 特产品运行态 `{状态位 u8, pad, 槽底价 u16, 供给 u8, 本月已购 u8}` | `(0,550,15,0)…` |
| +0x7e | 2 格 × 两字节：u8 trading_item + u8 等级 `[确认]`；原版按 word 拷贝 | → `CTown+0x70` 流行 2 格 `(trading_item, 等级)` | `(106,0)×2`（全表） |
| +0x82 | u8,u8 | → `CTown+0x74` 港口状态、`+0x75` 状态已持续月数 | 12, 0（全表） |

`+0x46` 起的映射来自 `CTown::vfunc_05`（body `0x1402ae3c0`）反汇编：`r_sFixTown` 在栈 `+0x30`，逐段 `movzx/mov` 到运行时偏移。`[确认]` 偏移与宽度；各段语义见下。

- `[确认]` 特产品槽（每槽 6 字节）：`+0` u8 `trading_item_id`；`+1` pad；`+2` u16 上架门槛（港口**商业**发展度 ≥ 门槛才出现）；`+4` u8 每月基准量（u16 读低字节；月初补货 `base*(commerce/50+100)/100`）。字段名以 `04-economy/04-special-products-investment.md` 与词表 §10 为准（`trading_item` / `listing_threshold` / `supply_base`）；更早的 JSON 键 `supply_or_price`/`grade` 是误标，`_data/towns_world0.json` 已改。空槽 `item_id = 0x6A = 106`（`ITEM_EMPTY`，不是 -1，也不是 0）。来源：`towns_world0.json`，`srcdata/source/Constants.h`，`SpecialProductID.h` 每港 5 槽，`num_values.json` `NUM_SPECIALPRODUCT = 5`。
- `[确认]` 这 5 槽（含商品外键）整行归经济；world 提取时转交，不保留。
- `[确认]` `CTown` 基类 `CSaveObject -> CObject -> CLocator`。来源：`class_hierarchy.txt`。`CTown::vfunc_05` RTC `r_sFixTown` 132 字节，从 `GetFixTownProc` `rep movsb` 拷进运行时对象。来源：`srcdata/source/game/town/CTown.cpp`。
- `[确认]` `CTownElement` 循环 `eTown >= 0x61`（97）通知每个港口对象。来源：`srcdata/source/game/world/CTownElement.cpp`。`CWorldData::vfunc_05` 同样 `GetFixTownProc` 循环 97。来源：`CWorldData.cpp`。
- `[确认]` `CTown` 存档/读档（`vfunc_07` / `vfunc_06`）按偏移扫：`+0x10,+0x14`；2 次 `+0x1e/+0x24`；3 次 `+0x18/+0x19`；5 次特产品 `+0x2c…+0x31`；**19** 次 `+0x4a`；2 次 `+0x70/+0x71`；再 `+0x74,+0x75`。来源：`CTown.cpp`，`catalog/fields.tsv` 行 `CTown`。拷贝宽度（`0x1402ae3c0` 反汇编）：`+0x70+2i` 按 u16 从 `+0x7e+2i` 拷、`+0x74` 按 u16 从 `+0x82` 拷，存档按字节拆成两半。
- `[确认]` `CTown+0x4a` 的 19 个 `u16` **不来自 DLL**：`vfunc_05` 循环 `< 0x13` 一律写 `0x64`（100）。19 与 `NUM_TRADINGITEMKIND = 19` 同数，`[推断]` 按交易种类挂的经济指数，初值 100。归 economy。
- `[确认]` `CTown+0x10`（← DLL `+0x46`）是 `u16` 位掩码。`CTown::vfunc_04`（vtable `0x1405ae9d8` 槽 4，thunk `0x14000965b`）= `fn_140262570(this+0x10, 0) > 0`，后者是 `word & (1 << idx)`。海域图列港（`fn_14001259e`）和航线显示（`fn_14001389f`，见 05）都要求它为真。World0 有 88 港置位，9 港为 0：86 朋沙科拉、87 阿布哈兹、88 雷利史塔特、89 淡水、90 圣马罗、92 班加西、94 釜山、95 江户、96 卡拉卡斯。`[推断]` bit0 = 「玩家已知该港」，开局未知的 9 港都是 PK 追加港。运行时谁置位 `[未知]`。
- `[确认]` `CTown+0x14`（← DLL `+0x48`）是地图点种类掩码：`10-mappoint-kinds.md` 的 `fn_140002806` 测 `*(obj+0x14) & (1<<kind)`。World0 用到 bit 0–8：码头 bit3 全 97 港；广场/交易所/旅馆 88；酒馆 85；总督府 58；造船厂 41；王宫 35；行会 21。bit 9–11（遗址/教会/城门）初始全 0。**这就是设施开关位**，归 town-life。
- `[确认]` `CTown+0x18`（← DLL `+0x4c`）3 槽 `(u8 force_id, u8 份额)`，即王宫/投资的**合约份额槽**。拷贝时值为 0 的槽把 id 改写为 `0x1a`（26 = `NUM_FORCE + 1`，`NUM_FORCE = 25`，与 98/106/232 同为「计数+1」空值惯例）。`fn_1400092af(town, force)` 扫 3 槽返回匹配势力的份额，读者有：投资加份额（`04-economy/07-investment-tribute.md`）、广场 `fn_140166580` 累加与 100 比、流行影响 `fn_1400088f0(town) < 0x50`（`02-factions/03-relations-influence.md`）、近港村传闻 `> 0x1e`（30）。伦敦初值 `(4 克利福德军, 80)`。初值表 `config/factions/town_shares_initial.yaml` 归 factions；投资写入的份额增量由 economy 算。**不是流行槽**：流行在 `+0x70`（下条），旧稿与 `08-town-life/01-facilities.md` 的「流行读法」作废。
- `[确认]` `CTown+0x2c` 5×6 字节（← DLL `+0x60`，`rep movsb` `0x1402ae5e6`）是特产品**运行态**：`+0x2c` u8 状态位（bit0 = 隐藏/待种锁，不上架）、`+0x2d` pad、`+0x2e` u16 **槽底价**（买入报价，伦敦煤 550）、`+0x30` u8 当前供给、`+0x31` u8 本月已购（月初补货清 0）。读写者见 `04-economy/04-special-products-investment.md` §2.1。归 economy。
- `[确认]` `CTown+0x70`（← DLL `+0x7e`）2 格，每格两字节：u8 trading_item + u8 等级 `[确认]`；原版按 word 拷贝（`CTown::vfunc_05`，`legacy_codec` 同此表述），`fn_1400131ce` 扫两格当流行度（`04-economy/02`），World0 初值全为 `(106, 0)`：空值是交易品空 106，不是 26。归 factions（流行）。
- `[确认]` `CTown+0x74`（← DLL `+0x82` 低字节）u8 港口状态 0–12：月初 `CTownElement::vfunc_04` 按它走跳表 `0x140267cc8`（`04-economy/02`）；改装/造船计价 `fn_14000643d` 用它选倍率（0 ×2、3 ×1.2、4 ×0.8，`05-ships-fleet/04`）。写入口 `fn_14000bcf8(town, state, flag)`（body `0x140265d40`）：`+0x74 = state`、`+0x75 = (u8)flag`，`state != 12` 时再调 `fn_14000db4d`；`fn_14000c509`（`0x140265270`）复位 `+0x74 = 12`、`+0x75 = 0`。World0 初值全 12。`[推断]` 3/4/12 = 繁荣/不景气/平常。`+0x75` = 本状态已持续月数 `[确认]`（写入方传 1、`fn_140002d1a` 月初 +1、`fn_14000c509` 清 0；状态机三段见 `04-economy/02` §2.3）。归 economy：对外是 `Economy.town_state(town_id)` 返回状态 key（不是 world 的 `Towns.kind`）。
- `[确认]` `CTown+0x1e` / `+0x20` 两个 u16 = 商业 / 军事发展度：`fn_14000bc03(town, 0/1)` 读（改装/造船/炮种上架、地图点解锁、密谈座位都经它）。伦敦初值 4600 / 4400。上限 9999。归 world `Towns` 运行态（06；旧稿「归 economy」作废）。
- `[确认]` 21 个 `is_capital==1` 的港，culture 0–20 各一。名单与 `Docs/00-overview/04-id-registry.md` 一致。
- `[确认]` `region`（`+0x25`）是 `culture_id`：`fn_14001055a` 读 STown `+0x25`，`fn_1400130d9` 用它乘 `0xe8` 索引 `CCultureElement`（`CWorld+0x16f90`，21 项，`CCultureElement.cpp` 构造 `(0xe8, 0x15)`）；`fn_140013a02` 把 `0x16` 当无效。`fn_14000a0c4` 在该值 `== 0x12`（18，日本圈）时换地图点贴图。它**不是** `eCountry`；文中沿用 `region` 字样仅为与旧表对齐，schema 改名 `culture_id`。
- `[确认]` `remains_id`（`+0x26`，旧 JSON 名 b38）是该港对应的遗迹 id，`26 = NUM_REMAINS+1` 为无（协调方核定；与遗迹名表互证：伦敦 0 巨石阵、斯德哥尔摩 3 木条教堂、塞维利亚 5 圣地亚哥大教堂、热那亚 6 罗马竞技场、雅典 7 巴特农、伊斯坦布尔 8 洞窟修道院、亚历山大 9 金字塔）。取值 0、2–24 各恰一港，余 73 港为 26；遗迹 1（木造教堂）无港对应。**不是流行 ID**。
- `[确认]` World1 港口名与 World0 前部对照相同（至少 0–9 同名）。来源：`Docs/_data/world1_names.json`。同一 schema，两份包。
- `[确认]` 坐标不是 `World.dat` 下标。见 `01-map-and-sea-areas.md`。`x1/x2`、`y1/y2` 差的绝对值 0–56，中位数 24。
- `[推断]` `x1,y1` 是城区点、`x2,y2` 是泊船点。依据：按 `/16` 落格后，97 个 `x1` 点有 92 个在陆地类、5 个在岸类；97 个 `x2` 点有 93 个在岸类、4 个在开阔海（`01-map-and-sea-areas.md` 三类谓词）。

## 3. 实体与 ID

| 项 | 值 |
| --- | --- |
| 主键 | 命名空间 key（`vanilla:london` …）；运行时整数由加载器分配，原版包导入器写 `vanilla_index` = `eTown` 0–96 |
| 原版条数 | 97（内容包实例计数；`CTownElement` 循环 `< 0x61`） |
| 每港特产品槽 | 5（`eSpecialProduct`；槽表归 economy） |
| `culture`（原 `region`） | 21 档，外键到 factions `cultures` |
| 首都 | 21 座，每文化圈一座（显式 `is_capital`） |
| 挂港遗迹 | `remains` 外键；原版 24 港有，余 73 港无 |

`eTownShare` 97 与港口 1:1，份额表不属于 world。

空值：运行时一律 `null`（`Option<Id>`）。原版哨兵只在导入 / 导出层 `config/ids/legacy_codec.yaml` 出现，导出越界拒绝（`00-overview/04-id-registry.md`）：

| 用途 | 原版值 | 重构 |
| --- | --- | --- |
| 非法港口 | -1 | `null` |
| 空特产品 | 106 = `ITEM_EMPTY` | 槽不写 / `null` |
| UI 选港未选、舰队不在港 | 0x62 = 98 = NUM_TOWN+1 | `null` |
| 份额槽空势力 | 0x1a = 26 = NUM_FORCE+1 | `null` |
| 文化圈无效 | 0x16 = 22 = 21+1（`fn_140013a02`） | `null` |
| 无挂港遗迹 | 0x1a = 26 = NUM_REMAINS+1 | `null` |

## 4. 算法 / 公式

港口定义无伤害/价格公式。

原版立刻数（进 caps / 内容包，不进代码）：

```
town.count_original          = 97
town.record_stride           = 0x84   # 仅 legacy 导入器用
town.name_bytes              = 20     # 同上
town.special_product_slots   = 5      # economy schema
town.culture_count_original  = 21
caps.town_develop            = 9999   # 发展度上限；发展度是 world Towns 运行态（06）
town.share_slots             = 3      # CTown+0x18，归 factions（Share.*）
town.trade_kind_index_init   = 100    # CTown+0x4a ×19，归 economy
```

首都判定：内容表 `is_capital: bool`，不要写 `if culture==0 -> 伦敦`。

特产品槽（商品、门槛、基准量、槽底价、运行态）整行归 economy（`04-economy/04-special-products-investment.md`），world 不再持有 `trading_item` 外键，避免两处各存一份。

## 5. 耦合分析

原版：

- `CTown` 是 `CSaveObject`，定义和运行时状态打在一个对象上。
- `vfunc_05` 每次从 DLL 拷 132 字节再映射到运行时偏移，改一个港要碰 World0.dll。
- 循环上界 `0x61`、`NUM_TOWN` 写死在 `CTownElement` / `CWorldData`。
- 特产品 6 字节里混了 `trading_item_id` 和价量（economy）。
- `culture_id`（旧称 region）又被拿去推七大海域（`CCulture.+0x10`）和日本圈贴图（`== 18`）。
- 遗迹归属不在遗迹表，而在港口记录 `+0x26`。
- 入港：Ocean 直接进 `CTownScene`，场景再找 `CBar` 等设施。

重构：

```
Ocean.ArrivedAtTown { fleet_id, town_id }
  -> TownLife.open(town_id)
  -> Economy.refresh(town_id)
  -> Narrative.check(town_id)
```

world 只回答 `Towns.def(town_id)`。港口位置对外是 `LocatorRef(town, town_id)`。

## 6. 解耦后的配置

`config/world/towns.yaml`：

```yaml
schema: dk4.world.towns
version: 2
pack: vanilla-world0
towns:
  - key: vanilla:london             # 主键；导入器另写 vanilla_index: 0
    name_key: town.london.name
    coords:
      space: original
      x1: 20080
      y1: 4272
      x2: 20136
      y2: 4280
    is_capital: true
    culture: vanilla:england        # 原 +0x25；外键到 factions/cultures.yaml
    remains: vanilla:stonehenge     # 原 +0x26；本表是遗迹↔港的唯一来源；无则省略
    known_at_start: true    # DLL +0x46 bit0；其余 15 位原版恒 0
    develop_initial: { commerce: 4600, military: 4400 }   # DLL +0x52；发展度归 world Towns（06）
    unknown:
      byte_0x27: 0
      u32x2_0x58: [0, 0]    # -> CTown+0x24，语义未知
```

DLL 记录里其余段**在提取时按所有权分发**到各系统的初值表，不留在 world（同一 town key）。拆分由导出脚本按段完成，不手抄：

```yaml
# config/town/town_map_points.yaml（town-life 拥有）   ← DLL +0x48
- town: vanilla:london
  kinds: [vanilla:plaza, vanilla:exchange, vanilla:shipyard, vanilla:dock, vanilla:bar, vanilla:guild, vanilla:palace, vanilla:inn]   # 0x17f
# config/factions/town_shares_initial.yaml（factions 拥有，02-factions/03）← DLL +0x4c，值 0 的槽丢弃
- town: vanilla:london
  shares: [{ force: vanilla:clifford, share: 80 }]                            # 原版 force 4
# config/economy/special_products.yaml                 ← DLL +0x28 定义段 + +0x60 运行态初值
# config/factions/fashion_initial.yaml                 ← DLL +0x7e，(106,0) = 无，内容里不写
# config/economy/town_state_initial.yaml               ← DLL +0x82，港口状态（原版全 12）
```

`culture` 的默认划分与下表首都对应。模组可加文化圈，但 `is_capital` 必须显式写。

## 7. 独立旋钮

- 增删港口（表长；模组用自己命名空间的 key，加载器顺序分配整数）
- 坐标、泊船点（`x2,y2`）
- 开局已知集合（`known_at_start`；剧本包可整体覆盖）
- 哪座是首都、文化圈划分
- 遗迹挂在哪座港（`remains`）
- 名字 / key / i18n（文案在 `ui/strings`）

改市价曲线、发展度上限、设施列表、特产品槽数不必改本表 schema。

## 8. 对外接口

接口里的 `*_id` 是运行时整数 Id（加载器按 key 分配），不是原版下标；内容表只写 key。

```
Towns.def(town_id) -> {
  key, name_key, x1, y1, x2, y2, is_capital, culture_id, remains_id | null
}
Towns.is_known(town_id, force_id = player) -> bool   # 状态；原版 CTown+0x10 bit0
Towns.develop(town_id, track) -> int                 # 状态；track = commerce|military，原版 CTown+0x1E/+0x20
Towns.by_key(key) -> town_id
Towns.capitals() -> [town_id]
Towns.in_culture(culture_id) -> [town_id]
Remains.at_town(town_id) -> remains_id | null        # 原版 STown+0x26
Remains.town(remains_id) -> town_id | null           # 反查同一字段（09）
World.locate(LocatorRef(town, town_id)) -> (x1, y1)
```

命令：

- `Towns.mark_known(town_id, force_id)`（发 `TownBecameKnown { town_id, force_id }`）。
- `Towns.apply_develop(town_id, track, delta, reason)`：写方 economy（投资转化）、combat（炮击 / 攻城）、narrative。`Towns` 夹到 `[0, caps.town_develop]` 后发 `TownDeveloped { town_id, track, old, new, reason }`；此事件**只由 world `Towns` 发**，economy / factions / combat 不另发（06「状态所有权」）。

其余定义只读；份额、流行、景气、地图点等状态分别归 factions / economy / town-life。

事件：不由 world 发「入港」。Ocean 发 `ArrivedAtTown { fleet_id, town_id }`。

## 9. 未核实字段

DLL 记录 `+0x46 .. +0x83` 的分段与运行时去向已由 `CTown::vfunc_05` 钉死（§2 表）。仍未命名的只剩下表几项。

| 偏移/符号 | 观察 | 等级 |
| --- | --- | --- |
| +0x26 `remains_id` | 遗迹 id，26=无 | `[确认]` |
| +0x27 `byte_0x27` | 全 0 | `[未知]` |
| +0x28 槽 `+1`、`+5` 字节 | 全 0 | 填充 `[推断]` |
| 运行时 +0x10 其余 15 位 | 原版恒 0 | `[未知]`；bit0 已知位 `[推断]`；谁置位 `[未知]` |
| 运行时 +0x18 3 槽 | `(force_id, 份额)`，空 26 | `[确认]`（读写者见 §2） |
| 运行时 +0x1e/+0x20 | 商业 / 军事发展度 | `[确认]` `fn_14000bc03` |
| 运行时 +0x24 2×u32 | 全 0 | `[未知]` |
| 运行时 +0x2c 5×6 | 状态位 / 槽底价 / 供给 / 本月已购 | `[确认]`，economy |
| 运行时 +0x4a 19×u16 | 初值恒 100 | `[推断]` 按交易种类的经济指数 |
| 运行时 +0x70 2×(u8,u8) | `(106,0)×2` | `[确认]` 流行 (item, 等级) |
| 运行时 +0x74 / +0x75 | 12 / 0 | 港口状态 `[确认]` 读写者（`fn_14000bcf8` / `fn_14000c509`）；名字 `[推断]`；`+0x75` = 状态已持续月数 `[确认]`（`04-economy/02` §2.3） |
| `eTownKind` | `fn_14000c71b` = `is_capital ? 1 : 0` | `[确认]`（海域图标记，派生量，不存） |
| `eTownAccess` `eTownStateKind` `eTownDevelop` | 头文件计数未恢复 | `[未知]`；`eTownDevelop` 是发展度两轨（`fn_140009ae8` RTC）`[推断]` |
| 设施开关位 | DLL `+0x48` → `CTown+0x14` | `[确认]`（与 `10-mappoint-kinds.md` 互证） |
| `x2,y2` 语义 | 93/97 落岸类格 | `[推断]` 泊船点 |

`CTown` 运行时对象还有 Locator 基类坐标，与 DLL 四整数的对应关系未逐字节对完。

---

## 原版 97 港（World0）

坐标为原版单位，来源 `Docs/_data/towns_world0.json`。`id` 列是原版下标（`vanilla_index`），`culture` 列是 `+0x25` 原值。

| id | key | 名 | culture | 首都 | x1 | y1 | x2 | y2 |
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

culture → 首都：

| culture | 首都 | 名 |
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
