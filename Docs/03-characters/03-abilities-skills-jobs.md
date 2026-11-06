# 能力、技能、职业与岗位

六维、十技能、岗位定义、星座成长表。舱室在船上怎么摆见 `05-ships-fleet/03-cabins-crew-posts.md`；本文件只定义岗位本身，以及人物系统向**修正器注册表**（`00-overview` DECISIONS D2）登记的 `ability.<key>` / `skill.<key>` 键。

## 1. 职责边界

本系统管：

- 六维定义与上下限（六维 0–100 = `ability_cap`；技能值 0–500 = `skill_value_cap`）
- 十技能定义、名称、与两维的配对、计算公式参数
- 岗位（`ePost`）定义：名称、所在舱室种类（`room`）、看哪项技能、门槛值
- 把「六维 + 等级 + 装备 + 身心」收成技能值，以 `skill.<key>` 键登记进修正器注册表
- 升级时的六维成长（星座倾向表 `characters/zodiac.yaml`，自 world 移交）

不管：

- 某条船有几间测量室、房间坐标（船）
- `level = isqrt(exp/30)` 的实现（progression 拥有；本系统消费 `level`）
- 买货折扣、炮击命中、交涉成功率的业务公式。业务系统用 `Fleets.post_holder(fleet, post_key)` 取人、用 `Modifiers.eval("skill.<key>", {person})` 取**技能值**，自己算
- 职业种类 `eJobKind` 的玩法含义（计数未还原，见第 9 节）

岗位与舱室的接口：`posts.yaml` 每行写 `room`（舱室种类 key），「舱室种类 → 岗位」由加载器反查生成；船侧只出舱室种类表（几何、`role`、上限）、`Ships.cabin_count(ship, role)` 与占用。人物侧不存房间。

## 2. 原版事实

### 六维

- `[确认]` `NUM_ABILITY = 6`，`INVALID_ABILITY = -1`。RTC `eAbility < 6`。来源：`AbilityID.h`。
- `[确认]` 顺序与名称：体力、敏捷、魅力、智力、精神、运气。来源：`Person.cpp`、`save_format.md`、exe 能力名指针表 `0x140686350`（6 项，文件偏移 `0x57eaf4` 起的字串）、`dk4save.c` `kStatName`。
- `[确认]` 六维运行时值域 **0–100**：加点函数 `fn_1400062da`（body `0x1401c7f20`）做 `clamp(0, ability + delta, 100)`，变了才回写 `CPerson+0x0F+i` 并返回 1。这就是 `exe_caps.md` 的「基础属性成长 100」补丁点（`0x1401c7f6c` 字节串）。硬顶 255（UCHAR）。
- `[确认]` **500 不是六维的帽。** `exe_caps.md` 叫「属性结算 500」的补丁点 `41 B8 F4 01 00 00 8B 54 24 30 33 C9` 唯一出现在 `fn_1401cbb40`（`clamp(0, x, 500)`），它只被技能函数 `fn_140009214`、`fn_1400135cf` 调用。所以 500 是**技能值上限**，规范键 `skill_value_cap`。
- `[确认]` 地方舰队买货里的 `a` 是 **技能**：`fn_14000e1ce(captain, 4)` 的第二参是 `eSkill`，4 = **会计术**，不是能力 4（精神）。证据：① 技能画面 `fn_14000dd69`（body `0x1401d4e50`）RTC 局部 `eSkill`（循环 `< 0x0A`）直接作第二参传给 `fn_14000e1ce`；② `fn_14000e1ce → fn_1400135cf → fn_140009214` 内用 `fn_1400099e9(k, j)` 查 `k < 10, j < 2` 的 10×2 表（见下）；③ 结果封在 `[0, 500]` 的正是技能帽。公式 `cost * (1000 - a + rand(100)) / 1000` 本身见 `exe_caps.md`。道具店买入还价 `base*(100 - a/50)/100` 的 `a` 也是 `fn_14000e1ce(p, 4)` 会计术 `[确认]`（`09-items`，`fn_14000f4ed`）；战斗里的 `500` 只出现在单挑败方损失 `(500 - 统率力)/500` `[确认]`（`07-combat/02-boarding.md`）。这些都是原版「业务直读人物」：重构后业务公式的输入单位是**技能值**，经 `Modifiers.eval("skill.<key>", ctx)` 取，不读六维下标。

CGoodSkillMenuData 里混有能力说明口吻的字串，但绑定是 `index < 0x0A`（十技能）。六维没有单独的说明函数被还原；能力说明以技能 tooltip 中间接出现（生命力←体力+精神，等等）。

- `[确认]` 升级时六维成长：`fn_140011c34`（加经验，RTC `nUpAbility` `aeAbility`）每段经验 `clamp(0, exp + d, 300000)`；段 0 升级则对 **体力/敏捷/魅力**（表 `0x1406863f0` = 0,1,2）、段 1 升级则对 **智力/精神/运气**（表 `0x140686400` = 3,4,5）各调 `fn_1400065fa`。每维增量 `rand(2) + rand(apt + 3) / 2`，`apt` 取星座成长表 `0x140688970`（见下）。增量 > 0 才走 `fn_1400062da`（封 100）。之后若生命力基础值（HP 上限）变大，把差值加到当前 HP。

### 星座成长表（自 `01-world/04` 移交）

- `[确认]` `.data` `0x140688970`，每条 32 字节 `{ name_ptr, i32[6] }`，12 条有效（第 13 条全 0 作结尾）。只被人物读：`fn_1400065fa` 取 `apt`，人物画面显示星座名。六列与六维同序。每行之和恰为 6。人物星座字段 = `CPerson+0x0B`，`CPerson::vfunc_11`（vtable `+0x58`）返回；主角改由生日现算（`01-person-model.md`）。

  | id | key | 名 | 体 | 敏 | 魅 | 智 | 精 | 运 |
  | --- | --- | --- | --- | --- | --- | --- | --- | --- |
  | 0 | `capricorn` | 摩羯座 | 0 | 2 | 1 | 2 | 0 | 1 |
  | 1 | `aquarius` | 水瓶座 | 1 | 1 | 0 | 1 | 2 | 1 |
  | 2 | `pisces` | 双鱼座 | 1 | 1 | 1 | 2 | 1 | 0 |
  | 3 | `aries` | 牧羊座 | 1 | 1 | 1 | 0 | 1 | 2 |
  | 4 | `taurus` | 金牛座 | 2 | 1 | 1 | 1 | 1 | 0 |
  | 5 | `gemini` | 双子座 | 1 | 1 | 1 | 1 | 0 | 2 |
  | 6 | `cancer` | 巨蟹座 | 0 | 1 | 2 | 1 | 1 | 1 |
  | 7 | `leo` | 狮子座 | 1 | 0 | 2 | 1 | 2 | 0 |
  | 8 | `virgo` | 处女座 | 2 | 1 | 0 | 1 | 1 | 1 |
  | 9 | `libra` | 天秤座 | 2 | 0 | 1 | 0 | 1 | 2 |
  | 10 | `scorpio` | 天蝎座 | 0 | 1 | 1 | 1 | 2 | 1 |
  | 11 | `sagittarius` | 射手座 | 1 | 2 | 1 | 1 | 0 | 1 |

  生日分界表 `0x140688ca0` = `[20,18,20,20,21,21,22,22,23,23,22,21]`（按月，0 起）。

### 十技能

- `[确认]` `NUM_SKILL = 10`，`INVALID_SKILL = -1`。来源：`SkillID.h`。
- `[确认]` 技能不存档，查询时现算。完整调用链（sc 反汇编逐条核过）：

  ```
  Skill(p, k)            = fn_14000e1ce(p, k)                    # thunk；body 0x1401c9f60
      cabin = 0x18                                               # 不在舰上 / 无舱
      if p.affiliation(+0x0C) == player_force: cabin = 舱室查询 fn_14000b9fb（未配置回 0x18）
      return SkillAt(p, k, cabin)
  SkillAt(p, k, cabin)   = fn_1400135cf                          # body 0x1401ca250
      v = Base(p, k, mental=1) + Equip(p, k, cabin)
      v = PhysFactor(v, p.physical(+0x0D))                       # fn_1401c87a0
      return clamp(v, 0, 500)                                    # fn_1401cbb40
  Base(p, k, mental)     = fn_140009214                          # body 0x1401c9d50
      L  = min(Lv0 + Lv1, 200)                                   # fn_1400123dc；200 = 技能封顶
      a, b = PAIR[k][0], PAIR[k][1]                              # fn_1400099e9，表 0x1406864a0；越界回 7
      s  = mental ? Adj(p,a) + Adj(p,b) : p.ab[a] + p.ab[b]      # Adj = fn_14000e098
      return clamp(s * (L + 100) / 150, 0, 500)
  Lv_i                   = min(isqrt16(exp_i / 30) / 256, 100)   # fn_1400146c3 / fn_140012085；q<=1 原样返回 → q==1 得 0
  ```

  `fn_14001406a` = `Base(p, k, 1)`；`Base(p, 0, 0)`（生命力、不计精神修正）= HP 上限 `fn_14000cf22`，见 `07-fatigue-health-mood.md`。`isqrt` 是 `fn_14000819d(x)`：对 `x * 0x10000` 做 16.16 定点 Newton 开方再 `/256`；`q = exp/30 == 1`（exp 30–59）时 `isqrt16` 原样返回 1，`1/256 = 0`，结果为 **0** 级（原版怪癖；重构式 `(q1_quirk and q == 1) ? 0 : isqrt(q)`，`13-algorithms/01` F-PROG-01、`11-progression/01-level-exp-skills.md` §4.1）。除法均为 C 截断。`level` 用的是**两段等级之和**。来源：上列函数反汇编。
- `[确认]` 精神状态对能力的修正 `Adj`（`fn_14000e098`，`CPerson+0x0E` 经 `fn_14000c4a5`）：

  | 精神 id | 名 | 修正（整数乘除） |
  | --- | --- | --- |
  | 0 | 高亢 | 体力 ×11/10、敏捷 ×105/100、智力 ×95/100、精神 ×105/100 |
  | 1 | 精神 | 体力、精神 ×105/100 |
  | 2 | 轻松 | 智力 ×105/100 |
  | 3 | 普通 | 无 |
  | 4 | 不高兴 | 精神 ×95/100 |
  | 5 | 混乱 | 体力 ×95/100、智力 ×9/10 |

- `[确认]` 身体状态对技能值的修正 `PhysFactor`（`fn_1401c87a0`，读 `CPerson+0x0D`）：疲劳/轻伤（1、2）×19/20；重伤（3）÷2；濒死（4）÷10；疾病（5）×3/5；健康（0）与死亡（6）不变。
- `[确认]` 装备加成 `Equip`（`fn_14000e0b6`，RTC `ePersonEquip`）对 5 槽求和 `fn_14000fcef`：槽值 `0xE8` = 空；`k == 5`（剑术）且道具种类（道具 `+0x29`）为 2 或 3 时直接加道具值（`+0x3A`）；否则道具带一个舱室种类（`+0x39`，`0x18` = 无），该舱的技能（运行时舱室表 `0x144294b40`）等于 `k`，且「持有者所在舱 == 该舱」或「持有者在舰长室（5）」或放行函数 `fn_14000f826(持有者)` 为真时才加。放行四项任一 `[确认]`（`09-items/02` §2）：① 主角；② 不属玩家势力；③ 另一支玩家舰队的提督；④ 玩家本队第 2–5 艘船的船长。所以 NPC 虽然舱恒为 `0x18`，舱室绑定用具对它们恒生效。效果数值属 `09-items`。
- `[确认]` 原版 `skill_level_cap = 200` 封的是**等级和**；技能**值**另封 500。`exe_caps.md` 的「两段之和 510」是 dk4save 改到单段 255 后的硬顶。
- `[确认]` `CGoodSkillMenuData::vfunc_01` 按 `eSkill` 0–9 取说明（`index >= 10` 则空）。三条长说明在 rdata，七条内联。重构文案 key 按名字：`skill.accounting.desc`，不按下标（下标 key 会被模组顺序分配撞号，D1）。从 `DK4HD_sc.exe` 抽出的完整 GBK：

  | id | 说明原文 |
  | --- | --- |
  | 0 | `%6s:ＨＰ最大值，对疾病等的抵抗力。` |
  | 1 | `%6s:舰长、提督的资质，指挥能力。` |
  | 2 | `%6s:驾驶的资质，航海术、造船能力。` |
  | 3 | `%6s:方向感、土地感觉，绘制海图能力。` |
  | 4 | `%6s:鉴定商品，物价情报的判断力。` |
  | 5 | `%4s　:剑的资质，白刃战的指挥能力。` |
  | 6 | `%6s:炮击的资质，大炮的命中力。` |
  | 7 | `%6s:辩论的资质，外交谈判能力。` |
  | 8 | `%6s:策略的资质，谍报能力。` |
  | 9 | `%6s:发现的资质，回避危险能力。` |

- `[确认]` exe 技能名指针表 `0x140686390`（10 项）按序指向：生命力、统率力（字串在 `0x14053c5f8`）、驾驶术、测量术、会计术、剑术（`0x140539520`）、命中力、说服力、谋略术、观察力。连续 8 字节槽里缺统率力、剑术，是因为这两条字串复用了别处。**eSkill 名称**：

  key 是本系统定的**唯一规范名**（其它文档、`skill.<key>` 修正器键、文案 key `skill.<key>.name` / `skill.<key>.desc` 都用它）。六维 key 用 `stamina/agility/charm/intelligence/spirit/luck`（与 `14-config-schemas/examples` 一致），所以技能 0 可以直接叫 `vitality` 而不撞名。

  | id | key | 名称 | 说明摘要 |
  | --- | --- | --- | --- |
  | 0 | `vitality` | 生命力 | HP 上限、抗病 |
  | 1 | `leadership` | 统率力 | 舰长 / 提督 |
  | 2 | `helmsmanship` | 驾驶术 | 航海术、造船 |
  | 3 | `surveying` | 测量术 | 海图、方向 |
  | 4 | `accounting` | 会计术 | 鉴定、物价 |
  | 5 | `fencing` | 剑术 | 白刃、指挥 |
  | 6 | `gunnery` | 命中力 | 炮击命中 |
  | 7 | `persuasion` | 说服力 | 外交谈判 |
  | 8 | `tactics` | 谋略术 | 策略、谍报 |
  | 9 | `observation` | 观察力 | 发现、避险 |

- `[确认]` 新游戏文案「请选择３种熟练的技能。」（mesfile `0xb63`），确认后「是%s﹑%s﹑%s吗？」（`0xb7b`）。UI 类 `CGoodSkillMenuData`。
- `[确认]` 符号 `NUM_SKILLABILITY` / `INVALID_SKILLABILITY` 存在（`SkillabilityID.h`），整数未恢复。技能由两维合成，此枚举即配对表的原版名。
- `[确认]` 公式里的 `level` 是**两段等级之和**：`fn_1400123dc = fn_1400146c3(段0) + fn_140012085(段1)`，在 `fn_140009214` 里 `> 0xC8` 则改 0xC8。
- `[确认]` 技能↔两维配对表在 `.data` `0x1406864a0`：`int[10][2]`，`fn_1400099e9(k, j)` 以 `k < 10`、`j < 2` 取值，越界返回 7。**必须进 YAML**，禁止业务代码写死下标：

  | 技能 | 维 A | 维 B | 原表 |
  | --- | --- | --- | --- |
  | 生命力 | 体力 | 精神 | 0,4 |
  | 统率力 | 魅力 | 精神 | 2,4 |
  | 驾驶术 | 敏捷 | 精神 | 1,4 |
  | 测量术 | 敏捷 | 智力 | 1,3 |
  | 会计术 | 智力 | 精神 | 3,4 |
  | 剑术 | 体力 | 敏捷 | 0,1 |
  | 命中力 | 敏捷 | 运气 | 1,5 |
  | 说服力 | 魅力 | **智力** | 2,3 |
  | 谋略术 | 智力 | 运气 | 3,5 |
  | 观察力 | 精神 | 运气 | 4,5 |

  旧版本把说服力写成「魅力+运气」，与 exe 表不符，已改。

### 岗位与舱室

- `[确认]` `ePost` / `NUM_POST` 有符号，整数未恢复。`INVALID_POST = -1`。来源：`PostID.h`。
- `[确认]` `eCabinKind` RTC 立即数 23 与 24；赋值默认 `0x18`，`>= 0x17` 则跳过。来源：`num_from_rtc.json`、`fn_1401dde00`。`NUM_SHIPCABIN = 31` 是船上房间**槽位数**，不是岗位种数。来源：`ShipCabinID.h`。
- `[确认]` exe 连续舱室名（从「桅杆」到「无」共 24 个字串）：桅杆、了望台、甲板、测量、船舵、舰长室、副官室、水兵室、武装炮台、物资仓库、货物仓库、划桨手室、单人房、木材室、医务室、厨房、娱乐室、饲养室、礼拜室、参谋室、会计室、追加1、追加2、无。来源：`DK4HD_sc.exe` `0x52dfbc`。
- `[确认]` 岗位名指针表（文件偏移 `0x5855f0`，16 项）：**舰长**（字串复用 `0x140542970`）、副官、参谋、会计长、测量员、操帆手、舵手、了望手、冲锋队长、炮手、传教士、船医、大厨、船木工、饲养员、甲板主管。连续字串 `0x58568c` 起只缺舰长，原因同技能名。「舰长室只能配置%s。」（`0x53b400`）、官方手册「舰长室（限主角）」。
- `[确认]` 舱室种类 → 岗位 `fn_140012dd2`（body `0x140069110`，`0..0x14` 跳表 `0x1400691cc`）：舰长室→0、副官室→1、参谋室→2、会计室→3、测量→4、桅杆→5、船舵→6、了望台→7、水兵室→8、武装炮台→9、礼拜室→10、医务室→11、厨房→12、木材室→13、饲养室→14、甲板→15；物资仓库/货物仓库/划桨手室/单人房/娱乐室及 `> 0x14` 返回 `0x11`（无岗位）。
- `[确认]` 反方向 岗位 → 舱室种类 `fn_140007e19`（body `0x140068f70`，`0..0xF` 跳表 `0x14006902c`，越界回 `0x18`）：0→5 舰长室、1→6、2→19、3→20、4→3、5→0、6→4、7→1 了望台、8→7 水兵室、9→8 武装炮台、10→18、11→14、12→15、13→13、14→17、15→2。两张跳表逐项互逆。这两张表与岗位名指针表一起钉死下表岗位 id。
- `[确认]` 按岗位取人有两条路（重构统一为 `Fleets.post_holder`）：`fn_140010cee(fleetdata, post)`（返回 person，空 = `0xEC`；`fn_14000f2d6` 包它）；`fn_14000da26(fleet, post, skill, …)` = `fn_140007e19(post)` 换舱 → `fn_140012e22` 取该舱人 → `fn_1400135cf(p, skill, cabin)`。战斗另有 `fn_14000dbac(mgr, cabin_kind, out[])`：按**舱室种类**（不是岗位）收集全舰队该舱的人（可多人），内部扫 `eShipCabin < 0x1F` 比较 `vfunc+0x38` 的舱种。
- `[确认]` 战斗与经济里的岗位调用点（原始指令核对）：
  - 岗位 0 / 1 舰长 / 副官：`fn_1401ac4a0(post, 7)` 取说服力进投降掷骰（`07-combat/04`）。
  - 岗位 3 会计长：`fn_140010cee(…, 3)` 取人，`fn_1400135cf(p, 4, 0x14)` = 会计术 @ 会计室；杀价概率 `min(100, (s-60)/2)`（`fn_1400137b4`），`s >= 160`（`cmp 0xa0`）开超额模式（`fn_14000833c`）。见 `04-economy`。广场「寻求支持」也是 `fn_140010cee(fleet, 3)` 取会计长、`fn_14000e1ce(p, 4) >= 100` 会计术（`08-town-life/06`）。
  - 岗位 7 了望手：`fn_1401ac4a0(7, 9)` 观察力（`fn_14019dd60`）；特殊擒获 `fn_140007e19(7)` → 舱 1 了望台，`fn_1400135cf(p, 6, 1)` 命中力 `> 150`（`fn_1401b9940`）。
  - 岗位 8 冲锋队长：接舷 `fn_1401ab6a0` 直接 `fn_14000dbac(mgr, 7, …)`——这个 7 是**舱室种类 7 水兵室**（`mov edx,7` @ `0x1401ab6e4`，不经 `fn_140007e19`），对应岗位 8；取剑术最高两人。
  - 岗位 9 炮手：`fn_14014a580(9, 6)` → `fn_14000d9ae` → `fn_14000da26` → `fn_140007e19(9)=8` 武装炮台，命中力进装填 / 射程（`07-combat/01`）。
  - 岗位 10 传教士：`fn_14000da26(fleet, 10, 7, 0)` = 说服力 @ 礼拜室。岗位 11 船医：观察力 @ 医务室（`fn_1400135cf(p, 9, 14)`）。岗位 12 大厨：`fn_14000da26(fleet, 0xC, 6)` 命中力进日耗（`06-ocean/01`）。见 `07-fatigue-health-mood.md`。
  - `07-combat` 第一轮把接舷的「`fn_14000dbac(mgr,7)`」读成岗位 7，于是和本表「8 = 冲锋队长」冲突；实为舱种 7，冲突消解，本表 id 不改。
- `[确认]` 舱室种类静态表（`0x1400256d0` 连调 24 次 `fn_14000b7bc` 建 `0x144294b40`，步长 0x18；`05-ships-fleet/03-cabins-crew-posts.md`）每行带 `+0x10` u8 技能（`0xb` = 无）与 `+0x12` u16。经岗位→舱室映射，16 个岗位的技能与 u16 全部可读（下表）。`+0x10` 被装备加成（`fn_14000fcef`：道具舱种的技能 == k 才加）读取 `[确认]`；它就是「该舱担任者看的技能」由上列调用点逐个印证（舰长 1、副官 7、会计 4、了望 9、冲锋 5、炮手 6、传教 7、船医 9、大厨 6 全部一致）。`+0x12` 的值与官方手册门槛（测量 120、会计 100、船舵 60、桅杆 80）逐个相等，语义 `[推断]` = 岗位门槛；任命时是否拦人 `[未知]`。
- `[确认]` mesfile 岗位效果散句：无测量员则不能建航线；船木工航行中修船、造船厂折扣；船医治病；大厨省水和粮食、减疲劳；炮手缩短炮击间隔；冲锋队长提高肉搏；舰长/副官攻城；副官写交涉文书；会计长战斗中无事可做、平时杀价。来源：`mesfile_sc.txt` `0x5d0f` `0xc0b3` `0xc187` `0x160bf` `0x17a83` 等。
- `[确认]` `eJobKind` / `NUM_JOBKIND` 仅有符号。来源：`JobkindID.h`。世界表 `+0x27` 像国籍/文化而非这 15 个岗位（拉斐尔=4 与里斯本 region 4 偶合，赫德拉姆=5 却不是瑞典），不能当成 `ePost`。

### 身心与疲劳

- `[确认]` `FATIGUE_MIN` / `FATIGUE_MAX` 约束在 `UCHAR`。来源：`asserts.txt`。
- `[确认]` 文案「疲劳度上升了%s。疲劳度恢复了%s。」（mesfile `0x343f`）；exe 状态条「%4d疲劳」。「在这里船员身体和精神都能够恢复」（`0xc0b3`）是**舱室**说明（与船木工 / 船医同块），不是旅馆句。数值疲劳是舰队字节 `CFleet+0x71`（0–100），见 `07` §2.4。
- `[确认]` 身体状态名指针表 `0x140686410`（7 项）：健康、**疲劳**（字串在 `0x14057e9e8`）、轻伤、重伤、濒死、疾病、死亡。精神状态名指针表 `0x140686458`（6 项）：高亢、**精神**（复用能力名字串）、轻松、**普通**（`0x140550bbc`）、不高兴、混乱。连续 8 字节字串缺的那几条是复用了别处字串。
- `[确认]` 身体状态存 `CPerson+0x0D`（初值 0，`CPerson::vfunc_04` 以 `== 6` 判死亡），精神状态存 `CPerson+0x0E`（初值 3 = 普通）。来源：`fn_140014709`、setter `fn_14000ab0f` / `fn_14000342c`。
- `[确认]` `MENTALLEVEL_MIN/MAX`、`PHYSICALLEVEL_MIN/MAX` 约束在 `UCHAR`。来源：`asserts.txt`。整数未恢复。
- 旧版本「疲劳不是身体枚举一项、以 exe 为准」是误读：exe 表里疲劳就是身体状态 1，官方手册是对的。

## 3. 实体与 ID

| 实体 | 枚举 | 原版计数 | 空值 | 拥有者 |
| --- | --- | --- | --- | --- |
| 能力 | `eAbility` | 6 | −1 | 人物 |
| 技能 | `eSkill` | 10 | −1 | 人物 |
| 技能维对 | `eSkillAbility` | `[未知]`（符号存在；数据 = 10×2 配对表） | — | 人物（`skills.yaml` 每行 `stat_a/stat_b`） |
| 岗位 | `ePost` | 16 `[确认]`（名称指针表 16 项；两张互逆跳表 0–15）。证据类型：判界 `cmp 0xf`（`fn_140007e19`） | `null`；原版无岗位 `0x11` 只在 `legacy_codec` | 人物 |
| 舱室种类 | `eCabinKind` | 23 合法（取表 `<0x17` 判界）；原版表 24 行含「无」，重构表 23 行、「无」不进表（词表 §4 / §14-8） | `null`；原版 `0x18` 只在 `legacy_codec` | **船** |
| 船上房间槽 | `eShipCabin` | 31（循环上界 `0x1F`） | `null` | **船** |
| 装备槽 | `ePersonEquip` | 5 | `null`；原版空道具 `0xE8` 只在 `legacy_codec` | 人物 |
| 性别 | `eSex` | 2：`CPerson+0x08 == 1` 显示「女」，否则「男」（`fn_140006b90`）`[确认]` | `null` | 人物 |
| 精神状态 | `eMentalState` | 6（名称指针表）`[确认]`；转移表行宽 6 | `null`；转移表内「不变」原版写 7，YAML 写 `-` | 人物 |
| 身体状态 | `ePhysicalState` | 7（名称指针表；`vfunc_04` 以 6 判死亡）`[确认]` | `null` | 人物 |
| 星座 | 表 `0x140688970` | 12 | `null`；世界值 13 = 开局随机 | 人物 |

原版岗位（id、舱室、技能、门槛值全部来自 exe 静态表与跳表 `[确认]`；门槛**语义** `[推断]`，见 §2）。原版 id 只作 `vanilla_index`，主键是 key：

| id | key | 名称 | 舱室种类（`room`） | 技能（舱表 `+0x10`） | 舱表 `+0x12` | 多人 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | `captain` | 舰长 | 5 舰长室 | 统率力 | 30 | 否；限主角 |
| 1 | `adjutant` | 副官 | 6 副官室 | 说服力 | 100 | |
| 2 | `staff` | 参谋 | 19 参谋室 | 谋略术 | 120 | |
| 3 | `purser` | 会计长 | 20 会计室 | 会计术 | 100 | |
| 4 | `surveyor` | 测量员 | 3 测量 | 测量术 | 120 | |
| 5 | `sail_master` | 操帆手 | 0 桅杆 | 驾驶术 | 80 | |
| 6 | `helmsman` | 舵手 | 4 船舵 | 驾驶术 | 60 | |
| 7 | `lookout` | 了望手 | 1 了望台 | 观察力 | 70 | |
| 8 | `boarding_chief` | 冲锋队长 | 7 水兵室 | 剑术 | 90 | 是：接舷 `fn_14000dbac` 收全部（每船最多 4 水兵室） |
| 9 | `gunner` | 炮手 | 8 武装炮台 | 命中力 | 90 | 舱可多间；`fn_14000da26` 经 `fn_140012e22` 只取一人 |
| 10 | `missionary` | 传教士 | 18 礼拜室 | 说服力 | 160 | |
| 11 | `surgeon` | 船医 | 14 医务室 | 观察力 | 150 | |
| 12 | `cook` | 大厨 | 15 厨房 | 命中力 | 70 | |
| 13 | `carpenter` | 船木工 | 13 木材室 | 测量术 | 130 | |
| 14 | `keeper` | 饲养员 | 17 饲养室 | 生命力 | 130 | |
| 15 | `deck_chief` | 甲板主管 | 2 甲板 | 生命力 | 30 | |

非岗位舱（物资仓库 9、货物仓库 10、划桨手室 11、单人房 12、娱乐室 16、追加 1/2）技能 `0xb`、`+0x12 = 0`。舱室种类表**在船包**（几何、`role`、`max_per_ship`）；岗位 → 舱室、技能、门槛属本表，船表不再重复存技能列（见第 6 节）。

## 4. 算法 / 公式

全部纯函数，常数进 `config/formulas/character.yaml`。

技能管线直接落在修正器注册表（D2）的固定堆叠顺序 `base → add → mul → clamp → override` 上，原版取整逐位复刻：

```
# ability.<key>（精神修正后的六维；技能公式消费）
ability.<k>  base  = person.abilities[k]                               # 0..ability_cap
             mul   = vitals.mental_states[mental].ability_mult[k]       # {num,den} 截断；source = vitals
# ability_raw.<key> 不挂任何贡献 = 原值（HP 上限、状态画面「不带精神修正」版本用）

# skill.<key>（技能值；业务公式的唯一输入单位）
q_i       = exp_i / exp_divisor                                       # progression 提供（F-PROG-01）
level_i   = min((q1_quirk and q_i == 1) ? 0 : isqrt(q_i), level_per_track)
eff_level = min(level_0 + level_1, $caps.skill_level_cap)
skill.<s>    base  = clamp((ability.<stat_a> + ability.<stat_b>) * (eff_level + skill_level_bias)
                           / skill_divisor, 0, skill_value_cap)        # 命名公式 dk4.skill.v1
             add   = equip_bonus(person, s, cabin)                     # source = item:<key>；规则见下
             mul   = vitals.physical_states[physical].skill_mult      # source = vitals；{num,den} 截断
             clamp = [0, skill_value_cap]
hp_max       = skill.vitality 的 base 段，ability 换成 ability_raw      # 原版 fn_140009214(p,0,0)
```

上下文 `ctx = { person, cabin? }`。`cabin` 只在 `formulas/character.yaml` `ability.params.cabin_lookup_scope` 命中时由 `Fleets.cabin_of(person)` 填（原版：隶属 == 玩家势力，否则视为无舱 `0x18`；但非玩家势力走放行函数 ②，舱室绑定用具照样生效，见 §2）。

原版内容包默认：

| 参数 | 原版 | 来源 |
| --- | --- | --- |
| `exp_divisor` | 30 | progression；人物不重复实现 |
| `level_per_track` | 100 | `fn_1400146c3` `r8d=0x64`（caps） |
| `q1_quirk` | true | `fn_14000819d` `q<=1` 原样返回（progression） |
| `skill_level_bias` | 100 | `fn_140009214` `add eax,0x64` |
| `skill_divisor` | 150 | `fn_140009214` `idiv 0x96` |
| `skill_level_cap` | 200 | `fn_140009214` `cmp 0xC8` |
| `skill_value_cap` | 500 | `fn_1401cbb40` |
| `level_source` | `total_tracks`（两段之和）`[确认]` | `fn_1400123dc` |
| `mental_adjust` / `physical_factor` | 见 §2 两张表 `[确认]` | `fn_14000e098` / `fn_1401c87a0` |

装备：5 槽的道具由物品系统登记贡献（加哪个技能、多少、`active_when`）。原版生效条件（`fn_14000fcef`）：剑术吃道具种类 2/3、不看舱；其余道具带一个舱室种类，其舱表技能 == 本技能，且 持有者在舰长室 / 放行函数 `fn_14000f826`（主角、非玩家势力、别支玩家舰队提督、本队 2–5 号船船长）/ 持有者所在舱 == 道具舱 三者之一。人物系统只提供 `cabin_of(person)`、`is_protagonist`，判定写成贡献的 `active_when` 谓词，不写 `if k == 5`。

身心：系数已还原（§2），是 `vitals.yaml` 状态行登记的 `mul` 贡献，不写进技能代码。

### 修正器键（D2；取代旧 `ModifierSet` 结构体）

人物系统只登记**人物本身**的键，不替业务系统算 factor：

| 键 | 值 | 贡献来源 |
| --- | --- | --- |
| `ability.<key>` | 精神修正后六维 | base = 状态；mul = `vitals:mental.<state>` |
| `skill.<key>` | 技能值 0–500 | base = 公式；add = `item:<key>`；mul = `vitals:physical.<state>` |
| `person.hp_max` | HP 上限 | base = `skill.vitality` 基段（不带精神） |

业务公式在消费方：「谁」用 `Fleets.post_holder(fleet, post_key)`（取不到回 `null`，由消费方 `holder_rule` 决定退到提督 / 船长 / 0），「多少」用 `Modifiers.eval("skill.<key>", {person})`，再代入自己的公式（`formulas/<系统>.yaml`）。旧结构体里的字段对应如下，只是索引，不是人物系统的输出：

| 旧字段 | 现在由谁算 | 原版输入 |
| --- | --- | --- |
| `buy_cost_factor` | economy `trade.npc_buy_cost` | `skill.accounting`（提督 / 会计长），`(1000 - s + rand(100))/1000` |
| 会计长杀价 / 超额 | economy | `skill.accounting` @ 会计长，`(s-60)/2`、`>= 160` |
| `item_price_factor` | items 还价 | `skill.accounting`，`100 - s/50` |
| `battle_hit` / 装填 | combat | `skill.gunnery` @ 炮手（玩家本舰队）否则提督 / 船长 |
| `battle_boarding` | combat | `skill.fencing` @ 水兵室全体；`(200 - 舰队疲劳)` |
| 投降掷骰 | combat | `skill.persuasion` @ 舰长 + 副官 |
| 特殊擒获 | combat | `skill.gunnery` @ 了望手 |
| 精神 / 疾病掷骰 | characters vitals | `skill.observation` @ 船医、`skill.leadership` @ 舰长、`skill.persuasion` @ 传教士 |
| `food_water_factor` | ocean 日耗 | `skill.gunnery` @ 大厨、饲养员技能 |
| `survey_unlock` / `diplomacy_ok` / `scheme_ok` | ocean / town | `post_holder(...) != null`（是否在岗） |

`buy_cost.base > skill_value_cap` 是加载期校验：否则原版负括号 + 无符号除法 bug 会复现（`exe_caps.md`）。这条约束属 economy 公式包，本系统只导出 `skill_value_cap`。

## 5. 耦合分析

原版：

- 贸易、战斗、道具价直接取能力下标。
- 岗位效果写在设施对话和战斗 AI 里，没有岗位表。
- `CCrewView` 同时画人、技能、房间。
- 技能公式常数 100、150、200、500、10×2 配对表、身心乘数写在 exe。

允许的依赖方向（原则 9）：`characters ← factions.force_id`；`ships/fleets ← characters.person_id`；`combat/economy/ocean ← Modifiers.eval("skill.*")` + `Fleets.post_holder`。禁止经济改经验、战斗写市价、任何系统写 `post == 9` / `skill == 5` 字面量（D3）。

## 6. 解耦后的配置

主键一律是命名空间 key（`vanilla:accounting`），定义行 `key:` 与引用处（含以内容 key 为键的映射）都写全（词表 §14-4）；`vanilla_index` 只给 `ids/legacy_codec.yaml` 导入导出用（D1）。文案 key 由 key 去掉 `vanilla:` 前缀派生（`skill.<key>.name` / `.desc`；模组 `mymod:diving` → `mymod:skill.diving.name`），不按下标。

```yaml
# config/characters/abilities.yaml
schema: dk4.characters.abilities
version: 2
abilities:
  - { key: vanilla:stamina,      vanilla_index: 0, name_key: ability.stamina.name }       # 体力
  - { key: vanilla:agility,      vanilla_index: 1, name_key: ability.agility.name }       # 敏捷
  - { key: vanilla:charm,        vanilla_index: 2, name_key: ability.charm.name }         # 魅力
  - { key: vanilla:intelligence, vanilla_index: 3, name_key: ability.intelligence.name }  # 智力
  - { key: vanilla:spirit,       vanilla_index: 4, name_key: ability.spirit.name }        # 精神
  - { key: vanilla:luck,         vanilla_index: 5, name_key: ability.luck.name }          # 运气

---
# config/characters/skills.yaml           # 配对表 .data 0x1406864a0 [确认]
schema: dk4.characters.skills
version: 2
skills:
  - { key: vanilla:vitality,     vanilla_index: 0, stat_a: vanilla:stamina,      stat_b: vanilla:spirit }
  - { key: vanilla:leadership,   vanilla_index: 1, stat_a: vanilla:charm,        stat_b: vanilla:spirit }
  - { key: vanilla:helmsmanship, vanilla_index: 2, stat_a: vanilla:agility,      stat_b: vanilla:spirit }
  - { key: vanilla:surveying,    vanilla_index: 3, stat_a: vanilla:agility,      stat_b: vanilla:intelligence }
  - { key: vanilla:accounting,   vanilla_index: 4, stat_a: vanilla:intelligence, stat_b: vanilla:spirit }
  - { key: vanilla:fencing,      vanilla_index: 5, stat_a: vanilla:stamina,      stat_b: vanilla:agility }
  - { key: vanilla:gunnery,      vanilla_index: 6, stat_a: vanilla:agility,      stat_b: vanilla:luck }
  - { key: vanilla:persuasion,   vanilla_index: 7, stat_a: vanilla:charm,        stat_b: vanilla:intelligence }   # 不是 luck
  - { key: vanilla:tactics,      vanilla_index: 8, stat_a: vanilla:intelligence, stat_b: vanilla:luck }
  - { key: vanilla:observation,  vanilla_index: 9, stat_a: vanilla:spirit,       stat_b: vanilla:luck }
# name_key / desc_key 缺省 = skill.<key>.name / skill.<key>.desc

---
# config/characters/posts.yaml            # 人物拥有岗位定义；舱室种类 key 由船包给
schema: dk4.characters.posts
version: 2
posts:
  - key: vanilla:captain
    vanilla_index: 0
    room: vanilla:captain_room           # 舱室种类 key（船包；原版舱种 5 舰长室）；fn_140007e19 / fn_140012dd2 互逆
    skill: vanilla:leadership                    # 原版舱表 +0x10
    threshold: 30                        # 原版舱表 +0x12；语义 [推断]
    enforce_threshold: false             # 原版任命是否拦人 [未知]；缺省不拦
    protagonist_only: true               # 「舰长室只能配置%s」
    multi_holder: false
  - key: vanilla:surveyor
    vanilla_index: 4
    room: vanilla:survey
    skill: vanilla:surveying
    threshold: 120
  - key: vanilla:boarding_chief
    vanilla_index: 8
    room: vanilla:marines                # 舱种 7 水兵室
    skill: vanilla:fencing
    threshold: 90
    multi_holder: true                   # 接舷收该舱全部人
  # 其余 13 行按第 3 节表生成（导出脚本读 0x1400256d0 + 两张跳表，不手抄）

---
# config/characters/zodiac.yaml           # 自 world 移交；.data 0x140688970
schema: dk4.characters.zodiac
version: 1
signs:
  - { key: vanilla:capricorn, vanilla_index: 0, growth: { vanilla:stamina: 0, vanilla:agility: 2, vanilla:charm: 1, vanilla:intelligence: 2, vanilla:spirit: 0, vanilla:luck: 1 } }
  - { key: vanilla:aquarius,  vanilla_index: 1, growth: { vanilla:stamina: 1, vanilla:agility: 1, vanilla:charm: 0, vanilla:intelligence: 1, vanilla:spirit: 2, vanilla:luck: 1 } }
  # … 12 行见第 2 节表
birthday_cutoff: [20, 18, 20, 20, 21, 21, 22, 22, 23, 23, 22, 21]   # 0x140688ca0；day < cut[m] ? m : m+1
world_random_value: 13            # 只在导入：世界表 +0x29 == 13 → index % 12（legacy_codec）

---
# config/formulas/character.yaml        # 形状 = 14-config-schemas README「公式层」（D3）
schema: dk4.formulas.character
version: 3
skill_value:                    # F-PROG-05；修正器 skill.<key> 的 base 段
  impl: vanilla.skill_v1        # 可整体换成 impl: expr（同输入、同 golden）
  int_mode: vanilla_i32         # 全部 C 截断；mul 贡献 {num,den} 先乘后除
  params:
    level_bias: 100               # 表中 skill_level_bias
    skill_divisor: 150            # 表中 skill_divisor
    level_cap: $caps.skill_level_cap    # 200；作用在等级和上
    value_cap: $caps.skill_value_cap    # 500；base 段与 clamp 段各封一次
    level_source: total_tracks    # total_tracks | track_max；原版 total_tracks [确认]
    ability_source: adjusted      # adjusted = ability.<key>（精神修正）| raw
  inputs: [stat_a, stat_b, total_level]   # stat_a / stat_b 配对在 characters/skills.yaml
  expr: null
  golden:
    - { stat_a: 59, stat_b: 58, total_level: 2, out: 79 }   # 拉斐尔 world0 开局生命力（raw）：(59+58)*102/150
ability:                        # F-PROG-09：业务系统读的技能值 = Modifiers.eval("skill.<key>")
  impl: vanilla.ability
  int_mode: vanilla_i32
  modifiers: skill.<skill_key>  # 每条技能一个 key；add 装备 → mul 身体态 → clamp
  params:
    output_clamp: $caps.skill_value_cap   # F-PROG-06；不夹六维
    cabin_lookup_scope: player_force      # 原版只给玩家势力查舱位；none | player_force | all
hp_max:                         # HP 上限 = 生命力 base（不过精神修正）
  impl: vanilla.skill_v1
  int_mode: vanilla_i32
  params: { ability_source: raw }
  refs: { skill: vanilla:vitality }
  modifiers: person.hp_max
# 六维成长（F-PROG-10）不在本文件：formulas/progression.yaml 的 stat_growth（11-progression/01），
# 每轨成长哪三维在 progression/level.yaml 的 tracks[].growth_abilities（原版 0x1406863f0 / 0x140686400）
```

装备生效条件不在本文件：物品登记 `skill.<key>` 的 `add` 贡献时自带 `active_when`（`always` = 剑术吃种类 2/3；`in_cabin_or_command` = 同舱 / 舰长室 / 放行四项，开关在 `formulas/items.yaml` 的 `equip_skill_bonus.command_check`），见 `09-items/02-equipment-effects.md`。

`config/caps.yaml`（键名见 `00-overview/10-key-glossary.md` §7）：`ability_cap: 100`、`skill_level_cap: 200`、`skill_value_cap: 500`、`level_per_track: 100`、`track_count: 2`、`exp_track_cap: null`（= `exp_of_level(level_per_track)`，原版 300000）。旧名 `ability_settled_cap` / `track_level_cap` / `skill_cap` 作废（500 不封六维）。

## 7. 独立旋钮

- 十技能改名、改配对两维、改 `level_bias/skill_divisor/cap`
- 岗位改所在舱室、所看技能与门槛，开关 `enforce_threshold`
- 星座成长表、生日分界
- 身心状态对能力 / 技能的乘数（`vitals.yaml` 状态行登记的 `mul` 贡献）
- 加第 11 个技能 / 第 17 个岗位（模组 `mymod:diving`）：`skills.yaml` / `posts.yaml` 追加；引擎按表长，ID 加载期紧凑分配
- 业务公式换技能（买货从会计术改到说服力）：改**消费方**公式包的 `skill_ref`，不改人物

改六维**含义**只允许改 `abilities.yaml` 与 `skills.yaml` 的 `stat_a/stat_b`，不允许任何代码出现 `ability[4]`。

## 8. 对外接口

```
# 修正器注册表（D2）—— 业务系统唯一入口
Modifiers.eval("skill.<key>",   { person, cabin? }) -> int      # 0..skill_value_cap
Modifiers.eval("ability.<key>", { person }) -> int              # 精神修正后
Modifiers.eval("person.hp_max", { person }) -> int
Modifiers.explain(key, ctx) -> [ { stage, order, source, value } ]   # 可追溯

# 人物侧查询
Characters.skill(person_id, skill_key) -> int                   # = 上面 eval 的便捷封装
Characters.ability_raw(person_id, ability_key) -> int
Characters.post_def(post_key) -> { room, skill, threshold, protagonist_only, multi_holder }
Characters.post_of_room(cabin_kind_key) -> post_key | null      # 加载期由 posts.room 反查

# 一级查询（D2；舰队 / 船拥有，列在这里是因为岗位语义在本表）
Fleets.post_holder(fleet_id, post_key) -> person_id | null
Fleets.post_holders(fleet_id, post_key) -> [person_id]          # multi_holder 岗位
Fleets.admiral(fleet_id) -> person_id | null                    # 提督不是岗位（16 岗无 admiral）；不写 post_holder(…, admiral)
Fleets.cabin_of(person_id) -> cabin_kind_key | null
Ships.cabin_count(ship_id, role) -> int

# 命令（ships 拥有；人物只做岗位校验）
Cabins.assign(ship_id, cabin_id, person_id) / unassign
    → 船侧：该船是否有此舱室
    → 人物：protagonist_only；enforce_threshold 时比 skill.<post.skill> >= threshold
    → 成功则船侧发 CabinBound
```

事件（ships 发）：`CabinBound` / `CabinUnbound { ship_id, cabin_id, person_id, post_key }`；人物订阅后使 `skill.*` 缓存失效。

战斗 / 经济只按键取值，不订阅六维、不读 `posts.yaml` 的下标。

## 9. 未核实字段

- `NUM_JOBKIND`、`NUM_SKILLABILITY`：有符号无整数。`NUM_POST` = 16（判界 `cmp 0xf` + 名称表）、`NUM_CABINKIND` 合法 23（判界 `<0x17`）、`NUM_SEX` 2、`NUM_MENTALSTATE` 6、`NUM_PHYSICALSTATE` 7、星座 12（名称表 / 比较立即数），头文件仍未写整数。
- 舱表 `+0x12` u16 的语义：值与手册门槛相等 `[推断]`；任命或岗位效果是否读它 `[未知]`（`09-items` 同列未解）。
- （已解决）`fn_14000f826` 四项放行 `[确认]`（`09-items/02` §2）；其中「另一支玩家舰队提督」（`fn_14000ee26`）的玩法语义按调用形 `[推断]`。
- `eJobKind` 与世界表 `+0x27`/`+0x28` 的关系：`[未知]`。
- 熟练三技能的机械效果（加点？阈值减免？）：`[未知]`，见 `04-recruitment-crew.md`。CPerson / CPlayerPerson 串行化里没有看到 3 个 skill_id 字段。
- 心情：CPerson 串行化里只有精神状态枚举 `+0x0E`，没有另一个心情计量；状态画面「心情」应即该枚举 `[推断]`（见 `07`）。HP 已还原：见 `07`。
- 舰队水手疲劳 `CFleet+0x71`（0–100）归舰队，见 `07` §2.4；具名航海士的「疲劳」是身体状态 1。
