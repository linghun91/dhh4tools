# 村落、遗迹与发现

村落和遗迹是世界图上的**可发现点**。world 拥有它们的 ID、坐标、与海域的静态关系。玩家怎么搜、用地图道具、发现后弹什么窗口，分别属于 ocean 和 items。

## 1. 职责边界

管：

- `eVillage` 24 个村落定义（位置、盲区、所属海域）
- `eRemains` 遗迹定义与发现状态位的**数据形状**
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
- `[确认]` 村落**不在** World0.dll 的 `GetFix*` 里。来源：`DllMain.cpp` 五个导出。
- `[确认]` 近港筛选 `fn_14000bcf3`（`srcdata/source/game/unassigned/fn_14014c350.cpp`）：RTC `sVillagePos`、`sTownPos`、`eVillage`、`aeVillageTable`（96 字节 = 24×4）。循环 `eVillage >= 0x18`。哨兵 `0x19`（25 = NUM_VILLAGE+1）。立刻数 `0x9c40 = 40000`、`0x4e20 = 20000` 做 dx/dy 阈值。用 Locator 取村落/港口坐标后比绝对值。
- `[确认]` 海域图同样扫 `eVillage`，带 `eBlindArea`、`sCrustSize`、`rcCrustSeaArea`。来源：`fn_1400018b1`。头文件 `VillageblindareaID.h`：`NUM_VILLAGEBLINDAREA` 整数未恢复。
- `[确认]` 大地图上村落相对玩家舰队：RTC `ptDispVillage` `posVillage` `posPlayerFleet` 等。来源：`catalog/unassigned_named.tsv`（`fn_1400044da` 等）。
- `[确认]` mesfile：「小亚细亚的古地图，把基督教徒藏身的村落位置标着记号。」来源：`mesfile_sc.txt` `0x1bf63`。24 个村落显示名未从 DLL/mesfile 抽出。

遗迹：

- `[确认]` `enum eRemains { NUM_REMAINS, INVALID_REMAINS = -1 }`，头文件注释「integer not recovered」。来源：`RemainsID.h`。`Remains.h` 提到 `REMAINSSTATE_BITS`。`constants.json` 有这两个符号。
- `[推断]` `NUM_REMAINS = 25`。依据：`CRemainsElement` 析构/存档/读档均 `s20 >= 0x19`；`fn_14000a321`（`fn_1402aad20.cpp`）`eRemains >= 0x19`。构造 `fn_140007dce(..., 0x10, 0x19, ...)` 按 0x19 个元素分配。
- `[确认]` `CRemains : CSaveObject`。`CRemainsElement` 在 `CWorld +0xf0d0`。来源：`class_hierarchy.txt`，`CWorld.cpp`，`CRemains.cpp`，`CRemainsElement.cpp`。
- `[确认]` `CMpRemains` 体积 `0x18`，城镇地图点上的遗迹视图。来源：`srcdata/source/game/town/CMpRemains.cpp`。贴图包 `Remains.dk4` 在 `Game/`。
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
| 村落 | `village_id` | 24 | -1；筛选哨兵 25 | Element 仅 `+0x8`，发现位 `[未知]` |
| 遗迹 | `remains_id` | `[推断]` 25 | -1 | `CRemains` + `REMAINSSTATE_BITS` |
| 村落盲区 | `eVillageBlindArea` | `[未知]` | INVALID_VILLAGEBLINDAREA | — |
| 地图道具 | `item_id` | 子集 155–173 | — | items 系统 |

村落名字、遗迹名字：未抽出。配置 `name: null` 直到从 UI/mesfile 补完。

禁止把「去北京的地图」的 item 155–173 直接写成 village 0–18。对应表未还原。

## 4. 算法 / 公式

近港村落候选（`fn_14000bcf3`，参数进 `config/formulas/ocean.yaml` 或 `world` 公式段）：

```
village_table_sentinel = 25          # 0x19
near_town_dx_limit     = 40000       # 0x9c40，原版坐标单位
near_town_dy_limit     = 20000       # 0x4e20

# 伪代码（控制流已还原，字段名是推断）
candidates = []
for village_id in villages:          # 表长，原版 24
    if not eligible(village_id):     # 原版两处谓词未命名
        continue
    candidates.append(village_id)
for v in candidates:
    dx = abs(town.x - village.x)     # Locator
    dy = abs(town.y - village.y)
    # 随后用 dx_limit / dy_limit 做最近比较（反编译分支多，完整谓词 [未知]）
```

搜索圈：

```
search_circle_segments = 64          # CSearchCircleSolidSprite，属 ocean
```

遗迹状态：

```
# REMAINSSTATE_BITS 宽度未从头文件恢复
Remains.state_bits[remains_id]  # 存档
```

发现成功后：ocean 发事件，world 只翻发现位。不要在 world 里读 `eItem`。

## 5. 耦合分析

原版：

- 村落有 Locator，但几乎不走 `CSaveObject`；发现位可能散在 World / 道具 / bitfield，未钉死。
- 海域绘制、近港判断、大地图点击都直接扫 `eVillage`。
- 遗迹 Element 与 231 件道具在同一函数里扫描。
- 搜索把 `eItem` 和舰队坐标绑在 `apfnSearchEvent`。
- 古地图是道具 UI，目标点却是世界坐标。

重构所有权：

| 数据 | 系统 |
| --- | --- |
| 村落/遗迹坐标、ID、静态海域 | world |
| 已发现 bit | world 存档状态（或 remains 子表） |
| 搜索指令、圈半径、成功率 | ocean |
| 地图道具定义与持有 | items |
| 「发现了××」标题、古地图窗 | ui |
| 发现触发剧情 | narrative 订阅 `VillageDiscovered` |

items 通过配置写 `unlocks: { village_id }` 或 `{ remains_id }`，外键校验；没有对应 ID 则提取器留 `unknown`。

## 6. 解耦后的配置

`config/world/villages.yaml`：

```yaml
schema: dk4.world.villages
version: 1
pack: vanilla-world0
sentinel_none: 25
villages:
  - id: 0
    key: village_00
    name: null
    coords:
      space: original
      x: null                 # Locator 表未抽出
      y: null
    sea_area_id: null
    blind_area_id: null
    discovered_by_default: false
    map_item_ids: []          # 反查用，FK -> items；未还原则空
    unknown_bytes: []
# id 1..23
```

`config/world/remains.yaml`：

```yaml
schema: dk4.world.remains
version: 1
pack: vanilla-world0
count_original_inferred: 25   # 循环 0x19，头文件未写死
state_bits: unknown           # REMAINSSTATE_BITS
remains:
  - id: 0
    key: remains_00
    name: null
    coords:
      space: original
      x: null
      y: null
    map_point_kind_id: null   # 与 CMpRemains 的 kind 未对齐
    discovered_by_default: false
    map_item_ids: []
    unknown_bytes: []
```

`config/formulas/ocean.yaml` 片段（搜索阈值，ocean 文档会再引一次）：

```yaml
village_near_town:
  dx_limit: 40000
  dy_limit: 20000
  sentinel: 25
search_circle_segments: 64
```

地图道具仍在 `config/items/items.yaml`，例如：

```yaml
- id: 165
  key: sahara_map
  name: 撒哈拉大沙漠地图
  kind: map
  # 目标村落/遗迹 ID 未还原，禁止填廷巴克图当 village_id
  reveals: { type: unknown }
```

## 7. 独立旋钮

- 村落/遗迹条数（表长）
- 各点坐标、海域、默认已发现
- 近港 dx/dy 阈值
- 搜索圈分段（ocean）
- 哪张地图道具揭示哪个点（items 外键，点不存在则校验失败）

改搜索成功率、测量士技能不改本表。

## 8. 对外接口

```
Village.def(village_id) -> VillageDef
Village.at_map(x, y, radius) -> [village_id]   # 实现用表，不扫 World.dat 实体
Village.discovered(village_id) -> bool
Remains.def(remains_id) -> RemainsDef
Remains.discovered(remains_id) -> bool
```

命令（由 ocean / narrative 调用）：

```
World.mark_village_discovered(village_id)
World.mark_remains_discovered(remains_id)
```

事件：

```
VillageDiscovered { village_id, fleet_id, item_id | null }
RemainsDiscovered { remains_id, fleet_id, item_id | null }
```

world 不读背包。`item_id` 由 ocean 在事件里带上。

## 9. 未核实字段

| 项 | 来源 | 等级 |
| --- | --- | --- |
| 24 个村落名与坐标 | 不在 GetFixTown | `[未知]`，Locator 运行时才有 |
| `CVillage +0x8` 存档字节 | `CVillageElement::vfunc_01` | `[未知]` |
| 发现位存在哪 | 村落非 SaveObject | `[未知]` |
| `NUM_VILLAGEBLINDAREA` | 头文件 | `[未知]` |
| `fn_14000bcf3` 两个 `eligible` 谓词 | 反编译 | `[未知]` |
| dx/dy 分支的完整最近邻规则 | 同函数后半 | `[未知]` |
| `NUM_REMAINS` | 仅循环 0x19 | `[推断]` 25 |
| `REMAINSSTATE_BITS` 宽度与各位 | `Remains.h` | `[未知]` |
| 遗迹名、坐标 | 无抽出表 | `[未知]` |
| item 155–173 ↔ village/remains | mesfile 只给叙事名 | `[未知]` 禁止手填 |
| `CMpRemains` 与 kind 的映射 | 仅类 | `[未知]` |
| `fn_14000a321` 里 `sItemOwner` 类型 0–3 | 道具所有权 | items，不在此命名 |
