# 事件指令与条件（命令解析器）

配置落点：`config/narrative/opcodes.yaml`、`config/narrative/trigger_kinds.yaml`。触发扫描与 EV 头见 `02-event-system.md`。剧本选包见 `01-scenarios.md`。

本系统是 **bytecode 解释器 + 条件求值器**。内容是 EV 包（触发器 + execute 表）和一一对应的 Sc 包（指令流 + 内嵌对白）。引擎不得把某条剧情编译进 C++。

EV 96 字节头（魔数 `'WE'`、9 个 `eTriggerKind` 计数与载荷偏移）已在 `02-event-system.md` 钉死，本篇只引用，不复述。本篇从 **头之后** 的触发记录、execute 表、Sc 指令流和解释器跳表往下拆。

## 1. 职责边界

本系统管：

- 把 `eTriggerKind` 0–8 当成 **扫描组**（何时扫），不是把「入港 / 日期 / 持有物」焊成 9 个互斥枚举
- 解析触发记录里的 **可选条件块**（日期、旗、多港口、战斗结束、引用对象）
- 按 execute 表取出脚本 id，加载对应 Sc 记录，逐步执行 16 位指令
- 把指令发出的 **命令** 交给其它系统（货舱、旗、人物、港口、UI、音频）
- 把指令发出的 **查询** 变成 0/1 结果，供后续分支指令使用

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

`id == 0`：`fn_1400050fb` 直接返回（vanilla 的「无脚本」）。`id == 0x3fff`：不跑脚本，调用 `fn_140010578` 把 `CEvent+0x88` 写成 **kind 3**，并把 `+0x90` 写成实参（扫描侧传入 1）。这是解释器的特殊出口，不是一条普通 opcode。

来源：`srcdata/source/game/unassigned/fn_1400c2290.cpp`、`fn_1400b93e0.cpp`、objdump。

### 触发记录：变长、双 dword 头、后面跟可选块

[确认] 单条触发不能按固定 stride 切。长度在 **dword[1] bits 0–7**，单位是 dword。`fn_14000df4e` 抽这个字段，`fn_14001435d` 做 `ptr += length * 4`。七个 EV 包用这条规则切完，每种 kind 的解析终点都等于下一个 kind 的起点（或 execute 段起点）。

[确认] EV0 例：kind 计数 `(5,16,16,4,2,85,10,203,5)` 和 = 346；切出来的字节与头里的 kind 偏移完全吻合。

头两个 dword 的位域（抽位原语 `fn_1400be320(lo,hi)` / `fn_1400be2c0(bit)`，读 dword `fn_14000f20e(index)`）：

| 字段 | 位置 | 用途 |
| --- | --- | --- |
| `script_id_false` | dword[0] bits 0–13 | 条件失败时的 execute 下标。七包 **全部为 0** |
| （运行时） | dword[0] bit 15 | `fn_14000fea2`。文件里全 0；扫描命中后可改写（一次性？见第 9 节） |
| `script_id_true` | dword[0] bits 16–29 | 条件成功时的 execute 下标。EV0 为 `1…346` 各用一次 |
| （运行时 skip） | dword[0] bit 31 | `fn_14000ea9d`。文件里全 0；非 0 则本条不求值 |
| `length_dwords` | dword[1] bits 0–7 | 整条记录长度 |
| `ctx` | dword[1] bits 8–15 | 与 `CEvent+0x8c/+0x94/+0x98/+0x9c` 比较的上下文（港口或其它命名空间） |
| `subtype` | dword[1] bits 16–17 | 0–3。kind 1–3 与 kind 4–7 各用一套 |
| `extra` | dword[1] bits 18–21 | 仅 kind 7 额外与 `CEvent+0x90` 比较 |
| `time_mode` | dword[1] bits 22–23 | 0 = 无时间块；1/2/3 = 三种日历比较 |
| `has_outcome` | dword[1] bit 24 | `SEventTriggerEventOutcome` |
| `has_reference` | dword[1] bit 25 | `SEventTriggerReference` |
| `has_flag_*` | dword[1] bits 26–29 | 四段事件旗块（见下） |
| dword[1] bits 30–31 | | EV0 出现 325 / 310 次。`fn_140001b09` 的 kind 开关 **不读**。[未知] |

`0xFF` / `0xFE` 出现在 `ctx` 里：港口组把它当「任意 / 另一种哨兵」，与 `fn_140001b09` 里 `uint8 == 0xFF` 则跳过比较一致。

### `eTriggerKind` 0–8 是扫描组

[确认] `NUM_TRIGGERKIND = 9`。没有 `TriggerKindID.h`。kind 写在 `CEvent+0x88`（`fn_1400029cd` 把该 dword 初值写成 `0xA`，大于 8，扫描开关直接 `ja` 掉 = 空闲）。

跳表（相对 image base 的目标 VA）：

| kind | 入口 | 与 1–3 / 4–7 的关系 |
| --- | --- | --- |
| 0 | `0x1400bf66a` | 单独。`ctx` 比 `CEvent+0x8c` |
| 1, 2, 3 | `0x1400bf693` | 共享。先抽 subtype，再比 `+0x94` / `+0x98` / `+0x9c` |
| 4, 5, 6, 7 | `0x1400bf744` | 共享。用 `fn_1400103bb(CEvent+0x8c)` **按港口取对象**。kind 7 额外比 `extra` 与 `+0x90` |
| 8 | `0x1400bf884` | 单独。`ctx` 比 `CEvent+0x8c`（与 kind 0 同一字段，不同扫描时机） |

[确认] kind **2** 在公共路径里 **无条件** 构造 `SEventTriggerCloseBattle`（ctor thunk `0x140010f1e`）。这是 0–8 里唯一能钉到 RTTI 类名的扫描组。

[确认] kind 4–7 且 subtype == 3 时构造 `SEventTriggerMultipleTown`。

[确认] 把 kind 写进 `CEvent+0x88` 的小函数（各 23 字节 `mov [obj+0x88], imm`）：

| kind | setter | 已见到的调用点 |
| --- | --- | --- |
| 0 | `fn_140007b58` `0x1400e45e0` | 解释器 class 6.5 默认支；`fn_140004c28`（RTC `asMenu`） |
| 1 | `fn_140006f50` `0x140112aa0` | `0x140111fa0` |
| 2 | `fn_14000b3c0` `0x140115d50` | `fn_140004c28` |
| 3 | `fn_140010578` `0x1400c42d0` | `id==0x3fff`；class 6.5 的 1 支；另两处 `0x1400ef13e`、`0x14010d039` |
| 4 | `fn_14000210d` `0x1400e2440` | class 6.5 的 0 支 |
| 5 | `fn_140014ab0` `0x1400d6b40` | 静画/窗口路径（其中一处 `cmp msg, 0x12`）；`0x140111fc0` |

kind 6、7、8 的 setter **没有** 同样的 `mov [+0x88], 6/7/8` 指令。它们多半由 `mov [+0x88], r32` 写入（文本段里这种存盘有 85 处，未逐条钉到 CEvent）。禁止把「没找到 imm8 写入」写成「这两种 kind 不存在」。

EV0 `ctx` 观察（只当证据，不当枚举名）：

| kind | 条数 | `ctx` 观察 |
| --- | --- | --- |
| 0 | 5 | 150, 154, 153, 48, 32。150+ **不是** `town_id`（港口 0–96） |
| 1 | 16 | 多数 &lt; 97，也有重复的 30、51 |
| 2 | 16 | 含 255、0、30；且必挂 CloseBattle |
| 3 | 4 | 10, 7, 1 |
| 4 | 2 | 254（subtype 3 多港口）、29 |
| 5 | 85 | 大量 255；其余落在 0–96 |
| 6 | 10 | 255 与少量港口 |
| 7 | 203 | 最多。255=任意港，其余 0–96 的港口 id。`extra` 为 1/3/4… |
| 8 | 5 | 1, 18, 22, 6 |

[推断] kind 4–7 是「当前有港口」扫描组，kind 7 是主港口扫描（条数过半 + `0xFF` 任意港）。依据：共享 `fn_1400103bb(town)`，`ctx` 落在 `town_id` / `0xFF` / `0xFE`。**不是** 头文件里的「kind 7 = 入港」符号。

### 可选条件块（所有 kind 在 kind 开关之后共用）

求值顺序（`fn_140001b09` 汇合点 `0x1400bf8a8` 之后）：

1. **EventOutcome**（bit 24）：两串下标，分别要求「已发生」和「未发生」。内部走 `fn_14000a0d8`（`0xFFFF` 跳过）。
2. **Time**（bits 22–23 ≠ 0）：ctor `SEventTriggerTime`。抽出 4 个字段构造日历再比。月份字 == `0xC`（12）时改成「当前月」；日字 == `0x1F`（31）时改成「当前日」——通配符，不是 13 月 / 32 日。mode 1/2/3 走三个比较函数 `0x1400bd3b0` / `0x1400bd670` / `0x1400bd550`。
3. **CloseBattle**（仅 kind == 2）：按 `CEvent+0x90` 的 0/1/2/3 与记录里四根标志位对齐。对不上则失败。
4. **MultipleTown**（kind ∈ 4–7 且 subtype == 3）：当前 `town_id` 必须落在名单里。
5. **EventFlag ×4**（bits 29, 28, 27, 26）：每段构造 `SEventTriggerEventFlag`，与当前 256 bit 旗做 `fn_140006082`（8 个 dword AND）。四段的通过条件不同（全匹配 / 结果非空 / 结果为空 / `fn_140012585`），**不要**合成一个「flag 条件」枚举。
6. **Reference**（bit 25）：`SEventTriggerReference`。类型在引用记录 dword[0] **bits 28–31**（`fn_140004291`），`ja` 上限 `0xE`，即 0–14 共 15 种。分派 `fn_14000eb1a`，跳表 VA `0x1400c679c`。

[确认] 引用类型 0–14 的处理函数（名字来自 RTC / 立即数比较，不是官方枚举）：

| eRef | 函数 | 立即数 / RTC | 语义 |
| --- | --- | --- | --- |
| 0 | `fn_1400031a2` | 与 `0x3E8=1000` 比 | [推断] 金币档 |
| 1 | `fn_140015145` | | [未知] |
| 2 | `fn_140009c46` | 下标 `&lt; 0xE7`（231=`NUM_ITEM`） | [推断] 道具存在 |
| 3 | `fn_1400097e1` | RTC `aePerson` | 人物 |
| 4 | `fn_1400021df` | RTC `sItemOwner` | 持有道具 |
| 5 | `fn_140011b2b` | | [未知] |
| 6 | `fn_14000fe52` | | [未知] |
| 7 | `fn_140008c33` | 人物 `&lt; 0xEB`（235=`NUM_PERSON`），再 `ja 0x17` 的子表 | 人物属性 / 技能一类 |
| 8 | `fn_1400029ff` | `CountFleetItem`；商品 `&lt; 0x69`（105=`NUM_TRADINGITEM`） | 舰队货舱件数 + 比较符 |
| 9 | `fn_1400049c1` | RTC `_eFleet` | 舰队 |
| 10 | `fn_140008f8a` | 下标 `&lt; 0x19`（25=`NUM_FORCE`/`NUM_SEAAREA`） | 势力或海域 |
| 11 | `fn_14001098d` | 先 `&lt; 0x61`（97=`NUM_TOWN`），再分 0/1/2：`&lt; 0x19` / `0xFF`/`0xFE` / `&lt; 0x0D` | 港口上的势力 / 海域 / 第三种 id |
| 12 | `fn_1400137cd` | | [未知] |
| 13 | `fn_14000880a` | | [未知] |
| 14 | `fn_14001393a` | | [未知] |

比较符三件套 `fn_14000ba0a` / `fn_140003959` / `fn_1400080e9`：相等、小于、否则。引用条件和货舱条件都走它们。

`CountFleetItem` / `AddLoad` 本身属于经济；事件侧调用点见 `srcdata/docs/trade_stack.md`：条件 `0x1400c7c00`，动作 `0x1400e8fc0`。

### Execute 表

[确认] EV 头 `+0x4C` 起 5 个 `uint32` 是 **段表**（相对载荷 = `file+0x60`）。`fn_14000ecf5(index)` 读 `header+0x4C+index*4`。

EV0：`0x1A8C, 0x1D44, 0x1D44, 0x20DC, 0x20DC`。

- 段 0（`0x1A8C…0x1D44`）：execute 表，uint16 数组。`fn_140008283` 按 `id * 2` 取一条，构造成 `SEventDataElementExecute`
- 段 1 空；段 3、4 空（起点 = 载荷末尾）
- 段 2（`0x1D44…0x20DC`，EV0 920 字节 = 头 `+0x22` 的 `0xE6=230` × 4）[未知] 精确元素；不是 Sc 指令流

[确认] execute 字（读 word：`fn_140012ed1`；抽位：`fn_1400c3e20` / `fn_1400c3dc0`）：

| 位 | 抽取 | 含义 |
| --- | --- | --- |
| 0–11 | `fn_140004f43` | 12-bit id。type 2 时是 Sc 记录下标 |
| 12–13 | `fn_14000522c` | type：1 = 段 4 脚本表（vanilla **空**）；2 = `CEventScriptBuffer` 加载 Sc |
| 14 | `fn_14000a5d3` | vanilla 全 0 |
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

[确认] 加载方是 `CEventScriptBuffer`（`CEvent+0xa0`）。构造把 `+0x8` 写成 `0xFFF`（空 id）。`fn_1400016db(buffer, id)` 在 id 变化时通过 `fn_1400095a7` 打开 `SC/Sc{n}.dk4` 取第 `id` 条。

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
- imm8 = bits 0–7，再加后续字。`lNextOffset` 按 **字** 前进（`fn_140012ed1` 用 `index*2`）
- `fn_1400e4640`：把 PC 向上对齐到偶数（内嵌变长 blob 之后）

class 跳表（`0x1400e23c0`，7 项）：

| class | 处理 | RTC / 观察 | 子表上限 |
| --- | --- | --- | --- |
| 0 | `fn_1400017f8` `0x1400dffc0` | 分支：用传入的 0/1/2 当谓词 | subclass `ja 5`（6 槽） |
| 1 | `fn_14000fede` `0x1400e0210` | RTC `sMenuData`；`CScriptMenuData` | 0/1/2 |
| 2 | `fn_140007199` `0x1400e0780` | RTC `eType`、`sElementMessage`、`cStillInformation` | `ja 0xB`（12 槽） |
| 3 | `fn_1400074d2` `0x1400e16d0` | 调 `DAT_14429d550`（事件扫描前后也用它放 BGM） | `ja 5`，槽 4 与 5 同一入口 |
| 4 | `fn_14000f3e4` `0x1400e0520` | RTC `sRefData`；走与触发器相同的 `fn_14000eb1a` | 0/1 |
| 5 | `fn_140011a86` `0x1400e20b0` | RTC `sUpdateData`；`SEventUpdateData` | 只认 subclass 0 |
| 6 | `fn_14000847c` `0x1400e1970` | 对世界发命令 / 查玩家状态 | `ja 6`（7 槽） |

[确认] `sScriptResult` 在 `fn_1400020fe`：

| 值 | 行为 |
| --- | --- |
| 0 | 停 |
| 1 | 等 UI（`fn_14001107c` / `fn_140006587`），本 tick 停 |
| 2 | 继续；步进前改写传入谓词 |
| 其它（handler 常写 3） | 本指令结束后若 PC 有增量则继续循环 |

class 6 的 7 个 subclass（跳表 `0x1400e1efc`）：

| sub | 入口 | 立即数 | 副作用 |
| --- | --- | --- | --- |
| 0 | `0x1400e19e7` | | 只把 result 写成 3 |
| 1 | `0x1400e1a06` | 内层 `ja 6` 的 7 个查询 | 把 0/1 写进 result，供 class 0 分支 |
| 2 | `0x1400e1ab7` | 港口 `0x62`（98=`NUM_TOWN+1`）当「当前港」哨兵 | 切场景 / 当前港（`fn_14000c5e0`） |
| 3 | `0x1400e1ba3` | `town_id &lt; 0x61`；`0xFF` 改成当前港；`person_id &lt; 0xEB` | 把人物放到港口（`fn_1400047a5`） |
| 4 | `0x1400e1c86` | `person_id &lt; 0xEB`，排除哨兵 `0x46` | 入队 / 更新人物会话 |
| 5 | `0x1400e1d80` | 0 → kind 4；1 → kind 3；其它 → kind 0 | 改扫描组 |
| 6 | `0x1400e1df3` | 0–5 档；人物常量 `0x3E=62` | 与 sub 4 类似的人物会话，对象写死 62 |

[确认] class 6 不直接 `call AddLoad`。增减货物在 `fn_14000e7eb`（`0x1400e8fc0`）：商品 `&lt; 0x69`，`AddLoad` 的调用点 `0x1400e9079`。它是引用/更新路径上的动作，不是 class 6 的 7 槽之一。

[确认] 置旗：`fn_140004e4e`（`0x1400e7b30`）对 `CEventData+8` 调 `fn_14000ac81`。脚本动作侧与 `SEventFlags` 同一套 256 bit。

### 指令直方图（只发布「脚本第一条指令」）

完整长度表未闭合：class 2 会把 GBK 对白内嵌在指令后面，按固定 2 字去切会把汉字当成 opcode（class 7 伪指令）。**禁止**把「整段 uint16 直方图」当成 ISA。

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
| 2.1 | `0x41` | 30 | display 1（PC+22，10 槽静画/立绘） |
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
| `trigger_kind` | 0–8，`NUM_TRIGGERKIND=9` | 运行时 `CEvent+0x88=0xA` 当空闲 | 扫描组，不是条件枚举 |
| `trigger_id` | 每包 250–346 | 无 | 重构 `(pack, index)`；execute 下标 = `index+1` |
| `execute_id` | 12-bit，vanilla 0…N-1 | 0 = 不跑；`0x3FFF` = 切 kind 3 | 与 Sc 记录 0-based 对齐 |
| `opcode_id` | `(class&lt;&lt;5) \| subclass`，即指令高字节 | 非法 class &gt; 6 进默认支 | 稳定 id；未命名用 `unknown_0xNN` |
| `flag_id` | 0–255 | 未置位 = 0 | 见 `02-event-system.md` |
| `ref_kind` | 0–14 | 无 `INVALID` 符号 | 引用条件 / class 4 共用 |
| `town_id` / `force_id` / `person_id` / `item_id` / `trading_item_id` | 各系统 | 各 `invalid`；脚本里另有 `0xFF`/`0xFE`/`0x62`/`0x46` | 只持 ID |
| `script_blob` | Sc 记录 payload | 空 payload 禁止当「已译码」 | 未识别的尾部进 `opaque` |

`SMemorySeeker` 是游标，不是玩法实体。`CEventScriptBuffer+0x8=0xFFF` 是「当前未加载」，不要当成 `max_script_bytes`（这一点 `02` 已标 [未知]）。

哨兵（内容包实例，进 `ids` / caps，不写进解释器）：

| 哨兵 | 值 | 出现处 |
| --- | --- | --- |
| 任意 / 跳过 | `0xFF`、`0xFE` | `ctx`、引用、港口 |
| 当前港 | `0x62`（98） | class 6.2 / 6.3 |
| 内嵌 blob | `0xFFFF` | class 2 / class 4 |
| 无 execute | `0` | execute[0]、false 脚本 |
| 解释器出口 | `0x3FFF` | 14-bit 全 1 |
| 人物排除 | `0x46`（70） | class 6.4 |
| 金币档 | `0x3E8`（1000） | 引用类型 0 |
| 月通配 | `0xC` | Time 块 |
| 日通配 | `0x1F` | Time 块 |

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
        if trigger.dword0.bit31: continue          # 运行时 skip
        if not match_kind_ctx(kind, trigger, ctx): continue
        if not match_optional_blocks(trigger, ctx): continue
        if trigger.dword0.bit15: force_miss        # 见第 9 节
        id = trigger.script_id_true if hit else trigger.script_id_false
        if id == 0: continue
        run_execute(id)
```

`match_kind_ctx` 只比较扫描组字段（上表）。日期、旗、引用、多港口、战斗结果全部在 optional blocks 里，**所有 kind 共享**。

### 执行

```
run_execute(id):
    if id == 0: return
    if id == exit_id:          # 原版 0x3FFF
        set_scan_kind(3, arg)
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
        result, delta = dispatch(class, sub, imm, words, pc, pred)
        if result == 0: return
        if result == 1: yield_ui(); return
        pc += delta
        if result == 2: pred = rewrite_pred(...)
```

`delta` 以字计。对白 / 引用内嵌 blob：先按 handler 读长度，再 `align_even(pc)`。未识别指令不得猜 `delta`；提取器把从这里到记录末尾写入 `opaque`。

## 5. 耦合分析

原版焊点：

1. **文件名 = 剧本号** `EV{n}` / `Sc{n}`。重构后由 `scenarios.yaml` 指出 `event_pack` / `scene_pack`。
2. **扫描组缓存在 `CEvent+0x88…+0x9c`**。这是「世界现在在哪」的缓存。重构改为求值时 LIVE 查询：`Town.current()`、`Calendar.snapshot()`、`Combat.last_result()`、`Flags.test()`。禁止事件对象持有 `World*` / `Player*`。
3. **class 6.5 与 `0x3FFF` 会改扫描组**。这是叙事内部状态，不要泄漏成 `if scenario==3`。
4. **引用类型 8 直接 `CountFleetItem`，动作路径直接 `AddLoad`**。允许，但必须走 `Economy.count_fleet_item(fleet_id, trading_item_id)` / `Economy.add_load(...)`。脚本只存 ID。
5. **class 2 / Sc 内嵌 GBK**。文案属于内容包，逻辑只持 `string_key`。提取器把 GBK 抽成 i18n，指令里留 key。
6. **BGM（class 3）写音频管理器**。叙事发 `Audio.play(clip_id)`，不持设备指针。
7. **其它系统若写 `if opcode==0xC3`** 即违规。opcode 的含义属于叙事 ISA；贸易只认收到的 `AddLoad` 命令。

`CEvent` 是 `CSaveObject`：旗、扫描组、已触发位进存档。EV / Sc 原文不进存档。

## 6. 解耦后的配置

跳表有 37 个 class.subclass 槽。能命名的命名；不能命名的写成 `unknown_0xNN` 并挂证据。**禁止**补一套「完整剧情汇编语言」。

```yaml
# config/narrative/trigger_kinds.yaml
schema: dk4.narrative.trigger_kinds
version: 1
# eTriggerKind = 扫描组。NUM_TRIGGERKIND=9 是原版包槽数，运行时读表长。
kinds:
  - id: 0
    key: scan_0
    name: null
    setter: fn_140007b58
    ctx_field: event.ctx0          # 原版 CEvent+0x8c
    notes: "EV0 ctx 含 150+，不是 town_id 空间"
  - id: 1
    key: scan_1
    name: null
    setter: fn_140006f50
    shares_filter_with: [2, 3]
    subtype_fields: [event.ctx1, event.ctx2, event.ctx3]  # +0x94/98/9c
  - id: 2
    key: close_battle
    name: 战斗结束
    setter: fn_14000b3c0
    rtti_unconditional: SEventTriggerCloseBattle
    result_field: event.battle_slot   # +0x90，0–3
  - id: 3
    key: scan_3
    name: null
    setter: fn_140010578
    notes: "execute id 0x3FFF 与 class 6.5 会切到本组"
  - id: 4
    key: town_scan_4
    name: null
    setter: fn_14000210d
    town_lookup: true
  - id: 5
    key: town_scan_5
    name: null
    setter: fn_140014ab0
    town_lookup: true
  - id: 6
    key: town_scan_6
    name: null
    setter: null                   # 未见 imm8 写入
    town_lookup: true
  - id: 7
    key: town_scan_7
    name: null
    setter: null
    town_lookup: true
    extra_field: event.ctx_extra   # bits 18–21 vs +0x90
    notes: "条数最多；ctx 为 town_id 或 0xFF。不要写成确认的「入港」符号"
  - id: 8
    key: scan_8
    name: null
    setter: null
    ctx_field: event.ctx0
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
current_town_sentinel: current     # 原版 0x62
inline_blob_sentinel: 0xFFFF

# id = (class << 5) | subclass = 指令高字节
opcodes:
  # class 0  flow
  - { id: 0x00, class: 0, subclass: 0, key: flow_0, operands: [word], side_effect_system: narrative, evidence: "fn_1400017f8 jt[0]; PC+2" }
  - { id: 0x01, class: 0, subclass: 1, key: unknown_0x01, operands: [word], side_effect_system: narrative, evidence: "jt[1] fallthrough" }
  - { id: 0x02, class: 0, subclass: 2, key: branch_pred_eq_0, operands: [word], side_effect_system: narrative, evidence: "cmp pred,0" }
  - { id: 0x03, class: 0, subclass: 3, key: branch_pred_eq_1, operands: [word], side_effect_system: narrative, evidence: "cmp pred,1" }
  - { id: 0x04, class: 0, subclass: 4, key: branch_pred_eq_2, operands: [word], side_effect_system: narrative, evidence: "cmp pred,2" }
  - { id: 0x05, class: 0, subclass: 5, key: unknown_0x05, operands: [word], side_effect_system: narrative, evidence: "jt[5] 0x1400e0132" }

  # class 1  menu
  - { id: 0x20, class: 1, subclass: 0, key: menu_open, operands: [], side_effect_system: ui, evidence: "sMenuData; result=3 wait" }
  - { id: 0x21, class: 1, subclass: 1, key: unknown_0x21, operands: [word], side_effect_system: ui, evidence: "PC+2 then extra" }
  - { id: 0x22, class: 1, subclass: 2, key: unknown_0x22, operands: [word, word], side_effect_system: ui, evidence: "PC+3" }

  # class 2  display / talk / still
  - { id: 0x40, class: 2, subclass: 0, key: say, operands: [msg_id_or_inline], side_effect_system: ui, evidence: "sElementMessage; 0xFFFF 内嵌 GBK; PC+4; 首指令 572 次" }
  - { id: 0x41, class: 2, subclass: 1, key: play_still_layout, operands: [slot_x10], side_effect_system: ui, evidence: "PC+22; 循环 0–9 槽; cStillInformation" }
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
  - { id: 0x60, class: 3, subclass: 0, key: audio_0, operands: [], side_effect_system: audio, evidence: "DAT_14429d550; 首指令 491 次" }
  - { id: 0x61, class: 3, subclass: 1, key: audio_1, operands: [clip_or_ff], side_effect_system: audio, evidence: "0xFF=当前; id+2 后播放; 首指令 353 次" }
  - { id: 0x62, class: 3, subclass: 2, key: unknown_0x62, operands: opaque, side_effect_system: audio, evidence: "jt 0x1400e17c4" }
  - { id: 0x63, class: 3, subclass: 3, key: unknown_0x63, operands: opaque, side_effect_system: audio, evidence: "jt 0x1400e1825; 首指令 55 次" }
  - { id: 0x64, class: 3, subclass: 4, key: unknown_0x64, operands: opaque, side_effect_system: audio, evidence: "与 subclass 5 同一入口 0x1400e1898" }
  - { id: 0x65, class: 3, subclass: 5, key: unknown_0x65, operands: opaque, side_effect_system: audio, evidence: "同上" }

  # class 4  query reference
  - { id: 0x80, class: 4, subclass: 0, key: query_ref, operands: [ref_id_or_inline], side_effect_system: narrative, evidence: "sRefData; 0xFFFF 内嵌; 走 fn_14000eb1a; 首指令 202 次" }
  - { id: 0x81, class: 4, subclass: 1, key: query_flag, operands: [flag_pack], side_effect_system: narrative, evidence: "fn_14000a0d8; 首指令 13 次" }

  # class 5  mutate reference
  - { id: 0xA0, class: 5, subclass: 0, key: update_ref, operands: [ref_id_or_inline], side_effect_system: narrative, evidence: "SEventUpdateData; result=3; 首指令 168 次。具体写入（旗/货/人）看内嵌 ref_kind" }

  # class 6  world commands
  - { id: 0xC0, class: 6, subclass: 0, key: yield, operands: [], side_effect_system: narrative, evidence: "result=3" }
  - { id: 0xC1, class: 6, subclass: 1, key: query_player, operands: [which_0_6], side_effect_system: narrative, evidence: "内层 7 查询 → result 0/1" }
  - { id: 0xC2, class: 6, subclass: 2, key: set_current_town, operands: [town_id_or_current], side_effect_system: ocean, evidence: "0x62 哨兵; fn_14000c5e0" }
  - { id: 0xC3, class: 6, subclass: 3, key: move_person_to_town, operands: [town_id, person_id], side_effect_system: characters, evidence: "town<0x61 person<0xEB; fn_1400047a5" }
  - { id: 0xC4, class: 6, subclass: 4, key: join_person, operands: [person_id], side_effect_system: characters, evidence: "排除 0x46; 首指令 1 次" }
  - { id: 0xC5, class: 6, subclass: 5, key: set_scan_kind, operands: [0|1|other], side_effect_system: narrative, evidence: "0→kind4 1→kind3 else→kind0" }
  - { id: 0xC6, class: 6, subclass: 6, key: unknown_0xC6, operands: [slot_0_5], side_effect_system: characters, evidence: "写死 person 0x3E" }

# 不在 class 6 跳表、但脚本动作路径会发的命令
side_commands:
  - { key: add_load, side_effect_system: economy, evidence: "fn_14000e7eb → AddLoad; 商品 < 0x69" }
  - { key: set_flag, side_effect_system: narrative, evidence: "fn_140004e4e → fn_14000ac81" }
  - { key: count_fleet_item, side_effect_system: economy, evidence: "条件 fn_1400029ff → CountFleetItem; 不是 opcode class" }
```

触发器译码完成后的 `when` / `then` 只许出现 ID 与纯值（与 `02` 第 6 节升级路径一致）：

```yaml
when:
  kind: close_battle            # 扫描组
  battle_slot: 0
  time_mode: 1
  calendar: { month: any, day: any }   # 原版 0xC / 0x1F
  flags_all: [flag.intro]
  ref:
    kind: cargo                 # 原版 ref 8
    trading_item_id: 3
    cmp: ge
    count: 1
then:
  commands:
    - { op: say, string_key: ui.event.ev0.000 }
    - { op: menu, choices: [ui.event.yes, ui.event.no] }
    - { op: set_flag, flag_id: 12 }
    - { op: add_load, fleet_id: 0, trading_item_id: 3, delta: -1 }
    - { op: move_person_to_town, person_id: 7, town_id: 11 }
    - { op: set_scan_kind, kind: 4 }
```

未译码尾部进 `opaque`。禁止用「参考大航海4」的散文代替 `unknown_0xNN`。

## 7. 独立旋钮

改这些不应迫使经济 / 战斗 / 航海改代码：

- 某触发的扫描组、可选条件块、是否一次性
- 某 opcode 的 operand（仍是 ID / 纯值）
- 把一条「日期 + 入港」改成「只看旗」
- 新增扫描组（引擎按表长；原版解释器写死 9）
- 新增 opcode（登记进 `opcodes.yaml`；原版 class 0–6 是默认包）
- Sc 对白换成另一套 `string_key`
- `current_town_sentinel` / `exit_execute_id` / 通配符

需要其它系统配合的只有：命令里出现的 `town_id` / `item_id` / `person_id` / `trading_item_id` / `force_id` 必须在对应表里存在（加载期外键校验）。`AddLoad` 的堆叠规则属于经济，不在本表。

## 8. 对外接口

本系统订阅（扫描组由订阅方的事件选择，不在贸易代码里写 kind 号）：

| 事件 | 来源 | 扫描组（原版观察） |
| --- | --- | --- |
| `ScenarioSelected` | 剧本 | 加载 EV + Sc + EVA |
| `ArrivedAtTown {fleet_id, town_id}` | Ocean / Town | kind 4–7（[推断] 主港口扫描 = 7） |
| `BattleResolved {result}` | Combat | kind 2 `close_battle` |
| `DayAdvanced {calendar}` | 世界时间 | Time 块；不独占某个 kind |
| `FlagChanged` | 自己 | 再扫带旗块的触发 |
| `CargoCommitted` | Economy | 带 ref 8 的触发 |

查询（LIVE，取不到则本条 no-op）：

```
Calendar.snapshot() -> {year, month, day, era_tag}
Town.current_for(fleet_id) -> town_id | none
Combat.last_result(fleet_id) -> slot_0_3 | none
Flags.test(flag_id) -> bool
Economy.count_fleet_item(fleet_id, trading_item_id) -> int
Characters.has_item(person_id, item_id) -> bool
Characters.in_roster(person_id) -> bool
Factions.force_of_town(town_id) -> force_id | none
World.town_exists(town_id) -> bool
```

命令（脚本可发；接收方是各系统）：

```
Flags.set(flag_id, bool)
Economy.add_load(fleet_id, trading_item_id, delta)
Items.give(person_id, item_id)            # 引用更新路径，opcode 级未钉
Characters.move_to_town(person_id, town_id)
Characters.join(person_id)
Town.set_current(town_id)
Narrative.set_scan_kind(kind)
UI.play_still(still_id)
UI.say(string_key)
UI.menu(choices) -> choice_index
Audio.play(clip_id)
Audio.stop()
```

禁止脚本拿到 `CTown*` / `CPerson*`。存档只存：`event_pack` id + schema 版本、256 旗、扫描组、已触发集合（若有）、解释器 PC（若中途存盘）。不存 EV / Sc 原文。

「set ship」在跳表和 `AddLoad` 路径里 **没有** 独立 opcode。需要改船时走船只系统自己的命令，等引用更新（class 5）译码后再挂，不在本表编一条。

## 9. 未核实字段

| 项 | 现状 | 禁止 |
| --- | --- | --- |
| kind 0/1/3/4/5/6/8 的符号名 | 有跳表、setter、`ctx` 分布 | 不要写成「0=入港 1=日期」 |
| kind 7 = 入港 | [推断] 港口扫描组 | 不要升成 [确认] |
| kind 6、8 的 setter | 无 `mov [+0x88], 6/8` | 不要说这两种 kind 是死槽（EV 包里有记录） |
| dword[1] bits 30–31 | 文件里很常见，求值开关不读 | 不要当「已消耗」位（那是 dword[0] bit 15/31） |
| 触发是否一次性 | 扫描里 bit 15 与 `fn_1400128f0` | YAML `once` 可配，默认值未证明 |
| Time 四字段的年月日旬 | 有通配符 `0xC`/`0x1F` | 日历文档负责；这里只比 snapshot |
| 引用类型 1,5,6,12,13,14 | 只有跳表槽 | `unknown_ref_N` |
| 引用类型 0 的「1000」 | 与 `0x3E8` 比 | 不要写成确认的「1000 金币」 |
| 引用类型 10 的 25 | `NUM_FORCE` 与 `NUM_SEAAREA` 同数 | 不要独断是势力还是海域 |
| class 2 多数 subclass | 跳表在，长度表未闭合 | 整段对白当 `opaque`，不要猜「立绘槽 3」 |
| class 3 各 subclass 的 clip 语义 | 调同一音频对象 | 不要写成 BGM / SE / 语音 三套确认名 |
| class 6.1 的 7 个查询 | 只有调用点 | `query_player_0…6` |
| class 6.6 的人物 62 | 立即数 `0x3E` | 不要猜成某具名 NPC 再写进确认列 |
| EV 段 2（`+0x22` 个 uint32） | 长度对得上 | `unknown_segments[2]` |
| `CEventScriptBuffer+0x8=0xFFF` | 空 id | 不要当 max_script_bytes |
| 跳过 DS 8 字节的具体指令 | 执行起点不在魔数上 | 不要写成确认的 `add ptr, 8` 行号 |
| Sc 记录里指令以外的版面字节 | 与「场景图」说法并存 | 不要把 Sc 整包当 G1T（魔数是 `DS` 不是 `GT1G`） |
| set ship / 改炮 / 改帆 | 本 ISA 未见独立 class | 不要编 opcode |
| 整段 opcode 直方图 | 对白污染 | 只发布「首指令」表，直到长度表闭合 |

提取器：EV 头按 `02` 进 YAML；每条触发的位域进 `when`；Sc payload 用跳表能切的指令进 `commands`，其余进 `opaque`。升 schema version 的条件是：某条 `unknown_0xNN` 有了长度 **和** 副作用系统，而不是「看起来像对白」。
