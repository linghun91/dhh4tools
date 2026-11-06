# 三种地图：航海 / 航线 / 黑块

原版至少有三套**互不相同的地图投影**：出海跟船的航海图、港与航线的示意图、未发现村落上盖的壳/盲区（黑块）。它们读同一套 `town_id` / `route_id` / 原版坐标，但贴图、雾、点击完全不是一张表。重构后三种 `kind` 独立可配，禁止再焊进 `COceanScene` 或 `CRouteMap` 子类。

网格字节本身仍由 `01-map-and-sea-areas.md` 的 `config/world/map.yaml` 拥有。本文件只拥有**怎么画、怎么点、雾/壳何时揭开**。

## 1. 职责边界

管：

- 三种 `map_kind`：`sailing`、`route`、`blackblock`
- 每种的资源 key、帧下标、雾/壳参数、交互（点击、选港、是否跟船）
- 投影用的坐标空间标签与像素立刻数（壳 `MulDiv` 分母、UI 布局）
- `World.dat` 取样事实（尺寸、统计）；网格 schema、`/16` 换算与格值分类归 `01-map-and-sea-areas.md` 的 `map.yaml`
- 黑块与村落发现位之间的**接口形状**（揭开谓词、每村 2 个壳矩形）

不管：

- `World.dat` 作为世界包单例资源的 schema 头（`config/world/map.yaml`）
- 七大海域与 `aeSeaAreaTown` / `aeSeaAreaRoute` 成员（factions 注册；`01-map-and-sea-areas.md`）
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

### 2.3 坐标空间：原版单位 vs chip 网格

两套空间都在仓库里。航行坐标 → chip 是**向 0 截断的 `/16`**（`[确认]`，见下；配置落点是 `map.yaml` 的 `coordinate.units_per_chip`）；航行坐标与港口 Locator 是否同一空间仍是 `[推断]`（强依据）。禁止 `/8`。

**空间 A — 原版定位单位**（港口/村落 Locator）

- `[确认]` 伦敦 `x1=20080` `y1=4272`。全表 `x1` 9248–35520，`y1` 3376–13792。来源：`Docs/_data/towns_world0.json`，`01-map-and-sea-areas.md`。
- `[确认]` 黑块投影 `MulDiv(sCrustPoint, ptSize, 0x9c40)` 与 `MulDiv(..., 0x4e20)`。`0x9c40=40000`、`0x4e20=20000`。来源：`fn_140011824`。近港村落筛选用同一对立刻数（`03-villages-discovery.md`）。
- `[确认]` 这是 Locator 空间里的比例尺分母，用来把壳矩形贴到航线图像素，不是 2500 网格宽。

**空间 B — chip / `World.dat` 查找**

- `[确认]` `World.dat` 是 **2500 × 1250 的 `uint16` LE**（6250000 字节）。证据：唯一读取函数 `fn_14000ecdc`（body `0x140183740`）= `movzx eax, WORD PTR [buf + (y*0x9c4 + x)*2]`。`formats.md` 写的「2500×2500 `uint8`」是错的（需该文件修正）。
- `[确认]` `fn_140006d93`（`fn_1401821c0.cpp`）`CFile` 打开 `"World.dat"`，把缓冲指针写入地图对象 `+0x8`（即 `COceanMapData`）。失败不装入。释放 `fn_14000d341`。
- `[确认]` chip 行走 X 在 `0x9c4=2500` 处环绕（到 2500 归 0，到 −1 写成 `0x9c3=2499`）。Y 合法区间 `[0, 0x4e2)`，`0x4e2=1250`，越界直接放弃。来源：`fn_140007347` `fn_14000a9e8` `fn_1400140d3` `fn_1400140bf`。
- `[确认]` 航行坐标 → chip：`fn_140014a74`（body `0x1401846b0`，RTC `ptWorldChip`）、`fn_14000a9e8`（body `0x140183d20`，RTC `ptChipNext,ptChipIn,ptWorldChip`）对点的 x、y 各做 `cdq; and edx,0xf; add; sar 4`，即**向 0 截断的 `/16`**；子格偏移 = `pos - chip*16`（0–15），`fn_14000a9e8` 再判 `< 5` / `>= 0xb`（`0x140183dfa` / `0x140183e5b`，y 同理）决定是否查邻格。`fn_140002a54`（RTC `posPlayerFleet,posChipBase,posChip,posCrust`）把玩家舰队点 `/0x10` 得 chip、`*0x10` 回到世界点。
- `[推断]` 航行坐标与港口 Locator 同一空间：`2500*16 = 40000`、`1250*16 = 20000`，恰等于黑块 `MulDiv` 分母和经纬全幅（`07-calendar-and-coords.md`：40000 单位 = 360°，20000 = 180°）。伦敦 `(20080,4272)` → chip `(1255,267)`。未在同一函数里看到「舰队点 == 港口 x1」的比较，故不升格。
- `[确认]` 单元格解码：`v` 是 16 位**贴图编号**，`col = v & 0xf`、`row = v >> 4`（`fn_14000e354` / `fn_140003738` / `fn_140011ec8` 经 `fn_14001214d` 组点）。两个谓词：
  - `fn_14000d2c9`（`0x140184830`）：`col <= 1 && row <= 1`（即 `v ∈ {0,1,16,17}`）
  - `fn_1400100ff`（`0x140183cb0`）：`2 <= row <= 12`，或 `row == 13 && (col <= 4 || col >= 13)`
  - `fn_14000e354` = 两者皆假 → 真；`fn_140014a74` 遇真返回 0（不可进）。

因此：内容表坐标继续标 `space: original`。chip 下标由 `chip = trunc(pos / units_per_chip)`（原版 16）得到，参数在 `map.yaml`。实现不得写 `/8`。

### 2.4 `World.dat` 取样（本机 `Game/World.dat`）

`[确认]` 文件 6250000 字节，MD5 `c917bfa037c410b81c31e403ba48ec8e`。下面先给旧的 `uint8` 读法统计（**错误布局**，仅留作对照）：

| 项 | 值 |
| --- | --- |
| 唯一取值 | **0–255 全部出现**（256 种） |
| 最高频 | `0x00` 3422734（54.764%） |
| 其后 | `0x01` 8.800%；`0x11` 7.484%；`0x10` 7.483%；`0x02` 5.162%；`0x08`/`0x18`/`0x19`/`0x09` 各约 2.14% |
| 前 16 字节 | `08 00 09 00` 重复 |
| 末尾 | `18 00 19 00` 重复 |
| 第 0 行 unique | `{0,8,9}`；第 2499 行 `{0,24,25}` |

`0x00/0x01/0x02/0x08/0x09/0x10/0x11/0x18/0x19` 合计约 92%。这**不像**「陆地/浅海/深海/不可航」那种 4～8 档地形枚举。

`[确认]` 正确读法 **2500×1250 的 uint16 LE**：unique **836**，最大 **917**（`row` 最大 57），第 0 行 `8,9,8,9,…`，末行 `24,25,24,25,…`。最高频 `0`、`16`、`1`、`17` 各约 46.7 万格（合计 59.8%，正是 `fn_14000d2c9` 集合）；`fn_1400100ff` 集合 1.7%；其余 38.5%。

`[推断]` 三类分别是开阔海 / 沿岸可航带 / 陆地。依据：占比与 `fn_140014a74` 遇「两者皆假」即拒绝进入。谓词名原版未恢复。

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

- `[确认]` RTC `eBlindArea` 两处循环 `>= 2`。函数：`fn_1400018b1`（海域图，且 `*(obj+0x98)==7` 才画壳）、`fn_140011824`（小地图，无此开关）。
- `[确认]` `eVILLAGEBLINDAREA` / `NUM_VILLAGEBLINDAREA` 头文件整数未恢复（`VillageblindareaID.h`，断言 `0 <= eVal < NUM_VILLAGEBLINDAREA`）。`[推断]` `NUM_VILLAGEBLINDAREA = 2`（证据类型：两处真实循环上界 + 静态村表每条恰 2 块；不是投票值），与 `01` / `03` 一致。
- `[确认]` `sCrustSize` `sCrustPoint` `posCrust` `rcCrustSeaArea` 与 `eVillage` 成对出现。`fn_140014a65` 在海域图区号（七大海域）变化时重算 `rcCrustSeaArea` 再调 `fn_14000c9b4`。

壳矩形存在村落对象上 `[确认]` `fn_140003553`（body `0x1402a01c0`）：

```
# eBlindArea==0：村 +0x20 点、+0x28 尺寸；尺寸<=0 则失败
# eBlindArea==1：村 +0x30 点、+0x38 尺寸；尺寸<=0 则失败
```

`CVillage` 体积 `0x48`，Locator 在 `+8`（`CVillage.cpp`）。`[确认]` 两块壳来自 exe 静态村表 `0x140687b40` 记录 `+0x10..+0x2c` 的 8 个 `int32`，`fn_1400032c9` 逐个 `imul 0x10` 后写入 `+0x20..+0x3c`：静态数据按 chip 单位编写、×16 进原版空间，这又一次支持「原版单位 = chip×16」。数值见 `09-remains-oldmaps-catalog.md` §2.3。`fields.tsv` 只记了 `+0x8/+0x10`，**没覆盖**壳字段；以本函数为准。`fn_1400032c9` 按 `eVillage >= 0x18` 从 exe 静态表拷进运行时村对象（含这些偏移）。

**揭开** `[确认]` 控制流，字段名 `[推断]`：

1. `fn_1400100e6(village)` 为真 → 画村落精灵，**跳过**该村全部 `eBlindArea`。
2. 为假 → 对 `eBlindArea=0,1` 调 `fn_140003553`，成功则把点/尺寸投到屏幕并 `fn_14000d5e9` 画壳。
3. `fn_1400100e6` → `fn_14000f286`：对 `CVillageElement+0x8`（u32，每村一位）做位与，非 0 则返回 1。存档只写这一个 u32（`03-villages-discovery.md`）。

所以黑块揭开绑定**村落发现位**，不是 `World.dat` 格子被「探索过」。海上探索成功发 `VillageDiscovered` 之后，world 翻位，下一帧航线图不再盖壳。

未证实的揭开路径（不得写进公式当默认真）：

- 持有地图道具 155–173：items 外键未还原（`03-villages-discovery.md`）
- 驶入某 chip / `SEAAREA_BITS`（七大海域位集 `[推断]`，读写方未找到）：「已航行」语义 `[未知]`（`01-sailing.md`）
- 打开 `OldMap.dk4`：那是古地图 UI，不是黑块表
- 航海 `fogcolor`：大气雾，不读发现位

`[确认]` 小地图画壳时用 `RouteMap.dk4` **帧 7** 当源（`fn_140011824` `fn_140006faa(..., 7)`），再 `MulDiv` 到像素。黑块的 art 挂在航线资源包上，但配置必须能离开 `RouteMap.dk4` 单独换一张壳图。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 哨兵 | 内容在哪 |
| --- | --- | --- | --- | --- |
| 地图种类 | `map_kind` | 3（重构） | — | 本表 |
| 世界网格 | 单例文件 | chip 2500×1250，`uint16` 贴图编号 | — | `map.yaml` + 本表引用 |
| 港口 | `town_id` | 97 | `null`（原版选港 UI 98，legacy） | `towns.yaml` |
| 航线 | `route_id` | 200 | `null`（原版导航器 201，legacy） | `routes.yaml` |
| 村落 | `village_id` | 24 | `null`（原版 25，legacy） | `villages.yaml` |
| 盲区槽 | 村行 `blind_areas[]` 下标 | 每村 2 `[推断]`（循环上界） | 列表短即可 | `villages.yaml` |
| 七大海域 | `major_sea_id` | 7（区号 7 = 全世界视图） | — | factions |
| 势力色（标记帧） | `force_id` 色槽 | 8 | — | factions；本表只引帧公式 |
| 港口种类标记 | 由 `is_capital` 派生 | 2（0 非首都 / 1 首都，`fn_14000c71b`） | — | `towns.yaml` |

运行时读表长。原版包：航线图标记帧按上表，模组可加帧但不能假设 `RouteMkr` 只有 15 张。

## 4. 算法 / 公式

全部立刻数进配置。网格与换算参数只在 `map.yaml` 定义一次（`grid.*`、`coordinate.units_per_chip`、`grid.cell_classes`、`grid.sub_cell_probe`），本表引用，不复制：

```
# map.yaml（01 拥有）
grid.width / height        = 2500 / 1250   # 0x9c4 / 0x4e2；X 环绕，Y 半开
coordinate.units_per_chip  = 16            # 航行坐标 / 16（向 0 截断）→ chip；子格 0..15
grid.cell_decode           = col = v & 15, row = v >> 4
grid.sub_cell_probe        = { lo: 5, hi: 11 }   # 子格 < 5 或 >= 11 才看邻格
# maps.yaml（本篇拥有）
original_span_x      = 40000   # 0x9c40，壳 MulDiv 分母（加载期校验 == map.coordinate.world_units）
original_span_y      = 20000   # 0x4e20
fog_mask_mode1       = 0x40
fog_mask_mode3       = 0x20
```

chip 环绕与解码（航行碰撞由 ocean 调 `Map.*`；实现只有一份，在 world）：

```
def chip_of(pos):                      # fn_140014a74 / fn_14000a9e8
    return (trunc_div(pos.x, map.units_per_chip), trunc_div(pos.y, map.units_per_chip))

def cell(x, y) -> u16:                 # fn_14000ecdc
    return world_dat[y * map.grid.width + x]

def tile_class(v):                     # 谓词集合在 map.yaml grid.cell_classes，不写死
    if matches(v, cell_classes.open_sea): return open_sea   # fn_14000d2c9
    if matches(v, cell_classes.shore):    return shore      # fn_1400100ff
    return land                                             # fn_14000e354 为真

def wrap_chip_x(x):
    if x == map.grid.width: return 0
    if x == -1: return map.grid.width - 1
    return x

def chip_in_bounds(x, y):
    return 0 <= y < map.grid.height
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
    return Village.discovered(village_id)

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
# fn_14000e354：解码 (col,row) 后
#   fn_14000d2c9 真 或 fn_1400100ff 真 → e354 返回 0；两者皆假 → 1
#   fn_140014a74：e354 为 1 → 不可进（返回 0）
# 见上 tile_class（open_sea / shore / land 三名与 01 一致；原版谓词名未恢复）
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
| 网格文件、原版坐标、`/16`、格值分类 | world `map.yaml` / `towns.yaml`；七大海域成员归 factions |
| 三种投影的 art、雾、交互 | world `maps.yaml`（本文件） |
| 发现位 | world 存档（村 u32 位、遗迹 u8 位；`SEAAREA_BITS` 未证） |
| 跟船移动、碰撞读格 | ocean 查 `Map.cell` + `Maps.kind("sailing")` |
| 选港、无航线句子 | ui；边是否存在问 `Routes` |
| 市价钉 | economy 查询 + ui 用 `kind: route` 的相机 |
| Chip/Sky/Route* 贴图容器 | ui 资源包，本表只留 key |

## 6. 解耦后的配置

`config/world/maps.yaml`：

```yaml
schema: dk4.world.maps
version: 2
pack: vanilla-core

# 网格、/16 换算、格值分类只在 map.yaml（01）。本文件只引用并给投影用的比例尺。
coordinate_spaces:
  original:
    note: "港口/村落 Locator；伦敦示例不是网格下标"
    example: { town: vanilla:london, x: 20080, y: 4272 }
    span_x: 40000          # MulDiv 分母 0x9c40；加载期校验 == map.coordinate.world_units.width
    span_y: 20000          # 0x4e20
  chip: { ref: map.grid }  # 2500×1250，X 环绕；units_per_chip 见 map.coordinate
  ui_pixel:
    note: "各 kind.layout 立刻数；改分辨率只改这里"

# 原版哨兵 98 / 25 / 201 不进本表，只在 config/ids/legacy_codec.yaml

kinds:
  - kind: sailing
    name: 航海地图
    name_i18n: { zh-CN: 航海地图 }
    camera: follow_fleet
    view_base: rect_view          # 原版 CMapView : CRectView
    grid: map.grid                # 只 sailing 读网格（01 map.yaml）
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
    passable_decode: map.grid.cell_classes

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
      slots_per_village: 2        # 原版；[推断] NUM_VILLAGEBLINDAREA=2（循环上界）
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
  - key: vanilla:market_price
    uses_kind: route
    class_original: CMarketPriceRouteMap
    marker_frame: 12
    owns_routes: false
    owns_prices: false
```

`config/world/map.yaml` 是网格文件、`/16` 换算与格值分类的唯一定义处（见 `01-map-and-sea-areas.md`）。`maps.yaml` 用 `grid: map.grid` 引用，不复制一份 2500×1250 表或谓词。

`config/world/villages.yaml`（schema 见 `03`）的 `blind_areas: [{x,y,w,h}, …]` 已是原版单位（exe 源 chip 单位 ×16），本篇直接投影。

`config/ui/scenes.yaml` 只登记场景边：`ocean.sailing` 订 `Maps.kind("sailing")`，`ocean.sea_area` 订 `kind("route")+kind("blackblock")`。禁止 Scene 直接 new `CRouteMap`。

## 7. 独立旋钮

改这些值不应迫使贸易/战斗/航线边表改代码：

- 三种 kind 的资源文件与帧下标（含把黑块从 `RouteMap` 帧 7 换成独立图）
- 航海雾调色板、mask 标志、是否启用大气雾
- 黑块揭开谓词（只发现村 / 将来可加海域 bits / 道具）；默认原版包 = 仅 `village_discovered`
- 每村盲区槽数（原版 2；模组可加）
- `original_span_*`（本表）；`units_per_chip`、chip 环绕、`World.dat` 宽高、格值三类集合在 `map.yaml`（引擎硬顶另写 `engine_limits.yaml`）
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
# Map.chip_of / Map.cell / Map.cell_raw / Map.tile_class 见 01 §8（唯一定义）；类名 open_sea | shore | land
Blackblock.rects(village_id, kind_pixel_size) -> [rect]
Blackblock.visible_crust(village_id) -> bool        # not discovered && size>0
```

命令：

```
World.mark_village_discovered(village_id, via, fleet_id?, item_id?)   # 已在 03；本系统只订阅 VillageDiscovered
Maps.reload_pack(pack_id)                   # 换 World.dat / 贴图 key
```

事件：

```
MapKindOpened { kind, major_sea_id | null }   # 海域图区号；null = 全世界
BlackblockCleared { village_id }            # 发现位翻转后 UI 重画
FogModeChanged { sailing_mode }             # 0/1/3，语义未知
# 不发 EnteredSeaArea — 那是 Ocean 的
```

Ocean 问 `Map.cell` 做碰撞；UI 问 `Maps.kind` 做绘制。Economy 只向 route 相机投 `quote(town_id)`。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| `World.dat` 布局 | `fn_14000ecdc` 读 WORD | `[确认]` 2500×1250 uint16；`formats.md` 需改 |
| 单元格三类的设计名 | 59.8% / 1.7% / 38.5% | `[推断]` 海 / 沿岸 / 陆 |
| 港口 `x1=20080` → chip 下标 | 航行点 trunc `/16`；全幅 40000×20000 吻合 | `[推断]` 同空间 |
| `NUM_VILLAGEBLINDAREA` | 头文件无整数 | `[推断]` 2（循环上界 + 静态表） |
| 村落发现位存档 | `CVillageElement+0x8` u32 | `[确认]`（03） |
| 访问海域 / 走格子 / 古地图是否揭壳 | `SEAAREA_BITS` `OldMap.dk4` | `[未知]` |
| sailing 是否叠黑块 | 无 `eBlindArea` | `[未知]`，默认否 |
| `Route.dk4` | 非 G1T，无加载字符串 | `[未知]` |
| `CSeaAreaScene +0x118` 0/1/2 | 模式分支 | `[未知]` |
| `eTownKind` 两个值 | `fn_14000c71b` = `is_capital ? 1 : 0` | `[确认]` |
| `fn_140012e4f` 雾模式含义 | 返回 0/1/3 | `[未知]`（天气？时刻？） |
| `C3DMapData +0x20/24/28` | 渲染 | 不进玩法 |
| `aszMapPointName` 12 名 | 城镇点 | 不进本表 |
| 市价钉的像素规则 | `vfunc_46` `posOffset` | ui/economy |
| G1T `+0x10` 是否纹理数 | 文件头 | `[推断]` |

---

### 已确认

- 三种投影：航海 `COceanScene`+`CMapView`+`Chip`/`Sky`/`World.dat`；航线 `CRouteMap` 族 + `RouteMap`/`RouteFrm`/`RouteMkr`；黑块 = 村未发现时的 crust，每村 2 槽。
- 共享 `town_id`/`route_id`/`village_id`/原版坐标；相机、资源、雾、点击分离。
- `World.dat` 6250000 字节 = 2500×1250 uint16 贴图编号；chip X 环绕 2500、Y 上限 1250；航行点 trunc `/16` → chip。
- 伦敦坐标是原版单位，不是 2500 下标。
- 航海雾 = `CMapSolidSprite`+Chip 帧 5+四档 RGB；黑块揭开 = `fn_1400100e6` 发现位。
- `CMarketPriceRouteMap` 是经济投影，不是第四种世界图。
- `NUM_MAPPOINTKIND=12` 是港口内点，不是这三种。

### 未知

- 三类贴图的设计名；港口 Locator 与航行坐标是否严格同一空间（强推断）。
- 海域 bits / 道具 / 走访是否揭壳。
- `Route.dk4`、雾模式 0/1/3 的设计名、航线图 `+0x118` 模式。
- sailing 要不要画黑块（原版未见，配置默认 false）。
