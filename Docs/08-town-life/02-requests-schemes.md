# 委托与计略

公会接委托、酒馆出计略。原版**没有**委托/计略内容目录：69 个委托槽、24 个计略槽都是运行时槽，进港时按规则现场生成。真正的「目录」是 14 种委托 kind 与 10 种计略 kind。本篇写委托；计略只留摘要，细节在 [04-schemes-rhetoric.md](04-schemes-rhetoric.md)。金币、份额、友好度的加减通过命令发给 economy / factions，不在本系统里写市价或关系公式。

## 1. 职责边界

本系统管：

- `eREQUESTKIND` 14 种 kind 目录（名字、参数形状、生成规则）
- 委托槽：69 个港口公告槽 + 1 个玩家当前委托槽
- 进公会时的委托生成（每港最多 3 条）
- 委托涉及人物表 `RequestPerson`（100 条）
- 接取 / 交付 / 放弃（违约金）
- 完成时发奖励命令（金币、地图道具、友好度、份额……）
- 计略：只做入口转发，见 04 篇

本系统不管：

| 事项 | 归属 |
| --- | --- |
| 运货的市价与货格 | economy |
| 人物是否在某港、能否入队 | characters |
| 海盗舰队战斗 | combat |
| 村庄/遗迹发现 | world / ocean |
| 介绍信/推荐信的合约效果 | factions / economy |
| 酒馆是否开放 | 设施文档 |
| 计略 kind、槽、到期、效果 | [04-schemes-rhetoric.md](04-schemes-rhetoric.md) |

入口：公会 `ListRequests` / `DeliverRequest`；酒馆 `OpenScheme`（mesfile `0xc237`「如果参谋在，可在酒馆里拟定策略。」）。

## 2. 原版事实

### 2.1 计数与容器

`[确认]`

| 符号 | 值 | 来源 |
| --- | --- | --- |
| `NUM_REQUEST` | 69 | `RequestID.h`、`num_values.json` |
| `INVALID_REQUEST` | -1 | 同上 |
| `NUM_SCHEME` | 24 | `SchemeID.h` |
| `INVALID_SCHEME` | -1 | 同上 |
| `NUM_REQUESTKIND` | 头文件无整型 | `RequestkindID.h` 只有符号；见下 |
| `NUM_SCHEMEKIND` | 头文件无整型 | `SchemekindID.h` 只有符号；`[推断]` 10，见 04 篇 |
| `NUM_REQUESTPERSON` | 头文件无整型 | `RequestPersonID.h`；见下 |

`[确认]` 没有 `GetFixRequestProc` / `GetFixSchemeProc`。World DLL 只导出 Fleet/Force/Item/Person/Town 五个 `GetFix*`。委托和计略不在内容包里。来源：`srcdata/source/world/DllMain.cpp`、`strings Game/SC/World0.dll`。

`[确认]` `CRequestElement` ctor：`fn_140007dce(this+8, stride=0x18, count=0x46)`，70 个 `CRequest`。析构/存档循环 `>= 0x46`。来源：`world/CRequestElement.cpp`。

`[确认]` 第 70 槽（下标 `0x45`）是**玩家当前委托**，不是哨兵：

- 接受委托时把选中槽 0x18 字节整体复制到 `fn_1400027f7(0x45)`（`fn_14000fb5a(dst, src, 0x18)`），再把原槽清空。来源：`fn_14015b600`（body `0x14015b600`，`0x14015bf8a`–`0x14015c0ce`）。
- 公会菜单先看 `slot[0x45]` 是否非空，非空走交付分支，否则列本港委托。来源：`fn_14000b366`（body `0x14015aec0`）。
- 放弃、交付检查、港内找人都读 `slot[0x45]`（`fn_1400093f4`、`fn_14014dff0`、`fn_14015eb80` 等）。

`[推断]` 玩家同时只能持有 1 条委托。依据：只有一个玩家槽；公会菜单在 `slot[0x45]` 非空时只走交付分支，不再列新委托。

`[确认]` 港口公告槽是 0–68。找空槽 `fn_14000182f`（body `0x1401f0020`）扫 `eRequest < 0x45`。`CRequestElement::vfunc_04`（thunk `0x14000936d`，body `0x1401f0a20`）对 0–68 逐个 `fn_14000e1f6`（只写 `+0x09 = 0x0F`）清空，玩家槽不动。它由世界 tick `0x1402aaf10` 在 `fn_14000e9da(0x1442a7b10)`（该对象 `+0x1c == 0`）时调用。`[确认]` `0x1442a7b10` 是 `fn_14000277f` 每拍拷出的 `SCalendar` 快照，`+0x1c == 0` 是「月首第一拍」（`01-world/07-calendar-and-coords.md` 2.3–2.4，调度表第 8 项）。即**每月初整板作废**，进公会时再现场生成。

`[确认]` 本港公告列表 `fn_140006078`（body `0x14015dd20`）扫 `eRequest < 0x46`（含玩家槽），收 `kind != 空` 且 `+0x08 == 当前港` 的槽。

`[确认]` `CSchemeElement` 循环 `>= 0x18`（24），ctor `stride=0x18, count=0x18`。与 `NUM_SCHEME=24` 一致。

`[确认]` `CWorld` 持有两份表：`+0x182a0` `CSchemeElement`，`+0x19f48` `CRequestElement`（`world/CWorld.cpp`）。

### 2.2 委托 kind：14 种，名字表在 exe

`[确认]` `CRequest::vfunc_02`（body `0x1401f0180`）：`kind = movzx byte +0x09`；`kind < 0` 或 `>= 0x0E` 返回 NULL，否则返回 `0x1405873f0[kind]`。该表 14 个指针，第 15 格起为 0。SC 字串：

| kind | key | SC 名 | 接受消息号 | 句意 |
| --- | --- | --- | --- | --- |
| 0 | `deliver_guild_letter` | 传递文书 | `0xb2e` | 把此文书交给 `%s` 的同业公会 |
| 1 | `buy_cargo_here` | 购买交易品 | `0xb2f` | 买 `%s` 个仓库的 `%s` 运到这里 |
| 2 | `buy_cargo_to_town` | 运送交易品 | `0xb30` | 买…运到 `%s` 的交易所 |
| 3 | `find_person_bring` | 找人 | `0xb31` | 寻找居住在 `%s` 某处的 `%s`，带到这里 |
| 4 | `escort_person` | 护送 | `0xb32` | 委托人是 `%s`，从这个城市带到 `%s` |
| 5 | `collect_debt` | 催收欠款 | `0xb33` | 讨回住在 `%s` 的 `%s` 的借款 |
| 6 | `capture_criminal` | 搜捕犯人 | `0xb34` | 捉回犯罪嫌疑人 `%s` |
| 7 | `deliver_item_guild` | 运送宝物 | `0xb35` | 把这个 `%s` 送到 `%s` 的同业公会 |
| 8 | `buy_item_in_town` | 购买宝物 | `0xb36` | `%s` 在 `%s` 某处有售，买回来 |
| 9 | `find_treasure` | 寻找宝物 | `0xb37` | 宝物埋在某处，帮我找回来 |
| 10 | `hunt_pirate` | 讨伐海盗 | `0xb38` | 讨伐海盗 `%s` |
| 11 | `find_village` | 探索村子 | `0xb39` | 找到村庄 `%s` 在哪里 |
| 12 | `get_intro_letter` | 取得介绍信 | `0xb3a` | 弄到介绍信，推荐信更好 |
| 13 | `get_recommend_letter` | 取得推荐信 | `0xb3b` | 弄来推荐信 |

`[确认]` 接受消息号 = `0xb2e + kind`，按 exe 消息号表（`.data 0x140683620` / `0x1406836c0`）解出，14 句与 kind 一一对应。旧稿给的是 mesfile 块偏移（`0x15267` 等），不作文案 key（D10）。

名字表顺序与 mesfile 模板顺序一致。kind 编号由名字表下标钉死，不再是文案顺序推断。

`[确认]` 接受对话：`fn_14015b600` 按 kind 0–13 跳表（`cmp kind, 0xd` / `ja`）。部分 kind 走专用分支（消息 `0xb2e`–`0xb32`、`0xb35`–`0xb39`，带参数格式化），其余走 `0xb2e + kind`。

`[推断]` `NUM_REQUESTKIND = 14`。依据：名字表 14 格、`vfunc_02` 上界 `0x0E`、接受 switch 上界 13。空值 `0x0F`（15）落在枚举外；`0x0E` 不是合法 kind。

`[确认]` `CRequest::vfunc_04`（body `0x1402aa3c0`）：`+0x09 == 0x0F` 返回 0，否则 1。即「槽是否占用」。

### 2.3 `CRequest` 布局

序列化 `vfunc_07` / `vfunc_06`（`world/CRequest.cpp`）：`+0x08..+0x0A` 字节、`+0x0C` word（`fn_140004421` / `fn_14000a19b`）、`+0x0E..+0x11` 字节。槽步长 0x18。

清空 `CRequest::vfunc_05` → `fn_140001a73`（body `0x1401f05e0`）写入的空值 `[确认]`，各 kind 填充器写哪格 `[确认]`（`fn_1401eb2a0.cpp` `0x1401ef850`–`0x1401f0009`）：

| 偏移 | 宽 | 空值 | 用途 | 谁写 |
| --- | --- | --- | --- | --- |
| `+0x08` | 1 | `0x62`（98） | 发布港 `town_id` | `fn_140009ac0` 写当前港 `fn_14000e68d()` |
| `+0x09` | 1 | `0x0F` | kind | `fn_140009ac0(req, kind)` |
| `+0x0A` | 1 | `0x6A`（106） | 交易品 | kind 1/2 |
| `+0x0C` | 2 | 0 | 仓库数 | kind 1/2，夹到 `[1, 25]`（`0x19`） |
| `+0x0E` | 1 | `0xE8`（232） | 道具 `item_id` | kind 7/8/9 |
| `+0x0F` | 1 | `0x62` | 目标港 `town_id` | kind 0/2/3/4/5/6/7；kind 8 写道具所在港 |
| `+0x10` | 1 | `0x19`（25） | 村庄 `village_id` | kind 11 |
| `+0x11` | 1 | `0xFF` | 人物：kind 3–6 为 `RequestPerson` id；kind 10 为舰队 `+0x10` 字节 | `fn_14000e23c` / `fn_140004d9f` |
| `+0x12..+0x17` | | | 垫到 0x18 | `[推断]` |

`[推断]` 空值都是「计数 + 1」：98 = `NUM_TOWN+1`，106 = `NUM_TRADINGITEM+1`，232 = `NUM_ITEM+1`，25 = `NUM_VILLAGE+1`。kind 10 写入的舰队 `+0x10` 是否为提督 person id `[未知]`。

`[确认]` 委托**没有**金币字段。奖励与违约金都在结算时现算。

### 2.4 委托涉及人物 `RequestPerson`

`[确认]` 取址 `fn_14000ae0c`（body `0x1401f3440`）：`0 <= id < 0x64` 返回 `0x140686690 + id*0x15`，否则 NULL。exe 内静态表 100 条 × 21 字节。RTC `eRequestPerson >= 0x64`、`aeRequestPerson` 400 字节（`fn_14000e23c`）。

`[推断]` `NUM_REQUESTPERSON = 100`。

`[确认]` 这 100 人**不是** `ePerson`（235）的子集，是独立名字表（泰勒、强纳生…米盖尔）。记录布局：

| 偏移 | 内容 | 标记 |
| --- | --- | --- |
| `+0x00` | GBK 名字，NUL 结尾（最长 10 字节） | `[确认]` |
| `+0x11` | 0/1（10 条为 1：汉纳、帕特利希、苏珊娜、伊拉意札、朱巴意达、贺逊、卡舒瓦拉、永森、孙、赫莲娜） | `[推断]` 性别（多数像女性名，贺逊 / 永森 / 孙难判） |
| `+0x12` | 0–6 | `[确认]` 被比较；`[推断]` 七大海域 id |
| `+0x13` | 0/1（0–49 为 0，50–99 为 1） | `[确认]` 被比较；`[推断]` 0=良民 1=欠债/嫌犯 |
| `+0x14` | 87–203 | `[未知]` 疑似头像号 |

`[确认]` 抽人 `fn_14000e23c(req, area, flag)`：扫 100 人，收 `+0x12 == area` 且 `+0x13 == flag` 的 id，`rand(n)` 选一个写 `req+0x11`。`area` = `fn_140014e48(fn_1400103bb(town))`。

| kind | town 取自 | flag |
| --- | --- | --- |
| 3 找人 | 目标港 `+0x0F` | 0 |
| 4 护送 | 发布港 `+0x08` | 0 |
| 5 催收 | 发布港 `+0x08` | 1 |
| 6 搜捕 | 目标港 `+0x0F` | 1 |

### 2.5 生成：进公会时现场出题

`[确认]` `fn_14000c99b`（body `0x1401571e0`）：

1. 列本港公告（`fn_140006078`）。`fn_14000790f` 返回非 0 则重列。`[推断]` 先清掉失效委托。
2. 本港已有 ≥1 条 → 不生成。
3. 否则按固定顺序逐个尝试生成器。每次先看本港已有数 `>= 3` 就停；再 `fn_14000182f` 找空槽，0–68 满了就停；生成成功则计数 +1。

生成器顺序与门槛。`rand(N)` 为 `fn_140005272(N)`，`!= 0` 即放弃，所以触发率 1/N。`infl(a)` = 玩家势力在大海域 `a` 的影响力 word（`fn_1400023ab(fn_140012bfc(), a)`，读 `CForce+0x10+a*2`）；`infl_total` = 7 格之和（`fn_140008b6b`，RTC `eSeaArea < 7`）；`here` = 当前港所在大海域。

| 序 | kind | 生成器 body | 1/N | 额外条件 `[确认]` |
| --- | --- | --- | --- | --- |
| 1 | 12 / 13 | `0x14015daa0` | 10 | `infl_total >= 5000`；`infl_total >= 10000` 且本港商业发展度 `fn_14000bc03(town,0) >= 5000` → 12，否则 13 |
| 2 | 11 | `0x14015e0a0` | 15 | 村庄 0–23 中：所在大海域 == `here`、未发现、`infl(here) >= (b+2)²`（`b` = `fn_140003cba(village)` 字节，语义 `[未知]`） |
| 3 | 10 | `0x14015e280` | 8 | `infl(here) >= 1000`；舰队 0–68 中势力为空（`fn_14000d599 == 0x1A`）且所在港属 `here` 的；无则退到舰队 0–62 另一组条件 |
| 4 | 9 | `0x14015e6c0` | 7 | 先要求道具 `0xBF` 持有者类型 == 6，否则直接放弃；候选按 `here` 与 `infl(here)` 对道具价筛 |
| 5 | 8 | `0x14015d3f0` | 6 | 道具 0–230 中由别港商店持有（owner 类型 4）的 |
| 6 | 6 | `0x14015c870`(`edx=6`) | 6 | 港 0–96 中有效、`fn_14000c65d`、有酒馆热点 `fn_140002806(t,4)`、大海域 != `here` |
| 7 | 5 | `0x14015c870`(`edx=5`) | 6 | 同上 |
| 8 | 3 | `0x14015dea0` | 5 | 大海域 != `here`，有广场 `fn_140002806(t,0)` 或旅馆 `(t,8)` 热点 |
| 9 | 4 | `0x14015db60` | 4 | 大海域 != `here` |
| 10 | 7 | `0x14015ca70` | 3 | 本港可交付道具非空；`infl(here) < 1000` 时只挑满足 `fn_14000c71b==1` 的别港 |
| 11 | 2 | `0x14015ccf0` | 3 | 从候选港（`fn_140158030`）特产品里挑目的港不产的 |
| 12 | 0 | `0x14015d070` | 2 | 目标港 != 本港 |
| 13 | 1 | `0x14015d600` | — | 无随机门；本港不产、别港特产；`infl(here) < 10000` 时只看同大海域 |

`[确认]` 运货量（kind 1/2）：

```
L   = fn_1400123dc(player)            # 总等级
qty = (L + rand(L)) / 15 + 1          # 整除 0xF
qty = clamp(qty, 1, 25)               # 填充器 fn_1400087c9 / fn_140005bcd
```

`[确认]` `fn_140002806(t, k)` = `CTown+0x14` 热点位测试（`01-world/10-mappoint-kinds.md` 2.5）：k=0 广场、4 酒馆、8 旅馆。即「人在有酒馆 / 广场 / 旅馆的港」。重构写成 `TownLife.has_map_point(town_id, kind_key)`。

`[未知]` `fn_14000c65d`、`fn_14000c71b` 的港口语义（候选：势力关系 / 港口开放）。

### 2.6 接受、交付、放弃

接受 `[确认]`（`fn_14015b600`）：

- 本港公告最多 3 条，菜单 4 项（`fn_14000f3ad(..., 4)`，第 4 项取消），选择下标夹到 `[0, 2]`。
- 确认后复制到玩家槽、清原槽。
- kind 9：接受时即给藏宝地图（`fn_1401582e0(item, 1)`，消息 `0xb3c` ↔ mesfile「这就是那宝物埋藏之处的地图。」）。
- kind 10：接受时对目标舰队 `fn_14000f349(fleet, 8, town)`。`[推断]` 让海盗舰队驻留/出现在本海域某港。

放弃 `[确认]`（`fn_1400093f4`，body `0x1401567f0`）：

```
penalty = (fn_1400123dc(player) + 1) * 200           # imul 0xC8
if kind == 5 and fn_1400043c2(player):               # [推断] 已收到欠款
    penalty += (15 - rp[+0x13]) * 10000              # fn_14000556f，imul 0x2710
if gold < penalty: 消息 0xb2d                        # mesfile 0x1525b「钱不够呀。」
else: 扣金；kind 7 且条件满足时收回道具；清玩家槽
```

提示消息 `0xb2c` ↔ mesfile `0x15243`「要交出违约金%s枚金币。」。消息号 ↔ mesfile 偏移的对应 `[推断]`：按内容与相邻顺序（`0xb2c`/`0xb2d` ↔ `0x15243`/`0x1525b`，`0xb2e`… ↔ `0x15267` 起 14 条模板）。kind 5 的人物 `+0x13` 恒为 1，故追加额为 140000。

交付/报酬文案 `[确认]`：

- `0x15537`「确实收到了 `%s` 份 `%s`。」
- `0x1554b` 收金币；捉到人「没想到躲到了 `%s`」
- `0x155a3` 可用「有趣的地图」代替报酬
- `0x15633`「还有，这是剩下的报酬。」
- `0x1564b`「收下报酬吧。」
- `0x15223`「从 `%s` 来的吗？谢谢，请代为问好。」（文书类）
- `0x318b` 任务日志短句「购买%s。寻找%s。讨伐海盗%s。…得到介绍信。得到推荐信。」

港内交付检查读玩家槽：例如 kind 3 在目标港 `+0x0F == 当前港` 时触发对话（`fn_14015eb80`，消息 `0xb4a`）；kind 5/6 走 `fn_14014dff0`（消息 `0xb4b`–`0xb4f`）。

`[未知]` 完成报酬金额公式。查过：公会菜单 `fn_14000b366`、接受 `fn_14015b600`、放弃 `fn_1400093f4`、港内找人 `fn_14014dff0`、`fn_14015eb80`；未找到发奖金的 imul/常量。

UI：`CRequestInformationView`（`game/CRequestInformationView.cpp`），ctor 写 `+0xb4=0x47`（71）。`[未知]` 语义；与槽数无直接关系的证据不足。

### 2.7 计略摘要（详见 04 篇）

`[确认]` 本节结论均在 04 篇给出来源，这里只纠正旧版说法：

- 24 是运行时槽，不是 24 条具名计略；开局全空（kind 写 `0x0B`）。
- kind 合法 0–9：填充 `fn_1400112cf` 拒 `kind >= 0xA`；结算 `fn_14000cea5` 按 0–9 跳表分发；名字表 `0x140687630` 10 格（亲善、缔结协议、撕毁协议、停战、宣战布告、威压、引导舰队、伪造假文书、坏话、收买）。
- 0–5 是公会「交涉文书」（副官），6–9 是酒馆计谋（参谋）。旧版的 11 个「效果族」中 `cut_amity` 是坏话的结果句，`false_intel` 即引导舰队。
- `CScheme+0x10` 是**到期日历**：`now + delay * 0x30`。`CSchemeElement::vfunc_04` 用 `ja` 比较 `+0x10` 与 `arg+0x18`，是「到期才结算」，**不是**金币筛。发动金 1000 在入口扣。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空/非法 | 备注 |
| --- | --- | --- | --- | --- |
| 委托槽 | `eRequest` | 69 公告 + 1 玩家 | -1；空 kind `0x0F` | 运行时生成 |
| 委托种类 | `eREQUESTKIND` | `[推断]` 14 | -1；运行时空 15 | 名字表 `0x1405873f0` |
| 委托人物 | `eREQUESTPERSON` | `[推断]` 100 | -1；槽内空 `0xFF` | exe 表 `0x140686690` |
| 计略槽 | `eScheme` | 24 | -1；空 kind `0x0B` | 见 04 篇 |
| 计略种类 | `eSCHEMEKIND` | `[推断]` 10 | -1；运行时空 11 | 见 04 篇 |

重构 ID（D1）：kind 主键是命名空间 key；原版 0–13 只作 `legacy_kind` 供导入层对照，运行时整数加载时分配。空 kind / 空槽一律 `null`；`0x0F`、`0x62`、`0x6A`、`0xE8`、`0x19`、`0xFF` 只出现在 `ids/legacy_codec.yaml`。槽号仅用于存档对照。

空委托：`kind == empty`（原版 `0x0F`），不要用 `id=0` 当空。

## 4. 算法 / 公式

全部参数进 `config/formulas/town.yaml` 与 kind 表。禁止 `if kind==3` 散落。

```
generate_at(town):
  if listed(town) >= 1: return                     # 原版：已有就不生成
  for rule in generators (按 order):
    if listed(town) >= per_town_max: break         # 3
    slot = free_board_slot() or break
    if rand(rule.one_in) != 0: continue            # 1/N；N 缺省视为必过
    if not rule.preconditions(town, player): continue
    fill(slot, rule.kind, rule.pick_params(...))

cargo_qty = clamp((L + rand(L)) / cargo_level_div + 1, cargo_min, cargo_max)
            # L = Progression.total_level(player)；div=15, min=1, max=25

can_accept(request_id):
  player_slot empty
  AND request.kind != empty

abandon_penalty = (L + 1) * penalty_per_level      # 200
                + (kind.adds_debt_on_abandon AND debt_collected ? debt_amount(person) : 0)
debt_amount(p) = (debt_base - p.flag) * debt_unit  # 15, 10000

on_complete(request):
  gold = request_reward(kind, ...)                 # 跳转表 fn_140010e10；kind 12/13 已还原（02-factions/04 §2.6），其余 [未知]
  gold = Modifiers.eval("request.reward_gold", { request_kind: kind, base: gold })   # 原版剧本 0 ×3/2（无符号截断），全种类；剧本包注入
  Force.add_gold(player_force, gold, reason: request)
  emit RewardItem { item_id }                      # 地图替代报酬可选
  # 按 kind 再发 FindVillage / DefeatFleet / ...

on MonthStarted:                                   # schedule: month_start；原版 SCalendar 快照 +0x1c == 0
  clear all board slots (保留 player slot)
```

kind 前置（查询，不在本系统算）：

| kind key | 查询 |
| --- | --- |
| `buy_cargo_*` | `Economy.specialProducts(town)`、`Economy.canBuy` |
| `find_person_*` / `escort` / `collect` / `capture` | `RequestPersons.pick(area, flag)` |
| `hunt_pirate` | `Fleets.listUnaffiliatedIn(area)` |
| `find_village` | `World.villageDiscovered`、`World.villageArea` |
| `get_*_letter` | `Factions.influenceTotal`、`Towns.develop(town_id, commerce)` |
| 全部 | `Factions.influence(player_force, major_sea_id)`（06 规范名，词表 §14.26；`major_sea_id = MajorSea.of_town(town_id)`） |

计略公式见 04 篇。

## 5. 耦合分析

原版：

- `CRequest` / `CScheme` 是 `CSaveObject`，和城镇、人物同一存档链
- 生成器直接扫港口 0–96、道具 0–230、村庄 0–23、舰队 0–68，读势力影响力、港口发展度、道具持有者
- 完成委托直接改金币、给道具、改发现状态；接受 kind 10 直接改舰队行为
- 委托人物是 exe 内 100 条静态表，按大海域 + 良/恶旗抽
- 循环上界写死 69/70/100/97/231/24；触发率 1/2…1/15、阈值 1000/5000/10000、违约 200、债额 15×10000、运量 /15 与 25 全是立即数

重构后只发：

```
Requests.complete → Force.add_gold / Items.grant / Letters.deliverForRequest（kind 12/13） / World.mark_village_discovered
Requests.accept   → Fleets.stationPirate (kind hunt_pirate) / Items.grant (find_treasure 地图)
Schemes.cast      → 见 04 篇
```

本系统不读六维，不读市价曲线；影响力、发展度、总等级只经查询接口取标量。

## 6. 解耦后的配置

`config/town/request_kinds.yaml`：

```yaml
schema: dk4.town.request_kinds
version: 1
kinds:
  - key: vanilla:deliver_guild_letter
    legacy_kind: 0
    name: 传递文书
    accept_i18n: ui.request.deliver_guild_letter
    needs: [dest_town_id]
  - key: vanilla:buy_cargo_here
    legacy_kind: 1
    name: 购买交易品
    needs: [trading_item_id, quantity]
  - key: vanilla:buy_cargo_to_town
    legacy_kind: 2
    name: 运送交易品
    needs: [trading_item_id, quantity, dest_town_id]
  - key: vanilla:find_person_bring
    legacy_kind: 3
    name: 找人
    needs: [request_person_id, dest_town_id]
    person_pick: { area_from: dest, flag: 0 }
  - key: vanilla:escort_person
    legacy_kind: 4
    name: 护送
    needs: [request_person_id, dest_town_id]
    person_pick: { area_from: origin, flag: 0 }
  - key: vanilla:collect_debt
    legacy_kind: 5
    name: 催收欠款
    needs: [request_person_id, dest_town_id]
    person_pick: { area_from: origin, flag: 1 }
    adds_debt_on_abandon: true
  - key: vanilla:capture_criminal
    legacy_kind: 6
    name: 搜捕犯人
    needs: [request_person_id, dest_town_id]
    person_pick: { area_from: dest, flag: 1 }
  - key: vanilla:deliver_item_guild
    legacy_kind: 7
    name: 运送宝物
    needs: [item_id, dest_town_id]
  - key: vanilla:buy_item_in_town
    legacy_kind: 8
    name: 购买宝物
    needs: [item_id, dest_town_id]
  - key: vanilla:find_treasure
    legacy_kind: 9
    name: 寻找宝物
    needs: [item_id]
    on_accept: [grant_treasure_map]
  - key: vanilla:hunt_pirate
    legacy_kind: 10
    name: 讨伐海盗
    needs: [fleet_ref]
    on_accept: [station_pirate]
  - key: vanilla:find_village
    legacy_kind: 11
    name: 探索村子
    needs: [village_id]
  - key: vanilla:get_intro_letter
    legacy_kind: 12
    name: 取得介绍信
    needs: []
  - key: vanilla:get_recommend_letter
    legacy_kind: 13
    name: 取得推荐信
    needs: []
```

`config/town/request_generation.yaml`（原版包的规则，不是实例）：

```yaml
schema: dk4.town.request_generation
version: 1
board_slots: 69              # 原版 NUM_REQUEST
player_slots: 1              # 原版槽 0x45
per_town_max: 3
generate_only_if_board_empty: true
clear_board_on: month_start  # Clock 订阅；原版调度表第 8 项
generators:                  # 按顺序尝试
  - { kind: [vanilla:get_intro_letter, vanilla:get_recommend_letter], one_in: 10,
      require: { influence_total_min: 5000 },
      choose_first_if: { influence_total_min: 10000, town_commerce_min: 5000 } }
  - { kind: vanilla:find_village,       one_in: 15, village_area: here,
      require: { influence_here_min_formula: "(village.b + 2) ^ 2" } }
  - { kind: vanilla:hunt_pirate,        one_in: 8,  require: { influence_here_min: 1000 } }
  - { kind: vanilla:find_treasure,      one_in: 7,  gate_item: 191 }       # 0xBF；语义 [未知]
  - { kind: vanilla:buy_item_in_town,   one_in: 6 }
  - { kind: vanilla:capture_criminal,   one_in: 6,  dest_area: other, dest_has_map_point: [vanilla:bar] }
  - { kind: vanilla:collect_debt,       one_in: 6,  dest_area: other, dest_has_map_point: [vanilla:bar] }
  - { kind: vanilla:find_person_bring,  one_in: 5,  dest_area: other, dest_has_map_point_any: [vanilla:plaza, vanilla:inn] }
  - { kind: vanilla:escort_person,      one_in: 4,  dest_area: other }
  - { kind: vanilla:deliver_item_guild, one_in: 3 }
  - { kind: vanilla:buy_cargo_to_town,  one_in: 3 }
  - { kind: vanilla:deliver_guild_letter, one_in: 2 }
  - { kind: vanilla:buy_cargo_here,     one_in: null,
      same_area_only_if: { influence_here_lt: 10000 } }
```

`config/formulas/town.yaml` 追加：

```yaml
request_cargo_qty:             # 委托运量
  impl: expr
  params:
    request_cargo_level_div: 15
    request_cargo_min: 1
    request_cargo_max: 25
  inputs: [L]                  # L = Progression.total_level(player)
  expr: "clamp((L + rand(L)) / request_cargo_level_div + 1, request_cargo_min, request_cargo_max)"

request_abandon_penalty:       # 违约金
  impl: expr
  params:
    request_penalty_per_level: 200
    request_debt_base: 15
    request_debt_unit: 10000
  inputs: [L, adds_debt_on_abandon, debt_collected, person_flag]   # person_flag = 委托人物 flag
  expr: "(L + 1) * request_penalty_per_level + (adds_debt_on_abandon && debt_collected ? (request_debt_base - person_flag) * request_debt_unit : 0)"

request_reward:                # [未知]；kind 12/13 金额在 factions letters.yaml；剧本倍率走修正器 request.reward_gold，不进本表
  impl: null                   # 跳转表 fn_140010e10
  unverified: true
  modifiers: request.reward_gold
```

`config/town/request_persons.yaml`（原版包 100 行，由导出脚本按 `.data` 段换算从 exe `0x140686690` 抽，文件偏移 = VA − `0x140001600`）：

```yaml
schema: dk4.town.request_persons
version: 1
persons:
  - key: vanilla:rp_taylor
    vanilla_index: 0
    name: 泰勒
    major_sea: vanilla:north_sea       # +0x12，七大海域（原版 0）
    flag: 0                    # +0x13；0 良民 1 欠债/嫌犯 [推断]
    female: false              # +0x11 [推断]
    portrait: 203              # +0x14 [未知]
```

计略配置见 04 篇 `scheme_kinds.yaml`。本篇不再维护 `schemes.yaml` / `requests.yaml` 实例表：原版两者都是运行时槽，进存档，不进内容包。

`config/ids/` 由 kind 表与人物表生成注册项。模组可加 kind / 人物，引擎按表长循环；模组 kind 用命名空间 key（`mymod:smuggle`），整数只给原版包。

## 7. 独立旋钮

- 增删 kind（新 key + 参数形状 + 前置查询名 + 生成规则）
- 生成顺序、1/N、每港上限、公告槽数、玩家可持委托数
- 运量公式三参数、违约金、债额
- 委托人物表（加人、改大海域/旗）
- 公告作废时机

不该出现在本表：友好度公式、市价、人物六维阈值、影响力计算。

## 8. 对外接口

事件：

```
RequestsGenerated { town_id, request_ids[] }
RequestAccepted { request_id, town_id, fleet_id }
RequestCompleted { request_id, kind, rewards[] }
RequestAbandoned { request_id, penalty_gold }
RequestBoardCleared { }
```

查询：

```
Requests.listAt(town_id) -> [request_id]
Requests.active(player_id) -> request_id | null
Requests.get(request_id) -> { kind, origin_town, dest_town, item, trading_item, quantity, village, person }
Requests.abandonQuote(player_id) -> gold
```

命令：

```
GenerateRequests { town_id }          # 进公会时
AcceptRequest { request_id }
DeliverRequest { }
AbandonRequest { }
```

完成时本系统只 emit 奖励事件；economy / factions / items / ocean 订阅。

存档：公告槽与玩家槽的可变字段（kind、港、货、量、道具、村、人物）。kind 定义、生成规则、人物表不进存档。

## 9. 未核实字段

- `NUM_REQUESTKIND` / `NUM_REQUESTPERSON` / `NUM_SCHEMEKIND` 整数（现为推断 14 / 100 / 10）。
- `CRequest+0x11` 在 kind 10 下的语义（舰队 `+0x10` 字节）。
- `RequestPerson+0x11`（性别？）、`+0x14`（头像？）。
- 完成报酬金额公式；「有趣的地图」替代报酬的条件。
- `fn_14000790f` 清理条件；作废时机 `0x1442a7b10+0x1c == 0` 是换日还是换月。
- 生成器里 `fn_14000c65d`、`fn_14000c71b`、`fn_140003cba(village)`、道具 `0xBF` 持有者类型 6 的语义。
- `fn_1400043c2(player)`（放弃催收时追加债额的条件）。
- `CRequestInformationView+0xb4=0x47`。

## 附：mesfile 块偏移 → exe 消息号

按 D10，文案 key 用 exe 消息号，不用 mesfile 字节偏移。正文里 `0x` 开头、落在 mesfile 的数多为 `catalog/mesfile_sc.txt` 的 NUL 块偏移；一块常含多条消息。下表由脚本按 exe 消息号表（`.data 0x140683620` 37 块 `{first_id,size}` + `0x1406836c0` 块内偏移；`.data` 文件偏移 = VA − `0x140001600`）切句，再按正文引号句匹配生成。只给一条的是已匹配到句；给区间的需导出时按句再选。正文里已标「消息」的号（如 `0x58a`、`0xb2e`）本来就是消息号，不在此表。

| 块偏移 | 消息号 | 首句 |
| --- | --- | --- |
| `0x318b` | `0x125` | 讨伐海盗%s。 |
| `0xc237` | `0x5c9` | 如果参谋在，可在酒馆里拟定策略。 |
| `0x15223` | `0xb2b` | 从%s来的吗？谢谢，请代为问好。 |
| `0x15243` | `0xb2c` | 要交出违约金%s枚金币。 |
| `0x1525b` | `0xb2d`（块内多条，按句选） | 钱不够呀。 |
| `0x15267` | `0xb2e`（块内多条，按句选） | 希望把此文书交给%s的同业公会。愿意接受吗？ |
| `0x15537` | `0xb3e` | 确实收到了%s份%s。 |
| `0x1554b` | `0xb41` | 这家伙是%s吗？没想到躲到了%s，终于捉到了。 |
| `0x155a3` | `0xb44` | 我得到了一张有趣的地图，可以让给你，用来代替报酬 |
| `0x15633` | `0xb45` | 还有，这是剩下的报酬。 |
| `0x1564b` | `0xb46` | 收下报酬吧。 |
