# 交易品目录

交易品是经济系统的内容主表：105 个可买卖货物、19 个种类。本篇只定义「是什么」；市价算法见 `02-market-price.md`，买卖会话见 `03-trade-loop.md`，港口特产品槽与投资见 `04-special-products-investment.md`。

## 1. 职责边界

本系统管：

- `eTradingItem` / `eTradingItemKind` 的稳定 ID、别名、显示名、种类归属
- 交易品静态属性（目录底价 `data_price`、种类、等级步长 `grade_step`、开局发现位、解锁发现物外键、百科文案 key）
- 空货格：运行时 `Option<TradingItemId>` / `null`；原版 106 只在 `ids/legacy_codec.yaml`

不管：

- 某港口产什么、供给多少（特产品槽，整行归 `04`）
- 市价、报价、金币（`02`）
- 货舱格子、堆叠、会话（`03`）
- 装备 / 发现物 `eItem`（`09-items`，231 件，不是交易品）
- UI 列宽、图标、`%6d` 格式（`12-ui-save`）

## 2. 原版事实

| 事实 | 标记 | 来源 |
| --- | --- | --- |
| `NUM_TRADINGITEM = 105`，合法 id 0–104 | [确认] | `srcdata/source/TradingItemID.h` |
| `INVALID_TRADINGITEM = -1` | [确认] | 同上；`constants.json` `INVALID_` |
| 空货格 / 空特产品 = `0x6A` = 106，**不在** 0–104 内 | [确认] | `Constants.h` `ITEM_EMPTY`；`num_values.json` `ITEM_EMPTY: 106`；`fn_14000680c` 等处 `cmp …,0x6a` |
| `NUM_TRADINGITEMKIND = 19`，循环上界 `0x13` | [确认] | `TradingItemKindID.h`；`CTrade::CommitDeal` RTC `eTradingItemKind` |
| 目录是 exe 内置表，**不在** World DLL | [确认] | `.data` VA `0x140688190` 起 105×16 B；World DLL 只导出 fleet/item/person/town/force（`srcdata/source/world/DllMain.cpp`） |
| 运行时 `CTradingItem` = `CTradingItemElement + 8 + id*0x40` | [确认] | `fn_140006884`（`imul 0x40` / `+8`）；取对象入口 `fn_140001c94(id)` |
| 对象 `+0x10` u16 = 目录 `data_price` | [确认] | `fn_140007022` → `fn_140127020`（`movzx word`） |
| 对象 `+0x12` u16 = `grade_step` | [确认] | `fn_140005da8` |
| 对象 `+0x14` u8 = `kind_id` | [确认] | `fn_14000c2f2` → `fn_1400ff2a0`（`movzx byte`） |
| 对象 `+0x16` = 已发现位（开局取目录 +14） | [确认] | `fn_14000f9e3`；见 `05` §2.2 |
| `TRADINGITEM_DATAPRICE` 是 `USHORT`，有 MIN/MAX 符号 | [确认] | `Price.h` 注释；`asserts.txt` `sizeof(USHORT)` |
| 等级取值 1–5，6 = 不可买 | [确认] | `fn_140015168` 末尾 `fn_1400103d4(1, 6-q, 5)`；不可上架返回 6。`TRADINGITEMGRADE_MIN/MAX` 整数未写进头文件 |
| `TRADINGITEMSTATE_BITS` 存在 | [确认] 符号 | `constants.json`、`asserts.txt:133`；与 +0x16 的对应 `[推断]` |
| 种类短名 19 条 | [确认] | `fn_14000c522` / `fn_140007c8e`（RTC `as_szTradingItemKindName`）；串在 VA `0x1405aa064` 起，「武器」与别处共用 `0x140557524` |
| 种类归属 = 目录 +10 | [确认] | 与百科 19 组、下表 id 区间一致 |
| 百科文案在 mesfile `0x11fb`–`0x2d9b`，按 id 连写 | [确认] | `srcdata/catalog/mesfile_sc.txt` |
| 世界表 5 个特产品槽的 `item_id` 覆盖 0–104 **全部** 105 个 id | [确认] | `Docs/_data/towns_world0.json` |
| `CTradingItem` / `CTradingItemElement` 是运行时对象，目录是 exe 静态表 | [确认] | `srcdata/source/game/trade/CTradingItem.cpp` |

禁止用 0 当空格：0 是小麦。

### 2.1 exe 目录行布局（16 B）

| 偏移 | 宽 | 字段 | 观察 |
| --- | --- | --- | --- |
| +0 | QWORD | 名称指针 | GBK 短名 |
| +8 | u16 | `data_price` | 1000–9500 |
| +10 | u8 | `kind_id` | 0–18 |
| +11 | u8 | 解锁发现物 `eItem` | 0–19；`0xE8`=无 |
| +12 | u16 | `grade_step` | 300–1900 |
| +14 | u8 | 开局已发现 | 12 件为 0 |
| +15 | u8 | — | 全 0 |

`[确认]` 逐行读 `Game/DK4HD_sc.exe`；与 `05-goods-art-spawn-display.md` §2.2 同表。

`data_price` 的三处用途 `[确认]`：

1. `fn_14000a691` 在该货**不是**本港特产品时的底价（`fn_140007022` 分支）。玩家买货栏只列特产品，所以这条路径主要服务下一条
2. 种类指数滑动时的参考价（`fn_1400108b6` 对同种类全部已发现货按等级 4 报 `fn_14000a691` 取平均）
3. 文化圈卖价回升的上限（`CommitDeal` 同一货卖 ≥15 件；见 `02` §2.6）

本港特产品买价**不用**它，用港口槽价（`04`）；卖价**不用**它，用文化圈价（`02`）。

## 3. 实体与 ID

| 实体 | 原版枚举 | 原版计数 | 非法 | 空哨兵 | 重构 key |
| --- | --- | --- | --- | --- | --- |
| 交易品 | `eTradingItem` | 105 | -1 | — | `vanilla:<alias>`（域 `trading_item`） |
| 种类 | `eTradingItemKind` | 19 | -1 | — | `vanilla:<alias>`（域 `trading_item_kind`） |
| 空货格 | 非枚举成员 | 106 | — | `0x6A` | 运行时 `null`；106 只在 `legacy_codec` |
| 交易品等级 | `TRADINGITEMGRADE_ID` | 1–5 | — | 6 = 不可买 | `trading_item_grade` |

运行时以表长循环，不写死 105。原版包实例计数是 105 / 19。

模组追加交易品用命名空间 key（`mymod:saffron`），整数下标由加载器紧凑分配、只在运行时用。原版 106 只是导入 / 导出编码：模组顺序分配到第 2 件就会撞上 106，所以运行时空货一律 `null`，导出原版格式时下标 ≥105 拒绝（DECISIONS D1）。

### 3.1 种类 0–18 `[确认]`

短名取 exe `as_szTradingItemKindName`（§2）。id 区间取目录 +10。引擎只依赖 `id` 与 `item.kind_id`。

| id | key | name | 交易品 id 范围 | 件数 |
| --- | --- | --- | --- | --- |
| 0 | food | 食品 | 0–10 | 11 |
| 1 | seasoning | 调味料 | 11–16 | 6 |
| 2 | delicacy | 珍品 | 17–21 | 5 |
| 3 | spice | 香辣料 | 22–30 | 9 |
| 4 | stimulant | 嗜好品 | 31–34 | 4 |
| 5 | alcohol | 酒类 | 35–38 | 4 |
| 6 | precious_metal | 贵金属 | 39–41 | 3 |
| 7 | craft | 工艺品 | 42–46 | 5 |
| 8 | fiber | 纤维 | 47–50 | 4 |
| 9 | textile | 纺织品 | 51–57 | 7 |
| 10 | luxury | 奢侈品 | 58–61 | 4 |
| 11 | incense | 香料 | 62–65 | 4 |
| 12 | ornament | 装饰品 | 66–72 | 7 |
| 13 | gem | 宝石 | 73–78 | 6 |
| 14 | medicine | 药 | 79–83 | 5 |
| 15 | dye | 染料 | 84–89 | 6 |
| 16 | weapon | 武器 | 90–93 | 4 |
| 17 | ore | 矿石 | 94–99 | 6 |
| 18 | sundries | 杂货 | 100–104 | 5 |

### 3.2 交易品 0–104

`[确认]` 全表取自 exe 目录 `.data` VA `0x140688190`（文件偏移 `0x686b90`），步长 16，105 行（布局见 §2.1）。`name` 是 +0 指针指向的 GBK 短名；`kind` 是 +10；`data_price` 是 +8；`grade_step` 是 +12；`开局已发现` 是 +14；`发现物 eItem` 是 +11（`232`=`0xE8` 无，记作 —）。`key` 是稳定英文别名，不是原版符号。

百科长文（mesfile `0x11fb`–`0x2d9b`，按 id 连写）只作 `encyclopedia_key`，不再用来推短名。旧版用百科反推的两条短名改正：id 1 是「稻米」不是「米」，id 2 是「玉蜀黍」不是「玉米」。id 26「甜椒」百科讲的是多香果、id 45「象牙制品」百科讲镶嵌、id 55「绒织品」百科讲地毯——短名以 exe 为准，百科不一致照原样保留。

| id | key | name（exe） | kind | data_price | grade_step | 开局已发现 | 发现物 eItem |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | wheat | 小麦 | 0 | 1100 | 500 | 1 | — |
| 1 | rice | 稻米 | 0 | 1200 | 300 | 1 | — |
| 2 | corn | 玉蜀黍 | 0 | 1100 | 400 | 1 | — |
| 3 | potato | 马铃薯 | 0 | 1000 | 300 | 1 | — |
| 4 | soybean | 大豆 | 0 | 1000 | 500 | 1 | — |
| 5 | fish | 鱼肉 | 0 | 1100 | 600 | 1 | — |
| 6 | beef | 牛肉 | 0 | 1200 | 700 | 1 | — |
| 7 | pumpkin | 南瓜 | 0 | 1300 | 500 | 1 | 0 |
| 8 | cheese | 乳酪 | 0 | 1200 | 600 | 1 | — |
| 9 | tomato | 番茄 | 0 | 1300 | 800 | 1 | 1 |
| 10 | banana | 香蕉 | 0 | 1400 | 700 | 1 | 2 |
| 11 | sugar | 砂糖 | 1 | 1700 | 900 | 1 | — |
| 12 | honey | 蜂蜜 | 1 | 1800 | 1000 | 0 | 3 |
| 13 | salt | 盐 | 1 | 1600 | 800 | 1 | — |
| 14 | olive_oil | 橄榄油 | 1 | 1200 | 900 | 1 | — |
| 15 | coconut_oil | 椰子油 | 1 | 1100 | 600 | 1 | — |
| 16 | ginger | 生姜 | 1 | 2000 | 600 | 1 | — |
| 17 | lychee | 荔枝 | 2 | 2000 | 1000 | 1 | — |
| 18 | hazelnut | 榛子 | 2 | 1600 | 800 | 1 | — |
| 19 | raisin | 葡萄干 | 2 | 1200 | 300 | 1 | — |
| 20 | caviar | 鱼子酱 | 2 | 5200 | 1000 | 0 | 4 |
| 21 | shark_fin | 鱼翅 | 2 | 5200 | 1000 | 0 | 4 |
| 22 | pepper | 胡椒 | 3 | 4200 | 700 | 1 | 5 |
| 23 | clove | 丁香 | 3 | 4200 | 900 | 1 | 6 |
| 24 | nutmeg | 豆蔻 | 3 | 3500 | 800 | 1 | — |
| 25 | cinnamon | 肉桂 | 3 | 4200 | 700 | 1 | 7 |
| 26 | allspice | 甜椒 | 3 | 3500 | 800 | 1 | 8 |
| 27 | tamarind | 罗望子 | 3 | 2500 | 600 | 0 | — |
| 28 | saffron | 藏红花 | 3 | 2400 | 700 | 1 | — |
| 29 | vanilla | 香草 | 3 | 3200 | 700 | 1 | — |
| 30 | chili | 辣椒 | 3 | 3800 | 600 | 1 | — |
| 31 | coffee | 咖啡 | 4 | 3500 | 600 | 1 | 9 |
| 32 | tea | 茶 | 4 | 3500 | 700 | 1 | 10 |
| 33 | cacao | 可可 | 4 | 3300 | 700 | 1 | 11 |
| 34 | tobacco | 烟草 | 4 | 4000 | 500 | 1 | 12 |
| 35 | wine | 葡萄酒 | 5 | 1900 | 1000 | 1 | — |
| 36 | brandy | 白兰地 | 5 | 2100 | 1100 | 1 | — |
| 37 | rum | 朗姆酒 | 5 | 1750 | 600 | 1 | — |
| 38 | sake | 清酒 | 5 | 2000 | 900 | 1 | — |
| 39 | gold | 金 | 6 | 9000 | 1800 | 1 | — |
| 40 | silver | 银 | 6 | 5200 | 1300 | 1 | — |
| 41 | platinum | 白金 | 6 | 9500 | 1900 | 0 | 13 |
| 42 | glassware | 玻璃 | 7 | 2600 | 900 | 1 | 14 |
| 43 | ceramics | 陶瓷器 | 7 | 4200 | 1200 | 1 | — |
| 44 | lacquerware | 漆器 | 7 | 2700 | 800 | 1 | — |
| 45 | inlay | 象牙制品 | 7 | 2200 | 600 | 1 | — |
| 46 | goldwork | 黄金艺品 | 7 | 3000 | 800 | 0 | — |
| 47 | wool | 羊毛 | 8 | 1500 | 600 | 1 | — |
| 48 | cotton | 棉花 | 8 | 1400 | 500 | 1 | — |
| 49 | silk | 丝绸 | 8 | 3500 | 900 | 1 | 15 |
| 50 | hemp | 麻 | 8 | 1100 | 300 | 1 | — |
| 51 | woolen | 毛织品 | 9 | 2200 | 800 | 1 | 16 |
| 52 | cotton_cloth | 棉织品 | 9 | 2100 | 600 | 1 | 16 |
| 53 | silk_cloth | 丝织品 | 9 | 5000 | 1100 | 1 | 16 |
| 54 | linen | 麻织品 | 9 | 1600 | 400 | 1 | 16 |
| 55 | pile_cloth | 绒织品 | 9 | 4200 | 900 | 1 | — |
| 56 | muslin | 纱布 | 9 | 3500 | 500 | 1 | — |
| 57 | velvet | 天鹅绒 | 9 | 3200 | 700 | 1 | — |
| 58 | antique | 古董 | 10 | 5500 | 1400 | 1 | — |
| 59 | fur | 毛皮 | 10 | 2900 | 1000 | 1 | — |
| 60 | painting | 绘画 | 10 | 6000 | 1200 | 1 | — |
| 61 | sculpture | 雕刻 | 10 | 5900 | 1300 | 1 | — |
| 62 | musk | 麝香 | 11 | 3500 | 900 | 0 | — |
| 63 | frankincense | 乳香 | 11 | 3800 | 800 | 1 | — |
| 64 | ambergris | 龙涎香 | 11 | 4000 | 1000 | 0 | — |
| 65 | sandalwood | 白檀 | 11 | 3400 | 800 | 1 | — |
| 66 | jadeite | 翡翠 | 12 | 4000 | 1000 | 1 | — |
| 67 | ivory | 象牙 | 12 | 4400 | 900 | 1 | — |
| 68 | coral | 珊瑚 | 12 | 3200 | 700 | 1 | — |
| 69 | tortoiseshell | 玳瑁甲 | 12 | 2750 | 900 | 1 | — |
| 70 | amber | 琥珀 | 12 | 3400 | 700 | 1 | — |
| 71 | crystal | 水晶 | 12 | 2600 | 1000 | 1 | — |
| 72 | nephrite | 软玉 | 12 | 2000 | 700 | 1 | — |
| 73 | pearl | 珍珠 | 13 | 5200 | 1200 | 1 | 17 |
| 74 | diamond | 金刚石 | 13 | 9500 | 1800 | 0 | — |
| 75 | sapphire | 蓝宝石 | 13 | 7500 | 1500 | 1 | — |
| 76 | emerald | 绿宝石 | 13 | 6900 | 1400 | 1 | — |
| 77 | ruby | 红宝石 | 13 | 7200 | 1300 | 1 | — |
| 78 | lapis | 天青石 | 13 | 3400 | 700 | 1 | — |
| 79 | rhinoceros_horn | 犀牛角 | 14 | 4300 | 900 | 1 | — |
| 80 | ginseng | 人参 | 14 | 5200 | 600 | 1 | — |
| 81 | quinine | 奎宁 | 14 | 3100 | 500 | 1 | — |
| 82 | apricot_kernel | 杏仁 | 14 | 2000 | 300 | 1 | — |
| 83 | cordyceps | 冬虫夏草 | 14 | 6100 | 1300 | 0 | 18 |
| 84 | turmeric | 郁金 | 15 | 2300 | 500 | 1 | — |
| 85 | indigo | 靛蓝 | 15 | 3400 | 600 | 1 | — |
| 86 | cochineal | 胭脂红 | 15 | 2300 | 500 | 1 | — |
| 87 | catechu | 槟榔 | 15 | 2300 | 500 | 1 | — |
| 88 | rhubarb | 大黄 | 15 | 2200 | 800 | 1 | — |
| 89 | mangosteen | 山竹果 | 15 | 2200 | 700 | 0 | 19 |
| 90 | musket | 洋枪 | 16 | 3400 | 1000 | 1 | — |
| 91 | cannon | 大炮 | 16 | 5200 | 1200 | 1 | — |
| 92 | sword | 刀剑 | 16 | 1700 | 700 | 1 | — |
| 93 | armor | 甲胄 | 16 | 1900 | 800 | 1 | — |
| 94 | iron_ore | 铁矿石 | 17 | 2100 | 600 | 1 | — |
| 95 | copper_ore | 铜矿石 | 17 | 1750 | 500 | 1 | — |
| 96 | tin_ore | 锡矿石 | 17 | 1750 | 400 | 1 | — |
| 97 | saltpeter | 硝石 | 17 | 2100 | 400 | 1 | — |
| 98 | sulfur | 硫磺 | 17 | 2500 | 300 | 1 | — |
| 99 | coal | 煤 | 17 | 2000 | 700 | 1 | — |
| 100 | lumber | 木材 | 18 | 1550 | 400 | 1 | — |
| 101 | leather | 皮草 | 18 | 1700 | 300 | 1 | — |
| 102 | wax | 蜡 | 18 | 1700 | 500 | 1 | 3 |
| 103 | gum | 糖胶 | 18 | 1700 | 400 | 1 | — |
| 104 | guano | 鸟粪石 | 18 | 4800 | 300 | 0 | — |
| 106 | empty | （空） | — | — | — | — | — |

106 是哨兵，不是目录行。

`name_i18n` 进 `config/ui/strings.*.yaml`，本表只留 `name` 作原版包缺省中文。

## 4. 算法 / 公式

本篇无玩法公式。目录是查表：

```
kind_id(item) = trading_items[item_id].kind_id
is_empty(item) = item is null                  # 运行时 Option<TradingItemId>
is_catalog(item) = item in trading_items       # 按 key 查表，不比下标区间
```

`data_price` 的使用见 `02-market-price.md` 的 `quote()`。世界表特产品槽里 100–6000 那列是**商业发展门槛**，不是价格；港口槽自己的底价在城镇记录 `+0x62+slot*6`（伦敦煤 550），见 `04` §2.1。`grade_step` 的使用见 `05` §2.4。

## 5. 耦合分析

原版硬编码：

- `CommitDeal` 栈上 `anSellTradingItemNum[105]`、`anBuyTradingItemKindNum[19]`
- 空货用 `0x6A`；hold `+2` 是买入港 `town_id`，清空写 `0x62`（无港哨兵，不是货物哨兵）
- 港口特产品、流行品、委托扣货都直接拿 `eTradingItem`
- `CTradingItemElement` 对象数组长度 `0x69`，末槽写 `0x6A`

重构后别人只许要 `trading_item_id` / `trading_item_kind_id`。

## 6. 解耦后的配置

```yaml
# config/economy/trading_item_kinds.yaml
schema: dk4.economy.trading_item_kinds
version: 1
kinds:                       # 主键 = 命名空间 key；vanilla_index 只供导入导出对照
  - key: vanilla:food
    vanilla_index: 0
    name_key: trading_item_kind.food.name     # 缺省中文「食品」在语言包
  - key: vanilla:spice
    vanilla_index: 3
    name_key: trading_item_kind.spice.name
  # ... 共 19 条，原版包
```

```yaml
# config/economy/trading_items.yaml   （本篇拥有；05 只在同一行追加 icon / spawn 字段）
schema: dk4.economy.trading_items
version: 1
items:
  - key: vanilla:wheat
    vanilla_index: 0
    name_key: trading_item.wheat.name
    kind: vanilla:food
    data_price: 1100       # exe 目录 +8
    grade_step: 500        # +12
    initially_discovered: true   # +14
    unlock_item: null      # +11；原版 0xE8 = 无 → null
    encyclopedia_key: trading_item.wheat.desc
  - key: vanilla:pepper
    vanilla_index: 22
    name_key: trading_item.pepper.name
    kind: vanilla:spice
    data_price: 4200
    grade_step: 700
    initially_discovered: true
    unlock_item: vanilla:peppercorn    # eItem 5（胡椒粒），key 同 09-items/01 目录；见 05 §2.7
    encyclopedia_key: trading_item.pepper.desc
```

没有 `empty_id` 字段：空货是 `null`。原版 106 / 0xE8 只出现在 `config/ids/legacy_codec.yaml`（`00-overview/04-id-registry.md`）。整数下标由加载器分配，存档头 `id_map` 存 key↔下标，不手写 `config/ids/` 映射表。

原版包 105 行全部从 exe 目录生成（§3.2），不手填。目录 `data_price` 与港口槽 `data_price`（`04`）、文化圈卖价（`02`）是三张表，各管一条报价路径，不要合并成一个「基础价」。

World0 / World1 共用这张目录（exe 内置）。模组换包时目录也是内容包的一部分，不是引擎常量。

## 7. 独立旋钮

改这些不应迫使航海 / 战斗改代码：

- 加种类、加交易品（表长变了，循环跟表）
- 改某件的 `kind_id`（市价按种类存，改归属会换那条 kind-rate）
- 改显示名 / 百科
- 空货永远是 `null`；原版 106 只在 `legacy_codec` 里改

## 8. 对外接口

查询（只读）：

```
Economy.item(trading_item_id) -> { id, key, kind_id, data_price, grade_step }
Economy.is_discovered(trading_item_id) -> bool          # 运行时位；写入见 05
Economy.kind(trading_item_kind_id) -> { id, key }
Economy.is_empty(trading_item_id | null) -> bool
Economy.catalog_size() -> int
```

不暴露 `CTradingItem*`、mesfile 偏移、世界表 hex。

事件：本篇不发事件。货物进出由 `03` 发 `TradeCommitted`。

## 9. 未核实字段

| 字段 | 现状 |
| --- | --- |
| `TRADINGITEMGRADE_MIN/MAX` 的整数 | 符号有，值无；代码夹 1–5、6 = 不可买 |
| `TRADINGITEMSTATE_BITS` | 只有符号；是否就是对象 +0x16 发现位 `[推断]` |
| 目录 +15 | 全 0，未见读取 |
| hold `+1/+2` | 已钉：+1 等级，+2 买入港（见 `03` §2.2） |
| World1 交易品是否同一 105 件 | 目录在 exe，两包共用 `[确认]`；World1 槽内容另见 `04` |
