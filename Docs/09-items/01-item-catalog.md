# 道具目录（eItem）

配置落点：`config/items/kinds.yaml` `config/items/items.yaml`。效果与装备槽见 [02-equipment-effects.md](02-equipment-effects.md)。

本系统管 **231 件宝物/发现物/地图/船首像**（`eItem`）。不管 105 件交易货（`eTradingItem`）。两套 ID 禁止混用。

## 1. 职责边界

管：

- `eItem` 定义表：稳定 ID、key、种类、基础标价、图标/文案 key
- 舰队侧「是否持有」的唯一物库存（原版不堆叠）
- 发现物种进港口/村落变成特产品的 **数据声明**（真正改市况交给 economy）
- 地图类道具声明要解锁的遗迹/村落（真正改探索状态交给 world / ocean）
- 拼合碎片的配方表（神秘狩猎 UI 只读配方）

不管：

- 交易所货格、市价、特产品供给（`eTradingItem`，见 `04-economy`）
- 人物六维/技能公式（`03-characters` / `11-progression`）
- 船首像装上之后的航速、补给、炮击修正（`05-ships-fleet` 读本表效果，本系统不算船）
- 遗迹/村落是否已发现（`01-world`）
- 广场/教会文案（`ui/strings.*.yaml`）

ID 0「南瓜种子」是发现/种植道具，**不是**货舱货物。空货格哨兵是 `trading_item.empty = 106`，与本表无关。

## 2. 原版事实

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `NUM_ITEM = 231`，`INVALID_ITEM = -1` | [确认] | `srcdata/source/ItemID.h`，`GetFixItemProc` 断言 `0 <= eItem && eItem < NUM_ITEM` |
| World 表步长 `0x20`，World0 VA `0x1800e1ad0`，World1 VA `0x1800e1ae0` | [确认] | `World0_item.c` / `World1_item.c`，`DllMain.cpp`，`Item.h` 注释 |
| `SItem`：`const char *name` + `rest[0x18]` | [确认] | `srcdata/include/world.h`，`DllMain.cpp` |
| 名称指针占记录前 8 字节（x64）；南瓜种子随后 `u32 = 2000`，再两个 `s32 = -1` | [确认] | 用户指定解码 + `items_world0.json` hex `d0070000ffffffff` |
| 名称 0–230 与 `Docs/_data/items.tsv` 一致；World1 中文名与 World0 相同 | [确认] | `ItemID.h` 注释、`items.tsv`、World1 抽出 |
| 运行时库存按 231 槽扫，不按堆叠 | [确认] | `CItemElement` 循环 `eItem >= 0xe7`（231），构造 `0xe7` 个步长 `0x40` 的对象 |
| 装备/选择空槽写 `0xe8 = 232`，不是 `-1`，也不是 `0` | [确认] | `CSelectItemView` 填 `0xe8`；`fn_140012af8` / `fn_140006811` / `fn_140012657` 与 `0xe8` 比较 |
| `NUM_ITEMKIND` 头文件未写出整数 | [确认] | `ItemKindID.h`：`NUM_ITEMKIND, /* integer not recovered */` |
| 种类菜单把 `eItemKind` 从 0 扫到 `< 0xd`（13 档），选项 1..13 | [推断] | `CItemKindMenuData`：`if (eItemKind >= 0xd)`；`vfunc_01` 里 `s38 >= 0xe` 后 `s38 -= 1` |
| 记录 `+0x14` 取值恰好 0–12，覆盖全部 231 条 | [推断] | 对 `items_world0.json` 全表统计；与上条 13 档对齐，当作 `eItemKind` |
| `+0x08` 的 u32 是基础标价 | [推断] | 南瓜种子 2000；全表 100–420000；卖出路径 `fn_14000f4ed` 用 `fn_140007423` 取出后乘折扣 |
| 选择界面有 6 个 tag，不是种类数 | [确认] | `NUM_SELECTITEMTAG = 6`，`CSelectItemView` `eSelectItemTag >= 6` |
| 卖出/装备筛选里 `eItemTag` 初值 7 | [确认] | `fn_14000f4ed`、`CPersonInformationView::vfunc_13`：`eItemTag = 7` |
| `ITEMSTATE_BITS` 存在，宽度未还原 | [确认] | `constants.json` / `asserts.txt`：`0 <= eData && eData < _GetBitTotal_BYTE2DWORD(sizeof(ITEMSTATE_BITS))` |
| 文案 2055 条里有逐件说明（SC） | [确认] | `mesfile_sc.txt` 约 `0x19783` 起：南瓜种子、武器、船首像、地图 |
| 把发现物交给城市可变成特产品 | [确认] | mesfile `0x7737`：「如果有那%s，%s会成为这个城市的特产品」 |
| 地图说明指向遗迹/村落/首都航线 | [确认] | mesfile `0x1bf07`–`0x1c123`：环状石柱、竞技场、岩窟村、撒哈拉、清真寺、莫卧儿、寺院、北京、王陵、黄金寺、古代城市、阿斯帝加 |
| 拼合类道具在文案里成对出现 | [推断] | 「神秘的石板上半部/下半部」「竹艺品的组合说明图」「太阳图纹的刀鞘」 |
| 神秘狩猎是港口里的道具 UI | [确认] | `CItemMysteryHunting` → `CSurfaceSpriteView`；与 `CItemInformationView` / `CSelectItemView` 同目录 |
| 船首像不能在海上换 | [确认] | mesfile `0x3753`；造船厂 `0xc693` |
| World0 / World1 同 231 名，126 条 `rest` 字节不同 | [确认] | 对两份 `World*_item.c` 比较；偏移 +0x0C..+0x13、+0x15、+0x18、+0x19 有差异 |

`CItem.cpp` 的 `vfunc_05` 只证明运行时会 `GetFixItemProc` 后 `rep movsb` 拷 32 字节，再 `lstrcpyA` 名字。`rest` 字段名原版未恢复，禁止把推断写成确认。

## 3. 实体与 ID

| 实体 | 原版计数 | 空/非法 | 运行时 |
| --- | --- | --- | --- |
| `eItem` | 231（0–230） | `INVALID_ITEM = -1` | 读表长 |
| 装备/库存空槽 | — | `232`（`0xe8`） | `caps.yaml`：`item_empty_slot` |
| `eItemKind` | 13（0–12）`[推断]` | `INVALID_ITEMKIND = -1` | 读 `kinds.yaml` 表长 |
| `eSelectItemTag` | 6 | | UI 筛选，不是种类 |
| `eItemTag` | 循环初值 7 | | 卖出/装备过滤，语义 `[未知]` |
| `ePersonEquip` | 5 | `INVALID_PERSONEQUIP = -1` | 见装备文档 |
| `eFigureHead` | 头文件无整数；本表 kind=5 共 12 件 | `INVALID_FIGUREHEAD = -1` | 船侧槽，ID 仍是 `item_id` |
| `eTradingItem` | 105 | 空格 `106` | **另一张表** |

禁止：

- 用 `0` 当空槽（0 是南瓜种子）
- 用 `106` 当宝物空槽（那是交易品空货格）
- 用 `-1` 当已装备槽的占位（原版装备字节是无符号 `0xe8`）

模组追加 `id >= 1000` 或独立命名空间，不覆盖 0–230。

### 3.1 种类（`+0x14`，[推断]）

依据：该字节 0–12 穷尽 231 条 + `CItemKindMenuData` 13 档。中文名只用来给种类起 key，不参与判定。

| kind | key | 件数 | 名称特征 | 引擎用途（数据，不写死 ID） |
| --- | ---: | ---: | --- | --- |
| 0 | `plant_discovery` | 20 | 种子/苗/树/工艺书，id 0–19 | `plant_in_village` / 特产品发现 |
| 1 | `voyage_charm` | 4 | 猫、莱姆、祈祷、华陀中药 | 航海护佑/消耗 |
| 2 | `weapon` | 31 | 剑刀枪斧；含 PK 后加的佩剑等 | 人物装备槽 0 |
| 3 | `armor` | 30 | 盾盔甲袍；含祝福之盾、普利德温 | 人物装备槽 1 |
| 4 | `person_gear` | 62 | 航海工具、书籍、岗位用具、宝石 | 人物装备槽 2–4 |
| 5 | `figurehead` | 12 | `*像`，id 114–123、199–200 | 船首像，不进人物槽 |
| 6 | `fragment` | 14 | 羊皮纸、石板半片、刀鞘等，标价 100 | 神秘狩猎配方材料 |
| 7 | `gift` | 21 | 工艺品/赠礼，id 127–147 | 赠礼；不进人物槽 |
| 8 | `relic` | 14 | 乌勒尔弓箭…水晶头骨，及哈尔舒塔一套 | 遗迹宝物 |
| 9 | `sage_map` | 7 | 泰勒斯…比塔科史，七张材料地图 | 合成/剧情地图材料 |
| 10 | `place_map` | 12 | 「去…的地图」 | `unlock_remain` / `unlock_village` |
| 11 | `plot_set` | 3 | 布钱、天文仪、斗篷 | 剧情三件，标价 100 |
| 12 | `treasure_map` | 1 | 藏宝图 | 藏宝事件 |

`冶金丸` 的 `+0x14 = 2`，按武器走槽，不按名字改种类。

### 3.2 全表（231）

`base_price` 来自 `+0x08`，[推断]。`kind` 来自 `+0x14`，[推断]。名称来自 `ItemID.h` / `items.tsv`，[确认]。

| id | key | name | kind | kind_key | base_price |
| ---: | --- | --- | ---: | --- | ---: |
| 0 | pumpkin_seed | 南瓜种子 | 0 | plant_discovery | 2000 |
| 1 | tomato_seedling | 番茄苗 | 0 | plant_discovery | 2200 |
| 2 | banana_tree | 香蕉树 | 0 | plant_discovery | 2400 |
| 3 | beehive | 蜜蜂窝 | 0 | plant_discovery | 1200 |
| 4 | baby_shark | 小鲨鱼 | 0 | plant_discovery | 9000 |
| 5 | peppercorn | 胡椒粒 | 0 | plant_discovery | 8500 |
| 6 | clove_bud | 丁香粒 | 0 | plant_discovery | 8600 |
| 7 | cinnamon_tree | 肉桂树 | 0 | plant_discovery | 7800 |
| 8 | bell_pepper | 甜椒果 | 0 | plant_discovery | 6500 |
| 9 | coffee_tree | 咖啡树 | 0 | plant_discovery | 6700 |
| 10 | tea_tree | 茶树 | 0 | plant_discovery | 7500 |
| 11 | cacao_pod | 可可果 | 0 | plant_discovery | 6800 |
| 12 | tobacco_seedling | 烟草苗 | 0 | plant_discovery | 7200 |
| 13 | alchemy_book | 炼金术书 | 0 | plant_discovery | 12500 |
| 14 | glassmaking_dictionary | 玻璃的制作辞典 | 0 | plant_discovery | 8000 |
| 15 | silkworm | 蚕 | 0 | plant_discovery | 7700 |
| 16 | loom | 织布机 | 0 | plant_discovery | 11000 |
| 17 | pearl_oyster | 珍珠贝 | 0 | plant_discovery | 9000 |
| 18 | tcm_compendium | 中医宝典 | 0 | plant_discovery | 12000 |
| 19 | mangosteen_seed | 山竹果种子 | 0 | plant_discovery | 3000 |
| 20 | golden_cat | 金黄色的猫 | 1 | voyage_charm | 15000 |
| 21 | lime_drops | 莱姆的滴汁 | 1 | voyage_charm | 10000 |
| 22 | hermes_prayer | 赫尔墨斯的祈祷 | 1 | voyage_charm | 18000 |
| 23 | hua_tuo_medicine | 华陀的中药 | 1 | voyage_charm | 12000 |
| 24 | black_twinblade | 漆黑双刃长剑 | 2 | weapon | 18000 |
| 25 | laocoon_sword | 拉奥孔剑 | 2 | weapon | 18400 |
| 26 | lohengrin_rapier | 罗恩格林佩剑 | 2 | weapon | 66000 |
| 27 | crescent_scimitar | 残月弯刀 | 2 | weapon | 44000 |
| 28 | hero_crescent_blade | 勇者新月刀 | 2 | weapon | 28000 |
| 29 | golden_saber | 黄金军刀 | 2 | weapon | 30000 |
| 30 | courage_sword | 勇气宝剑 | 2 | weapon | 24000 |
| 31 | indian_glass_sword | 印度琉璃剑 | 2 | weapon | 16000 |
| 32 | pirate_hunter_twinblade | 荡寇双刃剑 | 2 | weapon | 56000 |
| 33 | loyalty_guandao | 忠诚偃月刀 | 2 | weapon | 29200 |
| 34 | fallen_angel_dagger | 堕落天使短剑 | 2 | weapon | 42000 |
| 35 | silver_claymore | 银光双手剑 | 2 | weapon | 35200 |
| 36 | blood_crescent | 浴血月牙刀 | 2 | weapon | 64000 |
| 37 | judas_demon_sword | 犹大的魔剑 | 2 | weapon | 100000 |
| 38 | zhaoyun_spear | 赵子龙的枪 | 2 | weapon | 132000 |
| 39 | requiem_sword | 赎罪镇魂剑 | 2 | weapon | 22400 |
| 40 | redhead_pirate_sword | 红发女海盗的宝剑 | 2 | weapon | 57000 |
| 41 | minotaur_axe | 米诺陶洛斯的大斧 | 2 | weapon | 47200 |
| 42 | ogre_cleaver | 鬼怪菜刀 | 2 | weapon | 10000 |
| 43 | tlaloc_knife | 托拉洛克的小刀 | 2 | weapon | 31400 |
| 44 | halberd | 斧枪 | 2 | weapon | 67000 |
| 45 | kublai_blade | 忽必烈的大刀 | 2 | weapon | 87000 |
| 46 | ares_lance | 战神的圣枪 | 2 | weapon | 170000 |
| 47 | excalibur | 厄克斯卡里巴圣剑 | 2 | weapon | 250000 |
| 48 | muramasa | 妖刀村正 | 2 | weapon | 100000 |
| 49 | azure_oak_shield | 苍蓝橡木盾 | 3 | armor | 16000 |
| 50 | armadillo_shield | 犰狳皮盾 | 3 | armor | 9500 |
| 51 | silver_helm | 银盔 | 3 | armor | 13000 |
| 52 | phoenix_helm | 不死鸟的头盔 | 3 | armor | 52000 |
| 53 | elephant_turtle_shield | 大象龟的盾 | 3 | armor | 14200 |
| 54 | sprite_robe | 精灵服 | 3 | armor | 18000 |
| 55 | bladebreaking_gi | 断刃之武艺服 | 3 | armor | 19500 |
| 56 | cool_mail | 凉爽的铠衣 | 3 | armor | 41000 |
| 57 | kanem_hero_shield | 卡涅姆勇者之盾 | 3 | armor | 20000 |
| 58 | fanatic_knight_armor | 狂徒骑士铠甲 | 3 | armor | 25000 |
| 59 | crimson_chain_mail | 深红连环神甲 | 3 | armor | 48000 |
| 60 | timur_war_robe | 铁木尔的锁战袍 | 3 | armor | 50000 |
| 61 | peacock_robe | 孔雀袍 | 3 | armor | 32000 |
| 62 | tula_warrior_helm | 图拉战士的头盔 | 3 | armor | 30000 |
| 63 | ninja_blacks | 忍者的黑衣 | 3 | armor | 65535 |
| 64 | amber_armor | 琥珀色铠甲 | 3 | armor | 70000 |
| 65 | jaguar_spirit_mail | 附迦戈神灵的护身衣 | 3 | armor | 62000 |
| 66 | corrupt_demon_armor | 背德魔铠 | 3 | armor | 100000 |
| 67 | empress_robe | 女皇的长袍 | 3 | armor | 84000 |
| 68 | noritsune_greaves | 教经的铠甲护腿 | 3 | armor | 95000 |
| 69 | attila_set | 阿提拉套装 | 3 | armor | 180000 |
| 70 | medusa_shield | 美杜沙的盾 | 3 | armor | 125000 |
| 71 | saladin_silver_armor | 撒拉丁的银铠 | 3 | armor | 220000 |
| 72 | charles_martel_armor | 查理·马特的铠甲 | 3 | armor | 270000 |
| 73 | minerva_shield | 密涅瓦的盾 | 3 | armor | 88000 |
| 74 | magic_leather_gloves | 魔法皮手套 | 4 | person_gear | 90000 |
| 75 | sprite_hawser | 精灵粗绳 | 4 | person_gear | 120000 |
| 76 | milky_way_chart | 天河星图 | 4 | person_gear | 50000 |
| 77 | zodiac_globe | 星座天体仪 | 4 | person_gear | 62500 |
| 78 | horace_poems | 贺拉提斯诗集 | 4 | person_gear | 1500 |
| 79 | arabian_nights | 天方夜谭 | 4 | person_gear | 2000 |
| 80 | world_anecdotes | 世界奇闻集 | 4 | person_gear | 3500 |
| 81 | golden_compass | 黄金圆规 | 4 | person_gear | 50000 |
| 82 | pocket_watch | 怀表 | 4 | person_gear | 5000 |
| 83 | locke_sailing_guide | 洛克的驾船指南书 | 4 | person_gear | 90000 |
| 84 | heaven_bracelet | 天佑手镯 | 4 | person_gear | 240000 |
| 85 | virtue_staff | 导善手杖 | 4 | person_gear | 420000 |
| 86 | warrior_ocarina | 战士小鹅笛 | 4 | person_gear | 82000 |
| 87 | talking_beak | 能言善辩的鸟嘴 | 4 | person_gear | 75000 |
| 88 | travels_of_marco_polo | 东方见闻录 | 4 | person_gear | 10000 |
| 89 | war_drum | 战鼓 | 4 | person_gear | 2500 |
| 90 | poseidon_roar | 波塞冬的怒吼 | 4 | person_gear | 110000 |
| 91 | gallic_wars | 高卢战记 | 4 | person_gear | 8000 |
| 92 | alexander_anabasis | 亚历山大远征记 | 4 | person_gear | 12000 |
| 93 | miracle_bomb_necklace | 奇迹的炸弹项链 | 4 | person_gear | 140000 |
| 94 | gunpowder_scroll | 精制火药秘笺 | 4 | person_gear | 50000 |
| 95 | demon_piercing_arrow | 射穿魔鬼的箭 | 4 | person_gear | 320000 |
| 96 | lion_tooth_saw | 狮牙锯 | 4 | person_gear | 65000 |
| 97 | phidias_chisel | 菲狄亚斯的凿子 | 4 | person_gear | 170000 |
| 98 | eversharp_plane | 不用磨刀的刨子 | 4 | person_gear | 30000 |
| 99 | herophilus_medical_book | 赫洛菲洛斯的医书 | 4 | person_gear | 97000 |
| 100 | da_vinci_anatomy | 达·芬奇的人体解剖图 | 4 | person_gear | 20000 |
| 101 | canon_of_medicine | 医学典范 | 4 | person_gear | 62000 |
| 102 | hestia_pot | 赫斯提亚的锅 | 4 | person_gear | 300000 |
| 103 | pink_apron | 粉红色的围裙 | 4 | person_gear | 7000 |
| 104 | gauze_mask | 带纱布的口罩 | 4 | person_gear | 2500 |
| 105 | fine_boots | 考究的长统鞋 | 4 | person_gear | 95000 |
| 106 | gregory_crown | 格里高利的皇冠 | 4 | person_gear | 235000 |
| 107 | illustrated_bible | 有插图的圣经 | 4 | person_gear | 20000 |
| 108 | ancient_cross | 古代十字架 | 4 | person_gear | 60000 |
| 109 | art_of_war | 孙子兵法 | 4 | person_gear | 58000 |
| 110 | gupta_sacred_beast | 古伏塔的灵兽 | 4 | person_gear | 220000 |
| 111 | hannibal_commentaries | 汉尼拔战记 | 4 | person_gear | 10000 |
| 112 | kinokuniya_abacus | 纪伊国屋的算盘 | 4 | person_gear | 20000 |
| 113 | vissas_scale | 维萨斯的天秤 | 4 | person_gear | 195000 |
| 114 | eagle_figurehead | 大鹫像 | 5 | figurehead | 58000 |
| 115 | piglet_figurehead | 小猪像 | 5 | figurehead | 36000 |
| 116 | orca_figurehead | 虎鲸像 | 5 | figurehead | 50000 |
| 117 | white_whale_figurehead | 白鲸像 | 5 | figurehead | 45000 |
| 118 | dragon_figurehead | 龙像 | 5 | figurehead | 60000 |
| 119 | dolphin_figurehead | 海豚像 | 5 | figurehead | 49000 |
| 120 | maiden_figurehead | 处女像 | 5 | figurehead | 63000 |
| 121 | devil_figurehead | 魔鬼像 | 5 | figurehead | 20000 |
| 122 | king_figurehead | 王者像 | 5 | figurehead | 70000 |
| 123 | madonna_figurehead | 圣母像 | 5 | figurehead | 80000 |
| 124 | compass | 罗盘 | 4 | person_gear | 5500 |
| 125 | sextant | 六分仪 | 4 | person_gear | 2500 |
| 126 | aristarchus_telescope | 阿里斯塔克斯望远镜 | 4 | person_gear | 90000 |
| 127 | frozen_rose | 冰冻的玫瑰 | 7 | gift | 1250 |
| 128 | rainbow_glass_bead | 彩虹玻璃珠 | 7 | gift | 1500 |
| 129 | gold_dust | 沙金 | 7 | gift | 5000 |
| 130 | bread_mortar | 制作面包的石臼 | 7 | gift | 1000 |
| 131 | huizong_painting | 徽宗的北宋画 | 7 | gift | 35000 |
| 132 | fairy_silk | 仙女的彩绸 | 7 | gift | 50000 |
| 133 | sappho_poems | 萨佛诗集 | 7 | gift | 5000 |
| 134 | wonder_jar | 惊叹之壶 | 7 | gift | 30000 |
| 135 | venus_de_milo | 米罗的维纳斯 | 7 | gift | 80000 |
| 136 | stained_glass_flower | 彩色玻璃小花 | 7 | gift | 2000 |
| 137 | silla_gold_crown | 新罗的金冠 | 7 | gift | 55000 |
| 138 | ceramic_earrings | 陶瓷耳环 | 7 | gift | 9000 |
| 139 | jade_bead | 翡翠大珠 | 7 | gift | 7500 |
| 140 | oath_ring_play | 誓言戒指的剧本 | 7 | gift | 8500 |
| 141 | shosoin_pitcher | 正仓院的水壶 | 7 | gift | 52000 |
| 142 | black_glass_bowl | 黑琉璃碗 | 7 | gift | 7000 |
| 143 | sunflower_carpet | 大花朵的剌绣毛毯 | 7 | gift | 7500 |
| 144 | snow_robe | 画细雪的长袍 | 7 | gift | 5000 |
| 145 | goryeo_celadon_censer | 高丽青瓷香炉 | 7 | gift | 12000 |
| 146 | fine_magnifier | 高级放大镜 | 7 | gift | 10000 |
| 147 | rainbow_parrot | 七色鹦鹉 | 7 | gift | 13000 |
| 148 | ullr_bow | 狩猎之神乌勒尔的弓箭 | 8 | relic | 90000 |
| 149 | cambyses_crown | 肯毕西斯的王冠 | 8 | relic | 110000 |
| 150 | aksum_gold_seal | 阿克斯姆王的金印 | 8 | relic | 140000 |
| 151 | vedas | 吠陀本集 | 8 | relic | 80000 |
| 152 | kediri_amulet | 库德里的永久护符 | 8 | relic | 120000 |
| 153 | qin_palace_lamp | 秦始皇的长信宫灯 | 8 | relic | 100000 |
| 154 | crystal_skull | 水晶头骨 | 8 | relic | 95000 |
| 155 | thales_paper_map | 泰勒斯的纸地图 | 9 | sage_map | 1000 |
| 156 | cleobulus_cloth_map | 库雷欧布罗的布地图 | 9 | sage_map | 1000 |
| 157 | periander_stone_map | 贝利安德罗的石地图 | 9 | sage_map | 1000 |
| 158 | solon_leaf_map | 梭伦的叶地图 | 9 | sage_map | 1000 |
| 159 | bias_coin_map | 毕阿斯的货币地图 | 9 | sage_map | 1000 |
| 160 | chilon_bamboo_map | 科以伦的竹地图 | 9 | sage_map | 1000 |
| 161 | pittacus_blade_map | 比塔科史的刀刃地图 | 9 | sage_map | 1000 |
| 162 | map_stonehenge | 去环状石柱遗址的地图 | 10 | place_map | 500 |
| 163 | map_colosseum | 去古代竞技场的地图 | 10 | place_map | 700 |
| 164 | map_cave_village | 去岩窟村庄的地图 | 10 | place_map | 1500 |
| 165 | map_sahara | 撒哈拉大沙漠地图 | 10 | place_map | 1500 |
| 166 | map_imperial_mosque | 去帝王清真寺的地图 | 10 | place_map | 2000 |
| 167 | map_mughal | 莫卧儿帝国地图 | 10 | place_map | 1000 |
| 168 | map_ancient_temple | 去古代寺院地图 | 10 | place_map | 1000 |
| 169 | map_beijing | 去北京的地图 | 10 | place_map | 1000 |
| 170 | map_royal_tomb | 去王陵的地图 | 10 | place_map | 2500 |
| 171 | map_golden_temple | 去黄金寺的地图 | 10 | place_map | 3000 |
| 172 | map_ancient_city | 去古代城市遗址的地图 | 10 | place_map | 1000 |
| 173 | map_aztec | 阿斯帝加王国彩绘地图 | 10 | place_map | 2000 |
| 174 | old_parchment | 老旧的羊皮纸 | 6 | fragment | 100 |
| 175 | patterned_cloth | 画有图案的布 | 6 | fragment | 100 |
| 176 | mystery_slate_top | 神秘的石板上半部 | 6 | fragment | 100 |
| 177 | unwithering_lotus | 不枯萎的莲叶 | 6 | fragment | 100 |
| 178 | ancient_kingdom_coin | 古代王国的钱币 | 6 | fragment | 100 |
| 179 | tang_bamboo_craft | 唐代的竹艺品 | 6 | fragment | 100 |
| 180 | ritual_knife | 典礼用的小刀 | 6 | fragment | 100 |
| 181 | red_dye | 红色的染料 | 6 | fragment | 100 |
| 182 | brass_oil_lamp | 黄铜制煤油灯 | 6 | fragment | 100 |
| 183 | mystery_slate_bottom | 神秘的石板下半部 | 6 | fragment | 100 |
| 184 | kushan_platter | 贵霜朝的大盘 | 6 | fragment | 100 |
| 185 | lotion_jar | 装有乳液的壶 | 6 | fragment | 100 |
| 186 | bamboo_craft_diagram | 竹艺品的组合说明图 | 6 | fragment | 100 |
| 187 | sun_pattern_sheath | 太阳图纹的刀鞘 | 6 | fragment | 100 |
| 188 | gilt_spade_coin | 金铜的布钱 | 11 | plot_set | 100 |
| 189 | muezzin_astrolabe | 莫阿丁的天文观测仪 | 11 | plot_set | 100 |
| 190 | cumbi_cloak | 昆毕制成的斗篷 | 11 | plot_set | 100 |
| 191 | treasure_map | 藏宝图 | 12 | treasure_map | 100 |
| 192 | hallstatt_offering_cart | 哈尔舒塔的供品车 | 8 | relic | 100000 |
| 193 | agamemnon_mask | 阿葛曼侬的面具 | 8 | relic | 100000 |
| 194 | ram_headed_sphinx | 羊头狮身像 | 8 | relic | 100000 |
| 195 | sargon_bust | 萨尔贡的头像 | 8 | relic | 100000 |
| 196 | trung_tracs_crown | 琼恰克的王冠 | 8 | relic | 100000 |
| 197 | shang_square_ding | 殷代方形鼎 | 8 | relic | 100000 |
| 198 | chimu_axe | 契慕的斧头 | 8 | relic | 100000 |
| 199 | monster_fish_figurehead | 怪鱼像 | 5 | figurehead | 80000 |
| 200 | admiral_figurehead | 提督像 | 5 | figurehead | 65000 |
| 201 | flamingo_feather_shoes | 红鹤羽毛鞋 | 4 | person_gear | 72000 |
| 202 | babangida_sword | 巴班吉达的佩剑 | 2 | weapon | 51000 |
| 203 | wavy_trance_sword | 恍惚的波浪形剑 | 2 | weapon | 89000 |
| 204 | instant_kill_jambiya | 瞬杀阿拉伯短剑 | 2 | weapon | 81000 |
| 205 | blessing_shield | 祝福之盾 | 3 | armor | 78000 |
| 206 | overlord_helm | 霸王的头盔 | 3 | armor | 57000 |
| 207 | scipio_armor | 思奇比奥的铠甲 | 3 | armor | 105000 |
| 208 | lion_pupil_gem | 狮子瞳 | 4 | person_gear | 228000 |
| 209 | black_chalcedony | 黑玉髓 | 4 | person_gear | 80000 |
| 210 | boulder_opal | 砾背蛋白石 | 4 | person_gear | 70000 |
| 211 | cursed_seal_sword | 咒印剑 | 2 | weapon | 68000 |
| 212 | serpentine_mask | 蛇纹石假面 | 3 | armor | 60000 |
| 213 | ancient_rite_book | 古代仪典 | 4 | person_gear | 72000 |
| 214 | gold_coins | 金币 | 4 | person_gear | 50000 |
| 215 | gold_ingot | 金块 | 4 | person_gear | 100000 |
| 216 | pirate_badge | 海盗徽章 | 4 | person_gear | 40000 |
| 217 | star_dust_crystal | 星粉水晶 | 4 | person_gear | 90000 |
| 218 | phantom_rutile | 幻影金红石 | 4 | person_gear | 96000 |
| 219 | metallurgy_pill | 冶金丸 | 2 | weapon | 50000 |
| 220 | charm_ruin_statue | 魅惑与毁灭之像 | 4 | person_gear | 25000 |
| 221 | dragon_horn | 龙角 | 4 | person_gear | 80000 |
| 222 | prydwen | 普利德温 | 3 | armor | 90000 |
| 223 | exorcist_red_rose | 破魔红蔷薇 | 2 | weapon | 70000 |
| 224 | pure_gold_watch | 纯金怀表 | 4 | person_gear | 80000 |
| 225 | crimson_ring | 真红指环 | 4 | person_gear | 64000 |
| 226 | achaemenid_goblet | 阿契美尼斯的高脚杯 | 4 | person_gear | 96000 |
| 227 | rose_ornament_cup | 玫瑰花饰杯 | 4 | person_gear | 110000 |
| 228 | madoka | 玛多加 | 4 | person_gear | 40000 |
| 229 | niastika | 尼亚斯蒂卡 | 4 | person_gear | 40000 |
| 230 | love_potion_vial | 小瓶爱情灵药 | 4 | person_gear | 198000 |

## 4. 算法 / 公式

本系统几乎没有战斗公式。库存与使用规则如下。参数进 `caps.yaml` / `formulas/`，禁止在引擎里写 `231`、`232`、`13`。

### 4.1 唯一物库存

```
owned[item_id] ∈ {0, 1}     # vanilla unique_stack = 1
0 <= item_id < len(items)
empty_slot_id = caps.item_empty_slot   # vanilla 232
```

原版 `CItemElement` 为每个 `eItem` 留一个 `CItem` 对象。重构后仍按「持有/未持有」；若模组把 `unique_stack` 调大，引擎按表长与堆叠上限循环，不改代码。

### 4.2 种类 → 可装备目标

```
target = kinds[item.kind].equip_target
# person_slot_group | ship_figurehead | none | consume
```

原版内容包默认：

| kind_key | equip_target | 说明 |
| --- | --- | --- |
| weapon | `person_slot_group: weapon` | 槽 0 |
| armor | `person_slot_group: armor` | 槽 1 |
| person_gear | `person_slot_group: accessory` | 槽 2–4 |
| figurehead | `ship_figurehead` | 造船厂；海上拒绝 |
| 其余 | `none` 或 `consume` | 地图/种植/拼合走命令，不进槽 |

判定只看 `kind`，不看 `item_id`。

### 4.3 发现物 → 特产品

mesfile 把「拿出某件宝物」和「某城出现特产品」连在一起。原版实现散在事件与城镇逻辑里。重构只允许：

```
on UseItem(item_id) where effects contains plant_in_village:
    emit PlantSpecialProduct { town_id or village_id, trading_item_id, item_id }
```

`town_id` / `trading_item_id` 写在该道具的 effect 列表。引擎不得 `if item_id == 5: 卡利卡特出胡椒`。

`+0x18` 在多种植物上等于一个合法 `eTown`（胡椒 42 卡利卡特、可可 65 委拉克鲁斯、蚕 57 汉城、织布机 0 伦敦）。值为 `98`（`NUM_TOWN+1`）时当作「未绑港口」哨兵 `[推断]`。

### 4.4 地点地图 → 遗迹/村落

```
on UseItem(item_id) where effects contains unlock_remain | unlock_village:
    emit UnlockRemain { remain_id } / UnlockVillage { village_id }
    ocean/world 订阅后改探索状态
```

文案已写明环状石柱、岩窟村等。`+0x18` 对 kind 10 也是 0–96 内的 `eTown`（例如去北京的地图 → 57 汉城），更像航线锚点而不是遗迹下标 `[推断]`。遗迹 ID 尚未从 `CRemains` 钉死，YAML 先留 `remain_key` / `unknown_bytes`。

### 4.5 神秘狩猎

`CItemMysteryHunting` 是港口 UI。配方进 `config/items/recipes.yaml`：

```
inputs: [item_id...]
output: item_id
consume_inputs: true
```

原版成对名称（石板上下、竹艺+图解、小刀+刀鞘）只作原版包的实例，不是引擎规则。

卖出折扣见装备文档的 `item_price` 公式。目录表只存 `base_price`。

## 5. 耦合分析

原版硬编码：

- `GetFixItemProc` 断言 `NUM_ITEM=231`，加道具必须改 DLL
- 人物装备用 `+0x14` 的 2/3/4 分流到 5 个槽（`fn_140012657`）
- 卖出 `fn_14000f4ed` 直接 `fn_14000e1ce(person, 4)` 取人物数值乘价
- 船首像与人物宝物共享 `eItem` 空间，造船厂和人物装备各自筛 kind
- 地图/种植效果写在事件脚本与设施里，按道具名分支
- UI 字符串、图标（`Item32.dk4` / `Item56.dk4` / `EvItem.dk4`）和逻辑表绑在一起

重构切断点：本系统只产出 `item_id` + 效果列表。人物、船、世界、经济只读 ID 与 `ModifierSet` / 事件。

## 6. 解耦后的配置

```yaml
# config/items/kinds.yaml
schema: dk4.items.kinds
version: 1
kinds:
  - id: 0
    key: plant_discovery
    name: 特产品发现物
    equip_target: none
  - id: 2
    key: weapon
    name: 武器
    equip_target: person_slot_group
    slot_group: weapon
  - id: 3
    key: armor
    name: 防具
    equip_target: person_slot_group
    slot_group: armor
  - id: 4
    key: person_gear
    name: 人物用具
    equip_target: person_slot_group
    slot_group: accessory
  - id: 5
    key: figurehead
    name: 船首像
    equip_target: ship_figurehead
  - id: 10
    key: place_map
    name: 地点地图
    equip_target: none
```

```yaml
# config/items/items.yaml
schema: dk4.items.items
version: 1
items:
  - id: 0
    key: pumpkin_seed
    name: 南瓜种子
    name_i18n: item.pumpkin_seed.name
    desc_i18n: item.pumpkin_seed.desc
    kind: 0
    base_price: 2000
    icon: item56/0
    effects:
      - op: plant_in_village
        # 原版 +0x18=98 → 未绑具体港；由事件指定目标
        trading_item_key: pumpkin   # 外键进 economy，禁止引擎猜
    unknown_bytes: [0x00, 0x08, 0x18, 0x00, 0x62, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]

  - id: 5
    key: peppercorn
    name: 胡椒粒
    kind: 0
    base_price: 8500
    effects:
      - op: plant_in_village
        town_id: 42          # 卡利卡特；+0x18=42 [推断]
        trading_item_key: pepper

  - id: 24
    key: black_twinblade
    name: 漆黑双刃长剑
    kind: 2
    base_price: 18000
    effects:
      - op: stat_mod          # 效果表，见 02
        amount: 24            # +0x17 [推断]
    unknown_u32_0c: 0x4eb8
    unknown_u32_10: 0x1618

  - id: 114
    key: eagle_figurehead
    name: 大鹫像
    kind: 5
    base_price: 58000
    effects:
      - op: ship_mod
        modifier: sail_speed_factor
        # 具体系数未从 hex 还原，禁止编造

  - id: 162
    key: map_stonehenge
    name: 去环状石柱遗址的地图
    kind: 10
    base_price: 500
    effects:
      - op: unlock_remain
        remain_key: stonehenge   # 遗迹 ID [未知]，先用 key
        anchor_town_id: 2        # +0x18=阿姆斯特丹 [推断]
```

```yaml
# config/items/recipes.yaml
schema: dk4.items.recipes
version: 1
recipes:
  - key: mystery_slate
    inputs: [mystery_slate_top, mystery_slate_bottom]
    output: null            # 产出原版未钉死，禁止猜
    site: town_mystery_hunting
```

未命名字节进 `unknown_bytes` / `unknown_u32_*`。提取器只输出确认字段；推断字段必须带注释，schema 升 version 后再改名。

World0 / World1 是两份内容包，同一 schema。126 条 `rest` 不同的记录按包覆盖，不在引擎里 `if world==1`。

## 7. 独立旋钮

改这些值不应迫使其他系统改代码：

| 旋钮 | 文件 | 原版包 |
| --- | --- | --- |
| 道具表长 | `items.yaml` | 231 |
| 种类表长 | `kinds.yaml` | 13 `[推断]` |
| `item_empty_slot` | `caps.yaml` | 232 |
| `unique_stack` | `caps.yaml` | 1 |
| `base_price` | 每条道具 | 100–420000 |
| 图标、中文名 | `ui/strings` + `icon` | mesfile / Item*.dk4 |
| 配方 | `recipes.yaml` | 未完全还原 |
| World0 vs World1 字段差 | 内容包 | 126 条 rest |

`ITEMEFFECT_AMOUNT_*`、装备槽、卖价公式在 02 与 `formulas/`。

## 8. 对外接口

只传 ID 与值。

查询：

- `Items.get(item_id) -> ItemDef | none`
- `Items.kind(item_id) -> kind_id`
- `Items.owned(fleet_id, item_id) -> bool`
- `Items.list_owned(fleet_id, kind_id?) -> [item_id]`
- `Items.effects(item_id) -> [Effect]`
- `Items.base_price(item_id) -> int`

命令：

- `AcquireItem { fleet_id, item_id }` 已有则按 `unique_stack` 拒绝或叠加
- `DiscardItem { fleet_id, item_id }`
- `UseItem { fleet_id, item_id, target }` 解释 effect 列表，发事件
- `CombineItems { fleet_id, item_ids }` 查 `recipes.yaml`

事件（本系统发出，别人订阅）：

- `ItemAcquired { fleet_id, item_id }`
- `PlantSpecialProduct { town_id?, village_id?, trading_item_id, item_id }`
- `UnlockRemain { remain_id }` / `UnlockVillage { village_id }`
- `FigureheadChanged { ship_id, item_id }`（由船系统执行，本系统只确认持有）

不暴露 `SItem*`、`CItemElement`、World DLL 指针。

## 9. 未核实字段

`SItem` 步长 `0x20`。已命名与未命名如下。

| 偏移 | 宽 | 原版包观察 | 状态 |
| --- | ---: | --- | --- |
| +0x00 | 8 | 名称指针 `0x00000001800xxxxx` | [确认] `name` |
| +0x08 | 4 | u32，南瓜 2000，最大 420000（导善手杖），最小 100（碎片） | [推断] `base_price` |
| +0x0C | 4 | 146 条为 `-1`；否则像 World DLL 内偏移（如肉桂 `0x79f8`） | [未知] |
| +0x10 | 4 | 与 +0x0C 同时为 `-1` 或同时非 `-1` | [未知] |
| +0x14 | 1 | 0–12，与种类菜单 13 档对齐 | [推断] `eItemKind` |
| +0x15 | 1 | 0–6 或 `8`；`8` 常与 +0x0C==-1 同时出现。kind 8/9 上即使 +0x0C==-1 也走 0–6 | [未知] 兼有「能力下标 / 套装序号 / 哨兵 8」三种用法嫌疑 |
| +0x16 | 1 | 武器/防具/发现物多为 `0x18=24`；人物用具为 0–8、13–15、17–20 | [未知] 不是 `eSkill`（`NUM_SKILL=10`）。更像岗位/舱室轨 |
| +0x17 | 1 | 仅 kind 2/3/4 非 0：武器 15–62，防具 18–65，用具 12–80 | [推断] `ITEMEFFECT_AMOUNT` / 攻防显示值 |
| +0x18 | 1 | 183 条为 `98`；否则落在 0–96 的 `eTown` | [推断] 关联港或「98=未绑」 |
| +0x19 | 1 | 取值 0、8、10、11；World1 有 97 条与 World0 不同 | [未知] |
| +0x1A..0x1F | 6 | 全 0 | [确认] 填充 |

`CItem::vfunc_05` 拷贝整条后写一串 `*(sB0+0x29)`…`+0x3c` 的运行时字节，与世界表字段的对应 `[未知]`。

`ITEMSTATE_BITS` 位宽、`eItemTag=7` 的枚举名、`CItemElement+0x39c8` 的 7 字节、`NUM_KIND=9`（RTC，未必是道具）全部 `[未知]`。

地点地图对应哪条 `eRemains` / `eVillage`，世界表里还没有字段名。禁止把 `+0x18` 的城镇 ID 直接当成遗迹 ID。

对照数据：`Docs/_data/items_world0.json`（名+32 字节 hex）、`Docs/_data/items.tsv`。
