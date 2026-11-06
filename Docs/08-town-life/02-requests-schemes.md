# 委托与计略

公会接委托、酒馆出计略。本系统拥有两份**目录**（69 条委托、24 条计略）和完成/失败判定。金币、份额、友好度的加减通过命令发给 economy / factions，不在本系统里写市价或关系公式。

## 1. 职责边界

本系统管：

- `eRequest` 69 条实例目录与 kind
- `eScheme` 24 条实例目录与 kind
- 委托人槽 `eRequestPerson`
- 接取 / 交付 / 放弃（违约金文案）
- 计略可否发动（参谋在队、金币是否够）的**入口检查**；费用数字来自本表
- 完成时发奖励命令（金币、地图道具、友好度、份额……）

本系统不管：

| 事项 | 归属 |
| --- | --- |
| 运货的市价与货格 | economy |
| 人物是否在某港、能否入队 | characters |
| 海盗舰队战斗 | combat |
| 村庄/遗迹发现 | world / ocean |
| 介绍信/推荐信的合约效果 | factions / economy |
| 酒馆是否开放 | 设施文档 |
| 计略用的友好度曲线 | factions |

入口：公会 `ListRequests` / `DeliverRequest`；酒馆 `OpenScheme`（mesfile「如果参谋在，可在酒馆里拟定策略。」）。

## 2. 原版事实

### 2.1 计数与容器

`[确认]`

| 符号 | 值 | 来源 |
| --- | --- | --- |
| `NUM_REQUEST` | 69 | `RequestID.h`、`num_values.json` |
| `INVALID_REQUEST` | -1 | 同上 |
| `NUM_SCHEME` | 24 | `SchemeID.h` |
| `INVALID_SCHEME` | -1 | 同上 |
| `NUM_REQUESTKIND` | 头文件无整型 | `RequestkindID.h` 只有符号 |
| `NUM_SCHEMEKIND` | 头文件无整型 | `SchemekindID.h` 只有符号 |
| `NUM_REQUESTPERSON` | 头文件无整型 | `RequestPersonID.h` |

`[确认]` `CRequestElement` 析构/存档循环 `>= 0x46`（70），`vfunc_04` 循环 `eRequest >= 0x45`（69）。ctor：`fn_140007dce(..., stride=0x18, count=0x46)`。`[推断]` 分配 70 槽、有效 69 条，多 1 条哨兵槽。

`[确认]` `CSchemeElement` 循环 `>= 0x18`（24），ctor `stride=0x18, count=0x18`。与 `NUM_SCHEME=24` 一致。

`[确认]` `CWorld` 持有两份表：`+0x182a0` `CSchemeElement`，`+0x19f48` `CRequestElement`（`world/CWorld.cpp`）。

`[确认]` RTC `eRequestPerson >= 0x64`，`aeRequestPerson` 400 字节（`fn_14000e23c`）。`[推断]` `NUM_REQUESTPERSON=100`。该函数按两个匹配条件筛选委托人，把命中 id 写入数组，再把随机选中的 id 存到 `*(s210+0x11)`。

断言只保证 `0 <= id < NUM_REQUESTKIND`，没有写出 kind 个数（`srcdata/catalog/asserts.txt`）。

### 2.2 记录布局

`CRequest` 存档序列化（`vfunc_07` / `vfunc_06`，`world/CRequest.cpp`）`[确认]` 字段偏移：

| 偏移 | 宽度（由读写函数推断） | 用途 |
| --- | --- | --- |
| `+0x08` | 1 | `[未知]` |
| `+0x09` | 1 | kind。`vfunc_02`：若 `<0` 或 `>= 0x0E` 则跳过；`vfunc_04`：`== 0x0F` 返回 0 否则 1 |
| `+0x0A` | 1 | `[未知]` |
| `+0x0C` | 2 | `[未知]`（走 word 读写） |
| `+0x0E`–`+0x11` | 各 1 | `[未知]`；`fn_14000e23c` 写 `+0x11` 为委托人 id |

`[推断]` kind 有效范围 0–13 共 14 档，`0x0F` 为未启用/空。`0x0E` 本身是否合法 kind `[未知]`。`fields.tsv`：`CRequest` 热点 `+0x9`。

`CScheme` 序列化 `[确认]`：`+0x08`–`+0x0D` 六个字节 + `+0x10`。`vfunc_04`：`+0x08 == 0x0B` 返回 0 否则 1。`[推断]` `+0x08` 是 scheme kind，`0x0B` 为空/禁用。`NUM_SCHEMEKIND` 仍 `[未知]`。

`CSchemeElement::vfunc_04` `[确认]` 扫 24 条：先取一个标量（`fn_14001424a`），再对每条比 `fn_140012fb2` 与 `*(s68+0x18)`。`[推断]` 用玩家持有金币筛「付得起的计略」；`+0x18` 是费用或阈值，未命名。

对象体积：`CRequest` / `CScheme` 继承 `CSaveObject`。元素槽步长 0x18。

### 2.3 委托文案：14 种模板

SC mesfile `0x15267`–`0x154d7` 连续「愿意做吗/愿意接受吗」句。源码 **没有** kind 名字。下面按文案出现顺序编号，**不是**已恢复的 `eRequestKind` 值。标记 `[确认]` 文本，`[推断]` 这 14 句对应 `>= 0x0E` 的 14 档。

| 序 | key | 文案要点 | 完成时涉及 |
| --- | --- | --- | --- |
| 0 | `deliver_guild_letter` | 把文书交给 `%s` 的同业公会 | 到港公会交付 |
| 1 | `buy_cargo_here` | 买 `%s` 个仓库的 `%s` 运到**这里** | economy 买货 + 运回 |
| 2 | `buy_cargo_to_town` | 买 `%s` 个仓库的 `%s` 运到 `%s` 的交易所 | 同上，目的港不同 |
| 3 | `find_person_bring` | 寻找住在 `%s` 的 `%s`，带到这里 | characters / 委托人槽 |
| 4 | `escort_person` | 委托人是 `%s`。把他从这个城市带到 `%s` | 护送 |
| 5 | `collect_debt` | 讨回住在 `%s` 的 `%s` 的借款 | 对话收钱 |
| 6 | `capture_criminal` | 捉回名叫 `%s` 的犯罪嫌疑人 | 人物 |
| 7 | `deliver_item_guild` | 把这个 `%s` 送到 `%s` 的同业公会 | 道具 |
| 8 | `buy_item_in_town` | `%s` 在 `%s` 有售，买回来 | items / economy |
| 9 | `find_treasure` | 宝物埋在某处，帮我找回来 | 发现物 |
| 10 | `hunt_pirate` | 讨伐海盗 `%s` | combat |
| 11 | `find_village` | 找到村庄 `%s` 在哪里 | world/ocean |
| 12 | `get_intro_letter` | 弄到介绍信，推荐信更好 | factions |
| 13 | `get_recommend_letter` | 弄来推荐信 | factions |

交付/报酬文案 `[确认]`：

- `0x15537`「确实收到了 `%s` 份 `%s`。」
- `0x1554b` 收金币、捉到人「没想到躲到了 `%s`」
- `0x155a3` 可用「有趣的地图」代替报酬
- `0x15633`「还有，这是剩下的报酬。」
- `0x1564b`「收下报酬吧。」
- `0x15243` 放弃：「要交出违约金 `%s` 枚金币。」
- `0x15223`「从 `%s` 来的吗？谢谢，请代为问好。」（文书类）

UI：`CRequestInformationView`（`game/CRequestInformationView.cpp`），ctor 写 `+0xb4=0x47`（71）。`[未知]` 71 与 69 的差是对齐还是含空槽。

### 2.4 计略文案：效果族，不是 24 个 kind 名

24 是**实例条数**，不是 kind 数。kind 名未恢复。mesfile 里能看到的**效果**（结果句 `0x38c7`–`0x3b77`，交涉句 `0x1479f`–`0x1516f`）`[确认]` 文本：

| key | 成功/失败句 |
| --- | --- |
| `letter_amity` | 「亲笔信已送到%s。友好度上升%s了。」 |
| `alliance` | 「%s提议联合对抗%s」「缔结了对抗%s的协议」 |
| `break_alliance` | 「协议被解除」「撕毁协议」 |
| `ceasefire` | 「%s请求停战」「停战了」 |
| `declare_war` | 「%s向%s宣战」 |
| `merge` | 「%s加入麾下了」「拒绝了合并的请求」 |
| `false_intel` | 「%s收到了假情报。诱导%s好像成功了。」 |
| `false_document` | 「收到了假文书。成功地让%s和%s反目了。」 |
| `rumor` | 「在%s，流传着%s的坏话。」「这传闻没有被相信。」「制造谣言」 |
| `bribe_share` | 「经纪人收取了书信」「夺取了%s的占有率」「收买失败了」 |
| `cut_amity` | 「由于谁的计谋，我们与%s的友好度降低了」 |

`[未知]` 上述 11 个效果族是否等于 `eSchemeKind`，以及 `+0x08==0x0B` 是否落在其中。

前置 `[确认]`：酒馆 + 参谋岗位（`0xc237`）。金币不够时 `CSchemeElement::vfunc_04` 跳过。

### 2.5 与设施的衔接

- 接委托 / 交委托：同业公会。
- 出计略：酒馆（需参谋）。
- 介绍信、推荐信：交易所卖流行品获得（economy 文案 `0x7be3`），委托 12/13 要的是「弄到」它们，效果在宫殿用（`0x7d47`）。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空/非法 | 备注 |
| --- | --- | --- | --- | --- |
| 委托实例 | `eRequest` | 69 | -1 | 内容包目录 |
| 委托种类 | `eREQUESTKIND` | `[未知]` 整型；文案 14 模板 `[推断]` | -1 | `+0x09`，`>=0x0E` 拒 |
| 委托人 | `eREQUESTPERSON` | `[推断]` 100 | -1 | RTC `0x64` |
| 计略实例 | `eScheme` | 24 | -1 | |
| 计略种类 | `eSCHEMEKIND` | `[未知]` | -1 | `+0x08`，`0x0B` 禁用 `[推断]` |

重构 ID：实例 id 与原版 0–68 / 0–23 对齐，便于对照存档。kind 用 key，整数等还原后再锁。

空委托：`kind == empty`（原版候选 `0x0F`），不要用 `id=0` 当空。

## 4. 算法 / 公式

全部参数进 `config/formulas/town.yaml` 与各实例表。禁止 `if kind==3` 散落。

```
can_accept(request_id):
  slot free
  AND kind != empty
  AND preconditions(kind, params)     # 查外系统，见下

abandon_penalty = request.penalty_gold     # 文案 %s，实例表给出

can_cast(scheme_id, player):
  Characters.modifier.can_scheme == true   # 参谋
  AND player.gold >= scheme.cost_gold      # CSchemeElement 与 +0x18 比较
  AND scheme.kind != empty                 # 原版 0x0B

on_complete(request):
  emit RewardGold { amount }               # 可 0
  emit RewardItem { item_id }              # 地图替代报酬可选
  # 按 kind 再发 FindVillage / DefeatFleet / ...
```

kind 前置（查询，不在本系统算）：

| kind key | 查询 |
| --- | --- |
| `buy_cargo_*` | `Economy.canBuy` / 货格 |
| `find_person_*` / `escort` / `capture` | `Characters.locate` |
| `hunt_pirate` | `Combat` 或 ocean 遭遇 |
| `find_village` | `World.villageDiscovered` |
| `get_*_letter` | `Factions.hasLetter` |

计略成功/失败骰子、友好度变化量 `[未知]`，factions 提供 `applySchemeEffect(kind, ids, params)`，params 来自本表。

## 5. 耦合分析

原版：

- `CRequest` / `CScheme` 是 `CSaveObject`，和城镇、人物同一存档链
- 完成委托直接改金币、给道具、改发现状态
- 计略直接改友好度、份额、宣战
- 委托人从 100 人表里抽，比较两个未命名条件（`fn_14000e23c`）
- 循环上界写死 69/24/100

重构后只发：

```
Requests.complete → Economy.grantGold / Items.grant / Factions.grantLetter / Ocean.markVillage
Schemes.cast     → Factions.applySchemeEffect
```

本系统不读六维，不读市价曲线。

## 6. 解耦后的配置

`config/town/request_kinds.yaml`：

```yaml
schema: dk4.town.request_kinds
version: 1
# original_id 未恢复；pack_index 仅原版包文案顺序
kinds:
  - pack_index: 0
    key: deliver_guild_letter
    name: 送达公会文书
    accept_i18n: ui.request.deliver_guild_letter
    needs: [town_id]
  - pack_index: 1
    key: buy_cargo_here
    needs: [trading_item_id, quantity]
  - pack_index: 2
    key: buy_cargo_to_town
    needs: [trading_item_id, quantity, dest_town_id]
  - pack_index: 3
    key: find_person_bring
    needs: [person_id, town_id]
  - pack_index: 4
    key: escort_person
    needs: [person_id, dest_town_id]
  - pack_index: 5
    key: collect_debt
    needs: [person_id, town_id]
  - pack_index: 6
    key: capture_criminal
    needs: [person_id, town_id]
  - pack_index: 7
    key: deliver_item_guild
    needs: [item_id, town_id]
  - pack_index: 8
    key: buy_item_in_town
    needs: [item_id, town_id]
  - pack_index: 9
    key: find_treasure
    needs: [item_id]
  - pack_index: 10
    key: hunt_pirate
    needs: [fleet_id]
  - pack_index: 11
    key: find_village
    needs: [village_id]
  - pack_index: 12
    key: get_intro_letter
    needs: []
  - pack_index: 13
    key: get_recommend_letter
    needs: []
empty_kind_byte: 15          # 原版 +0x09==0x0F；确认前勿当设计真理
kind_reject_ge: 14           # 原版 >=0x0E
```

`config/town/requests.yaml`（69 行，由内容管线从运行时表抽出后补 key）：

```yaml
schema: dk4.town.requests
version: 1
- id: 0
  key: request_00            # 人工补名字前用序号
  kind: deliver_guild_letter
  reward_gold: null          # [未知] 实例值
  penalty_gold: null
  params: {}
  unknown_bytes: []          # +0x08 +0x0A +0x0C +0x0E..+0x10
```

`config/town/scheme_kinds.yaml`：

```yaml
schema: dk4.town.scheme_kinds
version: 1
# 下列 key 来自结果文案，不是已恢复枚举名
kinds:
  - key: letter_amity
  - key: alliance
  - key: break_alliance
  - key: ceasefire
  - key: declare_war
  - key: merge
  - key: false_intel
  - key: false_document
  - key: rumor
  - key: bribe_share
  - key: cut_amity
empty_kind_byte: 11          # 原版 +0x08==0x0B
```

`config/town/schemes.yaml`：

```yaml
schema: dk4.town.schemes
version: 1
- id: 0
  key: scheme_00
  kind: rumor                # 未还原前可空，校验允许 unknown
  cost_gold: null
  requires_advisor: true
  unknown_bytes: []          # +0x09..+0x0D +0x10
```

`config/ids/` 由这两表生成 `request` / `scheme` 注册项。模组可加 `id >= 1000` 的委托，引擎按表长循环。

## 7. 独立旋钮

- 增删委托/计略实例（表长）
- 改 kind 表（新模板 = 新 key + 前置查询名）
- 违约金、计略费用
- 空 kind 哨兵值
- 是否必须参谋才能计略（`requires_advisor`）

不该出现在本表：友好度公式、市价、人物六维阈值。

## 8. 对外接口

事件：

```
RequestAccepted { request_id, town_id, fleet_id }
RequestCompleted { request_id, rewards[] }
RequestAbandoned { request_id, penalty_gold }
SchemeResolved { scheme_id, kind, success, target_ids[] }
```

查询：

```
Requests.listAt(town_id) -> [request_id]
Requests.get(request_id) -> { kind, params, reward_gold }
Schemes.affordable(gold, has_advisor) -> [scheme_id]
```

命令：

```
AcceptRequest { request_id }
DeliverRequest { request_id }
AbandonRequest { request_id }
CastScheme { scheme_id, target_force_id?, target_town_id? }
```

完成时本系统只 emit 奖励事件；economy / factions / items / ocean 订阅。

存档：每条委托/计略的可变字段（是否已接、委托人 id、目标港）。定义（kind、文案 key）不进存档。原版 `CRequest`/`CScheme` 是 `CSaveObject`，重构后按「状态/定义分离」切。

## 9. 未核实字段

- `NUM_REQUESTKIND` / `NUM_SCHEMEKIND` 整数。
- 14 条 mesfile 模板与 `+0x09` 取值 0–13 的对应。
- `+0x08 +0x0A +0x0C +0x0E–+0x10`（委托）、`+0x09–+0x0D +0x10`（计略）。
- `0x0E` 是否合法 kind；`0x0F` / `0x0B` 是否空哨兵（现为推断）。
- 69 条实例各自的 kind、报酬金币、违约金（表未从 DLL 抽出）。
- 24 条计略实例与 11 个效果族的多对一关系。
- `CSchemeElement` 比较的 `+0x18` 确是金币还是名声/等级。
- `fn_14000e23c` 两个匹配条件（`fn_14000baff` / `fn_140008d91`）语义。
- `CRequestInformationView+0xb4=0x47`。
- 同时可接几条委托；是否存档「当前委托槽」。
- 计略成功概率公式。
- `RequestPerson` 类与 `ePerson` 是否同一 235 人表的子集。
