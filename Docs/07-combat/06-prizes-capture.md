# 战后战利品、俘船与俘虏

炮击在 `01-naval-battle.md`，接舷在 `02-boarding.md`，NPC 何时投降在 `03-battle-ai.md`。这一篇只定义**战斗已经结束之后**怎么把金币、货、船体、水粮交出去。炮弹 tick 里不许写货格。

`01` 把 `BattleResolved.loot` 写成一行占位。本篇给出完整载荷。`fn_140006b95(ship, 10)` 是货舱计数，不是金币公式——金币在同一次战后函数里的另一段。

## 1. 职责边界

本系统管：

- 战后结算会话 `CaptureSession`：从参战快照抽出可擒船、货、水粮、金币
- 金币与水粮入账（只发命令，不持金库）
- 货舱分配 UI 的**规则**（25 格、拖船仓库、没装的丢掉）
- 船体去向：拖带 / 入坞 / 丢弃
- `BattleResolved` 的战利品载荷

本系统不管：

- 船态何时变成「可擒」（炮击溃败、接舷投降，见 `01` / `02`）
- 金库、市价、品牌判定（`economy`）
- 货格持久化、船坞容量（`ships/`；坞的内部见计划中的 `05-ships-fleet/07-dock-warehouse.md`）
- 航海士怎么招人（`characters`；本系统最多发 `CrewCaptured`）
- 「战利品 / ★ / %s等级」怎么排版（`ui/`；本篇只规定**何时**用哪条 key）
- 占港（只把 `town_id` 放进载荷，势力自己写）

禁止：combat → `Economy.quote`；combat 直接 `Person.ability +=`；把俘虏写成自动招为航海士。

## 2. 原版事实

### 2.1 战后只有一条入口

`fn_14000ec6e`（`0x1401c4a70`，RTC `cCatchedShipArray` 24 B、`cAfterBattleSituation` 8 B、`pEnemyFleet`、`iCaptureCrew`）是玩家海战结束的总收口。同文件稍后的 `fn_140007b9e` 是短路径，只建 `cAfterBattleSituation` / `cCatchedShipArray`，**不**开货舱窗。

顺序（`s158 != 2` 才收集敌舰队；`== 2` 跳过 `[推断]` 逃走 / 无缴获）：

1. `fn_14000a817`（`0x1401c4e20`）— 水粮、金币
2. `fn_1400069ce` + `fn_14000907a` — 构造 8 字节局面、24 字节擒船数组
3. `fn_140007aea`（`0x14018a160`）— 船体进数组或留着拖
4. 若上一步成功且 `pEnemyFleet != 0`：`fn_140005fb0` 开货舱窗，再 `fn_1400013ca` 清空敌方货
5. 后面可能再走占港（`0x45`、势力 `== 10` 等），本篇不展开

来源：`srcdata/source/game/unassigned/fn_1401b9940.cpp`。`01` 写的「结算阶段用 `CaptureSession`」就是这一串，不是炮弹函数。

### 2.2 可擒船态

`fn_1400106a4(ship)` 的返回值：

| 值 | 出现位置 | 行为 |
| --- | --- | --- |
| 4 | `fn_14000a817`、`fn_140007aea`、`fn_140005fb0` | 可拖 / 可分货。`02` 已标可拖 `[推断]` |
| 5 | `fn_14000a817`、`fn_140007aea` | 直接推进 `cCatchedShipArray`，从敌舰队卸掉 |

不要把 4 写成「投降」、5 写成「白旗」——文案有投降 / 白旗 / 占领 / 擒获多套，船态枚举未恢复。

`eCaputureShip`（原拼写 Capture）循环 `>= 5` 结束，与 `NUM_FLEETSHIP` 同宽 `[确认]` `fn_140005fb0` RTC。

### 2.3 金币不在 `fn_140005fb0`

`fn_140006b95`（`0x14023e4c0`，RTC `eShipCabin`）扫 31 个舱位（循环 `>= 0x1F`），数「`vfunc+0x38(i)` == 第二参」的间数。第二参是**舱室种类**：10 = 货物仓库、9 = 物资仓库（`05-ships-fleet/03` `cabins.yaml`，role `cargo_room` / `supply_room`）。`Constants.h` 的 `NUM_HOLD_SCAN=10` 是把这个参数误读成「扫 10 格」（`13-algorithms` 已标删除）。重构查询 `Ships.cabin_count(ship, role)`。

因此：

| 调用 | 第二参 | 含义 |
| --- | --- | --- |
| `fn_140005fb0` 里 `6b95(ship, 10)` | 10 | 货舱数，累加到 `s30`，用来决定要不要开分货窗 |
| `fn_14000a817` 里 `6b95(ship, 9)` | 9 | 物资仓库间数，累加到 `s3C`，用来发水粮 |
| `fn_1400034ae` 里 `6b95(tow_ship, 10)` | 10 | 拖船货舱数，作为拷货循环上界 |

金币在 `fn_14000a817` 后半 `[确认]`（原始指令 `0x1401c4fe9`–`0x1401c53fd`）：

```
for ship in enemy 5 slots (vfunc8 有效):
    st = ship.state(+0x2a)
    if st == 4:                                   # 被擒
        n9  += count_cabins(ship, 9)
        gap += fn_1400084db(ship) - hull(ship)    # 单参，最大耐久 − 当前耐久
        *out_crew += crew(ship)                   # 第 3 参输出：俘获水手总数
    if st in (4, 5) and fn_140004a1b(DAT_144296500) == 1:   # eScenario == 1（赫德拉姆剧本）
        s44 += ship.vfunc_20()
gold = gap * 100                                  # imul 0x64
if 敌提督船存在 且 状态 4 且 战斗归属 == 玩家 (fn_1401aba20 == fn_14000e3c2):
    gold += enemy_fleet.money(+0xc) / (fn_140012b75(敌舰队) ? 50 : 20)   # 无符号除
if gold > 0: 消息 0x13c；enemy_fleet.money -= gold（fn_14001117b）；player += gold（fn_140002cb6）
```

`fn_1400084db` 只有一个参数（旧文的 `, 9` 是寄存器残留）。`fn_14001117b` 把数量取负再调同一加减函数 `[确认]`。

文案（`mesfile_sc.txt`）：

- `从擒获的船上得到%s金币`（`0x3363`，同记录还拼了选对手、没装的丢掉）
- `获得了%s枚金币的战利品！` 一族（`0x13bfb`）
- `得到%s枚金币的战利品！`（`0x13ca3`）
- `收缴到战利品：金货%s枚。`（`0x13cbb`，后半句是攻城）

这些是 UI key，不是 `6b95` 的返回值。`fn_14000a817` 实际 `fn_14000fb14` 的类型号是 `0x13c`（船体差那笔）和 `0x141`（`s44 > 10` 的另一笔）。`eMessageType` 下标与 mesfile 偏移不是同一套数 `[确认]` `12-ui-save/03-localization.md`。

`s44`：船态 4 或 5 且 `fn_140004a1b(DAT_144296500)==1` 时累加 `vfunc+0x20(ship)`；`s44 > 10` 则 `s44 /= 10`，消息 `0x141`，玩家 `+= s44` `[确认]`。`fn_140004a1b(DAT_144296500)` = `CDk4Game+0x7c` = **剧本号 eScenario** `[确认]`（`04-economy/07` §2.1、`10-narrative/01`），1 = 赫德拉姆（瑞典海军提督）剧本。`vfunc+0x20` 是船价 `[推断]`，即「赫德拉姆剧本里每擒获 / 降服一船另得船价 1/10 奖金」。按 D5，重构不比较剧本号：这是剧本包旗标 `scenario.flags.prize_ship_value_bounty`，战斗读 `Scenario.flag("prize_ship_value_bounty")`（布尔开关，非修正器；§4.3）。

### 2.4 水粮

`fn_14000a817` 在 `s3C > 0` 时算出 `sA0 = s3C*20 + rand(10)`、`sA4 = s3C*20 + rand(10)` `[确认]`（`imul 0x14`，`fn_140005272` = `rand`），然后：

- `fn_14000fb14(0x13e, sA0, sA4)` 两个数
- `fn_1400044f8(fleet, sA0, sA4, 1)`（`0x1400fed50`，RTC `eFleetShip`）按玩家五船写**补给槽 0 / 1**（`fn_14000f033(ship, 0|1, qty)`；`fn_140013e1c(ship, 0|1)` 读现量）——第二参是补给下标（`NUM_SHIPSUPPLY=2`），**不是**舱室种类。旧稿「写进舱种 9 和舱种 1」作废

文案：`从俘获的船上得到%s桶水，%s桶粮食`（`0x33ab`），参数顺序 sA0、sA4 → 补给 0 = 水、1 = 粮 `[推断，强]`。

`fn_1400044f8` 每船：`n = cabin_count(ship, 物资仓库)`，容量 `n*100`（`imul 0x64`），水份额 `n*per`、粮 `n*100 − n*per`（`per` 来自局部 `+0x20`，`[未知]`）；装得下就写入，否则填满溢到下一艘。这是补给，不是交易品。

### 2.5 `fn_140005fb0`：拖船 + 分货窗

`0x1401e2e70`，699 字节。RTC：`cView` 976 B、`asShipLoad` 200 B、`eCaputureShip`、`apInTowShip` 40 B。

`0x3d0 = 976`，正是 `COrganizeLoadView` 析构尺寸 `[确认]` `COrganizeLoadView.cpp` `fn_14000750e(..., 0x3d0)`。`40 B = 5` 个指针。`200 B = 25 × 8` 字节货记录（5 船 × 5 格）。

每艘敌船：

1. `fn_14000afb5(fleet, slot)` 取船；空则 continue
2. 态 `!= 4` continue
3. `fn_140001bea(ship)` 真则 continue（过滤器，语义 `[未知]`）
4. `s30 += CountCabins(ship, 10)`
5. 若 `fn_140008783(DAT_144296500)` 失败则 continue
6. `fn_1400018e8` 克隆出新实例 `sA0`；失败则 continue
7. 拷船体（`vfunc+0x88`、`fn_140006e92(..., 0x62)`、`fn_14000b7f8`、`vfunc+0x50`）
8. `apInTowShip[s34] = sA0`；`s34++`

`0x62=98` 与 dk4save 常数 `SAVE_SAILOR_COUNT` 同数（原版无 98 名册，见 §2.9），也是空货记录 `+2`、`CShip+0x54` 初值。克隆路径里它是拷贝宽度还是哨兵 `[未知]`，禁止写成「这里把俘虏写入名册」。

`s30 > 0` 才考虑开窗：`fn_140007266(fleet, &asShipLoad)` 把敌舰队货倒进 200 字节栈缓冲。倒出来的件数 `<= 0` 时，若 `fn_14000d599(fleet) != 0x1A` 则不开窗。`0x1A` 在 `CBattleResult::vfunc_04` 里当空结果哨兵，这里是否同一枚举 `[未知]`。

开窗：

```
mode = fn_140008783(DAT_144296500) ? 4 : 3
COrganizeLoadView ctor(mode)          # fn_1401e1440
fn_1400034ae(view, fleet, s30, &apInTowShip)
COrganizeLoadView 第二段 ctor         # fn_1401e26f0
if s34 > 0: fn_14000fb14(0x143)       # 有拖船才报
```

`fn_14000c1e4` 按 `view+0x1c0`（ctor 写入的 mode）选提示：`<=1 → 0x104`，`3 → 0x13d`，`4 → 0x146`。对应文案候选：`没装的东西就不要了，行吗？`、`将没装的货物放到拖船的仓库里，行吗？`（都在 `0x3363` / `0x343f`）。类型号 ↔ 句子的表未恢复。

### 2.6 `COrganizeLoadView` 与 `CCapturedLoadSprite`

| 类 | 尺寸 | 基类 | 作用 |
| --- | --- | --- | --- |
| `COrganizeLoadView` | `0x3d0` | `CView` | 分货会话。ctor 清 5 个船指针 `+0x290`、**25** 个按钮指针 `+0x2b8`、5 个 `+0x380` |
| `CShipLoadButton` | 含 `+0xc8` 一条 8 字节货记录 | `CSurfaceSprite` + `CPushButton` | 一格货。ctor 里 `ClearHoldRecord(+0xc8)` |
| `CCapturedLoadSprite` | `0x108` | `CSurfaceSprite` | 擒获货卡片精灵。ctor 写 `+0xa8=0`；析构 `fn_14000750e(..., 0x108)` |

来源：`ship/COrganizeLoadView.cpp`、`ship/CCapturedLoadSprite.cpp`、`ship/CShipLoadButton.cpp`、`class_hierarchy.txt`、`fields.tsv`。

`fn_140005fb0` **new 的是 `COrganizeLoadView`**，不是 `CCapturedLoadSprite`。后者与前者同带、同功能目录，作为子精灵挂在 View 的 `+0x3a8…+0x3c8` 那五个槽上 `[推断]`（`vfunc_29` 按槽释放）。逻辑层不要依赖精灵类名。

`COrganizeLoadView::vfunc_13` 点击号：

| 点击 id | 范围 | 行为 |
| --- | --- | --- |
| 5–9 | 5 艘己船 | 把该船当前格清掉，货回到按钮 |
| `0xB`–`0x23` | 25 格 | 若选中船不是 6（哨兵）：`fn_140014d80` 写入该船货格，再清按钮 |
| `0x25`–`0x29` | 5 | 写 `+0x1c4` 选中拖船 / 己船页 |

空货 `item == 0x6A`。写入走贸易同一条 `fn_140014d80`（生成 8 字节记录）。来源：`COrganizeLoadView.cpp` `0x1401e9a50`。

`fn_1400034ae`：先给 View 建 `InfoBox2.dk4` 框，再扫最多 `0x19=25` 个 `+0x2b8` 按钮找空位，把每艘拖船的货舱（`CountCabins(10)`）逐格 `fn_1400151e5` 填进去。25 格满了就停。这就是「没装下的货」来源。

### 2.7 敌方货要清掉

`fn_1400013ca`（`0x1400fccd0`）：若 `fn_14000e638(fleet)` 真（玩家势力分支），对 5 船 × 5 格 `ClearHoldRecord`；否则清 `fleet+0x74` 那条压缩货记。来源：`fn_1400f7c10.cpp`。战后必须先分完再清，顺序已经由 `00ec6e` 保证。

`ClearHoldRecord`（`0x1400fce20`）：`item=0x6A`，`+2=0x62`，`+4=0`。与贸易关格相同。

### 2.8 船体：拖带 vs 入坞

`fn_140007aea` 对玩家五船：

- 态 4：`fn_140001e7e(situation)==1` → `fn_14001351b` 推进数组并 `fn_140001979` 从舰队卸掉；否则只打「还要拖」标志
- 态 5：一律推进数组并卸掉
- 水手 `vfunc+0x58 == 0`：与态 4 类似
- 最后把仍留在舰队里的水手人数加进 `iCaptureCrew`

`fn_14001351b`（`0x1401b7b40`）：

```
token = ship->vfunc+0x18() + 0x64     # +100
array[count] = token                  # 每槽 4 字节
count++                               # 写在 +0x14
```

24 字节布局：5 个 `u32` + `+0x14` 计数 `[确认 尺寸]`。`vfunc+0x18` 是船模 id（`02` §2.2），`船模 + 100` 落在船实例池的静态定义段 100–206（`05-ships-fleet/01`）——即原版隐含区间「model+100 = 该船模的模板船槽」`[推断]`。D1：重构改成船模表显式字段 `template_hull`，token 存船模 key，不做 `+100` 算术。

文案：

- `拖走捕获的船。`（`0x342f`）
- `拖拉的船丢失了。把拖来的船停泊在船坞里。`（`0x343f`）
- `一只舰队最多可以有五艘船，新船放在船坞里。`（`0x7df7`）
- `持有的船过多了！`（`0x2e4f`）
- `擒获敌人的%s！` / `占领敌人的%s！` / `%s号被俘获了`（`0x13b97`–`0x13bdb`）
- `俘虏了敌人的%s！` / `提督，%s被俘虏了！`（`0xf45f`–`0xf4af`）—— `%s` 在这批战斗句里与船名句混排，**不能**据此断言写了名册

入坞菜单：`fn_140004cb4`（`0x140172f30`，RTC `cDock` 16 B、`asShipLoad` 200 B）选项 0–3；选项 1 调 `fn_140154110(&cDock)`。`fn_140010eb0`（`0x140177570`，RTC `cDock` / `sMenuOption`）是船坞选项窗，本任务 cpp 反编译失败，内部表 `[未知]`。战斗只负责把 token / 拖船指针交出去。

### 2.9 人物记录与俘虏

`[确认]` 原版没有独立的「98 名册」数组。存档人物段从 `+0x2C03` 起按 `person_id` 0–234 共 235 条 `CPerson` 记录：id < 87（`0x57`）每条 25 字节（隶属、六维、经验、HP、身心、装备），其余每条 2 字节（隶属 + `+0x22`），再接主角 `CPlayerPerson`。来源：`CPerson::vfunc_06/07`（body `0x1401cb480` / `0x1401caf70`，`cmp eax,0x57`）、`03-characters/04`、`03-characters/01`。

`SAVE_SAILOR_COUNT=98`、「`+0x2C00` 步长 `0x19`」是 dk4save 工具常数（`Constants.h`、`save_format.md`）：起点差 3 字节，ids 0–86 字段偏移相抵；行 87–97 是错读，会写坏 2 字节区。不是引擎常数，不进 caps。

`[推断]` 可招募航海士在 `person_id` 0–97 内（人物文档）；引擎侧未找到显式 98 上限检查。

`[确认]` `fn_14000ec6e` 有局部 `iCaptureCrew`，初值 0，地址传给 `fn_14000a817`，**值**传给 `fn_140007aea`。`007aea` 用它做「还留在船上的水手人数」累加器，水手是 `vfunc+0x58`（headcount），不是 `person_id`。

`[确认]` `fn_14000eda9` 循环 `ePerson >= 0xEB`（235），对在某舰队上的人调 `fn_1400141ff`。调用点带 `sEventTrigger`（`fn_1401ac810.cpp`），**不是** `00ec6e` 的下一步。

`fn_1400141ff`：若 `person+0x20 > 0` 且第二参 `<= 0`，则 `fn_14000ab0f(person, 4)`、`fn_1400026d5(person, 0x190)`，再写 `+0x20`。`0x190=400`。这不像入册，倒像改一个人物状态字。

结论：本批代码**没有**「把敌方航海士招入玩家舰队」的确认路径。`02` 已标 `[未知]`。重构仍发 `CrewCaptured { person_id }`，人物系统可以 no-op。无名水手人数走 `Ships.crew`，不要塞进名册。

### 2.10 舱位左列：战利品 / ★ / `%s等级`

`CTrade::DrawCargoList`（`0x140285130`，RTC `rcGrade` / `rcPrice` / `szBuff`）。左列在 `+0x590 != 1`（买入价列关闭）或跳过买入价分支时绘制。来源：`trade/CTrade.cpp`、`trade_stack.md`。

GBK 字面量被反编译器读成 UTF-16，按字节还原：

| 反编译 | 字节 | 句子 |
| --- | --- | --- |
| `"\ubdd5\ufbc0\ub7c6"` | `D5 BD C0 FB C6 B7` | `战利品` |
| `"\u7325\uc8b5\ub6bc"` | `25 73 B5 C8 BC B6` | `%s等级` |

`DAT_1405a4d84` 是 ★ 那一支用的短串（仓库未抽字节，按 `trade_stack.md` 观察）。

判定：

```
# fn_140003cf6：买入价为 0 → 真
is_prize(slot) = (fn_14000f3da(slot) == 0)

# fn_14000bf00
is_star(slot) =
    item(slot) == brand_item(slot)     # fn_14022fa20
    and grade(slot) == 1               # fn_14000e421
    and not is_prize(slot)
```

绘制：

- 买入价列开、且不是 `fn_1400110c7` 那条跳过：`is_prize` → 画「战利品」，否则 `%6d` 买入价（橙色 `0xffffc080`）
- 否则走等级列：`is_star` → 画 ★ 串；否则 `wsprintf("%s等级", grade)`
- `is_prize` 为真时等级列改成偏红 `0xffff8080`

`fn_14000e421` 从货记录 `+1` 读等级 `[推断]`（函数里对指针 `+1` 再读）。`fn_14000bc6c` 读 `item`。`fn_14000f3da` 体只有 16 字节，按贸易文档是 `+4` 的 `u32` 买入价 `[推断]`。

因此：

- **战利品** = 这格没有买入价。战后分过来的货只要 `buy_price` 保持 0，交易所就会显示「战利品」
- **★** = 等级 1 且是品牌货，且**不是**战利品。属经济显示态，不是战斗公式。本系统不得为了「看起来像星」去改市价或品牌位

`TRADINGITEMGRADE` 符号有、MIN/MAX 整数无（`01-trading-items.md`）。战斗只原样拷 `grade` 字节。

### 2.11 战后人物扫不是本系统的入册

`fn_14000eda9` 见 2.9。经验 / 名声由 `progression` 订 `BattleResolved` 另算（`01` 已禁战斗写经验）。

## 3. 实体与 ID

| 实体 | ID | 原版 | 空值 |
| --- | --- | --- | --- |
| 战后会话 | `capture_session_id` | 运行时 | |
| 擒获槽 | `capture_ship_slot` | 5（`eCaputureShip`） | 与 `fleet_ship_id` 同宽 |
| 拖船 | `tow_ship_id` | 5 指针 | `null`（原版 `0xD0`） |
| 分货按钮 | `prize_hold_slot` | 25 | 货 `null`（原版 `0x6A`） |
| 入坞 token | `dock_token` | 5×u32 + count | count=0 |
| 货记录 | `HoldRecord` | 8 B | `null`（原版 item=106） |
| 人物 | `person_id` | characters 拥有 | 不由本系统分配 |
| 消息类型 | `message_type_id` | `0x13c/13e/141/143/13d/146/104` | `INVALID_MESSAGETYPE` |

运行时按 caps 循环。原版包：`fleet_ships: 5`、`hold_slots_per_ship: 5`、`prize_hold_buttons: 25`。

会话快照只持 ID 与标量：`ship_instance_id`、`trading_item_id`、`qty`、`grade`、`buy_price`、`gold`、`water`、`food`。不持 `CShip*`、`World*`、`Player*`。

## 4. 算法 / 公式

立即数进 `config/formulas/combat.yaml` 的 `prizes:`。金币与水粮的乘除数已从原始指令补齐（§2.3、§2.4）。

### 4.1 哪些船进结算

```
eligible_tow(ship):
  return state(ship) == captured_state           # vanilla 4
         and not skip_predicate(ship)            # fn_140001bea

eligible_dock_now(ship):
  return state(ship) == taken_state              # vanilla 5
         or (state(ship) == captured_state
             and situation.take_now)             # fn_140001e7e == 1
```

### 4.2 水粮（原版包）

```
n = sum(Ships.cabin_count(ship, supply_room) for ship in captured_state_4)
if n <= 0: supplies = {water: 0, food: 0}
else:
  water = n * supplies_per_cabin + rand(supply_rand)
  food  = n * supplies_per_cabin + rand(supply_rand)
  emit LootSupplies { water, food }
  Supplies.add(player_fleet_id, water, food)   # ships 拥有；原版 fn_1400044f8：按物资仓库容量逐船填，溢出下一艘
```

```yaml
supply_cabin_role: supply_room     # 原版种类 9
supplies_per_cabin: 20             # imul 0x14 [确认]
supply_rand: 10
```

`Supplies.add` 的容量规则（物资仓库间数 ×100、水粮配比）属 ships，战斗只给两个数。实现对着 `fn_1400044f8`，不要改成「平均分」。

### 4.3 金币

```
gap = 0
extra = 0
for ship in enemy_alive_slots:
  if state == 4:
    gap += Ships.max_hull(ship) - hull(ship)     # fn_1400084db 单参
  if state in (4, 5) and Scenario.flag("prize_ship_value_bounty"):   # 原版 eScenario==1；剧本旗标，非修正器
    extra += Ships.value(ship)

gold = hull_gold_factor * gap
if enemy_flagship_captured and battle_owner == player:
  gold += enemy_fleet_money / (fleet_money_div_alt if flag_b75 else fleet_money_div)
if gold > 0: Economy.transfer(enemy_fleet, player, gold)

if extra > extra_gold_min:
  Force.add_gold(player_force, extra / extra_div, reason=prize_bounty)   # 原版：先 extra /= 10 再入账
```

```yaml
hull_gold_factor: 100            # [确认] imul 0x64
fleet_money_div: 20              # [确认]
fleet_money_div_alt: 50          # [确认] fn_140012b75 为真时；两者都是无符号 div
extra_gold_min: 10               # [确认]
extra_div: 10                    # [确认]
supplies_per_cabin9: 20          # [确认] 水 / 粮各 n9*20 + rand(10)
supplies_rand: 10
```

`Force.add_gold` 走有符号饱和（factions 拥有；原版 `fn_140002cb6` 封 [0, `$caps.gold_max`=99,999,999]）。剧本 1 的船价奖金由剧本包旗标打开：

```yaml
# scenarios/vanilla_helmut.yaml 片段（D5）
flags: { prize_ship_value_bounty: true }   # 战斗读 Scenario.flag("prize_ship_value_bounty")
```

战斗模块自己不算「这格货值多少钱」——那是市价，战后货的 `buy_price` 保持 0。

### 4.4 分货

```
holds = dump_enemy_holds(enemy_fleet)            # 最多 5×5，asShipLoad 200 B
buttons = 25
place(holds → first empty buttons)
player assigns button → own_ship.hold or tow_ship.hold
on confirm:
  Ships.setHold(...)
  leftover:
    if mode == leftover_to_tow: offer_tow_warehouse
    if mode == leftover_discard: discard          # ClearHoldRecord
  Ships.clearHolds(enemy_fleet)                  # fn_1400013ca
```

拷到玩家格时 **不要** 填买入价。`buy_price=0` 才会在交易所画「战利品」。`grade`、`item` 原样拷。`qty` 原版包 = 1（`caps.cargo_stack`）。

### 4.5 船体去向

```
if eligible_dock_now(ship):
  token = Ships.ship_type(ship).template_hull     # 原版 vfunc+0x18 + 100
  emit HullCaptured { token, ship_id, instance_id }
  Fleets.detach(enemy_fleet, slot)
elif eligible_tow(ship):
  inst = Ships.clone(ship)                       # fn_1400018e8
  emit ShipTowed { instance_id: inst }
# 进港后
if player_fleet.free_slots > 0:
  Fleets.attach(player_fleet, inst)
else:
  Dock.store(inst)                               # 「新船放在船坞里」
```

```yaml
tow_slots: 5
```

舰队已满仍 `Fleets.attach` 必须失败并改走坞。坞满则 `Dock.store` 失败，UI 用 `ships.too_many_hulls`，战斗不删 token 以外的世界表。

### 4.6 显示（左列，逻辑只给 flag）

```
HoldDisplay:
  prize: buy_price == 0 and item != empty
  star:  grade == star_grade and not prize and Economy.is_brand(item)
  grade: raw grade byte
```

```yaml
star_grade: 1
prize_buy_price: 0
```

UI 选 key：`trade.prize` / `trade.star` / `trade.grade_fmt`。战斗不画字。

## 5. 耦合分析

| 原版纠缠 | 重构 |
| --- | --- |
| `6b95` 既当货舱数又被旧笔记写成金币 | 金币、水粮、货舱三种调用分三个命令 |
| `COrganizeLoadView` 在 `ship/`，由海洋战斗函数 new | Ocean 发 `BattleResolved`；UI 订 `PrizeHoldOpened` |
| `CCapturedLoadSprite` 与分货 View 同带 | 精灵属 `ui/`，逻辑不 new |
| 拖船指针数组 40 B | `tow_ship_id[]`，LIVE 取实例 |
| `cCatchedShipArray` 24 B 挂在战后栈上 | `Dock.store` / `Fleets.attach`，不把数组做成世界对象 |
| 入坞菜单 `cDock` 在城镇 / 造船 | town 订 `ShipTowed`，战斗不打开造船厂 |
| 交易所左列画「战利品」 | 只靠 `buy_price==0`；economy 不特判 battle_id |
| ★ 用品牌 + 等级 1 | `Economy.is_brand`；combat 不写品牌 |
| 人物 / 航海士 | 不写；`CrewCaptured` 可空 |
| `ClearHoldRecord` 与贸易共用 | `Ships.clearHold` 同一条 |
| `fn_14000fb14` 类型号 | `ui/` 映射 `message_type_id → string_key` |

NPC 互打的 `Combat.resolve_offscreen`（`03`）仍发 `BattleResolved`，`loot` 可空、不开分货窗。

## 6. 解耦后的配置

### `config/combat/prizes.yaml`

```yaml
schema: dk4.combat.prizes
version: 1
capture:
  ship_state_captured: 4                 # [推断] 可拖、可分货
  ship_state_taken: 5                    # [推断] 立刻入坞数组
  skip_predicate: fn_140001bea           # 实现时换成命名查询
  tow_slots: 5
  dock_slots: 5
  organize:
    hold_buttons: 25
    mode_leftover_tow: 3
    mode_leftover_player: 4              # fn_140008783 真时
    click_own_ships: { lo: 5, hi: 9 }
    click_holds: { lo: 11, hi: 35 }      # 0xB..0x23，属 UI
    click_tow_pages: { lo: 37, hi: 41 }
    selected_ship_none: null             # 原版 6（越界哨兵）
message_types:
  loot_gold_hull: 0x13c
  loot_supplies: 0x13e
  loot_gold_extra: 0x141
  tow_announce: 0x143
  leftover_mode3: 0x13d
  leftover_mode4: 0x146
  leftover_mode01: 0x104
string_keys:                             # 只引用，中文在 ui/strings
  gold_from_captured_ship: battle.gold_from_captured_ship
  loot_gold: battle.loot_gold
  loot_gold_short: battle.loot_gold_short
  loot_gold_siege: battle.loot_gold_siege
  water_food_from_captured: battle.water_food_from_captured
  tow_captured_ship: battle.tow_captured_ship
  dock_towed_ship: ships.dock_towed
  leftover_discard: battle.leftover_discard
  leftover_to_tow: battle.leftover_to_tow
  prize_label: trade.prize
  star_label: trade.star
  grade_fmt: trade.grade_fmt
  ship_captured: battle.ship_captured
  ship_occupied: battle.ship_occupied
  crew_captured: battle.crew_captured    # 人；原版是否发 [未知]
hold:
  empty: null                            # 原版 0x6A，legacy codec
  prize_buy_price: 0
  copy_grade: true
  copy_item: true
  fill_buy_price_on_capture: 0
cabin_roles:
  cargo: cargo_room                # 原版种类 10
  supply: supply_room              # 原版种类 9
```

引擎只问 `Ships.cabin_count(instance, role)`；原版种类号只在 `cabins.yaml`。

### `config/formulas/combat.yaml`（战利品段）

```yaml
prizes:
  impl: vanilla.prizes
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }         # 舰队金币 / fleet_money_div(_alt) 为无符号 div（§4.3）
  params:
    supply_rand: 10
    supplies_per_cabin: 20
    extra_gold_min: 10
    extra_div: 10                          # [确认]
    hull_gold_factor: 100                  # [确认] imul 0x64
    fleet_money_div: 20                    # [确认] 无符号
    fleet_money_div_alt: 50
    star_grade: 1
    prize_buy_price: 0
    organize_hold_buttons: 25
```

### `config/caps.yaml` 片段

```yaml
fleet_ships: 5
hold_slots_per_ship: 5
prize_hold_buttons: 25                   # 原版 5×5；运行时 = fleet_ships * hold_slots
tow_slots: 5
dock_capture_slots: 5
```

`prize_hold_buttons` 必须等于 `fleet_ships * hold_slots_per_ship`。改一侧要改另一侧，校验器报错，不要在代码里写死 25。

### `config/ui/strings.zh-CN.yaml` 片段（不进逻辑表）

```yaml
battle.gold_from_captured_ship: 从擒获的船上得到{gold}金币
battle.loot_gold: 获得了{gold}枚金币的战利品！
battle.water_food_from_captured: 从俘获的船上得到{water}桶水，{food}桶粮食
battle.tow_captured_ship: 拖走捕获的船。
ships.dock_towed: 把拖来的船停泊在船坞里。
battle.leftover_discard: 没装的东西就不要了，行吗？
battle.leftover_to_tow: 将没装的货物放到拖船的仓库里，行吗？
trade.prize: 战利品
trade.star: ★
trade.grade_fmt: "{grade}等级"
```

原版 `0x13bfb` 一条记录里有七句同义句，导入器拆成 `battle.loot_gold` 的变体表，由 UI 随机或按角色取。逻辑只传 `{gold}`。

## 7. 独立旋钮

改这些不应迫使贸易公式、人物成长、城镇改代码：

- `tow_slots` / `dock_capture_slots` / `prize_hold_buttons`
- 擒获是否自动入坞、舰队满时是否拒绝拖带
- 分货 mode 3/4（剩货进拖船还是进己船）
- `prize_buy_price`（想让缴获货以买入价入账就改 0；原版包保持 0）
- `supply_rand`、`extra_gold_min`
- 是否发 `CrewCaptured`（默认发、人物可忽略）
- offscreen NPC 互打要不要金币（默认不要，与 `03` 的空 `loot` 一致）

加一种「只抢货不拖船」：只加 `capture.allow_tow: false`，不要改炮击。俘虏航海士入队：人物系统订 `CrewCaptured`，本文件不改。

## 8. 对外接口

查询：

```
Combat.capture_snapshot(battle_id) -> CaptureSnapshot
Ships.cabin_count(instance_id, role) -> int
Ships.holds(instance_id) -> [HoldRecord]
Ships.free_fleet_slots(fleet_id) -> int
Dock.free_slots() -> int
Economy.is_brand(trading_item_id) -> bool
Characters.in_party() -> [person_id]          # 俘虏入册前先问有没有空语义
```

`CaptureSnapshot` 只有 ID 与值：

```
CaptureSnapshot {
  battle_id,
  enemy_fleet_id,
  gold_hull, gold_extra,
  water, food,                     # 对应关系 [未知] 前按载荷两项不命名
  holds: [{ trading_item_id, qty, grade, buy_price }],
  tow:   [{ ship_instance_id, ship_id }],
  dock:  [{ token, ship_id }],
  capture_crew_headcount           # iCaptureCrew 同源；不是 person_id
}
```

命令：

```
Force.add_gold(force_id, delta, reason)        # factions；正数入账
Supplies.add(fleet_id, kind_id, qty)           # ships
Ships.setHold(instance_id, slot, HoldRecord)
Ships.clearHolds(fleet_id)
Ships.clone(instance_id) -> instance_id
Fleets.detach(fleet_id, fleet_ship_id)
Fleets.attach(fleet_id, instance_id) -> ok|full
Dock.store(instance_id) -> ok|full
PrizeHolds.open(battle_id, mode_id)
PrizeHolds.commit(session_id)
PrizeHolds.cancel(session_id)                  # 没装的丢掉
```

事件（载荷只有 ID 与值）。`01` 的 `BattleResolved` 在本系统里**展开**为：

```
BattleResolved {
  battle_id:            int
  outcome:              key                  # 会话结局；不是 eBattleResult 槽号（01 §2.12）
  force_a, force_b:     force | null         # factions 据此写战果记录槽
  winner_side:          0 | 1 | null
  attacker_fleet_id:    fleet_id
  defender_fleet_id:    fleet_id
  sea_area_id:          sea_area_id
  town_occupied:        town_id | null

  gold:                 int                  # gold_hull + gold_extra，≥0
  gold_parts:           { hull: int, extra: int }

  supplies:             { a: int, b: int }   # 水/粮；key 待舱种对照后改名

  cargo: [
    {
      trading_item_id:  trading_item_id | null   # 空格 null，不省略槽
      qty:              int                  # 原版包 1
      grade:            int
      buy_price:        int                  # 缴获为 0
      prize:            bool                 # buy_price == prize_buy_price
      from_ship_id:     ship_instance_id | null
    }
  ]

  captured_ships: [
    {
      ship_id:          int                  # 目录
      instance_id:      int                  # 克隆后的活船；入坞前也可有
      slot:             capture_ship_slot
      destination:      "tow" | "dock" | "discard"
      token:            int | null           # 仅 dock
    }
  ]

  captured_persons:     [ person_id ]        # 可空；原版路径未确认
  capture_crew_headcount: int                # 无名水手人数

  loot:                 [                    # 给旧订阅者的扁平投影，可生成
    { kind: "gold", qty: int },
    { kind: "supply", key: "a"|"b", qty: int },
    { kind: "cargo", trading_item_id, qty }
  ]
}
```

订阅：

| 事件 | 谁订 | 做什么 |
| --- | --- | --- |
| `BattleResolved` | economy | 不改市价（金币由 combat 直接调 `Force.add_gold`，§4.3） |
| | ships / fleets | `Supplies.add`、`attach` / `Dock.store` |
| | factions | 读 `force_a/force_b`、`winner_side`、`town_occupied`，写战果记录槽 |
| | progression | 经验 / 名声（本系统不算） |
| | narrative | 剧本触发 |
| | ui | 战果句、分货窗 |
| `PrizeHoldOpened` | ui | 打开 `COrganizeLoadView` 等价场景 |
| `PrizeHoldCommitted` | ships | 按玩家分配写格 |
| `ShipTowed` | ocean / town | 进港时「停泊在船坞」 |
| `HullCaptured` | ships | 坞 token |
| `CrewCaptured` | characters | 可选 `PersonRecruited`；默认忽略 |
| `LootGoldCredited` | ui | `battle.loot_gold` |
| `LootSuppliesTaken` | ui | `battle.water_food_from_captured` |

战斗发完 `BattleResolved` 即丢会话。分货窗是 UI 对快照的编辑；确认后只许发 `PrizeHoldCommitted`，不许再改 `gold`。

`ShipCaptured`（`02` 已列）保留为近战当时的单船事件，最终仍汇总进本载荷的 `captured_ships[]`。不要两套金币。

## 9. 未核实字段

| 项 | 事实 | 不要写成 |
| --- | --- | --- |
| `fn_140006b95(ship, 10)` 在 `005fb0` | 货舱计数 | 金币公式 |
| `fn_14000a817` 金币 | `[确认]` `gap*100` + 敌舰队现金 /20 或 /50；`s44/10` | 「货值市价」 |
| `fn_140012b75(敌舰队)` | 决定 /50 还是 /20 | 海盗 / 王国定论 |
| 物资仓库（9）/ 补给槽 0、1 | 9 是舱室种类（容量）；0/1 是补给下标，0 = 水、1 = 粮 `[推断，强]` | 把补给下标当舱室种类 |
| `fn_1400084db(ship)` | 单参；`(改装2,3数)*表[size] + (vfunc48==3?10:0) + fn_14000e129` | 最大耐久 `[推断]` |
| `fn_140001bea` | 真则跳过该船 | 「已是玩家船」 |
| `fn_140008783(DAT_144296500)` | 决定 mode 4/3、能否克隆 | 「是否玩家回合」 |
| 克隆里的 `0x62` | 与 dk4save 常数 98 同数 | 写入名册 |
| `fn_14000d599 == 0x1A` | 无货时仍可能开窗 | 与 `CBattleResult` 同一枚举 |
| `vfunc+0x18 + 100` | 入坞 token = 船模 + 100（模板船槽 `[推断]`） | 引擎里做 `+100` 算术 |
| `cCatchedShipArray` 5 个 u32 | 尺寸 24 B | 3 个指针 |
| `iCaptureCrew` | 水手 headcount 累加 | 航海士人数 |
| `fn_14000eda9` / `fn_1400141ff` | 改 `person+0x20`、`0x190` | 俘虏入队 |
| 具名航海士入册 | 无本链代码 | 原版会招俘虏当航海士 |
| `eMessageType` 0x13c 等 | 类型号 | mesfile 偏移 |
| `0x13c/13e/141/143` ↔ 哪句 | 只有个数与上下文 | 逐句钉死 |
| `fn_1400110c7` | DrawCargoList 跳过买入价 | 空实现当「无货」确认 |
| `fn_14000f3da` | 16 字节 | 不标推断就当买入价 API 名 |
| ★ | 品牌 ∧ 等级 1 ∧ 非战利品 | 「缴获货画星星」 |
| 船态 4/5 的名字 | 两套分支 | 投降 / 白旗 / 占领 |
| `s158 == 2` | 跳过 `00a817` | 「逃走」确认 |
| `fn_140010eb0` 船坞菜单 | RTC 有 `sMenuOption`，反编译失败 | 坞容量公式 |
| `COrganizeLoadView+0x380` 5 指针 | ctor 清零 | 第三组货舱 |
| 拖船在大地图上怎么跟 | 文案有「拖拉的船丢失了」 | 航速公式 |
| offscreen 互打的金币 | `03` 允许空 loot | 与玩家同一公式 |

`fn_14000a817` 全长 `0x5f5`，`fn_140005fb0` 全长 `0x2bb`。已确认：调用图、金币（`gap*100` + 敌舰队现金 /20 或 /50 + 剧本 1 船价 /10）、水粮（`n9*20 + rand(10)`）、舱种 10/9、25 格、拖船 5 指针、入坞 24 字节数组、战利品=买入价 0、★=品牌等级 1。未展开：`fn_140012b75`、`fn_1400044f8` 的水粮配比 `per`。不要从市价或别作的打捞表抄金币。
