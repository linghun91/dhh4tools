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

`fn_14000ee8a`（`0x1401c3210`）每发活弹在 `shell_acquire_limit`（`0x240 = 576`）内找最近目标：

1. 先取一个城镇指针（局部名 `posTown` / `s70`）
2. 再扫 `eMobileShip 0..19`
3. 更近的船会盖掉城；否则命中城

打到船上走船体 / 水手（`01` §2.9）。打到城镇：

```
military = fn_14000bc03(town, 1)          # 军事发展度
if military <= 0: skip
raw     = fn_140012b34(shell)
damage  = fn_140011bd0(raw, military)    # 与打船体共用合成器
fn_140004f84(town, damage)
fn_14000fd62(hud, damage)                # 跳数字，无公式
```

来源：`fn_1401b9940.cpp` `fn_14000ee8a` 约 `0x1401c35b8`。`idiv` 除数在反编译里经常丢失，**禁止**把 `fn_140011bd0` 补成「每炮种 -N 点」表。

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

1. 取城坐标；`fn_14000c71b(town)` 为真则半宽 `0x30`，否则 `0x20`，用来对准城图标中心
2. `military = fn_14000bc03(town, 1)`，`sA0 = military + 1`
3. 有效半径与舰炮同一套折点：`sA0 <= 100` 则半径 −12；否则把 `sA0` 加进半径（`idiv` 丢失）
4. **最小距离** `0x384 = 900`：更近则 return（与「靠得太近炮就无法射击了」同源）
5. 距离平方超过半径平方则 return
6. `fn_1400052e0(...)` 生成炮弹——与舰炮同一入口

`fn_14000c71b`（`0x1402626d0`）读城镇定义 `+0x24`：非 0 返回 1。World 表 `+0x24` 是 `is_capital`。`[确认]` 首都城图标更大；`[推断]` 半宽 0x30/0x20 只影响瞄准点，不改伤害。

谁在「城的回合」开火：`fn_1400013ac`（`0x1401a7260`）先让船打城（`fn_14000c8c4`），扫完 `eMobileShip` 仍没有射手时调 `fn_140005b37`。来源：`fn_140194480.cpp`。

通报文案：

| 偏移 | 句子 |
| --- | --- |
| `0xac7` | `%s城正在炮击%s舰队` / `%s城正在受到海盗%s的攻击！` |
| `0x13af3` | `%s城市炮击我们！` |
| `0x13b17` | `%s城市向我们开炮！` |

`fn_1400078bf`（`0x1401be400`）用 `fn_14000bc03(town, 1)` 和城名 `wsprintfA`，是城防 HUD 句。来源：`fn_1401b0e80.cpp` 调用。

场景火炮池 `eBattleGun >= 0x15`（21）与「4 支 mobile fleet × 5 船 + 1 门城炮」同数，仍标 `[推断]`（`01` §2.4）。

### 2.5 玩家投降检查（海战 tick 尾）

`fn_14000601e` 在战斗中取当前城：

```
military = fn_14000bc03(town, 1)
if military == 0:
    fn_140007b9e(scene, town)          # 立刻占领收尾
elif military < fn_140002824(...):     # 门槛，函数体未还原
    roll = fn_1401ac4a0(1, 7) + fn_1401ac4a0(1, 7)   # idiv 丢失
    if roll <= rand(100):
        fn_140007b9e(scene, town)
# 否则城还在打，继续把目标点对准城
```

来源：`fn_140188eb0.cpp` 约 `0x1401910a0`。`fn_140002824` 体长 0x14，反编译是空壳；门槛值 `[未知]`。`fn_1401ac4a0` 另用于航海随机，第二参 `7` 不要写成「说服力」除非对照上 `eSkill`。

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

`combat/` 无陆战类。`CPalace` 只有 ctor/dtor（`town/CPalace.cpp`）。投降检查 `fn_14000601e` **没有**扫舰长室 / 副官室。舱室扫倒是有：`fn_14000f33a`（RTC `eShipCabin`）按 `vfunc+0x38` 为 2 / 8 / 其它给人物 10 / 20 / 5——这是战斗中舱室贡献，不是「有舰长才能占城」的守卫。

结论：

- `[确认]` 海战进行中不能进城交涉（职文）
- `[确认]` 占城主路径是炮击把军事打到 0（或过门槛掷骰），不是广场白刃
- `[未知]` 舰长 / 副官岗位是否改投降掷骰；未在本批 cpp 看到岗位守卫。原版包不要把「无副官则永不投降」写成已还原规则

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

六炮中文名 mesfile 无独立条目，禁止按「卡隆 / 加农」猜。

### 2.8 占领改合约，不改市价

`fn_140012fee`（`0x14025f130`，RTC `nShare`）是占城写状态的函数。玩家收尾 `fn_140007b9e` 和 NPC `fn_140004809` 在进攻方胜时都调用它。

```
fn_140012fee(town, occupier_force_id):
  n = 非空合约槽数（force != 0x1A）
  if n > 0:
    for slot in 0..2:
      if slot.force == 0x1A: continue
      # 对原槽势力做一次关系通知 fn_1400089f9（友好度？[未知]）
      share' = saturate(share - 12)          # 立即数 0xC；前面 idiv 丢失
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

来源：`fn_14025ae30.cpp`。空合约哨兵 `force = 0x1A`（26），与势力运行时空值相同。禁止用 `force_id = 0` 当空（0 是卡斯特罗）。

**没有**写 `market_price` / 特产品 / 供给的调用。占领不是经济公式。

守城势力查询 `fn_140004c19`（`0x140260590`）：

```
if slot.force == 0x1A: return empty
if slot.share < 0x64 (100): return empty
return slot.force
```

反编译只读 `CTown+0x1C/+0x1D`（按 `+0x18` 起每槽 2 字节，这是**槽 2**）。「独占 = 某槽份额 ≥ 100」是确认谓词；「独占是否永远写在槽 2」标 `[未知]`。重构用「任一槽 `share >= exclusive_share`」当引擎规则，原版包可把检查槽写成旋钮。

### 2.9 NPC 何时离屏打港口

`fn_140014f56`（`0x1400eac50`，RTC `aeShareForce`）：

```
if fleet.force != 0x1A:
    owner = fn_140004c19(town)
    if owner != 0x1A and relation(fleet.force, owner) == 2:
        fn_140004809(fleet, town, owner)     # 宣战关系打独占港
else:
    if fleet_kind == 9 and town 有份额 and 恰好 1 个持份势力:
        if military + 200 <= rand(300):      # 0xC8 / 0x12C；military 侧 idiv 丢失
            fn_140004809(fleet, town, 0x1A)  # 文案侧的海盗攻城
```

`fn_140012dc8 == 2` 被 `fn_14001087f` 包成「可攻击」布尔。来源：`fn_14010be70.cpp`。与「友好势力的舰队，无法攻击」（`0x33ef`）一致的是**关系枚举**，不是友好度阈值。`2` 的名字未恢复，YAML 用 `relation_war: 2` 并标原版立即数。

`fleet_kind == 9` 是海盗分支 `[推断]`（对照文案「海盗%s的攻击」）；`fn_14000aeac(fleet) == 6` 则整段 skip。

离屏模拟 `fn_140004809` 要点：

- 用 `fn_14000bc03(town, 1)` 当城的耐久
- 循环己方 5 船，对城 `fn_140004f84`
- 军事打到 ≤0：进攻方胜（`s140 = 0`），人物贡献 +50（`0x32`，`fn_1400026d5`），不是再打一发城伤
- 旗舰水手 0 或船体 ≤30：进攻失败（与海战溃败阈值 30 同源）
- 胜则 `fn_140012fee(town, fleet.force)`
- 消息号 0x5F–0x62 / 0x64 / 0x65 / 0x166（`fn_14000fb14` 索引，不是 mesfile 文件偏移）

### 2.10 卫兵与宫殿不是攻城战斗单元

人物 136–140：卫兵·欧洲 / 阿拉伯 / 东南亚 / 中国 / 日本，六维全 0，不入 98 名册。来源：`PersonID.h`、`persons.tsv`、`01-person-model.md`。

同 9 个文化圈词还用在酒馆掌柜、总督 / 国王 / 大名等职能 NPC 上。港口设施按 `town_id + culture` 选哪个 `person_id` 站岗。来源：`02-factions/02-countries-culture-fashion.md`。

宫殿：设施入口在 `08-town-life/01-facilities.md`；签合约、介绍信放大份额在 factions。卫兵是对话立绘，不是接舷槽，也不是城防炮操作者。城防开火不读卫兵六维。

### 2.11 船上的「武装炮台」舱室

exe 舱室名第 9 项（下标 8）：武装炮台。来源：`03-abilities-skills-jobs.md` `0x52dfbc`。

「由于武装炮台增加了，需要%s枚金币 / 减少了，退还%s枚金币」（`0x32b7` / `0x32db`）是**改装账单**，属 `ships/`。

「武装炮台的火药着火」（`0xf1eb`）是战斗 happening 文案（`03` §2.4），未在城防函数字符串里出现。不要把舱室着火写成「城炮被毁」。

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
| 港口 | `town_id` | 97 | `INVALID_TOWN = -1`；未选 98 |
| 发展轨 | `town_develop_track` | 2（0 商业 / 1 军事） | `INVALID_TOWNDEVELOP = -1` |
| 合约槽 | `share_slot` | 每港 3 | `force_id = 26` |
| 势力 | `force_id` | 25 | 26 |
| 攻城会话 | `port_assault_id` | 运行时 | 无原版枚举 |
| 卫兵人物 | `person_id` 136–140 | 5 个文化圈 | 不入名册 |
| 炮种 | `gun_type_id` | 6 | `INVALID_GUNTYPE = -1` |

参战快照额外持 `town_id`（可空）。无城镇则本篇整段 no-op，退回舰对舰。

哨兵：`force.empty = 26`、`town.none = 98`、`person.empty = 236`（接舷槽，本篇不用）。

## 4. 算法 / 公式

立即数进 `config/formulas/combat.yaml` 的 `port_assault:`。人物侧若以后证实副官影响掷骰，只消费 `ModifierSet`，不读六维。

### 4.1 能否瞄准港口

```
can_fire_at_town(shooter, town_id):
  pos = town_aim_point(town_id)     # 定义坐标 + 图标半宽
  return Combat.can_fire(shooter, pos)   # 复用 01 的 900 / 射界
```

```yaml
town_sprite_half_capital: 48    # 0x30；is_capital
town_sprite_half_normal: 32     # 0x20
min_cannon_range: 900           # 与 naval 同一键
```

### 4.2 炮弹 → 军事发展度

```
military = Town.military(town_id)          # develop track 1
if military <= 0: return
damage = combine_shell_and_hp(shell, military)   # fn_140011bd0；闭式未知
Town.apply_develop(town_id, track=military, delta=-damage)
# 饱和 [0, caps.town_develop]
emit TownBombarded { battle_id, town_id, damage, military_after }
```

禁止在这里改商业发展度、市价、份额。

### 4.3 城防开火

```
try_town_fire(town_id, target_ship):
  m = Town.military(town_id) + 1
  radius = town_base_range()
  if m <= aim_skill_cut: radius -= aim_skill_penalty
  else: radius += m                 # 原版还有 idiv，未还原前不要再乘表
  dist2 = distance2(town, ship)
  if dist2 < min_cannon_range: return none
  if dist2 > radius * radius: return none
  return ShellRequest { from: town_id, to: ship_id }
```

```yaml
aim_skill_cut: 100
aim_skill_cut_hi: 300          # 0x12C，城防与舰炮同一折点
aim_skill_penalty: 12
```

`town_base_range` 闭式 `[未知]`，实现按 `fn_140005b37` 调用图对照，不编「每军事点 = N 格」。

### 4.4 投降

```
check_surrender(town_id):
  m = Town.military(town_id)
  if m <= 0: return true
  if m >= surrender_threshold(town_id): return false
  return (town_surrender_roll() <= rand(surrender_roll_span))
```

```yaml
surrender_at_zero: true
surrender_roll_span: 100       # rand(0x64)
surrender_threshold: null      # fn_140002824 未还原；禁止填假数
surrender_roll_args: [1, 7]    # fn_1401ac4a0 原版参数；语义未知
```

`surrender_threshold` 未还原前：原版包只保证「军事 0 必降」；低于门槛的掷骰做成可关开关 `surrender_roll_enabled: false` 直到对照完成。

### 4.5 占领结果（战斗只发事件）

原版 `fn_140012fee` 的数值，重构后由 **factions** 执行，战斗不得直接写槽：

```yaml
occupation_share_cut: 12       # 每条旧约
occupation_share_grant: 10     # 给占领方；槽满（3）则不加
exclusive_share: 100           # fn_140004c19
home_town_if_none: true        # 占领方母港为 98 则写成此港
relation_on_displace: unknown  # fn_1400089f9
```

```
emit TownOccupied {
  town_id,
  from_force_id,               # 原独占方；无则 force.empty
  to_force_id,
  battle_id
}
# factions:
#   各旧槽 share -= occupation_share_cut，到 0 拆约
#   Share.add(town, to_force, occupation_share_grant)
#   若 to_force.home == town.none: home = town
```

三槽之和仍封到 `share_max - 已占`（factions 已有）。战斗不调用 `Economy.quote`。

### 4.6 NPC 离屏是否开打

```
if attacker.force != empty:
    owner = Share.exclusive_owner(town_id)    # share >= exclusive_share
    if owner != empty and Factions.relation(attacker, owner) == relation_war:
        Combat.resolve_offscreen_town(fleet_id, town_id)
else if attacker.kind == pirate_kind and Share.holder_count(town) == 1:
    if Town.military(town) + pirate_military_bias <= rand(pirate_roll):
        Combat.resolve_offscreen_town(fleet_id, town_id)
```

```yaml
relation_war: 2
pirate_kind: 9                 # [推断]
pirate_military_bias: 200
pirate_roll: 300
offscreen_win_person_bonus: 50 # 军事打穿时 fn_1400026d5
defeat_hull_or_crew: 30        # 与 naval 共用
```

## 5. 耦合分析

| 原版纠缠 | 重构 |
| --- | --- |
| `CTown` 军事 word 既是船厂解锁又是城防 HP | 状态归 town；战斗 / 造船厂都只读 `Town.military` |
| `fn_140012fee` 在战斗收尾里直接改三槽 | 战斗发 `TownOccupied`；factions 写份额 / 母港 |
| `fn_1400122f6` 同时服务流行品加份额和占城 +10 | 一个 Share 命令，两个 `reason` |
| 炮弹合成器与打船体共用 `fn_140011bd0` | `combine_shell_and_hp` 纯函数，目标类型当参数 |
| 城防与舰炮共用 `fn_1400052e0` / 最小距离 900 | 同一 `ShellRequest` 池 |
| `COceanScene` 把攻城收尾塞进海战 tick | Ocean 发 `BattleTick`；Combat 判断会话是否带 `town_id` |
| 职文舰长 / 副官 | `ui/strings`；未还原前不要在战斗代码 if 岗位 |
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
  roll_enabled: false          # 门槛 fn_140002824 还原前关闭掷骰支
  roll_span: 100
occupation:
  event: TownOccupied
  share_cut: 12
  share_grant: 10
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
ui_keys:
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
  min_cannon_range: 900
  shell_acquire_limit: 576
  aim_skill_cut: 100
  aim_skill_cut_hi: 300
  aim_skill_penalty: 12
  surrender_roll_span: 100
  occupation_share_cut: 12
  occupation_share_grant: 10
  exclusive_share: 100
  pirate_military_bias: 200
  pirate_roll: 300
  offscreen_win_person_bonus: 50
  defeat_hull_or_crew: 30
```

与 `naval:` 重复的键（900、576、30、100/12）保持同名同值；改一处时两段一起改，或抽到 `combat.shared`。不要为攻城另做一套射程。

炮弹对城的合成函数未还原，不在 YAML 里填伤害骰。

### `config/ships/guns.yaml`（只读，已在 01）

军事门槛 1 / 1 / 2000 / 3500 / 5000 / 7000。造船厂问 `Town.military(current_port)`，不问战斗会话。

### `config/caps.yaml` 片段

```yaml
town_develop: 9999
town_develop_tracks: 2
share_slots_per_town: 3
share_max: 100
force_empty: 26
town_none: 98
```

## 7. 独立旋钮

改这些值不应迫使贸易、人物成长、航线改代码：

- 投降是否允许「未打到 0 的掷骰」、`surrender_roll_span`
- `occupation_share_cut` / `share_grant` / `exclusive_share`
- 城防是否与舰炮共用最小距离 900
- NPC 是否离屏攻城、海盗 `kind`、宣战关系值
- 首都 / 非首都瞄准半宽
- 占城后是否把空母港写成此城

加「陆战阶段」：新开 `layouts[]` / 场景，不改炮弹池，也不要复用接舷 20 槽去塞卫兵。

把军事发展度改成独立 `town_hp` 字段：只加 town 状态一列，战斗继续读同一查询；不要让战斗持有第二份 HP。

## 8. 对外接口

查询：

```
Town.military(town_id) -> int
Town.commerce(town_id) -> int
Town.is_capital(town_id) -> bool
Share.exclusive_owner(town_id) -> force_id | empty
Share.slots(town_id) -> [{ force_id, share }]
Factions.relation(a, b) -> int
Factions.may_attack(a, b) -> bool
Combat.snapshot(battle_id).town_id -> town_id | null
```

命令：

```
Combat.start(BattleStart { ..., town_id? })
Combat.issue(..., target: town_id)
Town.apply_develop(town_id, track, delta, reason)   # 战斗 reason=bombard
Combat.resolve_offscreen_town(fleet_id, town_id)
```

事件：

```
TownBombarded  { battle_id, town_id, damage, military_after }
TownFired      { battle_id, town_id, target_ship_id }
TownOccupied   {
  battle_id,
  town_id,
  from_force_id,             # force.empty 表示无独占方
  to_force_id
}
BattleResolved {
  ...,
  town_occupied: town_id | null
}
```

`TownOccupied` 与 `BattleResolved.town_occupied` 同时发：前者给 factions 做合约，后者给 UI / 名声。factions 文档里的 `TownCaptured` 与本事件同一载荷，实现用 `TownOccupied` 这一个名字。

订阅：

| 系统 | 事件 | 动作 |
| --- | --- | --- |
| factions | `TownOccupied` | 切份额、拆旧约、或写母港；**不算**伤害 |
| town 状态 | `TownBombarded` | `apply_develop(military, -damage)` |
| ships / 船厂 UI | `TownDeveloped`（factions/town 发出） | 刷新可卖炮种 |
| progression | `BattleResolved` | 经验 / 名声 |
| economy | — | **不订**占城；不改市价 |

查询人物：本篇默认不读 `ModifierSet`。副官若以后进投降掷骰，只加键 `battle_town_surrender`，vanilla 映射待对照。

## 9. 未核实字段

| 项 | 事实 | 不要写成 |
| --- | --- | --- |
| `fn_140011bd0` 城伤闭式 | 与打船体共用；`idiv` 丢失 | 每炮种固定减军事 N 点 |
| `fn_140005e07` 写回 +0x22 vs +0x20 | 脏偏移；语义跟 track 1 | 另做 `town_hp` 字段当原版 |
| `fn_140002824` 投降门槛 | 体长 0x14，空壳 | 「军事 < 30% 必降」 |
| `fn_1401ac4a0(1, 7)` | 两掷相加 vs `rand(100)` | 副官说服力、技能下标 7 |
| 舰长 / 副官是否门闩 | 只有职文；投降函数未扫岗位 | 无舰长不能占城（已还原） |
| `0xb1df`「绝对不会投降」 | 确认框 | 首都 / 剧本城永不降 |
| `fn_140004c19` 只读槽 2 | 偏移 +0x1C/+0x1D | 引擎写死 exclusive_slot=2 为玩法 |
| 独占 = 单槽 100 vs 仅一槽非空 | 谓词是 share≥100 | 二者等同 |
| `fn_1400089f9` 占城时对旧势力 | 有调用 | 友好度 −N |
| `fleet_kind == 9` 海盗 | 文案有海盗攻城 | 已恢复的 `eFleetKind` 名 |
| `relation == 2` | 可攻击布尔 | `eForceRelation` 正式名 |
| 城防基数半径 | 与 `fn_140002d2e(1)` 有关 | 每军事点 1 格 |
| happening「炮台着火」 | 文案；可能是船舱 8 | 城炮被毁确认 |
| 卫兵 136–140 | 设施 NPC，六维 0 | 攻城白刃单位 |
| 进城交涉 | 海战中禁止；无陆战类 | 第二套接舷 |
| 消息号 0x5F–0x62 与 mesfile 偏移 | `fn_14000fb14` 索引 | 等于 `0x13a93` |
| 离屏 `idiv`（军事÷什么） | 多处 cdq/idiv | 军事/船数 当确认式 |
| 占城是否清流行 / 特产品 | 本批 cpp 未见 | 占领后市场重置 |
| `CTown +0x24` 两 dword | 存盘有；近邻有 9999999 封顶 | 城防最大 HP |

还原伤害时对着 `fn_14000ee8a`（`0x1401c3210`）和 `fn_140004f84`（`0x140126f70`）下断点。还原投降门槛对着 `fn_14000601e`（`0x1401910a0`）与 `fn_140002824`（`0x1401ad490`）。不要从光荣其他作的攻城表抄数值。
