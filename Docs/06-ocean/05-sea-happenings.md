# 海上 happening

大地图航行时掷出来的随机事件：暴风雨、暴风雪、坏血病、老鼠、海豚（航海日志）、鲨鱼（货损 / 幼鱼新交易品）、沉船、美人鱼、气泡、传染病、无风、水手哗变。探索圈和 NPC 舰队遭遇在 `02-search-encounters.md`。海战五命令在 `07-combat/03-battle-ai.md`，不要和本表共用 `happening_id`。

原版符号 `eHappening` 只出现在掷骰函数。`num_values.json` 的 `NUM_HAPPENING=5` **不是**这张表的长度。

## 1. 职责边界

本系统管：

- 航海日步上的 happening **门闩、掷骰、20 槽派发**
- 槽 → 结局 `outcome_id` 的映射（气候 / 纬度 / 出航天数 / 船首像）
- 只发事件：风暴命中、疾病、货损、日志邀请、交易品候选、小游戏启动、哗变选项
- 持续态（老鼠还在船上、哗变未平）如何让后续日步继续扣补给——**发状态事件**，不自己改货舱

不管：

- 捉鼠规则、30 秒、20 只舱室槽（`CRatGameView` 属 `town/minigames.yaml`）；Ocean 只发 `MinigameStart { key: rats }`
- 航海日志怎么存（Save）；只发 `LogbookOffer`
- 幼鱼进不进交易所（Economy 收 `NewTradeGoodCandidate`，已有交易品鱼翅 id 21、发现物小鲨鱼 id 4）
- 坏血病 / 传染病怎么改 `physical_state`（`03-characters/07-fatigue-health-mood.md` 订阅）
- 每 tick 正常扣水粮（`01-sailing.md`）
- 气候场怎么生成（`01-world/04-climate-weather-zodiac.md`）；本系统读当前气候字节和坐标
- 海战 happening 命令号 2–6（Combat）
- 港口传闻「定期船遇到暴风雨沉没了」（计略 / 广场，不是本表）
- 文案正文（只挂 `ui.ocean.happenings.*` key）

## 2. 原版事实

### 2.1 两个「happening」计数 `[确认]` 分开

| 符号 | 值 | 用在哪 | 来源 |
| --- | --- | --- | --- |
| `eHappening` RTC | 循环上界 20 | `fn_1400f2d90` 扫 CDF | `rtc_index.tsv`；反汇编 `cmp …, 0x14` |
| 未命中哨兵 | 21（`0x15`） | 表扫完仍未命中则返回 | 同函数 `mov eax, 0x15` |
| `NUM_HAPPENING` | 5 | `num_values.json` RTC 启发式 | **没有** `HappeningID.h`；`constants.json` 的 `NUM_` 列表里也没有这项 |
| 海战命令号 | 2–6 共 5 个有效枝 | `fn_14000cdc9` | `07-combat/03-battle-ai.md` |

`num_from_rtc.json` 给 `eHappening` 记了比较立即数 5（6 次）、34（2 次）、20（1 次）。掷骰函数本身只有 20 与 30，没有 5。5 当作本表长度是工具误伤，重构禁止再写 `NUM_HAPPENING=5` 当海上事件数。

运行时读 YAML 表长，禁止把 20 写进代码。

### 2.2 掷骰 `fn_1400f2d90` `[确认]`

body `0x1400f2d90`，end `0x1400f2e44`，文件 `srcdata/source/game/unassigned/fn_1400ed7c0.cpp`（反编译把表指针搅成 `0xcccccccc` 乘法，以反汇编为准）。

```
ecx = days_at_sea                    # 调用方传入
table_index = (days_at_sea >= 30)    # 0x1E
table = DAT_14054e1d0 + table_index * 0x50   # 每表 20 个 dword
roll  = rand(200)                    # fn_140005272(0xC8)，[0, 200)
for i in 0..19:
    if roll < table[i]: return i
return 21                            # 本包末槽 CDF=200，[0,200) 扫完必命中，21 是安全哨兵
```

两张表已从 `Game/DK4HD_sc.exe` `.rdata` dump（VA `0x14054e1d0`，fileoff `0x54cfd0`）。比较式是 `roll < cdf[i]`，所以表是**累积阈值**，不是独立权重。

**表 0**（出航天数 `< 30`）：

```
cdf:    [30, 65, 153, 158, 159, 165, 166, 168, 171, 172, 173, 174, 178, 181, 185, 191, 194, 196, 198, 200]
mass:   [30, 35,  88,   5,   1,   6,   1,   2,   3,   1,   1,   1,   4,   3,   4,   6,   3,   2,   2,   2]
```

**表 1**（出航天数 `>= 30`）：

```
cdf:    [20, 50, 139, 144, 145, 151, 152, 154, 157, 158, 159, 160, 164, 167, 171, 191, 194, 196, 198, 200]
mass:   [20, 30,  89,   5,   1,   6,   1,   2,   3,   1,   1,   1,   4,   3,   4,  20,   3,   2,   2,   2]
```

和表 0 相比：槽 0/1 变轻，槽 15（哗变，见 2.4）从 6 跳到 20。远航更常闹事，不是另一套事件目录。

唯一调用点：`fn_140005a15`（body `0x1400fa870`，RTC `eNewFleetState, asCompanion, eFleetShip, nLatitude, nLongitude, anSupply, aePerson, nShipTotal, anSupplyShortage`）在 `0x1400fb76d`。该函数反编译失败，派发以反汇编为准。

### 2.3 日步门闩 `[确认]`

`fn_140005a15` 在扫完船/补给/同伴之后：

1. `fn_14000b87a(fleet)` 读 `fleet+0x30`。不等于 `0x1B=27` 则整段跳过。`[推断]` 27 是「可掷 happening」空闲态；命中后 `fn_14000a8fd` 把 `+0x30` 写成 `outcome_id`，直到入港或持续态结束才回到 27。
2. `rand(30)==0` 才继续。日步期望 1/30。
3. 两个查询 `fn_140006c12`、`fn_140003689` 都为假时，才把 `fn_140012e36(fleet)` 的返回值交给 `fn_1400f2d90`。否则 `rand(3)==0` 时**强行槽 2**，不掷表。这两个查询的语义 `[未知]`（自动航线 / 无瞭望 / 非玩家舰队都像，未钉）。
4. 掷出 21 则结束。
5. `fn_1400f55d0(slot, &outcome)`：槽合法且门闩通过则返回 1，并**回写** `outcome`（初值 27）。返回 0 视为本轮无事。
6. 按 `outcome` 三路特判，其余走通用效果：

| `outcome` | 函数 | RTC / 可见副作用 |
| ---: | --- | --- |
| 20（`0x14`） | `fn_1400f6300` | `eShipSupply, eFleetShip, aszMenu`；三选一「镇压 / 说服 / 置之不理」 |
| 21（`0x15`） | `fn_1400f5460` | 无 RTC；对每船 `rand(10)+10` 两次后改船态 3 或 4 |
| 22（`0x16`） | `fn_1400f6090` | `acFleetShip`；随机一艘，可能咬货、船体 `rand(5)+3` |
| 其它 | `fn_1400ec260(outcome, ships)` 然后 `fn_14000a8fd(fleet, outcome)` | 按结局改船态；把 `+0x30` 与时间戳 `+0x34` 写上 |

函数末尾无论本轮有没有掷中，都会再 `fn_1400f0450(state, ships)`（RTC `anSupply`）。`[推断]` 持续 happening 态在日步里继续扣补给，老鼠/疾病不是「播一句就结束」。

### 2.4 出航天数 `[确认]`

`fn_140012e36`（body `0x1401df010`）：

```
days_at_sea = (now - fleet[+0x2c]) / 48
```

`now` 来自 `fn_1400104b5`。48 `[推断]` 一天的时间单位（与航行文档 `nTime` 日步同量级）。`+0x2c` 相对 `fn_14000a8fd` 写的 `+0x34` 不是同一格：`[未知]` 哪一格是「离港时刻」、哪一格是「上次 happening」。配置里参数名仍叫 `days_at_sea`，除数 `ticks_per_day=48`。

同一返回值还做：

- 选 CDF 表：`>= 30` 用表 1
- 槽 0、1：`< 15` 则 mapper 失败（近岸不刮本表风暴）
- 槽 15：`>= 40` → outcome 21；`>= 20` 且 `fn_14000ee12(fleet) >= 50` → outcome 20；否则失败。`fn_14000ee12` 读 `fleet+0x71` `[未知]` 是士气还是剩余补给

### 2.5 纬度、月份、冰盖 `[确认]`

坐标由 `fn_14000a6fa` 填 `nLatitude` / `nLongitude`（本函数 RTC 名）。`fn_140008dff` 就是绝对值，给纬度门用。

`fn_1400f95c0`（RTC `posCrust`）按 `|lat|` 与月份（`fn_140009c19`）判「冰」：

| 条件 | 结果 |
| --- | --- |
| `|lat| >= 70` | 冰 |
| `|lat| >= 60` 且月份 `<= 2` 或 `>= 10` | 冰 |
| `|lat| >= 45` 且月份 `<= 1` 或 `>= 11` | 冰 |
| 其它 | 非冰 |

槽 2 在气候字节 ∈ {2, 3} 时：冰 → outcome 5，否则 outcome 4。`[推断]` 暴风雪 vs 暴风雨，高权重 44% 被气候门大部分打成空弹。

气候字节：`fn_1400010f5` → `fn_140101330`，读 `fleet+0x14` 的 1 字节。取值 0–5 与 `fn_140078640` 把气候夹到 `0..5` 一致 `[推断]`。

### 2.6 船首像 / 护佑 `[确认]` 检查点，`[推断]` 效果号

槽 0 与槽 17 共用风暴保护：`fn_140003f99(fleet, 9)` 扫 5 船，跳过空道具 `0xD0=208`，对物品 `vfunc+0x48 == 9` 则整槽失败（当没发生）。文案对应虎鲸像（id 116，「连可怕的暴风雨也可以躲过」）和赫尔墨斯的祈祷（id 22）。重构读 `ModifierSet.storm_protect`，禁止 `switch(item_id)`。

槽 7：`fn_140004bfb(0x75)` 取道具 id **117 白鲸像**，`fn_140013ecb` 看是否已装；装了则失败。文案「不易沉船」。这是**写死 item_id**，与效果号 9 不是同一条。重构改成 `ModifierSet.wreck_protect`。

金黄色的猫（id 20）「有老鼠作乱时一定能发挥很大的作用」；莱姆的滴汁（id 21）防坏血病；华陀的中药（id 23）/ 圣母像（id 123）防传染病。这些检查不在 mapper 的 20 个 case 里，在后续持续态 / Characters 订阅里做。

### 2.7 mapper `fn_1400f55d0` 20 路 `[确认]` 控制流

body `0x1400f55d0`，跳表 `0x1400f5e14`，`slot > 0x13` 失败。先查 `fn_1400104b5` 一类「能否 happening」的守卫，失败直接返回 0。

| 槽 | 写出 outcome | 额外门闩 | 可见调用 |
| ---: | ---: | --- | --- |
| 0 | 0 | `days >= 15`；效果号 9 则失败 | 台词表 +（outcome≠0 时）`fn_140003c92(..., 0xB, 0, 30)` |
| 1 | 1 | `days >= 15` | 台词表。无效果号 9 |
| 2 | 4 或 5 | 气候 ∈ {2, 3}；冰盖决定 5/4 | 无直接台词 |
| 3 | 6 | 气候 ∈ {3, 5} | 无直接台词 |
| 4 | 2 或 3 | `\|lat\| >= 55` → 3 否则 2；气候==4 另播台词并 `fn_140003c92(..., 7, 0, 100)` | |
| 5 | 7、8 或失败 27 | 气候 0/1/4：`\|lat\|>=50` → 8 否则失败；气候 2/3 → 7 | 台词；状态 7，时长 150（outcome 8）或 10（outcome 7） |
| 6 | 9 或 10 | `\|lat\| >= 45` → 9 否则 10 | 特效 `fn_14000cf1d(..., 0x1E, 2, 1)` + 状态 7 时长 300 |
| 7 | 11 | 白鲸像则失败；`fn_140101cf0` 为假则失败 | 无直接台词 |
| 8 | 12 | `fn_140101cf0` 为假则失败 | 无直接台词 |
| 9 | 13 | 无 | 台词 `ecx=0xF` |
| 10 | 14 | 无 | 两段台词 + 状态 7 时长 50 |
| 11 | 15 | 无 | 台词 + `fn_14000bef6(0x1F8)` + 状态 7 时长 200 |
| 12 | 16 | 无 | `fn_140014803(outcome)` 填 `%s` 名 + 状态 7 时长 150 |
| 13 | 17 | 无 | 同 12 |
| 14 | 18 或 19 | `\|lat\| >= 55` → 19 否则 18；`fn_14000f26d(fleet, 1)` 且 outcome==18 则失败 | 同 12，时长 100 |
| 15 | 20 或 21 | 见 2.4 天数 / `+0x71` | 不在 mapper 播词，交给 2.8 |
| 16 | 22 | 无 | 交给 2.8 鲨鱼 |
| 17 | 23 | 与槽 0 汇合（效果号 9） | 走槽 0 的「outcome≠0」台词枝 |
| 18 | 24 | 无 | 台词 `ecx=5` |
| 19 | 25 | 无 | 与槽 6 同类特效 + 状态 7 时长 30 |

`fn_140003c92(fleet, post_or_kind, val, duration)`：`fn_14000f2d6(fleet, edx)` 取一名在岗人物，再 `fn_140011c34` 写持续标记。edx 在本表里出现 7、9、0xB。`[未知]` 是岗位还是状态种类；哗变里同一函数用 edx=8 取冲锋队长（岗位表第 8 项，见 `04-id-registry.md`）。

槽 12–14 的 `%s` 来自 `fn_140014803(outcome)` + 格式串。反汇编把该 RIP 标成 `0x140527ce3`，落在 exe 配置键 `SOFTWARE\KoeiTecmo\...` 上，**指针未对上人可读的「提督！是%s」**。文案本身在 mesfile `0x479b`（「提督！是%s！」）。名字表未还原，不能把 outcome 16 写成「美人鱼确认」。

### 2.8 三个特判结局 `[确认]` 结构

**鲨鱼 outcome 22**（`fn_1400f6090`）：

- 枚举玩家 5 船进 `acFleetShip`，`rand(n)` 挑一艘
- 旗舰枝：播词 + `fn_140003c92(..., 0xD, 1, 100)`
- 其它船：若货格有物，`fn_14000ca4a` 走「鲨鱼吃了%s」一类（接舷沉船同款取名函数）
- 不论哪枝：`fn_14000c202(ship, rand(5)+3)` 改船体；`fn_140009467(fleet, 2)` 写状态 2

后续选择「杀 / 放生 / 养着」在 mesfile `0x122f7`–`0x125ff`：「也许会成为新的交易品」。幼鱼是发现物 id 4「小鲨鱼」，对应交易品 id 21「鱼翅」（`09-items/03-discovery-planting.md`）。Ocean **不**插入 105 项表。

**哗变 outcome 20**（`fn_1400f6300`）：

菜单三句写死在 `0x14054e290`（GBK）：镇压、说服、置之不理。

- 镇压：`fn_14000f2d6(fleet, 8)` 取冲锋队长。没有则失败台词「没有冲锋队长，控制不了水手」（`0x5133`）。有则对各船改态，延迟常数 300 / 500
- 说服：对 5 船 × 2 种 `eShipSupply` 扣补给（按人数比例 `[推断]`）。扣到 0 另播词，并施加 `0xFFFFFFE2 - rand(20)`（约 −30～−49）的惩罚值
- 置之不理：第三枝 `sF8==2`

**远航危机 outcome 21**（`fn_1400f5460`，`days >= 40`）：无菜单。对每船两个 `rand(10)+10`，按 `fn_14000c4a5` 返回 2/3 改船态 3 或 4。玩法名 `[未知]`（长时间未靠岸的溃败 / 疾病加重都像）。

### 2.9 mesfile 目录 `[确认]` 文案，`[推断]` 与槽的对齐

SC `mesfile.dk4` 从文件偏移 `0x3b8b` 起是一整段海上告警，按出现顺序：

| 主题 | 文件偏移（摘） | 玩家可见效果 |
| --- | --- | --- |
| 暴风雨 | `0x3b8b` `0x3ed7` | 告警；「刚才还是晴天」是另一组 |
| 暴风雪 | `0x3c03` | 告警 |
| 坏血病 | `0x3c2b` | 必须靠岸；莱姆可防 |
| 老鼠 | `0x3d6f` | 粮食危险；催促捉鼠 |
| 看天空 | `0x3feb` | 无数值句 |
| 雾 | `0x407b` | 看不见 |
| 妖怪 / 歌声（塞壬） | `0x4107` `0x418b` | 无战斗句 |
| 有人在游泳 → 美人鱼 | `0x420f` `0x431b` | 无战斗句 |
| 奇怪气泡 / 吞船 | `0x4403` `0x45d3` | 「转舵！全体各就各位」 |
| 传染病 | `0x481f` | 不靠岸会全死 |
| 无风 | `0x48f3` | 不能动，等风 |
| 鲨鱼 | `0x4a0f` | 小心；后面接货损 / 幼鱼 |
| 水手不满 / 哗变 | `0x4aa3` `0x4c97` | 三选项；无冲锋队长镇不住 |

海豚**不在**这段里，在 `0x11a4f`：「可以将它们记录到航海日志中吗？」拒绝则无状态（`0x11c37` 离开句仍可播）。

捉鼠规则句 `0x3507`：「现在开始捉老鼠游戏。请在３０秒之内捉住各船舱出现的老鼠。」成功 `0x1293f`，失败必须靠港 `0x12747`，猫替代 `0x131eb`。

**没有**「粮食腐烂 / 发霉 / 变质」独立句 `[确认]` 全 mesfile 检索。玩家看到的「饭没了」来自：老鼠、哗变说服扣补给、航行日步正常消耗、小猪像减少饭量（船首像，不是 happening）。不要编一个 `food_spoilage` 槽冒充原版。

沉船：战斗击沉句（`0x37e7` `0xf2a7`）不是本表。本表能对上的是白鲸像挡住的槽 7、气泡吞船、风暴改船体。港口「定期船沉没」是计略传闻。

### 2.10 捉鼠 View 的触发 `[确认]` 类与工厂，调用链间接

- `CRatGameView` ctor thunk `0x14000268a`，body `0x1401ecae0`：20 槽（`>= 0x14`）在 `+0x88`，每槽 `+0x128 = 0x20`。对象体积析构时 `0x198`。
- `vfunc_13` 只吃点击号 `0x37 <= id < 0x56`。
- `fn_14000db89`（RTC `asRatData` 160 字节、`eShipCabin`）：`s158 >= 0x1E` 且剩余 `+0x17c > 0` 才继续。30 与文案秒对齐 `[推断]`。
- 工厂 `fn_1400132f0`（body `0x1401ee910`，RTC `cView`）：播特效号 `0x42`，`call 0x14000268a` 构造 View，`fn_140013a07` 跑完返回胜负，再析构。对工厂的直接 `E8` 不在 happening 派发里——Ocean tick **不 new View**。

重构：Ocean 发 `MinigameStart { key: rats, fleet_id }`。UI / town-life 订阅后打开同一套规则表。猫道具在订阅方把这次 minigame 改成自动成功，不让 Ocean 认识 `CRatGameView`。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key | 拥有者 |
| --- | --- | --- | --- | --- |
| 掷骰槽 | 0–19 | 21=未命中 | `happening_id` | Ocean 表 |
| 结局 | 0–25；27=空闲 | 27 不当内容行 | `outcome_id` | Ocean 运行时态，存档随舰队 |
| CDF 表 | 2 | | `table_index` | 由 `days_at_sea` 选 |
| 海战 happening | 5 条命令 | | `combat.happening_id` | Combat，禁止重名 |
| `eShipSupply` | 2 | | `supply_kind` | Ships |
| `eFleetShip` | 5 | | `ship_slot` | Fleets |
| `eShipCabin` | 31 | | `cabin_id` | Ships；捉鼠热区用 |
| 发现物小鲨鱼 | item 4 | | `item_id` | Items |
| 交易品鱼翅 | trading 21 | | `trading_item_id` | Economy |
| 日志条目 | 无枚举 | | `log_key` | Save；Ocean 只发 key |
| 小游戏 | minigame `rat` | | `minigame_id` | town-life |

`NUM_HAPPENING=5` 不进本系统 caps。本表 caps 是 `happening_slots`（原版包 20）和 `happening_miss_id`（21）。

## 4. 算法 / 公式

魔数进 `config/formulas/ocean.yaml` 的 `happenings:` 段。槽目录进 `config/ocean/happenings.yaml`。代码只解释表。

### 4.1 日步

```
on Ocean.calendar_step { fleet_id }:
  fleet = Fleets.live(fleet_id)
  if fleet.happening_state != ready_state:          # 原版 27
      emit HappeningPersist { fleet_id, outcome_id: fleet.happening_state }
      return
  if uniform(0, daily_roll_max) != 0: return        # 原版 30，命中 0
  days = (World.now_ticks() - fleet.left_port_ticks) / ticks_per_day
  if happenings.use_table(fleet):                   # 原版两查询都假
      slot = cdf_pick(tables[days >= table_split], happening_roll_max)
  else if uniform(0, force_slot_roll) == 0:
      slot = force_slot                              # 原版槽 2
  else:
      return
  if slot == miss_id: return
  ok, outcome = resolve(slot, days, lat, month, climate, modifiers)
  if not ok: return
  dispatch(outcome, fleet_id)
```

`HappeningPersist` 给补给 / 疾病订阅方扣当日分量；Ocean 自己不 `supply -=`。

### 4.2 CDF

```
cdf_pick(cdf, roll_max):
  r = uniform(0, roll_max)          # 原版 200 → [0, 200)
  for i, threshold in enumerate(cdf):
      if r < threshold: return i
  return miss_id
```

内容包直接存原版累积数组。引擎不要先差分再掷——差分只给人看。

### 4.3 槽目录（原版包）

权重是 mass（CDF 差分）。`key` 在未用结局函数钉死的地方标 `[推断]`，禁止当确认名写进 UI。

| id | mass0 | mass1 | key | 等级 | 触发摘要 | 发出 |
| ---: | ---: | ---: | --- | --- | --- | --- |
| 0 | 30 | 20 | `storm` | `[推断]` 风暴（有保护） | `days>=15`；`storm_protect` 则无事 | `StormHit` |
| 1 | 35 | 30 | `storm_unprotected` | `[推断]` 风暴变种，无效果号 9 | `days>=15` | `StormHit` |
| 2 | 88 | 89 | `storm_or_blizzard` | `[推断]` | 气候 2/3；冰→暴风雪 | `StormHit { kind: blizzard\|storm }` |
| 3 | 5 | 5 | `climate_3_or_5` | `[未知]` | 气候 3/5 | 待对照文案后再挂 key |
| 4 | 1 | 1 | `high_lat` | `[未知]` | `\|lat\|` 分 2/3；气候 4 加状态 7 | |
| 5 | 6 | 6 | `rats_or_scurvy` | `[推断]` 鼠疫/坏血病带 | 气候+纬度；失败写回 27 | `MinigameStart{rats}` 或 `ScurvyOutbreak` |
| 6 | 1 | 1 | `long_fx` | `[未知]` | 纬度分 9/10；状态 7 时长 300 | |
| 7 | 2 | 2 | `wreck_danger` | `[推断]` 沉船险 | `wreck_protect`（白鲸像）则无事 | `WreckThreat` |
| 8 | 3 | 3 | `wreck_or_leak` | `[推断]` | `fn_140101cf0` | `HullLeak` |
| 9 | 1 | 1 | `dolphins` | `[推断]` 只播词 | 无门闩 | `LogbookOffer { dolphins }` |
| 10 | 1 | 1 | `plague` | `[推断]` | 状态 7 时长 50；通用效果改船态 1–4 | `PlagueOutbreak` |
| 11 | 1 | 1 | `siren_or_bubble` | `[推断]` | `fn_14000bef6(504)` | `SeaHazard` |
| 12 | 4 | 4 | `named_sight_a` | `[未知]` `%s` | 名字表未还原 | `NamedSighting { outcome }` |
| 13 | 3 | 3 | `named_sight_b` | `[未知]` | 含美人鱼文案组的最大嫌疑 | `NamedSighting` |
| 14 | 4 | 4 | `named_sight_c` | `[未知]` | 高纬换名；已见过则取消 | `NamedSighting` |
| 15 | 6 | 20 | `mutiny` | `[确认]` 走哗变函数 | `days>=20/40` | `MutinyStart` |
| 16 | 3 | 3 | `shark` | `[确认]` 走鲨鱼函数 | 无门闩 | `SharkEncounter` |
| 17 | 2 | 2 | `sudden_storm` | `[推断]` 「刚才还是晴天」 | 与槽 0 共用保护 | `StormHit` |
| 18 | 2 | 2 | `crew_fatigue` | `[推断]` 水手要靠岸 | 无门闩 | `CrewUnrest` |
| 19 | 2 | 2 | `fog_or_calm` | `[推断]` 雾 / 看天空 / 无风 | 短时长 30 | `WeatherHalt` |

槽 5 同时挨着坏血病句和老鼠句，**不能**在 YAML 里写死只发老鼠。用 `outcomes_by_gate` 让气候/纬度选 `rats` 或 `scurvy`；未还原前两条都列出，运行时按表走，缺门闩的那条标 `unverified: true`。

### 4.4 冰盖

```
ice = |lat| >= ice_lat_always                                  # 70
   or (|lat| >= ice_lat_winter and month not in open_months_hi) # 60；开放月 3..9
   or (|lat| >= ice_lat_deep   and month not in open_months_lo) # 45；开放月 2..10
```

月份 0/1 起算与原版 `fn_140009c19` 对齐 `[未知]` 是否 0 基。YAML 用「闭区间月份」原样填 2、10、1、11，注释写「与立即数一致，未证 0 基」。

### 4.5 鲨鱼

```
ship = pick_uniform(alive_ships)
emit SharkEncounter { fleet_id, ship_slot }
if ship has cargo:
    emit CargoLost { fleet_id, ship_slot, trading_item_id }   # 文案「鲨鱼吃了%s」
Ships.add_hull(ship, -(shark_hull_base + uniform(0, shark_hull_rand)))  # 原版 rand(5)+3；符号：函数是 fn_14000c202，战斗里同款曾给船体加值，方向 [未知]
# 玩家选：
kill    → 无幼鱼
release → 无幼鱼
keep    → emit GrantItem { item_id: baby_shark }              # 4
           emit NewTradeGoodCandidate { hint_key: shark_fry, trading_item_id: shark_fin }
```

`keep` 不把鱼翅写进港口特产品槽。种植 / 特产品解锁是 Economy + Items。

### 4.6 哗变

```
emit MutinyStart { fleet_id, choices: [suppress, persuade, ignore] }
suppress:
  if not Characters.has_post(fleet_id, charge_captain): fail_need_officer
  else: apply ship_state patch from YAML
persuade:
  drain both supply_kind by YAML factor * crew
  if any supply hit 0: extra penalty persuade_zero_penalty - uniform(0, 20)
ignore:
  YAML 第三枝；原版有函数枝，数值 [未知]
```

冲锋队长是 `post_id`，不是人物 id。Ocean 问 `Characters.has_post`。

### 4.7 老鼠

```
emit MinigameStart { key: rats, fleet_id }
# 若 ModifierSet.rat_auto_clear（金黄色的猫）：改发 MinigameWon { key: rats, auto: true }
# View 参数只在 town/minigames.yaml：seconds=30, slots=20, click_id [0x37, 0x56)
on MinigameWon  → happening_state = ready；继续航海
on MinigameLost → happening_state 保持老鼠；发出 MustDock { reason: rats }
```

Ocean 不读 `CRatGameView` 字段。

### 4.8 海豚

```
emit LogbookOffer { fleet_id, log_key: dolphins }
accept → Save 记一条；拒 → 无状态
```

无战斗、无货损。海豚船首像「看见更多」是探索半径，不是本表。

### 4.9 粮食

没有独立 spoilage 槽。配置仍留 `food` 段，只描述**会扣粮的结局**，避免以后有人按「大航海常识」再加腐烂骰：

- `rats`：持续态 + `HappeningPersist` → Fleets 扣 `supply_kind=food`
- `mutiny.persuade`：立刻扣水粮
- 航行日步正常消耗：`01-sailing.md`，本表不复制公式
- 小猪像：`ModifierSet.food_use_factor`，Items 提供

## 5. 耦合分析

原版：

- 日步、补给、happening、哗变菜单挤在同一个 4KB 函数 `fn_140005a15` 里
- 白鲸像按 item_id 117 硬判断；风暴保护按物品虚函数返回 9
- 捉鼠直接 new `CRatGameView`
- 鲨鱼幼鱼、鱼翅、发现物种植共用同一段「新交易品」话
- RTC 名 `eHappening` 被 `num_values` 收成 5，和 20 槽、海战 5 命令缠在一起
- `02-search-encounters.md` 曾把本表缩成 `encounters.yaml` 的 6 行 kind——那是探索文档的索引，**正式表是本文件的 `happenings.yaml`**

重构：

- Ocean 只掷骰、只发 ID
- Characters 订阅疾病；Fleets/Ships 订阅船体和补给；Items 提供 ModifierSet；town-life 订阅老鼠；Economy 订阅幼鱼候选；Save 订阅日志
- Combat 的 5 条 happening 继续待在 `config/combat/ai.yaml`
- 探索命中村/遗迹继续待在 `config/ocean/search.yaml`

## 6. 解耦后的配置

### `config/ocean/happenings.yaml`

```yaml
schema: dk4.ocean.happenings
version: 1
ready_state: 27                 # fleet+0x30；可再掷
miss_id: 21
force_slot: 2                   # 两查询失败时 1/3 强行
slots: 20
tables:
  - key: near_port              # days < table_split
    cdf: [30, 65, 153, 158, 159, 165, 166, 168, 171, 172, 173, 174, 178, 181, 185, 191, 194, 196, 198, 200]
  - key: long_voyage
    cdf: [20, 50, 139, 144, 145, 151, 152, 154, 157, 158, 159, 160, 164, 167, 171, 191, 194, 196, 198, 200]
happenings:
  - { id: 0,  key: storm,              event: StormHit,            protect: storm_protect, min_days: 15 }
  - { id: 1,  key: storm_unprotected,  event: StormHit,            min_days: 15 }
  - { id: 2,  key: storm_or_blizzard,  event: StormHit,            climate: [2, 3], ice_outcome: blizzard, no_ice_outcome: storm }
  - { id: 3,  key: climate_3_or_5,     climate: [3, 5],            unverified: true }
  - { id: 4,  key: high_lat,           lat_split: 55,              unverified: true }
  - id: 5
    key: rats_or_scurvy
    unverified: true            # 鼠 / 坏血病尚未按气候钉死
    gates:
      - { climate: [2, 3], event: ScurvyOutbreak }
      - { abs_lat_ge: 50, event: MinigameStart, minigame: rats }
  - { id: 6,  key: long_fx,            lat_split: 45,              unverified: true }
  - { id: 7,  key: wreck_danger,       event: WreckThreat,         protect: wreck_protect }
  - { id: 8,  key: wreck_or_leak,      event: HullLeak,            unverified: true }
  - { id: 9,  key: dolphins,           event: LogbookOffer,        log_key: dolphins }
  - { id: 10, key: plague,             event: PlagueOutbreak }
  - { id: 11, key: siren_or_bubble,    event: SeaHazard,           unverified: true }
  - { id: 12, key: named_sight_a,      event: NamedSighting,       unverified: true }
  - { id: 13, key: named_sight_b,      event: NamedSighting,       unverified: true }  # 美人鱼最大嫌疑
  - { id: 14, key: named_sight_c,      event: NamedSighting,       lat_split: 55, skip_if_flag: 1 }
  - { id: 15, key: mutiny,             event: MutinyStart }
  - { id: 16, key: shark,              event: SharkEncounter }
  - { id: 17, key: sudden_storm,       event: StormHit,            protect: storm_protect }
  - { id: 18, key: crew_fatigue,       event: CrewUnrest }
  - { id: 19, key: fog_or_calm,        event: WeatherHalt }
mutiny:
  choices: [suppress, persuade, ignore]
  suppress_requires_post: charge_captain
shark:
  grant_item: baby_shark          # eItem 4
  trade_candidate: shark_fin      # eTradingItem 21
food:                             # 不是独立槽；列出会扣粮的结局
  drain_on: [rats, mutiny_persuade]
  no_spoilage_slot: true
rats:
  minigame: rats                  # 规则在 town/minigames.yaml
  auto_clear_modifier: rat_auto_clear
```

### `config/formulas/ocean.yaml`（happenings 段）

```yaml
happenings:
  daily_roll_max: 30
  happening_roll_max: 200
  table_split_days: 30
  ticks_per_day: 48
  force_slot_roll: 3
  min_days_storm: 15
  mutiny_days: 20
  mutiny_hard_days: 40
  mutiny_need_stat: 50            # fleet+0x71；语义未钉
  ice_lat_always: 70
  ice_lat_winter: 60
  ice_lat_deep: 45
  ice_month_hi: 10                # >=10 或 <=2 算冬（60°）
  ice_month_hi_lo: 2
  ice_month_deep: 11
  ice_month_deep_lo: 1
  lat_split_55: 55
  lat_split_50: 50
  lat_split_45: 45
  shark_hull_base: 3
  shark_hull_rand: 5
  status_durations: { 7_short: 10, 7_mid: 50, 7_long: 150, 7_very: 200, 7_max: 300, b_30: 30 }
  persuade_zero_base: -30         # 0xFFFFFFE2
  persuade_zero_rand: 20
  white_whale_item_id: 117        # 仅原版包对照；运行时走 wreck_protect
  storm_protect_effect: 9         # 仅原版包对照；运行时走 storm_protect
```

地点、道具名、交易品实例不进这两个文件。

## 7. 独立旋钮

- 两张 CDF、槽数、日步 1/30、出航 30 天分表
- 风暴最低天数、哗变 20/40 天、冰盖纬度与月份
- 老鼠是否必须玩小游戏（改 `rat_auto_clear` 或把槽 5 的 event 换成自动成功率）
- 海豚是否强制写日志
- 鲨鱼幼鱼能否成为交易品（Economy 开关）
- 白鲸 / 虎鲸 / 猫 / 莱姆只改 Items 的 ModifierSet，不改本表代码
- 把槽 13 的 key 从 `named_sight_b` 改成 `mermaid` 只改 YAML——前提是名字表还原之后

加一种海上事件 = 在 `happenings.yaml` 加一行 + 文案 key + 必要时一个 `outcome` 处理器。不要在 `COceanScene` tick 里再写 `if (rand() < 30)`。

## 8. 对外接口

事件：

```
HappeningRolled     { fleet_id, happening_id, outcome_id }
HappeningPersist    { fleet_id, outcome_id }
StormHit            { fleet_id, sea_area_id, kind: storm|blizzard|sudden }
ScurvyOutbreak      { fleet_id }
PlagueOutbreak      { fleet_id }
MinigameStart       { key: rats, fleet_id }
MinigameWon / Lost  { key: rats, fleet_id, auto?: bool }
LogbookOffer        { fleet_id, log_key }
SharkEncounter      { fleet_id, ship_slot }
CargoLost           { fleet_id, ship_slot, trading_item_id? , supply_kind? }
NewTradeGoodCandidate { hint_key: shark_fry, item_id, trading_item_id }
GrantItem           { fleet_id, item_id }
MutinyStart         { fleet_id, choices }
CrewUnrest          { fleet_id }
WreckThreat         { fleet_id }
HullLeak            { fleet_id, ship_slot? }
NamedSighting       { fleet_id, outcome_id, name_id? }
SeaHazard           { fleet_id, key }
WeatherHalt         { fleet_id, key: fog|calm }
MustDock            { fleet_id, reason }
```

查询：

```
Ocean.days_at_sea(fleet_id) -> int
Ocean.happening_state(fleet_id) -> outcome_id | ready
World.climate_at(x, y, date) -> { climate_byte, month }
World.wind_at(...)                 # 无风结局可问是否已是 0
Characters.has_post(fleet_id, post_id) -> bool
Characters.modifier_set(person_or_fleet) -> storm_protect / wreck_protect / scurvy_immune / plague_immune / rat_auto_clear
Fleets.live(fleet_id) -> snapshot | none
```

命令：

```
Ocean.logbook_accept / logbook_reject { fleet_id, log_key }
Ocean.mutiny_choose { fleet_id, choice: suppress|persuade|ignore }
Ocean.shark_choose { fleet_id, choice: kill|release|keep }
```

Ocean 不调用 `new CRatGameView`。不 `TradingItems.insert`。不 `Person.physical_state =`。

## 9. 未核实字段

| 项 | 依据 | 不要写成 |
| --- | --- | --- |
| `NUM_HAPPENING=5` | json 启发式，掷骰函数无立即数 5 | 海上事件只有 5 种 |
| 槽 0–14、17–19 的玩法名 | 只有门闩和文案邻接 | 「槽 0 确认是暴风雨」 |
| 槽 5 = 老鼠还是坏血病 | 气候分枝未对照台词 | 只发捉鼠 |
| 槽 9 = 海豚 | 只播词、无门闩；海豚句在另一段 mesfile | 确认 |
| 槽 13 = 美人鱼 | `%s` 名字表未还原 | 确认 |
| 槽 7 = 沉船 | 白鲸像「不易沉」+ `fn_140101cf0` | 确认发现一艘沉船可打捞 |
| `fn_140101cf0` | 取某全局对象再查非空 | 「船上有货」已证实 |
| `fleet+0x30 == 27` | 空闲才掷 | 枚举名 `HAPPENING_IDLE` 已在头文件 |
| `fleet+0x2c` vs `+0x34` | 天数用 +0x2c，写戳用 +0x34 | 同一字段 |
| `ticks_per_day=48` | `idiv 0x30` | 确认是小时 |
| `fn_140006c12` / `fn_140003689` | 失败则 1/3 槽 2 | 自动航线确认 |
| 气候字节 0–5 的名字 | 夹紧 0..5 | 0=晴 2=风暴已还原 |
| 效果号 9 | 虚函数返回值 | 一定是虎鲸像、不是戒指 |
| `fn_140003c92` 的 edx | 7 / 9 / 0xB / 8 | 全是岗位 |
| `fn_14000c202` 对鲨鱼船体的符号 | 战斗里同款曾加耐久 | 一定是扣血 |
| 粮食腐烂槽 | mesfile 无句 | 原版有独立腐败骰 |
| 海豚是否在 20 槽内 | 文案偏移远离 0x3b8b | 一定是探索命令而不是 happening |
| `CRatGameView` 工厂谁 call | 无 E8 到 happening 派发 | Ocean tick 直接 new View |
| 跳表 RIP `0x140527ce3` | 落在 exe 配置串上 | 那就是「提督！是%s」的格式串 |
| outcome 21 远航危机的玩法名 | 只见改船态 3/4 | 确认是坏血病 |

对照下一层数据时只改 YAML 的 `key` / `event`，不要把 20 槽再展开成另一套硬编码 `switch`。海战那 5 个命令继续待在 Combat。
