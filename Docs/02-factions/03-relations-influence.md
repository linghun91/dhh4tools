# 友好、影响力、份额、合约、上缴

友好度、影响力、港口占有率、宫殿合约、宗主关系，全是**状态**，不是 World 表定义。定义侧的初值在 [01-forces.md](01-forces.md)；本篇管上下限、谁改这些数、改完发什么事件。买卖价、投资、上缴金额公式属于 economy（`04-economy/07-investment-tribute.md`），攻城属于 combat。

## 1. 职责边界

**管：**

- 势力↔势力：友好度（`FORCEAMITY`）、关系旗、亲笔信 / 协议 / 宣战 / 停战 / 合并（文案层）
- 势力→七大海域：影响力（`FORCE_INFLUENCE`，USHORT；原版 7 格，下标 `major_sea_id`，海域表见 06）
- 势力→港口：最多 3 条合约槽上的占有率（`SHARE`，UCHAR）
- 宫殿签合约、介绍信 / 推荐信对份额的放行（效果在本层，信物品本身是 `items`）
- 份额状态的所有读写：签约、投资优待、卖流行品、用信、占领（各路的「加多少」由来源系统算好或由本篇参数给，写槽只在本层）
- 宗主关系 `Overlord` 的生命周期（设、宣战清、灭亡清）；独占判定 `Share.exclusiveOwner`
- 影响力的所有写入（投资、卖流行品、遗迹首访三路调用 `Influence.add`）

**不管：**

- 交易单价、供给、特产品产量 → `economy`
- 海战 / 攻城如何把港从 A 转到 B → `combat`（本层收 `TownOccupied`，按 §2「份额与合约」改槽）
- 港口商业/军事发展度 → world `Towns` 运行态持有；economy 07（投资转化）、combat、narrative 只发命令 `Towns.apply_develop(...)`，`TownDeveloped` 只由 `Towns` 发；本层只读 `Towns.develop(town_id, track)`
- 独占港上缴、珍品出租费的金额与时点 → economy 07（`formulas/tribute.yaml`）；本层只提供 `Share.exclusiveOwner`
- 宗主上缴（附庸税）：本层在 tick `forces` 阶段（月初）执行步 `factions.suzerain_tribute` 并发 `SuzerainTributePaid`；金额式引用 economy 的 `formulas/tribute.yaml#suzerain`（06「状态所有权」）
- 舰队月结买货亏损 → `exe_caps.md` / 委任，只动 `Force.gold`

## 2. 原版事实

### 上限符号

来源：`srcdata/catalog/constants.json`、`asserts.txt`、`Force.h` 注释、`exe_caps.md`。

| 符号 | 存储 | 设计上限 | 等级 |
| --- | --- | --- | --- |
| `FORCEAMITY_MIN/MAX` | UCHAR | 整数未还原 | `[确认]` 类型；`[未知]` 数值 |
| `FORCE_INFLUENCE_MIN/MAX` | USHORT | 符号整数未还原；**加减封 `[0, 9999]`** | `[确认]` `fn_14000ae7a` `0x270F` |
| 势力金币 | uint32 `CForce+0x0C` | **99,999,999**，下限 0 | `[确认]` `fn_140002cb6` |
| `SHARE_MIN/MAX` | UCHAR | 整数未还原；玩法按 100 封 | `[确认]` 类型；100 见下 |
| `TOWNDEVELOPMENT_MIN/MAX` | USHORT | **9999**（主程序立即数） | `[确认]` 9999、硬顶 65535；归 world `Towns`（`caps.town_develop`） |
| `NUM_TOWNDEVELOP` | 枚举 | `[推断]` **2**（商业 / 军事） | RTC `eTownDevelop` 与 2 比；UI `posDevelopCommerceNum` / `posDevelopMilitaryNum` |
| `NUM_TOWNSHARE` | 头文件 97 = **RTC 投票误采** | `[推断]` **3**（每港合约槽） | `num_from_rtc.json`：`eTownShare` 比较立即数 `{3: 27 次, 97: 6, …}`，`fill_gaps.py` 只收 `>= 4` 才取了 97 |

观测友好度（World 表 +0x38，25 字节）：默认 50，偏离 10、30、35、40、45、60、70。见 01 的矩阵。`[推断]` 设计区间 0–100，因份额明确用 100 做「已占」补，友好度同属 0–100 光荣惯例，但断言没写出 100。

观测 7×uint16（World +0x10）：122–1509（World0）、232–1254（World1）。`[推断]` 即影响力初值（宽度 = `FORCE_INFLUENCE`，下标 = 七大海域已确认，见下）。

### 友好度

- `[确认]` 类型 UCHAR。来源：`FORCEAMITY_MAX <= sizeof(UCHAR)`。
- `[推断]` 初值矩阵在势力记录 +0x38。
- `[确认]` **对称、只存半矩阵**：读 `fn_140008f4e`（body `0x14010a0e0`）在 `self.id < other` 时读 `self[+0x38+other]`，否则交换双方；`a == b` 不读。存盘只存 `self.id < i` 的格（`fn_140010bc7`）。World 表下三角原版不读。关系字节 `+0x1F` 同规则（`fn_140012dc8`）。重构 = 无序对表（D10）。
- `[确认]` 改友好度 `fn_1400089f9(force, other, d)`：结果封 100（`0x64`，05 §2.8）。已知调用：占领 `fn_140012fee` 对每个被挤的旧槽势力 `d = -50`（`0xffffffce`，见下「份额与合约」）。
- `[确认]` 文案：亲笔信「友好度上升 %s」；计略「与 %s 的友好度降低了」；「好像加深了友好度」。来源：`mesfile_sc.txt` `0x38c7`、`0x3a53`、`0x3a67`、`0x11cf`。
- `[确认]` 外交动词（文案，公式未还原）：联合对抗协议、解除协议、停战、宣战、加入麾下 / 合并、假情报反目。来源：`0x38c7`–`0x3b77`、`0xb0af`–`0xb207`。
- `[确认]` 「友好势力的舰队，无法攻击」。来源：`0x33ef`。敌我判定用友好度还是协议旗：`[未知]`。
- `[推断]` +0x1F 的 0/1 是特殊关系旗（三对对称：阿博科鲁克↔西鲁韦拉、巴鲁迪斯↔玛尔德纳德、巴夏↔海雷丁），与友好度并列。

### 影响力

- `[确认]` 类型 USHORT。
- `[确认]` 7 格下标 = 七大海域（RTC `eSeaArea` 上界 7；活跃势力非零格 = 母港所属文化圈的 `major_sea_id`）。见 06。
- `[确认]` `fn_140008b6b`（body `0x14010a6c0`）= 7 格求和，即势力总影响力。
- `[确认]` 加减 `fn_14000ae7a(force, major_sea_id, d)`：`d == 0` 直接返回，否则 `clamp(0, cur + d, 9999)`。已知三路调用：
  1. 投资转化：每个持份势力 `d = total_gain * share / 2000`，格 = `fn_140014e48(town)`（`04-economy/07-investment-tribute.md` §4.4）
  2. 卖流行品：份额/信件分支有结果后，玩家势力在本港大海域 `+x`（`x` = 两格流行品件数和）；`fn_140010f46() == 2`（= `eScenario`，`fn_14000d486` 恒等）时 `×2`（`04-economy/03-trade-loop.md` §2.6 第 14 步）
  3. 遗迹首访：玩家势力加查表值，剧本条件下再加一次（`01-world/09-remains-oldmaps-catalog.md`、`11-remains-puzzle-system.md`）
  剧本倍数在重构里是修正器 `factions.influence_gain`（`ctx.source = fashion_sale | investment | remains`），由剧本包注入，不比较剧本号。
- `[推断]` 存档头 `+0x28` 的「势力值」就是 `fn_140008b6b` 总影响力（`12-ui-save/02-save-session.md`；算法未逐位对拍）。
- `[确认]` 海域知名度有文案：「%s 在本海域的知名度还是低的」。来源：`0x1143`。是否等于这 7 格之一：`[未知]`。

### 份额与合约

- `[确认]` UI 与逻辑循环 `eTownShare >= 3`：每港 **3 个合约槽**。来源：`CTown.cpp` 存盘循环 3；`fn_14010e490`、`fn_1402645a0`、`fn_1401f0ab0` RTC。
- `[确认]` `CTown+0x18` 槽布局：每槽 `(u8 force, u8 share)`。开局由 `CTown::vfunc_05` 从 DLL 港口记录 `+0x4C` 拷 3 个 word；share 为 0 的槽写成 `force=0x1A, share=0`（objdump `0x1402ae4e3`–`0x1402ae54c`）。伦敦初值 `(4 克利福德军, 80)`。所以**开局合约有初值**，在 World 包里。
- `[确认]` 槽里是势力 id，不是流行：`fn_14010e490`（RTC `eTownShare`）扫 97 港 × 3 槽，按槽内 id 填 25 长的 `abVying`（势力标记）；`fn_1400092af(town, force)` 按势力取份额；`fn_140004c19` 取独占方。`00-overview/04-id-registry.md` 把 `+0x4C` 记成 `eTownFashion` 是误读。
- `[确认]` 独占方 `fn_140004c19`（body `0x140260590`）：只看**槽 0**（`imul rax, 2*0`），`force != 0x1A` 且 `share >= 100` 返回该势力，否则空。
- `[确认]` 占领改槽 `fn_140012fee(town, occupier)`（body `0x14025f130`，objdump）：
  1. `n` = 非空槽数；`n > 0` 时 `cut = 12 / n`（有符号 `idiv`，`0x14025f1b4`）
  2. 每个非空槽：若 `occupier != 0x1A`，`fn_1400089f9(occupier, 槽势力, -50)`（友好度 −50）；`share = max(0, share - cut)`（`fn_14000939f(0, ·)`）；`> 0` 写回，否则 `force = 0x1A`、`n -= 1`；再 `fn_140008279(槽势力, town, -50, share, 0x1A)`（通知，语义 `[未知]`）
  3. `occupier != 0x1A` 且 `n < 3`：`fn_1400122f6(town, occupier, 10, 0, 1)`（加 10，内含「无母港则写母港」）
  - 占领方自己的旧槽也被削（循环不跳过它）；对自己调 −50 时友好度读写的效果 `[未知]`（读函数 `a == b` 不读）。
- `[确认]` 无占有率则不能交易，交易所会问「要投资吗」。来源：`0x7943`。投资可「稍微优待」占有率：`0x7ac3`。
- `[确认]` 宫殿：最低合约费 `%s` 枚；「根据你支付的合约金来承认占有率」；钱不够拒签。已有独占约则拒其他人。三槽占满：「已经与很多势力签了合约，没有其他势力的份儿了」。来源：`0xb2f3`、`0xb44b`、`0xb48f`、`0xb53b`、`0xb207`、`0xb283`。
- `[确认]` 介绍信只用于小城市份额通融；推荐信任何城市。卖流行品到门槛可发信。来源：`0x7d47`、`trade_stack.md`。
- `[确认]` 卖流行品加份额（**不是**物价公式）：

  来源：`srcdata/docs/trade_stack.md` `CommitDeal 0x140277090`。

  - 占有率 `< 80`（`88f0(town) < 0x50`）：两格都有流行品时**两种都须 ≥ 阈值**（`0x140277b9a`–`0x140277bbc` 两个 `jl`），只一格时看该种；达标后**一次**调用 `122f6(town, 玩家势力, x/6+1, 1, 1)`，`x` = 两格件数和（`0x140277dda` `idiv 6`）。增加量由 `0x1402635f6` 封到 `100 − 已占`。
  - 占有率 `≥ 80`：改发介绍信 / 推荐信（件数门槛 10 / 15 / 16），不再走同一加份额分支。

- `[确认]` 广场「寻求支持」结果 A（消息 `0x341` + `0x8f`「%s的市场份额上涨了。」）：`fn_1400122f6(town, 玩家势力, 1, 1, 1)`（`0x1401653b2`–`0x1401653cd`；`edx` 来自 `fn_1400054cf` → `fn_140004381` 当前玩家势力），即份额 **+1**，后两参与卖流行品同形。掷骰与门槛见 `08-town-life/06-plaza-intel-adman.md`。
- `[确认]` 其它份额文案：推销扩大占有率、占有率下降、夺取占有率、经纪人被收买。来源：`0xaf7`、`0xb1b`、`0x3ac7`、`0x3aeb`、`0x3b23`。计略细节属 `town-life` / `narrative`，本层只收结果。

### 发展度

- `[确认]` 商业 / 军事两轨，上限 9999，USHORT。来源：`exe_caps.md`、港口情报 UI 两个 Develop 位置、`CTown` 循环 2 个 word 在 `+0x1E`。状态归 world `Towns`；本层不持有。
- `[确认]` 「多亏了巨额的商业投资，%s 的景气恢复了 / 越来越繁荣」。来源：`0xed0`、`0xfa7`。
- `[确认]` 年结品牌：「今年我商会经营的 1 级 %s 作为品牌商品得到了市场的认可」。来源：`0xfa7`。
- `[确认]` 投资已由 economy 钉死（`04-economy/07-investment-tribute.md` §4）：`CTown+0x24` 两个 dword = 待转化投资；月初第一拍转发展度；最低合约费 `(商业+军事)/4`；份额优待 `floor(gold_paid / min_fee)` 经 `fn_1400122f6` 交本层写槽。本篇不再重复公式。

### 上缴与月结

- `[确认]` 文案：「本月的上缴金额是 %s 枚金币。请缴纳。」多句变体。独占：「%s 以独占的城市得到上缴金额 %s 枚金币。当作工具的出租费，有 %s 枚金币的收入。」来源：`0x8cb`–`0x9e7`。
- `[确认]` 宫殿珍品陈列：「若是这件，每月支付 %s 枚金币」。来源：`0xb5db`。对应出租费。
- `[确认]` 「向 %s 交纳了 %s 枚金币作为缴纳金」。来源：`0x3787`。
- `[确认]` **宗主上缴**（`CForceElement::vfunc_04` `0x14010dde8` 起，objdump）：
  1. 剧本 `fn_140004a1b(DAT_144296500)`（读 `+0x7C` = `eScenario`）`== 6`（主角家 = 21 昆廷韦拉斯，见 01）
  2. 世界时钟 `CWorld+0x10 > 0`（`fn_1400104b5`；开局第一刻不收）
  3. 玩家势力 `+0x1E`（`fn_140012788`）`!= 0x1A`
  4. `amount = gold / 100`（`div ecx=0x64`，无符号）；`amount == 0` 不收
  5. 消息 `0x167`（宗主名、金额串）；宗主 `+gold`（`fn_140002cb6`，封 99,999,999），玩家 `-gold`（`fn_14001117b`，不足置 0）
  - `[确认]` 在日历门 `fn_14000e9da`（`SCalendar+0x1c == 0`）之内 = **月初第一拍**（`01-world/07-calendar-and-coords.md` §2.3、economy 07 §2.1）。
  - 文案「向 %s 交纳了 %s 枚金币作为缴纳金」（`0x3787`）`[推断]` 是这条。
  - 重构：本层在 `forces` 阶段（月初）执行步 `factions.suzerain_tribute`，金额式引用 `formulas/tribute.yaml#suzerain`（economy 文件）；开关是剧本旗标 `scenario.flags.pay_suzerain_tribute`（原版只有剧本 6 为真）；成功后发 `SuzerainTributePaid`。
- `[确认]` **独占港上缴**在 `CTownElement::vfunc_04` 月末最后一拍：每座独占港（槽 0 份额 ≥ 100）给独占方 `商业/10`，按港口状态 ×3/2 或 ×1/2（economy 07 §4.5）。本层只提供 `Share.exclusiveOwner`。
- `[确认]` NPC 舰队买货：`cost * (1000 - 精神 + rand(100)) / 1000`，记到账本支出或直扣势力金币；月结付不起转入下月。来源：`exe_caps.md`。属委任经济，本层只提供金币账户。

### 信与推荐

卖流行品 ≥ 门槛且份额已高：无信 → 介绍信（件数 10–15）或推荐信（≥16）；已有介绍信且 ≥16 → 升推荐信。来源：`trade_stack.md`。信在宫殿放大份额（`0xb69f`），不在本篇写物品效果表。

## 3. 实体与 ID

状态主键全是 ID，不存指针。

```
Amity{a,b}                 : uchar             # 无序对，(a,b) 与 (b,a) 同一行；原版 +0x38 低 id 行
Relation{a,b}              : relation_key      # 无序对；原版 +0x1F；枚举与副作用见 05
Influence[force_id][major_sea_id] : ushort     # 原版 7 格 = 七大海域；不是文化圈；按 major_sea 表长
Overlord[force_id]         : force_id | null   # 原版 +0x1E
ShareSlot[town_id][0..n-1] : { force_id | null, share : uchar }   # n = share_slots_per_town
ExclusiveOwner(town_id)    : force_id | null   # 派生：槽 0 且 share ≥ exclusive_share（fn_140004c19）
```

发展度、最低合约费、待转化投资不在本层：world `Towns` + economy 07。

`eTownShare` 是**槽下标**（循环上界 3），不是港口 id；头文件 97 是投票误采。

空合约 / 无宗主：`null`。原版 `0x1A` 只在 `ids/legacy_codec.yaml`。禁止用 `force_id = 0` 当空（0 是卡斯特罗）。

## 4. 算法 / 公式

命名参数进 `config/caps.yaml` 与 `config/formulas/factions.yaml`。原版魔数当 default 包。

```yaml
# config/caps.yaml
force_amity_min: 0          # [推断]；符号 FORCEAMITY_MIN
force_amity_max: 100        # [确认] fn_1400089f9 加减路径封 0x64；符号值未读到；存储 UCHAR
force_influence_min: 0      # [确认] fn_14000ae7a clamp 下界
force_influence_max: 9999   # [确认] fn_14000ae7a 0x270F；存储 USHORT
share_min: 0
share_max: 100              # 玩法封顶 100−已占；存储 UCHAR 硬顶 255
share_slots_per_town: 3
exclusive_share: 100        # [确认] fn_140004c19；与 economy 07 同键
# 发展度上限归 world Towns（caps.town_develop: 9999），本层不重复
```

```yaml
# config/formulas/factions.yaml   D3 形状（14-config-schemas README「公式层」）
schema: dk4.formulas.factions
version: 2
# 本段除法都是有符号 idiv（x/6、12/n），int_mode 一律 vanilla_i32；无无符号怪癖。宗主上缴的无符号 div 在 economy tribute.yaml。
# 宗主上缴金额式不在本文件：引用 economy formulas/tribute.yaml#suzerain，
# 开关 scenario.flags.pay_suzerain_tribute（narrative 剧本包）。执行步 factions.suzerain_tribute。

amity_init:                     # 开局友好度缺省
  impl: vanilla.amity_init
  params:
    amity_default: 50           # World 表大量 0x32 [确认观测]

# 卖流行品（订阅 economy SoldFashionGoods）：F-ECO-04/05/06，键名与 13-algorithms、14 示例 formulas/factions.yaml 同（词表 §14-9）
# 来源 trade_stack.md、economy 03 §2.6 [确认]
fashion_share_threshold:              # F-ECO-04：每种流行品的件数门槛
  impl: expr
  int_mode: vanilla_i32
  params: { fashion_threshold_numer: 1, fashion_threshold_denom: 2 }   # 原版 ceil(s/2)（0x1402776ea）
  refs: { cabin_role: cargo_room }
  inputs: [cargo_rooms]               # = SoldFashionGoods.hold_sum（五船货物仓库间数合计）
  expr: "(cargo_rooms * fashion_threshold_numer + fashion_threshold_denom - 1) / fashion_threshold_denom"
  golden: [{cargo_rooms: 5, out: 3}, {cargo_rooms: 4, out: 2}]
  # 卖了两格则两格都 ≥ 阈值，卖一格则该格 ≥

fashion_share_gain:                   # F-ECO-05：份额 < share_letter_cut 时
  impl: vanilla.fashion_share_gain
  int_mode: vanilla_i32               # cdq; idiv 6; inc（0x140277dda），有符号
  quirks: {}
  params: { fashion_share_div: 6, fashion_gain_add: 1 }
  inputs: [x0, x1, threshold, share]  # 份额加值 trunc((x0 + x1) / fashion_share_div) + fashion_gain_add，经 Share.add 封到 share_max − 已占（0x1402635f6）
  # 份额 / 信件分支有结果后加影响力 x0 + x1 [确认] economy 03 §2.6 第 14 步
  modifiers: factions.influence_gain  # 影响力出口，ctx.source = fashion_sale；原版剧本 2 ×2 由剧本包注入，公式不再乘

# 占领改份额（订阅 combat TownOccupied）[确认] fn_140012fee
occupation:
  impl: vanilla.occupation_share
  int_mode: vanilla_i32
  params:
    share_cut_total: 12           # 每条旧约削 trunc(12 / 非空槽数)
    amity_delta_displaced: -50    # 占领方对每个旧槽势力
    occupier_grant: 10            # 剩余非空槽 < 槽数 时给占领方
    set_home_if_none: true        # 经 Share.add 的统一规则

# 发信门槛（份额 ≥ share_letter_cut 分支）[确认]；键名与 13-algorithms F-ECO-06 同
fashion_letter:                       # F-ECO-06
  impl: vanilla.fashion_letter
  int_mode: vanilla_i32               # 有符号 cmp
  params:
    share_letter_cut: 80          # 0x50；低于此走 fashion_share_gain
    letter_intro_min: 10          # 0x140277e63
    letter_intro_max: 15          # 0x140277e71
    letter_recommend_min: 16      # 0x14027805f
    letter_upgrade_min: 16        # 介绍信升推荐信，独立立即数 0x14027829d
  inputs: [x, share, letter_state]    # x = x0 + x1（不看 threshold）；letter_state ∈ none | intro | recommend

# 合约金 → 份额：economy 07 已钉 floor(gold_paid / min_fee)（Invest.pay 返回 share_gain）；只走一条路：
# Contract.sign 由本层执行并调 Share.add(reason: contract)。InvestmentMade 不带 share_gain（词表 §14-21）。
# 独占上缴、出租费、投资 → 发展度：economy 07（formulas/investment.yaml、tribute.yaml）。本文件不再占位。
```

实现约束：

- 所有加减之后饱和到 `caps`，禁止 uint8 绕回。
- `fashion_share_gain` 的份额加值封到 `share_max - 已占`，三槽之和 ≤ `share_max`。
- 原版友好度/关系是**无序对**（一对一个字节）。重构保持对称：`Amity.get(a,b) == Amity.get(b,a)`。要做单向好感，另开新表，不要改这张。
- 份额来源全部走 `Share.add(town, force, delta, reason)`：内部饱和、找槽、空母港写母港（01 §4），再发 `ShareChanged`。

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
| `Amity` / `Relation` / `Influence` / `Gold` / `Overlord` | factions | 命令 |
| `ShareSlot[n]` | factions（按 `town_id` 索引） | `Share.add` / `Contract.sign` |
| 发展度两轨 | world `Towns` | economy 07 / combat / narrative 发 `Towns.apply_develop`，`Towns` 发 `TownDeveloped`；本层只读 |
| 投资、独占上缴、出租费金额 | economy 07 | 本层只供 `Share.exclusiveOwner` |
| 宗主上缴执行 | factions（`forces` 阶段） | 金额式引用 economy `tribute.yaml#suzerain`；发 `SuzerainTributePaid` |
| 介绍信 / 推荐信 | factions 04 | 见 04 |

Economy 卖货结束发 `SoldFashionGoods { town_id, force_id, items[], qtys[], hold_sum, bargain_ok }`（economy 03 §8），由本系统决定加份额、发信、加影响力。Combat 发 `TownOccupied { town_id, from_force_id, to_force_id, battle_id }`，本系统按 `occupation` 削旧约、给新约。禁止 economy / combat 自己写槽。

## 6. 解耦后的配置

```yaml
# config/factions/relations.yaml
schema: dk4.factions.relations
version: 1
pack: vanilla-world0

# 影响力下标引用 config/world/major_seas.yaml（ID 域由 06 登记），本表不再自建 theaters
# 无序对：pair 两端是势力 key，顺序无关；同一对出现两次 = 加载失败。
# 导入器只取 World 表 other > self 的上三角（原版只读低 id 行），下三角丢弃。

# 开局友好度：只列非默认。默认 amity_default
amity_overrides:
  - { pair: [vanilla:castro, vanilla:albuquerque], value: 70 }
  - { pair: [vanilla:castro, vanilla:barudis],     value: 45 }
  - { pair: [vanilla:agate, vanilla:barudis],      value: 30 }
  - { pair: [vanilla:li, vanilla:kurushima],       value: 10 }
  - { pair: [vanilla:li, vanilla:korean_navy],     value: 60 }
  # 其余由管线从 hex +0x38 生成，不要手抄

initial_relations:                # World +0x1F 为 1 的对；关系 key 见 05 war.yaml
  - { pair: [vanilla:albuquerque, vanilla:silvaela],  relation: vanilla:allied }
  - { pair: [vanilla:barudis, vanilla:maldonado],     relation: vanilla:allied }
  - { pair: [vanilla:pasha, vanilla:hayreddin],       relation: vanilla:allied }
```

```yaml
# config/factions/town_shares_initial.yaml   开局合约（每世界包一份；DLL 港口记录 +0x4C 拆出的 factions 初值表）
schema: dk4.factions.town_shares_initial
version: 1
pack: vanilla-world0
# 导入器：3 个 (u8 force, u8 share)；share == 0 的槽丢弃（原版写成 0x1A）[确认]
towns:
  vanilla:london: [{ force: vanilla:clifford, share: 80 }]
  # 其余由管线生成
```

外交动词表（宣战 / 停战 / 结盟 / 合并）只在 05 `war.yaml` 的 `diplomatic_actions` 定义一次；亲笔信只改友好度，走 `Amity.add(reason: letter)`。

完整初值由提取器从 `forces_world0.json` hex `[0x1F:0x38]`、`[0x38:0x51]` 的上三角生成。`relations.yaml` 给人看的是 overrides。

发展度初值属 world `towns.yaml` 的 `develop_initial` 列，本系统需要时 `Towns.develop(town_id, track)` 读取，不复制一份。

## 7. 独立旋钮

- 友好度 / 影响力 / 份额上下限、独占份额（`caps.yaml`）
- 每港合约槽数（原版 3）
- 卖流行品除数 6、高份额阈值 80、发信件数
- 七大海域的划分（改 `world/major_seas.yaml` + 文化圈 `major_sea_id` + 势力初值，不改战斗）
- 占领削份额总量 12、友好度 −50、占领方 +10
- 影响力倍数（修正器 `factions.influence_gain`，剧本包注入）
- 默认友好度 50
- 是否允许三槽份额之和超过 100（原版封到 100−已占，默认保持）

不在本文件改：单价、攻城伤害、经验、投资与上缴金额（economy 07）、宗主上缴开关（剧本包 `scenario.flags.pay_suzerain_tribute`）。

## 8. 对外接口

查询：

```
Amity.get(a, b) -> int
Relation.isAllied(a, b) -> bool
Factions.influence(force_id, major_sea_id) -> int   # 06 规范名；旧写 Influence.get
Influence.total(force_id) -> int            # 原版 fn_140008b6b
Overlord.of(force_id) -> force_id | null
Share.slots(town_id) -> [{ force_id | null, share }]
Share.of(town_id, force_id) -> int          # 无约则 0
Share.canTrade(town_id, force_id) -> bool   # 原版：无份额不能交易
Share.exclusiveOwner(town_id) -> force_id | null   # 槽 0 且 share ≥ exclusive_share
Share.view() -> ShareView                   # 只读批量快照（全港 × 槽），会话内缓存；ShareChanged 失效
```

外部文档的别名统一到上面：`Factions.share(town, force)` / `query_share` = `Share.of`；`Factions.player_share(town)` = `Share.of(town, Force.player())`；`Factions.Share.slots` / `query_slots` = `Share.slots`；`Factions.major_sea(town)` = `MajorSea.of_town`（06）；`Factions.add_influence` = `Influence.add`；`Influence.get` = `Factions.influence`（06 规范名）；`Factions.influenceTotal` = `Influence.total`；`Factions.relation` / `may_attack` = 05 `Relation.get` / `mayAttack`。实现只注册一套名字。

修正器（D2，按 key 求值，本层是消费方）：

| key | ctx | 用处 | 原版提供方 |
| --- | --- | --- | --- |
| `factions.influence_gain` | `{ force, major_sea, source }` | `Influence.add` 前对正增量求值 | 剧本 2 卖流行品 ×2；遗迹首访剧本倍数（world 09/11） |

命令：

```
Amity.add(a, b, delta, reason)
Share.add(town_id, force_id, delta, reason) # 内部饱和、占槽
Share.setSlot(town_id, slot, force_id | null, share)
Contract.sign(town_id, force_id, share_gain) # share_gain 由 economy Invest.pay 算好；本层执行，内部调 Share.add(reason: contract)，发 ContractSigned
Contract.break(town_id, force_id)
Influence.add(force_id, major_sea_id, delta, reason) -> int   # 过修正器（ctx.source = reason）、封 caps；返回修正后、夹紧前的加值
Overlord.set(force_id, overlord | null, reason)
```

事件（本系统发出，载荷只含 ID 与值）：

| 事件 | 载荷 |
| --- | --- |
| `ShareChanged` | `{ town_id, force_id, old, new, reason }` |
| `ContractSigned` | `{ town_id, force_id, share_gain, share }`（`share_gain` = `Contract.sign` 入参；`share` = 签约后份额。合约金不在本载荷：factions 不经手金额，见 economy `InvestmentMade.gold`） |
| `ContractBroken` | `{ town_id, force_id }` |
| `AmityChanged` | `{ force_a, force_b, old, new, reason }` |
| `InfluenceChanged` | `{ force_id, major_sea_id, old, new, source }` |
| `OverlordChanged` | `{ force_id, old, new, reason }` |
| `SuzerainTributePaid` | `{ from_force, to_force, gold }` |

`TownDeveloped` 由 world `Towns` 发；`ExclusiveTributePaid` 由 economy 07 发；外交事件见 05。

订阅：

| 来源 | 事件 | 本层动作 |
| --- | --- | --- |
| economy | `SoldFashionGoods` | 份额 < `share_letter_cut` 按 `fashion_share_threshold` / `fashion_share_gain` 加份额，否则按 `fashion_letter` 交 04 发信；有结果再加影响力 `x0 + x1`（过 `factions.influence_gain`） |
| economy | `InvestmentMade`（不带 `share_gain`） | 不改份额；合约份额只经 `Contract.sign`（town-life 调用） |
| economy | 交易所 / 军事加投资优待份额（07 §4.3，已有份额才走） | economy 调命令 `Share.add(reason: investment)`；`reason: contract` 只由 `Contract.sign` 内部使用 |
| economy | 投资转化（07 §4.4） | 直接调 `Influence.add(reason: investment)` |
| combat | `TownOccupied` | 按 `occupation` 削旧约、友好度 −50、给占领方；**不**算战斗伤害 |
| world | 遗迹首访（`on_enter_town_map_point`，kind 9/10/11） | `Influence.add(reason: remains)` |
| town-life | 广场寻求支持结果 A | `Share.add(town, player, 1, reason: support)` |
| narrative | 外交脚本 | `Amity.add` / 05 协议命令 |

`reason` 用稳定字符串：`fashion_sale`、`contract`、`investment`、`scheme`、`capture`、`letter`、`support`，便于 UI 与回放。

不暴露 25 字节数组、不暴露 `CTown+0x18`。

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| `FORCEAMITY_MIN/MAX` 符号整数 | 只有符号；观测 10–70；`fn_1400089f9` 加减路径封 100 | 头文件常量已恢复 |
| `FORCE_INFLUENCE_MIN/MAX` 符号值 | 加减路径封 9999；符号本身未读到 | 65535 |
| 7 格 = 影响力 | 宽度 USHORT；下标 = 七大海域已确认 | `eForceAimKind`；25 航海分区 id |
| +0x1F 旗 | 三对对称 1 | 具体 `eForceRelation` 名（同盟/从属/联姻） |
| 敌我阈值 | 「友好势力无法攻击」 | `amity > 50` 即友军 |
| 合约金 / 投资 → 份额、最低合约费、独占上缴、出租费、投资 → 发展度 | economy 07 已钉 | 在本篇重复公式 |
| 独占判定 | 已钉：槽 0 且 share ≥ 100（`fn_140004c19`） | 「任一槽 100」 |
| `fn_140008279` 占领通知 | 参数 `(旧势力, town, -50, 新份额, 0x1A)` | 确认的玩法效果 |
| 友好度对自己 −50 | 占领方削自己旧槽时发生 | 有效果 |
| 介绍信「小城市」判定 | 已钉：`is_capital == 0`（04 §2.5） | 发展度阈值 |
| 消息 `0x167` | 宗主上缴时发，参数（宗主名、金额） | mesfile 文件偏移 |

份额加减的除数 6、阈值 80、封顶 `100−已占` 是确认的；**不要**把它们复制进 economy 物价公式。
