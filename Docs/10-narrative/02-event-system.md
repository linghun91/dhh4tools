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

- `fn_1400050fb`（RTC：`sScriptTable`, `sExecute`）：循环取下一条。`sExecute==1` 走脚本表，`==2` 走内联脚本，`id==0x3fff` 为特殊退出。
- `fn_1400020fe`（RTC：`sScriptResult`, `lNextOffset`, `sScript`）：按结果 0/1/2 决定停、跳、继续。`lNextOffset` 是字节码程序计数器。

来源：`srcdata/source/game/unassigned/fn_1400c2290.cpp`。

[确认] 脚本会跨系统读 / 改状态。仓库里已挂上的入口：

- `CountFleetItem`：舰队某商品件数（注释写「委托/剧情」）
- `AddLoad`：舰队增减货物（`n<0` 扣）

来源：`srcdata/INDEX.md`、`srcdata/docs/trade_stack.md`。`dhh4tools/README.md` 也按「事件检查和扣除货物」描述同一对函数。

[确认] 另有一段脚本辅助（`fn_14001098d`，body `0x1400c8390`）把操作数与 `0x61`（`NUM_TOWN=97`）和 `0x19`（25，与 `NUM_FORCE` / `NUM_SEAAREA` 同数）比较后再去取对象。这是「脚本持有 `town_id` / 势力或海域 id」的证据，不是完整 opcode 表。

opcode 级译码**未完成**。解耦配置按「不透明脚本 blob + 触发器表」落地，见第 6 节。

### EV 文件格式（已还原的头）

[确认] `Game/SC/EV0.dk4`…`EV6.dk4` 头 96 字节（`0x60`）。加载时 `CEvent::vfunc_05` → `fn_140007dec` 读全文件，校验 `*(u16*)file == 0x4557`（`'W''E'`）。`fn_14000af9c` 还会按 7 个剧本各读 0x60 头缓存到 `DAT_140682858`。

布局：

| 偏移 | 类型 | 含义 |
| --- | --- | --- |
| +0x00 | uint16 | 魔数 `0x4557` `'WE'` |
| +0x02 | uint16 | 版本。七个文件分别为 `0x0106, 0x0104, 0x0106, 0x0103, 0x0104, 0x0105, 0x0104` |
| +0x04 | uint32 | 载荷字节数。`文件大小 = 0x60 + 该值` |
| +0x08 | uint16 | 触发条目总数 |
| +0x0A | uint16[9] | 每种 `eTriggerKind` 的条数。`fn_140010465` 读 `word[kind]` |
| +0x1C | uint16 | EV0 为 0 |
| +0x1E | uint16 | 再次出现总数（与 +0x08 相同） |
| +0x20 | uint32 | 额外段偏移/计数。EV0 = `0xE6`。[未知] 精确语义 |
| +0x24 | uint32 | 0 |
| +0x28 | uint32[9] | 每种触发器在**载荷**里的起始偏移（相对 `file+0x60`）。`fn_140011c57` 读 `dword[kind]` |
| +0x4C 起 | uint32… | 后续段起点（脚本 / 消息 / 静画？）。EV0：`0x1A8C, 0x1D44, 0x1D44, 0x20DC, 0x20DC`。[未知] 段名 |

[确认] 九个计数之和等于 +0x08。EV0：`(5,16,16,4,2,85,10,203,5)` 和 = 346。

[确认] 单条触发记录变长：用 `SMemorySeeker` 按流解析，不能用固定 stride 切。`fn_1400095c5(event, out, kind)` 把指针放到 `buffer + offset[kind] + 0x60`。

### 触发器种类（9 槽）

[确认] `NUM_TRIGGERKIND = 9`。求值函数 `fn_140001b09`（body `0x1400bf5e0`）对 `CEvent+0x88` 做 `switch (kind)`，`ja` 上限 8，即 kind 0–8。跳表 VA `0x1400c05bc`。

[确认] 扫描入口 `fn_140013c32`：取当前日历 `SCalendar`，按 `CEvent+0x88` 的 kind 取该类全部触发器，逐条 `fn_140001b09(event, trigger, calendar)`；命中则 `fn_1400050fb` 执行脚本。

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

[未知] kind 0–8 与上表类名的 1:1。没有 `TriggerKindID.h`。禁止把「入港 / 日期 / 持有道具」写成已确认的枚举值。用户侧的玩法语言（入港、日期、旗、持有物）对应这些 RTTI / RTC 名，但槽位号未钉死。

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

k7 条数最多（约一半），k0 在 EV5/EV6 为 0。

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

[确认] `Sc0.dk4`…`Sc6.dk4` 与 EV 一对一。`ScN` 第一个 `uint32` 等于 `EVN` 触发总数（Sc0=0x15A=346）。这是「每条触发/事件一条场景图目录」的强证据，内部块格式未还原。`formats.md` 只写「场景图」。

### 其它相关计数

[确认] `NUM_HAPPENING = 5`，RTC 名 `eHappening`。求值样本 `fn_1400f2d90` 里同一局部还和 `0x14`、`0x1E` 比较，**不能**当成 EV 的第五种触发。更像海上随机遭遇 / 另一套「发生」枚举。未并入 `eTriggerKind`。

[确认] `NUM_KIND = 9` 与 `NUM_TRIGGERKIND = 9` 同数；事件侧循环写 `eKind >= 9` 与 `eTriggerKind >= 9` 各有一份。是否同一枚举未在头文件钉死，配置里只暴露 `trigger_kind` 0–8。

[确认] `CWaitTriggerView`：UI 按钮，等玩家点「继续」一类触发，不是 `eTriggerKind` 的一种。

## 3. 实体与 ID

| 实体 | 原版 | 空值 | 备注 |
| --- | --- | --- | --- |
| `event_pack` | EV0–EV6 | 无 | 由剧本表选择 |
| `trigger_kind` | 9 | 无 `INVALID` 符号 | 运行时按表长；原版 0–8 |
| `trigger_id` | 每包 250–346 条 | 无 | 重构用 `(pack, index)` 或稳定 key |
| `flag_id` | 256 bit | 未置位 = 0 | `word = id>>5` |
| `script_blob` | 不透明 | 空 blob 禁止开火 | opcode 未译码前不拆 |
| `still_id` | EventBG 常规 + EX 特例 id | `0xFFFFFFFF` 在 Still 构造里出现 | |
| `town_id` / `item_id` / `person_id` | 外键 | 各系统的 `invalid` | 触发条件与脚本命令都只持 ID |

`SMemorySeeker` 是流式游标（`ptr`），不是玩法实体。重构不要把 seeker 指针存进存档。

## 4. 算法 / 公式

### 开火扫描

原版每个 tick / 场景切换：对当前 kind（`CEvent+0x88`）的全部触发走：

```
for trigger in pack.triggers_of(kind):
    if not match_condition(trigger, context):
        continue
    if already_consumed(trigger):          # 一次性？未还原，见第 9 节
        continue
    run_script(trigger.script)
```

`context` 至少包括：`SCalendar`（年/月/日/旬，`SCalendar+0x1c` 出现 0/1/2 三态）、当前港 `town_id`、战斗是否刚结束、事件旗、引用对象。日历构造把日 `== 0x2F`（47）当特殊月末路径——**日期编码未完全还原**，叙事只查询 `Calendar.snapshot()`，不自己算闰月。

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

opcode 未译码时：

```
run_script(blob, context) =
    interpreter.step(blob, pc=0, context)
    # 遇到 query：Narrative.ask(system, id…)
    # 遇到 command：Narrative.issue(command)
    # 遇到 0x3fff / result==0：停
```

原版 `CEventScriptBuffer+0x8 = 0xFFF`（4095）[未知] 是容量还是哨兵，不要当成确认的最大脚本长度。

## 5. 耦合分析

原版焊点：

1. **文件名 = 剧本号** `EV{n}` / `Sc{n}`。重构后由 `scenarios.yaml` 指出包 id。
2. **触发求值读 `CEvent` 成员**（`+0x88` kind，`+0x8c/+0x90/+0x94/+0x98` 当前上下文）。这是把「世界现在在哪」缓存在事件对象上。重构改为求值时 **LIVE 查询**：`Town.current()`, `Calendar.snapshot()`, `Combat.last_result()`, `Flags.test()`。禁止事件对象持有 `World*` / `Player*`。
3. **脚本直接调贸易货舱**（`CountFleetItem` / `AddLoad`）。允许，但必须走命令接口 `Economy.count_fleet_item(fleet_id, item_id)` / `Economy.add_load(...)`，脚本字节里只存 ID。
4. **`fn_140001b09` 体积约 4KiB 的巨型 switch**——kind 语义与脚本 then 块搅在一起。重构把「条件」放到 `triggers.yaml`，「结果」放到 `events.yaml` 的 `commands:` / `script_blob`。
5. 其它系统若写 `if scenario==3 && flag==12` 即违规。旗的**含义**属于叙事内容包，贸易只认「收到 `GiveItem` 命令」。

`CEvent` 是 `CSaveObject`：旗与「已触发」状态进存档，定义（EV 文件）不进存档。

## 6. 解耦后的配置

opcode 未还原之前，内容管线把每个 EV 拆成「触发器表 + 不透明脚本」两张表，禁止假装已经译码。

```yaml
# config/narrative/triggers.yaml
schema: dk4.narrative.triggers
version: 1
# kind 0–8 的符号名未还原。下面只登记槽位。
# RTTI 类名全集（不能 1:1 填进 id）：
#   SEventTriggerTime, SEventTriggerCloseBattle, SEventTriggerEventFlag,
#   SEventTriggerMultipleTown, SEventTriggerReference,
#   SEventTriggerReferenceMultiple, SEventTriggerEventOutcome
kinds:
  - id: 0
    key: kind_0
    name: null
  - id: 1
    key: kind_1
    name: null
  - id: 2
    key: kind_2
    name: null
  - id: 3
    key: kind_3
    name: null
  - id: 4
    key: kind_4
    name: null
  - id: 5
    key: kind_5
    name: null
  - id: 6
    key: kind_6
    name: null
  - id: 7
    key: kind_7
    name: null
  - id: 8
    key: kind_8
    name: null
```

```yaml
# config/narrative/events.yaml
schema: dk4.narrative.events
version: 1
pack: EV0
triggers:
  - id: 0
    kind: 0
    once: true                   # 原版是否一次性未还原，先可配
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
      script_blob: "base64:...."
      # 译码完成后升级为 commands: []
      outcome_opaque: "base64:...."
```

译码完成后 `when` / `then` 只许出现 ID 与纯值：

```yaml
when:
  kind: multiple_town
  town_ids: [11, 12]
  flags_all: [flag.raphael_intro]
then:
  commands:
    - { op: set_flag, flag_id: 12 }
    - { op: give_item, person_id: 0, item_id: 40 }
    - { op: add_load, fleet_id: 0, trading_item_id: 3, delta: -1 }
    - { op: move_fleet, fleet_id: 0, town_id: 11 }
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
| `ArrivedAtTown {fleet_id, town_id}` | Ocean / Town | 扫描港口类触发 |
| `DayAdvanced {calendar}` | 世界时间 | 扫描日期类触发 |
| `BattleResolved {result}` | Combat | 扫描 `CloseBattle` |
| `FlagChanged` | 自己 | 再扫一遍旗触发（防止漏） |
| `CargoCommitted` | Economy | 若条件含持有货物则扫描 |

查询（LIVE，取不到则本条 no-op）：

```
Calendar.snapshot() -> {year, month, day, era_tag}
Town.current_for(fleet_id) -> town_id | none
Combat.last_result(fleet_id) -> result_id | none
Flags.test(flag_id) -> bool
Economy.count_fleet_item(fleet_id, trading_item_id) -> int
Characters.has_item(person_id, item_id) -> bool
World.town_exists(town_id) -> bool
```

命令（脚本可发，接收方是各系统，不是本系统内部表）：

```
Flags.set(flag_id, bool)
Economy.add_load(fleet_id, trading_item_id, delta)
Items.give(person_id, item_id)
Fleet.move_to(fleet_id, town_id)          # 未还原 opcode，接口先留
UI.play_still(still_id)
UI.play_anim(clip_id)
UI.say(string_key)
UI.menu(choices) -> choice_index
```

禁止脚本拿到 `CTown*` / `CPerson*`。存档只存：`event_pack` id + schema 版本、256 旗、已触发集合（若有）、解释器 PC（若中途存盘）。不存 EV 原文。

## 9. 未核实字段

| 项 | 现状 | 禁止 |
| --- | --- | --- |
| `eTriggerKind` 0–8 的符号名 | 只有 9 槽、跳表、7 个 RTTI 类、RTC 局部名 | 不要写成「0=入港 1=日期 …」 |
| `SEventTriggerReference` 引用的是道具、人物还是事件 | 类名 + `SEventReferenceData` | 不要写成确认的「持有道具」 |
| `sTriggerDate` vs `SEventTriggerTime` | RTC 两个名字，RTTI 一个 Time 类 | 不要拆成两个确认 kind |
| EV 头 `+0x20`、`+0x4C` 后各段 | 偏移存在，名字没有 | `unknown_segments[]` |
| EVA 解析后魔数 `0x01004145`、17 clip 的内容 | 文件是 offset 表 | 不要当 WE 脚本 |
| Sc* 内部块 | 头一个 uint32 = EV 总数 | 不要当已还原的静画 atlas |
| 触发是否一次性、是否可重复 | 扫描里有 `fn_14000fea2` 把命中改写为 0 的分支 | 标 [未知]，YAML `once` 可配但默认值未证明 |
| `CEvent+0x8c/+0x90/+0x94/+0x98` | 求值拿来和触发记录比 | 未命名；重构后取消这些缓存，改为 LIVE 查询 |
| `CEventScriptBuffer+0x8=0xFFF` | 构造写入 | 不要当 max_script_bytes |
| `SCalendar` 日 `0x2F`、`+0x1c` 的 0/1/2 | 月末？旬？ | 日历文档负责，叙事只读快照 |
| 256 旗的存档偏移 | `CEvent` 是 SaveObject | 等 `12-ui-save` 标偏移 |
| `NUM_HAPPENING=5` | 与触发 9 槽不是同一枚举 | 不要并进 `trigger_kind` |
| `EventBG` 每文件多少张 | 映射函数里出现除数 `0x32=50`、`8` 张包 [推断] | 未当确认 |
| 脚本 opcode 表 | Execute / Message / Menu 三类 RTTI | 未译码前整段当 blob |

提取器：确认的头字段进 YAML；每种 kind 的原始字节进 `opaque`；直到有 opcode 文档再升 schema version。
