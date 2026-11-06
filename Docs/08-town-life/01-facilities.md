# 港口设施

城镇生活的入口层。玩家入港后看到的是设施图，不是贸易公式、船数值或人物六维。

设施只做两件事：决定「这座港能点开什么」，以及把点击翻译成对其他系统的命令。价格、船耐久、合约金、人物属性一律不在本系统计算。

## 1. 职责边界

本系统管：

- 设施目录（命名空间 key；地图点 kind → 处理器 + 参数）
- 每座港开放哪些设施（原版：每港热点位域，初值来自港口内容表）
- 设施内菜单 → 命令的映射
- 广场作为入港枢纽：人群事件、流行展示、情报入口
- 旅馆休息的**资费与天数**（恢复效果发给人物系统）
- 造船厂修理等待日、木工折扣的**入口参数**（折扣率查询人物修正器）

本系统不管：

| 事项 | 归属 |
| --- | --- |
| 市价、买卖、特产品、投资金额曲线 | economy（`CTrade`、`CInvestmentPadView`） |
| 船型、耐久、改装件、水粮补给量计算 | ships / fleets |
| 招募谁、六维、岗位技能 | characters |
| 合约金、份额、友好度、势力关系 | factions |
| 委托 kind / 生成规则与完成判定 | 同目录 `02-requests-schemes.md`；计略见 `04-schemes-rhetoric.md` |
| 小游戏规则与奖励 | 同目录 `03-minigames.md` |
| 港口坐标、首都标记、`culture_id`（港口 `+0x25`） | world |
| 文案字符串 | ui / localization |

交易所在原版是 `CTrade`，挂在广场按钮上，但会话与货格属于经济系统。本系统只发 `OpenExchange { town_id, fleet_id }`。

## 2. 原版事实

### 2.1 设施对象是空壳，玩法在场景

`[确认]` `CBar` `CInn` `CGuild` `CPalace` `CPort` `CShipyard` 的还原 cpp 只有 ctor / dtor，没有菜单逻辑。`CBar` 写 `+0x14=0`、`+0x18=5`；`CGuild` 写 `+0x1c=0`。来源：`srcdata/source/game/town/CBar.cpp` 等。

真正的入港场景是：

| 类 | 来源 | 作用 |
| --- | --- | --- |
| `CTownScene` / `CTownSceneView` | `town/CTownScene.cpp` `CTownSceneView.cpp` | 港口地图；`eMapPointKind` 槽 0–11 |
| `CPlazaView` / `CPlazaScreen` / `CPlazaButton` | `town/CPlazaView.cpp` 等 | 广场枢纽与设施按钮 |
| `CShopButton` | `town/CShopButton.cpp` | 广场上的店铺按钮 |
| `CMapPointView` | `town/CMapPointView.cpp` | 地图热点；ctor 写 `+0xa8=0xd` |
| `CTownInformationView` | `town/CTownInformationView.cpp` | 港口情报面板 |
| `CPlazaSylphid` | `town/CPlazaSylphid.cpp` | 广场角色动画，循环 10 槽 |

`CMpDock`（体积 `0x10`）在带 RTC `cDock`+`asShipLoad` 的函数里构造（`fn_14016cc40.cpp`）。热点 kind 3 码头分派到 `CPort`，`CMpDock` `[推断]` 是其内部子对象。旧稿「海域地图停泊点」无证据，已撤回（`01-world/10-mappoint-kinds.md` 2.3）。

### 2.1a 地图热点 → 设施类分派

`[确认]` `CTownScene::vfunc_02`（body `0x14026d160`，thunk `0x140006a46`；反编译失败，按 `disasm_sc/text_full_sc.txt` 读；RTC 局部 `cShipyard,cRemains,cGuild,cInn,cPalace,cBar,cPort,cTrade,cPlazaView`）。取当前热点种类（`fn_1400099f3(&0x144296500)`），`> 0xB` 不分派；否则查跳表 `0x14026d92c`（12 项）在栈上构造设施对象、调它的入口函数、析构：

| `eMapPointKind` | 构造的类（ctor thunk） | 入口函数 thunk | 备注 |
| --- | --- | --- | --- |
| 0 | `CPlazaView`（`0x14000254a`） | `0x14000fd8a` | 广场 |
| 1 | `CTrade`（`0x140013688`） | `0x14000e0e3` | 交易所；先查/置全局旗标位 `0x2d`，写 `this+0x50` |
| 2 | `CShipyard`（`0x140001f05`） | `0x140010eb0` | RTC `cDock,sMenuOption` |
| 3 | `CPort`（`0x1400087ec`） | `0x140004cb4` | 返回值 `<2` 时可能直接离开场景（出港） |
| 4 | `CBar`（`0x140008260`） | `0x1400035cb` | |
| 5 | `CGuild`（`0x14000a3bc`） | `0x140009af7` | RTC `sArrivalDayNameList` |
| 6、7 | `CPalace`（`0x14000fb28`） | `0x14000c5fe` | 两种热点同一类 |
| 8 | `CInn`（`0x1400044b2`） | `0x14000b8e3` | |
| 9、10、11 | `CMpRemains`（`0x140006a73`） | `0x14000f803`（传入 kind） | 遗址 / 教会 / 城门。`remain_id = STown+0x26`；遗迹未发现（`fn_14000c6f3()==0`）走首访 `fn_14000c4d2`（加七大海域影响力，表 `.data 0x140686650`）；已发现走 `fn_14000eacf`（`nMonth/nDay/nTime`），按 9→`0x140008e9a`、10→`0x140002171`、11→`0x14000e840` 分三支仪式（05 篇 2.3） |

当前种类 `== 0xD` 时不走跳表，改按 `0xC..0x10` 处理系统按钮（与 `CTownSceneView::vfunc_13` 的 12–16 对应；`CMapPointView` ctor 写 `+0xa8=0xD`）。`[推断]` `0xD` 是「未选热点」哨兵。

结论：
- `[确认]` 12 种热点名（exe getter `0x1401417a0`，字符串 `0x14055d5a4` 起，8 字节步长）：广场、交易所、造船厂、码头、酒馆、行会、王宫、总督府、旅馆、遗址、教会、城门。
- `[确认]` 热点只分派到 9 个类。**教会（10）没有自己的设施类**，与遗址（9）、城门（11）共用 `CMpRemains` 入口，按 kind 分支。
- `[确认]` `CChurchGameView` 不由教会热点直接构造：它的 ctor（`0x140007045`）只经 `0x14000dd32` 被小游戏分派函数（thunk `0x14000847c`，body `0x1400e1970`）调用；该函数用 `fn_140005c13` 取**脚本指令字低位**作游戏号（`fn_140012ed1(script,0)` 取指令字，再 `fn_1400e2a70(&w,0,7)` 取位段；不是随机数），`cmp 6/ja` 后查跳表，教会游戏是第 5 号；调用方是事件脚本解释器 `fn_1400020fe`（RTC `sScript`），指令 `0xC1`。见小游戏文档 2.0。
- `[确认]` 6/7 共用 `CPalace`，只差称号；王宫/总督府由每港热点位域决定（数据上互斥，王宫 35 港 ≠ 首都 21 港），不是由首都标记现场判断（`01-world/10-mappoint-kinds.md` 2.5）。

### 2.2 广场点击编号

`[确认]` `CPlazaView::vfunc_13`（`CPlazaView.cpp`）：

- `4 <= id < 9`：下标 `id-4`，先调 `+0x80` 子对象（`CPlazaScreen`）的 `fn_140004ba1`，再把 `CPlazaView+0xf0` 起 5 指针中的第 `id-4` 个交给 `fn_14000d01c`（RTC `aeItemTable`；对象 `+0xa8 == 0x6A` 时直接返回）。ctor 循环 `<5` 清这 5 个指针。`[推断]` 5 个槽是广场零售店按钮（`CShopButton` ctor 写 `+0xa8=0x6A`）。
- `id == 1` `2` `3` `0xA`：另四条分支。`2` → `fn_14001361a`（用 `0x3e8` 随机判定，文案号 `0x341/0x342`）后查 `fn_140166580`（RTC `eTownShare`，循环 `<3`）；`3` → `fn_140011e19`（RTC `nLoadTotal,asShipLoad`，货舱为空不进入）。`[推断]` 3 与「散发交易品」有关；各分支玩法归属见广场文档。

`[确认]` `CTownSceneView::vfunc_13`：

- `0 <= id < 0xC`：12 个 `eMapPointKind`（`NUM_MAPPOINTKIND=12`，`MapPointID.h`）
- `0xC <= id < 0x11`：再 5 个点击号（12–16）

`[确认]` 两段都转同一个 `fn_140014d58`。0–11 是 12 个地图热点（分派见 2.1a），12–16 是场景系统按钮，不是 17 种设施。

`[确认]` 每港开哪些热点 = `CTown+0x14`（u32 位域，第 k 位 = kind k）。测试 `fn_140002806(town, kind)` = `(CTown+0x14 & (1<<kind)) > 0`。初值由 `CTown::vfunc_05` 从 DLL 记录 `STown+0x48` 拷入。运行时改位两处：发展度解锁 `fn_140008fbc`（`.rdata` 表 `0x1405a0490`，4 行 × 6 项 `{need_a, need_b, kind}`，首个不满足即停）；`fn_140001f87`（`[推断]` 事件改港数据）。来源与统计见 `01-world/10-mappoint-kinds.md` 2.5。

`CPlazaScreen` ctor 写死四对整数：`(0x96,0xDA)` `(0x8A,0xBA)` `(0x3AB,0x287)` `(0x3C0,0x316)`。`[推断]` 广场布局坐标，不是玩法规则。

### 2.3 文案里的设施与菜单

SC `mesfile_sc.txt`（用户检索词：广场 / 酒馆 / 旅馆 / 公馆 / 行会 / 造船 / 码头 / 教会 / 宫殿 / 委托 / 计略 / 投资 / 合约 / 情报 / 流行）：

| 设施 | 文案用法 | 标记 |
| --- | --- | --- |
| 广场 | 「提督。好像广场上聚集了一群人。」（多条变体，offset `0x12c` 起） | `[确认]` |
| 酒馆 | 迎客、倒酒、情报、招募；「如果参谋在，可在酒馆里拟定策略。」`0xc237` | `[确认]` |
| 旅馆 | 「要１枚金币。好的，您请休息吧。」「住１晚１枚金币，要住几天？」`0xba97` `0xbab7` | `[确认]` |
| 造船 / 船坞 | 买船入坞、修理、改造；「新船放在船坞里。」`0x7df7` | `[确认]` |
| 码头 / 港 | 「在%s停靠吗？」补给水粮比例 `0x81af` `0x822f` | `[确认]` |
| 同业公会 | 委托交付点「送到%s的同业公会」`0x15267`；停战入口「前去『同业公会』要求停战吗？」`0x18463` | `[确认]` |
| 教会 | 热点名「教会」（kind 10，exe 字符串）；仪式文案是消息号 `0x58a`–`0x5bf`（mesfile 块 `0xbcd3`–`0xc0b3`），分属遗址 / 教会 / 城门三支；「来遗迹这里有事吗？」是消息 `0x5a2`，城门支（kind 11）的未开放回复 | `[确认]` 入口是 `CMpRemains` kind 10（2.1a）；`CChurchGameView` 是另一条小游戏路径 |
| 王宫 / 总督府 | 「拿着这个到小城市的王宫或者总督府去」`0x7d47`；签合约、份额 | `[确认]` |
| 交易所 | 「如果是大宗买卖，请去交易所。」`0x5fc3`；主计长杀价 `0xc237` | `[确认]` 入口在本系统，算法在 economy |
| 公馆 | SC mesfile **无**「公馆」二字 | `[未知]` 是否为总督府别译 |
| 行会 | 热点名「行会」是 kind 5（exe 字符串 `0x14055d5cc`），类 `CGuild`；文案里称「同业公会」 | `[确认]` 同一设施 |

旅馆恢复：`0xc0b3`「在这里船员身体和精神都能够恢复。」与后面的船木工 / 船医 / 大厨说明同块，是**舱室**说明，不是旅馆证据。旅馆真实规则见 `05-inn-church-palace.md` 2.2（`CInn` 入口 `0x14000b8e3`：单晚 1 金；长住 3/10/30 晚；每晚扣 1 金、走 1 天）。`[确认]` 身体 / 精神是人物状态，本系统只发休息命令。

木工折扣：`0x7fcb`「如果那个人肯帮忙，修理费用就少算%s％吧……。」`0xc337` 改造费同样「减少%s％」。`[确认]` 折扣百分比来自在队木工，本系统不读六维。

修理等待：`0x8037`「到修理完成前，还得等一天。」改造：`0xc39f`「到改造完成要等２天。」另有「要等%s天」`0xc3c3`。`[确认]` 修理默认 1 日；改造默认 2 日，也可走 `%s` 日。

补给：`0x81cb` 水手疲劳提示休息；`0x822f`「改变供水和供食物的比例吗？」；`CSupplyRateView`（`game/CSupplyRateView.cpp`）是比例 UI。`[确认]` 码头打开补给，数量公式在 ships。

### 2.4 每港是否缺设施

`[确认]` `GetFixTownProc` 记录 0x84 字节（`CTown::vfunc_05` RTC `r_sFixTown` 132b）。序列化循环：3 个份额槽（DLL `+0x4C` → `CTown+0x18`）、2 个字/dword、5 个特产品、**19** 个 word（= `NUM_TRADINGITEMKIND`）、2 字节、`+0x74/+0x75`。其中 `+0x48` u32 就是热点位域初值（写 `CTown+0x14`，见 2.2）。

`[确认]` 小城市也有王宫/总督府（`0x7d47`）。首都标记是 world 的 `is_capital`（伦敦等 21 座，见 `04-id-registry.md`），只改海域图标；王宫/总督府看位域第 6/7 位。

`[确认]` 原版**不是**「97 港设施全开」：每港热点由位域决定。例：伦敦 `0x17f`（广场、交易所、造船厂、码头、酒馆、行会、王宫、旅馆），布里斯托尔 `0x19f`（总督府替王宫）；江户只有码头 + 王宫；码头 97 港全有；王宫 / 总督府从不同时置位，4 港两者皆无。World0 初值从不置 9/10/11；World1 置 24 个遗迹港各一位（`01-world/10-mappoint-kinds.md` 2.5、`11-remains-puzzle-system.md` 2.11）。发展度上涨会按解锁表补开热点。`eTownKind` 在海域图循环上界为 2（`CSeaAreaMapView` `eTownKind >= 2`），对应首都 / 非首都两种画法，不是「有无宫殿」。

`[确认]` `CTown::vfunc_05` 3 个两字节槽（`+0x18/+0x19`）：源值 `<=0` 时写 `0x1A, 0`。槽 = 合约份额 `(force_id, share)`（`01-world/02-towns.md` §2：`fn_1400092af` 扫 3 槽取势力份额；`0x1A` = `NUM_FORCE + 1` 空值）。旧稿的「流行槽」读法作废：流行在 `CTown+0x70`。不是设施开关。

### 2.5 广场人群与流行

`[确认]` 入港台词「广场上聚集了一群人」多主角变体（`0x12c`–`0x217`）。

`[确认]` 广场可散发交易品：传教士在队时「在广场上还能散发交易品。」`0xc1fb`。

`[确认]` 零售店招牌听流行：「请选择零售店的招牌，就可以听取流行情报。」`0x1143`；阿科布把流行带到别的广场 `0x6fa3`。流行目录属 factions（`eTownFashion=26`），广场只展示与触发。

### 2.6 酒馆配角

`[确认]` RTC `eBarmaid`，循环 `eBarmaid >= 0x15`（`fn_14000448f`，`fn_140032ba0.cpp`），喝酒时用它按 `town_id` 找本港女招待。`[推断]` 女招待表长 21。`CBar+0x18=5` 在每次喝酒时重写（05 篇 §2.6），不是同屏人数，语义 `[未知]`。

酒馆酒价文案：`0x733`「需要１０枚金币。5枚金币。」`0x7bf`「收您２０枚金币。10枚金币。」`[确认]` 自己喝 5（普通）/10（高档），请女招待 10/20；高档 = 某玩家量 ≥3000 且发展度 ≥3000（05 篇 §2.6）。

招募入口：`0x7627`「有人要乘坐提督的船……让他上船吧？」`[确认]` 酒馆发招募命令，名单在 characters。

划拳：`0x753f`「喂，我们划拳吧。」不是 `C*GameView` 类，见小游戏文档「酒吧划拳」。

### 2.7 世界对象持有

`[确认]` `CWorld` ctor 在 `+0x182a0` 构造 `CSchemeElement`，`+0x19f48` 构造 `CRequestElement`（`world/CWorld.cpp` 调 `fn_1402a65c0` / `fn_1402a64d0`）。委托/计略表挂在世界对象上，入口仍是公会/酒馆。

## 3. 实体与 ID

| 实体 | 原版符号 | 计数 | 空/非法 | 来源 |
| --- | --- | --- | --- | --- |
| 设施 | 无独立 `eFacility` | 12 个 kind 分派到 9 个类 | — | `CTownScene::vfunc_02` 跳表 |
| 地图热点种类 | `eMapPointKind` | 12 | `INVALID_MAPPOINTKIND=-1`；`0xD` 未选 `[推断]` | `MapPointID.h` / RTC |
| 城镇种类 | `eTownKind` | `[未知]` 整型；循环上界 2 | `INVALID_TOWNKIND=-1` | `TownKindID.h` |
| 城镇通行 | `eTOWNACCESS` | `[未知]` | `INVALID_TOWNACCESS=-1` | `TownaccessID.h`；`STownAccess` |
| 流行 | `eTownFashion` | 26 | `[推断]` 26 为空；流行槽在 `CTown+0x70`，`+0x18` 是份额槽（`01-world/02-towns.md`） | `TownFashionID.h` |
| 女招待 | `eBarmaid` | `[推断]` 21 | — | RTC 循环 `0x15` |
| 港口 | `eTown` | 97 | `INVALID_TOWN=-1` | world |

重构设施（原版无枚举）。按 DECISIONS D1：主键是命名空间 key（`vanilla:plaza`），整数只在加载时紧凑分配、仅运行时内部用；原版热点 kind 号只出现在 `ids/legacy_codec.yaml`。按 D9：设施 = 地图点 kind → 处理器 + 参数，一个处理器可服务多个 kind。

| key | 处理器（原版类） | 原版 kind | 中文 | 默认命令目标 |
| --- | --- | --- | --- | --- |
| `plaza` | `plaza`（`CPlazaView`） | 0 | 广场 | 本系统（人群/流行/情报） |
| `exchange` | `exchange`（`CTrade`） | 1 | 交易所 | economy |
| `shipyard` | `shipyard`（`CShipyard`） | 2 | 造船所 | ships |
| `dock`（旧稿 `port`） | `port`（`CPort`） | 3 | 码头 | ships（补给/入坞/出港） |
| `bar` | `bar`（`CBar`） | 4 | 酒馆 | characters / 计略 / 本系统酒价 |
| `guild` | `guild`（`CGuild`） | 5 | 同业公会 | 委托 / factions 停战入口 |
| `palace` | `palace`（`CPalace`） | 6 | 王宫 | factions 合约；economy 投资入口 |
| `governor` | `palace`（`CPalace`） | 7 | 总督府 | 同上，只换 `title_key` |
| `inn` | `inn`（`CInn`） | 8 | 旅馆 | characters（休息） |
| `remains`（旧稿 `ruins`） | `rite_site`（`CMpRemains`） | 9 | 遗址 | 本系统（首访影响力 / 仪式） |
| `church` | `rite_site`（`CMpRemains`） | 10 | 教会 | 同上；小游戏另走事件脚本 |
| `gate` | `rite_site`（`CMpRemains`） | 11 | 城门 | 同上（3 日窗口、走 4 天） |

地图热点 kind → 设施已还原（2.1a），写进 `map_point_kinds`。广场点击号 4–8 是 5 个零售店槽 `[确认]`（`06-plaza-intel-adman.md` 2.1）；1 离开、2 寻求支持、3 发放交易品 `[确认]`，`0xA` `[未知]`。配置里单独放 `click_id`，禁止在代码里写死 `if (click==4)`。

## 4. 算法 / 公式

设施层公式极少。全部进 `config/formulas/town.yaml`，禁止在场景代码里写立即数。

设施内每个菜单项都是 D9 的通用 `Action`（town / 海上 happening / 叙事共用一个执行器）：

```
Action {
  cost:   { gold, days }            # days 交给 Clock.advance_days(n)；原版 1 天 = 48 tick，逐拍跑完整调度（fn_14000b122）
  guard:  [predicate]               # 例：has_gold(n)、post_holder(fleet, key) != null、rite_day_open(site)
  roll:   formula_ref | null        # 命名公式，走命名随机流
  effects:[command]                 # 只发命令，不直接改别系统状态
  repeat: { n, interruptible }      # 旅馆长住 = repeat n 晚、可打断
}
```

```
inn_cost = inn_gold_per_night * nights
inn_gold_per_night     # 原版默认 1
nights ∈ {1} ∪ inn_long_stay_options   # 原版 1 / 3 / 10 / 30，见 05 篇

repair_wait_days       # 原版 1（fn_14000b122(…,1)，01-world/07 2.4）
# 其他走日：旅馆每晚 1、遗址/教会仪式 1、城门仪式 4、寻求支持 1；交易所成交 1 天归 economy
remodel_wait_days      # 原版默认 2；文案另有 %s 日
repair_price_quoted    = Ships.quoteRepair(fleet_id)          # 外系统
repair_price_charged   = Modifiers.eval("ship.repair_cost", { base: repair_price_quoted, fleet_id })
# 船木工贡献由 characters 以 post_holder(fleet, carpenter) 注册到该 key；
# 文案「少算%s％」= mul 段贡献。本系统不读岗位、不读技能值。

bar_drink_cost = bar_drink_price[tier]   # 原版 normal 5 / high 10；请客 10 / 20，见 05 篇
```

休息效果不在本系统：

```
Actions.run(inn.rest_single | inn.rest_long, { town_id, fleet_id, nights })
  → Characters.apply_rest { fleet_id, source: inn_single | inn_long }   # 每晚一次
```

`inn_rest_params` 原版已还原（见 05 篇 2.2：单晚与长住两套概率），参数归人物系统 `formulas/vitals.yaml` 的 `rest.tables.inn_single` / `rest.tables.inn_long`（`03-characters/07`），本系统只传 `nights` 与 `source`。

补给比例 UI 打开后：

```
TownLife.OpenSupply { fleet_id, town_id }
  → Ships.restock(fleet_id, water_ratio, food_ratio, town_prices)
```

比例与桶数属于 ships；本系统只提供「打开码头补给」命令。

人群事件：

```
on ArrivedAtTown:
  if rand() < plaza_crowd_chance:   # 原版触发率 [未知]
    emit PlazaCrowdGathered { town_id }
```

原版是否每次入港都喊「聚集了一群人」，未从 cpp 还原，只确认文案存在。

## 5. 耦合分析

原版硬编码：

- 广场按钮直接 new `CTrade` / 造船 UI / 人物招募 UI
- 旅馆直接改人物身体/精神字节
- 造船修理读在队人物是否「船木工」（岗位 13 `carpenter`，见 `03-characters/03-abilities-skills-jobs.md`）
- 宫殿直接改份额与合约（factions 状态）
- 公会直接扫 `CRequest` 表
- `NUM_MAPPOINTKIND=12`、广场 5 槽写死在 ctor；每港热点是 `CTown+0x14` 整数位域
- 酒价 5/10/20、住宿 1 金币写在 mesfile 与代码立即数两侧

允许的依赖方向（`03-decoupling-principles.md`）：

```
town-life → world.town_id
town-life → characters（`Modifiers.eval(key)`、`Fleets.post_holder(fleet, post_key)`；旅馆经 `Actions.run` 调 `Characters.apply_rest`；发 Recruit 命令）
town-life → economy（OpenExchange / `Invest.pay(town_id, force_id, gold, reason)`）
town-life → ships（OpenShipyard / OpenSupply / QuoteRepair）
town-life → factions（OpenContract / 停战入口）
town-life → 本目录 requests / schemes / minigames
```

禁止：economy 改设施表；combat 写旅馆；ui 写存档设施状态。

## 6. 解耦后的配置

`config/town/facilities.yaml` — 本系统唯一设施表。

```yaml
schema: dk4.town.facilities
version: 1

# D1：key 为主键；无整数 id。D9：处理器 + 参数；一处理器服务多个设施
facilities:
  - key: vanilla:plaza
    handler: plaza
    name_key: ui.facility.plaza
    original_class: CPlazaView
    actions: [hear_fashion_intel, seek_support, distribute_goods, open_price_intel]   # 定义见 06 篇
  - key: vanilla:exchange
    handler: exchange
    original_class: CTrade
    actions: [open_exchange]
  - key: vanilla:shipyard
    handler: shipyard
    original_class: CShipyard
    actions: [buy_ship, sell_ship, repair, remodel, name_ship]
  - key: vanilla:dock                 # 与 01-world/10 map_point_kinds 同名（旧稿 port）
    handler: port
    original_class: CPort
    actions: [supply, dock, undock]
  - key: vanilla:bar
    handler: bar
    original_class: CBar
    actions: [buy_drink, recruit, open_scheme, listen_bar_rumor]   # 定义见 05 / 04 篇
  - key: vanilla:guild
    handler: guild
    original_class: CGuild
    actions: [list_requests, deliver_request, send_negotiation, ask_ceasefire]
  - key: vanilla:palace
    handler: palace
    title_key: ui.facility.palace
    original_class: CPalace
    actions: [open_contract, open_investment, donate_brand]
  - key: vanilla:governor
    handler: palace              # 同处理器，只换称号
    title_key: ui.facility.governor
    original_class: CPalace
    actions: [open_contract, open_investment, donate_brand]
  - key: vanilla:inn
    handler: inn
    original_class: CInn
    actions: [rest_single, rest_long]
  - key: vanilla:remains
    handler: rite_site           # CMpRemains；参数见 05 篇 rite_sites
    params: { site: vanilla:remains }
    original_class: CMpRemains
  - key: vanilla:church
    handler: rite_site
    params: { site: vanilla:church }     # CChurchGameView 由事件脚本 0xC1 号 5 进入，不在本菜单
    original_class: CMpRemains
  - key: vanilla:gate
    handler: rite_site
    params: { site: vanilla:gate }
    original_class: CMpRemains

# 每港开哪些设施 = town 状态里的 key 集合（原版 CTown+0x14 位域）。
# 初值与发展度解锁表由 01-world/10-mappoint-kinds.md 的 town_map_points.yaml 提供；本表不重复。
availability:
  source: town_map_points      # 不再有 mode: all

click_map:            # 广场点击号 → 动作，见 06 篇 plaza.yaml；这里只留场景系统按钮
  town_scene:
    extra_click_lo: 12
    extra_click_hi: 16
```

原版 kind 号 ↔ key 的对照只在导入层（D1），引擎不读：

```yaml
# config/ids/legacy_codec.yaml 片段
map_point_kind:
  0: vanilla:plaza
  1: vanilla:exchange
  2: vanilla:shipyard
  3: vanilla:dock
  4: vanilla:bar
  5: vanilla:guild
  6: vanilla:palace
  7: vanilla:governor
  8: vanilla:inn
  9: vanilla:remains
  10: vanilla:church
  11: vanilla:gate
  none: 0xD          # CMapPointView+0xa8 初值，「未选热点」[推断]；运行时用 null
```

## 7. 独立旋钮

改这些值不应迫使其他系统改代码：

- 增减设施（新 key + handler + actions 列表）；新 kind 指向已有处理器只改表
- 某港开/关某热点（town_map_points 初值、发展度解锁表）
- 住宿单价、修理/改造等待日
- 酒价表
- 广场点击号 → 动作映射
- 王宫 / 总督府的文案 key（按设施 key，不按首都标记）

改这些会碰到边界，但仍不应改贸易/战斗代码：

- 休息恢复曲线 → 只动 `characters` 的 rest 公式
- 木工折扣 → 只动修正器 key `ship.repair_cost` 的贡献
- 补给桶数 → ships

## 8. 对外接口

订阅：`ArrivedAtTown { fleet_id, town_id }`（world 发出；旧稿本系统自发的 `TownEntered` 已并入它）；`RemainsRevisited { remains_id, town_id, kind }`（world 发出，已发现遗迹再进 kind 9/10/11 → 跑仪式，见 05 篇 4.2；首访由 world 处理，`01-world/11` §4）。

事件（本系统发出）：

```
FacilityEntered { town_id, map_point_kind }   # 订阅方含 world（kind 9/10/11 遗迹首访 / 重访，词表 §14.22）
PlazaCrowdGathered { town_id }
InnRested { town_id, mode, nights_done, gold_paid }
BarDrinkPaid { town_id, gold_paid, tier }
```

查询：

```
TownLife.map_points(town_id) -> [map_point_kind]  # 读 town 状态的热点集合
TownLife.has_map_point(town_id, kind_key) -> bool
TownLife.isOpen(town_id, facility_key) -> bool
TownLife.innQuote(nights) -> gold
# 广场流行直接查 factions：Fashion.current(town_id) -> [{ trading_item_id, level }]，空槽丢弃；本系统不另设查询
```

命令（本系统接收点击后转发，参数只有 ID 与标量）：

```
OpenExchange { town_id, fleet_id }          → economy
OpenShipyard { town_id, fleet_id }          → ships
QuoteRepair { fleet_id }                    → ships
OpenSupply { fleet_id, town_id }            → ships
RecruitAtBar { town_id }                    → characters
Actions.run(inn.rest_*, { town_id, fleet_id, nights })  → core 执行器 → Characters.apply_rest
OpenContract { town_id }                    → factions
Invest.pay(town_id, force_id, gold, reason)  → economy（交易所 / 王宫投资垫板确认后调用）
ListRequests { town_id }                    → town-life/requests
OpenScheme { town_id }                      → town-life/schemes
DistributeGoods { town_id, fleet_id }       → economy（数量/效果由其定义）
```

原版设施代码不启动小游戏（03 篇 2.0）。模组若要「点设施进小游戏」，在动作 `effects` 里写 `Minigames.run(key) -> int`，不另设设施 → 小游戏硬路径。

存档：设施定义不存。每港热点集合是 town 状态（原版 `CTown+0x14`），存 key 列表。「今晚是否已休息」原版无此状态（旅馆每晚即时结算）。

## 9. 未核实字段

- 无 `eFacility` 枚举；设施 key 是重构命名，不是原版符号。
- `CPlazaView` 点击 `0xA`（`fn_140003d37`）。
- 仪式奖励子类型 0–7 的具体内容（05 篇 2.3）。
- `eMapPointKind` 12 值的原名（类分派与中文名已还原）；`0xD` 作「未选」为推断。
- `eTownKind` 的 `NUM_TOWNKIND`；是否仅首都/非首都。
- `eTOWNACCESS` / `STownAccess`：dtor 把槽写成 `0xC9`，语义未还原。
- `STown+0x70` 两字节、`+0x74/+0x75`：不是设施开关，具体名见 world 文档。`+0x48` 是热点位域（2.2）。
- 「公馆」一词未出现在 SC mesfile。
- `CBar+0x18=5` 语义。高档判定量已知：玩家势力在本港大海域的影响力（05 篇 2.6）。
- 旅馆每夜恢复量已在 05 篇还原（+max/20、状态概率降级、疲劳 −4..6 / −5..9）；人物字段 `+0x0D` = 身体态、`+0x0E` = 精神态（`03-characters/01-person-model.md` §2 已钉死）。
- 改造「%s 天」与默认 2 天何时分叉。
- 木工折扣百分比公式（人物系统）。
- 人群事件触发率（原版小游戏只由剧本脚本启动，见 03 篇 §2.0）。
- `CPlazaSylphid` 10 槽状态机（`+0x2d4` 出现 0–4）是否纯表现。
- 运行时改位 `fn_140001f87` 的调用者 `fn_140008a08` 是否为事件指令（`[推断]`）。

已解决：`CShopButton+0xa8=0x6A` 是交易品槽（招牌挂 `fn_1401696e0` 返回的 `trading_item_id`，06 篇 2.3），不是人物 ID；`ePerson_106` 只是同值巧合。

## 附：mesfile 块偏移 → exe 消息号

按 D10，文案 key 用 exe 消息号，不用 mesfile 字节偏移。正文里 `0x` 开头、落在 mesfile 的数多为 `catalog/mesfile_sc.txt` 的 NUL 块偏移；一块常含多条消息。下表由脚本按 exe 消息号表（`.data 0x140683620` 37 块 `{first_id,size}` + `0x1406836c0` 块内偏移；`.data` 文件偏移 = VA − `0x140001600`）切句，再按正文引号句匹配生成。只给一条的是已匹配到句；给区间的需导出时按句再选。正文里已标「消息」的号（如 `0x58a`、`0xb2e`）本来就是消息号，不在此表。

| 块偏移 | 消息号 | 首句 |
| --- | --- | --- |
| `0x12c` | `0x0` | 提督。好像广场上聚集了一群人。 |
| `0x217` | `0x8` | 提督。好像广场上聚集了一群人。 |
| `0x733` | `0x39` | 需要１０枚金币。 |
| `0x7bf` | `0x41` | 收您２０枚金币。 |
| `0x1143` | `0x8e` | 请选择零售店的招牌，就可以听取流行情报。 |
| `0x5fc3` | `0x2db` | 你是水手吧？如果是大宗买卖，请去交易所。 |
| `0x6fa3` | `0x32e–0x333`（块内多条，按句选） | 谢谢！那赶快工作了！ |
| `0x753f` | `0x360` | 喂，我们划拳吧。 |
| `0x7627` | `0x36a` | 有人要乘坐提督的船……让他上船吧？ |
| `0x7d47` | `0x39b` | 拿着这个到小城市的王宫或者总督府去，占有率的事会 |
| `0x7df7` | `0x3a1` | 一只舰队最多可以有五艘船，新船放在船坞里。 |
| `0x7fcb` | `0x3af` | 如果那个人肯帮忙，修理费用就少算%s％吧……。 |
| `0x8037` | `0x3b2` | 到修理完成前，还得等一天。 |
| `0x81af` | `0x3be`（块内多条，按句选） | 水好像很少，要平均补给吗？ |
| `0x81cb` | `0x3bf`（块内多条，按句选） | 水手们好像很疲劳，不让他们休息一下吗？ |
| `0x822f` | `0x3c2` | 改变供水和供食物的比例吗？ |
| `0xba97` | `0x572` | 好的，您请休息吧。 |
| `0xbab7` | `0x573` | 住１晚１枚金币，要住几天？ |
| `0xbcd3` | `0x589–0x58a`（块内多条，按句选） | 提督！我想试着找出%s。好像在%s。 |
| `0xc0b3` | `0x5c2` | 在这里船员身体和精神都能够恢复。 |
| `0xc1fb` | `0x5c8` | 如果有传教士，能够消除水手的不满。在广场上还能散 |
| `0xc237` | `0x5c9` | 如果参谋在，可在酒馆里拟定策略。 |
| `0xc337` | `0x5d3` | 如果那个人能够帮忙，改造费可以给你减少%s％。折 |
| `0xc39f` | `0x5d6` | 到改造完成要等２天。 |
| `0xc3c3` | `0x5d8` | 到改造完成要等%s天。 |
| `0x15267` | `0xb2e` | 希望把此文书交给%s的同业公会。愿意接受吗？ |
| `0x18463` | `0xd0a` | 前去“同业公会”要求停战吗？ |
