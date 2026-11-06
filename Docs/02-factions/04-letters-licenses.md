# 介绍信 / 推荐信

交易所卖流行品发信，玩家持有一封，拿到王宫 / 总督府换份额。委托 12/13 要的是「弄到并交回」这封信，不是第二发放口。份额加减仍走 [03-relations-influence.md](03-relations-influence.md) 的 `Share.add`；本篇管**信本身**的获得、持有、花费与「小城市」判定。

亲笔信（计略 `letter_amity`）是外交文书，不在本系统。`CLetterSelectView` 是选字 / 名字 UI，与介绍信无关。

## 1. 职责边界

**管：**

- 介绍信 / 推荐信两种 `letter_kind` 的状态机（无 / 介绍 / 推荐，互斥、单槽）
- 从经济事件 `DealCommitted` 判定是否发信、发哪种、是否升级
- 在宫殿花费信件、换多少份额、哪些港算小城市
- 委托 kind 12/13 的完成条件与交信消耗
- `eLICENSELOCATIONCHOICES` 的 ID 空间（宫殿菜单项表长）
- 持有形态：原版旗位，或内容包改成 `eItem`（二选一，禁止双轨并行）

**不管：**

- 流行品件数怎么从舱位统计、份额 `< 80` 时加多少点 → `economy` 发事件，公式见 `F-ECO-05` / `F-ECO-06`
- `ShareSlot[3]` 存储、`100 − 已占` 封顶 → 03，本系统只 `Share.add`
- 签合约金、珍品陈列、品牌奉献 → 宫殿其它菜单项，设施文档 / 03 / items
- 231 件宝物目录 → `items`（原版信**不是**其中之一）
- 亲笔信友好度 → 03 / 计略
- `CPalace` 绘制、菜单皮肤 → `ui`

系统之间只传 `force_id`、`town_id`、`person_id` / `fleet_id`、`letter_kind` 和标量。不传 `CPlayerPerson*`，不把信写成交易货。

## 2. 原版事实

### 2.1 不是 `eItem`

- `[确认]` `eItem` 0–230 无「介绍信」「推荐信」。来源：`ItemID.h`、`Docs/_data/items.tsv`。
- `[确认]` 全程序对 `fn_1400041ce` 的写入只有交易所发信与宫殿 / 委托消耗；没有 `AddLoad` / `CItemElement` 路径。来源：`xref.py` → `0x14027803e` / `250` / `47c`、`0x1401631cb`、`0x140157008` / `04e`。
- `[确认]` 宫殿菜单扫 231 件道具只为珍品陈列（kind 6/8/9/0xb 等），不把信当道具。来源：`fn_140014bfa` 循环 `eItem >= 0xe7`。

### 2.2 持有：玩家对象 `+0x78` 两位

`[确认]` 读写都经 `fn_140004e17`（`GetPlayer`：`DAT_1442a7b40+0x96a8` 再 `+0x2c18`）返回的对象。位域在该对象 `+0x78` dword。

| 函数 | VA | 作用 |
| --- | --- | --- |
| `fn_1401e0d70` | `0x1401e0d70` | `1 << bit` |
| `fn_1401e0a20` | `0x1401e0a20` | 测位 |
| `fn_1401e1270` | `0x1401e1270` | `r8=1` 置位 / `r8=0` 清位 |
| `fn_14000bd98` | `0x1401e08a0` | 读状态 |
| `fn_1400041ce` | `0x1401e1130` | 写状态 |

`bd98` `[确认]`：

```
if test_bit(+0x78, 1): return 1   # 推荐信
if test_bit(+0x78, 0): return 0   # 介绍信
return 2                          # 无信
```

`41ce(obj, kind)` `[确认]`：先清 bit0 与 bit1，再按 `kind` 置一位。

| `kind`（edx） | 结果 | 调用点 |
| --- | --- | --- |
| `0` | 介绍信 | `CommitDeal` `xor edx,edx`（`0x140278034`） |
| `1` | 推荐信 | 发推荐 / 升级（`0x140278243` / `46f`） |
| `2` | 无信 | 宫殿花费、委托交付（`mov edx,2`） |

其它 `kind` 清光后不置位，等价无信。互斥：不能同时持有两封。已有推荐信时交易所不再改这位。

`[推断]` 该 dword 即 `CPlayerPerson::vfunc_06/07` 存读的 `+0x78`（同宽、同偏移、同在玩家侧）。`0x96a8+0x2c18` 指针链未钉到类名，不升格。`PLAYERSTATE_BITS` 符号在 `CPlayerFleetData.h`，是否包含这两位 `[未知]`。

### 2.3 获得：只在交易所，且份额已高

来源：`srcdata/docs/trade_stack.md` `CommitDeal 0x140277090`；SC 正文 `CTrade.cpp`；立即数以 objdump 为准（反编译会把上一句的 `edx=0x38e` 残留写进 `41ce`，**以汇编为准**）。

- `[确认]` 本港占有率 `fn_1400088f0(town) >= 0x50`（80）才走发信枝；低于 80 走加份额 `122f6`，见 03 与 `F-ECO-05`。
- `[确认]` `x` = 本次卖出的两种流行品件数之和（`s564 = s4F4 + s4F0`；STK3 已按件数）。
- `[确认]` `bd98 == 2`（无信）：`10 <= x <= 15` 发介绍信（`41ce(0)`）；`x >= 16` 发推荐信（`41ce(1)`）。比较是 `cmp 0xa` / `0xf` / `0x10`。
- `[确认]` `bd98 == 0`（已有介绍信）且 `x >= 16`：升级推荐信（`41ce(1)`）。
- `[确认]` 已有推荐信（`bd98 == 1`）或件数不够：不改信。
- `[确认]` 文案：`mesfile_sc.txt` `0x7be3`、`0x7ca3`、`0x7cd7`、`0x7d17`、`0x7d47`。`0x7d47` 三句连在一起：介绍信去**小城市**王宫 / 总督府；推荐信「不管哪个城市」；「介绍信只能用在小城市，如果是推荐信，则任何城市都通用。」
- `[确认]` 任务日志句「得到介绍信。得到推荐信。」`0x318b`。

重构后 economy 只 `emit DealCommitted`；是否发信由本系统决定。禁止 `CTrade` 自己 `41ce`。

### 2.4 花费：宫殿 `fed4`

`CPalace.cpp` 只有 ctor / dtor。`[确认]` 玩法在 `fn_14015b600.cpp` 这一簇，紧挨 `CPlazaView`。

宫殿主循环 `fn_14000c5fe`（`0x140161c50`）`[确认]`：

1. `fn_140014bfa` 组 **8** 项菜单（`f3ad(..., 8)`，项步长 `0x18`，下标 0–7）
2. `switch` 跳转表在 `0x140161d14`

| 槽 | 默认亮 | 何时改亮 | 跳到 | 作用 |
| --- | --- | --- | --- | --- |
| 0 | 暗 | 本势力本港份额 `== 0` | `fn_140013d1d` | 签新约（合约金，03） |
| 1 | 亮 | 常亮 | `fn_14001077b` | 协助防卫 |
| 2 | 暗 | `bd98 == 1` 推荐信 | `fn_14000fed4` | 用信扩份额 |
| 3 | 暗 | `bd98 == 0` 介绍信 | `fn_14000fed4` | 同上 |
| 4 | 暗 | 持有可陈列珍品 | `fn_140003afd` | 陈列 |
| 5 | 暗 | 已有寄放 | `fn_14000dcce` | 取回 |
| 6 | 暗 | `[未知]` 品牌相关 | `fn_1400065c8` | 品牌奉献 |
| 7 | 亮 | 常亮 | 空跳 | 离开 |

2 与 3 都进 `fed4`；发多少点由信旗决定，不由槽号决定。文案 id：推荐 `0x54a`、介绍 `0x54c`、首都拒介绍 `0x55f`、扩约 `0x560`、无份额落脚 `0x561`。对应 mesfile `0xb3d3`、`0xb3ff`、`0xb69f`、`0xb72b`。

`fn_14000fed4`（`0x140162d30`，RTC `eTownShare` / `aeShareForce`）`[确认]` 成功路径：

1. 三槽已满且没有自己的槽 → 拒（`0x543`，「没有其他势力的份儿了」）
2. 己方份额 `>= 100` → 拒（`0x549`）
3. 介绍信且 `fn_14000c71b(town) == 1` → 拒（`0x55f`，「光凭介绍信有点困难」），**不消耗**
4. 己方份额 `< 1`：加 **1** 点（`0x561`）
5. 己方份额 `>= 1`：推荐信加 **10**，否则加 **5**（介绍信走 5）；再与剩余空间取 `min`（`fn_140011bd0`）
6. `fn_1400122f6(town, force, grant, 1)` 写份额
7. `41ce(2)` **消耗**信

剩余空间初值 `s4C = 100`，若已有自己的槽则改成 `100 − 己方份额`。随后对「本地」他势力份额还有一轮扣减 / 加回（`fn_140012c0b` / `fn_14000cffe`），命名未恢复，见 §9。

无信时槽 2/3 不亮，正常玩法进不了 `fed4`。`bd98 == 2` 落在「否则 +5」枝是防御性代码，`[推断]` 不可达。

### 2.5 小城市 = 非首都

- `[确认]` `fn_14000c71b`（`0x1402626d0`）：`GetFixTown`（`fn_140003643` → `fn_140004a66`，步长 `0x84`）读 **`BYTE [STown+0x24]`**，非 0 返回 1，否则 0。
- `[确认]` `STown+0x24` 即港口表 `is_capital`。来源：`Docs/01-world/02-towns.md`、`towns_world0.json`。21 座首都，其余 76 座非首都。
- `[确认]` 介绍信在 `c71b == 1`（首都）被拒；推荐信不看这一位。与 `0x7d47`「介绍信只能用在小城市 / 推荐信任何城市」一致。
- `[确认]` **不是**发展度阈值。03 把「小城市判定」列成未知并猜测发展度，以本篇汇编为准。
- `[推断]` 文案「小城市」= `is_capital == 0`。`eTownKind` 海域图循环上界 2 是首都 / 非首都画法，不是第三套城市等级。

### 2.6 委托：消耗方，不是发放方

`[确认]` 到港交付 `fn_140156c70` 按 `fn_14000d9bd` 的 kind `<= 0x0D` 跳转（表 `0x14015708c`，14 项）。

| kind | 入口 | 完成条件 | 然后 |
| --- | --- | --- | --- |
| 12 | `0x140156fc9` | `bd98 != 2`（介绍**或**推荐） | `41ce(2)` 消耗 |
| 13 | `0x14015700f` | `bd98 == 1`（必须推荐） | `41ce(2)` 消耗 |

文案 `[确认]`：`0x1549f`「能给我弄到介绍信吗？要是推荐信那就更好了。」`0x154d7`「希望弄来推荐信。」与 08 篇 pack_index 12/13 对齐，这里升格为确认。

报酬 `fn_140010e10` 跳转表 `0x140158d80` `[确认]`：

| kind | 金额 |
| --- | --- |
| 12 | 持介绍信 `0x186a0 = 100000`；否则 `0x1fbd0 = 130000`（推荐信走这枝，「更好了」） |
| 13 | `0x249f0 = 150000` |

之后若 `fn_140004a1b == 0`，金额 `* 3 / 2`。该开关语义 `[未知]`。

委托**不调用** `41ce(0/1)`。玩家必须先在交易所拿到信，再送到公会；交了就不能再去宫殿用。

### 2.7 `NUM_LICENSELOCATIONCHOICES`

- `[确认]` 符号：`NUM_LICENSELOCATIONCHOICES`、`INVALID_LICENSELOCATIONCHOICES = -1`、`LICENSELOCATIONCHOICES_ID`。来源：`LicenselocationchoicesID.h`、`asserts.txt`、`constants.json`。
- `[确认]` 头文件未写出整数；`num_values.json` / RTC 名表无此项。
- `[推断]` 宫殿 `14bfa` 的 8 项（0–7）就是这个枚举：个数、下标、与「在许可证场所做选择」的名字同形。未在断言里读到立即数 8，不升格。
- 禁止把该枚举猜成 97 港、76 座小城市或 21 座首都。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空 / 非法 | 重构 |
| --- | --- | --- | --- | --- |
| 信种类 | `bd98` 返回值 0/1/2 | 3 态 | 2 = 无信 | `letter_kind`: `none` / `intro` / `recommend` |
| 信旗位 | `+0x78` bit0 / bit1 | 2 bit | 皆 0 | 存档一个枚举，不要两 bit 外露 |
| 宫殿选项 | 菜单下标 0–7 | 8（运行时读表长） | `-1` | `location_choice_id` |
| `eLICENSELOCATIONCHOICES` | 符号 only | `[未知]` 整型；8 `[推断]` | `-1` | 与 `location_choice_id` 同一张表 |
| 小城市 | `is_capital == 0` | World0：76 | 首都 21 座不是 | 谓词，不是第二套 `town_id` 枚举 |
| `eItem` 信 | 无 | — | — | 仅当 `storage: item` 才分配 `id >= 1000` |

哨兵：

- 无信用 `none`，不要用 `item_id = 0`（南瓜种子）或 `232`（道具空槽）或 `106`（货格空）。
- 空菜单项用 `-1`，不要用 0（0 是签新约）。
- 一次只允许一封。升级是原地改 kind，不是第二槽。

模组若改 `storage: item`：两件新道具，kind 自定，不覆盖 0–230；持有查询改走 `Items.has`，花费走 `Items.consume`。原版包必须 `storage: flag`。

## 4. 算法 / 公式

数值进 `config/formulas/factions.yaml` 与 `config/caps.yaml`。原版立即数当 default。禁止在宫殿 / 交易所代码里写 `5`、`10`、`80`、`16`。

```yaml
# config/caps.yaml  （本系统用到的键；与 03 / 13 共用）
share_cap: 100
share_high: 80
share_slots_per_town: 3
letter_lo: 10              # 介绍信件数下沿（含）
letter_hi: 16              # 推荐信件数下沿（含）；介绍信含到 hi-1
```

```yaml
# config/formulas/factions.yaml  本系统段
schema: dk4.formulas.factions
version: 1

# 获得。件数阈值与 03 letters_from_fashion / F-ECO-06 必须同键同值
letters_from_fashion:
  share_high: 80
  intro_min_qty: 10          # letter_lo
  intro_max_qty: 15          # letter_hi - 1
  recommend_min_qty: 16      # letter_hi
  upgrade_intro_to_recommend_min_qty: 16

# 宫殿花费
letters_at_palace:
  intro_share_delta: 5
  recommend_share_delta: 10
  no_share_foothold: 1       # 己方份额 < 1 时用信落脚
  remaining_base: 100        # 与 share_cap 对齐

# 委托报酬（kind 12/13）
letters_as_request:
  intro_request_gold_if_intro: 100000
  intro_request_gold_if_recommend: 130000
  recommend_request_gold: 150000
  gold_boost_numer: 3        # *3/2 的开关条件 [未知]
  gold_boost_denom: 2
```

### 4.1 发信 `grant_from_fashion`

输入：`share`、`sold_fashion_qty`、当前 `letter_kind`。只在 `share >= share_high` 时运行。

```
x = sold_fashion_qty
if kind == none and intro_min_qty <= x <= intro_max_qty:  grant intro
if kind == none and x >= recommend_min_qty:               grant recommend
if kind == intro and x >= upgrade_min_qty:                grant recommend
else:                                                     no-op
```

`share < share_high` 不进本函数（那是 `F-ECO-05`）。

### 4.2 小城市 `is_small_city(town_id)`

原版包：

```
is_small_city(town_id) = not World.is_capital(town_id)
```

内容包可换谓词（§6），引擎只调这一处。禁止散落 `if develop < K`。

### 4.3 花费 `spend_at_palace`

```
kind = Letters.get(holder)
if kind == none: reject
if kind == intro and not is_small_city(town_id): reject   # 不消耗
if Share.foreign_slots_full(town_id, force_id): reject
if Share.of(town_id, force_id) >= share_cap: reject

if Share.of(town_id, force_id) < 1:
    grant = no_share_foothold
else if kind == recommend:
    grant = recommend_share_delta
else:
    grant = intro_share_delta

grant = min(grant, remaining_room(town_id, force_id))
Share.add(town_id, force_id, grant, reason="letter")
Letters.set(holder, none)
```

`remaining_room` 原版至少是 `share_cap − 己方份额`。他势力「本地」份额如何挤占 `[未知]`，未还原前**不要**实现那一轮扣减；最坏只是略多给几点，可被 `share_cap` 封住。禁止发明「发展度 × 5%」。

`Share.add` 内部仍走 03 的饱和。本系统不直接写槽字节。

### 4.4 委托完成

```
kind 12: ok = Letters.get(holder) in {intro, recommend}
kind 13: ok = Letters.get(holder) == recommend
if ok:
    gold = table[kind, Letters.get(holder)]
    # gold_boost 条件未还原前不乘
    Letters.set(holder, none)
    emit RewardGold { gold }
```

实现约束：发信、升级、消耗必须在同一同步栈里改唯一状态；禁止「先给道具再清旗」的双轨。`storage: item` 时用事务化转移（扣信 → 加份额 / 发金币 → 失败回滚）。

## 5. 耦合分析

原版：

- `CommitDeal` 直接 `bd98` / `41ce` / `122f6`，贸易模块知道 80、10、16
- 宫殿 `fed4` 直接改份额并清信
- 委托交付直接清信并发金币
- 小城市判定读港口定义表 `+0x24`
- 信旗打在玩家对象上，和人物存档绑在一起
- 8 项宫殿菜单把签约、防卫、用信、珍品、品牌焊在一个 `switch`

重构后数据所有权：

| 状态 | 所有者 | 别人怎么碰 |
| --- | --- | --- |
| `letter_kind` | factions（本篇） | `Letters.get/set`；`storage: item` 时底层转调 items |
| `ShareSlot[3]` | factions（03） | `Share.add` |
| `is_capital` | world | `World.is_capital(town_id)` |
| 流行品件数、份额是否已 ≥80 | economy 事件字段 | 本系统只读事件，不读舱位 |
| 委托是否完成 | town-life | 查询 `Letters.satisfies(request_kind)`，消耗由本系统执行 |
| 宫殿 8 项菜单 | town-life 设施入口 | 用信项调 `Letters.spendAtPalace`；其它项不进本系统 |

禁止 economy 发信、禁止 town-life 自己清旗、禁止把 `is_capital` 复制进本表。

## 6. 解耦后的配置

```yaml
# config/factions/letters.yaml
schema: dk4.factions.letters
version: 1
pack: vanilla-world0

# 原版：旗。改成 item 时 kinds[].item_id 必填且 ≥ 1000
storage: flag                  # flag | item

kinds:
  - id: 0
    key: intro
    name: 介绍信
    name_i18n: ui.letter.intro
    bit: 0                     # 仅文档对照；运行时不要散落 bit 号
    bd98: 0
    usable_in: small_city
    palace_delta_key: intro_share_delta
    grant_41ce: 0
    item_id: null              # storage: item 才填
  - id: 1
    key: recommend
    name: 推荐信
    name_i18n: ui.letter.recommend
    bit: 1
    bd98: 1
    usable_in: any_city
    palace_delta_key: recommend_share_delta
    grant_41ce: 1
    item_id: null
none:
  key: none
  bd98: 2
  grant_41ce: 2                # 消耗

# 小城市。原版包唯一谓词：非首都
small_city:
  predicate: not_capital       # not_capital | explicit_list | develop_lt
  town_ids: []                 # explicit_list 才用
  develop_track: commerce      # develop_lt 才用
  develop_threshold: null      # 原版不是这条；未还原前禁止填整数

# 获得。fashion 阈值与 formulas/factions.yaml 同键；此处只声明谁准许发
obtain:
  fashion_sale:
    enabled: true
    require_share_at_least: share_high
    source_event: DealCommitted
  requests:
    enabled: false             # 原版委托不发信。模组可改 true
    grant_on_complete: null
  script:
    enabled: true              # 叙事可 Letters.grant；原版 exe 无调用

# 消耗
spend:
  palace:
    enabled: true
    consume: true
    foothold_if_share_lt: 1
  request_delivery:
    enabled: true
    consume: true
    kinds:
      - request_kind: get_intro_letter
        original_kind_byte: 12
        accept: [intro, recommend]
        gold:
          intro: intro_request_gold_if_intro
          recommend: intro_request_gold_if_recommend
      - request_kind: get_recommend_letter
        original_kind_byte: 13
        accept: [recommend]
        gold:
          recommend: recommend_request_gold

# eLICENSELOCATIONCHOICES。表长运行时读。vanilla 8 项 [推断]
location_choices:
  - { id: 0, key: sign_contract,     enabled_when: share_eq_0,           dispatch: Contract.sign }
  - { id: 1, key: assist_defense,    enabled_when: always,               dispatch: Palace.assistDefense }
  - { id: 2, key: spend_recommend,   enabled_when: letter_eq_recommend,  dispatch: Letters.spendAtPalace }
  - { id: 3, key: spend_intro,       enabled_when: letter_eq_intro,      dispatch: Letters.spendAtPalace }
  - { id: 4, key: display_treasure,  enabled_when: has_displayable_item, dispatch: Palace.displayTreasure }
  - { id: 5, key: retrieve_treasure, enabled_when: has_deposited_item,   dispatch: Palace.retrieveTreasure }
  - { id: 6, key: offer_brand,       enabled_when: unknown,              dispatch: Palace.offerBrand }
  - { id: 7, key: leave,             enabled_when: always,               dispatch: ui.close }
```

World0 / World1 同一 schema。信旗是玩家状态，不进内容包实例表。

`config/ids/` 从本表收集 `letter.intro` / `letter.recommend` / `letter.none` 与 `location_choice.*`。

文案 key 进 `ui/strings.*.yaml`：`0x7d47`、`0xb69f` 等只在那里出现，不进本文件。

## 7. 独立旋钮

- 发信件数 10 / 15 / 16、高份额阈值 80
- 宫殿 +5 / +10 / 无份额 +1
- 小城市谓词（首都以外、显式名单、或以后还原出的发展度）
- `storage: flag | item`
- 委托 12/13 是否消耗、报酬金币、是否接受「介绍信任务交推荐信」
- 宫殿菜单项增减（表长；原版 8）
- 是否允许无信仍扩份额（原版菜单不亮，默认否）
- 叙事 `Letters.grant` 开关

不在本文件改：市价、合约金公式、发展度上限、`eItem` 0–230 名录、亲笔信友好度。改 `share_cap` 在 `caps.yaml`，本系统的 `remaining_base` 必须跟它走。

## 8. 对外接口

查询：

```
Letters.get(holder_id) -> none | intro | recommend
Letters.canSpend(holder_id, town_id) -> bool
Letters.isSmallCity(town_id) -> bool
Letters.satisfies(holder_id, accept: [kind]) -> bool
Palace.locationChoices(town_id, force_id, holder_id) -> [{ id, key, enabled }]
```

`holder_id` 原版是玩家人物 / 玩家势力侧对象；重构用 `person_id`（主角）或 `force_id` 之一，内容包写明，禁止两处各存一份。

命令：

```
Letters.grant(holder_id, kind, reason)       # 发或升级；已有推荐则 no-op
Letters.spendAtPalace(holder_id, town_id, force_id)
Letters.deliverForRequest(holder_id, request_id)
Letters.clear(holder_id, reason)             # 仅调试 / 剧本；玩法走 spend/deliver
```

事件（载荷只含 ID 与值）：

| 事件 | 载荷 |
| --- | --- |
| `LetterGranted` | `{ holder_id, old, new, reason }` |
| `LetterSpent` | `{ holder_id, kind, town_id, share_delta, reason }` |
| `LetterDelivered` | `{ holder_id, kind, request_id, gold }` |
| `LetterRejected` | `{ holder_id, kind, town_id, why }` |

`reason`：`fashion_sale`、`upgrade`、`palace`、`request_12`、`request_13`、`script`。`why`：`not_small_city`、`slots_full`、`share_capped`、`no_letter`。

订阅：

| 来源 | 事件 | 本层动作 |
| --- | --- | --- |
| economy | `DealCommitted { town_id, force_id, fashion_sold_qty, share }` | `grant_from_fashion` |
| town-life | 宫殿槽 2/3 | `spendAtPalace` |
| town-life | `RequestCompleted` 且 kind ∈ {12,13} | 先 `satisfies`，再 `deliverForRequest` |
| narrative | 脚本 | `grant` / `clear` |

不暴露 `+0x78`、不暴露 `bd98` 的 0/1/2、不暴露 `CPalace*`。

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| `NUM_LICENSELOCATIONCHOICES` 整数 | 只有符号；宫殿菜单 8 项 | 确认等于 8（YAML 里 8 必须标推断） |
| 8 项 = 该枚举 | 个数与「场所选择」同形 | `eLicenseLocationChoices` 的官方成员名 |
| `4e17` 返回值的类名 | 偏移链 `+0x96a8+0x2c18`，`+0x78` 与 `CPlayerPerson` 同形 | 确认就是 `CPlayerPerson` 或 `CPlayerFleetData` |
| `PLAYERSTATE_BITS` 是否含信旗 | 符号在舰队数据头文件 | 两位的正式枚举名 |
| `+0x78` 其余 bit | 只证明 bit0/1 是信 | 其它位的语义 |
| 他势力「本地」份额挤占 | `fed4` 调 `12c0b` / `cffe` 后改 `sBC` | 任何具体式子 |
| `0x547` 枝 | `sB4==0` 时拒 | 与友好度 / 母港的确认关系 |
| `fn_140004a1b` | 真则委托金不 `*3/2` | 难度 / 剧本 / 技能 |
| 槽 6 何时亮 | 品牌奉献 | 与「年结品牌」的双射 |
| 防卫项是否改份额 | `1077b` 有份额读取 | 防卫成功 = 加份额 |
| 无信调用 `fed4` | else 枝 +5 | 可玩路径 |
| 发展度 < K = 小城市 | 文案「小」 | 原版判定（原版是首都标记） |
| World1 不同规则 | 同一 exe | 第二套件数阈值 |
| 存档字节位置 | 人物 `+0x78` dword `[推断]` | 存档文件绝对偏移 |
| NPC 势力是否持信 | 只见玩家 `4e17` | 每势力一封 |

不要把 10 / 16 / 5 / 10 复制进 economy 物价公式。不要把介绍信做成 231 件里的隐藏项。不要用 `CLetterSelectView` 当本系统的 UI 类。
