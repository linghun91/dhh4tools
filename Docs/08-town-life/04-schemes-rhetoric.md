# 计谋与话术

港口外交与谍报。24 是**并发槽**，不是 24 个具名招式。exe 能抽出的名字是 **10 个 kind**（话术 6 + 计谋 4）。话术和计谋共用 `CScheme` 槽与到期结算，入口、岗位、技能、解锁门槛不同。

委托目录（69 条）在 [02-requests-schemes.md](02-requests-schemes.md)。本篇不写委托。金币、友好度、份额、舰队位置的加减发给 economy / factions / fleets，本系统只决定「哪条 kind、对谁、何时到期」。

## 1. 职责边界

本系统管：

- `eScheme` 24 槽的分配、到期、清空
- `eSCHEMEKIND` 10 档（0–9）目录：名字、目标形状、入口、技能
- 话术入口（同业公会「发送交涉文书」+ 副官 + `eSkill` 7 说服力）
- 计谋入口（酒馆「确定计划」+ 参谋 + `eSkill` 8 谋略术）
- 发动时的金币门槛、岗位检查、技能门槛、到期日历
- 到期时按 kind 分发效果命令（友好度 / 协议 / 宣战 / 份额 / 诱舰）

本系统不管：

| 事项 | 归属 |
| --- | --- |
| 友好度曲线、协议旗、份额三槽 | factions |
| 舰队被诱导后怎么走 | fleets / ocean |
| 副官/参谋是否在岗、技能怎么从六维算 | characters |
| 酒馆是否客满、公会是否开门 | 设施 [01-facilities.md](01-facilities.md) |
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
| 合法 kind | 0–9 | `fn_1400112cf`：`kind < 0` 或 `>= 0xA` 则失败；`fn_14000cea5` `ja` 若 `> 9` |

`[推断]` `NUM_SCHEMEKIND = 10`。依据：填充器与结算 switch 都把 0–9 当合法，空槽存 11 而不是 10。11 落在 `< NUM_SCHEMEKIND` 之外，与「空货格用 106、不进 0–104」同类：空值不是枚举成员。

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

公会文书内容（SC `0x566f20`，JP `0x5675a0`，TC `0x5671d0`）：

| kind | key | SC | JP | TC |
| --- | --- | --- | --- | --- |
| 0 | `goodwill` | 亲善交欢 | 親善交歓 | 親善交歡 |
| 1 | `alliance` | 缔结协议 | 協定締結 | 締結協議 |
| 2 | `break_alliance` | 撕毁协议 | 協定破約 | 撕毀協議 |
| 3 | `ceasefire` | 停战 | 停戦 | 停戰 |
| 4 | `declare_war` | 宣战布告 | 宣戦布告 | 宣戰布告 |
| 5 | `intimidate` | 威压 | 威圧 | 威壓 |

酒馆计谋（SC `0x5649e8`，JP `0x565050`，TC `0x564c98`）：

| kind | key | SC | JP | TC |
| --- | --- | --- | --- | --- |
| 6 | `lure_fleet` | 引导舰队 | 艦隊誘導 | 誘導艦隊 |
| 7 | `forge_document` | 伪造假文书 | 偽文書制作 | 製作假文書 |
| 8 | `rumor` | 坏话 | 悪い噂 | 壞話 |
| 9 | `bribe` | 收买 | 買収 | 收買 |

kind 编号 `[推断]`：填充包装器把立即数 1–9 写进 `fn_1400112cf` 的 `edx`；kind 4=`fn_14000d53a` 宣战、5=威压、1=缔结、6=引导、7=假文书、8=坏话、2=撕毁、9=收买、3=停战。kind 0 走通用包装 `fn_14000191f`（`edx` 原样下传），菜单第一项是亲善交欢，故 0=亲善 `[推断]`。

`[确认]` 同表后紧挨着的「喝一杯 / 请喝酒 / 募集水手 / 离开酒馆」和「购买宝物 / 接受委托 / 离开同业公会」是设施菜单，不是 kind。

### 2.4 24 槽三分窗

`fn_140005416(element, start, end)` 在 `[start, end)` 里找第一个空槽（`vfunc_04==0`）。

| 窗 | 调用 | 谁用 |
| --- | --- | --- |
| 0–3（4 格） | `fn_140009b79` → `fn_140005416(..., rdx, 4)` | `[未知]` 无其它已还原调用点 |
| 4–13（10 格） | `fn_140014f74` → `(..., 4, 0xE)` | `[确认]` 玩家：公会 `fn_14000808a`、酒馆「确定计划」`fn_1400110bd` |
| 14–23（10 格） | `fn_140010df7` → `(..., 0xE, 0x18)` | `[确认]` NPC 势力 AI `fn_140107150`（`eThisForce`） |

满窗：公会走消息 `0x531`；酒馆走 `DAT_14055ee10`。玩家话术和计谋**抢同一 10 格**。

势力被消灭时 `fn_140003788` 扫全部 24 槽，若 `force_a` / `force_b` / `force_c` 命中则清空。

### 2.5 `CScheme` 布局

对象 0x18 字节。序列化 `vfunc_07`/`vfunc_06`：`+0x08`–`+0x0D` 六个字节 + `+0x10` 一个 dword。

| 偏移 | 宽 | 用途 | 空哨兵 | 标记 |
| --- | --- | --- | --- | --- |
| `+0x00` | 8 | vptr | | `[确认]` |
| `+0x08` | 1 | kind | `0x0B` | `[确认]` |
| `+0x09` | 1 | `force_a` | 随 kind；清槽时与势力 id 比较 | `[确认]` 比较；语义 `[推断]` 发起方 |
| `+0x0A` | 1 | | | `[未知]`。填充走与势力不同的 clamp（`fn_140205690`），不是 force/town |
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

`fn_140004976` 按 kind + force_a + force_b +（可选 force_c）+（可选 town）查重。

### 2.6 入口与前置

**酒馆** `[确认]`

- 文案：「如果参谋在，可在酒馆里拟定策略。」`0xc237`。岗位文档 `has_staff_officer`。
- 参谋槽空（`0xEC`，与舱室空人相同）则「确定计划」不可用（`fn_14014c350` RTC `nMonth/nDay` 那段）。
- 金币 `< 1000`（`0x3E8`）同样禁用。
- 至少 1 个可对势力（`fn_14000bbdb`）。
- 客满：「提督，客人都满了，好像没有座位能密谈。改天吧。」`0x9783`。这是设施占用，不是 kind 冷却。
- 选定后：「那么，我来布置。您等着看结果吧。」`0x94df`。效果延后，不是当场结算。

**公会** `[确认]`

- 「１０００枚金币承办给其他势力传送文书。」`0xac0f`。
- 「我来写交涉文书吧，要写什么内容？」`0xad43`。副官在岗（人物系统 `diplomacy_ok`）。
- 本海域没有对象：`0xaee3`。
- 信使忙碌：「对不起，现在传送者出去了，再等五六天吧。」`0xafc7`。`[推断]` 5–6 日是公会侧冷却，不是 `CScheme+0x10`。
- 停战也可从公会进（设施文案 `0x18463`），kind 仍是 3，不是另一套表。
- 菜单 7 项（6 个 kind + 返回），`fn_14000a4f2` / `fn_14000f3ad(..., 7)`。

**宫殿** `[确认]` 不是本系统入口，见 2.2。

### 2.7 技能：说服力 vs 谋略术

`fn_14000e1ce(person, eSkill)` 是技能取值（见人物/成长文档）。

**话术菜单解锁** `[确认]` 数字，`[推断]` 档位与 kind 的对应（菜单项顺序 = kind 0–5）：

`fn_14000a4f2` 读副官 `eSkill` 7：

| 说服力 | 现象 |
| --- | --- |
| `< 80`（`0x50`） | 后面几项不亮 |
| `< 100`（`0x64`） | 再锁一档 |
| `< 120`（`0x78`） | 再锁一档 |
| `< 200`（`0xC8`，等于 `skill_cap`） | 威压（kind 5）不亮，还要额外 `fn_140014c31(..., 5)` |

威压另有发动检定：`fn_14000983b` 把说服力与若干项加总，`>= 0xFA`（250）才走成功对话并写入槽。

**计谋发动检定** `[确认]` 四个酒馆函数都读双方 `eSkill` 8：

```
score = tactics_self - ((tactics_other - 8) >> 1)
if score <= threshold: 走「难」对话（DAT_14055ee38）
else: 走「易」对话
确认后仍写入槽（延后结算）
```

| kind | 函数 | threshold |
| --- | --- | --- |
| 6 引导 | `fn_14000c94b` | 20（`0x14`） |
| 7 假文书 | `fn_140011d0b` | 30（`0x1E`） |
| 8 坏话 | `fn_140001c1c` | 40（`0x28`） |
| 9 收买 | （`fn_14014c350` 内，调 `fn_140007581`） | 50（`0x32`） |

阈值只换对话，不阻止写入。真正成败在到期 apply。

### 2.8 费用、延迟、到期

**发动金** `[确认]` 话术、计谋入口都是 1000。不进 `CScheme+0x10`。02 篇把 `vfunc_04` 对 `+0x10` 的比较当成金币筛，是错的。

**到期** `[确认]` `fn_1400112cf`：

```
now = dword at (fn_140012c79() + 0x10)   # 指向 CWorld / CWorldData
due = now + delay * 0x30                 # 48
scheme[+0x10] = due
```

酒馆四个发动函数：`delay = fn_140005272(5) + 8` → 8–12。威压同样 `rand(5)+8`。

`CWorldData+0x10` 初值 0，存档字段（气候文档）。`fn_140007e69` RTC 名 `nMonth` `nDay`。`[推断]` `+0x10` 是世界日历计数；48 是「一个月」或其它日历单位，整数未在头文件写出。

`CSchemeElement::vfunc_04`（世界 tick，`fn_1402aad20` 在 `s20!=0` 时调）：

```
if empty: skip
if due > *(arg + 0x18): skip
# s20 = (某对象+0x1030 != 0x46)
if s20==0 或 kind>=6 或 玩家不在 force_a/force_b:
    apply(kind)
```

`[推断]` `arg+0x18` 是当前日历，到期才 apply。`+0x1030==0x46`（70）的语义 `[未知]`（70 也是委托分配槽数）。

apply 末尾一律 `fn_14000cc52` 清空槽。

### 2.9 到期分发

`fn_14000cea5` 按 kind 0–9 jump table：

| kind | apply | 体积 | RTC / 观察 |
| --- | --- | --- | --- |
| 0 亲善 | `fn_1400136ab` `0x140202870` | 795 | |
| 1 缔结 | `fn_140004705` `0x1402031c0` | 1707 | |
| 2 撕毁 | `fn_140004471` `0x1402012b0` | 643 | 文案 0x175/0x177/0x17c/0x17d；`fn_140006e88(..., 0x32, ..., 0x1A)` |
| 3 停战 | `fn_14000a858` `0x140201e70` | 1403 | |
| 4 宣战 | `fn_140011680` `0x140202550` | 639 | |
| 5 威压 | `fn_1400018f7` `0x140202c60` | 1099 | |
| 6 引导 | `fn_14000f92f` `0x140203a20` | 904 | RTC `aeFleet` |
| 7 假文书 | `fn_14000421e` `0x140203e90` | 1334 | |
| 8 坏话 | `fn_1400091ec` `0x140204520` | 1362 | |
| 9 收买 | `fn_140007243` `0x1402015e0` | 1742 | RTC `szDstForceName`；`fn_1400122f6` 改份额 |

收买到期二次检定 `[确认]`：

```
roll = fn_140005272(0x78) + 0x32        # rand(120)+50 → 50–169
if roll >= tactics_other - ((tactics_ref - 8) >> 1): 失败
else: 调份额；消息 0x193/0x194 成功、0x196 失败
```

失败句 mesfile「收买失败了。」`0x3b57`。成功「夺取了%s的占有率。」`0x3aeb`。

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
| 8 坏话 | 目标港对某势力敌视；可能 `AmityChanged -` | 「在%s，流传着%s的坏话。」「这传闻没有被相信。」「由于谁的计谋，我们与%s的友好度降低了」 |
| 9 收买 | `Share.add/sub` | 「经纪人收取了书信」「夺取了%s的占有率」 |

友好度变化量、份额变化量除收买路径里的立即数外 `[未知]`。亲善的「上升%s」是格式化整数，公式未还原。

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

禁止用 `id=0` 当空槽（0 是合法下标）。空 = `kind: empty`。

运行时按表长循环，不写死 24/10。原版包：24 槽、10 kind。

## 4. 算法 / 公式

全部进 `config/formulas/town.yaml` 与 kind 表。禁止 `if kind==8` 散落。

```
# 日历。delay_unit 原版 48；calendar_now 读 world 日历字段
due = calendar_now + delay * delay_unit
delay_cast = rand(delay_rand_span) + delay_bias    # 5, 8 → 8..12

# 计谋发动分（只换对话）
tactics_score(self, other) =
    Skill(self, tactics) - ((Skill(other, tactics) - tactics_offset) >> tactics_shift)
# tactics_offset=8, tactics_shift=1

# 收买到期
bribe_roll = rand(bribe_rand_span) + bribe_rand_bias   # 120, 50
bribe_success = bribe_roll < tactics_score(other, ref)

# 话术菜单
rhetoric_unlocked(kind, persuasion, extra) =
    persuasion >= kind.unlock_persuasion
    AND extra_ok(kind)          # 原版 fn_140014c31 / 威压 250

can_open_rhetoric(player):
  Characters.modifier.diplomacy_ok     # 副官
  AND gold >= letter_cost_gold         # 1000
  AND Schemes.free_in(player_bank) > 0
  AND Factions.has_addressee_in_sea(sea_id)
  AND not guild.courier_busy

can_open_scheme(player):
  Characters.modifier.can_scheme       # 参谋，人 != empty_person 0xEC
  AND gold >= plot_cost_gold           # 1000
  AND Schemes.free_in(player_bank) > 0
  AND bar.private_seat_available       # 非客满
  AND Factions.has_scheme_target() > 0

cast(kind, targets, delay):
  slot = alloc(bank_for(caster))       # 玩家 4–13；NPC 14–23
  pay gold
  write {kind, force_a, force_b, force_c|empty, town|empty, due}
  emit SchemeQueued

on_world_tick:
  for slot in 0 .. len(slots)-1:
    if kind==empty: continue
    if due > calendar_now: continue
    apply_table[kind](slot)            # 只 emit 命令
    slot.kind = empty
```

`bank_for`：玩家固定 `player`（原版 4–13）；NPC 固定 `npc`（14–23）。0–3 窗保留为可配 `bank.extra`，原版包可空着不用。

apply 禁止本系统改友好度数字：`Factions.applySchemeEffect { kind, force_ids, town_id, params }`。

## 5. 耦合分析

原版硬编码：

- `CScheme` 是 `CSaveObject`，和城镇、人物同一存档链
- 填充直接写势力/港口字节，结算直接改友好度、份额、舰队
- 技能下标 7/8、阈值 20/30/40/50/80/100/120/200/250、金币 1000、延迟 `rand(5)+8`、乘 48，全是立即数
- 循环上界 24、kind 上界 10、空 kind 11、空势力 26、空港 98 写死
- 公会/酒馆 UI 直接 `new` 选势力/选港 View
- NPC AI（`fn_140107150`）自己挑 kind 往 14–23 填

重构后只发：

```
TownLife.CastScheme → Schemes.queue
Schemes.due        → Factions.applySchemeEffect
                   → Fleets.lureTo(town_id)          # kind lure_fleet
Schemes.queue      → Economy.debitGold { 1000 }
```

本系统不读六维原值，只问 `ModifierSet.persuasion` / `tactics`（或 `Skill(id, 7|8)` 查询）。不读市价。不持有 `CForce*`。

允许依赖：`town_id` / `force_id` / `person_id` / `fleet_id`；characters 的岗位与技能查询；world 日历。

## 6. 解耦后的配置

`config/town/scheme_kinds.yaml` — kind 目录（原版包 10 行）：

```yaml
schema: dk4.town.scheme_kinds
version: 1
empty_kind_byte: 11          # 原版 +0x08==0x0B；存档对照用
kind_count_vanilla: 10

kinds:
  - id: 0
    key: goodwill
    group: rhetoric
    name: 亲善交欢
    name_i18n: { ja: 親善交歓, zh-Hans: 亲善交欢, zh-Hant: 親善交歡 }
    entry: guild
    requires_post: adjutant
    skill: persuasion
    targets: [force_a, force_b]
    effect: amity_up
  - id: 1
    key: alliance
    group: rhetoric
    name: 缔结协议
    entry: guild
    requires_post: adjutant
    skill: persuasion
    targets: [force_a, force_b, force_c]
    effect: alliance_form
  - id: 2
    key: break_alliance
    group: rhetoric
    name: 撕毁协议
    entry: guild
    targets: [force_a, force_b]
    effect: alliance_break
  - id: 3
    key: ceasefire
    group: rhetoric
    name: 停战
    entry: guild
    targets: [force_a, force_b]
    effect: ceasefire
  - id: 4
    key: declare_war
    group: rhetoric
    name: 宣战布告
    entry: guild
    targets: [force_a, force_b]
    effect: declare_war
  - id: 5
    key: intimidate
    group: rhetoric
    name: 威压
    entry: guild
    targets: [force_a, force_b]
    effect: absorb
  - id: 6
    key: lure_fleet
    group: covert
    name: 引导舰队
    entry: bar
    requires_post: staff
    skill: tactics
    targets: [force_a, force_b, town]
    effect: lure_fleet
    cast_threshold: 20
  - id: 7
    key: forge_document
    group: covert
    name: 伪造假文书
    entry: bar
    requires_post: staff
    skill: tactics
    targets: [force_a, force_b, force_c]
    effect: feud
    cast_threshold: 30
  - id: 8
    key: rumor
    group: covert
    name: 坏话
    entry: bar
    requires_post: staff
    skill: tactics
    targets: [force_a, force_b, town]
    effect: rumor_cut_amity
    cast_threshold: 40
  - id: 9
    key: bribe
    group: covert
    name: 收买
    entry: bar
    requires_post: staff
    skill: tactics
    targets: [force_a, force_b, town]
    effect: steal_share
    cast_threshold: 50
```

`config/town/rhetoric.yaml` — 话术层（与 kind 表分离：解锁、文书金、信使）：

```yaml
schema: dk4.town.rhetoric
version: 1
# 不是第二份 24 行目录。6 行对应 kind 0–5。

facility: guild
post: adjutant
skill: persuasion
cost_gold: 1000
courier_busy_days_min: 5
courier_busy_days_max: 6
menu_i18n: ui.guild.send_letter          # 「发送交涉文书」
intimidate_score_min: 250                # fn_14000983b 0xFA

unlocks:
  - kind: goodwill
    persuasion_min: 0
  - kind: alliance
    persuasion_min: 80                   # 0x50；与菜单第 2 项对齐 [推断]
  - kind: break_alliance
    persuasion_min: 100                  # 0x64
  - kind: ceasefire
    persuasion_min: 120                  # 0x78
  - kind: declare_war
    persuasion_min: 0
    extra: has_war_target                # fn_14000529a + fn_140014c31(...,4)
  - kind: intimidate
    persuasion_min: 200                  # 0xC8 == skill_cap
    extra: absorb_possible
```

`config/town/schemes.yaml` — **24 行槽**（原版包定义窗，不定义 24 个名字）：

```yaml
schema: dk4.town.schemes
version: 1
# exe 没有 24 个招式名。每行是并发槽。kind 运行时写入，定义侧全 empty。

banks:
  extra:
    start: 0
    end: 4
    owner: unknown                       # 原版包装存在，调用点未还原
  player:
    start: 4
    end: 14
    owner: player                        # 公会文书 + 酒馆计谋
  npc:
    start: 14
    end: 24
    owner: npc_ai

slots:
  - { id: 0,  key: slot_00, bank: extra,  kind: empty }
  - { id: 1,  key: slot_01, bank: extra,  kind: empty }
  - { id: 2,  key: slot_02, bank: extra,  kind: empty }
  - { id: 3,  key: slot_03, bank: extra,  kind: empty }
  - { id: 4,  key: slot_04, bank: player, kind: empty }
  - { id: 5,  key: slot_05, bank: player, kind: empty }
  - { id: 6,  key: slot_06, bank: player, kind: empty }
  - { id: 7,  key: slot_07, bank: player, kind: empty }
  - { id: 8,  key: slot_08, bank: player, kind: empty }
  - { id: 9,  key: slot_09, bank: player, kind: empty }
  - { id: 10, key: slot_10, bank: player, kind: empty }
  - { id: 11, key: slot_11, bank: player, kind: empty }
  - { id: 12, key: slot_12, bank: player, kind: empty }
  - { id: 13, key: slot_13, bank: player, kind: empty }
  - { id: 14, key: slot_14, bank: npc,    kind: empty }
  - { id: 15, key: slot_15, bank: npc,    kind: empty }
  - { id: 16, key: slot_16, bank: npc,    kind: empty }
  - { id: 17, key: slot_17, bank: npc,    kind: empty }
  - { id: 18, key: slot_18, bank: npc,    kind: empty }
  - { id: 19, key: slot_19, bank: npc,    kind: empty }
  - { id: 20, key: slot_20, bank: npc,    kind: empty }
  - { id: 21, key: slot_21, bank: npc,    kind: empty }
  - { id: 22, key: slot_22, bank: npc,    kind: empty }
  - { id: 23, key: slot_23, bank: npc,    kind: empty }
```

`config/formulas/town.yaml` 本篇新增键：

```yaml
# 片段；与旅馆/酒价等其它 town 公式共存
scheme_delay_unit: 48
scheme_delay_rand_span: 5
scheme_delay_bias: 8
scheme_letter_cost_gold: 1000
scheme_plot_cost_gold: 1000
tactics_offset: 8
tactics_shift: 1
bribe_rand_span: 120
bribe_rand_bias: 50
intimidate_score_min: 250
rhetoric_unlock_80: 80
rhetoric_unlock_100: 100
rhetoric_unlock_120: 120
rhetoric_unlock_200: 200
empty_person_cabin: 236                  # 原版 0xEC；人物系统已有空岗哨兵则删此键
town_none: 98
force_none: 26
```

存档：24 槽的 kind/三势力/港/due。kind 名、解锁表、费用不进存档。

## 7. 独立旋钮

- 槽表长（可 >24；NPC/玩家窗按 `banks` 切）
- 增删 kind（新 key + apply 效果名 + 目标形状）
- 话术/计谋费用是否仍同为 1000
- 说服力解锁阶、威压 250、计谋四档阈值
- 延迟 `rand+bias` 与乘子 48
- 是否必须副官/参谋（`requires_post`）
- 空 kind / 空港 / 空势力哨兵
- 信使 5–6 日、酒馆密谈是否检查客满

不该出现在本表：友好度公式、份额封顶、市价、六维配对（配对在 `characters/skills.yaml`）。

## 8. 对外接口

事件：

```
SchemeQueued   { slot_id, kind, caster_force_id, targets[], due }
SchemeResolved { slot_id, kind, success, targets[] }
SchemeCleared  { slot_id, reason }          # 势力灭亡 / 到期 / 放弃
RhetoricMenuOpened { town_id, unlocked_kinds[] }
```

查询：

```
Schemes.free_in(bank) -> int
Schemes.get(slot_id) -> { kind, due, targets } | empty
Schemes.affordable_rhetoric(persuasion, gold, has_adjutant) -> [kind]
Schemes.can_plot(tactics, gold, has_staff, bar_seats) -> bool
```

命令：

```
CastRhetoric { kind, force_b, force_c? }           # 公会；force_a=玩家势力
CastScheme   { kind, force_b, town_id?, force_c? } # 酒馆
TickSchemes  { calendar_now }                      # world 订阅日历
```

完成时只 emit；factions / fleets / economy 订阅。UI 用 `CSelectForceView` / `CSelectTownView` 的重构等价物，只回传 ID。

## 9. 未核实字段

- `NUM_SCHEMEKIND` 头文件整数（现按 10 推断）。
- kind 0 与「亲善交欢」的立即数对应（包装器未写死 0）。
- 说服力 80/100/120 与菜单第几项的一一对应（顺序推断）。
- `+0x0A` 字节。
- 槽窗 0–3 的占用者。
- `vfunc_04` 里 `+0x1030==0x46` 的对象是谁。
- `due` 比较的 `arg+0x18` 确是日历而非别的计数。
- `delay_unit=48` 对应月还是日。
- 亲善「友好度上升%s」的整数公式；`fn_140006e88(..., 0x32)` 是加 50 还是设成 50。
- 坏话失败/成功如何在 apply 里掷骰（发动时阈值只换对话）。
- 引导舰队改的是 `fleet_id` 还是该势力「本月定期船」。
- 信使「五六天」是否写入状态，还是纯文案。
- NPC AI 选 kind 的权重。
- 同时是否允许同一对势力多封未到期文书（有 `fn_140004976` 查重，调用点未钉）。
- 攻城扰乱 1000 金的独立表（若有，不进 `schemes.yaml`）。
