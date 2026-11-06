# 名声与发现称号

原版没有还原出独立的「名声值」字段。能钉住的是发现物 UI（`CDiscoverTitleView`）和一批与「发现」相关的状态位、文案。本系统把这些收成可配置的发现进度，不为原版补一个仓库里不存在的数值。

## 1. 职责边界

本系统管：

- 发现进度：村落、遗迹、古地图、发现物是否已被玩家记录
- 发现称号的**展示定义**（称号 id、解锁条件、文案 key、小地图）
- 若日后还原出名声标量：其上下限、增减命令、查询接口

不管：

- 村落/遗迹在世界地图上的坐标与外观（`world`）
- 发现物作为 `item_id` 的装备效果（`items`）
- 探险队遇温泉、绿洲、兽袭的航海随机事件（`ocean`，只在结算时对本系统发 `Discover*`）
- 海域「知名度」文案（`%s在本海域的知名度还是低的`）——这是势力/商品知名度，归 `factions` / `economy`
- 人物传记里出现的「名声」二字（mesfile 传记，不是玩法字段）
- `CDiscoverTitleView` 的绘制与缩放动画（`ui`，只读本系统快照）

## 2. 原版事实

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 无 `eFame` / `CFame` / `NUM_FAME` 符号 | `[确认]` | `constants.json` `rtti_names.txt` `*ID.h` 全表检索 |
| mesfile 里「名声」只出现在人物传记，不是 HUD 数值 | `[确认]` | `mesfile_sc.txt` 两处传记 |
| 「称号」一词只出现在德雷克传记「爵士称号」，不是玩家称号系统 | `[确认]` | `mesfile_sc.txt` `0x224de` |
| `CDiscoverTitleView`：发现称号画面，体积 `0x100`，继承 `CView` | `[确认]` | `class_hierarchy.txt` `CDiscoverTitleView.cpp` |
| `CDiscoverTitleMiniMap`：称号画面内小地图 | `[确认]` | 同上；`vfunc_04` RTC `rcMiniMap`，有缩放（`+0x40` 右移减半）与 0x14 帧插值 |
| 村落 24 个，`INVALID_VILLAGE=-1` | `[确认]` | `VillageID.h` `NUM_VILLAGE=24` |
| 遗迹枚举存在，`NUM_REMAINS` 整数未还原；有 `REMAINSSTATE_BITS` | `[确认]` | `RemainsID.h` `Remains.h` `asserts.txt` |
| 遗迹是 `CSaveObject`，可进存档 | `[确认]` | `Remains.h` `CRemains -> CSaveObject` |
| 村庄是 `CSaveObject`（`CVillage` 体积 `0x48`） | `[确认]` | `CVillage.cpp` |
| 城镇场景会挂 `cRemains` | `[确认]` | `CTownScene::vfunc_02` RTC：`cShipyard,cRemains,cGuild,cInn,cPalace,cBar,cPort,cTrade,cPlazaView` |
| `CMpRemains` 是港口地图上的遗迹点 | `[确认]` | `CMpRemains.cpp` |
| 古地图窗口 `CGetOldMapWindow`、`COldMapInfomationView`、`COldMapSprite`、`COldMapButton` | `[确认]` | RTTI / `class_files.tsv` |
| 文案：「发现了村庄」「没有被发现的遗迹」「发现了%s」「重大发现」「霸者之证」 | `[确认]` | `mesfile_sc.txt` `0xb3f` `0xca27` `0x318b` `0x1024b` `0x112ab` |
| 「霸者之证」按文化圈检索（东南亚 / 东亚 / 新大陆） | `[确认]` | mesfile `0x1e643` `0x1e76b` `0x112ab` 一带 |
| 道具表 231 项，含发现物/地图，不是交易品 | `[确认]` | `04-id-registry.md` `GetFixItemProc` |
| 玩家侧有 `PLAYERSTATE_BITS` | `[确认]` | `PlayerFleetData.h` `asserts.txt`；**位含义未还原** |
| 发现称号的完整名单、解锁阈值、是否消耗名声 | `[未知]` | 无表、无公式、无枚举 |
| 个人「名声」标量是否存在于存档某处 | `[未知]` | 已还原的存档头/名册/主角体均无此字段名 |
| `CDiscoverTitleView` 的数据源（读遗迹位？读道具？读独立表？） | `[未知]` | 反编译只见绘制与小地图动画 |

因此：本篇把「发现称号」做成**配置化的解锁规则 + UI 投影**，把「名声」留成可选标量接口，默认 vanilla 包可以不启用标量。

## 3. 实体与 ID

| ID | 原版计数 | 空/非法 | 备注 |
| --- | --- | --- | --- |
| `village_id` | 24 | `invalid` = -1 | 定义在 `world` |
| `remains_id` | `[未知]` | `invalid` = -1 | `NUM_REMAINS` 未还原，运行时读表长 |
| `item_id` | 231 | `invalid` = -1 | 发现物/古地图/霸者之证 |
| `title_id` | `[未知]` | `invalid` = -1 | 重构新增稳定 id；vanilla 包在还原名单前可为空表 |
| `discovery_kind` | 配置 | — | `village` `remains` `item` `old_map` `gold_road` … |
| `fame_id` | 可选 | 不使用时不注册 | 原版无；模组若加「商会名声」走此 id |

状态不是 ID：已发现用 `discovered: bool` 或 bitfield 快照，不把 `World*` 存进本系统。

`REMAINSSTATE_BITS` / `PLAYERSTATE_BITS` 的位宽未还原。重构存档不用原版 bit 布局，用 `id -> flag` 映射；导入器再把原版位展开。

## 4. 算法 / 公式

原版**没有**可引用的名声公式。禁止编造「发现 +10 名声」。

重构默认规则（vanilla 包）：

```
on DiscoverVillage(village_id):
  if already: no-op
  else mark discovered
  emit DiscoveryUnlocked { kind: village, id: village_id }

on DiscoverRemains(remains_id):
  同上，kind: remains

on AcquireDiscoveryItem(item_id) where item.flags.discovery:
  mark item discovered
  emit DiscoveryUnlocked { kind: item, id: item_id }

title_unlocked(title_id) =
  all(title.requires) 为真
```

`requires` 只允许：

- `discovered.village == village_id`
- `discovered.remains == remains_id`
- `discovered.item == item_id`
- `count(kind) >= n`
- （可选）`fame >= n` —— 仅当内容包启用名声标量

称号不存「已装备哪个」除非内容包声明可展示槽。默认：所有已解锁称号可在 `CDiscoverTitleView` 对应画面列出，当前高亮由 UI 会话持有，不进玩法。

若启用名声标量：

```
fame' = saturate(fame + delta, fame_min, fame_max)
```

`fame_min` / `fame_max` 进 `caps.yaml`。原版无默认值 → vanilla 包 `enabled: false`。

## 5. 耦合分析

原版：

- 发现状态可能散落在 `CVillage` / `CRemains` / `ITEMSTATE_BITS` / `PLAYERSTATE_BITS` / 道具背包
- 称号画面直接画小地图，坐标来自世界表
- 城镇场景把 `cRemains` 当设施挂在 `CTownScene` 上
- 「霸者之证」同时是道具、剧情钥匙、发现物
- 海域知名度文案容易被误当成人物名声

重构：

- world 只提供坐标与 `village_id` / `remains_id`
- items 只提供 `item_id` 与 `discovery: true`
- narrative 可查询 `title_unlocked` / `discovered`
- ui 只读 `DiscoverySnapshot`
- 本系统不持有 `Location` / `World*`

## 6. 解耦后的配置

```yaml
# config/progression/fame.yaml
schema: dk4.progression.fame
version: 1
fame:
  enabled: false              # vanilla：仓库未还原标量
  min: 0
  max: 9999                   # 仅 enabled 时生效；数值任意，不是原版事实
titles:
  # 名单未还原前保持空数组。导入器不得臆造「男爵/子爵」。
  []
discovery_kinds:
  - key: village
    source_id: village_id
  - key: remains
    source_id: remains_id
  - key: item
    source_id: item_id
    filter: { item_flag: discovery }
  - key: old_map
    source_id: item_id
    filter: { item_flag: old_map }
```

称号条目一旦从原版还原，形状如下（示例，**不是**原版名单）：

```yaml
# 仅说明 schema，禁止当内容用
- id: 0
  key: example_only
  name_key: title.example
  requires:
    - { kind: village, count: 24 }
  minimap: true
```

`config/caps.yaml` 片段：

```yaml
fame_enabled: false
# fame_max 在 enabled 之后才校验
village_count: 24            # 原版包实例数；运行时读表长
```

文案全部走 `ui/strings.*.yaml`（`title.*` `discovery.*`），本文件不写中文句子。

## 7. 独立旋钮

| 旋钮 | 文件 | 说明 |
| --- | --- | --- |
| `fame.enabled` | `fame.yaml` | 关则接口恒返回 0，UI 隐藏条 |
| `titles[]` | `fame.yaml` | 加称号不必改引擎 |
| `requires` | 同上 | 改解锁条件 |
| `discovery_kinds` | 同上 | 加一种发现源（须已有 ID） |
| 称号名 / 发现提示 | 文案包 | 不进逻辑 |

改 `village_id` 表长是 world 的事；本系统只按表长做 `count`。

## 8. 对外接口

**查询**

```
Discovery.is_found(kind, id) -> bool
Discovery.count(kind) -> u32
Discovery.snapshot() -> { villages: [id], remains: [id], items: [id], titles: [title_id] }
Title.unlocked(title_id) -> bool
Fame.value() -> u32          # enabled=false 时恒 0
```

**命令**

```
MarkDiscovered { kind, id }
ClearDiscovered { kind, id }           # 仅调试/导入回滚；玩法默认只加不减
GrantFame { delta, source_tag }        # enabled=false 时 no-op
```

**事件**

```
DiscoveryUnlocked { kind, id }
TitleUnlocked { title_id }
FameChanged { old, new }               # 仅 enabled
```

UI：`OpenDiscoverTitleView` 是场景命令，不写本系统状态。小地图坐标向 world 现取。

## 9. 未核实字段

- 个人名声标量：有无、偏移、上下限、增减来源
- `title_id` 名单与文案（`CDiscoverTitleView` 的列表数据源）
- `NUM_REMAINS` 与每条遗迹的 `REMAINSSTATE_BITS` 位语义
- `PLAYERSTATE_BITS` / `ITEMSTATE_BITS` 是否含「已发现」
- 古地图与遗迹、道具 id 的对应表
- 「霸者之证」是 `item_id` 子集还是独立枚举
- `CDiscoverTitleView+0xb8/0xe0/0xe8` 指针、`+0x40/0x44` 缩放帧以外的字段
- `CDiscoverTitleMiniMap` 字段（`fields.tsv` 一堆偏移未命名）
- 发现称号是否影响交涉、投资、剧情触发（narrative 查询接口预留，默认不读）
- 开局是否已发现部分村落/遗迹（剧本差）
