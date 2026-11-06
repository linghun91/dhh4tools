# 架构决策记录（D1–D10）

第一轮全量核查（17 组）后定稿的跨系统决策。各系统文档与本篇冲突时，**改系统文档**。每条决策给：决策、原版依据或反例（带源码证据）、对各系统的约束、YAML / 接口落点。

地址约定：`fn_14000xxxx` 是 thunk 名，括号里是 body VA。文件偏移按段换算，见 D10。

---

## D1 ID：命名空间 key 为主键，整数只在运行时

### 决策

- 主键 = 命名空间字符串 key：`vanilla:london`、`mymod:new_port`。包内引用可省前缀。
- 运行时整数下标在加载期紧凑分配，只在进程内使用，不写进 YAML，不对外承诺稳定。原版条目另带 `vanilla_index`（原枚举值），只供导入 / 导出对照。
- 存档头存 `id_map`（key ↔ int）。读档按 key 重映射；引用不到的条目按该 ID 域的 `orphan_policy`（`drop` / `replace_with: <key>` / `refuse_load`）处理。
- 运行时空值一律 `Option<Id>` / YAML `null`。原版哨兵只出现在导入 / 导出层 `config/ids/legacy_codec.yaml`；导出原版格式时 id 越界则**拒绝导出**，不截断。
- 原版隐含区间改显式字段（`recruitable`、`is_protagonist`、`home_town`、`npc_pool`、`dock_template_of` …）。
- 按下标排的矩阵改成按 key 稀疏映射 + 默认值 + 加载期覆盖率校验。
- 每个 `NUM_*` 注明证据类型：**判界**（`cmp` 断言 / 取表函数界）、**循环上界**、**RTC 投票**（`srcdata/tools/fill_gaps.py` 按立即数众数猜）。投票得来的不得标 `[确认]`。
- 废弃「模组 id 从某个大整数起」的写法。模组不写整数。

### 原版依据 / 反例

| 现象 | 证据 | 后果 |
| --- | --- | --- |
| 哨兵 = `NUM+1`，落在可分配区间 | 空货 `0x6A`=106（`Constants.h ITEM_EMPTY`，`cmp …,0x6a` 95 处）；无港 `0x62`=98（`CWorldData::vfunc_05` `0x1402aeb15`）；空道具 `0xE8`；无人 `0xEC`；空舰队 `0x46`=70；未选航线 `0xC9`=201（`CRouteNavigator` ctor）；空文化 `0x16`（`fn_140013a02`）；空遗迹 / 空势力 `0x1A`=26 | 模组顺序分配的第 2 个港 = 98 = 原版「无港」；第 2 个交易品 = 106 = 空货 |
| 同一整数在不同枚举里含义不同 | `0x1A` 同时是空遗迹（港口 `+0x26`）与空势力（人物 `+0x2A`、份额槽）；`0x6A` 同时撞 `ePerson_106`；`0x62` 撞 `ePerson_098` | 不能全局把某整数当「空」 |
| 窄存储 | 港口特产品槽商品 id 是 u8（`fn_140266800` `movzx byte`）；航线 id u8；Locator 句柄 `kind<<24 \| index`（`fn_14000cf90`） | 导出原版格式受 256 值封顶 |
| `NUM_*` 投票误采 | `NUM_SHIPTYPE=5` 实为 39（`eShipType` 循环上界 `0x27`/`0x22`）；`NUM_BATTLEGUN=5` 实为 21（`fn_14000ee8a` 循环 `<0x15`）；`NUM_SHIPMAST=100` 实为 3；`NUM_SHIP=100` 而实例池 207 槽（`fn_14000ceaa` 扫 `0..0xCF`）；`NUM_HOLD_SCAN=10` 实为舱室种类 10（`fn_140006b95`）；`NUM_TOWNFASHION=26` 疑为 `0x1A` 误采 | 把投票值写进代码会截断内容 |
| 位置即语义 | 主角 = `ePerson` 0–6；以港命名的女性 214–234；船体槽 `id-100` = 定义（`fn_14000cff9` `(id-0x64)*0x30`）；槽 100–193 进 NPC 随机池（`fn_1400f4080`） | 加人 / 加船撞区间 |
| 按下标矩阵 | 文化圈 21×105 u16 卖价（`CCulture::vfunc_05` 循环 `<0x69`）；`CTown+0x4A` 19×u16 按种类；势力 7 格按大海域 | 加一种交易品要给 21 圈各补一格 |

### 约束

- 代码里不得出现 `== 0x62`、`== 0x6A` 之类哨兵比较；也不得出现 `id >= N` 推断「是不是模组」。
- 循环按表长；`NUM_*` 只作原版包实例计数。
- World0 / World1 共用一套 `vanilla:*` key；`vanilla-world1` 对同一批 key 做 `$op: patch`，不新建 key。
- 定义与实例分离：原版 NPC / 开局舰队直接挂船体模板槽（槽 100–206，`fn_140007153` 只写一个字节引用），重构一律从 `hull` 定义克隆出独立 `ship_instance`，不复用模板实例。
- 原版条目的整数字段名统一叫 `vanilla_index`（`10-narrative/*` 的 `legacy_index` 视为同义，应改名）。

### 落点

- `05-config-architecture.md`「ID 与命名空间」「与存档的关系」
- `04-id-registry.md`「遗留哨兵与存储宽度」「扩展风险」
- `config/ids/legacy_codec.yaml`（枚举 × 字段 → 原版空值字节、存储宽度）
- `12-ui-save/02-save-session.md` 存档头 `id_map`

---

## D2 修正器：按 key 注册的贡献，不是大结构体

### 决策

- 不定义写死字段的 `ModifierSet` 结构体。改为**修正器注册表**：`Modifiers.eval(key, ctx) -> value`，`key` 是命名空间字符串（`skill.accounting`、`fleet.speed`、`trade.npc_buy_cost`；规范 key 表见 `10-key-glossary.md` §6）。
- 提供方注册「带来源的贡献」：`{ key, stage, value, order, scope?, stack?, when?, source: { kind, key } }`；`value` 是整数或 `{ num, den }`，加 / 乘由 `stage` 决定（`10-key-glossary.md` §6.1）。来源包括装备、舱室、身心状态、船首像、剧本旗标、岗位、势力加成、事件 buff。
- 消费方只按 key 取值，不知道贡献来自哪里。
- 堆叠阶段固定：`base → add → mul → clamp → override`。阶段内按 `order` 再按 `source` 排序，结果确定、可追溯（UI 可列出每条贡献）。
- 每阶段的整数语义由 key 声明（D3 的 `int_mode`）。原版取整靠阶段顺序复刻。
- 业务公式输入单位是**技能值**（0–500，`skill_value_cap`），不是六维（0–100，`ability_cap`）。
- 「岗位持有人」`Fleets.post_holder(fleet_id, post_key)` 与「舱室计数」`Ships.cabin_count(ship_id, role_key)` 是一级查询。
- 随机项不进修正器；属于消费方公式（D3）。

### 原版依据

- 技能值链就是这条阶段序 `[确认]`：`fn_14000e1ce(person, eSkill)`（body `0x1401c9f60`）→ `fn_1400135cf`（`0x1401ca250`）：技能基值 `fn_140009214`（`0x1401c9d50`，`(A+B)*(min(L,200)+100)/150`，配对表 `.data 0x1406864a0`）+ 5 格装备 `fn_14000e0b6` → 身体态系数 `fn_1401c87a0`（1–2 ×19/20、3 ÷2、4 ÷10、5 ×3/5，整数截断）→ `fn_1401cbb40` clamp `[0,500]`。第二参是 `eSkill`：`fn_1400099e9`（`0x1401ca010`）断言 `k<10`，`fn_1401cb7f0.cpp` 调用处 RTC 局部名 `eSkill`。
- 航速链：船首像 0 `s += s/10`（`0x1401b088f mov ecx,0xA`）→ 船首像 7 `s = s*4/5`（`0x1401b08bc`）→ 海战态 `s /= 3`（`0x1401b08fe`），且 `/3` 在写速度条之后。顺序一变，取整就变。
- 反例 1：战斗、接舷、投降、装填各处写死「玩家本舰队取岗位 N，否则取提督 / 船长」三分支（`07-combat/01`、`02`、`04`）。
- 反例 2：舱室种类字面量 7 / 8 / 9 / 10 / 11 散在货格、炮门、水手、换炮价、战利品（`fn_140006b95(ship, kind)` 的各调用点）。
- 反例 3：船首像 id 比较 `CShip+0x53` ∈ {0,1,5,7,8} 分散在航速、补给、了望、战斗。

### 约束

- 经济、战斗、航海、道具不读六维原值，也不自己套技能公式；要的数值先登记为修正器 key。
- 提供方只注册贡献，不调用消费方；消费方缓存按失效事件清（`PersonEquipped`、`CabinBound`、`VitalsChanged`、`LevelChanged`、`FigureheadChanged`、`ScenarioSelected`）。
- 新增来源只加内容表行，不改代码。

### 落点

- `config/modifiers.yaml`：key 注册（`key`、`type: int|factor|bool`、`int_mode`、`clamp`、`owner_system`）与阶段序
- 贡献在各内容表：`items/effects.yaml`、`ships/cabins.yaml`、`characters/vitals.yaml`、`ships/figureheads.yaml`、`narrative/scenarios.yaml`
- `03-decoupling-principles.md` 原则 6；`06-system-interfaces.md`「修正器」

---

## D3 公式层：四档 + 整数语义 + 对拍

### 决策

| 档 | 形式 | 用于 |
| --- | --- | --- |
| ① | `params` / `table` / `cdf` / `piecewise` | 标量、分档、分段系数 |
| ② | `impl: expr` 沙箱整数表达式：显式 `/`（截断）、`udiv`、`min/max/clamp`、查表、`rand(n)`（命名流） | 单式公式 |
| ③ | `impl: vanilla.<name>` 命名原版实现，或 `impl: <pack>.<name>` 插件 | 控制流重的公式 |
| ④ | `modifiers` 出口（D2） | 跨系统取值 |

- 每个公式声明 `int_mode: { div: trunc | floor, width: wrap32 | int64 | sat64 }`；预设 `vanilla_i32 = { div: trunc, width: wrap32 }`。
- 每个公式声明 `quirks`（原版怪癖开关，显式保留或关闭）：如 `unsigned_div`、`u8_truncate`、`level_q1_zero`。
- 每个公式带 `golden` 用例，加载期自检；原版包的 `expr` 版必须与 `vanilla.*` 逐位一致。
- 通用脚本只在叙事事件边界（D8），不进公式。
- 代码里不得有 `== 字面量 id` 判断（船首像、岗位、技能、舱室种类）；改成内容表 `effects` / `role` / `refs`。

### 原版依据

- 无符号除法：NPC 买货 `(1000 - a)` 后 `div 0x3e8` 是 32 位无符号（`05-ships-fleet/02`）；会计长杀价、买价都有 `div`。若 caps 把技能上限调过 1000，`1000-a` 为负会变巨大正数。
- 等级怪癖：`fn_140012085`（`0x1401c92b0`）`isqrt_fx(exp/30)/256`，`isqrt_fx(q)` 在 `q ≤ 1` 时原样返回 → exp 30–59 为 0 级；`q<<16` 32 位回绕。
- u8 截断：初始水手 `(u8)((min+max)/2)`（`0x1402ae0d6..0x1402ae13f`）。
- CDF 分档：天气 `rand(100)` 落在 `<30,<50,<70,<80,<87,<96`；d20 风档阈值 4/7/10/13/16/18（`fn_1402abb20`）。
- 反编译伪影：`cdq; sub; sar` 前的寄存器残留被当成公式常数（计略对抗式多出的「−8」、半速键 `0x11`、探索 `-5`）。关键立即数必须回原始指令核对。

### 约束

- 公式文件只放算法与参数，不放内容实例，不比较剧本号（D5）。
- caps 只放上限；同一参数只有一个来源，引用用 `$caps.<key>`。

### 落点

- `14-config-schemas/README.md`「公式层」；`13-algorithms/01-formulas.md`；`03-decoupling-principles.md` 原则 3

---

## D4 时间与顺序：统一 Clock + 配置化阶段

### 决策

- 原版 1 天 = 48 tick（`0x30`），进 `caps.calendar_ticks_per_day`。
- 引擎统一 `Clock`；系统按 `schedule: tick | day_start | month_start | month_end | year_start` 订阅。
- 世界 tick 的顺序与日历门写进 `config/engine/tick.yaml` 的阶段序列。同一日历点上多系统的先后写进配置，不靠订阅注册顺序。原版实测：月初先市价漂移，再逐港「投资转化 → 补货」（同一循环交错）；独占上缴在月末最后一拍，出租费在其前的 `items` 阶段（`CItemElement::vfunc_04`）；星品在年初（晚于当月补货）。
- 阶段内支持交错粒度 `per: town | phase`：原版「逐港交错」是怪癖，后序港当月转化解锁的货，前序港当月不补。
- 玩家舰队日步（`tick.yaml` 的 `daily` 阶段，旧称 `DailyTick`，不是事件）在港 / 海上都跑：在港分支只恢复疲劳（旅馆每夜快进同样触发）；海上分支按原版 `fn_140005a15` 调用序：补给日耗 → 舰队疲劳 → 人物 HP / 身心 → 海上事件掷骰 → 事件精神转移 → 持续态（调用点 `0x1400fa8e0`…`0x1400fb879`；人物 HP 读的是掷骰前、前一日的事件态）。在岗经验 `duty_exp` 每 3 天（`fn_140009b24` = `packed % 0x90`）、仅在海上结算，不是月结。顺序写进 `tick.yaml`，可改。
- 阶段可声明「读日初快照」，以便改顺序时仍能复刻原版取值。
- 禁止读 `SCalendar+0x1c` 之类字段；只用 time 事件 `DayStarted` / `MonthStarted` / `MonthEnding` / `YearStarted`。
- 「过 n 天」走 `Clock.advance_days(n)`：原版是逐拍跑完整世界调度，重构保留 `add_days_mode: simulate | jump` 开关。

### 原版依据

- 时间推进 `fn_140012049`（body `0x1402a7780`）：`CWorld+0x10 += 1`，再调世界 tick `fn_14000277f`（body `0x1402aad80`），再调玩家舰队 `fn_140011414`（`0x1400ec820`）。
- 世界 tick 按固定顺序、按门函数调 10 张表（`02-original-architecture.md`）。门：日初 `fn_14000906b`（`+0x14==0`）、月末最后一拍 `fn_14000f8b7`（`+0x1c==2`）、月初 `fn_14000e9da`（`+0x1c==0`）、半年 `fn_14001374b`、年初 `fn_140014dd0`（`+0x1c==0 && month%12==0`）。
- 门分两层：世界 tick 一层，函数体内又一层（`CTownElement::vfunc_04` 在同一函数里做市价漂移、逐港投资转化 `0x1402679cf` → 补货 `0x1402679f8`、月末独占上缴 `0x140267a0a`、状态切换）。
- 过日：`fn_14000b122` 跑 48·n 次完整调度；旅馆每晚 `0x30` tick；交易提交、修理各 1 天。

### 约束

- 系统不自建计时器，不读日历内部字段。
- 每个阶段结束是事件派发边界（原则 5）。改阶段顺序会改数值，`tick.yaml` 属规则包版本。

### 落点

- `05-config-architecture.md`「tick 顺序」；`01-world/07-calendar-and-coords.md`；`06-system-interfaces.md` 时间事件

---

## D5 剧本差异：旗标与修正器，不比较剧本号

### 决策

- 引擎代码和公式文件里不得比较剧本号。
- 剧本记录（`narrative/scenarios.yaml`）带 `flags: { <key>: bool|int }` 与 `modifiers: [贡献]`。叙事在 `ScenarioSelected` 时把贡献注入修正器注册表（来源 `scenario:<key>`），并开放 `Scenario.flag(key)` 查询。
- 公式里需要开关时写 `enabled_by: scenario_flag.<key>`。
- 剧本专属内容表（据点港、里程碑行、开局揭示港、主角势力）挂在剧本记录下，不挂在系统公式里。

### 原版依据

剧本号 = `DAT_144296500+0x7c`（`eScenario`；`fn_140009df9` 剧本→势力表 `{0,2,1,3,20,13,21}`，`fn_14000864d` `>=4` 选 World1）。已发现的分支：

| 剧本 | 差异 | 证据 |
| --- | --- | --- |
| 0 | 委托报酬 ×1.5；军事投资八折；遗迹首访影响力 ×2 | `fn_140004a1b()==0`（`02-factions/04` §2.6）；`04-economy/07`；`fn_140010f46()==0`（`01-world/09`、`11`） |
| 2 | 卖价 ×105/100；卖流行品影响力 ×2 | `fn_140010f46()==2`，`fn_140001b04`（body `0x1402758a0`） |
| 6 | 玩家势力每月 `gold/100` 交宗主 | `CForceElement::vfunc_04` `0x14010dde8` |
| 各剧本 | 据点港表、里程碑行（`0x1405a0490` 行号 = 剧本号，`>=4` 取 2）、开局揭示港 | `fn_14000d6c0`（`0x1402645a0`）；`fn_140008fbc`；`04-economy/08` |

### 约束

- 经济、势力、城镇只读修正器 key 或 `scenario_flag.*`；校验器拦截公式里的 `scenario_id` 字样。
- 布尔开关走 `flags`（如剧本 6 宗主上缴）；乘除差异走修正器：`request.reward_gold`、`invest.military_min_fee_pct`、`trade.sell_price_pct`、`factions.influence_gain`（`ctx.source` 区分流行品 / 遗迹首访）。同一倍数只乘一次，公式里不得再乘。

### 落点

- `10-narrative/01-scenarios.md`；`04-economy/02`、`07`；`02-factions/03`

---

## D6 派生量：唯一计算方，只读快照，不存档

### 决策

| 派生量 | 唯一计算方 | 输入 |
| --- | --- | --- |
| 必要 / 上限水手、帆值、炮门数、估价 `outfit_value`、可用货格 | Ships | 船模、桅、改装件、舱室（`ships/rig.yaml`、`cabins.yaml`） |
| 技能值、HP 上限 | Characters | 六维、经验、修正器 |
| 航速、搜索半径 | Ocean | Ships 快照、`Climate`、修正器 |
| 航线可见 / 可走 | World（routes） | 港口已知位、份额谓词 |
| 港口所属七大海域 | Factions | town → culture → major_sea（定义表） |
| 港口景气状态价格系数 | Economy | 港口状态 key |

- 消费方只读快照；派生量不存档，读档后重算。缓存按事件失效。

### 原版依据

- 必要水手 `fn_14000a466` 被航海与战斗各自调用；`crew_min/max` 现算（`0x14023f370`、`0x14023f1f0`），不是船体表字段。
- 帆值 / 必要人手只看旗舰、船首像看全队（`06-ocean/01`）。
- 航线显示 = 两端港已知位都真（`fn_14001389f`），原版无「已建立航线」存档位。
- 港口归属 7 区：`fn_140014e48` = town `+0x25` → `CCulture` → `+0x10`。

### 落点

- `06-system-interfaces.md`「派生量所有权」；`05-ships-fleet/01`、`06`；`06-ocean/01`

---

## D7 存档：分节、按 key、独立 profile、RNG 进档

### 决策

- 分节，每节自带版本，按节注册迁移链（`vN → vN+1` 纯函数，不跳步）。
- 列表一律按 id / key 存，不按下标；跨系统引用由 Save 层统一 remap。
- 模组状态在 `mod:<id>` 节原样保留（模组未启用也不丢）。
- 跨存档数据是独立 `profile`（原版 `savedata.dk4`：通关位、画廊点数、博物馆 / 图鉴位）。
- 存档头：启用包列表 + 版本、`id_map`、RNG 主种子与各流状态。
- 随机数：重构用分系统命名流（`config/engine/rng.yaml`），种子与流状态进存档；`vanilla_msvc` 单流仅供对拍。

### 原版依据

- 原版正文是无标签字段流，头 `+0x10` 格式版本须为 0，`+0x24` 锁剧本 EV 版本（四语言 EV 版本不同，跨语言档被拒）；列表扫描 52 个文件、选择框 50 条（`fn_14001300c`，body `0x140113640`）。
- 随机数：唯一玩法随机源 `fn_140005272(n)`（body `0x140140a70`，525 调用点）= `n>0 ? rand()%n : 0`，首次调用 `srand(time(NULL))`，全 exe 一处 `srand`，不进存档 → 原版不可重现。
- `savedata.dk4` 是跨存档系统档，不是会话（`12-ui-save/02`）。
- 存档人物段 `[确认]`：从 `0x2C03` 起按 `ePerson` 0–234 共 235 条 `CPerson` 记录，id < 87（`0x57`）每条 25 字节、其余 2 字节，`0x35AA` 接主角 `CPlayerPerson`（`CPersonElement::vfunc_01` body `0x1401cc3e0`；`CPerson::vfunc_06` `cmp eax,0x57`）。「98 行名册、步长 `0x19` 从 `0x2C00`」是 dk4save 工具常数，行 87–97 会写坏别人的隶属字节。见 `03-characters/01-person-model.md`、`12-ui-save/02-save-session.md`。

### 落点

- `12-ui-save/02-save-session.md`；`05-config-architecture.md`「与存档的关系」「随机数」

---

## D8 叙事：数据触发器 + 结构化命令树

### 决策

- 规范形式 = 数据触发器（事件源 + 条件 + `on_fire{one_shot, relatch}`）+ 结构化命令树（if / menu / 命令）。原版 EV 字节码只作导入格式（`narrative/opcodes.yaml` 只给提取器用）。
- 触发源是命名事件，不是整数 kind；原版 kind 0–8 只在导入映射里出现。
- 终局统一 `Session.end_scenario(outcome: cleared | game_over | to_title)`。
- 小游戏统一 `Minigames.run(id) -> int`，结果回写脚本谓词。
- 模组的算法需求走沙箱 `call_script`，只能用叙事接口的查询 / 命令。

### 原版依据

- EV ISA 37 槽、只向前跳、谓词 0/1/2 分支、无循环无变量 → 可无损反编译为 if / menu 树（`10-narrative/03`）。
- kind 0–8 `[确认]`：探索发现道具 / 海战开始 / 海战结束 / 舰队接触 / 入港 / 港内 / 航海 / 进设施 / 登陆村落（`fn_14001337c` 8 个调用点）。
- execute id `0x3FFF` → `CDk4Game` 流程态 3、参 1（剧情 Game Over，`0x1400c39a1`）；opcode `0xC5` imm 0 通关 / 1 Game Over / 其它回标题（`0x1400e1d80`）；opcode `0xC1` 跑 7 种小游戏（内层跳表 `0x1400e1f18`）。

### 落点

- `10-narrative/02-event-system.md`、`03-event-commands.md`；`06-system-interfaces.md`「叙事触发源映射」

---

## D9 动作执行器：town / 海上 happening / 叙事共用

### 决策

```yaml
Action:
  cost:   { gold: <formula|int>, days: <int> }
  guard:  [ <predicate> ]           # 日期窗口、岗位技能门、金币、旗标
  roll:   <formula_ref>             # 可空；用命名 RNG 流
  effects: [ <command> ]            # 发给状态所有者的命令
  repeat: { n: <int|formula>, interruptible: <bool> }
```

- 设施 = 地图点 kind → 处理器 + 参数。一个处理器可服务多个 kind（王宫 / 总督府、遗址 / 教会 / 城门），靠参数区分。
- 每港有哪些地图点进港口内容表（`town/town_map_points.yaml`），不用全局「全开」。

### 原版依据

- 地图点跳表 `0x14026d92c`（`CTownScene::vfunc_02`，body `0x14026d160`）：9 / 10 / 11 都构造 `CMpRemains`；6 / 7 都进 `CPalace`。
- 旅馆 `fn_14000b8e3`（body `0x140160270`）：单晚 / 长住 3·10·30 晚；每晚 `fn_14000af74` 扣 1 金、走 1 天、按概率恢复；长住可被事件打断 = `repeat{n, interruptible}`。
- 仪式日 `fn_14000eacf`：`(town_id+1)%28+1` 日期窗 = `guard`；奖励 `fn_14000a8ee` = `effects`。
- 广场寻求支持：岗位技能门 + `rand%1000` 掷骰 + 走 1 天（`06-plaza-intel-adman.md` §4）。
- 海上 happening：20 槽 CDF + 结局态（`06-ocean/05`）。

### 落点

- `08-town-life/01-facilities.md`、`05-inn-church-palace.md`；`06-ocean/05-sea-happenings.md`；`10-narrative/03`

---

## D10 内容提取：按段换算、按所有权拆表、消息号作文案 key

### 决策

- 地址换文件偏移**按 PE 段**：

| 段 | VA 起点 | 文件偏移 |
| --- | --- | --- |
| `.text` | `0x140001000` | VA − `0x140000A00` |
| `.rdata` | `0x140521000` | VA − `0x140001200` |
| `.data` | `0x140682000` | VA − `0x140001600`（只有前 `0x12400` 字节在文件里） |

- exe `.rdata` / `.data` 内容表用按段映射的导出脚本生成 `packs/vanilla-core/`，不手抄。已识别的表：船体定义 107 条（`0x1405b42e0`）、船模 39 条、炮种（`.data 0x140682e10`）、舱室种类 24 行（静态构造 `0x1400256d0`）、岗位名 16（`.rdata 0x1405867f0`）、文化圈 21×`0xE0`（`.rdata 0x1405b6160`）、技能配对（`.data 0x1406864a0`）、交易品目录 105×16（`.data 0x140688190`）、村落 24×`0x38`（`.data 0x140687b40`）、航线 200×6（`.data 0x140687070`）、折点 164（`.data 0x140682f50`）、风场 2×25×50（`0x140533890`）、星座 12（`0x140688970`）、消息号表。
- 一条 DLL 港口记录按所有权拆到 world / town-life / factions / economy 四张初值表。同一行跨系统时：一个系统拥有 id 与外键，另一个拥有数值列（文化圈行归 factions，105 卖价列归 economy `culture_prices.yaml`）。
- 文案 key 用 exe 消息号表（`.data 0x140683620` 37 块 `{first_id, size}` + `0x1406836c0` 每条 u16 偏移），不用 mesfile 字节偏移。
- 关系 / 友好度是无序对表（`a<b`）；原版只读半矩阵。
- 统一位置引用 `LocatorRef(kind, id)`；港口、村落、折点、舰队、道具共用。

### 原版依据 / 反例

- 反例：旧稿把 `.rdata` VA 按 `.text` 差值换算，读错位置，得出「SC 无 21 文化圈名表」「`0xE8` / `0xE0` 两套步长」两条错误结论。实为同一张表：`CCultureElement` 运行时 `0xE8` = 8 字节 vptr + `0xE0`（`fn_140010f73` body `0x1402ac510` `imul 0xE0; lea 0x1405b6160`）。
- 半矩阵：`fn_140008f4e` / `fn_140012dc8` 在 `self.id < other` 时读本行，否则交换；World 表下三角原版不读。
- Locator：`fn_14000cf90` `kind<<24 | index`，各 kind 有上界，越界记 −1。
- 港口 0x84 记录：`+0x25` culture_id（world 外键 → factions 定义）、`+0x26` remains_id、`+0x28` 特产品槽（economy）、`+0x48` 地图点位域（town-life）、`+0x4C` 3×(force, share)（factions）、`+0x52` 2×u16 发展度初值、`+0x7E` 2 格流行（每格两字节：u8 trading_item + u8 等级 `[确认]`，原版按 word 拷贝；初值 `(0x6A, 0)`）。见 `15-catalogs/towns.md`「记录布局」。

### 落点

- `_meta/source-map.md`「地址与文件偏移」；`05-config-architecture.md`「生成管线」；`12-ui-save/03-localization.md`

---

## 已确认的跨文档事实（各篇必须一致）

| 事实 | 出处 |
| --- | --- |
| 港口特产品槽：商品 id u8；`+2` = 商业上架门槛（发展度 ≥ 门槛才出现）；`+4` = u8 基础供给；配额 = 基数 × (商业/50+100)/100；槽底价在城镇记录 `+0x60` 段（伦敦煤 550） | `04-economy/04`、`15-catalogs/towns.md` |
| 港口 `+0x25` = culture_id；`+0x26` = 遗迹 id（26 = 无）；21×`0xE0` 文化表（名字、major_sea_id、105 卖价）在 SC VA `0x1405b6160` | `02-factions/06` |
| 技能→两能力配对表 `.data 0x1406864a0`；说服力 = 魅力 + 智力 | `03-characters/03` |
| 六维上限 100；技能值上限 500；单段等级 `min(isqrt(exp/30),100)`（16.16 定点，exp 30–59 为 0 级）；两段和在技能式里封 200 | `11-progression/01` |
| 身体态 7 种（健康 疲劳 轻伤 重伤 濒死 疾病 死亡），精神态 6 种；HP / 身体 / 精神进存档 | `03-characters/07` |
| `World.dat` = 2500×1250 uint16 LE 贴图编号；航行坐标 `/16`（向零截断）→ chip 下标 | `01-world/01`、`06` |
| 航速除数：船首像 0 `+s/10`、船首像 7 `×4/5`、海战 `/3`；第四个 `idiv` 除以 `2×必要水手` | `06-ocean/01` |
| 地图点 kind 9/10/11（遗址 / 教会 / 城门）共用 `CMpRemains`；教会是按日期的仪式，不是设施 | `08-town-life/05` |
| 原版没有名声与爵位，没有继承；Game Over 回标题 | `11-progression/02`、`03` |
| 存档目录 `zh-CN zh-TW ja ko`；槽扫描 52 / 列 50 | `12-ui-save/02` |
| 人物 203–210 中 7 个姓氏乱码 `놣/늣/뎣` 实为全角 `１/２/３` | `15-catalogs/persons.md` |
| 存档人物段：`0x2C03` 起 235 条，id < 87 每条 25 字节、其余 2 字节；98 行名册是 dk4save 工具常数 | `03-characters/01`、`12-ui-save/02` |
