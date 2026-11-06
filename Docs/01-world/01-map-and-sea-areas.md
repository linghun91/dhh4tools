# 地图与海域

原版把「整张地球」拆成三块：`World.dat` 网格、七大海域、港口/村落/遗迹等定位点。重构后这一层只拥有**静态地理**：网格、坐标系、可选的细分海域、地图点种类。七大海域由 factions 注册，本篇只引用。航行、搜索、风向应用、入港流程不在这里。

## 1. 职责边界

管：

- `World.dat` 的引用、尺寸、单元格类型表
- 原版坐标单位（港口 `x1/y1/x2/y2` 使用的整数空间）
- 可选 `sea_areas`（重构扩展的细分海域；原版包为空，见 §2「`eSeaArea` 就是七大海域」）
- `eMapPointKind` 12 种地图点种类的枚举
- 世界包选择（World0 / World1 同一 schema）

不管：

- 舰队位置、航速、操舵（`config/ocean/sailing.yaml`）
- 风向如何改航速（气候表在 `climate.yaml`，应用在 ocean）
- 港口设施、市价、份额（town-life / economy / factions）
- 村落/遗迹的发现状态机（本目录 `03-villages-discovery.md` 定义实体；搜索动作在 ocean）
- 航线是否已建立（`05-routes-navigation.md`）
- `RouteMap.dk4` / `mappoint.dk4` 等贴图（`config/ui/`）
- 七大海域 `major_sea_id` 的注册与港口归属（factions，`02-factions/06-seven-culture-circles.md`）
- 黄金航路、霸者之证（ocean / narrative；文案里的「北海/地中海」就是七大海域）

## 2. 原版事实

- `[确认]` `World.dat` 6250000 字节，是 **2500（宽）× 1250（高）的 `uint16` LE** 网格，不是 `formats.md` 写的 2500² `uint8`。依据：唯一取格函数 `fn_14000ecdc`（body `0x140183740`）反汇编为 `imul eax,[pt+4],0x9c4; add eax,[pt]; movzx eax, word [buf+rax*2]`，即 `buf16[y*2500 + x]`。行走函数 X 在 `0x9c4=2500` 处环绕、Y 合法区间 `[0,0x4e2=1250)`（`fn_140007347` `fn_1400140d3` `fn_14000a9e8`）。按 `uint16` 读：最大值 917，`0/1/16/17` 四值各约 467k 次。`formats.md` / `02-original-architecture.md` 的 uint8 说法需改。
- `[确认]` 原版坐标 → chip 格：`chip = pos / 16`（有符号向零截断，编译成 `(v + ((v>>31)&0xf)) >> 4`）。见 `fn_14000a9e8` `fn_140005c2c` `fn_140014a74`（RTC `ptWorldChip`）。格内偏移 `pos - chip*16` 的 `<5` / `>=0xb` 用来判断是否要查相邻格（`fn_14000a9e8` RTC `ptChipIn` `ptChipNext`）。
- `[推断]` 被除 16 的 `pos` 与港口 `x1/y1` 同一空间。依据：① `40000/16=2500`、`20000/16=1250`，与 X 环绕宽、Y 上限一致；② 村落盲区矩形在 exe 表里以 chip 单位存放，装入 Locator 时 `imul …,0x10`（`fn_1400032c9`，见 `03-villages-discovery.md`）；③ 97 港 `(x1/16, y1/16)` 全部落在非海格，`(x2/16, y2/16)` 有 93 个落在「岸」类格（下条）。
- `[确认]` 格值解码：`col = v & 0xf`、`row = v >> 4`（`fn_14000e354` → `fn_14001214d` 写 `pt.x=col, pt.y=row`）。三类谓词：
  - `fn_14000d2c9`：`row <= 1 && col <= 1` → 真（取值 0/1/16/17）。全图 59.81%。
  - `fn_1400100ff`：`2 <= row <= 12`，或 `row == 13 && (col <= 4 || col >= 13)` → 真。全图 1.67%。
  - `fn_14000e354`：两者都假才返回 1。全图 38.52%。
  `[推断]` 三类分别是开阔海 / 岸线（含格内子形状）/ 陆地；`v` 本身像 `Chip.DK4` 16 列图块的 `(row, col)`。名字未恢复。
- `[确认]` 主程序用 `CFile` 打开 `"World.dat"`，读入后把缓冲区指针写进地图对象。来源：`srcdata/source/game/unassigned/fn_1401821c0.cpp` 的 `fn_140006d93`（body `0x140183560`，RTC `cFile`，字符串 `World.dat`）。失败时不装入。析构路径 `fn_14000d341` 在 `+0x8 != 0` 时释放该指针。
- `[确认]` `COceanMapData` 对象体积 `0x10`，构造把 `+0x8` 置 0，析构走上述释放。来源：`srcdata/source/game/ocean/COceanMapData.cpp`。
- `[确认]` `COceanScene` 构造调用 `COceanMapData` 构造（`fn_140183490`）写在场景 `+0x88`；场景体积 `0x218`。来源：`srcdata/source/game/ocean/COceanScene.cpp`。
- `[确认]` `CWorld` 体积 `0x1a5e0`，基类链 `CWorld -> CWorldData -> CSaveObject -> CObject -> CWorldElement -> CClimate`，并内嵌 Town/Remains/Village 等 Element。来源：`srcdata/catalog/class_hierarchy.txt`，`srcdata/source/game/world/CWorld.cpp` 构造函数，`srcdata/source/WorldData.h`。
- `[确认]` World0.dll / World1.dll **不**导出海域、地图、航线。五个 `GetFix*` 只有 Town / Person / Item / Fleet / Force。来源：`srcdata/source/world/DllMain.cpp`。海域、`World.dat`、地图点在主 exe。
- `[确认]` 断言里有 `NUM_SEAAREA` / `INVALID_SEAAREA` / `SEAAREA_BITS` / `SEAAREA_ID` 符号，**没有整数**（`catalog/asserts.txt`，含 `eVal <= NUM_SEAAREA`）。
- **撤回** 旧稿「`NUM_SEAAREA = 25` `[确认]`」「`NUM_NEXTBASETOWN = 25`」。两数都只来自 `srcdata/tools/fill_gaps.py` 的 RTC 投票（`num_from_rtc.json`：`eSeaArea [[25,7],[7,2]]`、`eNextBaseTown [[25,2],[98,1],…]`），`SeaAreaID.h` 的 `= 25` 是该脚本回填的。投票器从函数头起扫 8 KB，会扫进相邻函数同栈偏移的比较。逐个核对：
  - 带 RTC `eSeaArea` 的只有两个函数：`fn_140008b6b`（body `0x14010a6c0`）循环 `cmp [rsp+0x34],7`；`fn_14000a8c1`（body `0x14010cf80`）`eSeaArea@+0x84` 循环 `cmp …,7`，同函数里比 `0x19` 的是 `eForce@+0x64`（25 势力）。两处都是按七大海域取势力影响力（`fn_140006573` / `fn_1400023ab`）。
  - `eNextBaseTown`（`fn_14000b096` body `0x14010c800`）是**港口变量**：初值 `0x62`、`= eTown`、与 `0x62` 比较。不是 25 槽数组。
  - `[推断]`（证据类型：循环上界）`eSeaArea` 就是七大海域，`NUM_SEAAREA = 7`；海域表 `0x14058bb30` 第 8 条「全世界」解释了断言里的 `eVal <= NUM_SEAAREA`。全仓未见任何 25 档的海域表、海域判定函数或 25 次海域循环。
- `[确认]` `CNextAreaView` 体积 `0x98`（`srcdata/source/game/ocean/CNextAreaView.cpp`），「下一根据地」按港口选，不是 25 槽。
- `[确认]` 海域图绘制用 RTC 名 `aeSeaAreaTown`（388 字节 = 97×4）、`aeSeaAreaRoute`（800 字节 = 200×4）、`eVillage`、`eBlindArea`、`eTownKind`、`rcCrustSeaArea`。来源：`srcdata/source/game/unassigned/fn_14020bbd0.cpp` 的 `fn_1400018b1` / `fn_14000c9b4`，资源 `RouteMap.dk4` `RouteMkr.dk4`。
- `[确认]` **这张「海域图」用的不是 25 个 `eSeaArea`，而是七大海域 `major_sea_id`**。场景 `+0x98` 的区号经 `fn_140211880` 映射：`0 <= a < 7` 返回 `a`，否则返回 `8`（`RouteMap.dk4` 帧号；`a == 7` 走「全世界」分支，倒序画 6..0 七块区框）。`fn_14000c9b4` 在 `a == 8` 时直接返回。港口入表谓词 `fn_140268310`：区号 `== 8` 恒真，否则 `fn_140014e48(town) == a`；`fn_140014e48` = 港口 `culture_id`（STown `+0x25`，`0x16` 视为无）→ `CCultureElement`（`CWorld+0x16f90`，步长 `0xe8`）第 `culture_id` 项 → 该项 `+0x10`（= 静态文化表 VA `0x1405b6160` 21×`0xE0` 的 `+0x08`，开局拷入）。航线入表 `fn_140007ad6(route, a, both)`：`both` 真要求两端港都在该区，假则任一端即可。来源：`fn_14020bbd0.cpp` `fn_140260c10.cpp` `fn_14025ae30.cpp` `fn_1400b93e0.cpp` `fn_1401f0ab0.cpp`。所以 `aeSeaAreaTown/Route` 是七大海域 scratch。七大海域的注册（名字、海域表 `.rdata` VA `0x14058bb30` 8×`0x38`：名、矩形、邻接；第 8 条「全世界」）归 factions（`02-factions/06-seven-culture-circles.md`）；BGM 绑定见 `08-sea-area-music-icons.md`。
- `[确认]` 同函数循环上界：`eVillage >= 0x18`、`eBlindArea >= 2`、`eForceColor >= 8`、`eTownKind >= 2`、`eTown >= 0x61`。`[推断]` `NUM_VILLAGEBLINDAREA = 2`、`NUM_TOWNKIND = 2`（仅循环上界，头文件未写整数）。
- `[确认]` `CSeaAreaScene` 继承 `CRouteMap -> CView -> … -> CScene`，体积 `0x128`。来源：`class_hierarchy.txt`，`CSeaAreaScene.cpp`。
- `[确认]` `eMapPointKind`：`NUM_MAPPOINTKIND = 12`，`INVALID_MAPPOINTKIND = -1`。来源：`srcdata/source/MapPointID.h`，`num_values.json`。断言见 `catalog/asserts.txt`（含 `MAPPOINTKIND_BITS`、`MAPPOINTKIND_ID`）。
- `[确认]` `CTownSceneView` 构造按 `eMapPointKind` 循环 `>= 0xc` 清 `+0x80` 起 12 个指针。来源：`srcdata/source/game/town/CTownSceneView.cpp`。
- `[确认]` 未分配函数持有 RTC `aszMapPointName`，并加载 `mappoint.dk4`。来源：`catalog/unassigned_named.tsv`（`fn_140002d4c`，`fn_14000a0c4`）。12 个字面量（广场…城门）已由 `10-mappoint-kinds.md` 从 `.rdata` `0x14055d5a4` 抽出。
- `[确认]` 港口坐标不是网格下标：伦敦 `x1=20080` `y1=4272`。全表 `x1` 范围 9248–35520，`y1` 范围 3376–13792。来源：`Docs/_data/towns_world0.json`，`Docs/_meta/source-map.md`。伦敦落在 chip `(1255, 267)`。禁止除 8；除数是 16（见上）。
- `[确认]` 世界原版单位全宽 40000、全高 20000：除 16 得 chip，乘 `50/40000`、`25/20000` 得气候格（`04-climate-weather-zodiac.md`）。
- `[确认]` `C3DMapData` 体积 `0x450`，构造写 `+0x20=0x20`、`+0x24=4`、`+0x28=4`。来源：`C3DMapData.cpp`。这是 3D 地图网格参数，不是 `World.dat` 尺寸。
- `[确认]` `CMapView` 体积 `0xd8`。来源：`CMapView.cpp`。
- `[确认]` mesfile 出现「北海 / 地中海 / 非洲 / 印度洋 / 东南亚 / 东亚 / 加勒比海 / 波罗的海」，用于霸者之证与闲聊；前六个与海域表名一致，「加勒比海 / 波罗的海」是地名，不是第 8、9 个海域。来源：`srcdata/catalog/mesfile_sc.txt` `0x1e247` 起。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 非法值 | 内容在哪 |
| --- | --- | --- | --- | --- |
| 世界网格 | 无（单例资源） | 2500×1250 `uint16` | — | `Game/World.dat` |
| 七大海域（海域图用） | `major_sea_id` 0–6；区号 7 = 全世界视图 | 7 | 8（帧号哨兵，仅原版） | factions 注册；本篇只引用 |
| 原版 `eSeaArea` | = 七大海域 | 7 `[推断]`（循环上界） | -1 | 同上；不另立 ID 域 |
| 细分海域（重构扩展） | `sea_area` key | 原版 0 条 | `null` | `config/world/sea_areas.yaml` |
| 地图点种类 | `eMapPointKind` / `map_point_kind_id` | 12 | -1 | 主 exe |
| 港口 | `eTown` | 97 | -1 | World DLL，见 `02-towns.md` |
| 航线 | `eRoute` | 200 | -1 | 主 exe，见 `05-routes-navigation.md` |
| 村落 | `eVillage` | 24 | -1 | 主 exe，见 `03-villages-discovery.md` |

运行时循环读表长。原版没有「25 海域」：旧稿据投票值预留的 25 行占位已删，禁止再凭空建 25 条。

地图点种类名字：12 个字面量已由 `10-mappoint-kinds.md` 抽出（广场…城门），本篇只引用 `map_point_kind_id`。

## 4. 算法 / 公式

本系统几乎没有玩法公式。仅记录原版立刻数，全部进配置。

```
world_grid.width   = 2500   # 0x9c4，X 环绕
world_grid.height  = 1250   # 0x4e2，Y 越界放弃
world_grid.cell    = uint16 # fn_14000ecdc: buf16[y*2500+x]
units_per_chip     = 16     # pos/16，向零截断
world_units_width  = 40000  # = 2500*16
world_units_height = 20000  # = 1250*16
cell.col = v & 0xf ; cell.row = v >> 4
is_open_sea(v) = row <= 1 && col <= 1                         # fn_14000d2c9
is_shore(v)    = 2 <= row <= 12 || (row == 13 && (col <= 4 || col >= 13))   # fn_1400100ff
is_land(v)     = !is_open_sea(v) && !is_shore(v)               # fn_14000e354
```

坐标：

- 港口/村落/舰队定位用 **原版坐标单位**（伦敦 20080, 4272）。
- `coord_to_grid(p) = trunc(p / units_per_chip)`，X 再按 `width` 取模环绕；Y 超出 `[0, height)` 视为不可查。代码事实 `[确认]`；与港口坐标同空间 `[推断]`（见 §2）。禁止 `/ 8`。
- `x1/y1` 与 `x2/y2` 差值绝对值 0–56、中位数 24（`towns_world0.json`）。`[推断]` 第二点是同港的另一个地图锚（码头/城区），不是另一套坐标系。

海域归属：

- 原版 `aeSeaAreaTown[97]`、`aeSeaAreaRoute[200]` 是**七大海域**的 scratch，归属走 `town.culture_id → CCulture[culture_id].+0x10`（静态链，不按坐标判定）。见 §2。重构里「港口属于哪个大海域」是派生量，唯一计算方是 factions（`MajorSea.of_town(town_id)`），world 不存第二份。
- 重构扩展 `sea_areas`（原版包为空）：若模组要比七大海域更细的分区，由内容表给矩形/多边形与静态成员，`SeaArea.at(x, y)` 按表判定；静态成员与坐标判定必须校验一致，不允许代码里写死多边形。

`C3DMapData` 的 `0x20 / 4 / 4` 是渲染细分，进 `config/ui/`，不进世界表。

## 5. 耦合分析

原版硬编码：

- `COceanScene` 直接内嵌 `COceanMapData`，场景析构才放 `World.dat` 缓冲。
- `CSeaAreaScene` 继承航线图 `CRouteMap`，海域 UI 和航线 UI 缠在一起。
- 海域绘制一次扫 `eTown`、`eVillage`、`eRoute`、`eForceColor`、`eTownKind`、`eBlindArea`。
- 「海域图」的区号是七大海域，经 `town.culture_id → CCulture.+0x10` 静态链取得；`eSeaArea` / `CSeaArea*` 名字与七大海域是同一个东西，不要再拆成两套 ID。
- 地图点 12 类指针数组写死在 `CTownSceneView`。

重构切断：

- world 只提供 `Map.cell(x,y)`、`SeaArea.at(x,y)`（扩展）、`Towns.def(town_id)`；大海域归属问 `MajorSea.of_town(town_id)`；位置引用统一用 `LocatorRef(kind, id)`（港 / 村 / 折点 / 舰队 / 道具共用，取坐标走 `World.locate(ref)`）。
- ocean 场景加载网格字节，但不拥有海域定义表。
- UI 贴图 key 进 `config/ui/scenes.yaml`，世界表只留 `icon` 字符串 key。

## 6. 解耦后的配置

`config/world/map.yaml`：

```yaml
schema: dk4.world.map
version: 1
pack: vanilla-core          # World.dat 只有一份，World0/World1 共用
grid:
  file: packs/vanilla-core/World.dat
  width: 2500
  height: 1250
  cell: uint16le
  wrap_x: true
  cell_decode: { col: "v & 0xf", row: "v >> 4" }
  cell_classes:            # 谓词即数据；名字是推断
    open_sea: { row_max: 1, col_max: 1 }
    shore:    { rows: [2, 12], extra: { row: 13, col_le: 4, col_ge: 13 } }
    land:     { otherwise: true }
  sub_cell_probe: { lo: 5, hi: 11 }   # fn_14000a9e8 格内偏移 <5 / >=0xb 查邻格
coordinate:
  space: original          # 禁止当网格下标
  units_per_chip: 16       # pos/16 向零截断；与港口坐标同空间为推断
  world_units: { width: 40000, height: 20000 }
  example:
    town: vanilla:london
    x1: 20080
    y1: 4272
    note: "伦敦；chip=(1255,267)，不是网格下标"
```

`config/world/sea_areas.yaml`（重构扩展；原版包为空表）：

```yaml
schema: dk4.world.sea_areas
version: 1
pack: vanilla-core
sea_areas: []      # 原版没有比七大海域更细的分区
# 模组示例：
# - key: mymod:baltic
#   name_key: mymod:sea_area.baltic.name
#   major_sea: vanilla:north_sea     # 外键到 world/major_seas.yaml（ID 域由 factions 登记）
#   shape: { rect: [21000, 3200, 23000, 4400] }   # 原版单位
#   towns: [vanilla:stockholm, vanilla:riga]       # 静态成员，加载期与 shape 校验
```

七大海域（原版「海域图」的区号、`eSeaArea`）不进 world：它的 ID 域由 factions 登记（表在 `world/major_seas.yaml`），港口归属由 `culture → major_sea` 推出（派生量，唯一计算方 factions）。world 只保证 `Towns.def(town_id).culture_id` 可查，不复制第二份归属。

地图点种类目录只在 `config/world/map_point_kinds.yaml`（见 `10-mappoint-kinds.md`），本篇不再另列。

`config/ids/` 由内容表生成 `sea_area`、`map_point_kind` 域。主键是命名空间 key（`vanilla:plaza`、`mymod:new_zone`），运行时整数由加载器紧凑分配，存档存 `id_map`（`00-overview/05-config-architecture.md`「ID 与命名空间」）。

## 7. 独立旋钮

改这些值不应迫使贸易/战斗改代码：

- 网格宽高（实现硬顶另写 `engine_limits.yaml`）
- 细分海域 `sea_areas`（表长、形状、静态成员；原版空）
- 地图点种类条数
- 坐标空间与 chip 比例（`coordinate.units_per_chip`；换世界图分辨率只改这里与 `grid.*`）
- 格值分类谓词（`grid.cell_classes`；换图块集只改表）

单元格取值 → 通行/补给消耗 的映射一旦还原，进 `config/ocean/sailing.yaml` 的查询表，不进本文件的公式。

## 8. 对外接口

查询（只读，ID + 值）：

```
Map.chip_of(x_original, y_original) -> (chip_x, chip_y)        # trunc(p / units_per_chip)
Map.cell(x_original, y_original) -> { chip_x, chip_y, cell: u16, class: open_sea|shore|land } | None
  # X 环绕，Y 越界返回 None
Map.cell_raw(chip_x, chip_y) -> u16 | None
Map.tile_class(x_original, y_original) -> CellClass   # ocean 的碰撞/靠岸只调这个
World.locate(LocatorRef(kind, id)) -> (x_original, y_original) | None   # kind: town|village|junction|fleet|item|remains
SeaArea.def(sea_area_id) -> SeaAreaDef               # 扩展；原版空表
SeaArea.at(x_original, y_original) -> sea_area_id | null
SeaArea.towns(sea_area_id) -> [town_id]
# 七大海域：MajorSea.of_town(town_id) / MajorSea.def(major_sea_id)，归 factions，不在 world
MapPointKind.by_id(kind_id) -> KindDef
WorldPack.grid_file() -> path
```

事件：

```
WorldPackLoaded { pack_id, grid_id }
# 不发「进入海域」——那是 Ocean 在位置变化后查询本系统再发
```

Ocean 应发：`EnteredMajorSea { fleet_id, major_sea_id }`（原版 BGM 就按它选曲，见 08），有 `sea_areas` 时另发 `EnteredSeaArea { fleet_id, sea_area_id }`。world 不持有舰队。

## 9. 未核实字段

| 项 | 位置 | 现状 |
| --- | --- | --- |
| `World.dat` uint16 的三类以外细义（图块 row/col 与 Chip.DK4 对应） | 6250000 字节 | 三类谓词 `[确认]`，类名 `[推断]` |
| 原版坐标 → chip 的 `pos` 是否就是港口 `x1/y1` | `fn_14000a9e8` 等 | `[推断]`（三条旁证，见 §2） |
| `NUM_SEAAREA` 整数 | 断言无整数；真实循环 `< 7` | `[推断]` 7；旧值 25 是 RTC 投票伪影，撤回 |
| `NUM_NEXTBASETOWN` | `eNextBaseTown` 是港口变量 | 25 撤回；无此计数 |
| `SEAAREA_BITS` | 断言符号 | 七大海域位集 `[推断]`；读写方未找到 |
| `aeSeaAreaTown` 388 字节布局 | `fn_1400018b1` | 97×int 压紧列表 `[确认]`（`*(arr + n*4) = eTown; n++`）；用于 7 区 |
| `eTownKind` 取值 | `fn_14000c71b` = `STown+0x24 ? 1 : 0` | 语义 `[确认]` 0 非首都 / 1 首都；NUM=2 `[推断]`（循环上界） |
| `eBlindArea` / `NUM_VILLAGEBLINDAREA` | 海域/村落绘制 | 循环上界 2，`[推断]` NUM=2；每村两块盲区，见 03 |
| `aszMapPointName` 12 个字符串 | `fn_140002d4c` | 已抽出，见 `10-mappoint-kinds.md` |
| `CMpRemains` / `CMpDock` 对应哪个 kind | 跳表 `0x14026d92c` | kind 9/10/11 → `CMpRemains` `[确认]`；kind 3 → `CPort`，`CMpDock` 是其子对象 `[推断]`（`10-mappoint-kinds.md`） |
| `C3DMapData` `+0x20/24/28` | 渲染 | 不进玩法表 |
| `CRouteMap` 构造里一串 `0x210, 0x19, 0x139…` | `CRouteMap.cpp` | UI 布局立即数 |
| 七大海域 vs `eSeaArea` | RTC 循环 `< 7`、海域表 `0x14058bb30` | 同一 ID 空间 `[推断]` |
