# 场景图与切换

UI 是所有玩法系统的只读投影。场景切换发命令、订事件，禁止从 View 写存档字段或改经济/战斗数值。

## 1. 职责边界

本系统管：

- 场景 id、允许的切换边、每场景订阅哪些系统查询
- 标题 / 选剧本 / 读存档 / 大地图 / 港口 / 战斗 / 事件 的分层
- 港口设施作为 `CTownScene` 的子视图，而不是另一套模拟

不管：

- 场景里具体算法（市价、航行、伤害）
- 文案内容（`03-localization.md`）
- 存档二进制（`02-save-session.md`）
- KTGL / D3D11 / 贴图容器怎么画（`TitleSC.dk4` 等只当资源 id）
- 小游戏内部规则（`town/minigames.yaml`）；本文件只登记它们挂在哪条边上

## 2. 原版事实

宿主：

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `CDk4Game` 继承 `CSceneManager`，体积 `0x98`；类名下只还原出 ctor/dtor（vtable 只有 dtor），实际方法都在 `unassigned`，以全局实例 `DAT_144296500` 调用 | `[确认]` | `CDk4Game.cpp`；`fn_1400251f0.cpp:871` 对 `DAT_144296500` 调 ctor |
| 顶层循环在 unassigned 函数 `fn_1400079a0`（外层：标题↔游戏）→ `fn_14000e8a4`（`while state==1`），this 均为 `CDk4Game`；每帧逻辑在各 Scene | `[确认]` | `fn_14010be70.cpp`；`INDEX.md` 原写「循环不在 `CDk4Game` 方法表」只对 vtable 成立 |
| `CScene` 体积 8 字节量级（dtor `0x8`） | `[确认]` | `CScene.cpp` |
| `CSceneManager` dtor 分配 `0x78` | `[确认]` | `CSceneManager.cpp` |
| 启动：互斥体 `KOUKAI4PK_MUTEX`，窗类 `KT_DK4_PK_HD_WINDOW_NAME`，标题 `Uncharted Waters IV HD Version` | `[确认]` | `source/steam/Startup.cpp` |
| `CMainWindow` 体积 `0x28` | `[确认]` | `CMainWindow.cpp` |
| `CViewManager` 管理叠加 View | `[确认]` | `CViewManager.cpp` |

标题与新游戏：

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| SC exe 硬编码加载 `TitleSC.dk4` | `[确认]` | `fn_1401200a0.cpp` `fn_1401b9940.cpp`（含 GameOver still） |
| `CAcrossGameView`（方法 25）是事件脚本小游戏 2（过河），**不是**标题/跨场景按钮；旧稿归类错 | `[确认]` | `CAcrossGameView.cpp`；`08-town-life/03-minigames.md` 2.0（脚本 `0xC1` 内层跳表 `0x1400e1f18`） |
| `CDummyTopView` 体积 `0x80` | `[确认]` | `CDummyTopView.cpp` |
| `CHitAnyKey` 与标题图一起用 | `[确认]` | `fn_1401b9940.cpp` |
| `CScenarioSelectView` 体积 `0x138`，构造循环 `s20 >= 7`，与 `NUM_SCENARIO=7` 一致 | `[确认]` | `CScenarioSelectView.cpp` `ScenarioID.h` |
| `CScenarioSelectView::vfunc_13` 接受 `0..7`（含 7） | `[确认]` | 同文件；**7 是否取消键 `[未知]`** |
| `CScenarioMenuView` 同样 7 槽 | `[确认]` | `CScenarioMenuView.cpp` |
| `CScenarioButton` 选剧本按钮 | `[确认]` | `CScenarioButton.cpp` |
| 开局：输入生日、选 3 技能、输入姓名 | `[确认]` | mesfile `请输入生日` `请选择３种熟练的技能`；`CPlayerNameView` `CLetterSelectView` `CGoodSkillMenuData` `CDatePadView` |
| `CPlayerStatusView` 可改姓名 / 势力名 / 月日（vfunc_13 分支 8/9/0xa） | `[确认]` | `CPlayerStatusView.cpp` RTC `cFirstName,cMiddleName,cFamilyName,cForceName,nMonth,nDay` |
| `CFaceView` 肖像 | `[确认]` | `CFaceView.cpp` |
| 读档/存档 UI：`CSelectSaveDataView`（方法 19）+ `CSaveDataScrollView` + `CSaveDataFile` | `[确认]` | `source/game/save/` |
| `CSelectSaveDataView` 构造里 5 行槽位坐标（x `0x7d`，y `0xd9` 起步长 `0x97`） | `[确认]` | `+0xc0..+0xe4` |
| 存/读列表扫描 52 个槽文件，交给选择框 50 条，一屏 `min(50,5)` 行滚动；默认选中最新存档；取消返回 `0x33` | `[确认]` | `fn_14001300c` `< 0x34`、`r8d=0x32`；`CSelectSaveDataView::vfunc_02` `>= 5`；`fn_1400125da`（见 `02-save-session.md`） |
| 列表每格文案：`"\n主人公∶%s·%s\n势力值∶%8s  %22s"`，末项为「位置:」+ 港名 / 区域名 / 「海上」 | `[确认]` | `fn_14001300c` |

顶层状态机（`CDk4Game+0x88`，以下函数都以 `DAT_144296500` 为 this）：

| 值 | 含义 | 置位 / 判定 | 等级 |
| --- | --- | --- | --- |
| 0 | 标题 / 未在游戏中 | ctor 置 0；`fn_1400079a0` 入口置 0；系统菜单「结束游戏」调 `fn_140007b58`（`0x14011a909` `0x14011a954`）；事件脚本 `0xC5` imm 非 0/1（`0x1400e1dd2`）=「回标题」 | `[确认]` |
| 1 | 游戏进行中；主循环条件 | `fn_140006f50` 置 1；`fn_140004ead` 判 `== 1`；`fn_14000e8a4` `while` | `[确认]` |
| 2 | 游戏中选了「载入进度」 | 系统菜单 `fn_140004c28` 选槽成功 → `fn_14000b3c0` 置 2；回到标题流程 `fn_140006677` 见 `!= 0` 跳过菜单直接读 `+0x80` 槽 | `[确认]` |
| 3 | **Game Over**（子码 `+0x90`） | 唯一 setter `fn_140010578(this, code)`（body `0x1400c42d0`），4 个写入点：事件 `execute id 0x3fff`（`0x1400c39ba`，参 1）、脚本 `0xC5` imm 1（`0x1400e1dc4`，参 1）、舰队覆灭 `fn_14000a9ed`（`0x1400ef13e`，玩家本队，参 0）、舰队解散 `fn_14000a8c1`（`0x14010d039`，玩家舰队，参 0）。后处理（`fn_14000e8a4` `0x1401128e2`）：参 1 先显示消息 `0x170`…`0x173`（exe 消息号表：寿终四句，见 `11-progression/03-gameover-succession.md`），再 `fn_14000a894`（RTC `rcGameOver`，`TitleSC.dk4` still），然后置 0 **回标题**；无继承、无选人 | `[确认]` |
| 4 | 剧本通关 | `fn_14000210d`（body `0x1400e2440`）唯一调用点 = 事件脚本 `0xC5` imm 0（`0x1400e1db1`，在 `fn_14000847c` 内）。后处理（`0x140112980`）：`fn_14000b677(1)` 片尾动画（RTC `cEventAnimOP`）→ 置 0 → `fn_14000a321` 把已发现道具/遗迹同步进系统档图鉴位图 → 若本剧本通关位未置：点数 `+4`（`fn_14000d021`）、置通关位（`fn_140010127`）、按 `aeScenarioPortrait` 免费解锁（`fn_140008c51`）→ 无论首通与否都写 `savedata.dk4`（`fn_140002045`） | `[确认]`（`0x1401129a7`…`0x140112a0c` 反汇编） |
| 5 | 退出程序 | `WM_QUIT`(`0x12`) 时 `fn_140014ab0` 置 5；`fn_140013219` = `!= 5` 是最外层循环条件；事件动画等自带消息泵的循环收到 `WM_QUIT` 同样置 5 | `[确认]` |

主循环结束（`fn_14000e8a4`，body `0x140112820`，尾部 `+0x88` 分派）：3、4 处理完都归 0（除非已是 5）；2、5 保持。外层 `fn_1400079a0` 每轮先载 `startup.dk4` 再进标题流程。`[确认]`

叙事侧只有「结束剧本」一个出口（脚本 `0xC5`：imm 0 通关 / 1 剧情 Game Over / 其它回标题；`0x3fff` = 剧情 Game Over），见 `10-narrative/03-event-commands.md`「流程态」。重构统一为 `Session.end_scenario(outcome, cause)`，由本层决定片尾、profile 记账、回标题；`game_over` 时本层调 `GameOver.resolve(cause)` 查终局表（`11-progression/03-gameover-succession.md`）；叙事和战斗不得直接写流程态。

`CDk4Game` 其它字段：`+0x78` 新游戏种子带来的布尔（写入存档头 `+0x1C`），`+0x7c` 剧本，`+0x80` 待读槽号，`+0x84` 待读标志，`+0x8c` 存档次数，`+0x90` 状态 3 子码（1 = 剧情/寿终，0 = 玩家舰队覆灭或解散），`+0x94` ctor 置 0 `[未知]`。`[确认]`（`fn_140006668` `fn_140002658` `fn_14000d684` `fn_140010578`）

标题与系统菜单：

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 标题根菜单 7 项：载入进度 / 开始新游戏 / 音乐厅 / 博物馆 / 摄影沙龙 / 照相馆 / 结束游戏 | `[确认]` | `fn_14000d783`（body `0x140112f50`，RTC `asRootMenu`）字符串；跳表 7 项 |
| 音乐厅、博物馆、摄影沙龙、照相馆只在系统档有任一剧本通关位时可选 | `[确认]` | 同函数对第 2–5 项调 `fn_14000e1c9`（body `0x140121b20`：循环 `< 7` 查 `savedata.dk4+0x38` 各位，任一为 1 即真） |
| 跳表 `0x140113458` 各项：0 读档、1 新游戏、2 音量（`cSoundBoard`）、3 博物馆（扫 `eItem` `< 0xE7`）、4 摄影沙龙 = 画廊（`CShowPortrait`，见 `10-narrative/05-gallery-unlock.md`）、5 照相馆（`fn_1400032a1`，内容 `[未知]`）、6 结束 | `[确认]` 分派；5 的内容 `[未知]` | `10-narrative/05-gallery-unlock.md`「画廊宿主」 |
| 博物馆无宝物时提示「没有发现任何宝物！」，否则「已经发现的宝物。」 | `[确认]` | 同函数 |
| 标题流程分支：新游戏 / 读档 / 「结束游戏吗？」/「改变已经设定的选项，可以吗？」 | `[确认]` | `fn_140006677` |
| 新游戏：`fn_140006483`（RTC `cStatusView,cSenarioSelectView`）选剧本与设定；剧本 4 另走一段初始化 | `[确认]` 分支；剧本 4 那段做什么 `[未知]` | `fn_140006677` `+0x7c != 4` |
| 游戏中系统菜单 5 项：储存进度 / 载入进度 / 结束游戏 / 选项 / 返回 | `[确认]` | `fn_140004c28`（body `0x14011a760`，RTC `asMenu`） |
| 选项子菜单：背景音乐 / 音效 / 变更主人公名 / 变更势力名称 / 取消 | `[确认]` | `fn_140009331`（RTC `asOptMenu`） |

大地图 / 港口：

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `COceanScene : CScene, CView, …, COceanMapData`，方法 19 | `[确认]` | `class_hierarchy.txt` `COceanScene.cpp` |
| `aeOceanSceneButtonIndex` 五个下标 0–4 | `[确认]` | `COceanScene::vfunc_14` |
| `CGeneralCommandView` 大地图指令 | `[确认]` | `CGeneralCommandView.cpp` |
| `CSeaAreaScene : CRouteMap, …, CScene`，方法 13 | `[确认]` | `CSeaAreaScene.cpp` |
| `CMapView` `CNextAreaView` `CRouteMap` `CTownNavigator` `CWheelView` | `[确认]` | ocean 目录 |
| `CPauseSprite` 暂停 | `[确认]` | `CPauseSprite.cpp` |
| `CTownScene : CScene`，体积 `0x58`，`+0x40=3` | `[确认]` | `CTownScene.cpp` |
| `CTownScene::vfunc_02` 宿主设施对象：造船、遗迹、公会、旅馆、宫殿、酒馆、港口、交易所、广场 | `[确认]` | functions.tsv RTC `cShipyard,cRemains,cGuild,cInn,cPalace,cBar,cPort,cTrade,cPlazaView,bItemPosInformation` |
| `CTownSceneView` 循环 `eMapPointKind >= 0xc` → 12 个地图点 | `[确认]` | `CTownSceneView.cpp`；`NUM_MAPPOINTKIND=12` |
| `CMapPointView` 港口地图点 | `[确认]` | `CMapPointView.cpp` |
| `CPlazaView : CView, …, CPlazaScreen`，5 个子指针 | `[确认]` | `CPlazaView.cpp` 循环 `>= 5` |
| `CPlazaButton` `CPlazaScreen` | `[确认]` | 同目录 |
| `CBar` `CInn` `CGuild` `CPalace` `CPort` `CShipyard` 均为独立类 | `[确认]` | `source/game/town/` |
| `CChurchGameView` 教会小游戏（方法 15） | `[确认]` | `CChurchGameView.cpp` |
| `CMpDock` `CMpRemains` 地图点：船坞 / 遗迹；地图点 kind 9/10/11（遗迹 / 教会 / 城门）共用 `CMpRemains`，教会是按日期的仪式 | `[确认]` | town 目录；`08-town-life`、`01-world/10-mappoint-kinds.md` |
| `CVillage` 体积 `0x48` | `[确认]` | `CVillage.cpp` |

战斗 / 事件 / 其它叠加：

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `CIntoBattleView` 进入战斗确认 | `[确认]` | `CIntoBattleView.cpp` |
| `CCombatScreen`；`CCombatCrewScreen` / `CCombatSingleScreen` 继承它 | `[确认]` | `class_hierarchy.txt` |
| `CEvent : CSaveObject, CEventSeeker, CEventData, CEventStillManager` | `[确认]` | `CEvent.cpp` |
| 事件脚本 `EV0..EV6.dk4` `EVA.DK4` 头 `WE`；`Sc0..Sc6.dk4` 是 DS 脚本指令流 + 内嵌对白，**不是**场景图（旧稿与 `formats.md` 写法错） | `[确认]` | `10-narrative/02-event-system.md`；第一轮 10 组报告 |
| `CDeckShipView` 甲板编成（方法 18，RTC 桅/舱） | `[确认]` | `CDeckShipView.cpp` |
| `CWaitTriggerView` 等触发 | `[确认]` | `CWaitTriggerView.cpp` |
| `CDiscoverTitleView` 存在 `[确认]`；「发现称号画面」降为 `[推断]`，更像发现时弹出的标题卡（原版无名声/爵位） | `[推断]` | `11-progression/02-fame-titles.md` |
| `CGoldRoadInfomationView` 黄金航路 | `[确认]` | game 目录 |
| `CGetOldMapWindow` 古地图 | `[确认]` | 同上 |
| 小游戏：脚本 `0xC1` imm 0–6 = `CCoinGameView` `CNimGameManager` `CAcrossGameView` `CWaterGameView` `CCatchGameView` `CChurchGameView` `CStoneWallGameView`，各只有这一个构造点；另有航海捉鼠 `CRatGameView`（海上 happening 启动） | `[确认]` | RTTI；`08-town-life/03-minigames.md` 2.0；`06-ocean/05-sea-happenings.md` |
| Game Over still 仍从 `TitleSC.dk4` 取图 | `[确认]` | `fn_140115330.cpp` |

顶层状态（上表）已钉死；但游戏内「从哪一场景可以进哪一场景」的边表**没有**从 vfunc 还原成一张图。下面 6 节的 `allowed` 里，带 `[推断]` 的边来自类职责 + 设施 RTC + 架构文档，实现时要用事件名约束，不得在代码里再写死另一套。

## 3. 实体与 ID

| ID | 含义 | 空 |
| --- | --- | --- |
| `scene_id` | 本文件注册的场景 key | `null` |
| `scenario_id` | 剧本 key（`vanilla:raphael`…，以 `10-narrative/01` 为准）；原版 `eScenario` 0–6 只作 `vanilla_index` | `null` |
| `facility_id` | 港口设施 key（酒馆、交易所…） | `null` |
| `map_point_kind` | 地图点种类 key；原版 `eMapPointKind` 0–11 只作 `vanilla_index` | `null` |
| `view_id` | 叠加 View（存档、状态、技能） | `null` |
| `game_state` | 会话流程态 key（`title` `playing` `load_requested` `game_over` `scenario_clear` `quit`）；原版 `CDk4Game+0x88` 0–5 | — |

`scene_id` 用稳定 key，不把 `COceanScene*` 传出 UI 层。空值一律 `Option` / `null`，不用 -1（D1）。

12 个 `map_point_kind` 已有名字（`01-world/10-mappoint-kinds.md`：0 广场 1 交易所 2 造船厂 3 码头 4 酒馆 5 行会 6 王宫 7 总督府 8 旅馆 9 遗址 10 教会 11 城门）。按名字对 9 个设施对象：plaza→0、trade→1、shipyard→2、port→3、bar→4、guild→5、palace→6、inn→8、remains→9 `[推断]`（只是名字对应，未见分发代码）；7 总督府没有同名设施对象；9/10/11 共用 `CMpRemains`（见上表）。配置里设施用自己的 key，地图点表单独列，外键可空。

## 4. 算法 / 公式

无数值公式。切换是一张有向图：

```
can_enter(from, to, trigger) =
  (from, to, trigger) ∈ edges
  AND guards(to) 全部为真
```

`guards` 只读查询：例如 `combat` 需要 `BattleOffered`；`town.trade` 需要当前 `town_id` 有效。失败：不切场景，发 `SceneEnterRejected`。

原版没有这张表；重构把边写进 `config/ui/scenes.yaml`。缺边就是不允许，不做「任何 View 都能 new 出来」的隐式切换。

暂停 / 存档是叠加，不换 `scene_id`。事件 `CEvent` 默认也是叠加在 ocean/town 上（`[推断]`：它继承 `CSaveObject` 而不是 `CScene`）。

## 5. 耦合分析

原版病：

- SC exe 把 `TitleSC.dk4`、`IconSC.dk4`、`SC/mesfile.dk4` 写死在调用点
- `CTownScene` 直接持有 `CTrade` `CBar` 等对象，设施逻辑与场景生命周期绑死
- `COceanScene` 同时是 `CScene` 和 `COceanMapData`
- 选剧本 7、地图点 12、海洋按钮 5 写在构造循环里
- Game Over 复用标题图包

重构：

- 场景只订阅接口：`Economy.quote` `Ocean.snapshot` `Combat.phase`
- 设施是 town-life 系统，场景只持 `facility_id`
- 语言包提供 `title_texture` `icon_texture` `mesfile` 路径（见 `03-localization.md`）
- 剧本选择只写 `scenario_id`，由 narrative 加载世界包

## 6. 解耦后的配置

```yaml
# config/ui/scenes.yaml
schema: dk4.ui.scenes
version: 1
boot:
  first: title
  splash: startup                          # 原版每轮回标题先载 startup.dk4
  window_class: KT_DK4_PK_HD_WINDOW_NAME   # 原版窗类，可改
game_states:                               # 原版 CDk4Game+0x88；只做对照，运行时用事件
  - { id: title,          original: 0 }
  - { id: playing,        original: 1 }
  - { id: load_requested, original: 2, next: title, then: LoadSlot }
  - { id: game_over,      original: 3, param: cause, emits: ScenarioFailed, next: title }  # 原版子码 +0x90：1 → cause scripted，0 → fleet_lost；子码原值作 end_code
  - { id: scenario_clear, original: 4, emits: ScenarioCleared, next: title }      # 片尾 → profile 写通关位并记账 → 回标题
  - { id: quit,           original: 5 }
title_menu:                                # 原版 fn_14000d783，7 项
  - { key: load,        action: LoadGame }
  - { key: new_game,    action: NewGame }
  - { key: music_hall,  action: OpenExtra, extra: sound_board,  requires: profile.any_cleared }
  - { key: museum,      action: OpenExtra, extra: encyclopedia, requires: profile.any_cleared }   # 读 profile.encyclopedia
  - { key: photo_salon, action: OpenExtra, extra: gallery,      requires: profile.any_cleared }   # CShowPortrait；规则见 10-narrative/05
  - { key: photo_studio, action: OpenExtra, extra: photo_studio, requires: profile.any_cleared } # 内容 [未知]
  - { key: quit,        action: Quit, confirm: true }
system_menu:                               # 原版 fn_140004c28，游戏中
  - { key: save,    action: SaveGame }
  - { key: load,    action: LoadGame }
  - { key: quit,    action: ReturnToTitle, confirm: true }
  - { key: options, submenu: [bgm, se, rename_protagonist, rename_force] }
  - { key: back }
overlays:
  - pause
  - save
  - status
  - person_info
  - discover_title
  - event
  - game_over
scenes:
  - id: title
    key: title
    original_class: CDummyTopView          # 宿主未完全钉死；标题资源见 language pack
    resources: [title_texture]
    edges:
      - { to: scenario_select, trigger: NewGame }
      - { to: save, trigger: LoadGame, overlay: true }
  - id: scenario_select
    key: scenario_select
    original_class: CScenarioSelectView
    count: 7                               # NUM_SCENARIO；运行时读表长
    edges:
      - { to: new_game_setup, trigger: ScenarioChosen }
      - { to: title, trigger: Back }
  - id: new_game_setup
    key: new_game_setup
    views: [CPlayerNameView, CLetterSelectView, CDatePadView, CGoodSkillMenuData, CFaceView]
    edges:
      - { to: town, trigger: SetupDone }   # 开局在港或在海 [未知]，由 scenario 内容包指定 spawn
      - { to: ocean, trigger: SetupDone }
  - id: save
    key: save
    original_class: CSelectSaveDataView
    overlay: true
    slots: 50                              # 原版选择框条数（扫描 52）；读 config/ui/save.yaml
    rows_per_page: 5
  - id: ocean
    key: ocean
    original_class: COceanScene
    command_buttons: 5                     # aeOceanSceneButtonIndex 0–4 含义 [未知]
    subscribe: [ocean, fleets, world, climate]
    edges:
      - { to: town, trigger: ArrivedAtTown }
      - { to: sea_area, trigger: OpenSeaArea }
      - { to: combat_offer, trigger: Encounter }
      - { to: village, trigger: FoundVillage }
      - { to: remains, trigger: FoundRemains }
    overlays_allowed: [pause, save, status, event, discover_title, gold_road, deck]
  - id: sea_area
    key: sea_area
    original_class: CSeaAreaScene
    edges:
      - { to: ocean, trigger: Back }
  - id: town
    key: town
    original_class: CTownScene
    subscribe: [town, economy, characters, ships]
    facilities:
      - { key: shipyard, class: CShipyard, map_point: shipyard }   # map_point 对应 [推断]
      - { key: remains, class: CRemains, map_point: remains }     # 设施对象名 cRemains
      - { key: guild, class: CGuild, map_point: guild }
      - { key: inn, class: CInn, map_point: inn }
      - { key: palace, class: CPalace, map_point: palace }
      - { key: bar, class: CBar, map_point: bar }
      - { key: port, class: CPort, map_point: dock }
      - { key: trade, class: CTrade, map_point: exchange }
      - { key: plaza, class: CPlazaView, map_point: plaza }
    map_point_kinds: 12                    # 表在 world/map_point_kinds；运行时读表长
    edges:
      - { to: ocean, trigger: Depart }
      - { to: facility, trigger: FacilityEntered }
    overlays_allowed: [pause, save, status, event, deck, person_info]
  - id: facility
    key: facility
    param: facility_id
    edges:
      - { to: town, trigger: LeaveFacility }
  - id: plaza
    key: plaza
    original_class: CPlazaView
    child_slots: 5
    edges:
      - { to: town, trigger: Back }
  - id: combat_offer
    key: combat_offer
    original_class: CIntoBattleView
    edges:
      - { to: combat, trigger: Accept }
      - { to: ocean, trigger: Decline }
  - id: combat
    key: combat
    original_class: CCombatScreen
    variants: [CCombatCrewScreen, CCombatSingleScreen]
    subscribe: [combat, fleets, characters]
    edges:
      - { to: ocean, trigger: BattleResolved }
      - { to: town, trigger: BattleResolvedInPort }  # 是否存在 [未知]
  - id: village
    key: village
    original_class: CVillage
    edges:
      - { to: ocean, trigger: Leave }
  - id: remains
    key: remains
    original_class: CMpRemains             # 服务地图点 kind 9/10/11（遗迹/教会/城门）
    edges:
      - { to: ocean, trigger: Leave }
      - { to: town, trigger: Leave }
  - id: deck
    key: deck
    original_class: CDeckShipView
    overlay: true
minigames:                                 # 只登记宿主 overlay；入口是 Minigames.run(id)，规则在 town/minigames.yaml
  overlay: true                            # 叠在当前场景上，结束回原场景
  launched_by: [narrative.run_minigame, ocean.happening]   # 原版：脚本 0xC1 imm 0–6；捉鼠由海上 happening
  ids: [coin, nim, across, water, catch, church, stone_wall, rat]   # 原版类名见 08-town-life/03-minigames.md
```

`edges` 中未标来源的边一律视为 `[推断]`，落地前用集成测试钉死。原版类名只作对照，运行时不要 `new CTownScene`。

## 7. 独立旋钮

| 旋钮 | 作用 |
| --- | --- |
| `scenes[].edges` | 加/禁切换，不必改系统代码 |
| `command_buttons` | 大地图按钮数 |
| `map_point_kinds` / `facilities` | 港口可点设施 |
| `overlays_allowed` | 某场景能否存档、开状态 |
| `boot.first` | 启动进标题还是直接进某调试场景（调试场景不得进正式包） |
| `title_menu[].requires` | 额外菜单的解锁条件（原版：任一剧本通关），查 `profile`，不读存档字节 |
| `system_menu` | 游戏中菜单项增删 |
| 资源 key | 换标题图，走语言包（`Sc*.dk4` 是叙事脚本，不是场景资源） |

改 `NUM_SCENARIO` 是 narrative 的表长；本文件 `count: 7` 只是 vanilla 默认。

## 8. 对外接口

**命令（谁都可以发，场景图裁决）**

```
EnterScene { scene_id, params }
LeaveScene { scene_id }
OpenOverlay { view_id }
CloseOverlay { view_id }
ReturnToTitle {}
Session.end_scenario { outcome: cleared | game_over | to_title, cause?, end_code? }   # 叙事 0xC5/0x3fff、舰队覆灭都走这里；原版直接写 +0x88
                                           # cause: scripted | protagonist_died | fleet_lost | bankrupt | manual；game_over 时调 GameOver.resolve(cause)
                                           # end_code：原版 +0x90 子码，只供导入 / 对拍，玩法不读
RequestLoad { slot_id }                    # 原版：置状态 2，经标题流程读档
```

**查询**

```
Scene.current() -> scene_id
Scene.overlay_stack() -> [view_id]
Scene.can_enter(to, trigger) -> bool
```

**事件**

```
SceneEntered { scene_id, from }
SceneLeft { scene_id }
OverlayOpened { view_id }
FacilityEntered { town_id, map_point_kind }   # town-life 发；场景图据此切 facility
GameStateChanged { from, to, cause? }      # 只给 ui；玩法订阅 ScenarioCleared / ScenarioFailed。对应原版 +0x88 / +0x90
ScenarioCleared { scenario_id }            # profile 订阅：写通关位，并按 gallery.yaml 规则首通加点、免费解锁（10-narrative/05）；画廊只读 Profile.cleared（词表 §14-24）
ScenarioFailed { scenario_id, cause, end_code? }   # Game Over still 后回标题；原版无继承。cause 语义枚举；end_code 仅原版导入（词表 §14-3）
```

UI 控件（`CSprite` / `CButton` 家族）不进玩法配置，也不出现在上述接口里。

禁止：`CTrade` 从按钮回调里直接改货舱；正确路径是 `CommitDeal` 命令进 economy，成功后再投影。

## 9. 未核实字段

- 完整允许边表（尤其：战斗结束回海还是回港、开局 spawn、事件是否盖住 `CScene`）
- `aeOceanSceneButtonIndex` 0–4 的命令名
- 12 个 `eMapPointKind` 与 9 个设施对象的分发代码（目前只按名字对上）；总督府进的是什么
- `CTownScene+0x40=3` 的语义
- `CPlazaView` 5 个子对象是什么
- `CScenarioSelectView` 接受的 7 是不是取消
- `CDummyTopView` 是否就是标题根
- `savedat50` / `savedat51`：扫描 52、选择框 50，多出的两个是否另有入口
- 标题流程中剧本 4（`+0x7c == 4`）额外初始化的内容
- 标题第 5 项「照相馆」`fn_1400032a1` 的内容
