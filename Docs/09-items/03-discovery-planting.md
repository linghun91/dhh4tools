# 发现物种植、村落赠礼与神秘狩猎

配置落点：`config/items/planting.yaml` `config/items/hunting.yaml`。目录与种类见 [01-item-catalog.md](01-item-catalog.md)；装备效果见 [02-equipment-effects.md](02-equipment-effects.md)。

本篇管三件事：**把种子/苗/工艺书交给港口变成特产品**、**在村落送 kind=7 赠礼**、**神秘狩猎配方**。藏宝图 / 地点地图 / 遗迹对照表不在这里，见 [01-world/03-villages-discovery.md](../01-world/03-villages-discovery.md)。

`eItem` 0–19 是种植发现物（kind 0）。0–21 在 mesfile 里连成同一段「可以拿去某处」的口味文案，但 20–21 是航海护佑（kind 1），**不会**解锁特产品。

## 1. 职责边界

管：

- kind `plant_discovery`（原版包 id 0–19）→ `trading_item_id` 的声明
- 广场「如果有那%s，%s会成为这个城市的特产品」这条命令怎么发
- kind `gift`（原版包 id 127–147）在村落的赠礼偏好（区域文案）
- `CItemMysteryHunting` 只读的配方表（碎片拼合）
- 上述流程消耗唯一物、发事件；真正改特产品槽交给 economy

不管：

- 特产品 5 槽供给、买卖、品牌、投资（[04-economy/04-special-products-investment.md](../04-economy/04-special-products-investment.md)）
- 港口坐标、默认 5 槽里已经写死的货（world）
- 村落/遗迹坐标与发现位（world）；海上搜索圈（ocean）
- 地点地图 155–173、藏宝图 191、霸者之证解谜（world / remains 目录）
- 航海护佑 20–23 怎么挡老鼠、坏血病、暴风雨（ocean 读 `voyage_charm` 效果）
- 广场 UI、选物窗绘制（`CPlazaView`、`CSelectItemView` 是 View）

拆分：items 只说「这件 `item_id` 种下去会变成哪件 `trading_item_id`」。economy 听 `PlantSpecialProduct` 后找空槽写入。禁止 items 直接改 `CTown` 特产品字节。

## 2. 原版事实

### 2.1 发现物 0–21 与种类

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `NUM_ITEM=231`，id 0 南瓜种子、1 番茄苗 | [确认] | `ItemID.h`，`GetFixItemProc` |
| 记录 `+0x14`：0–19 为 0，20–21 为 1 | [推断] | `World0_item.c` / `items_world0.json`；与 `CItemKindMenuData` 13 档对齐，见 01 |
| kind 0 共 20 件，名称都是种子/苗/树/窝/工艺书 | [确认] 名称；[推断] 种类名 `plant_discovery` | `items.tsv` |
| kind 1 共 4 件：金黄色的猫、莱姆的滴汁、赫尔墨斯的祈祷、华陀的中药 | [确认] 名称；[推断] `voyage_charm` | 同上，id 20–23 |
| 空货格哨兵是交易品 106，不是南瓜种子 | [确认] | `ITEM_EMPTY`；见 01、04 |

`15-catalogs/items.md` 把 0–23 粗分成「seed」。那是目录分组，不是 `eItemKind`。本系统只把 **0–19** 当种植行。

### 2.2 世界表 hex（种植行）

`SItem` 步长 `0x20`。字段名见 01 第 9 节。本篇只用到种植相关观察。

World0 原版包，id 0–21：

| id | name | kind `+0x14` | `base_price` `+0x08` | `+0x18` | `+0x18` 解作 `eTown` | `+0x0C/+0x10` |
| ---: | --- | ---: | ---: | ---: | --- | --- |
| 0 | 南瓜种子 | 0 | 2000 | 98 | 哨兵 `NUM_TOWN+1` | 皆 -1 |
| 1 | 番茄苗 | 0 | 2200 | 98 | 哨兵 | 皆 -1 |
| 2 | 香蕉树 | 0 | 2400 | 50 | 雅加达 | 皆 -1 |
| 3 | 蜜蜂窝 | 0 | 1200 | 34 | 索法拉 | 皆 -1 |
| 4 | 小鲨鱼 | 0 | 9000 | 98 | 哨兵 | 皆 -1 |
| 5 | 胡椒粒 | 0 | 8500 | 42 | 卡利卡特 | 皆 -1 |
| 6 | 丁香粒 | 0 | 8600 | 50 | 雅加达 | 皆 -1 |
| 7 | 肉桂树 | 0 | 7800 | 98 | 哨兵 | `0x79f8` / `0x2588` |
| 8 | 甜椒果 | 0 | 6500 | 61 | 哈瓦那 | 皆 -1 |
| 9 | 咖啡树 | 0 | 6700 | 98 | 哨兵 | `0x5f68` / `0x2b48` |
| 10 | 茶树 | 0 | 7500 | 98 | 哨兵 | `0x8268` / `0x1c88` |
| 11 | 可可果 | 0 | 6800 | 65 | 委拉克鲁斯 | 皆 -1 |
| 12 | 烟草苗 | 0 | 7200 | 61 | 哈瓦那 | 皆 -1 |
| 13 | 炼金术书 | 0 | 12500 | 98 | 哨兵；`+0x19=10` | 皆 -1 |
| 14 | 玻璃的制作辞典 | 0 | 8000 | 98 | 哨兵 | 皆 -1 |
| 15 | 蚕 | 0 | 7700 | 57 | 汉城 | 皆 -1 |
| 16 | 织布机 | 0 | 11000 | 0 | 伦敦 | 皆 -1 |
| 17 | 珍珠贝 | 0 | 9000 | 47 | 马六甲 | 皆 -1 |
| 18 | 中医宝典 | 0 | 12000 | 98 | 哨兵 | 皆 -1 |
| 19 | 山竹果种子 | 0 | 3000 | 47 | 马六甲 | 皆 -1 |
| 20 | 金黄色的猫 | 1 | 15000 | 98 | 哨兵 | `0x5d88` / `0x1cd8` |
| 21 | 莱姆的滴汁 | 1 | 10000 | 98 | 哨兵 | `0x50e8` / `0x2708` |

`+0x18=98` 当作「未绑港口」`[推断]`：`NUM_TOWN=97`，合法港 0–96。非 98 的值都落在 0–96。**禁止**把这个字节当成种植目标港：卡利卡特默认特产品里没有胡椒（22），委拉克鲁斯默认没有可可（33）。它更像发现物自己的关联港。

kind 0 的 `+0x16` 一律 `0x18=24`，`+0x17=0`（与武器/防具的效果量不同）。`+0x15` 在 `+0x0C==-1` 时多为 8，肉桂/咖啡/茶例外。语义 `[未知]`。

World1 同 231 名，种植行 `+0x18` **大量不同**（01 已记 126 条 rest 有差）。World1 抽样：

| id | World0 `+0x18` | World1 `+0x18` |
| ---: | --- | --- |
| 0 南瓜 | 98 | 65 委拉克鲁斯 |
| 2 香蕉 | 50 雅加达 | 98 |
| 3 蜜蜂窝 | 34 索法拉 | 98 |
| 5 胡椒 | 42 卡利卡特 | 47 马六甲 |
| 7 肉桂 | 98 | 42 卡利卡特 |
| 9 咖啡 | 98 | 38 巴士拉 |
| 10 茶 | 98 | 54 杭州 |
| 11 可可 | 65 委拉克鲁斯 | 98 |
| 14 玻璃辞典 | 98 | 2 阿姆斯特丹 |
| 15 蚕 | 57 汉城 | 98 |
| 16 织布机 | 0 伦敦 | 2 阿姆斯特丹 |
| 18 中医宝典 | 98 | 57 汉城 |
| 20 金猫 | 98 | 42 卡利卡特 |

两包同一 schema，用内容包覆盖，引擎禁止 `if world==1`。

### 2.3 广场：发现物 → 特产品

mesfile SC `0x7737` `[确认]`：

> 你！不把那东西让给我吗！？如果有那%s，%s会成为这个城市的特产品……，怎么样？真的！你是好人，最好的人。

拒绝 `0x779f`：「是吗，你真小气。」上下文是广场人群（同段有阿科布、流行、投资），不是海上探索。

两个 `%s`：第一件是手上的发现物（`eItem` 名），第二件是将出现的交易品（`eTradingItem` 名）`[推断]`。没有还原出填槽函数，经济侧只承认「收到解锁槽命令」。

世界表 97 港 × 5 槽里，空槽 `item_id=106` 共 45 个 `[确认]` `towns_world0.json`。攻略所谓「原产物导入才出现的特产品」与这 45 个空位同一数量级，但是否一一对应 `[未知]`。

鲨鱼幼鱼：ocean 文案「得到了鲨鱼的幼鱼」「也许会成为新的交易品」（`0x4A0F`、`0x11473`）。那是获得 id 4 的途径，不是种植公式。目录里已有交易品鱼翅（21）。

### 2.4 种植口味文案（气候 / 区域）

道具说明 0–21 在 `0x19783` 起。紧挨着、按同一顺序的「拿到某处去」句在 `0x1ccc3` 末尾～`0x1d08f` `[推断]`：条数与 0–21 对齐，插在爱情灵药说明之后、装备用法之前。

| id | 口味（摘） | 文案指向的区域 |
| ---: | --- | --- |
| 0 | 北海周围也很有可能可以生长 | 北海 |
| 1 | 拿到地中海附近，好像也会长得很好 | 地中海 |
| 2 | 有可能生长于新大陆或非洲或的热带地区 | 新大陆 / 非洲 / 热带 |
| 3 | 用地中海盛开的花制成的蜂蜜一定也很好吃 | 地中海 |
| 4 | 中国吃鲨鱼的鳍，地中海深处的黑海周围则吃鲨鱼的卵 | 东亚 / 黑海 |
| 5 | 东南亚的气候最适合胡椒生长 | 东南亚 |
| 6 | 东南亚……适合丁香；地中海……也许很难 | 东南亚（地中海难） |
| 7 | 如果地中海能生产肉桂就赚大钱了 | 地中海 |
| 8 | 在非洲能将这种香料当商品就好了 | 非洲 |
| 9 | 新大陆一定适合生产咖啡 | 新大陆 |
| 10 | 非洲附近好像也能生产茶 | 非洲 |
| 11 | 只生长在热带，带到非洲试种 | 热带 → 非洲 |
| 12 | 要是能在地中海种植、销售就好了 | 地中海 |
| 13 | 非洲的矿石也许能成为新的产物 | 非洲 |
| 14 | 在非洲和东亚，玻璃是贵重的东西 | 非洲 / 东亚 |
| 15 | 丝绸过去是东亚的主力商品 | 东亚 |
| 16 | 北海和地中海的纺织产业还有发展的空间 | 北海 / 地中海 |
| 17 | 东亚好像也能采集珍珠 | 东亚 |
| 18 | 听说中国有许多名贵的中药 | 中国 / 东亚 |
| 19 | 东南亚有很多稀有的水果，不过这皮可以提炼染料 | 东南亚 |
| 20 | 有老鼠作乱时，一定能发挥很大的作用 | （护佑，不是特产品） |
| 21 | 为防备航海时出现坏血病，一定要有这种东西 | （护佑，不是特产品） |

气候文档已写明：港口作物闲聊**不进**气候逻辑表。这些句子是种植内容的区域标签，不是 `CClimate` 采样。

### 2.5 发现物对应哪件交易品

`SItem` 里**没有** `eTradingItem` 字段 `[确认]` 布局。对照只能靠名称 + 百科 + 口味 `[推断]`：

| eItem | 交易品 id | key | 依据 |
| ---: | ---: | --- | --- |
| 0 南瓜种子 | 7 | pumpkin | 名称；汉堡槽已有 7 |
| 1 番茄苗 | 9 | tomato | 名称；百科「红茄子」 |
| 2 香蕉树 | 10 | banana | 名称 |
| 3 蜜蜂窝 | 12 | honey | 说明写蜂蜜 |
| 4 小鲨鱼 | 21 | shark_fin | 海洋「新交易品」+ 百科鱼翅 |
| 5 胡椒粒 | 22 | pepper | 名称 |
| 6 丁香粒 | 23 | clove | 名称 |
| 7 肉桂树 | 25 | cinnamon | 名称 |
| 8 甜椒果 | 26 | allspice | SC 短名「甜椒」；备选辣椒 30，未钉 |
| 9 咖啡树 | 31 | coffee | 名称 |
| 10 茶树 | 32 | tea | 名称 |
| 11 可可果 | 33 | cacao | 名称 |
| 12 烟草苗 | 34 | tobacco | 名称 |
| 13 炼金术书 | — | — | 「矿石」；具体 `eTradingItem` `[未知]` |
| 14 玻璃辞典 | 42 | glassware | 名称 |
| 15 蚕 | 49 | silk | 说明吐丝；汉城槽有 49 |
| 16 织布机 | — | — | 「纺织」；伦敦/毛织品 51 是候选，未钉 |
| 17 珍珠贝 | 73 | pearl | 名称 |
| 18 中医宝典 | — | — | 「中药」；人参 80 / 冬虫夏草 83 都可能 |
| 19 山竹果种子 | 89 | mangosteen | 名称；马六甲槽有 89 |

未钉的三行 YAML 写 `trading_item_id: null`。禁止为填表编造。

### 2.6 村落赠礼

kind 7 赠礼 21 件，id 127–147，不进人物槽 `[推断]` 种类，见 01。

村落到访文案（粮食、水、修船、募集村民、谢礼金币）属于 ocean / world，本篇不拥有。本篇只拥有：**把 kind=7 的 `eItem` 交给村落里的人**。

选物对白 `0x84cf` `[确认]`：

> 嗯，这个吗？这是“%s”。……喔，太棒了！想要吗？喜欢吗？……难道……要给我？真的！？太开心了！谢谢，我一定会好好珍惜！

回绝 `0x859f`：「是吗……，对不起，我太随便了。」随后可接位置提示 `0x85bf`「详细的位置是……对了，在%s附近。」这条链出现在酒馆/广场情报段，是否与村落共用同一套 `CSelectItemView` `[未知]`。

赠礼说明 `0x1dd83`～`0x1e247` 按 127–147 的目录顺序写「哪一带的人喜欢」`[推断]`：

| id | name | 口味区域 |
| ---: | --- | --- |
| 127 | 冰冻的玫瑰 | 北海女性 |
| 128 | 彩虹玻璃珠 | 航海技术发达的国家 |
| 129 | 沙金 | 北方女性迷恋黄金 |
| 130 | 制作面包的石臼 | 喜欢做菜的北欧女性；说明另写「劳动妇女最棒的礼物」`0x1b72b` |
| 131 | 徽宗的北宋画 | 西欧珍惜东洋艺术品 |
| 132 | 仙女的彩绸 | 欧洲人眼中的阿拉伯服装 |
| 133 | 萨佛诗集 | 地中海 / 古希腊 |
| 134 | 惊叹之壶 | 地中海古代遗址 |
| 135 | 米罗的维纳斯 | 地中海沿岸 |
| 136 | 彩色玻璃小花 | 地中海彩色玻璃 |
| 137 | 新罗的金冠 | 中国遇到的女孩想要 |
| 138 | 陶瓷耳环 | 印度洋酒馆学舞的女孩 |
| 139 | 翡翠大珠 | 东非、东洋船队当吉祥物 |
| 140 | 誓言戒指的剧本 | 东南亚喜印度古典 |
| 141 | 正仓院的水壶 | 波斯→日本 / 亚洲 |
| 142 | 黑琉璃碗 | 西洋人难体会（东亚向） |
| 143 | 大花朵的剌绣毛毯 | 东亚地毯 |
| 144 | 画细雪的长袍 | 东洋人喜欢 |
| 145 | 高丽青瓷香炉 | 西非陶器 |
| 146 | 高级放大镜 | 新大陆难得到欧洲器具 |
| 147 | 七色鹦鹉 | 新大陆妇女喜欢动物 |

World0 若干赠礼 `+0x18` 也是港 id（玻璃珠→阿姆斯特丹 2，石臼→亚历山大 25，萨佛→雅典 19，彩色玻璃→热那亚 15，黑琉璃碗→汉城 57，毛毯→巴士拉 38，长袍/放大镜→斯德哥尔摩 7）。与口味区域大体同向，仍禁止把 `+0x18` 写成 `village_id`（村落 24 个，港 97 个）。

成功之后给发现物、给坐标提示、还是只加友好：函数未还原 `[未知]`。

### 2.7 神秘狩猎

`CItemMysteryHunting` `[确认]`：

- 文件 `srcdata/source/game/item/CItemMysteryHunting.cpp`，与 `CItemInformationView` / `CSelectItemView` 同目录
- 继承 `CSurfaceSpriteView`（`class_hierarchy.txt`）
- 析构 `fn_14000750e(..., 0x320)` → 对象体积 **0x320**
- 主构造 body `0x1401296a0` 把 `+0xb0`～`+0x1ec` 写成一长串立即数（`0xbf,0xae,0x1cc,0x1f2…`），与 `CInvestmentPadView` 同类：**UI 矩形，不是配方**
- `+0x208` 是子对象指针；`vfunc_02` 非空则虚调用 `+0x10` 再销毁、置 0
- 其余 vfunc 多为 16 字节 thunk，拼合判定**没有**从这些方法还原

kind 6 碎片 14 件，id 174–187，标价一律 100，World0 `+0x18=98` `[确认]` hex。

名称成对 `[推断]`，产出未钉死（01 已禁止手填 output）：

| 输入 | mesfile |
| --- | --- |
| 176 石板上半部 + 183 下半部 | `0x1c1d7` / `0x1c2fb` |
| 179 唐代的竹艺品 + 186 组合说明图 | `0x1c21f` / `0x1c383` |
| 180 典礼用的小刀 + 187 太阳图纹的刀鞘 | `0x1c297` / `0x1c383` |

七张贤者地图（kind 9，id 155–161：纸/布/石/叶/货币/竹/刀刃）与若干碎片原料同名 `[推断]` 有关，**不是**引擎规则。产出、是否进狩猎窗、是否和藏宝图共用 `CItemMysteryHunting`：ocean 文档曾写「探索命中藏宝图时打开它」；藏宝图内容归 remains，本表只放碎片配方。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 空 / 非法 | 本系统 |
| --- | --- | --- | --- | --- |
| 种植发现物 | `item_id` kind 0 | 20（0–19） | 0 是南瓜，不是空 | 行在 `planting.yaml` |
| 航海护佑 | `item_id` kind 1 | 4（20–23） | — | **不**进种植表 |
| 赠礼 | `item_id` kind 7 | 21（127–147） | — | `planting.yaml` `gifts` |
| 碎片 | `item_id` kind 6 | 14（174–187） | — | `hunting.yaml` 材料 |
| 特产品槽 | `(town_id, slot 0–4)` | 每港 5 | `trading_item.empty=106` | 不拥有；命令 economy |
| 关联港字节 | `item.+0x18` | 0–96 或 98 | 98 = 未绑 `[推断]` | 内容字段 `anchor_town_id` |
| 村落 | `village_id` | 24 | -1 | 赠礼目标；定义在 world |
| 配方 | `recipe_id` / key | 原版未计数 | — | `hunting.yaml` 表长 |
| 区域标签 | `region_tag` | 文案 7 圈级 | — | 北海…新大陆；不是 `eSeaArea` |

禁止：

- `item_id` 与 `trading_item_id` 混用（南瓜种子 0 ≠ 南瓜 7）
- 用 `+0x18` 当 `village_id` 或 `remains_id`
- 把 20–21 写进种植成功路径
- 把 191 藏宝图写进 `hunting.yaml` 产出

模组可加种植行，id 不覆盖 0–230。引擎按表长，不写死 20。

## 4. 算法 / 公式

魔数进 YAML。原版种植/赠礼/拼合的**精确谓词都未还原**，下面是可配置骨架；标了 `[未知]` 的分支默认 no-op 或只发事件，禁止在 C++ 里补「看起来合理」的判定。

### 4.1 参数

```
# config/items/planting.yaml 与 caps 共用
unbound_town_id          = 98        # +0x18 哨兵，NUM_TOWN+1
special_slots_per_town   = 5         # NUM_SPECIALPRODUCT
empty_trading_item_id    = 106       # ITEM_EMPTY
item_unique_stack        = 1         # 发现物不堆叠
consume_on_plant         = true      # [推断] 交出去才能变特产品
consume_on_gift          = true      # [推断] 「给我」
consume_on_combine       = true      # [推断] 拼合
```

### 4.2 种植

广场 NPC 开口的条件未还原。重构只提供查询，由 town-life / narrative 决定何时播 `0x7737`。

```
function can_plant(town_id, item_id, fleet_id) -> bool:
    row = Planting.get(item_id)
    if row is none: return false                 # 非种植行（含 20–21）
    if not Items.owned(fleet_id, item_id): return false
    if row.trading_item_id is none: return false # 未钉交易品，禁止猜
    slots = Economy.specials(town_id)            # LIVE
    if any slot.item_id == row.trading_item_id: return false   # 已是特产品
    if not any slot.item_id == empty_trading_item_id: return false
    # climate_tags ∩ town.region_tags ：[未知] 是否为硬条件
    # vanilla 先不拦；内容包可开 require_region: true
    return true

function plant(town_id, item_id, fleet_id):
    if not can_plant(...):
        emit PlantRejected { town_id, item_id, reason }
        return                       # 「你真小气」走 UI，不扣物
    Items.DiscardItem { fleet_id, item_id }      # unique → 0
    emit PlantSpecialProduct {
        town_id,
        trading_item_id: row.trading_item_id,
        item_id
    }
    # economy 订阅后：找一个 empty 槽写入 trading_item_id，发 SupplyChanged
```

`anchor_town_id`（`+0x18`）**不**参与 `can_plant`。它只是内容注释 / 百科「原产地」。

气候标签是内容，不是 `CClimate`。`require_region` 默认 false，与气候文档「闲聊不进逻辑表」一致。

### 4.3 村落赠礼

```
function present_gift(village_id, gift_item_id, fleet_id):
    if not World.discovered(village_id): return
    row = Planting.gift(gift_item_id)
    if row is none: return                       # 非 kind 7
    if not Items.owned(fleet_id, gift_item_id): return
    village = World.Village.def(village_id)      # LIVE；已卸载/无定义则 no-op
    liked = region_match(row.liked_region_tags, village.region_tags)
    # liked 只影响文案/报酬表；匹配规则 [未知]
    if consume_on_gift:
        Items.DiscardItem { fleet_id, gift_item_id }
    emit VillageGiftPresented {
        village_id,
        gift_item_id,
        matched: liked
    }
    # 报酬（发现物 / 坐标提示 / 友好）[未知]，由 narrative 或本表 rewards 订阅
```

闭包只捕获 `village_id` + `item_id` + `fleet_id`。执行时 LIVE 取村、取背包；村未发现或人不在则整体 no-op。禁止把 `Village*` / `Player*` 推进 Map。

村落到访的水粮、修船、募集：ocean 已发自己的事件，不走赠礼表。

### 4.4 神秘狩猎

```
function combine(fleet_id, item_ids: [item_id]):
    recipe = Hunting.match(item_ids)             # 无序集合匹配，表驱动
    if recipe is none:
        emit CombineRejected { item_ids, reason: no_recipe }
        return
    if recipe.output_item_id is none:
        emit CombineRejected { item_ids, reason: output_unspecified }
        return                                   # vanilla 多条 output 仍 null
    for id in recipe.inputs:
        if not Items.owned(fleet_id, id): return
    if recipe.consume_inputs:
        for id in recipe.inputs:
            Items.DiscardItem { fleet_id, id }
    Items.AcquireItem { fleet_id, recipe.output_item_id }
    emit ItemsCombined { fleet_id, recipe_key, output_item_id }
```

引擎不得写 `if has(176) and has(183): give ???`。没有 output 的原版包行保持 `null`，UI 可以显示「还缺研究」，不能发明一件遗迹宝物。

`CItemMysteryHunting` 的矩形立即数进 `config/ui/`（若抽），不进 `hunting.yaml`。

## 5. 耦合分析

原版：

- 广场对话、选物、特产品槽、世界表 `+0x18` 搅在一起
- 种植成功直接改港口 5 槽（函数未还原，但 mesfile 与空槽并存）
- 赠礼、情报、位置提示同一段 mesfile
- `CItemMysteryHunting` 既是 View 又可能被探索/港口打开
- World0 / World1 用 DLL 换关联港，逻辑仍是同一份 exe

重构：

```
items.planting.yaml  --PlantSpecialProduct-->  economy.unlock_slot
items.planting.yaml  --VillageGiftPresented-->  narrative / ocean（报酬）
items.hunting.yaml   --ItemsCombined-->         items 库存
town-life plaza      只决定何时问「让给我吗」
world                只提供 town_id / village_id / region_tags
ocean                只负责搜到幼鱼、发现村；不填特产品
remains / 地点地图   不进本表
```

economy 不读 `eItem`。world 不读背包。combat 不出现。

## 6. 解耦后的配置

```yaml
# config/items/planting.yaml
schema: dk4.items.planting
version: 1
pack: vanilla-world0
unbound_town_id: 98
consume_on_plant: true          # [推断]
consume_on_gift: true           # [推断]
require_region: false           # 气候句默认不进判定
region_tags:                    # 文案圈，不是 eSeaArea
  - north_sea
  - mediterranean
  - black_sea
  - africa
  - tropics
  - indian_ocean
  - se_asia
  - east_asia
  - china
  - new_world

plants:
  - item_id: 0
    key: pumpkin_seed
    trading_item_id: 7          # pumpkin [推断] 名称
    anchor_town_id: 98          # World0 +0x18
    region_tags: [north_sea]
    unknown_bytes:
      plus_15: 8
      plus_16: 24
      plus_17: 0
      plus_19: 8

  - item_id: 1
    key: tomato_seedling
    trading_item_id: 9
    anchor_town_id: 98
    region_tags: [mediterranean]

  - item_id: 5
    key: peppercorn
    trading_item_id: 22
    anchor_town_id: 42          # 卡利卡特；不是种植目标
    region_tags: [se_asia]

  - item_id: 8
    key: bell_pepper
    trading_item_id: 26         # allspice；备选 30 chili，见第 9 节
    anchor_town_id: 61          # 哈瓦那
    region_tags: [africa]

  - item_id: 13
    key: alchemy_book
    trading_item_id: null       # 「矿石」未钉到具体 id
    anchor_town_id: 98
    region_tags: [africa]

  - item_id: 16
    key: loom
    trading_item_id: null       # 纺织类未钉
    anchor_town_id: 0           # 伦敦
    region_tags: [north_sea, mediterranean]

  - item_id: 19
    key: mangosteen_seed
    trading_item_id: 89
    anchor_town_id: 47          # 马六甲
    region_tags: [se_asia]

# id 2–4, 6–7, 9–12, 14–15, 17–18 按第 2.5 节填；20–21 禁止出现在 plants

gifts:
  - item_id: 127
    key: frozen_rose
    liked_region_tags: [north_sea]
    anchor_town_id: 98
    rewards: []                 # [未知]
  - item_id: 130
    key: bread_mortar
    liked_region_tags: [north_sea]
    anchor_town_id: 25          # 亚历山大；口味却是北欧 [未知] 对齐
    rewards: []
  - item_id: 147
    key: rainbow_parrot
    liked_region_tags: [new_world]
    anchor_town_id: 98
    rewards: []
# 其余 127–147 按 2.6 表
```

World1 包只覆盖 `anchor_town_id`（及 rest 未命名字节），不换 schema。例：World1 南瓜 `anchor_town_id: 65`。

```yaml
# config/items/hunting.yaml
schema: dk4.items.hunting
version: 1
pack: vanilla-world0
site: item_mystery_hunting      # CItemMysteryHunting；不是海上搜索圈
consume_inputs: true            # [推断]
ui_object_size: 0x320           # 析构立即数；给 UI，不给判定

recipes:
  - key: mystery_slate
    inputs: [176, 183]          # 石板上 / 下
    output_item_id: null        # 未钉，禁止猜
    consume_inputs: true

  - key: bamboo_craft
    inputs: [179, 186]
    output_item_id: null

  - key: sun_sheath
    inputs: [180, 187]
    output_item_id: null

# 其余 8 件碎片不成对名称：不进 recipes，留 unknown_fragments
unknown_fragments: [174, 175, 177, 178, 181, 182, 184, 185]
```

`01` 里的 `config/items/recipes.yaml` 并入本文件，不再单列第三份真相。

地点地图、藏宝图、贤者地图的「揭示遗迹」仍在 `items.yaml` 的 `unlock_remain` / `unlock_village`，由 world 订阅。本文件不写 `remain_id`。

## 7. 独立旋钮

| 旋钮 | 文件 | 改它不必动 |
| --- | --- | --- |
| 种植行、交易品外键 | `planting.yaml` | economy 只收 `trading_item_id` |
| `anchor_town_id` / World1 差 | 内容包 | 判定默认不读它 |
| `require_region`、区域标签 | `planting.yaml` | 气候系统、海域表 |
| `consume_on_plant` / gift / combine | 同上 / hunting | 库存仍走 `DiscardItem` |
| 空槽哨兵、每港槽数 | `caps.yaml` | 与 economy 共用 |
| 赠礼喜欢哪一圈 | `gifts[].liked_region_tags` | 村落坐标 |
| 配方输入/产出 | `hunting.yaml` | UI 只列材料 |
| `CItemMysteryHunting` 矩形 | ui 布局 | 配方表 |

改「胡椒粒对应胡椒」只动一行外键。改「地中海才能种番茄」只开 `require_region` 并填 tag。不要为新作物加 `switch (item_id)`。

## 8. 对外接口

查询：

```
Planting.get(item_id) -> PlantRow | none
Planting.can_plant(town_id, item_id, fleet_id) -> bool
Planting.gift(item_id) -> GiftRow | none
Hunting.match([item_id]) -> Recipe | none
Hunting.recipes() -> [Recipe]
```

`PlantRow = { item_id, trading_item_id | null, anchor_town_id, region_tags }`  
`GiftRow = { item_id, liked_region_tags, anchor_town_id, rewards }`  
`Recipe = { key, inputs, output_item_id | null, consume_inputs }`

命令：

```
PlantSpecial { fleet_id, town_id, item_id }      # 本系统检表、扣物、发事件
PresentVillageGift { fleet_id, village_id, item_id }
CombineItems { fleet_id, item_ids }              # 与 01 相同，改查 hunting.yaml
```

事件：

```
PlantSpecialProduct { town_id, trading_item_id, item_id }
PlantRejected { town_id, item_id, reason }       # not_owned / not_plant / no_empty_slot / already_special / output_unspecified
VillageGiftPresented { village_id, gift_item_id, matched }
ItemsCombined { fleet_id, recipe_key, output_item_id }
CombineRejected { item_ids, reason }
```

economy 订阅 `PlantSpecialProduct` 后：

```
Economy.unlock_special_slot(town_id, trading_item_id)
# 写第一个 empty 槽；没有空槽则发 SupplyUnchanged，items 不负责回滚（事务在命令里先 can_plant）
```

查询 world：`Village.def` / `Village.discovered` / `town.region`。查询 economy：`specials(town_id)`。不反向写。

存档：不另存种植表。状态是「背包有没有这件发现物」+「该港槽里是不是已经有这件交易品」。读档后不必重建派生缓存。

UI 回调只带 `fleet_id` + `town_id`/`village_id` + `item_id`。人已离队、村未发现、港已卸载：no-op。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| 广场开口的完整谓词（是否要空槽、是否看区域、是否看份额） | 仅 mesfile `0x7737` | `[未知]` |
| 种植是否必须消耗发现物 | 文案「让给我」 | `[推断]` true |
| `+0x18` 除关联港以外的用法 | 全表统计 | `[推断]` 港 id / 98；种植判定不用 |
| `+0x15/+0x16/+0x19` 在 kind 0 上的含义 | hex | `[未知]` |
| 炼金术书 / 织布机 / 中医宝典 → 哪件 `eTradingItem` | 只有种类口味 | `[未知]` |
| 甜椒果 = 26 还是 30 | SC 短名 vs 「辣椒的一种」说明 | `[推断]` 先 26 |
| 45 个空槽是否就是种植目标全集 | 数量相近 | `[未知]` |
| World1 `+0x18` 差是否改「原产地」叙事 | 两包 hex | `[确认]` 有差；语义仍按关联港 |
| `0x84cf` 链是否就是村落赠礼 | 选物 +「给我」 | `[推断]` 赠礼 UI；可能与酒馆共用 |
| 赠礼成功报酬 | 无函数 | `[未知]` 禁止编造「必掉南瓜种子」 |
| 石臼 `+0x18=25` 与「北欧女性」口味冲突 | hex vs mesfile | `[未知]` |
| 24 村与 21 赠礼是否有对照表 | 无抽出 | `[未知]` 用区域 tag，不用 `village_id` 手填 |
| `CItemMysteryHunting` 配方判定代码 | 还原 cpp 几乎是布局 | `[未知]` |
| 三对碎片的 `output_item_id` | 只有成对名称 | `[未知]` |
| 贤者地图 × 碎片是否进同一窗 | 名称平行 | `[推断]` 有关；不写进 vanilla 配方 |
| 狩猎窗是否兼开藏宝图 | ocean 文档一句 | 藏宝图内容归 remains |
| ctor `+0xb0` 起立即数的矩形含义 | 布局常数 | `[未知]` UI |
| `+0x208` 子对象类型 | vfunc_02 | `[未知]` |
| `ITEMSTATE_BITS` 是否含「已种植过」 | 只有符号 | `[未知]` 重构用槽内容表达 |

未还原前：YAML 留 `null` / 空 `rewards` / `unknown_bytes`。实现只跑确认项与带注释的推断项。禁止为「表好看」给炼金术书指定硫磺、给石板指定水晶头骨。
