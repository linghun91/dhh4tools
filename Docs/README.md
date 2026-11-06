# DK4 重构设计文档

把《大航海时代4 PK HD》的玩法、算法、数值拆成可独立配置的系统规格。

原作数据在 `srcdata/` 与 `Game/`。本目录是设计层，不修改 exe。

## 先读

1. [_meta/writing-guide.md](_meta/writing-guide.md) — 章节模板与解耦铁律
2. [00-overview/01-product-vision.md](00-overview/01-product-vision.md) — 重构目标
3. [00-overview/03-decoupling-principles.md](00-overview/03-decoupling-principles.md) — 系统怎么拆
4. [00-overview/05-config-architecture.md](00-overview/05-config-architecture.md) — `config/` 布局
5. [00-overview/04-id-registry.md](00-overview/04-id-registry.md) — 全部 ID 与计数
6. [00-overview/06-system-interfaces.md](00-overview/06-system-interfaces.md) — 查询 / 命令 / 事件
7. [00-overview/07-open-unknowns.md](00-overview/07-open-unknowns.md) — 实现前必须钉死的未知项、原版哨兵对照
8. [00-overview/09-architecture-decisions.md](00-overview/09-architecture-decisions.md) — 跨系统架构决策 D1–D10（与之冲突时改系统文档）
9. [00-overview/10-key-glossary.md](00-overview/10-key-glossary.md) — 全局 key 规范名（写 YAML key、修正器、tick 阶段前先查）

改表从 [15-catalogs](15-catalogs/README.md) 对照原版实例；改公式从 [13-algorithms/01-formulas.md](13-algorithms/01-formulas.md)。

## 目录

### 00 总览

| 文档 | 内容 |
| --- | --- |
| [01-product-vision.md](00-overview/01-product-vision.md) | 要保住的体验、成功标准 |
| [02-original-architecture.md](00-overview/02-original-architecture.md) | exe / DLL / 场景 / 类 / 原版世界 tick |
| [03-decoupling-principles.md](00-overview/03-decoupling-principles.md) | 四层、事件语义、修正器堆叠、依赖图 |
| [04-id-registry.md](00-overview/04-id-registry.md) | 枚举、哨兵、上限 |
| [05-config-architecture.md](00-overview/05-config-architecture.md) | YAML 目录、加载顺序、模组合并、ID 命名空间、tick 顺序、随机数 |
| [06-system-interfaces.md](00-overview/06-system-interfaces.md) | 跨系统查询、命令、事件规范名；状态 / 派生量所有权裁决 |
| [07-open-unknowns.md](00-overview/07-open-unknowns.md) | 阻塞实现的未知项 |
| [08-expert-coverage.md](00-overview/08-expert-coverage.md) | 对照资深清单：哪些有、哪些薄、本轮补哪些、核查后更正 |
| [09-architecture-decisions.md](00-overview/09-architecture-decisions.md) | D1 ID / D2 修正器 / D3 公式层 / D4 时间 / D5 剧本旗标 / D6 派生量 / D7 存档 / D8 叙事 / D9 动作执行器 / D10 内容提取 |
| [10-key-glossary.md](00-overview/10-key-glossary.md) | 全局 key 词表：修正器、六维 / 技能、岗位、舱室 / role、caps、配置路径、海域、特产品槽字段、tick 阶段的唯一规范名，附「别名 → 规范名」文件:行对照 |

### 01 世界

| 文档 | 配置落点 |
| --- | --- |
| [01-map-and-sea-areas.md](01-world/01-map-and-sea-areas.md) | `config/world/map.yaml` `major_seas.yaml` `sea_areas.yaml`（原版空） |
| [02-towns.md](01-world/02-towns.md) | `config/world/towns.yaml` |
| [03-villages-discovery.md](01-world/03-villages-discovery.md) | `villages.yaml` `remains.yaml` |
| [04-climate-weather-zodiac.md](01-world/04-climate-weather-zodiac.md) | `climate.yaml` |
| [05-routes-navigation.md](01-world/05-routes-navigation.md) | `routes.yaml` `junctions.yaml` |
| [06-three-maps.md](01-world/06-three-maps.md) | 航海图 / 航线图 / 黑块图 |
| [07-calendar-and-coords.md](01-world/07-calendar-and-coords.md) | 日期算法、经纬度 |
| [08-sea-area-music-icons.md](01-world/08-sea-area-music-icons.md) | 海域 BGM / 图示 |
| [09-remains-oldmaps-catalog.md](01-world/09-remains-oldmaps-catalog.md) | 25 遗迹名、24 村落 |
| [10-mappoint-kinds.md](01-world/10-mappoint-kinds.md) | 港口 12 种热点 |
| [11-remains-puzzle-system.md](01-world/11-remains-puzzle-system.md) | **遗迹 + 解谜（五层）** |

### 02 势力

| 文档 | 配置落点 |
| --- | --- |
| [01-forces.md](02-factions/01-forces.md) | `config/factions/forces.yaml` |
| [02-countries-culture-fashion.md](02-factions/02-countries-culture-fashion.md) | `countries.yaml` `cultures.yaml` `fashion.yaml` |
| [03-relations-influence.md](02-factions/03-relations-influence.md) | `relations.yaml` |
| [04-letters-licenses.md](02-factions/04-letters-licenses.md) | 介绍信 / 推荐信 |
| [05-war-failure-absorb.md](02-factions/05-war-failure-absorb.md) | 开战 / 灭亡 |
| [06-seven-culture-circles.md](02-factions/06-seven-culture-circles.md) | **七大海域 + 21 文化圈**（更正） |

### 03 人物

| 文档 | 配置落点 |
| --- | --- |
| [01-person-model.md](03-characters/01-person-model.md) | `persons.yaml` |
| [02-protagonists-scenarios.md](03-characters/02-protagonists-scenarios.md) | 主角人物在 `persons.yaml`；剧本 → 主角 / `player_force` 在 `narrative/scenarios.yaml`（不设 `protagonists.yaml`） |
| [03-abilities-skills-jobs.md](03-characters/03-abilities-skills-jobs.md) | `abilities.yaml` `skills.yaml` `jobs.yaml` `posts.yaml` `zodiac.yaml` |
| [04-recruitment-crew.md](03-characters/04-recruitment-crew.md) | 招募规则 |
| [05-portraits-animation.md](03-characters/05-portraits-animation.md) | Chara / bustup |
| [06-companions-bonds.md](03-characters/06-companions-bonds.md) | 开局同伴 |
| [07-fatigue-health-mood.md](03-characters/07-fatigue-health-mood.md) | `vitals.yaml`：身体 7 态 / 精神 6 态 |

### 04 经济

| 文档 | 配置落点 |
| --- | --- |
| [01-trading-items.md](04-economy/01-trading-items.md) | `trading_items.yaml` `trading_item_kinds.yaml` |
| [02-market-price.md](04-economy/02-market-price.md) | `culture_prices.yaml` `town_state_price_rules.yaml` `formulas/economy.yaml` |
| [03-trade-loop.md](04-economy/03-trade-loop.md) | 买卖会话、货格、堆叠 |
| [04-special-products-investment.md](04-economy/04-special-products-investment.md) | `special_products.yaml` `investment.yaml` |
| [05-goods-art-spawn-display.md](04-economy/05-goods-art-spawn-display.md) | 图标 / 上架 / 星品 |
| [06-npc-fleet-trade.md](04-economy/06-npc-fleet-trade.md) | NPC 自动买卖 |
| [07-investment-tribute.md](04-economy/07-investment-tribute.md) | 投资→发展度、上缴 |
| [08-price-intel.md](04-economy/08-price-intel.md) | 付费查行情 |

### 05 船与舰队

| 文档 | 配置落点 |
| --- | --- |
| [01-ship-model.md](05-ships-fleet/01-ship-model.md) | `ships/models.yaml` `hulls.yaml` `rig.yaml` |
| [02-fleet-proxy.md](05-ships-fleet/02-fleet-proxy.md) | `fleets/templates.yaml` `proxy_rules.yaml` |
| [03-cabins-crew-posts.md](05-ships-fleet/03-cabins-crew-posts.md) | `cabins.yaml` `characters/posts.yaml` |
| [04-remodel-guns-supplies.md](05-ships-fleet/04-remodel-guns-supplies.md) | `guns.yaml` `supplies.yaml` `figureheads.yaml` |
| [05-hull-catalog-market-start.md](05-ships-fleet/05-hull-catalog-market-start.md) | 107 船体 / 39 船模、买卖、开局船 |
| [06-crew-headcount.md](05-ships-fleet/06-crew-headcount.md) | 无名水手人数 |
| [07-dock-warehouse.md](05-ships-fleet/07-dock-warehouse.md) | 船坞 100 槽 |
| [08-gun-shell-catalog.md](05-ships-fleet/08-gun-shell-catalog.md) | 六炮中文名 |

### 06 航海

| 文档 | 配置落点 |
| --- | --- |
| [01-sailing.md](06-ocean/01-sailing.md) | `ocean/sailing.yaml` `wind.yaml` |
| [02-search-encounters.md](06-ocean/02-search-encounters.md) | `search.yaml` `encounters.yaml` |
| [03-gold-road.md](06-ocean/03-gold-road.md) | `gold_road.yaml` |
| [04-manual-auto-sail.md](06-ocean/04-manual-auto-sail.md) | 手动 / 自动航线 |
| [05-sea-happenings.md](06-ocean/05-sea-happenings.md) | 20 槽海上事件 |
| [06-npc-fleet-ai.md](06-ocean/06-npc-fleet-ai.md) | NPC 大地图 AI |

### 07 战斗

| 文档 | 配置落点 |
| --- | --- |
| [01-naval-battle.md](07-combat/01-naval-battle.md) | `combat/naval.yaml` |
| [02-boarding.md](07-combat/02-boarding.md) | `boarding.yaml` |
| [03-battle-ai.md](07-combat/03-battle-ai.md) | `ai.yaml` |
| [04-ship-vs-port.md](07-combat/04-ship-vs-port.md) | 舰对港 / 占领 |
| [05-formation-happening.md](07-combat/05-formation-happening.md) | 五槽阵型、场景 happening |
| [06-prizes-capture.md](07-combat/06-prizes-capture.md) | 战利品 / 俘船 |

### 08 港口生活

| 文档 | 配置落点 |
| --- | --- |
| [01-facilities.md](08-town-life/01-facilities.md) | `town/facilities.yaml` |
| [02-requests-schemes.md](08-town-life/02-requests-schemes.md) | `request_kinds.yaml` `request_generation.yaml` `schemes.yaml` |
| [03-minigames.md](08-town-life/03-minigames.md) | `minigames.yaml` |
| [04-schemes-rhetoric.md](08-town-life/04-schemes-rhetoric.md) | 10 种话术/计谋 |
| [05-inn-church-palace.md](08-town-life/05-inn-church-palace.md) | 旅馆 / 王宫规则；遗址·教会·城门按日仪式 |
| [06-plaza-intel-adman.md](08-town-life/06-plaza-intel-adman.md) | 广场招牌 / CAdman |

### 09 道具

| 文档 | 配置落点 |
| --- | --- |
| [01-item-catalog.md](09-items/01-item-catalog.md) | `items/items.yaml` `kinds.yaml` |
| [02-equipment-effects.md](09-items/02-equipment-effects.md) | `effects.yaml` |
| [03-discovery-planting.md](09-items/03-discovery-planting.md) | 种子→特产品 |
| [04-ancient-treasures-hd.md](09-items/04-ancient-treasures-hd.md) | **HD 古宝 / 霸者之证** |

### 10 叙事

| 文档 | 配置落点 |
| --- | --- |
| [01-scenarios.md](10-narrative/01-scenarios.md) | `narrative/scenarios.yaml` |
| [02-event-system.md](10-narrative/02-event-system.md) | `events.yaml` `triggers.yaml` |
| [03-event-commands.md](10-narrative/03-event-commands.md) | 16-bit 指令集 |
| [04-world0-world1-diff.md](10-narrative/04-world0-world1-diff.md) | 两包差集 |
| [05-gallery-unlock.md](10-narrative/05-gallery-unlock.md) | 通关画廊点数 |

### 11 成长

| 文档 | 配置落点 |
| --- | --- |
| [01-level-exp-skills.md](11-progression/01-level-exp-skills.md) | `progression/level.yaml` `formulas/progression.yaml` |
| [02-fame-titles.md](11-progression/02-fame-titles.md) | 图鉴；原版无名声 / 爵位（`fame.yaml` 默认关闭） |
| [03-gameover-succession.md](11-progression/03-gameover-succession.md) | 寿终 / Game Over（原版无继承） |

### 12 界面与存档

| 文档 | 配置落点 |
| --- | --- |
| [01-scenes-flow.md](12-ui-save/01-scenes-flow.md) | `ui/scenes.yaml` |
| [02-save-session.md](12-ui-save/02-save-session.md) | 存档 schema |
| [03-localization.md](12-ui-save/03-localization.md) | `ui/strings.*.yaml` |

### 13 算法总表

| 文档 | 配置落点 |
| --- | --- |
| [01-formulas.md](13-algorithms/01-formulas.md) | `config/formulas/*` |
| [02-caps-ranges.md](13-algorithms/02-caps-ranges.md) | `caps.yaml` `engine_limits.yaml` |
| [03-adjustment-factors.md](13-algorithms/03-adjustment-factors.md) | 等级 / 视野 / 补给 |

### 14 配置 schema

[14-config-schemas/README.md](14-config-schemas/README.md) — 各表字段、类型、外键、默认原版包。

### 15 原版内容目录

从 DLL 抽出的实例表，供对照，不是运行时格式。

| 文档 | 数据 |
| --- | --- |
| [towns.md](15-catalogs/towns.md) | 97 港 |
| [persons.md](15-catalogs/persons.md) | 235 人 |
| [items.md](15-catalogs/items.md) | 231 道具 |
| [forces.md](15-catalogs/forces.md) | 25 势力 |
| [trading-items.md](15-catalogs/trading-items.md) | 105 交易品（文案） |

### 数据与规范

- [_data/](_data/) — 从 World0 抽出的 JSON/TSV
- [_meta/source-map.md](_meta/source-map.md) — 源码索引
- [_meta/designer-spreadsheets.md](_meta/designer-spreadsheets.md) — 大佬 dh4 图 vs SC exe

## 系统依赖（允许的方向）

见 [03-decoupling-principles.md](00-overview/03-decoupling-principles.md)「系统依赖图」：查询依赖无环，反向只走事件。叙事可查询一切，不可被经济/战斗反向依赖。事件合同见 [06-system-interfaces.md](00-overview/06-system-interfaces.md)。

## 事实等级

正文用 `[确认]` / `[推断]` / `[未知]`。实现时只把确认项写进代码默认包；推断进 YAML 可改；未知保持 `null` 或 `unknown_bytes`。

## 规模

97 篇 Markdown（含 `_meta` 与各目录 README）+ 28 份 YAML 示例（`14-config-schemas/examples/`，含 `mods/mymod/` 6 份）。原版包对照：97 港、235 人、231 道具、105 交易品、25 势力、21 文化圈 / 七大海域、107 船体 / 39 船模、200 航线 / 164 折点、24 村落、25 遗迹、12 种地图点；七剧本、两份世界包。

计数注意：`srcdata` 里部分 `NUM_*` 是 RTC 投票误采（`NUM_SHIPTYPE=5` 实 39、`NUM_BATTLEGUN=5` 实 21、`NUM_SHIPMAST=100` 实 3），引用时注明证据类型（判界 / 循环上界 / 投票）。原版没有名声、爵位、继承；教会不是设施。
