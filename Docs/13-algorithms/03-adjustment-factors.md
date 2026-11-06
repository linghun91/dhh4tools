# 调节因子总表（等级 · 视野 · 补给）

三个跨系统标量：人物**总等级**、舰队**搜索半径**、船上**水/粮库存**。本篇是资深清单「调节因子」的收口；等级公式本身不重写，见 [11-progression/01-level-exp-skills.md](../11-progression/01-level-exp-skills.md) 与 [01-formulas.md](01-formulas.md) `F-PROG-*`。

人物对外只出 `ModifierSet`。业务系统禁止再读六维下标、禁止再读 `CShip*`。

## 1. 职责边界

本系统管：

- 把总等级当成**输入因子**喂给其它公式（技能封顶、修理折扣、战斗加成……）
- 搜索半径怎么从了望手 / 观察力 / 了望台 / 船字段算出来
- 港口、村落、遗迹、道具、NPC 舰队各自用哪一套「看见」判定
- 水、粮两种补给的库存、日耗、短缺，以及耗尽时写港口哨兵 `0x62`
- 大厨 / 饲养员 / 物资仓库 / 气候怎么改消耗
- 对应 YAML：`config/formulas/vision.yaml`、`config/formulas/supply.yaml`；等级参数仍在 `formulas/progression.yaml`

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
if level > 0xC8: level = 0xC8          # skill_cap=200
skill = (stat_a + stat_b) * (level + 0x64) / 0x96   # +100 / 150
return clamp(skill, 0, 0x1F4)          # 500
```

来源：`fn_140009214` 反汇编；与 `Save.cpp` / `dk4save.c` 一致。`[确认]`

`fn_14000e1ce` → `fn_1400135cf` 在此基础上再加装备（`fn_14000e0b6`，RTC `ePersonEquip`）并按人物 `+0x0D` 身心档打折（`fn_1401c87a0`：×19/20、/2、/10、×3/5），最后再夹 500。`[确认]` 身心档整数语义见人物文档。

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
- `[推断]` `+0x53 == 5` 是海豚船首像（道具 119，figurehead 下标 5，文案「看见更多」）。`0x0D` 像空哨兵（超过 0–9）。不是了望台（舱室 1）。写入点未穷尽，不升确认。
- `[确认]` 了望手道具文案：「如果瞭望手有了它，就会很容易发现宝物和港口。」「可以发现的范围更大。」走观察力 → 半径，不读六维。
- `[确认]` 港口 `fn_140007806`：循环 `eTown < 0x61`，空结果哨兵 `0x62`。显示点 `+= 0x10` 或 `0x18`（`fn_14000c71b == 1` 时 24，否则 16）。未发现：`dist² ≤ (radius + offset)²` 则发现。已发现：用 `offset+0x10` 做更近的「选为入港目标」盒。
- `[确认]` 村落 `fn_140001186`：循环 `eVillage < 0x18`，哨兵 `0x19`。偏移恒 `0x10`。判定同左。
- `[确认]` 主动探索 `fn_14001266b`：扫描 `eItem < 0xE7`，`dist² ≤ radius²` 才算圈内道具。随后 `Ability(person, 9)` 进成功率（`rand(100)` vs `100 * x + observation`，`x` 来自 `fn_14000f218`）。`[未知]` `x` 的设计名。
- `[确认]` 遗迹**不**走 `fn_14019dd60`（该函数只有上述 3 个调用方）。遗迹是探索 outcome，见 search 文档。
- `[确认]` NPC 舰队遭遇 `fn_14000cd8d` **不**用搜索半径。`fn_140008530`：屏幕空间 `dx,dy` 都 `< 0x60=96` 才继续，否则跳过。方向名 `fn_1401a9c90` 按 256 分圆八分位取字符串，不是射程。
- `[确认]` 半径公式**不含** `nTime`、气候 id。昼夜 / 天气不改搜索半径。气候改的是补给（§2.3），不是视野。

### 2.3 补给

- `[确认]` `eShipSupply` 循环上界 2。`anSupply` RTC 8 字节 = 两个 int32。`NUM_SHIPSUPPLY` 头文件未写整数；运行时就是 2。断言 `0 <= eVal && eVal < NUM_SHIPSUPPLY`。
- `[推断]` 0 = 水、1 = 粮。依据：饲养员「节约粮食」只加在 `kind==1` 的除数上；气候 1 把 `kind==1` 的库存 `× 0x5F/0x64`（95%）；文案两种桶。
- `[确认]` 库存在 `CShip+0x46 + kind*2`，u16。读 `fn_140013e1c`，写 `fn_14000f033`（`mov word [ship+kind*2+0x46], ax`，**赋值不是加减**）。
- `[确认]` 物资仓库 = 舱室 kind 9。`fn_140006b95(ship, kind)` 扫 31 槽，`vfunc+0x38 == kind` 则计数。`fn_140012724` = `count(kind==9)`。
- `[确认]` 日耗在 `fn_140005aa6`（body `0x1401dc830`，RTC `anSupplyShortage` / `asSupplyData` / `eShipSupply`）。由 `fn_140005a15` 在舰队当前港 `== 0x62`（海上）时调用。
- `[确认]` `fn_140005a15`（body `0x1400fa870`，4192 字节）RTC：`eNewFleetState`、`asCompanion`、`eFleetShip`、`nLatitude`、`nLongitude`、`anSupply`、`aePerson`、`nShipTotal`、`anSupplyShortage`。港不是 `0x62` 则整段跳过（在港不耗）。
- `[确认]` 消耗分子是船 `vfunc+0x58` = `CShip::vfunc_11` = `CShip+0x4C` u16。`[推断]` 当前水手人数（HUD 把五船该值加总；吃粮人数）。
- `[确认]` 大厨：`fn_14000da26(post=12, skill=6)`。饲养员：`(post=14, skill=0)`。空岗技能 0。
- `[确认]` 气候 id 1：粮库存先 `× 95/100`。气候 id ∈ [2,5]：`fn_14000ce78` 为真，水除数 `+10`。
- `[确认]` `nTime == 0` 的那一拍：`fn_140004101`（海域图）重算 `fn_14000bcc1` 求和，**不在这里扣库存**。扣库存是 `fn_140005aa6` 写回。
- `[确认]` `fn_14000bcc1`：五船两类补给分别求和，返回第一个 `≤ 0` 的 kind；两类都 `> 0` 返回 3，调用方跳过告警。
- `[确认]` 耗尽：`fn_140009f1b` 把 `COceanScene+0xA0` 写成 `0x62`，再切命令。文案「提督，这样下去物资就要没了……。」
- `[确认]` 发现补给 / 初始化：`fn_140008486` 把两类都 **赋成** `warehouse_count / 2`（村落发现路径会调）。比例重分配 `fn_140011ff9`：`water = cap * rate / 100`，`food = cap - water`，`rate` 来自 `CSupplyRateView`。
- `[确认]` 另有菜单路径 `fn_1400f6300` 选项 1：`new = max(0, stock - crew/10)`，两边同样扣。由 `fn_140005a15` 末尾按状态调用。与 `005aa6` 的可变除数不是同一条式子。

## 3. 实体与 ID

| 实体 | 原版 | 空 / 非法 | 重构 key |
| --- | --- | --- | --- |
| 总等级 | 两轨之和，缓存 `save+0x70` | 0 合法 | `total_level` |
| 技能 | `eSkill` 10 | -1 | `skill_id`（观察力=9） |
| 岗位 | `ePost` 至少 16 | -1；绑定空人物 `0xEC` | `post_id`（了望手=7） |
| 舱室种类 | `eCabinKind` 24 名 | `0x18=无` | `cabin_kind_id`（了望台=1，物资仓库=9，厨房=15，水兵室=7） |
| 补给种类 | `eShipSupply` **2** | -1 | `supply_kind`（0 水 / 1 粮，顺序 `[推断]`） |
| 港口空哨兵 | `0x62=98` | 不是 `INVALID_TOWN=-1` | `town.empty`（Ocean 入港槽与耗尽共用） |
| 搜索圈顶点 | 64 | — | `search_circle_vertices` |
| 角模 | 256 | — | `angle256` |

运行时循环读表长。原版 2 / 5 / 31 / 64 / 97 只作 vanilla 包实例。

## 4. 算法 / 公式

全部魔数进 YAML。溢出：**饱和**，禁止绕回 0、禁止无符号把负数当成几百万。

### 4.1 等级 → 技能（只引用）

`level_for_skill = min(total_level, skill_cap)`，再进 `F-PROG-05`。参数 `skill_cap=200`、`level_bias=100`、`skill_divisor=150`。重构饱和到 `caps.skill_cap`，禁止原版 32 位 `isqrt` 绕回。

`ModifierSet.level_for_skill` 就是这个夹紧后的整数。业务系统若只需「等级因子」读它，不要自己 `isqrt`。

### 4.2 等级直接读者（穷尽 `fn_1400123dc` 的 22 处 call）

| 系统 | 函数 | 做什么 | 等级 |
| --- | --- | --- | --- |
| progression / characters | `fn_140009214` | `F-PROG-05` 技能 | `[确认]` |
| characters | `fn_140006c67` / `fn_140008387` | 加经验（夹 `0x493E0=300000`）后按**升了几级**循环成长六维 | `[确认]` |
| ships / town | `fn_140175b00` 修理、`fn_1400103b6` 改造 | 船木工（舱 13）`level >= 25` 才打折：`pct = level / 5`，`cost -= cost * pct / 100`。25→5%，100→20%，200→40% | `[确认]` |
| combat | `fn_140084400`（RTC `anBonus`） | `bonus = level * table[idx] / 100`，表 `{50,40,30,20,10}`。`idx` 来自 `fn_1400844e0` `[未知]` | `[确认]` 式；`[未知]` 下标 |
| items | `fn_1400093f4` | 扣金 `(level+1)²`（再被道具/委托状态加减）。用途 `[未知]` | `[确认]` 式 |
| ui | `fn_14000a38f`、`fn_14000dd69`、`fn_14000127b`、`fn_14000141a` | 状态画面 / 六边形 / 存档卡显示 | `[确认]` 投影 |
| items | `fn_140014097`、`fn_140010e10` | 读等级，业务 `[未知]` | `[未知]` |
| economy | `fn_14015ccf0`、`fn_14015d600`（特产品 RTC） | 读等级，是否进供给 `[未知]` | `[未知]` |
| ocean / happening | `fn_140008c33`、`fn_1400ebfd0`、`fn_1401dc310` | 读等级，语义 `[未知]` | `[未知]` |

**没有**任何 call 把总等级直接变成「自动航行天数」。`ModifierSet.auto_sail_days` 是设计位：原版驾驶术经 `fn_1401ac4a0(6, 2)`（舵手，驾驶术）进航速，等级只通过技能间接进去。禁止编造「等级=可自动航行日数」。

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
obs = ModifierSet 取了望手观察力        # 空岗 0；已含等级与装备
r   = radius_base + floor(obs / obs_div) # 32 + obs/10
if any_ship.byte_53 == figurehead_vision_id:  # 原版 5
    r += radius_figurehead_bonus             # 16
r = saturate(r, 0, search_radius_hard)
```

参数默认：`radius_base=32`，`obs_div=10`，`figurehead_vision_id=5`，`radius_figurehead_bonus=16`。`search_radius_hard` 进 `engine_limits`，禁止 u16 绕回。

圈是 360°。`ModifierSet.search_radius` 就是这个 `r`（按舰队算一次，不按人）。

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
    if ship_flag_e4:                        # vfunc+0x48==1，[未知]
        divisor += food_flag_div_bonus      # +2
    stock = stock * climate_food_mul / climate_food_div
            # 气候 1：95/100；其它气候不乘
if kind == water and climate in water_save_climates:  # [2,5]
    divisor += water_climate_div_bonus      # +10

need = (crew * drain_numer) / divisor       # crew*10 / divisor
if rand(10) < (need % 10):
    need += 10                              # 余数再掷一次
units = need / 10

stock' = saturate(stock - units, 0, supply_amount_max)
# 跨船分摊未扣完的 units → shortage[kind]
```

`shortage[kind] > 0` 写入 `anSupplyShortage`（两个 int32），并给 `fn_140005a15` 的海上事件强度 `+1`。两类都扣光：告警，入港槽 = `empty_town_sentinel`。

菜单简化式（`fn_1400f6300` 选项 1）保留为同一 YAML 的 `drain_simple: crew / 10`，默认包可关；重构主路径只用可变除数那条。

容量：

```
cap = count_cabins(ship, warehouse_kind)    # kind 9
fill_each_kind = cap / 2                    # 发现水粮 / 初始化
# 比例：water = cap * supply_rate / 100 ; food = cap - water
```

水手人数 `crew` 来自 fleets 快照。水兵室是否加算船员：`[未知]`，YAML 留 `marine_room_crew_bonus: null`。

`ModifierSet.supply_drain_factor` = `drain_div_base / divisor`（大厨/饲养员/气候生效后；1.0 为无人无气候）。Ocean 用 `crew * drain_numer * factor / 10` 等价于上面的 `units`（先乘后除，64 位中间值）。

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
| `skill` 乘法 | 32 位 | 64 位后再夹 `ability_settled_cap` |
| 搜索半径 | u32 局部 | 饱和到 `search_radius_hard` |
| 补给 u16 | 写 word | 饱和 `[0, supply_amount_max]`；缺符号整数则先用 65535 |
| 修理 `cost * pct / 100` | 32 位 `imul`+`div` | 有符号饱和，`cost ≥ 0` |
| `(level+1)²` 金币 | 32 位 | 64 位再饱和到金币宽 |

## 5. 耦合分析

原版缠在一起：

- 观察力既进探索成功率又进半径；了望手岗位映射写死在 `fn_140007e19`
- `CShip+0x53` 同时被航速（type 0/7）和半径（==5）当不同语义读——重构拆成 `ship_type_code` 与 `figurehead_id`
- 补给库存、物资仓库计数、水手、大厨技能、气候、入港哨兵都在 `COceanScene` / 未挂类函数里直接改船字节
- `0x62` 既是「没选港口」又是「补给耗尽停止入港」
- 等级被修理、改造、战斗、道具、UI、特产品同一 `fn_1400123dc` 拉走

重构切断：

- Ocean 只读 `ModifierSet.search_radius` 与 `SailingSnapshot.supplies / crew / warehouse_count`
- 耗尽只发 `SuppliesDepleted`，由会话把入港槽写成配置哨兵
- 修理折扣由 ships 读 `ModifierSet` 的 `repair_discount_pct`（人物按等级算好再给）
- 战斗读 `ModifierSet.battle_level_bonus`，不读等级原值

## 6. 解耦后的配置

```yaml
# config/formulas/vision.yaml
schema: dk4.formulas.vision
version: 1
search_radius:
  radius_base: 32
  obs_div: 10
  lookout_post: lookout          # post 7
  observation_skill: observation # skill 9
  lookout_cabin: lookout_deck    # cabin 1
  figurehead_vision_id: 5        # [推断] 海豚像
  radius_figurehead_bonus: 16
  circle_vertices: 64
  angle_step: 4                  # 64*4=256 全周
  angle_modulus: 256
spot:
  village_offset: 16
  town_offset_default: 16
  town_offset_alt: 24            # fn_14000c71b==1
  discovered_click_extra: 16
  fleet_screen_box: 96           # 不用 search_radius
  remains_uses_radius: false
night_weather:
  affect_search_radius: false
overflow: saturate
```

```yaml
# config/formulas/supply.yaml
schema: dk4.formulas.supply
version: 1
kinds:
  - id: 0
    key: water
    name: 水
  - id: 1
    key: food
    name: 粮食
storage: u16                     # CShip+0x46
warehouse_cabin: supply_room     # cabin 9
cook_post: cook                  # 12
cook_skill: gunnery              # eSkill 6，与岗位表一致
keeper_post: keeper              # 14
keeper_skill: vitality_skill     # eSkill 0
drain:
  div_base: 10
  numer: 10                      # crew * numer / divisor
  skill_roll: 500
  cook_div_bonus: 2
  keeper_food_div_bonus: 8
  food_flag_div_bonus: 2         # ship vfunc+0x48==1，[未知]
  water_climate_div_bonus: 10
  remainder_roll: 10
  climate_food:
    climate_id: 1
    mul: 95
    div: 100
  water_save_climates: [2, 3, 4, 5]
  simple_menu_div: 10            # fn_1400f6300；主路径不用
fill:
  each_kind_from_cap: half       # cap/2
empty_town_sentinel: 98          # 0x62
warn_on_kind_empty: true
marine_room_crew_bonus: null     # 水兵室→水手 [未知]
overflow: saturate
```

```yaml
# config/formulas/progression.yaml  （已有，不改形状；本因子只读）
level_for_skill:
  source: total_tracks
  cap: 200                       # 与 caps.skill_cap 同一值，校验器检查
```

```yaml
# config/formulas/character.yaml  modifiers 段追加
modifiers:
  search_radius:                 # 由人物+岗位+船快照合成，ocean 只读
    from: F-ADJ-01
  supply_drain_factor:
    from: F-ADJ-03               # drain_div_base / divisor
  level_for_skill:
    from: F-PROG-05              # min(total_level, skill_cap)
  repair_discount_pct:           # F-ADJ-04
    post: carpenter
    level_min: 25
    level_div: 5
  battle_level_bonus:            # F-ADJ-05
    table: [50, 40, 30, 20, 10]
    div: 100
    index: null                  # fn_1400844e0 [未知]
  auto_sail_days: null           # 原版无此立即数；禁止填假
```

World0 / World1 共用公式。气候 id 表在 `config/world/climate.yaml`。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗改代码：

- `radius_base` / `obs_div` / 海豚 +16 / 圈顶点数
- 港口 / 村落 offset、舰队屏幕盒 96
- 补给种类数、日耗除数、大厨/饲养员加成、气候 95%
- 船木工 25 级门槛与 `/5`
- `skill_cap`（只改技能尺度，不改半径闭形式）
- `empty_town_sentinel`（存档迁移表另写）
- `overflow: saturate`

改了望手是哪一个 `post_id`：只改 `vision.yaml` 的 `lookout_post`。改观察力是哪一条技能：只改 `observation_skill`。

## 8. 对外接口

**查询**

```
Progression.total_level(person_id) -> u32
Progression.level_for_skill(person_id) -> u32     # min(total, skill_cap)
Characters.modifiers(person_id) -> ModifierSet
  # 必含：search_radius 不在单人上；见 fleets
Fleets.spotting(fleet_id) -> { search_radius, lookout_person_id | none }
Ocean.search_radius(fleet_id) -> int               # = Fleets.spotting
Ocean.supplies(fleet_id) -> [{ kind, amount, cap }]
Ocean.supply_shortage(fleet_id) -> [{ kind, unmet }]
Ocean.can_sail(fleet_id) -> { ok, reason }         # 耗尽则 reason=supplies
```

取不到舰队：整体 no-op。

**命令**

```
Ocean.advance_calendar_step { fleet_id }           # 扣补给；nTime==0 语义
Fleets.set_supply_rate { fleet_id, water_pct }     # CSupplyRateView
Fleets.refill_supplies { fleet_id, gold }          # 港口会话，金币走 economy
```

**事件**

```
SuppliesDepleted { fleet_id, kind, shortage: [int] }
SuppliesLow      { fleet_id, kind }                # 告警档，阈值可配
DiscoveryFound   { … }                             # 半径命中后由 ocean 发
FleetSpotted     { … }                             # 屏幕盒，不是半径
LevelChanged     { person_id, … }                  # progression 发；修理折扣订阅重拉
```

`ModifierSet` 本篇锁定的键：

```
ModifierSet {
  level_for_skill,          # min(total_level, skill_cap)
  search_radius,            # 舰队级，合成后写入 sailing snapshot
  supply_drain_factor,      # 相对除数 10
  repair_discount_pct,      # 船木工等级
  battle_level_bonus,       # anBonus
  auto_sail_days,           # 占位 null
  …
}
```

禁止：Ocean 读 `eAbility`；修理函数读 `fn_1400123dc`；战斗读 `CShip+0x53`。

## 9. 未核实字段

| 项 | 依据 | 状态 |
| --- | --- | --- |
| `CShip+0x53 == 5` 是否必为海豚像 | 文案 + 下标 5；无写入穷尽 | `[推断]` |
| 水 / 粮 与 kind 0、1 | 饲养员与气候 1 都打 kind 1 | `[推断]` |
| `CShip+0x4C` 必为当前水手 | 耗粮分子 + HUD 加总 | `[推断]` |
| 水兵室是否加算 `+0x4C` | 舱室文案「最多 4」 | `[未知]` |
| `fn_1400844e0` 的 `anBonus` 下标 | 表五档 | `[未知]` |
| `fn_1400093f4` 的 `(level+1)²` 金币用途 | mesfile `0xB2C` 未对上玩法名 | `[未知]` |
| 特产品两函数为何读等级 | 仅 xref | `[未知]` |
| `fn_14000c71b == 1` 的港口分支名 | 偏移 24 vs 16 | `[未知]` |
| 气候 id 1 / 2–5 的枚举名 | 立即数 | `[未知]` |
| `ship vfunc+0x48==1` 的食物除数 +2 | `sE4` | `[未知]` |
| `SUPPLYAMOUNT_MIN/MAX` 十进制 | 仅符号；存储 u16 | `[未知]` |
| 自动航行天数 | 无立即数、无 mesfile | `[未知]` 禁止填假 |
| 探索成功率里的 `fn_14000f218` | 与观察力相加 | `[未知]` |
| `fn_140005a15` 在玩家大地图 vs 仅 NPC 舰队 AI | 玩家告警走 `fn_140009f1b` | `[未知]` 是否双路径 |
| `fn_1400f6300` 与 `fn_140005aa6` 是否同一日历步都跑 | 前者菜单式 `crew/10` | `[未知]` 主路径以 `005aa6` 为准 |
| 身心档 `+0x0D` 对技能的五档名字 | `fn_1401c87a0` | 见人物文档 |

查明后只改本篇对应行与 YAML default，不改调用方代码。
