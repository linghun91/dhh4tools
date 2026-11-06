# 资深清单覆盖矩阵

对照「大路通天」11-06 提出的设计轴。结论：**大类目录有，细设定没有补全。** 本轮按缺口补文档，不另起一套分类。

标记：

- **有** — 已有专篇且字段可配
- **薄** — 有专篇或散落，缺图、缺表、缺公式、缺出现条件
- **无** — 清单里的轴在 Docs 里还没有独立规格

「状态」列记的是补文档**之前**的状态；「本轮补」列的文档均已存在（逐个核对过路径）。

| 清单轴 | 细项 | 状态 | 已有文档 | 本轮补 |
| --- | --- | --- | --- | --- |
| 地图 | 航海地图 | 薄 | `01-world/01-map-and-sea-areas.md`（`World.dat` 2500²） | `01-world/06-three-maps.md` |
| 地图 | 航线地图 | 薄 | `01-world/05-routes-navigation.md`（`CRouteMap`） | 同上 |
| 地图 | 黑块地图 | 薄 | 只出现 `eBlindArea` / `fogcolor` | 同上 |
| 交易 | 商品含图 | 无 | 无 `TItem24/48` `Item32/56` 映射 | `04-economy/05-goods-art-spawn-display.md` |
| 交易 | 出现条件 | 薄 | 特产品 5 槽有，市场何时上架未钉 | 同上 |
| 交易 | 价格 | 有 | `04-economy/02-market-price.md` | 加深出现条件侧 |
| 交易 | 等级 | 薄 | 舱位左列「%s等级」/★ | 05 篇显示规则 |
| 交易 | 星品 | 薄 | 品牌=「１级」；★ 是显示态 | 05 篇把星品当显示+等级 |
| 交易 | 显示 | 无 | `trade_stack.md` 只谈列宽 | 05 篇 |
| 交易 | 主舰队交易 | 有 | `04-economy/03-trade-loop.md` | — |
| 交易 | NPC 舰队交易 | 薄 | 只有买货折扣 `1000-a` | `04-economy/06-npc-fleet-trade.md` |
| 舰船 | 船型 / 船种 | 薄 | `NUM_SHIP=100` `NUM_SHIPTYPE=5`，中文名未抽 | `05-ships-fleet/05-hull-catalog-market-start.md` |
| 舰船 | 船模 | 无 | `ship.dk4` `DeckShip` `StlShip` 未映射 | 同上 |
| 舰船 | 赠与和买卖 | 薄 | 造船厂入口有，赠与/剧本送船无表 | 同上 |
| 舰船 | 初始船 | 无 | `NUM_PLAYEREVENTSHIP` 仅符号 | 同上 |
| 舰船 | 船舱 / 帆 / 炮台 / 装甲 / 首像 | 有 | `03-cabins` `04-remodel-guns-supplies` | 05 篇补名录与买卖 |
| 人物 | 六维 / 等级 | 有 | `03-characters` `11-progression` | — |
| 人物 | 图片 / 动画 / 显示 | 无 | `Chara00–41` `bustup` `Portrait` 未映射 | `03-characters/05-portraits-animation.md` |
| 攻防 | 舰对舰 / 炮战 / 白刃 | 有 | `07-combat/01` `02` | — |
| 攻防 | 舰对港 / 港口占领 | 薄 | 文案和 `town_occupied` 事件 | `07-combat/04-ship-vs-port.md` |
| 计谋 | 话术 / 计谋数 / 模型 | 薄 | `NUM_SCHEME=24`，kind 名未抽 | `08-town-life/04-schemes-rhetoric.md` |
| 调节因子 | 等级 | 有 | 经验公式 | `13-algorithms/03-adjustment-factors.md` 收口 |
| 调节因子 | 视野 | 薄 | 搜索圈 64 顶点，半径公式无 | 同上 |
| 调节因子 | 补给 | 薄 | 按日历步消耗，量未还原 | 同上 |
| 剧情 | 条件 | 薄 | 9 种 `eTriggerKind` 计数已知、名未知 | `10-narrative/03-event-commands.md` |
| 剧情 | 指令 / 指令解析 | 无 | 明确写成不透明 blob | 同上（opcode 尽量从 EV+解释器抽出） |
| 日期经纬 | 时间坐标 / 日期算法 | 无 | `SCalendar` 只被叙事当快照 | `01-world/07-calendar-and-coords.md` |
| 日期经纬 | 坐标体系 / 经纬度计算 | 无 | 伦敦 (20080,4272)；UI `nLatitude/nLongitude` `%03d` | 同上 |
| 航行 | 风速 / 洋流 / 帆 | 有 | `06-ocean/01-sailing.md` | — |
| 航行 | 水手舱 | 薄 | 岗位表有水兵室，未挂钩航速 | `06-ocean/04-manual-auto-sail.md` |
| 航行 | 航线 / 自动 / 手动 | 薄 | 自动走 `fn_140008814`，模式未独立成篇 | 同上 |
| 航行 | 升级 | 薄 | 驾驶术/测量术影响未量化 | 同上 + 调节因子 |
| 海域文化圈 | 音乐 | 无 | `bgm.ktsl2*` 未按海域编目 | `01-world/08-sea-area-music-icons.md` |
| 海域文化圈 | 图示 | 薄 | `CSeaArea*View` 类在，无图标表 | 同上 |
| 海域文化圈 | 流行模型 | 有 | `02-factions/02-countries-culture-fashion.md` | 08 篇只引用 |
| 海域文化圈 | 商品价格定义 | 薄 | 市价按种类指数，是否按海域/文化分表未钉 | 08 篇 + 经济 05 |

不在这份清单、Docs 里已经单列的：黄金航路、委托、小游戏、存档、四语言。保持原目录，不合并进大佬轴。

## 第一轮核查后的更正

上表「状态」「已有文档」两列记的是补文档之前的认识，其中几条已被源码推翻。以本节为准。

| 轴 | 旧认识 | 核查结论 | 文档 |
| --- | --- | --- | --- |
| 舰船 船型 / 船种 | `NUM_SHIP=100`、`NUM_SHIPTYPE=5`、名未抽 | 船体定义 107 条（实例槽 100–206，另有 0–99 动态槽）、船模 39 条，名字都在 exe；`NUM_SHIPTYPE=5` 是 RTC 投票误采 | `05-ships-fleet/01`、`05` |
| 舰船 炮台 | 每舰队 5 炮位 | `NUM_BATTLEGUN=5` 误采；海战炮池 21（gun i ↔ 船 i，20 = 城炮）；`NUM_SHIPMAST` 实为 3 | `05-ships-fleet/04`、`08`、`07-combat/01` |
| 人物 六维 / 等级 | 「属性结算 500」 | 500 是**技能值**上限；六维上限 100；exp 30–59 为 0 级 | `03-characters/03`、`11-progression/01` |
| 人物 身心 | 身体 6 态、精神 4 态 | 身体 7 态（含疲劳）、精神 6 态 | `03-characters/07` |
| 名声 / 爵位 | 有专篇 | 原版**不存在**名声与爵位；`fame.yaml` 默认关闭 | `11-progression/02` |
| 港口设施 | 教会是设施 | 12 种地图点；遗址 / 教会 / 城门共用 `CMpRemains`，是按日仪式 | `01-world/10`、`08-town-life/01`、`05` |
| 剧情 条件 | 9 种 kind 名未知 | kind 0–8 全部 `[确认]`；终局、小游戏 opcode 已钉 | `10-narrative/03` |
| 日期经纬 坐标体系 | 换算未还原、禁止除 8 | 航行坐标 `/16` → chip；`World.dat` 2500×1250 uint16 | `01-world/01`、`06`、`07` |
| 调节因子 补给 / 视野 | 消耗量、半径公式无 | 补给日耗、探索全流程、遭遇分带 `[确认]` | `06-ocean/01`、`02`、`13-algorithms/03` |
| 航行 风速 / 帆 | 三处除数未知 | `+s/10`、`×4/5`、`/3` 全钉 | `06-ocean/01` |
| 黄金航路 | 30 节点权重 | 那是完成动画时间轴，已撤回；月初奖金 `fn_1400108fc` 已钉，节点表与点亮条件仍未知 | `06-ocean/03`、`05-config-architecture.md` |
| 海域文化圈 商品价格 | 是否按文化分表未钉 | 21 文化圈 × 105 卖价底表 `[确认]`（SC `.rdata 0x1405b6160`） | `02-factions/06`、`04-economy/02` |
| 7×uint / 文化圈 | 7 格当文化圈 | 七大海域 + 21 文化圈（见下节，已纠正） | `02-factions/06` |
| 海域 | 25 个航海分区（`NUM_SEAAREA=25`） | 25 是投票误采；`eSeaArea` 就是七大海域，原版无细分区，`sea_areas` 只作模组扩展 | `01-world/01`、`04-id-registry.md` |

## 冰山下（自行补的轴）

大佬清单是分类提示，不是穷举。下面这些原版有系统、上一轮 Docs 只有一句或没有专篇，本轮一并抽：

| 轴 | 本轮文档 |
| --- | --- |
| 介绍信 / 推荐信 | `02-factions/04-letters-licenses.md` |
| 同伴入队 / 打听 / 羁绊 | `03-characters/06-companions-bonds.md` |
| 无名水手人数 vs 具名航海士 | `05-ships-fleet/06-crew-headcount.md` |
| 海上 happening（风暴/鼠/海豚/鲨） | `06-ocean/05-sea-happenings.md` |
| 势力战争 / 失败 / 吞并 | `02-factions/05-war-failure-absorb.md` |
| 遗迹 + 古地图对照表 | `01-world/09-remains-oldmaps-catalog.md` |
| 投资 / 上缴公式 | `04-economy/07-investment-tribute.md` |
| 船坞额外船体 | `05-ships-fleet/07-dock-warehouse.md` |
| 海战阵型 / 命令 / happening | `07-combat/05-formation-happening.md` |
| 种子种植 → 特产品 | `09-items/03-discovery-planting.md` |
| 死亡 / 寿终 / Game Over（原版**无继承**，继承只作默认关闭的挂钩） | `11-progression/03-gameover-succession.md` |
| World0 vs World1 全差异 | `10-narrative/04-world0-world1-diff.md` |
| 12 种地图点 | `01-world/10-mappoint-kinds.md` |
| 炮种 / 炮弹名录 | `05-ships-fleet/08-gun-shell-catalog.md` |
| 旅馆 / 王宫规则；教会是地图点 kind 10 的按日仪式，**不是设施** | `08-town-life/05-inn-church-palace.md` |
| NPC 舰队大地图 AI | `06-ocean/06-npc-fleet-ai.md` |
| 战利品 / 俘船 / 俘虏 | `07-combat/06-prizes-capture.md` |
| 广场情报 / 广告人 | `08-town-life/06-plaza-intel-adman.md` |
| 疲劳 / 伤病 / 精神 | `03-characters/07-fatigue-health-mood.md` |
| 付费查行情 | `04-economy/08-price-intel.md` |
| 通关画廊点数 | `10-narrative/05-gallery-unlock.md` |
| 原版世界 tick 顺序 / 日历门 | `02-original-architecture.md`、`05-config-architecture.md`「tick 顺序」 |
| 随机数（CRT `rand`，`time()` 播种，不进存档） | `05-config-architecture.md`「随机数」 |
| 模组：命名空间、覆盖合并、存档 id 重映射、迁移 | `05-config-architecture.md` |
| 修正器堆叠顺序（技能 + 装备 → 身心系数 → 封 500） | `03-decoupling-principles.md` 原则 6 |
| 跨系统架构决策 D1–D10（ID、修正器、公式层、时间、剧本旗标、派生量、存档、叙事、动作执行器、内容提取） | `09-architecture-decisions.md` |
| 事件 / 查询 / 命令规范名与所有权裁决 | `06-system-interfaces.md` |
| 全局 key 词表（修正器、技能 / 六维、岗位、舱室 role、caps、配置路径、海域、特产品槽字段、tick 阶段）与别名对照 | `10-key-glossary.md` |

## 大佬补刀（11-06 16:55 后）

| 轴 | 处理 |
| --- | --- |
| HD 古宝系统 | `09-items/04-ancient-treasures-hd.md` |
| 遗迹 + 解谜（最值得搞） | `01-world/11-remains-puzzle-system.md` |
| 7×uint / 文化圈 | **已纠正**：七大海域 + 21 文化圈。见 `02-factions/06-seven-culture-circles.md`。7 格 RTC 名是 `eSeaArea` 上界 7；21 是 `CCulture` 槽 `0x15`。不要把 7 格再写成文化圈。 |
