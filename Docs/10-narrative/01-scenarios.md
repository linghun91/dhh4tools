# 剧本

配置落点：`config/narrative/scenarios.yaml`。主角定义外键指向 `config/characters/protagonists.yaml` / `persons.yaml`。世界包与事件包由本表选择，不在经济、战斗、航海代码里写死。

## 1. 职责边界

本系统管：

- 一次开局选哪个 `scenario_id`
- 该局加载哪份世界内容包、哪份事件脚本包、哪份场景图包
- 该局的主角 `person_id`、开局种子（生日等）
- 本局终局（通关 / Game Over / 回标题）统一走 `Session.end_scenario(outcome, cause)`（DECISIONS D8）；画廊点数与解锁规则归 `05-gallery-unlock.md`
- **剧本特例**：原版散在经济 / 势力 / 世界里的 `eScenario` 比较，全部写成本表的 `flags` 与 `modifiers`，开局由本系统注入修正器注册表（DECISIONS D5、D2）

不管：

- 港口、人物、商品、势力的实例数值（世界包）
- 事件脚本字节码与触发器求值（`02-event-system.md`）
- 贸易、航海、战斗公式
- UI 文案正文（只引用 `string_key`）

原则 7：其它系统禁止 `if scenario_id == 3`。需要差异时读本表给出的 `world_pack` / `event_pack`、修正器注册表里的剧本贡献（`Modifiers.eval(key, ctx)`）、或 `Scenario.flag(key)`，或订阅叙事发出的命令。

## 2. 原版事实

### 计数与选择界面

[确认] `eScenario`：`NUM_SCENARIO = 7`，`INVALID_SCENARIO = -1`。来源：`srcdata/source/ScenarioID.h`，RTC `NUM_SCENARIO`。

[确认] 选剧本 UI 按 7 槽循环：

- `CScenarioMenuView` 构造把 `+0x80` / `+0xb8` 的 7 个指针清零（循环 `>= 7` 结束），析构同样扫 7 槽。`vfunc_13` 的边界是 `< 0` 或 `> 7`（比合法 id 0–6 多一个 7）。[未知] 7 是空槽还是反编译把 `>= 7` 写成了 `> 7`。
- `CScenarioSelectView` 同样扫 7 槽（`+0xf8`），内嵌 `SGameSeed`（`+0x80`）。`vfunc_13` 边界与菜单相同。
- `CScenarioButton` 是槽按钮。

来源：`srcdata/source/game/game/CScenarioMenuView.cpp`、`CScenarioSelectView.cpp`、`CScenarioButton.cpp`。

[确认] 开局要输入生日。mesfile 与七人简介同一文案块：`请输入生日。` 来源：`srcdata/catalog/mesfile_sc.txt` 偏移 `0xbab`。种子对象是 `SGameSeed`（体积 `0x78`），挂在 `CScenarioSelectView+0x80`。字段名未还原，见第 9 节。

### 七个主角 = person 0–6

[确认] 七个剧本对应 `ePerson` 0–6，World0 `GetFixPersonProc` 姓名与六维如下。来源：`srcdata/source/PersonID.h`、`srcdata/source/world/World0_person.c`、`Docs/00-overview/04-id-registry.md`。

| scenario_id | key | 名 | 姓 | 第三名 | 体力 | 敏捷 | 魅力 | 智力 | 精神 | 运气 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | raphael | 拉斐尔 | 卡斯特罗 | | 59 | 60 | 70 | 62 | 58 | 61 |
| 1 | helmut | 赫德拉姆 | 柏格斯通 | 约阿其姆 | 83 | 69 | 78 | 55 | 73 | 48 |
| 2 | lulu | 丽璐 | 阿歌特 | | 49 | 54 | 69 | 70 | 78 | 66 |
| 3 | hua_mei | 华梅 | 李 | 玛莉亚 | 66 | 58 | 81 | 86 | 69 | 78 |
| 4 | kyotaro | 杏太郎 | 佐伯 | | 79 | 59 | 80 | 53 | 71 | 51 |
| 5 | woodin | 亚伯拉罕 | 伍丁 | 易文 | 78 | 85 | 60 | 55 | 81 | 60 |
| 6 | tia | 蒂雅 | 恰斯卡 | 瓦曼 | 45 | 61 | 68 | 81 | 62 | 55 |

[确认] 存档 `savedatXX.dk4 +0x20` 写主角水手 id。工具注释写成「0 拉斐尔 … 5 亚伯拉罕」，未点名 id 6。来源：`srcdata/include/save.h`、`srcdata/docs/save_format.md`。重构按 `0..6` 处理，缺 6 视为注释不全，不是「只有六个剧本」。

### 简介文案（选人界面）

[确认] SC mesfile 在 `0xbab`–`0xed0` 一段里连续放七人简介与开场自称，以及跨剧本画廊提示。抽出时多条字符串粘在同一块，按语义切开如下。来源：`srcdata/catalog/mesfile_sc.txt`。

| id | 简介（第三人称） | 自称 |
| --- | --- | --- |
| 0 | 莫名地憧憬着要当一名水手的葡萄牙少年。受朋友的鼓动而出海。推荐初次玩此游戏的玩家选择他。 | 我是拉斐尔。是初次出海。 |
| 1 | 实现再次独立的瑞典提督。率领徒有海军虚名的非官方舰队。为把本国舰队建成世界最强的舰队而奋斗。 | 我是赫德拉姆。 |
| 2 | 经商的雄心极其旺盛的荷兰少女。和童年伙伴卡米尔一起梦想做一番大事业而周游世界。 | 我是丽璐。全世界都是我的交易伙伴！ |
| 3 | 为了保卫受倭寇侵略之苦的人们，敢于漠视被订为国法的锁国政策，率领非官方舰队进行斗争的明朝女提督。 | 我是华梅。我不允许倭寇横行霸道。 |
| 4 | 九州岛某豪族的后裔。一族人被敌人袭击，父亲剖腹自杀。被追杀，听天由命地坐着小渔船出海。 | 我的名字叫杏太郎。 |
| 5 | 阿拉伯大富商的后代。表面上是为了兴趣在收集古董，实际上是为了追查自己身世的秘密而漫无目的地旅行。 | 我叫伍丁。在阿拉伯经营商会。 |
| 6 | 继承统治印加帝国一个地方之王族血统的混血儿。担任埃斯康特军的翻译……。 | 我是蒂雅。我一定要独立。 |

更长人物传记在 mesfile `0x1f447` 一带（华梅、杏太郎、蒂雅等），属画廊 / 图鉴文案，不进玩法表。

### 世界包：World0.dll / World1.dll

[确认] 静态世界表是两份 DLL，同一套导出，不是两套逻辑：

- 路径格式 `SC/World%d.DLL`（语言目录随 exe：`Game/SC/` 等）
- 导出 `GetFixTownProc` / `GetFixPersonProc` / `GetFixItemProc` / `GetFixForceProc` / `GetFixFleetProc`
- `CDllManager` 构造调用 `LoadLibraryA`，再 `GetProcAddress` 五个符号

来源：`srcdata/source/game/unassigned/fn_1400b93e0.cpp`（`fn_14000c702`）、`srcdata/source/WorldData.h`、`srcdata/source/world/DllMain.cpp`、`srcdata/docs/formats.md`。

[确认] 选哪份 DLL 由 `fn_14000ff60`（body `0x1402a89a0`，23 字节，`cmp [rsp+0x10],4; jge`）决定：

```
if (id >= 4) return 1;  /* World1.dll */
else         return 0;  /* World0.dll */
```

`CWorldData::vfunc_05`、`CPerson::vfunc_05`、`CTown` / `CFleet` / `CForce` / `CItem` 初始化都走：

```
world_index = fn_14000ff60(*(owner + 0xc))
CDllManager(world_index)  →  LoadLibrary("SC/World%d.DLL")
```

同一条「4」分界还出现在画廊：`fn_1400089e5` 先扫 `eScenario` 0–3（任一通关则解锁画廊条目 `0x3E`），再扫 4–6（任一通关则解锁 `0x25/0x30/0x37/0x45`）。那是画廊内容，不是世界包规则，见 `05-gallery-unlock.md`。来源：反汇编 `0x1402a89a0`、`0x1401203d0`、`srcdata/source/game/world/CWorldData.cpp`。

因此原版内容包绑定是：

| scenario_id | 世界包 | 事件包 | 场景图包 |
| --- | --- | --- | --- |
| 0–3 | `vanilla-world0`（World0.dll） | EV0–EV3 | Sc0–Sc3 |
| 4–6 | `vanilla-world1`（World1.dll） | EV4–EV6 | Sc4–Sc6 |

World1 里 person 0–6 **仍是这七人**，部分六维与未命名字节与 World0 不同（例如 World1 赫德拉姆 82 68 77 54 73 47）。那是内容差异，不是另一套主角枚举。来源：`srcdata/source/world/World1_person.c`。

### 事件包与场景图包

[确认] 每个剧本一份事件脚本、一份场景图，外加全体共用的动画包：

| 文件 | 用途 |
| --- | --- |
| `SC/EV0.dk4` … `SC/EV6.dk4` | 该剧本触发器 + 脚本。头 `WE` |
| `SC/Sc0.dk4` … `SC/Sc6.dk4` | 该剧本脚本指令流 + 内嵌对白（每条记录魔数 `DS`）。文件头第一个 `uint32` 等于对应 EV 的触发条目总数。旧称「场景图」不对，见 `03-event-commands.md` |
| `SC/EVA.DK4` | 事件动画，七剧本共用 |

文件名表在 exe：`SC/EV0.dk4`…`SC/EV6.dk4` 连续存放，`fn_140009985(i)` 取 `QWORD table[i]`（表 VA `0x140682880`）；`Sc` 名表 VA `0x1406828c8`，下标取 `CDk4Game+0x7c`（`fn_140004a1b`）。`CEvent::vfunc_05` 用 `*(arg+0xc)` 当剧本下标去加载 EV。来源：`Game/SC/` 实文件、`srcdata/docs/formats.md`、`srcdata/source/game/event/CEvent.cpp`、字符串转储。

EV 条目总数（头 `+0x08`）：

| 包 | 文件大小 | 条目 |
| --- | --- | --- |
| EV0 | 8508 | 346 |
| EV1 | 8064 | 328 |
| EV2 | 8768 | 339 |
| EV3 | 7512 | 306 |
| EV4 | 6252 | 250 |
| EV5 | 7268 | 299 |
| EV6 | 6388 | 253 |

脚本格式见 `02-event-system.md`。

### 跨剧本画廊

[确认] 通关后用点数解锁人物简介和额外照片：

> 如果在其他剧本中顺利过关，使用１点就能显示此人物的人物简介，要显示吗？（剩%s点）使用%s点显示额外的照片吗？（剩%s点）

来源：mesfile `0xed0`。UI 宿主是 `CShowPortrait`（`fn_1400089e5` 构造；花点数的方法是其 vfunc 13，body `0x14011a140`）。**不是** `CAcrossGameView`：那是事件脚本 opcode `0xC1` 启动的小游戏（见 `03-event-commands.md`、`08-town-life/03-minigames.md`）。

[确认] 通关记账在会话层：流程态 4（脚本 opcode `0xC5` imm 0 置）→ `0x140112980`：放片尾动画后，若 `savedata.dk4` 里本剧本通关位未置，则点数 `+4`、置通关位、按 `aeScenarioPortrait` 免费解锁若干条目，再写回文件。点数、通关位、各解锁位的偏移与花费规则见 `05-gallery-unlock.md`。

### 开局条件

[确认] 新局至少写入：主角 `person_id`、`SGameSeed`（含生日输入）。

[确认] 剧本的终局由脚本发出：opcode `0xC5` imm 0 = 通关（流程态 4）；imm 1 或 execute id `0x3FFF` = 剧情 Game Over（流程态 3、参 1 = 寿终，片前放寿终四句消息 `0x170`–`0x173`）；其它 = 回标题。Game Over 共四个入口，唯一写入者 `fn_140010578`（body `0x1400c42d0`）：参 1 = `0x3FFF`、`0xC5` imm 1；参 0 = 玩家本队覆灭 `fn_14000a9ed`、玩家舰队解散 `fn_14000a8c1`。见 `03-event-commands.md`「流程态」、`11-progression/03-gameover-succession.md`。原版没有继承：Game Over 后只回标题。

[确认] 开局舰队 = 本剧本所用世界包里与剧本同号的舰队模板（W0 舰队 0–3、W1 舰队 4–6），船体见 `05-ships-fleet/05-hull-catalog-market-start.md`「开局船」；驻港 `town_id` 属舰队模板（`fleets/templates`）。重构从 hull 定义克隆独立船实例，不挂原版模板槽（D1）。开局舰队表只在 `narrative/starting_fleets.yaml`（05-ships-fleet/05 定义，按剧本 key 查行）；`scenarios.yaml` 不重复舰队 / 船体字段（词表 §14-12）。

[未知] 开局金币。人物表 `+0x18` 起两个 `uint32`（拉斐尔均为 `0x64=100`）[推断] 是两段经验初值（`04-world0-world1-diff.md` 第 9 节），禁止当成金币。原版没有名声与爵位。

### 剧本 → 玩家势力

[确认] `fn_140009df9` 用 7 元素常量表 `{0, 2, 1, 3, 20, 13, 21}` 把剧本号映射成玩家势力（卡斯特罗、柏格斯通、阿歌特、李家、佐伯、伍丁、昆廷韦拉斯）；`fn_140004381`（当前玩家势力）= 该表[`CDk4Game+0x7c`]。来源：`02-factions/01-forces.md`、`04-economy/07-investment-tribute.md` §2.1。重构写成本表 `player_force`。

### 剧本特例（原版写死在各系统的 `eScenario` 比较）

[确认] `CDk4Game+0x7c` 就是 `eScenario`：`fn_140004a1b` 直接读它；`fn_140010f46`（body `0x14009bc10`）→ `fn_140010131`（`0x1401cc0c0`）→ `fn_140004a1b(DAT_144296500)` → `fn_14000d486`（`0x140122f70`，恒等返回），所以「`fn_140010f46()==n`」也是剧本号比较。第一轮各组在别的系统里找到的剧本分支汇总如下。它们都是**剧本内容**，不是公式常量：

| 剧本 | 原版位置 | 效果 | 等级 | 来源文档 |
| --- | --- | --- | --- | --- |
| 0 拉斐尔 | `fn_140004a1b()==0`，委托报酬结算 | 报酬 `*3/2`（无符号 `div 2`） | [确认] 比较；「为何只给剧本 0」无文案 | `02-factions/04-letters-licenses.md` §2 |
| 0 拉斐尔 | `fn_14001077b` 宫殿军事加投资 | 最低投资额 `*8/10`，下限 1 | [确认] | `04-economy/07-investment-tribute.md` §4 |
| 0 拉斐尔 | `fn_14000c4d2` 遗迹/教会/城门首访 | 第二次加七海域影响力前 `v *= 2` | [确认] 比较（`fn_140010f46()==0`） | `01-world/09-remains-oldmaps-catalog.md` |
| 2 丽璐 | `fn_140010f46()==2`，卖价 | 卖出报价 `*105/100` | [确认] 比较 | `04-economy/02-market-price.md` §4 |
| 2 丽璐 | 同一判断，卖流行品 | 流行影响力 ×2 | [确认] 比较 | `04-economy/02`、`03` §2.6 |
| 6 蒂雅 | `0x14010dde8` 起，月初 | 玩家势力每月把 `gold/100` 交给宗主 `CForce+0x1E`（开局第 0 拍不收） | [确认] | `04-economy/07` §4.6、`02-factions/01-forces.md` |
| 0/1/2/3/6 | `fn_14000d6c0(town, eScenario)` | 据点港：0→86、1→87、2→88、3→89、6→{89,90,92,94,95,96}，4/5 无 | [确认] | `04-economy/07` §4 |
| 全部 | `fn_140008fbc`，表 `0x1405a0490` | 据点里程碑行 = World1 ? 2 : eScenario | [确认] | `04-economy/07` §4、`01-world/10-mappoint-kinds.md` |
| 全部 | `0x1402a8e10` 开局跳表 | 开局可点选行情的港（初始已知港） | case→港 [确认]；case = eScenario [推断] | `04-economy/08-price-intel.md` §2.5 |

另有几处按 `+0x7c >= 4` 分支（伍丁立绘 sheet、宝物分组），那是「World0 / World1」的二分，重构写成世界包属性，不是剧本特例。

[推断] 人物行 `+0x26` 为性别：拉斐尔 / 赫德拉姆 / 杏太郎 / 伍丁 = 0，丽璐 / 华梅 / 蒂雅 = 1。依据：与简介性别一致，且 `eSEX` 符号存在。枚举名（男/女）未从 `SexID.h` 恢复。

## 3. 实体与 ID

| 实体 | 原版计数 | 空值 | 稳定 key 规则 |
| --- | --- | --- | --- |
| `scenario` | 7 | `null`（原版 -1 只在 `legacy_codec`） | `vanilla:raphael`…`vanilla:tia`；`vanilla_index` = 原版 `eScenario` |
| `protagonist`（人物外键） | 原版 0–6 是提督，全表 235 | `null` | 人物 key（`vanilla:raphael_castro`）；「主角 = 0–6」改成本表显式字段，不靠区间 |
| `world_pack` | 原版 2 份 | 无 | `vanilla-world0` / `vanilla-world1` |
| `event_pack` | 原版 7 份 | 无 | `vanilla-ev0`…`vanilla-ev6`（源文件 `EV0`…`EV6`）；包清单带 `state_schema_version` |
| `scene_pack` | 原版 7 份 | 无 | `vanilla-sc0`…（源文件 `Sc0`…`Sc6`） |
| `animation_pack` | 原版 1 份共用 | 无 | `vanilla-eva`（源文件 `EVA.DK4`） |
| `flags` / `modifiers` | 原版散在 9 处比较 | 空 | 键名由消费方登记；值属本表（第 6 节） |

`NUM_SCENARIO=7` 只是原版内容包实例数（RTC + 选人 UI 7 槽循环，判界证据）。引擎按 `scenarios.yaml` 表长循环。主键是命名空间 key（`vanilla:raphael`、`mymod:new_hero`），整数下标由加载器紧凑分配；原版 `eScenario` 0–6 只作 `vanilla_index` 供导入/导出（DECISIONS D1）。空值用 `null`，不用 -1。

## 4. 算法 / 公式

本系统几乎没有战斗式公式。原版唯一硬编码分派写成命名参数：

```
world_pack_index = 0
if scenario_id >= world_pack_split:
    world_pack_index = 1
```

| 参数 | 原版默认 | 落点 |
| --- | --- | --- |
| `world_pack_split` | 4 | 只作提取规则（`legacy_codec`）；运行时每条剧本显式写 `world_pack` |

重构**不要**保留 `>= 4` 这条规则当引擎语义。

### 开局注入剧本特例

```
on Narrative.select_scenario(scenario_key, seed):
    sc = scenarios[scenario_key]
    Session.begin(sc.world_pack, sc.event_pack, sc.protagonist, seed)
    for c in sc.modifiers:                         # 第 2 节「剧本特例」表
        Modifiers.register(key=c.key, stage=c.stage, order=c.order,
                           value=c.value, source="scenario:" + sc.key,
                           scope=c.scope)          # 默认 scope = 玩家势力 / 主角
    Scenario.flags = sc.flags                      # 只读，Scenario.flag(key) 查询
    emit ScenarioSelected { scenario_id: sc.key, world_pack, event_pack, protagonist_id: sc.protagonist }
```

- 数值型差异（×3/2、×105/100、×2）进修正器注册表，消费方只按 key 取值：`Modifiers.eval("request.reward_gold", ctx)`。取整按 D2 顺序 `base → add → mul → clamp → override`，`mul` 用 `{num, den}` 整数截断，复刻原版 `*3/2`、`*105/100`。
- 开关型差异（附庸税）进 `flags`，公式包用 `enabled_by: scenario_flag.<key>` 读。军事八折是乘数，按 D5 走修正器 `invest.military_min_fee_pct`（`mul 8/10`；下限 1 由经济公式 `floor` 管，`04-economy/07`），不设旗标。
- 内容表型差异（据点港、里程碑表、初始已知港）是剧本包里的数据，由拥有方系统在 `ScenarioSelected` 时读取本表字段，不进修正器；开局舰队由舰队在 `ScenarioSelected` 时按 `scenario_id` 查 `narrative/starting_fleets.yaml`（§14-12）。
- 修正器来源统一 `scenario:<key>`，存档不存修正器值（读档时按剧本重新注入）。

画廊公式（点数、花费、首通奖励）全部归 `05-gallery-unlock.md`。本表只在通关时由会话层发 `ScenarioCleared`。

## 5. 耦合分析

原版把「选剧本」焊在三处：

1. **世界 DLL 文件名** `World%d` 与 `scenario_id >= 4` 分支——内容选择混进 `CWorldData` / `CPerson` / `CTown` 初始化。
2. **事件文件名** `EV{n}.dk4` 与 `Sc{n}.dk4` 用同一整数下标，逻辑上等于 `scenario_id`。
3. **其它系统里的 `if scenario==n`**（原则 7 点名的病）：第一轮已找到 9 处（第 2 节「剧本特例」表），分布在经济、势力、世界三个系统。重构验收标准：除叙事与 UI 选人外，零处读取 `scenario_id`；校验器拦截公式包里出现剧本号比较。

允许的依赖方向：叙事 → 查询任意系统 + 发命令 + 注册修正器贡献。禁止反向。

跨剧本画廊读「其它剧本是否通关」：通关位是 profile 状态（profile 订阅 `ScenarioCleared` 写位，画廊只读 `Profile.cleared`，词表 §14-24），不要让人物系统去读 `scenario_id`。

## 6. 解耦后的配置

```yaml
# config/narrative/scenarios.yaml
schema: dk4.narrative.scenarios
version: 2

scenarios:
  - key: vanilla:raphael            # 主键；整数下标由加载器分配
    vanilla_index: 0                # 原版 eScenario，只给导入/导出
    name_i18n: ui.scenario.raphael.name
    blurb_key: ui.scenario.raphael.blurb
    quote_key: ui.scenario.raphael.quote
    recommended_for_first_play: true
    protagonist: vanilla:raphael_castro       # 原版 person 0
    player_force: vanilla:castro              # fn_140009df9 表[0] = 0
    world_pack: vanilla-world0
    event_pack: vanilla-ev0              # 版本在包清单 pack.yaml（02 第 6 节）
    scene_pack: vanilla-sc0
    animation_pack: vanilla-eva
    home_gallery_entry: vanilla:gallery_raphael   # 05；原版条目号 = 剧本号是巧合
    new_game:
      birthday_required: true
      # 开局舰队：narrative/starting_fleets.yaml 以本剧本 key 查行（05-ships-fleet/05，词表 §14-12）；本表不写舰队 / 船体
      starting_gold: null                     # 未还原
    initial_known_towns: [vanilla:lisbon, vanilla:ceuta]     # 04-economy/08 case 0
    base_towns: [vanilla:pensacola]                          # 04-economy/07：86
    map_point_unlock_table: vanilla:map_point_unlock_0     # 表 0x1405a0490 行 0；01-world/10 unlock_tables（04-economy/07 称据点里程碑）；原版只对据点港生效
    modifiers:                                # 开局注入注册表，source = scenario:raphael
      - { key: request.reward_gold,          stage: mul, value: { num: 3, den: 2 } }    # 02-factions/04
      - { key: invest.military_min_fee_pct,  stage: mul, value: { num: 8, den: 10 } }   # 04-economy/07；D5 不用旗标
      - { key: factions.influence_gain,      stage: mul, value: { num: 2, den: 1 }, when: { source: remains } }   # 01-world/09 首访第二次

  - key: vanilla:helmut
    vanilla_index: 1
    protagonist: vanilla:helmut_bergstone
    player_force: vanilla:bergstone           # 2
    world_pack: vanilla-world0
    event_pack: vanilla-ev1
    base_towns: [vanilla:abkhazia]            # 87
    map_point_unlock_table: vanilla:map_point_unlock_1
    modifiers: []

  - key: vanilla:lulu
    vanilla_index: 2
    protagonist: vanilla:lulu_agate
    player_force: vanilla:agate               # 1
    world_pack: vanilla-world0
    event_pack: vanilla-ev2
    base_towns: [vanilla:ralistat]            # 88
    map_point_unlock_table: vanilla:map_point_unlock_2
    modifiers:
      - { key: trade.sell_price_pct,         stage: mul, value: { num: 105, den: 100 } }   # 04-economy/02
      - { key: factions.influence_gain,      stage: mul, value: { num: 2, den: 1 }, when: { source: fashion_sale } }   # 04-economy/02、03；公式里不再乘

  - key: vanilla:tia
    vanilla_index: 6
    protagonist: vanilla:tia_chaska
    player_force: vanilla:quintin_veras       # 21
    world_pack: vanilla-world1
    event_pack: vanilla-ev6
    base_towns: [vanilla:tamsui, vanilla:saint_malo, vanilla:benghazi, vanilla:busan, vanilla:edo, vanilla:caracas]   # 89 90 92 94 95 96
    map_point_unlock_table: vanilla:map_point_unlock_2     # World1 ? 2；原版只有剧本 6 真正用到
    flags:
      pay_suzerain_tribute: true              # 04-economy/07：月初 gold/100 交宗主
    modifiers: []
  # 剧本 3–5 同 schema，略；4 / 5 无据点港
```

字段归属：`flags` / `modifiers` 的**值**属本表；**键名**由消费方系统登记（修正器键表、`scenario_flag.*` 清单），加载期校验键存在，拼错拒绝启动。`base_towns` / `initial_known_towns` 的数据格式由经济定；开局舰队只在 `narrative/starting_fleets.yaml`（舰队拥有，按剧本 key 引用本表），`map_point_unlock_table` 指向 `01-world/10` 的 `unlock_tables`，本表只持 key。表里出现的 key 示意形状，真实 key 由各内容表生成。

主角六维**不**复制到本表，人物系统是唯一来源。加载顺序（与 `05-config-architecture.md` 一致）：caps / formulas / 内容表 → 本表（外键校验）→ 用 `world_pack` 选世界包 → `event_pack` 交给事件解释器 → 注入修正器。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗 / 航海改代码：

- 某剧本换世界包（World0 ↔ World1，或模组包）
- 某剧本换事件包 / 场景图包
- 主角换成另一个人物 key（只要人物表有这一行）
- 增加第 8 个剧本（新 key + 新包；无需整数）
- 是否要求生日、推荐新手标记
- 某剧本的修正器贡献（委托 ×1.5 挪给别的剧本、卖价 +5% 改 +10%）与 `flags`
- 据点港、初始已知港、里程碑表；开局舰队改 `narrative/starting_fleets.yaml` 对应行

需要人物系统配合的只有：新主角必须在 `persons.yaml` 有定义。需要世界系统配合的只有：新世界包符合同一 schema。新修正器键需要消费方先登记。

## 8. 对外接口

事件（本系统发出）：

| 事件 | 载荷 | 谁听 |
| --- | --- | --- |
| `ScenarioSelected` | `scenario_id`, `world_pack`, `event_pack`, `protagonist_id` | 存档会话、世界加载、事件解释器、经济 / 舰队 / 人物开局（读本表内容字段）、UI |

开局种子（生日等纯值）随 `Narrative.select_scenario(scenario_key, seed)` 交给会话层，不另发事件（旧名 `NewGameSeeded` 并入 `ScenarioSelected`）。

终局不由本系统发事件：脚本命令 `Session.end_scenario(outcome, cause)`（outcome = `cleared | game_over{ending_id} | to_title`；脚本触发的 `cause = scripted`），会话层据此发 `ScenarioCleared { scenario_id }`（profile 订阅并写通关位，画廊只读 `Profile.cleared`；词表 §14-24）或 `ScenarioFailed { scenario_id, cause, end_code? }`（`cause` 语义枚举；`end_code` 仅原版导入用；`11-progression/03`）。载荷名按词表 §14-3。舰队全灭 / 解散的 Game Over 由进度系统调同一命令，不经叙事。

查询（本系统提供）：

```
Scenario.current() -> scenario_key
Scenario.flag(flag_key) -> bool             # 公式包 enabled_by: scenario_flag.* 用
Scenario.player_force() -> force_key
Scenario.base_towns() -> [town_key]
```

查询（本系统只读别人）：

```
Characters.get(person_key) -> PersonDef
World.pack_exists(world_pack) -> bool
Events.pack_exists(event_pack) -> bool
Profile.cleared(scenario_id) -> bool          # save / profile 拥有，订阅 ScenarioCleared 写位（§14-24）；05 只读
```

命令：

```
Narrative.select_scenario(scenario_key, seed) -> Session     # 别人向本系统
Modifiers.register(contribution)                             # 本系统向注册表，开局 / 读档
Session.end_scenario(outcome, cause)                         # 脚本经本系统向会话层
```

只传 ID 与纯值。禁止传 `World*`、`CEvent*`、DLL 句柄。

## 9. 未核实字段

| 位置 | 现状 | 禁止 |
| --- | --- | --- |
| 开局 `town_id` | 由开局舰队模板驻港给出（`fleets/templates`）；本表不重复 | 不要用简介里的「葡萄牙 / 瑞典 / 荷兰」猜港并写进本表 |
| 开局金币、货物 | 人物 `+0x18` 两个 `uint32` [推断] 是经验初值 | 不要把拉斐尔的 `100, 100` 写成「确认的 100 金币」 |
| 初始已知港 case 号 = `eScenario` | 7 支与七提督出身对上，参数来源未追到写入点 | `initial_known_towns` 标 [推断] |
| 委托 ×3/2 只给剧本 0 的设计意图 | 比较已钉，无文案 | 不要写成「新手难度」 |
| 人物 `+0x27` 起若干字节 | 拉斐尔 `04 02 02 00 00 01`，各主角不同 | 不要当成 force / region / job |
| `SGameSeed` `0x78` 字节 | 构造写 `+0x70`，拷贝 `+0x8/+0xc/+0x58/+0x5c` | 除「含生日」外字段一律 unknown_bytes |
| 存档 `+0x20` 与 id 6 | 注释只写到伍丁 | 读档实现按 uint32 0–6 测，不要根据注释丢掉蒂雅 |
| World1 相对 World0 的人物未命名字节 | 例如 Raphael `+0x30` World0=0、World1=1 | 进 `unknown_bytes`，不要猜「世界包标记」 |

提取器只输出确认字段；残余进 `unknown_bytes`。
