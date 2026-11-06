# 剧本主角

七个可选提督（`ePerson` 0–6）与七个剧本（`eScenario` 0–6）的人物侧规格。剧本事件脚本、触发器属 `10-narrative`；本文件只定「选谁、带什么初始人物数据、World0/World1 差在哪」。

## 1. 职责边界

本系统（人物）管：

- 七个主角的姓名、六维、初始经验、短介绍 / 长传记的文案 key
- 开局时把该 `person_id` 写入名册主角槽，并把同伴按剧本内容包列入名册
- World0 / World1 两份主角数值差（同一 schema，两份数据）

不管：

- EV0–EV6 事件脚本、过关条件、霸者之证（`10-narrative`）
- 开局舰队船体、货舱、金币（`05-ships-fleet` / `04-economy`）
- 「请选择３种熟练的技能」的公式效果（技能定义在 `03-abilities-skills-jobs.md`，开局流程在 `04-recruitment-crew.md`）
- 生日怎么进存档（偏移未还原）

叙事系统用 `protagonist_person_id` 选人，不在贸易/战斗里写 `if scenario==3`。

## 2. 原版事实

- `[确认]` `NUM_SCENARIO = 7`，`INVALID_SCENARIO = -1`。来源：`ScenarioID.h`。
- `[确认]` 七个剧本对应七个提督 `ePerson` 0–6。来源：`PersonID.h` 注释、`id-registry.md`、mesfile 选人台词。
- `[确认]` 选人短介绍在 mesfile SC `0xbab` 起与「请输入生日」同一串，后接 `0xcbb` `0xd73`。来源：`mesfile_sc.txt`。
- `[确认]` 自报家门口号：`0xe1b` 拉斐尔/赫德拉姆，`0xe5e` 丽璐，`0xe83` 华梅/杏太郎，`0xed0` 伍丁/蒂雅。
- `[确认]` 长传记从 `0x1ef2f` 拉斐尔起，按人连续排到蒂雅 `0x1f943`，其后是同伴传记。来源：`mesfile_sc.txt`。
- `[确认]` World0 六维见下表。来源：`GetFixPersonProc` / `Docs/_data/persons_world0.json` / `id-registry.md`。
- `[确认]` World1 与 World0 **同名**，但 9 人六维不同（主角里是赫德拉姆、丽璐、华梅；军官另有弗利奥、克莉丝汀娜、铁礼列、安杰洛、易安）；id 202 的姓名字段 World1 写成「中南美/长老」。来源：`World0_person.c` vs `World1_person.c`、`world1_names.json`。
- `[确认]` 新游戏还要「请输入生日」。来源：mesfile `0xbab`。
- `[确认]` 内容包切换：`narrative/scenarios.yaml` 指定 `world_pack` + `event_pack`（EV0–EV6）。来源：`05-config-architecture.md`。
- `[推断]` 世界舰队表 id 0–6 的 `+0x10` 为 `uint16` 港口，对应七个开局港：里斯本、斯德哥尔摩、阿姆斯特丹、杭州、马尼拉、巴士拉、波托韦洛。依据：`World0_fleet.c` 前七行该字节为 `0x0B 0x07 0x02 0x36 0x31 0x26 0x43`，与 `towns.tsv` 11/7/2/54/49/38/67 对得上，且与各人传记地理一致。舰队行其余字节未解码，故开局港不升为确认。
- `[推断]` 世界表两段 `uint32` 为开局经验。World0 拉斐尔 100/100，赫德拉姆 2400/5400；World1 赫德拉姆改为 1800/3400、丽璐 2800/250、华梅 8600/4200、伍丁 4500/7500。见 `01-person-model.md`。

## 3. 实体与 ID

| id | key | 名 | 姓 | 第三名 | 开局港 `[推断]` | 事件包 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | `raphael` | 拉斐尔 | 卡斯特罗 | （空） | 里斯本 `lisbon` | EV0 |
| 1 | `hedram` | 赫德拉姆 | 柏格斯通 | 约阿其姆 | 斯德哥尔摩 `stockholm` | EV1 |
| 2 | `liru` | 丽璐 | 阿歌特 | （空） | 阿姆斯特丹 `amsterdam` | EV2 |
| 3 | `hua_mei` | 华梅 | 李 | 玛莉亚 | 杭州 `hangzhou` | EV3 |
| 4 | `kyotaro` | 杏太郎 | 佐伯 | （空） | 马尼拉 `manila` | EV4 |
| 5 | `woodin` | 亚伯拉罕 | 伍丁 | 易文 | 巴士拉 `basra` | EV5 |
| 6 | `tia` | 蒂雅 | 恰斯卡 | 瓦曼 | 波托韦洛 `portobelo` | EV6 |

`person_id` 与 `scenario_id` 在原版内容包里相等。模组可以让某个剧本用别的 `person_id`，但必须在 `scenarios.yaml` 显式写 `protagonist_person_id`，禁止引擎假设 `scenario_id == person_id`。

存档 `+0x20` 存的是**名册水手 id**（0 拉斐尔 … 5 亚伯拉罕；蒂雅为 6）。`save_format.md` 原文举例写到 5，id 6 同样合法。

## 4. 算法 / 公式

开局不是公式，是一次「内容包 → 名册」的装填：

1. 读 `scenarios.yaml` 得到 `world_pack`、`event_pack`、`protagonist_person_id`。
2. 从该世界包的 `persons.yaml` 取主角六维与 `init_exp`。
3. 名册槽 `person_id` 写入六维、两段经验、隶属=玩家势力。
4. 同步主角体 `+0x35AA` 一组字段，并写 `+0x20`、`+0x70`。
5. 同伴名单来自该剧本的 `companions:` 列表（叙事表），人物系统只执行 `PersonRecruited`。
6. 玩家选 3 个熟练技能（`04-recruitment-crew.md`），人物系统记下 `proficient_skill_ids[3]`。
7. 生日写入叙事/人物状态（存档偏移 `[未知]`）。

没有「按剧本 id 分支」的人物公式。World1 的数值差已经在内容包里，不在代码里。

### World0 主角六维 `[确认]`

| id | 体力 | 敏捷 | 魅力 | 智力 | 精神 | 运气 | init_exp `[推断]` |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | 59 | 60 | 70 | 62 | 58 | 61 | 100, 100 |
| 1 | 83 | 69 | 78 | 55 | 73 | 48 | 2400, 5400 |
| 2 | 49 | 54 | 69 | 70 | 78 | 66 | 3100, 250 |
| 3 | 66 | 58 | 81 | 86 | 69 | 78 | 12600, 7400 |
| 4 | 79 | 59 | 80 | 53 | 71 | 51 | 1500, 4000 |
| 5 | 78 | 85 | 60 | 55 | 81 | 60 | 6000, 11000 |
| 6 | 45 | 61 | 68 | 81 | 62 | 55 | 100, 800 |

### World1 相对 World0 的六维差 `[确认]`

未列出的维与 World0 相同。姓名三字段 0–6 两包一致。

| id | World1 六维 | 相对 World0 |
| --- | --- | --- |
| 0 | 59 60 70 62 58 61 | 无差 |
| 1 | 82 68 77 54 73 47 | 体−1 敏−1 魅−1 智−1 运−1 |
| 2 | 48 53 68 70 77 66 | 体−1 敏−1 魅−1 神−1 |
| 3 | 66 58 80 85 68 77 | 魅−1 智−1 神−1 运−1 |
| 4 | 79 59 80 53 71 51 | 无差 |
| 5 | 78 85 60 55 81 60 | 无差 |
| 6 | 45 61 68 81 62 55 | 无差 |

军官差（人物包一并改，不单属主角）：弗利奥 魅 66→65、运 68→67；克莉丝汀娜 敏 86→85、神 57→66；铁礼列 智 78→77；安杰洛 魅 80→79；易安 魅 79→78。

### 选人短介绍 `[确认]` mesfile

| id | 原文（SC `0xbab` / `0xcbb` / `0xd73`） |
| --- | --- |
| 0 | 莫名地憧憬着要当一名水手的葡萄牙少年。受朋友的鼓动而出海。推荐初次玩此游戏的玩家选择他。 |
| 1 | 实现再次独立的瑞典提督。率领徒有海军虚名的非官方舰队。为把本国舰队建成世界最强的舰队而奋斗。 |
| 2 | 经商的雄心极其旺盛的荷兰少女。和童年伙伴卡米尔一起梦想做一番大事业而周游世界。 |
| 3 | 为了保卫受倭寇侵略之苦的人们，敢于漠视被订为国法的锁国政策，率领非官方舰队进行斗争的明朝女提督。 |
| 4 | 九州岛某豪族的后裔。一族人被敌人袭击，父亲剖腹自杀。被追杀，听天由命地坐着小渔船出海。 |
| 5 | 阿拉伯大富商的后代。表面上是为了兴趣在收集古董，实际上是为了追查自己身世的秘密而漫无目的地旅行。 |
| 6 | 继承统治印加帝国一个地方之王族血统的混血儿。担任埃斯康特军的翻译……。 |

自报家门：拉斐尔「是初次出海」；赫德拉姆「正努力建成世界最强的舰队」；丽璐「全世界都是我的交易伙伴」；华梅「我不允许倭寇横行霸道」；杏太郎「我一定要为族人报仇」；伍丁「在阿拉伯经营商会」；蒂雅「我一定要独立」。

长传记文案 key 指向 mesfile `0x1ef2f` 一段（葡萄牙次子、小提琴与读书、库拉乌迪青梅竹马）至 `0x1f943`（蒂雅/印加混血、埃斯康特翻译、塞维）。重构后进 `ui/strings.zh-CN.yaml`，逻辑配置只留 key。

## 5. 耦合分析

原版：

- 选人界面、EV 脚本、世界 DLL、主角 id 绑在一起。`if (scenario==3)` 会散落。
- 开局港写在舰队表 hex 里，人物表自己没有 `home_town_id` 字段。
- World1 不只换事件，还改若干人六维与初始经验，像「同一世界的另一组平衡」而不是「新地图」。

重构：

- `scenarios.yaml` 是唯一「这个剧本加载哪份世界包、哪个主角、哪份 EV」的地方。
- 开局港属于舰队模板（`fleets/templates.yaml`），人物包用 `person_id` 关联，不在 `persons.yaml` 里复制一份坐标。

## 6. 解耦后的配置

人物侧：`config/characters/protagonists.yaml`（短介绍 key、熟练技能默认、与剧本的关联）。叙事侧：`config/narrative/scenarios.yaml`。

```yaml
# config/narrative/scenarios.yaml   # 叙事拥有
schema: dk4.narrative.scenarios
version: 1
- id: 0
  key: raphael
  world_pack: vanilla-world0
  event_pack: EV0
  protagonist_person_id: 0
  start_fleet_template: raphael_start   # 内含 start_town_id: lisbon
  companions: [8]                       # 库拉乌迪；完整名单由叙事表给

# config/characters/protagonists.yaml  # 人物拥有
schema: dk4.characters.protagonists
version: 1
- person_id: 0
  key: raphael
  blurb_key: ui.scenario.raphael.blurb
  bio_key: ui.scenario.raphael.bio
  quote_key: ui.scenario.raphael.quote
  recommend_for_first_play: true
  default_proficient_skills: []     # 玩家自选 3 个；可给新手推荐但不写死
```

World1 不是另一套主角 id，只是 `world_pack: vanilla-world1` 里同一 `person_id` 的六维/经验不同。

## 7. 独立旋钮

- 换短介绍 / 长传记文案（只动 strings）
- 某剧本改用另一个 `person_id`（只动 `scenarios.yaml`）
- World1 再调主角六维（只动 world1 人物包）
- 把开局港从里斯本改到休达（只动舰队模板）

不要把「华梅必须从杭州开」写进人物代码。

## 8. 对外接口

```
Narrative.current_scenario() -> scenario_id
Characters.protagonist() -> person_id

# 开局
Commands:
  StartScenario { scenario_id, proficient_skill_ids[3], birthday }
    → 人物装填名册
    → 舰队系统生成 start_fleet_template
    → 叙事加载 event_pack
```

查询：`PersonSnapshot` 与普通人相同，另加 `is_protagonist: true`。舰长室「只能配置主角」的规则在岗位表（`posts.yaml` 的 `protagonist_only`），不在主角文档里写死舱室。

## 9. 未核实字段

- 开局港：舰队 `+0x10` 的解读是 `[推断]`；舰队其余 0x18 字节未命名。
- 各剧本开局同伴的完整 `person_id` 列表：传记提到库拉乌迪（拉斐尔）、卡米尔（丽璐）、希恩（华梅）、塞维（蒂雅）等，但未从 EV 脚本还原成表。`companions:` 等叙事文档补。
- 生日存档偏移、星座是否影响六维：`[未知]`。
- 「使用１点显示人物简介」（`0xed0`）的点数系统：属叙事/图鉴，本文件不收。
- World1 id 202 姓名改「中南美/长老」是否影响任何主角流程：`[未知]`，只记差异。
- `save+0x20` 原文举例「0…5 亚伯拉罕」，蒂雅 id 6 是否在所有语言档都以 6 写入：按枚举应是，未拿七份新存档逐字节对。
