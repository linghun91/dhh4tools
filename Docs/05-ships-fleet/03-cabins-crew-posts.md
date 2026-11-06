# 舱室、船员与岗位

人物系统拥有人（`person_id`）和岗位定义（`posts.yaml`，`03-characters/03`）。船体系统拥有 31 个舱室槽（`ship_model_id` / `hull_id` 定义默认布局，`ship_instance_id` 持有当前布局）和舱室种类表（`cabins.yaml`）。本篇定义**绑定表**：`(ship_instance_id, cabin_id) → person_id`（岗位由舱室种类经 `posts.yaml.room` 反查得出，不另存），以及换旗舰时怎么拆绑；并提供两个一级查询 `Ships.cabin_count(ship, role)` 与（经 `02`）`Fleets.post_holder(fleet, post_key)`（D2）。

## 1. 职责边界

本系统管：

- 每船 31 个舱室槽（`eShipCabin`）
- 舱室种类（`eCabinKind`）表；岗位（`ePost`）只引用
- 槽 ↔ 人物的绑定、解绑、最佳人选 UI
- 换旗舰时「船舱担任者将被解任」

本系统不管：

- 人物六维、技能、招募、人物名册（`03-characters`）
- 船体耐久 / 货格（`01-ship-model.md`）
- 舱室对航海 / 战斗的数值效果公式（本篇列出**效果入口**，公式参数进 `formulas/`，战斗与航海来查）
- 甲板装修按钮布局（UI）

## 2. 原版事实

### 槽数

- `[确认]` `NUM_SHIPCABIN=31`，`INVALID_SHIPCABIN=-1`。证据类型：循环上界（下条四处 `>= 0x1F`）；RTC 投票 46 次为旁证。来源：`ShipCabinID.h`。
- `[推断]` `NUM_CABINSURFACE=31`。证据类型：仅 RTC 投票（`eCabinSurface` 与 31 比 2 次），未见循环。甲板画 31 个舱室表面。
- `[确认]` `CDeckPersonnelView` / `CDeckRemodelView` / `CShip::vfunc_05` / `CShipType::vfunc_05` 都按 `eShipCabin >= 0x1F` 循环。来源：对应 cpp。
- `[确认]` `CabinID.h` 与 `CabinKindID.h` 恢复时都写成了 `eCabinKind`（源码还原赝品），当作同一种枚举。头文件没写整数。
- `[推断]` `NUM_CABINKIND=23`（`0x17`）。证据类型：判界（取表函数 `< 0x17`）+ 循环上界（`0x1401d0d05`）；头文件无整数，表本身 24 行含「无」，故不升确认。见下「舱室种类表」。
- `[确认]` `NUM_POST=16`。证据类型：判界（名称表取表 `< 0x10`、`fn_140007e19` 跳表 `[0,15]`）+ `fn_140012dd2` 输出 0–15。见下「岗位」。

### 空值

- `[确认]` 人事初始化把舱室人物写成 `0xEC`。来源：`CPlayerFleetData::vfunc_05`（`0x1401df190`，`mov [rsp+0x2c],0xec`，循环 31）。
- `[推断]` `0xEC=236` 在 `NUM_PERSON=235` 外侧，是空人物哨兵（与舰队模板 `+0x14` 常用值相同）。
- `[确认]` 船模 / 船体初始化若舱室种类 `== 0x17` 则改成 `0x18`。来源：`CShip::vfunc_05`（`0x1402ae03b`）、`CShipType::vfunc_05`（`0x1402ae309`）。
- `[确认]` 运行时 `0x18` = 「此槽不是舱室」：`fn_14000b898`（`0x1401df5f0`）、`0x140115c93`、`0x1401c915e` 都先判 `kind == 0x18` 再跳过。`CPlayerFleetData` 4×31 记录第 2 字节也初始化为 `0x18`。
- `[推断]` 静态表里 `0x17`（名称表第 23 行「无」）只作「无舱室」的数据写法，载入时统一成 `0x18`。两者都不在合法种类 0–22 内。

### 旗舰与舰长

- `[确认]` 「一旦改变旗舰，船舱担任者将被解任。」来源：`mesfile_sc.txt` `0x2f0f`。
- `[确认]` 「请选择要更换舰长的船。」「将舰长委任给属下。」「任命谁为舰长？」「可以吗？解雇舰长，并交给其属下的水手吗？」来源：`0x2ed3`–`0x2f33`。
- `[确认]` 「提督不担任舰长怎么行？」来源：`0x80a7`。
- `[确认]` 海上命令必须从舰长室发：「提督，请从舰长室下命令。」来源：`0x5e4f`（与「没有航线」句同串）。

### 舱室种类表（exe）

- `[确认]` 静态初始化 `0x1400256d0` 连调 24 次 `fn_14000b7bc(entry, name, skill_u8, value_u16)`，建表 `0x144294b40`，步长 `0x18`：`+0x08` 名称指针、`+0x10` u8、`+0x12` u16。名称是 `.rdata 0x14052f1bc..` 的 GBK 串。所以种类 id ↔ 名称是**同一张表的行号**，不是只靠串序旁证：7 水兵室、8 武装炮台、9 物资仓库、10 货物仓库、11 划桨手室都 `[确认]`。
- `[确认]` 取表函数 `0x140068f30`（thunk `fn_14000c7fc`）：`0 <= kind < 0x17` 才返回条目，否则 `NULL`。合法种类 **23 个（0–22）**；第 23 行「无」在界外。反查 `0x1400690b0`：`(ptr - table) / 0x18`。
- `[确认]` `+0x10` 字节经 `fn_140009e53` 读出，调用方一律「`== 0xb` 则跳过」（`0x1400966b4`、`0x14009b7e0`、`0x1401ca7f6`、`0x1401d0d3b`、`0x140115cb6`）。取值 0–9 与 `eSkill` 0–9 一一合理对上（舰长室→统率、测量→测量术、会计室→会计术、参谋室→谋略、武装炮台→命中力、水兵室→剑术、了望台→观察力），`0xb` = 无岗位技能。字段名 `[推断]` = 该舱室担任者看的技能。技能名见 `03-characters/03-abilities-skills-jobs.md`。
- `[确认]` 下列种类的**玩法用途**有代码证据（`CountCabinKind` = `fn_140006b95`，扫 31 槽数 `vfunc+0x38 == kind`）：

| id | 名称（exe） | 技能 `+0x10` | `+0x12` | 代码用途 | 文案 |
| --- | --- | --- | --- | --- | --- |
| 0 | 桅杆 | 2 驾驶术 | 80 | 默认在槽 0–2（三根桅） | |
| 1 | 了望台 | 9 观察力 | 70 | | |
| 2 | 甲板 | 0 生命力 | 30 | | |
| 3 | 测量 | 3 测量术 | 120 | | |
| 4 | 船舵 | 2 驾驶术 | 60 | | |
| 5 | 舰长室 | 1 统率力 | 30 | | 「请从舰长室下命令」 |
| 6 | 副官室 | 7 说服力 | 100 | | |
| 7 | 水兵室 | 5 剑术 | 90 | 水手上限 `+60/间`（`fn_140013b92`） | 冲锋队长；最多 4 间 |
| 8 | 武装炮台 | 6 命中力 | 90 | 炮门 `24/间`（`fn_140004836`）；换炮价 `3^n`（`08`）；必要水手 `+20/间` | 炮手；最多 4 室 |
| 9 | 物资仓库 | — | 0 | 探索补给量、战利品水粮 `n×20+rand(10)`（`06-ocean/02`、`07-combat/06`） | 收藏水和粮食 |
| 10 | 货物仓库 | — | 0 | 可用货格数（`PackToShip`，见 `01`） | 收藏交易品；最多 5 |
| 11 | 划桨手室 | — | 0 | 必要水手 `+40/间`（`fn_1400053ee`） | |
| 12 | 单人房 | — | 0 | | 船员身心恢复 |
| 13 | 木材室 | 3 测量术 | 130 | | 船木工：航行中修船 |
| 14 | 医务室 | 9 观察力 | 150 | | 船医：治病 |
| 15 | 厨房 | 6 命中力 | 70 | | 大厨：节约水粮、减疲劳 |
| 16 | 娱乐室 | — | 0 | | 恢复精神、减疲劳 |
| 17 | 饲养室 | 0 生命力 | 130 | | 饲养员：节约粮食 |
| 18 | 礼拜室 | 7 说服力 | 160 | | 传教士：消不满；广场散发 |
| 19 | 参谋室 | 8 谋略术 | 120 | | 参谋：酒馆拟定策略 |
| 20 | 会计室 | 4 会计术 | 100 | | 主计长：杀价、屯积 |
| 21 | 追加1 | — | 0 | | 追加船舱（改装件） |
| 22 | 追加2 | — | 0 | | 同上 |
| 23 | 无 | — | 0 | 界外；数据 `0x17` | |

- `[推断]` 文案与 id 的对应：`mesfile_sc.txt` `0xc0b3`–`0xc237` 的说明句顺序正好是 12→20（单人房、木材室、医务室、厨房、娱乐室、饲养室、礼拜室、参谋室、会计室），`0xc503`–`0xc59f` 顺序是 7→10。代码用途列是 `[确认]`。
- `[推断]` `+0x12` u16 = 担任该舱岗位所需的**技能值门槛**：测量 120、会计室 100、船舵 60、桅杆 80 与官方手册的测量员 120 / 会计长 100 / 驾驶 60·80 四处吻合（`03-characters/03` 第 268、421 行）。但已知取表调用点只读 `+0x10`，读 `+0x12` 的代码未找到——原版可能根本不强制。重构作 `required_skill` 数据列，`posts.yaml` 的门槛应引用它而不是手册。
- `[确认]` 139 行默认布局（39 船模 + 107 船体，排除海怪全 `0x17` 行）统计：7、8 最多 4 间，10 最多 5 间，与文案上限一致；9 最多 8 间；18 礼拜室在任何默认布局里都不出现。各种类的槽位固定：0 只在槽 0–2，5 只在槽 14，6 只在槽 15，13 只在槽 13，等等。来源：exe 船模表 `+0x16`、船体表 `+0x0F` 直读。
- `[确认]` 「可以追加船舱。但是，追加船舱速度会降低。」来源：`0xc5d7`。额外舱室是改装件，见 `04`。

### 岗位

- `[确认]` `.rdata 0x1405867f0` 是 16 个名称指针，取表函数 `0x1401ecaa0` 以 `0 <= i < 0x10` 判界：0 舰长、1 副官、2 参谋、3 会计长、4 测量员、5 操帆手、6 舵手、7 了望手、8 冲锋队长、9 炮手、10 传教士、11 船医、12 大厨、13 船木工、14 饲养员、15 甲板主管。
- `[确认]` 这就是 `ePost`，`NUM_POST=16`：`fn_140012dd2`（舱→岗，body `0x140069110`）输出 0–15、`fn_140007e19`（岗→舱，body `0x140068f70`）以 `[0,15]` 跳表，与名称表判界一致（`03-characters/03`、`13-algorithms/03` §2.1）。
- `[确认]` 岗位与舱室种类是两套 id（16 vs 23），两张跳表互逆：

| post | 岗位 | 舱室种类 | post | 岗位 | 舱室种类 |
| --- | --- | --- | --- | --- | --- |
| 0 | 舰长 | 5 舰长室 | 8 | 冲锋队长 | 7 水兵室 |
| 1 | 副官 | 6 副官室 | 9 | 炮手 | 8 武装炮台 |
| 2 | 参谋 | 19 参谋室 | 10 | 传教士 | 18 礼拜室 |
| 3 | 会计长 | 20 会计室 | 11 | 船医 | 14 医务室 |
| 4 | 测量员 | 3 测量 | 12 | 大厨 | 15 厨房 |
| 5 | 操帆手 | 0 桅杆 | 13 | 船木工 | 13 木材室 |
| 6 | 舵手 | 4 船舵 | 14 | 饲养员 | 17 饲养室 |
| 7 | 了望手 | 1 了望台 | 15 | 甲板主管 | 2 甲板 |

  物资仓库 9、货物仓库 10、划桨手室 11、单人房 12、娱乐室 16、追加 21/22 无岗位（`fn_140012dd2` 返回 `0x11`；`fn_140007e19` 越界返回 `0x18`）。所以「炮手室最多 4 间」「冲锋队长」说的就是 8 武装炮台 / 7 水兵室。

### 等级门闩

- `[确认]` `fn_1400040bb`（`0x140069270`）RTC `s_abGradeShipCabin`，栈上 **31 个 int**（`rsp+0x30..0xa8`）：

```
0,0,0, 1,1, 0,0,0,0,0,0, 1, 0,0,0,0,0,0, 2,
1,1,1,1,1,1, 2,2,2,2,2,2
```

  逻辑：`0 <= slot < 0x1f` 且 `table[slot] <= grade`（有符号 `jg`）返回 1，否则 0。
- `[确认]` 该函数**没有调用者**：全 `.text` 扫 `E8/E9 rel32`，只有自身 thunk 指向它；也没有 8 字节指针引用。是死代码。第二参的含义因此无法从调用方确认 `[未知]`。
- `[推断]` 若启用，第二参最像船模体型档 0/1/2（`CShipType+0x18`）。

### 人事 UI

- `[确认]` `CDeckPersonnelView`：31 个舱室按钮（`+0x170` 起，循环 `< 0x1f`）、5 个船页（`+0x130`，循环 `< 5`）、`+0x278=0xEC`、`+0x27C=0x20`。来源：`CDeckPersonnelView.cpp`。
- `[推断]` `+0x278` = 当前选中人物，`+0x27C` = 当前选中舱室槽，空值 `0x20`（31 槽外）。依据：取消选择时两者同时复位（`CDeckPersonnelView.cpp` L456–457），确认前比 `fn_14000a911(ship, +0x27C) == +0x278`。
- `[确认]` `CCabinBestPersonView`：为某个舱室挑人的 UI。已还原的方法只有构造（大量布局坐标）、析构与 `vfunc_02`，**没有**评分代码。来源：`CCabinBestPersonView.cpp`。
- `[确认]` `CDeckNoPostButton`：无岗位 / 未配置。来源：类名。
- `[确认]` `CDeckCabinButton`。来源：`game/ship/`。
- `[确认]` 未配置水手确认：「%s名水手未配置，可以吗？」来源：`mesfile_sc.txt` `0x86b`。
- `[确认]` 函数 `0x14009a130` RTC：`nNumPerson`、`aePerson`、`eShipCabin`。来源：`functions.tsv`。
- `[确认]` `0x140155190` RTC：两份 `aeCabinKind` 各 940 字节、两份 `aePerson` 各 940 字节。`940/4=235=NUM_PERSON`。来源：`rtc.json`。这是按人物扫舱室，还是 5 船展开，未钉死。

### `CPlayerFleetData` 绑定块

相对对象（`vfunc_05/06/07`）：

| 偏移 | 内容 | 等级 |
| --- | --- | --- |
| `+0x08` | 位域：bit0 半开帆、bit1 锁舵（`fn_14000f26d`，`06-ocean/01`） | `[推断]` |
| `+0x09` | 1 字节 | `[未知]` |
| `+0x0A` 起 31 字节 | 每槽 `0xEC`（经 `fn_1401dfb80`，与下面人物字节同一转换） | `[推断]` 旗舰 31 槽的人物 |
| `+0x29=0x0B` | 港 id：大地图 BGM 按它取 `major_sea`（`fn_1400083e6`，`01-world`）；当前 / 最近港 `[推断]` | `[推断]` |
| `+0x30=0x1B` | happening 结局态（27 = 空闲，24 = 无风；`06-ocean/05`） | `[推断]` 强 |
| `+0x2C=-1` `+0x34=0` | | `[未知]` |
| `+0x38` 起 `4×31×2` 字节 | 步长 `0x3E`/船、2 字节/槽。字节 0 = `0xEC`（`fn_1401dfb80`）；字节 1 = `0x18`（`fn_1401dc810`） | `[推断]` `person_id` + `cabin_kind`（`0x18` 正是「非舱室」值） |
| `+0x130` | dword | `[未知]` |

`4×31×2=248=0xF8`，`0x38+0xF8=0x130`，与下一字段对齐 `[确认 尺寸]`。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空 | 重构 key |
| --- | --- | --- | --- | --- |
| 舱室槽 | `eShipCabin` | 31 | `INVALID_SHIPCABIN=-1` | `cabin_id` |
| 舱室种类 | `eCabinKind` | 23 `[推断]`（表 24 行含「无」） | 实例内 `0x18`；数据 `0x17` | `cabin_kind` key |
| 岗位 | `ePost` | 16 `[确认]` | `INVALID_POST=-1`；无岗位 `0x11` | `post` key（`03-characters`） |
| 人物 | `ePerson` | 235 | `-1`；绑定空值 `0xEC` | `person_id` |
| 绑定 | — | 每船 31 | 人物 `null` | `cabin_binding` |

主键一律命名空间 key（`vanilla:gun_deck`），`vanilla_index` 只供导入；`0x17/0x18/0x11/0xEC` 只在 `ids/legacy_codec.yaml`，运行时空值是 `null`（D1）。

重构绑定主键：`(ship_instance_id, cabin_id)`。不要用 `CShip*` 当键。

同一个人不能同时占两个槽。换船 / 换旗舰先解绑再绑。

## 4. 算法 / 公式

```
# 槽是否对这艘船开放
# 原版 s_abGradeShipCabin 是死代码；默认关闭，模组可开
cabin_unlocked(size_class, cabin_id) =
    not cabins.grade_gate_enabled
    or size_class >= cabins.grade_required[cabin_id]

# 种类数量上限（文案；参数化，按 kind 数据驱动）
count(kind == k) <= cabins[k].max_per_ship               # 水兵室 4、武装炮台 4、货物仓库 5

# 按 role 计数是对外的唯一入口（货格、炮门、水手式、补给、战斗都只调它；D2）
Ships.cabin_count(ship, role) = |{ slot : layout[slot] != null and cabins[layout[slot]].role == role }|
# 原版 fn_140006b95(ship, kind)：kind 7/8/9/10/11 = role marine_room/gun_room/supply_room/cargo_room/rower_room

# 「非舱室槽」运行时是 null；原版数据 0x17 与运行时 0x18 只在 legacy_codec 互转

# 担任者看哪项技能（原版 +0x10；0xb = 无）
skill_of(kind) = cabins[kind].skill                      # skill key 或 null

# 岗位 = 舱室种类经 posts.yaml.room 反查（原版 fn_140012dd2 / fn_140007e19）
post_of(kind)  = posts.find(room == kind)                # 无 → null（原版 0x11）

# 换旗舰
on FlagshipChanged:
    for each cabin of that fleet:
        binding.person = null

# 最佳人选（UI；评分公式未还原）
best_person(cabin_kind) = argmax(persons, score(person, cabin_kind))
# score 原版未抽出。default 内容包可用 skill_of(kind) 的技能值排序（[推断]），
# 但不得把未证实的权重写成 [确认]
```

舱室效果分两类入口，其它系统只用这两个查询，不比种类 id（D3）：

| 入口 | 例 | 订阅方 | 原版依据 |
| --- | --- | --- | --- |
| `Ships.cabin_count(ship, role)` | `gun_room` | combat（倍率 / 装填 / 着火）、`08` 炮价与门数、`06` 必要水手 +20/间 | `fn_140006b95(ship,8)` |
| | `marine_room` | `06` 水手上限 +60/间 | `fn_140006b95(ship,7)` |
| | `supply_room` | ocean 探索补给、战利品水粮 `n×20+rand(10)` | `fn_140006b95(ship,9)` |
| | `cargo_room` | `01` 可用货格（≤5） | `fn_140006b95(ship,10)` |
| | `rower_room` | `06` 必要水手 +40/间、ocean 划桨 | `fn_140006b95(ship,0xb)` |
| | `recreation_room` | ocean 疲劳 / 精神恢复（`06-ocean/01`） | 种类 16 娱乐室 |
| `Fleets.post_holder(fleet, post)` | `surgeon` 船医、`cook` 大厨、`keeper` 饲养员、`carpenter` 船木工、`missionary` 传教士、`purser` 会计长、`boarding_chief` 冲锋队长、`gunner` 炮手、`lookout` 了望手、`captain` 舰长 | characters 日结、ocean 补给 / 探索、economy 杀价、combat | 原版按岗位取人 `fn_14000f2d6(fleet, post)` |

「有没有某岗」= `post_holder != null`，不再单设 `has_cook` 之类布尔。数值幅度归各订阅方的公式（如补给 `F-ADJ-03`、战斗 `07-combat`），本篇不写。

## 5. 耦合分析

- 船体定义自带 31 默认种类；实例再被改装改写（`04` 的追加船舱）。
- 人物系统只回答「这个 `person_id` 能不能当这个 `post_id`」（技能 / 装备限制）。绑定表不存六维。
- 换旗舰事件来自 `02-fleet-proxy.md`。
- 原版可用货格数**就是**种类 10 的间数（`PackToShip` 调 `fn_140006b95(ship, 10)`）。货格记录仍属 `01`，但「几格可用」由本系统的布局决定。重构只暴露 `Ships.cabin_count(ship_id, role_key)`，`01` 查询它。
- 炮门（`08`）、水手上下限（`06`）、换炮价（`08`）、战斗倍率 / 装填 / 着火（`07-combat`）、补给（`06-ocean`）也都直接数种类 7 / 8 / 9 / 10 / 11。种类 id 是这些系统之间的隐性共享词汇，必须进 `cabins.yaml` 的 `role`，各系统只调 `Ships.cabin_count(ship, role)`，不能散落成裸 `7`、`8`、`9`、`10`、`11`。
- 存档人物段就是 `aePerson[235]`：从 `0x2C03` 起 235 条，id < 87 每条 25 B，其余 2 B（`12-ui-save/02-save-session.md`、`04-id-registry.md`）。「`0x2C00` 起 98 行 × `0x19`」是 dk4save 工具常数，不是引擎结构。能进舱室的是在队人物，规则由人物系统判定。

## 6. 解耦后的配置

```yaml
# config/caps.yaml
cabins_per_ship: 31
# 空人物运行时为 null；原版 0xEC 只在 legacy_codec（person.empty）
```

```yaml
# config/ships/cabins.yaml
schema: dk4.ships.cabins
version: 1
grade_gate_enabled: false   # 原版 s_abGradeShipCabin 无调用者
grade_required: [0,0,0,1,1,0,0,0,0,0,0,1,0,0,0,0,0,0,2,1,1,1,1,1,1,2,2,2,2,2,2]   # 按 cabin 槽 0–30
# key 是主键；vanilla_index 只供导入。role 是其它系统计数用的稳定 key（Ships.cabin_count）。
# skill / required_skill = exe 表 +0x10 / +0x12（+0x12 语义 [推断]）。crew_need / crew_capacity 见 06。
# copy_group：同规格复制船天数的舱室分组（05；原版按种类区间 [7,12) / [12,21) 判）。
kinds:
  - { key: vanilla:mast, vanilla_index: 0,  name: 桅杆,     skill: vanilla:helmsmanship, required_skill: 80 }
  - { key: vanilla:crows_nest,   vanilla_index: 1,  name: 了望台,   skill: vanilla:observation, required_skill: 70 }
  - { key: vanilla:deck, vanilla_index: 2,  name: 甲板,     skill: vanilla:vitality, required_skill: 30 }
  - { key: vanilla:survey,       vanilla_index: 3,  name: 测量,     skill: vanilla:surveying,   required_skill: 120 }
  - { key: vanilla:helm, vanilla_index: 4,  name: 船舵,     skill: vanilla:helmsmanship, required_skill: 60 }
  - { key: vanilla:captain_room, vanilla_index: 5,  name: 舰长室,   skill: vanilla:leadership,  required_skill: 30 }
  - { key: vanilla:adjutant_room, vanilla_index: 6, name: 副官室,   skill: vanilla:persuasion,  required_skill: 100 }
  - { key: vanilla:marines,      vanilla_index: 7,  name: 水兵室,   skill: vanilla:fencing,     required_skill: 90, role: marine_room, max_per_ship: 4, crew_capacity: 60, copy_group: lo }
  - { key: vanilla:gun_deck,     vanilla_index: 8,  name: 武装炮台, skill: vanilla:gunnery,     required_skill: 90, role: gun_room,    max_per_ship: 4, crew_need: 20, copy_group: lo }
  - { key: vanilla:stores,       vanilla_index: 9,  name: 物资仓库, skill: null, role: supply_room, copy_group: lo }
  - { key: vanilla:cargo,        vanilla_index: 10, name: 货物仓库, skill: null, role: cargo_room,  max_per_ship: 5, copy_group: lo }
  - { key: vanilla:rowers,       vanilla_index: 11, name: 划桨手室, skill: null, role: rower_room,  crew_need: 40, copy_group: lo }
  - { key: vanilla:cabin_single, vanilla_index: 12, name: 单人房,   skill: null, copy_group: hi }
  - { key: vanilla:carpentry,    vanilla_index: 13, name: 木材室,   skill: vanilla:surveying,   required_skill: 130, copy_group: hi }
  - { key: vanilla:infirmary,    vanilla_index: 14, name: 医务室,   skill: vanilla:observation, required_skill: 150, copy_group: hi }
  - { key: vanilla:galley,       vanilla_index: 15, name: 厨房,     skill: vanilla:gunnery,     required_skill: 70, copy_group: hi }
  - { key: vanilla:lounge,       vanilla_index: 16, name: 娱乐室,   skill: null, role: recreation_room, copy_group: hi }
  - { key: vanilla:livestock,    vanilla_index: 17, name: 饲养室,   skill: vanilla:vitality, required_skill: 130, copy_group: hi }
  - { key: vanilla:chapel,       vanilla_index: 18, name: 礼拜室,   skill: vanilla:persuasion,  required_skill: 160, copy_group: hi }
  - { key: vanilla:staff_room,   vanilla_index: 19, name: 参谋室,   skill: vanilla:tactics,     required_skill: 120, copy_group: hi }
  - { key: vanilla:purser_room,  vanilla_index: 20, name: 会计室,   skill: vanilla:accounting,  required_skill: 100, copy_group: hi }
  - { key: vanilla:extra_1,      vanilla_index: 21, name: 追加1,    skill: null }
  - { key: vanilla:extra_2,      vanilla_index: 22, name: 追加2,    skill: null }
# 原版第 23 行「无」不进表：运行时空槽 = null。0x17 / 0x18 在 ids/legacy_codec.yaml（cabin_kind）。
# 岗位不写在这里：posts.yaml（03-characters 拥有）每行 room: <cabin key>，加载器反查生成 kind → post。
# 原版 vanilla 映射（fn_140007e19）：captain→captain_room, adjutant→adjutant_room, staff→staff_room, purser→purser_room,
#   surveyor→survey, sail_master→mast, helmsman→helm, lookout→crows_nest, boarding_chief→marines, gunner→gun_deck,
#   missionary→chapel, surgeon→infirmary, cook→galley, carpenter→carpentry, keeper→livestock, deck_chief→deck
```

岗位目录 `config/characters/posts.yaml` 归 `03-characters/03`（16 行，key 见上表 post 列：`captain`、`adjutant`、`staff`、`purser`、`surveyor`、`sail_master`、`helmsman`、`lookout`、`boarding_chief`、`gunner`、`missionary`、`surgeon`、`cook`、`carpenter`、`keeper`、`deck_chief`）。本篇只要求它有 `room: <cabin_kind key>` 列，门槛引用本表 `required_skill`。

```yaml
# 运行时状态（存档，不是内容包）
# save/ships/*/cabin_bindings.yaml
- ship_instance_id: 12
  cabin_id: 14               # 槽号 0–30
  cabin_kind: vanilla:captain_room
  person: vanilla:raphael_castro
# 岗位不存：由 cabin_kind 经 posts.yaml.room 反查
```

## 7. 独立旋钮

- 每船舱室槽数（原版 31；改了只影响本系统与甲板 UI）
- `grade_required[31]`
- 各种类 `max_per_ship`
- 种类的 `role`（货格 / 炮门 / 水手式只认 role，不认 id）
- 是否启用等级门闩
- 各种类 `crew_need` / `crew_capacity` / `required_skill`
- 岗位表行数与 `room`（人物系统）

加一种舱室：在 `cabins.yaml` 加行（需要计数就给 `role`，需要担任者就在 `posts.yaml` 加一行 `room` 指向它）。不要去改 `CTrade`。

## 8. 对外接口

事件：

- `CabinBound { ship_instance_id, cabin_id, person_id, post? }`
- `CabinUnbound { ship_instance_id, cabin_id, person_id }`
- `CabinsCleared { fleet_id }`          # 换旗舰之后
- `CabinKindChanged { ship_instance_id, cabin_id, cabin_kind? }`  # 改装追加船舱；Ships 据此重算派生量（D6）

查询：

- `Ships.cabin_count(ship_instance_id, role) -> int`  （一级查询，D2；`marine_room` / `gun_room` / `supply_room` / `cargo_room` / `rower_room` / `recreation_room`；原版 `fn_140006b95`）
- `Cabins.layout(ship_instance_id) -> [{ cabin_id, cabin_kind?, unlocked }]`
- `Cabins.binding(ship_instance_id, cabin_id) -> { person?, post? }`
- `Cabins.personShip(person_id) -> { ship_instance_id, cabin_id, post? } | none`
- `Cabins.occupants(fleet_id) -> [{ person_id, ship_instance_id, cabin_kind }]`  （人物系统据此解析 cabin_kind → post → skill）
- `Cabins.bestPerson(cabin_kind) -> person_id | none`

岗位持有人 `Fleets.post_holder(fleet_id, post_key)` 由 `02` 提供，内部用 `posts.yaml.room` + 本表绑定解析。

命令：

- `Cabins.assign(ship_instance_id, cabin_id, person_id)`
- `Cabins.unassign(ship_instance_id, cabin_id)`
- `Cabins.unassignPerson(person_id)`
- `Cabins.setKind(ship_instance_id, cabin_id, cabin_kind?)`  # 仅改装系统调用

订阅：`FlagshipChanged` → 对该队 `CabinsCleared`。

## 9. 未核实字段

- `NUM_CABINKIND=23` 只有判界 / 循环证据，没有头文件
- 种类表 `+0x12` u16：值与手册岗位门槛吻合，但读它的代码未找到
- `s_abGradeShipCabin` 第二参（无调用者）
- `CPlayerFleetData`：`+0x0A` 31 字节是否就是旗舰人物；4×31 记录对应哪 4 艘（舰队 2–5 号船？）
- `+0x09` `+0x2C` `+0x34` `+0x130`；`+0x29` 是当前港还是最近港
- `best_person` 的评分公式
- 单人房 / 娱乐室的身心效果（在 `03-characters/07` 日结里查，本篇不写）
- `aeCabinKind` / `aePerson` 940 字节数组的第二维（235 人 vs 5×47）
- 礼拜室（18）不在任何默认布局里，如何获得
