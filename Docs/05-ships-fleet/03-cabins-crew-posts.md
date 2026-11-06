# 舱室、船员与岗位

人物系统拥有人（`person_id`）。船体系统拥有 31 个舱室槽（`ship_id` 定义默认布局，`ship_instance_id` 持有当前布局）。本篇定义**绑定表**：`(ship_instance_id, cabin_id) → { person_id, post_id }`，以及换旗舰时怎么拆绑。

## 1. 职责边界

本系统管：

- 每船 31 个舱室槽（`eShipCabin`）
- 舱室种类（`eCabinKind`）与岗位（`ePost`）
- 槽 ↔ 人物的绑定、解绑、最佳人选 UI
- 换旗舰时「船舱担任者将被解任」

本系统不管：

- 人物六维、技能、招募、名册 98 槽（`03-characters`）
- 船体耐久 / 货格（`01-ship-model.md`）
- 舱室对航海 / 战斗的数值效果公式（本篇列出**效果入口**，公式参数进 `formulas/`，战斗与航海来查）
- 甲板装修按钮布局（UI）

## 2. 原版事实

### 槽数

- `[确认]` `NUM_SHIPCABIN=31`，`INVALID_SHIPCABIN=-1`。来源：`ShipCabinID.h`、`num_values.json`。
- `[确认]` `NUM_CABINSURFACE=31`。甲板画 31 个舱室表面。来源：`num_values.json`。
- `[确认]` `CDeckPersonnelView` / `CDeckRemodelView` / `CShip::vfunc_05` / `CShipType::vfunc_05` 都按 `eShipCabin >= 0x1F` 循环。来源：对应 cpp。
- `[确认]` `NUM_CABINKIND` 整数未还原。`CabinID.h` 与 `CabinKindID.h` 恢复时都写成了 `eCabinKind`（源码还原赝品），当作同一种枚举。
- `[确认]` `NUM_POST` 整数未还原。`PostID.h` 只有符号。

### 空值

- `[确认]` 人事初始化把舱室人物写成 `0xEC`。来源：`CPlayerFleetData::vfunc_05`（`s2C = 0xec`，循环 31）。
- `[推断]` `0xEC=236` 在 `NUM_PERSON=235` 外侧，是空人物哨兵（与舰队模板 `+0x14` 常用值相同）。
- `[确认]` 船体初始化若舱室种类 `== 0x17` 则改成 `0x18`。来源：`CShip::vfunc_05`、`CShipType::vfunc_05`。两种类的名字 `[未知]`。

### 旗舰与舰长

- `[确认]` 「一旦改变旗舰，船舱担任者将被解任。」来源：`mesfile_sc.txt` `0x2f0f`。
- `[确认]` 「请选择要更换舰长的船。」「将舰长委任给属下。」「任命谁为舰长？」「可以吗？解雇舰长，并交给其属下的水手吗？」来源：`0x2ed3`–`0x2f33`。
- `[确认]` 「提督不担任舰长怎么行？」来源：`0x80a7`。
- `[确认]` 海上命令必须从舰长室发：「提督，请从舰长室下命令。」来源：`0x5e4f` 一带。

### 舱室种类（文案，不是完整枚举）

全部来自 `mesfile_sc.txt` `0xc0b3`–`0xc59f` 及岗位对白。名字可进 `cabins.yaml`；**id 未还原**，配置里先用 key，抽出 exe 表后再填整数。

| key | 文案效果 | 上限（文案） |
| --- | --- | --- |
| `captain_cabin` | 舰长室，发航海命令 | — |
| `carpenter` | 船木工：航行中修船 | — |
| `doctor` | 船医：治船员 / 水手的病 | — |
| `cook` | 大厨：节约水与粮食，减轻水手疲劳 | — |
| `keeper` | 饲养员：节约粮食 | — |
| `missionary` | 传教士：消水手不满；广场可散发交易品 | — |
| `staff_officer` | 参谋：酒馆拟定策略 | — |
| `purser` | 主计长 / 会计长：交易所杀价、买进屯积 | — |
| `assault_leader` | 冲锋队长：提高肉搏；无此人不能武力镇压哗变 | — |
| `marine_room` | 水兵室 | 最多 4 |
| `gunner_room` | 炮手：炮击间隔变短 | 最多 4 |
| `supply_room` | 收藏水与粮食 | — |
| `cargo_room` | 收藏交易品 | 最多 5 |
| `rest_room` | 「在这里船员身体和精神都能够恢复」 | — |
| `morale_room` | 「船员进到这里能恢复精神，水手也能减少疲劳」 | — |

岗位对白还出现：副官、操舵手、操帆手、测量士 / 测量员、甲板要员、瞭望手。这些是 **`ePost` 文案**，是否 1:1 对应舱室种类 `[未知]`。

- `[确认]` 「最多可以设４个水兵室。」「如果有炮手……最多可以设４个室。」「收藏交易品的房间。最多可以设５个。」来源：`0xc503` / `0xc59f`。
- `[确认]` 「可以追加船舱。但是，追加船舱速度会降低。」来源：`0xc5d7`。额外舱室是改装件，见 `04`。

### 等级门闩

- `[确认]` `fn_1400040bb`（`0x140069270`）RTC `s_abGradeShipCabin`，**124 字节 = 31 个 int**。用「船的等级 vs 该槽要求」决定舱室能不能开。来源：`game/unassigned/fn_1400678d0.cpp`。
- `[确认]` 31 个立即数（下标 0–30）：

```
0,0,0, 1,1, 0,0,0,0,0,0, 1, 0,0,0,0,0,0, 2,
1,1,1,1,1,1, 2,2,2,2,2,2
```

比较：`if (required > ship_grade) 拒绝`。因此至少存在等级 0/1/2。`NUM_SHIPGRADE` 仍 `[未知]`。

### 人事 UI

- `[确认]` `CDeckPersonnelView`：31 个舱室按钮（`+0x170` 起）、5 个船页（`+0x130`）、`+0x278=0xEC`。来源：`CDeckPersonnelView.cpp`。
- `[确认]` `CCabinBestPersonView`：为某个舱室挑人。来源：类名 + 构造。
- `[确认]` `CDeckNoPostButton`：无岗位 / 未配置。来源：类名。
- `[确认]` `CDeckCabinButton`。来源：`game/ship/`。
- `[确认]` 未配置水手确认：「%s名水手未配置，可以吗？」来源：`mesfile_sc.txt` `0x86b`。
- `[确认]` 函数 `0x14009a130` RTC：`nNumPerson`、`aePerson`、`eShipCabin`。来源：`functions.tsv`。
- `[确认]` `0x140155190` RTC：两份 `aeCabinKind` 各 940 字节、两份 `aePerson` 各 940 字节。`940/4=235=NUM_PERSON`。来源：`rtc.json`。这是按人物扫舱室，还是 5 船展开，未钉死。

### `CPlayerFleetData` 绑定块

相对对象（`vfunc_05/06/07`）：

| 偏移 | 内容 | 等级 |
| --- | --- | --- |
| `+0x08` `+0x09` | 各 1 字节 | `[未知]` |
| `+0x0A` 起 31 字节 | 旗舰舱室，初始化 `0xEC` | `[推断]` 人物或种类 |
| `+0x29=0x0B` `+0x2C=-1` `+0x30=0x1B` `+0x34=0` | | `[未知]` |
| `+0x38` 起 `4×31×2` 字节 | 另外 4 艘船；每槽 2 字节 | `[推断]` `person_id` + `post_id` / 种类 |
| `+0x130` | dword | `[未知]` |

`4×31×2=248=0xF8`，`0x38+0xF8=0x130`，与下一字段对齐 `[确认 尺寸]`。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空 | 重构 key |
| --- | --- | --- | --- | --- |
| 舱室槽 | `eShipCabin` | 31 | `INVALID_SHIPCABIN=-1` | `cabin_id` |
| 舱室种类 | `eCabinKind` | `[未知]` | `INVALID_CABINKIND=-1` | `cabin_kind_id` |
| 岗位 | `ePost` | `[未知]` | `INVALID_POST=-1` | `post_id` |
| 人物 | `ePerson` | 235 | `-1`；绑定空值 `0xEC` | `person_id` |
| 绑定 | — | 每船 31 | 人物 `empty_person` | `cabin_binding` |

重构绑定主键：`(ship_instance_id, cabin_id)`。不要用 `CShip*` 当键。

同一个人不能同时占两个槽。换船 / 换旗舰先解绑再绑。

## 4. 算法 / 公式

```
# 槽是否对这艘船开放
cabin_unlocked(ship_grade, cabin_id) =
    ship_grade >= cabins.grade_required[cabin_id]     # s_abGradeShipCabin

# 种类数量上限（文案；参数化）
count(kind == marine_room)  <= caps.cabin_marine_max     # 4
count(kind == gunner_room)  <= caps.cabin_gunner_max     # 4
count(kind == cargo_room)   <= caps.cabin_cargo_max      # 5
# cargo_room 上限应与 hold_slots_per_ship 对齐，但原版是两套文案，配置里分开写

# 初始化别名
cabin_kind = alias.get(raw, raw)                         # 0x17 → 0x18

# 换旗舰
on FlagshipChanged:
    for each cabin of that fleet:
        binding.person_id = empty_person
        binding.post_id   = empty_post

# 最佳人选（UI；评分公式未还原）
best_person(cabin_kind) = argmax(persons, score(person, cabin_kind))
# score 原版未抽出。重构先暴露接口，default 内容包可以先按相关技能排序，
# 但不得把未证实的权重写成 [确认]
```

舱室效果（给其它系统的**开关**，不是本篇的伤害公式）：

| key | 订阅方 | 文案保证 |
| --- | --- | --- |
| `has_carpenter` | ocean / repair | 航行中可修 |
| `has_doctor` | characters | 可治病 |
| `has_cook` | ocean supplies | 节水粮、减疲劳 |
| `has_keeper` | ocean supplies | 节粮食 |
| `has_missionary` | ocean morale, town plaza | 消不满；广场散发 |
| `has_staff_officer` | town tavern | 可拟定策略 |
| `has_purser` | economy | 杀价 / 屯积 |
| `has_assault_leader` | combat boarding, ocean mutiny | 肉搏加成；否则不能武力镇压 |
| `gunner_rooms` | combat | 炮击间隔缩短，最多 4 |
| `marine_rooms` | combat | 最多 4 |
| `captain_on_flagship` | ocean command | 无舰长室不能下令 |

数值幅度（缩短多少间隔、节约百分之几）`[未知]`，进 `formulas/` 时 default 先留 null，禁止编造。

## 5. 耦合分析

- 船体定义自带 31 默认种类；实例再被改装改写（`04` 的追加船舱）。
- 人物系统只回答「这个 `person_id` 能不能当这个 `post_id`」（技能 / 装备限制）。绑定表不存六维。
- 换旗舰事件来自 `02-fleet-proxy.md`。
- 贸易「最多 5 个交易品房间」与货格 5 对齐，但是舱室种类，不是货格本身。货格仍属 `01`。
- `aePerson[235]` 与存档名册 98 水手槽不是同一集合（`04-id-registry.md`）。能进舱室的是在队人物，规则由人物系统判定。

## 6. 解耦后的配置

```yaml
# config/caps.yaml
cabins_per_ship: 31
cabin_marine_max: 4
cabin_gunner_max: 4
cabin_cargo_max: 5
empty_person: 0xEC
```

```yaml
# config/ships/cabins.yaml
schema: dk4.ships.cabins
version: 1
alias:
  - from: 0x17
    to: 0x18
grade_required: [0,0,0,1,1,0,0,0,0,0,0,1,0,0,0,0,0,0,2,1,1,1,1,1,1,2,2,2,2,2,2]
kinds:
  - key: captain_cabin
    id: null
    name: 舰长室
    effect: ocean_command
  - key: carpenter
    id: null
    name: 船木工
    effect: repair_at_sea
  - key: doctor
    id: null
    name: 船医
    effect: heal
  - key: cook
    id: null
    name: 大厨
    effect: save_water_food
  - key: keeper
    id: null
    name: 饲养员
    effect: save_food
  - key: missionary
    id: null
    name: 传教士
    effect: calm_crew
  - key: staff_officer
    id: null
    name: 参谋
    effect: tavern_scheme
  - key: purser
    id: null
    name: 主计长
    effect: market_bargain
  - key: assault_leader
    id: null
    name: 冲锋队长
    effect: boarding_lead
  - key: marine_room
    id: null
    name: 水兵室
    max_per_ship: 4
  - key: gunner_room
    id: null
    name: 炮手
    max_per_ship: 4
  - key: supply_room
    id: null
    name: 水和粮食的房间
  - key: cargo_room
    id: null
    name: 收藏交易品的房间
    max_per_ship: 5
  - key: rest_room
    id: null
    name: "[文案]船员身心恢复"
  - key: morale_room
    id: null
    name: "[文案]恢复精神、减少疲劳"
```

```yaml
# config/characters/posts.yaml
schema: dk4.characters.posts
version: 1
# 岗位目录属人物系统；本篇引用 post_id。
- key: captain
  name: 舰长
- key: adjutant
  name: 副官
- key: purser
  name: 会计长
- key: helmsman
  name: 操舵手
- key: sailhand
  name: 操帆手
- key: surveyor
  name: 测量士
- key: doctor
  name: 船医
- key: carpenter
  name: 船木工
- key: gunner
  name: 炮手
- key: assault_leader
  name: 冲锋队长
- key: cook
  name: 大厨
- key: missionary
  name: 传教士
- key: keeper
  name: 饲养员
- key: staff_officer
  name: 参谋
- key: deckhand
  name: 甲板要员
- key: lookout
  name: 瞭望手
```

```yaml
# 运行时状态（存档，不是内容包）
# save/fleets/*/cabin_bindings.yaml
- ship_instance_id: 12
  cabin_id: 0
  cabin_kind_id: null
  person_id: 0
  post_id: captain
```

## 7. 独立旋钮

- 每船舱室槽数（原版 31；改了只影响本系统与甲板 UI）
- `grade_required[31]`
- 水兵室 / 炮手室 / 货舱上限
- 空人物哨兵
- 种类 ↔ 效果 key 的映射（战斗只认 `effect:`，不认中文名）
- 岗位表行数（人物系统）

加一种舱室：在 `cabins.yaml` 加行 + 一个 `effect` key。不要去改 `CTrade`。

## 8. 对外接口

事件：

- `CabinBound { ship_instance_id, cabin_id, person_id, post_id }`
- `CabinUnbound { ship_instance_id, cabin_id, person_id }`
- `CabinsCleared { fleet_id }`          # 换旗舰之后
- `CabinKindChanged { ship_instance_id, cabin_id, cabin_kind_id }`  # 改装追加船舱

查询：

- `Cabins.layout(ship_instance_id) -> [{ cabin_id, kind_id, unlocked }]`
- `Cabins.binding(ship_instance_id, cabin_id) -> { person_id, post_id }`
- `Cabins.personShip(person_id) -> { ship_instance_id, cabin_id, post_id } | none`
- `Cabins.flags(ship_instance_id) -> { has_carpenter, has_doctor, gunner_rooms, ... }`
- `Cabins.bestPerson(cabin_kind_id) -> person_id | none`

命令：

- `Cabins.assign(ship_instance_id, cabin_id, person_id, post_id)`
- `Cabins.unassign(ship_instance_id, cabin_id)`
- `Cabins.unassignPerson(person_id)`
- `Cabins.setKind(ship_instance_id, cabin_id, cabin_kind_id)`  # 仅改装系统调用

订阅：`FlagshipChanged` → 对该队 `CabinsCleared`。

## 9. 未核实字段

- `NUM_CABINKIND`、`NUM_POST`、`NUM_SHIPGRADE`
- 每种舱室的整数 id（文案有名字，表没抽出来）
- `0x17` / `0x18` 是哪两种
- 岗位与舱室种类是不是同一套 id
- `CPlayerFleetData`：旗舰 31 字节到底是 `person_id` 还是 `cabin_kind`；4 船的第 2 字节是 `post_id` 还是种类
- `+0x08` `+0x09` `+0x29` `+0x2C` `+0x30` `+0x34` `+0x130`
- `best_person` 的评分公式
- 各 `effect` 的数值幅度
- `aeCabinKind` / `aePerson` 940 字节数组的第二维（235 人 vs 5×47）
- `CDeckPersonnelView+0x278=0xEC`、`+0x27c=0x20` 的含义
