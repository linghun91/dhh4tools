# 航线与导航

航线是港口之间已经「建立」的图边。world 存边的定义（`eRoute`、端点、途经点）。航海场景怎么画线、玩家选哪座港、没有航线时说哪句台词，分别是 ocean / ui。市价航线图是经济 UI，只读 `route_id`。

## 1. 职责边界

管：

- `eRoute` 200 条航线定义
- `eJunction` / `eRouteLocator` 途经点（计数未恢复也要占 schema）
- 航线端点对 `town_id` 的外键
- 「是否已建立」作为存档状态的形状

不管：

- 舰队沿航线自动航行的速度（ocean）
- 操舵轮 UI（`CWheelView`）
- 选港界面布局（`CSelectTownRouteMap` 里的像素立刻数）
- 市价叠在航线图上（`CMarketPriceRouteMap` → economy + ui）
- 没有航线时的句子（`config/ui/strings`，本文件只引用 key）
- 海域多边形（`01-map-and-sea-areas.md`）

## 2. 原版事实

计数与头文件：

- `[确认]` `NUM_ROUTE = 200`，`INVALID_ROUTE = -1`。来源：`srcdata/source/RouteID.h`，`num_values.json`。
- `[确认]` `Route.h` 注释：`NUM_ROUTE / NUM_ROUTELOCATOR / SRouteArray`；RTTI 只标了 `CTownRoute`。来源：`srcdata/source/Route.h`。
- `[确认]` `eRouteLocator`：`NUM_ROUTELOCATOR` 未恢复，`INVALID_ROUTELOCATOR = -1`。来源：`RouteLocatorID.h`。
- `[确认]` `eJUNCTION`：`NUM_JUNCTION` 未恢复，`INVALID_JUNCTION = -1`。来源：`JunctionID.h`。`Junction.h` 基类 `CLocator`，注释 `SJunction, NUM_JUNCTION`。
- `[确认]` 航线**不在** World DLL `GetFix*`。来源：`DllMain.cpp`。
- `[确认]` SC 文件偏移 `0x685A70` 起 **200 × 6 字节**：`leave_town`、`arrive_town`、四个途经点。空途经点填 `0xA5`（165）。来源：直接读 `DK4HD_sc.exe`；与大佬笔记（`dh4/8.png`）一致。例：第 0 条 `[0,1,3,129,130,0]`（伦敦→布里斯托尔）；第 1 条 `[0,2,4,5,97,165]` 末字节 `A5`。
- `[确认]` SC 文件偏移 `0x681950` 起折点 **164 × 8 字节**（两个 int32：x, y）。笔记「168」与文件不符，实测 164（编号 00–A3）。第 0 点 `(19352, 4296)`。`0xA5=165 > 164`，所以航线里的 `A5` 不是合法折点下标。
- `[确认]` `CRouteNavigator::vfunc_03` 扫运行时表（`DAT_14429ac50`）。文件里的 200×6 是静态定义。

导航对象：

- `[确认]` `CRouteNavigator` 体积 `0x10`。构造写 `+0x8 = 0xc9`（201 = NUM_ROUTE+1）。来源：`srcdata/source/game/ocean/CRouteNavigator.cpp`。`fields.tsv`：`+0x8`×2、`+0xa`、`+0xb`。
- `[确认]` `CRouteNavigator::vfunc_01`：若 `+0x8 == 0xc9` 则否；若 `+0xa == 7` 则是。`+0xa` 语义 `[未知]`。
- `[确认]` `CRouteNavigator::vfunc_03` RTC `eRoute`，循环 `eRoute >= 0xc8`（200），查表匹配后 `fn_140002d9c(s50, eRoute, …)`。
- `[确认]` `CTownNavigator : CRouteNavigator`，体积 `0x40`。构造：`+0x10 = 0x62`、`+0x11 = 0x62`（98 = NUM_TOWN+1），并构造内部 `CTownRoute`（`fn_1401f35b0` 于 `+0x18`）。来源：`CTownNavigator.cpp`，`class_hierarchy.txt`。
- `[确认]` `CTownRoute` 体积 `0x378`，`+0x8 = 0`。来源：`CTownRoute.cpp`。布局 `[未知]`，体积像路径点缓冲。

海域与航线：

- `[确认]` `aeSeaAreaRoute` 800 字节，循环 `eRoute >= 0xc8`。来源：`fn_14000c9b4`（`fn_14020bbd0.cpp`）。`[推断]` `int32[200]`。
- `[确认]` `CSeaAreaScene` 继承 `CRouteMap`。来源：`class_hierarchy.txt`。
- `[确认]` `CRouteMap` 构造写入一串 UI 立刻数（`+0x80=0x210`、`+0x84=0x19` …）。来源：`CRouteMap.cpp`。属 UI。
- `[确认]` `CSelectTownRouteMap` 继承 `CRouteMap`，`+0x110 = 0x62`（未选港口哨兵 98）。体积 `0x118`。来源：`CSelectTownRouteMap.cpp`。
- `[确认]` `CMarketPriceRouteMap -> CSelectTownRouteMap -> CRouteMap`。来源：`class_hierarchy.txt`。经济读航线，不拥有航线。

资源：

- `[确认]` `Game/Route.dk4` `RouteFrm.dk4` `RouteMap.dk4` `RouteMkr.dk4`。海域绘制函数引用后三个。来源：`fn_1400018b1` 字符串。

文案（选港但没有航线）：

- `[确认]` `mesfile_sc.txt`：
  - `0x5d0f` 含「还没有建立到那座城市的航线。」「还没有建立到达那座城市的航线。」（同块还有「没有测量员。」）
  - `0x5dd3` 「还没有建立通向那座城市的航线。」
  - `0x5df3` 「还没有建立通往那座城市的航线。」
  - `0x5e13` 「还没有建立通往该城市的航线。」「还没有建立到达那座城市的航线。」
  - `0x5e4f` 「与那座城市之间并未建立航线。」「并未建立通往那座城市的航线。」「和那座城市没有航线。」
- 逻辑不得内嵌这些汉字。UI 按 key 取。原版多句是同一条件的变体。

闲聊里的「印度航线」「印度洋航线开通」是历史叙述，不是 `eRoute` 表。来源：`0x6577` `0x65ff`。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 哨兵 |
| --- | --- | --- | --- |
| 航线 | `route_id` | 200 | -1；运行时未选 201（0xc9） |
| 港口（端点） | `town_id` | 97 | 选港 UI 未选 98（0x62） |
| 途经点 | `route_locator_id` | `[未知]` | INVALID_ROUTELOCATOR |
| 结点 | `junction_id` | `[未知]` | INVALID_JUNCTION |

`CTownNavigator +0x10/+0x11` 两个 0x62：`[推断]` 当前/目标港口未设。

航线是否「已建立」是状态。原版存在哪一字节 `[未知]`；重构单独布尔/bitfield，不要把 201 哨兵当「未建立」。

200 条航线的端点表未抽出。配置先按 id 占位，提取器补 `from_town_id` / `to_town_id`。

## 4. 算法 / 公式

建立判定（接口级，原版谓词未还原）：

```
route_none_sentinel = 201     # CRouteNavigator +0x8
town_none_sentinel  = 98      # CTownNavigator / CSelectTownRouteMap
route_flag_byte     = unknown # +0xa，比较值 7

def can_navigate(from_town, to_town) -> bool:
    route = Routes.find(from_town, to_town)   # 表驱动
    if route is None: return False
    return route.established                  # 存档状态
```

找不到边时 UI 发 `RouteMissing { from, to }`，文案 key 指向上述 mesfile 变体，逻辑层只丢事件。

`vfunc_03` 在 200 条里线性搜匹配：重构可用 `(from,to)` 索引，结果仍返回 `route_id`。

`CTownRoute` `0x378` 字节：`[未知]` 是否折线点。schema 预留 `points: [{x,y}]`，原版包可空。

无航速公式。沿航线移动是 ocean。

## 5. 耦合分析

原版：

- 导航器既当「当前航线 id」又当「未选哨兵」。
- `CTownNavigator` 把航线基类和 `CTownRoute` 焊在一起。
- `CSeaAreaScene` 继承航线图，海域 UI 依赖航线绘制。
- 市价图继承选港航线图。
- 航线表在 exe 静态区，加一条航线要改程序。
- 「没有航线」与「没有测量员」挤在同一 mesfile 块。

重构：

- world：边表 + 已建立状态。
- ocean：沿边移动、搜索不到边时发事件。
- ui：`strings` 里 `route.not_established` 多句随机或按语气档。
- economy：`CMarketPriceRouteMap` 改为查询 `Routes.list() + Economy.quote(town_id)`。

## 6. 解耦后的配置

`config/world/routes.yaml`：

```yaml
schema: dk4.world.routes
version: 1
pack: vanilla-world0
none_sentinel: 201
routes:
  - id: 0
    key: route_000
    from_town_id: null        # 未抽出
    to_town_id: null
    bidirectional: unknown
    locators: []              # eRouteLocator
    established_by_default: unknown
    sea_area_ids: []
    unknown_bytes: []
# id 1..199，提取器填端点后校验 town_id
```

`config/world/junctions.yaml`：

```yaml
schema: dk4.world.junctions
version: 1
pack: vanilla-world0
count_original: unknown
junctions:
  - id: 0
    key: junction_000
    coords: { space: original, x: null, y: null }
    unknown_bytes: []
```

`config/ui/strings.zh-CN.yaml` 片段（文案，不进逻辑）：

```yaml
route.not_established:
  - 还没有建立到那座城市的航线。
  - 还没有建立到达那座城市的航线。
  - 还没有建立通向那座城市的航线。
  - 还没有建立通往那座城市的航线。
  - 还没有建立通往该城市的航线。
  - 与那座城市之间并未建立航线。
  - 并未建立通往那座城市的航线。
  - 和那座城市没有航线。
# 来源 mesfile_sc.txt 0x5d0f–0x5e4f
```

资源 key：`Route.dk4` `RouteMap.dk4` `RouteFrm.dk4` `RouteMkr.dk4` 进 ui，不进航线表。

## 7. 独立旋钮

- 航线条数、端点、是否双向
- 默认已建立集合（剧本包可覆盖：开局只有母港邻边）
- 途经点折线
- 未选哨兵值（实现硬顶另写；内容包保持 98/201 以便对照原版存档）
- 无航线文案变体

改市价、改航速、改选港窗口尺寸不必动本表。

## 8. 对外接口

```
Routes.find(from_town_id, to_town_id) -> route_id | invalid
Routes.def(route_id) -> { from, to, locators, established }
Routes.established(route_id) -> bool
Routes.from_town(town_id) -> [route_id]
Junction.def(junction_id) -> { x, y }
```

命令：

```
World.establish_route(route_id)
World.abandon_route(route_id)    # 原版是否能拆 [未知]；接口先留着，原版包可不暴露
```

事件：

```
RouteEstablished { route_id, from_town_id, to_town_id }
RouteMissing { from_town_id, to_town_id }   # UI 播 route.not_established
```

Ocean 在玩家指定目标港之后查询；不要在 world 里弹窗。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| 200 条航线的端点、是否双向 | exe `DAT_14429ac50` | `[未知]`，未抽出 |
| `SRouteArray` 元素步长 | `Route.h` 仅注释 | `[未知]` |
| `NUM_ROUTELOCATOR` | 头文件 | `[未知]` |
| `NUM_JUNCTION` | 头文件 | `[未知]` |
| `CTownRoute` `0x378` 布局 | 析构尺寸 | `[未知]` |
| `CRouteNavigator +0xa` 比较 7 | `vfunc_01` | `[未知]` |
| `+0xb` | fields.tsv | `[未知]` |
| 「已建立」存在存档哪一偏移 | 未对照 save_format | `[未知]` |
| 原版能否拆除航线 | 无符号 | `[未知]` |
| `CRouteMap +0x80…` 立即数 | UI | 不进玩法 |
| mesfile `0x5d0f` 与测量员字符串同块 | 打包 | 不是同一条件 |

`CLocator` 同时是港口、村落、结点的基类；结点坐标抽出前 junction 表允许空，校验器对原版包放行 `count unknown`。
