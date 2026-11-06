# 疲劳、健康与精神

具名航海士的 HP、身体状态（含疲劳）、精神状态及其每日转移。无名水手的数值疲劳是舰队字节 `CFleet+0x71`，归舰队（本系统只在休养时发命令）。旅馆只报价；船医 / 舰长 / 传教士只提供技能值进掷骰；海战只报告谁受伤、掉多少 HP。主角进入「死亡」之后交给 `11-progression/03-gameover-succession.md`，本系统不播 GameOver 图。

## 1. 职责边界

本系统管：

- `ePhysicalState`（7 态）：健康 / 疲劳 / 轻伤 / 重伤 / 濒死 / 疾病 / 死亡
- `eMentalState`（6 态）：高亢 / 精神 / 轻松 / 普通 / 不高兴 / 混乱
- 当前 HP（存档字段）与 HP 上限（派生 = 生命力基础值）
- 每日 HP 增减、身体状态恶化 / 恢复、精神状态漂移 / 事件转移
- 身心对能力 / 技能的乘数（定义在本系统状态表，技能公式在 `03` 消费）
- `PHYSICALLEVEL` / `MENTALLEVEL`（World DLL 断言符号；与上面枚举的关系未还原）
- 坏血病 / 传染病落到人物状态上的**应用**
- 把旅馆、舱室、船医、道具对**人物**的恢复 / 抗病折成规则；旅馆对水手疲劳的减量作为命令转给舰队
- 主角或在队人物进入「死亡」时发事件，不解释继承

不管：

| 事项 | 归属 |
| --- | --- |
| 旅馆房价、住几晚的 UI | town-life `01-facilities.md`（`CInn` 只发 `Actions.run(action_key, ctx)`，动作内调 `Characters.apply_rest`） |
| 教会仪式规则 | town-life `03-minigames.md`；休息仍走本系统 |
| 舱室几何、医务室 / 厨房 / 娱乐室坐标 | ships `03-cabins-crew-posts.md` |
| 海上每 tick 何时涨疲劳、坏血病掷骰 | ocean `01-sailing.md` / `02-search-encounters.md`；本系统收事件 |
| 接舷伤害公式、单挑 | combat；本系统收 `ApplyInjury` |
| 水手**人数**、哗变武力镇压 | ships / ocean；不满计量若独立，见第 9 节 |
| 舰队水手疲劳 `CFleet+0x71`（0–100）的状态与读写 | fleets（`Fleets.fatigue` / `Fleets.add_fatigue`）；ocean 探索 / 温泉、combat 接舷、本系统旅馆都只发增量 |
| 生命力技能怎么从六维算 | `03-abilities-skills-jobs.md`；本系统只读 `skill.vitality` |
| 寿终日历、继承、GameOver still | `11-progression/03-gameover-succession.md` |
| 文案字符串 | ui / localization |

人物对外数值只经修正器注册表（D2）：身体 / 精神状态登记 `skill.*` / `ability.*` 的 `mul` 贡献。禁止业务系统读 `ePhysicalState` 下标。

## 2. 原版事实

### 2.1 符号与断言

`[确认]` World DLL 断言（`srcdata/catalog/asserts.txt`）：

| 符号 | 断言 | 宽度 |
| --- | --- | --- |
| `eData` / `idData` / `eVal` ∈ `eMentalState` | `0 <= x < NUM_MENTALSTATE` 或 `INVALID_MENTALSTATE` | `MENTALSTATE_ID` |
| 同上 `ePhysicalState` | `0 <= x < NUM_PHYSICALSTATE` 或 `INVALID_PHYSICALSTATE` | `PHYSICALSTATE_ID` |
| `FATIGUE_MIN <= data <= FATIGUE_MAX` | `FATIGUE_MAX <= sizeof(UCHAR)` | UCHAR |
| `MENTALLEVEL_MIN <= data <= MENTALLEVEL_MAX` | MAX 约束在 UCHAR | UCHAR |
| `PHYSICALLEVEL_MIN <= data <= PHYSICALLEVEL_MAX` | MAX 约束在 UCHAR | UCHAR |

`NUM_MENTALSTATE` / `NUM_PHYSICALSTATE` / 四个 MIN/MAX 的整数均未写进头文件（`MentalstateID.h` `PhysicalstateID.h` 只有 `NUM_*` 与 `INVALID_*=-1`）。`constants.json` 有符号无名。两个 `NUM_*` 的值由 2.2 的名称表钉为 6 / 7。

### 2.2 状态名指针表

`[确认]` `.data` 里紧挨能力名表（`0x140686350`）、技能名表（`0x140686390`）的两张指针表：

| 表 | 项 | 内容（下标即枚举值） |
| --- | --- | --- |
| `0x140686410` | 7 | 0 健康、**1 疲劳**（字串 `0x14057e9e8`）、2 轻伤、3 重伤、4 濒死、5 疾病、6 死亡 |
| `0x140686458` | 6 | 0 高亢、**1 精神**（复用能力名字串）、2 轻松、**3 普通**（`0x140550bbc`）、4 不高兴、5 混乱 |

文件偏移 `0x57eb78` 起的连续 8 字节字串只有 6 + 4 条，是因为疲劳、精神、普通复用了别处字串；旧版本据此推出「6 / 4 态、疲劳不是身体状态」是错的。枚举值由下列代码钉死：`CPerson+0x0D` 初值 0、`vfunc_04` 以 `== 6` 判死亡；`CPerson+0x0E` 初值 3；每日函数的 `switch` 用到 1–5 / 0–5（见 2.11）。

`[确认]` 大地图 HUD 把疲劳画成数字：同文件 `0x57d7dc`「水手」、`0x57d7e4`「`%4d`」、`0x57d7e8`「疲劳」。情报面板 RTC `rcTiredTitle` / `posTired`（`fn_140005f51`，`fn_1402645a0.cpp`）是这块字的排版，不是公式。

### 2.3 存档：HP、身体、精神都存

`[确认]` `CPerson::vfunc_06/07` 串行化 `+0x20`（`uint16` 当前 HP）、`+0x0D`（身体）、`+0x0E`（精神），ids 0–86 与主角 `CPlayerPerson` 都写。存档样本：`0x2C03 + id×25` 记录的 `+0x10` HP、`+0x12` 身体、`+0x13` 精神；主角在 `+0x35BA` / `+0x35BC` / `+0x35BD`。拉斐尔 HP 79 = `(59+58)×(1+1+100)/150`，正是生命力基础值。见 `01-person-model.md`。

`Person.cpp` / `Save.cpp` 注释「HP / mood are computed… not stored」「健康 / 心情不存」是旧还原的误判：存的是当前 HP 与两个状态枚举，派生的只有 **HP 上限**和技能。ids 87–234 不存这些字段，读档后回到世界表初值（HP 上限、健康、普通）。

### 2.3a HP

- `[确认]` HP 上限 `fn_14000cf22 = fn_14000b307(p, 0) = fn_140009214(p, skill=0, mental=0)`：生命力**基础值**（体力 + 精神，等级项同技能公式，封 0–500），**不**计精神修正、装备、身体系数。
- `[确认]` 改 HP `fn_140002996(p, d)`：`hp = clamp(0, hp + d, hp_max)`，经 setter `fn_1400141ff`。
- `[确认]` setter `fn_1400141ff`：旧值 > 0 且新值 ≤ 0 时，身体状态置 **4 濒死**（`fn_14000ab0f(p,4)`），并给段 0 经验 400（`fn_1400026d5(p, 0x190, 0)`）。HP 本身不触发「死亡」。
- `[确认]` 升级后若 HP 上限变大，差值加到当前 HP（`fn_1400065fa` 尾部）。

### 2.4 疲劳文案

SC `mesfile_sc.txt`：

| 偏移 | 句 | 用法 |
| --- | --- | --- |
| `0x343f` | 「疲劳度上升了%s。」「疲劳度恢复了%s。」 | 与温泉经验、拖船同一长块 |
| `0x81cb` | 「水手们好像很疲劳，不让他们休息一下吗？」 | 码头补给 |
| `0xc187` | 「如果有大厨，能节约水和粮食，水手也减轻疲劳。」 | 厨房岗位 |
| `0xc187` | 「船员进到这里能恢复精神，水手也能减少疲劳。」 | 娱乐室 |
| `0x10c9b`–`0x10ceb` | 温泉「解除旅途的疲劳」 | 探索 |
| `0x1dd13` | 某船首像「消除疲劳」 | 道具 |
| `0xcf63`–`0xd0c7` | 「太累了……在“娱乐室”里……。」「过于疲劳了吧？」 | 航海士台词 |

`[推断，强]` HUD `%4d疲劳` 跟在「水手」后面，是**舰队水手**的疲劳计量 = 舰队字节 `+0x71`：setter `fn_140009467(fleet, d)`（body `0x1400ea820`）`+0x71 = clamp(+0x71 + d, 0, 100)` `[确认]`，getter `fn_14000ee12`（body `0x1400c52e0`）读同一字节，情报面板 `posTired` 画它。写入方：探索 / 温泉 / 鲨鱼（`06-ocean/02`，播 `0x147/0x148`）、旅馆（2.5）。读取方：划桨门 `< 50`、哗变门 `>= 50`（且出港 ≥ 20 天）（`06-ocean`）、接舷伤害 `(200 − 疲劳)`（`07-combat/02`）。`FATIGUE_MIN/MAX` 断言应属它，值 `[推断]` 0 / 100。具名航海士的「疲劳」是身体状态 1（由低 HP 触发、HP 回到 95% 以上解除，见 2.11），不是数值。

### 2.5 旅馆 `CInn`

`[确认]` `CInn` 还原 cpp 只有 ctor / dtor，写 vftable，无菜单逻辑。来源：`srcdata/source/game/town/CInn.cpp`（body `0x14015ea70` / `0x14015eaa0` / dtor `0x14015eac0`）。玩法在 `CTownScene` 宿主里（`CTownScene::vfunc_02` RTC 含 `cInn`）。

`[确认]` 资费文案：`0xba97`「要１枚金币。好的，您请休息吧。」`0xbab7`「住１晚１枚金币，要住几天？」`0xbad3`「住%s天吗？」。单价与天数属 town-life；恢复效果属本系统。

`[确认]` `0xc0b3`「在这里船员身体和精神都能够恢复。」同一长块后面才是船木工 / 船医说明，所以这句话是**舱室**（单人房）说明，不是旅馆句。单人房的人物侧效果见 2.11（HP +2、计入 `rest`）。

`[确认]` 旅馆规则（`08-town-life/05-inn-church-palace.md` §2.2，入口 `fn_14000b8e3`）：菜单「住一晚 / 长住 3·10·30 晚 / 离开」。每晚 `fn_14000af74`（body `0x140160ae0`）依次：人物休养 `fn_1400044cb(fleet, flag)`（2.12）→ 水手疲劳 `fn_14000efde(fleet, flag)`（body `0x140160520`）→ 走 1 天、扣 1 金。`flag = 1` 住一晚、`flag = 0` 长住。水手疲劳减量：`flag ? rand(3)+4 : rand(5)+5`，再 `min(减量, 当前)`，经 `fn_1400051c8` 写回。

教会**不**提供休息：「安安静静地祈祷」（`0xc01f`）是按日期的仪式台词（`08-town-life/05` §2.3），旧稿「教会可当休息点」作废。

### 2.6 船医、大厨、舱室

`[确认]` 岗位名 exe `0x58568c` 含「船医」「大厨」。舱室名 exe `0x52dfbc` 含「医务室」「厨房」「娱乐室」「单人房」。

| 岗位 / 舱 | 文案保证 | 来源 |
| --- | --- | --- |
| 船医 | 「当船员和水手的身体有病时能够治病」 | `0xc0b3` |
| 船医 | 「要是受伤了，我来缠绷带」「打起仗来，船医特别忙」 | `0x18ecb` `0x18077` |
| 大厨 | 节水粮，水手减疲劳 | `0xc187` |
| 单人房类 | 船员身体和精神都能恢复 | `0xc0b3` |
| 娱乐室 | 船员恢复精神，水手减少疲劳 | `0xc187` |
| 传教士 | 消除水手的不满 | `0xc1fb` |

船医、单人房、娱乐室的人物侧幅度已还原（2.11）。大厨节水粮 = 日耗里 `rand(500) <` 大厨命中力（`fn_14000da26(fleet, 12, 6)`）则除数 +2（`06-ocean/01` §日耗）；大厨「减轻疲劳」= 舰队疲劳日增量里 `rand(500) < 船医观察力/2 + 大厨命中力/2` 则 `d -= 1` `[确认]`（`06-ocean/01` §2「舰队疲劳日增量」，状态归 fleets）。

### 2.7 战斗受伤

`[确认]` 航海士伤台词 `0xd0f3`–`0xd26b`：「这点儿伤……没关系」「血止不住了」「由于这伤，力气……。」濒死 / 死亡 `0xd453`–`0xd5bf`：「身体不能动了……我不行了吗？」「我要死了吗？」

`[确认]` 接舷公式里有 `Ability(*, 5) * (200 - fn_14000ee12)`（`07-combat/02-boarding.md`）；这里的 `Ability` 是 `fn_14000e1ce`，5 = **剑术**（技能），不是能力「运气」。`fn_14000ee12` 读舰队 `+0x71`，即 2.4 的舰队水手疲劳 `[推断，强]`；接舷伤害随疲劳线性下降。

`[确认]` 战斗区代码写身体状态的调用点：`fn_1400108a2`（RTC `aePerson`）以 2 / 3（轻伤 / 重伤）、`fn_1401bdaa0` 以 2 / 1（轻伤 / 疲劳）调 `fn_14000ab0f`；触发条件未读。HP 归零经 setter 自动变濒死（2.3a）。

海战水手人数打到 0 是「水手全被杀死了，提督逃到其他船」（`0xec53`），不是人物「死亡」枚举。旗舰进水 / 俘船属 combat。

### 2.8 海上疾病

`[确认]` 遭遇文案（ocean 掷骰，本系统应用）：

| 事件 | 文案 | 效果（文案能看见的） |
| --- | --- | --- |
| 坏血病 | `0x3c2b`–`0x3d6f`「水手们好像染上了坏血病！不赶快靠岸，大家的身体……。」 | 需入港；「全都完了」 |
| 传染病 | `0x481f`–`0x48e3`「发生了传染病！不赶快靠岸就会全死光的！」 | 需入港 |
| 瘟疫（人物） | `0xd29b`「好像染上了什么瘟疫。身体发烧……。」及感冒 / 流行病变体 `0xd2bf`–`0xd3e3` | 身体状态「疾病」`[推断]` |

`[确认]` 生命力说明：「ＨＰ最大值，对疾病等的抵抗力。」（技能 id 0，exe `0x5910c4`）。抗病曲线未还原。

`[确认]` 抗病 / 治疗道具（效果进 items，本系统读修正器）：

| id | 名 | 文案 |
| --- | --- | --- |
| 21 | 莱姆的滴汁 | 「航海中可以为海员治疗坏血病」（`0x19c6f`）；「为防备航海时出现坏血病」（`0x1d08f`） |
| 23 | 华陀的中药 | voyage_charm |
| 99 | 赫洛菲洛斯的医书 | 岗位用具分组 14 `[推断]` |
| 104 | 带纱布的口罩 | 分组 17 `[推断]` |
| 123 | 圣母像 | 「抵御瘟疫」（`04-remodel-guns-supplies.md`） |
| — | 某船首像 | 「好像不会得病」（`0x1dd83`） |

莱姆是消耗护佑，圣母像是船首像，不进人物槽。

### 2.9 死亡与 GameOver

`[确认]` 身体枚举最后一项是「死亡」。人物台词有临终句（2.7）。

`[确认]` mesfile `0x383b`「寿终正寝，结束了波澜壮阔的一生。……留下了一个个的传说，迎向崭新的时代。」与亲笔信句挤在同一长块。这是**剧本结束 / 寿终**文案，日历怎么判寿终、谁能继承，不在本文件。

`[确认]` GameOver 画面：`fn_14000a894`（`0x14011fb70`）RTC `cGameOverStill` `cGameOverSurfaceSprite` `posGameOver` `sizeGameOver` `rcGameOver`，从 `TitleSC.dk4` 取 still。场景表 overlay `game_over`（`12-ui-save/01-scenes-flow.md`）。本系统**不**加载这张图。

主角死亡 → 发 `PersonDied { person_id, cause, is_protagonist: true }` → 寿终 / 继承 / GameOver 文档接收。属下死亡是否移出名册 `[未知]`。`CPerson::vfunc_04` 以 `+0x0D != 6` 返回「存活」。

### 2.10 手册差（已消解）

官方手册把疲劳列为身体状态、精神分「通常 / 元気」等六档——与 exe 指针表一致（简中「普通 / 精神」）。旧版本「以 exe 字串为准」的结论作废。

### 2.11 每日结算（`fn_140005a15`，body `0x1400fa870`）`[确认]`

对玩家舰队在舱人员（`fn_140002e3c` 给出 `(person_id, cabin_kind)` 列表）逐人依次调用。记号：`hp`/`max` = 当前 HP / 上限；`rest` = 舱 ∈ {`0x18` 无舱, 12 单人房}；`hap` = 舰队 `+0x30` 的事件码（`fn_14000b87a`；值域与海上事件名表 `0x140686550` 对齐：0 坏血病、1 有老鼠、2 暴风雨、3 暴风雪 … 23 传染病、24 无风、25 鲨鱼，`0x1B` = 无事 `[推断]` 语义）；`doc` = 船医（岗位 11）的 `Skill@医务室(观察力)`，无船医为 0；`lead` = 舰长（岗位 0）的 `Skill@舰长室(统率力)`；`miss` = `fn_14000da26(fleet, 10, 7, 0)` = 岗位 10 传教士经 `fn_140007e19(10)` 换成礼拜室（18）后的 `Skill@礼拜室(说服力)` `[确认]`（`03` §2）。

顺序 `[确认]`（`fn_140005a15` 原始指令调用序）：补给日耗 `fn_140005aa6` → 列在舱人 → 下列 1–5 逐人（`0x1400fb578`–`0x1400fb5fc`）→ 海上事件掷骰 `fn_1400f2d90`（`0x1400fb76d`）→ 6 事件精神转移 `fn_1400ec260`（`0x1400fb841`）→ 写事件态。即 1–5 用的 `hap` 是**掷骰前**（昨天留下）的事件态。在港（港槽 `!= 0x62`）时同一函数只走舰队疲劳的在港枝 `-(rand(5)+2)`，跳过补给、人物 1–5 与掷骰（`0x1400fa8cd` / `0x1400fb392`，`06-ocean/01` §4.4）；人物侧在港不结算。

1. **HP 增减** `fn_1400ebe40`：
   - 基值：`hap` 0 或 23 → −2；2、3 → −3；其余 0。
   - `hap == 0x1B` 时：健康 +2；疲劳 / 轻伤 +1。
   - 疾病 −1。
   - 舱：单人房 +2；娱乐室（16）或无舱 +1；甲板（2）：`n = 5 − (Skill(p,生命力) − hp)/100`，`n > 1` 则再减 `rand(n)`。
2. **身体恢复** `fn_1400fbe50`（命中即改，船医得经验 `fn_140011c34(doc, e0, e1)`）：

   | 当前 | 自然条件 | 否则若 `rest` 且 `rand(500) < X` | 船医经验 |
   | --- | --- | --- | --- |
   | 疲劳 | `hp > max×95/100` → 健康 | — | — |
   | 轻伤 | `hp > max×95/100` → 健康 | `X=(hp+doc)/2` → 健康 | 10, 10 |
   | 重伤 | `hp > max×7/10` → 轻伤 | `X=(hp+doc)/3` → 轻伤 | 100, 50 |
   | 濒死 | `hp > max×4/10` → 重伤 | `X=(hp+doc)/4` → 重伤 | 250, 250 |
   | 疾病 | — | `X=(hp+doc)/3` → 健康 | 50, 100 |

3. **身体恶化** `fn_1401019b0`：健康且 `hp < rand(max) − max/10` → `hap` 为 0 或 23 时疾病，否则疲劳；重伤且非 `rest` 且 `hp < rand(max) − max/2` → 濒死。
4. `fn_1400fbcf0`：按娱乐室 / 单人房以 1/4 概率算出「好一档」的精神值，但**只写局部变量、从不回写**——原版这段是无效代码。
5. **精神日漂移** `fn_1400ebfd0`：
   - 舱不在 {娱乐室, 单人房} 且非混乱：`new = DAILY[mental][rand(10)]`，7 = 不变。
   - 否则：混乱时 `rand(500) < lead/2 + 2×精神(生值)` → 普通，舰长得经验 10/10；未成功或非混乱则：疲劳且 `rand(5)==0` 且 `hp < max×9/10` 且 `rand(500) > (lead+miss)/2` → 不高兴；再否则在甲板且 `rand(500) > (lead+miss)/2` 且 `rand(100) < 等级和` → 不高兴（甲板分支只有混乱者能走到）。

   `DAILY`（`0x1406829e0`，6×10 int，行 5 不用）：

   | 当前 | 10 格结果（7=不变） |
   | --- | --- |
   | 高亢 | 不高兴、普通、轻松、精神、7×6 |
   | 精神 | 普通、7×8、轻松 |
   | 轻松 | 普通、7×8、高亢 |
   | 普通 | 不高兴、7×8、轻松 |
   | 不高兴 | 普通、普通、7×7、普通 |

6. **海上事件的精神转移** `fn_1400ec260(code, …)`（`code` 为当日掷出的事件，缺省 `0x1B`；20/21/22 先交专门函数，不进这里）：code 4 → 高亢者 1/2 变不高兴；code 14 → 精神→不高兴、轻松→精神、普通→轻松、不高兴→普通；code ≤ 19 的其它值及 23–26、> 27 → 非混乱者按 `EVENT[mental][rand(10)]`；20–22、27 不变。

   `EVENT`（`0x140682ad0`）：

   | 当前 | 10 格结果 |
   | --- | --- |
   | 高亢 | 不高兴、7、普通、精神、7×6 |
   | 精神 | 不高兴、7、7、普通、7×4、高亢、高亢 |
   | 轻松 | 不高兴、7、普通、7×5、高亢、高亢 |
   | 普通 | 不高兴、7×8、精神 |
   | 不高兴 | 7×8、普通、精神 |

### 2.12 休养（`fn_1400044cb`，body `0x1401605f0`）`[确认]`

对在舱人员逐人：`hp = min(hp + max/20, max)`；疲劳 / 轻伤 → 健康；疾病 → `flag==0` 必愈、否则 1/3；重伤 → 1/3 轻伤；濒死 → 1/3 重伤。精神：混乱 → 不高兴；普通 → 轻松、不高兴 → 普通（`flag==0` 时 2/3，否则 1/2）。入口：`fn_14000c9eb`（RTC `sMenuOption`，循环 N 次、`flag=0`）= 旅馆长住，`fn_14000295a`（单次、`flag=1`）= 旅馆住一晚，都经每晚包装 `fn_14000af74`（同一晚还减水手疲劳，见 2.5）`[确认]`（`08-town-life/05` §2.2）。扫的是 `aeCompanion`（`fn_140009106`）在舱名单。

## 3. 实体与 ID

| 实体 | 原版 | 原版计数 | 空 / 非法 | 重构 key | 拥有者 |
| --- | --- | --- | --- | --- | --- |
| 身体状态 | `ePhysicalState`，`CPerson+0x0D` | 7 `[确认]` | `INVALID_PHYSICALSTATE=-1` | `physical_state_id` | 人物 |
| 精神状态 | `eMentalState`，`CPerson+0x0E` | 6 `[确认]` | `INVALID_MENTALSTATE=-1`；转移表内 7 = 不变 | `mental_state_id` | 人物 |
| 当前 HP | `CPerson+0x20` u16 | 存档 | — | `hp` | 人物 |
| HP 上限 | 无字段 | 派生 | — | `hp_max` | 运行时 |
| 舰队水手疲劳 | 舰队 `+0x71`，HUD `%4d疲劳`，`FATIGUE_*` `[推断，强]` | 0–100（`fn_140009467` 钳制） | — | `Fleets.fatigue(fleet)` | **fleets**；本系统只发增量 |
| 身体 / 精神计量 | `PHYSICALLEVEL_*` / `MENTALLEVEL_*` | UCHAR 宽 | — | 暂不建字段 | `[未知]`，人物对象里没有对应字节 |
| 海上事件码 | 舰队 `+0x30`、当日掷骰 | 26（名表）+ `0x1B` 无事 | 运行时 `null`；`0x1B` 只在 `legacy_codec` | happening key | **ocean**；本系统只消费 |

原版包身体状态（id = 原版枚举值）：

| id | key | 名称 | 技能乘数 | 每日 HP（`hap==无事`） | 自然恢复 |
| --- | --- | --- | --- | --- | --- |
| 0 | `healthy` | 健康 | 1 | +2 | — |
| 1 | `fatigued` | 疲劳 | 19/20 | +1 | `hp > 95%` → 健康 |
| 2 | `light_wound` | 轻伤 | 19/20 | +1 | `hp > 95%` → 健康 |
| 3 | `heavy_wound` | 重伤 | 1/2 | 0 | `hp > 70%` → 轻伤 |
| 4 | `dying` | 濒死 | 1/10 | 0 | `hp > 40%` → 重伤 |
| 5 | `disease` | 疾病 | 3/5 | −1 | 只靠休养 / 船医掷骰 |
| 6 | `dead` | 死亡 | 1 | 0 | 终态（`vfunc_04` 返回 0） |

精神状态（id = 原版枚举值）：

| id | key | 名称 | 能力乘数 |
| --- | --- | --- | --- |
| 0 | `elated` | 高亢 | 体力 11/10、敏捷 105/100、智力 95/100、精神 105/100 |
| 1 | `spirited` | 精神 | 体力、精神 105/100 |
| 2 | `relaxed` | 轻松 | 智力 105/100 |
| 3 | `normal` | 普通 | — （初值） |
| 4 | `unhappy` | 不高兴 | 精神 95/100 |
| 5 | `confused` | 混乱 | 体力 95/100、智力 9/10 |

运行时以表长循环，禁止写死 `7` / `6`。「死亡」「初值」「转移表的不变哨兵」都用 YAML 标记，不用下标。

恢复来源（不是枚举，是命令 `source`）：

| key | 谁发命令 | 原版效果 |
| --- | --- | --- |
| `inn_long` | town 旅馆长住（`fn_14000c9eb`） | 2.12 `flag=0`，每晚一次；水手疲劳 −(rand(5)+5) |
| `inn_single` | town 旅馆住一晚（`fn_14000295a`） | 2.12 `flag=1`；水手疲劳 −(rand(3)+4) |
| `cabin_rest` | 每日结算 | 单人房：HP +2、计入 `rest`、免精神漂移 |
| `cabin_morale` | 每日结算 | 娱乐室：HP +1、免精神漂移 |
| `hot_spring` | ocean 探索 | 只改舰队疲劳 −15（`06-ocean/02`）；人物侧无效果 |
| `surgeon` | 每日结算 | `doc` 进恢复掷骰 |
| `item` | items | 莱姆 / 中药等，`[未知]` |

## 4. 算法 / 公式

全部进 `config/formulas/vitals.yaml`（D3 形状：`hp` / `daily` / `rest` 三条，§6）。代码只留参数名；下列伪码里 `hp.*` / `daily.*` / `rest[*]` 指该文件同名条目的 `params` / `refs` / `tables`。默认值都来自 §2.3a / 2.11 / 2.12。

### 4.1 HP

```
hp_max = Modifiers.eval("person.hp_max", { person })     # base = formulas/character.yaml hp_max（生命力 base，raw）
add_hp(p, d):
    old = p.hp
    p.hp = clamp(old + d, 0, hp_max(p))
    if old > 0 and p.hp <= 0:
        p.physical = hp.zero_state                 # 原版 dying
        Progression.grant_exp(p, add{track_a: 400}, once_per_call, source: vitals.hp_zero)
                                                   # 量 = vitals hp.zero_exp [400, 0]；11 路由写 amount: from_event
on LevelChanged(p): p.hp += max(0, hp_max_after - hp_max_before)
```

心情不是独立数值：状态画面的「心情」= `mental_state_id` `[推断]`。

### 4.2 每日结算（`tick.yaml` 阶段，D4）

本系统挂在 `tick.yaml` phase `daily`（`schedule: [day_start]`，**不挂** `when: at_sea`）的两个步骤 `characters.vitals` 与 `characters.mental_transition`，不自订 tick、不读 `SCalendar` 字段。步骤序已定（`00-overview/10-key-glossary.md` §11 / §14-18、`06-ocean/01` §4.4）：海上分支 补给 → 疲劳 → 人物 → 掷骰，即 `ocean.supply_drain → fleets.fatigue → ocean.happening_upkeep → characters.vitals → ocean.happening_roll → characters.mental_transition → ocean.happening_persist`；在港分支只走 `fleets.fatigue` 的疲劳恢复，本系统两步按 `Fleets.location(fleet_id).town_id` 判在港直接跳过（旅馆每夜推进日首时同样只触发在港分支）。所以 `characters.vitals` 读的是**当日掷骰前**的事件态（原版先结算人物、后掷事件，见 2.11 顺序）；`Ocean.happening_at_day_start(fleet)` 只是这个值的查询名。

```
stage characters.vitals (fleet):                        # 只对 daily.scope 命中的舰队；原版 = 玩家舰队
  if Fleets.location(fleet).town_id != null: return    # 在港分支：人物不结算（原版只做疲劳恢复，fleets 负责）
  hap = Ocean.happening_at_day_start(fleet)              # 掷骰前的事件态；null = 无事（原版 0x1B）
  doc  = skill_of(Fleets.post_holder(fleet, daily.surgeon.post), daily.surgeon)   # 无人 = 0
  lead = skill_of(Fleets.post_holder(fleet, daily.captain.post), daily.captain)
  miss = skill_of(Fleets.post_holder(fleet, daily.morale_support.post), daily.morale_support)
  for (p, cabin) in Characters.aboard_with_cabin(fleet):
    rest   = cabin in daily.rest_cabins                  # [null, vanilla:cabin_single]
    # 1 HP
    d  = hap == null ? 0 : (Ocean.outcome(hap).daily.hp or 0)   # ocean/happenings.yaml outcomes.<key>.daily.hp
    if hap == null: d += physical[p.physical].calm_hp
    d += physical[p.physical].hp_daily                   # 疾病 -1
    d += daily.hp.by_cabin.get(cabin, 0)
    if cabin in daily.hp.strain_cabins:                  # 甲板
        n = strain.base - (skill(p, vitality) - p.hp) / strain.div
        if n > 1: d -= rand(n)
    add_hp(p, d)
    # 2 身体恢复：physical[*].recover 行驱动（阈值、掷骰分母、医生经验 → emit SurgeonWorked）
    # 3 身体恶化：physical[*].worsen 行驱动（疾病/疲劳由 hap 决定）
    # 4 精神：不在 mood_cabins 且非 confused → transitions.daily；否则 confused 掷骰回 normal，
    #         再按 fatigue / strain 规则掷「不高兴」
step characters.mental_transition (fleet, happening):    # 原版 fn_1400ec260，紧跟 ocean.happening_roll
    apply transitions.by_happening[happening] or transitions.event_default
```

`skill_of(holder, rule)` = `Modifiers.eval("skill.<rule.skill>", {person: holder, cabin: rule.cabin})`。岗位只按 key 引用，不写 11 / 0 / 10。

所有阈值、分母、经验量、转移表都是 `vitals.yaml` 的行，不是 `if`。原版 `fn_1400fbcf0` 的无效计算**不**还原；若模组要「娱乐室/单人房提升精神」，填 `transitions.cabin_bonus` 表即可。

### 4.3 休养

```
Characters.apply_rest(fleet, source, nights):           # town 旅馆 Action 调；source ∈ rest 表 key
  repeat nights:                                         # 原版每晚还走 1 天、扣 1 金（town 负责）
    for p in Characters.aboard(fleet):
      add_hp(p, hp_max(p) * rest[source].hp_frac)        # 原版 1/20
      roll rest[source].physical / mental 转移行           # 原版见 2.12
    Fleets.add_fatigue(fleet, -roll(rest[source].crew_fatigue), reason: source)   # 原版 2.5；fleets 钳 0..100
  emit VitalsChanged
```

旅馆每晚「走 1 天」推进日首，由 clock 触发 `daily` 的在港分支（只有 fleets 疲劳恢复，人物两步跳过），不在本命令内。长住可被事件打断（`08-town-life/05`）：打断判定在 town 的 `Action.repeat.interruptible`（D9），本系统每晚独立执行。旅馆单价在 town 公式包（默认 1）。本系统不扣金币。

### 4.4 疾病与事件

坏血病 / 传染病的掷骰仍归 ocean；人物侧只有两条原版规则：每日 HP 基值（坏血病 / 传染病 −2、暴风雨 / 暴风雪 −3）与「健康者 `hp < rand(max) − max/10` 时，事件是坏血病/传染病则疾病、否则疲劳」。免疫修正（莱姆、圣母像）在 ocean 掷骰前判，`[未知]` 是否也影响人物侧。

### 4.5 战斗受伤

```
Characters.set_physical(person_id, grade)         # combat 发；grade = physical_state key
  p.physical = max_severity(p.physical, grade)    # injury_order 表
Characters.add_hp(person_id, hp_delta)            # combat 发
  add_hp(p, hp_delta)                             # 归零 → dying（原版 setter 行为）
```

原版战斗侧写身体状态的调用点：`fn_1400108a2`（RTC `aePerson`）写 2/3、`fn_1401bdaa0` 写 2/1 `[确认]` 调用值，`[推断]` 是接舷 / 海战伤；无任何调用点直接写 6（死亡）。

### 4.6 死亡出口

```
set_physical(p, dead, cause):                     # 任何来源（事件 / progression 寿终）
  p.physical = dead
  emit PersonDied { person_id, cause, is_protagonist: Characters.is_protagonist(person_id) }
  # 非主角：名册策略 YAML officer_death_policy
```

原版：HP 归零只到濒死；谁把人写成死亡未找到直接写 6 的调用点（`fn_14000ca77` 以变量写，未读），`[未知]`。

### 4.7 修正器贡献（D2，本系统登记）

| 键 | stage | 来源 | 值 |
| --- | --- | --- | --- |
| `ability.<key>` | `mul` | `vitals:mental.<state>` | `mental_states[*].ability_mult` |
| `skill.<key>` | `mul` | `vitals:physical.<state>` | `physical_states[*].skill_mult` |

船医 / 舰长 / 传教士的技能**不是**本系统的贡献：本系统自己是这些技能值的消费方（经 `Fleets.post_holder` + `skill.<key>`）。业务系统禁止读 `physical_state` 下标；需要「伤员不能单挑」之类规则时，由 `vitals.yaml` 状态行加 `flags`（如 `can_duel: false`），消费方按 flag 查。

## 5. 耦合分析

原版硬编码：

- 旅馆 / 教会 / 舱室说明句直接改「身体 / 精神」，没有 rest 表
- 大地图 HUD 把水手人数和 `%4d疲劳` 画在一起
- 坏血病 / 传染病是 ocean happening 文案，治愈看船医岗位和莱姆道具
- 接舷可能读某个 `+0x71` 字节，语义未钉
- 每日结算 `fn_140005a15` 一个函数里同时做补给、HP、身体、精神、船医经验，阈值 95/70/40%、分母 500、转移表全是立即数或 `.data` 表
- 技能公式直接 `switch` 身心枚举值算乘数；舱室种类 2/12/16/`0x18` 写死在每日函数
- 海上事件码（ocean）直接决定人物 HP 基值与疾病/疲劳分支
- 主角死亡切 `TitleSC.dk4` GameOver still，与人物对象同一条调用链

允许的依赖方向：

```
town-life  → Characters.apply_rest(fleet, source, nights)
clock      → tick.yaml phase daily 步骤 characters.vitals / characters.mental_transition（D4）
ocean      → Ocean.happening_at_day_start / 当日掷出的 happening（查询）
combat     → Characters.set_physical / add_hp       （命令）
items      → 修正器注册表贡献（抗病等，键由 ocean 登记）
fleets     → Fleets.post_holder / Fleets.fatigue；人物只发 Fleets.add_fatigue
characters → PersonDied { is_protagonist } → progression / ui
```

禁止：ocean 改 `CPerson+0x0D`；combat 加载 GameOver；town 自己清疾病；ui 写存档疲劳；任何系统按岗位 / 舱室字面量（11、0、10、2、12、16）判断。

## 6. 解耦后的配置

`config/characters/vitals.yaml` — 状态目录 + 转移表，本系统拥有。下面是原版包取值（全部 `[确认]`，出处见 §2）。

```yaml
schema: dk4.characters.vitals
version: 1

physical_states:            # vanilla_index = 原版枚举值；主键 key；模组可追加
  - { vanilla_index: 0, key: vanilla:healthy,     skill_mult: 1,      calm_hp: 2, hp_daily: 0 }
  - { vanilla_index: 1, key: vanilla:fatigued,    skill_mult: 19/20, calm_hp: 1, hp_daily: 0,
      recover: { to: vanilla:healthy, hp_above: 95/100 } }
  - { vanilla_index: 2, key: vanilla:light_wound, skill_mult: 19/20, calm_hp: 1, hp_daily: 0,
      recover: { to: vanilla:healthy, hp_above: 95/100, rest_roll_div: 2, surgeon_exp: [10, 10] } }
  - { vanilla_index: 3, key: vanilla:heavy_wound, skill_mult: 1/2, hp_daily: 0,
      recover: { to: vanilla:light_wound, hp_above: 7/10, rest_roll_div: 3, surgeon_exp: [100, 50] },
      worsen:  { to: vanilla:dying, unless_rest: true, hp_below_rand_minus: 1/2 } }
  - { vanilla_index: 4, key: vanilla:dying,       skill_mult: 1/10, hp_daily: 0,
      recover: { to: vanilla:heavy_wound, hp_above: 4/10, rest_roll_div: 4, surgeon_exp: [250, 250] } }
  - { vanilla_index: 5, key: vanilla:disease,     skill_mult: 3/5, hp_daily: -1,
      recover: { to: vanilla:healthy, rest_roll_div: 3, surgeon_exp: [50, 100] } }
  - { vanilla_index: 6, key: vanilla:dead,        skill_mult: 1, terminal: true }
physical_initial: vanilla:healthy
healthy_worsen:             # 健康者 hp < rand(max) - max*frac
  frac: 1/10
  disease_when_ref: ocean/happenings.yaml#outcomes.*.daily.disease   # true → disease（原版 scurvy / plague）
  to_default: vanilla:fatigued
injury_order: [vanilla:healthy, vanilla:fatigued, vanilla:light_wound, vanilla:heavy_wound, vanilla:dying, vanilla:dead]   # disease 不在全序内

mental_states:
  - { vanilla_index: 0, key: vanilla:elated,   ability_mult: { vanilla:stamina: 11/10, vanilla:agility: 105/100, vanilla:intelligence: 95/100, vanilla:spirit: 105/100 } }
  - { vanilla_index: 1, key: vanilla:spirited, ability_mult: { vanilla:stamina: 105/100, vanilla:spirit: 105/100 } }
  - { vanilla_index: 2, key: vanilla:relaxed,  ability_mult: { vanilla:intelligence: 105/100 } }
  - { vanilla_index: 3, key: vanilla:normal }
  - { vanilla_index: 4, key: vanilla:unhappy,  ability_mult: { vanilla:spirit: 95/100 } }
  - { vanilla_index: 5, key: vanilla:confused, ability_mult: { vanilla:stamina: 95/100, vanilla:intelligence: 9/10 } }
# name_key 缺省 = vitals.<key>.name；乘数登记为 mul 贡献，{num,den} 先乘后除截断
mental_initial: vanilla:normal

transitions:                # 用 key 写，不用下标；"-" = 不变（原版 7）
  daily:      # 10 格均匀
    vanilla:elated:   [vanilla:unhappy, vanilla:normal, vanilla:relaxed, vanilla:spirited, "-", "-", "-", "-", "-", "-"]
    vanilla:spirited: [vanilla:normal, "-", "-", "-", "-", "-", "-", "-", "-", vanilla:relaxed]
    vanilla:relaxed:  [vanilla:normal, "-", "-", "-", "-", "-", "-", "-", "-", vanilla:elated]
    vanilla:normal:   [vanilla:unhappy, "-", "-", "-", "-", "-", "-", "-", "-", vanilla:relaxed]
    vanilla:unhappy:  [vanilla:normal, vanilla:normal, "-", "-", "-", "-", "-", "-", "-", vanilla:normal]
  event_default:
    vanilla:elated:   [vanilla:unhappy, "-", vanilla:normal, vanilla:spirited, "-", "-", "-", "-", "-", "-"]
    vanilla:spirited: [vanilla:unhappy, "-", "-", vanilla:normal, "-", "-", "-", "-", vanilla:elated, vanilla:elated]
    vanilla:relaxed:  [vanilla:unhappy, "-", vanilla:normal, "-", "-", "-", "-", "-", vanilla:elated, vanilla:elated]
    vanilla:normal:   [vanilla:unhappy, "-", "-", "-", "-", "-", "-", "-", "-", vanilla:spirited]
    vanilla:unhappy:  ["-", "-", "-", "-", "-", "-", "-", "-", vanilla:normal, vanilla:spirited]
  by_happening:
    vanilla:rain:    { vanilla:elated: { to: vanilla:unhappy, chance: 1/2 } }
    vanilla:mermaid: { vanilla:spirited: vanilla:unhappy, vanilla:relaxed: vanilla:spirited, vanilla:normal: vanilla:relaxed, vanilla:unhappy: vanilla:normal }
    vanilla:mutiny: none
    vanilla:chaos: none
    vanilla:leak: none
  cabin_bonus: null         # 原版 fn_1400fbcf0 算了不写；默认关闭
```

`config/formulas/vitals.yaml` — 只放数，按 D3 公式形状分三条（`hp` / `daily` / `rest`）。

```yaml
schema: dk4.formulas.vitals
version: 2
# 每条 = { impl, int_mode, quirks, params, tables, refs, rng_stream, golden }（14-config-schemas README「公式层」，D3）

hp:                                       # §4.1 add_hp：夹 [0, hp_max]；由正转 ≤0 → zero_state
  impl: vanilla.vitals_add_hp
  int_mode: vanilla_i32
  params:
    zero_state: vanilla:dying
    zero_exp: [400, 0]                    # progression 路由 amount: from_event
  refs: { hp_max: formulas/character.yaml#hp_max }   # HP 上限 = 生命力 base（raw）
daily:                                    # §4.2 步骤 characters.vitals；在港分支跳过
  impl: vanilla.vitals_daily              # fn_140005a15 人物段（fn_1400ebe40 等）
  int_mode: vanilla_i32
  rng_stream: characters
  params:
    scope: player_fleet                   # 原版只结算玩家舰队
    rest_roll_den: 500                    # rand(500)
  refs:
    rest_cabins: [null, vanilla:cabin_single]   # null = 不在舱（原版 0x18）
    mood_cabins: [vanilla:cabin_single, vanilla:lounge]
    surgeon: { post: vanilla:surgeon, skill: vanilla:observation, cabin: vanilla:infirmary }
    captain: { post: vanilla:captain, skill: vanilla:leadership, cabin: vanilla:captain_room }
    morale_support: { post: vanilla:missionary, skill: vanilla:persuasion, cabin: vanilla:chapel }   # fn_14000da26(fleet,10,7,0)
  tables:
    hp:
      by_happening_ref: ocean/happenings.yaml#outcomes.*.daily.hp   # 原版 scurvy/plague −2，storm/blizzard −3
      by_cabin: { vanilla:cabin_single: 2, vanilla:lounge: 1, null: 1 }
      strain_cabins: { vanilla:deck: { base: 5, div: 100 } }
    mental:
      confused_recover: { den: 500, lead_div: 2, spirit_mult: 2, to: vanilla:normal, captain_exp: [10, 10] }
      fatigue_upset:    { gate_den: 5, hp_below: 9/10, den: 500, to: vanilla:unhappy }
      strain_upset:     { cabins: [vanilla:deck], den: 500, level_den: 100, to: vanilla:unhappy }
rest:                                     # §4.3 Characters.apply_rest；旅馆 fn_1400044cb flag 0 / 1 + fn_14000efde
  impl: vanilla.vitals_rest
  int_mode: vanilla_i32
  rng_stream: characters
  tables:
    inn_long:    { hp_frac: 1/20, disease_cure: 1,   heavy_to_light: 1/3, dying_to_heavy: 1/3,
                   confused_to: vanilla:unhappy, normal_to_relaxed: 2/3, unhappy_to_normal: 2/3,
                   crew_fatigue: "rand(5) + 5" }
    inn_single:  { hp_frac: 1/20, disease_cure: 1/3, heavy_to_light: 1/3, dying_to_heavy: 1/3,
                   confused_to: vanilla:unhappy, normal_to_relaxed: 1/2, unhappy_to_normal: 1/2,
                   crew_fatigue: "rand(3) + 4" }
# 舰队水手疲劳的范围 / 门槛归 fleets（formulas/fleets.yaml：range [0,100]）与 ocean（划桨 50、哗变 50）
# town-life 写的 rest_tables.inn[mode] 即本 rest.tables：mode single → inn_single，long → inn_long
```

`happening` key（`scurvy` / `plague` / `storm` / `rain` / `mermaid` …）由 ocean 的事件表定义，本系统只引用 key；「无事」是 `null`（原版 `0x1B` 只在 `legacy_codec`）。舱室 key 由船包 `cabins.yaml` 定义。World0 / World1 共用同一套 vitals，不按剧本复制。

## 7. 独立旋钮

改这些不应迫使贸易 / 海战 / 航行改代码：

- 增减身体 / 精神档（表长驱动；状态行自带乘数、恢复、恶化规则）
- 每日转移表、事件转移表、恢复阈值 95/70/40%、掷骰分母 500
- 休养每晚 HP 比例、各档治愈概率
- 船医 / 舰长 / 传教士参与掷骰用哪个技能、哪个舱
- 哪些舱算「休养舱」「情绪舱」「劳损舱」
- 属下死亡是留尸还是移出名册
- HUD 水手疲劳用 3 位还是 4 位（`%4d` 只是原版投影）

会碰到边界、但仍不改外系统代码：

- 计量（`PHYSICALLEVEL` 等）日后还原 → 只动本表与 save schema 人物节
- 主角死亡演出 → 只动 gameover 文档 / `ui/scenes.yaml` overlay

改生命力技能配对仍走 `skills.yaml`，不在本文件写六维下标。

## 8. 对外接口

查询：

```
Characters.vitals(person) -> {
  hp, hp_max, physical_state (key), mental_state (key)
}
Characters.can_act(person) -> bool             # 原版只有 dead 判否（vfunc_04）；其余进 YAML 状态行 flags
Modifiers.eval("skill.<key>" | "ability.<key>", { person })   # 本系统登记身心 mul 贡献

# 引用别系统
Fleets.fatigue(fleet) -> int                   # 舰队水手疲劳 0..100（原版 CFleet+0x71）
Fleets.post_holder(fleet, post_key) -> person | null
Ocean.happening_at_day_start(fleet) -> happening_key | null
```

命令（只收 ID 与标量）：

```
# 每日结算不是命令：tick.yaml phase daily 步骤 characters.vitals（§4.2，D4）
# 海上事件精神转移不是命令：phase daily 步骤 characters.mental_transition（§4.2）
Characters.apply_rest(fleet, source, nights)       # source ∈ rest 表 key；同时调 Fleets.add_fatigue
Characters.add_hp(person_id, delta)                # combat / 事件
Characters.set_physical(person_id, state_key)      # combat 伤（取较重）/ 事件 / progression 寿终写 dead
Characters.set_mental(person_id, state_key)        # 事件
```

事件：

```
VitalsChanged     { person_id, hp, physical_state, mental_state }
PersonDied        { person_id, cause, is_protagonist }   # progression / ui / narrative 订阅
SurgeonWorked     { person_id, exp: [e0, e1] }      # progression 订阅后加经验
```

`PersonDied`（`is_protagonist: true`）的订阅方：

- `11-progression/03-gameover-succession.md` — 寿终、继承、是否允许换提督
- `12-ui-save/01-scenes-flow.md` — overlay `game_over`（`TitleSC.dk4` still）

本系统不 `new` GameOver 精灵，不读 `SGameSeed` 生日。

存档：HP、身体、精神是**状态**，必须写；HP 上限与技能是派生，不写。导入原版档按 `01-person-model.md` 的记录成帧读；ids 87–234 原版无这些字段，用定义初值。

## 9. 未核实字段

| 项 | 依据 | 状态 |
| --- | --- | --- |
| `NUM_PHYSICALSTATE` / `NUM_MENTALSTATE` 头文件整数 | 名称表 7 / 6 项，`vfunc_04` 用 6 判死亡 | 值 `[确认]` 7 / 6；头文件仍未写 |
| `FATIGUE_MIN/MAX` 整数 | 仅 UCHAR 宽；`fn_140009467` 钳 0..100 | `[推断]` 0 / 100；归舰队 `+0x71` `[推断，强]` |
| `PHYSICALLEVEL` / `MENTALLEVEL` 是什么 | World DLL 断言；`CPerson` 里没有对应字节 | `[未知]` |
| 舰队 `+0x30` 事件码与海上事件名表的对应 | 值 0/1/2/3/23/`0x1B` 与名表下标吻合；`fn_14000a8fd` 写 outcome（`06-ocean/05`） | `[推断]` |
| `fn_14000da26(fleet, 10, 7, 0)` | `fn_140007e19(10)` = 礼拜室 18 → 传教士；技能 7 说服力 | `[确认]` |
| `fn_14000c9eb` / `fn_14000295a` 两个休养入口 | 旅馆长住（flag=0）/ 住一晚（flag=1）（`08-town-life/05`） | `[确认]` |
| 道具的人物侧效果 | 只有文案 | `[未知]`；教会不休息、温泉只改舰队疲劳（已确认） |
| 谁把人写成死亡（6） | 无直接常数调用点；`fn_14000ca77` 以变量写 | `[未知]` |
| 战斗调用点 `fn_1400108a2` / `fn_1401bdaa0` 的触发条件 | 写 2/3、2/1 | `[未知]` |
| 坏血病 / 传染病掷骰、莱姆 / 圣母像是否也挡人物侧疾病 | happening 未抽出 | `[未知]` |
| 水手「不满」是否独立于疲劳 | 传教士消不满；另有哗变句 | `[未知]`；不与疲劳合并 |
| `fn_14000ee12` | 读舰队 `+0x71`；`posTired`、划桨门、哗变门、接舷 `(200−疲劳)` 都读它 | 舰队水手疲劳 `[推断，强]`；不是人物身体状态 |
| 属下死亡后名册 | 临终台词有 | `[未知]` |
| 寿终判据（年龄 / 日历 / 生日） | 「寿终正寝」文案 | 归 gameover 文档 |
| `PLAYERSTATE_BITS` 是否含伤病 | 与同伴位同头文件 | `[未知]`，见名声文档 |
