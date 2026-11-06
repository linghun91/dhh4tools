# HD 古宝与霸者之证

配置落点：`config/items/ancient_treasures.yaml` `config/discovery/hegemony_proofs.yaml`。目录与种类见 [01-item-catalog.md](01-item-catalog.md)；装备效果见 [02-equipment-effects.md](02-equipment-effects.md)；发现称号镜头见 [11-progression/02-fame-titles.md](../11-progression/02-fame-titles.md)；遗迹/古地图目录见 [01-world/09-remains-oldmaps-catalog.md](../01-world/09-remains-oldmaps-catalog.md)；七文化圈见 [02-factions/01-forces.md](../02-factions/01-forces.md)。

HD 版**还有**这套收集：七个文化圈各一件「证」，证本身是 `eItem` kind 8 行，位置却是独立的 7 槽状态。mesfile 没有「古宝」二字。exe 筛选名把 kind 8 叫 **霸者之证**。本篇用「古宝」指收集内容包（14 件 kind 8 + 7 张贤者地图 + 生成规则），用「霸者之证七槽」指 `CItemElement+0x39c8` 那 7 字节与 7×8 出生点表。

三套东西不要混：

| 玩家口头 | 原版是什么 | 本系统 |
| --- | --- | --- |
| 古宝 / 遗物 | kind 8 的 14 件 `eItem`（两套 × 7 圈） | `ancient_treasures.yaml` |
| 霸者之证 | 同上 kind 名；另有 **7 槽** 出生点/收集状态 | `hegemony_proofs.yaml` |
| 普通装备 | kind 2/3/4（含 214–230 金币、星粉水晶、普利德温…） | 仍走 `items.yaml` + `effects.yaml`，**不进**本两表 |

127–147 是 kind 7 **礼物**，174–187 是 kind 6 **神秘小物件**，162–173 是 kind 10 **遗址的地图**。15-catalogs 曾把 127–154、174+、192+ 捆成「relic 52 件」，那是目录分组，不是 `eItemKind`。

## 1. 职责边界

管：

- kind 8 十四件收集物的**系列、文化圈、与七槽的绑定**
- 七槽本身：圈 id、World0/World1 各绑哪件 `item_id`、每槽 8 个世界坐标候选、开局如何抽一个
- kind 9 七张贤者地图作为「标有××霸者之证位置」的线索外键
- 海上搜索命中 kind 8 时发哪类事件（文案走 `ui/strings`）
- 上述状态哪些进存档（7 字节 + 231 槽持有位）

不管：

- 231 件的名字、标价、图标（[01](01-item-catalog.md) 的 `items.yaml`）
- 人物五槽、卖价、`+0x17` 效果量（[02](02-equipment-effects.md)）。kind 8 的 `+0x17=0`，不进装备槽
- 种子种植、村落赠礼、碎片配方（[03](03-discovery-planting.md)）
- 遗迹 25 个名字/发现位、地点地图 → 遗迹声明（[09-remains-oldmaps-catalog.md](../01-world/09-remains-oldmaps-catalog.md)）
- 搜索圈半径、测量士掷骰（ocean）
- `CDiscoverTitleView` 小地图缩放（ui；本系统只说「该播发现镜头」）
- 势力 7×uint16 影响力（factions 只**共用**圈 id 0–6）
- 发现称号名单（progression；本系统只发 `ProofDiscovered`，不发明「七证侯爵」）

ID 只共享 `item_id`、`culture_circle_id`、`remain_id`（可空）。禁止把 kind 8 写成 `remain_id`，禁止把七槽写成第三套 `eItem`。

## 2. 原版事实

### 2.1 种类名：kind 8 = 「霸者之证」

`[确认]` SC exe `.rdata` 连续 13 个种类名，指针表 VA `0x140682ea0`，字符串 `0x14055750c` 起。`CItemKindMenuData` 把 `eItemKind` 从 0 扫到 `< 0xd`（13 档）。来源：`DK4HD_sc.exe`；`CItemKindMenuData.cpp`。

| id | exe 原文 | 01 里的 key | 件数 |
| ---: | --- | --- | ---: |
| 0 | 原产物 | `plant_discovery` | 20 |
| 1 | 航海用品 | `voyage_charm` | 4 |
| 2 | 武器 | `weapon` | 31 |
| 3 | 防具 | `armor` | 30 |
| 4 | 人物装备品 | `person_gear` | 62 |
| 5 | 船首像 | `figurehead` | 12 |
| 6 | 神秘小物件 | `fragment` | 14 |
| 7 | 礼物 | `gift` | 21 |
| 8 | **霸者之证** | `relic`（01 目录名） | **14** |
| 9 | 地图 | `sage_map` | 7 |
| 10 | 遗址的地图 | `place_map` | 12 |
| 11 | 纪念品 | `plot_set` | 3 |
| 12 | 宝物地图 | `treasure_map` | 1 |

「霸者之证」在 exe 里**只出现这一次**（文件偏移 `0x556368`），就是种类名，不是第七件道具名。`items.tsv` 231 行里没有「北海」「霸者之证」这种道具名。World0/World1.dll 也不含这两词。

`[确认]` 种类名和 `Item32.dk4` / `Item56.dk4` 字符串写在同一带（`0x5563a8`）。筛选 UI 按 kind 分档，kind 8 的中文就是「霸者之证」。

01 用 `relic` 当 kind_key 可以保留作别名；本系统稳定 key 用 `hegemony_proof`，`id` 仍是 8。

### 2.2 十四件 kind 8：两套 × 七圈

`SItem+0x14=8`。`+0x15` 恰为 0–6，与势力表 7 文化圈、mesfile 七区同一套下标 `[确认]` 值域；语义「圈号」`[推断]`（与 01 里「kind 8/9 即使 +0x0C==-1 也走 0–6」一致）。

`+0x16=24`，`+0x17=0`（没有装备效果量），`+0x18=98`（未绑港）。`+0x19`：第一套全是 8，第二套全是 10。

**套 A（原版包，World0 海上放置）** `+0x19=8`：

| id | key | name | 圈 `+0x15` | 圈名 | base_price |
| ---: | --- | --- | ---: | --- | ---: |
| 148 | ullr_bow | 狩猎之神乌勒尔的弓箭 | 0 | 北海 | 90000 |
| 149 | cambyses_crown | 肯毕西斯的王冠 | 1 | 地中海 | 110000 |
| 150 | aksum_gold_seal | 阿克斯姆王的金印 | 2 | 非洲 | 140000 |
| 151 | vedas | 吠陀本集 | 3 | 印度洋 | 80000 |
| 152 | kediri_amulet | 库德里的永久护符 | 4 | 东南亚 | 120000 |
| 153 | qin_palace_lamp | 秦始皇的长信宫灯 | 5 | 东亚 | 100000 |
| 154 | crystal_skull | 水晶头骨 | 6 | 新大陆 | 95000 |

**套 B（哈尔舒塔一套，World1 海上放置）** `+0x19=10`，标价一律 100000：

| id | key | name | 圈 |
| ---: | --- | --- | ---: |
| 192 | hallstatt_offering_cart | 哈尔舒塔的供品车 | 0 |
| 193 | agamemnon_mask | 阿葛曼侬的面具 | 1 |
| 194 | ram_headed_sphinx | 羊头狮身像 | 2 |
| 195 | sargon_bust | 萨尔贡的头像 | 3 |
| 196 | trung_tracs_crown | 琼恰克的王冠 | 4 |
| 197 | shang_square_ding | 殷代方形鼎 | 5 |
| 198 | chimu_axe | 契慕的斧头 | 6 |

百科（`mesfile_sc.txt`）：

| 偏移 | 对齐 | 摘 |
| --- | --- | --- |
| `0x1bbcb` | 148 | 北欧神话狩猎之神乌勒尔的紫衫木弓 |
| `0x1bc0f` | 149 | 阿契曼尼王朝肯毕西斯的王冠 |
| `0x1bc77` | 150–152 | 阿克斯姆金印；吠陀；库德里木护符 |
| `0x1bdab` | 153–154 | 秦始皇长信宫灯；水晶磨成的头盖骨，制法是谜 |
| `0x1c47f` | 192–193 | 「澳大利亚北部的铁器文明哈尔舒塔」；阿葛曼侬黄金面具 |
| `0x1c4ff`–`0x1c53f` | 194–198 | 库舒羊头狮身；萨尔贡头像；琼恰克王冠；殷代方鼎；契慕祭斧 |

`0x1c47f` 把 Hallstatt 写成「澳大利亚北部」是 SC 文案事实，不是地理订正入口。逻辑表用 `hallstatt` key，中文走 `name_i18n`。

两套**都在** World0.dll / World1.dll 的 231 行里。差别是哪 7 件被写到海上 Locator，见 2.4。

### 2.3 七张贤者地图（kind 9）是线索，不是证

`[确认]` 155–161：`+0x14=9`，`+0x15=0..6` 与上表同序，`+0x18=98`，标价 1000。exe 种类名「地图」。

mesfile 在赠礼口味之后、紧挨着七句「这是××的霸者之证」，再接「标有××霸者之证的位置」（`0x1e247`–`0x1e41b`）。后一段按圈对齐 155–161 `[推断]`。

碎片 174–187（kind 6「神秘小物件」）的解谜句也指向「北海/地中海…之证的地图」（`0x1e52f`–`0x1e76b`）。配方仍在 `hunting.yaml`，本系统只承认：碎片合成**不得**直接产出 kind 8（01/03 已禁止手填 output）。

### 2.4 七槽状态 + 7×8 出生点

`[确认]` `CItemElement` 在 231 个 `CItem`（步长 `0x40`）之后，另存 **7 字节** `+0x39c8`。存档 `vfunc_01` / `vfunc_02` 按 `s24 >= 7` 各读写一字节。来源：`CItemElement.cpp`。01 第 9 节已标这 7 字节 `[未知]`；本篇把它钉成霸者之证七槽。

`[确认]` `fn_14000d346`（`0x140128f90`）：

```
if player+0x7c >= 4:   # fn_14000864d(DAT_144296500)；剧本 4–6 = World1
    base = 0xC0        # 192
else:
    base = 0x94        # 148
for slot in 0..6:
    item = Items[base + slot]
    item+0x30 = coord_table[slot][ byte_at_39c8[slot] ]
```

`fn_14000864d`：`*(obj+0x7c) < 4` 则 0，否则 1。与「`scenario>=4` 用 World1」同一刀 `[确认]` 阈值；对象是玩家/世界块 `[推断]`。

`[确认]` 坐标表 `DAT_1405575d0`（exe `0x1405575d0`）：7 条，步长 `0x44=68`。每条：`uint32 item_id`（148…154）+ **8 对** `(x, y)` uint32。值域与港口同一空间（伦敦 `20080,4272`）。圈 0 候选落在不列颠附近，圈 6 落在新大陆西侧。

| 槽 | 圈 | 表内 item | 8 个候选 (x, y) |
| ---: | --- | ---: | --- |
| 0 | 北海 | 148 | (20936,2920) (20920,4088) (21912,3928) (22552,3512) (20568,3096) (19656,4360) (21336,3624) (22328,3704) |
| 1 | 地中海 | 149 | (23624,6152) (20808,5880) (24072,5704) (22984,5672) (22248,6536) (23592,6536) (21112,6216) (24360,5416) |
| 2 | 非洲 | 150 | (17464,8200) (18408,8872) (20264,9288) (22360,13832) (23624,13064) (25416,11480) (24424,10872) (20840,9496) |
| 3 | 印度洋 | 151 | (27144,7192) (28104,7720) (28264,8424) (25608,8328) (28872,8680) (29240,8040) (29960,7592) (30040,7432) |
| 4 | 东南亚 | 152 | (32168,8584) (31144,10328) (33640,9176) (33992,10264) (32176,10080) (31224,8552) (33448,9960) (33736,8456) |
| 5 | 东亚 | 153 | (34328,6920) (32536,7592) (33752,7320) (33128,5768) (33560,6520) (33480,5448) (34312,5800) (32328,7784) |
| 6 | 新大陆 | 154 | (11800,8744) (11256,7048) (12984,7992) (10888,8984) (10200,8056) (9192,7192) (9704,6712) (9480,7960) |

World1 仍读**同一张**坐标表（记录头写的是 148–154），只把被放置的 `eItem` 换成 192–198。

`CItem` 构造在 `+8` 调 Locator（`fn_140138020`，与 `CVillage` 相同）`[确认]`。搜索读的是 Locator；`fn_14000d346` 写的是 `item+0x30`。xy 如何落到 Locator 字段 `[未知]`，但 7 件被放到世界坐标上 `[确认]`。

7 字节初值：`CItemElement` 里被标成析构的函数对 7 槽写 `fn_140005272(8)`（即 `rand(8)`，气候代码同一 thunk）再调 `fn_14000d346`。更像开局抽候选，不像析构 `[推断]`。值域 0–7 与 8 个坐标对对齐 `[推断]`。

### 2.5 海上搜索：kind==8 走「证」演出

`[确认]` `fn_14001266b`（`Search.DK4`，RTC `apfnSearchEvent, cptItem, eItem, posPlayerFleet`）：扫 `eItem` 0–230，取 Locator，距离平方与搜索半径比。`fn_14000598e`（读 kind）`== 8` 时走专用分支：`fn_14018b880(0xc8, 0xc8)`；否则普通发现 `fn_14018b880(0xa, 5)` 并 `fn_14000cf1d(..., 0x31, 2, 1)`。来源：`fn_1401b0e80.cpp`。

未找到 / 坐标非法（`cptItem < 0`）则跳过。故只有 Locator 有效且尚未被摘走的点能被搜到。

文案 `[确认]` `mesfile_sc.txt`：

| 偏移 | 用途 |
| --- | --- |
| `0x1024b`–`0x102f7` | 「重大发现。探险队发现了这种东西。」（泛用，不写「证」） |
| `0x112ab`–`0x113c3` | 「这是%s的霸者之证%s」「这是%s的称霸者的标志：%s。要探索吗？」 |
| `0x1e247`–`0x1e2e7` | 「这是北海的霸者之证。」…七句（圈名写死） |
| `0x1e313`–`0x1e41b` | 「标有北海霸者之证的位置。颜色深的部位是海 / 发白的部位是陆地。」 |
| `0x318b` | 委托句里的「发现了%s！」 |

两个 `%s`：第一段文化圈名、第二段道具名 `[推断]`。七句写死圈名的串给「已鉴定」说明用。

### 2.6 遗迹报酬 ≠ 七证

`[确认]` `NUM_REMAINS=25`，名字在 exe 指针表（巨石阵…阿瑜陀耶），贴图 `Remains.dk4` 帧数 `0x19=25`。`CRemains` 体积 `0x10`，**没有** Locator。来源：[09-remains-oldmaps-catalog.md](../01-world/09-remains-oldmaps-catalog.md)。

`[确认]` 不存在 `item_id → remain_id` 整数表。kind 8 与 25 遗迹不是 1:1。地点地图 162–173 只覆盖一部分遗迹，且走 kind 10。

`fn_14000a321`：先扫 231 道具（按 `SItemOwner` 类型 0–3 决定是否清/记），再扫 25 遗迹置发现位。这是读档/同步，不是「遗迹掉落表」。

遗迹内部究竟给不给 kind 8：函数未还原 `[未知]`。vanilla 包 `remain_id: null`，禁止为填表把水晶头骨写成蒂卡尔。

### 2.7 收集 UI 与发现镜头

**种类筛选** `[确认]`：`CItemKindMenuData` 13 档，第 9 档（kind 8）标题「霸者之证」。背包/选择物窗口按 kind 列出已持有的 14 件。没有名为「古宝图鉴」的 RTTI。

**发现镜头** `[确认]`：`CDiscoverTitleView` 体积 `0x100`，内嵌 `CDiscoverTitleMiniMap`。`vfunc_04` 把小地图矩形按 `+0x40` 右移直到 `< 8`，再线性插值 `0x14=20` 帧。这是「发现了某某」的通用相机，村落 / 遗迹 / 道具共用。数据源（读七槽？读背包？读遗迹位？）反编译只见绘制 `[未知]`。称号名单仍见 fame-titles 第 9 节。

**事件静画** `[确认]`：`EvItem.dk4` 头 `GT1G4600`，`tex_count=0x27=39`。`Item32.dk4` / `Item56.dk4` 才是 231 件库存图标。`EvItem` 被教会小游戏、硬币台、事件框、`EvFrame.dk4` 路径加载，**不是**种类 8 专用图集。`25 遗迹 + 14 证 = 39` 算术好看，下标规则未还原 `[推断]`，禁止当引擎映射。

### 2.8 与七文化圈

`[确认]` 势力 World 表 `+0x10` 起 7×uint16，格号 0–6 = **七大海域**（北海 / 地中海 / 非洲 / 印度洋 / 东南亚 / 东亚 / 新大陆）。kind 8 / kind 9 的 `+0x15` 用同一 0–6。本系统**不写**影响力；外键是 `major_sea_id`，不是文化圈。21 文化圈见 `02-factions/06-seven-culture-circles.md`。

不是：25 `eSeaArea`、9 个 NPC 形象圈、21 个港口 `region`。

### 2.9 相邻但不是古宝

| id 段 | kind | exe 名 | 本系统 |
| --- | ---: | --- | --- |
| 127–147 | 7 | 礼物 | 村落赠礼，[03](03-discovery-planting.md) |
| 155–161 | 9 | 地图 | 本表 `clue_map_item_id` |
| 162–173 | 10 | 遗址的地图 | world `unlock_maps.yaml` |
| 174–187 | 6 | 神秘小物件 | `hunting.yaml` |
| 188–190 | 11 | 纪念品 | 剧情三件，narrative |
| 191 | 12 | 宝物地图 | 藏宝事件 |
| 199–213 | 5/4/2/3 | 船首像 / 人物装备品 / 武器 / 防具 | 普通装备。PK 增补块（怪鱼像…古代仪典）`[推断]` 按 id 连续 |
| 214–230 | 2/3/4 | 武器 / 防具 / 人物装备品 | **普通装备**，见下 |

### 2.10 HD 多出来的 214–230：不是证

本仓库只有 HD 的 World DLL（`NUM_ITEM=231`），没有 1999 PK 光盘对照。214–230 **已经在** 这份 231 表里，种类全是 2/3/4，`+0x15=8`（哨兵，不是圈号），`+0x17` 有装备量，岗位口味在 `0x1ebe7` 起（金币给会计长、海盗徽章给冲锋队长、星粉水晶给炮手）。

| id | name | kind | `+0x16` 轨 | `+0x17` |
| ---: | --- | ---: | ---: | ---: |
| 214 | 金币 | 4 | 20 | 54 |
| 215 | 金块 | 4 | 20 | 60 |
| 216 | 海盗徽章 | 4 | 7 | 48 |
| 217 | 星粉水晶 | 4 | 8 | 61 |
| 218 | 幻影金红石 | 4 | 20 | 61 |
| 219 | 冶金丸 | 2 | 24 | 62 |
| 220 | 魅惑与毁灭之像 | 4 | 8 | 69 |
| 221 | 龙角 | 4 | 14 | 80 |
| 222 | 普利德温 | 3 | 24 | 65 |
| 223 | 破魔红蔷薇 | 2 | 24 | 59 |
| 224 | 纯金怀表 | 4 | 3 | 66 |
| 225 | 真红指环 | 4 | 2 | 48 |
| 226 | 阿契美尼斯的高脚杯 | 4 | 15 | 50 |
| 227 | 玫瑰花饰杯 | 4 | 19 | 56 |
| 228 | 玛多加 | 4 | 17 | 51 |
| 229 | 尼亚斯蒂卡 | 4 | 17 | 48 |
| 230 | 小瓶爱情灵药 | 4 | 5 | 58 |

`[推断]` 这一段是 HD 相对「经典 PK 0–213」多出来的装备块：紧挨 213 古代仪典之后；亚瑟王盾、爱情灵药材料、星彩水晶这一路名字不像套 A/B。namu 写「人物装备品全 62 种」与 HD kind 4=62 一致，不能证明 1999 PK 没有它们。

无论是不是 HD 独有：**不要**写进 `ancient_treasures.yaml`。它们走人物槽，圈号哨兵 8，搜索也不走 kind==8 分支。

`CItem` 每件另存 `+0x3b/+0x3c` 两字节（`vfunc_06/07`）。可能是 `ITEMSTATE_BITS`。是否含「已从海上摘走」`[未知]`。重构用 `owned[item_id]` + 七槽候选下标，不复用这两字节布局。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 空 / 非法 | 本系统 |
| --- | --- | --- | --- | --- |
| 古宝行 | `item_id` kind 8 | 14 | 0 是南瓜；空槽 232 | `ancient_treasures.yaml` |
| 霸者之证槽 | `proof_slot_id` / 圈 0–6 | 7 | 无「空圈」；未放置用 `placed: false` | `hegemony_proofs.yaml` |
| 文化圈 | `culture_circle_id` | 7 | — | 外键，定义在 factions |
| 贤者地图 | `item_id` kind 9 | 7 | 232 | 槽上的 `clue_map_item_id` |
| 出生候选下标 | `spawn_index` | 每槽 8 | 0 是合法点 | 存档 7 字节 |
| 遗迹 | `remain_id` | 25 | -1 | **可空**外键，vanilla 全 null |
| 普通装备 | `item_id` kind 2/3/4 | 含 214–230 | 232 | 不进本表 |

禁止：

- 给七槽另起 `eProof` 枚举去覆盖 148–154
- 用 `item_id=0` 当「未收集」
- 把 kind 7 礼物、kind 6 碎片、kind 10 地点地图、214–230 写进古宝表
- 把「北海」写成 `eSeaArea` 名
- World0 / World1 在引擎里 `if world==1` 换 192；用内容包换 `placed_item_id`

模组可加第三套 7 件（`id>=1000`），七槽表加一列 `pack`。引擎按表长，不写死 14、7、8。

`CItemElement+0x39c8` 原版是 7 个 UCHAR。重构存 `spawn_index: u8` × 槽数；导入器再压回 7 字节。

## 4. 算法 / 公式

魔数进 YAML。搜索半径、掷骰仍在 ocean；本系统只解释「这件是不是证、放在哪」。

### 4.1 参数

```
proof_slot_count          = 7      # 文化圈；运行时读表长
spawn_candidates_per_slot = 8      # DAT_1405575d0 每条 8 对
world1_scenario_min       = 4      # fn_14000864d；scenario >= 4 → 套 B
kind_hegemony_proof       = 8
item_unique_stack         = 1
coord_space               = original
```

### 4.2 开局放置

```
function place_proofs(scenario_id, pack):
    table = Proofs.slots()                          # 表长
    series = pack.series                            # world0 → original；world1 → hallstatt
    for slot in table:
        idx = uniform_int(0, len(slot.spawn_candidates)-1)
        Proofs.set_spawn_index(slot.id, idx)
        item_id = slot.item_id[series]
        xy = slot.spawn_candidates[idx]
        Items.set_world_location(item_id, xy)       # LIVE；写 Locator，不存 World*
        Items.set_map_available(item_id, true)      # 尚未入包
    emit ProofsPlaced { series, slots: snapshot() }
```

vanilla：`rand(8)` 对齐 `fn_140005272(8)`。模组可改成固定下标或按剧本锁点。

未放置的那一套（World0 的 192–198 / World1 的 148–154）仍在目录里，可以当遗迹/事件报酬，但**没有**默认海上 Locator。

### 4.3 搜索命中

ocean 在圈内扫「仍在地图上的 `item_id`」。命中后问本系统：

```
function on_search_hit(fleet_id, item_id):
    defn = Treasures.get(item_id)
    if defn is none:
        emit ItemFound { item_id, presentation: generic }   # 「重大发现」
        return
    slot = Proofs.slot_of(item_id)
    Items.AcquireItem { fleet_id, item_id }                 # unique
    Items.clear_world_location(item_id)
    emit ProofDiscovered {
        fleet_id,
        item_id,
        proof_slot_id: slot.id,
        culture_circle_id: slot.culture_circle_id
    }
    emit DiscoveryUnlocked { kind: item, id: item_id }      # progression / ui 订阅
```

kind==8 的演出（`0xc8` 尺寸、`0x112ab` 文案）由 ui 按事件选择。本系统不读 mesfile。

已入包的证：`owned==1` 且 Locator 无效 → 搜索跳过。再搜不到第二件同一 `item_id`（`unique_stack=1`）。

### 4.4 收集进度

```
proofs_owned(series) = count slot where Items.owned(slot.item_id[series])
all_seven            = proofs_owned(current_series) == len(slots)
```

原版**没有**「集齐七证 +N 名声」公式。`TitleUnlocked` 只在 `fame.yaml` 的 `requires` 写了 `discovered.item` 或 `count(kind)==7` 时由 progression 计算。vanilla 称号表保持空。

### 4.5 线索地图

持有 `clue_map_item_id` 不自动揭示坐标。UI 可显示「标有××霸者之证的位置」+ 海陆色块文案。是否在大地图画点：ocean/ui 订阅 `ClueMapOwned`；本表只给外键。禁止引擎 `if has(155): 画出 148 的点`。

## 5. 耦合分析

原版：

- kind 名、7×8 坐标、种类筛选、搜索 kind==8 全写在 exe，不在 World DLL
- 十四件定义在 World DLL（名 + hex）；World0/World1 同名
- 放置选套看 `scenario>=4`，与世界包绑死
- `CItemElement` 把 231 库存和 7 槽出生下标塞在同一个 Element 里存档
- 搜索、发现镜头、遗迹扫描、道具 kind 菜单搅在同一条「发现了」体验里
- 七圈 id 还被势力影响力、贤者地图 `+0x15` 使用

重构：

```
factions.culture_circles     --id 0..6-->  hegemony_proofs.slots
items.items.yaml             --item_id-->  ancient_treasures / proofs
hegemony_proofs              --Locator xy-->  ocean.search
ocean.SearchHit              --item_id-->  Treasures.on_search_hit
Treasures                    --ProofDiscovered-->  ui / progression / narrative
world.remains                --remain_id?-->  本表可空外键
items.hunting                --碎片-->  不产出 kind 8
```

combat / economy 不出现。势力影响力不因集齐七证而改，除非 narrative 显式命令。

`CDiscoverTitleView` 只读 `DiscoverySnapshot`，不持有 `Item*`。闭包只捕获 `item_id` + `fleet_id` + `proof_slot_id`，LIVE 取；人已离队 / 点已摘走：no-op。

## 6. 解耦后的配置

```yaml
# config/items/ancient_treasures.yaml
schema: dk4.items.ancient_treasures
version: 1
pack: vanilla-world0
kind_id: 8
kind_key: hegemony_proof
kind_name_i18n: itemkind.hegemony_proof   # exe「霸者之证」
equip_target: none                        # +0x17=0；不进人物槽
series:
  - key: original
    plus_19: 8
    item_ids: [148, 149, 150, 151, 152, 153, 154]
  - key: hallstatt
    plus_19: 10
    item_ids: [192, 193, 194, 195, 196, 197, 198]
treasures:
  - item_id: 148
    key: ullr_bow
    series: original
    culture_circle_id: 0
    remain_id: null                       # 禁止手填巨石阵
    unknown:
      plus_16: 24
      plus_17: 0
      plus_18: 98
      plus_19: 8
  - item_id: 154
    key: crystal_skull
    series: original
    culture_circle_id: 6
    remain_id: null
  - item_id: 192
    key: hallstatt_offering_cart
    series: hallstatt
    culture_circle_id: 0
    remain_id: null
    unknown:
      plus_19: 10
  - item_id: 198
    key: chimu_axe
    series: hallstatt
    culture_circle_id: 6
    remain_id: null
# 149–153、193–197 同样填；禁止把 127–147、174–187、214–230 写进来
```

名字、标价、图标仍在 `items.yaml`。本文件是收集覆盖层：系列、圈、遗迹外键。校验：`item_id.kind==8`，`culture_circle_id` 在 0..表长-1，同一系列 7 件圈号不重复。

```yaml
# config/discovery/hegemony_proofs.yaml
schema: dk4.discovery.hegemony_proofs
version: 1
pack: vanilla-world0
slot_count: 7
spawn_candidates_per_slot: 8
world1_scenario_min: 4
coord_space: original
randomize_spawn_on_new_game: true         # [推断] rand(8)
presentation:
  generic_find_i18n: discovery.major_find     # 0x1024b 重大发现
  proof_find_i18n: discovery.hegemony_proof   # 0x112ab %s的霸者之证%s
  kind_search_special: true                   # search 对 kind 8 走 0xc8 分支
slots:
  - id: 0
    key: north_sea
    culture_circle_id: 0
    name_i18n: circle.north_sea
    identify_i18n: proof.identify.north_sea   # 「这是北海的霸者之证。」
    clue_map_item_id: 155                     # 泰勒斯的纸地图
    item_by_series:
      original: 148
      hallstatt: 192
    spawn_candidates:
      - { x: 20936, y: 2920 }
      - { x: 20920, y: 4088 }
      - { x: 21912, y: 3928 }
      - { x: 22552, y: 3512 }
      - { x: 20568, y: 3096 }
      - { x: 19656, y: 4360 }
      - { x: 21336, y: 3624 }
      - { x: 22328, y: 3704 }
  - id: 1
    key: mediterranean
    culture_circle_id: 1
    clue_map_item_id: 156
    item_by_series: { original: 149, hallstatt: 193 }
    spawn_candidates:
      - { x: 23624, y: 6152 }
      - { x: 20808, y: 5880 }
      - { x: 24072, y: 5704 }
      - { x: 22984, y: 5672 }
      - { x: 22248, y: 6536 }
      - { x: 23592, y: 6536 }
      - { x: 21112, y: 6216 }
      - { x: 24360, y: 5416 }
  - id: 2
    key: africa
    culture_circle_id: 2
    clue_map_item_id: 157
    item_by_series: { original: 150, hallstatt: 194 }
    spawn_candidates:
      - { x: 17464, y: 8200 }
      - { x: 18408, y: 8872 }
      - { x: 20264, y: 9288 }
      - { x: 22360, y: 13832 }
      - { x: 23624, y: 13064 }
      - { x: 25416, y: 11480 }
      - { x: 24424, y: 10872 }
      - { x: 20840, y: 9496 }
  - id: 3
    key: indian_ocean
    culture_circle_id: 3
    clue_map_item_id: 158
    item_by_series: { original: 151, hallstatt: 195 }
    spawn_candidates:
      - { x: 27144, y: 7192 }
      - { x: 28104, y: 7720 }
      - { x: 28264, y: 8424 }
      - { x: 25608, y: 8328 }
      - { x: 28872, y: 8680 }
      - { x: 29240, y: 8040 }
      - { x: 29960, y: 7592 }
      - { x: 30040, y: 7432 }
  - id: 4
    key: se_asia
    culture_circle_id: 4
    clue_map_item_id: 159
    item_by_series: { original: 152, hallstatt: 196 }
    spawn_candidates:
      - { x: 32168, y: 8584 }
      - { x: 31144, y: 10328 }
      - { x: 33640, y: 9176 }
      - { x: 33992, y: 10264 }
      - { x: 32176, y: 10080 }
      - { x: 31224, y: 8552 }
      - { x: 33448, y: 9960 }
      - { x: 33736, y: 8456 }
  - id: 5
    key: east_asia
    culture_circle_id: 5
    clue_map_item_id: 160
    item_by_series: { original: 153, hallstatt: 197 }
    spawn_candidates:
      - { x: 34328, y: 6920 }
      - { x: 32536, y: 7592 }
      - { x: 33752, y: 7320 }
      - { x: 33128, y: 5768 }
      - { x: 33560, y: 6520 }
      - { x: 33480, y: 5448 }
      - { x: 34312, y: 5800 }
      - { x: 32328, y: 7784 }
  - id: 6
    key: new_world
    culture_circle_id: 6
    clue_map_item_id: 161
    item_by_series: { original: 154, hallstatt: 198 }
    spawn_candidates:
      - { x: 11800, y: 8744 }
      - { x: 11256, y: 7048 }
      - { x: 12984, y: 7992 }
      - { x: 10888, y: 8984 }
      - { x: 10200, y: 8056 }
      - { x: 9192,  y: 7192 }
      - { x: 9704,  y: 6712 }
      - { x: 9480,  y: 7960 }
```

`vanilla-world1` 包只改 `pack` 与默认 `series: hallstatt`（或由 `scenarios.yaml` 的 `world_pack` 选系列）。坐标表共用，不复制一份。

`config/items/kinds.yaml` 片段：

```yaml
- id: 8
  key: hegemony_proof
  aliases: [relic]                 # 兼容 01
  name_i18n: itemkind.hegemony_proof
  equip_target: none
- id: 9
  key: sage_map
  name_i18n: itemkind.map
```

`config/caps.yaml` 片段：

```yaml
hegemony_proof_slots: 7
hegemony_spawn_candidates: 8
item_kind_hegemony_proof: 8
```

文案全部 `ui/strings.*.yaml`（`itemkind.*` `proof.*` `discovery.*` `circle.*`）。本文件的中文只作原版包注释。

`writing-guide` 的目录树没有 `config/discovery/`。本系统按题目落在这里，与 `progression/fame.yaml` 的发现进度相邻；不要把七槽塞进 `items.yaml` 当第 232–238 件道具。

## 7. 独立旋钮

| 旋钮 | 文件 | 改它不必动 |
| --- | --- | --- |
| 14 件系列/圈/遗迹外键 | `ancient_treasures.yaml` | 装备、种植、遗迹定义 |
| 七槽、8 候选坐标 | `hegemony_proofs.yaml` | 搜索半径公式 |
| World0 放 148 还是 192 | 槽上 `item_by_series` + 剧本选包 | `if scenario>=4` 从引擎删除 |
| `randomize_spawn_on_new_game` | 同上 | 存档仍是 7 个下标 |
| 线索地图外键 | `clue_map_item_id` | 贤者地图的百科正文 |
| kind 显示名 | 文案包 | 筛选 UI 按表长画 13 档 |
| 214–230 效果量 | `items.yaml` / `effects.yaml` | 本两表 |
| 发现镜头帧数 20 | ui | 本表只发事件 |
| 集齐条件 | `fame.yaml` `requires` | 本表只供 `count` |

加第八圈：两边表加行，势力 7×uint16 也要加格（那是 factions 的事）。不要在搜索里写死 `==8` 的第二种 kind。

## 8. 对外接口

查询：

```
Treasures.get(item_id) -> TreasureRow | none
Treasures.is_proof(item_id) -> bool          # kind 8 且在本表
Proofs.slot(proof_slot_id) -> Slot
Proofs.slot_of(item_id) -> Slot | none
Proofs.spawn_index(proof_slot_id) -> u8
Proofs.world_xy(proof_slot_id) -> (x,y) | none
Proofs.owned_count(series?) -> u32
Proofs.snapshot() -> { slots: [{id, item_id, spawn_index, owned, xy}] }
```

`TreasureRow = { item_id, series, culture_circle_id, remain_id | null }`  
`Slot = { id, culture_circle_id, item_by_series, clue_map_item_id, candidates }`

命令：

```
PlaceProofs { pack_series }                 # 开局 / 读档后重建 Locator
RerollProofSpawn { proof_slot_id }          # 仅调试；玩法默认开局一次
MarkProofFound { fleet_id, item_id }        # ocean 命中后；内部 Acquire+清点
```

事件：

```
ProofsPlaced { series }
ProofDiscovered { fleet_id, item_id, proof_slot_id, culture_circle_id }
DiscoveryUnlocked { kind: item, id: item_id }   # 与 fame-titles 共用
```

ocean：`SearchHit` 只带 `item_id` + `fleet_id`。world 不读背包。ui：`OpenDiscoverTitleView` 是场景命令。

存档：

- 231 件持有位（items 已有）
- 7 个 `spawn_index`（本系统；对应 `CItemElement+0x39c8`）
- 不存 Locator 绝对值（读档后 `PlaceProofs` 用下标现算）。若导入原版档发现 Locator 与下标不一致，以下标为准并打日志

UI 回调只带 id。舰队已解散、点已入包：整段 no-op。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| `item+0x30` 与 Locator `+8` 哪一个是搜索坐标 | `fn_14000d346` 写 +0x30；搜索读 Locator | `[未知]` 表 xy 空间已确认 |
| 7 字节究竟是候选下标还是「已收集」旗 | `rand(8)` + 8 对坐标 | `[推断]` 下标；收集用 `owned` |
| 标成析构的函数里 `rand(8)` 是否就是开局 | `CItemElement.cpp` | `[推断]` 开局 |
| `+0x19` 8 vs 10 除「套」以外的含义 | hex | `[推断]` 系列；可能还有 UI 帧 |
| 遗迹是否掉 kind 8 | 无对照表 | `[未知]` `remain_id` 保持 null |
| `CDiscoverTitleView` 读七槽还是读背包 | 只有绘制 | `[未知]` |
| `EvItem.dk4` 39 帧下标 | 头 `tex_count=0x27` | `[未知]`；25+14 只是算术 |
| `0x112ab` 两个 `%s` 的填法 | 格式串 | `[推断]` 圈名 + 道具名 |
| 持有线索地图是否画海陆色块 | 文案有「颜色深的部位是海」 | `[未知]` 默认只给文案 |
| 集齐七证是否改交涉/投资/剧本 | 无公式 | `[未知]` 默认不改 |
| 214–230 是否 1999 PK 就有 | 无 PK 光盘 | `[推断]` HD 装备增补；**肯定不是** kind 8 |
| `CItem+0x3b/+0x3c` 是否「已从地图摘走」 | 每件存 2 字节 | `[未知]` |
| 村落源表 `+0x30` 也是 0–6 | 09-remains 目录 | `[未知]` 禁止直接当成圈号 |
| 未放置那一套如何获得（遗迹/事件/狩猎） | 无函数 | `[未知]` 不要发明「石板=水晶头骨」 |

未还原前：YAML 留 `null` / `unknown`。实现只跑确认项与带注释的推断项。禁止为「图鉴好看」给 25 遗迹各塞一件证，禁止把金币/星粉水晶算进七槽。
