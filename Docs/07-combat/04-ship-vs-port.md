# 舰对港（炮击港口、城防还击、占领）

舰对舰见 `01-naval-battle.md`，接舷见 `02-boarding.md`，NPC 选谁打见 `03-battle-ai.md`。这一篇只定义**舰队打港口**：炮弹打到城上减什么、城怎么打回来、城市何时投降、投降之后战斗交出什么。份额 / 合约 / 母港的写入属于 factions，本系统只发 `TownOccupied`。

## 1. 职责边界

本系统管：

- 海战会话里把 `town_id` 当成可瞄准目标（与船同一套炮弹池）
- 港口按军事发展度还击（城防炮）
- 炮弹命中港口之后的伤害落地（只改军事发展度，经城镇状态接口）
- 投降判定（军事归零，或低于门槛后的掷骰）
- 占领结果事件 `TownOccupied`（载荷只有 ID 与值）
- NPC 月结里舰队打港口的离屏模拟（无 `CCombatScreen`）

本系统不管：

- 船体、射界、舷侧弧的通用公式（`01`；本篇只补「目标是城」的差异）
- 接舷 / 单挑画面（`02`）
- NPC 何时对一座城开战（`03` 发 `Combat.start`；本篇收已经成立的攻城会话）
- 造船厂六炮种解锁表（`ships/guns.yaml` 只读；门槛读的是**当前所在港**的军事发展度）
- 份额三槽、合约、母港、友好度（`factions` 订阅 `TownOccupied` 再改）
- 市价、特产品、投资（**禁止**战斗写经济表）
- 宫殿签合约、卫兵对白（`town-life` / `characters`）
- 文案、像素、城图标大小（`ui/`）

「武装炮台」一词在原版里有两处，不要并成一个系统：船上舱室种类（改装花钱）是 `ships/cabins`；港口自己开炮是本篇的城防。

## 2. 原版事实

### 2.1 攻城不是第三套战斗画面

`combat/` 目录没有陆战 / 巷战 / 攻城 View。玩家打港口仍走 `COceanScene` 战斗 tick（`01` §2.2）：接舷结算 → 命令 / happening → 炮弹 `fn_14000ee8a`，再加一步攻城收尾 `fn_14000601e`（`0x1401910a0`）。

来源：`ocean/COceanScene.cpp` 约 `0x1401a6446` 调 `fn_14000601e`。

离屏（NPC 月结、地方舰队打港口）走 `fn_140004809`（`0x140125610`，RTC `cBattleFleet` / `eFleetShip`），不 new `CCombatScreen`。调用点：`fn_140014f56`（`0x1400eac50`）、`fn_1400f2f80`。

### 2.2 炮弹可以打城，接舷不能登城

`fn_14000ee8a`（`0x1401c3210`）每发弹只在落地 tick 判一次 `[确认]`（全式见 `01` §2.9）：

1. 城中心 = 城坐标 + 尺寸/2（尺寸 `0x30` 若 `fn_14000c71b` 为真，否则 `0x20`）；落点到中心距离平方 `> 576` 则不打城
2. 再扫 `eMobileShip 0..19`：落点在某船命中圆内（半径平方 81/100/121）且该半径平方 `<=` 当前最优值，则改打船、城作废
3. 都没有则落空

打到城镇 `[确认]`：

```
military = fn_14000bc03(town, 1)          # 军事发展度
if military > 0:
    eff    = fn_140012b34(shell)          # 弹威力 +0x28；城炮弹无暴击，舰炮弹按射手运气暴击 ×1.5
    damage = min(eff, military)           # fn_140011bd0 就是 min
    fn_140004f84(town, damage)            # 军事 −= damage
    fn_14000fd62(hud, damage)             # 跳数字
```

这里没有 `idiv`。`eff` 的来源是出膛威力 `(炮种威力 × (3×武装炮台数+1) + rand(3)) / 2`（舷侧），见 `01` §4.3。

接舷入口 `fn_14000660e` / `CCombatCrew*` 的两侧都是船指针（`02`）。没有「对城镇 new 接舷 View」的路径。港外若还有守军舰队，接舷打的是那些**船**，不是城墙。

### 2.3 `fn_140004f84(town, damage)` 只动军事发展度

```
fn_140004f84(town, damage) = fn_140005e07(town, -damage)   # 0x140126f70，体长 0x3a
fn_140005e07: 读发展度 word → 加 delta → fn_14000939f 饱和 → 写回 uint16
```

来源：`fn_140126770.cpp`、`fn_14025ae30.cpp` `0x14025db60`。

`fn_14000bc03(obj, track)`（`0x140262520`）是 `CTown` 发展度 getter：`track` 为 `eTownDevelop`（循环上界 2）。情报 UI 把 `fn_14000bc03(town, 0)` 画在 `posDevelopCommerceNum`，`fn_14000bc03(town, 1)` 画在 `posDevelopMilitaryNum`。来源：`fn_1402645a0.cpp` RTC。

因此炮弹减的是 **军事发展度**（`eTownDevelop = 1`），不是另做一条「城防 HP」，也不是商业发展度，更不是市价。上限 9999（`exe_caps.md` / `TOWNDEVELOPMENT_*`），存储 USHORT。

反编译把 `fn_140005e07` 的写回偏移折成 `+0x1e + 4`；存盘循环是 `+0x1e + track*2`（军事应为 `+0x20`）。实现跟 getter 的字段走，不要按脏偏移另开字段。见第 9 节。

### 2.4 城防还击（港口打舰队）

`fn_140005b37`（`0x1401be570`，RTC `posTown` / `vecTerget` / `posNow`）：

`[确认]`（原始指令）：

1. `fn_14000bed8` 取空弹槽，没有则 return 0
2. 发射点 = 城坐标 + `0x18`（`fn_14000c71b` 真）或 `0x10`，即城图标中心
3. `military = fn_14000bc03(town, 1)`；`g = military / 20 + 1`（当作城的「命中力」）
4. `r = fn_140002d2e(1) = 98`（城炮固定按炮种 1「曲射」的射程）；`g <= 100` → `r −= 12`；`g <= 300` → `r += g/75`；否则 `r += g/30`
5. `d2 < 900` 不开火；`d2 > r²` 不开火；**没有射界检查**
6. `power = fn_14000c6cb(1)（=2） × (3 × (military / 3000) + 1)`，再 `(power + rand(3)) / 2`——城把 `military/3000` 当成武装炮台数
7. `fn_1400052e0(...)` 生成炮弹——与舰炮同一入口，同一散布

城炮装填周期 `max(1, (550 − g) / 10)`（`fn_140003d5f` 无船分支，`01` §2.10）。

`fn_14000c71b`（`0x1402626d0`）读城镇定义 `+0x24`：非 0 返回 1。World 表 `+0x24` 是 `is_capital`。`[确认]` 首都城图标更大；`[推断]` 半宽 0x30/0x20 只影响瞄准点，不改伤害。

谁开火：`fn_1400013ac`（`0x1401a7260`）按炮控制器调用 `[确认]`。gun 0..19 有船：本船舰队 kind 字节 ∈ {0,1} 且有战斗城时**先**瞄城中心；否则按下标找第一艘敌对可打的船。gun 20（无船 = 城炮）：按下标找第一艘舰队 kind ∈ {0,1} 的存活船，调 `fn_140005b37`。

通报文案：

| 偏移 | 句子 |
| --- | --- |
| `0xac7` | `%s城正在炮击%s舰队` / `%s城正在受到海盗%s的攻击！` |
| `0x13af3` | `%s城市炮击我们！` |
| `0x13b17` | `%s城市向我们开炮！` |

`fn_1400078bf`（`0x1401be400`）用 `fn_14000bc03(town, 1)` 和城名 `wsprintfA`，是城防 HUD 句。来源：`fn_1401b0e80.cpp` 调用。

场景火炮池 `eBattleGun >= 0x15`（21）= 20 移动船 + 1 门城炮 `[确认]`（`fn_14000e6ba` 下标 ≥ 20 返回无船，`01` §2.4）。

### 2.5 玩家投降检查（海战 tick 尾）

`fn_14000601e` 在战斗中取当前城：

`[确认]`（原始指令）：

```
military = fn_14000bc03(town, 1)
if military == 0:
    fn_140007b9e(scene, town)                       # 立刻占领收尾
elif military < scene.surrender_check(+0xffa):      # fn_140002824 = getter
    scene.surrender_check = military - military/10  # fn_1400107bc；下一次要再掉 10%
    roll = (Skill(岗位1, 7) + Skill(岗位0, 7)) / 20  # fn_1401ac4a0(post, 7)；eSkill 7 = 说服力
    if roll <= rand(100):
        fn_140007b9e(scene, town)
```

- `+0xffa` 开战时初始化为 `military − military/10`（`0x14014c3ba`），即军事每掉约 10% 掷一次
- `fn_1401ac4a0(post, skill)` → `fn_14000d9ae` → `fn_14000da26`：玩家岗位 `post` 经 `fn_140007e19` 换舱室（0 → 舰长室 5、1 → 副官室 6），取舱内人的技能（带舱室参数）；没人返回 0。岗位 0 / 1 = 舰长 / 副官 `[确认]`（岗位名表 + 映射），与职文「要想让城市投降，需要舰长和副官」（`0x18463`）吻合
- 判定是 `roll <= rand(100)` 才投降：投降率 ≈ `(100 − roll) / 100`。说服力越高 roll 越大、**越难**投降——与职文语义、也与离屏攻城（§2.9，越高越易）相反，但指令如此；重构保留原版方向，作为可翻转旋钮 `surrender.roll_direction`，不在引擎里「修正」
- 旧文「两次都是 (1,7)」「idiv 丢失」均错：第二次是 `(0,7)`，除数是 20

`fn_140007b9e`（`0x1401c55a0`，RTC `cAfterBattleSituation` / `cCatchedShipArray`）：

- 播一段演出（`fn_14000cf1d(..., 0x2f)`）
- `fn_140012fee(town, occupier_force)` —— 改合约（见 §2.8）
- 收集擒获船数组（守军舰队仍按海战规则沉 / 俘）

文案：`%s城市投降了！` / `迫使%s城市投降了！`（`0x13a93`–`0x13ad7`）；`开始攻击%s城市！`（`0x13cbb`、`0x13cf7`）；`占领了%s城……。%s城投降了……。`（`0xa77`）。

### 2.6 舰长、副官、进城

职文（不是算法名）：

- 「要想让城市投降，需要舰长和副官。」（`0x18463`）
- 「要进攻城市，“舰长”﹑“副官”将大显身手。」（`0x1787f`）
- 同段：「现在不能进城，无法进行交涉。不能从这里进行指挥……。」
- 「进攻城市的话，我一定帮得上忙。」（`0x184cf`）
- 「海战时副官的工作只是汇报战况吧。」（`0x18463`）

`combat/` 无陆战类。`CPalace` 只有 ctor/dtor（`town/CPalace.cpp`）。投降检查 `fn_14000601e` 读岗位 0 / 1 的说服力（§2.5），不扫舱室。舱室扫倒是有：`fn_14000f33a`（RTC `eShipCabin`）按舱室种类 `vfunc+0x38` 为 2（甲板）/ 8（武装炮台）/ 其它给舱内人物 10 / 20 / 5——这是战斗中舱室贡献，不是「有舰长才能占城」的守卫。

结论：

- `[确认]` 海战进行中不能进城交涉（职文）
- `[确认]` 占城主路径是炮击把军事打到 0（或过门槛掷骰），不是广场白刃
- `[确认]` 舰长 / 副官岗位（0 / 1）的说服力进投降掷骰（§2.5）；没有「无副官则不投降」的守卫——岗位空时技能按 0 计，反而**更易**投降

「我看%s是绝对不会投降的，这样也可以吗？」（`0xb1df`）是确认框文案，绑定条件未还原。

### 2.7 军事发展度还解锁炮种（造船厂，不是攻城伤害）

`fn_140003bb1`（`0x1400a3290`，RTC `eGunType` / `anNecessaryDevelopment`）：

```
town = current_port(fleet)           # fn_14000c144 → fn_1400103bb
military = fn_14000bc03(town, 1)
for gun_type in 0..5:
    if military >= anNecessaryDevelopment[gun_type]: unlock
```

| `gun_type_id` | `anNecessaryDevelopment` |
| --- | --- |
| 0 | 1 |
| 1 | 1 |
| 2 | 2000 |
| 3 | 3500 |
| 4 | 5000 |
| 5 | 7000 |

来源：`fn_1400a1140.cpp`。门槛属于 `ships/guns.yaml`。攻城把一座港的军事打穿，**副作用**是该港船厂能卖的炮种变少——战斗模块不得自己改炮种表，只改军事发展度。

六炮中文名不在 mesfile，在 exe `.data 0x140682e10`：霰弹 / 曲射 / 加农曲射 / 加农 / 重加农 / 连射（`01` §2.5）。

### 2.8 占领改合约，不改市价

`fn_140012fee`（`0x14025f130`，RTC `nShare`）是占城写状态的函数。玩家收尾 `fn_140007b9e` 和 NPC `fn_140004809` 在进攻方胜时都调用它。

```
fn_140012fee(town, occupier_force_id):
  n = 非空合约槽数（force != 0x1A）
  if n > 0:
    cut = 12 / n                             # 有符号 idiv：1 槽 12、2 槽 6、3 槽 4（0x14025f1ae）
    for slot in 0..2:
      if slot.force == 0x1A: continue
      if occupier != 0x1A: Amity.add(occupier, slot.force, -50)   # fn_1400089f9(…, 0xffffffce)
      share' = saturate(share - cut)         # fn_14000939f 下限 0；占领方自己的旧槽也被削（与 02-factions/03 一致）
      if share' > 0: 写回 share
      else: slot.force = 0x1A; n -= 1        # 拆约
      fn_140008279(...)                      # 通知旧势力
  if occupier != 0x1A and n < 3:
    fn_1400122f6(town, occupier, 10, ...)    # 给占领方 +10 份额
```

`fn_1400122f6` 是通用加份额（卖流行品也走它，见 `02-factions/03-relations-influence.md`）。其中：

```
if 0 <= force_id < 25:
    if Force.home_town == 0x62:              # 98 = 未出场哨兵
        set home_town = this town            # fn_1400092ff
```

来源：`fn_14025ae30.cpp`。空合约哨兵 `force = 0x1A`（26）是原版势力空值；重构运行时用 `null`，26 只在 `ids/legacy_codec.yaml`（D1）。禁止用 `force_id = 0` 当空（0 是卡斯特罗）。

**没有**写 `market_price` / 特产品 / 供给的调用。占领不是经济公式。

守城势力查询 `fn_140004c19`（`0x140260590`）：

```
if slot0.force == 0x1A: return 0x1A        # 原版空势力
if slot0.share < 0x64 (100): return 0x1A
return slot0.force
```

`[确认]` 只读**槽 0**：`imul rax,rax,0x0` 后取 `CTown+0x18`（force）/ `+0x19`（share）（`0x1402605b3`–`0x1402605d4`）。旧稿「`+0x1C/+0x1D` = 槽 2」作废。独占 = 槽 0 且 `share >= exclusive_share`，与 `02-factions/03` `Share.exclusive_owner` 一致；判定归 factions。

### 2.9 NPC 何时离屏打港口

`fn_140014f56`（`0x1400eac50`，RTC `aeShareForce`）：

```
if fleet.force != 0x1A:
    owner = fn_140004c19(town)
    if owner != 0x1A and relation(fleet.force, owner) == 2:
        fn_140004809(fleet, town, owner)     # 宣战关系打独占港
else:
    if fleet_kind == 9 and town 有份额 and 恰好 1 个持份势力:
        if military / 100 + 200 <= rand(300):   # 0x64 / 0xC8 / 0x12C（0x1400ead5c）；军事 0 时 1/3，9999 时 1/300
            fn_140004809(fleet, town, 0x1A)  # 文案侧的海盗攻城
```

`fn_140012dc8 == 2` 被 `fn_14001087f` 包成「可攻击」布尔。来源：`fn_14010be70.cpp`。与「友好势力的舰队，无法攻击」（`0x33ef`）一致的是**关系枚举**，不是友好度阈值。`2` 的名字未恢复，YAML 用 `relation_war: 2` 并标原版立即数。

`fleet_kind == 9` 是海盗分支 `[推断]`（对照文案「海盗%s的攻击」）；`fn_14000aeac(fleet) == 6` 则整段 skip。

离屏模拟 `fn_140004809`（`0x140125610`）`[确认]`（原始指令）：

```
m0   = military(town);  base = m0/10 - 1
g    = clamp(m0/20 + 1, 1, 500)                 # 城「命中力」
pow  = gun_power(1) * (3*clamp(m0/3000, 0, 3) + 1)   # fn_14000eb29(1, n) = 2*(3n+1)
chk  = (military + base) * 10 / m0              # 开战时算一次，之后不更新
r = 2                                           # 2 = 进行中，0 = 攻方胜，1 = 攻方败
loop while r == 2:
    t = random_alive_slot(fleet)                # fn_14001484e；无 → r = 1
    if rand(300) <= max(1, g - slot[t].c - 驾驶/5):
        slot[t].hull -= pow；提督经验 +10
        hull <= 0 → 沉没；t == 0 → r = 1
    for i in 0..4 (alive, r == 2):
        hit = rand(500) <= 命中力
        if hit: military -= 炮威力*(3*n_battery+1)       # fn_14000826f；无运气放大
                military <= 0 → r = 0，经验 20+50
        经验 hit ? 20 : 5
    旗舰水手 <= 0 → r = 1；旗舰耐久 <= 30 → r = 1（退走）
    if m0 > 0 and (military + base)*10/m0 < chk and rand(100) < Skill(提督, 7)/12:
        r = 0（投降标记）                        # 说服力越高越易投降
```

- 与实时海战（§2.5）方向相反：这里说服力**提高**投降率
- 结果消息 0x5F / 0x60（败 / 退走）、0x61 / 0x62（攻陷 / 投降）
- 胜则 `fn_140012fee(town, fleet.force)`
- 消息号 0x5F–0x62 / 0x64 / 0x65 / 0x166（`fn_14000fb14` 索引，不是 mesfile 文件偏移）

### 2.10 卫兵与宫殿不是攻城战斗单元

人物 136–140：卫兵·欧洲 / 阿拉伯 / 东南亚 / 中国 / 日本，六维全 0，不招为航海士（`03-characters/04` `[推断]`；id ≥ 87，存档只存隶属与 `+0x22`）。来源：`PersonID.h`、`persons.tsv`、`01-person-model.md`。

同 9 个文化圈词还用在酒馆掌柜、总督 / 国王 / 大名等职能 NPC 上。港口设施按 `town_id + culture` 选哪个 `person_id` 站岗。来源：`02-factions/02-countries-culture-fashion.md`。

宫殿：设施入口在 `08-town-life/01-facilities.md`；签合约、介绍信放大份额在 factions。卫兵是对话立绘，不是接舷槽，也不是城防炮操作者。城防开火不读卫兵六维。

### 2.11 船上的「武装炮台」舱室

exe 舱室名表下标 8：武装炮台（`05-ships-fleet/03` `cabins.yaml`，`role: gun_room`）。

「由于武装炮台增加了，需要%s枚金币 / 减少了，退还%s枚金币」（`0x32b7` / `0x32db`）是**改装账单**，属 `ships/`。

「武装炮台的火药着火」（`0xf1eb`）`[确认]` 是**炮弹命中船**后的着火判定（`rand(100) <= 武装炮台数`，船态写 0，`01` §2.9），不是 happening，也不在城防函数里。不要把舱室着火写成「城炮被毁」。

### 2.12 两套攻城对照

| | 海上炮战（玩家可见） | 离屏（NPC） |
| --- | --- | --- |
| 入口 | `COceanScene` 战斗 + 目标为城 | `fn_140004809` |
| 城受伤 | 炮弹 → `fn_140004f84` | 每船直接 `fn_140004f84` |
| 城还击 | `fn_140005b37` 发炮弹 | 模拟圈里用军事当半径 / 耐久 |
| 投降 | 军事 0 或低于门槛掷骰 | 军事 0 则攻方胜 |
| 占领 | `fn_140007b9e` → `fn_140012fee` | 胜则 `fn_140012fee` |
| 接舷 | 只对守军船 | 无画面近战（`01` 的 `fn_14000d7ab` 是舰对舰） |
| 进城交涉 | 明文禁止 | 无 |

## 3. 实体与 ID

| 实体 | 稳定 ID | 原版 | 空值 |
| --- | --- | --- | --- |
| 港口 | `town` key | 97 | `null`（原版 98 = 无港，legacy codec） |
| 发展轨 | `town_develop_track` | 2（0 商业 / 1 军事） | `INVALID_TOWNDEVELOP = -1` |
| 合约槽 | `share_slot` | 每港 3 | 槽内势力 `null`（原版 26） |
| 势力 | `force` key | 25 | `null`（原版 26） |
| 攻城会话 | `port_assault_id` | 运行时 | 无原版枚举 |
| 卫兵人物 | `person_id` 136–140 | 5 个文化圈 | 不可招募 |
| 炮种 | `gun_type_id` | 6 | `INVALID_GUNTYPE = -1` |

参战快照额外持 `town_id`（可空）。无城镇则本篇整段 no-op，退回舰对舰。

哨兵：运行时一律 `null`；原版 `0x1A`（空势力）、`0x62`（无港）、`0xEC`（空人物）只在 `ids/legacy_codec.yaml`（D1）。本篇伪码里的 `0x1A` / `0x62` 是原版指令原文。

## 4. 算法 / 公式

立即数进 `config/formulas/combat.yaml` 的 `port_assault:`。投降掷骰读舰长 / 副官的说服力（已确认），人选走 `Fleets.post_holder`，数值走 `Modifiers.eval("skill.persuasion", ctx)`，不读人物字段。

### 4.1 能否瞄准港口

```
can_fire_at_town(shooter, town_id):
  pos = Towns.def(town_id).pos + town_center_offset[size(town_id)]
  return Combat.can_fire(shooter, pos)   # 复用 01 的距离 / 射程 / 射界
```

```yaml
town_center_offset: { large: 24, small: 16 }   # 0x30/2、0x20/2；large = 定义 +0x24 非 0
town_hit_r2: 576                               # 落点距中心平方
```

### 4.2 炮弹 → 军事发展度

```
military = Towns.develop(town_id, military)   # develop track 1（world Towns）
if military <= 0: return
damage = min(effective_power(shell), military)
Towns.apply_develop(town_id, military, -damage, reason=bombard)   # 命令；Towns 写后发 TownDeveloped
emit TownBombarded { battle_id, town_id, damage, military_after }
```

禁止在这里改商业发展度、市价、份额。

### 4.3 城防开火

```
try_town_fire(town_id, target_ship):
  m = Towns.develop(town_id, military)
  g = m / town_gunnery_div + 1
  r = range_base[town_gun_type] + range_bonus(g)          # 01 §4.3 同一折线
  d2 = dist2(town_center, ship)
  if d2 < min_cannon_range2 or d2 > r*r: return none     # 无射界
  n = m / town_battery_div
  power = (gun_power[town_gun_type] * (broadside_mul_k*n + broadside_mul_c) + rand(3)) / 2
  return ShellRequest { from: town_id, to: ship_id, power }
reload = max(reload_min, (reload_ref_other - g) / reload_div)
```

```yaml
town_gun_type: 1               # 曲射，射程 98、威力 2
town_gunnery_div: 20
town_battery_div: 3000         # 实时不封顶（9999 → 3）；离屏封到 town_battery_cap
town_battery_cap: 3
town_targets_side_kind: [0, 1]
```

### 4.4 投降

```
check_surrender(town_id):
  m = Towns.develop(town_id, military)
  if m <= 0: return true
  if m >= session.next_check: return false
  session.next_check = m - m / surrender_step_div
  roll = sum(Modifiers.eval("skill." + key_name(surrender_skill), {person: Fleets.post_holder(player_fleet, p)}) or 0
             for p in surrender_posts) / surrender_skill_div
  return surrender_when_roll_le_rand ? roll <= rand(100) : roll > rand(100)
  # key_name 去包前缀（vanilla:persuasion → persuasion）；修正器 key 形如 skill.persuasion
```

```yaml
surrender_at_zero: true
surrender_step_div: 10         # 开战 next_check = m0 - m0/10
surrender_skill_div: 20
surrender_posts: [vanilla:captain, vanilla:adjutant]   # 原版岗位 0 / 1 [确认]
surrender_skill: vanilla:persuasion   # eSkill 7
surrender_when_roll_le_rand: true   # 原版方向：说服力越高越难投降；改 false 即「越高越易」
offscreen:
  town_fire_roll: 300
  fleet_fire_roll: 500
  surrender_skill_div: 12      # rand(100) < 说服力/12；方向与实时相反（越高越易），原版不一致，两段各自保留
```

### 4.5 占领结果（战斗只发事件）

原版 `fn_140012fee` 的数值归 **factions**（`02-factions/03` §6 `occupation`：`share_cut_total: 12`、`occupier_grant: 10`、`amity_delta_displaced: -50`、`set_home_if_none`；独占判定 `exclusive_share: 100`）。本文件不再列值，战斗不得直接写槽：

```
emit TownOccupied {
  town_id,
  from_force_id,               # 原独占方；无则 null
  to_force_id,
  battle_id
}
# factions（02-factions/03 §2）：
#   各旧槽 share -= trunc(share_cut_total / n)，到 0 拆约；Amity.add(to, old, amity_delta_displaced)
#   Share.add(town, to_force, occupier_grant)
#   若 to_force 母港为 null：母港 = town
```

三槽之和仍封到 `share_max - 已占`（factions 已有）。战斗不调用 `Economy.quote`。

### 4.6 NPC 离屏是否开打

```
if attacker.force != null:
    owner = Share.exclusive_owner(town_id)    # 槽 0 且 share >= exclusive_share（factions）
    if owner != null and Relation.get(attacker, owner) == relation_war:
        Combat.resolve_offscreen_town(fleet_id, town_id)
else if attacker.kind == pirate_kind and Share.holder_count(town) == 1:
    if Towns.develop(town, military) / pirate_military_div + pirate_military_bias <= rand(pirate_roll):
        Combat.resolve_offscreen_town(fleet_id, town_id)
```

```yaml
relation_war: 2
pirate_kind: 9                 # [推断]
pirate_military_div: 100
pirate_military_bias: 200
pirate_roll: 300
offscreen_win_person_bonus: 50 # 军事打穿时 fn_1400026d5
defeat_hull_or_crew: 30        # 与 naval 共用
```

## 5. 耦合分析

| 原版纠缠 | 重构 |
| --- | --- |
| `CTown` 军事 word 既是船厂解锁又是城防 HP | 状态归 world `Towns`；战斗 / 造船厂都只读 `Towns.develop(town_id, military)`，战斗写只发命令 `Towns.apply_develop` |
| `fn_140012fee` 在战斗收尾里直接改三槽 | 战斗发 `TownOccupied`；factions 写份额 / 母港 |
| `fn_1400122f6` 同时服务流行品加份额和占城 +10 | 一个 Share 命令，两个 `reason` |
| 炮弹合成器与打船体共用 `fn_140011bd0` | `combine_shell_and_hp` 纯函数，目标类型当参数 |
| 城防与舰炮共用 `fn_1400052e0` / 最小距离 900 | 同一 `ShellRequest` 池 |
| `COceanScene` 把攻城收尾塞进海战 tick | Ocean 发 `BattleTick`；Combat 判断会话是否带 `town_id` |
| 投降掷骰写死岗位 0 / 1 | `surrender_posts` 列 post key；引擎不 if 岗位号 |
| 卫兵 person 136–140 | 设施 NPC，战斗不查 |
| 武装炮台舱室花费 | `ships/` 改装账单 |
| `CForceElement` 月结扫战果 | factions 订 `BattleResolved` / `TownOccupied` |

禁止：combat → economy 写价格；combat → 直接 `Share.setSlot`；UI 写存档。原版 `fn_140012fee` 在战斗函数里写份额，是要拆掉的硬编码，不是重构后的所有权。

## 6. 解耦后的配置

### `config/combat/port_assault.yaml`

```yaml
schema: dk4.combat.port_assault
version: 1
develop_track_military: 1
sprite:
  half_capital: 48
  half_normal: 32
surrender:
  at_military_zero: true
  roll_enabled: true           # 门槛 = 场景 +0xffa，已还原（§2.5）
  roll_span: 100
  roll_direction: le_rand      # 原版：roll <= rand 才降（说服越高越难）；改 gt_rand 即越高越易
occupation:
  event: TownOccupied
  share_cut_total: 12          # 每条旧约 −12/n
  share_grant: 10
  amity_delta_displaced: -50
  exclusive_share: 100
  grant_home_town_if_none: true
  max_slots: 3
npc:
  relation_war: 2
  pirate_kind: 9               # [推断]
  pirate_share_holders: 1
offscreen: true
allow_boarding_town: false     # 引擎不变量：城不是接舷目标
allow_enter_town_during_battle: false
ui_keys:                       # 文案 key 取自 exe 消息号表（D10），不用 mesfile 偏移
  - battle.town_attack_start
  - battle.town_surrendered
  - battle.town_fires
  - battle.town_occupied
  - battle.town_pirate_raid
```

文案 key 只引用，不把中文写进逻辑表。

### `config/formulas/combat.yaml`（攻城段）

```yaml
port_assault:
  impl: vanilla.port_assault
  int_mode: vanilla_i32
  params:
    min_cannon_range: 900
    shell_acquire_limit: 576
    aim_skill_cut: 100
    aim_skill_cut_hi: 300
    aim_skill_penalty: 12
    surrender_roll_span: 100
    # 占城份额 / 友好度 / exclusive_share 归 02-factions/03 occupation，不在此重复
    pirate_military_div: 100
    pirate_military_bias: 200
    pirate_roll: 300
    offscreen_win_person_bonus: 50
    defeat_hull_or_crew: 30
```

与 `naval:` 重复的键（900、576、30、100/12）保持同名同值；改一处时两段一起改，或抽到 `combat.shared`。不要为攻城另做一套射程。

炮弹对城的合成函数未还原，不在 YAML 里填伤害骰。

### `config/ships/guns.yaml`（只读，已在 01）

军事门槛 1 / 1 / 2000 / 3500 / 5000 / 7000。造船厂问 `Towns.develop(current_port, military)`，不问战斗会话。

### `config/caps.yaml` 片段

```yaml
town_develop: 9999
town_develop_tracks: 2
share_slots_per_town: 3
share_max: 100
# 空势力 26、无港 98 不进 caps：运行时 null，原版值只在 ids/legacy_codec.yaml（D1）
```

## 7. 独立旋钮

改这些值不应迫使贸易、人物成长、航线改代码：

- 投降是否允许「未打到 0 的掷骰」、`surrender_roll_span`
- 投降方向 `surrender.roll_direction`（实时原版越说服越难降，离屏原版越说服越易降；两者独立保留）
- 占城份额削减 / 给予 / 独占线 / 友好度（`02-factions/03` `occupation`、`exclusive_share`，不在本文件）
- 城防是否与舰炮共用最小距离 900
- NPC 是否离屏攻城、海盗 `kind`、宣战关系值
- 首都 / 非首都瞄准半宽
- 占城后是否把空母港写成此城

加「陆战阶段」：新开 `layouts[]` / 场景，不改炮弹池，也不要复用接舷 20 槽去塞卫兵。

把军事发展度改成独立 `town_hp` 字段：只加 town 状态一列，战斗继续读同一查询；不要让战斗持有第二份 HP。

## 8. 对外接口

查询：

```
Towns.develop(town_id, track) -> int             # world；城防读 military 轨
Towns.def(town_id).is_capital -> bool
Share.exclusive_owner(town_id) -> force_id | null
Share.slots(town_id) -> [{ force_id, share }]
Relation.get(a, b) -> int
Relation.may_attack(a, b) -> bool
Combat.snapshot(battle_id).town_id -> town_id | null
```

命令：

```
Combat.start(BattleStart { ..., town_id? })
Combat.issue(..., target: town_id)
Towns.apply_develop(town_id, track, delta, reason)   # world Towns；战斗 reason=bombard
Combat.resolve_offscreen_town(fleet_id, town_id)
```

事件：

```
TownBombarded  { battle_id, town_id, damage, military_after }
TownFired      { battle_id, town_id, target_ship_id }
TownOccupied   {
  battle_id,
  town_id,
  from_force_id,             # null 表示无独占方
  to_force_id
}
BattleResolved {
  ...,
  town_occupied: town_id | null
}
```

`TownOccupied` 与 `BattleResolved.town_occupied` 同时发：前者给 factions 做合约，后者给 UI / 名声。

订阅：

| 系统 | 事件 | 动作 |
| --- | --- | --- |
| factions | `TownOccupied` | 切份额、拆旧约、或写母港；**不算**伤害 |
| ui | `TownBombarded` | 炮击表现；发展度变化看 `TownDeveloped`（战斗已直接发命令 `Towns.apply_develop`，town 不再订此事件扣值） |
| ships / 船厂 UI | `TownDeveloped`（只由 world `Towns` 发出，`00-overview/06`） | 刷新可卖炮种 |
| progression | `BattleResolved` | 经验 / 名声 |
| economy | — | **不订**占城；不改市价 |

查询人物：投降掷骰经 `Fleets.post_holder(player_fleet, post_key)` + `Modifiers.eval("skill.persuasion", ctx)`；离屏攻城读提督说服力。不读人物字段。

## 9. 未核实字段

| 项 | 事实 | 不要写成 |
| --- | --- | --- |
| `fn_140011bd0` 城伤闭式 | `[确认]` = `min`，见 §2.2 | — |
| `fn_140005e07` 写回 +0x22 vs +0x20 | 脏偏移；语义跟 track 1 | 另做 `town_hp` 字段当原版 |
| `fn_140002824` 投降门槛 | `[确认]` = 场景 `+0xffa` getter，开战 `m0 − m0/10`，每掷一次重设为 `m − m/10` | — |
| 岗位 0 / 1 名字 | `fn_1401ac4a0(post, 7)` 取岗位上人物的说服力（eSkill 7）`[确认]`；0 = 舰长、1 = 副官 `[确认]`（名表 + `fn_140007e19`） | — |
| 舰长 / 副官是否门闩 | 只有职文；投降函数未扫岗位 | 无舰长不能占城（已还原） |
| `0xb1df`「绝对不会投降」 | 确认框 | 首都 / 剧本城永不降 |
| `fn_140004c19` 只读槽 0 | `[确认]` 偏移 +0x18/+0x19（`imul rax,rax,0x0`） | 「任一槽 100」或槽 2 |
| 独占 = 单槽 100 vs 仅一槽非空 | 谓词是 share≥100 | 二者等同 |
| `fn_1400089f9` 占城时对旧势力 | `[确认]` 占领方与每个旧约势力友好度 −50 | — |
| `fleet_kind == 9` 海盗 | 文案有海盗攻城 | 已恢复的 `eFleetKind` 名 |
| `relation == 2` | 可攻击布尔 | `eForceRelation` 正式名 |
| 城防射程 | `[确认]` `fn_140002d2e(1) = 98` ± 命中力折线，`g = 军事/20 + 1` | — |
| 「炮台着火」 | `[确认]` 炮弹命中船的着火判定（`01` §2.9） | 城炮被毁 |
| 卫兵 136–140 | 设施 NPC，六维 0 | 攻城白刃单位 |
| 进城交涉 | 海战中禁止；无陆战类 | 第二套接舷 |
| 消息号 0x5F–0x62 与 mesfile 偏移 | `fn_14000fb14` 索引 | 等于 `0x13a93` |
| 离屏 `idiv` | `[确认]` /10、/20、/3000、/5、/12，见 §2.8 后段 | — |
| 占城是否清流行 / 特产品 | 本批 cpp 未见 | 占领后市场重置 |
| `CTown +0x24` 两 dword | 存盘有；近邻有 9999999 封顶 | 城防最大 HP |

城伤、城炮、投降、离屏攻城、占城份额削减（`12/n`）、海盗攻城掷骰（`军事/100 + 200`）已按原始指令还原（§2.2、§2.4、§2.5、§2.8、§2.9）。不要从光荣其他作的攻城表抄数值。
