# 村落、遗迹与发现

村落和遗迹是世界图上的**可发现点**。world 拥有它们的 ID、坐标（遗迹继承所挂港口）、发现位。玩家怎么搜、用地图道具、发现后弹什么窗口，分别属于 ocean 和 items。

## 1. 职责边界

管：

- `eVillage` 24 个村落定义（位置、盲区）
- `eRemains` 遗迹的发现状态位**数据形状**（遗迹目录与 `remains.yaml` 本体见 `09-remains-oldmaps-catalog.md`）
- 发现状态哪些字段进存档（与 `CSaveObject` 对齐）

不管：

- 海上搜索圈、测量士、探索指令（`config/ocean/search.yaml`）
- 地图/古地图作为道具（`eItem` 155–173，「去××的地图」；`config/items/`）
- 发现成功 UI（`CDiscoverTitleView`、`CGetOldMapWindow`、`OldMap.dk4`）
- 遗迹里发生的剧情（narrative）
- 港口本身（`02-towns.md`）
- 霸者之证（ocean / narrative）

地图是物品：持有、装备、鉴定走 items。world 只回答「这个发现点在哪、发现过没有」。

## 2. 原版事实

村落：

- `[确认]` `NUM_VILLAGE = 24`，`INVALID_VILLAGE = -1`。来源：`srcdata/source/VillageID.h`，`num_values.json`。
- `[确认]` `CVillage` 基类 `CObject -> CLocator`（**不是** `CSaveObject`）。体积 `0x48`。构造调用 Locator（`fn_140138020` 写在 `+8`）。来源：`class_hierarchy.txt`，`srcdata/source/game/town/CVillage.cpp`。
- `[确认]` `CVillageElement` 挂在 `CWorld` `+0x19f38`，存档函数只写 `+0x8`。来源：`CWorld.cpp` 构造 / `vfunc_07`（`fn_1402a0750`），`CVillageElement.cpp`。
- `[确认]` `CVillageElement+0x8` 就是**发现位**（`u32`，每村一位）：`fn_1400100e6(village)` = `fn_14000f286(elem, id)` = `(elem+8) & (1 << id)` 再 `> 0`；移位在 `fn_1402a0400`（`shl eax, cl`）。`elem` 由 `fn_140002e55` 取 `CWorld+0x19f38`。24 村放得进 32 位。
- `[确认]` 村落**不在** World0.dll 的 `GetFix*` 里，静态表在主 exe `.data`：VA `0x140687b40`（文件偏移 `0x686540`），24 条 × `0x38`：

| 偏移 | 类型 | 去向（`CVillage`，全局数组 VA `0x1442a72e0`，步长 `0x48`） |
| --- | --- | --- |
| +0x00 | `char*` GBK 名 | `+0x10` |
| +0x08 / +0x0c | `i32 x, y`（原版单位） | `+0x18 / +0x1c`（Locator 取位置读这里） |
| +0x10..+0x1c | 盲区 0：`x, y, w, h`（chip 单位） | `×16` 后写 `+0x20..+0x2c` |
| +0x20..+0x2c | 盲区 1：同上 | `×16` 后写 `+0x30..+0x3c` |
| +0x30 / +0x31 | `u8, u8` | `+0x40 / +0x41` |

  装载函数 `fn_1400032c9`（body `0x1402a04d0`）循环 `< 0x18`；数组由静态初始化 `fn_14002ab60` 以 `(0x48, 0x18)` 构造。依赖：`CVillage::vfunc_03` 用 `fn_14000cf90(5, id)` 组 Locator 句柄，**kind 5 = 村落**。24 个名字与坐标已列在 `09-remains-oldmaps-catalog.md`；第 0 条依努维克 `(4880, 2304)`。
- `[确认]` 盲区读取 `fn_140003553(village, eBlindArea, &pt, &size)`：`eBlindArea==0` 读 `+0x20/+0x28`，否则读 `+0x30/+0x38`；`size.w <= 0` 视为无此盲区。World0 有 4 村（14 圣巴巴拉、15 纳尔维克、16 雷巴求、18 北海道）两块都有，其余只有盲区 0。
- `[确认]` 盲区就是航线图上的「黑块」：`fn_1400018b1` 在全世界视图（区号 7）里对每村 `eBlindArea < 2` 循环，**仅当该村未发现**（`fn_1400100e6 == 0`）时画 `sCrustPoint/sCrustSize` 矩形；已发现的村改画村落图标（`RouteMkr.dk4` 帧 `0xd`）。
- `[确认]` 近港村落传闻 `fn_14000bcf3`（body `0x140151250`，`srcdata/source/game/unassigned/fn_14014c350.cpp`）：RTC `sVillagePos`、`sTownPos`、`eVillage`、`aeVillageTable`（96 字节 = 24×4）。完整控制流：
  1. 前提：`fn_1400088f0(town) > 0x1e`，即玩家势力在该港 `CTown+0x18` 份额槽里的值 `> 30`（`fn_1400092af(town, player_force)`），否则直接返回。
  2. 候选：`eVillage` 0..23 中对象存在且 **未发现**（`fn_1400100e6 == 0`）的村，压进 `aeVillageTable`。无候选则返回。
  3. 取最近：`best = 0x19`（25 = NUM_VILLAGE+1，空），`best_dx = 0x9c40`（40000），`best_dy = 0x4e20`（20000）。对每个候选算 `dx=|town.x - v.x|`、`dy=|town.y - v.y|`（不做 X 环绕）。更新条件：`dx<best_dx && dy<best_dy`；或 `dx<best_dx && dy>best_dy && (best_dx-dx) > (dy-best_dy)`；或 `dx>best_dx && dy<best_dy && (dx-best_dx) < (best_dy-dy)`。等价于「曼哈顿距离严格更小，且两轴都不与当前最优相等」。
  4. `best != 25` 时：按调用者对象 `+0x10` 是否为 0 选消息号 `0x4b3` / `0x4b2`，参数是该村名字（`fn_14000bf55`），经 `fn_14000ecbe(town, 0xd)` 取得的港内对象发出。`[推断]` 这是酒馆类传闻「这附近有个村子」。
- `[确认]` 海域图同样扫 `eVillage`，带 `eBlindArea`、`sCrustSize`、`rcCrustSeaArea`。来源：`fn_1400018b1`。头文件 `VillageblindareaID.h`：`NUM_VILLAGEBLINDAREA` 整数未恢复。
- `[确认]` 大地图上村落相对玩家舰队：RTC `ptDispVillage` `posVillage` `posPlayerFleet` 等。来源：`catalog/unassigned_named.tsv`（`fn_1400044da` 等）。
- `[确认]` mesfile：「小亚细亚的古地图，把基督教徒藏身的村落位置标着记号。」来源：`mesfile_sc.txt` `0x1bf63`。24 个村落显示名未从 DLL/mesfile 抽出。

遗迹：

- `[确认]` `enum eRemains { NUM_REMAINS, INVALID_REMAINS = -1 }`，头文件注释「integer not recovered」。来源：`RemainsID.h`。`Remains.h` 提到 `REMAINSSTATE_BITS`。`constants.json` 有这两个符号。
- `[确认]` `REMAINSSTATE_BITS` = `CRemains+0x8`，按**字节**存档（`fn_14000c6da`）；位 0 = 已发现（测 `fn_14000c6f3`，置 `fn_1401ef750(&+8,0,1)`）。位 1–7 `[未知]`。详见 `09-remains-oldmaps-catalog.md` §2.2。
- `[确认]` `NUM_REMAINS = 25`（证据类型：循环上界 + 名字表长度，非 RTC 投票）。依据：名字指针表 VA `0x1405b2f80` 恰 25 个非空指针后接 NULL（巨石阵 … 阿瑜陀耶，复核于 SC exe）；`CRemainsElement` 析构/存档/读档均 `s20 >= 0x19`；`fn_14000a321`（`fn_1402aad20.cpp`）`eRemains >= 0x19`；构造 `fn_140007dce(..., 0x10, 0x19, ...)`。详表见 `09-remains-oldmaps-catalog.md`。
- `[确认]` 港口 DLL 记录 `+0x26` 存该港对应的 `remains_id`（26 = `NUM_REMAINS+1` 为无）；24 个遗迹各挂一港，遗迹 1（木造教堂）无港。见 `02-towns.md`。遗迹**没有独立坐标**，位置 = 所挂港口。
- `[确认]` `CRemains : CSaveObject`。`CRemainsElement` 在 `CWorld +0xf0d0`。来源：`class_hierarchy.txt`，`CWorld.cpp`，`CRemains.cpp`，`CRemainsElement.cpp`。
- `[确认]` `CMpRemains` 体积 `0x18`，港内地图点 kind 9/10/11（遗址 / 教会 / 城门）共用的处理类（跳表 `0x14026d92c`，`10-mappoint-kinds.md` §2.3）。来源：`srcdata/source/game/town/CMpRemains.cpp`。贴图 `Remains.dk4` 帧号 = `remains_id`。**这是原版唯一已证实的遗迹发现入口**：首访置位 0 并加七大海域影响力（`09` §2.6）；海上搜索置遗迹位的代码未见。
- `[确认]` `fn_14000a321` 先扫 `eItem >= 0xe7`（231 件道具），再扫遗迹；遗迹与道具发现有交叉。来源：`fn_1402aad20.cpp`。

发现动作（**不属于 world，列在这里只为划界**）：

- `[确认]` `Search.DK4` 被多处加载；`apfnSearchEvent, cptItem, eItem, posPlayerFleet`。来源：`unassigned_named.tsv` `fn_14001266b`。
- `[确认]` `CSearchCircleSolidSprite` 构造按角度循环 `>= 0x40`（64 段）画搜索圈。来源：`CSearchCircleSolidSprite.cpp`。
- `[确认]` `CDiscoverTitleView` / `CDiscoverTitleMiniMap`：发现标题与小地图。来源：`class_files.tsv`，`class_hierarchy.txt`。
- `[确认]` 古地图窗口：`CGetOldMapWindow` `COldMapButton` `COldMapInfomationView` `COldMapSprite`，资源 `OldMap.dk4`。来源：同上。
- `[确认]` 道具表 155–173 是地图类（泰勒斯的纸地图 … 阿斯帝加王国彩绘地图）。来源：`Docs/_data/items.tsv`。mesfile `0x1bf63` 描述其中若干目标（廷巴克图、德里、吴哥、北京等）。这些是 **item_id**，不是 `village_id` / `remains_id`。

## 3. 实体与 ID

| 实体 | ID | 原版计数 | 非法 / 哨兵 | 存档 |
| --- | --- | --- | --- | --- |
| 村落 | `village_id` | 24 | 运行时 `null`（原版 -1；筛选空 25 只在 legacy） | `CVillageElement+0x8` u32 发现位 |
| 遗迹 | `remains_id` | 25 | 运行时 `null`（原版 -1；港口表空 26 只在 legacy） | `CRemains+0x8` u8，位 0 已发现 |
| 村落盲区 | `eVillageBlindArea` | 每村 2 槽（`[推断]` NUM=2，证据类型：循环上界） | `size.w <= 0` 即空 | 不存档（静态） |
| 地图道具 | `item_id` | 子集 155–173 | — | items 系统 |

村落名字、遗迹名字：已从 exe 抽出，见 `09-remains-oldmaps-catalog.md`。配置里放 `name_key`，文案进 `ui/strings`。

禁止把「去北京的地图」的 item 155–173 直接写成 village 0–18。对应表未还原（声明表见 `11-remains-puzzle-system.md` `config/discovery/old_maps.yaml`）。

## 4. 算法 / 公式

近港村落传闻（`fn_14000bcf3`，参数进 `config/formulas/rumors.yaml` 的 `village_hint` 段；归 town-life，world 只提供查询）：

```
village_hint.min_share     = 30      # 0x1e，严格大于
# 原版「无结果」= 0x19 = NUM_VILLAGE+1，只在 legacy_codec；接口返回 null
village_hint.init_best_dx  = 40000   # 0x9c40 = 世界全宽
village_hint.init_best_dy  = 20000   # 0x4e20 = 世界全高
village_hint.msg_keys      = [0x4b3, 0x4b2]   # 按调用者 +0x10 二选一

def village_hint(town_id, force_id):
    if Share.of(town_id, force_id) <= min_share: return None
    best, bdx, bdy = None, init_best_dx, init_best_dy
    for v in Village.ids():                     # 表长，原版 24
        if Village.discovered(v): continue
        dx = abs(town.x1 - v.x); dy = abs(town.y1 - v.y)   # 不环绕
        if dx == bdx or dy == bdy: continue     # 原版分支的等号漏洞，可做开关
        if dx + dy < bdx + bdy: best, bdx, bdy = v, dx, dy
    return best
```

`dx == bdx or dy == bdy` 跳过是原版比较链的副作用。配置留 `tie_rule: vanilla | manhattan`，默认 `vanilla` 复刻。

盲区（黑块）：

```
blind_area.max_per_village = 2       # eBlindArea 循环上界
blind_area.units           = chip    # 装载时 ×16 转原版单位
blind_area.valid(r)        = r.w > 0
draw_blind(v) = not Village.discovered(v)   # 仅全世界航线图
```

搜索圈：

```
search_circle_segments = 64          # CSearchCircleSolidSprite，属 ocean
```

遗迹状态：

```
Remains.flags[remains_id]       # 存档按 key；原版 CRemains+0x8 u8，bit0 = discovered
```

发现成功后：ocean 发事件，world 只翻发现位。不要在 world 里读 `eItem`。

## 5. 耦合分析

原版：

- 村落有 Locator，不走 `CSaveObject`；发现位集中在 `CVillageElement+0x8` 一个 `u32`，村数被位宽锁死在 ≤32。
- 村落定义（名、坐标、盲区）写死在主 exe `.data`，World0/World1 共用一份，换剧本不能换村。
- 传闻函数同时读份额（factions）、发现位（world）、坐标（world）、港内对象（town-life）。
- 海域绘制、近港判断、大地图点击都直接扫 `eVillage`。
- 遗迹 Element 与 231 件道具在同一函数里扫描。
- 搜索把 `eItem` 和舰队坐标绑在 `apfnSearchEvent`。
- 古地图是道具 UI，目标点却是世界坐标。

重构所有权：

| 数据 | 系统 |
| --- | --- |
| 村落坐标、ID；遗迹 ID（坐标继承港口） | world |
| 已发现 bit | world 存档状态（或 remains 子表） |
| 搜索指令、圈半径、成功率 | ocean |
| 地图道具定义与持有 | items |
| 「发现了××」标题、古地图窗 | ui |
| 发现触发剧情 | narrative 订阅 `VillageDiscovered` / `RemainsDiscovered` |

地图道具 → 发现点的声明只在 `config/discovery/old_maps.yaml`（`11` §6.2）的 `target: { kind, key }`，外键校验；没有对应目标则 `key: null`。

## 6. 解耦后的配置

`config/world/villages.yaml`（本篇是该文件的 schema 所有者；`09` 只给全表数值）：

```yaml
schema: dk4.world.villages
version: 2
pack: vanilla-core             # 原版村表在主 exe .data，World0/World1 共用
villages:
  - key: vanilla:inuvik                # 主键；导入器另写 vanilla_index: 0
    name_key: village.inuvik.name   # 依努维克
    coords: { space: original, x: 4880, y: 2304 }
    blind_areas:               # 原版单位；exe 源是 chip 单位，导出脚本 ×map.units_per_chip（16）；最多 2 块
      - { x: 4064, y: 0, w: 4544, h: 3008 }
    discovered_by_default: false
    unknown: { u8_30: 5, u8_31: 10 }
  - key: vanilla:santa_barbara
    name_key: village.santa_barbara.name
    coords: { space: original, x: 6608, y: 6144 }
    blind_areas:
      - { x: 4064, y: 3008, w: 4544, h: 2192 }
      - { x: 4064, y: 5200, w: 2976, h: 5296 }
    discovered_by_default: false
    unknown: { u8_30: 6, u8_31: 8 }
# 其余 22 条由导出脚本从 exe 表 0x140687b40 生成（.data 按段换算文件偏移 0x686540）。
# 村落表放进内容包（World0/World1 可各自覆盖），不再锁在主程序。
```

`config/world/remains.yaml` 的 schema 与全表见 `09-remains-oldmaps-catalog.md` §6，本篇不重复。

`config/formulas/rumors.yaml` 片段（传闻属 town-life；`search_circle_segments` 仍在 ocean）：

```yaml
village_hint:                            # D3 形状；原版 fn_14000bcf3
  impl: vanilla.village_hint
  int_mode: vanilla_i32
  params:
    min_share_exclusive: 30
    init_best: { dx: 40000, dy: 20000 }   # = 世界全宽/全高，换图跟 map.yaml 走
    tie_rule: vanilla                      # vanilla: 任一轴与当前最优相等即跳过
    msg_keys: [rumor.village_near_a, rumor.village_near_b]   # 原版 0x4b3 / 0x4b2
```

地图道具定义仍在 `config/items/items.yaml`；它揭示哪个点只写在 `config/discovery/old_maps.yaml`（`11` §6.2），例如 165 撒哈拉地图 `target: { kind: remains, key: null }`，禁止填廷巴克图当 `village`。

## 7. 独立旋钮

- 村落/遗迹条数（表长）
- 各点坐标、默认已发现
- 传闻门槛份额、初始最优距离、并列规则
- 每村盲区矩形（换黑块形状不动代码）
- 发现位存储（原版 u32；重构按表长的 bitset，不设 32 上限）
- 搜索圈分段（ocean）
- 哪张地图道具揭示哪个点（`discovery/old_maps.yaml`，点不存在则校验失败）

改搜索成功率、测量士技能不改本表。

## 8. 对外接口

```
Village.def(village_id) -> VillageDef
Village.at_map(x, y, radius) -> [village_id]   # 实现用表，不扫 World.dat 实体
Village.discovered(village_id) -> bool
Village.blind_areas(village_id) -> [{x,y,w,h}]  # 未发现时 UI 画黑块
Village.nearest_undiscovered(x, y, tie_rule) -> village_id | null
Remains.def(remains_id) -> RemainsDef           # 见 09
Remains.discovered(remains_id) -> bool
World.locate(LocatorRef(village, village_id)) -> (x, y)
World.locate(LocatorRef(remains, remains_id)) -> 所挂港口坐标 | null
```

「已发现」位是存档状态，「是否画黑块」是派生量（唯一计算方 world：`not discovered && size>0`），UI 只读不存。

命令（调用方：items（`via: item`）、town-life（委托奖励）；村落半径发现与港内遗址点首访由 world 自己处理，不经命令；ocean 不调。world 置位后自己发下面的事件，调用方不另发）：

```
World.mark_village_discovered(village_id, via, fleet_id?, item_id?)
World.mark_remains_discovered(remains_id, via, town_id?, fleet_id?, item_id?)
# 道具触发：items 发 World.mark_*_discovered(id, via: item, item_id)（旧 items 事件 UnlockRemain / UnlockVillage 作废）
# 港内遗址点首访（via: town_map_point）不经此命令：world 在 on_enter_town_map_point 内处理（11 §4.3）
```

事件（本目录 03 / 09 / 11 统一用这一组形状）：

```
VillageDiscovered { village_id, via: ocean_search|item|event, fleet_id | null, item_id | null }
RemainsDiscovered { remains_id, via: town_map_point|item|event|ocean_search, town_id | null, fleet_id | null, item_id | null }
RemainsRevisited  { remains_id, town_id, kind }   # 已发现遗迹再进 kind 9/10/11；town-life 据此跑仪式
```

world 不读背包。`item_id` 由发起方在命令里带上。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| 24 个村落名与坐标 | exe `.data` `0x140687b40` | `[确认]`，见 §2 / 09 |
| 村落表 `+0x30/+0x31` 两字节 | 值域 0–6 / 1–10 | `[未知]` |
| 发现位 | `CVillageElement+0x8` u32 | `[确认]` |
| 谁置发现位 | 未追 | `[未知]` |
| `NUM_VILLAGEBLINDAREA` | 循环上界 2（`fn_1400018b1` / `fn_140011824`）+ 静态记录恰 2 块 | `[推断]` 2 |
| `fn_14000bcf3` 候选谓词 | 对象非空 + 未发现 | `[确认]` |
| 最近邻规则 | 三段比较链 | `[确认]`（见 §4） |
| 消息号 0x4b2/0x4b3 对应文案 | 未对 mesfile | `[未知]` |
| `NUM_REMAINS` | 指针表 + 循环 | `[确认]` 25 |
| `REMAINSSTATE_BITS` | `CRemains+0x8` 字节存档 | u8、位 0 已发现 `[确认]`；位 1–7 `[未知]` |
| 遗迹名 | `0x1405b2f80` | `[确认]` 25 条 |
| 遗迹坐标 | 挂港 `STown+0x26` | `[确认]` 无独立坐标，继承港口；木造教堂无港 |
| item 155–173 ↔ village/remains | mesfile 只给叙事名 | `[未知]` 禁止手填 |
| `CMpRemains` 与 kind 的映射 | 跳表 `0x14026d92c` | `[确认]` kind 9/10/11 |
| `fn_14000a321` 里 `sItemOwner` 类型 0–3 | 道具所有权 | items，不在此命名 |
