# 计谋与话术

港口外交与谍报。24 是**并发槽**，不是 24 个具名招式。exe 能抽出的名字是 **10 个 kind**（话术 6 + 计谋 4）。话术和计谋共用 `CScheme` 槽与到期结算，入口、岗位、技能、解锁门槛不同。

委托目录（69 条）在 [02-requests-schemes.md](02-requests-schemes.md)。本篇不写委托。金币、友好度、份额、舰队位置的加减发给 economy / factions / fleets，本系统只决定「哪条 kind、对谁、何时到期」。

## 1. 职责边界

本系统管：

- `eScheme` 24 槽的分配、到期、清空
- `eSCHEMEKIND` 10 档（0–9）目录：名字、目标形状、入口、技能
- 话术入口（同业公会「发送交涉文书」+ 副官 + `eSkill` 7 说服力）
- 计谋入口（酒馆「确定计划」+ 参谋 + `eSkill` 8 谋略术）
- 发动时的金币门槛、岗位检查、技能门槛、酒馆密谈座位散列、到期日历
- 到期掷骰（谋略对抗）与按 kind 分发效果命令（友好度 / 协议 / 宣战 / 份额 / 诱舰 / 执行人经验）

本系统不管：

| 事项 | 归属 |
| --- | --- |
| 友好度曲线、协议旗、份额三槽 | factions |
| 舰队被诱导后怎么走 | fleets / ocean |
| 副官/参谋是否在岗、技能怎么从六维算 | characters |
| 公会/酒馆是否开门、设施列表 | 设施 [01-facilities.md](01-facilities.md)（「客满」是本系统的座位散列，见 2.6） |
| 宫殿合约、介绍信/推荐信、投资、陈列宝物 | factions / economy；宫殿**不是**计谋入口 |
| 攻城「扰乱敌人」1000 金、「海战不能用策略」 | combat；不是 `CScheme` |
| 69 条委托 | 02 篇 |

## 2. 原版事实

### 2.1 计数

`[确认]`

| 符号 | 值 | 来源 |
| --- | --- | --- |
| `NUM_SCHEME` | 24 | `SchemeID.h`、`num_values.json`、`CSchemeElement` 循环 `>= 0x18` |
| `INVALID_SCHEME` | -1 | `SchemeID.h` |
| `NUM_SCHEMEKIND` | 头文件无整型 | `SchemekindID.h` 只有符号；断言 `0 <= eVal < NUM_SCHEMEKIND` |
| `INVALID_SCHEMEKIND` | -1 | 同上 |
| 空 kind 写入 | `0x0B`（11） | `CScheme::vfunc_04`、`fn_14000cc52` |
| 合法 kind | 0–9 | `fn_1400112cf`：写入后读回 `+0x08`（`movzx` 无符号，`< 0` 分支是死代码），`>= 0xA` 返回 0；`fn_14000cea5` `ja` 若 `> 9`；事件脚本建槽 `fn_14000a83a` 先拒 `kind<0 \|\| kind>=0xA` |

`[推断]` `NUM_SCHEMEKIND = 10`。依据：填充器、事件建槽与结算 switch 都把 0–9 当合法，空槽存 11 而不是 10。11 落在 `< NUM_SCHEMEKIND` 之外，与「空货格用 106、不进 0–104」同类：空值不是枚举成员。

`[确认]` `CWorld+0x182a0` 持有 `CSchemeElement`。ctor `fn_140007dce(..., stride=0x18, count=0x18)`：24 个 `CScheme`，每槽 0x18 字节。来源：`world/CWorld.cpp`、`CSchemeElement.cpp`。

`[确认]` 没有 `GetFixSchemeProc`。World0/World1 DLL 不带 24 条具名模板。24 是运行时槽，开局全空（`vfunc_05` → 写 kind=`0x0B`）。

### 2.2 话术 vs 计谋：同一张表，两套入口

mesfile 和 exe **没有**「话术」二字。资深清单里的「话术」对应公会交涉文书；「计谋」对应酒馆策谋。两边都写成 `CScheme`。

`[确认]` kind 以 6 为界：`fn_140204fc0` 对 kind `>= 6` 返回 1，否则 0。结算 `fn_14000cea5` 在 switch 之后，仅当 kind **不是** covert 时才走「若 force_b == 玩家则通知」分支。

| | 话术 rhetoric | 计谋 scheme |
| --- | --- | --- |
| kind | 0–5 | 6–9 |
| 设施 | 同业公会 `CGuild` | 酒馆 `CBar` |
| 菜单名（SC exe） | 「发送交涉文书」`0x566f98` | 「确定计划」`0x564a38`（JP「策謀の立案」） |
| 岗位 | 副官 | 参谋 |
| 技能 | `eSkill` 7 说服力 | `eSkill` 8 谋略术 |
| 技能说明 | 「辩论的资质，外交谈判能力。」 | 「策略的资质，谍报能力。」 |
| 玩家槽窗 | 4–13（与计谋共用） | 4–13 |
| 发动金 | 1000 | 1000 |
| 目标 UI | `CSelectForceView`（+ 三方时再选一个势力） | `CSelectForceView` / `CSelectTownView` |

`[确认]` 宫殿菜单（SC `0x567e28` 起）：军事投资、使用推荐信、使用介绍信、借出宝物、收回宝物、送礼、离开王宫/总督府。**没有**计谋/文书项。

`[确认]` 海战文案「在海战中不能使用“策略”。」「参谋在海战的时候也无事可做。」（`0x1851b`–`0x1858b`）。攻城「扰乱敌人要花１０００枚左右的金币」（`0x8dfb`）与本表无关。

### 2.3 10 个 kind 名（exe 抽出，不是 24 个）

在 `DK4HD_sc.exe` `.rdata` 连续菜单字串里抽出。JP/TC 对照同一槽。**没有**第 11 个 kind 名；02 篇的 `cut_amity` 是坏话的结果句，不是独立 kind。

公会文书内容（各 exe 的 `.rdata` **文件偏移**：SC `0x566f20` = VA `0x140568120`（`.rdata` 文件偏移 = VA − `0x140001200`），JP `0x5675a0`，TC `0x5671d0`）：

| kind | key | SC | JP | TC |
| --- | --- | --- | --- | --- |
| 0 | `goodwill` | 亲善交欢 | 親善交歓 | 親善交歡 |
| 1 | `alliance` | 缔结协议 | 協定締結 | 締結協議 |
| 2 | `break_alliance` | 撕毁协议 | 協定破約 | 撕毀協議 |
| 3 | `ceasefire` | 停战 | 停戦 | 停戰 |
| 4 | `declare_war` | 宣战布告 | 宣戦布告 | 宣戰布告 |
| 5 | `intimidate` | 威压 | 威圧 | 威壓 |

酒馆计谋（文件偏移：SC `0x5649e8` = VA `0x140565be8`，JP `0x565050`，TC `0x564c98`）：

| kind | key | SC | JP | TC |
| --- | --- | --- | --- | --- |
| 6 | `lure_fleet` | 引导舰队 | 艦隊誘導 | 誘導艦隊 |
| 7 | `forge_document` | 伪造假文书 | 偽文書制作 | 製作假文書 |
| 8 | `rumor` | 坏话 | 悪い噂 | 壞話 |
| 9 | `bribe` | 收买 | 買収 | 收買 |

kind 编号 `[确认]`：十个填充包装器各把立即数写进 `fn_1400112cf` 的 `edx`：0=`fn_14000191f`（`xor edx,edx`）、1=`fn_1400130a2`、2=`fn_140011851`、3=`fn_14000a6b9`、4=`fn_14000d53a`、5=`fn_140012035`、6=`fn_1400011b8`、7=`fn_14000e0bb`、8=`fn_14000cb5d`、9=`fn_140007581`。事件建槽 `fn_14000a83a` 的 0–9 跳表（`0x1400e9434`）按同一顺序调这十个包装器。

kind ↔ 名字 `[确认]`：公会文书菜单 `fn_14000a4f2` 的 7 项依次是 亲善交欢（栈内联字串）、`0x140568130` 缔结协议、`…140` 撕毁协议、`…14c` 停战、`…158` 宣战布告、`…164` 威压、`0x140536b54` 取消；`fn_14000808a` 把选中下标 0–5 原样当 kind 传给 `fn_14000a1ff`，后者 kind 0/2/3/4 调上面对应包装器，1→`fn_140002383`→`fn_1400130a2`，5→`fn_14000983b`→`fn_140012035`。酒馆计谋菜单 `fn_140006b0e` 的 5 项依次是 引导舰队（内联）、伪造假文书、坏话、收买、取消；`fn_1400110bd` 用 `选中下标 + 6` 当 kind（`fn_1400019d3`：6→`fn_14000c94b`、7→`fn_140011d0b`、8→`fn_140001c1c`、9→`fn_14001072b`）。

`[确认]` 同表后紧挨着的「喝一杯 / 请喝酒 / 募集水手 / 离开酒馆」和「购买宝物 / 接受委托 / 离开同业公会」是设施菜单，不是 kind。

### 2.4 24 槽三分窗

`fn_140005416(element, start, end)` 在 `[start, end)` 里找第一个空槽（`vfunc_04==0`）。

| 窗 | 调用 | 谁用 |
| --- | --- | --- |
| 0–3（4 格） | `fn_140009b79` → `fn_140005416(..., 0, 4)` | `[确认]` 事件脚本：`fn_14000a83a`（body `0x1400e9130`）是 `fn_140008a08` 13 路命令 switch 的第 12 路，上游 `fn_140011a86`（RTC `sRefData` `sUpdateData`）。0–3 无空位时退到 `fn_140005416(..., 0, 0x18)` 全表找空 |
| 4–13（10 格） | `fn_140014f74` → `(..., 4, 0xE)` | `[确认]` 玩家：公会 `fn_14000808a`、酒馆「确定计划」`fn_1400110bd` |
| 14–23（10 格） | `fn_140010df7` → `(..., 0xE, 0x18)` | `[确认]` NPC 势力 AI：`fn_140005489`（body `0x140107990`，RTC `eThisForce` `asBribe` `aeThirdForce`，10 个调用点）与 `fn_14000c2f7`（body `0x140106930`，`aeWarForce`）。旧稿写的 `fn_140107150` 是同名 cpp 分块里另一个 111 字节函数 |

满窗：公会走消息 `0x531`；酒馆走 `DAT_14055ee10`。玩家话术和计谋**抢同一 10 格**。事件建槽满了会去占 4–23，可挤掉玩家/NPC 的空位。

势力被消灭时 `fn_140003788` 扫全部 24 槽，若 `force_a` / `force_b` / `force_c` 命中则清空。

### 2.5 `CScheme` 布局

对象 0x18 字节。序列化 `vfunc_07`/`vfunc_06`：`+0x08`–`+0x0D` 六个字节 + `+0x10` 一个 dword。

| 偏移 | 宽 | 用途 | 空哨兵 | 标记 |
| --- | --- | --- | --- | --- |
| `+0x00` | 8 | vptr | | `[确认]` |
| `+0x08` | 1 | kind | `0x0B` | `[确认]` |
| `+0x09` | 1 | `force_a` | 随 kind；清槽时与势力 id 比较 | `[确认]` 比较；语义 `[推断]` 发起方 |
| `+0x0A` | 1 | `agent_person_id` 执行人 | 无字节级哨兵；岗位空（`0xEC`）时玩家入口已禁用 | `[确认]`。公会路径传副官（`fn_140010cee(...,1)`→`fn_140012bca`），酒馆路径传参谋（岗位 2），事件路径传 `fn_1400098a9(势力)` `[推断]` 首领。到期 apply 用 `fn_14000ec23` 读回、`fn_140001861` 取人物，拿他的谋略术掷骰并加经验。读取器 `fn_140205690` 只是另一枚举类型的字节 getter，不是 clamp |
| `+0x0B` | 1 | `force_b` | | `[确认]` 比较；`[推断]` 对手/盟友 |
| `+0x0C` | 1 | `town_id` | `0x62`（98=`NUM_TOWN+1`，与 `CSelectTownRouteMap` 未选港相同） | `[确认]` |
| `+0x0D` | 1 | `force_c` | `0x1A`（26，空势力） | `[确认]` |
| `+0x10` | 4 | 到期日历 | | `[确认]` 见 2.8 |
| `+0x14` | 4 | 垫到 0x18 | | `[推断]` |

包装器写入的目标形状 `[确认]`（`0x1A`/`0x62` 表示该槽不用）：

| kind | force_c | town | 目标 |
| --- | --- | --- | --- |
| 0 亲善、2 撕毁、3 停战、4 宣战、5 威压 | 空 | 空 | 势力↔势力 |
| 1 缔结协议 | **用**（共同敌人） | 空 | 三势力 |
| 7 假文书 | **用** | 空 | 三势力（离间） |
| 6 引导、8 坏话、9 收买 | 空 | **用** | 势力+港口 |

`[确认]` 填充签名 `fn_1400112cf(slot, kind, force_a, agent_person, force_b, delay_days, town, force_c)`；包装器签名 `(slot, force_a, agent_person, force_b, delay_days[, town | force_c])`。

`fn_140004976` 按 kind + force_a + force_b +（可选 force_c）+（可选 town）查重。唯一调用者 `fn_1400137cd`（body `0x1400c7f00`，3 处）。

### 2.6 入口与前置

**酒馆** `[确认]`

- 文案：「如果参谋在，可在酒馆里拟定策略。」`0xc237`。岗位文档 `has_staff_officer`。
- 酒馆主菜单 `fn_14000aa92`（body `0x140153750`，RTC `nMonth` `nDay` `nDummyTime` `aeDummyForce` `sMenuOption`）第 3 项「确定计划」默认关，四条全满足才开：
  1. 参谋（岗位 2）不是空人 `0xEC`
  2. 金币 `>= 1000`（`cmp 0x3e8; jb`，无符号）
  3. `fn_14000bbdb` 可对势力数 `>= 1`
  4. **密谈座位**：`seat = ((year + month + day) * 47 + commerce + military + town_index * 7) % 600`，参谋谋略术 `> seat` 才开。`year/month/day` 来自 `fn_140002068` / `fn_140007e69`；`commerce/military` = `fn_14000bc03(town, 0/1)`（港口发展度）；`town_index` = `fn_14000f498(town)` `[推断]` 港口下标。立即数 `0x2f` `7` `0x258`。
- 客满：「提督，客人都满了，好像没有座位能密谈。改天吧。」`0x9783`（同块多条变体，到 `0x9867`）。`[推断]` 就是第 4 条不满足时的台词：同港同日结果固定，换日或谋略术更高才可能开，不是 kind 冷却，也不是真实占座。
- 计谋子菜单 `fn_140006b0e` 5 项（4 kind + 取消），逐项解锁 `[确认]`：引导舰队 恒开；伪造假文书 要可对势力 `>= 2`；坏话 要 `fn_1400135cf(参谋, 8, 0x13) >= 150`（`0x96`）；收买 同值 `>= 200`（`0xC8`）。`fn_1400135cf` 是 `fn_14000e1ce` 的取值核，第三参默认 `0x18`、这里传 `0x13`，语义 `[未知]`。
- 选定后：「那么，我来布置。您等着看结果吧。」`0x94df`。效果延后，不是当场结算。
- 写槽成功后 `fn_1400110bd`：参谋经验 `fn_140006e88(参谋, 30, 1)`，扣金 `fn_14001117b(gold, 1000)`。

**公会** `[确认]`

- 「１０００枚金币承办给其他势力传送文书。」`0xac0f`。
- 公会主菜单 `fn_14000b366`（body `0x14015aec0`）第 2 项「发送交涉文书」：金币 `< 1000`（无符号 `jb`）或副官（岗位 1）为 `0xEC` 则关。**没有**酒馆那种座位散列。
- 「我来写交涉文书吧，要写什么内容？」在 `0xad43` 块内（mesfile 偏移是块首，同块多句）。副官在岗（人物系统 `diplomacy_ok`）。
- `fn_14000808a` 一次进门可连写多封：每封后重查「玩家窗有空槽 且 可对势力 `>= 1` 且 金币 `>= 1000`」，满足就回到文书菜单。
- 写槽成功后 `fn_14000a1ff`：副官经验 `fn_140006e88(副官, 30, 1)`，扣金 1000，消息 `0x529`。
- 本海域没有对象：`0xaee3`。
- 信使忙碌：「对不起，现在传送者出去了，再等五六天吧。」`0xafc7`。`[推断]` 5–6 日是公会侧冷却，不是 `CScheme+0x10`。
- 停战也可从公会进（设施文案 `0x18463`），kind 仍是 3，不是另一套表。
- 菜单 7 项（6 个 kind + 返回），`fn_14000a4f2` / `fn_14000f3ad(..., 7)`。

**宫殿** `[确认]` 不是本系统入口，见 2.2。

### 2.7 技能：说服力 vs 谋略术

`fn_14000e1ce(person, eSkill)` 是技能取值（见人物/成长文档）。

**话术菜单解锁** `[确认]`（反汇编逐项看写入 `me[i].enabled`，`i` = kind）：

`fn_14000a4f2` 先取副官（岗位 1）；空人 `0xEC` 则只剩第 0 项。否则读副官 `eSkill` 7，`fn_140014c31(kind)` 返回该 kind 的可选对象势力数：

| kind | 说服力门槛 | 另需 |
| --- | --- | --- |
| 0 亲善 | 无（恒开） | — |
| 1 缔结 | `>= 80`（`0x50`） | 对象数 `>= 2`（三方协议） |
| 2 撕毁 | `>= 100`（`0x64`） | 对象数 `> 0` |
| 3 停战 | `>= 120`（`0x78`） | 对象数 `> 0` |
| 4 宣战 | **无** | `fn_14000529a(...,3)` 真 且 对象数 `> 0` |
| 5 威压 | `>= 200`（`0xC8`） | `fn_1400113e2(...)` 真 且 对象数 `> 0` |

200 只是威压门槛，与 `caps.skill_level_cap=200`（技能式等级项上限）不是同一个量，不要绑在一起。

威压发动检定 `[确认]`：`fn_14000983b` 算 `说服力 + 友好度项 + 势力规模项`，`>= 0xFA`（250）直接写槽；`< 250` 弹警告对话 `DAT_14055f068`，玩家确认后**仍写槽**。250 只换对话，与计谋阈值同类。

**计谋发动检定** `[确认]` 四个酒馆函数都读 `eSkill` 8（参谋 vs 目标势力首领 `fn_14001490c`）：

```
score = tactics_agent - tactics_target_leader / 2     # cdq; sub; sar 1 = 有符号除 2，向零取整
if score <= threshold: 走「难」对话（DAT_14055ee38），玩家确认后仍写槽
else: 走「易」对话
delay_days = rand(5) + 8
```

旧稿写成 `((tactics_other - 8) >> 1)`，`8` 是反编译把前一条 `mov edx,8`（技能下标）误并进来，原指令里**没有减 8**。

| kind | 函数（thunk / body） | threshold |
| --- | --- | --- |
| 6 引导 | `fn_14000c94b` / `0x14014e730` | 20（`0x14`） |
| 7 假文书 | `fn_140011d0b` / `0x14014e910` | 30（`0x1E`） |
| 8 坏话 | `fn_140001c1c` / `0x14014eb40` | 40（`0x28`） |
| 9 收买 | `fn_14001072b` / `0x14014e550` | 50（`0x32`） |

阈值只换对话，不阻止写入。真正成败在到期 apply（2.9）：apply 掷 `rand(span) + threshold`，`roll < score` 才成。所以「难」对话 ⇔ `score <= threshold` ⇔ 到期时若双方谋略术没变则**必败**；「易」只表示有机会。（目标无首领时 apply 把对方项当 0。）

### 2.8 费用、延迟、到期

**发动金** `[确认]` 话术、计谋入口都是 1000。不进 `CScheme+0x10`。02 篇把 `vfunc_04` 对 `+0x10` 的比较当成金币筛，是错的。

**到期** `[确认]` `fn_1400112cf`：

```
now = dword at (fn_140012c79() + 0x10)   # fn_140012c79 = lea 全局 CWorld 0x1442a7b40；+0x10 是打包日历
due = now + delay_days * 0x30            # imul ecx,[rsp+0x68],0x30
scheme[+0x10] = due
```

`fn_140005272(n)` = `rand() % n`（`n<=0` 返回 0）。十条发动路径（公会 0–5、酒馆 6–9）都是 `delay_days = rand(5) + 8` → 8–12。事件建槽的 delay 由脚本数据给。

`[确认]` 48 = 每日拍数 `caps.calendar_ticks_per_day`（日历文档 `07-calendar-and-coords.md` §2.2：打包 `% 48` 是日内拍 `nTime`）。所以 `delay` 单位是**天**，文书/计谋 8–12 天后结算。

`CSchemeElement::vfunc_04`（body `0x1402059e0`）由世界 tick `fn_14000277f`（body `0x1402aad80`）调：

```
# fn_14000277f
cal = 全局 SCalendar 0x1442a7b10  ← 用当前打包重建
if fn_14000906b(cal):            # cal+0x14 (nTime) == 0，即每天第 0 拍
    CWorld+0x182a0 .vfunc_04(cal)

# CSchemeElement::vfunc_04(cal)
s20 = fn_14001424a(CWorld+0x37e8)    # CMobileShipElement+0x1030 != 0x46
for slot in 0..23:
  if empty: skip
  if due > cal+0x18: skip             # SCalendar+0x18 = 打包整数
  if s20==0 或 kind>=6 或 玩家不在 force_a/force_b:
      apply(kind)
  # 否则留槽，等下一天再看
```

`[确认]` 结算一天扫一次，`due <= now` 即到期。`[确认]` `CWorld+0x37e8` 是 `CMobileShipElement`，其 ctor 把 `+0x1030` 写成 `0x46`、紧挨着把 `+0xff8` 写成空港 `0x62`。`[推断]` `0x46 = 70 = NUM_FLEET(69)+1`，是该字节的「未设舰队」哨兵（与 `0x62 = NUM_TOWN+1` 同模式）。该字节何时被设、因而玩家参与的话术何时被暂扣，语义 `[未知]`。

apply 末尾一律 `fn_14000cc52` 清空槽。

### 2.9 到期分发

`fn_14000cea5` 按 kind 0–9 jump table：

跳表 `0x140201220` 十项逐一核过 `[确认]`。switch 后：kind `< 6` 且 `force_b == 玩家势力` 时取 `force_a` 首领调 `fn_140006b8b(person, 1, 1)`（`[推断]` 通知/见面）；最后一律 `fn_14000cc52` 清槽。

| kind | apply | 体积 | 立即数（`rand` 跨度 / 经验 / 友好度调用数） | RTC / 观察 |
| --- | --- | --- | --- | --- |
| 0 亲善 | `fn_1400136ab` `0x140202870` | 795 | rand 2, 200；`fn_1400089f9` ×2 | 消息 0x174–0x177、0x18a |
| 1 缔结 | `fn_140004705` `0x1402031c0` | 1707 | rand 200, 10, 3；经验 100、150；友好度 ×2 | |
| 2 撕毁 | `fn_140004471` `0x1402012b0` | 643 | rand 10；经验 50；友好度 ×1 | 仅当 `fn_140012dc8(a,b)==1`（协议关系）才生效；`友好度 += rand(10) - 6`（−6..+3）；文案 0x175/0x177/0x17c/0x17d |
| 3 停战 | `fn_14000a858` `0x140201e70` | 1403 | rand 200, 10, 3；经验 150、100；友好度 ×2 | |
| 4 宣战 | `fn_140011680` `0x140202550` | 639 | rand 10；经验 50；友好度 ×1 | |
| 5 威压 | `fn_1400018f7` `0x140202c60` | 1099 | rand 200, 2, 10；经验 70；友好度 ×2 | |
| 6 引导 | `fn_14000f92f` `0x140203a20` | 904 | 掷骰 `rand(50)+20`；经验 50 | RTC `aeFleet`；消息 0x186/0x187 |
| 7 假文书 | `fn_14000421e` `0x140203e90` | 1334 | 掷骰 `rand(70)+30`；后段 rand 200、50 + 读 `eSkill` 7；经验 50×2；友好度 ×1 | 消息 0x188–0x18d |
| 8 坏话 | `fn_1400091ec` `0x140204520` | 1362 | 掷骰 `rand(100)+40`；份额 `−(rand(5)+2)`；经验 70×3；友好度 **0** | 消息 0x18e–0x190 |
| 9 收买 | `fn_140007243` `0x1402015e0` | 1742 | 掷骰 `rand(120)+50`；友好度 `−(rand(5)+1)`；经验 70×3 | RTC `szDstForceName`；`fn_1400122f6` 改份额 |

「经验」= `fn_140006e88(person, n, 1)` → `fn_140011c34(person, 0, n, 1)`：第 0 轨 +0、第 1 轨 +n，夹 `0x493E0`（300000）。旧稿把撕毁里的 `fn_140006e88(..., 0x32)` 当成友好度，是错的；它给执行人（`+0x0A`）加 50 经验。

**计谋到期统一掷骰** `[确认]`（6–9 同构，以收买为例 `0x1402017a0`）：

```
target_leader = 首领(force_b)；无首领则其谋略术记 0
roll  = rand(span) + bias              # 6: 50,20  7: 70,30  8: 100,40  9: 120,50
score = Skill(agent_person, 8) - Skill(target_leader, 8) / 2
success = roll < score                 # cmp roll,score; jge → 失败
```

`bias` 与发动阈值 20/30/40/50 一一相等（见 2.7）。

收买：前置「`force_b` 在该港份额 `> 0`」，否则整条不生效；成功后 `k = rand(5)+1`，经纪人要价（「付钱吗？」，玩家发起时 `fn_140009b1f(0x192)` 确认，金额 `[未知]` 公式，`fn_14001117b` 扣金），`force_b` 份额 `−k`、`force_a` 份额 `+k`、`force_b` 对 `force_a` 友好度 `−k`（`k` 受目标现有份额约束的分支 `[未知]`）；玩家是发起方时消息 0x193/0x194，玩家是目标时 0x195，失败 0x196。执行人、目标首领各 +70 经验，目标是玩家时其参谋也 +70。

坏话：前置同上（`force_b` 在该港份额 `> 0`）；成功 `force_b` 该港份额 `−(rand(5)+2)`（−2..−6）。成功后若 `fn_140005b19` 判为玩家方且该方有参谋（`fn_14000f2d6(...,2)`），再比一次 `rand(Skill(参谋,8))` 与 `rand(fn_1400135cf(参谋,8,0x13))`，决定一条额外消息（`fn_14000ca4a`）`[推断]` 是否察觉造谣；两侧取值细节 `[未知]`。

失败句 mesfile「收买失败了。」`0x3b57`。成功「夺取了%s的占有率。」在 `0x3aeb` 块内。

### 2.10 效果 ↔ kind（文案）

mesfile `0x38c7`–`0x3b77` 结果句、`0x1479f`–`0x1516f` NPC 反应。下表是 10 个 kind 的效果族，**不是** 24 条实例名。

| kind | 成功时 factions/fleets 该做的 | 文案要点 |
| --- | --- | --- |
| 0 亲善 | `AmityChanged +` | 「亲笔信已送到%s。友好度上升%s了。」；拒信「%s拒绝接受亲笔信」 |
| 1 缔结 | `AllianceFormed`（三方：A+B vs C） | 「%s和%s缔结了对抗%s的协议」 |
| 2 撕毁 | `AllianceBroken` | 「协议被解除」「撕毁协议」 |
| 3 停战 | `CeasefireSigned` | 「%s和%s停战了」/拒绝 |
| 4 宣战 | `WarDeclared` | 「%s向%s宣战」 |
| 5 威压 | `ForceAbsorbed` 或拒 | 「%s加入麾下了」/「拒绝了合并的请求」 |
| 6 引导 | 把目标舰队诱到 `town_id` | 「%s收到了假情报。诱导%s好像成功了。」；「把%s的舰队引到%s」 |
| 7 假文书 | 两势力反目 | 「收到了假文书。成功地让%s和%s反目了。」 |
| 8 坏话 | `Share.sub(town, force_b, rand(5)+2)`；**不**调友好度函数 | 「计划好像失败了。」「在%s，流传着%s的坏话。」「这传闻没有被相信。」（`0x3a9f`–`0x3ad7`） |
| 9 收买 | 份额 `force_b −k / force_a +k` + `force_b→force_a` 友好度 `−k`，`k=rand(5)+1`；另扣经纪人要价 | 「经纪人收取了书信」「夺取了%s的占有率」 |

「由于谁的计谋，我们与%s的友好度降低了」（`0x3a67`）`[推断]` 属假文书（kind 7 apply 有友好度调用；坏话没有）。

撕毁 `rand(10)-6`、坏话/收买的份额与友好度立即数见 2.9 `[确认]`。亲善/缔结/停战/威压/假文书里 `rand(200)` 等立即数已列，如何组合成友好度增量 `[未知]`。亲善的「上升%s」是格式化整数。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空/非法 | 重构 |
| --- | --- | --- | --- | --- |
| 槽 | `eScheme` | 24 | -1；空 kind=`0x0B` | `scheme_slot_id` 0–23，与存档下标对齐 |
| 种类 | `eSCHEMEKIND` | `[推断]` 10 | -1；运行时空值 11 | `scheme_kind` key |
| 话术项 | 无独立枚举 | 6（kind 0–5） | | `rhetoric.yaml` 引用 kind key |
| 技能 | `eSkill` | 10 | -1 | 7=`persuasion` 8=`tactics` |
| 岗位 | `ePost` | exe 至少 16 名 | -1 | `adjutant` `staff` |
| 目标势力 | `eForce` | 25 | 26=`0x1A` | `force_id` |
| 目标港 | `eTown` | 97 | 98=`0x62` | `town_id` |
| 执行人 | `ePerson` 字节 `+0x0A` | — | 岗位空 `0xEC` 时入口关 | `agent_person_id` |

禁止用 `id=0` 当空槽（0 是合法下标）。空 = `kind: null`（D1）。

运行时按表长循环，不写死 24/10。原版包：24 槽、10 kind。

## 4. 算法 / 公式

全部进 `config/formulas/town.yaml` 与 kind 表。禁止 `if kind==8` 散落。

```
# 日历。delay 单位是天；每日拍数取 $caps.calendar_ticks_per_day（原版 48），本系统不另存
due = Clock.now() + delay_days * $caps.calendar_ticks_per_day      # D4：原版 48
delay_days = rand(delay_rand_span) + delay_bias    # 5, 8 → 8..12

# 谋略对抗分（发动对话与到期掷骰共用）
tactics_score(agent, target_leader) =
    Skill(agent, tactics) - trunc_div(Skill(target_leader, tactics), tactics_target_divisor)
# tactics_target_divisor=2；无首领时 Skill=0

# 计谋到期（kind 6–9 同构）
roll = rand(kind.resolve_roll.span) + kind.resolve_roll.bias
success = roll < tactics_score(agent, leader(force_b))
# 发动时 score <= kind.cast_hint_threshold → 「难」对话；原版 threshold == bias

# 话术菜单（逐 kind）
rhetoric_unlocked(kind, persuasion) =
    persuasion >= kind.persuasion_min
    AND Factions.count_targets(kind) >= kind.min_targets
    AND extra_ok(kind)          # 宣战 fn_14000529a；威压 fn_1400113e2

# 威压发动提示（不阻止写槽）
intimidate_score = persuasion + amity_term + size_term   # 组合细节 [未知]
if intimidate_score < intimidate_hint_min: 警告对话

can_open_rhetoric(player):
  Fleets.post_holder(fleet, adjutant) != null    # 原版岗位 1；fn_140010cee(...,1) != 0xEC
  AND gold >= letter_cost_gold         # 1000
  AND Schemes.free_in(player_bank) > 0
  AND Factions.has_addressee_in_sea(sea_id)
  AND not guild.courier_busy

can_open_scheme(player):
  Fleets.post_holder(fleet, staff) != null       # 原版岗位 2；!= 0xEC
  AND gold >= plot_cost_gold           # 1000
  AND Schemes.free_in(player_bank) > 0
  AND Factions.has_scheme_target() >= 1
  AND Skill(post_holder(fleet, staff), tactics) > bar_seat_hash(date, town)   # 原版「客满」

bar_seat_hash(date, town) =
  ((year + month + day) * seat_date_mul + commerce + military + town_index * seat_town_mul) % seat_mod
# 47, 7, 600；可整体替换为别的公式或关掉

cast(kind, targets, agent_person, delay_days):
  slot = alloc(bank_for(caster))       # 玩家 4–13；NPC 14–23；事件 extra→全表
  pay gold
  Progression.grant_exp(agent_person, add{1: cast_exp}, once_per_call, source: scheme)    # 原版 30
  write {kind, force_a, agent_person, force_b, force_c|null, town|null, due}
  emit SchemeQueued

on DayStarted:                         # schedule: day_start；原版只在 nTime==0 扫一次
  for slot in 0 .. len(slots)-1:
    if kind == null: continue
    if due > Clock.now(): continue
    if hold_player_rhetoric() and kind.group==rhetoric and player in {force_a, force_b}: continue
    apply_table[kind](slot)            # 只 emit 命令
    slot.kind = null
```

`bank_for`：玩家固定 `player`（原版 4–13）；NPC 固定 `npc`（14–23）；事件脚本 `event`（原版 0–3，满则全表找空）。`hold_player_rhetoric` 对应原版 `CMobileShipElement+0x1030 != 0x46`，语义未钉，做成谓词钩子。

apply 禁止本系统改友好度数字：`Factions.applySchemeEffect { kind, force_ids, town_id, params }`。

## 5. 耦合分析

原版硬编码：

- `CScheme` 是 `CSaveObject`，和城镇、人物同一存档链
- 填充直接写势力/港口字节，结算直接改友好度、份额、舰队
- 技能下标 7/8、阈值 20/30/40/50/80/100/120/150/200/250、掷骰跨度 50/70/100/120、经验 30/50/70/100/150、座位散列 47/7/600、金币 1000、延迟 `rand(5)+8`、乘 48，全是立即数
- 循环上界 24、kind 上界 10、空 kind 11、空势力 26、空港 98 写死
- 公会/酒馆 UI 直接 `new` 选势力/选港 View
- NPC AI（`fn_140005489` / `fn_14000c2f7`）自己挑 kind 往 14–23 填

重构后只发：

```
TownLife.CastScheme → Schemes.queue
Schemes.due        → Factions.applySchemeEffect
                   → Fleets.lureTo(town_id)          # kind lure_fleet
Schemes.queue      → Force.add_gold(force_id, -cost_gold, reason: scheme)   # 原版 1000
```

本系统不读六维原值，只问 `Fleets.post_holder(fleet, adjutant|staff)` 与 `Characters.skill(person, persuasion|tactics)`（技能值，0–500 量纲；原版 eSkill 7/8）。若要加装备 / 状态修正，走 `Modifiers.eval("skill.tactics", ctx)`，不在本篇算。不读市价。不持有 `CForce*`。

允许依赖：`town_id` / `force_id` / `person_id` / `fleet_id`；characters 的岗位与技能查询；world 日历。

## 6. 解耦后的配置

`config/town/scheme_kinds.yaml` — kind 目录（原版包 10 行）：

```yaml
schema: dk4.town.scheme_kinds
version: 1
# 原版空 kind 0x0B、空势力 0x1A、空港 0x62 只在 ids/legacy_codec.yaml；运行时 null
kind_count_vanilla: 10
# apply_exp：apply 里 fn_140006e88 的经验值（第 1 轨），按出现顺序；收买为 执行人/目标首领/目标参谋，其余收件人 [推断]

kinds:
  - key: vanilla:goodwill
    legacy_kind: 0
    group: rhetoric
    name: 亲善交欢
    name_i18n: { ja: 親善交歓, zh-Hans: 亲善交欢, zh-Hant: 親善交歡 }
    entry: vanilla:guild
    requires_post: vanilla:adjutant
    skill: vanilla:persuasion
    targets: [force_a, force_b]
    effect: amity_up
  - key: vanilla:alliance
    legacy_kind: 1
    group: rhetoric
    name: 缔结协议
    entry: vanilla:guild
    requires_post: vanilla:adjutant
    skill: vanilla:persuasion
    targets: [force_a, force_b, force_c]
    apply_exp: [100, 150]
    effect: alliance_form
  - key: vanilla:break_alliance
    legacy_kind: 2
    group: rhetoric
    name: 撕毁协议
    entry: vanilla:guild
    targets: [force_a, force_b]
    apply_exp: [50]
    apply_precondition: relation_is_alliance       # fn_140012dc8(a,b)==1，否则 apply 空跑
    effect: alliance_break
  - key: vanilla:ceasefire
    legacy_kind: 3
    group: rhetoric
    name: 停战
    entry: vanilla:guild
    targets: [force_a, force_b]
    apply_exp: [150, 100]
    effect: ceasefire
  - key: vanilla:declare_war
    legacy_kind: 4
    group: rhetoric
    name: 宣战布告
    entry: vanilla:guild
    targets: [force_a, force_b]
    apply_exp: [50]
    effect: declare_war
  - key: vanilla:intimidate
    legacy_kind: 5
    group: rhetoric
    name: 威压
    entry: vanilla:guild
    targets: [force_a, force_b]
    apply_exp: [70]
    effect: absorb
  - key: vanilla:lure_fleet
    legacy_kind: 6
    group: covert
    name: 引导舰队
    entry: vanilla:bar
    requires_post: vanilla:staff
    skill: vanilla:tactics
    targets: [force_a, force_b, town]
    apply_exp: [50]
    effect: lure_fleet
    cast_hint_threshold: 20      # 原版 == resolve_roll.bias
    resolve_roll: { span: 50, bias: 20 }
  - key: vanilla:forge_document
    legacy_kind: 7
    group: covert
    name: 伪造假文书
    entry: vanilla:bar
    requires_post: vanilla:staff
    skill: vanilla:tactics
    targets: [force_a, force_b, force_c]
    apply_exp: [50, 50]
    effect: feud
    cast_hint_threshold: 30
    resolve_roll: { span: 70, bias: 30 }
    menu_min_targets: 2
  - key: vanilla:rumor
    legacy_kind: 8
    group: covert
    name: 坏话
    entry: vanilla:bar
    requires_post: vanilla:staff
    skill: vanilla:tactics
    targets: [force_a, force_b, town]
    apply_exp: [70, 70, 70]
    apply_precondition: target_share_in_town_gt_0  # fn_1400092af > 0
    effect: rumor_cut_share     # 原版 apply 不调友好度；份额 −(rand(5)+2)
    cast_hint_threshold: 40
    resolve_roll: { span: 100, bias: 40 }
    menu_tactics_min: 150
  - key: vanilla:bribe
    legacy_kind: 9
    group: covert
    name: 收买
    entry: vanilla:bar
    requires_post: vanilla:staff
    skill: vanilla:tactics
    targets: [force_a, force_b, town]
    apply_exp: [70, 70, 70]
    apply_precondition: target_share_in_town_gt_0
    effect: steal_share
    cast_hint_threshold: 50
    resolve_roll: { span: 120, bias: 50 }
    menu_tactics_min: 200
```

`config/town/rhetoric.yaml` — 话术层（与 kind 表分离：解锁、文书金、信使）：

```yaml
schema: dk4.town.rhetoric
version: 1
# 不是第二份 24 行目录。6 行对应 kind 0–5。

facility: vanilla:guild
post: vanilla:adjutant
skill: vanilla:persuasion
cost_gold: 1000
courier_busy_days_min: 5
courier_busy_days_max: 6
menu_i18n: ui.guild.send_letter          # 「发送交涉文书」
intimidate_hint_min: 250                 # fn_14000983b 0xFA；只换对话，不阻止写槽
allow_multiple_per_visit: true           # fn_14000808a 循环

unlocks:                                 # fn_14000a4f2 逐项 [确认]
  - kind: vanilla:goodwill
    persuasion_min: 0                    # 恒开
    min_targets: 0
  - kind: vanilla:alliance
    persuasion_min: 80                   # 0x50
    min_targets: 2                       # 三方
  - kind: vanilla:break_alliance
    persuasion_min: 100                  # 0x64
    min_targets: 1
  - kind: vanilla:ceasefire
    persuasion_min: 120                  # 0x78
    min_targets: 1
  - kind: vanilla:declare_war
    persuasion_min: 0                    # 原版不查说服力
    min_targets: 1
    extra: has_war_target                # fn_14000529a
  - kind: vanilla:intimidate
    persuasion_min: 200                  # 0xC8；与 caps.skill_level_cap 无关
    min_targets: 1
    extra: absorb_possible               # fn_1400113e2
```

`config/town/plots.yaml` — 计谋入口层（酒馆）：

```yaml
schema: dk4.town.plots
version: 1
facility: vanilla:bar
post: vanilla:staff
skill: vanilla:tactics
cost_gold: 1000
menu_unlock_skill_cap_arg: 19            # fn_1400135cf 第三参 0x13，语义 [未知]
seat_check:                              # 原版「客满」；enabled:false 即永远有座
  enabled: true
  formula: bar_seat_hash                 # formulas/town.yaml
```

`config/town/schemes.yaml` — **24 行槽**（原版包定义窗，不定义 24 个名字）：

```yaml
schema: dk4.town.schemes
version: 1
# exe 没有 24 个招式名。每行是并发槽。kind 运行时写入，定义侧全 null。

banks:
  extra:
    start: 0
    end: 4
    owner: event_script                  # fn_14000a83a（事件命令第 12 路）
    overflow: any                        # 满则 0–23 全表找空
  player:
    start: 4
    end: 14
    owner: player                        # 公会文书 + 酒馆计谋
  npc:
    start: 14
    end: 24
    owner: npc_ai

slots:
  - { id: 0,  key: slot_00, bank: extra,  kind: null }
  - { id: 1,  key: slot_01, bank: extra,  kind: null }
  - { id: 2,  key: slot_02, bank: extra,  kind: null }
  - { id: 3,  key: slot_03, bank: extra,  kind: null }
  - { id: 4,  key: slot_04, bank: player, kind: null }
  - { id: 5,  key: slot_05, bank: player, kind: null }
  - { id: 6,  key: slot_06, bank: player, kind: null }
  - { id: 7,  key: slot_07, bank: player, kind: null }
  - { id: 8,  key: slot_08, bank: player, kind: null }
  - { id: 9,  key: slot_09, bank: player, kind: null }
  - { id: 10, key: slot_10, bank: player, kind: null }
  - { id: 11, key: slot_11, bank: player, kind: null }
  - { id: 12, key: slot_12, bank: player, kind: null }
  - { id: 13, key: slot_13, bank: player, kind: null }
  - { id: 14, key: slot_14, bank: npc,    kind: null }
  - { id: 15, key: slot_15, bank: npc,    kind: null }
  - { id: 16, key: slot_16, bank: npc,    kind: null }
  - { id: 17, key: slot_17, bank: npc,    kind: null }
  - { id: 18, key: slot_18, bank: npc,    kind: null }
  - { id: 19, key: slot_19, bank: npc,    kind: null }
  - { id: 20, key: slot_20, bank: npc,    kind: null }
  - { id: 21, key: slot_21, bank: npc,    kind: null }
  - { id: 22, key: slot_22, bank: npc,    kind: null }
  - { id: 23, key: slot_23, bank: npc,    kind: null }
```

`config/formulas/town.yaml` 本篇新增键：

```yaml
# 片段；与旅馆/酒价等其它 town 公式共存
# 延迟单位 = 天；换算拍数读 caps.yaml 的 calendar_ticks_per_day（原版 48），这里不重复存
scheme_delay:                            # delay_days
  impl: expr
  params:
    scheme_delay_rand_span: 5
    scheme_delay_bias: 8                 # → 8..12 天
  expr: "rand(scheme_delay_rand_span) + scheme_delay_bias"

tactics_score:                           # 谋略对抗分（发动对话与到期掷骰共用）
  impl: expr
  params:
    tactics_target_divisor: 2            # score = agent - leader/2（向零取整）；原版没有「减 8」
  refs: { skill: vanilla:tactics }
  inputs: [agent_tactics, leader_tactics]   # 无首领时 leader_tactics = 0
  expr: "agent_tactics - leader_tactics / tactics_target_divisor"

scheme_cast_exp:
  impl: vanilla.scheme_cast_exp
  params:
    scheme_cast_exp: 30                  # 写槽时执行人第 1 轨经验

bar_seat_hash:                           # 酒馆「客满」
  impl: vanilla.bar_seat_hash            # 含取模，expr 沙箱算子表无 %（14 README D3），故走 vanilla
  params:
    date_mul: 47                         # 0x2f
    town_index_mul: 7
    mod: 600                             # 0x258
  inputs: [year, month, day, commerce, military, town_index]
  # ((year + month + day) * date_mul + commerce + military + town_index * town_index_mul) mod mod
# 每 kind 的 resolve_roll / cast_hint_threshold / 解锁门槛在 scheme_kinds.yaml / rhetoric.yaml，不在此重复
# 空人/空港/空势力哨兵属 ids/，不在本文件定义
```

apply 的经验奖励（50/70/100/150）随 kind 进 `scheme_kinds.yaml` 的 `apply_exp`，原版值见 2.9。

存档：24 槽的 kind / force_a / agent_person / force_b / town / force_c / due。kind 名、解锁表、费用不进存档。

## 7. 独立旋钮

- 槽表长（可 >24；NPC/玩家窗按 `banks` 切）
- 增删 kind（新 key + apply 效果名 + 目标形状）
- 话术/计谋费用是否仍同为 1000
- 说服力解锁阶、每 kind 最少对象数、威压提示 250、计谋菜单 150/200、计谋四档阈值
- 每 kind 到期掷骰 `resolve_roll {span, bias}`、对抗除数 2
- 延迟 `rand+bias`（天）；拍数换算跟 world 日历走
- 执行人经验（写槽 30、apply 50/70/100/150）
- 是否必须副官/参谋（`requires_post`）
- 空 kind / 空港 / 空势力哨兵
- 信使 5–6 日、酒馆座位散列（47/7/600）或整个关掉
- 是否保留「玩家参与的话术暂扣」谓词（`hold_player_rhetoric`）

不该出现在本表：友好度公式、份额封顶、市价、六维配对（配对在 `characters/skills.yaml`）。

## 8. 对外接口

事件：

```
SchemeQueued   { slot_id, kind, caster_force_id, agent_person_id, targets[], due }
SchemeResolved { slot_id, kind, success, agent_person_id, targets[] }
SchemeCleared  { slot_id, reason }          # 势力灭亡 / 到期 / 放弃
RhetoricMenuOpened { town_id, unlocked_kinds[] }
```

查询：

```
Schemes.free_in(bank) -> int
Schemes.get(slot_id) -> { kind, due, targets } | null
Schemes.affordable_rhetoric(persuasion, gold, has_adjutant) -> [kind]
Schemes.can_plot(tactics, gold, has_staff, town_id, date) -> bool
Schemes.plot_hint(kind, agent_person_id, force_b) -> easy | hard   # hard = 原版必败
```

命令：

```
CastRhetoric { kind, force_b, force_c? }           # 公会；force_a=玩家势力，agent=副官
CastScheme   { kind, force_b, town_id?, force_c? } # 酒馆；agent=参谋
QueueScheme  { kind, force_a, agent_person_id, force_b, town_id?, force_c?, delay_days, bank } # 事件脚本 / NPC AI
# 到期扫描不是命令：engine/tick.yaml phase `schemes`（schedule: [day_start]）
```

完成时只 emit；factions / fleets / economy 订阅。UI 用 `CSelectForceView` / `CSelectTownView` 的重构等价物，只回传 ID。

## 9. 未核实字段

- `NUM_SCHEMEKIND` 头文件整数（现按 10 推断；所有校验都是 `< 0xA`）。
- `CMobileShipElement+0x1030` 何时被写成非 `0x46`；即「玩家参与的话术被暂扣」在玩法上对应什么状态。
- 话术 apply（0–5）与假文书 apply（7）里 `rand(200)` 等立即数如何组合成成败与友好度增量；亲善「友好度上升%s」的整数公式。
- 威压检定 `fn_14000983b` 中友好度项、规模项的精确组合（含 `idiv`）。
- `fn_1400135cf` 第三参（默认 `0x18`，酒馆计谋菜单传 `0x13`）的语义。
- 酒馆座位散列里 `fn_14000f498(town)` 是否就是 `town_id`。
- 收买经纪人要价的金额公式；`k` 被目标现有份额约束的分支。
- 坏话成功后对玩家的「察觉」比较两侧取值。
- 引导舰队改的是 `fleet_id` 还是该势力「本月定期船」（apply 里 `fn_14000f349(...,3)`）。
- 信使「五六天」是否写入状态，还是纯文案。
- NPC AI（`fn_140005489`）选 kind 的权重。
- `fn_140004976` 查重的调用者 `fn_1400137cd` 属于哪条流程。
- 事件路径 `+0x0A` 填的 `fn_1400098a9(势力)` 是否首领。
- 攻城扰乱 1000 金（mesfile `0x8dfb` / `0x8ea3` 块）的独立表（若有，不进 `schemes.yaml`）。

## 附：mesfile 块偏移 → exe 消息号

按 D10，文案 key 用 exe 消息号，不用 mesfile 字节偏移。正文里 `0x` 开头、落在 mesfile 的数多为 `catalog/mesfile_sc.txt` 的 NUL 块偏移；一块常含多条消息。下表由脚本按 exe 消息号表（`.data 0x140683620` 37 块 `{first_id,size}` + `0x1406836c0` 块内偏移；`.data` 文件偏移 = VA − `0x140001600`）切句，再按正文引号句匹配生成。只给一条的是已匹配到句；给区间的需导出时按句再选。正文里已标「消息」的号（如 `0x58a`、`0xb2e`）本来就是消息号，不在此表。

| 块偏移 | 消息号 | 首句 |
| --- | --- | --- |
| `0x38c7` | `0x175–0x179`（块内多条，按句选） | 亲笔信已送到%s。 |
| `0x3a67` | `0x18b` | 由于谁的计谋，我们与%s的友好度降低了！ |
| `0x3a9f` | `0x18e` | 在%s，流传着%s的坏话。 |
| `0x3ad7` | `0x190` | 这传闻没有被相信。 |
| `0x3aeb` | `0x193` | 夺取了%s的占有率。 |
| `0x3b57` | `0x196` | 收买失败了。 |
| `0x3b77` | `0x198`（块内多条，按句选） | %s和%s缔结了协议。 |
| `0x8dfb` | `0x434` | 扰乱敌人要花１０００枚左右的金币。要采取什么策略 |
| `0x8ea3` | `0x435`（块内多条，按句选） | 如果有１０００枚金币就能扰乱敌人。采取什么样的策 |
| `0x94df` | `0x46f` | 那么，我来布置。您等着看结果吧。 |
| `0x9783` | `0x484` | 提督，客人都满了，好像没有座位能密谈。改天吧。 |
| `0x9867` | `0x489` | 提督，客人都满了，好像没有座位能密谈。改天吧。 |
| `0xac0f` | `0x50c` | １０００枚金币承办给其他势力传送文书。 |
| `0xad43` | `0x522` | 我来写交涉文书吧，要写什么内容？ |
| `0xaee3` | `0x52a–0x52c`（块内多条，按句选） | 提督，这片海域没有可以送交涉文书的势力。 |
| `0xafc7` | `0x531` | 对不起，现在传送者出去了，再等五六天吧。 |
| `0xc237` | `0x5c9` | 如果参谋在，可在酒馆里拟定策略。 |
| `0x1479f` | `0xacf`（块内多条，按句选） | 协议……？ |
| `0x1516f` | `0xb27–0xb2a`（块内多条，按句选） | 好像%s正在%s制造谣言，可是城市里的人们都站在 |
| `0x18463` | `0xd08–0xd0b`（块内多条，按句选） | 要想让城市投降，需要舰长和副官。 |
| `0x1851b` | `0xd0f` | 在海战中不能使用“策略”。 |
| `0x1858b` | `0xd13`（块内多条，按句选） | 不需要什么策略！让我去战斗吧！ |
