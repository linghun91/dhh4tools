# 事件指令与条件（命令解析器）

配置落点：`config/narrative/opcodes.yaml`、`config/narrative/trigger_kinds.yaml`。触发扫描与 EV 头见 `02-event-system.md`。剧本选包见 `01-scenarios.md`。

本系统是 **bytecode 解释器 + 条件求值器**。内容是 EV 包（触发器 + execute 表）和一一对应的 Sc 包（指令流 + 内嵌对白）。引擎不得把某条剧情编译进 C++。

EV 96 字节头（魔数 `'WE'`、9 个 `eTriggerKind` 计数与载荷偏移）已在 `02-event-system.md` 钉死，本篇只引用，不复述。本篇从 **头之后** 的触发记录、execute 表、Sc 指令流和解释器跳表往下拆。

## 1. 职责边界

本系统管：

- 把 `eTriggerKind` 0–8 当成 **事件源**（哪种场景切换在问：探索、海战始末、接触、入港、港内、航海、设施、村落），日期 / 旗 / 持有物等是所有 kind 共用的可选条件块
- 解析触发记录里的 **可选条件块**（日期、旗、多港口、战斗结束、引用对象）
- 按 execute 表取出脚本 id，加载对应 Sc 记录，逐步执行 16 位指令
- 把指令发出的 **命令** 交给其它系统（货舱、旗、人物、港口、UI、音频）
- 把指令发出的 **查询** 变成谓词（0/1/2），供后续分支指令使用
- 维护每条触发的运行时状态（锁存、已命中、禁用、下一段 execute id）

不管：

- 选哪个剧本、哪份世界包（剧本表）
- EV 头字段的再命名（`02-event-system.md`）
- 静画贴图文件本身（`EventBG*` / `EvFrame` / `EvItem`，UI / 资源层）
- mesfile 正文（Sc 记录里另有内嵌 GBK；mesfile 仍是其它 UI 文案）
- 市价、伤害、经验公式
- `EVA.DK4` 动画包（不是 `'WE'` 脚本，也不是本 ISA）

叙事可以查询全部系统、发出命令。其它系统不得依赖 `scenario_id` 或某条 `opcode` 的硬编码分支。

## 2. 原版事实

### 两层内容，一套解释器

[确认] 一次开火走两层文件：

| 层 | 文件 | 角色 |
| --- | --- | --- |
| 触发 + execute 索引 | `SC/EV0.dk4`…`EV6.dk4` | `'WE'` 头 + 变长触发记录 + execute 表 |
| 指令流 + 内嵌对白 | `SC/Sc0.dk4`…`Sc6.dk4` | 目录头一个 `uint32` = 该包触发总数；每条记录魔数 `'DS'`，版本字与对应 EV 的版本字相同（EV0/Sc0 = `0x0106`，EV3/Sc3 = `0x0103`，七包均 1:1） |

来源：`Game/SC/` 实文件、`CEvent::vfunc_05` → `fn_140007dec`、`fn_1400016db`（`CEventScriptBuffer` 按 id 加载）、rdata 里连续的 `SC/Sc0.dk4`…路径。

[确认] 解释器主体：

| 符号 | body | 角色 |
| --- | --- | --- |
| `fn_140013c32` | `0x1400bdee0` | 按 `CEvent+0x88` 的 kind 扫该类触发 |
| `fn_140001b09` | `0x1400bf5e0` | 条件求值。`switch (kind)`，`ja` 上限 8。跳表 VA `0x1400c05bc` |
| `fn_1400050fb` | `0x1400c3960` | RTC：`sScriptTable`、`sExecute`。取 execute 字，分 type 1/2 |
| `fn_1400020fe` | `0x1400c3be0` | RTC：`sScriptResult`、`lNextOffset`、`sScript`。逐步执行 |
| `fn_14000508d` | `0x1400e2270` | 单条指令分派。读当前字 bits 13–15 为 class，`ja` 上限 6。跳表 VA `0x1400e23c0` |
| `fn_14001337c` | `0x1400c0d30` | 开火入口。把调用方的 24 字节 `sEventTrigger` 请求 `rep movs` 拷进 `CEvent+0x88…+0x9F`，置 `+0xb8=1`，再调 `fn_140013c32` 扫描 |

`id == 0`：`fn_1400050fb` 直接返回（vanilla 的「无脚本」）。`id == 0x3fff`：不跑脚本，调用 `fn_140010578(DAT_144296500, 1)`。注意 `rcx` 是全局 `DAT_144296500`（`CDk4Game`，场景管理器；`+0x7c` 为剧本号，见 `fn_140004a1b`），**不是** `CEvent`。该函数写 `CDk4Game+0x88 = 3`（游戏流程态「结束」）、`+0x90 = 1`。流程态见下文「游戏流程态」。这是解释器的特殊出口，不是一条普通 opcode，也不改扫描组。

来源：`srcdata/source/game/unassigned/fn_1400c2290.cpp`、`fn_1400b93e0.cpp`、objdump `0x1400c39a1`、`0x1400c42d0`、`0x1400c0d30`。

### 触发记录：变长、双 dword 头、后面跟可选块

[确认] 单条触发不能按固定 stride 切。长度在 **dword[1] bits 0–7**，单位是 dword。`fn_14000df4e` 抽这个字段，`fn_14001435d` 做 `ptr += length * 4`。七个 EV 包用这条规则切完，每种 kind 的解析终点都等于下一个 kind 的起点（或 execute 段起点）。

[确认] EV0 例：kind 计数 `(5,16,16,4,2,85,10,203,5)` 和 = 346；切出来的字节与头里的 kind 偏移完全吻合。

头两个 dword 的位域（抽位原语 `fn_1400be320(lo,hi)` / `fn_1400be2c0(bit)`，读 dword `fn_14000f20e(index)`）：

| 字段 | 位置 | 用途 |
| --- | --- | --- |
| `script_id_false` | dword[0] bits 0–13 | 条件失败时的 execute 下标。七包文件里 **全部为 0**；运行时可被脚本改写（见 class 0.0） |
| `latched` | dword[0] bit 15 | 读 `fn_14000fea2`，写 `fn_1400128f0`。置位时本次命中按「未命中」处理；条件为假时清零。文件里全 0 |
| `script_id_true` | dword[0] bits 16–29 | 条件成功时的 execute 下标。EV0 文件为 `1…346` 各用一次；运行时可被脚本改写 |
| `fired` | dword[0] bit 30 | `fn_14000e700` 在命中后置 1。文件里全 0 |
| `disabled` | dword[0] bit 31 | 读 `fn_14000ea9d`，写 `fn_14001510e`。非 0 则本条不求值。文件里全 0 |
| `length_dwords` | dword[1] bits 0–7 | 整条记录长度 |
| `ctx` | dword[1] bits 8–15 | 与 `CEvent+0x8c/+0x94/+0x98/+0x9c` 比较的上下文（港口或其它命名空间） |
| `subtype` | dword[1] bits 16–17 | 0–3。kind 1–3 与 kind 4–7 各用一套 |
| `extra` | dword[1] bits 18–21 | 仅 kind 7 额外与 `CEvent+0x90` 比较 |
| `time_mode` | dword[1] bits 22–23 | 0 = 无时间块；1/2/3 = 三种日历比较 |
| `has_outcome` | dword[1] bit 24 | `SEventTriggerEventOutcome` |
| `has_reference` | dword[1] bit 25 | `SEventTriggerReference` |
| `has_flag_*` | dword[1] bits 26–29 | 四段事件旗块（见下） |
| `relatch` | dword[1] bit 30 | `fn_140005d80`。命中后若置位，则置 dword[0] bit 15（边沿触发：条件回落前不再开火）。EV0 325 / 346 条 |
| `one_shot` | dword[1] bit 31 | `fn_140009c4b`。命中后若置位，则置 dword[0] bit 31（永久禁用）。EV0 310 / 346 条 |

`0xFF` / `0xFE` 出现在 `ctx` 里：港口组把它当「任意 / 另一种哨兵」，与 `fn_140001b09` 里 `uint8 == 0xFF` 则跳过比较一致。

### `eTriggerKind` 0–8 = 「哪一种场景切换在问」

[确认] `NUM_TRIGGERKIND = 9`。没有 `TriggerKindID.h`。kind 不是事件对象的长期状态，而是每次开火时调用方填的 24 字节请求 `sEventTrigger`（RTC 名）：

| 请求偏移 | 拷入 `CEvent` | 含义 |
| --- | --- | --- |
| +0x00 | `+0x88` | kind |
| +0x04 | `+0x8c` | `ctx0`（主上下文 id） |
| +0x08 | `+0x90` | kind 2 = 战斗结果；kind 7 = 设施号 |
| +0x0C / +0x10 / +0x14 | `+0x94` / `+0x98` / `+0x9c` | kind 1–3 的三个对手属性 |

请求构造器默认 kind = `0xA`（`fn_1400029cd`，body `0x1400bcde0`）。`fn_14001337c` 只有 8 个调用点，全部在场景切换 / 探索 / 登陆钩子里。按调用点钉死每个 kind：

| kind | 构造器 | 调用方（RTC / 条件） | `ctx0` | 玩法含义 |
| --- | --- | --- | --- | --- |
| 0 | `fn_14001245e` | `fn_14001266b`（`apfnSearchEvent`、`eItem`、`posPlayerFleet`）：海上探索按 `eItem < 0xE7` 找到道具后 | `item_id` | 探索发现道具 |
| 1 | `fn_140013895(…,1)` | `fn_140008477`：新场景为海图且子态 = 1（进入海战），对手 `person_id ≠ 0x46` | 对手 `person_id` | 海战开始 |
| 2 | `fn_14000edb8` | `fn_140008477`：旧场景子态 = 1、新场景子态 = 0（海战结束回海图） | 对手 `person_id`；`+0x90` = 战斗结果 | 海战结束 |
| 3 | `fn_140013895(…,3)` | `fn_14000cd8d`（`sChangeBattle`、`eMobileFleet`）：与移动舰队接触 | 对手 `person_id` | 舰队接触（交战前） |
| 4 | `fn_140002d8d(…,4)` | `fn_14000443f`：旧场景海图 → 新场景港口 | 新场景 `town_id` | 入港 |
| 5 | `fn_140002d8d(…,5)` | `fn_140004822`（港口场景 tick）；或 `fn_140008477` 进港口场景且设施号 = `0xD` | `town_id` | 港内（街区地图） |
| 6 | `fn_140002d8d(…,6)` | `fn_140004822`（海图场景 tick；调用方含 `fn_140010a00`，RTC `nDay/nMonth`） | 玩家舰队对象 `+0x70` 字节（所在港，海上多为 `0xFF`） | 航海中 |
| 7 | `fn_1400070ae` | `fn_140008477`：新场景港口且设施号 ≠ `0xD` | `town_id`；`+0x90` = 设施号 | 进入港内设施 |
| 8 | `fn_1400100b4` | `fn_1400031fc`：`fn_140001186`（`posVillage`、`eVillage`）找到村落并确认登陆 | `village_id`（`NUM_VILLAGE=24`） | 登陆村落 |

三个钩子 `fn_140004822` / `fn_14000443f` / `fn_140008477` 都以 `DAT_144296500`（`CDk4Game`）为 `this`，读 `+0x8` 的当前场景描述 `SSceneDesc`：`+0x8` 场景类型（0 海图、2 港口）、`+0xc` 海图子态（1 = 海战）或港口 `town_id`、`+0x10` 对手 `person_id` 或设施号。调用方含 `CTownScene::vfunc_02`、`CSeaAreaScene::vfunc_02/35/36/38`。

EV0 `ctx` 与上表吻合：kind 0 = 150 / 154 / 153 / 48 / 32（都是 `item_id`，不是港口）；kind 8 = 1 / 18 / 22 / 6（都 < 24）；kind 7 的 `extra` = 设施号。

来源：objdump `0x1400c0d30`、`0x1401ffaf0`、`0x1401ffdb0`、`0x1402009b0`、`0x140192610`、`0x1401ae157`、`0x1401b4fcf`；构造器 `0x1400bcb10`…`0x1400bcdc8`；`srcdata/catalog/rtc_index.tsv`（`sEventTrigger`、`sNextSceneDesc`）。

跳表（`fn_140001b09`，`0x1400c05bc`）：

| kind | 入口 | 比较 |
| --- | --- | --- |
| 0 | `0x1400bf66a` | `ctx` 比 `CEvent+0x8c`（`item_id`） |
| 1, 2, 3 | `0x1400bf693` | 抽 `subtype`：0 → `ctx` 比 `+0x94`；1 → 比 `+0x98`；2 → 比 `+0x9c`；3 → 不比。**不比** `+0x8c` |
| 4, 5, 6, 7 | `0x1400bf744` | kind 7 先比 `extra` 与 `+0x90`。再用 `fn_1400103bb(+0x8c)` 取港口对象：`subtype` 0 → `ctx` 为 `0xFF` 则通过，否则比港口 id；1 / 2 → 比港口的两种属性（`fn_14001055a` / `fn_140014e48`）；3 → 交给 MultipleTown 块 |
| 8 | `0x1400bf884` | `ctx` 比 `CEvent+0x8c`（`village_id`） |

[确认] kind **2** 在公共路径里 **无条件** 构造 `SEventTriggerCloseBattle`（ctor thunk `0x140010f1e`）。

[确认] kind 4–7 且 subtype == 3 时构造 `SEventTriggerMultipleTown`。

[推断] kind 1–3 的 `+0x94/+0x98/+0x9c`：`fn_140013895` 写 `+0x0C = fn_14001460f(person)`（人物对象 `+0x10` 字节，`0xEC` 为空）、`+0x10 = fn_14000d599(person)`（默认 `0x1A` 空势力，[推断] 所属势力）、`+0x0C == 0xC5` 时 `+0x14 = 船型值 − 0x22`。字段名未还原。

### 不是 kind 的「流程态」：`CDk4Game+0x88`

旧版本把 `fn_140007b58` 等 23 字节 setter 当成「写 `CEvent+0x88` 的 kind setter」。**错误。** 这些函数的 `this` 全部是 `DAT_144296500`（`CDk4Game`），写的是游戏流程态。`fn_140004ead` 判 `== 1` 为「游戏进行中」，扫描循环、解释器循环每步都查它。

| 流程态 | setter | 已见写入点 | 后续处理（`0x140112820`） |
| --- | --- | --- | --- |
| 0 | `fn_140007b58` `0x1400e45e0` | 系统菜单「结束游戏」；脚本 6.5 其它值 | 回标题 |
| 1 | `fn_140006f50` `0x140112aa0` | `0x140111fa0`（调用方语义未逐条核） | 进行中（`fn_140004ead` 判此值） |
| 2 | `fn_14000b3c0` `0x140115d50` | 系统菜单「载入进度」成功 | 读档 |
| 3 | `fn_140010578` `0x1400c42d0`（第二参写 `+0x90`） | `execute id 0x3FFF`（参 1）；脚本 6.5 值 1（参 1）；`fn_14000a9ed`（`eFleetShip`，参 0）；`fn_14000a8c1`（参 0） | Game Over。`+0x90` = 1 即寿终：先 `fn_14000fb14(0x170…0x173)` 放寿终四句（消息号表 .data `0x140683620` 解码，见 `11-progression/03`），再 `fn_14000a894`（RTC `rcGameOver`、`TitleSC.dk4`）；参 0（本队覆灭 / 舰队解散）直接 still。之后只回标题，无继承 |
| 4 | `fn_14000210d` `0x1400e2440` | 脚本 6.5 值 0 | **剧本通关**：`fn_14000b677(1)`（RTC `cEventAnimOP`，片尾动画），再给 `savedata.dk4` 记通关（见 `05-gallery-unlock.md`） |
| 5 | `fn_140014ab0` `0x1400d6b40` | 动画 / 事件窗口消息泵（其中一处 `cmp msg, 0x12` = `WM_QUIT`）、`0x14029f16d` | 退出程序；`0x140112820` 见 5 时不改回 0 |

菜单文案来源：`fn_140004c28` 选项串 `储存进度 / 载入进度 / 结束游戏 / 选项 / 返回`（rdata `0x140555700` 等）。

### 可选条件块（所有 kind 在 kind 开关之后共用）

求值顺序（`fn_140001b09` 汇合点 `0x1400bf8a8` 之后）：

1. **EventOutcome**（bit 24）：两串下标，分别要求「已发生」和「未发生」。内部走 `fn_14000a0d8`（`0xFFFF` 跳过）。
2. **Time**（bits 22–23 ≠ 0）：ctor `SEventTriggerTime`。抽出 4 个字段构造日历再比。[确认] 月份字 == `0xC`（12）时改成主角生日月（`fn_140004e17` → `fn_1400022b6`，`CPlayerPerson+0x63`）；日字 == `0x1F`（31）时改成主角生日日（`fn_140003e18`，`+0x64`）——生日哨兵，不是通配，也不是 13 月 / 32 日（`0x1400bfaac`–`0x1400bfae1`；引用类型 6 `fn_14000fe52` 同样处理，见 `01-world/07` §2.5）。通配是字段值 `0xFF`，比较函数逐字段跳过。mode 1/2/3 走三个比较函数 `0x1400bd3b0` / `0x1400bd670` / `0x1400bd550`。
3. **CloseBattle**（仅 kind == 2）：按 `CEvent+0x90` 的 0/1/2/3 与记录里四根标志位对齐。对不上则失败。
4. **MultipleTown**（kind ∈ 4–7 且 subtype == 3）：当前 `town_id` 必须落在名单里。
5. **EventFlag ×4**（bits 29, 28, 27, 26）：每段构造 `SEventTriggerEventFlag`，与当前 256 bit 旗做 `fn_140006082`（8 个 dword AND）。四段的通过条件不同（全匹配 / 结果非空 / 结果为空 / `fn_140012585`），**不要**合成一个「flag 条件」枚举。
6. **Reference**（bit 25）：`SEventTriggerReference`。类型在引用记录 dword[0] **bits 28–31**（`fn_140004291`），`ja` 上限 `0xE`，即 0–14 共 15 种。分派 `fn_14000eb1a`，跳表 VA `0x1400c679c`。

[确认] 引用类型 0–14 的处理函数（名字来自 RTC / 立即数比较，不是官方枚举）：

| eRef | 函数 | 立即数 / RTC | 语义 |
| --- | --- | --- | --- |
| 0 | `fn_1400031a2` | `fn_140005272(0x3E8)` 取 0–999 随机数，与记录值三态比较（小于 → 0、等于 → 1、大于 → 2） | [确认] 千分比随机判定。**不是**金币档 |
| 1 | `fn_140015145` | 对 `CEvent+8`（`CEventData`）调 `fn_140013016(id)` → 0/1 | [推断] 事件数据侧查询（结果 / 旗一类） |
| 2 | `fn_140009c46` | 下标 `&lt; 0xE7`（231=`NUM_ITEM`），`fn_14000f0f6(item)` → 0/1；越界返回 1 | [推断] 道具状态位 |
| 3 | `fn_1400097e1` | RTC `aePerson` | 人物 |
| 4 | `fn_1400021df` | RTC `sItemOwner` | 持有道具 |
| 5 | `fn_140011b2b` | | [未知] |
| 6 | `fn_14000fe52` | RTC `sWorldDate`、`sCheckDate` | [确认] 日期比较 |
| 7 | `fn_140008c33` | 人物 `&lt; 0xEB`（235=`NUM_PERSON`），再 `ja 0x17` 的子表 | 人物属性 / 技能一类 |
| 8 | `fn_1400029ff` | RTC `anSupply`；`CountFleetItem`；商品 `&lt; 0x69`（105=`NUM_TRADINGITEM`） | 舰队货舱件数 + 比较符 |
| 9 | `fn_1400049c1` | RTC `_eFleet` | 舰队 |
| 10 | `fn_140008f8a` | 下标 `&lt; 0x19`；函数内多次与 `0x1A`（空势力哨兵）比较 | [推断] 势力（关系）。不要写成海域 |
| 11 | `fn_14001098d` | 先 `&lt; 0x61`（97=`NUM_TOWN`），再分 0/1/2：`&lt; 0x19` / `0xFF`/`0xFE` / `&lt; 0x0D` | 港口上的势力 / 海域 / 第三种 id |
| 12 | `fn_1400137cd` | | [未知] |
| 13 | `fn_14000880a` | 对 `CEventData` 取某 id 的 `u16`，再与当前日期序号 `fn_1400040c5` 相减 | [推断] 距某事件结果的经过时间 |
| 14 | `fn_14001393a` | 下标 `&lt; 0x19`，`fn_14000fa92(id)` → `fn_14000c6f3` | [确认] 遗迹（`eRemains`）状态。同一对函数在 `fn_14000a321`（RTC `eRemains`）里按 `< 0x19` 循环，故 `NUM_REMAINS = 25` |

比较符三件套 `fn_14000ba0a` / `fn_140003959` / `fn_1400080e9`：相等、小于、否则。引用条件和货舱条件都走它们。

原版 `CountFleetItem` / `AddLoad` 在贸易栈里，重构归货舱所有者 ships（`Ships.count_item` / `Ships.add_load`）；事件侧调用点见 `srcdata/docs/trade_stack.md`：条件 `0x1400c7c00`，动作 `0x1400e8fc0`。

### Execute 表

[确认] EV 头 `+0x4C` 起 5 个 `uint32` 是 **段表**（相对载荷 = `file+0x60`）。`fn_14000ecf5(index)` 读 `header+0x4C+index*4`。

EV0：`0x1A8C, 0x1D44, 0x1D44, 0x20DC, 0x20DC`。

[确认] 段号由 `fn_140001ef1(seg)` 的五个调用点钉死：

| 段 | 调用方 | 用途 | vanilla |
| --- | --- | --- | --- |
| 0 | `fn_140008283` | execute 表，uint16 数组，按 `id * 2` 取，构造 `SEventDataElementExecute` | EV0 `0x1A8C…0x1D44` |
| 1 | `fn_140011702` ← `fn_140013efd` ← class 2 | 对白消息表（`SEventDataElementMessage`）。非内嵌对白按下标取 | 空 |
| 2 | `fn_1400121fc` ← `fn_140001b09`（RTC `sReferenceData`、`sMultiple`）与 class 4 | 引用表（`SEventReferenceData`）。非内嵌引用按下标取 | EV0 920 字节 = 头 `+0x22` 的 `0xE6=230` 个 uint32 |
| 3 | `fn_14000a8ad` ← class 5（RTC `sUpdateData`） | 更新表（`SEventUpdateData`） | 空 |
| 4 | `fn_14000d85a` | execute type 1 的脚本表 | 空 |

vanilla 段 1/3/4 为空，所以对白与更新全部以 `0xFFFF` 内嵌在 Sc 指令流里。

[确认] execute 字（读 word：`fn_140012ed1`；抽位：`fn_1400c3e20` / `fn_1400c3dc0`）：

| 位 | 抽取 | 含义 |
| --- | --- | --- |
| 0–11 | `fn_140004f43` | 12-bit id。type 2 时是 Sc 记录下标 |
| 12–13 | `fn_14000522c` | type：1 = 段 4 脚本表（vanilla **空**）；2 = `CEventScriptBuffer` 加载 Sc |
| 14 | `fn_14000a5d3` | vanilla 全 0。非 0 时脚本跑完调 `fn_140001690` → `fn_140011da1(DAT_1442943e0)`，语义 [未知] |
| 15 | `fn_140010e06` | vanilla 全 0。非 0 会 `id++` 再取下一条 execute（链式） |

EV0 execute：348 个字，其中 type 2 正好 346 条、type 0 两条（含下标 0 的 `0x0000`）。type 2 的字为 `0x2000, 0x2001, …`（id `0…345`）。触发 `script_id_true = k` 对应 execute[k]，Sc 记录下标 `k-1`。七包同构，type 2 条数 = 触发总数。

### Sc 记录 = 指令流

[确认] `ScN.dk4`：

```
uint32 count;                 /* = 该包 EV 触发总数 */
struct { uint32 off, size; } rec[count];
/* rec[0].off == 4 + count*8，目录之后紧跟载荷 */
```

每条记录：

```
uint16 magic = 0x5344;        /* 'D''S' */
uint16 version;               /* 与 EVN 版本字相同 */
uint32 payload_bytes;         /* = size - 8 */
uint16 insn[];                /* 16 位指令流；对白以 GBK 0 结尾内嵌 */
```

[确认] 加载方是 `CEventScriptBuffer`（`CEvent+0xa0`）。`+0x8` = 当前已加载的记录 id，构造写 `0xFFF`（12 bit 全 1 = 未加载）。`fn_1400016db(buffer, id)` 仅在 `id != +0x8` 时通过 `fn_1400095a7` 重新打开 `SC/Sc{n}.dk4` 取第 `id` 条；文件名表 VA `0x1406828c8`，下标是 `fn_140004a1b(DAT_144296500)` = `CDk4Game+0x7c`（剧本号）。

[确认] 解释器实际执行的是 **DS 头之后** 的第一个 uint16：若把 `'DS'`（`0x5344`）当指令，class=2、subclass=19，超过 class 2 的 `ja 0xB`，七包每条脚本都会立刻进默认支。首指令直方图（下节）与 handler 对得上，说明执行起点不在魔数上。[推断] 8 字节头是在 `fn_1400016db` / `fn_1400095a7` 或 `SEventScript` 构造里跳过的，未钉到那一条 `add ptr, 8`。

[确认] Sc0 第一条 payload 内嵌 GBK「这是非洲的」「霸者之证」——对白在 Sc 里，不在 EV 里。

### 指令字格式（ISA）

[确认] 一条指令至少一个 uint16。当前字：

```
15        13  12        8  7           0
┌──────────┬───────────┬──────────────┐
│  class   │ subclass  │    imm8      │
│  3 bit   │  5 bit    │    8 bit     │
└──────────┴───────────┴──────────────┘
```

- class = bits 13–15，`fn_14001287d`。合法 0–6，默认支把 `sScriptResult` 写成 3
- subclass = bits 8–12，`fn_14000ccd4`。每个 class 自己的 `ja` 上限
- imm8 = bits 0–7，再加后续字。读操作数按 **字下标**（`fn_140012ed1(seeker, i)` 取 `ptr + i*2`）；但 `lNextOffset` 是 **字节** 偏移：分派入口 `fn_14000508d` 先写 `lNextOffset = 2`（一个字），handler 再加字节数；`fn_1400020fe` 用 `ptr + movsxd(lNextOffset)` 前进（`0x1400c3d09`）
- `fn_1400e4640`：把 PC 向上对齐到偶数（内嵌变长 blob 之后）

class 跳表（`0x1400e23c0`，7 项）：

| class | 处理 | RTC / 观察 | 子表上限 |
| --- | --- | --- | --- |
| 0 | `fn_1400017f8` `0x1400dffc0` | 流程：结束 / 跳转 / 谓词分支 / 等待（见下表） | subclass `ja 5`（6 槽） |
| 1 | `fn_14000fede` `0x1400e0210` | RTC `sMenuData`；`CScriptMenuData` | 0/1/2 |
| 2 | `fn_140007199` `0x1400e0780` | RTC `eType`、`sElementMessage`、`cStillInformation` | `ja 0xB`（12 槽） |
| 3 | `fn_1400074d2` `0x1400e16d0` | 音频：调 `DAT_14429d550`（声音管理器；Game Over 前也用它放 BGM `0x38/0x39`） | `ja 5`，槽 4 与 5 同一入口（空操作） |
| 4 | `fn_14000f3e4` `0x1400e0520` | RTC `sRefData`；走与触发器相同的 `fn_14000eb1a` | 0/1 |
| 5 | `fn_140011a86` `0x1400e20b0` | RTC `sUpdateData`；`SEventUpdateData` | 只认 subclass 0 |
| 6 | `fn_14000847c` `0x1400e1970` | 对世界发命令：小游戏、切场景、人物、结束剧本 | `ja 6`（7 槽） |

[确认] `sScriptResult` 在 `fn_1400020fe`：

| 值 | 行为 |
| --- | --- |
| 0 | 停 |
| 1 | **改写本触发的下一条 execute id 后停**。命中路径（`fire≠0`）走 `fn_14001107c`：写 dword[0] bits 16–29 = 值，值为 0 时再置 bit 31 禁用；未命中路径走 `fn_140006587`：写 bits 0–13 = 值 |
| 2 | 继续；谓词改成 result 的第二个 dword（`sScriptResult` 8 字节 = {code, value}） |
| 其它（handler 常写 3） | 继续，按 `lNextOffset` 前进 |

这条 result 1 是原版**多段剧情的状态机**：一条触发跑完脚本后，把自己下次要跑的 execute id 改掉。触发记录（含运行时位）随 `CEvent` 存盘（`vfunc_06/07`）。

class 0 的 6 个 subclass（跳表 `0x1400e0180`）：

| sub | 操作数 | 行为 |
| --- | --- | --- |
| 0 | 字 1：bit 15 = 有续接，bits 0–14 = 新 execute id | 有续接 → result 1（见上表）；否则 result 0（结束） |
| 1 | 字 1：无符号字节偏移 | 无条件跳：`lNextOffset = 4 + 字1`（只能向前） |
| 2 / 3 / 4 | 字 1：同上 | 谓词 == 0 / 1 / 2 时跳，否则跨过操作数 |
| 5 | imm8 | `fn_140014b37(DAT_1442a7210, imm8 × 100)`：等待 imm8 × 100 ms（忙等 + 消息泵） |

class 3 的 6 个 subclass（跳表 `0x1400e18c8`）：

| sub | 操作数 | 行为 [推断] 名 |
| --- | --- | --- |
| 0 | imm8 | `fn_140003dff(snd, imm8)`：停 / 淡出 BGM |
| 1 | imm8 | `0xFF` → `fn_1400151d6` 恢复当前场景 BGM；否则播 BGM `imm8 + 2` |
| 2 | imm 7 bit | `fn_14000e9df(snd, id, 0)`；若 `fn_14001346c(id)` 先 `fn_140007fe0` |
| 3 | imm 7 bit | `fn_14000cf1d(snd, id, 2, 1)` 播音效；若 `fn_14001346c(id)` 先停 BGM 并改模式 1 |
| 4 / 5 | — | 空操作 |

class 6 的 7 个 subclass（跳表 `0x1400e1efc`）：

| sub | 入口 | 立即数 | 副作用 |
| --- | --- | --- | --- |
| 0 | `0x1400e19e7` | | 只把 result 写成 3 |
| 1 | `0x1400e1a06` | 游戏号 = 操作字**低 8 位**（`fn_140005c13` → `fn_1400e2a70(word, 0, 7)` 取位域 0–7），内层 `ja 6`（跳表 `0x1400e1f18`）：0 `CCoinGameView`、1 Nim（`CNimGameManager`）、2 `CAcrossGameView`、3 `CWaterGameView`、4 `CCatchGameView`、5 `CChurchGameView`、6 `CStoneWallGameView`；>6 不启动 | **运行小游戏**，返回值经 `fn_14000f86c` 写成 result 2 的谓词，供 class 0 分支 |
| 2 | `0x1400e1ab7` | imm8 bit 7 = 带设施号，bits 0–6 = 设施号（缺省 `0xD` 街区）。港口取玩家对象的港口；若为 `0x62`（98，无港口哨兵）改用当前所在港 | 构造 `SSceneDesc` 交 `CEvent::fn_14000c5e0` 切到港口场景；不带设施号时置 `CEvent+0xbc=1`（本轮扫描到此为止） |
| 3 | `0x1400e1ba3` | `town_id &lt; 0x61`；`0xFF` 改成当前港；`person_id &lt; 0xEB` | 把人物放到港口（`fn_1400047a5`） |
| 4 | `0x1400e1c86` | `person_id &lt; 0xEB`，排除哨兵 `0x46` | 入队 / 更新人物会话 |
| 5 | `0x1400e1d80` | imm8：0 → 流程态 4；1 → 流程态 3、参 1；其它 → 流程态 0 | **结束剧本**：0 = 通关（片尾 + 画廊记账），1 = 剧情 Game Over，其它 = 回标题。写 `CDk4Game+0x88`，不是扫描组。result 0（脚本停） |
| 6 | `0x1400e1df3` | imm 0–5 档；人物常量 `0x3E=62` | 先 `fn_140014169(person62, 档)`，再走与 sub 4 相同的人物会话。对象写死 62 |

[确认] 原版七个小游戏**只由脚本 6.1 启动**，设施不直接开小游戏。`CChurchGameView` 唯一构造链也走这里（`fn_1400020fe` → … → `fn_14000847c`）；港内「教会」地图点（kind 10）走 `CMpRemains`，是按日期的仪式（`target=(town_id+1)%28+1` 当天给奖励），与小游戏 5 无关。见 `08-town-life/03-minigames.md`、`05-inn-church-palace.md`。重构：town-life 不持「点设施 → 小游戏」硬路径；要做「设施里玩小游戏」就写一条 `facility_entered` 触发 + `run_minigame` 命令。

[确认] class 6 不直接 `call AddLoad`。增减货物在 `fn_14000e7eb`（`0x1400e8fc0`）：商品 `&lt; 0x69`，`AddLoad` 的调用点 `0x1400e9079`。它是引用/更新路径上的动作，不是 class 6 的 7 槽之一。

[确认] 置旗：`fn_140004e4e`（`0x1400e7b30`）对 `CEventData+8` 调 `fn_14000ac81`。脚本动作侧与 `SEventFlags` 同一套 256 bit。

### 指令直方图（只发布「脚本第一条指令」）

完整长度表未闭合：class 2 会把 GBK 对白内嵌在指令后面，按固定 2 字去切会把汉字当成 opcode（class 7 伪指令）。**禁止**把「整段 uint16 直方图」当成 ISA。

表与 YAML 里的「PC+N」一律指 handler 对 `lNextOffset` 追加的**字节**数（在初值 2 之上）；2.0 的 +4、2.1 的 +22、2.8 的 +2/+4 已对反汇编核过。

下面是七个 Sc 包、2121 条记录、**跳过 8 字节 DS 头之后第一个 uint16** 的 (class, subclass) 计数。这是「一条脚本以什么开头」，不是动态执行次数。

| class.sub | 高字节 `(class&lt;&lt;5)\|sub` | 次数 | 已命名？ |
| --- | --- | --- | --- |
| 2.0 | `0x40` | 572 | display 默认（对白 / 0xFFFF 内嵌串） |
| 3.0 | `0x60` | 491 | audio 0 |
| 3.1 | `0x61` | 353 | audio 1（`0xFF` = 当前） |
| 4.0 | `0x80` | 202 | 引用查询 |
| 5.0 | `0xA0` | 168 | 引用更新 |
| 2.8 | `0x48` | 88 | display 8（静画/对白变体，PC+2，flag 0 时再 +4） |
| 2.6 | `0x46` | 80 | unknown_0x46 |
| 3.3 | `0x63` | 55 | audio 3 |
| 0.0 | `0x00` | 48 | flow 0 |
| 2.1 | `0x41` | 30 | 9 路选择分支（`lNextOffset += 22` 字节；扫 9 槽，取「未排除且值最大」的槽，`lNextOffset` 再加该值；全排除时走 `0xA` 哨兵支）[推断] |
| 4.1 | `0x81` | 13 | 引用查询（查事件旗 `fn_14000a0d8`） |
| 2.3 | `0x43` | 9 | unknown_0x43 |
| 2.5 | `0x45` | 6 | unknown_0x45 |
| 2.11 | `0x4B` | 4 | unknown_0x4B |
| 6.4 | `0xC4` | 1 | 人物入队 |
| 2.4 | `0x44` | 1 | unknown_0x44 |

World0 剧本（EV0–3）第一条以 class 2.0 最多；World1（EV4–6）第一条以 class 3.0 / 3.1（音频）更多。这是内容差异，不是两套 ISA。

解释器跳表里还有一些 subclass **从未当过脚本首指令**（例如 class 6.0–3、6.5、6.6，class 1 菜单）。它们出现在对白之后，要等长度表闭合才能计数。仍然是合法 opcode，配置里按跳表登记，key 用 `unknown_0xNN`。

## 3. 实体与 ID

| 实体 | 原版 | 空值 | 备注 |
| --- | --- | --- | --- |
| `trigger_kind` | 0–8，`NUM_TRIGGERKIND=9` | 请求构造器默认 `0xA` | 「哪种场景切换在问」，见第 2 节 kind 表。不是条件枚举 |
| `trigger_id` | 每包 250–346 | 无 | 重构 `(pack, index)`；execute 下标 = `index+1` |
| `execute_id` | 触发里 14 bit，execute 字里 12 bit | 0 = 不跑；`0x3FFF` = 剧情 Game Over（流程态 3、参 1） | type 2 的 12 bit 与 Sc 记录 0-based 对齐 |
| `opcode_id` | `(class&lt;&lt;5) \| subclass`，即指令高字节 | 非法 class &gt; 6 进默认支 | 稳定 id；未命名用 `unknown_0xNN` |
| `flag_id` | 0–255 | 未置位 = 0 | 见 `02-event-system.md` |
| `ref_kind` | 0–14 | 无 `INVALID` 符号 | 引用条件 / class 4 共用 |
| `game_flow_state` | 0–5（`CDk4Game+0x88`） | — | 不属叙事；叙事只发「结束剧本」命令 |
| `minigame_id` | 0–6（class 6.1 内层） | — | 外键到 `08-town-life/03-minigames.md` |
| `town_id` / `force_id` / `person_id` / `item_id` / `trading_item_id` | 各系统 | 各 `invalid`；脚本里另有 `0xFF`/`0xFE`/`0x62`/`0x46` | 只持 ID |
| `script_blob` | Sc 记录 payload | 空 payload 禁止当「已译码」 | 未识别的尾部进 `opaque` |

`SMemorySeeker` 是游标，不是玩法实体。`CEventScriptBuffer+0x8=0xFFF` 是「当前未加载」：`fn_1400016db` 拿它与请求 id 比，不等才重载 [确认]。不要当成 `max_script_bytes`。

哨兵（内容包实例，进 `ids` / caps，不写进解释器）：

| 哨兵 | 值 | 出现处 |
| --- | --- | --- |
| 任意 / 跳过 | `0xFF`、`0xFE` | `ctx`、引用、港口 |
| 无港口 → 用当前港 | `0x62`（98） | class 6.2（玩家港口为 98 时改用所在港）；class 6.3（港口越界时置 98） |
| 内嵌 blob | `0xFFFF` | class 2 / class 4 |
| 无 execute | `0` | execute[0]、false 脚本 |
| 解释器出口 | `0x3FFF` | 14-bit 全 1 |
| 无人物 | `0x46`（70） | class 6.3/6.4、场景描述 `+0x10`（无对手） |
| 随机数上界 | `0x3E8`（1000） | 引用类型 0 千分比判定 |
| 月 = 主角生日月 | `0xC` | Time 块、引用类型 6 |
| 日 = 主角生日日 | `0x1F` | Time 块、引用类型 6 |
| 时间字段通配 | `0xFF` | Time 块（引用类型 6 另把年 ≥ 99、月 > 12、日 > 31、拍 ≥ 48 解成 `0xFF`） |

## 4. 算法 / 公式

### 抽位

```
extract_bits(word, lo, hi) = (word >> lo) & ((1 << (hi - lo + 1)) - 1)
extract_bit(word, n)       = (word >> n) & 1
```

| 参数 | 原版 | 落点 |
| --- | --- | --- |
| `opcode_class_bits` | 13–15 | `opcodes.yaml` |
| `opcode_subclass_bits` | 8–12 | 同上 |
| `execute_id_bits` | 0–11 | 同上 |
| `execute_type_bits` | 12–13 | 同上 |
| `script_id_bits` | 14（触发 dword[0] 两段各 14 bit） | `trigger_kinds.yaml` / events 提取器 |
| `max_trigger_kind` | 8（`ja`） | 运行时读表长；原版包 9 槽 |
| `max_opcode_class` | 6 | caps / opcodes |
| `max_ref_kind` | 14 | opcodes / triggers |

硬编码循环上界视为 bug：`NUM_TOWN=97` 只是原版包的港口数。脚本里的 `&lt; 0x61` 在提取器里改成 `&lt; towns.length`，哨兵 `0x62` 改成独立 key `current`。

### 开火

```
scan(kind, ctx):
    for trigger in pack.triggers_of(kind):
        if not game.running: break                 # CDk4Game+0x88 == 1
        if trigger.disabled: continue              # dword0 bit31
        hit  = match_kind_ctx(kind, trigger, ctx) and match_optional_blocks(trigger, ctx)
        fire = hit and not trigger.latched         # dword0 bit15
        if not hit: trigger.latched = 0
        id = trigger.script_id_true if fire else trigger.script_id_false
        if id != 0:
            open_event_presentation_once()         # fn_14000f02e
            run_execute(id, trigger, fire)
        if fire:
            trigger.fired = 1                      # dword0 bit30
            if trigger.one_shot: trigger.disabled = 1   # dword1 bit31
            if trigger.relatch:  trigger.latched  = 1   # dword1 bit30
        if event.stop_scan: break                  # CEvent+0xbc，class 6.2 可置
```

`match_kind_ctx` 只比较 kind 字段（上表）。日期、旗、引用、多港口、战斗结果全部在 optional blocks 里，**所有 kind 共享**。来源：`fn_140013c32` body `0x1400bdee0`。vanilla 文件里 `one_shot` 310/346、`relatch` 325/346（EV0），所以「一次性」是每条触发自带的数据位，不是引擎默认。

### 执行

```
run_execute(id):
    if id == 0: return
    if id == exit_id:          # 原版 0x3FFF
        Session.end_scenario(game_over{ending_id: natural_death}, cause: scripted)   # 原版参 1
        return
    word = execute_table[id]
    typ  = extract_bits(word, 12, 13)
    sub  = extract_bits(word, 0, 11)
    if typ == 1: run_script(script_table[sub])     # vanilla 空
    if typ == 2: run_script(load_sc(sub))          # Sc 记录 payload
    if extract_bit(word, 15): run_execute(id+1)    # vanilla 未用
```

```
run_script(words, pc=0, pred=0):
    while pc in range:
        class = extract_bits(words[pc], 13, 15)
        sub   = extract_bits(words[pc], 8, 12)
        imm   = extract_bits(words[pc], 0, 7)
        delta = 2                                  # 字节
        result, value, delta = dispatch(class, sub, imm, words, pc, pred, delta)
        if result == 0: return
        if result == 1:                            # class 0.0 带续接
            trigger.set_next_execute(value, fire)  # 0 → disabled
            return
        if result == 2: pred = value               # class 6.1 小游戏结果等
        pc += delta                                # 字节
```

`delta` 以**字节**计（初值 2 = 一个字）。对白 / 引用内嵌 blob：先按 handler 读长度，再 `align_even(pc)`。未识别指令不得猜 `delta`；提取器把从这里到记录末尾写入 `opaque`。

## 5. 耦合分析

原版焊点：

1. **文件名 = 剧本号** `EV{n}` / `Sc{n}`。重构后由 `scenarios.yaml` 指出 `event_pack` / `scene_pack`。
2. **kind 请求拷进 `CEvent+0x88…+0x9c`**。这是「世界现在在哪」的快照，由三个场景切换钩子 + 探索 + 登陆共 8 个调用点手工拼。重构改为求值时 LIVE 查询：`Town.current()`、`Calendar.snapshot()`、`Combat.last_result()`、`Flags.test()`。禁止事件对象持有 `World*` / `Player*`。
3. **class 6.5 与 `0x3FFF` 直接写 `CDk4Game` 流程态**（通关 / Game Over / 回标题）。叙事越权改了游戏主循环。重构改为发命令 `Session.end_scenario(outcome, cause)`（DECISIONS D8），由会话层决定片尾、画廊记账、回标题；`game_over` 带 `ending_id`，会话层发 `ScenarioFailed { scenario, cause }`。四个原版入口的映射：`0x3FFF` 与 6.5 imm 1 → `game_over{ending_id: natural_death}`、`cause: scripted`（叙事发，导入器生成）；本队覆灭 / 舰队解散 → `player_fleet_lost`（进度系统自己调同一命令，不经叙事）。
8. **class 6.1 直接 `new` 七种小游戏 View**，把返回值塞进谓词。重构改为 `Minigames.run(minigame_id) -> result_int`，叙事只拿整数。
9. **剧情进度藏在触发记录的运行时位里**（`latched/fired/disabled` + 可改写的 execute id）。重构把它们拆成存档里的 `trigger_state[]`，定义（EV 文件）只读。
4. **引用类型 8 直接 `CountFleetItem`，动作路径直接调原版 `AddLoad`**。允许，但必须走货舱所有者 ships 的 `Ships.count_item(fleet_id, item_id)` / `Ships.add_load(fleet_id, item_id, delta)`。脚本只存 ID。
5. **class 2 / Sc 内嵌 GBK**。文案属于内容包，逻辑只持 `string_key`。提取器把 GBK 抽成 i18n，指令里留 key。
6. **BGM（class 3）写音频管理器**。叙事发 `Audio.play(clip_id)`，不持设备指针。
7. **其它系统若写 `if opcode==0xC3`** 即违规。opcode 的含义属于叙事 ISA；货舱只认收到的 `Ships.add_load` 命令。

`CEvent` 是 `CSaveObject`：旗、触发记录（含运行时位与改写过的 execute id）进存档。EV / Sc 原文不进存档。

## 6. 解耦后的配置

跳表有 37 个 class.subclass 槽。能命名的命名；不能命名的写成 `unknown_0xNN` 并挂证据。**禁止**补一套「完整剧情汇编语言」。

```yaml
# config/narrative/trigger_kinds.yaml
schema: dk4.narrative.trigger_kinds
version: 2
# kind = 「哪种世界事件在问」。NUM_TRIGGERKIND=9 是原版包槽数，运行时读表长。
# 引擎只认 key；整数 id 只是原版包的编码。模组加 kind 用命名空间 key（mymod:dock_repair）。
kinds:
  - id: 0
    key: item_discovered          # 海上探索找到道具
    source_event: DiscoveryFound   # 06：DiscoveryFound { fleet_id, kind: item, id }
    ctx: { field: id, compare: eq }
  - id: 1
    key: battle_started
    source_event: BattleStarted
    ctx: { field: opponent, compare: by_subtype, subtypes: [opponent_attr_a, opponent_force, opponent_attr_c, any] }
  - id: 2
    key: battle_ended
    source_event: BattleResolved
    ctx: { field: opponent, compare: by_subtype, subtypes: [opponent_attr_a, opponent_force, opponent_attr_c, any] }
    implicit_blocks: [close_battle]   # 原版 kind 2 无条件构造 SEventTriggerCloseBattle
  - id: 3
    key: fleet_contact
    source_event: FleetSpotted
    ctx: { field: opponent, compare: by_subtype, subtypes: [opponent_attr_a, opponent_force, opponent_attr_c, any] }
  - id: 4
    key: port_arrived             # 海图 → 港口
    source_event: ArrivedAtTown
    ctx: { field: town_id, compare: by_subtype, subtypes: [town_or_any, town_attr_1, town_attr_2, town_list] }
  - id: 5
    key: in_town
    source_event: [TownTick, FacilityEntered]   # 06：港口场景 tick（scene=town）；或进港时设施 = 街区 0xD
    ctx: { field: town_id, compare: by_subtype, subtypes: [town_or_any, town_attr_1, town_attr_2, town_list] }
  - id: 6
    key: at_sea
    source_event: SeaTick         # 06；scene=sea；[推断] 日粒度：调用方 fn_140010a00 带 RTC nDay/nMonth
    ctx: { field: fleet_town_id, compare: by_subtype, subtypes: [town_or_any, town_attr_1, town_attr_2, town_list] }
  - id: 7
    key: facility_entered
    source_event: FacilityEntered
    ctx: { field: town_id, compare: by_subtype, subtypes: [town_or_any, town_attr_1, town_attr_2, town_list] }
    extra: { field: facility_id, bits: [18, 21] }
  - id: 8
    key: village_landed
    source_event: VillageLanded
    ctx: { field: village_id, compare: eq }
# ctx 缺省 = 任意港。原版 0xFF（任意）/ 0xFE 只在 ids/legacy_codec.yaml（DECISIONS D1）
```

```yaml
# config/narrative/opcodes.yaml
schema: dk4.narrative.opcodes
version: 1
word_bits: 16
class_bits: [13, 15]
subclass_bits: [8, 12]
imm_bits: [0, 7]
exit_execute_id: 0x3FFF
no_town_sentinel: none            # 原版 0x62；6.2 遇到它改用当前港
inline_blob_sentinel: 0xFFFF

# id = (class << 5) | subclass = 指令高字节
opcodes:
  # class 0  flow
  - { id: 0x00, class: 0, subclass: 0, key: end_or_rearm, operands: [rearm_flag_bit15, next_execute_id_bits0_14], side_effect_system: narrative, evidence: "jt 0x1400e0026; bit15→result 1 改写触发 execute id; 否则 result 0" }
  - { id: 0x01, class: 0, subclass: 1, key: jump, operands: [byte_offset_u16], side_effect_system: narrative, evidence: "jt 0x1400e007d; lNextOffset = 4 + word1" }
  - { id: 0x02, class: 0, subclass: 2, key: branch_pred_eq_0, operands: [word], side_effect_system: narrative, evidence: "cmp pred,0" }
  - { id: 0x03, class: 0, subclass: 3, key: branch_pred_eq_1, operands: [word], side_effect_system: narrative, evidence: "cmp pred,1" }
  - { id: 0x04, class: 0, subclass: 4, key: branch_pred_eq_2, operands: [word], side_effect_system: narrative, evidence: "cmp pred,2" }
  - { id: 0x05, class: 0, subclass: 5, key: wait, operands: [imm8_x100ms], side_effect_system: ui, evidence: "jt 0x1400e0132; fn_140014b37(DAT_1442a7210, imm8*100)" }

  # class 1  menu
  - { id: 0x20, class: 1, subclass: 0, key: menu_open, operands: [], side_effect_system: ui, evidence: "sMenuData; result=3 wait" }
  - { id: 0x21, class: 1, subclass: 1, key: unknown_0x21, operands: [word], side_effect_system: ui, evidence: "PC+2 then extra" }
  - { id: 0x22, class: 1, subclass: 2, key: unknown_0x22, operands: [word, word], side_effect_system: ui, evidence: "旧稿记 PC+3，奇数字节不合 lNextOffset 字节语义，未复核 [未知]" }

  # class 2  display / talk / still
  - { id: 0x40, class: 2, subclass: 0, key: say, operands: [msg_id_or_inline], side_effect_system: ui, evidence: "sElementMessage; 0xFFFF 内嵌 GBK; lNextOffset += 4 字节; 非内嵌 id 查段 1; 首指令 572 次" }
  - { id: 0x41, class: 2, subclass: 1, key: select_branch_9, operands: [slot_x9], side_effect_system: narrative, evidence: "lNextOffset += 22 字节; 循环 i < 9; 0xA = 无选中; 语义 [推断]" }
  - { id: 0x42, class: 2, subclass: 2, key: unknown_0x42, operands: opaque, side_effect_system: ui, evidence: "jt 0x1400e0bb0" }
  - { id: 0x43, class: 2, subclass: 3, key: unknown_0x43, operands: opaque, side_effect_system: ui, evidence: "jt 0x1400e0c00; 首指令 9 次" }
  - { id: 0x44, class: 2, subclass: 4, key: unknown_0x44, operands: opaque, side_effect_system: ui, evidence: "jt 0x1400e0c85; 首指令 1 次" }
  - { id: 0x45, class: 2, subclass: 5, key: unknown_0x45, operands: opaque, side_effect_system: ui, evidence: "jt 0x1400e0d04; 首指令 6 次" }
  - { id: 0x46, class: 2, subclass: 6, key: unknown_0x46, operands: opaque, side_effect_system: ui, evidence: "jt 0x1400e0e02; 首指令 80 次" }
  - { id: 0x47, class: 2, subclass: 7, key: unknown_0x47, operands: opaque, side_effect_system: ui, evidence: "jt 0x1400e0f36" }
  - { id: 0x48, class: 2, subclass: 8, key: say_or_still, operands: opaque, side_effect_system: ui, evidence: "PC+2 再条件 +4; 首指令 88 次; EV0 脚本 0 以此开头后跟「霸者之证」" }
  - { id: 0x49, class: 2, subclass: 9, key: unknown_0x49, operands: opaque, side_effect_system: ui, evidence: "jt 0x1400e1162" }
  - { id: 0x4A, class: 2, subclass: 10, key: unknown_0x4A, operands: opaque, side_effect_system: ui, evidence: "jt 0x1400e119c" }
  - { id: 0x4B, class: 2, subclass: 11, key: unknown_0x4B, operands: opaque, side_effect_system: ui, evidence: "jt 0x1400e124a; 首指令 4 次" }

  # class 3  audio
  - { id: 0x60, class: 3, subclass: 0, key: bgm_stop, operands: [imm8], side_effect_system: audio, evidence: "fn_140003dff(snd, imm8); 名 [推断]; 首指令 491 次" }
  - { id: 0x61, class: 3, subclass: 1, key: bgm_play, operands: [clip_or_ff], side_effect_system: audio, evidence: "0xFF → fn_1400151d6 恢复场景 BGM; 否则 fn_1400018a7(snd, imm8+2); 首指令 353 次" }
  - { id: 0x62, class: 3, subclass: 2, key: unknown_0x62, operands: [imm7], side_effect_system: audio, evidence: "jt 0x1400e17c4; fn_14000e9df(snd, id, 0)" }
  - { id: 0x63, class: 3, subclass: 3, key: se_play, operands: [imm7], side_effect_system: audio, evidence: "jt 0x1400e1825; fn_14000cf1d(snd, id, 2, 1); 名 [推断]; 首指令 55 次" }
  - { id: 0x64, class: 3, subclass: 4, key: nop_0x64, operands: [], side_effect_system: none, evidence: "与 subclass 5 同一入口 0x1400e1898，只写 result 3" }
  - { id: 0x65, class: 3, subclass: 5, key: nop_0x65, operands: [], side_effect_system: none, evidence: "同上" }

  # class 4  query reference
  - { id: 0x80, class: 4, subclass: 0, key: query_ref, operands: [ref_id_or_inline], side_effect_system: narrative, evidence: "sRefData; 0xFFFF 内嵌; 走 fn_14000eb1a; 首指令 202 次" }
  - { id: 0x81, class: 4, subclass: 1, key: query_flag, operands: [flag_pack], side_effect_system: narrative, evidence: "fn_14000a0d8; 首指令 13 次" }

  # class 5  mutate reference
  - { id: 0xA0, class: 5, subclass: 0, key: update_ref, operands: [ref_id_or_inline], side_effect_system: narrative, evidence: "SEventUpdateData; result=3; 首指令 168 次。具体写入（旗/货/人）看内嵌 ref_kind" }

  # class 6  world commands
  - { id: 0xC0, class: 6, subclass: 0, key: nop_0xC0, operands: [], side_effect_system: none, evidence: "只写 result 3" }
  - { id: 0xC1, class: 6, subclass: 1, key: run_minigame, operands: [minigame_imm8_low], side_effect_system: minigames, evidence: "游戏号 = 操作字 bits 0-7 (fn_140005c13); 内层跳表 0x1400e1f18: Coin/Nim/Across/Water/Catch/Church/StoneWall; >6 不启动; 返回值 → 谓词" }
  - { id: 0xC2, class: 6, subclass: 2, key: goto_town_scene, operands: [facility_flag_bit7, facility_id_bits0_6], side_effect_system: ui_scenes, evidence: "玩家港口 98 → 当前港; SSceneDesc → fn_14000c5e0; 无设施号置 CEvent+0xbc 停扫" }
  - { id: 0xC3, class: 6, subclass: 3, key: move_person_to_town, operands: [town_id, person_id], side_effect_system: characters, evidence: "town<0x61 person<0xEB; fn_1400047a5" }
  - { id: 0xC4, class: 6, subclass: 4, key: join_person, operands: [person_id], side_effect_system: characters, evidence: "排除 0x46; 首指令 1 次" }
  - { id: 0xC5, class: 6, subclass: 5, key: end_scenario, operands: [outcome_imm8], side_effect_system: session, evidence: "0→CDk4Game 流程态 4 通关; 1→流程态 3 参 1 Game Over; 其它→流程态 0 回标题; result 0" }
  - { id: 0xC6, class: 6, subclass: 6, key: unknown_0xC6, operands: [slot_0_5], side_effect_system: characters, evidence: "fn_140014169(person 0x3E, slot) 后同 0xC4 会话; 写死 person 0x3E" }

# 不在 class 6 跳表、但脚本动作路径会发的命令
side_commands:
  - { key: add_load, side_effect_system: ships, evidence: "fn_14000e7eb → AddLoad; 商品 < 0x69" }   # Ships.add_load
  - { key: set_flag, side_effect_system: narrative, evidence: "fn_140004e4e → fn_14000ac81" }
  - { key: count_item, side_effect_system: ships, evidence: "条件 fn_1400029ff → CountFleetItem; 不是 opcode class" }   # Ships.count_item
```

触发器译码完成后的 `when` / `then` 只许出现 ID 与纯值（与 `02` 第 6 节升级路径一致）：

```yaml
when:
  kind: battle_ended            # 原版 kind 2
  battle_slot: 0
  time_mode: 1
  calendar: { year: any, month: birthday, day: birthday, time_of_day: any, compare: match }   # 原版月 0xC / 日 0x1F = 主角生日；0xFF = any；time_mode 1 = 逐字段相等（01-world/07 规范形式）
  flags_all: [flag.intro]
  ref:
    kind: cargo                 # 原版 ref 8
    trading_item: vanilla:item_t3
    cmp: ge
    count: 1
then:
  commands:
    - { op: say, string_key: ui.event.ev0.000 }
    - { op: menu, choices: [ui.event.yes, ui.event.no] }
    - { op: set_flag, flag_id: 12 }
    - { op: add_load, fleet: player, trading_item: vanilla:item_t3, delta: -1 }
    - { op: move_person_to_town, person: vanilla:person_7, town: vanilla:lisbon }
    - { op: run_minigame, minigame: nim, store: pred }      # 原版 0xC1 imm 1
    - { op: if, pred_eq: 1, then: [ { op: end_scenario, outcome: cleared } ] }   # 0xC5 imm 0
    - { op: rearm, next: stage_2 }                          # 0x00 bit15：本触发下次跑 stage_2
```

### 6.1 新引擎脚本化方案：结论

原版 ISA 很小：37 个 class.subclass 槽；跳转只向前（0.1–0.4 的偏移是无符号字节）；分支只看谓词 0/1/2；没有循环、没有变量，只有 256 旗 + 触发自身的运行时位。所以三种方案比较如下：

| 方案 | 优点 | 缺点 |
| --- | --- | --- |
| 保留字节码 VM | 原版 1:1 回放 | 不可读、不可 diff、模组无法按 key 合并；外键校验要先反汇编 |
| 通用脚本语言（Lua 等）为主 | 表达力最强 | 难静态校验外键与存档兼容；模组之间无法合并同一事件；跨系统调用难收口 |
| **数据触发器 + 结构化命令树**（推荐） | 可校验、可 diff、可按 key 覆盖；原版只向前跳 + 谓词分支 → 能无损转成 `if/choice` 树 | 需要一次性写反编译器 |

推荐落地：

1. **规范形式 = 数据**：`when`（kind key + 条件块）+ `then.commands`（结构化树：`if` / `menu` / `run_minigame` / `say` / `end_scenario` / `rearm`…）。原版 Sc 字节码只作导入格式，提取器反编译成命令树；解不了的段落进 `opaque` 并让该触发拒绝加载，而不是运行时猜。
2. **剧情进度显式化**：原版靠 0.0 改写「下次跑哪个 execute」+ `latched/fired/disabled` 位。新引擎写成 `stages:`（每条触发一个小状态机）+ `on_fire: { one_shot, relatch }`，状态存进存档的 `trigger_state[]`，定义只读。
3. **逃生口**：模组需要算法时用 `{ op: call_script, script: mymod:foo }`，脚本运行在沙箱里，只能用第 8 节列出的查询 / 命令，返回值写回谓词。引擎核心不内置 VM。
4. **opcode 表只服务导入器**：`opcodes.yaml` 留给提取器做「字节码 → 命令」映射，运行时不读。

未译码尾部进 `opaque`。禁止用「参考大航海4」的散文代替 `unknown_0xNN`。

## 7. 独立旋钮

改这些不应迫使经济 / 战斗 / 航海改代码：

- 某触发的 kind、可选条件块、`one_shot` / `relatch`
- 某 opcode 的 operand（仍是 ID / 纯值）
- 把一条「日期 + 入港」改成「只看旗」
- 新增 kind（命名空间 key；由新的世界事件源发出；原版解释器写死 9）
- 新增 opcode（登记进 `opcodes.yaml`；原版 class 0–6 是默认包）
- Sc 对白换成另一套 `string_key`
- `no_town_sentinel` / `exit_execute_id` / 通配符
- `end_scenario` 各结局走哪条会话流程（片尾、通关记账、回标题）——属会话层表，不在叙事；通关记账由 profile 订阅 `ScenarioCleared { scenario_id }` 执行（词表 §14-24）

需要其它系统配合的只有：命令里出现的 `town_id` / `item_id` / `person_id` / `trading_item_id` / `force_id` 必须在对应表里存在（加载期外键校验）。`Ships.add_load` 的堆叠规则属于 ships，不在本表。

## 8. 对外接口

本系统订阅（kind 由事件源决定，不在贸易代码里写 kind 号）。原版对应关系已由 `fn_14001337c` 的 8 个调用点钉死：

| 事件 | 来源 | kind |
| --- | --- | --- |
| `ScenarioSelected` | 剧本 | 加载 EV + Sc + EVA |
| `DiscoveryFound {kind: item, id}` | Ocean（探索） | 0 `item_discovered` |
| `BattleStarted {opponent_person_id}` | Combat | 1 `battle_started` |
| `BattleResolved {opponent_person_id, result}` | Combat | 2 `battle_ended` |
| `FleetSpotted {other_fleet_id → 对手 person}` | Ocean | 3 `fleet_contact` |
| `ArrivedAtTown {town_id}` | Ocean → Town | 4 `port_arrived` |
| `TownTick {town_id}`（scene = town）/ `FacilityEntered {facility: street}` | Town-life | 5 `in_town` |
| `SeaTick {fleet_id}`（scene = sea） | Ocean | 6 `at_sea`（ctx = 玩家舰队所在港，`Town.current_for`） |
| `FacilityEntered {town_id, facility_id}` | Town | 7 `facility_entered`（设施 `0xD` 街区按 5） |
| `VillageLanded {village_id}` | Ocean（登陆） | 8 `village_landed` |

原版没有「旗变了再扫」「货变了再扫」：条件块只在上表事件到来时求值。新引擎若加 `FlagChanged` 重扫，是新功能，要单独开关。

查询（LIVE，取不到则本条 no-op）：

```
Calendar.snapshot() -> {year, month, day, era_tag}
Town.current_for(fleet_id) -> town_id | none
Combat.last_result(fleet_id) -> slot_0_3 | none
Flags.test(flag_id) -> bool
Ships.count_item(fleet_id, item_id) -> int        # item_id = 交易品 key
Items.owned(fleet_id, item_id) -> bool
Characters.in_roster(person_id) -> bool
Share.of(town_id, force_id) / Share.exclusive_owner(town_id) -> force_id | null   # 引用类型 10 [推断] 势力；读哪项未钉
Towns.def(town_id) -> TownDef | null             # 存在性 = 非 null
```

命令（脚本可发；接收方是各系统）：

```
Flags.set(flag_id, bool)
Ships.add_load(fleet_id, item_id, delta)          # item_id = 交易品 key
Items.acquire(fleet_id, item_id)          # 引用更新路径，opcode 级未钉
Characters.move_to_town(person_id, town_id)
Characters.join(person_id)
Scenes.goto_town(town_id | current, facility_id?)
Minigames.run(minigame_id) -> int
Session.end_scenario(outcome: cleared | game_over{ending_id} | to_title, cause)   # 脚本来源 cause = scripted
UI.play_still(still_id)
UI.say(string_key)
UI.menu(choices) -> choice_index
Audio.play(clip_id)
Audio.stop()
```

禁止脚本拿到 `CTown*` / `CPerson*`。存档只存：`event_pack` id + schema 版本、256 旗、每条触发的 `trigger_state`（`latched/fired/disabled` + 当前 stage / execute id）。不存 EV / Sc 原文。原版无「脚本中途存盘」：Sc 脚本在一次扫描里跑完。

「set ship」在跳表和原版 `AddLoad` 路径里 **没有** 独立 opcode。需要改船时走船只系统自己的命令，等引用更新（class 5）译码后再挂，不在本表编一条。

## 9. 未核实字段

| 项 | 现状 | 禁止 |
| --- | --- | --- |
| kind 原版符号名 | 语义已由 8 个调用点钉死，枚举名仍无头文件 | key 用本篇自拟英文名，不冒充原版符号 |
| kind 1–3 的 `+0x94/+0x98/+0x9c` 三个对手属性 | 只有取值函数 | 不要写死为「船型 / 势力」，字段名 [推断] |
| kind 4–7 subtype 1 / 2 比的两种港口属性 | `fn_14001055a`（港口对象 `+0x25` 起）/ `fn_140014e48` | 不要猜成文化圈 / 海域 |
| 场景描述设施号 `0xD` | 进港时 = 5、否则 = 7 | 不要猜具体设施名 |
| 流程态 3 参 1 时的 `fn_14000fb14(0x170…0x173)` | 已解：寿终四句 A–D（`11-progression/03`） | 仍不要当原因枚举；哪条 EV 脚本、什么条件发 `0x3FFF` 未解 |
| Time 四字段的年月日旬 | 生日哨兵 `0xC`/`0x1F`、通配 `0xFF` 已钉；mode 1 = 逐字段相等；mode 2 / 3 是逐字段单向比较（`0x1400bd670` 用 `jge`、`0x1400bd550` 用 `jle`），不是字典序，哪个是「已到」未核参数顺序 | 日历文档负责编码（`01-world/07`）；这里只比 snapshot |
| 引用类型 5、12 | 只有跳表槽 | `unknown_ref_N`。1 / 13 只标 [推断] |
| 引用类型 0 | `rand(1000)` 三态比较 | 不要写成「金币」 |
| 引用类型 10 的 25 | 函数内与 `0x1A` 空势力比较 | 势力 [推断]，不要写成海域 |
| class 2 多数 subclass | 跳表在，长度表未闭合 | 整段对白当 `opaque`，不要猜「立绘槽 3」 |
| class 3 各 subclass 名 | 调同一音频对象；3.1 有「恢复场景 BGM」分支 | `bgm_*` / `se_play` 只是 [推断] 名 |
| class 6.1 七个小游戏的返回值语义 | 只知道写进谓词 | 胜负编码归 `08-town-life/03-minigames.md` |
| class 6.6 的人物 62 | 立即数 `0x3E` | 不要猜成某具名 NPC 再写进确认列 |
| EV 段 2 引用表的单条布局 | 段用途已钉（`sReferenceData`） | 单条字段进 `opaque` 直到逐位核完 |
| 跳过 DS 8 字节的具体指令 | 执行起点不在魔数上 | 不要写成确认的 `add ptr, 8` 行号 |
| Sc 记录里指令以外的版面字节 | 与「场景图」说法并存 | 不要把 Sc 整包当 G1T（魔数是 `DS` 不是 `GT1G`） |
| set ship / 改炮 / 改帆 | 本 ISA 未见独立 class | 不要编 opcode |
| 整段 opcode 直方图 | 对白污染 | 只发布「首指令」表，直到长度表闭合 |

提取器：EV 头按 `02` 进 YAML；每条触发的位域进 `when`；Sc payload 用跳表能切的指令进 `commands`，其余进 `opaque`。升 schema version 的条件是：某条 `unknown_0xNN` 有了长度 **和** 副作用系统，而不是「看起来像对白」。
