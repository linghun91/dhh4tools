# 配置 Schema 总表

运行时只读 `config/`。这里是规格：每个文件的 schema 名、所属文档、主键、外键、定义 vs 状态。真正的表由内容管线从 World DLL / mesfile 生成后再补 key。

加载顺序、内容包、schema 版本规则见 [05-config-architecture.md](../00-overview/05-config-architecture.md)。公式见 [13-algorithms/01-formulas.md](../13-algorithms/01-formulas.md)。上限见 [13-algorithms/02-caps-ranges.md](../13-algorithms/02-caps-ranges.md)。ID 计数见 [04-id-registry.md](../00-overview/04-id-registry.md)。

每个 YAML 头：

```yaml
schema: dk4.<domain>.<file>
version: 1
```

引擎按 `schema` 选解析器。加字段升 version。缺字段不回退代码默认值。

**定义** = 只读内容包。**状态** = 存档覆盖。禁止把定义拷进存档再改（原版部分表运行时会被改写，重构必须标出可变字段）。

主键一律 `id: int`（与原版枚举一致）+ `key: string`（稳定别名）。外键只传 ID，不传指针。

示例目录：`Docs/14-config-schemas/examples/`（不是运行时路径；运行时在 `config/`）。

---

## 全局

### `config/caps.yaml`

| 项 | 值 |
| --- | --- |
| schema | `dk4.caps` |
| 所属文档 | [13-algorithms/02-caps-ranges.md](../13-algorithms/02-caps-ranges.md) |
| 主键 | 无行主键；键即上限名 |
| 外键 | 无 |
| 定义 / 状态 | **定义**。存档只存「公式包/caps 包 version」，不存上限原文 |

字段：`level_per_track`、`ability_growth_cap`、`ability_settled_cap`、`buy_ability_clamp`、`skill_cap`、`town_develop`、`cargo_stack`、`fleet_ships`、`hold_slots_per_ship`、`sailor_roster` 等。必须 ≤ `engine_limits`。

### `config/engine_limits.yaml`

| 项 | 值 |
| --- | --- |
| schema | `dk4.engine_limits` |
| 所属文档 | 同上 |
| 主键 | 无 |
| 外键 | 无 |
| 定义 / 状态 | **定义**（引擎实现硬顶，内容包不能突破） |

字段：整数宽度、`overflow: saturate`、`level_per_track: 255`、`town_develop: 65535`、`cargo_stack: 255`。

---

## `config/formulas/`

全部 **定义**。无主键行（公式名为键）。无外键到内容表；可引用 `eSkill` 下标（内容在 `characters/skills.yaml`）。

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 / 状态 |
| --- | --- | --- | --- | --- | --- |
| `progression.yaml` | `dk4.formulas.progression` | [11-progression/01-level-exp-skills.md](../11-progression/01-level-exp-skills.md)、[01-formulas.md](../13-algorithms/01-formulas.md) F-PROG-* | 公式名 | `buy_skill_index` 等指向 `eSkill` | 定义 |
| `economy.yaml` | `dk4.formulas.economy` | [04-economy/02-market-price.md](../04-economy/02-market-price.md)、F-ECO-* | 公式名 | `buy_skill_index` → skill | 定义。投资/进贡/修理占位键必须存在，值可标 `unverified: true` |
| `combat.yaml` | `dk4.formulas.combat` | [07-combat/01-naval-battle.md](../07-combat/01-naval-battle.md)、F-CBT-* | 公式名 | `battle_skill_index` → skill | 定义 |
| `ocean.yaml` | `dk4.formulas.ocean` | [06-ocean/01-sailing.md](../06-ocean/01-sailing.md)、F-OCN-* | 公式名 | 无 | 定义。航速占位 |
| `character.yaml` | `dk4.formulas.character` | [03-characters/03-abilities-skills-jobs.md](../03-characters/03-abilities-skills-jobs.md) | 公式名 | 无 | 定义。夹紧参数；配对表在 skills.yaml |

---

## `config/ids/`

### `generated.json`

| 项 | 值 |
| --- | --- |
| schema | `dk4.ids` |
| 所属文档 | [04-id-registry.md](../00-overview/04-id-registry.md) |
| 主键 | 各枚举 `id` + `key` |
| 外键 | 无（本文件就是注册表） |
| 定义 / 状态 | **定义**，构建时从各内容表收集，禁止手写两份 |

含 `invalid: -1`、`trading_item.empty: 106`。校验：任何外键必须指向已注册 ID 或哨兵。

---

## `config/world/`

所属：[01-world](../01-world/)（写作时目录为空，owner 路径仍按总 README）。

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态（存档） |
| --- | --- | --- | --- | --- | --- | --- |
| `map.yaml` | `dk4.world.map` | `01-world/01-map-and-sea-areas.md` | 无（一份地图） | 资源 `World.dat` | 坐标系、尺寸 2500×2500、像素网格引用 | 无。探索迷雾另表 |
| `sea_areas.yaml` | `dk4.world.sea_areas` | 同上 | `id` (`eSeaArea`) | 无 | 海域名、邻接 | `[未知]` 是否有运行时占领 |
| `towns.yaml` | `dk4.world.towns` | `01-world/02-towns.md` | `id` (`eTown`) | `region` → country `[推断]`；`special_products[].trading_item_id` | 坐标 `x1,y1,x2,y2`、`is_capital`、特产品槽定义、`unknown_bytes` | 发展度、合约、流行、份额、存在位。**市价不属于本表** |
| `villages.yaml` | `dk4.world.villages` | `01-world/03-villages-discovery.md` | `id` (`eVillage`) | 坐标/海域 `[未知]` | 村落定义 | 发现状态 |
| `remains.yaml` | `dk4.world.remains` | 同上 | `id` (`eRemains`) | `[未知]` | 遗迹定义 | `REMAINSSTATE_BITS` |
| `routes.yaml` | `dk4.world.routes` | `01-world/05-routes-navigation.md` | `id` (`eRoute`) | `town_id` 端点 `[未知]` 是否双向 | 航线定义 | 是否已开辟 |
| `junctions.yaml` | `dk4.world.junctions` | 同上 | `id` (`eJunction`) | route | 节点定义 | `[未知]` |
| `climate.yaml` | `dk4.world.climate` | `01-world/04-climate-weather-zodiac.md` | 气候带 / `eZodiac` | `sea_area_id` | 气候、星座定义 | 当日天气（状态） |

港口坐标属于 world；港口市价属于 economy；港口驻军属于 factions。同一 `town_id` 关联。

---

## `config/factions/`

所属：[02-factions](../02-factions/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `countries.yaml` | `dk4.factions.countries` | `02-factions/02-countries-culture-fashion.md` | `id` (`eCountry`) | 无 | 国家名。计数 `[未知]` | 无或极少 |
| `cultures.yaml` | `dk4.factions.cultures` | 同上 | `id` (`eCulture`) | 无 | 文化圈 | 无 |
| `forces.yaml` | `dk4.factions.forces` | `02-factions/01-forces.md` | `id` (`eForce`) | `country_id` `[未知]` | 势力名、色、World 行 `unknown_bytes` | 金币、友好、影响、账本 |
| `relations.yaml` | `dk4.factions.relations` | `02-factions/03-relations-influence.md` | `(force_a, force_b)` | `force_id` ×2 | 默认关系 | 当前关系、战争、协议 |
| `fashion.yaml` | `dk4.factions.fashion` | `02-countries-culture-fashion.md` | `id` (`eTownFashion`) | `trading_item_id` `[未知]` | 流行品定义 | 各港当前流行（状态，常挂在 town 会话） |

---

## `config/characters/`

所属：[03-characters](../03-characters/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `abilities.yaml` | `dk4.characters.abilities` | `03-characters/03-abilities-skills-jobs.md` | `id` (`eAbility` 0–5) | 无 | 六维名：体力 敏捷 魅力 智力 精神 运气 | 无 |
| `skills.yaml` | `dk4.characters.skills` | 同上 | `id` (`eSkill` 0–9) | `stat_a`, `stat_b` → ability | 技能名、双属性配对 | 无（技能不存档） |
| `jobs.yaml` | `dk4.characters.jobs` | 同上 | `id` (`eJobKind`) | `[未知]` | 职业 | 人物当前职业为状态 |
| `posts.yaml` | `dk4.characters.posts` | `05-ships-fleet/03-cabins-crew-posts.md` | `id` (`ePost`) | `cabin_kind` `[未知]` | 岗位定义 | 船上任命 |
| `persons.yaml` | `dk4.characters.persons` | `03-characters/01-person-model.md` | `id` (`ePerson`) | `force_id` 初始所属 `[未知]` | 姓名、初始六维、`unknown_bytes` | 经验两段、六维成长、隶属、装备、疲劳、心情 |
| `protagonists.yaml` | `dk4.characters.protagonists` | `03-characters/02-protagonists-scenarios.md` | `person_id` | `person_id`、`scenario_id` | 哪几个 id 可当主角 | 存档 `+0x20` 当前主角 |

---

## `config/economy/`

所属：[04-economy](../04-economy/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `trading_item_kinds.yaml` | `dk4.economy.trading_item_kinds` | `04-economy/01-trading-items.md` | `id` (`eTradingItemKind`) | 无 | 19 类名 | 无 |
| `trading_items.yaml` | `dk4.economy.trading_items` | 同上 | `id` (`eTradingItem`) | `kind_id` | 基础价、等级、空哨兵行 `id=106` | 无（市价在 price 状态） |
| `special_products.yaml` | `dk4.economy.special_products` | `04-economy/04-special-products-investment.md` | `(town_id, slot 0–4)` | `town_id`、`trading_item_id` | 每港 5 槽、供给/价、grade | 供给余量、品牌 |
| `price_curves.yaml` | `dk4.economy.price_curves` | `04-economy/02-market-price.md` | `trading_item_id` 或 kind | `trading_item_id`、`town_id` 可选 | 弹性曲线参数 | 当日市价、供给 |
| `investment.yaml` | `dk4.economy.investment` | `04-special-products-investment.md` | 无或档位 id | `town_id` | 投资档位（式子 `[未知]`） | 已投资额、发展度变化 |

交易会话（5×5 槽、卖出列表）是运行时状态，进存档船货，不进这些 YAML。

---

## `config/ships/`

所属：[05-ships-fleet](../05-ships-fleet/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `types.yaml` | `dk4.ships.types` | `05-ships-fleet/01-ship-model.md` | `id` (`eShipType`) | 无 | 船型上限 | 无 |
| `hulls.yaml` | `dk4.ships.hulls` | 同上 | `id` (`eShip`) | `type_id` | 船体定义、耐久/船员上限 | 当前耐久、帆、炮、货 |
| `cabins.yaml` | `dk4.ships.cabins` | `05-ships-fleet/03-cabins-crew-posts.md` | `id` (`eShipCabin`) | `post_id` | 舱室定义 | 船上是否安装 |
| `guns.yaml` | `dk4.ships.guns` | `05-ships-fleet/04-remodel-guns-supplies.md` | `id` (`eGunType`) | 无 | 火炮类型 | 船上炮位 |
| `shells.yaml` | `dk4.ships.shells` | 同上 | `id` (`eShell`) | `gun_type_id` `[未知]` | 炮弹 | 库存 |
| `figureheads.yaml` | `dk4.ships.figureheads` | 同上 | `id` (`eFigureHead`) | 无 | 船首像 | 安装 |
| `supplies.yaml` | `dk4.ships.supplies` | 同上 | `id` (`eShipSupply`) | 无 | 补给种类 | 船上余量 |

货格内容状态在舰队/船对象上：`(ship_id, slot) → {trading_item_id, qty, buy_price}`。空 = `trading_item.empty`。

---

## `config/fleets/`

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `templates.yaml` | `dk4.fleets.templates` | `05-ships-fleet/02-fleet-proxy.md` | `id` (`eFleet`) | `force_id`、`ship_id`、`person_id` | World 舰队模板 hex | 位置、编制、委任 |
| `proxy_rules.yaml` | `dk4.fleets.proxy_rules` | 同上 | 规则 id | `force_id` | 地方舰队 AI/买货用哪条公式 | 地方舰队存活 |
| `formations.yaml` | `dk4.fleets.formations` | 同上 | 阵型 id | 无 | 阵型定义 | 当前阵型 |

---

## `config/combat/`

所属：[07-combat](../07-combat/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `naval.yaml` | `dk4.combat.naval` | `07-combat/01-naval-battle.md` | 无或算法 id | 读 `ModifierSet`、船、炮 | 海战参数 | 战斗会话不进内容包 |
| `boarding.yaml` | `dk4.combat.boarding` | `07-combat/02-boarding.md` | 无 | `battle_skill_index` | 接舷参数（含 F-CBT-01） | 会话 |
| `ai.yaml` | `dk4.combat.ai` | `07-combat/03-battle-ai.md` | `id` (`eBattleAlgo`) | 无 | AI 算法 | 无 |

---

## `config/ocean/`

所属：[06-ocean](../06-ocean/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `sailing.yaml` | `dk4.ocean.sailing` | `06-ocean/01-sailing.md` | 无 | 船、帆 | 航速参数（占位） | 舰队位置、航向 |
| `wind.yaml` | `dk4.ocean.wind` | 同上 | 无或海域 | `sea_area_id` | 风场参数 | 当前风 |
| `search.yaml` | `dk4.ocean.search` | `06-ocean/02-search-encounters.md` | 无 | village/remains | 探索参数 | 已发现 |
| `encounters.yaml` | `dk4.ocean.encounters` | 同上 | 遭遇 id | `fleet_id` 模板 | 遭遇表 | 无 |
| `gold_road.yaml` | `dk4.ocean.gold_road` | `06-ocean/03-gold-road.md` | 航路 id | town/route | 黄金航路定义 | 进度 |

---

## `config/town/`

所属：[08-town-life](../08-town-life/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `facilities.yaml` | `dk4.town.facilities` | `08-town-life/01-facilities.md` | 设施 kind | `town_id` 可用性 | 交易所/造船/酒馆/公馆/公会/教会/宫殿 | 开闭、NPC |
| `requests.yaml` | `dk4.town.requests` | `08-town-life/02-requests-schemes.md` | `id` (`eRequest`) | `town_id`、`trading_item_id`、`person_id` | 委托定义 | 接取/完成 |
| `schemes.yaml` | `dk4.town.schemes` | 同上 | `id` (`eScheme`) | `[未知]` | 计略定义 | 冷却/结果 |
| `minigames.yaml` | `dk4.town.minigames` | `08-town-life/03-minigames.md` | 小游戏 id | 无 | 规则参数 | 无 |

---

## `config/items/`

所属：[09-items](../09-items/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `kinds.yaml` | `dk4.items.kinds` | `09-items/01-item-catalog.md` | `id` (`eItemKind`) | 无 | 分类 | 无 |
| `items.yaml` | `dk4.items.items` | 同上 | `id` (`eItem`) | `kind_id` | 装备/发现物名、World hex | 持有、发现 |
| `effects.yaml` | `dk4.items.effects` | `09-items/02-equipment-effects.md` | `(item_id, slot)` | `item_id`、`ability_id`/`skill_id` | 效果量 | 装备在人/船上为状态 |

道具价折扣读 `ModifierSet.item_price_factor`，不把六维写进 effects。

---

## `config/narrative/`

所属：[10-narrative](../10-narrative/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `scenarios.yaml` | `dk4.narrative.scenarios` | `10-narrative/01-scenarios.md` | `id` (`eScenario` 0–6) | `world_pack`、`protagonist_person_id`、`event_pack` | 剧本选包 | 当前剧本（开局选定） |
| `events.yaml` | `dk4.narrative.events` | `10-narrative/02-event-system.md` | 事件 id | 多（只读查询） | EV0–EV6 脚本映射 | 事件旗标 |
| `triggers.yaml` | `dk4.narrative.triggers` | 同上 | 触发器 id | `town_id`、`person_id`、旗标 | 条件定义 | 已触发 |

叙事可查询一切，不可被经济/战斗反向依赖。

---

## `config/progression/`

所属：[11-progression](../11-progression/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `level.yaml` | `dk4.progression.level` | `11-progression/01-level-exp-skills.md` | 无 | 无 | 可放曲线注释；**魔数在 formulas/progression.yaml** | 经验在人物状态 |
| `fame.yaml` | `dk4.progression.fame` | `11-progression/02-fame-titles.md` | 称号 id | `[未知]` | 名声阈值 | 当前名声 |

不要在 `level.yaml` 再写一份 `exp_divisor`。公式唯一源是 `formulas/`。

---

## `config/ui/` 与 `localization/`

所属：[12-ui-save](../12-ui-save/)

| 文件 | schema | 所属文档 | 主键 | 外键 | 定义 | 状态 |
| --- | --- | --- | --- | --- | --- | --- |
| `ui/scenes.yaml` | `dk4.ui.scenes` | `12-ui-save/01-scenes-flow.md` | 场景 id | 无玩法外键 | 场景流 | 当前场景（会话，可不进存档） |
| `ui/strings.zh-CN.yaml` | `dk4.ui.strings` | `12-ui-save/03-localization.md` | 文案 key | 无 | **只文案** | 无 |
| `localization/languages.yaml` | `dk4.localization.languages` | 同上 | 语言码 | `strings.*` 文件 | zh-CN / zh-TW / ja-JP / ko-KR | 玩家语言设置 |

禁止把 UI 字符串写进逻辑配置。地方舰队 100000 金币进 `formulas/economy.yaml`，文案只引用格式化参数。

---

## 存档（非 `config/`，但字段分类在此对照）

所属：`12-ui-save/02-save-session.md`。源：`srcdata/docs/save_format.md`。

存：内容包 id + schema version、公式包 version、各系统**状态**列。

不存：公式原文、技能值、可由经验重建的等级（可缓存）。

原版技能不存、等级有缓存（`save+0x70`）。重构同此：经验是源。

---

## 校验规则

1. `engine_limits` → `caps`（caps ≤ 硬顶）→ `formulas` → 内容表并行 → `narrative/scenarios.yaml` 选包 → 存档覆盖状态。
2. 外键 ∈ 注册表 ∪ `{invalid, empty}`。
3. `trading_item_id == 0` 合法；空格必须是 `empty`（106）。
4. `unknown_bytes` 原样保存，禁止猜成确定字段后仍留在「已命名」区。
5. World0 / World1 同一 schema，两份数据。

## 示例文件

| 路径 | 覆盖 |
| --- | --- |
| [examples/caps.yaml](examples/caps.yaml) | 设计上限 |
| [examples/engine_limits.yaml](examples/engine_limits.yaml) | 硬顶 |
| [examples/formulas/progression.yaml](examples/formulas/progression.yaml) | F-PROG-* |
| [examples/formulas/economy.yaml](examples/formulas/economy.yaml) | F-ECO-* 含占位 |
| [examples/formulas/combat.yaml](examples/formulas/combat.yaml) | 海战射程/溃败/接舷 |
| [examples/formulas/ocean.yaml](examples/formulas/ocean.yaml) | 风力 CDF / 航速参数 |
| [examples/ids/generated.snippet.yaml](examples/ids/generated.snippet.yaml) | 伦敦/布里斯托尔/拉斐尔/空货格 |
| [examples/world/towns.yaml](examples/world/towns.yaml) | 伦敦、布里斯托尔 |
| [examples/characters/persons.yaml](examples/characters/persons.yaml) | 拉斐尔 |
| [examples/economy/trading_items.yaml](examples/economy/trading_items.yaml) | 空哨兵 + 伦敦特产品槽 0 的 id 99 |

系统文档 01–12 成文后，本表「所属文档」列保持不变；若某文件改名，只改路径。
