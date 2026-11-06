# 事件系统

配置落点：`config/narrative/events.yaml`、`config/narrative/triggers.yaml`。剧本选择见 `01-scenarios.md`。本系统是脚本解释器，不是把剧情编译进 C++。

## 1. 职责边界

本系统管：

- 加载当前 `event_pack`（原版 `EV0`–`EV6` + 共用 `EVA`）
- 按触发器扫描「现在该不该开火」
- 执行脚本：对白、选项、置位、以及对其它系统的**命令**（给道具、搬舰队、改货、改旗）
- 事件静画 / 动画 / 背景的播放请求（真正贴图在 UI / 资源层）
- 事件旗（存档状态）

不管：

- 选哪个剧本、哪份世界包（剧本表）
- 市价、伤害、经验公式
- mesfile 正文（只持有 `string_key` / 原版消息下标）
- 把某个 `scenario_id` 写进贸易代码

叙事可以**查询**全部系统、**发出**命令；其它系统不得依赖具体剧本号或具体事件 id 的硬编码分支。

## 2. 原版事实

### 事件是脚本，不是编译出来的 C++

[确认] 剧情在 `EV*.dk4` 里。引擎侧是解释器：

| 类 | 角色 |
| --- | --- |
| `CEvent` | 会话对象，继承 `CSaveObject`（状态要存盘）兼 `CEventSeeker` |
| `CEventData` | 事件数据宿主 |
| `CEventSeeker` | 按触发器扫描 |
| `CEventScriptBuffer` | 脚本缓冲 |
| `SEventScript` | 一条脚本，基类 `SMemorySeeker` |
| `SEventDataScriptTable` | 脚本表 |
| `SEventDataElementExecute` | 可执行元素 |
| `SEventDataElementMessage` / `MessageTable` | 对白元素 |
| `CScriptMenuData` | 脚本里的选项菜单 |
| `SEventFlags` | 事件旗 |

来源：`srcdata/docs/formats.md`、`srcdata/catalog/class_hierarchy.txt`、`srcdata/source/game/event/`、`srcdata/include/rtti_classes.h`。

解释器主体：

- `fn_1400050fb`（RTC：`sScriptTable`, `sExecute`）：取 execute 字。type 1 走段 4 脚本表，type 2 走 `CEventScriptBuffer` 加载的 Sc 记录。`id==0x3fff` 不跑脚本，置 `CDk4Game`（`DAT_144296500`）流程态 3、参 1 = 寿终 Game Over（先放寿终四句消息 `0x170`–`0x173`，见 `03-event-commands.md`「流程态」、`11-progression/03-gameover-succession.md`）。
- `fn_1400020fe`（RTC：`sScriptResult`, `lNextOffset`, `sScript`）：结果 0 停；1 改写本触发下一段 execute id 后停；2 改谓词后继续；其它继续。`lNextOffset` 是**字节**偏移（初值 2）。

来源：`srcdata/source/game/unassigned/fn_1400c2290.cpp`。

[确认] 脚本会跨系统读 / 改状态。仓库里已挂上的入口：

- `CountFleetItem`：舰队某商品件数（注释写「委托/剧情」；原版符号名，重构查询 `Ships.count_item`）
- 原版符号 `AddLoad`：舰队增减货物（`n<0` 扣；重构命令 `Ships.add_load`）

来源：`srcdata/INDEX.md`、`srcdata/docs/trade_stack.md`。`dhh4tools/README.md` 也按「事件检查和扣除货物」描述同一对函数。

[确认] 另有一段脚本辅助（`fn_14001098d`，body `0x1400c8390`，引用类型 11）把操作数与 `0x61`（`NUM_TOWN=97`）和 `0x19` 比较后再去取对象。这是「脚本持有 `town_id` / 势力 id」的证据。

[确认] opcode 已按 handler 级译出：7 个 class、37 个 class.subclass 槽，class 0（流程）、3（音频）、6（世界命令）逐槽有语义；class 2 多数槽与引用 / 更新记录的单条布局仍未闭合。详表见 `03-event-commands.md`。解耦配置的规范形式是「数据触发器 + 结构化命令树」，字节码只作导入格式，见 `03` 第 6.1 节。

### EV 文件格式（已还原的头）

[确认] `Game/SC/EV0.dk4`…`EV6.dk4` 头 96 字节（`0x60`）。加载时 `CEvent::vfunc_05` → `fn_140007dec` 读全文件，校验 `*(u16*)file == 0x4557`（`'W''E'`）。`fn_14000af9c` 还会按 7 个剧本各读 0x60 头缓存到 `DAT_140682858`。

布局：

| 偏移 | 类型 | 含义 |
| --- | --- | --- |
| +0x00 | uint16 | 魔数 `0x4557` `'WE'` |
| +0x02 | uint16 | 版本。SC 七个文件分别为 `0x0106, 0x0104, 0x0106, 0x0103, 0x0104, 0x0105, 0x0104`；存档锁用（见下） |
| +0x04 | uint32 | 载荷字节数。`文件大小 = 0x60 + 该值` |
| +0x08 | uint16 | 触发条目总数 |
| +0x0A | uint16[9] | 每种 `eTriggerKind` 的条数。`fn_140010465` 读 `word[kind]` |
| +0x1C | uint16 | 七包均为 0 |
| +0x1E | uint16 | 七包均等于 +0x08 |
| +0x20 | uint16 | 七包均为 0 |
| +0x22 | uint16 | 段 2（引用表）的 uint32 个数。EV0 = `0xE6`=230 = `(0x20DC−0x1D44)/4`。旧稿把 +0x20 读成 uint32 `0xE6`，错：按 uint32 读是 `0x00E60000` |
| +0x24 | uint32 | 0 |
| +0x28 | uint32[9] | 每种触发器在**载荷**里的起始偏移（相对 `file+0x60`）。`fn_140011c57` 读 `dword[kind]` |
| +0x4C | uint32[5] | 段表：0 execute 表、1 消息表、2 引用表、3 更新表、4 脚本表（`fn_140001ef1` 五个调用点钉死，见 `03`）。EV0：`0x1A8C, 0x1D44, 0x1D44, 0x20DC, 0x20DC`，段 1/3/4 为空 |

[确认] 九个计数之和等于 +0x08。EV0：`(5,16,16,4,2,85,10,203,5)` 和 = 346。

[确认] **EV 版本锁存档**。槽文件头 `+0x24` 存当时剧本 EV 的 `+0x02` 版本；严格读档要求它等于 `<lang>/EV<+0x20>.dk4` 的版本，不等则拒绝（`fn_140121700` → `fn_1400c26a0` 查表 `0x140682858`，表由 `fn_14000af9c` 读七个 EV 头填入）。见 `12-ui-save/02-save-session.md` 槽文件头。四语言 EV 版本（`Game/<lang>/EV*.dk4` 偏移 2，本轮实读）：

| 语言 | EV0 | EV1 | EV2 | EV3 | EV4 | EV5 | EV6 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| SC | 0x106 | 0x104 | 0x106 | 0x103 | 0x104 | 0x105 | 0x104 |
| TC / JP / KR | 0x101 | 0x101 | 0x101 | 0x102 | 0x104 | 0x105 | 0x104 |

所以剧本 0–3 的简中存档放到繁中 / 日 / 韩目录也会被拒；4–6 版本相同。版本号随脚本修订变，与存档里的触发运行时位是否兼容无关——原版用「版本相等」硬锁代替迁移。

[确认] 单条触发记录变长：用 `SMemorySeeker` 按流解析，不能用固定 stride 切。`fn_1400095c5(event, out, kind)` 把指针放到 `buffer + offset[kind] + 0x60`。

### 触发器种类（9 槽）

[确认] `NUM_TRIGGERKIND = 9`。求值函数 `fn_140001b09`（body `0x1400bf5e0`）对 `CEvent+0x88` 做 `switch (kind)`，`ja` 上限 8，即 kind 0–8。跳表 VA `0x1400c05bc`。

[确认] 开火入口 `fn_14001337c`（body `0x1400c0d30`）：把调用方的 24 字节请求 `sEventTrigger` 拷进 `CEvent+0x88…+0x9F`（kind、`ctx0` 与四个上下文 dword），再调扫描 `fn_140013c32`：取当前日历 `SCalendar`，按 kind 取该类全部触发器，逐条 `fn_140001b09(event, trigger, calendar)`；按命中 / 锁存位选 true / false 的 execute id，非 0 则 `fn_1400050fb` 执行。`CEvent` 全局实例在 `0x144295490`。

跳表分组（反汇编）：

| kind | 入口 | 观察 |
| --- | --- | --- |
| 0 | `0x1400bf66a` | 从触发流抽位域（bit 8–15）与 `CEvent+0x8c` 比较。单独路径 |
| 1, 2, 3 | `0x1400bf693` | 共享。抽 2 bit（0x10–0x11）再分 0/1/2 支，分别比 `CEvent+0x94 / +0x98` 等 |
| 4, 5, 6, 7 | `0x1400bf744` | 共享。kind==7 额外比一次；子支 0 把 `uint8` 与 `0xFF` 哨兵比较 |
| 8 | `0x1400bf884` | 单独。`fn_140004af7` 与 `CEvent+0x8c` 比较。随后与其它 kind 汇合 |

汇合点之后**所有 kind** 都会构造 `SEventTriggerEventOutcome`（ctor `0x14000e755`）——这是「命中之后的结果块」，不要和 kind 8 的条件部分混成同一个枚举名。

[确认] 七个带 RTTI 的触发子类（均 `-> SMemorySeeker`）：

| 类 | 含义（按类名） |
| --- | --- |
| `SEventTriggerTime` | 时间 |
| `SEventTriggerCloseBattle` | 战斗结束 |
| `SEventTriggerEventFlag` | 事件旗 |
| `SEventTriggerMultipleTown` | 多港口 |
| `SEventTriggerReference` | 引用（道具 / 人物 / 其它对象，见第 9 节） |
| `SEventTriggerReferenceMultiple` | 多引用 |
| `SEventTriggerEventOutcome` | 事件结果（也出现在每条触发的「then」侧） |

[确认] 求值函数 RTC 局部名（同一函数栈上）还出现：`sTriggerDate`、`sTriggerTime`、`sTriggerMultipleTown`、`sTriggerCloseBattle`、`sTriggerEventFlag` / `sResultEventFlag`、`sTriggerReference`、`sTriggerEventOutcome`、`idTown`（1 字节，港口 0–96 放得下）。

[确认] 七个 RTTI 类是**可选条件块**，不是 kind。kind 的语义由 `fn_14001337c` 的 8 个调用点（请求构造器 + 场景切换钩子）钉死：

| kind | 含义 | `ctx0` |
| --- | --- | --- |
| 0 | 海上探索发现道具 | `item_id` |
| 1 | 海战开始 | 对手 `person_id` |
| 2 | 海战结束（无条件挂 `CloseBattle` 块） | 对手 `person_id` |
| 3 | 与移动舰队接触 | 对手 `person_id` |
| 4 | 入港（海图 → 港口） | `town_id` |
| 5 | 港内街区（事件源 `TownTick`） | `town_id` |
| 6 | 航海中（海图 tick；事件源 `SeaTick`） | 舰队所在港字节 |
| 7 | 进入港内设施 | `town_id` + 设施号 |
| 8 | 登陆村落 | `village_id` |

证据与调用点见 `03-event-commands.md` 第 2 节。日期、旗、持有物不是 kind，而是所有 kind 共用的条件块。没有 `TriggerKindID.h`，枚举符号名仍无。

各包每 kind 条数：

| 包 | k0 | k1 | k2 | k3 | k4 | k5 | k6 | k7 | k8 | 合计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| EV0 | 5 | 16 | 16 | 4 | 2 | 85 | 10 | 203 | 5 | 346 |
| EV1 | 3 | 9 | 18 | 8 | 5 | 85 | 7 | 187 | 6 | 328 |
| EV2 | 2 | 8 | 11 | 2 | 0 | 98 | 6 | 206 | 6 | 339 |
| EV3 | 3 | 9 | 17 | 3 | 2 | 68 | 4 | 193 | 7 | 306 |
| EV4 | 2 | 10 | 23 | 3 | 4 | 62 | 6 | 139 | 1 | 250 |
| EV5 | 0 | 5 | 21 | 6 | 4 | 106 | 4 | 152 | 1 | 299 |
| EV6 | 0 | 5 | 17 | 4 | 3 | 80 | 4 | 137 | 3 | 253 |

k7（进入设施）条数最多（约一半）；k0（探索发现道具）在 EV5/EV6 为 0。

### 事件旗（存档）

[确认] `SEventFlags` 体积 `0x28`：vptr + 8 个 `uint32` = **256 bit**。

- 清零：`fn_140004f89` 把 8 个 dword 写 0
- 置/清：`fn_14000ac81`  
  `word = flag_id >> 5`，`mask = 1 << (flag_id & 31)`，OR 或 AND-NOT
- 测试：`fn_14000b0c3` 同样的位运算
- 从 4 字节打包旗构造：逐字节，`0xFF` 跳过，否则置位

`CEvent` 继承 `CSaveObject`，`vfunc_06` / `vfunc_07` 调用 `fn_140013f66` / `fn_140003e4a` 读写触发器侧数据（同样按 8 dword + 4 个 uint16 + 9 kind 流）。来源：`srcdata/source/game/unassigned/SEventFlags.cpp`、`fn_1400c2290.cpp`、`CEvent.cpp`。

[未知] 256 旗在 `savedatXX.dk4` 的文件偏移。`save_format.md` 尚未标这一段。

### 静画与动画

[确认] 表现类（`srcdata/source/game/event/`）：

| 类 | 体积（析构） | 角色 |
| --- | --- | --- |
| `CEventStill` | `0x30` | 一张静画。构造里出现常数 `5` 与 `0xFFFFFFFF` |
| `CEventStillManager` | `0x18` | 静画列表，`CEvent+0x48` |
| `CEventBackView` | `0x80` | 事件背景 View |
| `CEventBackEffect` | `0x40` | 背景效果 |
| `CEventAnimation` | `0xA8` | 动画。`vfunc_01` / `vfunc_02` 自带 Win32 消息泵 |
| `CEventAnimOP` | 继承 Animation | 开场类动画 |
| `CTextAnimeSprite` | | 逐字文本动画 |

[确认] 背景图：

- `EventBG1.dk4`…`EventBG8.dk4`：常规事件背景（G1T）
- `EventBGEX.dk4`：额外背景。`fn_140004106` 把一批静画 id（`0x14D, 0x17B–0x184`）映射到该文件的子下标 0,3,6,9–15
- `EvFrame.dk4`、`EvItem.dk4`：事件窗框 / 道具图
- `OldMapStill.dk4`：古地图静画

[确认] `SC/EVA.DK4` 由 `fn_140008fb7` 加载（字符串 `'SC/EVA.DK4'`）。文件本身**不是** `WE` 头：开头 `uint32 count=0x11`（17），随后 size/offset 对，总长 33360。加载函数里比较 `0x01004145`（`'EA'` + 版本？）是解析后的结构，不是文件头。来源：`fn_1400c8390.cpp`、文件转储。

[确认] `Sc0.dk4`…`Sc6.dk4` 与 EV 一对一。`ScN` 第一个 `uint32` 等于 `EVN` 触发总数（Sc0=0x15A=346）。每条记录魔数 `'DS'`、版本字与 EV 相同，载荷是**脚本指令流 + 内嵌 GBK 对白**，不是场景图（`formats.md` 的「场景图」说法应改）。格式见 `03-event-commands.md`。

### 其它相关计数

[确认] `NUM_HAPPENING = 5`，RTC 名 `eHappening`。求值样本 `fn_1400f2d90` 里同一局部还和 `0x14`、`0x1E` 比较，**不能**当成 EV 的第五种触发。更像海上随机遭遇 / 另一套「发生」枚举。未并入 `eTriggerKind`。

[确认] `NUM_KIND = 9` 与 `NUM_TRIGGERKIND = 9` 同数；事件侧循环写 `eKind >= 9` 与 `eTriggerKind >= 9` 各有一份。是否同一枚举未在头文件钉死，配置里只暴露 `trigger_kind` 0–8。

[确认] `CWaitTriggerView`：UI 按钮，等玩家点「继续」一类触发，不是 `eTriggerKind` 的一种。

## 3. 实体与 ID

| 实体 | 原版 | 空值 | 备注 |
| --- | --- | --- | --- |
| `event_pack` | EV0–EV6 | 无 | 由剧本表选择；包 id 用 `vanilla-ev0` 形式，带 `version`（第 6 节） |
| `trigger_kind` | 9 | 无 `INVALID` 符号；请求默认 `0xA` | 运行时按表长；原版 0–8，语义见第 2 节 |
| `trigger_id` | 每包 250–346 条 | 无 | 稳定 key（`vanilla:ev0.t012`）；原版 `(pack, index)` 只在导入层。存档 `trigger_state` 按 key 存，不按下标 |
| `flag_id` | 256 bit | 未置位 = 0 | `word = id>>5` |
| `script_blob` | Sc 记录载荷 | 空 blob 禁止开火 | 导入格式；规范形式是命令树（`03`） |
| `trigger_state` | 每条触发 4 位 + 两个 14 bit execute id | 文件里全 0 | 运行时状态，进存档；定义只读 |
| `still_id` | EventBG 常规 + EX 特例 id | `0xFFFFFFFF` 在 Still 构造里出现 | |
| `town_id` / `item_id` / `person_id` | 外键 | 运行时 `null`；原版哨兵（`0xFF` 任意港、`0x62` 无港、`0x46` 无人、`0xE8` 空道具）只在 `ids/legacy_codec.yaml` | 触发条件与脚本命令都只持命名空间 key（DECISIONS D1） |

`SMemorySeeker` 是流式游标（`ptr`），不是玩法实体。重构不要把 seeker 指针存进存档。

## 4. 算法 / 公式

### 开火扫描

原版每个 tick / 场景切换：对当前 kind（`CEvent+0x88`）的全部触发走：

```
for trigger in pack.triggers_of(kind):
    if state[trigger].disabled: continue
    hit  = match_condition(trigger, context)
    fire = hit and not state[trigger].latched
    if not hit: state[trigger].latched = 0
    run_execute(state[trigger].next_id(fire))    # 0 = 不跑
    if fire: apply on_fire (one_shot → disabled, relatch → latched)
```

锁存 / 一次性 / 可改写 execute id 的位定义与证据见 `03-event-commands.md` 第 2、4 节。

`context` 至少包括：当前日期、当前港 `town_id`、战斗是否刚结束、事件旗、引用对象。原版直接读 `SCalendar` 字段（日 `== 0x2F` 特殊路径、`+0x1c` 三态）；重构禁止读日历结构偏移（DECISIONS D4），叙事只查询 `Calendar.snapshot()`；日期条件在 kind 事件到来时求值（kind 5 / 6 的事件源是 06 的 `TownTick` / `SeaTick`，词表 §14-6；`schedule: tick` 只是调度门，不是叙事事件源），不自己算月末 / 闰月。

### 事件旗

```
word_index = flag_id / flag_bits_per_word     # 原版 32
bit_mask   = 1 << (flag_id % flag_bits_per_word)
if word_index >= flag_word_count: saturate/reject
set:   words[word_index] |= bit_mask
clear: words[word_index] &= ~bit_mask
test:  (words[word_index] & bit_mask) != 0
```

| 参数 | 原版 | 落点 |
| --- | --- | --- |
| `flag_word_count` | 8 | `config/caps.yaml` → `narrative.flag_words` |
| `flag_bits_per_word` | 32 | 随实现整数宽；内容包按 256 旗生成 |
| `flag_sentinel_byte` | 0xFF | 打包四旗时的「无旗」 |

上限进 `caps.yaml`。模组可加长旗数组，旧存档按包版本补 0。

### 脚本

运行时（命令树形式；字节码只在导入期解释）：

```
run_script(blob, context) =
    interpreter.step(blob, pc=0, context)
    # 遇到 query：Narrative.ask(system, id…)
    # 遇到 command：Narrative.issue(command)
    # 原版 result 0 停；result 1 = 改写本触发下一段后停；execute 0x3FFF = 剧情 Game Over
```

[确认] `CEventScriptBuffer+0x8` 是「当前已加载 Sc 记录 id」，构造写 `0xFFF` 表示未加载；`fn_1400016db` 只在请求 id 不等时重载。不是最大脚本长度。

## 5. 耦合分析

原版焊点：

1. **文件名 = 剧本号** `EV{n}` / `Sc{n}`。重构后由 `scenarios.yaml` 指出包 id。
2. **触发求值读 `CEvent` 成员**（`+0x88` kind，`+0x8c/+0x90/+0x94/+0x98/+0x9c` 当前上下文，由调用方的 `sEventTrigger` 请求拷入）。这是把「世界现在在哪」缓存在事件对象上。重构改为求值时 **LIVE 查询**：`Town.current()`, `Calendar.snapshot()`, `Combat.last_result()`, `Flags.test()`。禁止事件对象持有 `World*` / `Player*`。
3. **脚本直接调货舱**（原版 `CountFleetItem` / `AddLoad`）。允许，但必须走货舱所有者 ships 的接口 `Ships.count_item(fleet_id, item_id)` / `Ships.add_load(fleet_id, item_id, delta)`，脚本字节里只存 ID。
4. **`fn_140001b09` 体积约 4KiB 的巨型 switch**——kind 语义与脚本 then 块搅在一起。重构把「条件」放到 `triggers.yaml`，「结果」放到 `events.yaml` 的 `commands:` / `script_blob`。
5. 其它系统若写 `if scenario==3 && flag==12` 即违规。旗的**含义**属于叙事内容包，道具系统只认「收到 `Items.acquire` 命令」。

`CEvent` 是 `CSaveObject`：旗与「已触发」状态进存档，定义（EV 文件）不进存档。

## 6. 解耦后的配置

内容管线把每个 EV 拆成「触发器表 + 脚本」两部分。脚本能按 `03` 反编译的进 `commands`，不能的进 `opaque`，禁止假装已经译码。

```yaml
# packs/vanilla-ev0/pack.yaml  事件包清单（12-ui-save 要求带版本）
schema: dk4.narrative.event_pack
version: 1
id: vanilla-ev0
content_version: 0x106          # 原版 EV 头 +0x02；只作来源标记
state_schema_version: 1         # 本包 trigger_state / flags 的存档节版本
save_compat:                    # 读档时比较存档节里的 state_schema_version
  accept: ">=1 <=1"
  migrate: []                   # 迁移链：[{from: 1, to: 2, script: ...}]
requires: [{ id: vanilla-core, version: ">=1" }, { id: vanilla-world0, version: ">=1" }]
```

存档记 `event_pack` id + `state_schema_version`，**不**记 `content_version` 作锁：内容修订（改对白、加触发）只要不改已有触发 key 就能读旧档；删掉的触发按 `orphan_policy` 丢弃其状态。原版「EV 版本不等就拒」只作 `legacy` 导入时的提示。

```yaml
# config/narrative/triggers.yaml
schema: dk4.narrative.triggers
version: 1
# kind 语义已由开火调用点钉死（见 03）。整数只是原版包编码，引擎认 key。
# RTTI 类名是可选条件块，不是 kind：
#   SEventTriggerTime, SEventTriggerCloseBattle, SEventTriggerEventFlag,
#   SEventTriggerMultipleTown, SEventTriggerReference,
#   SEventTriggerReferenceMultiple, SEventTriggerEventOutcome
kinds:
  - { id: 0, key: item_discovered }
  - { id: 1, key: battle_started }
  - { id: 2, key: battle_ended }
  - { id: 3, key: fleet_contact }
  - { id: 4, key: port_arrived }
  - { id: 5, key: in_town }
  - { id: 6, key: at_sea }
  - { id: 7, key: facility_entered }
  - { id: 8, key: village_landed }
# 字段定义（ctx 比较方式、事件源）在 trigger_kinds.yaml，见 03。
```

```yaml
# config/narrative/events.yaml
schema: dk4.narrative.events
version: 1
pack: EV0
triggers:
  - id: 0
    kind: 0
    on_fire: { one_shot: true, relatch: true }   # 原版 dword[1] bit31 / bit30，逐条数据
    when:
      opaque: "base64:...."      # 原版变长记录原样
      # 译码完成后升级为：
      # calendar: { year: 1522, month: 4 }
      # towns: [11]
      # flags_all: [3, 7]
      # flags_none: [4]
      # owns_item: 12
      # after_battle: true
    then:
      script_blob: "base64:...."   # 导入格式
      # 规范形式：commands: [ ... ]（见 03 第 6.1 节）
      outcome_opaque: "base64:...."
```

译码完成后 `when` / `then` 只许出现 ID 与纯值：

```yaml
when:
  kind: port_arrived
  towns_any: [vanilla:lisbon, vanilla:ceuta]   # 原版 MultipleTown 块
  flags_all: [vanilla:ev0.raphael_intro]
then:
  commands:
    - { op: set_flag, flag: vanilla:ev0.f12 }
    - { op: acquire_item, fleet: player, item: vanilla:item_40 }   # → Items.acquire
    - { op: add_load, fleet: player, trading_item: vanilla:item_t3, delta: -1 }   # → Ships.add_load
    - { op: move_fleet, fleet: player, town: vanilla:lisbon }
    - { op: say, string_key: ui.event.ev0.012 }
    - { op: menu, choices: [ui.event.yes, ui.event.no], store_flag: 13 }
    - { op: play_still, still_id: 8 }
    - { op: play_anim, clip: EVA/3 }
```

未译码的 opcode 不得用「参考大航海4」的散文代替。提取器把残余写入 `opaque`。

静画 / 背景：

```yaml
# config/narrative/stills.yaml   # 可选，或并进 events
schema: dk4.narrative.stills
version: 1
backgrounds:
  - id: 0
    pack: EventBG1
  - id: 0x14d
    pack: EventBGEX
    sub_index: 0
```

`EVA` 17 条 clip 进 `animation_pack`，事件只引用 clip 下标。

## 7. 独立旋钮

改这些不应迫使经济 / 战斗改代码：

- 某触发的 kind、条件、是否一次性
- 脚本 blob / 将来的 `commands` 列表
- 旗宽度（`flag_words`）
- 背景图包、EVA clip、Sc 场景图
- 把一条「入港触发」改成「日期触发」
- 新增 kind（引擎按表长；原版解释器写死 9，重构后 9 只是默认包）

需要其它系统配合的只有：命令里出现的 `town_id` / `item_id` / `person_id` 必须在对应表里存在（加载期外键校验）。

## 8. 对外接口

本系统订阅：

| 事件 | 来源 | 动作 |
| --- | --- | --- |
| `ScenarioSelected` | 剧本 | 加载 `event_pack` + `EVA` + `scene_pack` |
| `DiscoveryFound` / `BattleStarted` / `BattleResolved` / `FleetSpotted` / `ArrivedAtTown` / `TownTick`(港内) / `SeaTick`(海上) / `FacilityEntered` / `VillageLanded` | Ocean / Combat / Town-life | 按 kind 0–8 扫描（映射表见 `03` 第 8 节；名称以 `00-overview/06-system-interfaces.md` 为准） |

原版没有「旗变了 / 货变了再扫」：日期、旗、货物只是条件块，在上面这些事件到来时一起求值。

查询（LIVE，取不到则本条 no-op）：

```
Calendar.snapshot() -> {year, month, day, era_tag}
Town.current_for(fleet_id) -> town_id | none
Combat.last_result(fleet_id) -> result_id | none
Flags.test(flag_id) -> bool
Ships.count_item(fleet_id, item_id) -> int        # item_id = 交易品 key
Items.owned(fleet_id, item_id) -> bool
Towns.def(town_id) -> TownDef | null             # 存在性 = 非 null
```

命令（脚本可发，接收方是各系统，不是本系统内部表）：

```
Flags.set(flag_id, bool)
Ships.add_load(fleet_id, item_id, delta)         # item_id = 交易品 key
Items.acquire(fleet_id, item_id)
Scenes.goto_town(town_id | current, facility_id?)   # 原版 0xC2
Minigames.run(minigame_id) -> int                   # 原版 0xC1
Session.end_scenario(cleared | game_over{ending_id} | to_title, cause) # 原版 0xC5；0x3FFF = game_over{natural_death}；脚本来源 cause = scripted
UI.play_still(still_id)
UI.play_anim(clip_id)
UI.say(string_key)
UI.menu(choices) -> choice_index
```

禁止脚本拿到 `CTown*` / `CPerson*`。存档只存（`narrative` 节，自带节版本，DECISIONS D7）：`event_pack` id + `state_schema_version`、旗（按 key）、每条触发的 `trigger_state`（按触发 key）。不存 EV 原文，不以 EV 内容版本作锁。原版脚本在一次扫描内跑完，没有中途 PC 要存。

## 9. 未核实字段

| 项 | 现状 | 禁止 |
| --- | --- | --- |
| `eTriggerKind` 0–8 的原版符号名 | 语义已钉（第 2 节），符号名无头文件 | key 用自拟英文名；日期 / 旗不是 kind |
| `SEventTriggerReference` 15 种引用类型 | 0 千分比随机、6 日期、8 货舱、14 遗迹等已钉，5 / 12 未知（见 `03`） | 不要把 0 写成金币 |
| `sTriggerDate` vs `SEventTriggerTime` | RTC 两个名字，RTTI 一个 Time 类 | 不要拆成两个确认 kind |
| EV 段 2 / 3 单条记录布局 | 段用途已钉，逐位未核 | 单条进 `opaque` |
| EVA 解析后魔数 `0x01004145`、17 clip 的内容 | 文件是 offset 表 | 不要当 WE 脚本 |
| Sc* 记录 | `DS` 头 + 指令流 + 内嵌对白（`03`） | 不要当静画 atlas / 场景图 |
| `CEvent+0x94/+0x98/+0x9c`（kind 1–3 对手属性） | 取值函数已知，字段名未还原 | 重构后由事件载荷直接给 |
| `SCalendar` 日 `0x2F`、`+0x1c` 的 0/1/2 | 月末？旬？ | 日历文档负责，叙事只读快照 |
| 256 旗与触发状态的存档偏移 | `CEvent` 是 SaveObject，`vfunc_06/07` 读写 | 等 `12-ui-save` 标偏移 |
| `NUM_HAPPENING=5` | 与触发 9 槽不是同一枚举 | 不要并进 `trigger_kind` |
| `EventBG` 每文件多少张 | 映射函数里出现除数 `0x32=50`、`8` 张包 [推断] | 未当确认 |
| class 2 多数 subclass、class 1 菜单操作数 | 跳表在，长度未闭合 | 进 `opaque`，该触发拒绝加载 |

提取器：确认的头字段进 YAML；触发位域进 `when` / `on_fire`；Sc 字节码按 `03` 反编译成命令树，残余进 `opaque`。
