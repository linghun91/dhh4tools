# 剧本

配置落点：`config/narrative/scenarios.yaml`。主角定义外键指向 `config/characters/protagonists.yaml` / `persons.yaml`。世界包与事件包由本表选择，不在经济、战斗、航海代码里写死。

## 1. 职责边界

本系统管：

- 一次开局选哪个 `scenario_id`
- 该局加载哪份世界内容包、哪份事件脚本包、哪份场景图包
- 该局的主角 `person_id`、开局种子（生日等）
- 跨剧本画廊（通关点数换简介 / 照片）的**规则入口**（点数经济仍走 progression / ui）

不管：

- 港口、人物、商品、势力的实例数值（世界包）
- 事件脚本字节码与触发器求值（`02-event-system.md`）
- 贸易、航海、战斗公式
- UI 文案正文（只引用 `string_key`）

原则 7：其它系统禁止 `if scenario_id == 3`。需要差异时读本表给出的 `world_pack` / `event_pack` / 主角修正器，或订阅叙事发出的命令。

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

[确认] 选哪份 DLL 由 `fn_14000ff60`（body `0x1402a89a0`，17 字节）决定：

```
if (id >= 4) return 1;  /* World1.dll */
else         return 0;  /* World0.dll */
```

`CWorldData::vfunc_05`、`CPerson::vfunc_05`、`CTown` / `CFleet` / `CForce` / `CItem` 初始化都走：

```
world_index = fn_14000ff60(*(owner + 0xc))
CDllManager(world_index)  →  LoadLibrary("SC/World%d.DLL")
```

同一条「4」分界出现在跨剧本画廊：`fn_1400089e5` 先扫 `eScenario` 0–3，再从 4 扫到 7。来源：反汇编 `0x1402a89a0`、`srcdata/source/game/world/CWorldData.cpp`、`srcdata/source/game/unassigned/fn_1401200a0.cpp`。

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
| `SC/Sc0.dk4` … `SC/Sc6.dk4` | 该剧本场景图。文件头第一个 `uint32` 等于对应 EV 的触发条目总数 |
| `SC/EVA.DK4` | 事件动画，七剧本共用 |

文件名表在 exe：`SC/EV0.dk4`…`SC/EV6.dk4` 连续存放，`fn_140009985(i)` 取 `QWORD table[i]`（表 VA `0x140682880`）。`CEvent::vfunc_05` 用 `*(session+0xc)` 当剧本下标去加载。来源：`Game/SC/` 实文件、`srcdata/docs/formats.md`、`srcdata/source/game/event/CEvent.cpp`、字符串转储。

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

[确认] 通关其它剧本后，用点数解锁人物简介和额外照片：

> 如果在其他剧本中顺利过关，使用１点就能显示此人物的人物简介，要显示吗？（剩%s点）使用%s点显示额外的照片吗？（剩%s点）

来源：mesfile `0xed0`。UI 类：`CAcrossGameView`、`CAcrossGameCharacterPushButton`（RTTI 名即「跨游戏」）。画廊把 0–3 与 4–6 分成两组扫 `eScenario`。

[未知] 点数存在哪一段存档、通关置位的 bit 布局、照片价是不是固定「%s」。

### 开局条件

[确认] 新局至少写入：主角 `person_id`、`SGameSeed`（含生日输入）。

[未知] 开局港口 `town_id`、开局舰队模板、开局金币。人物表 `+0x18` 起两个 `uint32`（拉斐尔均为 `0x64=100`）可能是资金 / 名声一类初值，字段名未恢复，禁止当成确认的金币。

[推断] 人物行 `+0x26` 为性别：拉斐尔 / 赫德拉姆 / 杏太郎 / 伍丁 = 0，丽璐 / 华梅 / 蒂雅 = 1。依据：与简介性别一致，且 `eSEX` 符号存在。枚举名（男/女）未从 `SexID.h` 恢复。

## 3. 实体与 ID

| 实体 | 原版计数 | 空值 | 稳定 key 规则 |
| --- | --- | --- | --- |
| `scenario_id` | 7 | `invalid` = -1 | 整数与原版 `eScenario` 一致；另给 `raphael`…`tia` |
| `person_id`（主角） | 0–6 是提督，全表 235 | `invalid` = -1 | 外键到人物系统，不在叙事里复制六维 |
| `world_pack` | 原版 2 份 | 无 | `vanilla-world0` / `vanilla-world1` |
| `event_pack` | 原版 7 份 | 无 | `EV0`…`EV6` |
| `scene_pack` | 原版 7 份 | 无 | `Sc0`…`Sc6` |
| `animation_pack` | 原版 1 份共用 | 无 | `EVA` |

`NUM_SCENARIO=7` 只是原版内容包实例数。引擎按 `scenarios.yaml` 表长循环，允许模组追加 `id >= 1000` 或独立命名空间。

## 4. 算法 / 公式

本系统几乎没有战斗式公式。原版唯一硬编码分派写成命名参数：

```
world_pack_index = 0
if scenario_id >= world_pack_split:
    world_pack_index = 1
```

| 参数 | 原版默认 | 落点 |
| --- | --- | --- |
| `world_pack_split` | 4 | `config/narrative/scenarios.yaml` 的全局项，或每条剧本直接写 `world_pack` |

重构**不要**保留 `>= 4` 这条规则当引擎语义。每条剧本显式写 `world_pack:`。`world_pack_split` 只作为从原版 exe 生成 YAML 时的提取规则。

画廊：

```
can_show_bio(person)     = (cleared_other_scenarios(person) && gallery_points >= bio_cost)
can_show_extra_photo     = (cleared_other_scenarios(person) && gallery_points >= extra_photo_cost)
```

| 参数 | 原版 | 落点 |
| --- | --- | --- |
| `bio_cost` | 1 | `scenarios.yaml` / `progression` |
| `extra_photo_cost` | mesfile 为 `%s` | 未还原，先当独立旋钮，禁止写死 1 |

## 5. 耦合分析

原版把「选剧本」焊在三处：

1. **世界 DLL 文件名** `World%d` 与 `scenario_id >= 4` 分支——内容选择混进 `CWorldData` / `CPerson` / `CTown` 初始化。
2. **事件文件名** `EV{n}.dk4` 与 `Sc{n}.dk4` 用同一整数下标，逻辑上等于 `scenario_id`。
3. **其它系统若出现 `if scenario==n`**（原则 7 点名的病）会把剧情分叉泄漏进经济 / 航海。本仓库尚未把每一处 `eScenario` 比较都标成病征；重构验收标准是：除叙事与 UI 选人外，零处读取 `scenario_id`。

允许的依赖方向：叙事 → 查询任意系统 + 发命令。禁止反向。

跨剧本画廊读「其它剧本是否通关」，这是叙事自己的进度，不要让人物系统去读 `scenario_id`。

## 6. 解耦后的配置

```yaml
# config/narrative/scenarios.yaml
schema: dk4.narrative.scenarios
version: 1

gallery:
  bio_cost: 1
  extra_photo_cost: 1          # 原版 %s，未还原；先独立旋钮
  group_a: [0, 1, 2, 3]        # 原版 UI 先扫的 4 人
  group_b: [4, 5, 6]

scenarios:
  - id: 0
    key: raphael
    name: 拉斐尔
    name_i18n: ui.scenario.raphael.name
    blurb_key: ui.scenario.raphael.blurb
    quote_key: ui.scenario.raphael.quote
    recommended_for_first_play: true
    protagonist_person_id: 0
    world_pack: vanilla-world0
    event_pack: EV0
    scene_pack: Sc0
    animation_pack: EVA
    new_game:
      birthday_required: true
      starting_town_id: null          # 未还原，见第 9 节
      starting_gold: null
      starting_fleet_template: null
  - id: 1
    key: helmut
    protagonist_person_id: 1
    world_pack: vanilla-world0
    event_pack: EV1
    scene_pack: Sc1
    animation_pack: EVA
  - id: 2
    key: lulu
    protagonist_person_id: 2
    world_pack: vanilla-world0
    event_pack: EV2
    scene_pack: Sc2
    animation_pack: EVA
  - id: 3
    key: hua_mei
    protagonist_person_id: 3
    world_pack: vanilla-world0
    event_pack: EV3
    scene_pack: Sc3
    animation_pack: EVA
  - id: 4
    key: kyotaro
    protagonist_person_id: 4
    world_pack: vanilla-world1
    event_pack: EV4
    scene_pack: Sc4
    animation_pack: EVA
  - id: 5
    key: woodin
    protagonist_person_id: 5
    world_pack: vanilla-world1
    event_pack: EV5
    scene_pack: Sc5
    animation_pack: EVA
  - id: 6
    key: tia
    protagonist_person_id: 6
    world_pack: vanilla-world1
    event_pack: EV6
    scene_pack: Sc6
    animation_pack: EVA
```

未列出的字段（简介、自称、生日）与 0 号相同 schema，生成管线从 mesfile / `SGameSeed` 填。主角六维**不**复制到本表，人物系统是唯一来源。

加载顺序（与 `05-config-architecture.md` 一致）：读完 caps / formulas / 内容表之后，用本表的 `world_pack` 选出本次运行的世界包，再用 `event_pack` 交给事件解释器。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗 / 航海改代码：

- 某剧本换世界包（World0 ↔ World1，或模组包）
- 某剧本换事件包 / 场景图包
- 主角换成另一个 `person_id`（只要人物表有这一行）
- 增加第 8 个剧本（新 `id` + 新包）
- `bio_cost` / `extra_photo_cost`
- 是否要求生日、推荐新手标记

需要人物系统配合的只有：新主角必须在 `persons.yaml` 有定义。需要世界系统配合的只有：新世界包符合同一 schema。

## 8. 对外接口

事件（本系统发出）：

| 事件 | 载荷 | 谁听 |
| --- | --- | --- |
| `ScenarioSelected` | `scenario_id`, `world_pack`, `event_pack`, `person_id` | 存档会话、世界加载、事件解释器、UI |
| `NewGameSeeded` | `scenario_id`, `seed`（生日等纯值） | 人物 / 舰队开局 |
| `ScenarioCleared` | `scenario_id` | 画廊进度 |
| `GalleryUnlocked` | `person_id`, `kind=bio\|photo` | UI |

查询（本系统只读别人）：

```
Characters.get(person_id) -> PersonDef
World.pack_exists(world_pack) -> bool
Events.pack_exists(event_pack) -> bool
Progression.gallery_points() -> int
Progression.scenario_cleared(scenario_id) -> bool
```

命令（别人向本系统）：

```
Narrative.select_scenario(scenario_id, seed) -> Session
Narrative.clear_scenario(scenario_id)          # 通关
Narrative.spend_gallery(person_id, kind)       # 扣点失败则 no-op
```

只传 ID 与纯值。禁止传 `World*`、`CEvent*`、DLL 句柄。

## 9. 未核实字段

| 位置 | 现状 | 禁止 |
| --- | --- | --- |
| 开局 `town_id` | 人物行与 `SGameSeed` 均未命名到港口 | 不要用简介里的「葡萄牙 / 瑞典 / 荷兰」猜里斯本 / 斯德哥尔摩 / 阿姆斯特丹并写成确认 |
| 开局金币、舰队、货物 | 人物 `+0x18` 两个 `uint32` 未命名 | 不要把拉斐尔的 `100, 100` 写成「确认的 100 金币」 |
| 人物 `+0x27` 起若干字节 | 拉斐尔 `04 02 02 00 00 01`，各主角不同 | 不要当成 force / region / job |
| `SGameSeed` `0x78` 字节 | 构造写 `+0x70`，拷贝 `+0x8/+0xc/+0x58/+0x5c` | 除「含生日」外字段一律 unknown_bytes |
| 存档 `+0x20` 与 id 6 | 注释只写到伍丁 | 读档实现按 uint32 0–6 测，不要根据注释丢掉蒂雅 |
| 画廊点数存档偏移、通关 bit | 只有文案与 `CAcrossGame*` 类名 | 不要编点数上限 |
| `aeScenarioPortrait` 长度为 8 | RTC 局部，7 个剧本 + 1 个空槽？ | 标 [未知] |
| World1 相对 World0 的人物未命名字节 | 例如 Raphael `+0x30` World0=0、World1=1 | 进 `unknown_bytes`，不要猜「世界包标记」 |

提取器只输出确认字段；残余进 `unknown_bytes`。
