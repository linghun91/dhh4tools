# 交易品目录

交易品是经济系统的内容主表：105 个可买卖货物、19 个种类。本篇只定义「是什么」；市价算法见 `02-market-price.md`，买卖会话见 `03-trade-loop.md`，港口特产品槽与投资见 `04-special-products-investment.md`。

## 1. 职责边界

本系统管：

- `eTradingItem` / `eTradingItemKind` 的稳定 ID、别名、显示名、种类归属
- 交易品静态属性（基础价 `data_price`、种类、百科文案 key）
- 空货格哨兵 `trading_item.empty`

不管：

- 某港口此刻卖什么、供给多少（特产品槽，world 只存 item id，economy 管供给）
- 市价、报价、金币（`02`）
- 货舱格子、堆叠、会话（`03`）
- 装备 / 发现物 `eItem`（`09-items`，231 件，不是交易品）
- UI 列宽、图标、`%6d` 格式（`12-ui-save`）

## 2. 原版事实

| 事实 | 标记 | 来源 |
| --- | --- | --- |
| `NUM_TRADINGITEM = 105`，合法 id 0–104 | [确认] | `srcdata/source/TradingItemID.h` |
| `INVALID_TRADINGITEM = -1` | [确认] | 同上；`constants.json` `INVALID_` |
| 空货格 / 空特产品 = `0x6A` = 106，**不在** 0–104 内 | [确认] | `Constants.h` `ITEM_EMPTY`；`num_values.json` `ITEM_EMPTY: 106` |
| `NUM_TRADINGITEMKIND = 19`，循环上界 `0x13` | [确认] | `TradingItemKindID.h`；`CTrade::CommitDeal` RTC `eTradingItemKind` |
| `TRADINGITEM_DATAPRICE` 是 `USHORT`，有 MIN/MAX 符号 | [确认] | `Price.h` 注释；`asserts.txt` `sizeof(USHORT)` |
| `TRADINGITEMGRADE` 是独立 ID 类型，有 MIN/MAX | [确认] | `constants.json` `TRADINGITEMGRADE_ID/MIN/MAX` |
| `TRADINGITEMSTATE_BITS` 存在 | [确认] | `constants.json`；语义未还原 |
| 百科文案在 mesfile `0x11fb`–`0x2d9b`，按种类连写 | [确认] | `srcdata/catalog/mesfile_sc.txt` |
| 世界表 5 个特产品槽的 `item_id` 覆盖 0–104 **全部** 105 个 id | [确认] | `Docs/_data/towns_world0.json` |
| 伦敦槽 `(99,47,51,90,91)` = 煤 / 羊毛 / 毛织品 / 洋枪 / 大炮 | [确认] | 上表 + 百科顺序；与汉堡 `(5,6,7,36,94)` 鱼肉/牛肉/南瓜/白兰地/铁矿石 交叉一致 |
| 因此：**百科连写顺序 = `eTradingItem` 0–104** | [确认] | 上两条 |
| 种类顺序与百科分组一致，共 19 组 | [推断] | 分组数 = `NUM_TRADINGITEMKIND`；短名未在 mesfile 单独成表 |
| `CTradingItem` / `CTradingItemElement` 是运行时对象，不是目录 | [确认] | `srcdata/source/game/trade/CTradingItem.cpp`；world 侧 `CTradingItemElement` 扫 0x69 件 |

禁止用 0 当空格：0 是小麦。

## 3. 实体与 ID

| 实体 | 原版枚举 | 原版计数 | 非法 | 空哨兵 | 重构 key |
| --- | --- | --- | --- | --- | --- |
| 交易品 | `eTradingItem` | 105 | -1 | — | `trading_item.<alias>` |
| 种类 | `eTradingItemKind` | 19 | -1 | — | `trading_item_kind.<alias>` |
| 空货格 | 非枚举成员 | 106 | — | `0x6A` | `trading_item.empty` |
| 交易品等级 | `TRADINGITEMGRADE_ID` | [未知] | — | — | `trading_item_grade` |

运行时以表长循环，不写死 105。原版包实例计数是 105 / 19。

模组追加交易品：`id >= 1000`，或独立命名空间。不要占用 106。

### 3.1 种类 0–18 `[推断]`

短名取百科分组 + HD 中文攻略常用译；mesfile 没有 19 条种类标题。引擎只依赖 `id` 与 `item.kind_id`。

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

`name` 能在 mesfile 百科里直接读到的标 `[确认]`；只能靠描述 + 特产品交叉还原短名的标 `[推断]`。`key` 是稳定英文别名，不是原版符号。

| id | key | name | kind | 名称标记 | mesfile 依据（摘） |
| --- | --- | --- | --- | --- | --- |
| 0 | wheat | 小麦 | 0 | [确认] | 「与米（稻）﹑玉米并称为世界三大谷物。原产于地中海」 |
| 1 | rice | 米 | 0 | [确认] | 「去掉稻壳的稻的果实」 |
| 2 | corn | 玉米 | 0 | [确认] | 「与小麦﹑米（稻）并称为世界三大谷物」 |
| 3 | potato | 马铃薯 | 0 | [推断] | 「地下茎的块茎…原产于新大陆」；里加/雅加达槽 id=3 |
| 4 | soybean | 大豆 | 0 | [确认] | 「大豆的种子，是豆腐﹑味噌的原料」 |
| 5 | fish | 鱼肉 | 0 | [确认] | 「鱼，生吃或者烤着吃」；汉堡槽 id=5 |
| 6 | beef | 牛肉 | 0 | [确认] | 「牛肉，可煮或烤食用」；布里斯托尔槽 id=6 |
| 7 | pumpkin | 南瓜 | 0 | [推断] | 「蔬菜的一种，原产于新大陆。富含淀粉」；汉堡槽 id=7 |
| 8 | cheese | 乳酪 | 0 | [推断] | 「牛奶和山羊奶…乳酸菌发酵」；阿姆斯特丹槽 id=8 |
| 9 | tomato | 番茄 | 0 | [确认] | 「茄科的植物，别名红茄子」 |
| 10 | banana | 香蕉 | 0 | [推断] | 「芭蕉科的大型多年生草本植物」；德尔纳特槽 id=10 |
| 11 | sugar | 砂糖 | 1 | [确认] | 「甜味料，主要成份是蔗糖」；哈瓦那槽 id=11 |
| 12 | honey | 蜂蜜 | 1 | [确认] | 「蜜蜂采自花，贮藏在蜂巢里的蜜」 |
| 13 | salt | 盐 | 1 | [确认] | 「调味料的一种。也是人体生理上不可欠缺的成份」 |
| 14 | olive_oil | 橄榄油 | 1 | [确认] | 「地中海沿岸橄榄树的果实中提炼出来的油」 |
| 15 | coconut_oil | 椰子油 | 1 | [确认] | 「用椰子果实的胚乳榨的油，别名椰子油」 |
| 16 | ginger | 生姜 | 1 | [推断] | 「根茎可以食用和药用…独特的芳香和辣味」；马六甲槽 id=16 |
| 17 | lychee | 荔枝 | 2 | [推断] | 「果实呈白色胶状，味道酸甜，原产于中国南方」 |
| 18 | hazelnut | 榛子 | 2 | [确认] | 「别名榛果」；汉城槽 id=18 |
| 19 | raisin | 葡萄干 | 2 | [确认] | 「晒干的葡萄」 |
| 20 | caviar | 鱼子酱 | 2 | [推断] | 「鲟鱼的卵巢腌制而成，是珍品」；伊斯坦布尔槽 id=20 |
| 21 | shark_fin | 鱼翅 | 2 | [推断] | 「大型鲨鱼的鳍干燥而成」 |
| 22 | pepper | 胡椒 | 3 | [确认] | 「胡椒科的果实…辛香料」；亚历山大/德尔纳特槽 id=22 |
| 23 | clove | 丁香 | 3 | [确认] | 「蒲桃科丁香的花蕾」 |
| 24 | nutmeg | 豆蔻 | 3 | [确认] | 「肉豆蔻科…果实的核」；卡利卡特槽 id=24 |
| 25 | cinnamon | 肉桂 | 3 | [确认] | 「肉桂的树皮等干燥而成」；马六甲槽 id=25 |
| 26 | allspice | 甜椒 | 3 | [推断] | 文案是「豆蔻的种子磨成粉末…丁香﹑肉桂﹑肉豆蔻混合的香味」（多香果）；委拉克鲁斯槽 id=26。SC 短名取「甜椒」 |
| 27 | tamarind | 罗望子 | 3 | [推断] | 「豆科的常绿乔木的果实…辛香料」 |
| 28 | saffron | 藏红花 | 3 | [确认] | 「番红花的雄蕊干燥而成」；里斯本槽 id=28 |
| 29 | vanilla | 香草 | 3 | [确认] | 「香草的果实发酵而成，原产于新大陆」 |
| 30 | chili | 辣椒 | 3 | [确认] | 「辣椒的果实干燥而成…别名红辣椒」 |
| 31 | coffee | 咖啡 | 4 | [确认] | 「将咖啡树的种子（咖啡豆）炒熟」 |
| 32 | tea | 茶 | 4 | [确认] | 「用茶树的嫩芽﹑嫩叶制作的饮料」 |
| 33 | cacao | 可可 | 4 | [确认] | 「可可豆…是可可和巧克力的原料」 |
| 34 | tobacco | 烟草 | 4 | [确认] | 「将烟草的叶子干燥，剪细，点火吸其烟」 |
| 35 | wine | 葡萄酒 | 5 | [确认] | 「以葡萄的果实和果汁为原料发酵而成」 |
| 36 | brandy | 白兰地 | 5 | [确认] | 「蒸馏葡萄酒制作的白兰地」；汉堡槽 id=36 |
| 37 | rum | 朗姆酒 | 5 | [推断] | 「以甘蔗的榨汁或糖蜜为原料」；哈瓦那槽 id=37 |
| 38 | sake | 清酒 | 5 | [确认] | 「日本固有的酿造酒…也称为日本酒」；大阪槽 id=38 |
| 39 | gold | 金 | 6 | [确认] | 「有美丽的黄色光泽的金属」 |
| 40 | silver | 银 | 6 | [推断] | 「有青白色美丽光泽的金属」；吕贝克槽 id=40 |
| 41 | platinum | 白金 | 6 | [确认] | 「别名白金」；索法拉槽 id=41 |
| 42 | glassware | 玻璃 | 7 | [确认] | 「玻璃工艺品」；阿姆斯特丹槽 id=42 |
| 43 | ceramics | 陶瓷器 | 7 | [确认] | 「陶器﹑瓷器等陶瓷的总称」 |
| 44 | lacquerware | 漆器 | 7 | [确认] | 「用漆树的树液涂的器具，也叫漆器」；大阪槽 id=44 |
| 45 | inlay | 象牙制品 | 7 | [推断] | 文案是镶嵌工艺；大阪槽 id=45。SC 短名取「象牙制品」 |
| 46 | goldwork | 黄金艺品 | 7 | [推断] | 「雕刻金属的大师傅用铁槌錾打」 |
| 47 | wool | 羊毛 | 8 | [确认] | 「从羊或山羊身上剪下的毛」；伦敦槽 id=47 |
| 48 | cotton | 棉花 | 8 | [确认] | 「棉花生产成的纤维」 |
| 49 | silk | 丝绸 | 8 | [推断] | 「以蚕茧中抽出的纤维」；热那亚/大阪槽 id=49 |
| 50 | hemp | 麻 | 8 | [确认] | 「从泰麻﹑亚麻﹑马尼拉麻等的茎提炼的纤维」；布鲁日槽 id=50 |
| 51 | woolen | 毛织品 | 9 | [确认] | 「用绵羊﹑安哥拉兔﹑开司米等兽毛加工…纺织物」；伦敦槽 id=51 |
| 52 | cotton_cloth | 棉织品 | 9 | [确认] | 「以绵线为原料的纺织物」；塞维利亚槽 id=52 |
| 53 | silk_cloth | 丝织品 | 9 | [确认] | 「用丝线制成的纺织物」 |
| 54 | linen | 麻织品 | 9 | [确认] | 「用麻的纤维制成的纺织物」；布鲁日槽 id=54 |
| 55 | pile_cloth | 绒织品 | 9 | [推断] | 「作垫子用的毛织物…波斯地毯」；伊斯坦布尔槽 id=55 |
| 56 | muslin | 纱布 | 9 | [推断] | 「将人物﹑乌兽﹑花草等图案染上各种颜色而成的绵布」；卡利卡特槽 id=56 |
| 57 | velvet | 天鹅绒 | 9 | [推断] | 「绢的纵丝上起毛的纺织品」；塞维利亚槽 id=57 |
| 58 | antique | 古董 | 10 | [确认] | 「数量稀少，有美术价值的古美术品」 |
| 59 | fur | 毛皮 | 10 | [推断] | 「剥下哺乳动物的皮，连着毛制成的东西」 |
| 60 | painting | 绘画 | 10 | [推断] | 「平面上画出的视觉表现」；雅典槽 id=60 |
| 61 | sculpture | 雕刻 | 10 | [推断] | 「立体表现形像的造型艺术」；雅典槽 id=61 |
| 62 | musk | 麝香 | 11 | [确认] | 「从…麝的分泌物中提取的」；杭州槽 id=62 |
| 63 | frankincense | 乳香 | 11 | [确认] | 「焚香料，点着即冒出白烟」；索法拉槽 id=63 |
| 64 | ambergris | 龙涎香 | 11 | [推断] | 「从抹香鲸的肠内提取」 |
| 65 | sandalwood | 白檀 | 11 | [确认] | 「被制成白檀油，原产于印度」 |
| 66 | jadeite | 翡翠 | 12 | [确认] | 「别名硬玉，暗绿色」；杭州槽 id=66 |
| 67 | ivory | 象牙 | 12 | [确认] | 「象的门牙」；圣乔治槽 id=67 |
| 68 | coral | 珊瑚 | 12 | [确认] | 「将海中生物珊瑚加工而成」 |
| 69 | tortoiseshell | 玳瑁甲 | 12 | [确认] | 「海龟的一种玳瑁的甲壳」 |
| 70 | amber | 琥珀 | 12 | [推断] | 「古代的树脂在地下形成的化石」 |
| 71 | crystal | 水晶 | 12 | [确认] | 「酸化矿物石英…紫水晶﹑烟水晶﹑红水晶」 |
| 72 | nephrite | 软玉 | 12 | [确认] | 「别名软玉」；卡利卡特槽 id=72 |
| 73 | pearl | 珍珠 | 13 | [确认] | 「在珍珠贝等的体内形成的圆珠」；大阪槽 id=73 |
| 74 | diamond | 金刚石 | 13 | [推断] | 「无色透明的宝石，折光率高」 |
| 75 | sapphire | 蓝宝石 | 13 | [确认] | 「别名青玉，青宝玉」 |
| 76 | emerald | 绿宝石 | 13 | [确认] | 「鲜艳的绿色宝石，日本叫作翠玉」 |
| 77 | ruby | 红宝石 | 13 | [推断] | 「红色的宝石，日本叫作和玉」 |
| 78 | lapis | 天青石 | 13 | [确认] | 「别名青金石，日本也叫作琉璃」 |
| 79 | rhinoceros_horn | 犀牛角 | 14 | [确认] | 「以犀牛角的尖端磨碎而成」 |
| 80 | ginseng | 人参 | 14 | [确认] | 「将朝鲜人参树的根干燥而成」；汉城槽 id=80 |
| 81 | quinine | 奎宁 | 14 | [推断] | 「用奎树的皮精制而成，用于解热」；雅加达槽 id=81 |
| 82 | apricot_kernel | 杏仁 | 14 | [确认] | 「杏树果实的核」；里斯本槽 id=82 |
| 83 | cordyceps | 冬虫夏草 | 14 | [推断] | 「寄生在昆虫等的菌类，被当作强壮剂」；杭州槽 id=83 |
| 84 | turmeric | 郁金 | 15 | [确认] | 「将郁金的根茎加热提炼出的液体」；雅加达槽 id=84 |
| 85 | indigo | 靛蓝 | 15 | [确认] | 「蓝的叶子发酵制成的染料」 |
| 86 | cochineal | 胭脂红 | 15 | [确认] | 「将…胭脂虫干燥后制成的粉末」 |
| 87 | catechu | 槟榔 | 15 | [确认] | 「煮槟榔的果实制成」；马六甲槽 id=87 |
| 88 | rhubarb | 大黄 | 15 | [确认] | 「用大黄的根茎制成的黄色染料」 |
| 89 | mangosteen | 山竹果 | 15 | [确认] | 「用山竹果的皮制成的染料」；马六甲槽 id=89 |
| 90 | musket | 洋枪 | 16 | [推断] | 「以火药之力发射子弹的武器」；伦敦槽 id=90 |
| 91 | cannon | 大炮 | 16 | [推断] | 「发射大子弹的兵器」；伦敦槽 id=91 |
| 92 | sword | 刀剑 | 16 | [确认] | 「刀和剑，战斗期间销售良好」；布里斯托尔槽 id=92 |
| 93 | armor | 甲胄 | 16 | [确认] | 「铠甲和头盔，战斗期间卖得很好」 |
| 94 | iron_ore | 铁矿石 | 17 | [确认] | 「含有铁的石头」；汉堡槽 id=94 |
| 95 | copper_ore | 铜矿石 | 17 | [确认] | 「含有铜的石头」 |
| 96 | tin_ore | 锡矿石 | 17 | [确认] | 「含有锡的矿石」；布里斯托尔槽 id=96 |
| 97 | saltpeter | 硝石 | 17 | [推断] | 「火药﹑火柴﹑焰火的原料」；巴士拉槽 id=97 |
| 98 | sulfur | 硫磺 | 17 | [推断] | 「黄色的非金属固体…一点火就发出蓝色火焰」；伊斯坦布尔槽 id=98 |
| 99 | coal | 煤 | 17 | [推断] | 「古代的植物堆积在地里…变成可燃物」；伦敦槽 id=99 |
| 100 | lumber | 木材 | 18 | [确认] | 「砍伐树木，加工成适当的尺寸」；斯德哥尔摩槽 id=100 |
| 101 | leather | 皮草 | 18 | [确认] | 「皮革制品，除去皮上的毛制成的革」 |
| 102 | wax | 蜡 | 18 | [确认] | 「构成蜜蜂窝的蜡，别名蜜蜡」 |
| 103 | gum | 糖胶 | 18 | [推断] | 「橡胶的原料，煮橡胶树的树汁而成」；马六甲槽 id=103 |
| 104 | guano | 鸟粪石 | 18 | [确认] | 「别名鸟粪石」 |
| 106 | empty | （空） | — | [确认] | 哨兵，不是目录行 |

`name_i18n` 进 `config/ui/strings.*.yaml`，本表只留 `name` 作原版包缺省中文。

## 4. 算法 / 公式

本篇无玩法公式。目录是查表：

```
kind_id(item) = trading_items[item_id].kind_id
is_empty(item_id) = item_id == caps.trading_item_empty
is_catalog(item_id) = 0 <= item_id < len(trading_items)
```

`data_price` 的使用见 `02-market-price.md` 的 `quote()`。原版世界表特产品槽里的价格整数（100–6000）按 `TRADINGITEM_DATAPRICE` 解释，见 `04`。

## 5. 耦合分析

原版硬编码：

- `CommitDeal` 栈上 `anSellTradingItemNum[105]`、`anBuyTradingItemKindNum[19]`
- 空值同时用 `0x6A` 与 hold `+2=0x62`（清空记录）
- 港口特产品、流行品、委托扣货都直接拿 `eTradingItem`
- `CTradingItemElement` 对象数组长度 `0x69`，末槽写 `0x6A`

重构后别人只许要 `trading_item_id` / `trading_item_kind_id`。

## 6. 解耦后的配置

```yaml
# config/economy/trading_item_kinds.yaml
schema: dk4.economy.trading_item_kinds
version: 1
kinds:
  - id: 0
    key: food
    name: 食品
  - id: 3
    key: spice
    name: 香辣料
  # ... 共 19 条，原版包
```

```yaml
# config/economy/trading_items.yaml
schema: dk4.economy.trading_items
version: 1
empty_id: 106
items:
  - id: 0
    key: wheat
    name: 小麦
    kind_id: 0
    data_price: 0          # 目录缺省；港口特产品价在 special_products
    encyclopedia_key: mes.trade.wheat
  - id: 22
    key: pepper
    name: 胡椒
    kind_id: 3
    data_price: 0
    encyclopedia_key: mes.trade.pepper
```

```yaml
# config/ids/ 由内容表生成，不手写
trading_item:
  wheat: 0
  empty: 106
trading_item_kind:
  food: 0
  spice: 3
```

`data_price` 在原版更常出现在**港口特产品槽**而不是全局目录。目录字段保留为缺省；港口覆盖见 `04`。未从独立「交易品定义表」还原的全局 `data_price` 标 `[未知]`，不要填编造值。

## 7. 独立旋钮

改这些不应迫使航海 / 战斗改代码：

- 加种类、加交易品（表长变了，循环跟表）
- 改某件的 `kind_id`（市价按种类存，改归属会换那条 kind-rate）
- 改显示名 / 百科
- 把空哨兵从 106 改成别的（只动 `caps.yaml` 与内容包，引擎读配置）

## 8. 对外接口

查询（只读）：

```
Economy.item(trading_item_id) -> { id, key, kind_id, data_price }
Economy.kind(trading_item_kind_id) -> { id, key }
Economy.is_empty(trading_item_id) -> bool
Economy.catalog_size() -> int
```

不暴露 `CTradingItem*`、mesfile 偏移、世界表 hex。

事件：本篇不发事件。货物进出由 `03` 发 `CargoCommitted`。

## 9. 未核实字段

| 字段 | 现状 |
| --- | --- |
| 全局「交易品定义」除百科外的二进制表 | 未找到与 `GetFixTownProc` 同级的 GetFixTradingItem；目录从 mesfile + 特产品槽还原 |
| 每件全局 `data_price` | 只在港口槽看到 100–6000；是否另有底表 `[未知]` |
| `TRADINGITEMGRADE_MIN/MAX` 的整数、与 UI「%s等级」的对应 | 符号有，值无 |
| `TRADINGITEMSTATE_BITS` | 只有符号 |
| 种类短名的原版字符串表 | mesfile 无独立 19 条标题；上表种类名为 `[推断]` |
| id 26 短名「甜椒」vs 文案「多香果」 | 描述与 SC 短名不一致，保留短名并注明 |
| id 45 / 55 短名 | 文案是镶嵌 / 地毯，SC 短名取象牙制品 / 绒织品 `[推断]` |
| hold `+1/+2` 与目录哪一列对应 | `03` 再列；目录不解释舱位字节 |
| World1 交易品是否同一 105 件 | 未对照 World1.dll |
