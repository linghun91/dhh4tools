# 海域表现：音乐、图示、价格查询

本篇拥有**七大海域与港口的表现绑定**：大地图 / 海域图播哪首 BGM、港口播哪首、海域图上用哪套图标。原版的 `eSeaArea` 就是七大海域（`01-map-and-sea-areas.md` §2：真实循环上界 7，旧稿的「25 海域」是 RTC 投票伪影），由 factions 注册 `major_sea_id`（`02-factions/06-seven-culture-circles.md`），本篇只引用 key。地理网格、港口坐标、航线拓扑归 `01` / `05`。流行模型归势力（`02-factions/02-countries-culture-fashion.md`）。市价曲线归经济（`04-economy/02-market-price.md`）；原版价格表**不是**按海域分的，本篇只暴露查询。

## 1. 职责边界

**管：**

- `config/ui/bgm.yaml`：七大海域 → 大地图曲、文化圈 → 港口曲、特殊状态曲、停曲
- 海域图 pictogram 的资源键（原版 `RouteMap.dk4` / `RouteMkr.dk4` + `CSeaArea*View`）
- 把「当前大海域 / 当前港」翻译成 UI/音频要的 ID，不持有舰队、不持有港口对象
- 可选：`sea_areas`（`01` 的重构扩展）行上的 `bgm` 覆盖列

**不管：**

- 七大海域的注册、名字、矩形、邻接、港口归属（factions）
- `World.dat`、原版坐标（`01-map-and-sea-areas.md`）
- 航线是否已开辟、下一根据地玩法（`05-routes-navigation.md`；`CNextAreaView` 只是 UI）
- 大地图操舵、风向应用（`06-ocean`）
- 流行 26 目录、每港 2 槽、9 个 NPC 文化圈（`02-countries-culture-fashion.md`）
- 市价指数怎么滑、金币怎么报（`02-market-price.md`）；本系统不得存第二套价格矩阵
- `IconSC.dk4` 里技能/地图点/HUD 字图（`12-ui-save`）；语言包路径见 `03-localization.md`
- `CSoundBoard` 音量滑条布局（引擎 UI，不是曲目表）

## 2. 原版事实

### 2.1 三套地理 ID

| 空间 | 原版计数 | 用来干什么 | 来源 |
| --- | --- | --- | --- |
| 七大海域（= `eSeaArea` `[推断]`） | 7（海域表第 8 条「全世界」） | 势力影响力 7 格 `CForce+0x10`、海域图区号、大地图 BGM、遗迹首访奖励列 | [确认] RTC `eSeaArea` 仅两函数、循环都 `< 7`（`fn_140008b6b` / `fn_14000a8c1`）；海域表 `.rdata` VA `0x14058bb30` 8×`0x38` |
| 港口 `culture_id`（旧称 region）/ `CCulture` 槽 | 21（容器循环 `0x15`） | 文化圈、港口 BGM、105 卖价、日本圈图标 | [确认] `towns_world0.json`、`CCultureElement.cpp`、静态文化表 VA `0x1405b6160` 21×`0xE0` |
| NPC 文化圈 | 9 个文案 | 设施换装 | [确认] `02-countries-culture-fashion.md` |

归属链 `[确认]`：港口 `+0x25` culture → 静态文化表 `+0x08` / 运行时 `CCulture+0x10` = `major_sea_id`（`fn_140014e48`）。`fn_140013a02` 用 `STown+0x25` 直接取 `CCultureElement+8+i*0xE8`。

旧稿「`eSeaArea` 25 条、与七大海域并存」**撤回**：`NUM_SEAAREA = 25` 只来自 `fill_gaps.py` 投票，相邻函数里比 `0x19` 的是 `eForce`（25 势力）。

### 2.2 名字：七大海域有表，「25 海域名」本不存在

- 海域表 `0x14058bb30`（文件偏移按 `.rdata` 段 = VA − `0x140001200` = `0x58a930`）`[确认]`：`+0` 名字指针、`+8` 矩形 `x1,y1,x2,y2`（原版单位）、`+0x18` 8 个邻接 int（8 = 无）。名字依次：北海、地中海、非洲、印度洋、东南亚、东亚、新大陆、全世界。访问器 `fn_140004403`（`< 7`）/ `fn_140011423`（`< 8`）。完整行见 `02-factions/06-seven-culture-circles.md`。
- 「北海海域 / 北海海域周边 / 东南亚周边海域 …」长名出自 `fn_140009750`（RTC `SeaAreaMessage`，160 字节 = **20 个**指针），唯一调用者是古地图信息框 `fn_140002e50`（`OldMap.dk4`）。`[确认]` 它是古地图文案池，下标 0–19 **不是** `major_sea_id`，不能当七大海域名表，也不是 25 海域名表。旧稿据它写「7 区 + 周边变体」已更正。
- 21 个文化圈名：静态文化表每项 `+0` 是名字指针 `[确认]`（英吉利、佛兰德、德意志、北欧、葡萄牙、西班牙、意大利、希腊、土耳其、埃及、西非、东非、阿拉伯、印度、印度支那、印尼、中国、朝鲜、日本、加勒比海、墨西哥）。其中 8 个指针指进国名串（`0x14053a0xx`），另 13 个指向 `.rdata` VA `0x1405b776c` 起的独立串（文件偏移 `0x5b656c`，紧挨「小麦」）。旧稿把这 13 个当「能连续排下来的残缺名单」，实为同一张 21 名表的一部分。
- mesfile 里的「北海 / 地中海 / 非洲 / 印度洋 / 东南亚 / 东亚 / 加勒比海 / 波罗的海」是霸者之证、闲聊或商品百科文案。来源：`mesfile_sc.txt` `0x1e247` 起。

势力国名另在 `.rdata` `0x14053a08c` 一带（英吉利、法兰西、荷兰…印加），属 `02-factions/01-forces.md`。

### 2.3 BGM 运行时：42 号 + 哨兵

加载：[确认] `fn_1400037b0`（`fn_14023e3f0.cpp`）打开

- `res/bgm.ktsl2asbin`（目录，16704 字节，头 `KTSR`）
- `res/bgm.ktsl2stbin`（流，约 41.4 MB，内含 `OggS` 页）
- 以及 `res/SFX.ktsl2*`

播放入口：[确认] `fn_1400018a7`（`fn_140241970.cpp`）把当前曲号写入播放器对象再调下层。

`CBackupMusic`（体积由析构 `0x10`；字段 `+0x8`）：[确认] `CBackupMusic.cpp`。这是「保存当前曲、析构时恢复」的 RAII：构造先写 `+0x8=0x2B` 再改成 `fn_140008cba()`（当前曲号）；析构调 `0x140114df0` 按下表恢复。

| `+0x8` 曲号 | 恢复时行为 |
| --- | --- |
| `< 0` | 跳过 |
| `0 .. 0x17`（0–23） | `fn_1400018a7` 播主通道 |
| `0x18 .. 0x29`（24–41） | `fn_140003dff` 再 `fn_14000cf1d(id-0x18, 2, 1)` |
| `== 0x2B`（43） | 无备份，跳过 |
| 然后写回 `0x2B` | 清备份 |

即运行时曲号 **0–41 共 42 档**（主通道 24 + 副通道 18），`0x2B` 是「无曲 / 无备份」哨兵。asbin 头里的 cue 计数未还原，禁止把 42 写成容器文件里的轨数。

`CSoundBoard`（体积 `0x2c8`）：[确认] 构造把 `+0xb0` 起一长串立即数写成控件坐标（`0x224, 0x113, 0x278…`），继承 `CSurfaceSpriteView`。这是音量板 UI，不是曲目表。

大地图：[确认] `COceanScene::vfunc_05`

- 某状态 `== 0xd`：走 `fn_140003dff`（停）
- 否则若 `fn_140006c12 == 1`：再查 `fn_14000f26d(..., 3)`，真则播 **`0x15`（21）**，假则播 **`0x14`（20）**
- 否则播场景对象 `+0x208` 里存的曲号

构造把 `+0x20c = 0`，体积 `0x218`。[确认] `COceanScene.cpp`。

`+0x208` 的写入点：[确认] `fn_1400083e6`（body `0x1401ad8d0`，`fn_1401ac810.cpp`），`COceanScene::vfunc_02` 每帧第一个调用：

1. `town = fn_140005989(fn_1401ab9e0())` = `CPlayerFleetData+0x29` 字节（初值 `0x0B`；被当港 id 使用，语义「所属/最近港」`[推断]`）
2. `sea7 = fn_140014e48(town)` = 港口表 `+0x25`（文化圈）→ `CCulture+0x10` = **七大海域 `major_sea_id`**（链见 `02-factions/06-seven-culture-circles.md`）
3. `+0x208 = fn_1401a54e0(sea7)`：`0..6 → 4..10`（`sea7+4`），否则 `0x1B`（27）

所以大地图曲按 **七大海域** 选 7 首（4–10）。

海域图场景：[确认] `CSeaAreaScene::vfunc_02` 取 `s = fn_140007bee(...)`（对象 `+0xc`），`0 <= s < 7` 时 `fn_1400018a7(fn_14000da80(s))`。`fn_14000da80`（body `0x14011c110`）：`s==8` 时改取 `CWorldData+0x1e` 字节（`fn_14000a5b0`；该字节是开局所在七大海域，`04-climate-weather-zodiac.md` §2）；`0 <= s < 7` 返回 `s+4`，否则 `4`。与大地图 `fn_1401a54e0` 同一张「七大海域 → 4..10」映射，故 `s` 是七大海域下标 `[推断]`（旧稿写「7 是剧本数」无依据，已撤）。该函数同时每次迭代调 `fn_14000f871`（世界前进 1 拍，见 `07-calendar-and-coords.md`）。

### 2.4 港口 BGM 按 `town.culture_id`，九档，不按海域

`CTownScene::vfunc_05`：`fn_14000c144` 取当前港 → `fn_140002cbb` → `fn_1400018a7`。[确认] `CTownScene.cpp`。

`fn_140002cbb`（body `0x14011c3b0`，反编译失败，已用 capstone 还原）：

1. 参数是 `town_id`；`== 0x62`（空港哨兵 98）时不读港口
2. 否则 `fn_1400103bb` 取港口对象，`fn_14001055a` 读 **港口定义表 `+0x25`**（`culture_id`，旧称 `region`）。注意不是运行时 `CTown`：`fn_14001055a → fn_140003643 → fn_140004a66(id)` = `0x1442c8330 + id*0x84`（DLL `STown` 的 0x84 步长副本）再 `+0x25`
3. culture 缺省值 `0x16`（`town==0x62` 时不读表）；`> 0x14`（20）走默认曲 `0x0B`
4. 否则 21 路 jumptable（`0x14011c488`）映射到 `0x0B..0x13`

`fn_14001055a`：[确认] `add rax, 0x25` 再读字节（`fn_14025ae30.cpp` / 反汇编 `0x140260370`）。同一字节还决定港内地图点图标套（culture 18 → `kind+12`，见 `10-mappoint-kinds.md`）。

| culture_id | 首都（对照） | BGM id |
| --- | --- | --- |
| 0–3 | 伦敦 / 阿姆斯特丹 / 汉堡 / 斯德哥尔摩 | `0x0B`（11） |
| 4–7 | 里斯本 / 塞维利亚 / 热那亚 / 雅典 | `0x0C`（12） |
| 8, 9, 12 | 伊斯坦布尔 / 亚历山大 / 巴士拉 | `0x0D`（13） |
| 10–11 | 圣乔治 / 索法拉 | `0x0E`（14） |
| 13 | 卡利卡特 | `0x0F`（15） |
| 14–15 | 马六甲 / 雅加达 | `0x10`（16） |
| 16–17 | 杭州 / 汉城 | `0x11`（17） |
| 18 | 大阪 | `0x12`（18） |
| 19–20 | 哈瓦那 / 委拉克鲁斯 | `0x13`（19） |

[确认] 跳转目标与立即数。culture 9 埃及与 8、12 同曲；朝鲜（17）与中国同曲。

大佬音乐厅笔记（`dh4/5.png`）另给曲号 00–1C 中文名（勇跃、北欧的城市、日本的城市、拉斐尔…）。这些中文名 **不在** SC exe / mesfile 的 GBK 里。Soundtrack 日文文件名（`Hokuo_no_Machi` 等）与 9 档港曲对应，和 jumptable 的 `0x0B..0x13` 是否同一编号 **未在播放器里钉死**，两套表不要手工对齐。入口笔记：`+11ed0d`，对象 `+0x180` 曲号 / `+0x184` 音频 / `+0x188` 音效。`CBackupMusic` 把 `0..0x17` 与 `0x18..0x29` 分通道，和笔记「18 及以下 / 19–29」同形。

### 2.5 Soundtrack 目录名不是海域表

`Game/Soundtrack/` 是《大航海时代》30 周年合集，多作混装。文件夹名**不是**海域表。

与本作成曲最接近的是 DISC04（港曲按文化圈、海曲按大区，日文罗马字文件名）：

| 文件 | 名称（文件名） |
| --- | --- |
| `04_Hokuo_no_Machi.mp3` | 北欧之街 |
| `05_Nan'o_no_Machi.mp3` | 南欧之街 |
| `06_Isuramu_no_Machi.mp3` | 伊斯兰之街 |
| `07_Afurika_no_Machi.mp3` | 非洲之街 |
| `08_Indo_no_Machi.mp3` | 印度之街 |
| `09_Tonan-Ajia_no_Syuraku.mp3` | 东南亚聚落 |
| `10_Chugoku_no_Machi.mp3` | 中国之街 |
| `11_Nihon_no_Machi.mp3` | 日本之街 |
| `12_Shintairiku_no_Machi.mp3` | 新大陆之街 |
| `13_Oikaze_ni_Notte.mp3` | 乘着顺风 |
| `15_Minami_e_Iko.mp3` | 向南去 |
| `16_India_no_Kaze.mp3` | 印度之风 |
| `17_Nankai_no_Shimajima.mp3` | 南海诸岛 |
| `18_Higashi-Ajia_no_Umi.mp3` | 东亚之海 |
| `19_Suiheisen_no_Muko_e.mp3` | 向水平线之彼方 |

[推断] 九档港口 BGM `0x0B..0x13` 与 DISC04 的 04–12 同序（北欧…新大陆）。依据：档数相同、文化圈分组与曲名地理一致。未在 exe 里找到「BGM 11 = 该 mp3」的文件名表，**不得**写成确认。

DISC03（地中海 / 近欧 / 太平洋 / 北冰洋 / 印度洋 / 大西洋 / 伊比利亚 / 斯堪的纳维亚…）是 3 的配乐；DISC06 有 `North_Sea` `Mediterranean` `African_Sea`。都不是本作的海域名录。

### 2.6 海域图 View 与 pictogram

类与体积 / 基类：[确认] `class_hierarchy.txt`、各 `CSeaArea*.cpp`

| 类 | 体积（析构释放） | 角色 |
| --- | --- | --- |
| `CSeaAreaScene` | `0x128` | 海域图场景，继承 `CRouteMap` |
| `CSeaAreaMapView` | `0x340` | 底图；ctor 清 `eFleet>=0x45`（69）、`eTownKind>=2`、`eForceColor>=8` 三组指针 |
| `CSeaAreaMapObject` | 精灵+按钮 | 图上可点对象基类 |
| `CSeaAreaTownView` | 同族 | 港点；`trade/` 下另有 3 个空壳 vfunc |
| `CSeaAreaVillageView` | 同族 | 村点 |
| `CSeaAreaFleetView` | 同族 | NPC 舰队点 |
| `CSeaAreaRouteSprite` | `0x100` | 航线段 |
| `CSeaAreaAreaView` | `CPushButton` | 海域本身按钮 |
| `CSeaAreaForceView` | `0x98` | 势力条 |
| `CNextAreaView` | `0x98` | 下一根据地（按港口选；`eNextBaseTown` 是港口变量，旧稿「25 槽」撤回） |

`CSeaAreaMapView` ctor 立即数：`+0x98=9`、`+0xb0=0x1a`（26）。9 与港口 BGM 档数相同、26 与 `NUM_TOWNFASHION` 相同，**语义未从字段名确认**。

绘制 `fn_1400018b1`（`fn_14020bbd0.cpp`）：

- 底图 `RouteMap.dk4`（G1T，**8** 张）
- 标记 `RouteMkr.dk4`（G1T，**15** 张）
- 村：循环 `eVillage >= 0x18`，先加载 `RouteMkr` 下标 **`0xd`**
- 港：`eTownKind` 作下标，`RouteMkr` 取 `eTownKind*2` 与 `eTownKind*2+1` 两帧；kind = `fn_14000c71b(town)` = `STown+0x24`（is_capital）`? 1 : 0` `[确认]`（body `0x1402626d0`），即 0 非首都 / 1 首都
- scratch：`aeSeaAreaTown` 388 字节（97×4）、`aeSeaAreaRoute` 800 字节（200×4）、`rcCrustSeaArea`、`eBlindArea`、`eForceColor`

`fn_14000d63e` 再扫 `aeSeaAreaTown` 做计数。[确认] 这是「当前大海域里有哪些港」的运行时列表。

`CSeaAreaScene::vfunc_38` 弹 `InfoBox.dk4` 信息框，与海域曲无关。

大地图 HUD 另用 `OcFrm.DK4`（帆按钮、速度条）和 `IconSC.dk4`（`fn_140002e1e` 写入 `+0xa0=0x62` 空港、`+0xa4=0x19`）。`0x19=25` 与 `NUM_FORCE` 同数，字段名未恢复，禁止写成「当前海域 id」。

### 2.7 `IconSC.dk4` / `NxIcon.dk4`

| 文件 | G1T 纹理数 | 谁引用 |
| --- | --- | --- |
| `IconSC.dk4`（TC/JP/KR 各一份，皆 22 张） | 22 | `FirstGrp.dk4` 配技能六边形；`fn_140014394` 配 `eMapPointKind`；大地图 `fn_140002e1e`；`InfoBox2.dk4` |
| `NxIcon.dk4` | 18 | **SC exe 无 `"NxIcon"` 字符串** |

[确认] `IconSC` 是语言包 UI 字图（`03-localization.md` 的 `icon_texture`），不是海域图标表。`NxIcon` 留在 `Game/` 磁盘上，Steam 简中 exe 不加载；文件名像 Switch（NX）残留。海域 pictogram 的原版落点是 `RouteMkr.dk4`，不是这两个。

`mappoint.dk4`：148 张，给港口场景 12 种 `eMapPointKind`，不进本表。

### 2.8 价格：键是 `(town_id, trading_item_kind_id)`，不是海域

[确认] `CTown+0x4a` 起 19 个 `uint16` 种类指数；`quote` 读港 + 种类 + 特产品槽。来源：`04-economy/02-market-price.md`、`CTown.cpp`、`fn_14025ae30.cpp` RTC `nMarketPrice` / `eTradingItemKind >= 0x13`。

[确认] `CCulture+0x14` 的 105 个 word 是**文化圈卖价底**（`fn_1400097af`→`fn_140013a02`→`fn_140010a23`；初值来自静态文化表 `+0xC`）。数值列归 economy（`culture_prices.yaml`），文化圈行与 `major_sea` 外键归 factions。本篇不拥有。

全仓库价格路径没有海域维。原版**没有**「每海域一张种类价格表」，本篇不提供任何价格字段。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 空 | 内容在哪 |
| --- | --- | --- | --- | --- |
| 七大海域 | `major_sea_id` | 7（+ 全世界视图） | `null` | factions 注册；本篇只引用 key |
| BGM 曲 | `bgm_id` | 观察 0–41 | `null`（原版 `0x2B` 停，legacy） | `res/bgm.ktsl2*` |
| 文化圈 | `culture_id` | 21 | `null`（原版 `0x16`） | factions `cultures.yaml` |
| 种类市价 | `(town_id, kind_id)` | 97×19 | — | economy，本篇只查 |
| 细分海域（扩展） | `sea_area` | 原版 0 | `null` | `01` 的 `sea_areas.yaml` |

运行时循环读表长。

地图 pictogram 种类（原版写死，重构进表长）：

| 键 | 原版上界 | 资源 |
| --- | --- | --- |
| `town_kind` | 2 | `RouteMkr` 偶数/奇数帧 |
| `force_color` | 8 | 海域图旗 |
| `fleet_slot` | 69 | `CSeaAreaFleetView` 指针槽 |
| `village` | 24 | `CSeaAreaVillageView`；另用 `RouteMkr[13]` |

## 4. 算法 / 公式

全部立即数进配置。代码禁止写死 `7`、`21`、`0x14`、`0x2B`。

### 4.1 大地图播曲（本系统拥有）

原版常态按「玩家所属港 → 七大海域」7 选 1，特殊状态用两档固定曲。重构允许细分海域覆盖，缺省回落到原版行为：

```
ocean_bgm(fleet, flags) =
    if flags.special:                          # 原版 fn_140006c12 == 1
        return ocean_alt_bgm_id if flags.alt else ocean_default_bgm_id   # 0x15 / 0x14
    z = SeaArea.at(Fleet.pos(fleet))           # 重构扩展；原版包无细分海域
    if z and z.bgm != null: return z.bgm
    sea = MajorSea.of_town(Fleet.home_town(fleet))   # CPlayerFleetData+0x29 → culture → major_sea
    return major_sea_bgm.get(sea) or major_sea_bgm_fallback   # 原版 sea+4；无 → 0x1B

play_bgm(id):
    if id is null: stop                        # 原版停曲 0x2B 只在 legacy 层
    if 0 <= id < bgm_primary_count:            # vanilla 24
        mixer.play_primary(id)
    elif bgm_primary_count <= id < bgm_total:  # vanilla 42
        mixer.play_secondary(id - bgm_primary_count)
    else: no-op
```

`flags.alt` 对应原版 `fn_14000f26d(..., 3)` 的真假，谓词 [未知]，YAML 留钩子，禁止在代码里写死「3」。

### 4.2 港口曲

```
town_bgm(town_id) =
    if town_id is null: return town_bgm_default            # 原版 0x62 → 0x0B
    c = Towns.def(town_id).culture_id                       # STown+0x25
    return culture_to_town_bgm.get(c) or town_bgm_default
```

`culture_to_town_bgm` 即 §2.4 表，按文化圈 key 配，落 `config/ui/bgm.yaml`。

### 4.3 图示

```
sea_map_town_frames(town_id) =
    k = 1 if Towns.def(town_id).is_capital else 0     # eTownKind；原版循环上界 2
    (route_mkr[k * 2], route_mkr[k * 2 + 1])

sea_map_village_frame = route_mkr[village_marker_index]   # vanilla 0x0D
```

首都判定问 world，不在本表复制 `is_capital`。势力旗问 `Factions.color(force_id)`，本表只提供「海域图用哪张图集」。

### 4.4 价格（只查询）

本系统没有价格表，也不暴露 `kind_rate`。需要报价时直接问 `Economy.quote(town_id, item_id)` / `Economy.kind_rate(town_id, kind_id)`。

## 5. 耦合分析

原版：

- `CSeaAreaScene` 继承航线图，海域 UI 和航线 UI 同一对象
- 绘制一次扫港 / 村 / 航线 / 势力色 / 城镇种类
- 港口曲写死在 `fn_140002cbb` 的 21 路 switch，读港口定义 `+0x25`
- 大地图曲写死 `0x14` / `0x15`，常态曲由 `fn_1401a54e0` 写死「七大海域 +4」
- `IconSC.dk4` 路径写死在 SC exe
- `eSeaArea` 名字与 `major_sea` 实为同一物，又与港口 `culture_id` 链式推导；文档旧稿因 RTC 投票误生出「25 海域」

重构切断：

- factions 拥有七大海域注册；本篇只拥有表现映射（`bgm.yaml`、pictograms）
- ocean 在 `EnteredMajorSea` / `ArrivedAtTown` 之后问 `Bgm.for_ocean` / `Bgm.for_town`，自己不写曲号表
- UI 用图集 key 画 pictogram，不 new `CSeaAreaTownView*`
- 流行、文化圈、105 卖价：只通过势力 / 经济查询
- 市价：只通过经济查询

## 6. 解耦后的配置

`config/ui/bgm.yaml`（本篇拥有；按 key 映射，缺项走默认，加载期校验覆盖率）：

```yaml
schema: dk4.ui.bgm
version: 2
pack: vanilla-core
# 原版播放器（CBackupMusic）。改这些不应改经济/战斗代码。
player:
  primary_count: 24          # 0x18
  total_count: 42            # 0x2A
  assets:
    catalog: res/bgm.ktsl2asbin
    stream: res/bgm.ktsl2stbin
ocean:
  special_default: 20        # COceanScene 0x14（fn_140006c12 == 1）
  special_alt: 21            # COceanScene 0x15（再加 fn_14000f26d(...,3) 真）
  special_alt_predicate: unverified
  major_sea_bgm:             # fn_1401a54e0 / fn_14000da80：七大海域 → 4..10
    north_sea: 4
    mediterranean: 5
    africa: 6
    indian_ocean: 7
    se_asia: 8
    east_asia: 9
    new_world: 10
  fallback: 27               # 0x1B（大地图无港时）
  sea_map_fallback: 4        # 海域图越界回落
  sea_map_world_view_uses: start_major_sea   # 海域图区号 8 时读 CWorldData+0x1e
town:
  default: 11                # 0x0B；无港或 culture 越界
  culture_to_town_bgm:       # fn_140002cbb 21 路 jumptable
    england: 11
    flanders: 11
    germany: 11
    norse: 11
    portugal: 12
    spain: 12
    italy: 12
    greece: 12
    turkey: 13
    egypt: 13
    west_africa: 14
    east_africa: 14
    arabia: 13
    india: 15
    indochina: 16
    indonesia: 16
    china: 17
    korea: 17
    japan: 18
    caribbean: 19
    mexico: 19
  # [推断] 11–19 对齐 Soundtrack DISC04 的 04–12 港曲，未证实文件名
# 原版停曲 / 无备份 0x2B（43）只在 legacy 层；接口用 null
```

`config/ui/sea_map.yaml`（pictograms）：

```yaml
schema: dk4.ui.sea_map
version: 1
pictograms:
  atlas: RouteMkr.dk4        # 15 帧
  map_sheets: RouteMap.dk4   # 8 张
  town_kind_frames: 2        # eTownKind 0/1 各两帧
  force_color_base: 4
  village_marker_index: 13   # 0x0D
  # IconSC.dk4 / NxIcon.dk4 不是本表图集
```

细分海域（`01` 的 `sea_areas.yaml`，原版空）若要覆盖曲目，在该行上加 `bgm: <id>`；原版包不填。

`config/caps.yaml` 片段：

```yaml
bgm_id_max_vanilla: 41
sea_map_town_kinds: 2
sea_map_force_colors: 8
```

语言包继续提供 `icon_texture: IconSC.dk4`，与海域 `icon_id` 无关。

## 7. 独立旋钮

改这些不应迫使贸易/战斗/流行改代码：

- 七大海域 → 曲号表 `major_sea_bgm`（新大海域 = factions 加一行 + 这里加一项）
- 文化圈 → 港口曲 `culture_to_town_bgm`
- 特殊状态两曲、主通道长度
- `RouteMkr` 下标、首都/非首都两帧
- 细分海域的曲目覆盖（扩展）

改「卖流行品加多少份额」走势力/经济。改 `quote()` 走 `formulas/economy.yaml`。

## 8. 对外接口

查询（只传 ID 与整数）：

```
Bgm.for_ocean(fleet_id, flags={special, alt}) -> bgm_id | null
Bgm.for_sea_map(major_sea_id | world_view) -> bgm_id
Bgm.for_town(town_id | null) -> bgm_id
SeaMap.pictogram_atlas() -> { map_sheets, markers }
SeaMap.town_marker(town_id) -> { atlas, frame0, frame1 }  # 内部问 Towns.def(town_id).is_capital
SeaMap.village_marker(village_id) -> { atlas, frame }

# 价格：禁止本系统返回金币。需要报价时：
Economy.quote(town_id, item_id)            # 见 02-market-price.md
Economy.kind_rate(town_id, kind_id)

# 流行：禁止本系统返回 fashion_id
Fashion.current(town_id)                   # 见 02-countries-culture-fashion.md
```

命令：无。曲目切换由 ocean/ui 在收到事件后调 `Audio.play(bgm_id)`。

事件（不由本篇发）：

```
EnteredMajorSea { fleet_id, major_sea_id }   # ocean 发；ui 据此问 Bgm.for_ocean
ArrivedAtTown { fleet_id, town_id }          # ocean 发；ui 问 Bgm.for_town
# 本系统不发「价格变了」「流行变了」
```

不暴露 `CSeaAreaScene*`、`CBackupMusic*`、`CTown*`、`RouteMkr` 帧缓冲。

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| `NUM_SEAAREA` 整数 | 断言无整数；真实循环 `< 7` | `25`（RTC 投票伪影） |
| `CPlayerFleetData+0x29` | `+0x208` 曲号来源港 | 确认「当前所在港」（只证到是港 id 字节） |
| `fn_14000f26d` 第三参 3 | 切换 0x14/0x15 | 确认天气/昼夜/黄金航路 |
| `fn_140006c12 == 1` 的状态名 | 大地图特殊曲门 | 与 `06-ocean` 的海战态说法未对齐 |
| BGM 11–19 = DISC04 04–12 | 档数+地理 | 确认文件名绑定 |
| ktsl2 cue 个数 | asbin 头未还原 | `= 42` 作为容器轨数 |
| `CSeaAreaMapView+0x98=9`、`+0xb0=0x1a` | 立即数 | 确认「九文化 / 二十六流行」 |
| HUD `+0xa4=0x19` | 与 `NUM_FORCE` 同数 | 当前海域 id |
| `RouteMkr` 15 帧除 0–3、4–11、`0xc`、`0xd` 外 | 未逐帧命名 | 海域各一帧 |
| `NxIcon.dk4` | 磁盘有、exe 无字符串 | 原版海域图标包 |
| `IconSC` 22 张内容 | 语言 UI | 海域图标 |
| `SeaAreaMessage` 20 格的下标 | 古地图信息框用 | 七大海域名表 |
| `SEAAREA_BITS` | 断言符号 | 已知用途 |
