# 势力战争、失败与吞并

势力之间怎么开战、怎么私掠、失败记录怎么写、一条势力何时从地图上消失。友好度加减与份额仍见 [03-relations-influence.md](03-relations-influence.md)；海战伤害见 `07-combat`；「讨伐海盗」委托见 `08-town-life`。本篇管**关系枚举、月结开战 AI、失败槽、吞并/灭亡**。

## 1. 职责边界

**管：**

- 势力↔势力运行时关系 `eForceRelation`（存在 `CForce+0x1F` 的 25 字节里）
- 势力当前目标 `eForceAimKind`（运行时写在 `CForce+0x52`）
- 失败记录 `CForceFailure`（`CForceElement+0x8A0` 起 7 槽）
- 吞并 / 灭亡：母港写成空哨兵、人物脱籍、失败槽清掉
- 月结里 NPC 对谁开战、何时破产消失（`CForceElement::vfunc_04` → `fn_140005489` / `fn_14000c2f7` / `fn_14000a8c1`）
- 月结消费 5 个 `CBattleResult`（大地图互打的战果，不是玩家 HUD）
- 私家舰队 id=24 在月结里的特殊跳过

**不管：**

- 友好度、影响力、份额、上缴公式 → `03`
- 炮弹、接舷、城市炮台 → `07-combat`（本层只收 `BattleResolved` / `TownCaptured`）
- 具名海盗遭遇、讨伐委托 → `ocean` / `town-life`（文案「海盗%s」是人物名，不是关系枚举）
- 玩家海战结算画面 → combat 的 `CBattleResult` 字段名仍未恢复，本层只消费势力侧 5 槽
- 计略「假情报 / 亲笔信」成功率 → `08-town-life`（本层收结果：宣战、停战、合并）

系统之间只传 `force_id`、`town_id`、`fleet_id`、关系/失败/目标的 **ID 与标量**。不传 `CForce*`、`CForceFailure*`。

## 2. 原版事实

### 2.1 枚举只有符号

| 符号 | 头文件 | 整数 | 等级 |
| --- | --- | --- | --- |
| `NUM_FORCERELATION` / `INVALID_FORCERELATION=-1` | `ForcerelationID.h` | 未还原 | `[确认]` 符号 |
| `NUM_FORCEFAILUREKIND` / `INVALID_FORCEFAILUREKIND=-1` | `ForcefailurekindID.h` | 未还原 | 同上 |
| `NUM_FORCEAIMKIND` / `INVALID_FORCEAIMKIND=-1` | `ForceaimkindID.h` | 未还原 | 同上 |
| `NUM_ABSORBED` / `INVALID_ABSORBED=-1` | `AbsorbedID.h` | 未还原 | 同上 |
| `FORCERELATION_ID` 等 | `constants.json` `MAXMIN` | 宽度是 `BYTE2DWORD` | `[确认]` 字节级 ID |

断言形态一律 `(0 <= eData && eData < NUM_*) \|\| eData == INVALID_*`。来源：`srcdata/catalog/asserts.txt`。工程里**没有** `CAbsorbed` 类。

### 2.2 关系值：开局 0/1，运行时可以是 2

- `[确认]` `fn_140012dc8` 读 `CForce+0x1F+other`（存盘同样写这 25 字节）。来源：`fn_140107150.cpp`。
- `[确认]` `fn_14000b7da`：读出的值 `< 0` 或 `>= 3` 视为非法。运行时只用 **0、1、2**。`[推断]` `NUM_FORCERELATION = 3`。禁止把头文件整数写成已恢复。
- `[确认]` `== 2` 被当成交战：`fn_14001087f` 对 2 返回真；`fn_140005489` 把 `== 2` 的对方填进 `aeWarForce`。来源：同文件、`fn_14010be70.cpp`。
- `[确认]` World 表初值这一列几乎只有 0/1，三对对称 1：阿博科鲁克↔西鲁韦拉、巴鲁迪斯↔玛尔德纳德、巴夏↔海雷丁。见 01。**那是开局快照，不是布尔旗的证明**——月结会把字节写成 2。
- `[推断]` 0 = 无特殊协议（中立/平常），1 = 协议/同盟，2 = 交战。停战可能是 2 上的时限，而不是第 4 个枚举。名字不要升格为确认。

文案（`mesfile_sc.txt`，外交结果句 `0x38c7`–`0x3b77`，宫殿/交涉 `0xb0af`–`0xb207`，确认问句 `0xdc23`–`0xde33`）：

| 动词 | 例句 | 偏移 |
| --- | --- | --- |
| 宣战 | `%s向%s宣战` `%s向我们宣战` `我要向%s势力宣战` | `0x39af` `0xdc23` |
| 停战 | `%s和%s停战了` `%s请求停战` | `0x398f` `0x3967` |
| 协议 | `缔结了对抗%s的协议` `%s和%s缔结了协议` | `0x38c7` `0x3b77` |
| 开战通报 | `%s和%s开战了` | `0x3a8f` |
| 合并 | `%s加入麾下了` `%s拒绝了合并的请求` `%s无视合并的请求` | `0x39df` `0x39ef` `0x3b57` |
| 不能打 | `友好势力的舰队，无法攻击` `同盟势力占有的城市，无法攻击` | `0x33ef` `0x33cf` |

敌我判定：**文案同时用「友好」和「同盟」**。阈值与「友好 = 关系 1 还是友好度」仍 `[未知]`，禁止写 `amity > 50`。

### 2.3 目标种类 `eForceAimKind`

- `[确认]` `CForce` 初始化 `+0x52 = 4`。01 已禁止把这一字节升格成 `eForceColor`。
- `[确认]` 月结 `fn_140005489` 用 `fn_140002f31` 读 `+0x52`，和 **0 / 1 / 2** 比较：`== 1` 走舰队命令/收集 `aeWarForce`；`== 0` 走无港扩张；其它值跳过这两段，后面仍可能打仗、收买。来源：`fn_140107150.cpp`。
- `[确认]` `fn_14000c8b0`：`+0x52 == 2` 时读 `+0x53`（空 = `0x1A`）当「当前对手势力」，对手已灭则改命令。来源：`fn_14010be70.cpp`。
- `[推断]` `+0x52` 即 aim；观测到 0、1、2、4。3 没看见。`NUM_FORCEAIMKIND` 仍未知，禁止写成 5。
- `[推断]` `+0x53` 是交战/瞄准的对方 `force_id`（空哨兵 26）。与 `+0x51` 并列，不是同一字段。

舰队另有 `eFleetAimKind`（`FleetaimkindID.h`），不要和势力 aim 合成一张表。

### 2.4 `CForceFailure`：7 槽 × 4 字节

容器在 `CForceElement`：

| 偏移 | 个数 | stride | 对象 | 来源 |
| --- | --- | --- | --- | --- |
| `+8` | 25 | `0x58` | `CForce` | ctor `fn_140007dce(..., 0x58, 0x19)` |
| `+0x8A0` | **7** | `0x10` | `CForceFailure` | ctor `..., 0x10, 7`；析构/存盘同样循环 7 |
| `+0x910` | **5** | `0x10` | `CBattleResult` | ctor `..., 0x10, 5`；RTC `eBattleResult`；`NUM_BATTLERESULT=5` |

`CForceFailure : CSaveObject`。`vfunc_06/07` 只读写 **`+8 +9 +0xA +0xB` 四个字节**。`vfunc_04`：`+8 == 0x1A` 视为空。`vfunc_05` = `fn_140003dd7`：把 `+8` 写成 `0x1A`。来源：`CForceFailure.cpp`、`fn_140107150.cpp`。

三个写入函数（`fn_1400fe120.cpp`），调用点在战后 `fn_1401200a0.cpp`（先 `fn_14000d508` 找空的 `CBattleResult` 槽，再写失败记录）：

| 函数 | 前置 | `+8` | `+9` | `+0xA` | `+0xB` |
| --- | --- | --- | --- | --- | --- |
| `fn_14000f808` | 两势力都不是 `0x1A` | 势力 A | 势力 B | **`0x62`（无港）** | 旗（0/1，可按参数取反） |
| `fn_140005ce0` | A≠空、港≠`0x62`、B≠空 | 势力 A | 势力 B | 港口 | 旗 |
| `fn_1400087ba` | 势力≠空、港≠`0x62` | 势力 | **`0x1A`** | 港口 | 旗 |

`[推断]` 这是三种失败**形状**：舰队对舰队（无港）、带港的两势力、单势力丢港。`+0xB` 经 `fn_14010d850` 写入，参数为 0/1，**不像**多值 `eForceFailureKind`。种类枚举整数仍然 `[未知]`。

7 这个个数同时等于 `NUM_SCENARIO` 和势力 7 格影响力。01 按剧本数对齐。本篇不跟：灭亡路径用 RTC 名 `eSeaArea`、上界 **7** 扫这些槽（`fn_14000a8c1`），与 7 格大战区同一循环形态。三种假说并列，见 §9。

### 2.5 一条势力何时消失

判定（`fn_14000b096`，RTC `eNextBaseTown` / `eNextBaseCity`）：

1. `fn_14000d63e` 得到计数 `n`。`[推断]` 仍占有的港/份额槽数。
2. `n <= 0` → 要灭。
3. `n == 1` 且金币 `< 0x3E8`（**1000**）→ 要灭。
4. `n > 1` → 不灭，把母港改成「独占港优先，否则份额最高的港」。

真正抹掉（`fn_14000a8c1`，RTC `ePerson` / `eForce` / `eTown` / `eSeaArea`）：

1. 母港已是 `0x62`：跳过演出，仍可能走玩家分支。
2. 否则 `fn_1400012da(0x16b)`（消息下标，**不是** mesfile 文件偏移）。
3. 若 `fn_140005b19` 判定「这就是玩家势力」：另走日历/游戏对象 `fn_140010578`，**不**跑下面的 NPC 清场。
4. NPC 清场：
   - 97 港：`fn_140005501`（清该势力合约 `[推断]`）
   - 其它势力：清关系；若对方 `+0x51 ==` 灭亡 id 则改写；若对方母港指向灭亡者则 `fn_14000405c(..., 0x1A)`
   - 7 个失败槽：`fn_14000246e`（读 `+8`）匹配则 `fn_140003dd7` 清空
   - 235 人：隶属 == 灭亡 id 则写成 `0x1A`
   - **最后** `CForce+0x0A = 0x62`

另有调用点 `fn_1400ed7c0.cpp`（航海/战斗侧）。合并成功应先写 `+0x51` 再进这条抹除；`fn_140006861` 在非玩家、且某条件成立时把 `+0x51` 写成一个 `force_id`，`fn_1400086d4` 把它清回 `0x1A`。方向（胜利者记被吞者，还是被吞者记宗主）`[未知]`。

没有 `CAbsorbed` 对象。`eABSORBED` 只有断言。`[推断]` 被吞 = 已出场过、现在母港空、且 `+0x51` 仍指向某人；从未出场（开局就是 98）不要算被吞。

### 2.6 mesfile：消灭 / 投降 / 合并 / 灭亡

| 词 | 出现处 | 能否当失败种类名 |
| --- | --- | --- |
| 消灭 | `0xa5b`「消灭了敌人的舰队」；`0xfd1b` / `0xfd4b`「消灭了%s」；白鲸/老鼠旁白 | **否**。舰队或战斗句，没有「某某商会消灭了」 |
| 投降 | 舰员「我们投降」；`%s城市投降了`（`0x13a93`）；合并前「我看%s是绝对不会投降的」（`0xb1df`） | 城/舰/劝降，不是已还原的 `eForceFailureKind` |
| 合并 | 「加入麾下 / 拒绝 / 无视合并的请求」 | **外交动词**，对应吞并事件，种类整数未知 |
| 灭亡 | `0x682f` 莫朝越南「受打击灭亡了」；印加「刚刚灭亡的时代」人物传记 | **历史旁白**。没有玩法句「%s灭亡了」 |

实现禁止用这四个中文当枚举字面量。UI key 另表，例如 `force.merged`、`force.collapsed`。

### 2.7 私家舰队 id=24

- `[确认]` `eForce_024`，中文名「私家舰队」，`NUM_FORCE=25`。来源：`ForceID.h`、`Docs/_data/forces_world0.json`。
- `[确认]` World0 开局母港 98、金币 0、领袖 person 19 格尔哈特·阿迪肯、kind 观测 2。见 01。
- `[确认]` `CForceElement::vfunc_04` 第一段：`eForce == 0x18` **无条件 continue**。另外 `eForce == s20` 也跳过，`s20 = fn_140004381()`（从 `DAT_144296500` 取出的「当前玩家势力」）。来源：`CForceElement.cpp`、`fn_140107150.cpp`。
- `[确认]` `fn_140005b19`：本势力 id == `fn_140004381()` → 视为玩家势力（灭亡时不走 NPC 清场）。

24 和玩家势力是**两次独立跳过**。表上的领袖不是玩家；玩家隶属在存档另一处（`save_format.md` `+0x35AA`）。独立后是否占用 24 号槽：`[未知]`。即使占用，月结仍会因「玩家势力」跳过 AI。

### 2.8 月结 `CForceElement::vfunc_04`（`0x14010dc20`）

RTC：`eForce`、`eBattleResult`、`eFleet`、`aeShip`、`aeShipType`。

1. **日历门** `fn_14000e9da`：某对象 `+0x1C == 0` 才跑外交/战果/上缴。否则跳到舰队段。语义（月初？）`[未知]`。
2. **势力 0..24**：空（`CForce::vfunc_04`，母港 `0x62`）、玩家势力、**24** 都跳过。其余：`fn_140005489`（本势力 AI）再 `fn_14000c2f7`（交战善后）。
3. **战果 0..4**：对象在 `+0x910`。空槽（`+8 == 0x1A`）跳过；匹配后 `fn_140011518` = 把 `+8` 写成 `0x1A`（**消费即清空**）。匹配键反编译把 `s44` 和 `this` 搅在一起，字段 `[未知]`。
4. **上缴**：`fn_140004a1b(...) == 6` 才进；港不是 `0x1A`；`div` 出金额；消息下标 `0x167`。公式仍归 03。
5. **舰队 0..0x44（69）**：另一道门 `fn_14001374b`。跳过空、势力空、玩家势力、某状态 `== 6`、母港与目的港都是 `0x62`。然后 NPC 买船（金币门槛、船价）。属委任/经济，本篇只承认「灭亡势力的舰队不再买船」。

`fn_140005489`（约 6.9 KB）局部名：`eThisForce`、`aeWarForce`、`aeThirdForce`、`cEnemyForce`、`asBribe`、`aeExpandTown`、`cForceFleet`、`abForce`。立即数：

| 立即数 | 用法 | 命名 |
| --- | --- | --- |
| `0x2AF8` = 11000 | 金币少于此不做扩张/开战/收买 | `npc_action_gold_min` |
| `0x3E8` = 1000 | 单港且穷则灭亡；另有扣款 | `force_collapse_gold` |
| `0x64` = 100 | `fn_1400089f9` 改友好度时封顶 | 与 03 的 amity 上限对齐，本路径 `[确认]` 封 100 |
| `0x14` / `0x28` / `0x5A` / `0x2D` | 友好度分支 20 / 40 / 90 / 45 | 不要合成「开战阈值」 |
| `fn_140005272(0xA)` | 交战善后约 1/10 跳过 | `war_followup_skip_span: 10` |
| `0x17F` | `fn_14000c2f7` 发的消息下标 | 未对照 mes 句 |

硬编码的 `eThisForce` 分支（克利福德/李家、库恩/普雷依拉、阿博科鲁克/卡斯特罗、朝鲜水师/佐伯/昆廷）是**原版包剧本癖**，进内容表 `force_ai_overrides`，禁止写进引擎 `switch(force_id)`。

`fn_1400015dc`：势力 id ∈ `[0, 3]`（卡斯特罗…李家）为真。月结扩张块对这四家走不同路径。不要写成「主角家不会开战」。

### 2.9 大地图战果 vs 玩家战斗

- `[确认]` `NUM_BATTLERESULT = 5`。`CBattleResult` 存盘 **5** 字节 `+8..+0xC`（比失败记录多 1 字节）。空哨兵同样是 `+8 == 0x1A`。来源：`BattleResultID.h`、`CBattleResult.cpp`。
- `[确认]` `fn_14000d508` 在 `+0x910` 找第一个空槽，供战后写入。
- 月结对匹配槽只做 clear。胜负、金币、占港在**写入当时**已经落地（`07-combat/03-battle-ai.md` 的 offscreen 近战）。本系统订阅 `BattleResolved`，不要再算一遍伤害。
- 五个取值的名字未恢复。文案侧的胜利/逃走/沉没/擒获/占领是战斗句，未必 1:1 对应这 5 槽。

### 2.10 「私掠」没有单独枚举

搜不到 `ePirate` / 海盗关系值。海盗在原版是：

- 委托「讨伐海盗 `%s`」（人物）
- 遭遇「在%s发现了海盗%s」（ocean/combat）
- 历史/人物传记

势力层只有「能不能打」：同盟港不能打、友好舰队不能打、`relation == 2` 是正式交战。中立（0）是否不宣而战 = 私掠：`[未知]`。接口先留 `may_attack` / `is_war` / `is_piracy_allowed`，后一项默认 `null`。

## 3. 实体与 ID

```
Relation[force_a][force_b] : u8          # 原版 +0x1F；合法 0..2；空不是 -1
Aim[force_id]              : u8          # 原版 +0x52；观测 0,1,2,4
AimTarget[force_id]        : force_id    # 原版 +0x53；空 = 26
AbsorbedLink[force_id]     : force_id    # 原版 +0x51；空 = 26；方向未知
FailureSlot[0..6] : {
  force_a : force_id | empty,            # +8
  force_b : force_id | empty,            # +9
  town_id : town_id | empty_town,        # +0xA；空港 = 98
  flag    : 0 | 1                        # +0xB
}
BattleResultSlot[0..4] : 5 字节，本系统只认 occupied / owner，其余 unknown
```

哨兵（与 01 相同，禁止用 0）：

| 用途 | 原版 | 重构 key |
| --- | --- | --- |
| 非法枚举 | -1 | `invalid` |
| 空势力（关系、失败槽、+0x51/+0x53） | 26 (`0x1A`) | `force.runtime_empty` |
| 空母港 / 已灭亡 / 失败记录无港 | 98 (`0x62`) | `town.empty_sentinel` |
| 私家舰队 | 24 (`0x18`) | `private_fleet` |

主键全是 ID。7 个失败槽的下标用 `theater_id` 还是不透明 `slot`：配置里先叫 `failure_slot_id`，等 §9 假说钉死再改名。

派生：

```
active(force)     := home_town_id != town.empty_sentinel     # 与 01 相同
collapsed(force)  := not active                              # 含从未出场
# absorbed(force) 公式未知；禁止用 collapsed 冒充
at_war(a, b)      := Relation[a][b] == 2
```

## 4. 算法 / 公式

参数进 `config/formulas/factions.yaml` 与 `config/caps.yaml`。原版立即数当 default。**禁止**在代码里写 `11000`、`1000`、`0x18`。

```yaml
# config/caps.yaml 片段
force_count_vanilla: 25
private_fleet_id: 24
force_runtime_empty_id: 26
town_empty_sentinel: 98
force_failure_slots: 7
battle_result_slots: 5
force_relation_observed_max: 2   # 运行时合法 0..2；NUM 未还原
force_amity_max: 100             # fn_1400089f9 封顶 [确认此路径]
```

```yaml
# config/formulas/factions.yaml  战争段
schema: dk4.formulas.factions
version: 1

npc_action_gold_min: 11000       # 0x2AF8 [确认]
force_collapse_gold: 1000        # 0x3E8 [确认]
war_followup_skip_span: 10       # rand(10)==0 则跳过 [推断 函数即 rand]
amity_add_cap: 100

# 未还原，占位必须留，值 null
declare_war_amity_threshold: null
ceasefire_amity_threshold: null
merge_success: null              # 「加入麾下」成功率
piracy_without_war: null         # 关系 0 能否打
tribute_from_exclusive_town: null
```

灭亡：

```
holdings = count_holdings(force)          # 原版 fn_14000d63e；定义未钉
should_collapse(force) :=
    holdings <= 0
    OR (holdings == 1 AND gold < force_collapse_gold)

on_collapse(force):
    if is_player_force(force): player_collapse_path()   # 不跑 NPC 清场
    else:
        clear_contracts(force)
        for other in forces:
            Relation.clear_involving(force, other)
            if AbsorbedLink[other] == force: AbsorbedLink[other] = empty
            if Home[other] points_at force: Home[other] = empty
        FailureSlot.clear_where(force_a == force or force_b == force)
        Persons.set_force(person, empty) where force_id == force
        Home[force] = town.empty_sentinel
```

月结 NPC（伪代码，原版包）：

```
if not month_gate: skip_diplomacy
player = Force.player()                  # fn_140004381
for f in Force.list():
    if not active(f): continue
    if f == player or f == private_fleet_id: continue
    if gold(f) < npc_action_gold_min: skip_expensive_actions
    NpcForceAi.tick(f)                   # fn_140005489
    WarFollowup.tick(f)                  # fn_14000c2f7；可能发消息 0x17F、扣友好
for slot in BattleResultSlot:
    if occupied(slot) and matches(slot): consume(slot)

if should_collapse(f): Force.collapse(f)
else: Force.retarget_home(f)             # 独占港 > 最高份额港
```

`may_attack(a, b)` 在关系名钉死之前：

```
if a == b: false
if not active(a) or not active(b): false
if Relation[a][b] == war: true
if Relation[a][b] == allied: false       # 「同盟势力占有的城市，无法攻击」
# 「友好势力的舰队，无法攻击」：阈值 null，禁止填 50
if piracy_without_war is null: false     # 未还原前宁可不能打
```

友好度加减仍走 03 的 `Amity.add`；本层只改 `Relation` 和失败槽。`fn_1400089f9` 已经把友好度封到 100，重构两边都要饱和，禁止 uint8 绕回。

## 5. 耦合分析

原版：

- 关系字节、友好度、aim、母港、金币挤在同一个 `CForce` 88 字节里
- 失败槽和战果槽挂在 `CForceElement` 尾巴上，和 25 个势力一起存盘
- 月结一个 `vfunc_04` 兼做：外交 AI、交战善后、战果消费、上缴、NPC 买船
- 战后 `fn_1401200a0` 同时写 `CBattleResult` 和 `CForceFailure`
- 灭亡函数直接扫 97 港、25 势力、235 人、7 失败槽
- `eThisForce == 4` 这类分支把剧本癖写进 AI
- 私家舰队用立即数 `0x18` 跳过

重构后所有权：

| 状态 | 所有者 | 别人怎么碰 |
| --- | --- | --- |
| `Relation` / `Aim` / `AimTarget` / `AbsorbedLink` / `FailureSlot` | factions（本篇） | 命令 |
| `Home` / `Gold` / `active` | factions（01） | `Force.setHomeTown` / `Force.collapse` |
| `Amity` / `Share` | factions（03） | 本篇发 `Amity.add` 请求，不直接写 |
| `CBattleResult` 玩法含义 | combat | 本篇 `consume` 已写好的槽 |
| 人物隶属 | characters | 灭亡时 `Person.setForce(id, empty)` |
| 港合约 | 03 的 Share | `Share.clearForce(force_id)` |

禁止 combat 自己 `force.home = 98`。禁止 economy 读失败槽。禁止 `if force_id == 24` 写在战斗或航海里——只在 factions 的 `Force.aiSkips(force_id)`。

## 6. 解耦后的配置

落点：`config/factions/war.yaml`。公式参数见 §4。文案进 `config/ui/strings.zh-CN.yaml`。

```yaml
# config/factions/war.yaml
schema: dk4.factions.war
version: 1
pack: vanilla-world0

private_fleet_id: 24
player_force_from: session          # 原版 fn_140004381；不要写死 24
ai_skip_force_ids: [24]             # 另加「当前玩家势力」，运行时并入

# NUM_FORCERELATION 未还原。原版包按运行时 0..2 实例化 [推断]
relations:
  - { id: 0, key: none,      name_i18n: force.relation.none }      # [推断]
  - { id: 1, key: allied,    name_i18n: force.relation.allied }    # [推断] 开局三对 = 1
  - { id: 2, key: war,       name_i18n: force.relation.war }       # [确认] 代码 == 2

# NUM_FORCEAIMKIND 未还原。只登记观测到的 id
aim_kinds:
  - { id: 0, key: expand,     name_i18n: force.aim.expand }        # [推断] 月结 == 0 分支
  - { id: 1, key: campaign,   name_i18n: force.aim.campaign }      # [推断] 月结 == 1 分支
  - { id: 2, key: war,        name_i18n: force.aim.war }           # [推断] +0x52==2 读 +0x53
  - { id: 4, key: default,    name_i18n: force.aim.default }       # [确认] init 写入 4

# 失败记录形状 ≠ eForceFailureKind 整数。kind 表先空着
failure_slots: 7                    # 原版构造个数 [确认]；下标语义见 §9
failure_record_shapes:
  - { key: fleet_vs_fleet,  writer: fn_14000f808, town: empty }
  - { key: two_force_town,  writer: fn_140005ce0, town: required }
  - { key: one_force_town,  writer: fn_1400087ba, town: required, force_b: empty }
failure_kinds: []                   # NUM_FORCEFAILUREKIND 未还原；禁止填 消灭/投降/合并/灭亡

# eABSORBED 无容器。链接存在 CForce+0x51
absorbed_link_field: absorbed_force_id
num_absorbed: null

# 开局关系 1 的对；2 不应出现在内容包
initial_relations:
  - { a: 6,  b: 11, relation: 1 }
  - { a: 7,  b: 18, relation: 1 }
  - { a: 9,  b: 10, relation: 1 }

diplomatic_actions:
  - { key: declare_war,  sets: { relation: 2 }, event: WarDeclared }
  - { key: ceasefire,    sets: { relation: 0 }, event: CeasefireSigned }  # 停战后写 0 是占位
  - { key: alliance,     sets: { relation: 1 }, event: AllianceFormed }
  - { key: alliance_break, sets: { relation: 0 }, event: AllianceBroken }
  - { key: absorb,       event: ForceAbsorbed }   # 再 Force.collapse(victim)
  - { key: collapse,     event: ForceCollapsed }  # 破产 / 无港

# 原版 eThisForce 立即数，内容包癖，不是引擎
force_ai_overrides:
  - { force_ids: [0, 6],  note: "卡斯特罗 / 阿博科鲁克 月结特殊段" }
  - { force_ids: [3, 4],  note: "李家 / 克利福德" }
  - { force_ids: [15, 16], note: "普雷依拉 / 库恩" }
  - { force_ids: [20, 21, 23], skip_scheme_block: true }

month_tick:
  skip_inactive: true
  skip_player_force: true
  skip_private_fleet: true
  consume_battle_results: true
  collapse_after_ai: true
```

```yaml
# config/ui/strings.zh-CN.yaml 片段（只文案）
force.relation.war: 交战
force.merged: "%s加入麾下了。"
force.merge_refused: "%s拒绝了合并的请求。"
force.war_declared: "%s向%s宣战！"
force.cannot_attack_friendly_fleet: 友好势力的舰队，无法攻击。
force.cannot_attack_allied_town: 同盟势力占有的城市，无法攻击。
# 不要发明「%s灭亡了」——mesfile 没有这句
```

提取器不要手编失败种类中文。World 包只提供开局 `relation=1` 的对；运行时的 2 不进内容表。

## 7. 独立旋钮

改这些不应迫使贸易 / 海战 / 航海改代码：

- 关系枚举条数与名字（加 `ceasefire` 只改本表和 `may_attack`）
- aim 种类、默认 aim=4
- 失败槽个数（原版 7；运行时按表长）
- `private_fleet_id`、月结跳过名单
- `npc_action_gold_min`、`force_collapse_gold`
- 开局同盟对
- `force_ai_overrides`（删掉即所有 NPC 走同一 AI）
- 中立能否不宣而战（`piracy_without_war`，默认关闭）

不在本文件改：友好度怎么加、份额、攻城伤害、上缴。停战是改关系还是另开时限，只改本表 `diplomatic_actions`。

## 8. 对外接口

查询：

```
Relation.get(a, b) -> relation_id
Relation.atWar(a, b) -> bool
Relation.mayAttack(a, b) -> bool          # 舰队
Relation.mayAttackTown(a, town_id) -> bool
Aim.get(force_id) -> aim_id
Aim.target(force_id) -> force_id | empty
Force.isPlayer(force_id) -> bool          # fn_140005b19
Force.aiSkips(force_id) -> bool           # 玩家 或 私家舰队 或 未出场
Force.shouldCollapse(force_id) -> bool
Failure.slots() -> [{ force_a, force_b, town_id, flag }]
Failure.of(force_id) -> [slot]
Absorbed.link(force_id) -> force_id | empty
```

命令：

```
Relation.set(a, b, relation_id, reason)
Aim.set(force_id, aim_id, target_force_id | empty)
Failure.record(shape_key, force_a, force_b | empty, town_id | empty, flag)
Failure.clear(slot_id)
Force.collapse(force_id, reason)          # 内部发 Person / Share 命令
Force.absorb(victim, absorber)            # 写 link，再 collapse(victim)
Force.retargetHome(force_id)
```

事件（只含 ID 与值）：

| 事件 | 载荷 |
| --- | --- |
| `WarDeclared` | `{ a, b, reason }` |
| `CeasefireSigned` | `{ a, b }` |
| `AllianceFormed` / `AllianceBroken` | `{ a, b }` |
| `ForceAbsorbed` | `{ victim, absorber }` |
| `ForceCollapsed` | `{ force_id, reason }`  # `bankrupt` / `no_holdings` / `absorb` / `script` |
| `ForceFailureRecorded` | `{ slot, shape, force_a, force_b, town_id }` |
| `NpcWarIntended` | `{ force_id, target_force_id }` |

`reason`：`scheme`、`palace`、`month_ai`、`battle`、`bankrupt`、`script`。

订阅：

| 来源 | 事件 | 本层动作 |
| --- | --- | --- |
| time | `MonthTicked` | §4 月结；跳过 24 与玩家 |
| combat | `BattleResolved` | 可选写入 Failure 形状 `fleet_vs_fleet`；消费战果槽 |
| combat | `TownCaptured` | 写 `two_force_town` / `one_force_town`；再问 `shouldCollapse` |
| town-life / narrative | 外交计略成功 | `Relation.set` / `Force.absorb` |
| 01 | `ForceHomeChanged` | 母港变空则检查灭亡，变非空则出场 |

不暴露 25 字节数组、不暴露 `+0x8A0`、不暴露 `0x18`。UI（`CForceInformationView`、`CSeaAreaForceView`）只读查询。

## 9. 未核实字段

必须单列。推断可以进 YAML，禁止当确认公式。

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| `NUM_FORCERELATION` | 运行时 0..2，校验 `>= 3` 非法 | 头文件已是 3 |
| 关系名 0/1/2 | 2=交战确认；0/1 靠文案 | 0 一定是停战、1 一定是联姻 |
| `+0x1F` 开局 0/1 | World hex | 「布尔旗，运行时也只有 0/1」 |
| 停战 | 文案有，枚举没第 4 值 | `relation_id = 3` |
| `NUM_FORCEAIMKIND` | 观测 0,1,2,4 | `= 5` 或 `+0x52` 是颜色 |
| aim 名字 | 月结分支 | 「贸易/战争/海盗」三分类已还原 |
| `+0x51` 方向 | 可写可清的 force_id | 一定是宗主，或一定是被吞者 |
| `+0x53` | aim==2 时当对手 | 升格为确认字段名 `war_target` |
| `NUM_ABSORBED` | 仅断言；无类 | `= 7` 或 `= 25` |
| `NUM_FORCEFAILUREKIND` | 仅断言；`+0xB` 是 0/1 | 消灭/投降/合并/灭亡 = 0..3 |
| 7 个失败槽下标 | 个数 7；灭亡循环 RTC 名 `eSeaArea` 上界 7 | 确认 = `NUM_SCENARIO`，或确认 = 大战区 |
| 灭亡消息 `0x16B` | 下标，不是 mes 偏移 | 「某某灭亡了」 |
| 善后消息 `0x17F` | `fn_14000c2f7` | 一定是宣战句 |
| `fn_14000d63e` 的计数 | 0/1/>1 决定灭不灭 | 确认是「港口数」或「舰队数」 |
| `fn_14000e9da` 的 `+0x1C` | 0 才跑外交月结 | 确认是「每月 1 日」 |
| `fn_140004a1b == 6` | 上缴门闩 | 确认是六月或周六 |
| 战果匹配键 | 月结 clear 前有比较 | 一定是玩家势力，或一定是 eThisForce |
| `CBattleResult +9..+C` | 5 字节里多出来的 4 个 | 失败种类 |
| 私家舰队 = 玩家独立势力 | 月结跳过 24 **和** 玩家 | 表上领袖是玩家；id 24 恒等于 `Force.player()` |
| 中立私掠 | 没有海盗关系值 | `relation==0` 就可打 |
| `NUM_THISFORCE = 4` | RTC / `num_values.json` | 就是势力 0–3 或 4 支交战舰队 |
| 主角家 0–3 特殊 AI | `fn_1400015dc` | 这四家不会被灭、不会开战 |
| 友好无法攻击的阈值 | 只有文案 | `amity > 50` |

`CForceFailure` 0x10 里除 `+8..+0xB` 外的填充，进 `unknown_bytes`。hex 与存盘循环：`CForceFailure.cpp`、`CForceElement.cpp`、`fn_1400fe120.cpp`、`fn_14010be70.cpp`、`fn_140107150.cpp`、`fn_1401200a0.cpp`。
