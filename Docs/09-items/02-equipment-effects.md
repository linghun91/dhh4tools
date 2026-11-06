# 装备槽与效果

配置落点：`config/items/effects.yaml` `config/formulas/items.yaml`（还价、装备加成参数）`caps.yaml`。目录与种类见 [01-item-catalog.md](01-item-catalog.md)。

本系统把「穿上某件 `eItem`」变成修正器注册表里 `skill.<key>` 键的**带来源贡献**（原版装备只加技能；D2）。人物、战斗、航海、贸易只按键求值 `Modifiers.eval("skill.<key>", {person})`，不读六维原值，也不 `switch (item_id)`。

## 1. 职责边界

管：

- 人物 5 个装备槽的定义与合法性
- 效果操作表：`skill_bonus` / `modifier` / `world_flag` / `mark_remains_discovered` / `plant` / …
- 作为修正器提供方注册 `skill.*` 的 `add` 段贡献（`source: item:<key>`）：原版装备**只加技能**，不加六维
- 道具店买入还价公式的 **参数**（原版 `100 - 会计术/50`）
- 效果量上下限（`ITEMEFFECT_AMOUNT_*`）

不管：

- 道具有没有、叫什么（目录）
- 技能基值怎么从六维 + 等级算（`03-characters/03` 的 `base` 段；等级由 `11-progression` 给）
- 修正器堆叠顺序与取整（`base → add → mul → clamp → override`，`00-overview/03` 原则 6）
- 海战伤害、接舷（`07-combat` 读结算技能，如剑术、命中力）
- 买卖货物折扣（那是 `buy_cost_factor`，另一条公式）
- 船首像的效果（`05-ships-fleet` 的 `ships/figureheads.yaml` 以 `item` 外键指回本表，并由 ships 注册 `fleet.*` 贡献；见 3.2）
- UI「让谁装备？」（`12-ui-save`）

船首像（kind=`figurehead`）**不是**人物槽。人物槽只接受 kind 2/3/4。地图、发现物、碎片走 `UseItem` 命令，不走穿戴。

## 2. 原版事实

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| `NUM_PERSONEQUIP = 5`，`INVALID_PERSONEQUIP = -1` | [确认] | `PersonEquipID.h`；RTC `NUM_PERSONEQUIP` / `NUM_EQUIP` 均为 5 |
| 人物对象装备字节从 `+0x23` 起，按槽索引 | [确认] | `fn_140012af8`：`person+0x23`；`fn_140003d23` 循环 `ePersonEquip >= 5` 比较 `*(person + slot + 0x23)` |
| 空槽值 `0xe8 = 232` | [确认] | 同上；`fn_140006811` 见 `0xe8` 直接返回 |
| 装备合法性按 **种类** 分流：2 武器、3 防具、4 用具 | [确认] | `fn_140012657`：`fn_14000598e(GetFixItemProc(item))` 得 2/3/4 |
| 种类 4 扫槽 2、3、4 | [确认] | `fn_140012657`：`ePersonEquip = 2; while < 5`；`CMenuPersonEquipOther::vfunc_01` 选项 0..2 映射 `slot+2` |
| 选槽 `fn_140012657(person, item, force)`：kind 2 → 槽 0（空或 `force` 才给）；kind 3 → 槽 1；kind 4 → 槽 2..4 第一个空槽，全满且 `force` 给槽 4；否则返回 **6**（无槽） | [确认] | 指令 `0x1401c86a9 imul rax,rax,0`（槽 0）、`0x1401c86d0 imul ..,1`（槽 1）、`0x1401c872e mov eax,4`、`0x1401c8735 mov eax,6` |
| `SItemOwner` 构造把类型写成 2 / 3 / 4 | [确认] | `SItemOwner.cpp` 三个 ctor：`*(+0x8) = 2/3/4` |
| 人物信息窗为 5 个装备按钮留位 | [确认] | `CPersonInformationView`：`eEquip >= 5` 清 `+0x350` 起的 5 个指针 |
| 装备/卸下有文案 | [确认] | mesfile `0x3623`：「%s装备了%s。解除了%s。」`0x3607`：「让谁装备？」 |
| 效果量是 `UCHAR`，有 min/max 符号 | [确认] | `asserts.txt`：`ITEMEFFECT_AMOUNT_MAX <= sizeof(UCHAR)`；`ITEMEFFECT_AMOUNT_MIN <= data <= MAX` |
| 运行时道具对象（`CItem::vfunc_05` 拷 `SItem`）：`+0x2C` = 记录 `+0x08` 标价，`+0x29` = `+0x14` 种类，`+0x38/+0x39/+0x3A/+0x3B/+0x3C` = `+0x15/+0x16/+0x17/+0x18/+0x19` | [确认] | `0x1402ad18d..0x1402ad202` 逐字节 `mov`；取值函数：标价 `fn_140007423`（读 `+0x2c`）、种类 `fn_14000598e`（`+0x29`）、舱室 `fn_1400033e6`（`+0x39`）、效果量 `fn_14000f452`（`+0x3a`） |
| `+0x17` = 效果量（装备加到技能上的点数），12–80，仅 kind 2/3/4 非 0 | [确认] | `fn_14000fcef` 把 `fn_14000f452` 结果加进技能；道具信息窗 `fn_14000cbda` RTC `rcEffectAmount` 画它 |
| `+0x16` = `eCabinKind`（舱室种类 0–23），`24 = 0x18` = 无；非用具全为 24 | [确认] | `fn_14000fcef` `cmp 0x18`；`fn_14000c7fc` 查 `SCabinKind` 表；信息窗 RTC `rcCabinName` |
| 剑术分支只在 `skill == 5` **且** kind ∈ {2,3} 时走；`skill == 5` 但 kind 4 落到舱室分支 | [确认] | `0x1401c9113..0x1401c9136`：两次 `fn_14000598e` 比 2、3，不中则跳 `0x1401c9150` |
| 舱室种类表 `0x144294b40`（24×`0x18`）由 `0x1400256d0` 静态构造：`SCabinKind{name, u8 skill(+0x10), u16 ?(+0x12)}` | [确认] | `SCabinKind` ctor `fn_14000b7bc`（body `0x140068dc0`）；表见 3.5 |
| 装备加技能 `fn_14000fcef(person, skill, cabin, slot)`（body `0x1401c90a0`），对 5 槽求和（`fn_14000e0b6`）后进结算技能。第三参是人物所在**舱室种类**（`fn_14000e1ce` 给；非玩家势力恒 `0x18`），旧稿叫 `post` | [确认] | 见 4.2；调用方 `fn_1400135cf` |
| 放行函数 `fn_14000f826(_, person_id)`（body `0x1401284a0`）返回 1 当且仅当：① 是主角（`fn_140010f46`）；② **不属玩家势力**（`fn_140013f2f` 假）；③ 是另一支玩家势力舰队的提督（`fn_14000ee26` 扫 69 舰队比 `fn_14001460f`）；④ 是玩家本队 1–4 号船（第 2–5 艘）的船长（`fn_140010c71(p,i)` + `fn_1400056cd` 读船 `+0x50`） | [确认] | 指令 `0x1401284bd..0x140128568`；旧稿「[推断] 舰长/提督」补全 |
| 推论：NPC（非玩家势力）虽然舱室恒 `0x18`，但走②放行，**舱室绑定的用具对 NPC 恒生效** | [确认] | 同上；`03-characters/03` 写「NPC 拿不到舱位装备加成」与此不符，已报告 |
| 原版装备**不改六维** | [推断] | 唯一读 `+0x17` 并加到人身上的是 `fn_14000fcef`；`+0x17` 其余读者是 UI 与排序（`fn_1400020d1` `fn_140014097` RTC `aeItemTable`） |
| `fn_14000f4ed`（body `0x140156360`）是**道具店买入**（含还价），不是卖出 | [确认] | 消息：`0x4f1`「那个得花%s枚金币，你买不起吧？」`0x4f2`「那个要%s枚金币，要买吗？」`0x4f3`「谢谢！」`0x4f4..0x4fc` 还价台词 `0x4fd`「没办法，好吧，就%s枚金币了。」（消息号按 exe 表解码，见 `11-progression/01`） |
| 公式：`price' = base_price * (100 - skill(p,4)/50) / 100`；`skill/50` 有符号 `idiv`，乘后无符号 `div 0x64` | [确认] | `0x14015655b..0x140156584` |
| 参数 4 是 `eSkill` 4 会计术（结算技能，含装备、身体态，封顶 500） | [确认] | `fn_14000e1ce`→`fn_1400135cf`→`fn_140009214`→配对表 `fn_1400099e9` `< 0xa`；全 exe 86 处调用参数 0–9 |
| 余额判断用**未打折**价：`gold < base_price` → `0x4f1` 拒绝 | [确认] | `0x1401564da` `cmp; jb` 在打折之前 |
| 还价人 `p` = `fn_140010cee(fleet, 3)`；`0xec` 表示没有 → 原价成交（`0x4f3`）。岗位 3 = **会计长**：`fn_140010cee` 先 `fn_140007e19(post)` 把岗位转舱室（跳表 `0x14006902c`：3 → 20 会计室），再取该舱的人 | [确认] | `0x14015651d..0x140156534`；`fn_140010cee` body `0x1401de960`；岗位表见 `03-characters/03` |
| 还价成功：还价人轨 B 经验 +10 | [确认] | `0x1401565c5` `fn_140006e88(p, 10, 1)` |
| 折扣上限：技能封顶 500 → 最多 10% 折扣 | [确认] | 500/50 = 10 |
| 船首像效果写在文案，不在人物槽 | [确认] | mesfile：大鹫加快、小猪减饭量、虎鲸避风、白鲸不易沉、龙像炮击、海豚多看见、处女安逸、魔鬼多种效果、王者鼓舞、圣母防瘟疫 |
| 船首像数值在 exe 里按 `CShip+0x53`（船首像 id）字面量比较：0 航速 `+s/10`、7 航速 `×4/5`、1 粮食除数 `+2`、5 搜索半径 `+16`、8 白刃 `×6/5` | 数值 [确认]；`id = item_id − 114` [推断，强] | `06-ocean/01-sailing.md`、`13-algorithms/03-adjustment-factors.md`、`07-combat/02-boarding.md`；五处与 114 大鹫 / 115 小猪 / 119 海豚 / 121 魔鬼 / 122 王者 的文案逐一吻合 |
| `CItemEquipView` 是人物装备界面 | [确认] | `person/CItemEquipView.cpp`，内嵌 `CItemInformationView` |

原版没有修正器类型，装备加成直接写在技能函数里。重构按 D2：items 是 `skill.*` 键的贡献提供方之一，与 vitals（`mul` 段）、characters（`base` 段）并列；消费方只按键取值，不知道有装备这回事。

## 3. 实体与 ID

### 3.1 人物槽 `ePersonEquip`

| slot | key | 原版种类 | 容量 |
| ---: | --- | --- | ---: |
| 0 | `weapon` | kind 2 | 1 |
| 1 | `armor` | kind 3 | 1 |
| 2 | `accessory_0` | kind 4 | 1 |
| 3 | `accessory_1` | kind 4 | 1 |
| 4 | `accessory_2` | kind 4 | 1 |

槽位类型 `Option<item_id>`。原版存档字节 `0xe8`（232）只由导入器映射成 `None`（`ids/legacy_codec.yaml` 的 `item_empty_byte`）。不存 `Item*`、不存 `World*`。

同一 `item_id` 不能同时出现在两个槽；卸下写回舰队唯一物库存。

### 3.2 船首像

不是 `ePersonEquip`。船对象另有一个船首像字段（原版 `CShip+0x53`，初值 `0x0D`；`eFigureHead` 头文件计数未恢复）。原版 12 件 kind=5 的 `eItem` 兼作船首像。海上更换被拒绝（mesfile）。

效果表只有一份：`05-ships-fleet` 的 `ships/figureheads.yaml` 每行带 `item: <item key>` 外键，并在那里写 `effects`（如 `fleet.speed` `mul 11/10`、`supply.food_div` `add 2`、`fleet.search_radius_add` `add 16`），由 ships 作为提供方注册。本系统的 `items.yaml` 对船首像**不写**效果，只登记 kind 与标价，避免两份真相。

### 3.3 效果量

| 符号 | 宽度 | 原版包 |
| --- | --- | --- |
| `ITEMEFFECT_AMOUNT_MIN` | UCHAR | 未在头文件写出整数 |
| `ITEMEFFECT_AMOUNT_MAX` | UCHAR | ≤ 255；表内最大观察到 80（龙角 `+0x17`） |

`caps.yaml` 给设计上限；`engine_limits.yaml` 写死 UCHAR 255。溢出饱和，禁止绕回。

### 3.4 效果操作（重构词汇）

原版按道具 ID 散落。重构只允许下列 op。新 op 必须进 `effects.yaml` 的 `ops` 表，引擎按表解释。

| op | 作用对象 | 原版线索 |
| --- | --- | --- |
| `skill_bonus` | `skill`（技能 key）+ `amount` + `active_when` | 原版唯一的穿戴效果 `[确认]`：武器/防具 → 剑术，用具 → 舱室对应技能（4.2）。语法糖：等价于 `modifier { key: skill.<skill>, stage: add }` |
| `modifier` | `key` + `stage` + `value`（或 `num/den`）+ `active_when` | 原版**无**；给模组向任意已登记键（`ability.*`、`fleet.*`…）贡献。`+0x15` 语义 `[未知]`，不映射到这里 |
| `world_flag` | 旗标 key | 剧情三件、藏宝图 |
| `mark_remains_discovered`（旧名 `unlock_remain`） | `remains`（遗迹 key） | 地点地图；执行 = 发命令 `World.mark_remains_discovered(remains_id, via: item, item_id)`，world 发 `RemainsDiscovered`（词表 §14-23） |
| `mark_village_discovered`（旧名 `unlock_village`） | `village`（村落 key） | 地点地图（岩窟村等）；命令 `World.mark_village_discovered(village_id, via: item, item_id)`，world 发 `VillageDiscovered` |
| `plant` | 无字段 | 发现物 → 特产品。「哪件交易品要哪件发现物」是 economy 交易品目录的外键（`03-discovery-planting.md` 2.5），本表不写目标港 / 交易品 |
| `consume` | 一次性 | 护佑/药剂类 `[推断]` |

禁止在引擎出现 `if item_id == 47: 剑术+57`。船首像效果不在本表（3.2）。

### 3.5 舱室种类 → 技能（原版 `SCabinKind`，`[确认]`）

`0x1400256d0` 静态构造的 24 项；`+0x10` 是该舱室对应的 `eSkill`，11 = 无。`+0x12`（u16）读者未找到（`fn_14000c7fc` 12 个调用点只取名字 `fn_140002077` 与技能 `fn_140009e53`）；数值与官方手册岗位门槛逐项吻合（测量 120、会计室 100、船舵 60、桅杆 80），`[推断]` 是任职所需技能值（ships `cabins.yaml` 字段 `required_skill`）。

| cabin | 名 | skill | `+0x12` |
| ---: | --- | --- | ---: |
| 0 | 桅杆 | 2 驾驶术 | 80 |
| 1 | 了望台 | 9 观察力 | 70 |
| 2 | 甲板 | 0 生命力 | 30 |
| 3 | 测量 | 3 测量术 | 120 |
| 4 | 船舵 | 2 驾驶术 | 60 |
| 5 | 舰长室 | 1 统率力 | 30 |
| 6 | 副官室 | 7 说服力 | 100 |
| 7 | 水兵室 | 5 剑术 | 90 |
| 8 | 武装炮台 | 6 命中力 | 90 |
| 9–12 | 物资仓库 / 货物仓库 / 划桨手室 / 单人房 | 11 无 | 0 |
| 13 | 木材室 | 3 测量术 | 130 |
| 14 | 医务室 | 9 观察力 | 150 |
| 15 | 厨房 | 6 命中力 | 70 |
| 16 | 娱乐室 | 11 无 | 0 |
| 17 | 饲养室 | 0 生命力 | 130 |
| 18 | 礼拜室 | 7 说服力 | 160 |
| 19 | 参谋室 | 8 谋略术 | 120 |
| 20 | 会计室 | 4 会计术 | 100 |
| 21–23 | 追加1 / 追加2 / 无 | 11 无 | 0 |

表权属 `ships`（舱室）；本系统只读 `cabin → skill`。医务室 → 观察力、木材室 → 测量术是原版数据，照搬，不要按名字「纠正」。`03-characters/03` 的岗位「主要技能」列与本表经 `fn_140012dd2`（舱→岗位）逐项一致，可由此升 `[确认]`。

## 4. 算法 / 公式

所有魔数进 YAML。符号与原版立即数对照写在注释里。

### 4.1 道具店买入还价（vanilla 参数）

来源：`fn_14000f4ed`（body `0x140156360`）指令，`[确认]`。这是**买入**，不是卖出；原版宝物卖出价未在本函数。

```
if gold < base_price:                       # 先用原价判余额
    reject(cannot_afford)                   # msg 0x4f1
confirm(base_price)                         # msg 0x4f2
p = Fleets.post_holder(fleet, refs.post)             # 原版 fn_140010cee(fleet, 3) = 会计长；无人 → 原价
if p is none:
    pay(base_price)                         # msg 0x4f3
else:
    s = Modifiers.eval("skill." + refs.skill, {person: p})      # 结算技能，已封顶 skill_value_cap
    factor = price_base - s / skill_divisor              # 有符号整除
    price  = base_price * factor / price_scale           # 无符号整除
    pay(price)                                           # 还价台词随机 + msg 0x4fd
    emit ItemBargained { item, person: p, base_price, price }   # progression 按 grant_routes 给经验（原版轨 B +10）
```

原版内容包默认（`config/formulas/items.yaml` → `item_bargain`）：

| 参数 | vanilla | 依据 |
| --- | ---: | --- |
| `refs.skill`（旧名 `bargain_skill`） | `vanilla:accounting`（eSkill 4） | `fn_14000e1ce(p, 4)` → 技能链，`[确认]` |
| `skill_divisor` | 50 | `0x140156561 mov ecx,0x32; idiv` |
| `price_base` | 100 | `0x140156568 mov ecx,0x64; sub` |
| `price_scale` | 100 | `0x14015657f mov ecx,0x64; div`，`[确认]` |
| `price_factor_floor` | 0 | 重构护栏；原版靠技能封顶 500 保证 factor ≥ 90 |
| `affordability_check` | `before_discount` | 原版先比原价 |
| `refs.post`（旧名 `bargain_post`） | `vanilla:purser`（ePost 3） | `fn_140010cee(..., 3)` → 会计室 20，`[确认]` |

经验不在本表：`fn_140006e88(p, 10, 1)` 的「轨 B +10」写在 `formulas/progression.yaml` 的 `grant_routes`（事件 `ItemBargained`），见 `11-progression/01` 4.5。

`base_price` 来自 `items.yaml`，不是公式文件。物品系统只按键 `skill.accounting` 求值，不读六维，不自己夹 500，也不调 progression。

旧文的「`stat_index` 是精神还是技能」「`price_scale` 推断」均已由指令钉死；`exe_caps.md` 把本式归为「能力 ≤ 500」是把技能误称为能力。

### 4.2 装备 → 技能加成

原版 `fn_14000fcef(person, skill, cabin, slot)`，`[确认]`：

```
item = person.equip[slot]
if item == item_empty_slot: return 0
bonus = 0
if skill == sword_skill(5) and item.kind in (weapon 2, armor 3):
    bonus += item.amount                                   # +0x17，不看舱室
else if item.cabin != cabin_none(24) and cabin[item.cabin].skill == skill:
    if cabin == captain_cabin(5)                           # 穿戴者在舰长室
       or fn_14000f826(_, person)                          # 主角 / 非玩家势力 / 地方舰队提督 / 本队 2–5 号船船长
       or cabin == item.cabin:                             # 穿戴者就在该舱室
        bonus += item.amount
return bonus
```

`cabin` 由 `fn_14000e1ce` 给：人物属玩家势力时是其所在舱室，否则 `0x18`（无）；但非玩家势力人物走 `fn_14000f826` ② 放行，所以舱室绑定用具对 NPC 恒生效。

5 槽之和加到**基础技能**上，再乘身体态系数并封顶 500。效果量本身不另外夹 `ITEMEFFECT_AMOUNT_*`（原版直接取字节）。

重构出口（D2：按 key 注册的带来源贡献）：

```
# items 在加载时向注册表登记一个提供方
Modifiers.register_provider("items.equipment", keys: "skill.*", stage: add, order: equip_skill_bonus.order)

# 求值 skill.<k> 时注册表回调提供方，ctx 只含 person_id
Items.contribute(key = "skill.<k>", ctx) -> [Contribution]
  for slot in slot_groups[*].slots:      # 槽数 = items/effects.yaml slot_groups 的槽总数（原版 5）
    defn = Items.get(live_equip(ctx.person, slot)); if none: continue
    for e in defn.effects where e.op in (skill_bonus, modifier) and e.key == key:
      if active(e.active_when, ctx.person, defn):
        yield { key, stage: add, value: e.amount, order, source: item:<defn.key>@slot }
```

`active(...)` 只调一级查询：`Ships.cabin_of(person)`（所在舱室种类）、`Fleets.post_holder` / `Fleets.is_commander(person)`（D2「岗位持有人」）、`Characters.is_player_force(person)`；不读原版舱号字面量。

`active_when` 取值：`always`（原版武器/防具对剑术）、`in_cabin_or_command`（原版用具：同舱室 / 舰长室 / 放行函数）。放行规则的组成写在 `formulas/items.yaml` 的 `equip_skill_bonus.command_check`（4 项可单独关），原版 kind→规则的映射写进 `items.yaml` 的效果实例，不写进引擎。

来源 `item:<key>@slot` 让 UI 能解释「会计术 +54 来自金币（槽 2）」，卸下时按来源撤销。

### 4.3 穿上 / 卸下

```
equip(person_id, slot, item_id):
    1. 舰队必须 owned(item_id) 或该物已在本人物另一槽（换位）
    2. kind 必须匹配 slot.allowed_kinds
    3. 目标槽旧 item_id != empty → 退回舰队库存
    4. 扣库存 / 清旧槽
    5. 写新槽
    6. emit PersonEquipped { person_id, slot, item_id }   # 注册表据此失效该人物 skill.* 缓存

unequip: 对称，槽写 empty_slot。
```

事务顺序：先检、再扣源、再写槽。禁止先写槽后发现未持有。

### 4.4 船首像

```
Yard.equip_figurehead(ship_id, item_id):           # ships 执行（00-overview/06）；本系统只确认持有、移交库存
    要求 kind.equip_target == ship_figurehead
    要求不在海上（原版文案）
    旧像退回舰队库存
    ship.figurehead_item_id = item_id
    emit FigureheadChanged { ship_id, item_id }    # ships 发
```

船首像效果由 ships 按 `figureheads.yaml` 注册 `fleet.*` / `supply.*` 贡献（3.2），人物技能不掺船速。

## 5. 耦合分析

原版：

- 道具店买入直接叫 `fn_14000e1ce(person, 4)` 并就地加经验，商店函数认识人物、技能、经验三套布局
- `fn_140012657` 把 kind 2/3/4 写死成槽规则（含返回 6 的「无槽」哨兵）
- 技能函数 `fn_14000fcef` 写死「技能 5 + kind 2/3」与「舰长室 = 5」，放行函数 `fn_14000f826` 自己扫 69 支舰队和本队船长
- 船首像效果在航海 / 补给 / 搜索 / 白刃四处各比一次 `CShip+0x53` 字面量
- 舱室→技能表在 exe 静态构造，道具 `+0x16` 直接当它的下标
- 船首像、人物用具、地图共用 `eItem`，设施代码按名字分支
- `CItemEquipView` / `CMenuPersonEquipOther` 既画 UI 又改槽

重构：

```
Modifiers  --contribute(skill.*)--> items          # items 是提供方，被动回调
items      --query cabin_of / is_commander--> ships / fleets
items      --eval skill.accounting--> Modifiers     # 还价
items      --ItemBargained--> progression           # 经验由 grant_routes 决定
ships      --figureheads.yaml item 外键--> items（只读定义）
items      --World.mark_remains_discovered / mark_village_discovered(id, via: item, item_id)--> world   # 命令；world 发 *Discovered
items      --PlantSpecialProduct--> economy
```

禁止反向：combat 不改装备；economy 不改 `eItem` 库存；progression 不查 items。

## 6. 解耦后的配置

```yaml
# config/caps.yaml  片段
# 空槽不进 caps：运行时 Option；原版 0xe8 写在导入器配置 ids/legacy_codec.yaml: item_empty_byte: 232
# 装备槽数不进 caps：只读 items/effects.yaml 的 slot_groups 槽总数（原版 5，fn_14000e0b6）；旧 person_equip_slots 作废
item_unique_stack: 1
item_effect_amount_min: 0      # 符号 ITEMEFFECT_AMOUNT_MIN，整数未还原前先 0；仅导入校验用
item_effect_amount_max: 255    # UCHAR 硬顶；原版加成时不夹取，只是数据宽度
```

```yaml
# config/formulas/items.yaml  片段（原 character.yaml；公式归 items 自己）
# 形状 = 14-config-schemas README「公式层」（D3）：impl / int_mode / quirks / params / tables / refs / golden
schema: dk4.formulas.items
version: 2
item_bargain:                   # F-ECO-02：买入时会计长还价，原版最多 9 折
  impl: expr
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }        # 乘后 div 0x64 无符号
  params:
    skill_divisor: 50
    price_base: 100
    price_scale: 100              # [确认] div 0x64
    price_factor_floor: 0
    affordability_check: before_discount
  refs: { skill: vanilla:accounting, post: vanilla:purser }   # eSkill 4 / ePost 3 [确认]；会计长不在岗则不打折
  inputs: [price, ability]      # ability = Modifiers.eval("skill.accounting", {person: purser})
  expr: "price * max(price_factor_floor, price_base - ability / skill_divisor) udiv price_scale"
  golden: [{ price: 1000, ability: 500, out: 900 }]
  modifiers: trade.item_shop_price
  # 经验见 progression grant_routes（ItemBargained，轨 B +10）
equip_skill_bonus:              # 装备 → skill.<key> 的 add 贡献（D2）
  impl: vanilla.equip_skill_bonus
  params:
    stage: add                  # D2 堆叠段；base 由 characters 给
    order: 100                  # 同段内排序；vitals 在 mul 段，不冲突
    armament_kinds: [vanilla:weapon, vanilla:armor]
  refs:
    armament_skill: vanilla:fencing         # 原版写死 eSkill 5
    command_cabins: [vanilla:captain_room]  # 原版写死 cabin 5
  tables:
    command_check:                        # fn_14000f826 [确认]，四项任一为真即放行
      protagonist: true
      non_player_force: true              # NPC 恒放行（原版行为）
      detached_fleet_admiral: true        # 另一支玩家舰队的提督
      escort_ship_captain: true           # 本队第 2–5 艘船长
```

```yaml
# config/items/effects.yaml
schema: dk4.items.effects
version: 1
ops:
  - key: skill_bonus
    fields: [skill, amount, active_when]      # active_when: always | in_cabin_or_command
    expands_to: { op: modifier, key: "skill.{skill}", stage: add }
  - key: modifier               # 原版无；模组向任意已登记键贡献
    fields: [key, stage, value, active_when]
  - key: world_flag
    fields: [flag]
  - key: mark_remains_discovered   # 旧名 unlock_remain；执行 = World.mark_remains_discovered(…, via: item, item_id)
    fields: [remains]
  - key: mark_village_discovered   # 旧名 unlock_village
    fields: [village]
  - key: plant                  # 目标交易品在 economy 目录外键，不在这里
    fields: []
  - key: consume
    fields: []

slot_groups:
  - key: vanilla:weapon
    slots: [0]
    allowed_kinds: [vanilla:weapon]
  - key: vanilla:armor
    slots: [1]
    allowed_kinds: [vanilla:armor]
  - key: vanilla:accessory
    slots: [2, 3, 4]
    allowed_kinds: [vanilla:person_gear]
```

```yaml
# config/items/items.yaml  效果实例（原版包，amount = +0x17 [确认]）
- key: vanilla:excalibur
  vanilla_index: 47
  kind: vanilla:weapon
  base_price: 250000
  effects:
    - op: skill_bonus
      skill: vanilla:fencing            # 武器/防具一律剑术
      amount: 57
      active_when: always

- key: vanilla:milky_way_chart
  vanilla_index: 76
  kind: vanilla:person_gear
  base_price: 50000
  cabin: vanilla:crows_nest            # +0x16 = 1 了望台（ships 的舱室种类 key）
  effects:
    - op: skill_bonus
      skill: vanilla:observation        # 由 cabin 表派生（了望台 → 9 观察力），导入器展开
      amount: 32                # +0x17
      active_when: in_cabin_or_command

- key: vanilla:pumpkin_seed
  vanilla_index: 0
  kind: vanilla:plant_discovery
  base_price: 2000
  effects:
    - op: plant                 # 对应交易品（南瓜）由 economy 目录 unlock_item 指回本行
```

`+0x16` 是舱室种类（3.5 表），技能由舱室表派生。导入器把 `cabin` 与派生的 `skill` 都写出来；模组可以只改 `skill` 让用具脱离舱室。原版用具实际用到的舱室：0–8、13–15、17–20（9–12、16 是仓库/娱乐等无技能舱室）。

## 7. 独立旋钮

| 旋钮 | 文件 | 改它不必动 |
| --- | --- | --- |
| 槽数、每组 allowed_kinds | `effects.yaml` `slot_groups` | 人物 UI 按表长画格 |
| `item_empty_byte`（仅导入原版） | `ids/legacy_codec.yaml` | 运行时 Option |
| 还价 100、50、scale、余额判定时机 | `formulas/items.yaml` `item_bargain` | 商店 UI |
| `refs.skill` / `refs.post`（旧名 `bargain_skill` / `bargain_post`） | 同上 | 技能公式、岗位表 |
| 还价经验 | `formulas/progression.yaml` `grant_routes` | 本系统 |
| `refs.armament_skill` / `refs.command_cabins` / `tables.command_check.*` / `params.stage` / `params.order` | `formulas/items.yaml` `equip_skill_bonus` | 技能公式、修正器注册表 |
| 效果量上下限 | `caps.yaml` | 只做导入校验 |
| 某剑的 amount、某用具的 skill/cabin | `items.yaml` | 战斗、航海只读结算技能 |
| 船首像修正 | `ships/figureheads.yaml`（ships 拥有） | 本系统、航海积分公式 |
| 地图解锁目标 | `items.yaml` | world 执行命令并发 `*Discovered` |

原版「5 槽、3 件用具、不堆叠」全部是内容包默认，不是引擎常量。

## 8. 对外接口

查询：

- `Equip.slots(person_id) -> [item_id; 5]` LIVE 取，人已离队则空数组
- `Equip.get(person_id, slot) -> item_id | empty`
- `Items.bargain_quote(item_id, bargainer_person_id | none) -> gold`（读 `Modifiers.eval("skill.accounting")`）

修正器提供方（不是给业务系统直接调的查询）：

- `Items.contribute(key, ctx) -> [Contribution]`：只回应 `skill.*` 与内容包里 `modifier` op 声明过的键；注册表调用，业务系统禁止直调

不再定义写死字段的 `ModifierSet`，也不再提供 `equip_skill_bonus` 查询。模组加 `modifier` op 向 `ability.*` 等键贡献时，引擎不改代码。

命令：

- `Equip.on(person_id, slot, item_id)`
- `Equip.off(person_id, slot, item_id)`
- 船首像装卸不在本系统：走 ships `Yard.*`（船首像），本系统只提供道具定义
- `UseItem { fleet_id, item_id, target }`（非穿戴效果）

事件：

- `PersonEquipped { person_id, slot, item_id }`
- `PersonUnequipped { person_id, slot, item_id }`
- `ItemBargained { item, person, base_price, price }`（progression 订阅给经验）
- （`FigureheadChanged` 由 ships 发，见 4.4，不是本系统事件）

存档：每个 `person_id` 存 5 个 `item_id`（或 empty）。不存派生技能。

闭包/UI 回调只捕获 `person_id` + `item_id`，执行时 LIVE 取；人已不在则 no-op。

## 9. 未核实字段

| 项 | 状态 | 说明 |
| --- | --- | --- |
| 武器槽、防具槽的精确下标 | [确认] | `fn_140012657`：kind 2 → 0，kind 3 → 1，kind 4 → 2..4 |
| `+0x15` | [未知] | 0–8；kind 8 遗迹宝物上 0–6 各两件，像文化圈 / 套装序号；未见读取它的玩法函数（只拷到运行时 `+0x38`） |
| `+0x12`（`SCabinKind`，u16，30–160） | [推断] | 与手册岗位门槛逐项吻合；读者未找到 |
| `fn_14000f826(_, person)` | [确认] | 四项放行，见 §2；`fn_14000ee26` 的「另一支舰队提督」语义按调用形 `[推断]` |
| `fn_140010cee(fleet, 3)` 选还价人的岗位 3 | [确认] | 会计长：岗位→舱室跳表 3 → 20 会计室 |
| `ITEMEFFECT_AMOUNT_MIN/MAX` 的原版整数 | [未知] | 只有 UCHAR 宽度断言 |
| 船首像数值系数 | 五件已知（§2） | 其余 5 件（虎鲸、白鲸、龙、处女、圣母）及 199 怪鱼像、200 提督像的效果与 `CShip+0x53` 对应 `[未知]`；表归 ships |
| 发现物对应哪条 `eTradingItem` | [确认] | 交易品目录 +11 反向外键，见 `03-discovery-planting.md` 2.5；不在 `SItem` |
| 地点地图 → `eRemains`/`eVillage` 对照表 | [未知] | `+0x18` 是城镇，不是遗迹下标 |
| `eItemTag = 7` | [未知] | 卖出过滤初值；与 `NUM_SELECTITEMTAG=6` 不是同一枚举 |

未还原前：YAML 保留 `unknown_bytes`，效果列表可以空。禁止为了「看起来完整」编造 +10 魅力之类的数。
