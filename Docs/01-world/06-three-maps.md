# 三种地图：航海 / 航线 / 黑块

原版至少有三套**互不相同的地图投影**：出海跟船的航海图、港与航线的示意图、未发现村落上盖的壳/盲区（黑块）。它们读同一套 `town_id` / `route_id` / 原版坐标，但贴图、雾、点击完全不是一张表。重构后三种 `kind` 独立可配，禁止再焊进 `COceanScene` 或 `CRouteMap` 子类。

网格字节本身仍由 `01-map-and-sea-areas.md` 的 `config/world/map.yaml` 拥有。本文件只拥有**怎么画、怎么点、雾/壳何时揭开**。

## 1. 职责边界

管：

- 三种 `map_kind`：`sailing`、`route`、`blackblock`
- 每种的资源 key、帧下标、雾/壳参数、交互（点击、选港、是否跟船）
- 两套（以上）坐标空间的标签与立刻数：原版定位单位、chip 网格环绕
- `World.dat` 取样事实（尺寸、唯一字节、查找用 nibble）；单元格玩法语义仍标 unknown
- 黑块与村落发现位之间的**接口形状**（揭开谓词、每村 2 个壳矩形）

不管：

- `World.dat` 作为世界包单例资源的 schema 头（`config/world/map.yaml`）
- 25 海域多边形与 `aeSeaAreaTown` / `aeSeaAreaRoute` 成员表（`01-map-and-sea-areas.md`）
- 航线边、已建立状态（`05-routes-navigation.md`）
- 村落/遗迹实体与存档位的数据形状（`03-villages-discovery.md`）；本文件只订「位为假时画壳」
- 航行 tick、风向、入港（`config/ocean/sailing.yaml`）
- 市价数字（`CMarketPriceRouteMap` 是 economy 投影，见第 5 节）
- 港口内 12 种 `eMapPointKind` / `mappoint.dk4`（城镇设施点，不是世界三图）
- 古地图道具窗 `OldMap.dk4` / `CGetOldMapWindow`（items + ui）
- 探索圈 `Search.DK4`（ocean search）
- 经纬度显示 `nLatitude` / `nLongitude`（`07-calendar-and-coords.md`）

## 2. 原版事实

### 2.1 三种投影不是同一个类

| kind | 原版宿主 | 体积 | 基类要点 | 等级 |
| --- | --- | --- | --- | --- |
| sailing | `COceanScene` 内嵌 `COceanMapData`（场景 `+0x88`） | 场景 `0x218`；地图数据 `0x10` | `COceanScene -> CScene -> … -> COceanMapData` | `[确认]` `COceanScene.cpp` `class_hierarchy.txt` |
| sailing 视口 | `CMapView` | `0xd8` | `CMapView -> CRectView -> CView` | `[确认]` `CMapView.cpp` |
| route | `CRouteMap` | `0x110` | `CRouteMap -> CView`（**不是** `CRectView`） | `[确认]` `CRouteMap.cpp` |
| route 选港 | `CSelectTownRouteMap` | `0x118` | `+0x110 = 0x62` 未选港哨兵 | `[确认]` `CSelectTownRouteMap.cpp` |
| route 海域 | `CSeaAreaScene` | `0x128` | `CSeaAreaScene -> CRouteMap -> … -> CScene` | `[确认]` `CSeaAreaScene.cpp` |
| route 叠加 | `CSeaAreaMapView` | `0x340` | `CSeaAreaMapView -> CRectView`；ctor 清 `eFleet>=0x45`、`eTownKind>=2`、`eForceColor>=8` | `[确认]` `CSeaAreaMapView.cpp` |
| blackblock | 无独立 Scene | — | 画在航线图/小地图上：RTC `eBlindArea` `sCrustSize` `sCrustPoint` `rcCrustSeaArea` | `[确认]` `fn_1400018b1` `fn_140011824` |
| 市价叠图 | `CMarketPriceRouteMap` | `0x120` | `-> CSelectTownRouteMap -> CRouteMap`；**不是**第四种世界图 | `[确认]` `CMarketPriceRouteMap.cpp` |

`NUM_MAPPOINTKIND = 12`、`INVALID_MAPPOINTKIND = -1`（`MapPointID.h` / `MapPointKindID.h`，断言 `catalog/asserts.txt`）属于港口内地图点，不进本表。

### 2.2 共享 ID，不共享相机

- `[确认]` 三种投影都扫港口 / 村落 / 航线 ID：航海点击里城镇哨兵 `0x62`、村落哨兵 `0x19`（`CMapView::vfunc_16`）；海域图循环 `eTown >= 0x61`、`eVillage >= 0x18`、`eRoute` 表 200（`fn_1400018b1`）。
- `[确认]` 航海相机跟船：`CMapView::vfunc_16` 取 `posView`，鼠标 = 视口 + 事件，再和 `ptPlayer=(0xc4,0xc0)` 做向量。航线图不走这条。
- `[确认]` 航线相机是固定布局立刻数，写在 `CRouteMap` ctor：`+0x80=0x210`、`+0x84=0x19`、`+0x88=0x139`、`+0x8c=0x56`、`+0x90=0x15b`、`+0x94=0x54`、`+0x98=0x4d9`、`+0x9c=0x398`、`+0xa0=0x369`、`+0xa4=0x269`、`+0xa8=0x50e`、`+0xac=0x370`、`+0xb0/+0xb8=0x14c`、`+0xb4/+0xbc=0x2c`、`+0xc0=9`、`+0xc4=8`。属 UI，不进玩法公式。
- `[确认]` 航线输入分发 `CRouteMap::vfunc_13` 按键码区间 `0..5` / `6..0xe` / `0xf..0x16` / `0x16..0x2f` / `0x2f..0x90` / `0x90..0xa8` / `0xa8..0xed` 跳不同 vfunc。航海点击是三块像素矩形 + 相对玩家向量。两套交互表。

### 2.3 坐标空间：原版单位 vs chip 网格（禁止互转公式）

两套空间都在仓库里，**换算未钉死**。禁止用伦敦 `x1=20080` 去除 8 或除 16 当成 `World.dat` 下标。

**空间 A — 原版定位单位**（港口/村落 Locator）

- `[确认]` 伦敦 `x1=20080` `y1=4272`。全表 `x1` 9248–35520，`y1` 3376–13792。来源：`Docs/_data/towns_world0.json`，`01-map-and-sea-areas.md`。
- `[确认]` 黑块投影 `MulDiv(sCrustPoint, ptSize, 0x9c40)` 与 `MulDiv(..., 0x4e20)`。`0x9c40=40000`、`0x4e20=20000`。来源：`fn_140011824`。近港村落筛选用同一对立刻数（`03-villages-discovery.md`）。
- `[确认]` 这是 Locator 空间里的比例尺分母，用来把壳矩形贴到航线图像素，不是 2500 网格宽。

**空间 B — chip / `World.dat` 查找**

- `[确认]` `formats.md`：`World.dat` **6250000** 字节 = **2500 × 2500** `uint8`。
- `[确认]` `fn_140006d93`（`fn_1401821c0.cpp`）`CFile` 打开 `"World.dat"`，把缓冲指针写入地图对象 `+0x8`（即 `COceanMapData`）。失败不装入。释放 `fn_14000d341`。
- `[确认]` chip 行走 X 在 `0x9c4=2500` 处环绕（到 2500 归 0，到 −1 写成 `0x9c3=2499`）。Y 合法区间 `[0, 0x4e2)`，`0x4e2=1250`，越界直接放弃。来源：`fn_140007347` `fn_14000a9e8` `fn_1400140d3` `fn_1400140bf`。
- `[确认]` 查找函数把结果当 **16 位**用（`s34 = ax`，`fn_14000ecdc` 后）。同一点先 `>> 4` 再查格（`fn_14000a9e8` `fn_140014a74`）。该「点」是否等于港口 `x1` **未证明**。
- `[确认]` 取出的字节按 nibble 用：`& 0xf`、`>> 4`，并和 16 宽子格混算（`fn_14000c874` `fn_14000e354`）。随后 `fn_14000d2c9` / `fn_1400100ff` 用解码后的两个分量做可否通行。

因此：内容表坐标继续标 `space: original`。chip 下标是航行碰撞/贴地用的另一套。实现不得写 `grid_x = town.x1 / 8`。

### 2.4 `World.dat` 取样（本机 `Game/World.dat`）

`[确认]` 文件 6250000 字节，MD5 `c917bfa037c410b81c31e403ba48ec8e`。按 `uint8` 读：

| 项 | 值 |
| --- | --- |
| 唯一取值 | **0–255 全部出现**（256 种） |
| 最高频 | `0x00` 3422734（54.764%） |
| 其后 | `0x01` 8.800%；`0x11` 7.484%；`0x10` 7.483%；`0x02` 5.162%；`0x08`/`0x18`/`0x19`/`0x09` 各约 2.14% |
| 前 16 字节 | `08 00 09 00` 重复 |
| 末尾 | `18 00 19 00` 重复 |
| 第 0 行 unique | `{0,8,9}`；第 2499 行 `{0,24,25}` |

`0x00/0x01/0x02/0x08/0x09/0x10/0x11/0x18/0x19` 合计约 92%。这**不像**「陆地/浅海/深海/不可航」那种 4～8 档地形枚举。

若把同一文件改读成 **2500×1250 的 uint16 LE**（字节数仍是 6250000，且与 chip Y=1250 同形）：unique **836**，最大 **917**，第 0 行是 `8,9,8,9,…`，末行 `24,25,24,25,…`。这能解释「查找返回 `ax`」和 Y 上限 1250，但 **formats.md 写的是 uint8 2500²**，不得把 uint16 布局写成确认。

`[推断]` 原始 `uint8` 是打包字段（高低 4 位），通行判定发生在解码之后（`fn_1400100ff` 比较 2–12、13、≤4）。依据：查找路径与直方图，不是表里的枚举名。

`C3DMapData` 体积 `0x450`，ctor 写 `+0x20=0x20`、`+0x24=4`、`+0x28=4`（`C3DMapData.cpp`）。这是 3D 细分立刻数，**不是** `World.dat` 宽高。进 ui，不进世界网格表。

### 2.5 航海图 art / 雾 / 交互

资源（`Game/`，头 `GT1G4600` 除非另注）：

| 文件 | 大小 | G1T `+0x10` `[推断]` 纹理数 | 谁加载 |
| --- | --- | --- | --- |
| `Chip.DK4` | 1547468 | 6 | `COceanSurface` / `COceanTextureSurface` 帧 4；`fn_140004138` 帧 1、2、3、5 |
| `Sky.DK4` | 1966612 | 14 | `fn_14000e737`（`unassigned_named.tsv`）；`CSkySprite` |
| `OcFrm.DK4` | 4071344 | 27 | `CSpeedBarSprite` 帧 3；帆按钮 6–9（`01-sailing.md`） |
| `Weather.dk4` | 2788808 | — | 气候投影，不属本表玩法 |
| `World.dat` | 6250000 | 非 G1T | `COceanMapData` |

`[确认]` `fn_140004138` 还按像素尺寸建离屏面：`0x3c0×0x200`、`0x5c0×0x400`、`0x3e0×0x220`，并构造 `CLandTextureSurface`（`0x38`）。陆地纹理面挂在航海图上，不挂航线图。

`[确认]` `CSkySprite` ctor：某气候/海域 id `== 8` 时内部标志置 1；`+0x40=0x40`、`+0x44=0xf`（`CSkySprite.cpp`）。天空只属于 sailing。

**雾（航海，不是黑块）** `[确认]` `CMapSolidSprite::vfunc_07` RTC：`fogcolor` `srcFog` `sizeFog` `rcFog` `rcMask` `rcDest`。Chip 第 5 帧装进对象 `+0x80` 当雾源。`fn_140012e4f` 返回 0/1/3 时：

- 模式 1：mask 标志 `0x40`
- 模式 3：mask 标志 `0x20`
- `fn_14000e309` 为真：雾色走 `fn_14000baf5`（动态）
- 为假：两套调色板按 `fn_1400118c9` 插值，分量立刻数：

| 模式 | R | G | B |
| --- | --- | --- | --- |
| 0 | `0xdf` | `0xdf` | `0xdf` |
| 1 | `0` | `0` | `0` |
| 3 | `0x1f` | `0x1f` | `0x1f` |
| 其它 | `0xb2` | `0x1f` | `0x1f` |

末段还把 `0x1f` / `0xb2` 送进 `fn_14000baf5` 打包 `fogcolor`。这是**航海图大气雾**，与村落盲区不是同一条数据。

航海点击矩形（`CMapView::vfunc_16`，像素立刻数，属 UI）：`(0x4b7,0x37)–(0x773,0x91)`、`(0x56c,0x8b)–(0x77a,0xf3)`、`(0x595,0xbe)–(0x753,0x433)`。命中后才查 `+0x98` 上的城镇/村落。

### 2.6 航线图 art / 相机 / 市价投影

| 文件 | 大小 | 魔数 | G1T `+0x10` `[推断]` | 加载点 |
| --- | --- | --- | --- | --- |
| `RouteMap.dk4` | 7782716 | `GT1G4600` | 8 | `fn_1400018b1`；小地图 `fn_140014556`；壳层用**帧 7**（`fn_140011824`） |
| `RouteFrm.dk4` | 4798516 | `GT1G4600` | 8 | `fn_140011824` 与 `InfoBox2.dk4` 一起 |
| `RouteMkr.dk4` | 18880 | `GT1G0600` | 15 | 见下表帧分配 |
| `Route.dk4` | 107242 | 非 G1T；约 60% 字节 `0xAB` | — | **反编译字符串里没有** `"Route.dk4"`。文件在 `Game/`，用途 `[未知]` |

`RouteMkr.dk4` 帧 `[确认]` 立即数（`fn_1400018b1` `CMarketPriceRouteMap` ctor）：

| 帧 | 用途 |
| --- | --- |
| `eTownKind*2` 与 `+1`（0/1 与 2/3） | 两种 `eTownKind`（循环 `>= 2`） |
| `eForceColor+4`（4–11） | 8 个势力色 |
| `0xc`（12） | `CMarketPriceRouteMap` 市价钉 |
| `0xd`（13） | 村落标记 |

`[确认]` 海域图画港口前用 `vfunc +0x150` 过滤，命中的 `eTown` 写入 scratch `aeSeaAreaTown`（388 字节）。村落先 `fn_1400100e6`，真则建 `CSeaAreaVillageView`（`fn_14020d170`）。航线精灵 `CSeaAreaRouteSprite`（`fn_14020ce00`）。这些类只投影 `town_id` / `village_id` / `route_id`，不拥有边表。

`[确认]` `CSelectTownRouteMap::vfunc_40` 点击锚 `(0xa0, 0xd0)`；基类 `CRouteMap::vfunc_40` 用 `*(this+0x108)+9`。选港与海域图同一套 `CRouteMap` 布局，不是航海 `CRectView`。

`[确认]` `CMarketPriceRouteMap`：构造加载 `RouteMkr.dk4` 帧 `0xc`；`vfunc_45` 在对象类型 `== 5` 时拼 `"%s%s"` 并走市价字符串；`vfunc_46` RTC `posOffset` `size`，把标价钉在航线标记上。读 `route_id` + 经济报价，**不拥有航线，也不拥有地图网格**。重构归 economy + ui，见 `05-routes-navigation.md`。

`[确认]` 小地图外框像素：全屏 `0×0–0x780×0x438`（1920×1080），内图 `0x160,0x7c–0x620,0x39c`，再套 `0x13a,0x58–0x648,0x3c8`（`fn_140011824`）。`CRouteMap::vfunc_38` InfoBox 矩形 `0x628,0x3b0,0xa8` 资源 `InfoBox.dk4`。`CSeaAreaScene::vfunc_38` 另有 `0x556,0x3b0,0xc0`。

`CSeaAreaScene +0x118` 出现模式 0/1/2（`vfunc_30` `vfunc_36`）：`[未知]` 具体语义（可能是海域图页签）。选中港 `== 0x62` 则不当作有效港。

### 2.7 黑块：盲区 / 壳 / 雾色不是同一层

符号：

- `[确认]` RTC `eBlindArea` 两处循环 `>= 2`（`num_from_rtc.json` 计数 2）。函数：`fn_1400018b1`（海域图，且 `*(obj+0x98)==7` 才画壳）、`fn_140011824`（小地图，无此开关）。
- `[确认]` `eVILLAGEBLINDAREA` / `NUM_VILLAGEBLINDAREA` 头文件整数未恢复（`VillageblindareaID.h`，断言 `0 <= eVal < NUM_VILLAGEBLINDAREA`）。**不能**把 RTC 循环 2 写成 `NUM_VILLAGEBLINDAREA=2`。
- `[确认]` `sCrustSize` `sCrustPoint` `posCrust` `rcCrustSeaArea` 与 `eVillage` 成对出现。`fn_140014a65` 在海域 id 变化时重算 `rcCrustSeaArea` 再调 `fn_14000c9b4`。

壳矩形存在村落对象上 `[确认]` `fn_140003553`（body `0x1402a01c0`）：

```
# eBlindArea==0：村 +0x20 点、+0x28 尺寸；尺寸<=0 则失败
# eBlindArea==1：村 +0x30 点、+0x38 尺寸；尺寸<=0 则失败
```

`CVillage` 体积 `0x48`，Locator 在 `+8`（`CVillage.cpp`）。`fields.tsv` 只记了 `+0x8/+0x10`，**没覆盖**壳字段；以本函数为准。`fn_1400032c9` 按 `eVillage >= 0x18` 从 exe 静态表拷进运行时村对象（含这些偏移）。

**揭开** `[确认]` 控制流，字段名 `[推断]`：

1. `fn_1400100e6(village)` 为真 → 画村落精灵，**跳过**该村全部 `eBlindArea`。
2. 为假 → 对 `eBlindArea=0,1` 调 `fn_140003553`，成功则把点/尺寸投到屏幕并 `fn_14000d5e9` 画壳。
3. `fn_1400100e6` → `fn_14000f286`：对某对象 `+0x8` 做位与，非 0 则返回 1。`CVillageElement` 存档只写 `+0x8`（`03-villages-discovery.md`）。

所以黑块揭开绑定**村落发现位**，不是 `World.dat` 格子被「探索过」。海上探索成功发 `VillageDiscovered` 之后，world 翻位，下一帧航线图不再盖壳。

未证实的揭开路径（不得写进公式当默认真）：

- 持有地图道具 155–173：items 外键未还原（`03-villages-discovery.md`）
- 驶入某 chip / 访问海域 `SEAAREA_BITS`：位宽与「已航行」语义 `[未知]`（`01-sailing.md`）
- 打开 `OldMap.dk4`：那是古地图 UI，不是黑块表
- 航海 `fogcolor`：大气雾，不读发现位

`[确认]` 小地图画壳时用 `RouteMap.dk4` **帧 7** 当源（`fn_140011824` `fn_140006faa(..., 7)`），再 `MulDiv` 到像素。黑块的 art 挂在航线资源包上，但配置必须能离开 `RouteMap.dk4` 单独换一张壳图。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 哨兵 | 内容在哪 |
| --- | --- | --- | --- | --- |
| 地图种类 | `map_kind` | 3（重构） | — | 本表 |
| 世界网格 | 单例文件 | 2500×2500 uint8 或 chip 2500×1250 | — | `map.yaml` + 本表引用 |
| 港口 | `town_id` | 97 | 选港 UI 98 | `towns.yaml` |
| 航线 | `route_id` | 200 | 导航器 201 | `routes.yaml` |
| 村落 | `village_id` | 24 | 25 | `villages.yaml` |
| 盲区槽 | `blind_area_id` | RTC 每村 2；`NUM_VILLAGEBLINDAREA` 未知 | −1 | 本表 + 村行 |
| 海域 | `sea_area_id` | 25 | −1 | `sea_areas.yaml` |
| 势力色（标记帧） | `force_id` 色槽 | 8 | — | factions；本表只引帧公式 |
| 港口种类标记 | `town_kind_id` | 循环 2 | — | `[未知]` 枚举名 |

运行时读表长。原版包：航线图标记帧按上表，模组可加帧但不能假设 `RouteMkr` 只有 15 张。

## 4. 算法 / 公式

全部立刻数进配置。**没有** `original → chip` 的实现公式。

```
chip_wrap_x          = 2500    # 0x9c4
chip_extent_y        = 1250    # 0x4e2，半开区间
world_dat_width      = 2500    # formats.md uint8 视图
world_dat_height     = 2500
nibble_mask          = 15      # 查找用，不是地形枚举
chip_shift           = 4       # 查找路径里的 >>4；禁止套到 town.x1
original_span_x      = 40000   # 0x9c40，壳 MulDiv 分母
original_span_y      = 20000   # 0x4e20
blind_slots_per_village = 2    # eBlindArea 循环；不是 NUM_VILLAGEBLINDAREA
town_none            = 98
village_none         = 25
route_none           = 201
fog_mask_mode1       = 0x40
fog_mask_mode3       = 0x20
```

chip 环绕（航行碰撞，ocean 实现、参数在本表/ocean 公式皆可引用一次）：

```
def wrap_chip_x(x):
    if x == chip_wrap_x: return 0
    if x == -1: return chip_wrap_x - 1
    return x

def chip_in_bounds(x, y):
    return 0 <= y < chip_extent_y
```

航海雾调色（`CMapSolidSprite`，参数化；模式枚举名未恢复，用原版返回值 0/1/3）：

```
fog_palette:
  0:     { r: 0xdf, g: 0xdf, b: 0xdf }
  1:     { r: 0,    g: 0,    b: 0    }
  3:     { r: 0x1f, g: 0x1f, b: 0x1f }
  other: { r: 0xb2, g: 0x1f, b: 0x1f }
fog_pack_lo: 0x1f
fog_pack_hi: 0xb2
```

黑块：

```
def village_discovered(village_id) -> bool:
    # fn_1400100e6 / fn_14000f286：对象+0x8 位与
    return World.village_discovered(village_id)

def blackblock_rects(village_id, map_pixel_size):
    if village_discovered(village_id):
        return []
    out = []
    for slot in village.blind_areas:          # 原版 2 槽
        if slot.size <= 0:
            continue
        # MulDiv(point, map_pixel_size, original_span_*)
        out.append(project_original_rect(slot.point, slot.size, map_pixel_size))
    return out
```

`project_original_rect` 的实现必须用配置里的 `original_span_x/y`，禁止写死 40000。原版只在航线/小地图像素上做这次投影；sailing 是否盖同一套壳 `[未知]`（反编译未在 `CMapSolidSprite` 里看到 `eBlindArea`）。

通行（仅 sailing 读网格；结果给 ocean，不在本系统改航速）：

```
# fn_14000e354：解码 nibble 后
#   fn_14000d2c9 真 或 fn_1400100ff 真 → 不可走
# 谓词字段名未恢复，配置留 passable_decode: unknown
```

## 5. 耦合分析

原版焊死：

- `COceanScene` 内嵌 `COceanMapData`，析构才放 `World.dat`。
- `CSeaAreaScene` **继承** `CRouteMap`，海域 UI = 航线图。
- 黑块画在航线图帧 7 上，矩形却存在 `CVillage` 里。
- 航海雾是 `CMapSolidSprite` + Chip 第 5 帧，和盲区同叫「雾」但不是一张表。
- `CMarketPriceRouteMap` 继承选港航线图，市价 UI 依赖航线相机。
- `RouteMkr` 帧下标 = `force_color+4` / `town_kind*2`，加一种标记要改程序。
- `CMapView` 与 `CRouteMap` 两套像素立刻数写在 ctor。
- 12 个 `eMapPointKind` 指针数组在 `CTownSceneView`，容易被误当成世界地图种类。

重构切断：

| 数据 | 系统 |
| --- | --- |
| 网格文件、原版坐标、海域成员 | world `map.yaml` / `sea_areas.yaml` / `towns.yaml` |
| 三种投影的 art、雾、交互 | world `maps.yaml`（本文件） |
| 发现位 | world 存档（村/遗迹/或许海域 bits） |
| 跟船移动、碰撞读格 | ocean 查 `Map.cell` + `Maps.kind("sailing")` |
| 选港、无航线句子 | ui；边是否存在问 `Routes` |
| 市价钉 | economy 查询 + ui 用 `kind: route` 的相机 |
| Chip/Sky/Route* 贴图容器 | ui 资源包，本表只留 key |

## 6. 解耦后的配置

`config/world/maps.yaml`：

```yaml
schema: dk4.world.maps
version: 1
pack: vanilla-world0

# 坐标只打标签。禁止在本文件写 original→chip 公式。
coordinate_spaces:
  original:
    note: "港口/村落 Locator；伦敦示例不是网格下标"
    example: { town_id: 0, x: 20080, y: 4272 }
    span_x: 40000          # MulDiv 分母 0x9c40
    span_y: 20000          # 0x4e20
  chip:
    wrap_x: 2500           # 0x9c4
    extent_y: 1250         # 0x4e2 半开
    lookup_shift: 4        # 查找路径 >>4；不得套用到 town.x1
    nibble_mask: 15
  world_dat:
    file: packs/vanilla-world0/World.dat
    width: 2500
    height: 2500
    cell: uint8
    bytes: 6250000
    unique_uint8: 256
    cell_meanings: unknown
    alt_uint16_2500x1250: unverified   # 同文件另一读法，禁止当确认布局
  ui_pixel:
    note: "各 kind.layout 立刻数；改分辨率只改这里"

sentinels:
  town_none: 98
  village_none: 25
  route_none: 201

kinds:
  - kind: sailing
    name: 航海地图
    name_i18n: { zh-CN: 航海地图 }
    camera: follow_fleet
    view_base: rect_view          # 原版 CMapView : CRectView
    grid: world_dat               # 只 sailing 读网格
    art:
      tiles: { file: Chip.DK4, frames: [1, 2, 3, 4, 5], g1t_count_inferred: 6 }
      land_surface: CLandTextureSurface
      ocean_surface_frame: 4
      fog_frame: 5
      sky: { file: Sky.DK4, g1t_count_inferred: 14 }
      hud_frame: { file: OcFrm.DK4, g1t_count_inferred: 27 }
      offscreen:
        - { w: 960, h: 512 }      # 0x3c0 x 0x200
        - { w: 1472, h: 1024 }    # 0x5c0 x 0x400
        - { w: 992, h: 544 }      # 0x3e0 x 0x220
    fog:
      type: atmospheric           # 不是发现迷雾
      source: map_solid_sprite
      palette:
        0:     { r: 223, g: 223, b: 223 }
        1:     { r: 0,   g: 0,   b: 0   }
        3:     { r: 31,  g: 31,  b: 31  }
        other: { r: 178, g: 31,  b: 31  }
      mask_flags: { mode_1: 0x40, mode_3: 0x20 }
      pack: { lo: 31, hi: 178 }
      mode_enum: unknown          # fn_140012e4f 返回值
    interaction:
      click_player_vector: true
      click_town: true
      click_village: true
      select_route: false
      hud_hitboxes:               # UI，可迁 config/ui/
        - { x0: 1207, y0: 55,  x1: 1907, y1: 145 }
        - { x0: 1388, y0: 139, x1: 1914, y1: 243 }
        - { x0: 1429, y0: 190, x1: 1875, y1: 1075 }
    shows:
      town_id: true
      route_id: false             # 原版航海不画航线折线 [推断]
      village_id: true
      blackblock: unknown         # 反编译未见 eBlindArea
    passable_decode: unknown

  - kind: route
    name: 航线地图
    name_i18n: { zh-CN: 航线地图 }
    camera: fixed_layout          # CRouteMap : CView，非 CRectView
    view_base: view
    grid: none
    art:
      map: { file: RouteMap.dk4, g1t_count_inferred: 8 }
      frame: { file: RouteFrm.dk4, g1t_count_inferred: 8 }
      markers: { file: RouteMkr.dk4, g1t_count_inferred: 15 }
      route_blob: { file: Route.dk4, format: unknown, not_g1t: true }
      infobox: InfoBox.dk4
      marker_frames:
        town_kind_pair: [0, 1]    # eTownKind*2, *2+1；第二种 +2
        force_color_base: 4       # + eForceColor
        market_price: 12
        village: 13
    layout:                       # CRouteMap ctor；改窗口不必改边表
      fields:
        0x80: 0x210
        0x84: 0x19
        0x88: 0x139
        0x8c: 0x56
        0x90: 0x15b
        0x94: 0x54
        0x98: 0x4d9
        0x9c: 0x398
        0xa0: 0x369
        0xa4: 0x269
        0xa8: 0x50e
        0xac: 0x370
        0xb0: 0x14c
        0xb4: 0x2c
        0xb8: 0x14c
        0xbc: 0x2c
        0xc0: 9
        0xc4: 8
    minimap:
      outer: { w: 1920, h: 1080 }
      inner: { x0: 352, y0: 124, x1: 1568, y1: 924 }
    interaction:
      click_player_vector: false
      select_town: true
      select_route: true
      show_prices: false          # 基类不画价；市价子类另开
      input_dispatch: route_vfunc13
    shows:
      town_id: true
      route_id: true
      village_id: true
      force_color: true
      blackblock: true            # 原版画在这张图上
    scene_mode_field: unknown     # CSeaAreaScene +0x118 = 0/1/2

  - kind: blackblock
    name: 黑块地图
    name_i18n: { zh-CN: 黑块地图 }
    camera: overlay               # 原版寄生 route；重构允许独立层
    view_base: overlay
    grid: none
    art:
      crust_source: { file: RouteMap.dk4, frame: 7 }
      crust_may_replace: true     # 独立旋钮：可换非 RouteMap 图
    fog:
      type: discovery_crust
      source: village_blind_area
    reveal:
      predicate: village_discovered
      on_true: hide_crust_show_village
      on_false: draw_crust_slots
      also_revealed_by:
        visit_chip: unknown
        sea_area_bits: unknown
        map_item: unknown         # items 155–173 外键未还原
        old_map_window: false     # 那是另一套 UI
    blind:
      slots_per_village: 2
      num_village_blind_area: unknown
      village_fields:             # CVillage 体积 0x48
        - { id: 0, point_off: 0x20, size_off: 0x28 }
        - { id: 1, point_off: 0x30, size_off: 0x38 }
      skip_if_size_le: 0
      project_space: original
    interaction:
      clickable: false            # 原版壳不可点 [推断]
    shows:
      town_id: false
      route_id: false
      village_id: true            # 按村行取壳，不画村标
    draw_when_route_flag_eq: 7    # 海域图 *(+0x98)==7；小地图无此门

# 市价不是 kind。UI 在 route 相机上叠 Economy.quote(town_id)。
projections:
  - key: market_price
    uses_kind: route
    class_original: CMarketPriceRouteMap
    marker_frame: 12
    owns_routes: false
    owns_prices: false
```

`config/world/map.yaml` 继续只描述网格文件（见 `01-map-and-sea-areas.md`）。`maps.yaml` 用 `grid: world_dat` 引用，不复制一份 2500² 表。

`config/world/villages.yaml` 已有 `blind_area_id` 槽；提取器按 `+0x20..+0x38` 填 `blind_areas: [{point, size}, …]`，坐标 `space: original`。

`config/ui/scenes.yaml` 只登记场景边：`ocean.sailing` 订 `Maps.kind("sailing")`，`ocean.sea_area` 订 `kind("route")+kind("blackblock")`。禁止 Scene 直接 new `CRouteMap`。

## 7. 独立旋钮

改这些值不应迫使贸易/战斗/航线边表改代码：

- 三种 kind 的资源文件与帧下标（含把黑块从 `RouteMap` 帧 7 换成独立图）
- 航海雾调色板、mask 标志、是否启用大气雾
- 黑块揭开谓词（只发现村 / 将来可加海域 bits / 道具）；默认原版包 = 仅 `village_discovered`
- 每村盲区槽数（原版 2；模组可加）
- `original_span_*`、chip 环绕、`World.dat` 宽高（引擎硬顶另写 `engine_limits.yaml`）
- 航线图 ctor 布局立刻数、小地图像素框
- `RouteMkr` 帧与 `town_kind` / `force_color` 的映射
- 航海 HUD 点击矩形
- 是否在 sailing 上叠加 blackblock（原版未见，默认 false）

改市价公式、改航速、改 200 条航线端点、改 12 个港口地图点，都不必动本表。

## 8. 对外接口

查询（ID + 值，不传 Scene 指针）：

```
Maps.kind(kind) -> MapKindDef
Maps.art(kind) -> { files, frames, fog }
Maps.layout(kind) -> pixel table | follow_fleet
Map.cell_raw(grid_x, grid_y) -> u8 | u16 | None     # 换算未知时拒绝 original 输入
Map.chip_wrap(x, y) -> (x', y') | out_of_range
Blackblock.rects(village_id, kind_pixel_size) -> [rect]
Blackblock.visible_crust(village_id) -> bool        # not discovered && size>0
```

命令：

```
World.mark_village_discovered(village_id)   # 已在 03；本系统只订阅
Maps.reload_pack(pack_id)                   # 换 World.dat / 贴图 key
```

事件：

```
MapKindOpened { kind, sea_area_id | null }
BlackblockCleared { village_id }            # 发现位翻转后 UI 重画
FogModeChanged { sailing_mode }             # 0/1/3，语义未知
# 不发 EnteredSeaArea — 那是 Ocean 的
```

Ocean 问 `Map.cell` 做碰撞；UI 问 `Maps.kind` 做绘制。Economy 只向 route 相机投 `quote(town_id)`。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| `World.dat` 单元格语义 | 6250000 字节，256 种 uint8 | `[未知]`；不像小地形枚举 |
| uint8 2500² vs uint16 2500×1250 | 同文件两种读法 | uint16 为 `[推断]`，禁止当确认 |
| 港口 `x1=20080` → chip 下标 | 查找有 `>>4`，调用点未证是 `x1` | `[未知]`，禁止写公式 |
| `fn_14000d2c9` / `fn_1400100ff` 字段名 | 解码后 2–12、13、≤4 | `[未知]` 通行种类 |
| `NUM_VILLAGEBLINDAREA` | 头文件 | `[未知]`；RTC 每村 2 槽不能当此宏 |
| 发现位精确存档偏移 | `+0x8` 位与 | 位宽/哪一对象 `[未知]` |
| 访问海域 / 走格子 / 古地图是否揭壳 | `SEAAREA_BITS` `OldMap.dk4` | `[未知]` |
| sailing 是否叠黑块 | 无 `eBlindArea` | `[未知]`，默认否 |
| `Route.dk4` | 非 G1T，无加载字符串 | `[未知]` |
| `CSeaAreaScene +0x118` 0/1/2 | 模式分支 | `[未知]` |
| `eTownKind` 两个名字 | 循环 `>= 2` | `[未知]` |
| `fn_140012e4f` 雾模式含义 | 返回 0/1/3 | `[未知]`（天气？时刻？） |
| `C3DMapData +0x20/24/28` | 渲染 | 不进玩法 |
| `aszMapPointName` 12 名 | 城镇点 | 不进本表 |
| 市价钉的像素规则 | `vfunc_46` `posOffset` | ui/economy |
| G1T `+0x10` 是否纹理数 | 文件头 | `[推断]` |

---

### 已确认

- 三种投影：航海 `COceanScene`+`CMapView`+`Chip`/`Sky`/`World.dat`；航线 `CRouteMap` 族 + `RouteMap`/`RouteFrm`/`RouteMkr`；黑块 = 村未发现时的 crust，RTC `eBlindArea` 每村 2 槽。
- 共享 `town_id`/`route_id`/`village_id`/原版坐标；相机、资源、雾、点击分离。
- `World.dat` 6250000 字节；uint8 取满 0–255；chip X 环绕 2500、Y 上限 1250。
- 伦敦坐标是原版单位，不是 2500 下标。
- 航海雾 = `CMapSolidSprite`+Chip 帧 5+四档 RGB；黑块揭开 = `fn_1400100e6` 发现位。
- `CMarketPriceRouteMap` 是经济投影，不是第四种世界图。
- `NUM_MAPPOINTKIND=12` 是港口内点，不是这三种。

### 未知

- 网格字节含义、原版坐标↔chip 公式、uint16 布局是否为真。
- `NUM_VILLAGEBLINDAREA`、发现位布局、海域 bits / 道具 / 走访是否揭壳。
- `Route.dk4`、雾模式 0/1/3 的设计名、航线图 `+0x118` 模式。
- sailing 要不要画黑块（原版未见，配置默认 false）。
