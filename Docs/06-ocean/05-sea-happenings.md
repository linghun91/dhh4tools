# 海上 happening

大地图航行时掷出来的随机事件。结局名表 `.data 0x140686550`（26 项）：坏血病、有老鼠、暴风雨、暴风雪、雨、雪、雷、雾、极光、大墨鱼、怪鱼、鲸、海豚、奇怪的歌声、人鱼、神秘泡泡、龙卷风、大漩涡、暗礁、浮冰、叛乱、混乱、漏水、传染病、无风、鲨鱼。探索圈和 NPC 舰队遭遇在 `02-search-encounters.md`。海战五命令在 `07-combat/03-battle-ai.md`，不要和本表共用 `happening_id`。

原版符号 `eHappening` 只出现在掷骰函数。`num_values.json` 的 `NUM_HAPPENING=5` **不是**这张表的长度。

## 1. 职责边界

本系统管：

- 舰队日步（`DayStarted` 的 `daily` phase `ocean.happening_roll` 步）上的 happening **门闩、掷骰、20 槽派发**
- 槽 → 结局的映射（气候 / 纬度 / 出航天数 / 船首像），结局按 key 引用
- 只发事件：风暴命中、疾病、货损、日志邀请、交易品候选、哗变选项；小游戏走命令 `Minigames.run`
- 持续态（老鼠还在船上、哗变未平）如何让后续日步继续扣补给——**发状态事件**，不自己改货舱

不管：

- 捉鼠规则、30 秒、20 只舱室槽（`CRatGameView` 属 `town/minigames.yaml`）；Ocean 只在结局 Action 里调 `Minigames.run(rat) -> int`
- 航海日志怎么存（Save）；只发 `LogbookOffer`
- 幼鱼进不进交易所（Economy 收 `NewTradeGoodCandidate`，已有交易品鱼翅 id 21、发现物小鲨鱼 id 4）
- 坏血病 / 传染病怎么改 `physical_state`（`03-characters/07-fatigue-health-mood.md` 订阅）
- 日步正常扣水粮、疲劳日增量（`01-sailing.md` §2 / §4.4；happening 态只作为它们的输入）
- 气候场怎么生成（`01-world/04-climate-weather-zodiac.md`）；本系统读当前气候字节和坐标
- happening 态对人物 HP / 身体的每日影响（`03-characters/07` §2.11 读 `Ocean.happening_state`）
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

1. `fn_14000b87a(fleet)` 读 `fleet+0x30`。不等于 `0x1B=27` 则整段跳过。`[推断]` 27 是「可掷 happening」空闲态；命中后 `fn_14000a8fd` 把 `+0x30` 写成 `outcome_id`，直到入港或持续态结束才回到 27。`fleet` 实为 `CPlayerFleetData`（`fn_1401ab9e0()`；`03-cabins` 记录其 `+0x30` 初值 `0x1B`）。同一字段另有读者 `[确认]`：航速 / 取风在 `== 0x18`（无风）时风速置 0；`CWorld::vfunc_04` 在 `== 0x1B`，或 `== 4`（雨）且 `rand(5)==0` 时重掷风力档 `fn_1402abb20`（见 01 §4.2）；日耗（态 1、2–5）、疲劳日增量（跳表）、人物日结（`03-characters/07` §2.11）都按它分枝。
2. `rand(30)==0` 才继续。日步期望 1/30。
3. 两个查询 `fn_140006c12`、`fn_140003689` 都为 0 时，才把 `fn_140012e36(fleet)` 的返回值交给 `fn_1400f2d90`。否则 `rand(3)==0` 时**强行槽 2**，不掷表（`0x1400fb77b`–`0x1400fb789`，初值 21 = 无事）。两查询读当前 `SceneDesc`（`DAT_144296500+8`）的 `+0xC` / `+0x10`（原始指令 `0x140092d4b` / `0x1400f351b`）。`COceanScene::vfunc_02` 用 `+0x10==1` 选自动航线跳 `fn_140008814`，用 `+0xC==1` 走海战 tick → `[推断，强]` 前者自动航行、后者大地图海战态。即：**手动航行**才掷 20 槽表；自动航行 / 海战中每日只有 `1/30 × 1/3` 的强制槽 2（再过气候门）。
4. 掷出 21 则结束。
5. `fn_1400f55d0(slot, &outcome)`：槽合法且门闩通过则返回 1，并**回写** `outcome`（初值 27）。返回 0 视为本轮无事。
6. 按 `outcome` 三路特判，其余走通用效果：

| `outcome` | 函数 | RTC / 可见副作用 |
| ---: | --- | --- |
| 20（`0x14`） | `fn_1400f6300` | `eShipSupply, eFleetShip, aszMenu`；三选一「镇压 / 说服 / 置之不理」 |
| 21（`0x15`） | `fn_1400f5460` | 无 RTC；对每船 `rand(10)+10` 两次后改船态 3 或 4 |
| 22（`0x16`） | `fn_1400f6090` | `acFleetShip`；随机一艘，可能咬货、船体 `rand(5)+3` |
| 其它 | `fn_1400ec260(outcome, ships)` 然后 `fn_14000a8fd(fleet, outcome)` | 前者按结局对船上人物做精神事件转移（逐人 `fn_14000c4a5` / `fn_14000342c`，`03-characters/07` §2.11-6），不是改船态；后者把 `+0x30` 与时间戳 `+0x34` 写上 |

函数末尾无论本轮有没有掷中，都会再 `fn_1400f0450(state, ships)`（RTC `anSupply`）。`[推断]` 持续 happening 态在日步里继续扣补给，老鼠/疾病不是「播一句就结束」。

### 2.4 出航天数 `[确认]`

`fn_140012e36`（body `0x1401df010`）：

```
days_at_sea = (now - fleet[+0x2c]) / 48
```

`now` 来自 `fn_1400104b5`。48 = 一天的拍数 `[确认]`（`01-world/07`，`caps.calendar_ticks_per_day`）。`+0x2c` 相对 `fn_14000a8fd` 写的 `+0x34` 不是同一格：`[未知]` 哪一格是「离港时刻」、哪一格是「上次 happening」。配置里参数名仍叫 `days_at_sea`，除数 `ticks_per_day=48`。

同一返回值还做：

- 选 CDF 表：`>= 30` 用表 1
- 槽 0、1（坏血病、有老鼠）：`< 15` 则 mapper 失败（出航不满 15 天不得）
- 槽 15：`>= 40` → outcome 21；`>= 20` 且 `fn_14000ee12(fleet) >= 50` → outcome 20；否则失败。`fn_14000ee12` 读 `CFleet+0x71`；舰队情报面板把同一值画在 RTC `posTired` / `rcTiredTitle`（`fn_140005f51`）→ `[推断，强]` 舰队疲劳。即「出航 ≥20 天且疲劳 ≥50 才哗变」。航速里同一字节 `< 50` 才划桨（01 篇）

### 2.5 纬度、月份、冰盖 `[确认]`

坐标由 `fn_14000a6fa` 填 `nLatitude` / `nLongitude`（本函数 RTC 名）。`fn_140008dff` 就是绝对值，给纬度门用。

`fn_1400f95c0`（RTC `posCrust`）按 `|lat|` 与月份（`fn_140009c19`）判「冰」：

| 条件 | 结果 |
| --- | --- |
| `|lat| >= 70` | 冰 |
| `|lat| >= 60` 且月份 `<= 2` 或 `>= 10` | 冰 |
| `|lat| >= 45` 且月份 `<= 1` 或 `>= 11` | 冰 |
| 其它 | 非冰 |

槽 2 在气候字节 ∈ {2, 3} 时：冰 → outcome 5（雪），否则 outcome 4（雨）。高权重 44% 被气候门大部分打成空弹。

气候字节：`fn_1400010f5` → `fn_140101330`，读 `fleet+0x14` 的 1 字节。取值 0–5 与 `fn_140078640` 把气候夹到 `0..5` 一致 `[推断]`。

### 2.6 结局名表 `[确认]`，船首像护佑 `[推断，强]`

`fn_140014803`（body `0x1401dd940`）：`0 <= k < 0x1A` 时返回 `.data 0x140686550[k]`（GBK 名串指针），否则返回第二参。26 项：

| id | 名 | key | id | 名 | key |
| ---: | --- | --- | ---: | --- | --- |
| 0 | 坏血病 | `scurvy` | 13 | 奇怪的歌声 | `strange_song` |
| 1 | 有老鼠 | `rats` | 14 | 人鱼 | `mermaid` |
| 2 | 暴风雨 | `storm` | 15 | 神秘泡泡 | `mystery_bubbles` |
| 3 | 暴风雪 | `blizzard` | 16 | 龙卷风 | `tornado` |
| 4 | 雨 | `rain` | 17 | 大漩涡 | `whirlpool` |
| 5 | 雪 | `snow` | 18 | 暗礁 | `reef` |
| 6 | 雷 | `thunder` | 19 | 浮冰 | `ice_floe` |
| 7 | 雾 | `fog` | 20 | 叛乱 | `mutiny` |
| 8 | 极光 | `aurora` | 21 | 混乱 | `chaos` |
| 9 | 大墨鱼 | `giant_squid` | 22 | 漏水 | `leak` |
| 10 | 怪鱼 | `strange_fish` | 23 | 传染病 | `plague` |
| 11 | 鲸 | `whale` | 24 | 无风 | `calm` |
| 12 | 海豚 | `dolphins` | 25 | 鲨鱼 | `shark` |

27（`0x1B`）= 空闲，不在表内。名表下标与 `+0x30` 结局值同一套 `[推断，强]`，四路互证：人物日结 `hap` 0 / 23 → HP −2 且转疾病、2 / 3 → HP −3（`03-characters/07` §2.11）；日耗态 1 粮 ×95%、态 2–5 水除数 +10（F-ADJ-03）；态 24 风速归 0（01 §2）；疲劳表 0 / 13 / 23 +2（01 §2）。

护佑：槽 0 与槽 17 共用 `fn_140003f99(fleet, 9)`（body `0x1400f8b80`）：扫 5 船，跳过空船 `0xD0`（`05-ships-fleet/01` 空船哨兵），船 `vfunc+0x48`（`CShip+0x53` 船首像）`== 9` 则整槽失败。槽 0 = 坏血病、槽 17 = 传染病，船首像 9 = 道具 123 圣母像（`figurehead = item − 114`）→ `[推断，强]` 圣母像防疾病。旧稿写成「虎鲸像 / 风暴保护 / 物品效果号」是误读。重构：`ships/figureheads.yaml` 的 `effects` 向 `fleet.happening_block` 注册 `[scurvy, plague]`，禁止 `switch(id)`。

槽 7（鲸）：`fn_140004bfb(0x75)` 取道具 117（白鲸像），`fn_140013ecb` 为真则失败；再 `fn_140101cf0` 为假则失败。两个谓词语义 `[未知]`，重构做成 `guard`，不写死 item id。

金黄色的猫（id 20）「有老鼠作乱时一定能发挥很大的作用」；莱姆的滴汁（id 21）防坏血病；华陀的中药（id 23）防传染病。这些检查不在 mapper 的 20 个 case 里，`[未知]` 在哪一段（持续态 / 人物侧）。

### 2.7 mapper `fn_1400f55d0` 20 路 `[确认]` 控制流

body `0x1400f55d0`，跳表 `0x1400f5e14`，`slot > 0x13` 失败。先查 `fn_1400104b5` 一类「能否 happening」的守卫，失败直接返回 0。

`fn_140003c92(fleet, post, e0, e1)`（body `0x1401dc760`）`[确认]`：`fn_14000f2d6(fleet, post)` 取岗位持有人，非空则 `fn_140011c34(p, e0, e1, 1)` 加两段经验，返回 1。旧稿的「状态 7 时长 N」是把它误读成持续状态——实为**了望手（岗位 7）得经验 `(0, N)`**。

| 槽 | 结局 | 额外门闩 | 可见调用 |
| ---: | --- | --- | --- |
| 0 | 0 坏血病 | `days >= 15`；船首像 9 则失败 | 台词 |
| 1 | 1 有老鼠 | `days >= 15` | 台词 |
| 2 | 4 雨 / 5 雪 | 气候 ∈ {2, 3}；冰盖 → 雪 | 无直接台词 |
| 3 | 6 雷 | 气候 ∈ {3, 5} | 无直接台词 |
| 4 | 2 暴风雨 / 3 暴风雪 | `\|lat\| >= 55` → 暴风雪；气候 == 4 另播台词 | 了望手经验 (0, 100)（气候 4 枝） |
| 5 | 7 雾 / 8 极光 / 失败 | 气候 0/1/4：`\|lat\|>=50` → 极光，否则失败；气候 2/3 → 雾 | 台词；了望手经验 (0, 150) 极光 / (0, 10) 雾 |
| 6 | 9 大墨鱼 / 10 怪鱼 | `\|lat\| >= 45` → 大墨鱼 | 音效 `fn_14000cf1d(..., 0x1E, 2, 1)`；了望手经验 (0, 300) |
| 7 | 11 鲸 | 见 2.6 | 无直接台词 |
| 8 | 12 海豚 | `fn_140101cf0` 为假则失败 | 无直接台词 |
| 9 | 13 奇怪的歌声 | 无 | 台词 `ecx=0xF` |
| 10 | 14 人鱼 | 无 | 两段台词；了望手经验 (0, 50) |
| 11 | 15 神秘泡泡 | 无 | 台词 + `fn_14000bef6(0x1F8)`；了望手经验 (0, 200) |
| 12 | 16 龙卷风 | 无 | 「提督！是%s！」取 `fn_140014803` 名；了望手经验 (0, 150) |
| 13 | 17 大漩涡 | 无 | 同 12 |
| 14 | 18 暗礁 / 19 浮冰 | `\|lat\| >= 55` → 浮冰；`fn_14000f26d(fleet, 1)` 且暗礁则失败 | 同 12，了望手经验 (0, 100) |
| 15 | 20 叛乱 / 21 混乱 | 见 2.4 天数 / 疲劳 | 不在 mapper 播词，交给 2.8 |
| 16 | 22 漏水 | 无 | 交给 2.8 |
| 17 | 23 传染病 | 与槽 0 汇合（船首像 9） | 走槽 0 的「outcome≠0」枝：船医（岗位 11）经验 (0, 30) |
| 18 | 24 无风 | 无 | 台词 `ecx=5`；航速 / 取风在态 24 时风速置 0 |
| 19 | 25 鲨鱼 | 无 | 与槽 6 同类音效；了望手经验 (0, 30) |

经验立即数（`r9d`）逐条对过原始指令：`0x1400f5733`(0x1E, 岗位 0xB)、`0x1400f586b`(0x64)、`0x1400f598d`(150/10 变量)、`0x1400f5a26`(0x12C)、`0x1400f5b11`(0x32)、`0x1400f5b8d`(0xC8)、`0x1400f5c08`(0x96)、`0x1400f5cc4`(0x64)、`0x1400f5dd4`(0x1E)，`edx` 除首条外都是 7，`r8d` 都是 0。

### 2.8 三个特判结局 `[确认]` 结构

**漏水 outcome 22**（`fn_1400f6090`）：旧稿当成鲨鱼，名表证明是漏水。

- 枚举玩家 5 船进 `acFleetShip`，`rand(n)` 挑一艘
- 旗舰枝：播词 + `fn_140003c92(..., 0xD, 1, 100)` = 船木工（岗位 13）经验 (1, 100)
- 其它船：若货格有物，`fn_14000ca4a` 取货名播词（具体句 `[未知]`）
- 不论哪枝：`fn_14000c202(ship, rand(5)+3)` 扣船体（`fn_14000c202` = 加 `−x`，`07-combat` 已证）；`fn_140009467(fleet, 2)` = 疲劳 +2

**鲨鱼 outcome 25** 走通用效果 `fn_1400ec260`。mesfile `0x122f7`–`0x125ff` 的「杀 / 放生 / 养着」「也许会成为新的交易品」由哪个函数播 `[未知]`。幼鱼是发现物 id 4「小鲨鱼」，对应交易品 id 21「鱼翅」（`09-items/03-discovery-planting.md`）。Ocean **不**插入 105 项表。

**叛乱 outcome 20**（`fn_1400f6300`）：

菜单三句写死在 `0x14054e290`（GBK）：镇压、说服、置之不理。

- 镇压：`fn_14000f2d6(fleet, 8)` 取冲锋队长。没有则失败台词「没有冲锋队长，控制不了水手」（`0x5133`）。有则对各船改态，延迟常数 300 / 500
- 说服：对 5 船 × 2 种补给 `new = max(0, stock − crew/10)`（`13-algorithms/03-adjustment-factors.md` §2.3）。扣到 0 另播词，并施加 `0xFFFFFFE2 - rand(20)`（约 −30～−49）的惩罚值
- 置之不理：第三枝 `sF8==2`

**混乱 outcome 21**（`fn_1400f5460`，`days >= 40`）：无菜单。对每船两个 `rand(10)+10`，按 `fn_14000c4a5` 返回 2/3 改船态 3 或 4。

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
- `fn_14000db89`（RTC `asRatData` 160 字节、`eShipCabin`）：`s158` 每 1000 ms 加 1 到 `0x1E`，剩余 `+0x17c > 0` 才继续。30 是秒 `[确认]`（`08-town-life/03-minigames.md`）。
- 工厂 `fn_1400132f0`（body `0x1401ee910`，RTC `cView`）：播特效号 `0x42`，`call 0x14000268a` 构造 View，`fn_140013a07` 跑完返回胜负，再析构。对工厂的直接 `E8` 不在 happening 派发里——Ocean tick **不 new View**。

重构：结局 `rats` 的 Action（D9）调 `Minigames.run(rat) -> int`，town-life 按 `town/minigames.yaml` 同一套规则表开局。猫道具经 `fleet.rat_auto_clear` 让 Ocean 跳过这次调用、按胜处理，不让 Ocean 认识 `CRatGameView`。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key | 拥有者 |
| --- | --- | --- | --- | --- |
| 掷骰槽 | 0–19 | 未命中 = `null`（原版 21） | `happening` key | Ocean 表 |
| 结局 | 0–25（名表 26 项） | 空闲 = `null`（原版 27） | `outcome` key | Ocean 运行时态 `happening_state`，存档随舰队 |
| CDF 表 | 2 | | `table_index` | 由 `days_at_sea` 选 |
| 海战 happening | 5 条命令 | | `combat.happening_id` | Combat，禁止重名 |
| `eShipSupply` | 2 | | `supply_kind` | Ships |
| `eFleetShip` | 5 | | `ship_slot` | Fleets |
| `eShipCabin` | 31 | | `cabin_id` | Ships；捉鼠热区用 |
| 发现物小鲨鱼 | item 4 | | `item_id` | Items |
| 交易品鱼翅 | trading 21 | | `trading_item_id` | Economy |
| 日志条目 | 无枚举 | | `log_key` | Save；Ocean 只发 key |
| 小游戏 | minigame `rat` | | `minigame_id` | town-life |

`NUM_HAPPENING=5` 不进本系统 caps。槽数、结局数按表长；原版 21（未命中）、27（空闲）只在 `ids/legacy_codec.yaml`（D1）。

## 4. 算法 / 公式

魔数进 `config/formulas/ocean.yaml` 的 `happenings:` 段。槽目录进 `config/ocean/happenings.yaml`。代码只解释表。

### 4.1 日步

```
on DayStarted → daily.ocean.happening_roll { fleet_id }:   # 01 §4.4 第 5 步；在人物日结之后
  fleet = Fleets.live(fleet_id)
  if fleet.happening_state is not null: return       # 原版 != 27 跳过掷骰
  if rand(daily_roll_max) != 0: return               # 原版 30，命中 0
  days = (Clock.now_ticks() - fleet.left_port_ticks) / $caps.calendar_ticks_per_day
  if Session.mode() == manual:                 # 原版两查询都 0
      slot = cdf_pick(tables[days >= table_split], happening_roll_max)
  else if rand(force_slot_roll) == 0:
      slot = force_slot                              # 原版槽 2
  else:
      return
  if slot is null: return
  outcome = resolve(slot, ctx{days, lat, month, climate})   # guard 全过才有结局
  if outcome is null: return
  Actions.run(outcomes[outcome].action, fleet_id)    # D9；内含 set_happening_state

on daily.ocean.happening_persist:                    # 原版 fn_1400f0450，日步末尾
  emit HappeningPersist { fleet_id, outcome: fleet.happening_state }
```

`HappeningPersist` 给补给 / 疾病订阅方扣当日分量；Ocean 自己不 `supply -=`。`fn_1400f0450` 的具体效果 `[未知]`。

### 4.2 CDF

```
cdf_pick(cdf, roll_max):
  r = uniform(0, roll_max)          # 原版 200 → [0, 200)
  for i, threshold in enumerate(cdf):
      if r < threshold: return i
  return null                       # 原版 21
```

内容包直接存原版累积数组。引擎不要先差分再掷——差分只给人看。

### 4.3 槽目录（原版包）

权重是 mass（CDF 差分）。结局名来自名表 `[确认]`；名表下标 = `+0x30` 结局值 `[推断，强]`（§2.6 四路互证）。

| id | mass0 | mass1 | 槽 key | 结局 key | 门闩摘要 | 主要效果（Action effects） |
| ---: | ---: | ---: | --- | --- | --- | --- |
| 0 | 30 | 20 | `scurvy` | `scurvy` | `days>=15`；`fleet.happening_block` 含 scurvy 则无事 | 置态；人物日结 HP −2 / 转疾病（03-characters）；疲劳 +2/日 |
| 1 | 35 | 30 | `rats` | `rats` | `days>=15` | 置态；粮 ×95%/日（F-ADJ-03）；`Minigames.run(rat)` 的原版接法 `[未知]` |
| 2 | 88 | 89 | `rain_snow` | `rain` / `snow` | 气候 2/3；冰 → 雪 | 置态；水除数 +10 |
| 3 | 5 | 5 | `thunder` | `thunder` | 气候 3/5 | 置态 |
| 4 | 1 | 1 | `storm` | `storm` / `blizzard` | `\|lat\|>=55` → 暴风雪；气候 4 另台词 + 了望手经验 | 置态；HP −3；疲劳 +1；水除数 +10 |
| 5 | 6 | 6 | `fog_aurora` | `fog` / `aurora` | 气候 2/3 → 雾；0/1/4 且 `\|lat\|>=50` → 极光，否则无事 | 了望手经验 |
| 6 | 1 | 1 | `sea_monster` | `giant_squid` / `strange_fish` | `\|lat\|>=45` → 大墨鱼 | 音效 0x1E；了望手经验 (0, 300) |
| 7 | 2 | 2 | `whale` | `whale` | 白鲸像谓词、`fn_140101cf0` | — |
| 8 | 3 | 3 | `dolphins` | `dolphins` | `fn_140101cf0` | `LogbookOffer{dolphins}` `[推断]`（文案 `0x11a4f`） |
| 9 | 1 | 1 | `strange_song` | `strange_song` | 无 | 疲劳 +2/日 |
| 10 | 1 | 1 | `mermaid` | `mermaid` | 无 | 了望手经验 (0, 50)；疲劳 −1/日 |
| 11 | 1 | 1 | `mystery_bubbles` | `mystery_bubbles` | 无 | `fn_14000bef6(504)`；了望手经验；疲劳 +1/日 |
| 12 | 4 | 4 | `tornado` | `tornado` | 无 | 了望手经验 (0, 150)；疲劳 +1/日 |
| 13 | 3 | 3 | `whirlpool` | `whirlpool` | 无 | 同上 |
| 14 | 4 | 4 | `reef_ice` | `reef` / `ice_floe` | `\|lat\|>=55` → 浮冰；锁舵位且暗礁则无事 | 了望手经验 (0, 100) |
| 15 | 6 | 20 | `crew_unrest` | `mutiny` / `chaos` | `days>=40` → 混乱；`>=20` 且疲劳 `>=50` → 叛乱 | 叛乱菜单 / 混乱改船态（§2.8） |
| 16 | 3 | 3 | `leak` | `leak` | 无 | 船体 −(rand(5)+3)、可能失货、疲劳 +2、船木工经验 |
| 17 | 2 | 2 | `plague` | `plague` | 同槽 0 护佑 | 船医经验 (0, 30)；HP −2；疲劳 +2/日 |
| 18 | 2 | 2 | `calm` | `calm` | 无 | 风速 0（01 §2） |
| 19 | 2 | 2 | `shark` | `shark` | 无 | 音效 0x1E；了望手经验 (0, 30)；后续选择 `[未知]` |

「/日」项不是结局函数立刻做的，是态持续期间其它子阶段读 `happening_state` 得到的（01 §2 疲劳、§4.4 日耗、`03-characters/07` §2.11）。

### 4.4 冰盖

```
ice = |lat| >= ice_lat_always                                  # 70
   or (|lat| >= ice_lat_winter and month not in open_months_hi) # 60；开放月 3..9
   or (|lat| >= ice_lat_deep   and month not in open_months_lo) # 45；开放月 2..10
```

月份 0/1 起算与原版 `fn_140009c19` 对齐 `[未知]` 是否 0 基。YAML 用「闭区间月份」原样填 2、10、1、11，注释写「与立即数一致，未证 0 基」。

### 4.5 漏水与鲨鱼

```
# leak（outcome 22，fn_1400f6090）
ship = pick_uniform(alive_ships)
if ship is flagship: Progression.grant_exp(Fleets.post_holder(fleet_id, carpenter), add{0: 1, 1: 100}, roll_mode, source: leak)
elif ship has cargo: emit CargoLost { fleet_id, ship_slot, trading_item_id }
Ships.add_hull(ship, -(leak_hull_base + rand(leak_hull_rand)))    # 原版 3 + rand(5)，fn_14000c202 = 扣减
Fleets.add_fatigue(fleet_id, +2, reason: leak)                    # fleets 发 FatigueChanged
```

鲨鱼（outcome 25）走通用效果；「杀 / 放生 / 养着」与幼鱼的接法 `[未知]`。按文案保留接口：`keep → Items.acquire(baby_shark) + NewTradeGoodCandidate{shark_fry}`，`keep` 不把鱼翅写进港口特产品槽。

### 4.6 叛乱 / 混乱

```
emit MutinyStart { fleet_id, choices: [suppress, persuade, ignore] }
suppress:
  if Fleets.post_holder(fleet_id, boarding_chief) is null: fail_need_officer
  else: apply ship_state patch from YAML
persuade:
  for ship, kind: stock = max(0, stock - crew / persuade_crew_div)   # 原版 10
  if any supply hit 0: extra penalty persuade_zero_base - rand(20)
ignore:
  YAML 第三枝；原版有函数枝，数值 [未知]
```

冲锋队长是岗位 key（原版 8），不是人物 id。

### 4.7 老鼠

```
# outcome rats：置态，持续期间粮 ×95%/日（结局 effects 登记 supply.food_stock_pct，F-ADJ-03 读）
if Modifiers.eval("fleet.rat_auto_clear"): won = 1   # 金黄色的猫：不开小游戏
else: won = Minigames.run(rat)                       # Action.effects 里调（D9）；原版接法 [未知]
# View 参数只在 town/minigames.yaml：seconds=30, slots=20, click_id [0x37, 0x56)
won != 0 → happening_state = null；继续航海
won == 0 → happening_state 保持 rats；发出 MustDock { reason: rats }
```

Ocean 不读 `CRatGameView` 字段。

### 4.8 海豚

```
emit LogbookOffer { fleet_id, log_key: dolphins }
accept → Save 记一条；拒 → 无状态
```

无战斗、无货损。海豚船首像「看见更多」是探索半径，不是本表。

### 4.9 粮食

没有独立 spoilage 槽。只有这些结局会扣粮，避免以后有人按「大航海常识」再加腐烂骰：

- `rats`：态持续期间日耗前粮 ×95%（结局 effects 登记 `supply.food_stock_pct override 95`，F-ADJ-03 读）
- `mutiny.persuade`：立刻扣 `crew/10`
- 航行日步正常消耗：`01-sailing.md` §4.4，本表不复制公式
- 小猪像：修正器 `supply.food_div`，`ships/figureheads.yaml` 提供

## 5. 耦合分析

原版：

- 日步、补给、happening、哗变菜单挤在同一个 4KB 函数 `fn_140005a15` 里
- 白鲸像按 item_id 117 硬判断；疾病护佑按船首像 `== 9` 硬判断（`fn_140003f99`）
- 结局效果分散：日耗、疲劳、人物 HP 各自 `switch(+0x30)`，没有一张结局表
- 捉鼠直接 new `CRatGameView`
- 鲨鱼幼鱼、鱼翅、发现物种植共用同一段「新交易品」话
- RTC 名 `eHappening` 被 `num_values` 收成 5，和 20 槽、海战 5 命令缠在一起
- `02-search-encounters.md` 曾把本表缩成 `encounters.yaml` 的 6 行 kind——那是探索文档的索引，**正式表是本文件的 `happenings.yaml`**

重构：

- Ocean 只掷骰、只发 ID
- 结局表是唯一真源：每个结局一行 `Action`（D9）+ 被其它系统按 key 引用的属性（`daily.fatigue`、`daily.hp`）+ 态持续期间的修正器贡献 `effects`（`supply.food_stock_pct`、`supply.water_div`）
- Characters 订阅疾病；Fleets/Ships 订阅船体和补给；船首像 / 道具向 `Modifiers` 注册护佑；town-life 订阅老鼠；Economy 订阅幼鱼候选；Save 订阅日志
- Combat 的 5 条 happening 继续待在 `config/combat/ai.yaml`
- 探索命中村/遗迹继续待在 `config/ocean/search.yaml`

## 6. 解耦后的配置

### `config/ocean/happenings.yaml`

```yaml
schema: dk4.ocean.happenings
version: 2
# 空闲 = null（原版 27）、未命中 = null（原版 21）：只在 ids/legacy_codec.yaml
force_slot: rain_snow            # 非手动模式 1/3 强行的槽；原版槽 2
tables:
  - key: near_port              # days < table_split
    cdf: [30, 65, 153, 158, 159, 165, 166, 168, 171, 172, 173, 174, 178, 181, 185, 191, 194, 196, 198, 200]
  - key: long_voyage
    cdf: [20, 50, 139, 144, 145, 151, 152, 154, 157, 158, 159, 160, 164, 167, 171, 191, 194, 196, 198, 200]
slots:                          # 顺序 = CDF 下标；guard 失败即本日无事
  - { key: scurvy,   guard: [{ days_ge: 15 }, { not_blocked: scurvy }],  outcome: scurvy }
  - { key: rats,     guard: [{ days_ge: 15 }],                          outcome: rats }
  - { key: rain_snow, guard: [{ climate_in: [2, 3] }], pick: [{ if: ice, outcome: snow }, { outcome: rain }] }
  - { key: thunder,  guard: [{ climate_in: [3, 5] }],                   outcome: thunder }
  - { key: storm,    pick: [{ if: { abs_lat_ge: 55 }, outcome: blizzard }, { outcome: storm }] }
  - key: fog_aurora
    pick:
      - { if: { climate_in: [2, 3] }, outcome: fog }
      - { if: { climate_in: [0, 1, 4], abs_lat_ge: 50 }, outcome: aurora }
  - { key: sea_monster, pick: [{ if: { abs_lat_ge: 45 }, outcome: giant_squid }, { outcome: strange_fish }] }
  - { key: whale,    guard: [{ pred: vanilla.whale_gate }, { pred: vanilla.fn_140101cf0 }], outcome: whale }   # [未知] 语义
  - { key: dolphins, guard: [{ pred: vanilla.fn_140101cf0 }],          outcome: dolphins }
  - { key: strange_song,    outcome: strange_song }
  - { key: mermaid,         outcome: mermaid }
  - { key: mystery_bubbles, outcome: mystery_bubbles }
  - { key: tornado,         outcome: tornado }
  - { key: whirlpool,       outcome: whirlpool }
  - key: reef_ice
    pick:
      - { if: { abs_lat_ge: 55 }, outcome: ice_floe }
      - { if: { not: { fleet_flag: helm_lock } }, outcome: reef }
  - key: crew_unrest
    pick:
      - { if: { days_ge: 40 }, outcome: chaos }
      - { if: { days_ge: 20, crew_fatigue_ge: 50 }, outcome: mutiny }
  - { key: leak,    outcome: leak }
  - { key: plague,  guard: [{ not_blocked: plague }], outcome: plague }
  - { key: calm,    outcome: calm }
  - { key: shark,   outcome: shark }
outcomes:                       # 结局 = Action（D9）+ 被他系统引用的日效属性 daily + 态持续期间的修正器贡献 effects（写法同 figureheads.yaml）
  scurvy:   { name_key: ui.happening.scurvy,   action: { effects: [set_state] },
              daily: { fatigue: 2, hp: -2, disease: true } }
  rats:     { action: { effects: [set_state, { minigame: rat }] },        # Minigames.run(rat)
              effects: [ { key: supply.food_stock_pct, stage: override, value: 95 } ] }
  storm:    { action: { effects: [set_state, { exp: { post: lookout, e: [0, 100], when: { climate: 4 } } }] },
              daily: { fatigue: 1, hp: -3 }, effects: [ { key: supply.water_div, stage: add, value: 10 } ] }
  blizzard: { action: { effects: [set_state] }, daily: { fatigue: 1, hp: -3 },
              effects: [ { key: supply.water_div, stage: add, value: 10 } ] }
  rain:     { action: { effects: [set_state] }, effects: [ { key: supply.water_div, stage: add, value: 10 } ] }
  snow:     { action: { effects: [set_state] }, effects: [ { key: supply.water_div, stage: add, value: 10 } ] }
  thunder:  { action: { effects: [set_state] } }
  fog:      { action: { effects: [set_state, { exp: { post: lookout, e: [0, 10] } }] } }
  aurora:   { action: { effects: [set_state, { exp: { post: lookout, e: [0, 150] } }] } }
  giant_squid:  { action: { effects: [set_state, { se: 0x1E }, { exp: { post: lookout, e: [0, 300] } }] } }
  strange_fish: { action: { effects: [set_state, { se: 0x1E }, { exp: { post: lookout, e: [0, 300] } }] } }
  whale:    { action: { effects: [set_state] } }
  dolphins: { action: { effects: [set_state, { logbook_offer: dolphins }] } }   # [推断]
  strange_song: { action: { effects: [set_state] }, daily: { fatigue: 2 } }
  mermaid:  { action: { effects: [set_state, { exp: { post: lookout, e: [0, 50] } }] }, daily: { fatigue: -1 } }
  mystery_bubbles: { action: { effects: [set_state, { exp: { post: lookout, e: [0, 200] } }] }, daily: { fatigue: 1 } }
  tornado:  { action: { effects: [set_state, { exp: { post: lookout, e: [0, 150] } }] }, daily: { fatigue: 1 } }
  whirlpool: { action: { effects: [set_state, { exp: { post: lookout, e: [0, 150] } }] }, daily: { fatigue: 1 } }
  reef:     { action: { effects: [set_state, { exp: { post: lookout, e: [0, 100] } }] } }
  ice_floe: { action: { effects: [set_state, { exp: { post: lookout, e: [0, 100] } }] } }
  mutiny:   { action: { impl: vanilla.mutiny_menu } }      # fn_1400f6300
  chaos:    { action: { impl: vanilla.chaos } }            # fn_1400f5460
  leak:     { action: { effects: [ { hull: { base: -3, rand: -5 }, target: random_ship },
                                   { cargo_loss: { unless: flagship } },
                                   { exp: { post: carpenter, e: [1, 100], when: flagship } },
                                   { fatigue: 2 } ] } }
  plague:   { action: { effects: [set_state, { exp: { post: surgeon, e: [0, 30] } }] },
              daily: { fatigue: 2, hp: -2, disease: true } }
  calm:     { action: { effects: [set_state] }, daily: { wind_velocity: 0 } }
  shark:    { action: { effects: [set_state, { se: 0x1E }, { exp: { post: lookout, e: [0, 30] } }] } }
mutiny:
  choices: [suppress, persuade, ignore]
  suppress_requires_post: boarding_chief
  persuade_crew_div: 10
  persuade_zero_base: -30         # 0xFFFFFFE2
  persuade_zero_rand: 20
shark:
  grant_item: vanilla:baby_shark  # eItem 4（§14-4 全命名空间）
  trade_candidate: vanilla:shark_fin   # eTradingItem 21
rats:
  minigame: rat                   # Minigames.run(rat)；规则在 town/minigames.yaml
  auto_clear_modifier: fleet.rat_auto_clear
```

护佑不写在本表：`ships/figureheads.yaml` 的圣母像（原版船首像 9）注册 `{ key: fleet.happening_block, stage: add, value: [scurvy, plague], scope: fleet_any, stack: once }`；`not_blocked: x` 读 `Modifiers.eval("fleet.happening_block")`。

`daily` 是**被引用的属性**，不是本系统执行：疲劳由 01 §6 `crew_fatigue.tables.by_happening_ref` 读、`hp` / `disease` 由 `03-characters/07` 读。补给不走 `daily`：outcome 级 `effects` 在态持续期间向 `supply.food_stock_pct` / `supply.water_div` 登记贡献，F-ADJ-03 按 key 取值（`13-algorithms/03` §4.5）。01 已改成引用；`03-characters/07` §6 的 HP 基值表建议同样改成引用 `outcomes.<key>.daily`。

### `config/formulas/ocean.yaml`（happenings 段）

```yaml
happenings:
  impl: vanilla.happening_roll   # fn_1400f2d90；控制流重，走 ③
  int_mode: { div: trunc, width: int64 }
  params:
    daily_roll_max: 30
    happening_roll_max: 200
    table_split_days: 30
    force_slot_roll: 3
    ice_lat_always: 70
    ice_lat_winter: 60
    ice_lat_deep: 45
    ice_month_hi: 10                # >=10 或 <=2 算冬（60°）
    ice_month_hi_lo: 2
    ice_month_deep: 11
    ice_month_deep_lo: 1
    leak_hull_base: 3
    leak_hull_rand: 5
# ticks_per_day 归 caps.calendar_ticks_per_day；门闩天数 / 纬度写在 happenings.yaml 的 guard 里
```

## 7. 独立旋钮

- 两张 CDF、槽数、日步 1/30、出航 30 天分表
- 各槽 guard（坏血病 / 老鼠 15 天、叛乱 20/40 天、冰盖纬度与月份、纬度分枝）
- 结局的日效属性（疲劳、HP、补给态）——一处改，三个子阶段同时生效
- 老鼠是否必须玩小游戏（改 `fleet.rat_auto_clear` 或把 `rats` 的 effects 换成自动成功率）
- 海豚是否强制写日志
- 鲨鱼幼鱼能否成为交易品（Economy 开关）
- 圣母像 / 白鲸 / 猫 / 莱姆只改船首像或道具的修正器贡献，不改本表代码

加一种海上事件 = 在 `happenings.yaml` 加一个槽 + 一个结局（Action + 文案 key）。结局表 key 是命名空间字符串，模组可加 `mymod:kraken`。不要在 `COceanScene` tick 里再写 `if (rand() < 30)`。

## 8. 对外接口

事件：

```
HappeningRolled     { fleet_id, happening_id, outcome }   # happening_id = 槽 key；载荷按 06
HappeningPersist    { fleet_id, outcome }
StormHit            { fleet_id, major_sea_id, sea_area_id?, kind: storm|blizzard }   # major_sea_id 必填；sea_area_id 可选、默认 null（§13-2）
ScurvyOutbreak      { fleet_id }
PlagueOutbreak      { fleet_id }
LogbookOffer        { fleet_id, log_key }
CargoLost           { fleet_id, ship_slot, trading_item_id? , supply_kind? }
NewTradeGoodCandidate { hint_key: shark_fry, item_id, trading_item_id }
MutinyStart         { fleet_id, choices }
HullDamaged         { fleet_id, ship_slot, amount, source: leak }
Sighting            { fleet_id, outcome }               # 雾 / 极光 / 鲸 / 人鱼 / 龙卷风…，UI 播名表
MustDock            { fleet_id, reason }
```

查询：

```
Ocean.days_at_sea(fleet_id) -> int
Ocean.happening_state(fleet_id) -> outcome | null      # 03-characters / F-ADJ-03 / 01 疲劳 / world climate 重掷门读
Ocean.outcome(outcome_key) -> { name_key, daily, effects }   # 结局表只读
Climate.weather() -> { kind, until }                   # 当前气候（原版字节 0–5）；月份读 Clock
Fleets.post_holder(fleet_id, post) -> person_id | null
Modifiers.eval("fleet.happening_block" | "fleet.rat_auto_clear", ctx)
Fleets.live(fleet_id) -> snapshot | none
```

命令：

```
Ocean.logbook_accept / logbook_reject { fleet_id, log_key }
Ocean.mutiny_choose { fleet_id, choice: suppress|persuade|ignore }
Ocean.shark_choose { fleet_id, choice: kill|release|keep }
```

结局效果调他系统命令（Ocean 不发对应事件）：

```
Minigames.run(rat) -> int                   # town-life；非 0 = 捉完
Items.acquire(baby_shark)                   # items；鲨鱼 keep
Progression.grant_exp(person_id, add{track: n}, roll_mode, source)   # 了望手 / 船医 / 船木工，person 取 Fleets.post_holder
Fleets.add_fatigue(fleet_id, delta, reason) # fleets 钳 0..100 并发 FatigueChanged
```

Ocean 不调用 `new CRatGameView`。不 `TradingItems.insert`。不 `Person.physical_state =`。

## 9. 未核实字段

| 项 | 依据 | 不要写成 |
| --- | --- | --- |
| `NUM_HAPPENING=5` | json 启发式，掷骰函数无立即数 5 | 海上事件只有 5 种 |
| 名表下标 = `+0x30` 结局值 | 名表 `[确认]`；对齐靠 HP / 日耗 / 无风 / 疲劳四路互证 | 已有源码直接用 `+0x30` 索引名表（`fn_140014803` 的调用者未穷尽） |
| 船首像 9 = 圣母像 | `figurehead = item − 114` 推断 | 道具效果号 |
| 鲨鱼 25 的选择菜单与幼鱼 | 文案在 `0x122f7` 起；播放函数未找到 | 漏水函数 |
| 漏水失货句 | `fn_14000ca4a` 取货名 | 「鲨鱼吃了%s」 |
| 鲸槽的白鲸像谓词 `fn_140013ecb`、`fn_140101cf0` | 只见调用 | 「装了白鲸像不沉船」 |
| `fleet+0x2c` vs `+0x34` | 天数用 +0x2c，写戳用 +0x34 | 同一字段 |
| `fn_140006c12` / `fn_140003689` | 失败则 1/3 槽 2 | 自动航线确认 |
| 气候字节 0–5 的名字 | 夹紧 0..5 | 0=晴 2=风暴已还原 |
| 槽 14 的 `fn_14000f26d(fleet, 1)` | 与锁舵同一位 | 「抛锚」已证实 |
| 粮食腐烂槽 | mesfile 无句 | 原版有独立腐败骰 |
| `CRatGameView` 工厂谁 call | 无 E8 到 happening 派发 | Ocean tick 直接 new View |
| 老鼠结局怎么接到捉鼠游戏 | 只有态 1 与日耗 | Ocean tick 直接 new View |
| 混乱 21 改船态 3/4 的含义 | `fn_14000c4a5` 返回 2/3 | 已还原 |
| `fn_1400f0450` 持续效果 | RTC `anSupply` | 已还原 |
| 猫 / 莱姆 / 中药的检查点 | 不在 mapper 内 | 在 mapper 里 |

对照下一层数据时只改 YAML 的 `key` / `outcome`，不要把 20 槽再展开成另一套硬编码 `switch`。海战那 5 个命令继续待在 Combat。
