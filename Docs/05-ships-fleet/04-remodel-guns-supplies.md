# 改装、火炮与补给

造船厂把一艘已有的船改成另一套帆 / 部件 / 炮 / 船首像，并修理、补给。战斗里的炮弹对象也在这里给目录，伤害公式留给 `07-combat`。

## 1. 职责边界

本系统管：

- 改装件 `eAddParts`（9）与甲板改装槽（4）
- 桅位 `eShipMast`（3 根）与每根桅的状态位（`MASTSTATE_BITS`）
- 船首像 `eFigureHead`（与道具 114–123、199–200 重叠，见下）及其修正器贡献
- 帆值 / 改装件 / 桅位的数据表 `rig.yaml`、`add_parts.yaml`（帆值与必要水手由 Ships 现算，D6）
- 换炮命令与改装计价（炮种目录、门数、威力见 `08-gun-shell-catalog.md`）
- 飞行炮弹实例 `eShell`（100）——是弹道对象，不是弹药种类表
- 补给 `eShipSupply`（水、粮食至少两种）
- 修理 / 改造 / 买船的**天数与费用参数**

本系统不管：

- 船体目录与货格（`01`）
- 舱室绑定（`03`）；本系统只发「追加船舱」命令去改 `cabin_kind`
- 市价（经济系统报价，造船厂付钱）
- 炮击伤害、装填时间的战斗结算（本系统提供「炮种 id、炮位、有几个炮手室」）

## 2. 原版事实

### 计数

| 符号 | 值 | 来源 |
| --- | --- | --- |
| 符号 | 值 | 证据类型 / 来源 |
| --- | --- | --- |
| `NUM_ADDPARTS` | 9 `[确认]` | 循环上界：`fn_140013d68`（`0x1400a9d20`）`eAddParts < 9`；`vfunc_04` 估价同 |
| `NUM_DECKADDPARTS` | 4 `[推断]` | RTC 投票（4×8）+ `CDeckRemodelView` UI 循环 `>= 4` |
| `NUM_SHIPMAST` | 3 `[推断]` | 比较立即数：`eShipMast` 局部在十余个函数里只与 3 比较（`0x1400a9d93`、`0x14023f5ef`、`0x14023fd69` 等）；`ShipMastID.h` 的 100 是投票误采 |
| `NUM_GUNTYPE` | 6 `[确认]` | 判界：`fn_140012760`（`0x140123300`）`< 6` |
| `NUM_BATTLEGUN` | 21 `[确认]` 池 | 构造计数 + 循环上界 `0x15`（`08`）；头文件 5 是投票误采，作废 |
| `NUM_SHELL` | 100 `[确认]` | 构造计数 + 循环上界 `0x64`（`08`） |
| `NUM_FIGUREHEAD` | 12 `[推断]` | 道具 kind 5 共 12 件；`+0x53` 比较值 0–11 都有对应道具；空值 `0x0D` 像「计数+1」哨兵。无判界证据 |
| `NUM_SHIPSUPPLY` | 2 `[推断]` | RTC 投票（与 2 比 5 次）+ `anSupply` 2×int32 |

甲板改装 View `CDeckRemodelView` RTC：`eDeckAddParts`、`eShipMast`、`eShipCabin`、`eButton`。桅循环 `>= 3`，改装件循环 `>= 4`，舱室循环 `>= 0x1F`。来源：`CDeckRemodelView.cpp`。

### 实例上的两个位域

- `[确认]` 改装件：`CShip+0x4E` u16，`vfunc_06`（`0x1400a77c0`）读出时 `btr eax, 0xf` 清位 15；`fn_1400091e2(ship, n)` = 第 n 位是否置位。初值来自船体定义 `+0x0A` / 船模 `+0x14`（`01`）。
- `[确认]` 每根桅一字节：`CShip+0x23+i`（`i` 0–2），经 `vfunc+0x28` 读。四个判定函数都先判 `0 <= i < 3`，再测一位（`fn_14023ef10(&b, bit)`）。帆值 / 必要水手里各位的加项 `[确认]`（`06-ocean/01-sailing.md` §2），位名 `[推断]`：

| 函数 | 位 | 用途 | 帆值 / 人手加项 | 位名 |
| --- | --- | --- | --- | --- |
| `fn_14000ad76`（`0x14023fd40`） | 0 | 该位置有桅 | 必要水手 +grade 表 | `[推断]` 有桅 |
| `fn_14000f169`（`0x14023f500`） | 1 | 帆型（改了就 +2000）；越界返回 1 | 有桅且置位：横帆值 7；否则纵帆值 8 | `[推断]` 四角帆（横）/ 三角帆（纵） |
| `fn_140007ce3`（`0x14023fe80`） | 2 | 加横帆件 | 横帆 +3；必要水手 +2；估价 +100 | `[推断]` 上帆 |
| `fn_14000a970`（`0x14023fde0`） | 3 | 加纵帆件 | 纵帆 +4；必要水手 +1；估价 +100 | `[推断]` 斜帆 |

  船模默认值只出现 `0x00 0x01 0x03 0x05 0x07 0x0F`（`01` / `05`），与「位 0 存在、位 1–3 叠加」相符。

### 改装件与帆（文案）

来源：`mesfile_sc.txt` `0xc39f`–`0xc767`。

**部件（与 `eAddParts` 下标的对应见下表，名字多为 `[推断]`）：**

- 追加船舱：速度降低
- 撞角：侧面冲撞
- 船首炮：向前攻击，威力小
- 船首帆：中型以上；提高**横帆**，顺风加速
- 装甲板：耐久提高、速度稍降
- 二层装甲板：耐久再提高、速度再降
- 船尾炮：向后攻击，命中率低
- 船尾帆：大型船；提高**纵帆**，逆风加速
- 更换船首像
- 更改大炮种类

9 个 `eAddParts` 与上面清单不是逐条对齐（清单把船首像 / 换炮也写进去了）。

各位在代码里的**作用** `[确认]`（帆值 `fn_140006f64` / `fn_140002612`、罚值 `fn_14001292c`、必要水手 `fn_14000feac` / `fn_140001e2e`、装甲 `fn_14000c35b`、门数 `fn_140004836`、估价 `vfunc_04` 跳表 `0x1402302ac`）；名字由作用对文案 `[推断]`：

| 位 | 作用 | 估价 | 名 |
| --- | --- | --- | --- |
| 0 | 主桅横帆 +2（×8）；必要水手 +1 | 500 | 船首帆（「提高横帆」）`[推断]` |
| 1 | 主桅纵帆 +3（×8）；必要水手 +4 | 400 | 船尾帆（「提高纵帆」）`[推断]` |
| 2 | 装甲：耐久按体型档 +6/8/10；帆罚 +1 | 1000 | 装甲板 `[确认]` 装甲 |
| 3 | 同 2（两位共用一张表） | 1000 | 二层装甲板 `[确认]` 装甲 |
| 4 | 帆罚 +2 | 1000 | 追加船舱（「速度会降低」）`[推断]` |
| 5 | 帆罚 +2 | 1000 | 追加船舱 `[推断]` |
| 6 | 门数 +6；船首弧开火；必要水手 +2 | 400 | 船首炮 `[确认]` |
| 7 | 门数 +6；船尾弧开火；必要水手 +2 | 400 | 船尾炮 `[确认]` |
| 8 | happening 5（海域灾害）免疫（`07-combat/05`） | 400 | `[未知]`（撞角候选） |

**帆（`eShipMast` / 甲板加帆按钮 `CDeckAddSailButton`）：** 文案与桅位 / 改装位的对应 `[推断]`：船首帆 = 改装位 0、船尾帆 = 改装位 1、四角帆 / 三角帆 = 桅 bit1 置位 / 清零、上帆 = 桅 bit2、斜帆 = 桅 bit3。

| 帆 | 效果 | 约束 |
| --- | --- | --- |
| 船首帆 | 横帆↑，顺风↑ | 中型以上 |
| 船尾帆 | 纵帆↑，逆风↑ | 大型 |
| 上帆 | 横帆↑，顺风↑ | — |
| 斜帆 | 纵帆↑，逆风↑ | — |
| 三角帆 | 逆风↑ | 不能张挂在四角帆**之前** |
| 四角帆 | 顺风↑；文案另说「能提高纵帆、横帆两种」 | 不能张挂在三角帆**之后** |

加算数值 `[确认]` 见上两表与 `06-ocean/01-sailing.md` §2（帆值四项、罚值、必要水手）；三角帆 / 四角帆前后互斥规则落在哪个函数 `[未知]`。

「在海上不能更换船首像。」来源：`0x3753`。

### 装甲数值

`fn_14000c35b`（`0x1400a0420`）RTC `anAddArmorSizeTable`，`[确认]` 原始指令：

```
table = [6, 8, 10]                      # 栈上 3 个 int
delta = 0
for part in 2..3:                       # cmp 3; jg
    if new.has(part) and not old.has(part): delta += table[tier(new)]
    if not new.has(part) and old.has(part): delta -= table[tier(new)]
new.durability = clamp(1, new.durability + delta, durability_max(new))   # fn_1400103d4，vfunc+0x70 写回
```

`tier(new)` = `fn_14000d152` = 船模 `CShipType+0x18` 体型档 0/1/2（无船模时 4，越界读栈）。**不是**耐久阈值。`durability_max` 是 `fn_1400084db`，同一张 `{6,8,10}` 也在 `.data 0x1406876d0` 里（`01`）。`view+0x340` = 预览船（新），`view+0x338` = 当前船（旧）。

### 改装费

`fn_140013d68`（`0x1400a9d20`）RTC `ulPrice`、`eAddParts`、`aulAddPartsPrice`、`eShipMast`。结果累加在 `view+0x34C`，初值 0。`[确认]` 原始指令：

| 步骤 | 条件 | 加价 |
| --- | --- | --- |
| 桅 `i` 0–2 | 新船有桅（位 0）且帆型位 1 与旧船不同 | `+2000`（`0x7D0`） |
| | 新船有桅且附加帆位 2 新有旧无 | `+1000`（`0x3E8`） |
| | 新船有桅且附加帆位 3 新有旧无 | `+1000` |
| 改装件 0–8 | 新有旧无 | `+ aulAddPartsPrice[tier]`，表 `{1000, 800, 1200}` |
| 换炮 | 炮种或武装炮台间数变了 | 见下 |
| 港口系数 | 总价 `> 0` 时 | `fn_14000643d(town, &price)` |

`tier` 是**当前港口商业发展度**三档，不是船的体型：

```
dev  = fn_14000bc03(town, 0)       # town = fn_1400a7820()，当前停靠港；0x62 → NULL
tier = 0
if dev >= 7000: tier = 1           # 0x1B58，有符号 jl
if dev <  4000: tier = 2           # 0xFA0，有符号 jge
# tier0 4000–6999 → 1000；tier1 ≥7000 → 800；tier2 <4000 → 1200
```

换炮差价（`fn_140013593` = `gun.price_unit × 3^n × 20`，`n` = 武装炮台间数，详见 `08`）：

```
a = gun_price(new.gun, new.n_gun_rooms)
b = gun_price(old.gun, old.n_gun_rooms)
if new.gun == old.gun and new.n == old.n: 不变
elif new.gun == old.gun:
    price += (new.n > old.n) ? a - b : (a - b) / 2     # 减间数时退一半（有符号除，趋零）
else:
    price += a - b / 2                                   # 换炮种：旧炮折半抵价
```

港口系数 `fn_14000643d`（`0x140261a60`）按 `CTown+0x74` 字节（`fn_14001244f`）：

| `+0x74` | 系数 |
| --- | --- |
| 0 | `×2`（`shl 1`） |
| 3 | `×12 / 10`（无符号 `div`） |
| 4 | `×8 / 10` |
| 其它 | 不变 |

`CTown+0x74` 开局 12，语义 `[未知]`（`02-factions/02-countries-culture-fashion.md` 列为流行速度候选）。

- `[确认]` 总价 `<= 0` 时跳过港口系数，原样留在 `+0x34C`；函数内**没有**夹到 0。之后调 `fn_140004174(view)`，是否在那里夹取 / 打折 `[未知]`。
- `[确认]` 帮助改造：「如果那个人能够帮忙，改造费可以给你减少%s％。」来源：`0xc337`。折扣公式 `[未知]`。

旧稿两处错误已更正：「`aulAddPartsPrice` 下标是船的体型档」「`fn_140013593` 是货格差价」。

### 天数

| 行动 | 文案 | 来源 |
| --- | --- | --- |
| 修理 | 「到修理完成前，还得等一天。」 | `0x8037` |
| 改造 | 「到改造完成要等２天。」另有「要等%s天。」 | `0xc39f` / `0xc3c3` |
| 买船 | 「准备就需要%s天的时间，买吗？」 | `0x8053` |

「购买与已改造好的船完全一样的船？好了，那就先存放在船坞吧。」「假若和%s是同一规格。就没有必要进行改造了。」来源：`0x7ea3` / `0x80a7`。

### 修理费

「修理要花%s枚金币，可以吗？」「如果那个人肯帮忙，修理费用就少算%s％。」「花%s枚金币就可以完全修理。」没钱：`0x7fcb`。来源：`0x7fcb`–`0x8053`。底价公式 `[未知]`（修理计价函数未追到）。

- `[确认]` 出港检查 `fn_140014105`（`0x140172b26`）：`当前耐久 <= durability_max * 7 / 10`（有符号 `idiv 0xA`）时置修理提示，只提示不拦（`06` §2.2）。

### 船首像

- `[推断]` 强：`CShip+0x53` 就是船首像 id（`01`）。依据：9 个比较值的代码效果与道具文案逐一吻合（下表），且 `CShipObject::vfunc_09`（`0x14023e7f0`）在查不到船时返回 `0x0D` = 无。道具表 kind 5（船首像）共 12 件：114–123 与 199 怪鱼像、200 提督像（`Docs/_data/items.tsv`、`09-items/01`）。`figurehead_id → item` 映射 `[推断]`：0–9 = 114–123，10 = 199，11 = 200；造船厂按道具装像的函数未追到。
- 各值的比较点 `[确认]`（`vfunc+0x48` 返回值与字面量比较），效果名 `[推断]`：

| id | item | 名 | 文案 | 代码效果 | 证据 |
| --- | --- | --- | --- | --- | --- |
| 0 | 114 | 大鹫像 | 船飞速前进 | 舰队任一船装：航速 `+s/10` | `0x1401b088f`（`06-ocean/01` §2） |
| 1 | 115 | 小猪像 | 减少饭量 | 粮食日耗除数 `+2` | `fn_140005aa6`（F-ADJ-03） |
| 2 | 116 | 虎鲸像 | 躲暴风雨 | 未找到 | — |
| 3 | 117 | 白鲸像 | 不易沉 | 最大耐久 `+10` | `fn_1400084db`（`01`） |
| 4 | 118 | 龙像 | 对敌炮击提高 | 射手炮威力 `+2` | `fn_14000c6cb` `0x14023ebb0`（`08`） |
| 5 | 119 | 海豚像 | 看见更多 | 搜索半径 `+16`，全队只算一次 | F-ADJ-01（`13-algorithms/03`） |
| 6 | 120 | 处女像 | 舒适安逸 | `[推断]` 疲劳日增量改随机 `rand(10) < rand(days)` | `fn_140003f99(fleet, 6)`（`06-ocean/01` §2） |
| 7 | 121 | 魔鬼像 | 各种效果；不祥 | 航速 `×4/5`；海战中耐久写到 `<= 0` 时抬回 1 | `0x1401b08bc`；`CBattleShip::vfunc_14`（`0x140126e50`） |
| 8 | 122 | 王者像 | 鼓舞水手 | 近战伤害 `+x/5` | `fn_14000d7ab` `0x140124709`、`fn_14000660e` `0x1401bc38d`（`07-combat`） |
| 9 | 123 | 圣母像 | 抵御瘟疫 | `[推断，强]` 坏血病 / 传染病 happening 整槽失败 | `fn_140003f99(fleet, 9)` body `0x1400f8b80`（`06-ocean/05`） |
| 10 | 199 | 怪鱼像 | 敌人丧失战意 | 开战时（实时入场 + 无画面开场，接舷不触发）对侧每船水手 `-= crew/10` | `07-combat/02` §2.7 |
| 11 | 200 | 提督像 | 效果不能直接察觉 | 海战中耐久 / 水手写到 `<= 0` 时抬回 1 | `CBattleShip::vfunc_14` / `vfunc_16`（`0x140126dd0`） |
| `0x0D` | — | 无 | | | 初值；`vfunc_09` 回退值 |

- `[确认]` 「在海上不能更换船首像。」（`0x3753`）「恶魔的像损坏了！恶魔的像拆不下来……，无论如何都要解除吗？」（`0x364b`）：魔鬼像拆除另有确认 / 损坏分支，条件 `[未知]`。
- `[确认]` 拒绝：「噢，真的要把这船首像安装上去吗？那，那船首像的船我可驾驶不了。」（`0xc767`）。哪几个 id 触发、是否只对特定主角 `[未知]`。UI：`CFigureHeadButton`。另有 RTC `_bTakeAwayFigureHead`（`0x14011d1f0`）。

重构：上表每行的代码效果进 `figureheads.yaml` 的 `effects`，向修正器注册表登记贡献（D2）；航海 / 补给 / 战斗 / 耐久只按 key 取值，代码里不出现 `== 0/1/3/4/5/7/8/10/11`（D3）。

### 火炮

- `[确认]` `CSelectGunTypeView` 选炮种。来源：`game/ship/CSelectGunTypeView.cpp`。
- `[确认]` `fn_140003bb1`（`0x1400a3290`）RTC `eGunType`、`anNecessaryDevelopment`（24 字节 = 6 个 int）：

```
1, 1, 2000, 3500, 5000, 7000
```

对 `eGunType` 0–5：若当前港口发展度 `s70 < 表[gun]` 则该炮种不上架。`[确认]` `s70 = fn_14000bc03(town, 1)`（`0x1400a32fc` `mov edx,1`）= **军事**发展度；改装件价格用的是商业（`0`）。

- `[确认]` 6 个炮种的中文名不在 mesfile，但在 exe `.data 0x140682e10`（19 字节 / 条）：霰弹、曲射、加农曲射、加农、重加农、连射；`+0x11` 威力 1–5，`+0x12` 价格单位 10/50/70/100/200/250。逐条与多语言名见 `08`。`CShip+0x52` 是当前炮种，`7` = 无炮。
- `[确认]` `CBattleGun` 存档两个 u16：`+0x8`、`+0xA`。来源：`CBattleGun.cpp`。
- `[推断]` `NUM_BATTLEGUN=5` 只是 RTC 投票众数（`eBattleGun` 与 5 比 3 次、与 21 / 20 各 1 次）；场景 `CBattleGun` 池是 21 个（`08`）。用途不定，不要写成「每船 5 个炮位」。
- `[确认]` `CShell` 存档：`+0x0C..+0x20` 六个 dword，`+0x24/+0x26/+0x28` 三个 u16。100 个实例挂在 `CMobileShipElement+0x1230`。来源：`CShell.cpp`、`CMobileShipElement.cpp`。这是飞行中的弹，不是「炮弹种类=100」。

### 补给

- `[确认]` 文案把物资分成**水**和**粮食**。来源：`0x33ab`「从俘获的船上得到%s桶水，%s桶粮食」、`0x822f`「改变供水和供食物的比例吗？」、`0x81af`「水好像很少，要平均补给吗？」、`0xc59f`「收藏水和粮食的房间」。
- `[确认]` `CSupplyRateView` 改补给比例。来源：类名 + `0x822f`。
- `[确认]` `anSupply` RTC 8 字节 = 两个 int32。来源：`fn_14000646f`（`0x140173490`）。
- `[确认]` 该函数（原始指令 `0x14017351f..0x1401735ac`）：`target[s] = capacity × rate[s] / 100`（`idiv 0x64`，有符号），`capacity = fn_140013011(...)`，`rate` 两项来自 `fn_140004e17()` 对象的 `fn_140002dd8` / `fn_140003bd9`（百分比）。现量 `anSupply[0..1]` 都 `>=` 目标则直接返回 1。否则 `cost = Σ max(0, target−cur) × unit[s]`，`unit` 来自 `fn_140008936` / `fn_140004c23`。钱不够走按比例补的分支（`0x140173751` 起）。来源：`game/unassigned/fn_14016cc40.cpp` 内 `fn_14000646f` + 反汇编。
- `[推断]` `anSupply[0]` = 水、`[1]` = 粮：与 `CSupplyRateView`「供水和供食物的比例」的语序一致，代码里没有名字。
- `[确认]` 符号 `SUPPLYAMOUNT_MIN/MAX`。来源：`constants.json`。十进制 `[未知]`。
- `[推断]` `NUM_SHIPSUPPLY=2`（水、粮）：RTC `eShipSupply` 与 2 比较 5 次（`num_from_rtc.json`），`anSupply` 8 字节 = 2×int32。头文件无整数。弹药不是补给种类 `[推断]`。
- `[确认]` 大厨 / 饲养员节约水粮（`03` 的岗位 `post_holder`）。老鼠事件吃粮食（航海系统）。
- `[确认]` 外来水粮入船 `fn_1400044f8(fleet, water, food, 1)`（`0x1400fed50`）：第二参是补给下标 0 水 / 1 粮（不是舱室种类）；每船容量 = `supply_room` 间数 × 100（`imul 0x64`），装得下就写，否则填满溢到下一艘。水粮配比 `per` 来源 `[未知]`。来源：`07-combat/06` §2、`06-ocean/02`。

### 其它符号

`ADDPARTS_BITS`、`MASTSTATE_BITS`、`FIGUREHEAD_ID`、`GUNTYPE_ID`、`SHELL_ID`、`BATTLEGUN_ID`。来源：`constants.json` MAXMIN。`ADDPARTS_BITS` 实例是 u16（位 15 另用），`MASTSTATE_BITS` 实例是每桅 1 字节、至少用到位 0–3（见上「两个位域」）。

## 3. 实体与 ID

| 实体 | 原版 | 计数 | 空 | 重构 key |
| --- | --- | --- | --- | --- |
| 改装件 | `eAddParts` | 9 | `INVALID_ADDPARTS=-1` | `add_part` key |
| 甲板改装槽 | `eDeckAddParts` | 4 | — | `deck_add_part_slot` |
| 桅位 | `eShipMast` | 3 `[推断]` | `INVALID_SHIPMAST=-1` | `mast_slot` |
| 桅状态位 | `MASTSTATE_BITS` | ≥4 位 | — | `mast_flags` |
| 船首像 | `eFigureHead` | 12 `[推断]` | `INVALID_FIGUREHEAD=-1`；实例 `0x0D` | `figurehead` key |
| 炮种 | `eGunType` | 6 | `INVALID_GUNTYPE=-1`；实例 `7` | `gun_type` key |
| 战斗炮对象 | `eBattleGun` | 池 21 `[确认]`（`08`） | `INVALID_BATTLEGUN=-1` | 归 `07-combat` |
| 飞行炮弹 | `eShell` | 100 | `INVALID_SHELL=-1` | `shell_instance_id` |
| 补给种类 | `eShipSupply` | 2 `[推断]` | `INVALID_SHIPSUPPLY=-1` | `supply_id` |

船首像与 `eItem` 的关系：`figureheads.yaml` 每行 `item: <item key>` 外键到道具（09-items 持有道具本体），效果只写在 `figureheads.yaml` 一处。主键都是命名空间 key；`0x0D`、`7` 等实例空值只在 `ids/legacy_codec.yaml`，运行时为 `null`（D1）。

## 4. 算法 / 公式

全部进 `config/formulas/ships.yaml` / `config/ships/rig.yaml`。代码里不比改装件位号、桅位号、船首像 id（D3）。

```
# 装甲耐久（按船模体型档，不按耐久）
delta = Σ_{p : add_parts[p].armor} (+1 新装 / −1 拆除) * armor_by_tier[model(new).size_class]
new.durability = clamp(1, new.durability + delta, Ships.durabilityMax(new))
# armor_by_tier [6,8,10] 只在 01 定义；哪些件算装甲由 add_parts.yaml armor: true（原版位 2、3）

# 改装差价（fn_140013d68）
price = 0
for i in 0..mast_slots-1:
    if not new.mast[i].present: continue
    if new.mast[i].rig != old.mast[i].rig: price += mast_rig_fee            # 2000
    for f in [square_addon, fore_addon]:                                     # 桅位 bit2、bit3（rig.yaml mast_bits）
        if new.mast[i][f] and not old.mast[i][f]: price += mast_flag_fee     # 1000
tier = commerce_tier(Towns.develop(town_id, commerce))   # 阈值 [4000, 7000]
for part in add_parts:
    if new.has(part) and not old.has(part): price += add_parts_price[tier]
      # add_parts_price default [1000, 800, 1200]：下标 0 = 中档，1 = 高档，2 = 低档（原版顺序）
price += gun_change_delta(old, new)                       # 公式见 08；同炮减间数退半，换炮旧炮折半
if price > 0: price = town_state_price(town_id, price, remodel)   # 式见 05，表归 economy town_state_price_rules.yaml；原版与 ship_buy 同值（同一 fn_14000643d）：war ×2，boom ×1.2，slump ×0.8
price = apply_helper_percent(price, helper_discount_pct)  # 文案 %s％，公式未知
# 原版本函数不把负价夹到 0

# 炮种解锁（军事发展度）
gun_unlocked(gun_type_id, town_id) =
    Towns.develop(town_id, military) >= guns[gun_type_id].necessary_development
# default [1, 1, 2000, 3500, 5000, 7000]

# 三角帆 / 四角帆互斥（文案规则；原版落点函数未找到）
not (lateen_before_square)     # 「不能张挂在四角帆之前」
not (square_after_lateen)      # 「不能张挂在三角帆之后」

# 帆值与罚值（Ships 派生量，D6；公式 [确认] 见 06-ocean/01 §2，参数在 rig.yaml）
sq_mast(m) = (m.present and m.square ? rig.square_rig_value : 0) + (m.square_addon ? rig.square_addon_value : 0)
fa_mast(m) = (m.present and not m.square ? rig.fore_rig_value : 0) + (m.fore_addon ? rig.fore_addon_value : 0)
penalty    = rig.rig_penalty_by_grade[size_class] + Σ_{p installed} add_parts[p].rig_penalty
sail.square       = max(0, rig.square_main_mul * (sq_mast(main) + Σ add_parts[p].main_square_bonus) - penalty)
sail.fore         = max(0, rig.fore_main_mul   * (fa_mast(main) + Σ add_parts[p].main_fore_bonus)   - penalty)
sail.square_extra = max(0, rig.square_extra_mul * Σ_{m != main} sq_mast(m) - penalty)
sail.fore_extra   = max(0, rig.fore_extra_mul   * Σ_{m != main} fa_mast(m) - penalty)

# 修理
repair_days = formulas.repair_days                        # 1
repair_gold = unknown_base(durability_missing) * (100 - helper_discount_pct) / 100

# 改造
remodel_days = formulas.remodel_days                      # 2；文案另有 %s 天
# 买船：目录船当场交船（0 天）；同规格复制船的天数按差异累加，见 05

# 补给
target[water] = capacity * water_rate_pct / supply_rate_denominator     # 100
target[food]  = capacity * food_rate_pct  / supply_rate_denominator
need[s]       = max(0, target[s] - current[s])
supply_gold   = need[water]*price[water] + need[food]*price[food]
# CSupplyRateView 改的是 water_rate : food_rate

# 外来水粮（战利品 / 探索）：Supplies.add，原版 fn_1400044f8
for ship in fleet.ships:                                   # 舰队顺序
    cap = Ships.cabin_count(ship, supply_room) * supply_per_room   # 100
    写入 min(剩余, cap - 现量)，余量溢到下一艘                       # 不要改成平均分
```

人物帮忙的百分比：UI 传入，本系统不读六维。

## 5. 耦合分析

- 改装件单价读**港口商业发展度**、换炮门槛读**军事发展度**、总价再乘港口景气（`+0x74`）系数：造船厂计价同时依赖城镇三个字段。本系统只收 `Towns.develop(town_id, track)` 与 `Economy.town_state(town_id)` 两个查询；倍率表与买 / 卖船共用 economy 的 `town_state_price_rules.yaml`（改装用 `context: remodel`；原版与 `ship_buy` 同一函数 `fn_14000643d`、同值，单列便于模组单调）。
- 装甲加算按船模体型档（`01` 的 `size_class`），并直接改实例当前耐久。
- 换炮价依赖 `gun_room` 间数（`Ships.cabin_count`，`03`）。
- 原版同一改装位同时驱动帆值、罚值、必要水手、门数、估价、耐久六处，各处写死位号。重构把这些贡献收进 `add_parts.yaml` 一行：模组加一种改装件只加行。
- 追加船舱改 31 槽布局，发 `Cabins.setKind`。
- 炮种解锁读港口发展度（世界 / 势力），本系统只收一个 `town_develop` 标量。
- 原版船首像效果散在航速、补给、搜索、耐久、炮击、接舷、海战抬 1 七处 `== 字面量`。重构由 `figureheads.yaml` 的 `effects` 向修正器注册表登记带来源的贡献，消费方只按 key 取（D2 / D3）。
- 拒绝驾驶某船首像：人物 / 剧本条件，本系统提供 `figurehead` 与 `refusable` 标记。
- `CShell` 池建在移动船元素里（`02` 的对象树），重构后炮弹池归战斗。
- 补给消耗在航海 tick；本系统管港口补给会话。

## 6. 解耦后的配置

```yaml
# config/formulas/ships.yaml  （续 01；armor_by_tier 在 01 定义，此处不重复）
shipyard_days:                        # §4 修理 / 改造工期
  impl: vanilla.shipyard_days
  params:
    repair_days: 1
    remodel_days: 2
remodel_price:                        # §4 改装差价（fn_140013d68）
  impl: vanilla.remodel_price
  params:
    mast_rig_fee: 2000
    mast_flag_fee: 1000                   # 桅 square_addon / fore_addon 各算一次
  tables:
    commerce_tiers:                       # 按商业发展度选 add_parts_price 下标
      - { min: 4000, max: 6999, index: 0 }
      - { min: 7000,            index: 1 }
      - {            max: 3999, index: 2 }
    add_parts_price: [1000, 800, 1200]    # 改装：每件新装同价，与件种无关（fn_140013d68）
supply_target:                        # §4 补给 target[s]
  impl: expr
  params: { supply_rate_denominator: 100 }
  inputs: [capacity, rate_pct]          # rate_pct = water_rate_pct / food_rate_pct
  expr: "capacity * rate_pct / supply_rate_denominator"
supply_add:                           # §4 外来水粮 Supplies.add
  impl: vanilla.supply_add
  params:
    supply_per_room: 100                  # fn_1400044f8 imul 0x64：每间物资仓库容量
repair_warn:                          # 出港：耐久 <= max*7/10 提示修理
  impl: expr
  params: { num: 7, den: 10 }
  inputs: [durability, durability_max]
  expr: "durability <= durability_max * num / den"
# 港口景气倍率归 economy town_state_price_rules.yaml（改装 = context: remodel，原版值同 ship_buy；见 05）
# 炮种门槛、换炮价表归 08 的 guns.yaml / gun_size_mult
```

```yaml
# config/ships/rig.yaml               # Ships 拥有；Ocean 只读 Ships.derived().sail（06-ocean/01 §6 草案落到这里）
schema: dk4.ships.rig
version: 1
mast_slots: 3
main_mast_index: 1
mast_bits: { present: 0, square: 1, square_addon: 2, fore_addon: 3 }   # 原版每桅字节的位，只在 legacy_codec 解包时用
rig_penalty_by_grade: [0, 4, 10]       # .rdata 0x140596d88；按 models.size_class
crew_per_mast_by_grade: [3, 5, 10]     # .rdata 0x140596d98；也是必要水手基数、划桨除数（06、06-ocean）
square_main_mul: 8
fore_main_mul: 8
square_extra_mul: 5
fore_extra_mul: 6
square_rig_value: 7                    # 有桅且横帆
fore_rig_value: 8                      # 有桅且纵帆
square_addon_value: 3
fore_addon_value: 4
mast_addon_crew: { square_addon: 2, fore_addon: 1 }
mast_addon_outfit_fee: 100             # 估价每个加帆件 +100（05 outfit_value）
```

```yaml
# config/ships/add_parts.yaml          # 一件一行：所有系统读到的贡献都在这里（D3）
schema: dk4.ships.add_parts
version: 1
add_parts:
  - { key: vanilla:headsail, vanilla_index: 0, name: 船首帆,     main_square_bonus: 2, crew_need: 1, outfit_fee: 500 }    # 名 [推断]
  - { key: vanilla:spanker, vanilla_index: 1, name: 船尾帆,     main_fore_bonus: 3,   crew_need: 4, outfit_fee: 400 }    # 名 [推断]
  - { key: vanilla:armor_1, vanilla_index: 2, name: 装甲板,     armor: true, rig_penalty: 1, outfit_fee: 1000 }
  - { key: vanilla:armor_2, vanilla_index: 3, name: 二层装甲板, armor: true, rig_penalty: 1, outfit_fee: 1000 }
  - { key: vanilla:extra_cabin_1, vanilla_index: 4, name: 追加船舱,  rig_penalty: 2, outfit_fee: 1000 }                     # 名 [推断]
  - { key: vanilla:extra_cabin_2, vanilla_index: 5, name: 追加船舱,  rig_penalty: 2, outfit_fee: 1000 }                     # 名 [推断]
  - { key: vanilla:bow_gun, vanilla_index: 6, name: 船首炮,     gun_arc: bow,   door_bonus: 6, crew_need: 2, outfit_fee: 400 }
  - { key: vanilla:stern_gun, vanilla_index: 7, name: 船尾炮,     gun_arc: stern, door_bonus: 6, crew_need: 2, outfit_fee: 400 }
  - { key: vanilla:add_part_8, vanilla_index: 8, name: null,       immune_happenings: [vanilla:happening_5], outfit_fee: 400 }   # [未知] 撞角候选；happening key 以 07-combat/05 为准
  # 原版实例存 u16 位图（位 = vanilla_index，位 15 另用）；运行时存 key 集合。
```

```yaml
# config/ships/rig.yaml（续）          # 文案层的挂帆规则；旧稿单列 ships/sails.yaml，现并入 rig.yaml
sail_rules:
  lateen_not_before_square: true
  square_not_after_lateen: true
  bow_sail_min_size: 1                 # 「中型以上」= size_class >= 1 [推断]
  stern_sail_min_size: 2               # 「大型」 [推断]
sail_names:                            # 文案 ↔ 数据位 [推断]
  bow_sail: { add_part: vanilla:headsail }
  stern_sail: { add_part: vanilla:spanker }
  square: { mast_bit: square, value: true }
  lateen: { mast_bit: square, value: false }
  top_sail: { mast_bit: square_addon }
  staysail: { mast_bit: fore_addon }
```

```yaml
# config/ships/figureheads.yaml        # 效果 = 向修正器注册表登记带来源的贡献（D2）；stage: base→add→mul→clamp→override
schema: dk4.ships.figureheads
version: 1
figureheads:
  - key: vanilla:eagle
    vanilla_index: 0
    item: vanilla:eagle_figurehead       # 道具 114
    effects:
      - { key: fleet.speed, stage: mul, order: 10, value: { num: 11, den: 10 }, scope: fleet_any, stack: once }   # 原版 s += s/10
  - key: vanilla:piglets
    vanilla_index: 1
    item: vanilla:piglet_figurehead      # 115
    effects:
      - { key: supply.food_div, stage: add, value: 2, scope: fleet_any, stack: once }
  - key: vanilla:orca
    vanilla_index: 2
    item: vanilla:orca_figurehead        # 116
    effects: []                          # 「躲暴风雨」代码未找到
  - key: vanilla:white_whale
    vanilla_index: 3
    item: vanilla:white_whale_figurehead # 117
    effects:
      - { key: ship.durability_max, stage: add, value: 10, scope: ship }
  - key: vanilla:dragon
    vanilla_index: 4
    item: vanilla:dragon_figurehead      # 118
    effects:
      - { key: combat.gun_power, stage: add, value: 2, scope: ship }
  - key: vanilla:dolphin
    vanilla_index: 5
    item: vanilla:dolphin_figurehead     # 119
    effects:
      - { key: fleet.search_radius_add, stage: add, value: 16, scope: fleet_any, stack: once }
  - key: vanilla:maiden
    vanilla_index: 6
    item: vanilla:maiden_figurehead      # 120
    effects:
      - { key: fleet.fatigue_days_randomized, stage: override, value: true, scope: fleet_any, stack: once }   # [推断]；06-ocean/01
  - key: vanilla:devil
    vanilla_index: 7
    item: vanilla:devil_figurehead       # 121
    refusable: true                      # 拒驾名单未还原，先可配
    removal_confirm: true                # 0x364b「恶魔的像拆不下来」
    effects:
      - { key: fleet.speed, stage: mul, order: 20, value: { num: 4, den: 5 }, scope: fleet_any, stack: once }   # 先乘后除
      - { key: combat.hull_floor_at_zero, stage: override, value: 1, scope: ship }   # 写到 <=0 抬回 1
  - key: vanilla:king
    vanilla_index: 8
    item: vanilla:king_figurehead        # 122
    effects:
      - { key: combat.melee_damage, stage: mul, value: { num: 6, den: 5 }, scope: fleet }   # 原版 x += x/5
  - key: vanilla:madonna
    vanilla_index: 9
    item: vanilla:madonna_figurehead     # 123
    effects:
      - { key: fleet.happening_block, stage: add, value: [vanilla:scurvy, vanilla:plague], scope: fleet_any, stack: once }   # fn_140003f99(fleet, 9) [推断，强]；06-ocean/05
  - key: vanilla:monster_fish
    vanilla_index: 10
    item: vanilla:monster_fish_figurehead   # 199
    effects:
      - { key: combat.engage_fear_crew_div, stage: override, value: 10 }   # 07-combat 写法（词表 §13.4）；开战时触发——实时入场 + 无画面开场，非接舷（07-combat/02 §2.7，词表 §14.27）：对侧每船 crew -= crew/10
  - key: vanilla:admiral
    vanilla_index: 11
    item: vanilla:admiral_figurehead     # 200
    effects:
      - { key: combat.hull_floor_at_zero, stage: override, value: 1, scope: ship }
      - { key: combat.crew_floor_at_zero, stage: override, value: 1, scope: ship }
  # 原版顺序：航速先 add 再 mul，都在人手修正之后、战斗 /3 之前（06-ocean/01）。stage + order 复刻原版取整。
  # 道具 key 与 09-items/01 目录一致（114–123、199、200）。
```

```yaml
# config/ships/guns.yaml —— 由 08-gun-shell-catalog.md 拥有（名称、威力、价格单位、门槛已从 exe 抽出）。
# 本系统只读 guns[id].necessary_development 与 price_unit。
```

```yaml
# config/ships/shells.yaml
schema: dk4.ships.shells
version: 1
max_in_flight: 100
# 飞行实例，不是 100 种弹药
```

```yaml
# config/ships/supplies.yaml
schema: dk4.ships.supplies
version: 1
supplies:
  - key: vanilla:water
    vanilla_index: 0                 # anSupply[0] [推断]
    name: 水
    unit: 桶
  - key: vanilla:food
    vanilla_index: 1
    name: 粮食
    unit: 桶
```

## 7. 独立旋钮

- 改装件行数及每行贡献（帆值、罚值、必要水手、门数、估价、装甲、免疫）
- `rig.yaml` 的按档表与帆值乘数
- 商业发展度分档阈值；港口景气倍率（economy `town_state_price_rules.yaml`，ships 只选 context）
- 桅种类互斥规则、桅位数
- 炮种解锁发展度（`08`）
- 修理 / 改造天数
- 补给种类（原版 2；可加弹药，航海来消耗）
- 船首像行数与 `effects`（战斗 / 航海 / 补给只读 modifier key；加新像不改代码）
- 飞行炮弹池大小

改炮种名字只动 `guns.yaml` + 文案包。改「7000 发展度才能买 5 号炮」只动 `necessary_development`。

## 8. 对外接口

事件：

- `RemodelQuoted { ship_instance_id, gold, days }`
- `RemodelCommitted { ship_instance_id, gold, days }`
- `Repaired { ship_instance_id, gold, days }`
- `GunTypeChanged { ship_instance_id, gun_type }`
- `AddPartChanged { ship_instance_id }` / `MastChanged { ship_instance_id }`  → Ships 重算派生量（D6），`06` 订阅做超编分流
- `FigureheadChanged { ship_instance_id, figurehead? }`  → 注销旧像贡献、登记新像贡献
- `FigureheadRefused { ship_instance_id, figurehead, person_id }`
- `Supplied { fleet_id, water, food, gold }`

查询：

- `Yard.quoteRemodel(instance_id, preview) -> { gold, days }`
- `Yard.quoteRepair(instance_id) -> { gold, days }`
- `Yard.gunTypesAvailable(town_id) -> [gun_type_id]`
- 船首像不提供专用查询：效果经 `Modifiers.eval(key, ctx)` 取（来源可追溯到 `figurehead:<key>@ship`）
- `Supplies.of(instance_id) -> { water, food }`

命令：

- `Yard.commitRemodel(instance_id, preview, helper_person_id?)`
- `Yard.repair(instance_id, helper_person_id?)`
- `Yard.setGunType(instance_id, gun_type_id)`
- `Yard.setFigurehead(instance_id, figurehead_id)`
- `Yard.supply(fleet_id, water, food)`
- `Yard.setSupplyRate(fleet_id, water_rate, food_rate)`
- `Supplies.add(fleet_id, water, food)` / `Supplies.consume(fleet_id, water, food)`  # ships 拥有；add = 战利品 / 探索入船（ocean、combat 调），consume = 航海日耗（ocean 调）

付钱走 `Force.add_gold(force_id, -price, reason)`（factions），本系统不算市价。发展度走 `Towns.develop(town_id, track)`（`commerce` / `military`），港口景气状态走 `Economy.town_state(town_id)`（economy）。

## 9. 未核实字段

- `NUM_FIGUREHEAD=12`、`NUM_SHIPSUPPLY=2`、`NUM_SHIPMAST=3`、`NUM_DECKADDPARTS=4` 只有投票 / 比较 / 计数旁证
- 改装位 0、1、4、5 的名字（作用已确认）；位 8 是什么件
- 桅位 1–3 的帆名，以及互斥规则落在哪个函数
- 船首像 2 虎鲸的代码效果（6 处女 / 9 圣母已由 06-ocean 找到调用点）；`figurehead_id ↔ item` 的映射函数
- 改造 / 修理的人物折扣公式；`fn_140004174` 是否夹取负价
- `CTown+0x74` 的名字（系数表键 0 / 3 / 4）
- 买船准备天数按什么表
- 修理底价
- `CBattleGun+0x8/+0xA` 含义（`08`）
- `CShell` 各 dword（`08`）
- 拒驾船首像的条件（主角？特定像？）
- `SUPPLYAMOUNT_MIN/MAX` 十进制
- `capacity`（`fn_140013011`）与两项单价函数的展开
- `_bTakeAwayFigureHead` 的玩法含义
