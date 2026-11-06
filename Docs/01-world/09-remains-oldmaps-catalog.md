# 遗迹、村落与古地图目录

world 拥有遗迹 / 村落的 **ID、显示名、静态坐标、发现位形状**。古地图是 `eItem`，持有与使用走 items；本篇只回答「这张地图声明揭示哪个发现点」。海上怎么搜、弹什么窗口，见 [03-villages-discovery.md](03-villages-discovery.md) 与 [06-ocean/02-search-encounters.md](../06-ocean/02-search-encounters.md)、[09-items/01-item-catalog.md](../09-items/01-item-catalog.md)。

本篇补 03 未抽出的 **25 个遗迹名**、**24 个村落名与原版坐标**，以及 `eItem` 155–173 对遗迹 / 村落的对照。对照只写源码或文案能钉住的字段；`item_id → remain_id` 的整数表 **未在 exe 里找到**，禁止把 `SItem+0x18` 的城镇 ID 当成遗迹下标。

## 1. 职责边界

管：

- `eRemains` 25 条定义（名字、发现位形状）
- `eVillage` 24 条定义（名字、原版坐标、静态附加字节）
- 古地图道具 → 发现点的 **声明表**（`config/world/unlock_maps.yaml`）
- `CMpRemains` / `CGetOldMapWindow` 与资源包的归属（逻辑 ID vs UI）

不管：

- 搜索圈、测量士、探索掷骰（`config/ocean/search.yaml`）
- 道具库存、鉴定、神秘狩猎配方（`config/items/`）
- 发现标题镜头、古地图窗像素（`config/ui/`）
- 遗迹内部剧情、霸者之证（narrative / ocean）
- 港口本身（[02-towns.md](02-towns.md)）
- `World.dat` 网格语义（[01-map-and-sea-areas.md](01-map-and-sea-areas.md)）

遗迹与村落 **不在** World0.dll / World1.dll 的 `GetFix*` 里。名字和村落坐标在主 exe。World0 / World1 共用这一份表。

## 2. 原版事实

### 2.1 计数

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `enum eRemains { NUM_REMAINS, INVALID_REMAINS = -1 }`，头文件写「integer not recovered」 | [确认] | `srcdata/source/RemainsID.h` |
| `NUM_REMAINS = 25` | [确认] | 见下：名字指针表恰 25 条后接 NULL；所有运行时循环 `>= 0x19` |
| `CRemainsElement` 析构 / 存档 / 读档循环 `s20 >= 0x19`；构造 `fn_140007dce(..., 0x10, 0x19, ...)` 分配 25 个步长 `0x10` 的对象 | [确认] | `srcdata/source/game/world/CRemainsElement.cpp` |
| `fn_14000a321`：`eRemains >= 0x19` 结束扫描 | [确认] | `srcdata/source/game/unassigned/fn_1402aad20.cpp` body `0x1402ab000` |
| RTC `eRemains` 立即数统计 `[[25,1],[5,1]]` | [确认] | `srcdata/catalog/num_from_rtc.json`。`5` 的语义 [未知] |
| `NUM_VILLAGE = 24`，`INVALID_VILLAGE = -1` | [确认] | `VillageID.h`，`num_values.json` |
| 近港筛选哨兵 `0x19` = 25 = `NUM_VILLAGE+1` | [确认] | `fn_14000bcf3` body `0x140151250` |
| `REMAINSSTATE_BITS` 符号在，位宽未恢复 | [确认] | `Remains.h`，`asserts.txt`，`constants.json` |
| 遗迹 / 村落都不在 `GetFixTownProc` 等五个导出里 | [确认] | `srcdata/source/world/DllMain.cpp` |

`0x19` 在遗迹路径是 **表长 25**（循环 `id >= 25` 结束）。在村落近港路径是 **空哨兵 25**（合法 id 0–23）。不要混用。

### 2.2 遗迹对象

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `CRemains : CSaveObject`。每个对象步长 `0x10` | [确认] | 构造分配参数 `0x10, 0x19`；索引 `fn_14000a3b2` `imul rax, 0x10` 后 `+8` |
| 存档 / 读档只写 `+0x8`（`vfunc_07` / `vfunc_06` → `fn_14000c6da` / `fn_14000c978`） | [确认] | `CRemains.cpp` |
| `vfunc_05` 把 `+0x8` 置 0 | [确认] | 同文件 |
| `CRemainsElement` 挂在 `CWorld +0xf0d0` | [确认] | `CWorld.cpp` 构造 |
| 取遗迹元素：`fn_14001199b` 返回 `CWorld+0xf0d0` | [确认] | body `0x1402a8880` |
| 显示名：`CRemains::vfunc_02` 先 `fn_14000ae5c` 取下标，再 `fn_140008242` | [确认] | `CRemains.cpp` body `0x1401ef350` |
| `fn_140008242` thunk `jmp 0x1402ac5c0`：`lea rcx, [rip+…]` → 指针表 `0x1405b2f80`，`rcx + id*8` 得 `const char*` | [确认] | SC exe `DK4HD_sc.exe` |
| 指针表 25 个非 0 指针，第 26 槽为 0 | [确认] | 表 VA `0x1405b2f80`；字符串 `0x1405b3070`–`0x1405b31d8`，GBK |
| `CRemains` **不是** `CLocator`。对象内没有坐标字段 | [确认] | 类层次 `CRemains -> CSaveObject -> CObject`；体积 `0x10` |
| 25 对原版坐标不在 `.rdata` 连续 `(x,y)` 表里（按伦敦量级扫描为 0） | [确认] | 对 SC exe `.rdata` 扫描 |
| 发现位另有一份：`fn_14000a321` 用 `fn_140011cc5` 读某对象 `+0x34` 的 bit，`fn_1400134e4` 置位 | [确认] | `fn_1402aad20.cpp`。该对象是玩家还是世界 [未知] |

遗迹全名（SC exe GBK，下标即 `remain_id`）`[确认]`：

| id | key | name |
| ---: | --- | --- |
| 0 | stonehenge | 巨石阵 |
| 1 | wooden_church | 木造教堂 |
| 2 | st_martin_church | 圣马丁教堂 |
| 3 | stave_church | 木条教堂 |
| 4 | iwaya_temple | 岩心寺 |
| 5 | santiago_cathedral | 圣地亚哥大教堂 |
| 6 | colosseum | 罗马竞技场 |
| 7 | parthenon | 巴特农神殿 |
| 8 | cave_monastery | 洞窟修道院 |
| 9 | pyramids_sphinx | 金字塔和狮身人面像 |
| 10 | mud_mosque | 泥清真寺 |
| 11 | great_zimbabwe | 辛巴威神殿 |
| 12 | basra_mosque | 巴斯拉的清真寺 |
| 13 | taj_mahal | 泰姬玛哈陵 |
| 14 | angkor | 吴哥窟 |
| 15 | borobudur | 婆罗浮屠 |
| 16 | forbidden_city | 紫禁城 |
| 17 | muryeong_tomb | 武宁王陵 |
| 18 | kinkaku | 金阁寺 |
| 19 | tula_atlantes | 图拉战士像 |
| 20 | teotihuacan | 太阳和月亮的金字塔 |
| 21 | machu_picchu | 马丘比丘 |
| 22 | tikal | 蒂卡尔 |
| 23 | stupa | 佛塔 |
| 24 | ayutthaya | 阿瑜陀耶 |

`key` 是重构别名，不是原版枚举符。

### 2.3 村落对象与坐标

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `CVillage : CObject`，内嵌 `CLocator` 在 `+8`。体积 `0x48`。**不是** `CSaveObject` | [确认] | `class_hierarchy.txt`，`CVillage.cpp` 析构 `fn_14000750e(..., 0x48)` |
| `CVillageElement` 在 `CWorld +0x19f38`；存档只写元素 `+0x8`，**不**循环 24 个村落对象 | [确认] | `CWorld.cpp`，`CVillageElement.cpp` |
| 运行时数组：`fn_14000e912` `imul rax, 0x48` + `lea` → `0x1442a72e0`（`.data` BSS，24×`0x48`） | [确认] | body `0x1402a03d0` |
| 静态定义表：`fn_1400032c9` 循环 `id >= 0x18`，源步长 `0x38`，`lea` → `0x140687b40` | [确认] | body `0x1402a04d0` |
| 源记录：`+0x00` 名字指针；`+0x08/+0x0C` 两个 `int32` 拷到对象 `+0x18/+0x1C`；`+0x10..+0x2C` 六个 `int32` 各 ×16 写到 `+0x20..+0x3C`；`+0x30/+0x31` 两字节 → `+0x40/+0x41` | [确认] | 同函数 |
| `+0x18/+0x1C` 是原版坐标（与港口同一空间：伦敦 `20080,4272`） | [确认] | 值域与港口表重叠；北海道 `(35712,5248)` 在大阪 `(35040,6144)` 北侧（y 更小） |
| 显示名 C-string 在 `.rdata` `0x1405ace78` 起，GBK | [确认] | 源记录 `+0x00` |
| `CVillage::vfunc_03` 调 `fn_14000cf90(5, id)`：把 kind=5 与 id 打成名字句柄 | [确认] | `CVillage.cpp`；`fn_14000cf90` kind 0=港 / 1=舰队 / 2=道具 / 3=船 / 4=junction，5 走默认打包 |
| `NUM_VILLAGEBLINDAREA` 整数未恢复 | [确认] | `VillageblindareaID.h` |

村落全表（SC exe，下标即 `village_id`）`[确认]`：

| id | key | name | x | y | u8_30 | u8_31 | i32×16 六个源值 |
| ---: | --- | --- | ---: | ---: | ---: | ---: | --- |
| 0 | inuvik | 依努维克 | 4880 | 2304 | 5 | 10 | 254, 0, 284, 188, 0, 0 |
| 1 | hawaii | 夏威夷 | 2256 | 7536 | 4 | 9 | 0, 325, 254, 331, 0, 0 |
| 2 | rio_de_janeiro | 里约热内卢 | 15120 | 12544 | 2 | 4 | 806, 720, 234, 118, 0, 0 |
| 3 | churchill | 丘吉尔 | 9584 | 3472 | 6 | 9 | 538, 0, 282, 272, 0, 0 |
| 4 | valdivia | 瓦尔迪维亚 | 11824 | 14272 | 6 | 5 | 440, 838, 366, 412, 0, 0 |
| 5 | nome | 诺姆 | 1600 | 2416 | 5 | 9 | 0, 0, 254, 325, 0, 0 |
| 6 | cape_cod | 鳕角 | 12176 | 5344 | 6 | 3 | 538, 272, 502, 53, 0, 0 |
| 7 | montevideo | 孟特维的亚 | 13248 | 14304 | 2 | 5 | 806, 838, 234, 412, 0, 0 |
| 8 | hekla | 海克拉 | 17968 | 2912 | 0 | 2 | 1040, 0, 160, 240, 0, 0 |
| 9 | karibib | 卡里比布 | 21600 | 12544 | 2 | 1 | 1040, 900, 608, 350, 0, 0 |
| 10 | farwell | 法尔维尔 | 15584 | 2688 | 0 | 7 | 820, 0, 220, 272, 0, 0 |
| 11 | tahiti | 塔希提 | 3568 | 11648 | 4 | 10 | 0, 656, 440, 594, 0, 0 |
| 12 | dikson | 迪克逊 | 28960 | 1824 | 0 | 10 | 1520, 0, 440, 370, 0, 0 |
| 13 | azores | 亚束尔 | 16864 | 5696 | 1 | 1 | 1040, 240, 70, 260, 0, 0 |
| 14 | santa_barbara | 圣巴巴拉 | 6608 | 6144 | 6 | 8 | 254, 188, 284, 137, 254, 325 |
| 15 | narvik | 纳尔维克 | 22032 | 2240 | 0 | 2 | 1200, 0, 320, 170, 1430, 170 |
| 16 | rabaul | 雷巴求 | 33648 | 11824 | 3 | 4 | 1648, 660, 262, 156, 1910, 700 |
| 17 | callao | 喀劳 | 11248 | 10944 | 6 | 7 | 440, 720, 366, 118, 0, 0 |
| 18 | hokkaido | 北海道 | 35712 | 5248 | 5 | 3 | 1960, 180, 364, 136, 2240, 316 |
| 19 | perth | 珀斯 | 32848 | 13520 | 3 | 5 | 1648, 816, 552, 434, 0, 0 |
| 20 | guam | 关岛 | 36192 | 8288 | 4 | 7 | 2200, 500, 300, 316, 0, 0 |
| 21 | tiksi | 季克西 | 34336 | 1920 | 5 | 10 | 1960, 0, 540, 180, 0, 0 |
| 22 | korf | 科尔夫 | 38368 | 3280 | 5 | 5 | 2324, 180, 176, 320, 0, 0 |
| 23 | wanganui | 汪加努 | 39424 | 14400 | 4 | 9 | 2200, 816, 300, 434, 0, 0 |

`u8_30` 值域 0–6；`u8_31` 值域 1–10。不是 `eSeaArea`（25 档）。可能与盲区 / 气候带有关，**未命名**。六个 `int32` 乘 16 后进对象，像矩形或盲区多边形，禁止当第二套经纬。

24 个中文名里 **没有**「岩窟」「廷巴克图」「北京」「德里」。这三处只出现在古地图 **道具名 / 说明** 里，不能直接写成 `village_id`。

近港候选仍是 `fn_14000bcf3`：先扫 24 个村落，再用 `dx=40000` `dy=20000` 比港口 Locator。完整最近邻谓词见 03 篇，本篇不重复。

### 2.4 古地图道具（`eItem` 155–173）

道具名来自 World0 `GetFixItemProc` / `Docs/_data/items.tsv` `[确认]`。`SItem` 步长 `0x20`。地点地图 `+0x14 = 10`（kind `place_map`）`[推断]`，见物品目录。

地点地图 `rest` 公共字节（162–173 全相同，除 `+0x18`）：

```
+0x14 = 0x0A   # kind 10
+0x15 = 0x08
+0x16 = 0x18   # 24，与 NUM_VILLAGE 同数；七张贤者地图也是 0x18
+0x17 = 0x00
+0x18 = town_id
+0x19 = 0x08
```

`+0x18` 落在 0–96 的 `eTown`，与 97 港表对得上 `[确认]` 值域。物品目录把它当「关联港」`[推断]`。**不是** `remain_id`（遗迹只有 0–24；例如去北京的地图 `+0x18=57` 是汉城）。

七张贤者地图（155–161，kind 9）：`+0x15 = 0..6` 连续，`+0x18 = 98`（`NUM_TOWN+1`，未绑港）。说明在 mesfile 各自一条或两条「古地图。画在…」。

地点地图 12 张：

| id | key | name | base_price | +0x18 港 | 港名 |
| ---: | --- | --- | ---: | ---: | --- |
| 162 | map_stonehenge | 去环状石柱遗址的地图 | 500 | 2 | 阿姆斯特丹 |
| 163 | map_colosseum | 去古代竞技场的地图 | 700 | 5 | 汉堡 |
| 164 | map_cave_village | 去岩窟村庄的地图 | 1500 | 19 | 雅典 |
| 165 | map_sahara | 撒哈拉大沙漠地图 | 1500 | 29 | 圣乔治 |
| 166 | map_imperial_mosque | 去帝王清真寺的地图 | 2000 | 34 | 索法拉 |
| 167 | map_mughal | 莫卧儿帝国地图 | 1000 | 38 | 巴士拉 |
| 168 | map_ancient_temple | 去古代寺院地图 | 1000 | 50 | 雅加达 |
| 169 | map_beijing | 去北京的地图 | 1000 | 57 | 汉城 |
| 170 | map_royal_tomb | 去王陵的地图 | 2500 | 54 | 杭州 |
| 171 | map_golden_temple | 去黄金寺的地图 | 3000 | 42 | 卡利卡特 |
| 172 | map_ancient_city | 去古代城市遗址的地图 | 1000 | 65 | 委拉克鲁斯 |
| 173 | map_aztec | 阿斯帝加王国彩绘地图 | 2000 | 61 | 哈瓦那 |

`+0x18` 不是「最近港口」：北京说明对紫禁城，字节却是汉城；王陵若是武宁王陵应近汉城，字节却是杭州。当作购入 / 剧情锚点，不当作遗迹坐标。

说明文案（`mesfile_sc.txt` / `Game/SC/mesfile.dk4`）。mesfile 的 ID 是 **文件内字节偏移**。若干条说明挤在同一段 NUL 串里，游戏用子偏移切开；抽出器按 NUL 会粘在一起。

| 偏移 | 与道具的对齐 | 等级 | 文本 |
| --- | --- | --- | --- |
| `0x1be2b` | 155 | [确认] 单独串 | 古地图。画在纸上。 |
| `0x1be3f` | 156 | [确认] | 古地图。画在布上。 |
| `0x1be53` | 157+158 | [推断] 顺序 | 刻在石板上 / 画在荷叶上 |
| `0x1be93` | 159+160 | [推断] | 图案当地图 / 画在竹内 |
| `0x1beeb` | 161 | [确认] | 刻在大刀刀尖上 |
| `0x1bf07` | 162+163 | [推断] 顺序+地名 | 英国南部未知记号 / 古罗马首都一带 |
| `0x1bf63` | 164–169 | [推断] 顺序+地名 | 小亚细亚村落；廷巴克图商队；国王清真寺；莫卧儿/德里；柬埔寨王宫寺院；中国北部/北京 |
| `0x1c0bb` | 170 | [确认] 单独串 | 通向国王墓地的地图 |
| `0x1c0ef` | 171 | [确认] | 黄金之国日本的寺院 |
| `0x1c123` | 172+173 | [推断] 顺序 | 废墟古代城市与远岛往来 / 阿斯帝加王国图画书 |

`0x1c42f` 另有「标着%s位置的地图。」——藏宝图 / 通用句，不是 12 张地点图。

### 2.5 `item_id` → `remain_id` / `village_id`

在 SC exe 全文搜索：

- 连续 `162..173` 的 `int32` 表：无
- 按名字对上的 id 序列（如 `0,6,8,10,…`）的字节 / `int16` / `int32`：无
- `sub r32, 0xA2`（162）：无
- 对「巨石阵」字符串的指针：只有名字表自己

结论：`[确认]` 不存在「`remain_id = item_id - 162`」或 12 槽整数对照表。引擎不得写 `if item==162: remain=0`。

名字对名字的 **推断**（只作注释，不进运行时外键）。规则：物品名 **且** 说明同时唯一指向某一遗迹名，才给 `remain_id`。

| item | 物品名 / 说明 | 遗迹名 | remain_id | 强度 |
| ---: | --- | --- | ---: | --- |
| 162 | 环状石柱遗址；英国南部 | 巨石阵 | 0 | [推断] 强 |
| 163 | 古代竞技场；古罗马首都 | 罗马竞技场 | 6 | [推断] 强 |
| 164 | 岩窟村庄；小亚细亚基督徒村落 | 洞窟修道院 | 8 | [推断] 中（物品写「村庄」，24 村无名「岩窟」） |
| 165 | 撒哈拉；廷巴克图 | 泥清真寺 | 10 | [推断] 强 |
| 166 | 帝王清真寺；国王的清真寺 | 泥清真寺已占；巴斯拉的清真寺 | — | 不填。锚点是索法拉，与巴士拉无关 |
| 167 | 莫卧儿；德里 | 泰姬玛哈陵 | 13 | [推断] 强 |
| 168 | 古代寺院；柬埔寨王宫寺院 | 吴哥窟 | 14 | [推断] 强 |
| 169 | 去北京；中国北部 | 紫禁城 | 16 | [推断] 强 |
| 170 | 王陵；国王墓地 | 武宁王陵 | 17 | [推断] 中（只有「陵」字重合） |
| 171 | 黄金寺；日本寺院 | 金阁寺 | 18 | [推断] 强 |
| 172 | 古代城市遗址；废墟+远岛 | 马丘比丘 / 蒂卡尔 / 太阳金字塔 | — | 不填 |
| 173 | 阿斯帝加王国 | 图拉战士像 / 太阳金字塔 | — | 不填 |

12 张地点图覆盖不了 25 个遗迹。木造教堂、巴特农、辛巴威、婆罗浮屠、佛塔、阿瑜陀耶等 **没有** 对应 `eItem` 162–173。那些点只走海上探索。

kind 8 遗物（乌勒尔弓箭…契慕斧头，14 件）可能是遗迹出土物，与 25 遗迹不是 1:1，本篇不连。

### 2.6 `CMpRemains`

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 类体积 `0x18`。构造写 vftable，`+0x10 = 0` | [确认] | `CMpRemains.cpp`；析构 `fn_14000750e(..., 0x18)` |
| 无基类链上的 `CSaveObject` / `CLocator` | [确认] | `class_hierarchy.txt`：`CMpRemains` 单节点 |
| 与 `CTownSceneView` 的 12 个 `eMapPointKind` 槽（`+0x80` 起）同属城镇地图点 | [推断] | `CTownSceneView` 循环 `eMapPointKind >= 0xc`；`CMpDock` 同类前缀 |
| 贴图 `Game/Remains.dk4`。`fn_14001233c` 先加载 `EvFrame.dk4`，再 `fn_140010e6f(..., "Remains.dk4", sB8, &rcSrc)`，画布 `0x780×0x438`（1920×1080） | [确认] | `fn_1401eb2a0.cpp` body `0x1401ef150`。`sB8` 是帧下标，是否等于 `remain_id` [未知] |
| 构造函数 **不** 写入 `remain_id` | [确认] | `CMpRemains.cpp` |

`CMpRemains` 是城镇场景里的地图点视图，不是遗迹定义表。定义在 `CRemains` + 名字指针表。

### 2.7 `CGetOldMapWindow` 与古地图 UI

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `CGetOldMapWindow -> COldMapSprite -> CFlatSimpleSprite -> …` | [确认] | `class_hierarchy.txt` |
| `CGetOldMapWindow` / `COldMapSprite` 析构释放 `0xb8` | [确认] | `CGetOldMapWindow.cpp` |
| `COldMapInfomationView` 析构释放 `0x630`；内嵌 `COldMapSprite` 在 `+0xb0` | [确认] | `COldMapInfomationView.cpp` |
| `COldMapButton -> CPushButton -> …` | [确认] | 层次文件 |
| `COldMapInfomationView` 构造按 4 个槽分配步长 `0xd0` 的按钮（`fn_140010636(..., 0xd0, 4, …)`）；`vfunc_02` 循环 `s20 >= 4` 再拆 `+0x508/+0x510/+0x518/+0x520/+0x528` 五个指针 | [确认] | 同文件。4 是窗内槽数，不是 12 张地图、也不是 25 遗迹 |
| `COldMapSprite` 构造写入一批矩形立刻数，含 `+0x58=0x780`、`+0x5c=0x438`（1920×1080） | [确认] | `COldMapSprite.cpp` |
| 资源 `OldMap.dk4`、`OldMapStill.dk4`、`InfoBox.dk4` / `InfoBox2.dk4` | [确认] | `unassigned_named.tsv`；`Game/` |
| 加载帧：`OldMap.dk4` 至少用过 1、2、3、4、5 | [确认] | `fn_14012cb30.cpp`、`fn_140177480.cpp` |
| 获得古地图：`fn_140014132`（RTC `cGetOldMapWindow`）在 `+0x284==0` 时取一个 id 写入 `+0x288`，`-1` 则放弃；否则 `id * 0x20` 当 `SItem` 步长 | [确认] | body `0x14017c0e0`。空槽立刻数 `0xe8=232` 与道具空槽一致 |
| `CGetOldMapWindow::vfunc_03` 空函数 | [确认] | `CGetOldMapWindow.cpp` |
| 窗内 GBK「名称：」（字节 `C1 B6 D6 D7 A3 BC`） | [确认] | `fn_140002e50` 里 `fn_140001e65` 的字面量 |

古地图窗读的是 **道具记录**（步长 32），不是 `eRemains` 下标。揭示哪个发现点必须查 `unlock_maps.yaml`，不能从窗口类反推。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 非法 / 哨兵 | 存档 |
| --- | --- | --- | --- | --- |
| 遗迹 | `remain_id` / `eRemains` | 25 | -1 | 每条 `CRemains+0x8`；另有 `+0x34` bitfield |
| 村落 | `village_id` / `eVillage` | 24 | -1；近港空哨兵 25 | `CVillageElement+0x8` 仅 8 字节，24 个发现位如何挤进去 [未知] |
| 村落盲区 | `eVillageBlindArea` | [未知] | `INVALID_VILLAGEBLINDAREA` | — |
| 地点地图 | `item_id` 162–173 | 12 | 道具空槽 232 | items |
| 贤者地图 | `item_id` 155–161 | 7 | 同上 | items；合成材料，不直接揭示点 |
| 地图点种类 | `eMapPointKind` | 12 | -1 | `CMpRemains` 是其中一种视图 [推断] |

运行时循环读表长。原版包默认 25 / 24。模组加行用 `id >= 1000` 或独立包，不覆盖 0–24 / 0–23。

禁止：

- 用 `0` 当空遗迹 / 空村落（0 是巨石阵、依努维克）
- 用 `item_id - 162` 当 `remain_id`
- 用 `SItem+0x18` 当 `remain_id` 或 `village_id`
- 把「去北京的地图」写成 `village_id`（24 村里没有北京）

## 4. 算法 / 公式

本系统几乎没有伤害公式。立刻数进 caps / 内容包。

```
remain.count_original          = 25
remain.object_stride           = 0x10
remain.name_ptr_table          = exe+0x1405b2f80   # 内容管线抽出，运行时读 yaml
village.count_original         = 24
village.object_stride          = 0x48
village.def_stride             = 0x38
village.none                   = 25
village.near_town.dx_limit     = 40000
village.near_town.dy_limit     = 20000
item.place_map_ids             = 162..173
item.sage_map_ids              = 155..161
item.empty_slot                = 232
oldmap.window_slots            = 4
oldmap.canvas                  = 1920 x 1080
```

发现：

```
on UnlockRemain { remain_id }:
    if remain_id not in remains: reject
    remains_state[remain_id].discovered = true
    # 不读背包

on UnlockVillage { village_id }:
    同上
```

`UseItem` 在 items：查 `unlock_maps` 的 `unlocks` 列表，逐条发事件。`unlocks` 为空或 `type: unknown` 则只获得道具、不改发现位。

近港村落伪代码见 03 篇 `fn_14000bcf3`。坐标用本表 `x,y`，不要扫 `World.dat`。

遗迹坐标未抽出。海上命中遗迹走 ocean 的探索结果表，不在本系统按距离找点，直到坐标表补全。

## 5. 耦合分析

原版：

- 遗迹名、村落名、村落坐标都在主 exe `.rdata` / `.data`，World DLL 不知情
- `CWorld` 同时嵌 `CRemainsElement` 与 `CVillageElement`
- `fn_14000a321` 先扫 231 道具所有权，再扫 25 遗迹
- 古地图 UI 按 `SItem` 步长寻址，目标点却是世界发现位
- 城镇地图点 `eMapPointKind` 与 `CMpRemains` 绑在一起
- 说明字符串按 mesfile 字节偏移，多条说明共一段

重构所有权：

| 数据 | 系统 |
| --- | --- |
| 遗迹 / 村落 id、名、村落坐标、静态附加字节 | world（本篇三份 yaml） |
| 已发现 bit | world 存档 |
| 地图道具定义、持有 | items；`unlocks` 外键指向本篇 |
| 搜索、命中 | ocean |
| 古地图窗、遗迹图、发现标题 | ui |
| 遗迹剧情 | narrative 订阅 `RemainsDiscovered` |

items 不得在代码里写死 162→巨石阵。只读 `unlock_maps.yaml`。没有对应 ID 则 `type: unknown`，校验器警告、不失败启动（原版 12 张图本来就没把 25 个点盖满）。

## 6. 解耦后的配置

`config/world/remains.yaml`：

```yaml
schema: dk4.world.remains
version: 1
pack: vanilla-sc-exe          # 主 exe，不是 World0.dll
count_original: 25
invalid: -1
state_bits: unknown           # REMAINSSTATE_BITS
remains:
  - id: 0
    key: stonehenge
    name: 巨石阵
    name_i18n: remain.stonehenge.name
    coords:
      space: original
      x: null                 # CRemains 无 Locator；表未抽出
      y: null
    map_point_kind_id: null
    discovered_by_default: false
    map_item_ids: []          # 由 unlock_maps 反查，本表不写死
    unknown_bytes: []
  - id: 6
    key: colosseum
    name: 罗马竞技场
    coords: { space: original, x: null, y: null }
    discovered_by_default: false
  # id 1..5, 7..24 同构，名字见 §2.2
```

`config/world/villages.yaml`：

```yaml
schema: dk4.world.villages
version: 1
pack: vanilla-sc-exe
sentinel_none: 25
def_stride: 0x38
object_stride: 0x48
villages:
  - id: 0
    key: inuvik
    name: 依努维克
    name_i18n: village.inuvik.name
    coords:
      space: original
      x: 4880
      y: 2304
    sea_area_id: null
    blind_area_id: null
    discovered_by_default: false
    unknown_u8:
      b30: 5
      b31: 10
    unknown_i32_x16: [254, 0, 284, 188, 0, 0]
  - id: 18
    key: hokkaido
    name: 北海道
    coords: { space: original, x: 35712, y: 5248 }
    unknown_u8: { b30: 5, b31: 3 }
    unknown_i32_x16: [1960, 180, 364, 136, 2240, 316]
  # id 1..17, 19..23 同构，见 §2.3
```

`config/world/unlock_maps.yaml`：

```yaml
schema: dk4.world.unlock_maps
version: 1
pack: vanilla-world0          # 道具定义在 World DLL；揭示目标在 exe
# 运行时只读 unlocks[]。inferred_name_match 是文档，提取器可保留但不参与判定。

sage_maps:
  - item_id: 155
    item_key: thales_paper_map
    sage_index: 0             # SItem+0x15 [推断]
    unlocks: []               # 合成材料

place_maps:
  - item_id: 162
    item_key: map_stonehenge
    name: 去环状石柱遗址的地图
    anchor_town_id: 2         # +0x18 阿姆斯特丹 [推断] 锚点港
    unlocks: []               # 整数对照表未还原
    inferred_name_match:
      type: remain
      remain_id: 0
      remain_key: stonehenge
      strength: strong        # 环状石柱 ↔ 巨石阵；英国南部
  - item_id: 163
    item_key: map_colosseum
    anchor_town_id: 5         # 汉堡
    unlocks: []
    inferred_name_match:
      type: remain
      remain_id: 6
      remain_key: colosseum
      strength: strong
  - item_id: 164
    item_key: map_cave_village
    anchor_town_id: 19        # 雅典
    unlocks: []
    inferred_name_match:
      type: remain
      remain_id: 8
      remain_key: cave_monastery
      strength: medium        # 物品写村庄；24 村无「岩窟」
  - item_id: 165
    item_key: map_sahara
    anchor_town_id: 29        # 圣乔治
    unlocks: []
    inferred_name_match:
      type: remain
      remain_id: 10
      remain_key: mud_mosque
      strength: strong        # 廷巴克图 ↔ 泥清真寺
  - item_id: 166
    item_key: map_imperial_mosque
    anchor_town_id: 34        # 索法拉
    unlocks: []
    inferred_name_match: { type: unknown }
  - item_id: 167
    item_key: map_mughal
    anchor_town_id: 38        # 巴士拉
    unlocks: []
    inferred_name_match:
      type: remain
      remain_id: 13
      remain_key: taj_mahal
      strength: strong
  - item_id: 168
    item_key: map_ancient_temple
    anchor_town_id: 50        # 雅加达
    unlocks: []
    inferred_name_match:
      type: remain
      remain_id: 14
      remain_key: angkor
      strength: strong
  - item_id: 169
    item_key: map_beijing
    anchor_town_id: 57        # 汉城，不是遗迹下标
    unlocks: []
    inferred_name_match:
      type: remain
      remain_id: 16
      remain_key: forbidden_city
      strength: strong
  - item_id: 170
    item_key: map_royal_tomb
    anchor_town_id: 54        # 杭州
    unlocks: []
    inferred_name_match:
      type: remain
      remain_id: 17
      remain_key: muryeong_tomb
      strength: medium
  - item_id: 171
    item_key: map_golden_temple
    anchor_town_id: 42        # 卡利卡特
    unlocks: []
    inferred_name_match:
      type: remain
      remain_id: 18
      remain_key: kinkaku
      strength: strong
  - item_id: 172
    item_key: map_ancient_city
    anchor_town_id: 65        # 委拉克鲁斯
    unlocks: []
    inferred_name_match: { type: unknown }
  - item_id: 173
    item_key: map_aztec
    anchor_town_id: 61        # 哈瓦那
    unlocks: []
    inferred_name_match: { type: unknown }
```

把某条 `inferred_name_match` 升成真正的 `unlocks`，必须先在 exe / 事件脚本里钉到整数，再改 yaml 并升 `version`。禁止在引擎里读 `inferred_name_match`。

`config/formulas/ocean.yaml` 近港段仍只属于 ocean，本篇不复制。

## 7. 独立旋钮

| 旋钮 | 文件 | 原版包 |
| --- | --- | --- |
| 遗迹表长、名字 | `remains.yaml` | 25 |
| 村落表长、名字、坐标 | `villages.yaml` | 24 |
| 近港 dx/dy、哨兵 | `formulas/ocean.yaml` | 40000 / 20000 / 25 |
| 哪张地图揭示哪个点 | `unlock_maps.yaml` | 12 张 `unlocks: []` |
| 默认已发现 | 各条 `discovered_by_default` | false |
| 古地图窗槽数、画布 | `ui/`（不进 world 表） | 4；1920×1080 |
| World0 vs World1 | 道具在 DLL；点表在 exe | 点表一份 |

改搜索成功率、测量士、道具标价不改本表。补遗迹坐标只改 `remains.yaml` 的 `coords`，不改 ocean 代码。

## 8. 对外接口

只传 ID。

查询：

```
Remains.def(remain_id) -> RemainDef | none
Remains.name(remain_id) -> str
Remains.discovered(remain_id) -> bool
Remains.count() -> int                    # 表长
Village.def(village_id) -> VillageDef | none
Village.at_map(x, y, radius) -> [village_id]
Village.discovered(village_id) -> bool
UnlockMaps.for_item(item_id) -> [Unlock]  # 只返回 unlocks[]，不含 inferred_*
```

命令（ocean / items / narrative 调用）：

```
World.mark_remain_discovered(remain_id)
World.mark_village_discovered(village_id)
```

事件：

```
RemainsDiscovered { remain_id, fleet_id, item_id | null }
VillageDiscovered { village_id, fleet_id, item_id | null }
```

world 不读背包。`item_id` 由 items 在 `UseItem` 里带上。没有 `unlocks` 就不发事件。

不暴露 `CRemains*`、`CVillage*`、`0x1405b2f80`、mesfile 偏移。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| 25 个遗迹的原版坐标 | `CRemains` 无 Locator；`.rdata` 无 25 对 `(x,y)` | [未知] |
| `REMAINSSTATE_BITS` 宽度；`CRemains+0x8` 与 `fn_14000a321` 的 `+0x34` bit 是否同一份 | 存档 + 扫描函数 | [未知] |
| RTC `eRemains` 的立即数 `5` | `num_from_rtc.json` | [未知] |
| `CMpRemains` 对应哪一个 `eMapPointKind`；`Remains.dk4` 帧是否 `remain_id` | 仅类 + 加载 | [未知] |
| `CVillageElement+0x8` 的 8 字节如何表示 24 个发现位 | 存档只写这一处 | [未知] |
| 村落 `u8_30` / `u8_31`、六个 ×16 的 `int32` | 静态表 `+0x10..+0x31` | [未知] |
| `NUM_VILLAGEBLINDAREA` | 头文件 | [未知] |
| `fn_14000bcf3` 两个 eligible 谓词与完整最近邻 | 03 篇已列 | [未知] |
| `item 162–173 → remain_id` 整数表 | exe 无此表 | [未知] 运行时 `unlocks: []` |
| 上表「强/中」名字对齐 | 文案 | [推断] 禁止当外键 |
| `SItem+0x15/+0x16/+0x19` 在地点地图上恒为 `08/18/08` | 物品 hex | [未知] |
| `+0x18` 城镇的玩法含义（购入地 / 航线锚 / 其它） | 值是合法 `eTown` 但对不上遗迹地理 | [推断] 锚点 |
| mesfile 粘连说明的精确切分偏移 | 若干条共一段 NUL | [推断] 用顺序+地名 |
| `COldMapInfomationView` 4 槽与 12 张地图如何分页 | 只有槽数 | [未知] |
| `fn_140014132` 写入 `+0x288` 的 id 是否就是 `eItem` | 随后 `* 0x20` | [推断] 是道具下标 |
| 法尔维尔 / 海克拉 的英文地名是否 Cape Farewell / Hekla | 只有中文 | 不进判定；`key` 可改 |

对照：SC exe `Game/DK4HD_sc.exe` VA `0x1405b2f80`（遗迹名指针）、`0x140687b40`（村落定义）、`Docs/_data/items_world0.json`（地图 hex）、`srcdata/catalog/mesfile_sc.txt`。03 篇在抽出名字前写「name: null」；本篇表取代那一段空白，03 的职责边界仍然有效。
