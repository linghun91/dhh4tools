# 友好、影响力、份额、合约、上缴

友好度、影响力、港口占有率、宫殿合约、月结上缴，全是**状态**，不是 World 表定义。定义侧的初值在 [01-forces.md](01-forces.md)；本篇管上下限、谁改这些数、改完发什么事件。买卖价公式与攻城占领公式分别属于 economy / combat。

## 1. 职责边界

**管：**

- 势力↔势力：友好度（`FORCEAMITY`）、关系旗、亲笔信 / 协议 / 宣战 / 停战 / 合并（文案层）
- 势力→大战区：影响力（`FORCE_INFLUENCE`，USHORT；原版 7 格）
- 势力→港口：最多 3 条合约槽上的占有率（`SHARE`，UCHAR）
- 宫殿签合约、介绍信 / 推荐信对份额的放行（效果在本层，信物品本身是 `items`）
- 独占港月结上缴、珍品出租费（账本归经济，本层决定「该不该收、收谁的港」）
- 投资在**势力层**的后果：发展度变化、份额优待、品牌商品认可（钱怎么算见 economy）

**不管：**

- 交易单价、供给、特产品产量 → `economy`
- 海战 / 攻城如何把港从 A 转到 B → `combat`（本层收 `TownCaptured`）
- 港口商业/军事发展度的存储布局 → 港口状态（本层发 / 收 `TownDeveloped`）
- 舰队月结买货亏损 → `exe_caps.md` / 委任，只动 `Force.gold`

## 2. 原版事实

### 上限符号

来源：`srcdata/catalog/constants.json`、`asserts.txt`、`Force.h` 注释、`exe_caps.md`。

| 符号 | 存储 | 设计上限 | 等级 |
| --- | --- | --- | --- |
| `FORCEAMITY_MIN/MAX` | UCHAR | 整数未还原 | `[确认]` 类型；`[未知]` 数值 |
| `FORCE_INFLUENCE_MIN/MAX` | USHORT | 整数未还原 | 同上 |
| `SHARE_MIN/MAX` | UCHAR | 整数未还原；玩法按 100 封 | `[确认]` 类型；100 见下 |
| `TOWNDEVELOPMENT_MIN/MAX` | USHORT | **9999**（主程序立即数） | `[确认]` 9999、硬顶 65535 |
| `NUM_TOWNDEVELOP` | 枚举 | `[推断]` **2**（商业 / 军事） | RTC `eTownDevelop` 与 2 比；UI `posDevelopCommerceNum` / `posDevelopMilitaryNum` |
| `NUM_TOWNSHARE` | 97 | 与港口 1:1 | `[确认]` `TownShareID.h` |

观测友好度（World 表 +0x38，25 字节）：默认 50，偏离 10、30、35、40、45、60、70。见 01 的矩阵。`[推断]` 设计区间 0–100，因份额明确用 100 做「已占」补，友好度同属 0–100 光荣惯例，但断言没写出 100。

观测 7×uint16（World +0x10）：122–1509（World0）、232–1254（World1）。`[推断]` 即影响力初值。

### 友好度

- `[确认]` 类型 UCHAR。来源：`FORCEAMITY_MAX <= sizeof(UCHAR)`。
- `[推断]` 初值矩阵在势力记录 +0x38。`CForce` 存盘对每个对方势力写 `+0x38+i`（及 `+0x1F+i` 旗）。
- `[确认]` 文案：亲笔信「友好度上升 %s」；计略「与 %s 的友好度降低了」；「好像加深了友好度」。来源：`mesfile_sc.txt` `0x38c7`、`0x3a53`、`0x3a67`、`0x11cf`。
- `[确认]` 外交动词（文案，公式未还原）：联合对抗协议、解除协议、停战、宣战、加入麾下 / 合并、假情报反目。来源：`0x38c7`–`0x3b77`、`0xb0af`–`0xb207`。
- `[确认]` 「友好势力的舰队，无法攻击」。来源：`0x33ef`。敌我判定用友好度还是协议旗：`[未知]`。
- `[推断]` +0x1F 的 0/1 是特殊关系旗（三对对称：阿博科鲁克↔西鲁韦拉、巴鲁迪斯↔玛尔德纳德、巴夏↔海雷丁），与友好度并列。

### 影响力

- `[确认]` 类型 USHORT。
- `[推断]` 存在 7 格，与母港地理带对齐（01 §2）。UI / 月结如何读这 7 格未还原。
- `[确认]` 海域知名度有文案：「%s 在本海域的知名度还是低的」。来源：`0x1143`。是否等于这 7 格之一：`[未知]`。

### 份额与合约

- `[确认]` UI 与逻辑循环 `eTownShare >= 3`：每港 **3 个合约槽**。来源：`CTown.cpp` 存盘循环 3；`fn_14010e490`、`fn_1402645a0`、`fn_1401f0ab0` RTC。
- `[确认]` `CTown` 槽布局：每槽两字节，空槽写成 `force=0x1A`、`share=0`。来源：`CTown::vfunc_05`。
- `[确认]` `fn_14010e490` 扫 97 港 × 3 槽，按槽内势力 id 给 25 长的「此势力是否在某港有约」标记。来源：同函数。
- `[确认]` 无占有率则不能交易，交易所会问「要投资吗」。来源：`0x7943`。投资可「稍微优待」占有率：`0x7ac3`。
- `[确认]` 宫殿：最低合约费 `%s` 枚；「根据你支付的合约金来承认占有率」；钱不够拒签。已有独占约则拒其他人。三槽占满：「已经与很多势力签了合约，没有其他势力的份儿了」。来源：`0xb2f3`、`0xb44b`、`0xb48f`、`0xb53b`、`0xb207`、`0xb283`。
- `[确认]` 介绍信只用于小城市份额通融；推荐信任何城市。卖流行品到门槛可发信。来源：`0x7d47`、`trade_stack.md`。
- `[确认]` 卖流行品加份额（**不是**物价公式）：

  来源：`srcdata/docs/trade_stack.md` `CommitDeal 0x140277090`。

  - 占有率 `< 80`（`88f0(town) < 0x50`）：每种流行品件数达标后调用 `122f6(town, 势力, x/6+1, 1)`。除数立即数 6。增加量由 `0x1402635f6` 封到 `100 − 已占`。
  - 占有率 `≥ 80`：改发介绍信 / 推荐信（件数门槛 10 / 15 / 16），不再走同一加份额分支。

- `[确认]` 其它份额文案：推销扩大占有率、占有率下降、夺取占有率、经纪人被收买。来源：`0xaf7`、`0xb1b`、`0x3ac7`、`0x3aeb`、`0x3b23`。计略细节属 `town-life` / `narrative`，本层只收结果。

### 发展度

- `[确认]` 商业 / 军事两轨，上限 9999，USHORT。来源：`exe_caps.md`、港口情报 UI 两个 Develop 位置、`CTown` 循环 2 个 word 在 `+0x1E`。
- `[确认]` 「多亏了巨额的商业投资，%s 的景气恢复了 / 越来越繁荣」。来源：`0xed0`、`0xfa7`。
- `[确认]` 年结品牌：「今年我商会经营的 1 级 %s 作为品牌商品得到了市场的认可」。来源：`0xfa7`。
- 投资**花多少钱、发展度加多少**：在 `CTown` / `CForce` 反编译里**没有**命名公式。账本条目「投资」在势力账本 `+0x14`（`exe_caps.md`）。标 `[未知]`，指向 `Docs/04-economy/04-special-products-investment.md` 与 `config/economy/investment.yaml`。

### 上缴与月结

- `[确认]` 文案：「本月的上缴金额是 %s 枚金币。请缴纳。」多句变体。独占：「%s 以独占的城市得到上缴金额 %s 枚金币。当作工具的出租费，有 %s 枚金币的收入。」来源：`0x8cb`–`0x9e7`。
- `[确认]` 宫殿珍品陈列：「若是这件，每月支付 %s 枚金币」。来源：`0xb5db`。对应出租费。
- `[确认]` 「向 %s 交纳了 %s 枚金币作为缴纳金」。来源：`0x3787`。
- `[未知]` 上缴金额如何从发展度 / 份额 / 独占算出来。`CForceElement::vfunc_04` 月结里有 `div`、对 `!= 0x1A` 的港取值、消息 id `0x167`，无法把立即数钉成公式。
- `[确认]` NPC 舰队买货：`cost * (1000 - 精神 + rand(100)) / 1000`，记到账本支出或直扣势力金币；月结付不起转入下月。来源：`exe_caps.md`。属委任经济，本层只提供金币账户。

### 信与推荐

卖流行品 ≥ 门槛且份额已高：无信 → 介绍信（件数 10–15）或推荐信（≥16）；已有介绍信且 ≥16 → 升推荐信。来源：`trade_stack.md`。信在宫殿放大份额（`0xb69f`），不在本篇写物品效果表。

## 3. 实体与 ID

状态主键全是 ID，不存指针。

```
Amity[force_a][force_b] : uchar
RelationFlag[force_a][force_b] : bool          # 原版 +0x1F
Influence[force_id][theater_id] : ushort       # 原版 7 格；theater 表长可配
ShareSlot[town_id][0..2] : { force_id | empty, share : uchar }
Develop[town_id] : { commerce : u16, military : u16 }
ContractFeeMin[town_id] : gold                 # 宫殿开价；公式 [未知]
Exclusive(town_id) : bool                      # 派生：一槽份额把其它槽挤空，或文案「独占合约」
```

`eTownShare` 原版 = 港口 id（97）。重构后份额是港口状态的一部分，不必再单独一个与港口等长的枚举；若保留，只当 `town_id` 别名。

空合约：`force_id = 26`（与势力运行时空哨兵相同），`share = 0`。禁止用 `force_id = 0` 当空（0 是卡斯特罗）。

## 4. 算法 / 公式

命名参数进 `config/caps.yaml` 与 `config/formulas/factions.yaml`。原版魔数当 default 包。

```yaml
# config/caps.yaml
force_amity_min: 0          # [推断]；符号 FORCEAMITY_MIN
force_amity_max: 100        # [推断]；存储 UCHAR 硬顶 255
force_influence_min: 0      # [推断]
force_influence_max: 65535  # 硬顶 USHORT；设计上限未还原，先等于硬顶
share_min: 0
share_max: 100              # 玩法封顶 100−已占；存储 UCHAR 硬顶 255
share_slots_per_town: 3
town_develop_min: 0
town_develop_max: 9999      # [确认] exe_caps
town_develop_tracks: 2
```

```yaml
# config/formulas/factions.yaml
schema: dk4.formulas.factions
version: 1

amity_default: 50           # World 表大量 0x32 [确认观测]

# 卖流行品 → 份额。来源 trade_stack.md [确认]
share_from_fashion:
  high_share_threshold: 80  # 低于此加份额，高于此改发信
  qty_divisor: 6
  bonus: "floor(sold_fashion_qty / qty_divisor) + 1"
  cap: "share_max - share_already_held(town_id)"  # 0x1402635f6
  per_item_min_qty: "ceil(total_hold_slots / 2)"  # 原版 s4D0=ceil(s4A4/2)；细节属 economy

# 发信门槛（份额 ≥ 80 分支）[确认]
letters_from_fashion:
  intro_min_qty: 10
  intro_max_qty: 15
  recommend_min_qty: 16
  upgrade_intro_to_recommend_min_qty: 16

# 以下原版未还原，键先占位，值 null，禁止在代码里写死猜测
contract_share_from_gold: null    # 「按合约金承认占有率」
tribute_from_exclusive_town: null # 月结上缴
treasure_rent_per_month: null     # 宫殿陈列
investment_to_develop: null       # 指向 economy/investment.yaml
investment_share_bonus: null      # 「稍微优待一点」
```

实现约束：

- 所有加减之后饱和到 `caps`，禁止 uint8 绕回。
- `bonus` 封到 `share_max - 已占`，三槽之和 ≤ `share_max`。
- 友好度矩阵不必对称；原版多数对称但不要当不变量。
- `tribute_from_exclusive_town` 未还原前，月结只发 `TributeDue` 事件、金额走占位 0 或内容包显式表，**禁止**发明「发展度×份额%」一类假公式。

敌我：`CanAttack(a, b)` 原版「友好势力无法攻击」。在协议旗与友好度阈值还原前，接口先留：

```
hostile_if: relation == war
forbid_attack_if: relation == allied or relation == ceasefire
# 友好度阈值 [未知]，不写 50
```

## 5. 耦合分析

原版：

- 份额住在 `CTown` 3 槽，友好度住在 `CForce` 25 字节，投资金额住在账本 `+0x14`，发展度住在 `CTown` 2 个 word——同一「经营」动作改三处结构体
- `CommitDeal` 直接调份额函数和发信，贸易模块知道 80 和 6
- 宫殿、交易所、广场各自改份额
- 攻城成功后的隶属变化与份额清空缠在一起（本篇不还原战斗）
- NPC 买货折扣用船长精神，扣的是势力金币

重构后数据所有权：

| 状态 | 所有者 | 别人怎么碰 |
| --- | --- | --- |
| `Amity` / `RelationFlag` / `Influence` / `Gold` | factions | 命令 |
| `ShareSlot[3]` | factions（按 `town_id` 索引） | `Share.add` / `Contract.sign` |
| `Develop` 两轨 | town 状态 | factions 发 `TownDeveloped` 请求，town 写入 |
| 账本支出/收入/投资 | economy | factions 报 `TributeCollected` 金额 |
| 介绍信 / 推荐信 | items | factions 问 `HasLetter(person, kind)` |

Economy 卖货结束发 `DealCommitted { town_id, force_id, fashion_sold_qty, … }`，由本系统决定加份额还是发信。Combat 发 `TownCaptured { town_id, from_force, to_force }`，本系统清旧约、写新约。禁止 economy 自己 `town.share +=`。

## 6. 解耦后的配置

```yaml
# config/factions/relations.yaml
schema: dk4.factions.relations
version: 1
pack: vanilla-world0

theaters:                         # 7 格 [推断]；key 仅原版包注释
  - { id: 0, key: north_sea }
  - { id: 1, key: iberia_med }
  - { id: 2, key: africa }
  - { id: 3, key: arabia_india }
  - { id: 4, key: se_asia }
  - { id: 5, key: east_asia }
  - { id: 6, key: new_world }

# 开局友好度：只列非默认。默认 amity_default
amity_overrides:
  - { a: 0,  b: 6,  value: 70 }   # 卡斯特罗 ↔ 阿博科鲁克
  - { a: 0,  b: 7,  value: 45 }
  - { a: 1,  b: 7,  value: 30 }
  - { a: 3,  b: 17, value: 10 }   # 李家 ↔ 来岛
  - { a: 3,  b: 23, value: 60 }
  # 其余见 01 矩阵；管线从 hex +0x38 生成，不要手抄丢对称性

relation_flags:                   # World +0x1F 为 1 的对
  - { a: 6,  b: 11 }
  - { a: 7,  b: 18 }
  - { a: 9,  b: 10 }

# 开局合约 [未知]：CTown init 把空槽写成 0x1A。
# 若剧本脚本再填，放到 narrative，不进本表。
initial_contracts: []

diplomatic_actions:               # 只登记动词与事件，数值 [未知]
  - { key: letter,           event: AmityChanged }
  - { key: alliance,         event: AllianceFormed }
  - { key: alliance_break,   event: AllianceBroken }
  - { key: ceasefire,        event: CeasefireSigned }
  - { key: declare_war,      event: WarDeclared }
  - { key: absorb,           event: ForceAbsorbed }
```

完整 25×25 初值由提取器从 `forces_world0.json` hex `[0x38:0x51]` 生成。`relations.yaml` 给人看的是 overrides。

发展度初值若在港口 World 表 rest 里，属 `config/world/towns.yaml` 的状态初值，本系统启动时 `Town.getDevelop(town_id)` 读取，不复制一份。

## 7. 独立旋钮

- 友好度 / 影响力 / 份额 / 发展度上下限（`caps.yaml`）
- 每港合约槽数（原版 3）
- 卖流行品除数 6、高份额阈值 80、发信件数
- 7 个大战区的划分（只改 `theaters` + 势力初值，不改战斗）
- 默认友好度 50
- 是否允许三槽份额之和超过 100（原版封到 100−已占，默认保持）

不在本文件改：单价、攻城伤害、经验。投资金钱曲线在 `economy/investment.yaml`；本层只保留「投资成功后份额优待 / 发展度请求」两个开关。

## 8. 对外接口

查询：

```
Amity.get(a, b) -> int
Relation.isAllied(a, b) -> bool
Influence.get(force_id, theater_id) -> int
Share.slots(town_id) -> [{ force_id, share }]
Share.of(town_id, force_id) -> int          # 无约则 0
Share.canTrade(town_id, force_id) -> bool   # 原版：无份额不能交易
Share.isExclusive(town_id) -> bool
Develop.get(town_id) -> { commerce, military }
Tribute.pending(force_id) -> gold           # 公式未还原前可恒 0
```

命令：

```
Amity.add(a, b, delta, reason)
Share.add(town_id, force_id, delta, reason) # 内部饱和、占槽
Share.setSlot(town_id, slot, force_id, share)
Contract.sign(town_id, force_id, gold_paid) # gold 公式未还原则拒绝或走内容表
Contract.break(town_id, force_id)
Develop.request(town_id, track, delta, reason)  # 不直接写港口内存
```

事件（本系统发出，载荷只含 ID 与值）：

| 事件 | 载荷 |
| --- | --- |
| `ShareChanged` | `{ town_id, force_id, old, new, reason }` |
| `ContractSigned` | `{ town_id, force_id, share, gold_paid }` |
| `ContractBroken` | `{ town_id, force_id }` |
| `TownDeveloped` | `{ town_id, track, old, new, reason }` |
| `AmityChanged` | `{ force_a, force_b, old, new, reason }` |
| `InfluenceChanged` | `{ force_id, theater_id, old, new }` |
| `TributeDue` / `TributeCollected` | `{ force_id, gold, town_ids[] }` |
| `AllianceFormed` / `WarDeclared` | `{ a, b, … }` |

订阅：

| 来源 | 事件 | 本层动作 |
| --- | --- | --- |
| economy | `DealCommitted` | 按 `share_from_fashion` 加份额或通知 items 发信 |
| economy | `InvestmentSettled` | 请求 `TownDeveloped`；可选份额优待（优待量 `[未知]`） |
| combat | `TownCaptured` | 清旧约、写占领方槽；**不**算战斗伤害 |
| narrative | 外交脚本 | `Amity.add` / 协议命令 |
| time | `MonthTicked` | 发 `TributeDue`；金额公式未还原则跳过扣款 |

`reason` 用稳定字符串：`fashion_sale`、`contract`、`investment`、`scheme`、`capture`、`letter`，便于 UI 与回放。

不暴露 25 字节数组、不暴露 `CTown+0x18`。

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| `FORCEAMITY_MIN/MAX` 整数 | 只有符号；观测 10–70 | 确认 0–100（YAML 里 100 必须标推断） |
| `FORCE_INFLUENCE_MIN/MAX` 整数 | 只有符号；观测到 1509 | 任何设计封顶除 USHORT 硬顶外 |
| 7 格 = 影响力 | 宽度与地理带 | `eForceAimKind` 或海域 id |
| +0x1F 旗 | 三对对称 1 | 具体 `eForceRelation` 名（同盟/从属/联姻） |
| 敌我阈值 | 「友好势力无法攻击」 | `amity > 50` 即友军 |
| 合约金 → 份额 | 文案「按钱承认」 | 线性 `/N` |
| 最低合约费 | 文案有 `%s` | 与发展度的函数 |
| 独占判定 | 文案「独占合约」 | 「单槽 100」或「仅一槽非空」二者之一当确认 |
| 上缴公式 | 文案有金额；`vfunc_04` 有 div | 任何具体乘积 |
| 出租费公式 | 文案「每月支付 %s」 | 与珍品价格挂钩的确认式 |
| 投资金钱 | 账本 +0x14；`CTown`/`CForce` 无公式 | 本篇编造；见 economy |
| 投资 → 发展度增量 | 文案「巨额商业投资」 | `+N` 每金币 |
| 「稍微优待」份额 | 文案 | 固定 +5 之类 |
| `CTown +0x24` 两个 dword | 存盘有 | 累计投资额 |
| 介绍信「小城市」判定 | 文案 | 发展度 < K 的确认 K |
| 消息 `0x167` | `CForceElement` 月结 | 一定是上缴句 |

份额加减的除数 6、阈值 80、封顶 `100−已占` 是确认的；**不要**把它们复制进 economy 物价公式。
