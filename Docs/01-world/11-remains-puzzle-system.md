# 遗迹与解谜系统

发现玩法的旗舰规格。原版把「点在世界哪、怎么被找到、进门解什么谜、给什么东西、弹哪扇窗」焊在同一条探索链里。重构拆成 **五层独立表**，彼此只传 ID。

| 层 | 拥有者 | 配置 |
| --- | --- | --- |
| 1 遗迹实体 | world | `config/world/remains.yaml` |
| 2 发现条件 | discovery | `config/discovery/old_maps.yaml` 的 `target` / `discover`（全仓唯一的「地图 → 发现点」声明表）+ ocean 搜索圈 |
| 3 解谜模块 | discovery | `config/discovery/puzzles.yaml` |
| 4 奖励 | items / progression 兑现 | `puzzles.yaml` 的 `reward` 只引用 `item_id` / `title_id` |
| 5 UI | ui 读表 | `old_maps.yaml` 的 `ui`（`SearchEx1/2/End`、读字迹） |

村落坐标与发现位仍见 [03-villages-discovery.md](03-villages-discovery.md)。海上点「要探索吗？」见 [06-ocean/02-search-encounters.md](../06-ocean/02-search-encounters.md)。道具目录见 [09-items/01-item-catalog.md](../09-items/01-item-catalog.md)。倒水三桶是港口小游戏，见 [08-town-life/03-minigames.md](../08-town-life/03-minigames.md)，**不是**本系统的谜题类型。

遗迹 25 个显示名已从 SC exe 指针表 `0x1405b2f80` 抽出（见 [09-remains-oldmaps-catalog.md](09-remains-oldmaps-catalog.md) §2.2），本篇直接引用；百科式补充仍禁止。

术语：本篇的 0–6 即势力文档的**七大海域** `major_sea_id`（`CCulture+0x10`、`CForce+0x10` 七格影响力；原版 RTC 名 `eSeaArea`），不是 21 个文化圈。注册表是 `config/world/major_seas.yaml`（`02-factions/06`），本篇只引用 key。旧稿叫「七文化圈 / `culture_circle_id`」，已统一改名。

## 1. 职责边界

本系统管：

- `eRemains` 的稳定 ID、表长、存档状态形状（`CRemains` / `REMAINSSTATE_BITS`）
- 遗迹与 **七大海域** `0–6`（北海…新大陆）的静态归属：由所在港推出（港 → 文化圈 → `major_sea_id`）
- 遗迹 ↔ 港口挂载（原版 `STown+0x26`）与港内入口（地图点 kind 9/10/11）
- 首访奖励：七大海域影响力（表 `0x140686650`）
- 发现条件：持有哪张地图、是否要先解谜、是否走海上探索、是否走事件
- 解谜模块注册表（`none` / `combine_halves` / `symbol_weave` / `search_ex` / `mystery_hunt`）
- 半图配对 `puzzle_pairs`（文案「只有这一半」「有什么关系」）
- 古地图窗与遗迹内搜索窗的**阶段名、资源、文案 key**（不写像素逻辑）
- 奖励声明：发哪个 `item_id` / 金币 / 称号。真正改背包的是 items，改称号的是 progression

不管：

| 事项 | 归属 |
| --- | --- |
| 24 个村落的 Locator / 盲区 | `03-villages-discovery.md` |
| 海上搜索圈 64 顶点、`Search.DK4`、`rand(100)` | ocean `search.yaml` |
| 道具 kind / 标价 / 装备槽 | items |
| 碎片配方判定的矩形立即数 | ui（`CItemMysteryHunting` ctor） |
| 倒水 / 石墙 / 硬币 / 捉鼠 | `town/minigames.yaml` |
| `CDiscoverTitleView` 缩放动画 | ui / progression |
| 七个剧本 EV 脚本何时塞地图 | narrative 订阅本系统事件 |
| 21 个文化圈 `culture_id`（旧称 region）、9 个 NPC 形象圈 | factions；**不是**本篇的七大海域 |

World0 / World1 是两份内容包，同一 schema。地点地图 `+0x18` 的锚点港两包不同，只覆盖那一列，不写 `if world==1`。

## 2. 原版事实

### 2.1 遗迹实体与计数

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `enum eRemains { NUM_REMAINS, INVALID_REMAINS = -1 }`，头文件注释 integer not recovered | `[确认]` | `srcdata/source/RemainsID.h` |
| `CRemains : CSaveObject`；`Remains.h` 点名 `REMAINSSTATE_BITS` | `[确认]` | `Remains.h` `class_hierarchy.txt` |
| `CRemains` 体积 `0x10`；状态 `+0x8` 是 **u8**（字节存档器 `fn_14000c6da`；测试 `fn_1400c5fa0` `movzx` 字节）；`vfunc_07/06` 存/读该槽；`vfunc_05` 把它清 0 | `[确认]` | `CRemains.cpp`、反汇编 |
| 位 0 = 已发现：测 `fn_14000c6f3`，置 `fn_14000f27c → fn_1401ef750(&+8,0,1)` | `[确认]` | 反汇编 `0x1400c8790` `0x1401eefc0` |
| 遗迹挂港：`remains_id = STown[town]+0x26`（`fn_140002450`），`0x1a`=无。24 港各挂一座，木造教堂（1）无港 | `[确认]` | `0x1402611b0`、`world_tables.json`；表见 09 §2.2 |
| `CRemainsElement` 在 `CWorld +0xf0d0` | `[确认]` | `CWorld.cpp` ctor 调 `fn_1402a63e0` |
| 构造 `fn_140007dce((this+8), 0x10, 0x19, …)`：分配 **0x19** 个步长 **0x10** 的槽 | `[确认]` | `CRemainsElement.cpp` |
| 析构 / 存档 `vfunc_01` / 读档 `vfunc_02` 均 `s20 >= 0x19` 就停 | `[确认]` | 同上 |
| 存档扫描 `fn_14000a321`：先 `eItem >= 0xe7`（231），再 `eRemains >= 0x19` | `[确认]` | `fn_1402aad20.cpp` RTC `eRemains,sItemOwner,eItem` |
| `Game/Remains.dk4` 是 G1T，`ntex = 0x19 = 25`，文件 7680928 字节 | `[确认]` | 本机读文件头 `GT1G4600` + `u32[+0x10]=25` |
| 遗迹静帧 `fn_14001233c` 先铺 `EvFrame.dk4`，再 `fn_140010e6f(..., "Remains.dk4", sB8, rcSrc)`，`rcSrc` 为 HD `0×0–0x780×0x438`（1920×1080） | `[确认]` | `fn_1401eb2a0.cpp` |
| `sB8` 即贴图下标 = `remains_id` | `[确认]` | `fn_14000f803` 把 `STown+0x26` 直接传给 `fn_14001233c` |
| `NUM_REMAINS = 25`（原版实例计数） | `[确认]` | 名字指针表 25 条后接 NULL；循环 `0x19`；贴图 25；头文件整数仍未恢复 |
| 遗迹坐标 | `[确认]` 无独立坐标，继承所在港 | `CRemains` **不是** `CLocator`；入口是港内地图点 |
| `REMAINSSTATE_BITS` | u8；位 0 已发现 `[确认]`，位 1–7 `[未知]` | `sizeof=1` `[推断]` |

村落近港筛选的哨兵 `0x19` 是 `NUM_VILLAGE+1=25`，和遗迹循环 `0x19=NUM_REMAINS` **不是**同一个 25。不要混。

### 2.2 七大海域 `0–6`（霸者之证七区）

不是 21 个文化圈，不是 9 个 NPC 形象圈（原版也没有「25 海域」，见 `01` §2）。势力表 `+0x10` 的 7×uint16 与 mesfile 七区同序，见 [02-factions/01-forces.md](../02-factions/01-forces.md)。下表 key 与 `config/world/major_seas.yaml` 一致：

| id | key | 文案 |
| ---: | --- | --- |
| 0 | `north_sea` | 北海 |
| 1 | `mediterranean` | 地中海 |
| 2 | `africa` | 非洲 |
| 3 | `indian_ocean` | 印度洋 |
| 4 | `se_asia` | 东南亚 |
| 5 | `east_asia` | 东亚 |
| 6 | `new_world` | 新大陆 |

来源：`mesfile_sc.txt` `0x1e247`–`0x1e2e7`「这是××的霸者之证」、`0x1e313`–`0x1e41b`「标有××霸者之证的位置」。

道具世界表 `+0x15`：

| 对象 | `+0x15` | 等级 |
| --- | --- | --- |
| kind 8 古宝 148–154、192–198 | 恰好 0–6 各一件（两套） | `[确认]` hex；语义 `[推断]` = 七大海域 |
| kind 9 贤者地图 155–161 | 恰好 0–6 | 同上 |
| kind 10 地点地图 162–173 | 一律 `8` | `[确认]` 与「未绑」哨兵同形，**不是**文化圈下标 |

OldMapStill 旁白还有「北海海域 / 北海海域周边 / … / 新大陆海域」一组 caption `[确认]` exe `0x14056f6f8` 起，给读图窗用地，不是 `eSeaArea`。

### 2.3 地点地图 162–173：只能绑到文案目标 + 锚点港

kind 10，标价 500–3000。`+0x18` 落在 0–96 的 `eTown`，**禁止**当成 `remains_id` / `village_id`（遗迹循环 25，村落 24，港 97）。01 道具目录已写明。

说明文案按道具表顺序排在 `0x1bf07`–`0x1c123` `[确认]`。

World0 `+0x18`（锚点港，`[推断]` 航线/出发提示，不是遗址坐标）：

| item_id | key | 道具名 | mesfile 目标（摘） | 锚点港 W0 | W1 若不同 |
| ---: | --- | --- | --- | --- | --- |
| 162 | `map_stonehenge` | 去环状石柱遗址的地图 | 英国南部；未知记号 | 2 阿姆斯特丹 | 7 斯德哥尔摩 |
| 163 | `map_colosseum` | 去古代竞技场的地图 | 古罗马帝国首都；意大利几乎尽收 | 5 汉堡 | 22 伊斯坦布尔 |
| 164 | `map_cave_village` | 去岩窟村庄的地图 | 小亚细亚；基督教徒藏身的**村落** | 19 雅典 | 同 |
| 165 | `map_sahara` | 撒哈拉大沙漠地图 | 廷巴克图商队 | 29 圣乔治 | 同 |
| 166 | `map_imperial_mosque` | 去帝王清真寺的地图 | 正中央画着国王的清真寺 | 34 索法拉 | 同 |
| 167 | `map_mughal` | 莫卧儿帝国地图 | 到首都德里的路线 | 38 巴士拉 | 同 |
| 168 | `map_ancient_temple` | 去古代寺院地图 | 古代柬埔寨王朝；王宫兼寺院 | 50 雅加达 | 同 |
| 169 | `map_beijing` | 去北京的地图 | 中国北部；前往首都北京 | 57 汉城 | 同 |
| 170 | `map_royal_tomb` | 去王陵的地图 | 通向国王墓地；有些部位无法解读 | 54 杭州 | 59 大阪 |
| 171 | `map_golden_temple` | 去黄金寺的地图 | 黄金之国日本建造的寺院 | 42 卡利卡特 | 47 马六甲 |
| 172 | `map_ancient_city` | 去古代城市遗址的地图 | 废墟的古代城市与远岛往来 | 65 委拉克鲁斯 | 同 |
| 173 | `map_aztec` | 阿斯帝加王国彩绘地图 | 王国文化发源的图画书 | 61 哈瓦那 | 13 塞维利亚 |

164 的目标是村落不是遗迹 `[确认]` 文案「村落」。`village_id` 仍 `[未知]`，YAML 写 `target: { kind: village, key: null }`。

其余 11 张的 `remains_id` **全部 `[未知]`**。可以在 `inferred_name_match` 里给遗迹 key 作注释，导入器不得据此填 `target.key`。

171 World0 锚在卡利卡特、文案却是日本寺院：这正是「`+0x18` ≠ 遗址」的反例 `[确认]`。

「从伦敦 / 热那亚 / 圣乔治 / 卡利亥特 / 杭州 / 大阪 / 委拉克路斯开始探险吧」（`0x1e41b`–`0x1e52f`）是出发提示句，**未**与 12 张地图一一钉死，单独进 `ui.strings`，不写进 `unlock` 外键。

### 2.4 贤者地图、碎片、神秘狩猎

kind 9（155–161）说明在 `0x1be2b`–`0x1beeb`：纸 / 布 / 石板 / 荷叶 / 货币图案 / 劈开的竹子 / 刀尖。`+0x15` = 0–6，与 2.2 七大海域同序 `[推断]`。

kind 6 碎片 174–187，标价一律 100，`+0x18=98`（未绑港）`[确认]` hex。成对名称 + 与贤者地图原料同形 `[推断]`：

| 碎片 | 文案 | 推断产出（禁止当确认） |
| --- | --- | --- |
| 174 老旧的羊皮纸 | 空白的羊皮纸 | 155 纸地图 |
| 175 画有图案的布 | 一块画着奇怪图案的布 | 156 布地图 |
| 176 + 183 石板上/下半部 | 刻着神秘图案的石板的上/下半部 | 157 石地图 |
| 177 不枯萎的莲叶 | 不会干枯的莲叶 | 158 叶地图 |
| 178 古代王国的钱币 | 古王国钱币 | 159 货币地图 |
| 179 + 186 唐代竹艺品 + 组合说明图 | 竹工艺品 / 拼装图解 | 160 竹地图 |
| 180 + 187 典礼用的小刀 + 太阳图纹的刀鞘 | 仪式刀子 / 太阳形刀鞘 | 161 刀刃地图 |

181 红色染料、182 黄铜煤油灯、184 贵霜朝大盘、185 乳液壶：同 kind 6，配对对象 `[未知]`。不要发明第四对。

`CItemMysteryHunting` `[确认]`：

- `item/CItemMysteryHunting.cpp`，基类 `CSurfaceSpriteView`，析构 `fn_14000750e(..., 0x320)` → 体积 **0x320**
- 主构造 `0x1401296a0` 把 `+0xb0`–`+0x1ec` 写成一串立即数（`0xbf,0xae,0x1cc…`）= **UI 矩形，不是配方**
- `+0x208` 子对象指针；`vfunc_02` 非空则虚调 `+0x10` 再销毁
- 拼合谓词**没有**从这些方法还原

配方进 `puzzles.yaml` / items 的 `hunting.yaml`，引擎按无序集合匹配。output 未从代码钉死的行保持 `null`。

### 2.5 半图 `puzzle_pairs`（「只有这一半」「有什么关系」）

贤者地图拿到手之后还有第二层阅读文案 `0x1e52f`–`0x1e76b`。用户点名、且句子成对出现的是：

| pair key | 圈 | 文案 | 等级 |
| --- | --- | --- | --- |
| `africa_indian_ocean` | 2 + 3 | 「可能标明了非洲…**但只有这一半**也不清楚」「好像和标明**印度洋**霸者之证位置的地图**有什么关系**」（`0x1e52f` `0x1e6ff`） | `[确认]` 文案配对；道具 id `[推断]` 157+158 |
| `se_asia_east_asia` | 4 + 5 | 「也许它是发现**东南亚**…所必需的」「也许和**东亚**的霸者之证**有什么关系**」（`0x1e643` `0x1e76b`） | `[确认]` 文案配对；道具 id `[推断]` 159+160 |

同段还有「和北海 / 地中海 / 新大陆之证的地图有什么关系」。那是同一修辞，**未**写成「只有这一半」的对称句。YAML 可留 `related_unpaired`，默认不启用第二条半图规则。

石板 176+183、竹艺 179+186 是 **碎片层** 的半件，类型 `mystery_hunt`，不要和圈 2+3 / 4+5 的半图合成一层。

### 2.6 符号织纹 `symbol_weave`

`0x1e817` `[确认]`：

> 刻着奇怪的图案……「○◇　﹡□　﹟□　﹢○　◇﹡」没有一个国家用这样的文字。这图案的织法是部族的秘密。怎么织成的外人根本不知道。标明了宝物的位置。发白的部位是陆地。

紧挨着 190 昆毕斗篷「织成的是部族特有的图案」和 175「画着奇怪图案的布」`[推断]` 材料，不是引擎字面比较。图案串进 `puzzles.yaml` 当内容，解法 ISA **未还原**（是否重排、是否与布钱「黄鹤碧空　白发秋霜」`0x1e7e3` 成套：`[未知]`）。

### 2.7 遗迹内搜索窗 `search_ex`（`fn_140006429`）

这是遗迹/解谜的结算 UI，**不是**海上 `Search.DK4`。

| 项 | 值 | 等级 |
| --- | --- | --- |
| 函数 | `fn_140006429` body `0x14012cb30` size `0x1a62` | `[确认]` |
| RTC | `rcSearchEnd` `posSearchEx1` `rcSearchEx2` `rcSubTitle` `rcItem` `rcCloseButton` `rcWindow`… | `[确认]` `unassigned_named.tsv` |
| 资源 | `OldMap.dk4` 两次（下标 2 与 3）+ `InfoBox.dk4` | `[确认]` |
| 与 `CItemMysteryHunting` | 同址段（ctor `0x1401296a0`）；本函数写 `this+0x208` 再 `OldMap.dk4` 第 3 帧，狩猎窗 `vfunc_02` 拆这个指针 | `[推断]` 即该类的搭建函数，RTTI 未标在 thunk 上 |
| 三阶段文案（SC exe，GBK 小端） | SearchEx1「你终于解开了传说的谜团」`0x14055aa90`；SearchEx2「并获得了古老财宝！」`0x14055aab0`；SearchEnd「探索结束！」`0x14055aa80`；副标题「获得的财宝」`0x14055aac8` | `[确认]` |
| 分支 | 栈上模式值 `==1 / ==2 / ==3` 三套摆件；1 = 单件 `rcItem`，2/3 = 循环摆多件 | `[确认]` 控制流；模式语义 `[未知]` |
| 三个 `0xc8` 按钮槽 | 循环 `>= 3` | `[确认]` |

`Game/OldMap.dk4`：G1T `ntex=6`。本函数用 2、3；读字迹函数用 1、4。缺的 0、5 `[未知]`。

### 2.8 读字迹窗（`COldMap*` + `OldMapStill.dk4`）

另一扇窗，看地图、叠半图，不发「古老财宝」。

| 类 | 体积 / 要点 | 等级 |
| --- | --- | --- |
| `CGetOldMapWindow` | 析构 `0xb8`；继承 `COldMapSprite` | `[确认]` |
| `COldMapSprite` | ctor 写 HD 布局：`+0x58=0x780` `+0x5c=0x438` 以及一串 0x41d/0x16/… | `[确认]` `COldMapSprite.cpp` |
| `COldMapInfomationView` | 析构 `0x630`；ctor 分配 4 个 `0xd0` 槽；`+0x508…+0x528` 五个子指针，`vfunc_02` 逐个拆 | `[确认]` |
| `COldMapButton` | 推钮 | `[确认]` |

`fn_14000f290`（`fn_140177480.cpp`）`[确认]`：

- 拷 4 个 flag 到 `+0x7c`，非 0 则 `+0x8c++`
- 循环 `s188 >= 5`：`s188==0` 载 `OldMap.dk4` **帧 1**；否则把 `s188-1` 当层号，flag 非 0 才载 `OldMapStill.dk4`
- 下标表达式反编译成 `object_ptr * field_78 + layer`，是坏还原。内容层按「每遗迹 4 张 still」配

`Game/OldMapStill.dk4`：G1T `ntex=0x64=100` `[确认]`。`100 / 25 = 4` `[推断]` `stills_per_remain=4`，与上面 4 flag / 4 槽同形。

读字迹文案 `[确认]` exe `0x14056f6ac` 起：「读字迹」「没有地图」「地图所示的位置」「？？？？？？？」。前一句 DAT「古老地图」。再前面是 NPC 给图：「不知道和哪批货物一起混进来的…你拿走也不要紧的。」给图事件归 narrative（发命令 `Items.acquire`），本系统只读持有状态。

### 2.9 奖励：古宝 / 霸者之证 / 不是倒水奖金

exe 道具种类菜单连续 8 个名字，紧挨 `Item56.dk4` `[确认]` `0x140557548`：

`船首像 | 神秘小物件 | 礼物 | 霸者之证 | 地图 | 遗址的地图 | 纪念品 | 宝物地图`

与 kind 5–12 的件数特征对齐 `[推断]`：

| kind | key（01 目录） | 菜单名 |
| ---: | --- | --- |
| 5 | `figurehead` | 船首像 |
| 6 | `fragment` | 神秘小物件 |
| 7 | `gift` | 礼物 |
| 8 | `relic` | **霸者之证** |
| 9 | `sage_map` | 地图 |
| 10 | `place_map` | 遗址的地图 |
| 11 | `plot_set` | 纪念品 |
| 12 | `treasure_map` | 宝物地图 |

因此「霸者之证」在原版首先是 **kind 8 的 14 件遗物**，不是独立枚举。海上探险队句「这是%s的霸者之证%s」（`0x112ab`）把圈名和物件名填进同一件道具 `[推断]`。

两套 kind 8，`+0x15` 都是 0–6：

| 圈 | 原版 148–154（W0 `+0x19=8`） | PK/HD 192–198（W0 `+0x19=10`） |
| ---: | --- | --- |
| 0 | 148 狩猎之神乌勒尔的弓箭 | 192 哈尔舒塔的供品车 |
| 1 | 149 肯毕西斯的王冠 | 193 阿葛曼侬的面具 |
| 2 | 150 阿克斯姆王的金印 | 194 羊头狮身像 |
| 3 | 151 吠陀本集 | 195 萨尔贡的头像 |
| 4 | 152 库德里的永久护符 | 196 琼恰克的王冠 |
| 5 | 153 秦始皇的长信宫灯 | 197 殷代方形鼎 |
| 6 | 154 水晶头骨 | 198 契慕的斧头 |

World1 把两套的 `+0x19` **对调**（原版 10、PK 8）`[确认]` hex。语义 `[未知]`（哪套在哪包当遗迹掉落？）；内容包各写自己的 `reward.item_id`，引擎不读 `+0x19` 猜。

SearchEx2 的「古老财宝」指向这批 kind 8 `[推断]`，不是 105 件交易货。

金币、称号：本窗没有「得到了%s金币」；`CDiscoverTitleView` 是发现演出，名单未还原。奖励表允许 `gold` / `title_id`，vanilla 包默认 `null`。

藏宝图 191（kind 12）走自己的 `treasure_map` 事件，不进狩猎配方产出 `[确认]` 03 种植篇已禁止。

### 2.10 明确不是遗迹谜题

| 系统 | 为什么容易混 | 归属 |
| --- | --- | --- |
| `water` 倒水 | 三桶、答案表、像遗址机关 | `CWaterGameView`，港口小游戏 |
| `stone_wall` | 9 步标准解写死在函数里 | 同上 |
| 海上 `Search.DK4`（10 帧） | 也叫 Search | 探索圈；`fn_140001005` 等 |
| 温泉 / 绿洲 / 兽袭 | 探险队同一段 mesfile | ocean happening |
| 教会仪式「来遗迹这里有事吗？」`0xbe2f` | 句子含「遗迹」 | 与巡礼/绝食同一文案块；是否遗迹内 NPC `[未知]`，不注册成 puzzle type |

### 2.11 城镇里的 `cRemains`：原版唯一已证的遗迹入口

`[确认]` `CTownScene::vfunc_02` 的 kind 跳表把 9 遗址 / 10 教会 / 11 城门**都**分发到 `CMpRemains` → `fn_14000f803(obj, scene, kind)`（`10-mappoint-kinds.md` 2.3）。

`[确认]` 流程：

1. `remains_id = STown[当前港]+0x26`
2. `fn_14001233c` 画 `EvFrame.dk4` + `Remains.dk4[remains_id]`
3. 已发现 → `fn_14000eacf`（重访）；否则 `fn_14000c4d2`（首访）
4. 首访：播曲 `0x32`；消息 `0x129`（遗迹名）；`fn_14000f27c` 置位 0 并给玩家势力加七大海域影响力（表见下）；再按 `fn_140010f46()`（= `game+0x7c` 即 `eScenario`，`[确认]`，见 09 §2.6）为 0 时 ×2 后**第二次**加到同一势力；消息 `0x12a` 显示数值

影响力表 `0x140686650`（列按 `major_sea(town)`：北海·地中海 / 非洲 / 印度洋·新大陆 / 东南亚·东亚）：

| kind | 列 0 | 列 1 | 列 2 | 列 3 |
| --- | ---: | ---: | ---: | ---: |
| 9 遗址 | 3 | 5 | 10 | 15 |
| 10 教会 | 1 | 1 | 1 | 1 |
| 11 城门 | 5 | 10 | 15 | 20 |

`game+0x7c == 0`（`eScenario` 剧本 0）时表值 ×2；累加后夹到 9999。重构：倍数是 factions 修正器 `factions.influence_gain`（`source: remains`，剧本包注入），上限是 factions 的影响力 cap，本系统都不持有。

`[确认]` 已发现后再进入走 `fn_14000eacf`（重访）：按 kind 9/10/11 分到三支按日期的仪式（`0x140008e9a` / `0x140002171` / `0x14000e840`），玩法见 `08-town-life/05-inn-church-palace.md`。

`[确认]` 哪港出现入口由港位域 `CTown+0x14` 决定：World1 初值给 24 个遗迹港各置 9/10/11 之一；World0 初值全不置，需运行时开（`fn_140001f87`，`[推断]` 剧本改港数据）。

`[确认]` 叙事侧：`fn_14001393a`（`0x1400c7e80`，事件条件族）读一个 `0..0x18` 的遗迹号并返回其「已发现」。

**未见**海上搜索直接置遗迹位的代码路径；海上搜索区域判定（`fn_14000f218`/`fn_140009aca`）属道具搜索。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key | 拥有者 |
| --- | --- | --- | --- | --- |
| `eRemains` | `[确认]` 25（循环上界 + 名字表，非投票） | `null`（原版 -1；港口表空 26，legacy） | `remains` key | world |
| `REMAINSSTATE_BITS` | `CRemains+0x8` u8，位 0 已发现 | — | `remains_flags`（按 key 存，不复用原版位布局） | world 存档 |
| 七大海域 | 7（0–6） | `null`（道具 `+0x15=8` = 未绑，legacy） | `major_sea` key | factions 定义，本表引用 |
| 地点地图 | item 162–173 | — | `item_id` kind 10 | items |
| 贤者地图 | item 155–161 | — | `item_id` kind 9 | items |
| 碎片 | item 174–187 | — | `item_id` kind 6 | items |
| 古宝 / 霸者之证 | item 148–154、192–198 | — | `item_id` kind 8 | items |
| 解谜模块 | 无枚举 | — | `puzzle_type` | discovery |
| 半图对 | 文案 2 对确认 | — | `puzzle_pair_id` | discovery |
| 读图层 | 每遗迹 4 张 `[推断]` | flag 0 = 缺层 | `still_layer` 0–3 | ui / discovery |
| 搜索阶段 | SearchEx1/2/End | — | `search_ex_stage` | ui |
| 叙事遗址别名 | 12 张地图目标 | 不是 ID | `remains_key` / `site_key` | 内容注释 |

禁止：

- 用 `+0x18` 城镇 id 当 `remains_id`
- 用 `0` 当空遗迹（0 是合法下标）
- 把 164 岩窟村写成 remain
- 把 9 个 NPC 圈或 21 个文化圈填进 `major_sea_id`
- 为 25 个槽发明「金字塔 / 长城」这种仓库里没有的名字

模组追加遗迹用自己命名空间的 key（`mymod:atlantis`），加载器顺序分配整数，不覆盖原版条目。引擎按表长循环，禁止再写 `0x19`。

## 4. 算法 / 公式

魔数全部进 YAML。原版闭合式缺的步骤标 `[未知]`，默认 no-op 或只发事件。

### 4.1 参数

```
# caps / formulas/discovery.yaml
remains_count_vanilla          = 25          # 循环 0x19；运行时 len(remains)
major_sea_count               = 7           # 读 factions major_seas 表长
# 原版哨兵（遗迹 -1 / 26、道具 +0x18 未绑港 98）只在 legacy_codec
stills_per_remain             = 4           # OldMapStill 100/25 [推断]
oldmap_tex_count              = 6
remains_tex_count             = 25          # Remains.dk4 ntex
search_dk4_tex_count          = 10          # 海上，不进本公式
search_ex_button_slots        = 3
oldmap_flag_slots             = 4
hd_frame_w                    = 1920        # 0x780
hd_frame_h                    = 1080        # 0x438
```

### 4.2 层 1 — 实体只回答定义

```
function Remains.def(remains_id) -> RemainsDef | none:
    if remains_id is none or remains_id < 0: return none
    if remains_id >= len(remains): return none
    return remains[remains_id]          # 位置 = LocatorRef(town, Remains.town(id))，可 null；major_sea 由该港派生

function Remains.flags(remains_id) -> FlagSet:
    # 存档按 id 映射；导入原版时再展开 REMAINSSTATE_BITS
```

坐标 LIVE 取。表里禁止存 `World*` / `Location`。取不到世界或点已卸载：整步 no-op。

### 4.3 层 2 — 发现条件

```
function can_discover(remains_id, fleet_id) -> { ok, reason }:
    row = Remains.def(remains_id)
    if row is none: return { false, missing_def }
    if Remains.discovered(remains_id): return { false, already }
    cond = Discover.for_remain(remains_id)      # old_maps.yaml
    if cond is none: return { false, no_condition }

    if cond.needs_map_item:
        if not Items.owned(fleet_id, cond.map_item_id):
            return { false, need_map }
    if cond.needs_puzzle:
        if not Puzzle.solved(cond.puzzle_id):
            return { false, need_puzzle }
    if cond.needs_event_flag:
        if not Narrative.flag(cond.event_flag):
            return { false, need_event }
    # 海上半径：Ocean.search_radius(fleet_id) vs 点坐标
    # 原版世界半径 [未知]；点击盒 16px 是 UI，见 ocean 文档
    return { true, ok }

function on_enter_town_map_point(town_id, kind, fleet_id):     # 原版唯一已证路径；world 订阅 FacilityEntered（kind 9/10/11）
    remains_id = Remains.at_town(town_id)                       # STown+0x26
    if remains_id is none: return
    if Remains.discovered(remains_id):
        emit RemainsRevisited { remains_id, town_id, kind }       # town-life 据此跑仪式（fn_14000eacf）
        return
    set_discovered(remains_id)                                  # world 内部置位；事件在末尾发一次
    sea = MajorSea.of_town(town_id)
    v = influence.table[kind][influence.col_by_major_sea[sea]]
    v = Influence.add(player_force, sea, v, reason: remains)   # 倍数与 cap 在 factions；返回修正后、夹紧前的加值
    if influence.apply_twice:
        Influence.add(player_force, sea, v, reason: remains)   # 剧本 0 的 ×2 由同一修正器给出（09 §4）
    emit RemainsDiscovered { remains_id, via: town_map_point, town_id, fleet_id, item_id: null }

function on_ocean_search_hit(kind=remains, remains_id, fleet_id):   # 重构扩展；原版未见
    r = can_discover(...)
    if not r.ok: emit SearchNothing; return
    World.mark_remains_discovered(remains_id, via: ocean_search, fleet_id: fleet_id)   # world 置位并发 RemainsDiscovered

function on_use_place_map(item_id, fleet_id):      # items 侧 UseItem 调用；world 只收命令
    row = OldMaps.place_map(item_id)
    if row.target.key is none:
        emit UnlockRejected { item_id, reason: target_unspecified }
        return   # vanilla 12 张全 null → 只给读字迹，不改发现位
    if row.target.kind == village:
        World.mark_village_discovered(row.target.key, via: item, fleet_id: fleet_id, item_id: item_id)   # world 发 VillageDiscovered{via: item, item_id}
    elif row.target.kind == remains:
        World.mark_remains_discovered(row.target.key, via: item, fleet_id: fleet_id, item_id: item_id)   # world 发 RemainsDiscovered{via: item, item_id}
```

164 走 village 支。引擎不得 `if item_id==164: village=??`。

### 4.4 层 3 — 解谜模块

模块是表驱动的插件。未知 ISA 的类型仍注册，`solve()` 默认发 `PuzzleBlocked { reason: isa_unknown }`，禁止在 C++ 里补「看起来合理」的机关。

```
function Puzzle.solve(puzzle_id, fleet_id, input) -> Result:
    p = puzzles[puzzle_id]
    match p.type:

    none:
        return ok                      # 发现即完成

    combine_halves:
        a, b = p.item_ids              # 长度 2
        if not (Items.owned(a) and Items.owned(b)):
            return need_other_half
        emit PuzzleSolved { puzzle_id, pair: p.key }
        # 是否消耗半图 [未知]；consume_inputs 默认 false（文案是「有什么关系」不是「拼掉」）

    symbol_weave:
        # 原版比较式 [未知]
        return PuzzleBlocked { isa_unknown }

    mystery_hunt:
        recipe = Hunting.match(input.item_ids)   # 无序
        if recipe is none: return no_recipe
        if recipe.output_item_id is none:
            return output_unspecified  # vanilla 多行如此
        if recipe.consume_inputs:
            for id in recipe.inputs: Items.discard(fleet_id, id)
        Items.acquire(fleet_id, recipe.output_item_id)
        emit ItemsCombined { recipe_key, output_item_id }

    search_ex:
        # 只推进 UI 阶段；真正发奖走层 4
        stage = session.search_ex_stage + 1
        if stage > end: return already
        session.search_ex_stage = stage
        emit SearchExAdvanced { remains_id, stage }
        if stage == end: return ok
```

`water_buckets` **不在** `match` 里。城镇小游戏赢了发 `MinigameWon`，本系统不订阅。

半图对检查的是 **圈 2 与 3 的贤者地图是否都在包里**，不是 176 与 183（那是 mystery_hunt）。

### 4.5 层 4 — 奖励

```
function grant_puzzle_reward(puzzle_id, fleet_id, remains_id):
    r = puzzles[puzzle_id].reward
    if r.item_id is not none:
        Items.acquire(fleet_id, r.item_id)
        emit RelicGranted { remains_id, item_id: r.item_id }   # kind 8 时即霸者之证
    if r.gold:
        Force.add_gold(player_force, r.gold, reason: puzzle_reward)   # 命令；vanilla null
    if r.title_id is not none:
        emit TitleUnlocked { title_id }                      # vanilla 空表
```

SearchEx 播完「并获得了古老财宝」之后才 `grant`。item 已持有且 `unique_stack=1`：按 items 规则拒绝或 no-op，不另发明堆叠。

### 4.6 层 5 — UI 投影

```
# 读字迹
still_index(remains_id, layer) =
    remains_id * stills_per_remain + layer     # [推断]；layer 0–3
# flag[layer]==0 → 不载该 still（fn_14000f290）

# 遗迹静帧
remains_tex_index = remains_id                 # [推断]

# search_ex 文案
stage 1 → ui.discovery.search_ex1    # 你终于解开了传说的谜团
stage 2 → ui.discovery.search_ex2    # 并获得了古老财宝！
stage 3 → ui.discovery.search_end    # 探索结束！
subtitle → ui.discovery.loot_title   # 获得的财宝
```

像素矩形（`COldMapSprite+0x40` 一串、`CItemMysteryHunting+0xb0` 一串）进 `config/ui/`，不进玩法表。

海上确认「要探索吗？」`0x11467` 仍归 ocean。本窗不复用那句。

## 5. 耦合分析

原版焊点：

- `fn_14000a321` 存档一次扫 231 道具 + 25 遗迹
- 地点地图的目标在 mesfile，锚点港在 `SItem+0x18`，遗迹下标两头都没有
- 贤者地图 `+0x15`、古宝 `+0x15`、势力影响力 7 格共用文化圈序，但没有任何头文件叫 `eCultureCircle`
- `CItemMysteryHunting` 既是港口「神秘小物件」桌，又像遗迹结算窗（SearchEx 同函数族）
- `CTownScene` 名字上挂 `cRemains`，港口地图还有 `CMpRemains`
- 探险队发现霸者之证、温泉、兽袭写在同一段 mesfile
- World1 只改地点地图锚点港和古宝 `+0x19`，逻辑 exe 同一份

重构切断：

```
world.remains.yaml        只定义点
discovery.old_maps.yaml   地图 → 条件 / 读图层
discovery.puzzles.yaml    类型 → 输入输出
items.items.yaml          只提供 item_id
ocean.search.yaml         只调 World.mark_*_discovered（重构扩展）
progression.fame.yaml     只订阅 RelicGranted / RemainsDiscovered
ui                        只读阶段与资源 key
town.minigames            倒水不出现在 puzzles.types
```

ID 是唯一共享词汇：`remains_id` `item_id` `village_id` `major_sea_id` `puzzle_id` `fleet_id`。

## 6. 解耦后的配置

### 6.1 `config/world/remains.yaml`

schema 与全表只在 `09-remains-oldmaps-catalog.md` §6（含 `stills`、`influence_reward`、`apply_twice`）。本篇要的字段都在那里：遗迹 key、名字、读图层 `stills`、首访影响力表。挂港只在 `towns.yaml` 的 `remains` 字段；`major_sea` 由所挂港派生，不手填。

### 6.2 `config/discovery/old_maps.yaml`

全仓唯一的「地图道具 → 发现点」声明表（旧稿 `09` 的 `config/world/unlock_maps.yaml` 并入此处）。外键一律写 key。

```yaml
schema: dk4.discovery.old_maps
version: 2
pack: vanilla-world0             # 锚点港随世界包不同；World1 覆盖文件只改 anchor_town
assets:
  old_map: { file: OldMap.dk4, ntex: 6 }
  old_map_still: { file: OldMapStill.dk4, ntex: 100 }
  remains_still: { file: Remains.dk4, ntex: 25 }
  ev_frame: { file: EvFrame.dk4 }
  search_ocean: { file: Search.DK4, ntex: 10, owner: ocean }
  ev_item: { file: EvItem.dk4, ntex: 39, owner: ui }   # 教会也用
stills_per_remain: 4             # [推断]
hd_frame: { w: 1920, h: 1080 }
# 七大海域注册在 config/world/major_seas.yaml，本表只引用 key

# 贤者地图：材料图，不是地点图
sage_maps:
  - { item: vanilla:thales_paper_map,    major_sea: vanilla:north_sea }      # 原版 155；+0x15 [推断]
  - { item: vanilla:cleobulus_cloth_map, major_sea: vanilla:mediterranean }  # 156
  - { item: vanilla:periander_stone_map, major_sea: vanilla:africa }         # 157
  - { item: vanilla:solon_leaf_map,      major_sea: vanilla:indian_ocean }   # 158
  - { item: vanilla:bias_coin_map,       major_sea: vanilla:se_asia }        # 159
  - { item: vanilla:chilon_bamboo_map,   major_sea: vanilla:east_asia }      # 160
  - { item: vanilla:pittacus_blade_map,  major_sea: vanilla:new_world }      # 161

# 地点地图：发现条件。target.key 未从代码钉死则 null；inferred_name_match 只是注释，引擎不读
place_maps:
  - item: vanilla:map_stonehenge         # 原版 162
    anchor_town: vanilla:amsterdam       # +0x18；W1 = stockholm
    target: { kind: remains, key: null }
    inferred_name_match: { remains: vanilla:stonehenge, strength: strong }   # 环状石柱 ↔ 巨石阵；英国南部（09 §2.5）
    discover: { needs_map_item: true, needs_puzzle: false, open_search_ex: true }   # 进遗迹后是否弹 SearchEx [未知]
  - item: vanilla:map_colosseum          # 163
    anchor_town: vanilla:hamburg         # W1 = istanbul
    target: { kind: remains, key: null }
    inferred_name_match: { remains: vanilla:colosseum, strength: strong }
  - item: vanilla:map_cave_village       # 164
    anchor_town: vanilla:athens
    target: { kind: village, key: null }   # 文案确认是村落；24 村无「岩窟」
    inferred_name_match: { remains: vanilla:cave_monastery, strength: medium }
    discover: { needs_map_item: true, open_search_ex: false }
  - item: vanilla:map_sahara             # 165
    anchor_town: vanilla:saint_george
    target: { kind: remains, key: null }
    inferred_name_match: { remains: vanilla:mud_mosque, strength: strong }   # 廷巴克图
  - { item: vanilla:map_imperial_mosque, anchor_town: vanilla:sofala,     target: { kind: remains, key: null } }   # 166
  - item: vanilla:map_mughal             # 167
    anchor_town: vanilla:basra
    target: { kind: remains, key: null }
    inferred_name_match: { remains: vanilla:taj_mahal, strength: strong }
  - item: vanilla:map_ancient_temple     # 168
    anchor_town: vanilla:jakarta
    target: { kind: remains, key: null }
    inferred_name_match: { remains: vanilla:angkor, strength: strong }
  - item: vanilla:map_beijing            # 169
    anchor_town: vanilla:seoul           # 汉城，不是遗迹下标
    target: { kind: remains, key: null }
    inferred_name_match: { remains: vanilla:forbidden_city, strength: strong }
  - item: vanilla:map_royal_tomb         # 170
    anchor_town: vanilla:hangzhou        # W1 = osaka
    target: { kind: remains, key: null }
    inferred_name_match: { remains: vanilla:muryeong_tomb, strength: medium }
  - item: vanilla:map_golden_temple      # 171
    anchor_town: vanilla:calicut         # W1 = malacca
    target: { kind: remains, key: null }
    inferred_name_match: { remains: vanilla:kinkaku, strength: strong }
  - { item: vanilla:map_ancient_city, anchor_town: vanilla:veracruz, target: { kind: remains, key: null } }   # 172
  - { item: vanilla:map_aztec,        anchor_town: vanilla:havana,   target: { kind: remains, key: null } }   # 173；W1 = seville
# 缺省 discover = { needs_map_item: true, needs_puzzle: false, open_search_ex: true }
# 把 inferred_name_match 升成 target.key 前，必须先在 exe / 事件脚本里钉到整数并升 version

ui:
  read_inscription:
    class: COldMapInfomationView
    window: CGetOldMapWindow
    strings:
      title_old_map: ui.discovery.old_map          # 古老地图
      read: ui.discovery.read_inscription          # 读字迹
      no_map: ui.discovery.no_map                  # 没有地图
      marked_pos: ui.discovery.marked_position     # 地图所示的位置
      unknown_marks: ui.discovery.question_marks   # ？？？？？？？
    frames:
      chrome: { asset: old_map, index: 1 }
      stills: { asset: old_map_still, index_formula: still_index }
  search_ex:
    fn: fn_140006429
    class_inferred: CItemMysteryHunting
    strings:
      ex1: ui.discovery.search_ex1                 # 你终于解开了传说的谜团
      ex2: ui.discovery.search_ex2                 # 并获得了古老财宝！
      end: ui.discovery.search_end                 # 探索结束！
      loot: ui.discovery.loot_title                # 获得的财宝
    rtc: [rcSearchEx1, rcSearchEx2, rcSearchEnd, rcSubTitle, rcItem]
    frames:
      win1: { asset: old_map, index: 2 }
      win2: { asset: old_map, index: 3 }
    modes: [1, 2, 3]                               # 语义 [未知]
  remains_interior:
    fn: fn_14001233c
    frame: { asset: remains_still, index_formula: vanilla_index }
    ev_frame: EvFrame.dk4
```

### 6.3 `config/discovery/puzzles.yaml`

```yaml
schema: dk4.discovery.puzzles
version: 1
pack: vanilla-world0

# 倒水不在此列
types:
  - key: vanilla:none
    solve: auto
  - key: vanilla:combine_halves
    inputs: 2
    consume_inputs: false          # [未知] 默认不消耗
  - key: vanilla:symbol_weave
    isa: unknown                   # 只有图案串
  - key: vanilla:search_ex
    stages: [ex1, ex2, end]
  - key: vanilla:mystery_hunt
    site: item_mystery_hunting
    match: unordered_set

puzzle_pairs:
  - key: vanilla:africa_indian_ocean
    type: combine_halves
    major_seas: [vanilla:africa, vanilla:indian_ocean]
    items: [vanilla:periander_stone_map, vanilla:solon_leaf_map]   # [推断] 原版 157+158
    evidence: mesfile_0x1e6ff
    reward:
      item: null                   # 对应海域的 kind 8 未钉哪一件
      gold: null
      title: null
  - key: vanilla:se_asia_east_asia
    type: combine_halves
    major_seas: [vanilla:se_asia, vanilla:east_asia]
    items: [vanilla:bias_coin_map, vanilla:chilon_bamboo_map]      # [推断] 原版 159+160
    evidence: mesfile_0x1e643
    reward: { item: null, gold: null, title: null }

related_unpaired:                  # 「有什么关系」但不是半图
  - { major_sea: vanilla:north_sea,     text: mesfile_0x1e6bb }
  - { major_sea: vanilla:mediterranean, text: mesfile_0x1e6bb }
  - { major_sea: vanilla:new_world,     text: mesfile_0x1e76b }

symbol_weave:
  - key: vanilla:tribal_pattern
    type: symbol_weave
    pattern: "○◇　﹡□　﹟□　﹢○　◇﹡"
    items_inferred: [vanilla:patterned_cloth, vanilla:cumbi_cloak]  # 原版 175、190 [推断]
    isa: unknown

mystery_hunt:
  consume_inputs: true             # [推断]
  recipes:                         # output 全部 [推断]；不想猜就改 null
    - { key: vanilla:slate_halves,       inputs: [vanilla:mystery_slate_top, vanilla:mystery_slate_bottom], output: vanilla:periander_stone_map }  # 176+183 → 157
    - { key: vanilla:bamboo_diagram,     inputs: [vanilla:tang_bamboo_craft, vanilla:bamboo_craft_diagram], output: vanilla:chilon_bamboo_map }    # 179+186 → 160
    - { key: vanilla:knife_sheath,       inputs: [vanilla:ritual_knife, vanilla:sun_pattern_sheath],        output: vanilla:pittacus_blade_map }   # 180+187 → 161
    - { key: vanilla:parchment_to_paper, inputs: [vanilla:old_parchment],        output: vanilla:thales_paper_map }     # 174 → 155
    - { key: vanilla:cloth_to_cloth_map, inputs: [vanilla:patterned_cloth],      output: vanilla:cleobulus_cloth_map }  # 175 → 156
    - { key: vanilla:lotus_to_leaf,      inputs: [vanilla:unwithering_lotus],    output: vanilla:solon_leaf_map }       # 177 → 158
    - { key: vanilla:coin_to_coin_map,   inputs: [vanilla:ancient_kingdom_coin], output: vanilla:bias_coin_map }        # 178 → 159
    # red_dye / brass_oil_lamp / kushan_platter / lotion_jar（181/182/184/185）：无配对，不写

# 进遗迹后的结算谜。遗迹未绑定时挂在 site 上
search_ex_puzzles:
  - key: vanilla:remains_generic_search_ex
    type: search_ex
    remains: null
    reward:
      item: null                   # 运行时按 major_sea 查 kind 8 [未知]
      gold: null
      title: null

rewards_kind8_by_major_sea:        # 查询表，不是自动掉落；按 major_sea key
  vanilla:
    vanilla:north_sea: vanilla:ullr_bow
    vanilla:mediterranean: vanilla:cambyses_crown
    vanilla:africa: vanilla:aksum_gold_seal
    vanilla:indian_ocean: vanilla:vedas
    vanilla:se_asia: vanilla:kediri_amulet
    vanilla:east_asia: vanilla:qin_palace_lamp
    vanilla:new_world: vanilla:crystal_skull
  pk:
    vanilla:north_sea: vanilla:hallstatt_offering_cart
    vanilla:mediterranean: vanilla:agamemnon_mask
    vanilla:africa: vanilla:ram_headed_sphinx
    vanilla:indian_ocean: vanilla:sargon_bust
    vanilla:se_asia: vanilla:trung_tracs_crown
    vanilla:east_asia: vanilla:shang_square_ding
    vanilla:new_world: vanilla:chimu_axe
```

`config/formulas/discovery.yaml`：

```yaml
schema: dk4.formulas.discovery
version: 2
# D3 形状（14-config-schemas README「公式层」）
oldmap_layout:
  impl: vanilla.oldmap_layout
  params:
    stills_per_remain: 4
    search_ex_button_slots: 3
    oldmap_flag_slots: 4
    remain_tex_equals_id: true
# 海上半径不在这里，见 formulas/ocean.yaml
```

文案全部走 `ui/strings.*.yaml`（`discovery.*` `remain.*`）。本文件 YAML 里的中文只作 `name` 注释。

## 7. 独立旋钮

| 旋钮 | 文件 | 改了不必动引擎 |
| --- | --- | --- |
| 遗迹条数 | `remains.yaml` 表长 | 是 |
| 默认已发现 / 挂哪座港 | `remains.yaml` / `towns.yaml` | 是 |
| 哪张地点地图揭示哪个点 | `old_maps.yaml` `target.key` | 外键校验失败则拒绝导入 |
| 半图对、是否消耗 | `puzzle_pairs` | 是 |
| 碎片配方与产出 | `mystery_hunt.recipes` | output 可长期 null |
| SearchEx 三句文案、帧下标 | `old_maps.yaml` `ui` | 是 |
| `stills_per_remain` | formulas | 改 4 不必改 C++ |
| kind 8 哪套当奖励 | `rewards_kind8_by_major_sea` | World0/1 换包 |
| 倒水是否存在 | **不在本表** | 城镇小游戏开关 |

加一种谜题 = 在 `types` 加 key + 一个 `solve` 插件。加一座遗迹 = `remains` 加行 + 可选 `place_maps` 行。禁止为了对齐 25 去编造遗址名。

## 8. 对外接口

只传 ID 与值。

查询：

```
Remains.def(remains_id) -> RemainsDef | none
Remains.discovered(remains_id) -> bool
Remains.flags(remains_id) -> FlagSet
Remains.at_town(town_id) -> remains_id | none    # 原版 STown+0x26（02）
Remains.town(remains_id) -> town_id | none        # 反查；位置 = LocatorRef(town, …)
Discover.can_discover(remains_id, fleet_id) -> { ok, reason }
OldMap.place_map(item_id) -> PlaceMapRow | none
OldMap.sage_map(item_id) -> SageMapRow | none
Puzzle.def(puzzle_id) -> PuzzleDef
Puzzle.solved(puzzle_id) -> bool
Puzzle.match_hunt(item_ids) -> recipe | none
Reward.kind8(major_sea_id, pack) -> item_id | none
```

命令：

```
World.mark_remains_discovered(remains_id, via, town_id?, fleet_id?, item_id?)   # 形状见 03 §8；items 用 via: item
World.clear_remains_discovered(remains_id)      # 仅导入回滚
Puzzle.open { puzzle_id | remains_id, type }
Puzzle.submit { puzzle_id, fleet_id, input }
OldMap.read_inscription { item_id | remains_id, fleet_id }
SearchEx.advance { remains_id }
```

事件：

```
RemainsDiscovered / VillageDiscovered         # 形状见 03 §8（world 发）
RemainsRevisited { remains_id, town_id, kind }   # town-life 据此跑仪式
UnlockRejected { item_id, reason }
PuzzleSolved { puzzle_id, remains_id? }
PuzzleBlocked { puzzle_id, reason: isa_unknown|need_other_half|... }
ItemsCombined { recipe_key, output_item_id }
RelicGranted { remains_id?, item_id, major_sea_id? }
SearchExAdvanced { remains_id, stage }
OldMapOpened { item_id }
```

原版入口是 town-life 发 `FacilityEntered { town_id, map_point_kind: 9|10|11 }`；world 订阅它（06 的订阅方含 world），在 `on_enter_town_map_point` 内查 `Remains.at_town`：未发现则置位、加首访影响力并发 `RemainsDiscovered`，已发现则发 `RemainsRevisited`（§4.3）。ocean 海上搜索命中遗迹是重构扩展（原版未见），同样走 world 置位。items 的 `UseItem` 对 kind 10 查 `old_maps.yaml` 的 `target`，不直接改遗迹位，而是发命令 `World.mark_*_discovered(id, via: item, item_id)`，事件由 world 发（旧 `UnlockRemain` / `UnlockVillage` 作废）。

不暴露 `CRemains*`、`CRemainsElement`、`SItem*`。

## 9. 未核实字段

| 项 | 依据 | 状态 |
| --- | --- | --- |
| 头文件 `NUM_REMAINS` 整数 | 注释 not recovered | 实例计数 25 `[确认]`（名字表） |
| 25 个遗迹的显示名 | exe `0x1405b2f80` | `[确认]` 见 09 |
| 坐标 | 挂港 `STown+0x26` | `[确认]`；木造教堂无港 |
| `REMAINSSTATE_BITS` 位 1–7 | u8，位 0 = 已发现 | `[未知]` |
| `fn_140010f46` / 首访加两次 | 反汇编 | 谓词 `[确认]` = `eScenario`（剧本 0 ×2，见 09）；加两次是否有意 `[未知]` |
| World0 何时开 9/10/11 位 | `fn_140001f87` 港数据更新 | `[推断]` 剧本 |
| `OldMapStill` 下标闭合式 | 反编译把对象指针乘进去了 | `[未知]`；配 `id*4+layer` |
| 12 张地点地图 → `remains_id` / `village_id` | 只有文案目标 | `[未知]` 禁止手填 |
| `+0x18` 锚点港在玩法里做什么（出发？最近港？） | 171 反例已排除「等于遗址」 | `[未知]` |
| 贤者 `+0x15` = 七大海域 | 0–6 双射 | `[推断]` |
| kind 8 菜单名 =「霸者之证」 | 8 词连着 kind 5–12 | `[推断]` |
| 哪一件 kind 8 从哪一座遗迹掉 | 无表 | `[未知]` |
| `+0x19` 8↔10 在 W0/W1 对调 | hex | `[未知]` |
| 半图是否消耗、是否要叠 still 层 | 只有文案 | `[未知]` |
| `symbol_weave` 的合法操作 | 只有图案串 | `[未知]` |
| 181/182/184/185 碎片用途 | 无配对句 | `[未知]` |
| 188/189/190 纪念品是否解谜钥匙 | 布钱诗句、斗篷织法 | `[未知]` |
| `fn_140006429` 模式 1/2/3 的玩法差 | 只有摆件循环 | `[未知]` |
| `CItemMysteryHunting` 是否就是 SearchEx 窗 | 地址相邻、共享 `+0x208` | `[推断]` |
| `CMpRemains` 内 kind 9/10/11 除奖励表外的差异 | 重访 `fn_14000eacf` 分三支仪式 | `[确认]`；仪式奖励细节归 town-life |
| 海上探索半径（世界单位） | 点击 16px 是 UI | `[未知]` 见 ocean |
| 开局是否已发现若干遗迹（剧本差） | 无 | `[未知]` |
| 称号与遗迹的 `requires` | 名声篇空表 | `[未知]` |
| `EvItem.dk4` 39 帧与 14 古宝+地图的对齐 | 只有 ntex | `[未知]` |
| 「来遗迹这里有事吗」是否遗迹内对话 | 与教会仪式同一块 | `[未知]` |

遗迹名与挂港已从 exe / 港口表钉死（09 §2.2）。12 张地点地图 → `remains_id` 仍无代码连接，`target.key` 保持 `null`。
