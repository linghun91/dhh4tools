# 地图与海域

原版把「整张地球」拆成三块：`World.dat` 网格、海域枚举、港口/村落/遗迹等定位点。重构后这一层只拥有**静态地理**：网格、坐标系、海域划分、地图点种类。航行、搜索、风向应用、入港流程不在这里。

## 1. 职责边界

管：

- `World.dat` 的引用、尺寸、单元格类型表
- 原版坐标单位（港口 `x1/y1/x2/y2` 使用的整数空间）
- `eSeaArea` 25 个海域的定义（范围、所属港口/航线的外键）
- `eMapPointKind` 12 种地图点种类的枚举
- 世界包选择（World0 / World1 同一 schema）

不管：

- 舰队位置、航速、操舵（`config/ocean/sailing.yaml`）
- 风向如何改航速（气候表在 `climate.yaml`，应用在 ocean）
- 港口设施、市价、份额（town-life / economy / factions）
- 村落/遗迹的发现状态机（本目录 `03-villages-discovery.md` 定义实体；搜索动作在 ocean）
- 航线是否已建立（`05-routes-navigation.md`）
- `RouteMap.dk4` / `mappoint.dk4` 等贴图（`config/ui/`）
- 黄金航路、霸者之证（ocean / narrative；文案里的「北海/地中海」是霸权区，不是 25 海域）

## 2. 原版事实

- `[确认]` `World.dat` 6250000 字节 = 2500 × 2500 的 `uint8` 网格。来源：`srcdata/docs/formats.md`，`Docs/00-overview/02-original-architecture.md`。
- `[确认]` 主程序用 `CFile` 打开 `"World.dat"`，读入后把缓冲区指针写进地图对象。来源：`srcdata/source/game/unassigned/fn_1401821c0.cpp` 的 `fn_140006d93`（body `0x140183560`，RTC `cFile`，字符串 `World.dat`）。失败时不装入。析构路径 `fn_14000d341` 在 `+0x8 != 0` 时释放该指针。
- `[确认]` `COceanMapData` 对象体积 `0x10`，构造把 `+0x8` 置 0，析构走上述释放。来源：`srcdata/source/game/ocean/COceanMapData.cpp`。
- `[确认]` `COceanScene` 构造调用 `COceanMapData` 构造（`fn_140183490`）写在场景 `+0x88`；场景体积 `0x218`。来源：`srcdata/source/game/ocean/COceanScene.cpp`。
- `[确认]` `CWorld` 体积 `0x1a5e0`，基类链 `CWorld -> CWorldData -> CSaveObject -> CObject -> CWorldElement -> CClimate`，并内嵌 Town/Remains/Village 等 Element。来源：`srcdata/catalog/class_hierarchy.txt`，`srcdata/source/game/world/CWorld.cpp` 构造函数，`srcdata/source/WorldData.h`。
- `[确认]` World0.dll / World1.dll **不**导出海域、地图、航线。五个 `GetFix*` 只有 Town / Person / Item / Fleet / Force。来源：`srcdata/source/world/DllMain.cpp`。海域、`World.dat`、地图点在主 exe。
- `[确认]` `eSeaArea`：`NUM_SEAAREA = 25`，`INVALID_SEAAREA = -1`。来源：`srcdata/source/SeaAreaID.h`，`srcdata/catalog/num_values.json`。
- `[确认]` `NUM_NEXTBASETOWN = 25`，与海域同数。来源：`num_values.json`。`CNextAreaView` 是海域 UI，体积 `0x98`。来源：`srcdata/source/game/ocean/CNextAreaView.cpp`。
- `[确认]` 海域图绘制用 RTC 名 `aeSeaAreaTown`（388 字节 = 97×4）、`aeSeaAreaRoute`（800 字节 = 200×4）、`eVillage`、`eBlindArea`、`eTownKind`、`rcCrustSeaArea`。来源：`srcdata/source/game/unassigned/fn_14020bbd0.cpp` 的 `fn_1400018b1` / `fn_14000c9b4`，资源 `RouteMap.dk4` `RouteMkr.dk4`。
- `[确认]` `CSeaAreaScene` 继承 `CRouteMap -> CView -> … -> CScene`，体积 `0x128`。来源：`class_hierarchy.txt`，`CSeaAreaScene.cpp`。
- `[确认]` `eMapPointKind`：`NUM_MAPPOINTKIND = 12`，`INVALID_MAPPOINTKIND = -1`。来源：`srcdata/source/MapPointID.h`，`num_values.json`。断言见 `catalog/asserts.txt`（含 `MAPPOINTKIND_BITS`、`MAPPOINTKIND_ID`）。
- `[确认]` `CTownSceneView` 构造按 `eMapPointKind` 循环 `>= 0xc` 清 `+0x80` 起 12 个指针。来源：`srcdata/source/game/town/CTownSceneView.cpp`。
- `[确认]` 未分配函数持有 RTC `aszMapPointName`，并加载 `mappoint.dk4`。来源：`catalog/unassigned_named.tsv`（`fn_140002d4c`，`fn_14000a0c4`）。12 个名字字符串本体未抽出。
- `[确认]` 港口坐标不是网格下标：伦敦 `x1=20080` `y1=4272`。全表 `x1` 范围 9248–35520，`y1` 范围 3376–13792。来源：`Docs/_data/towns_world0.json`，`Docs/_meta/source-map.md`。换算到 2500 网格的公式未还原，禁止除 8。
- `[确认]` 网格单元格是 `uint8`。各取值对应陆地/浅海/深海/不可航等语义 **未还原**。
- `[确认]` `C3DMapData` 体积 `0x450`，构造写 `+0x20=0x20`、`+0x24=4`、`+0x28=4`。来源：`C3DMapData.cpp`。这是 3D 地图网格参数，不是 `World.dat` 尺寸。
- `[确认]` `CMapView` 体积 `0xd8`。来源：`CMapView.cpp`。
- `[确认]` mesfile 出现「北海 / 地中海 / 非洲 / 印度洋 / 东南亚 / 东亚 / 加勒比海 / 波罗的海」，用于霸者之证与闲聊，不是 25 个 `eSeaArea` 的名单。来源：`srcdata/catalog/mesfile_sc.txt` `0x1e247` 起。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 非法值 | 内容在哪 |
| --- | --- | --- | --- | --- |
| 世界网格 | 无（单例资源） | 2500×2500 | — | `Game/World.dat` |
| 海域 | `eSeaArea` / `sea_area_id` | 25 | -1 | 主 exe，不在 World DLL |
| 下一根据地港口槽 | `NUM_NEXTBASETOWN` | 25 | — | 与海域同数 `[推断]` 1:1 |
| 地图点种类 | `eMapPointKind` / `map_point_kind_id` | 12 | -1 | 主 exe |
| 港口 | `eTown` | 97 | -1 | World DLL，见 `02-towns.md` |
| 航线 | `eRoute` | 200 | -1 | 主 exe，见 `05-routes-navigation.md` |
| 村落 | `eVillage` | 24 | -1 | 主 exe，见 `03-villages-discovery.md` |

运行时循环读表长，不写死 `NUM_SEAAREA`。原版包默认 25。

海域显示名：原版 25 个名字未从 mesfile / DLL 抽出，配置里先占 `name: null`，校验允许待补。

地图点种类名字：`aszMapPointName` 符号在，字面量未还原。已知相关视图类：`CMapPointView`、`CMpRemains`、`CMpDock`，不能据此给 12 个 kind 强行命名。

## 4. 算法 / 公式

本系统几乎没有玩法公式。仅记录原版立刻数，全部进配置。

```
world_grid.width  = 2500    # formats.md
world_grid.height = 2500
world_grid.cell   = uint8
```

坐标：

- 港口/村落/舰队定位用 **原版坐标单位**（伦敦 20080, 4272）。
- `coord_to_grid`：**未还原**。配置保留 `space: original`，实现不得擅自 `/ 8`。
- `x1/y1` 与 `x2/y2` 差值绝对值 0–56、中位数 24（`towns_world0.json`）。`[推断]` 第二点是同港的另一个地图锚（码头/城区），不是另一套坐标系。

海域归属：

- 原版用 `aeSeaAreaTown[97]`、`aeSeaAreaRoute[200]` 做「当前海域里有哪些港口/航线」的 scratch。
- `[未知]` 港口是否静态绑定一个 `sea_area_id`，还是按坐标现场判定。重构默认：**内容表写静态 `sea_area_id`**，引擎也可再提供 `query_sea_area(x, y)`；两套必须校验一致，不允许代码里写死多边形。

`C3DMapData` 的 `0x20 / 4 / 4` 是渲染细分，进 `config/ui/`，不进世界表。

## 5. 耦合分析

原版硬编码：

- `COceanScene` 直接内嵌 `COceanMapData`，场景析构才放 `World.dat` 缓冲。
- `CSeaAreaScene` 继承航线图 `CRouteMap`，海域 UI 和航线 UI 缠在一起。
- 海域绘制一次扫 `eTown`、`eVillage`、`eRoute`、`eForceColor`、`eTownKind`、`eBlindArea`。
- `NUM_SEAAREA` 与 `NUM_NEXTBASETOWN` 写死为 25。
- 地图点 12 类指针数组写死在 `CTownSceneView`。
- 霸者之证文案用「北海/地中海/…」大区，与 25 海域不是同一枚举。

重构切断：

- world 只提供 `Map.query(x,y) -> Cell`、`SeaArea.id_at(x,y)`、`Town.coords(town_id)`。
- ocean 场景加载网格字节，但不拥有海域定义表。
- UI 贴图 key 进 `config/ui/scenes.yaml`，世界表只留 `icon` 字符串 key。

## 6. 解耦后的配置

`config/world/map.yaml`：

```yaml
schema: dk4.world.map
version: 1
pack: vanilla-world0
grid:
  file: packs/vanilla-world0/World.dat
  width: 2500
  height: 2500
  cell: uint8
  cell_meanings: unknown   # 取值表未还原，提取器原样保留
coordinate:
  space: original          # 禁止当网格下标
  grid_conversion: unknown
  example:
    town_id: 0
    x1: 20080
    y1: 4272
    note: "伦敦；不是 2500 网格下标"
next_base_town_slots: 25   # 原版 NUM_NEXTBASETOWN，运行时读表长
```

`config/world/sea_areas.yaml`：

```yaml
schema: dk4.world.sea_areas
version: 1
pack: vanilla-world0
sea_areas:
  - id: 0
    key: sea_00
    name: null              # 原版名未抽出
    name_i18n: null
    next_base_town_id: null # 与 NUM_NEXTBASETOWN 对齐后填
    town_ids: []            # 静态成员，校验时对照坐标查询
    village_ids: []
    route_ids: []
    unknown_bytes: []
# id 1..24 同样占位。禁止用霸者之证「北海」等名字冒充海域名。
```

`config/world/map_point_kinds.yaml`（可并入 `map.yaml`）：

```yaml
schema: dk4.world.map_point_kinds
version: 1
kinds:
  - id: 0
    key: mappoint_00
    name: null              # aszMapPointName 未还原
  # id 1..11
count_original: 12
invalid: -1
assets:
  mappoint: mappoint.dk4    # UI 包引用，不是玩法
```

`config/ids/` 由内容表生成 `sea_area`、`map_point_kind`。模组从 `id >= 1000` 追加。

## 7. 独立旋钮

改这些值不应迫使贸易/战斗改代码：

- 网格宽高（实现硬顶另写 `engine_limits.yaml`）
- 海域条数（表长）
- 每海域静态港口/航线列表
- `next_base_town_id` 映射
- 地图点种类条数
- 坐标空间标签（若将来还原了 grid 换算，只改 `coordinate.grid_conversion`）

单元格取值 → 通行/补给消耗 的映射一旦还原，进 `config/ocean/sailing.yaml` 的查询表，不进本文件的公式。

## 8. 对外接口

查询（只读，ID + 值）：

```
Map.cell(x_original, y_original) -> { grid_x, grid_y, cell: u8 } | None
  # 换算未知时：实现可拒绝转网格，只返回 original
SeaArea.by_id(sea_area_id) -> SeaAreaDef
SeaArea.at(x_original, y_original) -> sea_area_id | INVALID
SeaArea.towns(sea_area_id) -> [town_id]
SeaArea.routes(sea_area_id) -> [route_id]
MapPointKind.by_id(kind_id) -> KindDef
WorldPack.grid_file() -> path
```

事件：

```
WorldPackLoaded { pack_id, grid_id }
# 不发「进入海域」——那是 Ocean 在位置变化后查询本系统再发
```

Ocean 应发：`EnteredSeaArea { fleet_id, sea_area_id }`。world 不持有舰队。

## 9. 未核实字段

| 项 | 位置 | 现状 |
| --- | --- | --- |
| `World.dat` 每个 uint8 的枚举 | 6250000 字节 | 语义未还原 |
| 原版坐标 → 2500 网格 | `CTown` / `COceanMapData` 未见公式 | `[未知]`，禁止猜 |
| 25 个海域的显示名、边界多边形 | 不在 World DLL | `[未知]` |
| `NUM_NEXTBASETOWN` 与 `eSeaArea` 是否 1:1 | 仅同为 25 | `[推断]` |
| `aeSeaAreaTown` 388 字节布局 | `fn_1400018b1` | 按 97×int 解释是 `[推断]` |
| `eTownKind` 取值 | 海域绘制 RTC | 计数未还原 |
| `eBlindArea` / `NUM_VILLAGEBLINDAREA` | 海域/村落绘制 | 计数未还原 |
| `aszMapPointName` 12 个字符串 | `fn_140002d4c` | 未抽出 |
| `CMpRemains` / `CMpDock` 对应哪个 kind | 仅类名 | `[未知]` |
| `C3DMapData` `+0x20/24/28` | 渲染 | 不进玩法表 |
| `CRouteMap` 构造里一串 `0x210, 0x19, 0x139…` | `CRouteMap.cpp` | UI 布局立即数 |
| 霸者之证大区 vs `eSeaArea` | mesfile | 不是同一 ID 空间 |
