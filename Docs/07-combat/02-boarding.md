# 接舷、白刃与单挑

海战里船贴在一起之后，原版切到另一套 View：船员白刃（`CCombatCrew*`）或提督单挑（`CCombatSingle*`）。这一篇只定义近战规则。炮击与船体见 `01-naval-battle.md`。人物字段不进本系统：人选走 `Fleets.post_holder`，数值走修正器注册表 `Modifiers.eval(key, ctx)`（技能值单位）。

## 1. 职责边界

本系统管：

- 接舷会话：谁登谁的船、两侧参战人物、两侧「战力条」
- 甲板白刃与箱庭白刃两种布局
- 提督单挑（决斗）的触发与结算
- 近战导致的水手增减、投降、擒获船、拖船与货舱分配（经事件交出）
- 接舷演出用的 Sylphid 动画句柄（只发「播哪一段」，不解释 dk4 资源）

本系统不管：

- 炮弹、射界、城炮（`01-naval-battle.md`）
- NPC 何时选择接舷而不是开炮（`03-battle-ai.md`）
- 冲锋队长 / 水兵室 / 撞角的安装（`ships/` 只读）
- 把俘虏写进名册的规则细节（`characters` 订阅 `CrewCaptured`）
- 市价、经验、UI 文案

撞角命中（「撞角撞上了敌舰」`0xf677`）是海战命令的结果，本系统接收 `BoardingRequested`，不自己找碰撞几何。

## 2. 原版事实

### 2.1 类与画面

| 类 | 继承 | 体积（dtor `fn_14000750e`） | 作用 |
| --- | --- | --- | --- |
| `CCombatCrew` | `CView` | 0x4c8 | 白刃会话基类 |
| `CCombatCrewBox` | `CCombatCrew` | 0x4f0 | 箱庭布局；ctor 写 `+0x4d0 = 2` |
| `CCombatCrewDeck` | `CCombatCrew` | 0x6f8 | 甲板布局；另有 20+20 个指针槽 |
| `CCombatCrewScreen` | `CCombatScreen` | 0x210 | 接舷 HUD |
| `CCombatCrewSylphid` | `CSylphid` | 0x20 | 接舷角色动画 |
| `CCombatSingle` | `CView` | 0xc8 | 单挑会话 |
| `CCombatSingleScreen` | `CCombatScreen` | 0x348 | 单挑 HUD |
| `CCombatMesSprite` | `CSurfaceSprite` | 0x148 | 战斗气泡字 |

来源：`class_hierarchy.txt`、`combat/*.cpp`。`CSylphid` 是引擎 2D 角色播放器（广场也有 `CPlazaSylphid`），不是玩法对象。

### 2.2 从海战 tick 切进来

`fn_14000660e`（`0x1401bb5b0`，`exe_caps.md` 写明的战斗结算）RTC：`sCombatCrewResult` 12 字节、`sPlayer` / `sEnemy` 各 24 字节、`cpt`、`pos`、`posPlayerFleet`。下文 `[确认]` 均已对原始指令。

`fn_1400052ef` 找出一对接触的船（`s58/s60` 一侧，`s88/s90` 另一侧，`s68/s6C`、`s98/s9C` 是两侧的标志字），之后：

1. **幽灵船**：敌船 `vfunc+0x18`（**船模 id**，不是船态）`== 0x26`（船模 38 = 幽灵船，`05-ships-fleet/05` 船模表）：`fn_140011121(场景矩形, rand(8), rand(4)+1)` 算新坐标，`fn_1400144fc` 把幽灵船挪过去，return。`[确认]` 指令；「瞬移」是 `[推断]`
2. **海怪冲撞**：`fn_140009df4(敌船)` 为真时（海怪判定 `[推断]`）：
   - 船模 `0x24`（36 鲨鱼）且 `rand(3) == 0`：`fn_1400051cd(己船)` 挑一名船员，弹对白 `0x140560048`
   - `dmg = (rand(3) + 3) / 2`（1–2；有符号 /2）
   - 暴击率按船模：`0x22`（34 怪鱼）→50、`0x23`（35 大墨鱼）→40、`0x24`（36 鲨鱼）→10、其它（37 白鲸等）→100；`rate/10 > rand(100)` 则 `dmg += dmg/2`（打的是**己船耐久** `vfunc+0x50`，不是水手）
   - `dmg = min(dmg, hull)`，扣己船耐久；耐久 0 走沉没 `fn_14000c8dd`

   船模名对照见 `05-ships-fleet/05`（34–38 = 怪鱼 / 大墨鱼 / 鲨鱼 / 白鲸 / 幽灵船）。重构把这些写成**船模表字段**（§6 `ships/models.yaml` 的 `combat` 列），接舷代码不出现 `0x22..0x26`。
3. 否则 `fn_1400085a3` 失败则整段 return；成功则播音效 3，取两船中点
4. `fn_1400078a1(s88)` 取敌方提督 `sF8`；取不到则 `fn_14000ec6e(…,1)` 结束战斗
5. 玩家参与（`s68 == 0 && s6C == 0`）走 §2.4–2.6；否则走 §2.5 的自动肉搏

`s98==2 && s9C==0` 置 `s108 = 1`，它同时决定「特殊擒获」是否可能（§2.6）和白刃画面用 Box 还是 Deck。`s98` / `s9C` 语义 `[未知]`。不要把 Box 写成「室内」、Deck 写成「甲板」当确认。

`fn_1400096ab`（`0x14008ecf0`）`malloc(0x348)` 后调 `CCombatSingleScreen` ctor，这是打开单挑画面。

### 2.3 白刃会话内存

`CCombatCrew` ctor（`0x14007ecf0`）：

- 一串 HUD 坐标（属 UI，不进逻辑表）
- `+0x3f8 = 0x64`、`+0x3fc = 0x64`（两侧基准 100）
- `+0x4b8 = 0xec`
- 循环 `i = 0..19`：`+0x104 + i*4 = 0xec`，`+0x410 + i*8 = 0`（20 个角色槽）

`0xec = 236`。`NUM_PERSON = 235`（合法 id 0–234），236 是「空槽」哨兵 `[推断]`，与 `INVALID_PERSON = -1` 不是同一个值。`fn_140012224` / `fn_14000da26` 也用 `== 0xec` 判「岗位没人」，进一步支持。重构运行时用 `null`（`Option<PersonId>`），236 只在 `ids/legacy_codec.yaml`（D1）；不要用 -1 和 0 混用（0 是拉斐尔）。

`CCombatCrewDeck` 再在 `+0x4c8`、`+0x568` 清 20 个指针。`CCombatCrewBox` 额外 `+0x4d0 = 2`。20 与 `NUM_MOBILESHIP` 同数，但是否共用上限标 `[未知]`。

### 2.4 白刃一击（剑术 × 战力条）

`fn_14000827e`（`0x1400814a0`）`[确认]`，除数已从原始指令补齐：

```
person = lookup(slot[+0x104 + i*4])     # fn_140001861
s28 = Skill(person, 5) * gauge_self / 100      # fn_14000e1ce，eSkill 5 = 剑术；gauge_self = +0x3f8
s2C = (+0xfc) * gauge_foe / 100                # gauge_foe = +0x3fc
if (+0xf8) != i:
    s2C = s2C * 8 / 10                  # ×0.8（shl 3 再 idiv 10）
diff = |s28 - s2C|                      # fn_140008dff
if diff > rand(70):                     # 无符号比较
    return 0 if s28 > s2C else 1        # 胜方
return 2                                # 本回合不分胜负
```

旧文写的「运气」「×8」都错：第二参 5 走的是技能函数，乘 8 后还要除 10。`+0xfc` / `+0xf8` 名字 `[未知]`（`+0xf8` 像「当前对手槽」）。

### 2.5 接舷水手损失（实时路径）

玩家参与时，每次接触按下面顺序只走一支 `[确认]`：

**(a) 单挑**：双方提督 HP（`fn_14001399e` = `CPerson+0x20` u16，`[确认]`，见 `03-characters/07`）都 `> 30`；`d = |剑术_己 − 剑术_敌| / 100`（无符号除）；`rand(10 − d) == 0` 时开单挑（`fn_1400092dc(ui,0)` → `fn_14000e0a7(己, 敌)` 构造 `CCombatSingle`，返回己方是否胜）：

```
败方水手 -= min(crew, crew * (500 - Skill(败方提督, 1)) / 500)   # eSkill 1 = 统率力
fn_140009467(败方舰队, 10)                                         # 舰队疲劳 +10，夹 [0,100]（CFleet+0x71，06-ocean/02 [确认]）
```

这是 `exe_caps.md` 里 `500 - a` 的真实出处：**统率**减轻单挑败北的水手损失。`a` 是技能值，已被 `fn_1401cbb40` 夹在 `[0,500]`，不会出现负数。

**(b) 白刃画面**（`rand(10) == 0`）：`s108` 为真构造 `CCombatCrewBox`，否则 `CCombatCrewDeck`，传敌提督 id；返回 12 字节 `{a, b, duel_person}`：

```
敌损 = a * 5;  己损 = b * 5
if duel_person != 0xec:                     # 画面里发生了单挑
    if fn_14000e0a7(duel_person, 敌提督): 敌损 += 敌crew*(500-统率_敌)/500
    else:                                  己损 += 己crew*(500-统率_己)/500
各自 min(损, crew) 后写回
```

**(c) 其余 8/10 的接触——自动肉搏**（玩家不参与时只走这一支）：

```
fat(fleet) = fn_14000ee12(fleet)                   # 舰队疲劳 CFleet+0x71（0–100）[确认]
己伤敌 = P(己)                                      # 玩家参与：fn_1401ab6a0，见下
                                                   # 否则：p = (s68 == 0) ? 己船船长 : 己舰队提督；Skill(p,5)*(200-fat)/2000 + rand(3) + 1
if 己船 CShip+0x53 == 8: 己伤敌 += 己伤敌 / 5       # 船首像 8（王者像）×1.2；只加己方，敌方不查
敌伤己 = Skill(敌提督, 5) * (200 - fat(敌)) / 2000 + rand(3) + 1
己伤敌 /= 2;  敌伤己 /= 2
两边水手都 > 0 时各扣 min(伤, crew)
敌 crew == 0 → 敌船态 +0x2a = 4（被擒），弹被擒流程
```

玩家侧 `fn_1401ab6a0`（`0x1401ab6a0`）：`fn_14000dbac(ship, 7, out)` 扫 31 舱位、收集**舱室种类 7（水兵室）**里的人（`0xEC` 跳过；这里的 7 是舱室种类，不经 `fn_140007e19`），取 HP `> 0` 者剑术最高 `b1`、次高 `b2`；`f = fn_1401a8730(2, 5)`（`[未知]`）：

```
水兵室有人: P = b1*(200-fat)/1200 + rand(3) + 1 + (b2 + f)*(200-fat)/2000
无:     P = 剑术(提督)*(200-fat)/2000 + rand(3) + 1 + f*(200-fat)/2000
```

水兵室里的人 = 岗位 8 冲锋队长 `[确认]`：`fn_140007e19(8) = 7`、`fn_140012dd2(水兵室) = 8`、岗位名表第 8 项「冲锋队长」（`03-characters/03`）；可多人对应「最多可以设４个水兵室」（`0xc503`）。旧稿「岗位 7 = 冲锋队长」把舱室种类当成了岗位号。

### 2.6 特殊擒获（不是单挑）

`s108 != 0` 时、在单挑判定之前 `[确认]`：

```
p = 玩家岗位 7 上的人（fn_140007e19(7) = 舱室种类 1 了望台；fn_140012e22 取人；fn_14000d6ca(p) 须为真）
x = fn_1400135cf(p, 6, 1)               # 命中力技能（带舱室参数）
if x > 150 and rand(2000) <= x / 10:    # 0x96 / 0x7d0 / 0xa
    敌船态 = 4                          # 直接被擒
    fn_14018b940(2, 250, 0)；结束战斗（fn_14000ec6e / fn_1400041f1）
```

旧文把它当成「单挑触发」，并把 6 当成越界六维——6 是 eSkill 命中力。岗位 7 = **了望手** `[确认]`（岗位名表第 7 项 + `fn_140007e19(7) = 1` 了望台）：原版用了望手的**命中力**判特殊擒获，与了望手的主技能（观察力）不一致，但指令如此；重构保留为 `instant_capture.holder_post` / `skill` 两个旋钮。

### 2.7 舰队级近战（无画面）

`fn_14000d7ab`（`0x140123d30`）`[确认]`。不开 View，NPC 对打与玩家不看画面的遭遇走这里。

**准备**：每侧 `fn_1400094d0` 建 5 个 `CBattleShip` 槽（`01` §2.3）和提督快照：`+0xa8` 命中力、`+0xac` 驾驶术、`+0xb0` 剑术（`fn_14000e1ce` 6/2/5），`+0xb4` 运气原值（`fn_1400090ca(…,5)`）。

**开场**：某侧任一船船首像（`CShip+0x53`，`fn_140010c71(fleet,i)` 取船后 `vfunc+0x48`）`== 0xa` 时，**对侧**每船水手 `-= crew/10`，并对对侧提督调 `fn_14000342c(…, 5)`（精神状态 setter，`03-characters/03`；5 = 混乱 `[推断]`）。

**船首像 0xa 的入口一共两处，都是「开战时」，不是接舷开场** `[确认]`（全 exe 搜 `call [rax+0x48]; cmp eax,0xa` 只有这三条）：

| 入口 | 位置 | 谁带像 → 谁挨 | 附加条件 |
| --- | --- | --- | --- |
| 无画面结算开场 | `fn_14000d7ab` `0x140123df6` / `0x140123e6c` | 任一侧 → 对侧（对称） | 无 |
| 实时海战入场 | `fn_14000974b`（body `0x14018e8d0`，把双方绑进战斗场景，`06-ocean/06`）`0x14018ed91` | 场上 20 艘 `eMobileShip` 里满足 `fn_140014bd2` 的船（`[推断]` 玩家舰队）带像 → 移动舰队 1..3 每船 `crew -= crew/10`（`fn_140014943`，有符号 idiv）、各提督 `fn_14000342c(…, 5)` | 场上**没有**船模 `>= 0x22`（怪鱼 / 大墨鱼 / 鲨鱼 / 白鲸 / 幽灵船，`fn_140002d15`）；另有 `fn_14000f26d(scene,3)==0` 且 `rand(5)==0` 时每艘挨像船再调 `fn_140001a5f(ship,0,1)`（效果未拆） |

接舷 `fn_14000660e` 只比较 `== 8`（`0x1401bc37e`，王者像），**不**查 `0xa`。重构：修正器 `combat.engage_fear_crew_div` 在「开战」时机读取（实时入场与无画面开场两处同一 key），不挂在接舷。实时入场的「场上无怪物船」门写成 `engage_fear.realtime_skip_if_monster_present: true`（`formulas/combat.yaml` `offscreen` 段旁的 `engage_fear:`），`rand(5)` 附加效果待拆后再入表。

**先手**：统率力（eSkill 1）高者先；平手归进攻侧。

**回合**（结果 `r = 3` 表示未决，循环直到 `r != 3`）：

```
炮击段: for i in 0..4, for 行动方 s in (先手, 后手):
    atk = side[s].slot[i]; 须 alive
    t = random_alive_slot(side[1-s])           # fn_14001484e，6 = 无
    hit = rand(500) <= max(0, 命中力[s] - side[1-s].slot[t].c - 驾驶[1-s]/5)
    if hit:
        dmg = (炮种威力 * (3*n_battery+1)) * (运气[s] + 500) / 500     # fn_14000826f
        tgt.hull -= dmg；经验(s) 20、经验(1-s) 10
        if tgt.hull <= 0: 沉没（fn_140005470）；经验各 50；t == 0（旗舰）→ r = s
    else: 经验(s) 5
接舷段: 同样双循环
    board = rand(200) <= max(0, atk.c + 驾驶[s]/5 - tgt.c/2 - 驾驶[1-s]/10)
    if board:
        d1 = (剑术[s]/8 - fat(s)/10)   * (运气[s]+500)/500;   if atk 船首像==8: d1 += d1/5
        d2 = (剑术[1-s]/8 - fat(1-s)/10) * (运气[1-s]+500)/500; if tgt 船首像==8: d2 += d2/5
        tgt.crew -= d1; atk.crew -= d2；经验各 15
        captured = tgt.crew <= 0
                or (t != 0 and tgt.hull < maxhull(tgt)/2 and tgt.crew < need(tgt)
                    and rand(100) < (need - crew) * 50 / need)
        if captured: fn_140009ad9；经验各 50；t == 0 → r = s
        if atk.crew <= 0 and i == 0: r = 1 - s
回合末: 后手旗舰 hull<=30 或 crew<=30 → r = 先手、退走标记；否则先手旗舰同条件 → r = 后手、退走标记
```

- `slot.c` = `fn_140004610(ship)/8`（四项船属性之和，`[未知]` 哪四项）
- `maxhull` = `fn_1400084db(ship)`（单参）：`(改装2,3 个数) * 表0x1406876d0[size_class] + (船首像==3 ? 10 : 0) + fn_14000e129` `[推断]` 为最大耐久；派生量唯一计算方 Ships（D6），战斗只读 `Ships.max_hull`
- `need` = `fn_14000a466`：每桅 {3,5,10}[grade] + 划桨室 40 + 武装炮台 20/间 + 炮改装 2… `[推断，强]` 为必要水手（`06-ocean` / `05-ships-fleet/06`）；战斗只读 `Ships.crew_needed`
- 「经验」= `fn_1400045d9(side, n)` → `fn_1400026d5(提督, n, 1)` `[推断]`
- 运气以 `(luck+500)/500` 放大，不是 `500 - a`

**结算**：胜方取败方 `fn_1400084cc` 的金额，经 `fn_14000ff88` / `fn_140002cb6` 入账（玩家时弹 `0x140560860` 文案；`fn_140002cb6` 封 [0, 99,999,999]，`02-factions`）；`fn_140008c42` 处理擒获船；两舰队按剩余船数 `fn_14001249f > 0` 分别走 `fn_1400054a2` 或 `fn_14000a9ed`（舰队覆灭；玩家本队 → Game Over，见 `01` §2.15）。细节属 `06-prizes-capture.md`。

### 2.8 投降、擒获、占领

文案（`mesfile_sc.txt`）：

- 船：俘虏 / 夺取 / 劫获 / 举白旗 / 占领敌人的 `%s` / `%s号被占领了`
- 人：大量「我们投降吧」提督对白（`0xed47`–`0xefbf`）
- 城：`%s城市投降了`（占城，不是接舷）

`fn_140005fb0`（`0x1401e2e70`，RTC `eCaputureShip` / `asShipLoad` / `apInTowShip`）：

- 槽 0..4，船态 `+0x2a == 4`（`fn_1400106a4`）视为被擒可拖 `[确认]`
- `n += fn_140006b95(ship, 10)`：`[确认]` 是**舱室种类 10 的个数**，不是金币；`n` 传给分货窗口 `fn_1400034ae` 当第 3 参（种类 10 = 货舱，见 `06` §2.3）
- 货舱拷进 `asShipLoad`（200 字节 = 5 船 × 5 货格量级 `[推断]`）
- 拖船指针进 `apInTowShip`
- 打开 `CCapturedLoadSprite`（`ship/CCapturedLoadSprite.cpp`）
- 消息 id `0x143`

「从擒获的船上得到%s金币」（`0x3363`）是这条 UI 的文案。战斗交付的是 `gold` + `cargo slots` + `ship_id`，不是改市价。

有没有「把敌方水手写进 98 人名簿」的代码，本批 cpp 没读到。职文只强调冲锋队长加肉搏、水兵室最多 4。俘虏船员标 `[未知]`，接口先留 `CrewCaptured`，人物系统可以 no-op。

### 2.9 岗位与改装对近战的加成

只作为查询，不在本系统改船舱：

| 来源 | 效果（文案） |
| --- | --- |
| 冲锋队长 | 提高肉搏攻击 |
| 水兵室 ×≤4 | 水手容量 |
| 撞角 | 允许碰撞进攻，切接舷 |
| 项羽头盔等道具 | 「提高本队士气」——人物 modifier，不是接舷条 |

### 2.10 演出

`CCombatCrewSylphid` dtor 里有引用计数 `+0x18/+0x1c`，播完调 `fn_140004539`。逻辑层只发 `PlayBoardingAnim { clip_id, person_id }`。`CDamageDisplay` / `CDamageAnimeSprite` 是数字弹出，体积 0x10 / 0x100，无公式。

## 3. 实体与 ID

| 实体 | ID | 原版 | 空值 |
| --- | --- | --- | --- |
| 接舷布局 | `boarding_layout_id` | Box / Deck 两套 ctor | |
| 单挑会话 | `duel_id` | 运行时 | |
| 白刃槽 | `boarding_slot` | 20 | `null`（原版 `0xEC`，只在 `ids/legacy_codec.yaml`） |
| 擒获槽 | `capture_ship_id` | 5 | 与 `fleet_ship_id` 同宽 |

战果记录槽（`eBattleResult`）属 factions，不是本系统实体（`01` §2.12）。

`eCloseCombatTypeToIndex` 出现在 `fn_14001478b`（`0x1401a8e40`），但该函数在加载 `ship.dk4` 贴图，是近战船图索引，不是规则枚举。不要当成第三种白刃。

## 4. 算法 / 公式

### 4.1 白刃一击

```
power_self = Modifiers.eval("skill.fencing", {person}) * gauge_self / gauge_scale
power_foe  = slot_weight_foe * gauge_foe / gauge_scale
if not facing_flag: power_foe = power_foe * off_target_num / off_target_den   # vanilla 8/10
delta = abs(power_self - power_foe)
if delta > rand(hit_threshold): winner = self if power_self > power_foe else foe
else: no_decision
```

### 4.2 实时接舷（三选一）

```
if hp(a) > duel_min_alive and hp(b) > duel_min_alive
   and rand(duel_spread - udiv(|fencing(a) - fencing(b)|, duel_diff_div)) == 0:   # rand(n<=0) = 0 → 必开
    loser = Duel.resolve(a, b)
    loser.crew -= min(crew, crew * (skill_ref - clamp(leadership(loser), 0, skill_ref)) / skill_ref)
    Fleets.add_fatigue(loser.fleet, duel_loser_fatigue)      # 原版 +10，夹 [0,100]
elif rand(crew_view_roll) == 0:
    {a, b, duel} = BoardingView.run(layout)        # UI 返回双方「击倒数」
    foe_loss = a * crew_view_mul; own_loss = b * crew_view_mul (+ 同上 duel 损失)
else:
    dmg(side) = fencing(holder(side)) * (fatigue_ref - fatigue(side)) / melee_div + rand(melee_jitter) + 1
              # 玩家侧：holder = 水兵室（role marine_room）里 HP>0 者，最高 b1 用 /chief_best_div，见 §2.5
    dmg(own) = Modifiers.eval("combat.melee_damage", {ship: own_ship, base: dmg(own)})   # 原版王者像 ×6/5，仅己方
    dmg /= 2；双方同时扣
    foe.crew == 0 → state = captured
```

### 4.3 特殊擒获

```
p = Fleets.post_holder(player_fleet, instant_capture.holder_post)      # 原版 vanilla:lookout（岗位 7）
x = p ? Modifiers.eval("skill." + key_name(instant_capture.skill), {person: p, cabin: crows_nest}) : 0   # 原版 gunnery；cabin = posts[lookout].room
# key_name 去包前缀（vanilla:gunnery → gunnery）；修正器 key 形如 skill.gunnery（14 README：refs 写 vanilla:accounting，eval 用 skill.accounting）
if layout_flag and x > instant_capture_min and rand(instant_capture_roll) <= x / instant_capture_div:
    foe.state = captured; end battle
```

### 4.4 无画面结算

见 §2.7 伪码。命中、接舷成功、伤害、擒获概率的全部常数进 `offscreen:` 段。

```yaml
# formulas/combat.yaml  boarding: / offscreen:   全部 [确认]
boarding:
  impl: vanilla.boarding
  int_mode: vanilla_i32
  quirks:
    duel_diff_unsigned_div: true   # |Δ剑术|/100 无符号
    melee_bonus_own_side_only: true   # 王者像只加己方（fn_14000660e）
  params:
    gauge_init: 100
    gauge_scale: 100
    off_target_num: 8
    off_target_den: 10
    hit_threshold: 70
    duel_min_alive: 30
    duel_spread: 10
    duel_diff_div: 100
    skill_ref: $caps.skill_value_cap   # 原版 500；(500 - 统率)/500，与技能值上限同源
    duel_loser_fatigue: 10        # fn_140009467(败方舰队, 10)
    crew_view_roll: 10
    crew_view_mul: 5
    fatigue_ref: 200
    melee_div: 2000
    chief_best_div: 1200          # 玩家水兵室最高剑术 b1；次高 b2 用 melee_div
    melee_jitter: 3
    monster_ram: { base_rand: 3, base_add: 3, base_div: 2, crit_div: 10, crit_bonus_pct: 50 }   # 暴击率在船模行 combat.ram_crit_rate
    boarding_slots: 20            # 原为文件顶层键，并入接舷段
  tables:
    melee_holder: { player: { cabin_role: marine_room, top: 2, filter: hp_gt_0 }, other: [captain_if_s68_0, admiral] }
    instant_capture: { holder_post: vanilla:lookout, skill: vanilla:gunnery, min: 150, roll: 2000, div: 10 }   # 原版岗位 7 了望手的命中力
offscreen:
  impl: vanilla.offscreen
  int_mode: vanilla_i32
  params:
    gun_roll: 500
    helm_def_div: 5
    board_roll: 200
    helm_atk_div: 5
    slot_c_def_div: 2
    helm_def_div2: 10
    fencing_div: 8
    fatigue_div: 10
    luck_ref: 500                 # x * (luck + 500) / 500
    capture_hull_frac: 2          # hull < maxhull/2
    capture_pct_mul: 50           # (need-crew)*50/need
    defeat_hull_or_crew: 30
    # 开场威慑：由船首像 effects 贡献 combat.engage_fear_crew_div（原版 10），不在这里写 0xa
  tables:
    exp: { gun_hit: [20, 10], sink: [50, 50], miss: 5, board: [15, 15], capture: [50, 50] }
    admiral_inputs:               # 提督快照 +0xa8..+0xb4：技能值单位，运气为六维原值
      gunnery: skill.gunnery
      helm: skill.helmsmanship
      fencing: skill.fencing
      luck: ability.luck
engage_fear:                    # 船首像 0xa 开战威慑（§2.7 表）；除数本身由 combat.engage_fear_crew_div 修正器给，原版 10
  impl: vanilla.engage_fear
  int_mode: vanilla_i32                   # crew -= crew/div 有符号 idiv（fn_140014943）
  params:
    realtime_skip_if_monster_present: true  # 实时入场：场上有船模 >= 0x22 则不触发（fn_140002d15）
    realtime_extra_roll: null               # fn_14000f26d(scene,3)==0 且 rand(5)==0 → fn_140001a5f(ship,0,1)；效果未拆，unverified
  tables:
    at: [realtime_engage, offscreen_open]   # fn_14000974b 入场 / fn_14000d7ab 开场；接舷不触发
  refs:
    admiral_mental_state: vanilla:confused  # fn_14000342c(…, 5)；5 = 混乱 [推断]
  modifiers: combat.engage_fear_crew_div
```

`skill_ref` 直接引用 `caps.skill_value_cap`（`fn_1401cbb40` 夹 500），不另存 500。空人物 `0xEC` 只在 `ids/legacy_codec.yaml`。

### 4.5 投降 / 擒获

```
if crew <= 0: state = captured                     # 写 +0x2a = 4
if captured: emit ShipCaptured { ship_id }         # 金币 / 货另算，见 06
```

船态 4 = 被擒 `[推断]`：`fn_14000660e` 在敌水手为 0 时写 4，`fn_140005fb0` 只收 4。名字未恢复前用整数状态机，状态表进 `combat/boarding.yaml`。

## 5. 耦合分析

| 原版纠缠 | 重构 |
| --- | --- |
| 技能 `fn_14000e1ce(person, 1/2/5/6)`、六维 `fn_1400090ca(person, 5)` | `Modifiers.eval("skill.<key>" / "ability.luck", ctx)`（D2） |
| 舱室种类 7（水兵室）、岗位 7（了望手）硬编码；`fn_14000ee12` 舰队疲劳 | `Ships.occupants(ship, role)`、`Fleets.post_holder(fleet, post_key)`、`Fleets.fatigue(fleet)` |
| 海怪船模 id `0x22..0x26` 写死在接舷函数里 | 船模表字段 `combat.on_contact: teleport | ram` 与 `ram_crit_rate`（§6） |
| 船首像 `== 8 / == 0xa` | `figureheads.yaml` `effects` → `combat.melee_damage` / `combat.engage_fear_crew_div` |
| `500-a` 与技能值上限 | `skill_ref` 与 `caps.skill_value_cap` 绑定 |
| 船 `vfunc+0x50/0x58/0x48` | 会话快照 `hull/crew/state` |
| `COceanScene` 直接 new 接舷 View | Ocean 发 `BoardingRequested`；UI 订 `BoardingOpened` |
| `CCapturedLoadSprite` 在 ship 功能目录 | 战斗发事件，货舱 UI 属 ships/ui |
| 98 人名簿 | 本系统不写；最多发 `CrewCaptured { person_id }` |
| Sylphid / 坐标 | `ui/` |

接舷不得调用 `Economy.quote` / 写 `market_price`。

## 6. 解耦后的配置

### `config/combat/boarding.yaml`

```yaml
schema: dk4.combat.boarding
version: 1
layouts:
  - key: vanilla:box
    vanilla_index: 0
    ctor: CCombatCrewBox
    extra: { field_4d0: 2 }
  - key: vanilla:deck
    vanilla_index: 1
    ctor: CCombatCrewDeck
slots: 20
side_gauge_init: 100
capture_slots: 5
ship_state_towable: 4              # [推断]
duel:
  screen: CCombatSingleScreen
  session: CCombatSingle
```

### 船模表的接舷行为（所有者 `05-ships-fleet/05`，战斗只读）

```yaml
# config/ships/models.yaml 片段；原版值 [确认]（fn_14000660e 0x1401bb840 一带）
- { key: vanilla:monster_fish, combat: { on_contact: ram, ram_crit_rate: 50 } }                          # 船模 34
- { key: vanilla:giant_squid,  combat: { on_contact: ram, ram_crit_rate: 40 } }                          # 35
- { key: vanilla:shark,        combat: { on_contact: ram, ram_crit_rate: 10, ram_taunt_roll: 3 } }       # 36；rand(3)==0 挑船员说话
- { key: vanilla:white_whale,  combat: { on_contact: ram, ram_crit_rate: 100 } }                         # 37（「其它」分支）
- { key: vanilla:ghost_ship,   combat: { on_contact: teleport, teleport: { rand_a: 8, rand_b: 4, add_b: 1 } } }  # 38
```

「是否海怪」原版是 `fn_140009df4(ship)`（`[推断]`）；重构直接看 `combat.on_contact`，没有该字段的船走普通接舷。

### `config/formulas/combat.yaml`（接舷段）

见 §4.4 的 `boarding:` / `offscreen:`。人选与技能 key 都是数据（`melee_holder`、`instant_capture`），引擎不 if 技能名。

## 7. 独立旋钮

- `slots`、`gauge_init`、`hit_threshold`、`off_target_*`
- `skill_ref`（与 `caps.skill_value_cap` 一起改）
- 是否允许单挑、`duel_spread`、`crew_view_roll`（白刃画面出现率）
- `instant_capture_*`（可整段关掉）
- `offscreen.*`：NPC 对打的命中/接舷/擒获手感，与实时海战独立
- 擒获是否自动拖船、货舱是否可分到已有五艘以外

加一种白刃布局：只加 `layouts[]` 和 UI 场景，不改炮击。

## 8. 对外接口

```
Combat.request_boarding(battle_id, from_ship, to_ship, layout_id)
Boarding.tick(boarding_id) -> { winner_slot | continue | duel }
Duel.resolve(person_id_a, person_id_b) -> side
```

事件：

```
BoardingOpened    { battle_id, layout_id, side_a: [person_id], side_b: [person_id] }
BoardingHit       { boarding_id, slot, damage_or_flag }
DuelOpened        { battle_id, person_id_a, person_id_b }
DuelResolved      { battle_id, winner_person_id }
ShipCaptured      { battle_id, ship_id }            # 金币 / 货由 06 的 CaptureSession 补
# 水手减少不另设事件：ships 发 CrewChanged { ship_id, crew, reason: duel|melee|view }；会话内 UI 用 ShipCrewChanged
CrewCaptured      { battle_id, person_id }          # 人物系统可忽略
BoardingResolved  { battle_id, result_id }          # 再汇总进 BattleResolved
```

查询：`Modifiers.eval`、`Fleets.post_holder`、`Fleets.fatigue`、`Ships.occupants(ship, role)`、`Ships.has_add_part(ram)`、`Ships.crew(ship_id)`、`Ships.ship_type(ship_id).combat`。

## 9. 未核实字段

| 项 | 事实 | 不要写成 |
| --- | --- | --- |
| Box vs Deck 的玩法差 | 两套 ctor；`s98==2 && s9C==0` 选 Box | 「舱内战 / 甲板战」定论 |
| `CCombatCrew +0xf8,+0xfc,+0x100` | 参与 ×0.8 分支 | 士气 |
| `fn_14000ee12` = 舰队 `+0x71` 字节 | `[确认]` 疲劳（`06-ocean/02`：`fn_140009467` 夹 0–100、`posTired`） | — |
| `fn_14000283d` 单挑公式 | 只见调用 | 回合制伤害表 |
| `fn_1401a8730(2, 5)` | 玩家肉搏加项 | 「参谋剑术」定论 |
| `fn_14001399e` | `[确认]` = `CPerson+0x20` u16 = HP | — |
| `vfunc+0x48` 值 3/4/8/0xa | `[确认]` 是 `CShip+0x53` 船首像；3 白鲸 +10 耐久、4 龙 +2 炮威力、8 王者 肉搏 ×1.2 `[推断，强]`；0xa 开战（实时入场 + 无画面开场，§2.7）对侧 −10% 水手（道具 199？`[推断，弱]`） | 船态 |
| `fn_140009467(fleet, 10)` | `[确认]` 单挑败方舰队疲劳 +10（夹 0–100） | 士气 −10 |
| `eCloseCombatTypeToIndex` | 加载 `ship.dk4` | 第三种近战 |
| 俘虏水手入队 | 无代码 | 自动招人 |
| 20 槽是否等于 20 个 mobile ship | 同数 | 同一上限 |
| 船模 0x22/23/24/26；船态 4/7/8/0xb | 船模名已对上（`05-ships-fleet/05`）；船态名未恢复 | 把船模当船态 |
| `sCombatCrewResult` 12 字节 | RTC 大小 | 三场比分 |

`fn_14000660e`、`fn_14000827e`、`fn_1401ab6a0`、`fn_14000d7ab` 的算术已按原始指令补齐（§2.2–2.7）。剩下的空白是 `CCombatSingle` 内部（`fn_14000283d`）与白刃画面如何产生 `{a, b}` 两个击倒数。
