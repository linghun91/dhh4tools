# 航线与导航

航线是港口之间的双向图边。world 存边的定义（`eRoute`、端点、折点）与可见/可走谓词的选择；「已建立」在原版不是存档位，而是由港口已知位、势力份额与多跳寻路派生。航海场景怎么画线、玩家选哪座港、没有航线时说哪句台词，分别是 ocean / ui。市价航线图是经济 UI，只读 `route_id`。

## 1. 职责边界

管：

- `eRoute` 200 条航线定义
- `eJunction` 164 个折点；每线 6 槽 `[from, j1..j4, to]`
- 航线端点对 `town_id` 的外键
- 边权、可见、可走谓词（具名，可替换）
- 多跳寻路的定义（算法属本系统查询，移动执行属 ocean）

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
- `[确认]` `eJUNCTION`：头文件未写整数，`INVALID_JUNCTION = -1`。来源：`JunctionID.h`。`Junction.h` 基类 `CLocator`，注释 `SJunction, NUM_JUNCTION`。
- `[确认]` `NUM_JUNCTION = 164`：装载函数 `fn_140012f08`（body `0x140138280`）循环 `< 0xa4`，把 `.data` VA `0x140682f50` 的 `(i32 x, i32 y)` 拷到运行时 `SJunction` 数组 VA `0x144296c60`（步长 `0x10`，`+0x8 x`、`+0xc y`）；Locator 构造 `fn_14000cf90` 对 kind 4 也检查 `< 0xa4`。
- `[确认]` 航线**不在** World DLL `GetFix*`。来源：`DllMain.cpp`。
- `[确认]` 静态航线表 `.data` VA `0x140687070`（文件偏移 `0x685A70`），**200 × 6 字节**：`[from_town, to_town, j1, j2, j3, j4]`，`j*` 是折点（junction）下标，空填 `0xA5`（165）。与大佬笔记（`dh4/8.png`）一致。例：第 0 条 `[0,1,3,129,130,0]`（伦敦→布里斯托尔，经折点 3、129、130、0）；第 1 条 `[0,2,4,5,97,165]`。
- `[确认]` 装载 `fn_14000b230`（body `0x1401f53f0`）循环 `< 0xc8`，生成运行时 `SRoute`（VA `0x14429ac50`，步长 `0x1c`）：`+0x4` 槽 0 = `Locator(0, from)`，`+0x8..+0x14` 槽 1–4 = `Locator(4, j1..j4)`，`+0x18` 槽 5 = `Locator(0, to)`。**终点放在槽 5**，不是第 2 字节位置。
- `[确认]` Locator 句柄 = `kind << 24 | index`（`fn_14000cf90`，body `0x14013fb30`；取下标 `fn_1401f4980` 做 `& 0xffffff`）。按 kind 查范围，越界返回 `-1`：kind 0 港 `< 0x61`、kind 1 `< 0x45`、kind 2 `< 0xe7`、kind 3 `< 0x14`、kind 4 折点 `< 0xa4`；kind 5（村落）不查。所以 `0xA5` 折点在运行时变成 `-1`，被逐槽前进函数 `fn_140009ddb` 跳过。
- `[确认]` `SRoute+0x0` 是 `u32`，`fn_140007694` 直接返回它，被寻路当边权（见下）。`fn_14000b230` 不写它，谁算出来 `[未知]`；`[推断]` 是折线总长。
- `[确认]` 静态表统计（World0/1 共用，exe 内）：无自环、无重复无序港对；97 港都至少有一条边；164 个折点全部被引用；折点数分布 0:34、1:51、2:63、3:43、4:9；无「空槽后又有折点」的洞。折点坐标范围 x 10040–35464、y 3560–13912（原版单位）。
- `[确认]` 「另一端」`fn_14000dfdf(route, town)`：`town == from` 返回 `to`，`town == to` 返回 `from`，否则 `0x62`。**航线双向**。

导航对象：

- `[确认]` `CRouteNavigator` 体积 `0x10`。构造写 `+0x8 = 0xc9`（201 = NUM_ROUTE+1）。来源：`srcdata/source/game/ocean/CRouteNavigator.cpp`。`fields.tsv`：`+0x8`×2、`+0xa`、`+0xb`。
- `[确认]` `CRouteNavigator::vfunc_03(from, to)`：按 id 线性扫 200 条，取**第一条** `other_end(route, from) == to` 的边，调 `fn_140002d9c(this, route, from)`。
- `[确认]` `fn_140002d9c`（body `0x1401f5cb0`）：`from == route.from` 时 `+0x8 = (u8)route`、`+0x9 = 0`（当前槽）、`+0xb = 5`（终点槽）、`+0xa = next_slot(route, 0 → 5)`；`from == route.to` 时 `+0x9 = 5`、`+0xb = 0`、`+0xa = next_slot(route, 5 → 0)`。`next_slot`（`fn_140009ddb`）朝终点逐槽走，跳过值为 `-1` 的槽。
- `[确认]` `CRouteNavigator::vfunc_02`：`+0xb == 0` 返回 `route.from`，否则 `route.to`，即当前目的港。
- `[确认]` `CRouteNavigator::vfunc_01`：`+0x8 != 0xc9` 且 `+0xa == 7` 时为真。`[未知]` 谁把 `+0xa` 写成 7（槽号只到 5）；`[推断]` 是「已到达」标记。
- `[确认]` 航线 id 存在 **u8**（`+0x8`），哨兵 201 —— 原版航线数上限被字节宽度锁在 ≤ 254。
- `[确认]` `CTownNavigator : CRouteNavigator`，体积 `0x40`。构造：`+0x10 = 0x62`、`+0x11 = 0x62`（98 = NUM_TOWN+1），并构造内部 `CTownRoute`（`fn_1401f35b0` 于 `+0x18`）。来源：`CTownNavigator.cpp`，`class_hierarchy.txt`。
- `[确认]` `CTownRoute` 体积 `0x378`，`+0x8 = 0`。来源：`CTownRoute.cpp`。布局 `[未知]`，体积像路径点缓冲。

多跳寻路：

- `[确认]` `fn_1401f3aa0`（body `0x1401f3aa0`，RTC `asConnect` 1552 字节 = 97 × 16、`eRoute`、`eTown`）是港口图上的最短路：
  1. 每港初始化 `{route = 0xc9, prev = 0x62, hops = 上限, cost = 0xFFFFFFFF}`；起点 `hops = 0, cost = 0`。
  2. 按层 `k = 0, 1, …` 松弛（本轮无更新或 `k` 达上限即停）：对每个 `hops == k` 的港 `t`、每条航线 `r`，若力量过滤参数 `!= 0x1a` 则要求 `fn_14000a4fc(r, force)`（两端港该势力份额都 `> 0`）；`u = other_end(r, t)`，`c = cost[t] + SRoute[r].+0x0`，`c < cost[u]`（**无符号**）时改写 `u` 的 `{r, t, k+1, c}`。
  3. 终点 `hops` 不超上限时按 `prev` 回溯，把航线 id 逐个写成 `u8` 序列。
  调用方 `fn_140003742`（body `0x1401f4190`，力量参数取自虚函数 `+0x18`）与 `fn_140001a46`（RTC `aidRoute`）。
- `[推断]` 这就是 `CTownNavigator` / `CTownRoute`（`0x378` 字节）的路径来源：导航是**多跳**的，不是只认直连边。
- `[确认]` 航线在航线图上显示的条件 `fn_14001389f(route)`：两端港 `CTown::vfunc_04` 都为真（`CTown+0x10` bit0，见 `02-towns.md`）。只被 `fn_14000c9b4` 调用。
- `[确认]` `fn_14000cc43(route)`：两端港玩家势力份额都 `> 0`。`fn_14000c9b4` 在参数 `== 0x1a` 时用它挑另一批要画的边。
- `[未知]` 「还没有建立航线」文案由哪个谓词触发：候选是寻路失败（含份额过滤）或端点未知位。全仓未找到独立的「航线已建立」存档位。

海域与航线：

- `[确认]` `aeSeaAreaRoute` 800 字节 = `int32[200]` 压紧列表，循环 `eRoute >= 0xc8`。来源：`fn_14000c9b4`（`fn_14020bbd0.cpp`）。区号是**七大海域** `major_sea_id`（`== 8` 直接返回），归属谓词 `fn_140007ad6(route, area, both)` 按两端港的 `culture → major_sea` 判，见 `01-map-and-sea-areas.md`。
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
| 航线 | `route_id` | 200 | 运行时 `null`（原版 -1；导航器未选 201 = `0xc9`） |
| 港口（端点） | `town_id` | 97 | 运行时 `null`（原版选港 UI 未选 98 = `0x62`） |
| 途经点槽 | `via` 列表 | 原版每线 6 槽（两端 + 4 折点） | 列表短即可（原版空槽静态 `0xA5`、运行时 -1） |
| 折点 | `junction_id` | 164 | 运行时 `null`（原版静态 165 = `0xA5`） |
| 位置引用 | `LocatorRef(kind, id)` | 原版句柄 `kind<<24 \| index`：kind 0 港 / 4 折点 / 5 村 | `null` |

原版哨兵 98 / 201 / 165 / 26 只在 `config/ids/legacy_codec.yaml` 出现（`00-overview/04-id-registry.md`），导出原版存档时越界拒绝。

`CTownNavigator +0x10/+0x11` 两个 0x62：`[推断]` 当前/目标港口未设。

航线「可见 / 可走」不是存档位，是派生量：端点已知位 + 势力份额过滤 + 寻路。唯一计算方是 world 的 `Routes.visible` / `Routes.path`（谓词可在 formulas 里换），ui / ocean / economy 只读结果，不存档。不要把 201 哨兵当「未建立」。

200 条航线端点、164 个折点已从 exe 抽出（§2）。

## 4. 算法 / 公式

原版常量（代码不写死）。哨兵进 `legacy_codec`，其余进 `config/formulas/routes.yaml`：

```
# legacy_codec（导入导出原版存档 / exe 表用）
route.empty            = 201          # 0xc9，CRouteNavigator +0x8（u8）
town.empty             = 98           # 0x62
force.empty            = 26           # 0x1a；寻路「不过滤」参数，重构传 null
junction.empty_static  = 165          # 0xA5 -> 运行时 Locator -1
# 原版布局（导出脚本用，引擎不用）
route_slots            = 6            # [from, j1..j4, to]
arrived_marker         = 7            # +0xa，来源未知
```

直连查找（`vfunc_03`，取 id 最小的匹配边）：

```
def find_direct(a, b):
    for r in Routes.ids():                    # 原版 0..199 线性
        if other_end(r, a) == b: return r      # 双向
    return None
```

多跳寻路（`fn_1401f3aa0`）：

```
def path(a, b, force=None, max_hops=H):
    init cost[*]=INF, hops[*]=H, prev[*]=None; cost[a]=0; hops[a]=0
    for k in 0..H-1:
        changed = False
        for t with hops[t] == k:
            for r in Routes.ids():
                if force is not None and not edge_allowed(r, force): continue
                u = other_end(r, t);  if u is None: continue
                c = cost[t] + Routes.length(r)
                if c < cost[u]: cost[u], hops[u], prev[u] = c, k+1, (r, t); changed = True
        if not changed: break
    return backtrack(prev, b) if hops[b] <= H else None

edge_allowed_vanilla(r, f) = Share.of(from(r), f) > 0 and Share.of(to(r), f) > 0
route_visible_vanilla(r, f) = Towns.is_known(from(r), f) and Towns.is_known(to(r), f)
```

`H` 的原值与 `Routes.length` 的来源 `[未知]`；实现先用折线长度，配置可换。找不到路时 ocean 发 `RouteMissing { from, to }`，文案 key 指向下方 mesfile 变体。

无航速公式。沿航线移动是 ocean。

## 5. 耦合分析

原版：

- 导航器既当「当前航线 id」又当「未选哨兵」；航线 id 存 `u8`，表长被字节宽度锁死。
- `CTownNavigator` 把航线基类和 `CTownRoute` 焊在一起。
- `CSeaAreaScene` 继承航线图，海域 UI 依赖航线绘制；区号是七大海域。
- 寻路的边过滤直接读港口份额槽（factions 状态），显示过滤读港口已知位。
- 市价图继承选港航线图。
- 航线表、折点表在 exe 静态区，World0/World1 共用；加一条航线要改程序。
- Locator 句柄把 kind 塞进高 8 位，index 只有 24 位，kind 上限 256。
- 「没有航线」与「没有测量员」挤在同一 mesfile 块。

重构：

- world：边表 + 折点表 + 只读查询；边权、可见、可走谓词做成具名可替换函数。
- factions：提供 `share(town, force)` 查询，不让 world 读份额内部结构。
- ocean：多跳寻路结果、沿边移动、找不到路时发事件。
- ui：`strings` 里 `route.not_established` 多句随机或按语气档。
- economy：`CMarketPriceRouteMap` 改为查询 `Routes.list() + Economy.quote(town_id)`。

## 6. 解耦后的配置

`config/world/junctions.yaml`（exe `.data` `0x140682f50` 提取）：

```yaml
schema: dk4.world.junctions
version: 3
pack: vanilla-core             # exe 表，World0/World1 共用
junctions:
  - { key: vanilla:j000, x: 19352, y: 4296 }   # 原版 0
  - { key: vanilla:j003, x: 20216, y: 4280 }   # 原版 3
  # ... 共 164 条，原版单位；导出脚本按 .data 段换算偏移（VA−0x140001600）生成
```

`config/world/routes.yaml`（exe `.data` `0x140687070` 提取）：

```yaml
schema: dk4.world.routes
version: 3
pack: vanilla-core
routes:
  - key: vanilla:london__bristol         # 主键；原版 0
    from_town: vanilla:london
    to_town: vanilla:bristol
    via: [vanilla:j003, vanilla:j129, vanilla:j130, vanilla:j000]   # 0..4 个；原版 0xA5 不写
    length: null                     # null = 按折线自动算；原版来源未知
  - key: vanilla:london__amsterdam       # 原版 1
    from_town: vanilla:london
    to_town: vanilla:amsterdam
    via: [vanilla:j004, vanilla:j005, vanilla:j097]
# 原版 200 条；全部双向（other_end 对称），不设 bidirectional 字段
```

`config/formulas/routes.yaml`：

```yaml
schema: dk4.formulas.routes
version: 2
# D3 形状（14-config-schemas README「公式层」）；谓词 / 算法选择写成 impl
route_visible:
  impl: vanilla.both_endpoints_known     # 原版 fn_14001389f
edge_allowed:
  impl: vanilla.both_endpoints_share_gt  # 原版 fn_14000a4fc
  params: { min_share_exclusive: 0 }
edge_weight:
  impl: vanilla.route_length             # SRoute+0x0
path_search:
  impl: vanilla.layered_relax
  params: { max_hops: null, tie: lowest_route_id }
direct_lookup:
  impl: vanilla.first_route              # vfunc_03 取第一条
  params: { tie: lowest_route_id }
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

- 航线条数、端点、折点（表长；id 不受 u8 限制）
- 折点坐标、每线折点数（原版 ≤4，重构不设上限）
- 边权来源（折线长度 / 显式 `length`）
- 可见 / 可走谓词与其参数（份额门槛、是否要求端点已知）
- 寻路层数上限、并列规则
- 原版哨兵 98/201/165 只在 `legacy_codec`，内容包与引擎都不出现
- 无航线文案变体

改市价、改航速、改选港窗口尺寸不必动本表。

## 8. 对外接口

```
Routes.def(route_id) -> { from, to, via: [junction_id], length }
Routes.find_direct(a, b) -> route_id | null          # 双向，按 tie 规则
Routes.other_end(route_id, town_id) -> town_id | null
Routes.from_town(town_id) -> [route_id]
Routes.visible(route_id, viewer_force) -> bool       # 派生量，谓词由 formulas 选
Routes.path(a, b, force | null) -> [route_id] | null # 多跳；force=null 即原版 0x1a 不过滤
Junction.def(junction_id) -> { x, y }
World.locate(LocatorRef(junction, junction_id)) -> (x, y)
```

命令：无。原版没有「建立 / 拆除航线」的存档位；可用性由港口已知位与份额派生。剧本若要显式解锁，改港口已知位（`Towns.mark_known(town_id, force_id)`）或在 formulas 换谓词。

事件：

```
RouteMissing { from_town_id, to_town_id, force_id | null }   # ocean 发，UI 播 route.not_established
```

Ocean 在玩家指定目标港之后查询；不要在 world 里弹窗。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| 200 条端点与折点 | `.data 0x140687070` | `[确认]`，已抽出 |
| `SRoute` 步长 / 槽布局 | `fn_14000b230` | `[确认]` `0x1c`，`+0x4..+0x18` 六槽 |
| `SRoute+0x0` 边权由谁写 | 只见读 | `[未知]`，推断折线长 |
| `NUM_ROUTELOCATOR` | 头文件 | `[未知]`；每线 6 槽 `[确认]` |
| `NUM_JUNCTION` | 装载循环 | `[确认]` 164 |
| Locator kind 1 / 2 / 3 的对象 | 上界 0x45 / 0xe7 / 0x14 | `[推断]` 1 舰队（69 = NUM_FLEET）/ 2 道具（231 = NUM_ITEM）；3 `[未知]` |
| `CTownRoute` `0x378` 布局 | 析构尺寸 | `[未知]`；推断存寻路结果 |
| 寻路层数上限原值 | `fn_1401f3aa0` 参数 | `[未知]` |
| `CRouteNavigator +0xa == 7` | `vfunc_01` | `[未知]` |
| 「还没有建立航线」触发谓词 | 未追到调用点 | `[未知]` |
| `CRouteMap +0x80…` 立即数 | UI | 不进玩法 |
| mesfile `0x5d0f` 与测量员字符串同块 | 打包 | 不是同一条件 |

`CLocator` 同时是港口、村落、折点的基类；三者在原版共用一个 24 位 index + 8 位 kind 的句柄空间。重构用统一的 `LocatorRef(kind, id)`（kind: town / village / junction / fleet / item / remains），不压位；原版 kind 号只在 legacy 导入时映射。
