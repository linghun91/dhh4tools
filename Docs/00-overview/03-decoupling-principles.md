# 解耦原则

## 问题

原版是一份 C++ 程序 + 两份世界 DLL + 一堆 `.dk4`。玩法规则、数值上限、内容实例、UI 文案混在同一套编译单元里。改「伦敦的特产品」要碰二进制表；改「经验除数 30」要搜立即数。

重构把游戏切成四层：

```
内容实例  →  表（港口、人物、商品）
规则参数  →  公式与上限（caps / formulas）
系统逻辑  →  解释器（贸易、航海、战斗）
表现     →  场景、文案、动画
```

层与层只通过 **稳定 ID** 和 **窄接口** 说话。

## 原则 1 — 一个系统一份目录

每个玩法系统拥有：

- `Docs/<n>-<system>/` 设计文档
- `config/<system>/` 运行时表
- 一组事件名（`TownEntered`, `CargoCommitted`, `BattleResolved`）

不允许「贸易文档里夹一段航海伤害公式」。需要对方数据时，写**查询接口**，例如 `Economy.quote(town_id, item_id) -> PriceSnapshot`。

## 原则 2 — ID 注册表是唯一共享层

`config/ids/` 由内容表生成，不手写两份。运行时校验：

- 任何外键必须指向已注册 ID，或 `INVALID_*` / 空哨兵
- 空货格用 `trading_item.empty`（原版 `0x6A=106`），不用 -1 和 0 混用
- 内容包可以比原版更长；代码按表长循环，不按 `NUM_TOWN`

## 原则 3 — 公式全是纯函数

所有战斗、价格、经验、技能、航海速度写成：

```
skill_value = skill_formula(stat_a, stat_b, level, params)
```

`params` 来自 `config/formulas/`。系统代码不得出现除法常数 30、150、1000。原版魔数作为该公式的 **default 内容包** 写入 YAML。

## 原则 4 — 状态与定义分离

| 定义（只读内容） | 状态（存档） |
| --- | --- |
| 港口坐标、默认特产品 | 发展度、份额、合约、流行 |
| 人物初始六维、姓名 | 经验、装备、隶属、疲劳 |
| 船型上限 | 当前耐久、帆、炮、货 |
| 交易品基础价、种类 | 市场价、供给、品牌 |

定义进内容包；状态进存档 schema。禁止把定义拷进存档再改（原版部分表会在运行时被改写，重构时要标出哪些字段是可变的）。

## 原则 5 — 事件，不直调

例：入港

1. Ocean 发出 `ArrivedAtTown { fleet_id, town_id }`
2. Town 打开设施图
3. Economy 按 `town_id` 刷新当日市价
4. Narrative 检查该港触发器

Ocean 不知道交易所怎么算价。Economy 不知道船在哪一格。

## 原则 6 — 能力修正器统一出口

原版把「魅力/智力」直接塞进买货、战斗、道具价。重构后人物系统只暴露：

```
ModifierSet = {
  buy_cost_factor,
  sell_price_factor,
  battle_offense,
  item_price_factor,
  ...
}
```

各系统读修正器，不读六维原值。这样改六维含义不必改贸易代码。

## 原则 7 — 两种剧本 = 两份内容包

World0.dll 与 World1.dll 同一 schema。`narrative/scenarios.yaml` 指定每个剧本加载哪份世界包、哪份事件脚本（EV0–EV6）。禁止 `if scenario==3` 散落在系统里。

## 原则 8 — 上限可配且有硬顶说明

原版：单段等级 100（工具可到 255，再高 `isqrt` 溢出）。重构：

- `caps.yaml` 给设计上限
- `engine_limits.yaml` 给实现硬顶（整数宽度）
- 公式实现必须对溢出有定义（饱和，而不是绕回 0）

## 原则 9 — 原版兼容是内容包，不是引擎特性

「5 艘船、5 货格、商品不堆叠、技能封 200」全部是原版内容包的默认值。引擎允许更大。显示层自己处理「价格列放得下几位数字」，不要用 UI 宽度倒逼规则。

## 系统依赖图（允许的方向）

```
ids
  ↑
world ← climate, routes
  ↑
factions ← world.town_id
characters ← factions.force_id
ships / fleets ← characters.person_id
economy ← world.town_id, ships.hold
ocean ← world, fleets, climate
combat ← fleets, ships, characters.modifiers
town-life ← world, characters, economy, ships
items ← characters, ships
narrative ← 全部（只读查询 + 发命令）
progression ← characters
save ← 所有状态系统
ui ← 所有（只读投影）
```

禁止反向：economy 不得改人物经验；combat 不得写市价；ui 不得写存档字段。
