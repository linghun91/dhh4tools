# 无名水手人数（headcount）

船上有两种「水手」，不要混：

| | 无名人数 | 具名航海士 |
| --- | --- | --- |
| 是什么 | 每船一个整数 | `person_id`（玩家势力隶属人物；重构用显式字段。旧稿「名册 0–97」来自 dk4save 98 行常数，不是引擎区间） |
| 存哪 | 船实例字段 | 人物名册 + 舱室绑定 |
| 雇/解雇 | 城镇设施改这个整数 | 酒馆招人、名册隶属 |
| 出海 | 每船至少 1 人才能开 | 岗位效果另算；名册空也能开 |
| 战斗 | 当命中点（HP） | 提督统率力、冲锋队长剑术等技能值（经 `post_holder` 取人） |
| 文档 | **本篇** | `03-characters/04-recruitment-crew.md` |

`CCrewView`（`0x1400b5080`，`game/person/`）`[推断]` 是**人数**编制 UI：紧挨它的 `fn_140008bcf` / `fn_1400048db` / `fn_140013df4`（`0x1400b93e0`–`0x1400b9d76`）把一个舰队总人数池按各船 `crew_min` / `crew_max` 分到 5 艘船。`%s名水手未配置，可以吗？剩余%s名水手，可以吗？`（mesfile `0x86b`）因此更像人数池分不完的余数，**不是**具名者没进舱室。旧稿说法降级。

`crew_min`（必要）/ `crew_max`（上限）**不是**船体表字段，而是按当前船的体型档、桅、改装件、舱室**现算**的（§2.4，`[确认]`）。两者是 D6 派生量：只由 Ships 计算，放进 `Ships.derived()` 快照（`crew_required` / `crew_capacity`），Ocean 的航速人手修正、出港检查、雇佣 UI 都读同一个值，不存档。本篇管**必要 / 上限怎么算、当前人数怎么变、什么时候不许出海、战斗/探索怎么扣**。

## 1. 职责边界

本系统管：

- 每艘船实例的无名 `crew_count`（USHORT 语义）
- 必要 / 容量：按体型档、桅、改装件、舱室现算 `crew_min` / `crew_max`，再夹到 `SHIP_CREW_*`
- 城镇雇佣 / 解雇（一次付金币）
- 出海硬门闩：任一在编船 `crew_count < sail_min`
- 把战斗、探索、哗变对人数的**写入**收成同一条命令
- 战斗里人数当 HP 的**数据口**（溃败阈值本身属战斗公式）

不管：

| 事项 | 归属 |
| --- | --- |
| 人物名册、招具名航海士、岗位任命 | `03-characters/04-recruitment-crew.md` |
| `CShip` 字节布局、货格、空船哨兵 | `01-ship-model.md` |
| 水兵室槽位、最多 4 间、换旗舰解任 | `03-cabins-crew-posts.md`（本篇只读 `Ships.cabin_count(ship, marine_room)` 与各行 `crew_*` 贡献） |
| 旗舰耐久/水手 ≤30 判溃、炮弹溅射骰 | `07-combat` / `formulas/combat.yaml` |
| 探索逃走、补给按人数消耗、疲劳计量 | `06-ocean` 发事件；疲劳上限属人物 `FATIGUE_*` |
| 旅馆房价、码头补给比例 UI | `08-town-life` |
| 地方舰队账本收支 | `02-fleet-proxy.md` |
| 文案字符串 | `ui/strings` |

城镇只发 `Crew.hire` / `Crew.dismiss`。本系统不 new `CInn` / `CPort`，不读六维。

## 2. 原版事实

### 2.1 人数是船字段，不是人物

- `[确认]` `SHIP_CREW_MIN` / `SHIP_CREW_MAX` 约束在 `USHORT`，断言 `MAX <= _GetDataSizeMax_BYTE2DWORD(sizeof(USHORT))`，即硬顶 65535。十进制原版值**没有**写进头文件。来源：`ShipBase.h`、`catalog/asserts.txt`、`catalog/constants.json`、`13-algorithms/02-caps-ranges.md`。
- `[确认]` `CShip::vfunc_10` 读 `+0x4A`，`vfunc_14` 写 `+0x4A`；`vfunc_11` 读 `+0x4C`，`vfunc_16` 写 `+0x4C`。来源：`game/ship/CShip.cpp`。
- `[推断]` 强：`+0x4A` = 当前耐久，`+0x4C` = 当前水手（元素相对；主 vtbl `+0x50` / `+0x58` 读，`+0x70` / `+0x80` 写，写入只是截成 u16，**没有夹取**）。依据：海战 `fn_14000d7ab` 对 `vtbl+0x50` / `+0x58` 各比 `0x1e`；战斗槽初始化 `fn_140011603` 把两者存为 `hull` / `crew`（`07-combat/01`）；航速以 `+0x4C` 对必要水手（`06-ocean/01`）。字段名原版未恢复；本篇运行时只用 `durability` / `crew_count` 两个标量，不绑死偏移。
- `[确认]` 模板槽初始化（`CShip::vfunc_05` `0x1402adee0`，`this = 元素+8`）：
  - `this+0x42`（元素 `+0x4A`）= `fn_1400084db(ship)` = 船模基础耐久 `+0x0A` + 装甲 + 船首像加成（`0x1402ae127..0x1402ae13f`，`01`）。旧稿「直接拷船模 `+0x0A`」不对
  - `this+0x44`（元素 `+0x4C`）= `(u8)((crew_min + crew_max) / 2)`：有符号 `/2` 后 `movzx eax, al` 再写 word，**超过 255 会回绕**（原版缺陷，重构默认不复刻，`quirks.crew_init_u8` 可开）。按 exe 静态表算，6 条模板真的回绕（如 90 阿拉伯三角帆武装船 269→13，被多支 NPC 舰队引用），见 `01`
  - `crew_min = fn_14000a466(ship)`，`crew_max = fn_140012a26(ship)`，见 §2.4
- `[确认]` 海战会话对象 `CBattleShip` 体积 `0x20`，空船 `+0x8 == 0xD0`。`vfunc_16`：写入后若 `+0x14`（水手）`<= 0` 且 `vfunc+0x48 == 0xb` 则写成 1。`vfunc_14`：`+0x10`（耐久）`<= 0` 且 `vfunc+0x48` 为 7 或 `0xb` 则写成 1。`CBattleShip` 的 `vfunc+0x48` 是 `CShipObject::vfunc_09`（`0x14023e7f0`），转读源 `CShip+0x53` **船首像**（无船时 `0x0D`）：7 = 魔鬼像、11 = 提督像 `[推断]` 强（`04`「船首像」）。来源：`game/fleet/CBattleShip.cpp` + vtbl `0x140557200`。

### 2.2 出海：至少 1 人；「不足」另有一句

出港检查 `fn_140014105`（body `0x140172a00`，RTC `eFleetShip`）`[确认]`，对舰队 5 槽逐船：

```
crew = ship.vtbl58()
if crew == 0:            message 0x1d; return 2        # 「有的船没有１名水手…」 拦截
if crew_min(ship) > crew: warn_crew = 1                 # fn_14000a466；jle 跳过 → crew < min 才警告
if ship.vtbl50() <= max_durability(ship) * 7 / 10: warn_repair = 1   # fn_1400084db；有符号 idiv 0xA
...
if warn_crew: message 0x1e                              # 「有的船水手不足…」 只提示，不拦
```

mesfile `0x3eb` 同一串里两句人数、两句修理（和杀价句挤在一起）：

| 文案 | 用法 |
| --- | --- |
| 「有的船没有１名水手。要重新雇用水手或者重新编组。」 | `[确认]` 硬门闩：任一在编船人数 `== 0` |
| 「有的船水手不足。还是出海前重新编组的好。」 | `[确认]` 软警告：人数 `< crew_min` |

来源：上述反汇编、`srcdata/catalog/mesfile_sc.txt` `0x3eb`。这是 **headcount**，不是名册空。04 篇已写：名册空但 `crew_count > 0` 仍能开。修理提示阈值 `7/10` 属修理系统（`04`），此处只记出处。

解雇：

- `[确认]` 「如果解雇全体水手，船就动不了！」（`0x1323f`）及三句变体（`0x1326f` `0x132ef`）。来源：mesfile。与出海硬门闩同一条规则：不能把在用船收到 0。

### 2.3 雇佣：一次金币，有「必要」和「剩余房间」两条提示

- `[确认]` `CPort` / `CInn` 的还原 cpp 只有 ctor / dtor，对象 `0x10`，菜单逻辑在 unassigned。来源：`game/town/CPort.cpp` `CInn.cpp`、`08-town-life/01-facilities.md`。
- `[确认]` 设施 NPC：码头的小伙子 `person_id` 129–135（七文化圈），旅馆的老板娘 156–162。来源：`15-catalogs/persons.md`。
- `[确认]` 雇佣要钱：「提督，钱不够呀！要用%s金币募集水手。」及「要花%s金币募集水手。」变体。来源：mesfile `0x8c8b`–`0x8d23`。
- `[确认]` 雇完循环问：「必要水手人数还差%s人。还要募集吗？」「有%s人的剩余房间。继续募集吗？」「好像还可以乘坐%s名水手」。来源：`0x9fcf`–`0xa1c3`。
- `[确认]` 雇满失败：「船坐不下了。不能再雇用了。」（`0xa87b`）。同一带还有「不能再雇用」和买卖句，**不能**单凭邻句断定菜单在酒馆还是码头。04 篇已警告。
- `[确认]` 「募集了%s人。」（`0x8dfb`）与「有人愿意乘坐提督的船吗！」挤在同一段，后面马上是计略。人数雇佣 vs 具名上船要靠命令拆开，不能靠这段 mesfile 切设施。
- `[未知]` 雇佣菜单挂在 `CPort`、`CInn` 还是 `CBar`：设施类是空壳，unassigned 未钉到 View。本篇原版包把**人数**雇佣做成码头 + 旅馆可配入口（见 §6）；酒馆默认只招具名者（与 04 的 `recruit_named_officers` 对齐）。证出真正菜单后只改 `hire_sites`，不改命令。
- `[未知]` 每名水手金币公式。文案只有 `%s`。
- `[确认]` 「募集了%s名村民」是探索/村落谢礼（`0xcd4b`），不是本系统。

### 2.4 必要 / 上限的算式（水兵室加容量）

- `[确认]` 「最多可以设４个水兵室。」来源：mesfile `0xc503`；水兵室行 `max_per_ship: 4`（`03` 的 `cabins.yaml`）。
- `[确认]` exe 舱室种类表（`.rdata` `0x14052f1bc` 起 24 个名串，由静态初始化 `0x1400256d0` 按行号建表 `0x144294b40`，`03`）：0 桅杆、1 了望台、2 甲板、3 测量、4 船舵、5 舰长室、6 副官室、**7 水兵室**、**8 武装炮台**、9 物资仓库、10 货物仓库、**11 划桨手室**、12 单人房、13 木材室、14 医务室、15 厨房、16 娱乐室、17 饲养室、18 礼拜室、19 参谋室、20 会计室、21 追加1、22 追加2、23 无。名串行号 = `eCabinKind` 值 `[确认]`（同一张表）；旁证：默认舱室字节 `0x17` 正是「无」，`fn_14000c63f` 的 `[7,12)` / `[12,21)` 两段正好切在水兵室…划桨手室 / 单人房…会计室。
- `[确认]` 公式（反汇编 `0x14023f370`、`0x14023f1f0`、`0x14023f5a0`、`0x14023ed40`、`0x14023f4b0`、`0x14023e9e0`；`size_base` 表 `0x140596d98` = `[3, 5, 10]`，下标是船模体型档 `fn_14000d152`，即 `ships/rig.yaml` 的 `crew_per_mast_by_grade`（`06-ocean` 划桨除数同一张表）；下文 `size_base` 只是公式局部名；`count(kind)` = `fn_140006b95` 扫 31 舱 `vtbl+0x38 == kind`）：

```
mast_need = Σ over mast m in 0..2 with bit0:            # fn_14000feac
              size_base[size] + (bit3 ? 1 : 0) + (bit2 ? 2 : 0)
          + (part 0 ? 1 : 0) + (part 1 ? 4 : 0)
crew_min  = size_base[size]                              # fn_14000a466
          + mast_need
          + 40 * count(kind 11 划桨手室)                 # fn_1400053ee，imul 0x28
          + 20 * count(kind 8 武装炮台)                  # fn_140001e2e，imul 0x14
          + (part 6 ? 2 : 0) + (part 7 ? 2 : 0)
crew_max  = 5 * size_base[size]                          # fn_140012a26，imul 5
          + 60 * count(kind 7 水兵室)                    # fn_140013b92，imul 0x3C
          + crew_min
```

- 所以**每间水兵室 +60 容量** `[确认]`，不加必要人数。武装炮台每门 +20 必要，划桨手室每间 +40 必要。旧稿「每间加多少 `[未知]`」作废。
- 例：战列舰模板（体型 2，桅 `[15,15,5]`，改装位图 66 = part 1、6，舱室含 3 个 kind 8、2 个 kind 7）：`crew_min = 10 + (10+3 + 10+3 + 10+2 + 4) + 0 + 60 + 2 = 114`，`crew_max = 50 + 120 + 114 = 284`，初始 `(114+284)/2 = 199`。
- `[确认]` 炮手室最多 4、缩短炮击间隔——那是战斗间隔。「炮手」岗位（post 9）的舱室就是 kind 8 武装炮台（`fn_140007e19(9) = 8`，`03`），所以「炮手室」= 武装炮台；它同时抬高必要人数。

### 2.5 工资：只见到雇佣金，没见到月俸

- `[确认]` 人数变化付的钱，文案叫「募集水手」金币，是雇佣时一次付清。
- `[确认]` mesfile 没有「月俸 / 工资 / 水手费」这类句。给水手道具是「给了水手们%s」（`0x130f3`），不是发薪。
- `[确认]` 地方舰队有账本支出/收入（`02-fleet-proxy.md`），是否含水手月薪 `[未知]`。
- 原版包：`monthly_gold_per_crew = 0`，并标 `unverified`。模组要月薪就改这一项，不要把账本支出猜成水手工资写进代码。

### 2.6 战斗：人数当 HP；旗舰 ≤30 溃败

- `[确认]` `fn_14000d7ab`（`0x140123d30`）对两侧旗舰：`vfunc+0x50 <= 0x1e` **或** `vfunc+0x58 <= 0x1e` 则该侧 `s2B8 = eSide`，近战循环结束。`0x1e = 30`。来源：`fn_1401200a0.cpp` 约 L1842–L1858；已进 `formulas/combat.yaml` `naval.defeat_hull_or_crew: 30`。
- `[确认]` 炮弹打水手：每发命中 `loss = min(rand(6), crew - 1)`，炮击永远打不死最后一人。来源：`07-combat/01-naval-battle.md` §2.9。
- `[确认]` 单挑败方损失：`crew * (500 - Skill(败方提督, 统率力)) / 500`（`fn_14000e1ce` 的 1 是 `eSkill` 统率力，技能值已夹 `[0,500]`）。来源：`07-combat/02-boarding.md`。本篇只提供 `crew_count`，不读六维。
- `[确认]` 人数打光的文案：「%s号连一个水手也没有了！」（`0x37e7`）、「杀死了全部水手，俘获了敌军旗舰」（`0xea5b`–`0xecfb`）、「%s的水手全没了 / １个水兵都没有」（`0x1421f`）。来源：mesfile。
- `[确认]` RTC `nAddCrew`（`fn_14000d99f`，`0x1400a9070`）：`nAddCrew = crew - crew_max`；若 > 0：本船 `vtbl+0x80` 写成 `crew_max`，再按 `fn_1400a00f0`（`room = crew_max - crew`，>0 才倒）先倒进本队其它 4 槽，再倒进实例槽 `0..0x63` 中有效且 `fn_14000175d(ship) == fn_14000c144(fleet)` 的船（`[推断]` 同港船坞船）。倒完仍有剩余就**丢弃**（函数直接返回）。来源：`fn_1400a1140.cpp`。这是改装后超编分流，不是城镇雇佣 UI。
- `[确认]` 卖船也倒人：`fn_1401767e0`（卖船收尾）取被卖船 `crew`，`> 0` 时 `fn_140178240` 先倒本队 5 槽、再倒 `aeDockShip` 列表，余数丢弃；水与粮食走同形的 `fn_140178460`。来源：`fn_14016cc40.cpp` L3120–3160、`fn_140177480.cpp`。
- `[确认]` RTC `iCaptureCrew` 出现在战后 `fn_14000ec6e`。来源：`fn_1401b9940.cpp`。有没有把俘虏加进玩家 `crew_count` 或人物名册，接舷篇标 `[未知]`。本篇接口留 `CrewAbsorbed`，默认 no-op。

### 2.7 探索与疲劳：逃走扣人数；疲劳本身不是人数

- `[确认]` 探索落空会逃走：「有部分水手逃走了」「水手人数不够了！有人逃走了」「水手减少了%s人」。来源：mesfile `0x1063b`–`0x10843`、`0x34db`。人数公式 `[未知]`（ocean 02 已列）。
- `[确认]` 当地人/野兽袭击文案与逃走同一段，ocean 把 `locals_attack` / `beasts` 标成水手伤亡。减多少 `[未知]`。
- `[确认]` `FATIGUE_MIN/MAX` 是 `UCHAR`。文案「疲劳度上升了%s / 恢复了%s」（`0x343f`）、「水手们好像很疲劳，不让他们休息一下吗？」（`0x81cb`）。来源：asserts、mesfile、ocean 01。
- `[确认]` 大厨「水手也减轻疲劳」；娱乐/恢复类舱「水手也能减少疲劳」；传教士消**不满**（哗变），不是直接改人数。来源：`0xc187` `0xc1fb`。
- `[确认]` 无冲锋队长不能武力镇压水手。来源：`0x5133` `0x51c3`。哗变后果是否减 `crew_count` `[未知]`。
- 本篇**不**把疲劳条每 tick 扣人数。扣人数只走命令 `Crew.lose(ship_id, n, reason)`（逃走/伤亡/炮击/接舷），本系统改完发 `CrewChanged { reason }`。疲劳是否另掷一次逃走，公式未还原，YAML 里留关闭的可选阈。

## 3. 实体与 ID

| 实体 | 原版 | 计数 / 宽 | 空 | 重构 key |
| --- | --- | --- | --- | --- |
| 当前无名人数 | `SHIP_CREW_*` 夹住的 USHORT | 每 `ship_instance_id` 一个 | 0 = 没人（在用船不允许） | `crew_count` |
| 必要 / 上限 | `fn_14000a466` / `fn_140012a26` 现算 | 每 `ship_instance_id` | — | `Crew.min()` / `Crew.max()`（派生量，不存档） |
| 水兵室 | `eCabinKind` 7 水兵室 | 每船 0–4 | 0 间 | `Ships.cabin_count(ship, marine_room)` |
| 雇佣会话 | 无枚举 | 运行时 | — | `crew_hire_session` |
| 出海人数门闩 | 文案「１名」 | 每船 | — | `sail_min_crew` |
| 战斗溃败阈 | 立即数 `0x1e` | 旗舰 | — | `combat.defeat_hull_or_crew`（战斗拥有） |
| 具名航海士 | `ePerson`（0–234） | 显式「隶属玩家」集合 | `null`（不要用 0，0 是拉斐尔） | **不是**本系统 |

重构约定：

- 系统之间只传 `ship_instance_id` + 整数。禁止传 `CShip*` / `CBattleShip*`。
- `crew_count == 0` 合法于未编入舰队的船坞船；**在编船**必须 `>= sail_min_crew`。
- 不要用 `person_id` 表示无名水手。不要用名册槽数当容量。

## 4. 算法 / 公式

全部参数进 `config/ships/crew.yaml` 与已有的 `caps.yaml` / `formulas/combat.yaml`。代码里不准写裸 `1`、`30`、`4`。

### 4.1 必要、容量与夹取

把原版硬编码的加项做成**按来源的贡献表**，模组加新舱 / 新改装件只加行：

```
size_base   = rig.crew_per_mast_by_grade[model.size_class]              # [3, 5, 10]；局部名
mast_need   = Σ_{m present} (crew.mast.per_mast_size_mul * size_base
                             + Σ_{bit in m.flags} crew.mast.bit_need[bit])   # bit2→2, bit3→1
crew_min    = crew.min_size_mul * size_base                              # 1
            + mast_need
            + Σ_{cabin} cabins[cabin.kind].crew_need                     # 03 cabins.yaml：划桨手室 40、武装炮台 20
            + Σ_{part installed} add_parts[part].crew_need               # 04 add_parts.yaml：headsail 1、spanker 4、bow_gun 2、stern_gun 2
crew_max    = crew.max_size_mul * size_base                              # 5
            + Σ_{cabin} cabins[cabin.kind].crew_capacity                 # 水兵室 60
            + crew_min
crew_max    = clamp(crew_max, caps.ship_crew_min, caps.ship_crew_max)

crew_count  = clamp(crew_count, 0, crew_max)                             # 原版写入不夹，见 §2.1
initial_crew(template) = (crew_min + crew_max) // 2                     # 原版再截 u8，重构不截
```

`crew_min` 是「必要水手」（出港警告 `0x1e`、雇佣「还差%s人」的目标），**不是**出海硬门闩。硬门闩只有 `sail_min_crew`。

水兵室不再单独特判：只是 `cabins.yaml` 里 `crew_capacity: 60` 的一行。贡献都挂在各自的内容行上（舱室 / 改装件 / 桅加帆件），模组加一种舱或改装件只加行，不改本公式（D3）。

### 4.2 出海

```
can_sail(fleet_id):
  for slot in Fleets.ships(fleet_id):          # 跳过 empty
    if Ships.crew(slot) < sail_min_crew:       # 原版 1
      return block, reason=no_crew             # 「没有１名水手」
  for slot in Fleets.ships(fleet_id):
    if Ships.crew(slot) < Crew.min(slot):
      warn undercrewed                         # 「水手不足」0x1e；原版不拦
      # undercrewed_blocks_sail: false
  return ok
```

解雇：

```
can_dismiss(ship_instance_id, delta):
  if ship in active fleet
     and crew_count - delta < sail_min_crew:
      refuse                                   # 「解雇全体水手，船就动不了」
  apply
```

船坞里未编入的船可以收到 0。

### 4.3 雇佣

城镇发命令，本系统改人数、向经济要钱。

```
room     = crew_max - crew_count
needed   = max(0, crew_min - crew_count)
delta    = requested                         # UI；0 < delta <= room
gold     = delta * hire_gold_per_crew        # 单价 [未知] → null 则拒绝雇（不要瞎填）

if room <= 0:           refuse sit_full      # 「船坐不下了」
if gold > purse:        refuse cant_pay      # 「钱不够…募集水手」
Force.add_gold(force_id, -gold, crew_hire)   # 先扣钱（factions）
crew_count += delta                          # 再加人数（事务：扣款失败则不加）
emit CrewChanged
if needed - delta > 0:  ui.ask_continue_needed(needed - delta)
else if room - delta > 0: ui.ask_continue_room(room - delta)
```

`hire_gold_per_crew` 未还原前保持 `null`：原版包可以雇（否则港口玩不成），但**禁止**在引擎里写死一个猜的单价。实现：`null` 时读 `hire_gold_per_crew` 失败则用内容包占位键 `unverified: true` 且必须由策划填；代码无默认数字。

雇佣按 `ship_instance_id`。UI 可以按舰队 5 槽循环发 5 次命令。原版另有「舰队人数池 → 各船」的自动分配（`fn_140008bcf`：池 ≥ Σmax 则全满；否则先保各船 `crew_min` 再均分剩余，不够 min 时按缺口分），`[推断]` 属编制 UI。重构把它做成可替换的 `Crew.redistribute(fleet_id, policy)`，策略键 `fill_min_then_even`，不要硬塞进 `hire`。

超编分流（`nAddCrew`，换船/改容量后）：

```
surplus = crew_count - capacity                 # fn_140012a26 = crew_max，§2.4
if surplus > 0:
  crew_count = capacity
  pour into other fleet ships that have room
  then into docked instances at the same port
  leftover discarded                           # 原版直接丢
```

卖船时被卖船的全部人数走同一个 `pour`。

### 4.4 战斗当 HP

本系统不计算伤害。战斗读写快照：

```
snapshot.crew = Ships.crew(ship_instance_id)

# 战斗内（07 篇已写）
crew' = apply_crew_splash(crew, shell, rand(crew_splash_sides))
if flagship.hull <= defeat_hull_or_crew
   or flagship.crew <= defeat_hull_or_crew:
    emit SideBroken

# 战斗结束
Crew.set(id, snapshot.crew, battle_end)          # 夹到 [0, capacity]
```

`defeat_hull_or_crew: 30`、`crew_splash_sides: 6` 只在 `formulas/combat.yaml`。本篇不复制一份。

人数到 0：发 `CrewWiped { ship_instance_id }`。是否立刻沉/俘由战斗订阅。`CBattleShip` 在装提督像（水手 / 耐久）或魔鬼像（耐久）时把 0 抬回 1，是船首像的战斗效果（`04` 的 `combat.crew_floor_at_zero` / `hull_floor_at_zero` 修正器），只作用于海战会话；**不要**在大地图把 0 写成 1。

### 4.5 逃走 / 伤亡 / 疲劳

```
on Crew.lose(ship_id, amount, reason):       # 命令；ocean / combat 发
  crew_count = max(0, crew_count - amount)
  clamp to capacity
  emit CrewChanged { ship_id, crew, reason }
  if in fleet and crew_count < sail_min_crew:
    emit CrewBelowSailMin                    # 不自动雇；下一次出海再拦
```

`amount` 由 ocean / combat 带来。探索逃走公式未还原：ocean 表可先 `amount: null`，实现时 `null` 则只播文案、不改人数（宁可少扣，不要编骰）。

疲劳：

```
# 默认关闭。确认「疲劳过高会减员」之前不要启用。
if fatigue_desertion_threshold != null
   and ship_or_fleet_fatigue >= fatigue_desertion_threshold:
    Crew.lose(ship_id, fatigue_desertion_amount, fatigue_desertion)
```

原版包两项都 `null`。不满/哗变同理：本篇收 `Crew.lose(ship_id, n, mutiny)`，不自己扫心情。

补给消耗用人数：`need = crew_count * drain_per_crew_per_day * cook_saving_factor`（ocean 01）。本篇只被查询，不算补给。

## 5. 耦合分析

原版硬编码：

- 出海检查、修理检查、杀价句挤在同一 mesfile 段。
- `CPort` / `CInn` / `CBar` 都是空壳，真正雇佣可能在广场点击 unassigned 函数里。
- 海战直接 `vfunc+0x58` 当 HP，接舷再乘 `(500-a)`。
- 水兵室种类在船体 31 槽里，人数在另一个 USHORT，两套数据靠战斗/容量逻辑隐式相加。
- `nAddCrew` 扫 5 艘再扫 100 个实例槽，把人数和船体池搅在一起。
- 必要 / 上限由舱室种类、改装位、桅位、体型档硬编码相加（`[3,5,10]`、20、40、60、1/2/4）。
- 04 篇曾把 `bar.hire_crew_count: true` 写成人物侧配置——那是「酒馆会不会发雇人命令」的推断，容易和招航海士搅在一起。

重构切断：

```
Town(inn|port).HireCrew  → Crew.hire(ship_id, n, reason)
Town(bar).RecruitOfficer → Characters.PersonRecruited     # 04
Ocean.SearchDesertion    → Crew.lose(ship_id, n, search_desertion)
Combat.splash / melee    → 会话内改快照；结束时 Crew.set
Ships.cabin_count / cabins.yaml crew_* → 只读，进容量
Force.add_gold           → 雇佣金（factions）
Ocean.supply             → 查询 crew_count
```

禁止：人物名册增减去改 `crew_count`；战斗写 `hulls.yaml`；本系统读六维。

`03-characters/04` 的雇水手命令改由本系统 `Crew.hire` 接收（人物文档里那行「转发给船」落到这里）。酒馆 YAML 里的 `hire_crew_count` 当作设施旋钮的**旧位置**；人数雇佣的唯一配置是 `crew.yaml` 的 `hire_sites`。

## 6. 解耦后的配置

```yaml
# config/caps.yaml（摘本系统；与 01 / 03 共用键，不另造一套）
ship_crew_min: SHIP_CREW_MIN          # 抽出后替换；USHORT 硬顶见 engine_limits
ship_crew_max: SHIP_CREW_MAX
```

```yaml
# config/engine_limits.yaml
integer:
  ushort: 65535                       # SHIP_CREW_* 不得超过
```

```yaml
# config/ships/crew.yaml
schema: dk4.ships.crew
version: 1

# 与具名航海士的分界。引擎按这个开关拒绝对的命令。
named_officers: false                 # 本表不管 person_id
headcount: true

sail:
  min_per_active_ship: 1              # 硬门闩
  undercrewed_is_crew_min: true       # 「水手不足」= 低于 crew_min（0x1e）
  undercrewed_blocks_sail: false      # 原版像劝告

capacity:                           # fn_14000a466 / fn_140012a26，§2.4 [确认]
  # 体型基数直接读 ships/rig.yaml 的 crew_per_mast_by_grade（[3, 5, 10]，表 0x140596d98，按 models.size_class）；本文件不另设 size_base 键
  min_size_mul: 1
  max_size_mul: 5
  mast:
    per_mast_size_mul: 1              # 每根存在的桅 + size_base
    addon_need: $ships.rig.mast_addon_crew   # square_addon 2、fore_addon 1
  # 舱室贡献在 03 cabins.yaml 的 crew_need / crew_capacity；改装件贡献在 04 add_parts.yaml 的 crew_need
  # 水兵室间数上限在 03 cabins.yaml 的 max_per_ship
  initial_from_template: midpoint     # (min+max)/2；原版截 u8，不复刻
  leftover_after_pour: discard        # 原版 nAddCrew / 卖船倒不完直接丢

hire:
  # 原版菜单在哪个设施 [未知]。原版包先开这两个。
  sites:
    - facility: vanilla:inn
      key: inn
      npc_job: innkeeper              # person 156–162
    - facility: vanilla:dock
      key: port
      npc_job: dock_boy               # person 129–135
  bar_hires_headcount: false          # 酒馆默认只招具名
  gold_per_crew: null                 # 文案 %s；禁止代码默认数字
  gold_per_crew_unverified: true
  loop_prompt: true                   # 「继续募集吗？」
  refuse_if_no_room: true             # 「船坐不下了」
  allow_over_crew_min: true           # 可以雇到 crew_max，不必停在 crew_min

wages:
  monthly_gold_per_crew: 0            # 未见到月俸
  monthly_unverified: true
  hire_is_one_shot: true

combat:
  # 只声明人数是 HP；阈值不在本文件。
  acts_as_hit_points: true
  defeat_param: formulas.combat.naval.defeat_hull_or_crew
  write_back_on_battle_resolved: true
  do_not_clamp_zero_on_ocean: true    # 大地图允许 0（未编入）

loss:
  reasons:
    - cannon_splash
    - boarding
    - search_desertion
    - locals_attack
    - beasts
    - mutiny
    - capture_absorb
  search_amount: null                 # 「减少了%s人」公式未还原
  fatigue_desertion_threshold: null   # 不启用
  fatigue_desertion_amount: null

pour:                                 # fn_14000d99f 超编分流
  enabled: true
  complement_source: crew_max         # 原版 fn_140012a26
```

```yaml
# config/ships/hulls.yaml 不再带 crew_min / crew_max：两者由 capacity 表按当前配置现算。
# 本系统只读 models.yaml 的 size_class、实例的桅 / 改装件 / 舱室。
```

```yaml
# config/town/facilities.yaml（城镇拥有；本系统声明要哪些命令）
# inn.commands 增加 hire_crew
# port.commands 增加 hire_crew、sail_check
# bar.commands 保持 recruit（具名），不加 hire_crew
```

```yaml
# 存档状态（不是内容包）
# save/ships/*/crew.yaml
- ship_instance_id: 12
  crew_count: 80
```

World0 / World1 共用同一份 `crew.yaml`。船体必要/上限随 `hulls.yaml` 走，不按世界拆两套雇佣规则。

## 7. 独立旋钮

改这些不应迫使贸易 / 名册 / 炮击改代码：

- `sail_min_per_active_ship`（原版 1）
- `undercrewed_blocks_sail`
- `hire.sites`（inn / port / 将来加村）
- `gold_per_crew`、`monthly_gold_per_crew`
- `capacity.*` 乘数；各舱室 / 改装件行的 `crew_need` / `crew_capacity`（`03` / `04`）；`rig.yaml` 的按档表
- `fatigue_desertion_threshold`（默认关）
- `pour.enabled`

加一种减员原因：在 `loss.reasons` 加 key，ocean/combat 发 `Crew.lose(ship_id, n, reason)`，本系统发 `CrewChanged { reason }`。不要去改 `CTrade`。

溃败 30 是战斗旋钮，不在本表改。水兵室最多 4 间是舱室旋钮。

## 8. 对外接口

事件：

- `CrewChanged { ship_id, crew, reason }`
- `CrewWiped { ship_instance_id }`
- `CrewBelowSailMin { fleet_id, ship_instance_id }`
- `CrewHireRefused { ship_instance_id, why }`   # `sit_full` / `cant_pay` / `would_immobilize`
- `CrewAbsorbed { from_ship, to_ship, amount }` # 俘虏；默认无订阅者

查询：

- `Crew.get(ship_instance_id) -> int`
- `Crew.capacity(ship_instance_id) -> { min, max }`               # 现算
- `Crew.room(ship_instance_id) -> int`
- `Crew.needed(ship_instance_id) -> int`         # max(0, crew_min - current)
- `Crew.canSail(fleet_id) -> { ok, blockers[], warnings[] }`
- `Fleets.battle_snapshot` 已含水手；战斗不要另开接口绕过本系统

命令：

- `Crew.hire(ship_id, n, reason)`               # 扣金+加人数，同栈
- `Crew.dismiss { ship_instance_id, delta }`
- `Crew.set(ship_id, n, reason)`                # 战斗结束 / 读档 / 事件；仍夹容量
- `Crew.lose(ship_id, n, reason)`               # 逃走 / 伤亡 / 哗变；ocean、combat 调用
- `Crew.pourSurplus { ship_instance_id }`        # 换船后

订阅：

- `BattleResolved` → 按快照 `set`
- `CabinKindChanged` / `AddPartChanged` / `MastChanged` → 重算 min/max；若 `crew_count > crew_max` 则 `pourSurplus`
- `ShipScrapped` → 人数进 `pour` 或清零（YAML `on_scrap: pour`）
- `FlagshipChanged` **不**清 `crew_count`（清的是舱室人物，03 篇）

`HireCrewCount`（04 篇旧名）= `Crew.hire`。只留一个。

## 9. 未核实字段

必须单列，禁止猜成确定语义后写进「已命名」区。

| 项 | 现状 |
| --- | --- |
| `SHIP_CREW_MIN` / `MAX` 十进制 | 只有 USHORT 硬顶 65535 |
| `CShip+0x4A` / `+0x4C` 字段名 | 极像耐久/水手（初始化与溃败比较都对上），名字未恢复 |
| `CCrewView` 人数池分配（`fn_140008bcf` 一组）的完整规则与调用者 | 先补满 max / 先补 min 再均分的分支可见，入口未钉 |
| 雇佣菜单属于 `CPort` / `CInn` / `CBar` | 三类都是空壳 |
| 每名水手金币 | 只有 `%s` |
| 月薪 | 文案未见；账本支出是否包含 `[未知]` |
| 探索逃走 / 袭击减员公式 | 「减少了%s人」 |
| 疲劳是否直接减员 | 只有疲劳条与休息句 |
| 哗变失败是否减员 | 只有镇压要冲锋队长 |
| `iCaptureCrew` 写回点 | 战后 RTC 名 |
| `CBattleShip+0x10/+0x14` 与 `CShip+0x4A/+0x4C` 是否同一对语义 | 战斗会话 vs 大地图实例 |
| 船首像 7 / 11 让海战耐久 / 水手不归 0 | 比较值已确认；「船首像」语义 `[推断]` 强 |
| `%s名水手未配置` / `剩余%s名水手` | `[推断]` 人数池余数（见文首） |

找到雇佣函数之后：填 `gold_per_crew`、把 `hire.sites` 改成 cpp 对上的那一个设施。不要保留「inn 和 port 都开」当 fallback 双轨——原版包在证实后只留真实入口。
