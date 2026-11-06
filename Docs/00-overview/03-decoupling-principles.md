# 解耦原则

## 问题

原版是一份 C++ 程序 + 两份世界 DLL + 一堆 `.dk4`。玩法规则、数值上限、内容实例、UI 文案混在同一套编译单元里。改「伦敦的特产品」要碰二进制表；改「经验除数 30」要搜立即数。

重构把游戏切成四层：

```
内容实例  →  表（港口、人物、商品）
规则参数  →  公式与上限（caps / formulas）
系统逻辑  →  解释器（贸易、航海、战斗）
表现     →  场景、文案、动画
```

层与层只通过 **稳定 ID** 和 **窄接口** 说话。

## 原则 1 — 一个系统一份目录

每个玩法系统拥有：

- `Docs/<n>-<system>/` 设计文档
- `config/<system>/` 运行时表
- 一组事件名（`ArrivedAtTown`, `TradeCommitted`, `BattleResolved`；规范名登记在 `06-system-interfaces.md`）

不允许「贸易文档里夹一段航海伤害公式」。需要对方数据时，写**查询接口**，例如 `Economy.quote(town_id, item_id) -> PriceSnapshot`。

## 原则 2 — ID 注册表是唯一共享层

`config/ids/` 由内容表生成，不手写两份。运行时校验：

- 任何外键必须指向已注册 key，或为 `null`（运行时 `Option<Id>`）
- 空值不由计数推导。原版哨兵（空货 `0x6A=106`、无港 `0x62=98`、空道具 `0xE8`、无人 `0xEC`、空舰队 `0x46`、空遗迹 / 空势力 `0x1A` …）只出现在导入 / 导出层 `config/ids/legacy_codec.yaml`；不用 -1、0 或 NUM+1 当空
- 内容包可以比原版更长；代码按表长循环，不按 `NUM_TOWN`
- 跨包引用用**命名空间字符串 key**（`vanilla:london`、`mymod:new_port`）。整数只是运行时紧凑下标，不写进 YAML、不对外承诺稳定；原版条目另带 `vanilla_index` 供导入对照。存档存 key↔int 映射表，读档时重映射，缺失按 `orphan_policy`。规则见 `05-config-architecture.md`「ID 与命名空间」与 `09-architecture-decisions.md` D1

## 原则 3 — 公式全是纯函数

所有战斗、价格、经验、技能、航海速度写成：

```
skill_value = skill_formula(stat_a, stat_b, level, params)
```

`params` 来自 `config/formulas/`。系统代码不得出现除法常数 30、150、1000。原版魔数作为该公式的 **default 内容包** 写入 YAML。

只给参数不够。原版公式有四种形状，公式层分四档（`09-architecture-decisions.md` D3）：

| 档 | 例子 | 配置形式 |
| --- | --- | --- |
| ① 参数 / 表 / 分段 | 经验除数 30、技能 `(A+B)(L+100)/150` 的常数；天气 `rand(100)` 落在 `<30,<50,<70,<80,<87,<96`；身心系数 19/20、1/2、1/10、3/5 | `params:` / `table:` / `cdf:` / `piecewise:` |
| ② 沙箱表达式 | 道具还价、经验式、单式伤害 | `impl: expr`：整数四则、显式 `/`（截断）与 `udiv`、`min/max/clamp`、查表、`rand(n)`（命名流） |
| ③ 换实现 | 控制流重的航速、市价循环、投资分档 | `impl: vanilla.<name>` 代码注册的原版实现；模组可注册 `impl: <pack>.<name>` 插件 |
| ④ 修正器出口 | 技能值、航速加成、买价系数 | `modifiers:` 引用 `Modifiers.eval(key, ctx)`（原则 6） |

**不**做通用脚本；脚本只在叙事事件边界。

纯函数还要求：

- 随机数作为参数传入（命名 RNG 流句柄），公式自己不取全局随机
- 整数语义写在合同里：`int_mode: { div: trunc | floor, width: wrap32 | int64 | sat64 }`；原版是 32 位、向零截断、每步截断（如 `fn_1401c87a0` 先乘后除），预设 `vanilla_i32 = { div: trunc, width: wrap32 }`，原版包用它才能逐位重现
- 原版怪癖用 `quirks:` 显式保留或关闭，不悄悄修：`unsigned_div`（`1000-a` 为负后无符号 `div 0x3e8`）、`u8_truncate`（初始水手 `(u8)((min+max)/2)`）、`level_q1_zero`（exp 30–59 为 0 级）
- 每条公式带 `golden` 用例，加载期自检；原版包的 `expr` 版必须与 `vanilla.*` 逐位一致
- 代码与公式里不得出现 `== 字面量 id`（船首像 0/1/5/7/8、岗位、技能、舱室种类 7/8/9/10/11），改成内容表的 `effects` / `role` / `refs`

## 原则 4 — 状态与定义分离

| 定义（只读内容） | 状态（存档） |
| --- | --- |
| 港口坐标、默认特产品 | 发展度、份额、合约、流行 |
| 人物初始六维、姓名 | 经验、装备、隶属、疲劳 |
| 船型上限 | 当前耐久、帆、炮、货 |
| 交易品基础价、种类 | 市场价、供给、品牌 |

定义进内容包；状态进存档 schema。禁止把定义拷进存档再改（原版部分表会在运行时被改写，重构时要标出哪些字段是可变的）。

原版证据：`GetFix*` 返回 DLL 数组的非 const 指针（`DllMain.cpp`）；存档人物段对 id < 87 的人存六维与两段经验（`12-ui-save/02-save-session.md`），即「初始六维」在运行时已是状态。重构：每个字段在 schema 里标 `mutable: false | true`，`true` 的初值来自定义、此后只活在存档。

## 原则 5 — 事件，不直调

例：入港

1. Ocean 发出 `ArrivedAtTown { fleet_id, town_id }`
2. Town 打开设施图
3. Economy 按 `town_id` 刷新当日市价
4. Narrative 检查该港触发器

Ocean 不知道交易所怎么算价。Economy 不知道船在哪一格。

事件语义（合同，不是建议）：

- **事件是事实，不是请求。** 只有状态所有者发「已发生」事件；想改别人状态发命令（见 `06-system-interfaces.md`）。
- **同 tick 内排队，阶段边界派发。** 处理函数里发的新事件进队尾，同一阶段内处理完再进下一阶段。禁止处理函数同步递归调另一个系统。
- **派发顺序确定。** 订阅者按 `05-config-architecture.md`「tick 顺序」的系统序号排，不按注册先后。
- **处理函数可读快照、可发命令，不可直接写别的系统的状态。**
- 场景切换、UI 弹窗是「等待玩家」的异步点：模拟 tick 暂停，事件队列清空后才交给 UI。

## 原则 6 — 修正器统一出口

原版把技能值直接塞进买货、战斗、道具价（`1000-a`、`500-a`、`100-a/50`），把船首像、舱室种类、岗位写成 id 比较。重构后只有一个出口：

```
Modifiers.eval(key, ctx) -> value        # key: skill.accounting / fleet.speed / trade.npc_buy_cost …（规范 key 见 10-key-glossary §6）
```

它**不是**写死字段的结构体，而是 **键注册表 + 带来源的贡献列表**（`09-architecture-decisions.md` D2）：

- 键在 `config/modifiers.yaml` 声明（`key`、`type: int|factor|bool`、`int_mode`、`clamp`、`owner_system`）。模组可加键。
- 提供方注册贡献 `{ key, stage, value, order, scope?, stack?, when?, source: { kind, key } }`（`value` 是整数或 `{ num, den }`，写法见 `10-key-glossary.md` §6.1）。来源有装备、舱室、身心状态、船首像、岗位、剧本旗标、势力加成、事件 buff。提供方不调用消费方。
- 求值阶段固定：`base → add → mul → clamp → override`，阶段内按 `order` 再按 `source` 排序，保证确定、可追溯。原版技能值正是这条链：`技能基值(两维+等级) + 5 格装备 → × 身体态系数（19/20、1/2、1/10、3/5，整数截断）→ clamp [0, 500]`（`fn_1400135cf` → `fn_14000e0b6` → `fn_1401c87a0` → `fn_1401cbb40`）。航速也是：船首像 `+s/10` → `×4/5` → 海战 `/3`，顺序决定取整。
- 业务公式的输入单位是**技能值**（0–500，`skill_value_cap`），不是六维（0–100，`ability_cap`）。原版 `fn_14000e1ce(person, k)` 的 `k` 是 `eSkill`。
- 「岗位持有人」`Fleets.post_holder(fleet_id, post_key)` 与「舱室计数」`Ships.cabin_count(ship_id, role_key)` 是一级查询，替代原版「玩家本舰队取岗位 N，否则取提督 / 船长」三分支和舱室种类字面量。
- 保留来源列表供 UI 解释与撤销；缓存按事件失效（`PersonEquipped`、`CabinBound`、`VitalsChanged`、`LevelChanged`、`FigureheadChanged`、`ScenarioSelected`）。
- 修正器只给**确定值**。随机项（如买货的 `rand(100)`）属于消费方公式。

各系统只按 key 取值，不读六维原值，也不自己套技能公式。

## 原则 7 — 七个剧本、两份世界包、剧本差异只走旗标

World0.dll 与 World1.dll 同一 schema（只含五张表；其余原版表在 `vanilla-core`）。`narrative/scenarios.yaml` 指定每个剧本加载哪份世界包、哪份事件脚本（EV0–EV6）。

原版在系统代码里比较剧本号（`DAT_144296500+0x7c`）：剧本 0 委托 ×1.5、军事投资八折；剧本 2 卖价 +5%、流行影响力 ×2；剧本 6 宗主上缴。重构把这些全部写进剧本记录的 `flags` / `modifiers`，由叙事开局注入修正器注册表；公式只写 `enabled_by: scenario_flag.<key>`。引擎与公式文件禁止比较剧本号（D5）。

## 原则 8 — 上限可配且有硬顶说明

原版：单段等级封 100（`fn_14001377d` 无符号夹取）；改上限到 256 以上时 `q<<16` 32 位回绕（`fn_14000819d`）。重构：

- `caps.yaml` 给设计上限
- `engine_limits.yaml` 给实现硬顶（整数宽度）
- 公式实现必须对溢出有定义：默认 `int_mode.width: sat64`（饱和）；原版回绕只作 `quirks` 供对拍

## 原则 9 — 原版兼容是内容包，不是引擎特性

「5 艘船、5 货格、商品不堆叠、技能公式里等级和封 200、技能值封 500、六维封 100」全部是原版内容包的默认值。引擎允许更大。显示层自己处理「价格列放得下几位数字」，不要用 UI 宽度倒逼规则。

## 原则 10 — 时间与随机数是引擎服务

日历推进、tick 顺序、随机数流由引擎核心提供，系统只订阅 / 只拿句柄。原版把这些写死在一个世界 tick 函数和一个全局 `rand()` 里（`02-original-architecture.md`）。合同见 `05-config-architecture.md`「tick 顺序」「随机数」。

## 原则 11 — 派生量只有一个计算方

必要 / 上限水手、帆值、炮门数、估价、技能值、航速、搜索半径、航线可见、港口所属海域这类由多张表算出的量，指定唯一计算方，消费方只读快照，不存档、读档后重算。表见 `06-system-interfaces.md`「派生量所有权」（D6）。原版必要水手 `fn_14000a466` 被航海和战斗各算一遍，就是要避免的形状。

## 原则 12 — 动作执行器共用

旅馆、仪式、寻求支持、海上 happening、剧本命令都是同一形状：`Action { cost{gold,days}, guard[predicate], roll: formula_ref, effects[command], repeat{n, interruptible} }`。设施 = 地图点 kind → 处理器 + 参数，一个处理器可服务多个 kind（D9）。

## 系统依赖图（允许的方向）

两种依赖分开管：

- **查询依赖**（A 读 B 的快照）：必须是有向无环图。
- **事件订阅**（A 听 B 的事实）：允许反向，用来打破环。

查询依赖（`A ← B` = A 可以查询 B）：

```
core: ids, clock, rng, modifiers(注册表), content(只读定义，任何系统可读)
world      ← core                     # 含 Towns 运行态（已知位、发展度）、Climate、Routes
factions   ← world
characters ← world, factions          # 装备 / 舱室 / 身心经 modifiers 贡献进来，不直接查 items
ships      ← characters
fleets     ← ships, characters
economy    ← world, factions, fleets
ocean      ← world, factions, fleets
combat     ← world, factions, fleets, ships
town-life  ← world, factions, characters, economy, ships, fleets
items      ← characters, ships        # 道具状态（持有、装备位）
progression← characters
narrative  ← 全部（只读查询 + 发命令）
save       ← 所有状态系统（序列化接口）
ui         ← 所有（只读投影）
```

「content 只读定义」包括 `major_seas`、文化圈行、交易品目录、船模表等：读定义不构成查询依赖。world 用 town → culture → major_sea 选 BGM 不算查询 factions 状态。

所有系统都可调 `Modifiers.eval`（core）；提供方注册贡献，不被消费方调用。

已知环与断法：

| 环 | 断法 |
| --- | --- |
| economy 查 `Share.of` / `Share.slots`，factions 又查 `Economy.is_brand` | factions 不查 economy；订阅 `BrandRecognized` 自存需要的位 |
| characters 求装备加成 ↔ items 查人物 | items 在 `PersonEquipped` 时向修正器注册表登记 `skill.*` 贡献；characters 只调 `Modifiers.eval`，不查 items |
| progression 要贸易 / 战斗经验 | progression 订阅 `TradeCommitted` / `BattleResolved` 自己算，不被 economy / combat 调 |
| 技能值要等级、等级在 progression | 经验是人物状态字段，只有 progression 的 `Progression.grant_exp` 可写；等级式是 core 公式内容，characters 直接调，不查询 progression |
| 港口发展度被 economy / combat 写、被 ships / town-life 读 | 状态归 `Towns`（world 域）；写方发命令 `Towns.apply_develop`，读方查 `Towns.develop`（见 `06-system-interfaces.md` 裁决） |

禁止反向：economy 不得改人物经验；combat 不得写市价；ui 不得写存档字段。
