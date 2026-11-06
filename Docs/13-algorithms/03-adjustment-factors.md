# 调节因子总表（等级 · 视野 · 补给）

三个跨系统标量：人物**总等级**、舰队**搜索半径**、船上**水/粮库存**。本篇是资深清单「调节因子」的收口；等级公式本身不重写，见 [11-progression/01-level-exp-skills.md](../11-progression/01-level-exp-skills.md) 与 [01-formulas.md](01-formulas.md) `F-PROG-*`。

人物 / 船 / 道具只向修正器注册表登记贡献，消费方按 key 取 `Modifiers.eval(key, ctx)`（[DECISIONS D2](../00-overview/05-config-architecture.md)）。业务系统禁止再读六维下标、禁止再读 `CShip*`。

## 1. 职责边界

本系统管：

- 把总等级当成**输入因子**喂给其它公式（技能封顶、修理折扣、战斗加成……）
- 搜索半径怎么从了望手 / 观察力 / 了望台 / 船字段算出来
- 港口、村落、遗迹、道具、NPC 舰队各自用哪一套「看见」判定
- 水、粮两种补给的库存、日耗、短缺，以及耗尽时写港口哨兵 `0x62`
- 大厨 / 饲养员 / 物资仓库 / 船首像 / happening 持续态怎么改消耗
- 对应 YAML：`config/formulas/vision.yaml`、`config/formulas/supply.yaml`、`config/modifiers.yaml`（key 片段）；等级参数仍在 `formulas/progression.yaml`

不管：

- `level = isqrt(exp / 30)` 的实现与溢出（progression 拥有）
- 航速主公式、风力档（[06-ocean/01-sailing.md](../06-ocean/01-sailing.md)）
- 探索掷骰表、遭遇权重（[06-ocean/02-search-encounters.md](../06-ocean/02-search-encounters.md)）
- 舱室怎么摆在船上（[05-ships-fleet/03-cabins-crew-posts.md](../05-ships-fleet/03-cabins-crew-posts.md)）；本篇只引用 `cabin_kind → post`）
- 港口补给会话的金币（[05-ships-fleet/04-remodel-guns-supplies.md](../05-ships-fleet/04-remodel-guns-supplies.md)）
- `CSearchCircleSolidSprite` 的 Direct3D 顶点格式（投影，不是规则）
- 迷雾 / 黑块地图（world）

## 2. 原版事实

### 2.1 等级当作因子

等级公式、两轨、技能 `min(total_level, 200)`：**不要在这里复述**，见 progression。本篇只登记**谁读等级**。

总等级取值函数 `fn_1400123dc`（body `0x1401c9230`）。技能主式 `fn_140009214`（body `0x1401c9d50`）：

```
level = fn_1400123dc(person)
if level > 0xC8: level = 0xC8          # skill_level_cap=200
skill = (stat_a + stat_b) * (level + 0x64) / 0x96   # +100 / 150
return clamp(skill, 0, 0x1F4)          # 500
```

来源：`fn_140009214` 反汇编；与 `Save.cpp` / `dk4save.c` 一致。`[确认]`

`fn_14000e1ce` → `fn_1400135cf` 在此基础上再加装备（`fn_14000e0b6`，5 槽）并按人物 `+0x0D` 身心档打折（`fn_1401c87a0`：1–2 ×19/20、3 ÷2、4 ÷10、5 ×3/5），最后再夹 500。`[确认]` 全式见 [01-formulas.md](01-formulas.md) F-PROG-09；技能↔两维表见 F-PROG-05。

岗位 → 舱室（`fn_140007e19`，body `0x140068f70`，`post ∈ [0,15]` 跳转表，越界返回 `0x18=无`）。`[确认]`

| post | 岗位 | cabin | 舱室 |
| ---: | --- | ---: | --- |
| 0 | 舰长 | 5 | 舰长室 |
| 1 | 副官 | 6 | 副官室 |
| 2 | 参谋 | 19 | 参谋室 |
| 3 | 会计长 | 20 | 会计室 |
| 4 | 测量员 | 3 | 测量 |
| 5 | 操帆手 | 0 | 桅杆 |
| 6 | 舵手 | 4 | 船舵 |
| 7 | 了望手 | 1 | 了望台 |
| 8 | 冲锋队长 | 7 | 水兵室 |
| 9 | 炮手 | 8 | 武装炮台 |
| 10 | 传教士 | 18 | 礼拜室 |
| 11 | 船医 | 14 | 医务室 |
| 12 | 大厨 | 15 | 厨房 |
| 13 | 船木工 | 13 | 木材室 |
| 14 | 饲养员 | 17 | 饲养室 |
| 15 | 甲板主管 | 2 | 甲板 |

舱室字串顺序（桅杆=0 … 无=23）见 [03-abilities-skills-jobs.md](../03-characters/03-abilities-skills-jobs.md)。物资仓库是舱室 9，**不在**这 16 个岗位里。

在岗技能：`fn_1401ac4a0(post, skill)` → `fn_14000da26`：空槽 `0xEC` 返回 0，否则 `Ability(person, skill)`。`[确认]`

### 2.2 视野 / 搜索半径

- `[确认]` `CSearchCircleSolidSprite` ctor 循环 `>= 0x40`（64 顶点），每步给角 `+4`。`64 × 4 = 256`，走满 256 分圆，是**全周**实心圈，不是前向锥。对象 `0x638`。来源：`CSearchCircleSolidSprite.cpp`。`vfunc_07` 绘制时把 `0x40` 交给底层。这是投影。
- `[确认]` 半径由 `fn_14019dd60`（body `0x14019dd60`，RTC `eFleetShip`）算出，交给 `fn_140004322` 建圈。xref 仅三处：建圈 `fn_14000240f`、港口发现 `fn_140007806`、村落发现 `fn_140001186`。
- `[确认]` `fn_14019dd60`：

  ```
  r = 0x20                                      # 32
  r += fn_1401ac4a0(post=7, skill=9) / 10       # 了望手的观察力 / 10
  for ship in 0..4:
      if ship 存在且 vfunc+0x08 真且 vfunc+0x48 == 5:
          r += 0x10                             # +16，第一艘即停
          break
  return r
  ```

  `vfunc+0x48` = `CShip::vfunc_09` = 读 `CShip+0x53`（u8，初始化 `0x0D`）。
- `[推断]` `+0x53` 是 `eFigureHead`，`== 5` 是海豚像（道具 119 = 114+5，文案「看见更多」）。旁证：同一字节在航速里 `==0` 加速 ×1.1（大鹫像 114「飞速前进」）、`==7` ×0.8（魔鬼像 121「不祥」），在补给里 `==1` 粮除数 +2（小猪像 115「减少饭量」），在白刃里 `==8` 伤害 ×1.2（王者像 122「鼓舞水手」）。五处与道具目录顺序全部对上。`0x0D` 像空哨兵（超过 0–9）。不是了望台（舱室 1）。写入点未穷尽，不升确认。
- `[确认]` 了望手道具文案：「如果瞭望手有了它，就会很容易发现宝物和港口。」「可以发现的范围更大。」走观察力 → 半径，不读六维。
- `[确认]` 港口 `fn_140007806`：循环 `eTown < 0x61`，空结果哨兵 `0x62`。显示点 `+= 0x10` 或 `0x18`（`fn_14000c71b == 1` 时 24，否则 16）。未发现：`dist² ≤ (radius + offset)²` 则发现。已发现：用 `offset+0x10` 做更近的「选为入港目标」盒。
- `[确认]` 村落 `fn_140001186`：循环 `eVillage < 0x18`，哨兵 `0x19`。偏移恒 `0x10`。判定同左。
- `[确认]` 主动探索 `fn_14001266b`：扫描 `eItem < 0xE7`，`dist² ≤ radius²` 才算圈内道具。随后进成功率：`rand(100) <= 10*rich + skill(admiral, 9) / 50`（`imul 0xA`、`idiv 0x32` @ `0x1401b5201`；`admiral = fn_140004e17()` 提督，**不是**了望手；`rich = fn_14000f218(lat, lon)` 经纬矩形富度 0–4）。全流程见 [06-ocean/02-search-encounters.md](../06-ocean/02-search-encounters.md)。
- `[确认]` 遗迹**不**走 `fn_14019dd60`（该函数只有上述 3 个调用方）。遗迹是探索 outcome，见 search 文档。
- `[确认]` NPC 舰队遭遇 `fn_14000cd8d` **不**用搜索半径。`fn_140008530`：屏幕空间 `dx,dy` 都 `< 0x60=96` 才继续，否则跳过。方向名 `fn_1401a9c90` 按 256 分圆八分位取字符串，不是射程。
- `[确认]` 半径公式**不含** `nTime`、天气。昼夜 / 天气不改搜索半径。改补给的是舰队 happening 持续态（§2.3），不是视野，也不是气候 id。

### 2.3 补给

- `[确认]` `eShipSupply` 循环上界 2。`anSupply` RTC 8 字节 = 两个 int32。`NUM_SHIPSUPPLY` 头文件未写整数；运行时就是 2。断言 `0 <= eVal && eVal < NUM_SHIPSUPPLY`。
- `[推断]` 0 = 水、1 = 粮。依据：饲养员「节约粮食」只加在 `kind==1` 的除数上；happening 态 1 把 `kind==1` 的库存 `× 0x5F/0x64`（95%）；文案两种桶。
- `[确认]` 库存在 `CShip+0x46 + kind*2`，u16。读 `fn_140013e1c`，写 `fn_14000f033`（`mov word [ship+kind*2+0x46], ax`，**赋值不是加减**）。
- `[确认]` 物资仓库 = 舱室 kind 9。`fn_140006b95(ship, kind)` 扫 31 槽，`vfunc+0x38 == kind` 则计数。`fn_140012724` = `count(kind==9)`。
- `[确认]` 日耗在 `fn_140005aa6`（body `0x1401dc830`，RTC `anSupplyShortage` / `asSupplyData` / `eShipSupply`）。由 `fn_140005a15` 在舰队当前港 `== 0x62`（海上）时调用。
- `[确认]` `fn_140005a15`（body `0x1400fa870`，4192 字节；玩家舰队日步，日首门 `fn_14000906b` 后经 `fn_14000641f` 调用；内部顺序：补给 → 疲劳 → 人物日结 → happening 掷骰 → 持续态，见 06-ocean/01）RTC：`eNewFleetState`、`asCompanion`、`eFleetShip`、`nLatitude`、`nLongitude`、`anSupply`、`aePerson`、`nShipTotal`、`anSupplyShortage`。港槽不是 `0x62`（在港）时只走在港分支：疲劳恢复 `-(rand(5)+2)`，不耗补给、不掷事件（词表 §14-18，`06-ocean/01` §4.4）。
- `[确认]` 消耗分子是船 `vfunc+0x58` = `CShip::vfunc_11` = `CShip+0x4C` u16。`[推断]` 当前水手人数（HUD 把五船该值加总；吃粮人数）。
- `[确认]` 大厨：`fn_14000da26(post=12, skill=6)`。饲养员：`(post=14, skill=0)`。空岗技能 0。
- `[确认]` 这里读的**不是气候 id**。`fn_14000b87a(fleet)`（body `0x1401dd8f0`）= `movzx byte [CPlayerFleetData+0x30]`，即海上 happening 结局态（初值 / 空闲 `0x1B=27`；同一字节 `== 0x18` 是无风，见 [06-ocean/05-sea-happenings.md](../06-ocean/05-sea-happenings.md) §2.1）。态 `== 1`：粮库存先 `× 0x5F/0x64`（`0x1401dcab5`–`0x1401dcacd`）。`fn_14000ce78`（body `0x1401e0600`）取全局玩家舰队再读同一字节，`2 <= 态 <= 5` 为真 → 水除数 `+10`。结局名表 `.data 0x140686550`（26 项，getter `fn_140014803` body `0x1401dd940`）：态 1 = 有老鼠（粮被吃 5%），态 2–5 = 暴风雨 / 暴风雪 / 雨 / 雪（接雨雪水，水更省）`[确认]` 名（06-ocean 第二轮）。
- `[确认]` `nTime == 0` 的那一拍：`fn_140004101`（海域图）重算 `fn_14000bcc1` 求和，**不在这里扣库存**。扣库存是 `fn_140005aa6` 写回。
- `[确认]` `fn_14000bcc1`：五船两类补给分别求和，返回第一个 `≤ 0` 的 kind；两类都 `> 0` 返回 3，调用方跳过告警。
- `[确认]` 耗尽：`fn_140009f1b` 把 `COceanScene+0xA0` 写成 `0x62`，再切命令。文案「提督，这样下去物资就要没了……。」
- `[确认]` 发现补给 / 初始化：`fn_140008486` 把两类都 **赋成** `warehouse_count / 2`（村落发现路径会调）。比例重分配 `fn_140011ff9`：`water = cap * rate / 100`，`food = cap - water`，`rate` 来自 `CSupplyRateView`。
- `[确认]` 另有菜单路径 `fn_1400f6300` 选项 1：`new = max(0, stock - crew/10)`，两边同样扣。`fn_1400f6300` 是**哗变 outcome 20** 的菜单（镇压 / 说服 / 置之不理，选项 1 = 说服），由 `fn_140005a15` 末尾按 happening 态调用（见 06-ocean/05 §2.8）。不是日耗，与 `005aa6` 的可变除数不是同一条式子。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key |
| --- | --- | --- | --- |
| 总等级 | 两轨之和，缓存 `save+0x70` | 0 合法 | `total_level` |
| 技能 | `eSkill` 10 | -1 | `skill_id`（观察力=9） |
| 岗位 | `ePost` 至少 16 | -1；绑定空人物 `0xEC` | `post_id`（了望手=7） |
| 舱室种类 | `eCabinKind` 24 名 | `0x18=无` | 舱室种类 key（`crows_nest` / `galley` …）+ `role`（`supply_room` / `cargo_room` …）；公式按 role 计数 `Ships.cabin_count(ship, role)` |
| 补给种类 | `eShipSupply` **2** | -1 | `vanilla:water` / `vanilla:food`（下标 0 / 1，顺序 `[推断]`） |
| happening 结局态 | `CPlayerFleetData+0x30` u8，27 空闲 | 27 | `outcome` key；效果走结局表 `effects` |
| 港口空哨兵 | `0x62=98` | 不是 `INVALID_TOWN=-1` | 运行时 `Option<TownId>`；98 只在 `ids/legacy_codec.yaml` |
| 搜索圈顶点 | 64 | — | `search_circle_vertices` |
| 角模 | 256 | — | `angle256` |

运行时循环读表长。原版 2 / 5 / 31 / 64 / 97 只作 vanilla 包实例。岗位 / 技能 / 舱室在配置里写 key（`lookout`、`observation`），原版下标只在注释。

## 4. 算法 / 公式

全部魔数进 YAML。溢出：**饱和**，禁止绕回 0、禁止无符号把负数当成几百万。

### 4.1 等级 → 技能（只引用）

`level_for_skill = min(total_level, skill_level_cap)`，再进 `F-PROG-05`。参数 `skill_level_cap=200`、`level_bias=100`、`skill_divisor=150`。重构饱和到 `caps.skill_level_cap`，禁止原版 32 位 `isqrt` 绕回。

这个夹紧后的整数是技能公式（characters，`formulas/character.yaml`）内的中间量 `min(L0+L1, $caps.skill_level_cap)`，**不**注册为修正器。业务系统若只需「等级因子」读 `Progression.total_level` 再自己 `min`，不要自己 `isqrt`。

### 4.2 等级直接读者（穷尽 `fn_1400123dc` 的 22 处 call）

| 系统 | 函数 | 做什么 | 等级 |
| --- | --- | --- | --- |
| progression / characters | `fn_140009214` | `F-PROG-05` 技能 | `[确认]` |
| characters | `fn_140006c67` / `fn_140008387` | 加经验（夹 `0x493E0=300000`）后按**升了几级**循环成长六维 | `[确认]` |
| ships / town | `fn_140175b00` 修理、`fn_1400103b6` 改造 | 船木工（舱 13）`level >= 25` 才打折：`pct = level / 5`，`cost -= cost * pct / 100`。25→5%，100→20%，200→40% | `[确认]` |
| combat | `fn_140084400`（RTC `anBonus`） | `bonus = level * table[idx] / 100`，表 `{50,40,30,20,10}`。`idx` 来自 `fn_1400844e0` `[未知]` | `[确认]` 式；`[未知]` 下标 |
| items | `fn_1400093f4`（body `0x1401567f0`） | 金额 `(level+1) * 200`（`inc eax; imul eax,eax,0xC8`，`level` 取 `fn_140004e17()` 的人）；委托 `0x45` 状态 == 5 时再加一项。消息 `0xB2C`。用途 `[未知]` | `[确认]` 式 |
| ui | `fn_14000a38f`、`fn_14000dd69`、`fn_14000127b`、`fn_14000141a` | 状态画面 / 六边形 / 存档卡显示 | `[确认]` 投影 |
| items | `fn_140014097`、`fn_140010e10` | 读等级，业务 `[未知]` | `[未知]` |
| economy | `fn_14015ccf0`、`fn_14015d600`（特产品 RTC） | 读等级，是否进供给 `[未知]` | `[未知]` |
| ocean / happening | `fn_140008c33`、`fn_1400ebfd0`、`fn_1401dc310` | 读等级，语义 `[未知]` | `[未知]` |

**没有**任何 call 把总等级直接变成「自动航行天数」。`auto_sail_days` 是设计位：原版驾驶术经 `fn_1401ac4a0(6, 2)`（舵手，驾驶术）进航速，等级只通过技能间接进去。禁止编造「等级=可自动航行日数」。

间接通道（读技能，技能已含等级）：

| 通道 | 调用 | 作用 |
| --- | --- | --- |
| 观察力 | `fn_1401ac4a0(7, 9)` | 搜索半径 |
| 测量术 | `fn_1401ac4a0(4, 3)` | HUD：0 不显示经纬；`≤ 200` 有条件显示；`> 200` 恒显示 |
| 驾驶术 | `fn_1401ac4a0(6, 2)` | 航速 / 操舵（ocean） |
| 大厨 命中力 | `da26(12, 6)` | 补给除数 |
| 饲养员 生命力 | `da26(14, 0)` | 粮食除数 |
| 船木工 总等级 | 直接读 level | 修理 / 改造折扣（上表） |

### 4.3 F-ADJ-01 `search_radius`

```
holder = Fleets.post_holder(fleet, lookout_post)    # 空岗 → 0
obs = holder ? Characters.skill(holder, observation_skill) : 0  # 已含等级与装备
r   = radius_base + obs / obs_div                    # 32 + obs/10，截断
r  += Modifiers.eval("fleet.search_radius_add", fleet)
      # 原版：任一存在船 vfunc+0x48 == 5（[推断] 海豚像）→ +16，第一艘即停（不叠加）
r = saturate(r, 0, search_radius_hard)
```

参数默认：`radius_base=32`，`obs_div=10`。海豚像 +16 进 `ships/figureheads.yaml` `effects: [{key: fleet.search_radius_add, stage: add, value: 16, scope: fleet_any, stack: once}]`（`stack: once` 复刻「第一艘即停」，与 05-ships-fleet/04 一致）。`search_radius_hard` 进 `engine_limits`，禁止 u16 绕回。

圈是 360°。派生量 `Ocean.search_radius(fleet_id)` 就是这个 `r`（按舰队算一次，不按人；D6 唯一计算方 Ocean，不存档）。

### 4.4 F-ADJ-02 `in_spot_range`

```
# 港口 / 村落（世界坐标，原版单位，禁止 /8）
offset = town_undiscovered_offset          # 16；fn_14000c71b==1 时 24
         或 village_offset                 # 16
if dist2(player, point + offset) <= (radius + offset)^2:
    DiscoveryFound

# 圈内道具（主动探索）
if dist2(player, item) <= radius^2: candidate

# NPC 舰队：不用 radius
if abs(dx) < fleet_screen_box and abs(dy) < fleet_screen_box:  # 96
    FleetSpotted
```

发现成功后的掷骰仍走 search 文档，不进本因子。

### 4.5 F-ADJ-03 `supply_drain`

对每艘存在的船、每种补给（原版 2）：

```
divisor = drain_div_base                    # 10
if rand(drain_skill_roll) < cook_skill:     # rand(500) < 大厨技能
    divisor += cook_div_bonus               # +2
if kind == food:
    if rand(drain_skill_roll) < keeper_skill:
        divisor += keeper_food_div_bonus    # +8
    divisor += Modifiers.eval("supply.food_div", fleet)   # 原版：任一船 vfunc+0x48==1（[推断] 小猪像）→ +2
    stock = stock * Modifiers.eval("supply.food_stock_pct", fleet) / 100
            # 原版：happening 态 (CPlayerFleetData+0x30) == 1 有老鼠 → 95；否则 100
if kind == water:
    divisor += Modifiers.eval("supply.water_div", fleet)  # 原版：态 2–5 暴风雨/暴风雪/雨/雪 → +10

need = (crew * drain_numer) / divisor       # crew*10 / divisor
if rand(10) < (need % 10):
    need += 10                              # 余数再掷一次
units = need / 10

stock' = saturate(stock - units, 0, supply_amount_max)
# 跨船分摊未扣完的 units → shortage[kind]
```

原版的「船首像 1」「happening 态 1 / 2–5」是字面量比较；重构由船首像表 `effects` 与 happening 结局表 `effects` 注册修正器贡献（D2/D3），公式只按 key 取值。

`shortage[kind] > 0` 写入 `anSupplyShortage`（两个 int32），并给 `fn_140005a15` 的海上事件强度 `+1`。两类都扣光：告警，入港槽清空（原版写 `0x62`；重构 `None`）。

哗变「说服」扣补给（`fn_1400f6300` 选项 1，`crew / 10`）不是日耗，归 happening 动作（D9 `Action.effects`），本公式不持有。

容量：

```
cap = Ships.cabin_count(ship, supply_room)  # 原版舱室种类 9
fill_each_kind = cap / 2                    # 发现水粮 / 初始化
# 比例：water = cap * supply_rate / 100 ; food = cap - water
```

水手人数 `crew` 来自 fleets 快照。水兵室是否加算船员：`[未知]`，YAML 留 `marine_room_crew_bonus: null`。

Ocean 直接按上式算 `units`（64 位中间值）。不导出「`drain_div_base / divisor` 浮点因子」：大厨 / 饲养员每天各掷一次，因子不是常数，用浮点会和原版取整对不上。

### 4.6 F-ADJ-04 船木工折扣（等级直接）

```
if carpenter_person exists and total_level >= carpenter_level_min:  # 25
    pct = total_level / carpenter_level_div                         # /5
    cost = cost - cost * pct / 100
cost = saturate(cost, 0, gold_hard)
```

修理、改造各调一次。文案「减少%s％」。

### 4.7 溢出

| 量 | 原版 | 重构 |
| --- | --- | --- |
| 单轨等级 `> 255` | `isqrt` 画面变 0 | 饱和到 `level_per_track` |
| `skill` 乘法 | 32 位 | 64 位后再夹 `skill_value_cap`（500） |
| 搜索半径 | u32 局部 | 饱和到 `search_radius_hard` |
| 补给 u16 | 写 word | 饱和 `[0, supply_amount_max]`；缺符号整数则先用 65535 |
| 修理 `cost * pct / 100` | 32 位 `imul`+`div` | 有符号饱和，`cost ≥ 0` |
| `(level+1)*200` 金币 | 32 位 | 64 位再饱和到金币宽 |

## 5. 耦合分析

原版缠在一起：

- 观察力既进探索成功率又进半径；了望手岗位映射写死在 `fn_140007e19`
- `CShip+0x53`（船首像 `[推断]`）被航速（0、7）、半径（5）、补给（1）、白刃（8）各自用字面量判断——语义是同一个 `figurehead_id`，重构把效果收进 `ships/figureheads.yaml` 的 `effects`（修正器贡献 `fleet.search_radius_add`、`fleet.speed`、`supply.food_div`、`combat.melee_damage`），各系统只读效果，不比较 id
- 补给库存、物资仓库计数、水手、大厨技能、happening 结局态、入港哨兵都在 `COceanScene` / 未挂类函数里直接改船字节
- `0x62` 既是「没选港口」又是「补给耗尽停止入港」
- 等级被修理、改造、战斗、道具、UI、特产品同一 `fn_1400123dc` 拉走

重构切断：

- Ocean 自己算派生量 `Ocean.search_radius(fleet_id)`（唯一计算方），只读 `Fleets.sailing_snapshot(fleet_id)` 的 `supplies / crew` 与 `Ships.cabin_count(ship, supply_room)`
- 耗尽只发 `SuppliesDepleted`，由会话把入港槽清成 `None`（不写 98）
- 修理费由 ships 取 `Modifiers.eval("ship.repair_cost", {fleet, base: 报价})`（提供方：船木工岗位按等级登记 `mul (100 - discount)/100`）
- 战斗取 `Modifiers.eval("combat.level_bonus", ctx)`，不读等级原值
- happening 结局态改补给 → 结局表 `effects` 注册 `supply.food_stock_pct` / `supply.water_div`

## 6. 解耦后的配置

```yaml
# config/formulas/vision.yaml（D3 形状：impl / int_mode / quirks / params / tables / golden）
schema: dk4.formulas.vision
version: 2
search_radius:                   # F-ADJ-01
  impl: expr
  int_mode: { div: trunc, width: sat64 }   # 饱和到 engine_limits.search_radius_hard，禁止 u16 绕回
  params:
    radius_base: 32
    obs_div: 10
  refs:
    lookout_post: vanilla:lookout          # 原版 post 7
    observation_skill: vanilla:observation # 原版 eSkill 9
  inputs: [obs]                  # obs = Characters.skill(Fleets.post_holder(fleet_id, lookout_post), observation_skill)；空岗 0
  expr: "radius_base + obs / obs_div"
  modifiers: fleet.search_radius_add       # 海豚像 +16 由 figureheads.yaml 注册；out 含此贡献
  golden:
    - { obs: 0,   figurehead_add: 0,  out: 32 }
    - { obs: 255, figurehead_add: 16, out: 73 }
in_spot_range:                   # F-ADJ-02
  impl: vanilla.in_spot_range
  int_mode: { div: trunc, width: sat64 }
  params:
    circle_vertices: 64
    angle_step: 4                # 64*4=256 全周
    angle_modulus: 256
    village_offset: 16
    town_offset_default: 16
    town_offset_alt: 24          # fn_14000c71b==1
    discovered_click_extra: 16
    fleet_screen_box: 96         # 不用 search_radius
    remains_uses_radius: false
    night_affects_radius: false
```

```yaml
# config/formulas/supply.yaml（D3 形状）
schema: dk4.formulas.supply
version: 2
# 补给种类是内容（vanilla:water / vanilla:food，表长即种类数），定义在 ships/supplies.yaml，本文件只按 key 引用。
supply_drain:                    # F-ADJ-03 / F-OCN-05
  impl: vanilla.supply_drain     # rand 次序与余数掷骰按原版；可换 expr
  int_mode: { div: trunc, width: sat64 }   # 库存 u16（CShip+0x46）只在 legacy_codec 导出时校验
  params:
    div_base: 10
    numer: 10                    # crew * numer / divisor
    skill_roll: 500
    cook_div_bonus: 2            # 对所有种类
    keeper_food_div_bonus: 8     # 只对 food
    remainder_roll: 10
    warn_on_kind_empty: true
    marine_room_crew_bonus: null # 水兵室→水手 [未知]
  tables:
    keeper_bonus_kinds: [vanilla:food]
  refs:
    warehouse_cabin_role: supply_room       # 原版 cabin 9；Ships.cabin_count(ship, role)
    cook:   { post: vanilla:cook,   skill: vanilla:gunnery }   # 原版 post 12 / eSkill 6
    keeper: { post: vanilla:keeper, skill: vanilla:vitality }  # 原版 post 14 / eSkill 0
  inputs: [crew, stock, kind]
  rng_stream: ocean.supply
  schedule: day_start            # D4：tick.yaml daily phase 海上分支的 ocean.supply_drain 步；在港分支不耗（词表 §14-18）
  modifiers:                     # 由内容表 effects 注册，公式不比较 id
    food_div: supply.food_div          # 小猪像 +2（figureheads.yaml）
    food_stock_pct: supply.food_stock_pct  # 结局「有老鼠」(1) → 95（ocean/happenings.yaml outcomes.effects）
    water_div: supply.water_div        # 结局 暴风雨/暴风雪/雨/雪 (2–5) → +10
supply_fill:                     # 发现补给 / 初始化
  impl: expr
  params: { fill_den: 2 }        # cap/2
  inputs: [cap]
  expr: "cap / fill_den"
```

happening 结局表里对应的贡献（`config/ocean/happenings.yaml`，归 06-ocean 持有，此处只示意形状）：

```yaml
outcomes:
  - key: vanilla:rats            # 原版结局态 1「有老鼠」
    effects:
      - { key: supply.food_stock_pct, stage: override, value: 95 }
  - key: vanilla:storm           # 结局态 2；blizzard / rain / snow（3–5）同一贡献
    effects:
      - { key: supply.water_div, stage: add, value: 10 }
```

等级项归技能公式（`formulas/character.yaml` 的 `skill.level_source: total_tracks`，上限 `$caps.skill_level_cap`），本篇只读，不另设公式或修正器。

修正器 key（本篇锁定；注册表见 [14-config-schemas/README.md](../14-config-schemas/README.md)「修正器注册」）：

```yaml
# config/modifiers.yaml 片段
keys:
  fleet.search_radius_add:     { stages: [add], default: 0, provider_examples: [figureheads] }
  supply.food_div:             { stages: [add], default: 0, provider_examples: [figureheads] }
  supply.food_stock_pct:       { stages: [override], default: 100, provider_examples: [ocean.happenings] }
  supply.water_div:            { stages: [add], default: 0, provider_examples: [ocean.happenings] }
  ship.repair_cost:             # F-ADJ-04：船木工按等级登记 mul (100 - discount)/100
    derived: { post: vanilla:carpenter, level_min: 25, level_div: 5 }
  combat.level_bonus:           # F-ADJ-05 anBonus
    derived: { table: [50, 40, 30, 20, 10], div: 100, index: null }   # fn_1400844e0 [未知]
# auto_sail_days：原版无此立即数；不注册，禁止填假
```

World0 / World1 共用公式。本篇不读气候 id；happening 结局表在 `config/ocean/happenings.yaml`。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗改代码：

- `radius_base` / `obs_div` / 海豚 +16 / 圈顶点数
- 港口 / 村落 offset、舰队屏幕盒 96
- 补给种类数、日耗除数、大厨/饲养员加成、happening 结局 95% / +10（在结局表 effects）
- 船木工 25 级门槛与 `/5`
- `skill_level_cap`（只改技能尺度，不改半径闭形式）
- 港口空值是 `None`；原版 98 只在 `ids/legacy_codec.yaml`
- `overflow: saturate`

改了望手是哪一个岗位：只改 `vision.yaml` 的 `refs.lookout_post`（key）。改观察力是哪一条技能：只改 `observation_skill`。

## 8. 对外接口

**查询**

```
Progression.total_level(person_id) -> u32
Modifiers.eval(key, ctx) -> int                   # ctx = { person?, fleet?, ship? }
Fleets.post_holder(fleet_id, post_key) -> Option<PersonId>
Ships.cabin_count(ship_id, role) -> u32
Ocean.search_radius(fleet_id) -> int               # 派生量，Ocean 唯一计算方（不是修正器）
Ocean.supplies(fleet_id) -> [{ kind, amount, cap }]
Ocean.supply_shortage(fleet_id) -> [{ kind, unmet }]
Ocean.can_sail(fleet_id) -> { ok, reason }         # 耗尽则 reason=supplies
```

取不到舰队：整体 no-op。

**命令**

```
Clock.advance_days(n)                              # 日推进由 Clock 驱动；扣补给是 tick.yaml daily phase 的 ocean.supply_drain 步
Fleets.set_supply_rate { fleet_id, water_pct }     # CSupplyRateView
Supplies.consume / add                             # ships 拥有；港口补给走 add，金币走 economy
```

**事件**

```
SuppliesDepleted { fleet_id, kind, shortage: [int] }
SuppliesLow      { fleet_id, kind }                # 告警档，阈值可配
DiscoveryFound   { … }                             # 半径命中后由 ocean 发
FleetSpotted     { … }                             # 屏幕盒，不是半径
LevelChanged     { person_id, … }                  # progression 发；修理折扣订阅重拉
```

本篇锁定的修正器 key 见 §6 `config/modifiers.yaml` 片段。原结构体字段 `supply_drain_factor` 已撤销（随机除数不是常数因子），修正器一律经 `Modifiers.eval(key, ctx)`（注册表，非结构体）；`search_radius` 是 Ocean 派生量 `Ocean.search_radius`（D6），不是修正器；`auto_sail_days` 原版无依据，不注册。

禁止：Ocean 读 `eAbility`；修理函数读 `fn_1400123dc`；战斗读 `CShip+0x53`。

## 9. 未核实字段

| 项 | 依据 | 状态 |
| --- | --- | --- |
| `CShip+0x53` 是否必为 `eFigureHead` | 0/1/5/7/8 五处效果与道具 114–123 顺序吻合；`vfunc_09` 回退值 `0x0D` = 无；比较值 10 / 11 = 道具 199 怪鱼像 / 200 提督像（`05-ships-fleet/04`）；无写入穷尽 | `[推断]`（强） |
| 水 / 粮 与 kind 0、1 | 饲养员与 happening 态 1 都打 kind 1 | `[推断]` |
| `CShip+0x4C` 必为当前水手 | 耗粮分子 + HUD 加总 | `[推断]` |
| 水兵室是否加算 `+0x4C` | 舱室文案「最多 4」 | `[未知]` |
| `fn_1400844e0` 的 `anBonus` 下标 | 表五档 | `[未知]` |
| `fn_1400093f4` 的 `(level+1)*200` 金币用途 | mesfile `0xB2C` 未对上玩法名 | `[未知]` |
| 特产品两函数为何读等级 | 仅 xref | `[未知]` |
| `fn_14000c71b == 1` 的港口分支名 | 偏移 24 vs 16 | `[未知]` |
| happening 结局 1 / 2–5（读 `CPlayerFleetData+0x30`，**不是气候 id**） | 名表 `.data 0x140686550`：1 有老鼠；2–5 暴风雨 / 暴风雪 / 雨 / 雪 | `[确认]` |
| `ship vfunc+0x48==1` 的食物除数 +2 | `sE4`；船首像 1 = 小猪像 | `[推断]` |
| `SUPPLYAMOUNT_MIN/MAX` 十进制 | 仅符号；存储 u16 | `[未知]` |
| 自动航行天数 | 无立即数、无 mesfile | `[未知]` 禁止填假 |
| 探索成功率里的 `fn_14000f218` | 经纬矩形富度 0–4，`10*rich + skill(提督,9)/50` | 式 `[确认]`（06-ocean/02） |
| `fn_140005a15` 在玩家大地图 vs 仅 NPC 舰队 AI | 玩家告警走 `fn_140009f1b` | `[未知]` 是否双路径 |
| `fn_1400f6300` | 哗变 outcome 20 菜单，「说服」扣 `crew/10` | `[确认]` 不是日耗（06-ocean/05 §2.8） |
| 身心档 `+0x0D` 对技能的五档名字 | `fn_1401c87a0` | 见人物文档 |

查明后只改本篇对应行与 YAML default，不改调用方代码。
