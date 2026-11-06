# 海域表现：音乐、图示、价格查询

本篇拥有每条 `eSeaArea` 的**表现绑定**：进哪条海域播哪首 BGM、海域图上用哪套图标/图示。地理网格、港口坐标、航线拓扑仍归 `01-map-and-sea-areas.md` / `05-routes-navigation.md`。流行模型归势力（`02-factions/02-countries-culture-fashion.md`），本篇只引用。市价曲线归经济（`04-economy/02-market-price.md`）；原版价格表**不是**按海域分的，本篇只暴露查询。

## 1. 职责边界

**管：**

- `sea_areas.yaml` 上与表现有关的字段：`bgm_id`、`icon_id`、可选 `culture_id`、可选 `default_kind_rates[]`
- 海域图 pictogram 的资源键（原版 `RouteMap.dk4` / `RouteMkr.dk4` + `CSeaArea*View`）
- 把「当前海域」翻译成 UI/音频要的 ID，不持有舰队、不持有港口对象

**不管：**

- `World.dat`、原版坐标、25 条海域的多边形（`01-map-and-sea-areas.md`）
- 航线是否已开辟、下一根据地玩法（`05-routes-navigation.md`；`CNextAreaView` 只是槽位 UI）
- 大地图操舵、风向应用（`06-ocean`）
- 流行 26 目录、每港 2 槽、9 个 NPC 文化圈（`02-countries-culture-fashion.md`）
- 市价指数怎么滑、金币怎么报（`02-market-price.md`）；本系统不得存第二套价格矩阵
- `IconSC.dk4` 里技能/地图点/HUD 字图（`12-ui-save`）；语言包路径见 `03-localization.md`
- `CSoundBoard` 音量滑条布局（引擎 UI，不是曲目表）

禁止：用霸者之证「北海 / 地中海 / …」七个大区名去填 25 个 `eSeaArea.name`。

## 2. 原版事实

### 2.1 四套地理 ID，计数都对不上

| 空间 | 原版计数 | 用来干什么 | 本篇 |
| --- | --- | --- | --- |
| `eSeaArea` | 25，`INVALID_SEAAREA=-1` | 航海分区、`SEAAREA_BITS`、`NUM_NEXTBASETOWN=25` | 主键 | [确认] `SeaAreaID.h`、`num_values.json` |
| 港口 `region` / `CCulture` 槽 | 21（`region` 0–20；容器循环 `0x15`） | 国家档、文化块 105 word | **不是**海域 | [确认] `towns_world0.json`、`CCultureElement.cpp` |
| NPC 文化圈 | 9 个文案 | 设施换装 | 势力文档 | [确认] `02-countries-culture-fashion.md` |
| 七大海域 | 7（文案另有「全世界」） | `CForce+0x10`；RTC `eSeaArea` 上界 7；`SeaAreaMessage` | 与 `NUM_SEAAREA=25` 同时存在 | [确认] exe `0x58ab4c`、`CForce.cpp` |

`CCulture` 21 槽对应 **21 档港口 region**，不是 25 海域。[确认] 容器长度 21；[推断] 下标与 `town.region` 对齐，依据见势力文档，本篇不升格。

RTC 局部名 `eSeaArea` 在 `fn_140008b6b` / `fn_14010be70` 里上界是 **7**。[确认] 这 7 个就是七大海域（北海…新大陆），不是把 `NUM_SEAAREA` 改写成 7。25 仍是另一套航海分区。

### 2.2 25 个海域显示名：exe / mesfile 里没有

对 `DK4HD_sc.exe` 做 GBK 扫描、对 `DK4HD_jp.exe` 做 Shift-JIS 扫描、对 `catalog/mesfile_sc.txt` 全文检索「北海海域 / 地中海海域 / バルト海 / イベリア / 紅海 / 南シナ」等：

- **没有** 25 条 `eSeaArea` 显示名表。
- mesfile 里的「北海 / 地中海 / 非洲 / 印度洋 / 东南亚 / 东亚 / 加勒比海 / 波罗的海」全部是霸者之证、闲聊或商品百科，不是海域枚举。来源：`mesfile_sc.txt` `0x1e247` 起。

SC exe `.rdata` 里**实际存在**的海域相关字符串：

霸权大区短名（文件偏移 `0x58ab4c`，VA `0x14058bd4c` 一带）：

| 顺序 | 文案 |
| --- | --- |
| 0 | 北海 |
| 1 | 地中海 |
| 2 | 非洲 |
| 3 | 印度洋 |
| 4 | 东南亚 |
| 5 | 东亚 |
| 6 | 新大陆 |
| 7 | 全世界 |

`fn_140009750`（RTC `SeaAreaMessage`，160 字节指针块）里能解出的长名：[确认] 把抬码里的 UTF-16 伪字面按 GBK 反解：

- 新大陆海域
- 北海海域 / 北海海域周边
- 地中海海域 / 地中海海域周边
- 非洲海域
- 印度洋海域
- 东南亚海域 / 东南亚周边海域 / 东南亚
- 东亚海域

旁边还有「地图所示的位置」「？？？？？？？」。这是 7 区 + 周边变体，不是 25 名。

另一串 13 个地名（`0x5b656c` 起，紧挨交易品短名「小麦」）：佛兰德、北欧、意大利、希腊、土耳其、埃及、西非、东非、印度支那、印尼、朝鲜、加勒比海、墨西哥。缺英格兰 / 汉萨 / 葡西 / 阿拉伯 / 印度 / 中国 / 日本，**对不上** 21 region，也不是 25 海域。禁止拿来填 `sea_areas[].name`。

势力国名另在 `0x538e8c`（英吉利、法兰西、荷兰…印加），属 `02-factions/01-forces.md`。

### 2.3 BGM 运行时：42 号 + 哨兵，不是 25 首海域曲

加载：[确认] `fn_1400037b0`（`fn_14023e3f0.cpp`）打开

- `res/bgm.ktsl2asbin`（目录，16704 字节，头 `KTSR`）
- `res/bgm.ktsl2stbin`（流，约 41.4 MB，内含 `OggS` 页）
- 以及 `res/SFX.ktsl2*`

播放入口：[确认] `fn_1400018a7`（`fn_140241970.cpp`）把当前曲号写入播放器对象再调下层。

`CBackupMusic`（体积由析构 `0x10`；字段 `+0x8`）：[确认] `CBackupMusic.cpp`

| `+0x8` 曲号 | 行为 |
| --- | --- |
| `< 0` | 跳过 |
| `0 .. 0x17`（0–23） | `fn_1400018a7` 播主通道 |
| `0x18 .. 0x29`（24–41） | `fn_140003dff` 再 `fn_14000cf1d(id-0x18, 2, 1)` |
| 然后写回 `0x2B`（43） | 停/空哨兵 |

即运行时曲号 **0–41 共 42 档**，空哨兵 43。asbin 头里的 cue 计数未还原，禁止把 42 写成容器文件里的轨数。

`CSoundBoard`（体积 `0x2c8`）：[确认] 构造把 `+0xb0` 起一长串立即数写成控件坐标（`0x224, 0x113, 0x278…`），继承 `CSurfaceSpriteView`。这是音量板 UI，不是曲目表。

大地图：[确认] `COceanScene::vfunc_05`

- 某状态 `== 0xd`：走 `fn_140003dff`（停）
- 否则若 `fn_140006c12 == 1`：再查 `fn_14000f26d(..., 3)`，真则播 **`0x15`（21）**，假则播 **`0x14`（20）**
- 否则播场景对象 `+0x208` 里存的曲号

构造把 `+0x20c = 0`，体积 `0x218`。[确认] `COceanScene.cpp`。`+0x208` 的写入点未在已还原方法里出现，语义 [未知]（当前海域曲？事件覆盖？）。**没有**「按 `eSeaArea` 0–24 查 25 首不同曲」的表。

海域图场景：[确认] `CSeaAreaScene::vfunc_02` 在 `0 <= s < 7` 时 `fn_14000da80` 再 `fn_1400018a7`。7 是剧本数（`eScenario` 0–6），不是海域。

### 2.4 港口 BGM 按 `town.region`，九档，不按海域

`CTownScene::vfunc_05`：`fn_14000c144` 取当前港 → `fn_140002cbb` → `fn_1400018a7`。[确认] `CTownScene.cpp`。

`fn_140002cbb`（body `0x14011c3b0`，反编译失败，已用 capstone 还原）：

1. 参数是 `town_id`；`== 0x62`（空港哨兵 98）时不读港口
2. 否则 `fn_1400103bb` 取港口对象，`fn_14001055a` 读 **`CTown+0x25`**（region 字节）
3. region `> 0x14`（20）走默认曲 `0x0B`
4. 否则 21 路 jumptable（`0x14011c488`）映射到 `0x0B..0x13`

`fn_14001055a`：[确认] `add rax, 0x25` 再读字节（`fn_14025ae30.cpp` / 反汇编 `0x140260370`）。

| region | 首都（对照） | BGM id |
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

[确认] 跳转目标与立即数。这 9 档**不是** 9 个 NPC 文化圈：埃及（region 9）走伊斯兰曲而势力文档把 region 9 粗分到非洲圈；朝鲜（17）与中国共用曲。港口 BGM 表不属于 `sea_areas.yaml`，实现放 `config/ui/bgm.yaml`（或 cultures 上的 `town_bgm_id`），本篇只记录以免再去海域表里找。

### 2.5 Soundtrack 目录名对不上 25 海域

`Game/Soundtrack/` 是《大航海时代》30 周年合集，多作混装。文件夹名**不是** `eSeaArea` 表。

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

[推断] 九档港口 BGM `0x0B..0x13` 与 DISC04 的 04–12 同序（北欧…新大陆）。依据：档数相同、region 分组与曲名地理一致。未在 exe 里找到「BGM 11 = 该 mp3」的文件名表，**不得**写成确认。

DISC03（地中海 / 近欧 / 太平洋 / 北冰洋 / 印度洋 / 大西洋 / 伊比利亚 / 斯堪的纳维亚…）是 3 的配乐；DISC06 有 `North_Sea` `Mediterranean` `African_Sea`。都不是本作 25 海域名录。

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
| `CNextAreaView` | `0x98` | 下一根据地；与 `NUM_NEXTBASETOWN=25` 同数 `[推断]` 1:1 |

`CSeaAreaMapView` ctor 立即数：`+0x98=9`、`+0xb0=0x1a`（26）。9 与港口 BGM 档数相同、26 与 `NUM_TOWNFASHION` 相同，**语义未从字段名确认**。

绘制 `fn_1400018b1`（`fn_14020bbd0.cpp`）：

- 底图 `RouteMap.dk4`（G1T，**8** 张）
- 标记 `RouteMkr.dk4`（G1T，**15** 张）
- 村：循环 `eVillage >= 0x18`，先加载 `RouteMkr` 下标 **`0xd`**
- 港：`eTownKind` 作下标，`RouteMkr` 取 `eTownKind*2` 与 `eTownKind*2+1` 两帧；kind 只有 0/1（首都 / 非首都，见 `08-town-life/01-facilities.md`）
- scratch：`aeSeaAreaTown` 388 字节（97×4）、`aeSeaAreaRoute` 800 字节（200×4）、`rcCrustSeaArea`、`eBlindArea`、`eForceColor`

`fn_14000d63e` 再扫 `aeSeaAreaTown` 做计数。[确认] 这是「当前海域里有哪些港」的运行时列表，不是 25 套不同图标。

`CSeaAreaScene::vfunc_38` 弹 `InfoBox.dk4` 信息框，与海域曲无关。

大地图 HUD 另用 `OcFrm.DK4`（帆按钮、速度条）和 `IconSC.dk4`（`fn_140002e1e` 写入 `+0xa0=0x62` 空港、`+0xa4=0x19`）。`0x19=25` 与 `NUM_SEAAREA` / `NUM_FORCE` 同数，字段名未恢复，禁止写成「当前海域 id」。

### 2.7 `IconSC.dk4` / `NxIcon.dk4`

| 文件 | G1T 纹理数 | 谁引用 |
| --- | --- | --- |
| `IconSC.dk4`（TC/JP/KR 各一份，皆 22 张） | 22 | `FirstGrp.dk4` 配技能六边形；`fn_140014394` 配 `eMapPointKind`；大地图 `fn_140002e1e`；`InfoBox2.dk4` |
| `NxIcon.dk4` | 18 | **SC exe 无 `"NxIcon"` 字符串** |

[确认] `IconSC` 是语言包 UI 字图（`03-localization.md` 的 `icon_texture`），22 ≠ 25，不是海域图标表。`NxIcon` 留在 `Game/` 磁盘上，Steam 简中 exe 不加载；文件名像 Switch（NX）残留。海域 pictogram 的原版落点是 `RouteMkr.dk4`，不是这两个。

`mappoint.dk4`：148 张，给港口场景 12 种 `eMapPointKind`，不进本表。

### 2.8 价格：键是 `(town_id, trading_item_kind_id)`，不是海域、不是文化

[确认] `CTown+0x4a` 起 19 个 `uint16` 种类指数；`quote` 读港 + 种类 + 特产品槽。来源：`04-economy/02-market-price.md`、`CTown.cpp`、`fn_14025ae30.cpp` RTC `nMarketPrice` / `eTradingItemKind >= 0x13`。

全仓库价格路径没有 `sea_area_id` 维，也没有「按 `eCulture` 下标取市价指数」的确认读法。`CCulture+0x14` 的 105 个 word 语义仍是 [未知]；经济通过 `Culture.itemWord` **查询**，解释权在经济，本篇不拥有。

因此：原版**没有**「每海域一张种类价格表」。`default_kind_rates[]` 只是重构给模组的可选覆盖；原版包必须空，运行时问 `Economy.kind_rate(town_id, kind_id)`。

### 2.9 文化 21 槽 ≠ 海域 25

再钉一次，避免 `culture_id` 被填成海域 id：

- `CCultureElement` 构造 `fn_140007dce(..., stride=0xE8, count=0x15)`
- 存盘按 `eTradingItem` 0–0x68 写 105 个 word
- 港口 `region` 取值 0–20

25 与 21 不是双射。`sea_areas[].culture_id` 在原版包里应是 `null`。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 非法 / 空 | 内容在哪 |
| --- | --- | --- | --- | --- |
| 海域 | `eSeaArea` / `sea_area_id` | 25 | -1 | 主 exe；表现字段本表 |
| BGM 曲 | `bgm_id` | 观察 0–41 | 43=`0x2B` 停 | `res/bgm.ktsl2*` |
| 海域图标 | `icon_id` | 原版无 25 表 | — | 重构键；原版帧在 `RouteMkr.dk4` |
| 文化 | `eCulture` / `culture_id` | 21 `[推断]` | -1 | `factions/cultures.yaml` |
| 种类市价 | `(town_id, kind_id)` | 97×19 | — | economy，本篇只查 |
| 霸权大区 | 无独立头文件 | 7 | — | 叙事；禁止写入 `eSeaArea` |
| 港口 region | 字节 0–20 | 21 | — | `towns.yaml` |

运行时循环读表长。原版包默认 25 条海域。显示名未抽出则 `name: null`，校验允许待补，**不允许**用七大区名凑 25 行。

地图 pictogram 种类（原版写死，重构进表长）：

| 键 | 原版上界 | 资源 |
| --- | --- | --- |
| `town_kind` | 2 | `RouteMkr` 偶数/奇数帧 |
| `force_color` | 8 | 海域图旗 |
| `fleet_slot` | 69 | `CSeaAreaFleetView` 指针槽 |
| `village` | 24 | `CSeaAreaVillageView`；另用 `RouteMkr[13]` |

## 4. 算法 / 公式

全部立即数进配置。代码禁止写死 `25`、`21`、`0x14`、`0x2B`。

### 4.1 进海域播曲（本系统拥有）

原版大地图并不按 25 海域换曲，只用两档默认 + 一个场景缓存。重构允许每海域一条，缺省回落到原版行为：

```
ocean_bgm(sea_area_id, flags) =
    if flags.override_id != empty: return flags.override_id     # 事件 / 场景 +0x208
    if sea_areas[sea_area_id].bgm_id != null: return that
    if flags.alt: return ocean_alt_bgm_id      # vanilla 0x15
    return ocean_default_bgm_id                # vanilla 0x14

play_bgm(id):
    if id == bgm_stop_id: stop                 # vanilla 0x2B
    if 0 <= id < bgm_primary_count:            # vanilla 24
        mixer.play_primary(id)
    elif bgm_primary_count <= id < bgm_total:  # vanilla 42
        mixer.play_secondary(id - bgm_primary_count)
    else: no-op
```

`flags.alt` 对应原版 `fn_14000f26d(..., 3)` 的真假，谓词 [未知]，YAML 留钩子，禁止在代码里写死「3」。

### 4.2 港口曲（不拥有，只引用）

```
town_bgm(town_id) =
    if town_id == town_empty_id: return town_bgm_default   # 0x62 → 0x0B
    r = Town.region(town_id)                               # CTown+0x25
    return region_to_town_bgm[r] or town_bgm_default
```

`region_to_town_bgm` 即 §2.4 表。配置落 `config/ui/bgm.yaml`，改它不必动 `sea_areas.yaml`。

### 4.3 图示

```
sea_map_town_frames(town_id) =
    k = 1 if Town.is_capital(town_id) else 0     # eTownKind；原版循环上界 2
    (route_mkr[k * 2], route_mkr[k * 2 + 1])

sea_map_village_frame = route_mkr[village_marker_index]   # vanilla 0x0D
```

首都判定问 world，不在本表复制 `is_capital`。势力旗问 `Factions.color(force_id)`，本表只提供「海域图用哪张图集」。

### 4.4 价格（只查询）

```
# 本系统没有 kind_rate 表
SeaArea.kind_rate(sea_area_id, kind_id) 禁止存在

# 正确：
for town_id in SeaArea.towns(sea_area_id):
    yield Economy.kind_rate(town_id, kind_id)

# 模组可选：sea_areas[].default_kind_rates
# 仅当 Economy 没有该港指数时作为开局种子；原版包必须 []
```

`CCulture` 的 105 word 继续走 `Culture.itemWord(culture_id, trading_item_id)`，本篇不解释。

## 5. 耦合分析

原版：

- `CSeaAreaScene` 继承航线图，海域 UI 和航线 UI 同一对象
- 绘制一次扫港 / 村 / 航线 / 势力色 / 城镇种类
- 港口曲写死在 `fn_140002cbb` 的 21 路 switch，读 `CTown+0x25`
- 大地图曲写死 `0x14` / `0x15`
- `IconSC.dk4` 路径写死在 SC exe
- RTC 把 7 区循环也标成 `eSeaArea`，和 25 槽同名

重构切断：

- world 拥有 `sea_area_id` 定义 + 本篇的表现字段
- ocean 在 `EnteredSeaArea` 之后问 `SeaArea.bgm(sea_area_id)`，自己不写曲号表
- UI 用 `icon_id` / 图集 key 画 pictogram，不 new `CSeaAreaTownView*`
- 流行、文化圈、105 word：只通过势力查询
- 市价：只通过经济查询
- 七大区：叙事包，ID 空间独立（`hegemony_region_id` 0–6）

## 6. 解耦后的配置

`config/world/sea_areas.yaml` 的地理字段仍以 `01-map-and-sea-areas.md` 为准；本篇把表现列补全。同一文件，两篇文档分列所有权：地理列归 01，表现列归本篇。

```yaml
# config/world/sea_areas.yaml
schema: dk4.world.sea_areas
version: 1
pack: vanilla-world0

# 原版播放器（CBackupMusic）。改这些不应改经济/战斗代码。
bgm:
  primary_count: 24          # 0x18
  total_count: 42            # 0x2A
  stop_id: 43                # 0x2B
  ocean_default_id: 20       # COceanScene 0x14
  ocean_alt_id: 21           # COceanScene 0x15
  ocean_alt_predicate: unverified   # fn_14000f26d 第三参 3
  assets:
    catalog: res/bgm.ktsl2asbin
    stream: res/bgm.ktsl2stbin

pictograms:
  atlas: RouteMkr.dk4        # 15 帧
  map_sheets: RouteMap.dk4   # 8 张
  town_kind_frames: 2        # eTownKind 0/1 各两帧
  village_marker_index: 13   # 0x0D
  # IconSC.dk4 / NxIcon.dk4 不是本表图集

# 霸权 7 区：禁止写入下面 name
# hegemony: 北海 地中海 非洲 印度洋 东南亚 东亚 新大陆

sea_areas:
  - id: 0
    key: sea_00
    name: null                 # 25 名未从 exe/mesfile 抽出
    name_i18n: null
    bgm_id: null               # null = 用 ocean_default_id / ocean_alt_id
    icon_id: null              # 原版无 25 套海域图标
    culture_id: null           # 25 ≠ 21，原版包禁止填
    default_kind_rates: []     # 原版无此维；非空只给模组当开局种子
    next_base_town_id: null
    town_ids: []
    village_ids: []
    route_ids: []
    unknown_bytes: []
  # id 1..24 同样占位。校验：len(default_kind_rates) 为 0 或等于 trading_item_kind 表长。
```

港口曲（**不是**本文件，避免一张表既管海域又管港口）：

```yaml
# config/ui/bgm.yaml  片段；所属 12-ui-save，本篇引用
schema: dk4.ui.bgm
version: 1
town_empty_id: 98            # 0x62
town_bgm_default: 11         # 0x0B
# region 0–20 → bgm_id；原版 jumptable
region_to_town_bgm:
  0: 11
  1: 11
  2: 11
  3: 11
  4: 12
  5: 12
  6: 12
  7: 12
  8: 13
  9: 13
  10: 14
  11: 14
  12: 13
  13: 15
  14: 16
  15: 16
  16: 17
  17: 17
  18: 18
  19: 19
  20: 19
# [推断] 11–19 对齐 Soundtrack DISC04 的 04–12 港曲，未证实文件名
```

`config/caps.yaml` 片段：

```yaml
sea_area_count_vanilla: 25
bgm_id_max_vanilla: 41
bgm_stop_id_vanilla: 43
sea_map_town_kinds: 2
sea_map_force_colors: 8
```

语言包继续提供 `icon_texture: IconSC.dk4`，与海域 `icon_id` 无关。

## 7. 独立旋钮

改这些不应迫使贸易/战斗/流行改代码：

- 每海域 `bgm_id`（可多海域共享同一曲，原版就是这样）
- `ocean_default_id` / `ocean_alt_id` / 停曲哨兵 / 主通道长度
- `icon_id`、`RouteMkr` 下标、首都/非首都两帧
- 是否给海域填 `culture_id`（原版包保持 null；模组可挂）
- `default_kind_rates` 是否作为新游戏种子（原版包空）

改「卖流行品加多少份额」走势力/经济。改 `quote()` 走 `formulas/economy.yaml`。改九档港口曲走 `ui/bgm.yaml`。

## 8. 对外接口

查询（只传 ID 与整数）：

```
SeaArea.by_id(sea_area_id) -> SeaAreaDef
SeaArea.bgm(sea_area_id, flags={alt, override_id}) -> bgm_id
SeaArea.icon(sea_area_id) -> icon_id | null
SeaArea.culture(sea_area_id) -> culture_id | invalid   # 原版包恒 invalid
SeaArea.pictogram_atlas() -> { map_sheets, markers }
SeaArea.town_marker(town_id) -> { atlas, frame0, frame1 }  # 内部问 World.is_capital
SeaArea.village_marker(village_id) -> { atlas, frame }

# 价格：禁止本系统返回金币。需要报价时：
Economy.quote(town_id, item_id)            # 见 02-market-price.md
Economy.kind_rate(town_id, kind_id)

# 流行：禁止本系统返回 fashion_id
Fashion.current(town_id)                   # 见 02-countries-culture-fashion.md
```

命令：无。曲目切换由 ocean/ui 在收到事件后调 `Audio.play(bgm_id)`。

事件：

```
EnteredSeaArea { fleet_id, sea_area_id }   # ocean 发；ui 据此问 SeaArea.bgm
# 本系统不发「价格变了」「流行变了」
```

不暴露 `CSeaAreaScene*`、`CBackupMusic*`、`CTown*`、`RouteMkr` 帧缓冲。

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| 25 个 `eSeaArea` 显示名 | exe/mesfile 无表 | 用七大区或攻略私名当确认 |
| 25 条边界多边形 | 不在 World DLL | 本篇拥有几何 |
| RTC `eSeaArea` 循环上界 7 | 两处函数 | `NUM_SEAAREA=7` |
| `NUM_NEXTBASETOWN` 与海域 1:1 | 仅同为 25 | 确认双射 |
| `COceanScene+0x208` | 只读到播放 | 确认「当前海域 BGM」 |
| `fn_14000f26d` 第三参 3 | 切换 0x14/0x15 | 确认天气/昼夜/黄金航路 |
| BGM 11–19 = DISC04 04–12 | 档数+地理 | 确认文件名绑定 |
| ktsl2 cue 个数 | asbin 头未还原 | `= 42` 作为容器轨数 |
| `CSeaAreaMapView+0x98=9`、`+0xb0=0x1a` | 立即数 | 确认「九文化 / 二十六流行」 |
| HUD `+0xa4=0x19` | 与 25 同数 | 确认当前 `sea_area_id` |
| `RouteMkr` 15 帧除 0–3 与 `0xd` 外 | 未逐帧命名 | 25 海域各一帧 |
| `NxIcon.dk4` | 磁盘有、exe 无字符串 | 原版海域图标包 |
| `IconSC` 22 张内容 | 语言 UI | 海域图标 |
| `CCulture` 105 word | 与交易品同数 | 「海域物价倍率」 |
| `default_kind_rates` 原版有值 | 价格在港口上 | 原版按海域定价 |
| `culture_id` 与 `sea_area_id` | 21 vs 25 | 确认 1:1 |
| 13 地名（佛兰德…墨西哥） | 残缺 region 样 | 21 国名或 25 海域名 |
| `SeaAreaMessage` 160 字节剩余指针 | 部分解出 7 区 | 25 名表 |
)
