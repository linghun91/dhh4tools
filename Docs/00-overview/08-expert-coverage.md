# 资深清单覆盖矩阵

对照「大路通天」11-06 提出的设计轴。结论：**大类目录有，细设定没有补全。** 本轮按缺口补文档，不另起一套分类。

标记：

- **有** — 已有专篇且字段可配
- **薄** — 有专篇或散落，缺图、缺表、缺公式、缺出现条件
- **无** — 清单里的轴在 Docs 里还没有独立规格

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
| 死亡 / 寿终 / 继承 | `11-progression/03-gameover-succession.md` |
| World0 vs World1 全差异 | `10-narrative/04-world0-world1-diff.md` |
| 12 种地图点 | `01-world/10-mappoint-kinds.md` |
| 炮种 / 炮弹名录 | `05-ships-fleet/08-gun-shell-catalog.md` |
| 旅馆 / 教会 / 宫殿内部规则 | `08-town-life/05-inn-church-palace.md` |
| NPC 舰队大地图 AI | `06-ocean/06-npc-fleet-ai.md` |
| 战利品 / 俘船 / 俘虏 | `07-combat/06-prizes-capture.md` |
| 广场情报 / 广告人 | `08-town-life/06-plaza-intel-adman.md` |
| 疲劳 / 伤病 / 精神 | `03-characters/07-fatigue-health-mood.md` |
| 付费查行情 | `04-economy/08-price-intel.md` |
| 通关画廊点数 | `10-narrative/05-gallery-unlock.md` |

## 大佬补刀（11-06 16:55 后）

| 轴 | 处理 |
| --- | --- |
| HD 古宝系统 | `09-items/04-ancient-treasures-hd.md` |
| 遗迹 + 解谜（最值得搞） | `01-world/11-remains-puzzle-system.md` |
| 7×uint 与母港地理带 = **文化圈** | 已改正 `02-factions/01-forces.md`；专篇 `02-factions/06-seven-culture-circles.md`。七格 = 霸者之证七区（北海…新大陆），不是 9 个 NPC 形象圈，不是 21 个港口 region。 |
