# 装备槽与效果

配置落点：`config/items/effects.yaml` `config/formulas/character.yaml`（卖价参数）`caps.yaml`。目录与种类见 [01-item-catalog.md](01-item-catalog.md)。

本系统把「穿上某件 `eItem`」变成 `ModifierSet` 增量。人物、战斗、航海、贸易只读修正器，不读六维原值，也不 `switch (item_id)`。

## 1. 职责边界

管：

- 人物 5 个装备槽的定义与合法性
- 效果操作表：`stat_mod` / `skill_mod` / `world_flag` / `unlock_remain` / `plant_in_village` / `ship_mod` / …
- 把已装备道具折叠进 `ModifierSet`
- 宝物卖价公式的 **参数**（原版 `100 - a/50`）
- 效果量上下限（`ITEMEFFECT_AMOUNT_*`）

不管：

- 道具有没有、叫什么（目录）
- 技能怎么从经验派生（`11-progression`）
- 海战伤害、接舷（`07-combat` 读 `battle_offense` 等）
- 买卖货物折扣（那是 `buy_cost_factor`，另一条公式）
- 船首像改变航速的具体航海积分（`06-ocean` / `05-ships-fleet` 读 `ship_mod`）
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
| 种类 2/3 各对一条「是否允许覆盖」分支，槽下标在 +0x23 附近 | [推断] | 同函数 `s44==2` / `==3` 分支读 `*(s60+(1*1)+0x23)`，反编译下标含糊，但槽数=5 已确认 |
| `SItemOwner` 构造把类型写成 2 / 3 / 4 | [确认] | `SItemOwner.cpp` 三个 ctor：`*(+0x8) = 2/3/4` |
| 人物信息窗为 5 个装备按钮留位 | [确认] | `CPersonInformationView`：`eEquip >= 5` 清 `+0x350` 起的 5 个指针 |
| 装备/卸下有文案 | [确认] | mesfile `0x3623`：「%s装备了%s。解除了%s。」`0x3607`：「让谁装备？」 |
| 效果量是 `UCHAR`，有 min/max 符号 | [确认] | `asserts.txt`：`ITEMEFFECT_AMOUNT_MAX <= sizeof(UCHAR)`；`ITEMEFFECT_AMOUNT_MIN <= data <= MAX` |
| 世界表 `+0x17` 仅在武器/防具/用具非 0（12–80） | [推断] | 全表统计；与 `ITEMEFFECT_AMOUNT` 同宽 |
| 用具 `+0x16` 分组与岗位/技能文案对应，但取值到 20，超过 `NUM_SKILL=10` | [推断] | 天河星图=1、罗盘=3、战鼓=7、锯凿刨=13、医书=14、锅=15、口罩=17、圣经=18、兵法=19、算盘=20 |
| 卖价函数 `fn_14000f4ed`（body `0x140156360`） | [确认] | `unassigned_named.tsv` RTC：`eItemTag, nItemTotal, aeItemTable` |
| 公式形态 `100 - a/50` | [确认] | `srcdata/docs/exe_caps.md`；反编译：`s430 = s430 * (0x64 - rax)`，`rax` 来自 `fn_14000e1ce(person, 4)` 后 `idiv` |
| `fn_14000e1ce` 在技能面板对 `eSkill 0..9` 调用 | [确认] | `fn_140148480.cpp`、`fn_1401cb7f0.cpp` |
| 同函数被 exe_caps 归为「能力结算 ≤ 500」一类 | [确认] | `exe_caps.md` 把买货、战斗、道具价并列 |
| 参数 `4` 究竟是 `eAbility` 精神还是 `eSkill` 4 | [未知] | 同一 thunk、两种调用约定 |
| 乘完之后还有 `div ecx` | [推断] | 反编译同时留下 `idiv` 与 `div`；按百分数惯例 `price_scale=100` |
| 船首像效果写在文案，不在人物槽 | [确认] | mesfile：大鹫加快、小猪减饭量、虎鲸避风、白鲸不易沉、龙像炮击、海豚多看见、处女安逸、魔鬼多种效果、王者鼓舞、圣母防瘟疫 |
| `CItemEquipView` 是人物装备界面 | [确认] | `person/CItemEquipView.cpp`，内嵌 `CItemInformationView` |

原版没有名为 `ModifierSet` 的类型。那是重构出口，用来拆掉「魅力/智力直接进买货、战斗、道具价」。

## 3. 实体与 ID

### 3.1 人物槽 `ePersonEquip`

| slot | key | 原版种类 | 容量 |
| ---: | --- | --- | ---: |
| 0 | `weapon` | kind 2 | 1 |
| 1 | `armor` | kind 3 | 1 |
| 2 | `accessory_0` | kind 4 | 1 |
| 3 | `accessory_1` | kind 4 | 1 |
| 4 | `accessory_2` | kind 4 | 1 |

槽位存 `item_id` 或 `caps.item_empty_slot`（232）。不存 `Item*`、不存 `World*`。

同一 `item_id` 不能同时出现在两个槽；卸下写回舰队唯一物库存。

### 3.2 船首像

不是 `ePersonEquip`。船对象另有一个 `figurehead_item_id`（或 `eFigureHead`，头文件计数未恢复）。原版 12 件 kind=5 的 `eItem` 兼作船首像。海上更换被拒绝（mesfile）。

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
| `stat_mod` | `ability_id` + `amount` | `+0x15` 0–5 像 `eAbility` `[推断]` |
| `skill_mod` | `track_id` + `amount` | 用具 `+0x16` 分组 `[推断]`；**不是**直接 `eSkill` |
| `combat_power` | 攻或防 `amount` | 武器/防具 `+0x17` `[推断]` |
| `ship_mod` | 船修正器 key | 船首像文案 |
| `world_flag` | 旗标 key | 剧情三件、藏宝图 |
| `unlock_remain` | `remain_id` | 地点地图 |
| `unlock_village` | `village_id` | 地点地图（岩窟村等） |
| `plant_in_village` | `town_id`/`village_id` + `trading_item_id` | 发现物 → 特产品 |
| `consume` | 一次性 | 护佑/药剂类 `[推断]` |

禁止在引擎出现 `if item_id == 47: 攻击+57`。

## 4. 算法 / 公式

所有魔数进 YAML。符号与原版立即数对照写在注释里。

### 4.1 卖价（vanilla 参数）

来源：`exe_caps.md` 的 `fn_14000f4ed`（`100 - a/50`）。

```
a = person_stat(seller_person_id, stat_index)
a = clamp(a, 0, settled_stat_cap)

raw_factor = price_base - floor(a / ability_divisor)
raw_factor = max(raw_factor, price_factor_floor)

quoted = floor(base_price * raw_factor / price_scale)
quoted = max(quoted, price_min)
```

原版内容包默认（`config/formulas/character.yaml` → `item_price`）：

| 参数 | vanilla | 依据 |
| --- | ---: | --- |
| `stat_index` | 4 | `fn_14000e1ce(..., 4)`。4 是 `eAbility` 精神还是 `eSkill` 4：`[未知]` |
| `ability_divisor` | 50 | exe_caps `a/50` |
| `price_base` | 100 | exe_caps `100 - …`；立即数 `0x64` |
| `price_scale` | 100 | `[推断]` 自随后 `div ecx`；若实现发现没有这步，把 vanilla 改成 1 并升 schema |
| `price_factor_floor` | 0 | 防 `a` 大于 `price_base * ability_divisor` 时变负 |
| `settled_stat_cap` | 500 | exe_caps 把本公式与能力结算 500 归为一类；若 `stat_index` 实为技能则改用 `skill_cap=200` |
| `price_min` | 1 | `[推断]` 未在断言里见到 0 元宝物 |

`base_price` 来自 `items.yaml`，不是公式文件。

人物系统把这条公式的结果写成 `ModifierSet.item_price_factor = raw_factor / price_scale`。物品系统卖出时：

```
quoted = floor(Items.base_price(item_id) * ModifierSet.item_price_factor)
```

物品代码不读六维。改「4 号数值」的含义只改人物侧 `person_stat`，不改卖出代码。

金币不足时 `fn_14000f4ed` 走消息 `0x4f1` 一类分支，不扣物。重构发 `ItemSaleRejected { reason: cannot_afford }`。

### 4.2 装备 → ModifierSet

人物在「槽位变化 / 升级 / 读档」时重建，不缓存过期增量。

```
function rebuild_modifiers(person_id):
    m = zero_modifier_set()
    m += progression.modifiers(person_id)          # 等级、技能派生
    for slot in 0 .. len(person_equip_slots)-1:
        item_id = live_equip(person_id, slot)
        if item_id == caps.item_empty_slot: continue
        defn = Items.get(item_id)
        if defn is none: continue                  # 表被删，整槽 no-op
        if defn.kind not in slot.allowed_kinds:
            continue                               # 数据坏了也不崩溃
        for e in defn.effects:
            m = apply_effect(m, e)
    return m
```

`apply_effect` 只认识 `ops` 表里的 op：

```
stat_mod:     m.ability[ability_id] += amount
skill_mod:    m.skill_track[track_id] += amount
combat_power: m.battle_offense 或 m.battle_defense += amount
ship_mod:     不写进人；发给船系统
其余 op:      不在穿戴路径执行（地图/种植走 UseItem）
```

`amount` 先 clamp 到 `[ITEMEFFECT_AMOUNT_MIN, ITEMEFFECT_AMOUNT_MAX]`。

原版 `fn_1401c87a0` 对某个 1–5 的档做乘法/除 2/平方，像是效果量的档位曲线 `[未知]`，未进 vanilla 公式。确认前不要实现。

### 4.3 穿上 / 卸下

```
equip(person_id, slot, item_id):
    1. 舰队必须 owned(item_id) 或该物已在本人物另一槽（换位）
    2. kind 必须匹配 slot.allowed_kinds
    3. 目标槽旧 item_id != empty → 退回舰队库存
    4. 扣库存 / 清旧槽
    5. 写新槽
    6. rebuild_modifiers(person_id)
    7. emit PersonEquipped { person_id, slot, item_id }

unequip: 对称，槽写 empty_slot。
```

事务顺序：先检、再扣源、再写槽。禁止先写槽后发现未持有。

### 4.4 船首像

```
equip_figurehead(ship_id, item_id):
    要求 kind.equip_target == ship_figurehead
    要求不在海上（原版文案）
    旧像退回舰队库存
    ship.figurehead_item_id = item_id
    emit FigureheadChanged { ship_id, item_id }
```

船/航海系统把 `ship_mod` 折进自己的修正器，人物 `ModifierSet` 不掺船速。

## 5. 耦合分析

原版：

- 卖价直接叫 `fn_14000e1ce(person, 4)`，物品函数认识人物布局
- `fn_140012657` 把 kind 2/3/4 写死成槽规则
- 船首像、人物用具、地图共用 `eItem`，设施代码按名字分支
- 技能面板和卖价共用 `fn_14000e1ce`，参数语义缠在一起
- `CItemEquipView` / `CMenuPersonEquipOther` 既画 UI 又改槽

重构：

```
items  --ItemDef, effects-->  characters.rebuild_modifiers
characters --ModifierSet.item_price_factor--> items.quote_sale
characters --ModifierSet.battle_*--> combat
items --ship_mod event--> ships
items --UnlockRemain--> world/ocean
items --PlantSpecialProduct--> economy
```

禁止反向：combat 不改装备；economy 不改 `eItem` 库存。

## 6. 解耦后的配置

```yaml
# config/caps.yaml  片段
item_empty_slot: 232
person_equip_slots: 5          # 原版包；引擎读表长
item_unique_stack: 1
item_effect_amount_min: 0      # 符号 ITEMEFFECT_AMOUNT_MIN，整数未还原前先 0
item_effect_amount_max: 255    # UCHAR 硬顶；设计值可再降
```

```yaml
# config/formulas/character.yaml  片段
schema: dk4.formulas.character
version: 1
item_price:
  stat_index: 4                 # [未知] ability vs skill
  ability_divisor: 50
  price_base: 100
  price_scale: 100              # [推断]
  price_factor_floor: 0
  settled_stat_cap: 500
  price_min: 1
```

```yaml
# config/items/effects.yaml
schema: dk4.items.effects
version: 1
ops:
  - key: stat_mod
    fields: [ability_id, amount]
  - key: skill_mod
    fields: [track_id, amount]
  - key: combat_power
    fields: [axis, amount]      # axis: offense | defense
  - key: ship_mod
    fields: [modifier, amount]
  - key: world_flag
    fields: [flag]
  - key: unlock_remain
    fields: [remain_id]
  - key: unlock_village
    fields: [village_id]
  - key: plant_in_village
    fields: [town_id, village_id, trading_item_id]
  - key: consume
    fields: []

slot_groups:
  - key: weapon
    slots: [0]
    allowed_kinds: [2]
  - key: armor
    slots: [1]
    allowed_kinds: [3]
  - key: accessory
    slots: [2, 3, 4]
    allowed_kinds: [4]
```

```yaml
# config/items/items.yaml  效果实例（原版包，amount 来自 +0x17 [推断]）
- id: 47
  key: excalibur
  kind: 2
  base_price: 250000
  effects:
    - op: combat_power
      axis: offense
      amount: 57

- id: 76
  key: milky_way_chart
  kind: 4
  base_price: 50000
  effects:
    - op: skill_mod
      track_id: 1               # +0x16=1 [未知] 岗位/测量轨
      amount: 32                # +0x17

- id: 0
  key: pumpkin_seed
  kind: 0
  base_price: 2000
  effects:
    - op: plant_in_village
      trading_item_id: null     # 对应交易品未在本表钉死
```

`track_id` 在原版包里先按 `+0x16` 原样保存。对照（`[推断]`，供策划改名，不是引擎分支）：

| track_id | 用具例子 |
| ---: | --- |
| 0 | 魔法皮手套、精灵粗绳、红鹤羽毛鞋 |
| 1 | 天河星图、星座天体仪、望远镜 |
| 2 | 贺拉提斯诗集、天方夜谭、真红指环 |
| 3 | 黄金圆规、怀表、罗盘、六分仪 |
| 4 | 洛克的驾船指南书、黑玉髓 |
| 5 | 天佑手镯、导善手杖、战士小鹅笛 |
| 6 | 能言善辩的鸟嘴、东方见闻录 |
| 7 | 战鼓、波塞冬的怒吼、高卢战记 |
| 8 | 奇迹的炸弹项链、精制火药秘笺、射穿魔鬼的箭 |
| 13 | 狮牙锯、菲狄亚斯的凿子、刨子 |
| 14 | 医书、达·芬奇解剖图、龙角 |
| 15 | 赫斯提亚的锅、粉红色的围裙 |
| 17 | 带纱布的口罩、考究的长统鞋 |
| 18 | 格里高利的皇冠、圣经、古代十字架 |
| 19 | 孙子兵法、汉尼拔战记 |
| 20 | 纪伊国屋的算盘、维萨斯的天秤、金币 |

缺 9–12、16。命名之后升 `effects.yaml` version，旧包只读数字。

## 7. 独立旋钮

| 旋钮 | 文件 | 改它不必动 |
| --- | --- | --- |
| 槽数、每组 allowed_kinds | `effects.yaml` `slot_groups` | 人物 UI 按表长画格 |
| `item_empty_slot` | `caps.yaml` | 存档 schema 用同一 cap |
| 卖价 100、50、scale | `formulas/character.yaml` | 物品卖出只乘 factor |
| `stat_index` | 同上 | 人物 `person_stat` |
| 效果量上下限 | `caps.yaml` | clamp 在 apply_effect |
| 某剑的 amount | `items.yaml` | 战斗只读 ModifierSet |
| 船首像修正 | `items.yaml` `ship_mod` | 航海积分公式 |
| 地图解锁目标 | `items.yaml` | world/ocean 订阅事件 |

原版「5 槽、3 件用具、不堆叠」全部是内容包默认，不是引擎常量。

## 8. 对外接口

查询：

- `Equip.slots(person_id) -> [item_id; 5]` LIVE 取，人已离队则空数组
- `Equip.get(person_id, slot) -> item_id | empty`
- `Characters.modifiers(person_id) -> ModifierSet`
- `Items.quote_sale(item_id, person_id) -> gold` 内部只读 `base_price` 与 `item_price_factor`

`ModifierSet` 最小字段（人物系统拥有，物品往里填）：

```
ModifierSet = {
  buy_cost_factor,
  sell_price_factor,
  item_price_factor,     # 宝物卖价
  battle_offense,
  battle_defense,
  ability[6],
  skill_track[],         # 长度 = 效果轨表
  ...
}
```

命令：

- `EquipOn { person_id, slot, item_id }`
- `EquipOff { person_id, slot }`
- `EquipFigurehead { ship_id, item_id }`
- `UseItem { fleet_id, item_id, target }`（非穿戴效果）

事件：

- `PersonEquipped { person_id, slot, item_id }`
- `PersonUnequipped { person_id, slot, item_id }`
- `ModifiersRebuilt { person_id }`
- `FigureheadChanged { ship_id, item_id }`

存档：每个 `person_id` 存 5 个 `item_id`（或 empty）。不存派生 ModifierSet。读档后 `rebuild_modifiers`。

闭包/UI 回调只捕获 `person_id` + `item_id`，执行时 LIVE 取；人已不在则 no-op。

## 9. 未核实字段

| 项 | 状态 | 说明 |
| --- | --- | --- |
| `fn_14000e1ce(..., 4)` 的 4 | [未知] | 技能面板当 `eSkill`；exe_caps 当能力。vanilla 先原样保留 `stat_index: 4` |
| `price_scale=100` | [推断] | 依赖 `div ecx`；需对照一次实机卖价 |
| 武器槽、防具槽的精确下标 | [推断] | 5 槽、用具占 2–4 已确认；0/1 与 kind 2/3 的一一对应来自分流结构 |
| `+0x15` 作为 `ability_id` | [未知] | 用具/武器上 0–5 像六维；kind 8/9 上 0–6 像套装序号 |
| `+0x16` 作为 `track_id` | [未知] | 与岗位文案相符但越出 `NUM_SKILL` |
| `+0x17` 作为 `amount` | [推断] | 仅装备类非 0；未看到读取该字节的还原函数名 |
| `ITEMEFFECT_AMOUNT_MIN/MAX` 的原版整数 | [未知] | 只有 UCHAR 宽度断言 |
| `fn_1401c87a0` 档位曲线 | [未知] | 对 1–5 做乘、半、平方 |
| 船首像数值系数 | [未知] | 只有 mesfile 定性描述 |
| 发现物对应哪条 `eTradingItem` | [未知] | 种植事件在叙事包，不在 `SItem` |
| 地点地图 → `eRemains`/`eVillage` 对照表 | [未知] | `+0x18` 是城镇，不是遗迹下标 |
| `eItemTag = 7` | [未知] | 卖出过滤初值；与 `NUM_SELECTITEMTAG=6` 不是同一枚举 |

未还原前：YAML 保留 `unknown_bytes`，效果列表可以空。禁止为了「看起来完整」编造 +10 魅力之类的数。
