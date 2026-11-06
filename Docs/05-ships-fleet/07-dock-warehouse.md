# 船坞仓储

海上编队只有 5 个 `eFleetShip` 槽。多出来的船体不跟舰队走，停在某座港的船坞里：买来的、俘来的、解散委任剩下的，都先入坞，再用换船 UI 跟海上 5 艘对调。

本篇管**坞仓**：玩家持有但未编入当前舰队的船体、停泊港口、换入换出、俘拖入坞。船体数值见 `01-ship-model.md`，舰队 5 槽与委任见 `02-fleet-proxy.md`，舱室解任见 `03-cabins-crew-posts.md`，造船厂修理/改造见 `04-remodel-guns-supplies.md`。设施入口只发命令，见 `08-town-life/01-facilities.md`。

原版把 `NUM_SHIP=100` 同时当成船体**目录**长和玩家坞槽**下标上界**。重构必须拆开：目录走 `ship_id`，坞槽走 `dock_slots`。

## 1. 职责边界

本系统管：

- 玩家「持有但未出海」的船体（坞仓）
- 每艘坞内船停在哪座 `town_id`
- 入坞 / 出坞 / 与舰队 5 槽对调
- 持有上限（原版与 `NUM_SHIP` 同数）把关：买船、复制改造船、俘船入坞
- 拖航俘船在入港时落入哪座港的坞

本系统不管：

| 事项 | 归属 |
| --- | --- |
| 船体目录、耐久、水手、货格布局 | `01-ship-model.md` |
| 舰队 5 槽、委任、阵型 | `02-fleet-proxy.md` |
| 入坞时舱室人物解任 | `03-cabins-crew-posts.md`（本系统发事件） |
| 买船价、修理/改造天数 | `04-remodel-guns-supplies.md` / economy |
| 海战谁被俘、战利品金币 | `07-combat` |
| 造船所/码头按钮是否出现 | town-life 设施表 |
| 文案字符串 | ui / localization |

货格**属于船**。坞内船仍是完整 `CShip`，货还在船上。本系统只在入坞/卖掉时问「还有剩余货物吗」，不另建一座「港口仓库」货表。

## 2. 原版事实

### 2.1 两套上限：海上 5，持有 100

- `[确认]` 「一只舰队最多可以有五艘船，新船放在船坞里。」来源：`mesfile_sc.txt` `0x7df7`。买船函数 `fn_1400068c0` 在舰队船数 `>= 5` 时播立即数 `0x3a1`（与这句同序 `[推断]`）。
- `[确认]` 「持有的船过多了！这个城市没有可以制造的船！」来源：`0x2e4f`。`fn_1400068c0` / `fn_140007531` 在空坞槽分配失败时 `fn_14000fb14(0xfe)`，目录为空时 `0xff`。
- `[确认]` 「请给船取个名字！不能再把船处置掉了！还有剩余货物，这样可以吗？」来源：`0x2e93`。三句打在同一 mes 块；卖船/弃船的调用点未在已还原 cpp 里对上立即数。
- `[确认]` 「购买与已改造好的船完全一样的船？好了，那就先存放在船坞吧。」来源：`0x7ea3`。`fn_140007531` 开头 `0x3a2`，同样先跑空槽分配 `fn_1400018e8`。
- `[确认]` 玩家舰队船位 `NUM_FLEETSHIP=5`。来源：`FleetShipID.h`、`02-fleet-proxy.md`。
- `[确认]` `eShip` / `NUM_SHIP=100`。来源：`ShipID.h`、`catalog/num_values.json`。`01-ship-model.md` 把它当船体**目录**；坞仓扫描、换船 UI、空槽分配的循环上界也是 `0x64`。

买船并不先找舰队空位。`fn_1400068c0` 顺序：`fn_1400018e8` 取空坞槽 → 失败即「持有的船过多」→ 再看本港能否造 → 舰队已满才提示「新船放在船坞里」→ 报价 `hull_price<<1` → 钱不够 `0x100` → 确认 `0x101`。`[确认]` 新购实例落在坞槽下标上，不直接写 `eFleetShip`。

### 2.2 207 实例池切成几段

`[确认]` 运行时 `CShipElement` 207 个 `CShip`（`0xcf`），空实例哨兵 `0xD0`。来源：`CShipElement.cpp`、`01-ship-model.md`。

三段循环把 0–206 切开：

| 下标 | 循环 | 来源 | 用途 |
| --- | --- | --- | --- |
| 0–7 | 分配器**跳过** | `fn_1400018e8` 从 `eShip=8` 起 | `[未知]` 预留（剧本船 / `NUM_PLAYEREVENTSHIP` / 开场） |
| 8–99 | `eShip >= 0x64` 结束 | `fn_1400018e8` | `[确认]` 买船/复制船空槽；失败返回 0 |
| 0–99 | 同上 `0x64` | `fn_14011b5e0` `fn_140004854` `fn_140177030` 第三段 | `[确认]` 按停泊港收集坞内船 |
| 100–193 | `eShip=0x64` 到 `>= 0xC2` | `fn_14000a984` | `[未知]` 与坞仓分开的一段；World 舰队引用常落在这带 |
| 194–206 | 模板值 `0xC2` 起 | World0 舰队 0–6 的船字节 `C2 C3 C4…` | `[推断]` 开场玩家旗舰不在 0–99 |

`[确认]` 工作数组 `aeShipTable` / `aeMyShip` / `aeBuyShip` / `aeDockShip`（造船所一侧）都是 **828 字节 = 207×4**，先填 `0xD0` 再往里塞实例 id。来源：`rtc.json` `0x140176a20` 等、`fn_140177030`。

`[确认]` 入港拖船一侧 `aeDockShip` 与 `aeInTowShip` 各 **400 字节 = 100×4**。来源：`rtc.json` `0x14011aa60` `fn_14000db0c`（`CRouteMap` 调用；该函数反编译失败，只留下 RTC 名与尺寸）。

`[推断]` 玩家「额外船体」住在实例 0–99（新购实际 8–99）；海上 5 艘可以是 194+ 的开场实例，所以**持有总数可以大于 100**。100 是坞扫描/分配上界，不是「海上+坞内合计 100」。

### 2.3 停泊港在实例上，不是另张坞表

- `[确认]` `fn_14000175d` 读 `CShip+0x54`。来源：`fn_1400a1140.cpp`。
- `[确认]` 船体初始化把 `+0x54` 写成 `0x62=98`。来源：`01-ship-model.md`（该篇仍标未知；本篇按用法升级）。
- `[确认]` 98 号港不存在；舰队模板空槽的 `town` 也常用 `0x62`。来源：`02-fleet-proxy.md`、`TownID.h` `NUM_TOWN=97`。
- `[确认]` `fn_14011b5e0`：`eShip` 0–99，实例非空且 `fn_14000175d(ship)==town_id` 才写入列表。
- `[确认]` `fn_140010b95`：实参 `== 0x62` 时按 `eTown2` 扫 97 港（`>= 0x61`）再收集；否则只收一座港。来源：`fn_140115330.cpp`。
- `[确认]` `CShip::vfunc_07` 把 `+0x54` 当字节写入存档（`fn_14000c6da`）。坞状态没有独立表，就在 207 个 `CShip` 里。

`[推断]` `+0x54` 是停泊 `town_id`：在海 / 未入坞 = `0x62`；入坞 = 0–96。换船 UI 只列出**本港**坞内船 + 当前舰队 5 艘。船停在里斯本，人在汉堡看不见。

没有「每港坞位数」表。97 港共享同一组 0–99 实例，用 `town_id` 过滤。

### 2.4 四条入坞来源

**买 / 复制** — 见 2.1。复制改造船走 `fn_1400086b6`（`aeMyShip` vs `aeBuyShip`，按虚函数 `+0x18` 对船体种类），同样要空坞槽。

**解散委任** — `[确认]` 「将各艘船放在船坞里。请决定新舰队的旗舰。」来源：`0x30a3`。委任流程见 `02`；本系统收「这批实例入当前港坞」的命令。

**海战俘船** — 链是：

1. `[确认]` 战斗结束局部量 `cCatchedShipArray` **24 字节**。来源：`rtc.json` `fn_14000ec6e` 等。
2. `[确认]` 「拖走捕获的船。」「拖拉的船丢失了。」「把拖来的船停泊在船坞里。」「将没装的货物放到拖船的仓库里，行吗？」来源：`0x342f` / `0x343f`。
3. `[确认]` `aeInTowShip` 400 字节与 `aeDockShip` 400 字节出现在同一函数 `fn_14000db0c`，由 `CRouteMap` 调用（大地图入港）。
4. `[推断]` 单场俘船 ≤ 5（敌舰队也是 5 槽；24 字节像 5×id + 计数）。拖航数组按 100 槽配齐，是为了和坞仓同宽，不是单场能拖 100 艘。
5. `[确认]` 入坞前可整理货：`COrganizeLoadView`、`CCapturedLoadSprite`、`ClearHoldRecord`。来源：`COrganizeLoadView.cpp` RTC `sShipLoad`。

**舰队→坞** — `[确认]` `fn_1401767e0` RTC `aeDockShip` 828b + `eFleetShip`：对仍在本舰队的船写空人物 `0xEC`，有货则 `fn_1401778f0`，再 `fn_140010b95` 收集坞列表。来源：`fn_14016cc40.cpp`。

### 2.5 换船 UI：`CShipArrangeView` / `CShipChangeView`

继承：`CShipChangeView → CShipArrangeView → …`。来源：`class_hierarchy.txt`。

`[确认]` `CShipArrangeView` 构造（`0x140232280`）RTC `eShip`、`eFleetShip`：

- `+0x270` 起 5 个 dword = `0xD0`（海上 5 槽）
- `+0x284` 起 100 个 dword = `0xD0`（坞 100 槽）
- 另清 10 个指针 `+0x440`、2 个 `+0x4a0`

`[确认]` `CShipChangeView` 构造（`0x140232ef0`）先调 Arrange 构造，再写：

- `+0x4cc = 0xD0`（当前旗舰实例，空哨兵）
- `+0x4b8` 起 5 个 dword = `0xD0`（新编成）
- `+0x4d0` 起 100 个 dword = `0`（不是 `0xD0`；`[推断]` 选中/高亮，不是实例 id）
- 析构尺寸 `0x758`

`[确认]` `CShipChangeView::vfunc_13` 按 `eShip` 0–99 调 `fn_140014bb4`，非空则把 `+0x4d0[i]` 交给虚函数 `+0x80`（刷新坞侧 100 个格子）。

`[确认]` `CShipChangeView::vfunc_12` 确认换船：

- 新编成里不再出现的船：`fn_140009ade(..., 0xEC)` 清舱室人物
- 新旗舰舰长写成提督；其余 1–4 槽若舰长仍是提督也清掉
- 文案「一旦改变旗舰，船舱担任者将被解任。」「提督不担任舰长怎么行？」见 `03` / `0x2f0f` / `0x80a7`
- 失败分支立即数 `0x108` / `0x109`（文案 `[未知]`）

`[确认]` `CShipMoveButton` 是换船界面上的拖动按钮（坐标写死在构造）。来源：`CShipMoveButton.cpp`。

`[推断]` 玩家在本港把坞内一艘拖进海上 5 槽、把换下来的那艘写下 `+0x54=当前港`。开场实例 194+ 若只改 `+0x54` 而不搬到 0–99，坞扫描会看不见——入坞是否拷贝到 8–99 `[未知]`。

### 2.6 选船与编货

| 类 | 尺寸 | 作用 | 来源 |
| --- | --- | --- | --- |
| `CSelectShipView` | `0x290` | 通用选一艘（卖/修/复制源） | 析构 `fn_14000750e(..., 0x290)` |
| `CSelectShipScrollView` | `0x1e8` | 列表滚动（坞侧可到 100 条） | 同目录 |
| `CSelectShipFrameView` | — | 选船框；坐标写死 | 构造 |
| `cSelectShipPalam` | 40 B | 选船参数块，造船所多处 RTC | `rtc.json` |
| `COrganizeLoadView` | `0x3d0` | 货格对拖 | 析构 `0x3d0` |

`[确认]` `COrganizeLoadView` 构造清 5 个指针 `+0x290`、**25** 个 `+0x2b8`（`0x19`）、5 个 `+0x380`。点击：5–9、11–35（25 格）、37–41。空货 `0x6A`，写回走 `ClearHoldRecord`。25 = 5 船 × 5 格 `[推断]`，对应「拖船仓库」或对侧舰队货。

`[确认]` 港口菜单 `fn_140004cb4` RTC `cDock` 16 B + `asShipLoad` 200 B（= 25×8 货记录）。选项 1 构造 `CMpDock`；选项 2 走货。来源：`fn_14016cc40.cpp`。

### 2.7 `CMpDock` 不是仓库

- `[确认]` `CMpDock` 只有 ctor/dtor，体积 `0x10`。来源：`town/CMpDock.cpp`。
- `[确认]` 与 `CMpRemains` 同属地图点。来源：`12-ui-save/01-scenes-flow.md`、`class_files.tsv`。
- `[确认]` 设施文档：`CMpDock` 是海域/港口地图点，不是港内「码头」菜单。码头是 `CPort`，造船是 `CShipyard`，二者同样空壳。
- `[确认]` 仓储不在 `CMpDock` 字段里，在 `CShip[0..99]` 的空标记与 `+0x54`。

点地图上的「船坞」热点可以打开换船/选船 UI `[推断]`；实现时把 `map_point_kind` 映射成 `OpenDock` 命令，禁止在 `CMpDock` 里存实例表。

### 2.8 造船所菜单（只与坞有关的分叉）

`[确认]` `fn_140004fd9` 三项菜单：0 = `fn_1400068c0` 买新船，1 = `fn_140007531` 复制已改造船，其它退出。两条都要空坞槽。卖/修/改在其它 `fn_*`（`04`）。

`[确认]` 「卖哪艘船？」`0x7edf`。列表来源是 `fn_140177030`：先塞当前舰队 5 艘（跳过 `0xD0`），再塞本港 0–99 坞内船（去重）。所以坞里的船不必先换上舰队就能卖 `[推断 用途]`。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空 / 非法 | 重构 key |
| --- | --- | --- | --- | --- |
| 船体定义 | `eShip` | 100 | `INVALID_SHIP=-1` | `ship_id`（目录，本篇不拥有） |
| 船实例 | 无独立枚举 | 207 | `0xD0` | `ship_instance_id` |
| 坞槽下标 | 循环当 `eShip` 0–99 用 | 100 | 空实例 `+0x51=0x28` | `dock_slot` |
| 舰队船位 | `eFleetShip` | 5 | `INVALID_FLEETSHIP=-1`；槽内 `0xD0` | `fleet_ship_slot` |
| 停泊港 | `CShip+0x54` | 97 港 | `0x62=98` = 在海/未入坞 | `dock_town_id` |
| 拖航列表 | `aeInTowShip` | 数组 100×u32 | `0xD0` `[推断]` | `towed_slot` |
| 俘船缓冲 | `cCatchedShipArray` | 24 B | — | 战斗交出的 id 列表 |
| 地图点船坞 | `CMpDock` | 类，非表 | — | `map_point_kind`（world） |

重构约定：

- `ship_id` 永远是目录。`dock_slot` 只是「玩家额外船体」的下标，**不要**复用 `eShip` 类型。
- 原版 0–99 实例带恰好等于 `NUM_SHIP`。模组可以 `hulls.len != dock_slots`；代码按各自表长循环。
- `dock_town.empty = 0x62`，不要用 `-1`（与 `INVALID_TOWN` 撞）也不要用 `0`（伦敦）。
- `ship_instance.empty = 0xD0`，与舰队空槽相同。

## 4. 算法 / 公式

全部进 `config/caps.yaml` 与 `config/ships/dock.yaml`。禁止在换船/买船代码里写裸 `5`、`8`、`100`、`0x62`、`0xD0`。

```
# 空槽（原版 fn_1400018e8）
alloc_dock_slot():
  for id in [caps.dock_alloc_lo, caps.dock_slots):    # 8 .. 100
    if Ships.isEmptyInstance(id):
      return id
  return none

can_accept_hull =
  alloc_dock_slot() != none

# 买船（原版 fn_1400068c0）
buy_new_ship(town_id, ship_id):
  slot = alloc_dock_slot()
  if slot is none:
    reject owned_cap          # 「持有的船过多了」
  if not Yard.canBuild(town_id, ship_id):
    reject no_build           # 「这个城市没有可以制造的船」
  if Fleets.shipCount(player_fleet) >= caps.fleet_ships:
    hint fleet_full_to_dock   # 只提示，不改落点
  gold = Yard.buyPrice(ship_id)     # 原版虚+0x20 后 << 1，公式在 04
  pay; Ships.createInSlot(slot, ship_id)
  Dock.setTown(slot, town_id)       # +0x54 = town
  emit ShipDocked { slot, town_id }

# 本港可见列表（原版 fn_140177030）
visible_at(town_id, fleet_id):
  out = []
  for slot in Fleets.ships(fleet_id):          # 最多 fleet_ships
    if slot != empty: out.append(slot)
  for id in [0, caps.dock_slots):
    if Ships.isEmptyInstance(id): continue
    if Dock.townOf(id) != town_id: continue
    if id in out: continue
    out.append(id)
  return out

# 入坞
dock(instance_id, town_id):
  Cabins.unassignAll(instance_id)              # 0xEC；03 执行
  Dock.setTown(instance_id, town_id)
  Fleets.clearSlotIfPresent(instance_id)
  emit ShipDocked { instance_id, town_id }

# 出坞换入舰队
swap(fleet_id, fleet_slot, dock_instance_id, town_id):
  outgoing = Fleets.ships(fleet_id)[fleet_slot]   # 可 empty
  Fleets.setSlot(fleet_id, fleet_slot, dock_instance_id)
  Dock.setTown(dock_instance_id, sentinels.dock_town_at_sea)
  if outgoing != empty:
    Dock.setTown(outgoing, town_id)
    Cabins.unassignAll(outgoing)
  if fleet_slot == flagship:
    emit FlagshipChanged { fleet_id, dock_instance_id }
  emit ShipSwapped { fleet_id, outgoing, dock_instance_id }

# 俘船入港
on ArrivedAtTown(fleet_id, town_id):
  for id in Dock.towed(fleet_id):
    if not can_accept_hull:
      reject owned_cap          # 拖着但坞满；原版是否丢船 [未知]
      break
    dock(id, town_id)
    emit TowedShipDocked { id, town_id }
  Dock.clearTowed(fleet_id)
```

`[未知]` 原版 `hull_price<<1` 是否即买价，见 `04`。本篇只规定**落点在坞**。

`[推断]` 「不能再把船处置掉了」= 卖掉后玩家海上+坞内会变成 0 艘。未证明是否只拦舰队最后一艘、剧情船能否卖。配置里放 `min_owned_ships`，默认 1，触发条件未钉死前不要在代码写死。

坞内船货格仍走 `Ships.holds`。卖掉前提示「还有剩余货物」是 UI 确认，确认后 `Ships.clearHold` 各格写成 `trading_item.empty`。

## 5. 耦合分析

原版硬编码：

- `eShip` 一个枚举既扫船体目录又扫 0–99 实例
- `CShip+0x54` 停泊港夹在船体存档里，没有 `CDock` 表
- 买船、复制船、换船、拖船入港直接 `new CSelectShipView` / `CShipChangeView`
- 入坞写死 `0xEC` 清人物，不经人事系统
- `NUM_SHIP=100`、`NUM_FLEETSHIP=5`、分配起点 `8`、哨兵 `0x62` / `0xD0` 写在循环里
- `CRouteMap` 直接调 `fn_14000db0c` 处理拖船+坞
- NPC 舰队引用常 ≥ 100，与玩家坞 0–99 错开；玩家开场船 194+ 又在另一段

重构允许的方向：

```
dock → Ships.instance_id / town_id
dock → Fleets.fleet_id, fleet_ship_slot
dock → Cabins（只发 unassign 事件）
dock → Yard.buyPrice / canBuild（查询）
town-life → Dock.open / Dock.store
combat → Dock.receiveTowed（经 BattleResolved.captured_ships）
ocean → Dock（入港时交出 towed 列表）
```

禁止：economy 改坞槽；combat 写 `CShip+0x54`；ui 存 207 槽指针；把 `dock_slots` 写回 `hulls.yaml` 行数。

## 6. 解耦后的配置

```yaml
# config/caps.yaml（只摘本系统）
fleet_ships: 5
dock_slots: 100              # 原版与 NUM_SHIP 同数；运行时读本键，不读船体目录长
dock_alloc_lo: 8             # 原版 fn_1400018e8 起点
ship_instance_pool: 207
min_owned_ships: 1           # 「不能再处置」；触发未钉，先可配
towed_slots: 100             # aeInTowShip 400/4
captured_per_battle: 5       # [推断] 与舰队 5 同宽
```

```yaml
# config/ids/ 生成
sentinels:
  ship_instance_empty: 0xD0
  hull_uninitialized: 0x28
  dock_town_at_sea: 0x62     # 98；不是 INVALID_TOWN
  person_unassigned: 0xEC
```

```yaml
# config/ships/dock.yaml
schema: dk4.ships.dock
version: 1

# 原版把 207 实例切成几段。重构按表配置，禁止再写 0xC2。
instance_bands:
  - key: reserved_lo
    lo: 0
    hi: 8                    # 不含 8
    role: unknown_reserved   # 分配器跳过
  - key: player_dock
    lo: 0
    hi: 100
    alloc_lo: 8
    role: extra_hulls
  - key: other_mid
    lo: 100
    hi: 194                  # 原版 fn_14000a984 上界 0xC2
    role: unknown            # NPC 引用常在此 [推断]
  - key: player_start
    lo: 194
    hi: 207
    role: starting_flagships # World 模板 C2… [推断]

parking:
  field: dock_town_id        # 原版 CShip+0x54
  at_sea: 0x62
  per_town_cap: null         # 原版无每港上限；全局共享 dock_slots
  visible_with_fleet: true   # 本港列表 = 舰队 5 + 本港坞

intake:
  buy_always_docks: true     # 原版先分配坞槽，不先填舰队空位
  copy_remodeled_docks: true
  captured_docks_on_arrival: true
  proxy_disband_docks: true
  fleet_full_hint: true      # 仅提示，不改变落点

swap:
  unassign_cabins_on_leave: true
  admiral_must_captain_flagship: true
  ui: CShipChangeView
  picker: CSelectShipView
  cargo_view: COrganizeLoadView

tow:
  persist_while_sailing: true
  can_be_lost: true          # 「拖拉的船丢失了」
  cargo_prompt_on_dock: true
```

World0 / World1 共用这份规则。坞内船是存档状态，不是内容包。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗改代码：

- `dock_slots`、`dock_alloc_lo`（与 `ship_hulls` 脱钩）
- `fleet_ships`（舰队系统已有；本系统只读）
- `min_owned_ships`
- `towed_slots`、`captured_per_battle`
- 买船是否一律入坞，还是舰队未满时直接上阵
- 是否按港过滤（关掉 = 全球船坞，任意港能换任意坞内船）
- 入坞是否清舱室、是否提示剩货

改这些会碰到边界，但仍不应改海战伤害或市价：

- 俘船何时从 `cCatchedShipArray` 变成 `aeInTowShip` → combat 事件字段
- 开场船放在哪一段实例 → 内容包 `fleets/templates.yaml` 的 `ship_instance_id`

显示层不得用「选船列表画得下几行」倒逼 `dock_slots`。

## 8. 对外接口

事件：

- `ShipDocked { ship_instance_id, town_id, reason }`  reason: `buy` / `copy` / `swap` / `capture` / `proxy_disband`
- `ShipUndocked { ship_instance_id, fleet_id, fleet_ship_slot }`
- `ShipSwapped { fleet_id, outgoing_id, incoming_id, town_id }`
- `OwnedCapReached { town_id, attempted }`
- `TowedShipLost { ship_instance_id }`
- `TowedShipDocked { ship_instance_id, town_id }`
- `DockCargoPrompt { ship_instance_id }`  # 「还有剩余货物」；UI 确认后再 `Ships.clearHold`

查询（只返回 ID 与标量）：

- `Dock.list(town_id) -> [ship_instance_id]`  不含在海（`at_sea`）
- `Dock.listAll() -> [{ ship_instance_id, town_id }]`
- `Dock.townOf(ship_instance_id) -> town_id | at_sea | empty`
- `Dock.freeCount() -> int`  （按 `alloc_lo .. dock_slots`）
- `Dock.canAccept() -> bool`
- `Dock.visibleAt(town_id, fleet_id) -> [ship_instance_id]`
- `Dock.towed(fleet_id) -> [ship_instance_id]`

命令：

- `Dock.allocate(ship_id, town_id) -> ship_instance_id | OwnedCapReached`
- `Dock.store(ship_instance_id, town_id)`
- `Dock.retrieve(ship_instance_id, fleet_id, fleet_ship_slot)`
- `Dock.swap(fleet_id, fleet_ship_slot, dock_instance_id, town_id)`
- `Dock.receiveTowed(fleet_id, town_id, [ship_instance_id])`
- `Dock.loseTowed(ship_instance_id)`
- `Dock.scrap(ship_instance_id)`  # 卖/弃；金币由 Yard/economy 结算

town-life 点击造船所 / 船坞热点只发 `OpenDock { town_id, fleet_id }` 或已有的 `OpenShipyard`。combat 的 `BattleResolved.captured_ships` 由 ocean 在入港时转成 `Dock.receiveTowed`，战斗模块不写 `town_id`。

存档：原版无独立坞节，状态在每个 `CShip` 的空标记 + `+0x54`。重构可仍派生自实例，或显式写：

```yaml
# save 节 dock（可选；与实例二选一，禁止双写）
- ship_instance_id: 12
  town_id: 11          # 里斯本
```

拖航列表是会话/航行状态，入港成功后应空；存档是否保留未入港的拖船 `[未知]`，有则进 save，无则出港才允许拖。

## 9. 未核实字段

- `0–7` 为何不参与 `fn_1400018e8`。`NUM_PLAYEREVENTSHIP` 仍无整数。`CShipElement+0x6788` 的 8 个 `0xD0` 是否对应这 8 槽。
- 100–193 段 `fn_14000a984` 按虚函数 `+0x18` 匹配的语义（船体种类？势力？）。
- 开场 194+ 入坞时：改 `+0x54` 还是拷到 8–99。不搬则本港坞列表看不见。
- `CShip+0x54` 以外是否还有「所属提督 / 私家」字节。`01` 的 `+0x53=0x0D` 未证明。
- 「不能再把船处置掉了」调用点；是否只拦舰队最后一艘、剧情赠船能否卖。
- 舰队未满时，买来的坞内船会不会被某条隐藏逻辑立刻 `swap` 进空的 `eFleetShip`。原版买船函数本身不写 5 槽。
- `fn_14000db0c` 反编译失败；拖航丢失条件（风暴？战斗？距离？）。
- `cCatchedShipArray` 24 字节切分；单场俘船硬顶是否确为 5。
- `COrganizeLoadView` 点击 5–9 / 11–35 / 37–41 与「本队 5 船 × 5 格 vs 拖船」的一一对应。
- `CShipChangeView+0x4d0[100]` 初始化为 0 的字段名；`0x108` / `0x109` 文案。
- `CPlazaView` / `eMapPointKind` 哪一个 kind 是船坞热点；`CMpDock` 与 `CPort` / `CShipyard` 的点击边。
- 卖掉坞内船的价格公式（`04` 的 `<<1` 是否共用）。
- 坞内船是否仍消耗维修、是否能在本港直接修理而不换上舰队（选船列表含坞内船 `[推断]`）。
- 存档里拖航中的船：出港存盘再读，是否还挂在 `aeInTowShip`。
