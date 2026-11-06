# 舰队与地方舰队

舰队是「最多 5 艘船 + 一个提督 + 驻港 / 坐标 + 命令」的实体。World 表给 69 个模板；玩家自己的舰队、委任出去的地方舰队、大地图上的移动船，都挂在同一套 ID 上。

## 1. 职责边界

本系统管：

- 世界舰队模板 `eFleet`（69 条，步长 `0x18`）
- 玩家舰队（5 个 `eFleetShip` 槽）
- 地方舰队（委任、账本、命令）
- 阵型
- 大地图移动船 / 移动舰队（`eMobileShip` / `eMobileFleet`）

本系统不管：

- 船体数值、货格内容（`01-ship-model.md`；本系统只持有 `ship_instance_id`）
- 舱室人事（`03-cabins-crew-posts.md`）
- 地方舰队买货折扣用到的技能值（人物系统提供 `Skill(person, skill_key)`，公式在本篇，经济系统执行扣款）
- 海战 AI（`07-combat`）
- 航线、风向（`06-ocean`）

## 2. 原版事实

### 世界表

- `[确认]` `GetFixFleetProc`：World0 / World1 各 **69** 条，步长 **`0x18`**，导出名相同。来源：`FleetID.h`、`world/World0_fleet.c`、`world/World1_fleet.c`、`catalog/world_tables.json`。
- `[确认]` JSON / C 里 `name` 全是空串。舰队没有 World 表内嵌的显示名。来源：`Docs/_data/fleets_world0.json`。
- `[确认]` `CFleet::vfunc_05` RTC 局部名 `r_sFixFleet` 24 字节，从 `GetFixFleetProc` `rep movsb` 拷进对象。来源：`game/fleet/CFleet.cpp`。
- `[确认]` `CFleetElement` 构造：`fn_1400017df(..., 0xc8, 0x45)` → **69** 个 `CFleet`，每个 **`0xC8`**；另在 `+0x3600` 放 `CPlayerFleetData`。来源：`game/fleet/CFleetElement.cpp`。

### 玩家舰队

- `[确认]` `NUM_FLEETSHIP=5`。证据类型：构造计数（`CFleet` 构造 `fn_1400017df(..., 0x10, 5)`）+ 循环上界；RTC 投票 236 次只作旁证。来源：`FleetShipID.h`、`Constants.h`、`mesfile_sc.txt` `0x7df7`「一只舰队最多可以有五艘船」。
- `[确认]` `CFleetShip` 尺寸 `0x10`，字段 `+0x8`、`+0x9`。空构造 / 清空（`fn_14000dc06`）：`+0x8=0xD0`、`+0x9=0x15`；放船（`fn_140007153`，`0x1400ef900`）：`+0x8=eShip`、`+0x9=0x15`。只写一个字节，不分配、不复制 `CShip`。来源：`CFleetShip.cpp` + 反汇编。
- `[确认]` `CFleet` 构造在 `+0x18` 建 5 个 `CFleetShip`（`fn_1400017df(..., 0x10, 5)`）。`+0x75=5`、`+0x7c=1` 是 `vfunc_05`（`0x1402ac940`，按模板开局）写的，不是构造。来源：`CFleet.cpp` + 反汇编 `0x1402acb13` / `0x1402acb1f`。
- `[确认]` `fn_14000c144`（`0x1400f0bf0`）取舰队所在港：`+0x80` 位 3 置位则返回 `0x62`（在海），否则返回 `+0x70`。`0x62` 是「无港」哨兵，与 `CShip+0x54`、货格 `+2` 同值。
- `[确认]` 换旗舰：「一旦改变旗舰，船舱担任者将被解任。」「请决定新舰队的旗舰。」来源：`mesfile_sc.txt` `0x2f0f` / `0x30a3`。人事清槽的规则落在 `03`，本系统发事件。
- `[确认]` 「提督不担任舰长怎么行？」来源：`mesfile_sc.txt` `0x80a7`。
- `[确认]` 非玩家势力货物不走 5×5 真实货格，走 `CFleet+0x74`「一条记录 + 数量」。来源：`srcdata/docs/trade_stack.md`。

### 地方舰队（委任）

- `[确认]` 「建立地方舰队需要１０００００枚金币。」「不能再编制舰队了！」来源：`mesfile_sc.txt` `0x2f33`。
- `[确认]` 流程文案：选船 → 选提督（`由%s·%s出任提督，可以吗？`）→ 选旗舰 → 下命令（目标城镇）→ 可解散。来源：`mesfile_sc.txt` `0x2f9b`–`0x30a3`。
- `[确认]` 「其舰队的收支是赤字。要偿还舰队的借款，带的钱不够了。」来源：`0x3057`。
- `[确认]` 命令例句：买仓库货、运到某港、寻找人物、护送、催还借款。来源：`0x30f7`–`0x3158`。
- `[确认]` 战斗中误击会喊「是我，地方舰队的。」来源：`0xfda7` / `0xfe13`。
- `[确认]` UI：`CChangeProxyFleetView`，RTC `eFleetShip`；另有函数 RTC `aeProxyShip` 20 字节 = 5×4。来源：`CChangeProxyFleetView.cpp`、`catalog/rtc.json` `0x140154cb0`。
- `[确认]` 账本：`CFleetElement+0x3738`，循环 **6** 槽；`srcdata/docs/exe_caps.md` 写 `mgr+0x3738+idx*0x18`：`+0x00..0x0B` 上月，`+0x0C` 支出，`+0x10` 收入，`+0x14` 投资。余额 = 收入 − 支出 − 投资。
- `[确认]` 地方舰队自动买货 `fn_140010d25` / body `0x1400f6d20`：`cost = price * qty * (1000 - Skill(提督, 会计术) + rand(100)) / 1000`，记入账本。人是 `fn_1400078a1(fleet)`（`0x1400f0ad0`）= `CFleet+0x10` 提督；为 `0xEC` 时返回 NULL，**不打折**按原价。`fn_14000e1ce(person, 4)` 的 4 是 `eSkill` 会计术，不是六维（`03-characters/03`）。原始指令 `0x1400f741a..0x1400f7464`：`mov edx,4; call fn_14000e1ce; mov ecx,0x3e8; sub ecx,eax; … call rand(0x64); add; imul ecx,eax（32 位）; xor edx,edx; div 0x3e8（无符号）`。随后 `fn_140006e88(提督)` 给段 1 经验 +10（`0x1400f7480`）。技能值上限 `skill_value_cap=500`，括号恒 ≥ 500；只有把技能值上限改到 1000 以上括号才会变负、被 `div` 当巨大无符号数。来源：反汇编。
- `[未知]` `NUM_PROXYFLEET` 整数未还原。6 本账本是否 = 1 玩家 + 5 委任，未证明。

### 阵型

- `[确认]` `CChangeFormationView` 按 `eFleetShip` 0–4 循环，空位 `0xD0`；确认时在起点 / 终点间插值 **8** 帧：`s64` 0..7，`fn_140008e9f(&posStart, &posEnd, s64, 7)`，分母 7，含首尾。来源：`CChangeFormationView.cpp` L131–L139。
- `[未知]` 阵型种类枚举名与数量。没有 `NUM_FORMATION` 符号。

### 移动船

- `[确认]` `NUM_MOBILESHIP=20`（证据：`CMobileShipElement` 构造计数 20），`NUM_MOBILEFLEET=4`（证据：`CFleetElement+0x35f0` 存档循环 4 条，见下）。头文件 / `num_values.json` 的值只是 RTC 投票旁证。
- `[确认]` `CMobileShip` 尺寸 `0x30`，存档字段 `+0x8`、`+0xC`、`+0x10`、`+0x14`、`+0x16`、`+0x18`、`+0x1A`、`+0x1B`。来源：`CMobileShip.cpp`。
- `[确认]` `CMobileShipElement` 构造：20 个 `CMobileShip`（步长 `0x30`）+ `+0x1038` 处 21 个步长 `0x18` 的 `CBattleGun` + `+0x1230` 处 **100** 个步长 `0x30` 的 `CShell`（`fn_140007dce(..., 0x18, 0x15)` / `(..., 0x30, 0x64)`）。炮与弹池挂在移动船元素上；类名依据见 `08-gun-shell-catalog.md`。来源：`CMobileShipElement.cpp`。
- `[确认]` `CFleetElement+0x35f0` 起 4 条 4 字节记录，与 `NUM_MOBILEFLEET=4` 同数。来源：`CFleetElement.cpp` 存档循环。
- `[未知]` `NUM_MOBILEFLEETKIND`、`NUM_MOBILESHIPSTATE`、`NUM_FLEETSTATE`、`NUM_FLEETAIMKIND`、`NUM_FLEETCOLOR`。

### `CPlayerFleetData`

- `[确认]` 符号 `FLAGSHIPSTATE_BITS`、`PLAYERSTATE_BITS`、`PLAYERCOMPANION_BITS`。来源：`PlayerFleetData.h`、`constants.json`。
- `[确认]` 初始化：31 字节写成 `0xEC`（`vfunc_05` 循环 `0x1f`）；再 `4 × 31` 组，每组 2 字节。来源：`CPlayerFleetData.cpp`。布局与舱室绑定的关系见 `03`。
- `[确认]` `0xEC=236`：`NUM_PERSON=235` 的外侧，和舰队模板 `+0x14` 常用值相同，当作空人物哨兵 `[推断]`。

### 模板字节（World0，步长 0x18）

`CFleet::vfunc_05`（`0x1402ac940`）把 24 字节读进栈上 `r_sFixFleet`（`rsp+0x28`），再逐字节搬进 `CFleet`。`[确认]` 搬运关系来自原始指令；「含义」列除注明外是 `[推断]`。

| 偏移 | 宽 | 搬到 `CFleet` | 含义 | 依据 |
| --- | --- | --- | --- | --- |
| `+0x00` | u8 | `+0x10` | `person_id`（提督） | 舰队 0–6 等于主角 0–6；空槽 `0xEC` |
| `+0x01..+0x05` | 5×u8 | `CFleetShip[i]+0x8` | `eShip` 实例槽；`0xD0` 跳过 | `[确认]` `cmp 0xd0` 后 `fn_140007153`；值都在 100–202，即预置船槽（`01`） |
| `+0x06..+0x07` | 2 B | — | 不读 | 69 条皆 0 |
| `+0x08..+0x0F` | 2×u32 | `+0x68`（qword 整拷） | 大地图 X / Y | 舰队 4 = `33208, 8488`，近马尼拉 `33424, 8368` |
| `+0x10` | u8 | `+0x70` | `town_id`；`0x62` = 无港 | `[确认]` `fn_14000c144` 返回 `+0x70`；舰队 0=11 里斯本、1=7、2=2、3=54、6=67 |
| `+0x11` | u8 | `+0x71` | 未命名 | 皆 0；**单独**一字节，不是 `town_id` 高位 |
| `+0x12` | u8 | `+0x72` | 种类 / 势力 / 状态 | 取值 0、1、7、8、`0x0B` |
| `+0x13` | u8 | `+0x73` | 常 `0x62` | 与「无港」同值，更像港口哨兵而不是人物 98 |
| `+0x14` | u8 | — | 常 `0xEC` | `vfunc_05` 不搬 |
| `+0x15` | u8 | `+0x74` | NPC 紧凑货记的商品；`0x6A` = 空 | `[确认]` 搬运；`trade_stack.md` 的 `CFleet+0x74` |
| `+0x16` | u8 | `+0x80` | 状态位 | `+0x80` 位 3 = 在海（`fn_14000c144`） |
| `+0x17` | u8 | — | 不读 | 全 0 |

`vfunc_05` 另写 `+0x75=5`、`+0x7C=1`，并在 `+0x70 != 0x62` 时把舰队登记到所在港。

- `[确认]` 模板船引用是**共享的预置船槽**，不是新船：同一舰队里常有重复（舰队 9 `64 64 64` = 三个槽都指 100 号「小型汉萨式帆船」；舰队 8 `AA AA A9 A9 A9`），`fn_140007153` 只写一个字节。NPC 舰队的船因此只是「船体定义 + `CFleetShip+0x9`」，没有独立 `CShip` 状态。`+0x9` 初值 `0x15=21`，含义 `[未知]`。
- `[确认]` 引用到的船名（槽 − 100 = `hull_id`，名称来自 exe，逐行见 `05`）：`0xC2` 卡鲁提拉、`0xC3` 维斯泰洛斯、`0xC4` 阿伦海姆、`0xC5` 春申、`0xC6` 华昌、`0xC7` 埃斯佩兰莎、`0xC8` 安娜希莉雅、`0xC9` 拉乌卡、`0xCA` 怪鱼（舰队 62，提督 197）；`0x64`–`0x85` 船坞模板 0–33；`0x86`–`0xA3` 商用 / 运输变体；`0xA4`–`0xC1` 武装变体。

驻港与主角开场港一致，只对 0、1、2、3、6 `[确认]` 数字；「+0x00 就是该剧本主角」是 `[推断]`。舰队 4 驻马尼拉、舰队 5 驻巴士拉，未在文案里交叉证明。

### World0 vs World1

`[确认]` 69 条里 **57 条**不同（逐字节比 `world/World0_fleet.c` 与 `World1_fleet.c`）：

- 舰队 0–3 只差 `+0x16`（W0 = 0，W1 = 1）。
- 45 条的五船引用不同（如舰队 5 `BE BE 9F BD D0` → `C8 7E BD D0 D0`，舰队 9 `64 64 64 D0 D0` → `A4 A4 A4 64 64`）。
- 舰队 42 的 `+0x10` 不同（54 → 0）；41 的 `+0x12` 不同（0 → 1）；43、46–57、62 的 `+0x14` 在 W1 统一为 `0x63`。

两份是同一 schema 的不同内容，不能只记「舰队 5 不同」。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空 / 非法 | 重构 key |
| --- | --- | --- | --- | --- |
| 舰队模板 / 实例 | `eFleet` | 69 | `INVALID_FLEET=-1` | `fleet_id` |
| 玩家船位 | `eFleetShip` | 5 | `INVALID_FLEETSHIP=-1`；槽内空船 `0xD0` | `fleet_ship_slot` |
| 地方舰队 | `eProxyFleet` | `[未知]` | `INVALID_PROXYFLEET=-1` | `proxy_fleet_id` |
| 移动船 | `eMobileShip` | 20 | `INVALID_MOBILESHIP=-1` | `mobile_ship_id` |
| 移动舰队 | `eMobileFleet` | 4 | `INVALID_MOBILEFLEET=-1` | `mobile_fleet_id` |
| 舰队状态 | `eFleetState` | `[未知]` | `INVALID_FLEETSTATE=-1` | `fleet_state_id` |
| 舰队目标种类 | `eFleetAimKind` | `[未知]` | `INVALID_FLEETAIMKIND=-1` | `fleet_aim_kind_id` |
| 舰队颜色 | `eFleetColor` | `[未知]` | — | `fleet_color_id` |
| 账本槽 | — | 6 | — | `ledger_slot` |

`eFleet` 0–68 在 `FleetID.h` 写成 `eFleet_000` … `eFleet_068`，没有人类可读别名。重构 key 暂用 `vanilla:fleet_000`，`vanilla_index` 只供导入。运行时空值一律 `null`；表中 `0xD0`、`0xEC`、`0x62`、`0x6A`、`0x46`（空舰队）只在 `ids/legacy_codec.yaml`（D1）。禁止编造「某某提督舰队」作为原版包名字。

## 4. 算法 / 公式

```
# 地方舰队创建
can_create_proxy =
    gold >= formulas.proxy_create_gold          # 100000
    and proxy_count < caps.proxy_fleets         # 原版「不能再编制」的上限，整数未还原
    and selected_ships.length in 1 .. caps.fleet_ships

# 账本（exe_caps.md）
ledger.balance = ledger.income - ledger.expense - ledger.investment
# +0x00..0x0B 为上月三元组，月结时滚动

# 地方舰队买货成本（原版；读技能值，不读六维）
raw = price * qty
holder = Fleets.admiral(fleet_id)                      # 原版 CFleet+0x10；空 → 不打折
buy_cost = holder == null ? raw
         : raw * (params.buy_divisor - Skill(holder, accounting) + rand(params.buy_rand)) / params.buy_divisor
Progression.grant_exp(holder, add{track_b: 10}, roll_mode_default, source: proxy_buy)   # fn_140006e88；段 1 = track_b
# buy_divisor=1000, buy_rand=100（rand 取 0..99）；Skill 已夹到 $caps.skill_value_cap（原版 500）
# 原版不会炸：括号 >= 1000-500 = 500。只有 skill_value_cap > buy_divisor 时括号才变负（原版 32 位无符号 div 爆成巨数）。
# 重构：加载时校验 $caps.skill_value_cap < buy_divisor；int_mode 可选 i64_sat 并把括号夹到 >= 0

# 阵型插值
formation_lerp_frames = formulas.formation_lerp_frames  # 8（s=0..7，t=s/7，含首尾）
```

命令（内容，不是公式）：目标必须是已注册 `town_id` 或 `person_id`。种类从文案抽出，id 待 RTC / 命令枚举还原后入库。

岗位持有人（D2，供 `07-combat`、`06-ocean`、`04-economy` 共用）：

```
post_holder(fleet, post, ship = flagship(fleet)) =
    if fleet has cabin bindings:                      # 原版只有玩家本舰队（CFleetElement+0x3600，5 船 × 31 舱）
        room = posts[post].room                       # 原版 fn_140007e19；posts.yaml（03-characters）持有
        return Cabins.binding(ship, slot_of(ship, room)).person    # 03
    return post_fallback[post](fleet, ship)           # 原版：本舰队提督，或该船舰长 CShip+0x50
```

`post_fallback` 写在 `config/fleets/post_rules.yaml`，原版包把各调用点的「否则」分支逐条登记，引擎不 `if 玩家`。

## 5. 耦合分析

原版：

- `CFleet::vfunc_05` 直接 `GetFixFleetProc`，再按字节建 `CFleetShip`。
- NPC 舰队的船只是指向预置船槽 100–206 的字节，多条舰队、同队多槽共用一个 `CShip`：船体定义被当成实例用。重构必须把「舰队船位 → `hull_id`（定义）」和「→ `ship_instance_id`（有状态的活船）」分成两种引用。
- 玩家货物在船上，NPC 货物在 `CFleet+0x74`。同一 `CountFleetItem` / `AddLoad` 两支。来源：`trade_stack.md`。
- 地方舰队买货写死「提督 + 技能 4」。重构：公式 `refs` 写 `skill: accounting`、持有人取 `Fleets.admiral(fleet)`，输入单位是技能值（0–`skill_value_cap`），不读六维、不比 id。
- 换旗舰清舱室担任者：舰队系统发事件，人事系统执行。
- 月结 `CFleetElement::vfunc_04`（约 `0x140100180`）付不起的支出滚下月。来源：`exe_caps.md`。
- 移动船元素里塞了 100 个炮弹对象，航海表现和战斗弹药池绑在一起。

禁止：经济改舰队经验；战斗写账本收入；UI 写 `fleet_id` 以外的存档字段。

## 6. 解耦后的配置

```yaml
# config/caps.yaml
fleet_ships: 5
world_fleets: 69                 # 原版包实例数
mobile_ships: 20
mobile_fleets: 4
ledger_slots: 6
# 买货不另设能力上限：引用 $caps.skill_value_cap（03-characters），校验 < buy_divisor
```

```yaml
# config/formulas/fleets.yaml
schema: dk4.formulas.fleets
version: 1
proxy_create:                             # §4 can_create_proxy 的金币门槛
  impl: vanilla.proxy_create
  params: { proxy_create_gold: 100000 }
proxy_buy_cost:
  impl: expr
  int_mode: vanilla_i32
  quirks: { unsigned_div: true }          # 原版 div 0x3e8 无符号
  refs: { skill: vanilla:accounting }    # refs 只放内容 key
  params: { buy_divisor: 1000, buy_rand: 100, exp_track: track_b, exp_gain: 10 }   # track_b = 原版段 1
  inputs: [raw, skill]                    # skill = 调用方对 Fleets.admiral(fleet_id) 求 Modifiers.eval("skill.accounting")；无提督传 null（提督不是岗位）
  expr: "skill == null ? raw : udiv(raw * (buy_divisor - skill + rand(buy_rand)), buy_divisor)"
  golden: [ { raw: 1000, skill: 500, rand: 0, out: 500 } ]
formation_lerp:                           # §4 阵型插值
  impl: vanilla.formation_lerp
  params: { formation_lerp_frames: 8 }    # s=0..7，t=s/7，含首尾
npc_cargo:
  impl: vanilla.npc_cargo
  params:
    npc_compact_cargo: true          # 原版 NPC 走 fleet 紧凑货记
```

```yaml
# config/fleets/proxy_rules.yaml
schema: dk4.fleets.proxy_rules
version: 1
create_cost_gold: 100000
max_ships: 5
need_admiral: true
admiral_must_be_captain: true
unassign_cabins_on_flagship_change: true
orders:
  - key: buy_warehouse            # 「购买%s个仓库的%s」
  - key: deliver                  # 「把%s个仓库的%s运送到%s」
  - key: seek_person              # 「寻找在%s的%s」
  - key: escort                   # 「护送%s到%s」
  - key: collect_debt             # 「催%s的%s归还借款%s枚」
```

```yaml
# config/fleets/formations.yaml
schema: dk4.fleets.formations
version: 1
slots: 5                         # 空位运行时为 null；原版 0xD0 只在 legacy_codec
lerp_frames: 8
# 阵型种类原版未还原，先只保留槽规则
kinds: []
```

```yaml
# config/fleets/templates.yaml
schema: dk4.fleets.templates
version: 1
pack: vanilla-world0
templates:
  - key: vanilla:fleet_000
    vanilla_index: 0
    name: null
    admiral: vanilla:raphael_castro # +0x00 = 0 [推断]；原版 0xEC → null
    town: vanilla:lisbon            # +0x10 = 11 → CFleet+0x70；原版 0x62 → null（在海）
    x: 0
    y: 0
    ships: [vanilla:carutira]       # hull key；原版字节 0xC2 = 模板槽 194 = hull vanilla_index 94。开局时克隆成新实例，不挂模板
    byte_12: 1
    byte_13: 0x62                   # 语义未知的原始字节，原样保留，不当 id 解释
    byte_14: 0xEC                   # 同上；vfunc_05 不读
    cargo_item: null                # 原版 0x6A = 空
    flag_16: 0                      # World1 为 1
    unknown_bytes: [0x00, 0x00, 0x00]  # +0x06,+0x07,+0x17
```

World1 同一 schema，另份 `packs/vanilla-world1/fleets/templates.yaml`。差异见节 2「World0 vs World1」（57/69 条不同），由导出脚本逐字节生成，禁止手抄。

## 7. 独立旋钮

- 世界舰队行数（内容包）
- 每队船数、移动船 / 移动舰队上限
- 建立地方舰队金币、账本槽数
- 买货公式除数 / 随机幅 / 能力封顶
- 阵型插值步数
- NPC 是否走紧凑货记（关了以后 NPC 也用真实货格，贸易代码仍只调查询接口）

## 8. 对外接口

事件：

- `FleetSpawned { fleet_id, town_id }`
- 到港：不由本系统发；订阅 ocean 的 `ArrivedAtTown { fleet_id, town_id }`
- `FlagshipChanged { fleet_id, ship_instance_id }`  → 人事订阅并清舱室
- `ProxyFleetCreated { proxy_fleet_id, admiral_person_id, cost_gold }`
- `ProxyFleetDisbanded { proxy_fleet_id }`
- `ProxyOrderIssued { proxy_fleet_id, order_key, town_id }`
- `LedgerMonthRolled { ledger_slot, expense, income, investment }`
- `FormationChanged { fleet_id }`

查询：

- `Fleets.template(fleet_id) -> Template`
- `Fleets.ships(fleet_id) -> [ship_instance_id]`  （空槽不出现或给 `empty`）
- `Fleets.flagship(fleet_id) -> ship_instance_id`
- `Fleets.admiral(fleet_id) -> person_id?`
- `Fleets.post_holder(fleet_id, post_key, ship_instance_id?) -> person_id?`  （一级查询，D2；默认旗舰）：玩家舰队按 `03` 的舱室绑定（岗位 → 舱室种类 → 旗舰该舱的担任者）；`captain` 岗对非旗舰取该船 `CShip+0x50`；NPC 舰队无绑定时按 `post_fallback`（原版：提督 / 本船舰长）。替代战斗、航海、经济里的「玩家取岗位 N，否则取提督 / 船长」三分支
- `Fleets.ledger(ledger_slot) -> { income, expense, investment, last_month }`
- `Fleets.location(fleet_id) -> { town_id | x, y }`

命令：

- `Fleets.setFlagship(fleet_id, ship_instance_id)`
- `Fleets.createProxy(ship_instance_ids, admiral_person_id) -> proxy_fleet_id`
- `Fleets.disbandProxy(proxy_fleet_id)`
- `Fleets.issueOrder(proxy_fleet_id, order_key, args)`
- `Fleets.setFormation(fleet_id, slot_to_instance)`

只传 ID。海洋系统用 `location` 画旗，不读 `CLocator*`。

## 9. 未核实字段

### 模板 24 字节里尚未钉死的

- `+0x00` 是否**总是** `person_id`（0–6 很像，舰队 7 = 108「酒馆的掌柜」则不像战斗提督）
- `CFleetShip+0x9`（初值 `0x15`）的含义；NPC 船的耐久 / 水手存在哪里
- `+0x06..+0x07` 恒 0
- `+0x12` 取值 `{0,1,7,8,0x0B}`：可能是势力、目标种类或状态
- `+0x13` 常 `0x62`（= 无港哨兵），少数 `0x2D` / `0x52` / `0x4E`；字段名未知
- `+0x14` 不被 `vfunc_05` 搬运，谁读它未查到
- `+0x16` → `CFleet+0x80` 状态位；位 0 含义（位 3 = 在海已确认）
- `+0x17` 恒 0
- 未用槽 63–68：`person=0xEC`，船全 `0xD0`，`town=0x62=98`（没有 98 号港）

### World0 69 条 hex

名称栏原版为空。`+0x10` 是 u8 `town_id`（`[确认]` 搬到 `CFleet+0x70`，`fn_14000c144` 读它），城镇名按 `Docs/_data/towns.tsv` 对照。

| id | hex | +00 | 五船 | xy | town | +12 | +16 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | `00c2d0d0d0d0000000000000000000000b000162ec6a0000` | 0 | C2 D0×4 | 0,0 | 11 里斯本 | 01 | 00 |
| 1 | `01c3d0d0d0d00000000000000000000007000162ec6a0000` | 1 | C3 D0×4 | 0,0 | 7 斯德哥尔摩 | 01 | 00 |
| 2 | `02c4d0d0d0d00000000000000000000002000162ec6a0000` | 2 | C4 D0×4 | 0,0 | 2 阿姆斯特丹 | 01 | 00 |
| 3 | `03c5c6d0d0d00000000000000000000036000162ec6a0000` | 3 | C5 C6 D0×3 | 0,0 | 54 杭州 | 01 | 00 |
| 4 | `04c7d0d0d0d00000b88100002821000031000162ec6a0100` | 4 | C7 D0×4 | 33208,8488 | 49 马尼拉 | 01 | 01 |
| 5 | `05bebe9fbdd00000000000000000000026000162ec6a0000` | 5 | BE BE 9F BD D0 | 0,0 | 38 巴士拉 | 01 | 00 |
| 6 | `06c9d0d0d0d00000000000000000000043000162ec6a0100` | 6 | C9 D0×4 | 0,0 | 67 波托韦洛 | 01 | 01 |
| 7 | `6cd0d0d0d0d00000000000000000000000000162ec6a0100` | 108 | D0×5 | 0,0 | 0 伦敦 | 01 | 01 |
| 8 | `27aaaaa9a9a90000000000000000000000000162ec6a0000` | 39 | AA AA A9 A9 A9 | 0,0 | 0 | 01 | 00 |
| 9 | `28646464d0d00000000000000000000005000162ec6a0000` | 40 | 64 64 64 D0 D0 | 0,0 | 5 汉堡 | 01 | 00 |
| 10 | `29aa6a69a8d0000000000000000000000b000162ec6a0000` | 41 | AA 6A 69 A8 D0 | 0,0 | 11 | 01 | 00 |
| 11 | `2aaaaa6a6aa7000000000000000000000d000162ec6a0000` | 42 | AA AA 6A 6A A7 | 0,0 | 13 塞维利亚 | 01 | 00 |
| 12 | `2b8665a5d0d0000000000000000000000f000162ec6a0000` | 43 | 86 65 A5 D0 D0 | 0,0 | 15 热那亚 | 01 | 00 |
| 13 | `2cbbbab9bdbd0000000000000000000016000162ec6a0000` | 44 | BB BA B9 BD BD | 0,0 | 22 伊斯坦布尔 | 01 | 00 |
| 14 | `2dbbbebebebe0000000000000000000019000162ec6a0000` | 45 | BB BE BE BE BE | 0,0 | 25 亚历山大 | 01 | 00 |
| 15 | `2e86a766d0d0000000000000000000001d000162ec6a0000` | 46 | 86 A7 66 D0 D0 | 0,0 | 29 圣乔治 | 01 | 00 |
| 16 | `2f888668d0d00000000000000000000022000162ec6a0000` | 47 | 88 86 68 D0 D0 | 0,0 | 34 索法拉 | 01 | 00 |
| 17 | `30a8aca9d0d0000000000000000000000a000162ec6a0100` | 48 | A8 AC A9 D0 D0 | 0,0 | 10 里加 | 01 | 01 |
| 18 | `31be7ea0a07d000000000000000000002a000162ec6a0000` | 49 | BE 7E A0 A0 7D | 0,0 | 42 卡利卡特 | 01 | 00 |
| 19 | `326aa8a86886000000000000000000002f000162ec6a0000` | 50 | 6A A8 A8 68 86 | 0,0 | 47 马六甲 | 01 | 00 |
| 20 | `33aa6aac6c6b0000000000000000000032000162ec6a0000` | 51 | AA 6A AC 6C 6B | 0,0 | 50 雅加达 | 01 | 00 |
| 21 | `34929191848a000000000000000000003a000162ec6a0000` | 52 | 92 91 91 84 8A | 0,0 | 58 长崎 | 01 | 00 |
| 22 | `356a69a8a7d0000000000000000000003d000162ec6a0000` | 53 | 6A 69 A8 A7 D0 | 0,0 | 61 哈瓦那 | 01 | 00 |
| 23 | `36aa6aa8a7d00000000000000000000041000162ec6a0000` | 54 | AA 6A A8 A7 D0 | 0,0 | 65 委拉克鲁斯 | 01 | 00 |
| 24 | `38aa6969d0d00000000000000000000000000162ec6a0000` | 56 | AA 69 69 D0 D0 | 0,0 | 0 | 01 | 00 |
| 25 | `396aa9a9d0d00000000000000000000000000162ec6a0000` | 57 | 6A A9 A9 D0 D0 | 0,0 | 0 | 01 | 00 |
| 26 | `3a6aa868d0d0000000000000000000000b000162ec6a0000` | 58 | 6A A8 68 D0 D0 | 0,0 | 11 | 01 | 00 |
| 27 | `3b69a8a5d0d00000000000000000000022000162ec6a0000` | 59 | 69 A8 A5 D0 D0 | 0,0 | 34 | 01 | 00 |
| 28 | `3c6a6aa868d0000000000000000000000d000162ec6a0000` | 60 | 6A 6A A8 68 D0 | 0,0 | 13 | 01 | 00 |
| 29 | `3d69a86767d0000000000000000000000d000162ec6a0000` | 61 | 69 A8 67 67 D0 | 0,0 | 13 | 01 | 00 |
| 30 | `3e86a768d0d0000000000000000000002f000162ec6a0000` | 62 | 86 A7 68 D0 D0 | 0,0 | 47 | 01 | 00 |
| 31 | `3fbaba7e7e7d000000000000000000001b000162ec6a0000` | 63 | BA BA 7E 7E 7D | 0,0 | 27 阿尔及尔 | 01 | 00 |
| 32 | `409e7a7abed00000000000000000000013000162ec6a0000` | 64 | 9E 7A 7A BE D0 | 0,0 | 19 雅典 | 01 | 00 |
| 33 | `417b7a7a7ed00000000000000000000016000162ec6a0100` | 65 | 7B 7A 7A 7E D0 | 0,0 | 22 | 01 | 01 |
| 34 | `42a0bd7d7cd00000000000000000000027000162ec6a0000` | 66 | A0 BD 7D 7C D0 | 0,0 | 39 亚丁 | 01 | 00 |
| 35 | `43acac6cabd00000000000000000000032000162ec6a0000` | 67 | AC AC 6C AB D0 | 0,0 | 50 | 01 | 00 |
| 36 | `44b6b79872b10000000000000000000032000162ec6a0100` | 68 | B6 B7 98 72 B1 | 0,0 | 50 | 01 | 01 |
| 37 | `4592928585a3000000000000000000003a000162ec6a0100` | 69 | 92 92 85 85 A3 | 0,0 | 58 | 01 | 01 |
| 38 | `46a8a8a869d0000000000000000000003d000162ec6a0000` | 70 | A8 A8 A8 69 D0 | 0,0 | 61 | 01 | 00 |
| 39 | `476a68af6fd00000000000000000000041000162ec6a0000` | 71 | 6A 68 AF 6F D0 | 0,0 | 65 | 01 | 00 |
| 40 | `486aa8b06fd00000000000000000000041000162ec6a0100` | 72 | 6A A8 B0 6F D0 | 0,0 | 65 | 01 | 01 |
| 41 | `49a8a8d0d0d00000000000000000000000000062ec6a0100` | 73 | A8 A8 D0 D0 D0 | 0,0 | 0 | 00 | 01 |
| 42 | `56747281c07f0000000000000000000036000162ec6a0100` | 86 | 74 72 81 C0 7F | 0,0 | 54 | 01 | 01 |
| 43 | `37c1c080d0d00000000000000000000039000162ec6a0100` | 55 | C1 C0 80 D0 D0 | 0,0 | 57 汉城 | 01 | 01 |
| 44 | `1ebe7e7ebdbd0000000000000000000026000062ec6a0100` | 30 | BE 7E 7E BD BD | 0,0 | 38 | 00 | 01 |
| 45 | `13aaaaa9a9d00000000000000000000005000162ec6a0100` | 19 | AA AA A9 A9 D0 | 0,0 | 5 | 01 | 01 |
| 46 | `4a686765d0d00000000000000000000021000762396a0100` | 74 | 68 67 65 D0 D0 | 0,0 | 33 罗安达 | 07 | 01 |
| 47 | `4b889c94d0d00000000000000000000012000762396a0100` | 75 | 88 9C 94 D0 D0 | 0,0 | 18 威尼斯 | 07 | 01 |
| 48 | `4cab6767d0d00000000000000000000031000762396a0100` | 76 | AB 67 67 D0 D0 | 0,0 | 49 | 07 | 01 |
| 49 | `4da7a7add0d0000000000000000000003c000762396a0100` | 77 | A7 A7 AD D0 D0 | 0,0 | 60 那霸 | 07 | 01 |
| 50 | `4e68b0876bd0000000000000000000002c000762396a0100` | 78 | 68 B0 87 6B D0 | 0,0 | 44 锡兰 | 07 | 01 |
| 51 | `4f6967af8cd00000000000000000000040000762396a0100` | 79 | 69 67 AF 8C D0 | 0,0 | 64 牙买加 | 07 | 01 |
| 52 | `50ac6c6b93d00000000000000000000029000762396a0100` | 80 | AC 6C 6B 93 D0 | 0,0 | 41 霍尔木兹 | 07 | 01 |
| 53 | `516a686794d00000000000000000000030000762396a0100` | 81 | 6A 68 67 94 D0 | 0,0 | 48 文莱 | 07 | 01 |
| 54 | `52babeb9b99f0000000000000000000014000862396a0100` | 82 | BA BE B9 B9 9F | 0,0 | 20 克里特 | 08 | 01 |
| 55 | `53aa69a87272000000000000000000003e000862396a0100` | 83 | AA 69 A8 72 72 | 0,0 | 62 圣多明尼各 | 08 | 01 |
| 56 | `54aa6aa8a7a7000000000000000000001c000862396a0100` | 84 | AA 6A A8 A7 A7 | 0,0 | 28 突尼斯 | 08 | 01 |
| 57 | `55bbbebebdbd0000000000000000000026000862396a0100` | 85 | BB BE BE BD BD | 0,0 | 38 | 08 | 01 |
| 58 | `c06a7e6867d0000000000000000000003d000862ec6a0100` | 192 | 6A 7E 68 67 D0 | 0,0 | 61 | 08 | 01 |
| 59 | `c7bb7a7ebdbd000000000000000000004f00082dec6a0100` | 199 | BB 7A 7E BD BD | 0,0 | 79 马斯利巴丹 | 08 | 01 |
| 60 | `c8bebe7ebdbd0000000000000000000051000852ec6a0100` | 200 | BE BE 7E BD BD | 0,0 | 81 吉阿丁 | 08 | 01 |
| 61 | `c9aabebe7ed0000000000000000000002e00084eec6a0100` | 201 | AA BE BE 7E D0 | 0,0 | 46 安曼 | 08 | 01 |
| 62 | `c5cad0d0d0d00000000000000000000000000062396a0100` | 197 | CA D0×4 | 0,0 | 0 | 00 | 01 |
| 63 | `ecd0d0d0d0d00000000000000000000062000b62ec6a0100` | 236 | D0×5 | 0,0 | 98 `[非法镇]` | 0B | 01 |
| 64 | 同 63 | 236 | D0×5 | 0,0 | 98 | 0B | 01 |
| 65 | 同 63 | 236 | D0×5 | 0,0 | 98 | 0B | 01 |
| 66 | 同 63 | 236 | D0×5 | 0,0 | 98 | 0B | 01 |
| 67 | 同 63 | 236 | D0×5 | 0,0 | 98 | 0B | 01 |
| 68 | 同 63 | 236 | D0×5 | 0,0 | 98 | 0B | 01 |

46–57 的 `+0x14=0x39`（不是 `0xEC`），59 的 `+0x13=0x2D`，60 的 `+0x13=0x52`，61 的 `+0x13=0x4E`。这些字节保持 hex，不要写成确定人物。

### `CFleet` 运行时未命名

存档：`+0x10`，每船 `+0x20/+0x21`（反编译把步长打成 `s20*s20`，实际是 5 个 `CFleetShip`），`+0x68` `+0x6C` dword，`+0x70..+0x75`，`+0x7C` `+0x80`，`+0x88`。`+0x74` 对 NPC 是紧凑货记 `[确认 用途，未确认全部位]`。

### 其它未还原计数

`NUM_PROXYFLEET`、`NUM_FLEETSTATE`、`NUM_FLEETAIMKIND`、`NUM_FLEETCOLOR`、`NUM_MOBILEFLEETKIND`、`NUM_MOBILESHIPSTATE`、`NUM_PLAYEREVENTSHIP`。`CMobileShipElement+0x1038` 的 21×`0x18` 池。
