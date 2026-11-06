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
| `CDk4Game` 继承 `CSceneManager`，体积 `0x98`，还原出的只有 ctor/dtor | `[确认]` | `CDk4Game.cpp` `class_hierarchy.txt` `INDEX.md` |
| 游戏循环在 `CScene` / 各 Scene，不在 `CDk4Game` 方法表 | `[确认]` | `INDEX.md` `02-original-architecture.md` |
| `CScene` 体积 8 字节量级（dtor `0x8`） | `[确认]` | `CScene.cpp` |
| `CSceneManager` dtor 分配 `0x78` | `[确认]` | `CSceneManager.cpp` |
| 启动：互斥体 `KOUKAI4PK_MUTEX`，窗类 `KT_DK4_PK_HD_WINDOW_NAME`，标题 `Uncharted Waters IV HD Version` | `[确认]` | `source/steam/Startup.cpp` |
| `CMainWindow` 体积 `0x28` | `[确认]` | `CMainWindow.cpp` |
| `CViewManager` 管理叠加 View | `[确认]` | `CViewManager.cpp` |

标题与新游戏：

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| SC exe 硬编码加载 `TitleSC.dk4` | `[确认]` | `fn_1401200a0.cpp` `fn_1401b9940.cpp`（含 GameOver still） |
| `CAcrossGameView` 方法 25，标题/跨场景角色按钮 | `[确认]` | `CAcrossGameView.cpp` |
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
| `CSelectSaveDataView` 构造里 5 行槽位坐标 | `[确认]` | `+0xc0..+0xe4`；文件槽是 `savedat00..10` 共 11，故列表可滚 |

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
| `CMpDock` `CMpRemains` 地图点：船坞 / 遗迹 | `[确认]` | town 目录 |
| `CVillage` 体积 `0x48` | `[确认]` | `CVillage.cpp` |

战斗 / 事件 / 其它叠加：

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `CIntoBattleView` 进入战斗确认 | `[确认]` | `CIntoBattleView.cpp` |
| `CCombatScreen`；`CCombatCrewScreen` / `CCombatSingleScreen` 继承它 | `[确认]` | `class_hierarchy.txt` |
| `CEvent : CSaveObject, CEventSeeker, CEventData, CEventStillManager` | `[确认]` | `CEvent.cpp` |
| 事件脚本 `EV0..EV6.dk4` `EVA.DK4` 头 `WE`；场景图 `Sc0..Sc6.dk4` | `[确认]` | `formats.md` |
| `CDeckShipView` 甲板编成（方法 18，RTC 桅/舱） | `[确认]` | `CDeckShipView.cpp` |
| `CWaitTriggerView` 等触发 | `[确认]` | `CWaitTriggerView.cpp` |
| `CDiscoverTitleView` 发现称号 | `[确认]` | 见 `11-progression/02-fame-titles.md` |
| `CGoldRoadInfomationView` 黄金航路 | `[确认]` | game 目录 |
| `CGetOldMapWindow` 古地图 | `[确认]` | 同上 |
| 小游戏：`CCatchGameView` `CCoinGameView` `CRatGameView` `CStoneWallGameView` `CWaterGameView` `CNimGameManager` | `[确认]` | RTTI |
| Game Over still 仍从 `TitleSC.dk4` 取图 | `[确认]` | `fn_140115330.cpp` |

精确「从哪一场景可以进哪一场景」的边表**没有**从 vfunc 还原成一张图。下面 6 节的 `allowed` 里，带 `[推断]` 的边来自类职责 + 设施 RTC + 架构文档，实现时要用事件名约束，不得在代码里再写死另一套。

## 3. 实体与 ID

| ID | 含义 | 空 |
| --- | --- | --- |
| `scene_id` | 本文件注册的场景 | `invalid` |
| `scenario_id` | `eScenario` 0–6 | `invalid` = -1 |
| `facility_id` | 港口设施（酒馆、交易所…） | `invalid` |
| `map_point_kind` | `eMapPointKind` 0–11 | `invalid` |
| `view_id` | 叠加 View（存档、状态、技能） | 可空 |

`scene_id` 用稳定 key，不把 `COceanScene*` 传出 UI 层。

12 个 `map_point_kind` 与 9 个已点名设施如何对齐：`[未知]`。配置里设施用自己的 key，地图点表单独列，外键可空。

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
  window_class: KT_DK4_PK_HD_WINDOW_NAME   # 原版窗类，可改
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
    slots: 11                              # savedat00..10
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
      - { key: shipyard, class: CShipyard }
      - { key: remains, class: CRemains }      # 设施对象名 cRemains
      - { key: guild, class: CGuild }
      - { key: inn, class: CInn }
      - { key: palace, class: CPalace }
      - { key: bar, class: CBar }
      - { key: port, class: CPort }
      - { key: trade, class: CTrade }
      - { key: plaza, class: CPlazaView }
    map_point_kinds: 12
    edges:
      - { to: ocean, trigger: Depart }
      - { to: facility, trigger: EnterFacility }
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
    original_class: CMpRemains
    edges:
      - { to: ocean, trigger: Leave }
      - { to: town, trigger: Leave }
  - id: deck
    key: deck
    original_class: CDeckShipView
    overlay: true
  - id: church_game
    key: church_game
    original_class: CChurchGameView
    parent: facility
minigames:
  - CCatchGameView
  - CCoinGameView
  - CRatGameView
  - CStoneWallGameView
  - CWaterGameView
  - CNimGameManager
  - CChurchGameView
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
| 资源 key | 换标题图、场景图 `Sc*`，走语言包 |

改 `NUM_SCENARIO` 是 narrative 的表长；本文件 `count: 7` 只是 vanilla 默认。

## 8. 对外接口

**命令（谁都可以发，场景图裁决）**

```
EnterScene { scene_id, params }
LeaveScene { scene_id }
OpenOverlay { view_id }
CloseOverlay { view_id }
EnterFacility { town_id, facility_id }
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
```

UI 控件（`CSprite` / `CButton` 家族）不进玩法配置，也不出现在上述接口里。

禁止：`CTrade` 从按钮回调里直接改货舱；正确路径是 `CommitDeal` 命令进 economy，成功后再投影。

## 9. 未核实字段

- 完整允许边表（尤其：战斗结束回海还是回港、开局 spawn、事件是否盖住 `CScene`）
- `aeOceanSceneButtonIndex` 0–4 的命令名
- 12 个 `eMapPointKind` 与 9 个已点名设施的对应
- `CTownScene+0x40=3` 的语义
- `CPlazaView` 5 个子对象是什么
- `CScenarioSelectView` 接受的 7 是不是取消
- `CDummyTopView` 是否就是标题根
- `CAcrossGameView` 25 个 vfunc 的职责
- `CSelectSaveDataView` 5 行槽与 11 文件如何滚动
- `Sc0..Sc6.dk4` 与 `scene_id` / `scenario_id` 的对应（文件存在，映射未还原）
- 教会是否算第 10 个设施，还是宫殿/广场的子界面
- 小游戏从哪个设施进
