# 跨系统接口

系统之间只传 **ID、快照、事件**。禁止持有对方对象指针。本篇是**规范名**索引：跨系统调用只用本篇左列的名字；系统文档里出现的其它写法见各表「别名」列，实现时在适配层 / `triggers.yaml` 作别名，新文档不得再用。字段细节以「提供方」文档为准。

语义规则见 `03-decoupling-principles.md` 原则 5（事件是事实、阶段边界派发、顺序确定）；tick 顺序见 `05-config-architecture.md`；决策依据见 `09-architecture-decisions.md`（下称 D1–D10）。

## 命名规则

| 类别 | 形式 | 例 |
| --- | --- | --- |
| 查询 | `Service.snake_case(args) -> 值` | `Towns.develop(town_id, track)` |
| 命令 | `Service.snake_case(args)`；文档里的 `PascalCase { … }` 消息体视为同一命令 | `Cabins.assign(...)` ≡ `AssignPost { … }` |
| 事件 | `PascalCase` 过去时，只由状态所有者发 | `TownDeveloped` |

- 各篇已写的 camelCase 方法名（`isAllied`、`mayAttack`、`countRole`）与 snake_case 一一对应，不再单列别名。
- 参数里的 ID 一律是 key 解析后的运行时 ID；空值为 `null`，不传原版哨兵（98、106、0xEC …）。
- 只在单个系统内部或只给 UI 的事件（炮弹落点、阵型动画、小游戏走子等）不进本表，但不得与本表重名。

## 服务名与所有者

| 服务名 | 所有系统 | 文档 |
| --- | --- | --- |
| `Ids` `Clock` `Calendar` `Rng` `Modifiers` `Actions` `Session` | core | `05-config-architecture.md`、`01-world/07` |
| `Map` `Maps` `SeaArea` `Towns` `Village` `Remains` `Routes` `Junction` `Climate` `MapPointKind` `Coords` `Puzzle` `OldMap` | world | `01-world/*` |
| `Force` `Relation` `Amity` `Share` `Contract` `Influence` `Overlord` `Aim` `SeaRecord` `BattleResult` `Culture` `MajorSea` `Fashion` `Letters` `Country`；`Factions`（只有 `Factions.influence` 一个查询） | factions | `02-factions/*` |
| `Characters` `Presentation` | characters | `03-characters/*` |
| `Economy` `Trade` `Invest` `TreasureRent` `NpcTrade` | economy | `04-economy/*` |
| `Ships` `Hulls` `Cabins` `Crew` `Guns` `Yard` `Dock` `Supplies` | ships | `05-ships-fleet/*` |
| `Fleets` | fleets | `05-ships-fleet/02`、`06` |
| `Ocean` | ocean | `06-ocean/*` |
| `Combat` `Boarding` `Duel` `AI` `PrizeHolds` | combat | `07-combat/*` |
| `TownLife` `FacilityRules` `Plaza` `Adman` `Requests` `Schemes` `Minigames` | town-life | `08-town-life/*` |
| `Items` `Equip` `Planting` `Hunting` `Treasures` `Proofs` | items | `09-items/*` |
| `Narrative` `Scenario` `Flags` `Events` `Gallery` | narrative | `10-narrative/*` |
| `Progression` `GameOver` `Discovery` | progression | `11-progression/*` |
| `Scene` `Save` `Profile` `Loc` `UI` `Audio` | ui / save | `12-ui-save/*` |

## 状态所有权裁决

第一轮各篇对下列状态的所有者写法冲突。以本表为准；与之冲突的系统文档要改。

| 状态（原版位置） | 所有者 | 写入方式 | 读取 | 裁决理由 |
| --- | --- | --- | --- | --- |
| 港口商业 / 军事发展度（`CTown+0x1E` 2×u16，`fn_14000bc03(town, track)`） | **world 的 `Towns` 运行态** | economy（投资转化）、combat（炮击 / 攻城）、narrative 发命令 `Towns.apply_develop(town_id, track, delta, reason)`；`Towns` 饱和到 `caps.town_develop`（9999，`0x14025f577 mov ecx,0x270F`）后发 `TownDeveloped` | ships（上架、改装价、炮种）、economy（配额、上架门槛）、town-life（地图点解锁、酒馆档）、combat（城防） | 写方三家、读方四家，挂在任一业务系统都会成环；world 已持有港口定义与已知位 |
| 港口景气状态（`CTown+0x74`，0–12） | economy | 月初状态机（`CTownElement::vfunc_04`） | ships（买卖船、改装价系数 `fn_14000643d`）经 `Economy.town_state` | 原版市价目标表按它跳转（`0x140267cc8`）；价格系数表 `economy/town_state_price_rules.yaml` 按 `context: ship_buy / ship_sell / remodel` 取 |
| 港口份额槽（`CTown+0x18` 3×(force, share)，DLL `+0x4C`） | factions | `Share.add` / `Contract.sign` | economy、combat、town-life | 01-world 已核实 `+0x4C → +0x18` 是份额槽；**不是**流行槽 |
| 港口流行（`CTown+0x70` 2×(item, level)，DLL `+0x7E`，空 = 交易品空值） | factions | `Fashion.set_slot` | economy（流行因子、月初推力） | 原 `08-town-life/01`「流行 3 槽、`0x1A` 空」与份额槽混读，作废 |
| 特产品槽运行态（供给、本月已购、待种锁） | economy | `Economy.settle_special`、补货、种植 | ships、items | `04-economy/04` |
| 地图点集合（`CTown+0x14` 位域，DLL `+0x48`） | town-life | 订阅 `TownDeveloped`，先过据点港门（`fn_140009b42`，`0x14025f867`），再查 `map_point_unlock_table`（里程碑表 `0x1405a0490`）解锁；叙事命令 | world UI、narrative | D9：设施 = 地图点 kind → 处理器 |
| 港口已知位（`CTown+0x10` bit0） | world `Towns` | `Towns.mark_known` | routes（航线可见）、economy（行情可见） | 航线可见由它派生（`fn_14001389f`） |
| 势力金币（`CForce+0x0C`，封 0..99,999,999，`fn_140002cb6`） | factions | `Force.add_gold(force_id, delta, reason)` | 全部 | 玩家金币就是玩家势力金币；投资只扣它，不进账本 |
| 舰队账本（6 槽） | economy | `Economy.post_income / post_expense` | ui | `04-economy/04`、`05-ships-fleet/02` |
| 文化圈卖价（21×105） | economy（数值列） | 卖出回升、`fn_14000c658` | factions 只读 | D10 拆表规则；行（名、major_sea）归 factions |
| 七大海域 `major_sea_id` | factions（ID 域登记） | 定义表 `world/major_seas.yaml` | world（BGM）、items、economy | 定义是 core 内容，任何系统可读 |
| 人物经验（两段） | characters（状态字段） | 只有 progression 的 `Progression.grant_exp` 可写 | characters（技能值）、ui | 打破 characters ↔ progression 环 |
| 技能值 | characters（派生） | — | 全部经 `Characters.skill` 或 `Modifiers.eval("skill.<key>")` | `Progression.skill_value` 作别名 |
| 舱室任职（`CPlayerFleetData` 4×31） | ships `Cabins` | `Cabins.assign / unassign` | characters、fleets（`post_holder`） | 岗位由舱室种类派生（`fn_140012dd2`），人物不另存 |
| 舰队疲劳（`CFleet+0x71`，钳 0..100，`fn_140009467`）`[推断，强]` | fleets | `Fleets.add_fatigue(fleet_id, delta, reason)` | ocean、combat、characters | `Characters.crew_fatigue` 作别名；人物 HP / 身体态 / 精神态归 characters（`Characters.vitals`） |
| 海上 happening 态（`CPlayerFleetData+0x30`，初值 27） | ocean | 掷骰 / 结局 | ocean（无风）、world climate（重掷门） | `06-ocean/05` |
| 航行模式（`SceneDesc+0xC` 海战、`+0x10` 自动）`[推断，强]` | core `Session` | 场景切换 | ocean、combat、happening | `Session.mode() -> manual \| auto \| battle` |
| 星座（12，`0x140688970`） | characters | — | progression（成长） | world 不再暴露 `Zodiac.current()` |
| 宗主上缴（剧本旗标） | factions（执行、扣款、发 `SuzerainTributePaid`） | 月初 `forces` 阶段 `factions.suzerain_tribute` | economy 只给金额式，不执行 | `CForceElement::vfunc_04` |
| 独占港上缴 | economy | 月末 `towns` 阶段 | factions 只读 | `CTownElement::vfunc_04` 独占门 `0x140267a0a` |
| 出租费（`treasure_rent`） | economy | 月末 `items` 阶段，排在 `towns` 独占上缴之前 | ui | `CItemElement::vfunc_04`（世界 tick 第 3 行，早于第 5 行 `CTownElement`） |
| 合约份额 | factions | `Contract.sign` 由 factions 执行并调 `Share.add(…, reason: contract)`；只此一条路 | economy、town-life | `InvestmentMade` 不带份额增量，订阅方不得再加一次 |
| 补货月历（`restock_calendar`） | economy（`04-economy/04`） | — | `04-economy/05`、ocean 黄金航路奖金只引用 | 原版三处共用一张语义表，只留一份 |

## 派生量所有权（D6）

消费方只读快照，不自己重算；派生量不存档。

| 派生量 | 唯一计算方 | 规范查询 | 失效事件 |
| --- | --- | --- | --- |
| 必要 / 上限水手 | Ships | `Crew.capacity(ship_id) -> { min, max }` | `CabinKindChanged`、`AddPartChanged`、`MastChanged` |
| 帆值、旗舰索具 | Ships（`ships/rig.yaml`） | `Fleets.sailing_snapshot(fleet_id)` 内 `flagship` 段 | 同上、`FlagshipChanged` |
| 炮门数 | Ships | `Guns.doors(ship_id)` | `CabinKindChanged` |
| 估价 `outfit_value`、买卖价 | Ships（读 `Economy.town_state`） | `Yard.quote_buy / quote_sell` | `TownDeveloped` |
| 可用货格 | Ships | `Ships.usable_holds(ship_id)` | `CabinKindChanged` |
| 技能值、HP 上限 | Characters | `Characters.skill(person_id, skill_key)`、`Characters.vitals(person_id)` | `PersonEquipped`、`CabinBound`、`VitalsChanged`、`LevelChanged` |
| 航速 | Ocean | `Ocean.speed(fleet_id)` | 每拍现算 |
| 搜索半径 | Ocean | `Ocean.search_radius(fleet_id)` | `FigureheadChanged`、`LevelChanged` |
| 航线可见 / 可走 | world `Routes` | `Routes.visible(route_id, force_id)`、`Routes.path(a, b, force_id)` | `TownBecameKnown`、`ShareChanged` |
| 港口所属七大海域 | factions | `MajorSea.of_town(town_id)`（town → culture → major_sea） | 内容加载 |
| 岗位持有人 | fleets | `Fleets.post_holder(fleet_id, post_key)` | `CabinBound` / `CabinUnbound`、`FlagshipChanged` |
| 舱室计数 | Ships | `Ships.cabin_count(ship_id, role_key)` | `CabinKindChanged` |

## 查询（只读）

| 规范名 | 提供方 | 返回 | 别名（出现处） |
| --- | --- | --- | --- |
| `Ids.resolve(key) -> id` | core | 运行时整数 | — |
| `Calendar.snapshot()` | core（`01-world/07`） | 打包时间、月日拍、三态 | — |
| `Modifiers.eval(key, ctx)` | core（D2） | 值 + 可选贡献列表 | `Characters.modifiers`、`Characters.modifier_set`（`04-economy/06`、`06-ocean/05`）、`Characters.modifiers_for_post`、`Characters.protagonist_modifiers`（`04-economy/02`）、`Yard.figureheadModifiers`、`ModifierSet` 结构体 |
| `Scenario.flag(key)` | narrative（D5） | bool / int | 读 `DAT_144296500+0x7c`；`Narrative.current_scenario()` 只给 narrative / save / ui |
| `Session.mode()` | core | `manual \| auto \| battle` | `SceneDesc+0xC/+0x10`、`in_battle`、`Session.scene_mode` |
| `Towns.def(town_id)` | world（`01-world/02`） | 定义快照 | `Town.def`、`World.town`、`World.town_exists` |
| `Towns.develop(town_id, track)` | world | int | `Develop.get`（`04-economy/07`、`02-factions/03`）、`Town.develop`、`Town.commerce` / `Town.military`（`07-combat/04`）、`commerce(town_id)` |
| `Towns.is_known(town_id, force_id)` | world | bool | `Town.known` |
| `Climate.wind_at(x, y, month)` / `Climate.wind_effective(...)` | world（`01-world/04`） | WindSnapshot | `World.wind_at`（`06-ocean/05`）；`Ocean.wind_at` 可保留为 ocean 投影 |
| `Climate.weather()` | world | `{ kind, until }` | `World.climate_at` |
| `Map.cell(x, y)` / `Map.chip_of(x, y)` / `Map.tile_class(...)` | world（`01-world/01`、`06`） | 贴图编号 / 格类 | `Map.class_at` |
| `Village.def / discovered` | world | — | `World.village` |
| `Remains.def / discovered / at_town` | world | — | `Remain.*`（`01-world/11`）、`World.remains`、`Town.remains_of` |
| `Remains.town(remains_id)` | world（`01-world/02`、`09`、`11`） | town_id / null（由 `towns.yaml` 的 `remains` 外键反查） | — |
| `Routes.def / path / visible` | world（`01-world/05`） | — | `Routes.established` |
| `Share.of(town_id, force_id)` / `Share.slots(town_id)` / `Share.exclusive_owner(town_id)` | factions（`02-factions/03`） | 0–100 | `Factions.share`（`04-economy/*`）、`Factions.share_slots`、`query_share`、`Factions.force_of_town` |
| `Relation.get(a, b)` / `Relation.may_attack(a, b)` | factions（`02-factions/05`） | — | `Factions.relation`、`Factions.may_attack` |
| `Culture.of_town(town_id)` / `MajorSea.of_town(town_id)` | factions（`02-factions/06`） | id | `World.culture_of`、`Towns.theater`（`05-ships-fleet/05`）、`theater_of`、`MajorSea.ofTown`、`Factions.major_sea`、`Towns.major_sea` |
| `Fashion.current(town_id)` | factions（`02-factions/02`） | `[{ trading_item_id, level }]` | `Factions.fashion_slots`、`TownLife.plazaFashion` |
| `Force.get(force_id)` / `Force.gold(force_id)` | factions | — | `Factions.force`、`Economy.gold` |
| `Factions.influence(force_id, major_sea_id)` | factions（`02-factions/03`） | 影响力 int | `Influence.get`（`02-factions/03`）；`08-town-life/02`、`05`、`06` 已用本名 |
| `Characters.get(person_id)` | characters（`03-characters/01`） | PersonSnapshot | — |
| `Characters.skill(person_id, skill_key)` | characters | 技能值 0–`skill_value_cap` | `Progression.skill_value`、`Characters.skill_value`、`Progression.skill_base`（不含装备 / 身体态的中间值保留为 progression 投影） |
| `Characters.ability(person_id, ability_key)` | characters | 六维 0–`ability_cap` | `Progression.ability_base` |
| `Characters.vitals(person_id)` | characters（`03-characters/07`） | HP、身体态、精神态 | — |
| `Economy.quote(town_id, item_id, grade)` | economy（`04-economy/02`） | PriceSnapshot | — |
| `Economy.town_state(town_id)` | economy | 状态 key | `Town.state`（`04-economy/02`）、`Towns.kind`、`Towns.priceFactorKey`（`05-ships-fleet/04`、`05`）、`Economy.town_price_mod`；价格系数取 `economy/town_state_price_rules.yaml` 的 `context` |
| `Economy.special_offer(town_id, slot, force_id)` / `Economy.specials(town_id)` | economy（`04-economy/04`） | — | `Specials.*`（`04-economy/06`）、`Economy.special_slot` |
| `Economy.buy_cap(town_id, slot, force_id, channel)` | economy（`04-economy/04`） | 本月可买量；`channel` 取 `player_trade`（乘商业系数）或 `npc_proxy`（不乘） | `Specials.quota`、`Specials.buy_cap`、`fn_140001a37`、`fn_140013345` |
| `Economy.is_brand(town_id, slot)` | economy | bool。factions **不**调，订阅 `BrandRecognized` | — |
| `Ships.holds(ship_id)` / `Ships.count_item(fleet_id, item_id)` | ships（`05-ships-fleet/01`） | 货格 | `Economy.count_fleet_item`（`10-narrative/*`）、`Ships.cargo_view` |
| `Ships.cabin_count(ship_id, role_key)` | ships（D2） | int | `Cabins.countRole`、`Ships.countCabins`（`07-combat/06`）、`count_cabin_kind`、`fleet_cabin_count`、整数种类形参 |
| `Fleets.admiral(fleet_id)` / `Fleets.flagship(fleet_id)` / `Fleets.is_player_fleet(fleet_id)` | fleets | — | `Force.isPlayer` 判舰队时 |
| `Ships.occupants(ship_id, role_key)` | ships | `[person_id]` | `Characters.occupants` |
| `Clock.interrupt_pending()` | core | bool：快进中有事件要打断 | — |
| `TownLife.has_map_point(town_id, kind_key)` | town-life | bool | `Town.has_map_point`、`fn_140002806(t, 0/4/8)` |
| `Fleets.post_holder(fleet_id, post_key)` | fleets（D2） | person_id / null | `Fleets.person_in_post` / `person_in_post`（`04-economy/03`）、`Characters.has_post`、`Characters.postAbility`、`Characters.occupants` / `aboard`（投影可留） |
| `Fleets.sailing_snapshot(fleet_id)` / `Fleets.battle_snapshot(fleet_id)` | fleets（数据来自 ships） | 快照 | `SailingSnapshot`、`NavSnapshot` |
| `Fleets.location(fleet_id)` | fleets | `{ town_id \| x, y }` | `Fleets.town`、`Fleets.current_town`、`Ocean.position` |
| `Fleets.fatigue(fleet_id)` | fleets | 0–100 | `Characters.crew_fatigue` |
| `Ocean.happening_state(fleet_id)` | ocean | outcome key / ready | `current_happening` |
| `TownLife.map_points(town_id)` | town-life | `[map_point_kind]` | `TownLife.facilities`、`Town.map_points`（`01-world/10`）、`World.facilities` |
| `Items.owned(fleet_id, item_id)` | items（`09-items/01`） | bool | `Characters.has_item`（`10-narrative/*`） |
| `Progression.total_level(person_id)` | progression | int | — |
| `Profile.cleared(scenario_id)` | save / profile（订阅 `ScenarioCleared` 写通关位） | bool | `Gallery.cleared`（画廊只读本查询，不自写通关位） |
| narrative 任意 `query_*` / ui 任意投影 | 全部 | 只读，禁止写 | — |

## 命令（写状态）

命令发给状态所有者，由所有者校验并在成功后发事件。

| 规范名 | 接收方 | 谁可以发 | 别名 |
| --- | --- | --- | --- |
| `Clock.advance_days(n)` | core | 引擎；旅馆、修理、交易提交等「快进」经引擎；`add_days_mode: simulate` 时逐拍跑完整调度，可被 `Clock.interrupt_pending()` 打断 | `Calendar.add_days`、`Calendar.add_ticks`、`Calendar.advance`、`World.Calendar.advance`、`Ocean.advance_calendar_step`（改为 Clock 驱动） |
| `Actions.run(action_key, ctx) -> result` | core（D9） | town-life（旅馆、仪式、寻求支持）、ocean（happening）、narrative | 各设施专用命令（`RestAtInn`、`JoinRite`、`SeekSupport`）是带参数的 `Actions.run` |
| `Session.end_scenario(outcome, cause)` | core | narrative、progression（Game Over 判定）（D8） | `GameOver` / `BeginEnding`、opcode `0xC5`、execute `0x3FFF` |
| `Minigames.run(id) -> int` | town-life | narrative、ocean（老鼠） | `LaunchMinigame`、`MinigameStart` |
| `Towns.apply_develop(town_id, track, delta, reason)` | world | economy、combat、narrative | `Develop.request`、`Develop.apply`、`Town.apply_develop` |
| `Towns.mark_known(town_id, force_id)` | world | narrative、economy（开局揭示）、ocean | `World.mark_town_known` |
| `World.mark_village_discovered(village_id, via, fleet_id?, item_id?)` / `World.mark_remains_discovered(remains_id, via, town_id?, fleet_id?, item_id?)`（形状以 `01-world/03` §8 为准） | world（置位后发 `VillageDiscovered` / `RemainsDiscovered`） | world 自己（村落半径发现 `fn_140001186`；港内遗址点 kind 9/10/11 首访在 `on_enter_town_map_point` 内，town-life 不调）+ items 命令（`via: item`，带 `item_id`）；town-life 委托完成奖励村落（`08-town-life/02`，原版直接改发现位）同走本命令。ocean 不调 | `World.mark_discovered`、`World.mark_remain_discovered`；items 事件 `UnlockRemain` / `UnlockVillage`（`09-items/01`、`02`）作废，改发本命令 |
| `Force.add_gold(force_id, delta, reason)` | factions | 全部扣款 / 入账 | `Gold.debit` / `Gold.credit`（`05-ships-fleet/*`）、`Economy.credit`（`07-combat/06`）、`Wallet.add_gold`（`04-economy/08`）、`Force.setGold`、`GrantGold`、`GoldGained`、`RewardGold` |
| `Share.add(town_id, force_id, delta, reason)` / `Contract.sign / break` | factions | economy、town-life、combat、narrative | `Factions.apply_share`、`Factions.sign_contract`。合约份额只走一条路：`Contract.sign` 由 factions 执行，内部调 `Share.add(…, reason: contract)`；`InvestmentMade` 不带份额增量 |
| `Influence.add(force_id, major_sea_id, delta, reason) -> gain` | factions | economy、world（遗迹首访，`on_enter_town_map_point`） | `Factions.add_influence`；增量先过 `Modifiers.eval("factions.influence_gain", ctx)`；返回「修正后、夹紧前」的加值 |
| `Fashion.set_slot / clear` | factions | economy、narrative | — |
| `Trade.open / buy / commit / cancel` | economy | town-life（交易所） | `CommitDeal`（原版函数） |
| `Invest.pay(town_id, force_id, gold, reason)` | economy | town-life（交易所 / 王宫） | `Economy.invest`、`OpenInvestment`（UI 入口） |
| `Economy.settle_special(town_id, slot, qty, counts_toward_quota)` | economy | economy 内部、NPC 交易 | `Economy.consume_special`、`Specials.consume`。`supply -= qty`；`counts_toward_quota` 为真才 `month_bought += qty`（NPC 无账本时为假，`04-economy/06`） |
| `Ships.set_hold / add_load(fleet_id, item_id, delta)` | ships | economy、combat、narrative | `Economy.add_load`、`AddLoad` |
| `Cabins.assign(ship_id, cabin_id, person_id) / unassign` | ships | town / deck UI | `AssignPost` / `ClearPost`（`03-characters/03`、`04`）、`Ships.assign_cabin` |
| `Crew.hire / set / lose(ship_id, n, reason)` | ships | town-life、combat、ocean | `HireCrewCount`、`Crew.lost` |
| `Supplies.consume / add` | ships | ocean（日耗）、combat（缴获）、town-life（补给） | `Ships.addSupplies`、`Fleets.refill_supplies` |
| `Yard.*`（买卖、改装、换炮、船首像） | ships | town-life（造船厂） | `EquipFigurehead`（`09-items/02`） |
| `Fleets.set_flagship / set_formation / add_fatigue` | fleets | UI、ocean、town-life | `Fleets.addFatigue` |
| `Fleets.create_proxy(gold, …)` | fleets | town-life | `Fleets.found_proxy` |
| `Fleets.destroy(fleet_id, cause)` | fleets（成功后发 `FleetDestroyed`） | combat（`07-combat/01`）、ocean | 原版 `fn_14000a9ed`；是否终局由 progression 订阅 `FleetDestroyed` 后判 |
| `Characters.recruit / dismiss` | characters | town-life（酒馆）、narrative | `PersonRecruited`（作命令时） |
| `Characters.apply_rest / add_hp / set_physical / set_mental` | characters | town-life（旅馆）、combat、ocean、narrative | `RestAtInn`、`applyRest` / `Characters.applyRest`（`03-characters/07`）、`applyHappening` |
| `Progression.grant_exp(person_id, add{track: n}, roll_mode, source)` | progression | 订阅事件后自发；narrative | `GrantExp`、`ExperienceGranted`（`03-characters/01`）、`ExpGranted`、`CrewExpGranted`（全员广播由 progression 展开） |
| `Equip.on / off(person_id, slot, item_id)` | items | UI、narrative | `EquipOn` / `EquipOff`、`PersonEquipSet` |
| `Items.acquire / discard / use` | items | ocean、combat、narrative、town-life | `AcquireItem`、`GrantItem`（`06-ocean/05`）、`Items.give` |
| `PlantSpecialProduct { fleet_id, town_id, slot, item_id }` | economy（特产品槽所有者） | items（`AcceptPlant` 后）、narrative | 成功后 economy 发 `SpecialPlanted`（原样带回 `fleet_id`、`item_id`，items 据此交出发现物，不必自持待决记录） |
| `Ocean.set_course / plot_route / search` | ocean | ui、narrative | — |
| `Combat.start(BattleStart{…})` | combat | combat 自己（收到 `FleetSpotted` 后）、narrative | `Combat.begin` |
| `Flags.set(flag_id, bool)` | narrative | narrative 脚本 | — |
| `Save.register_section(name, version, export, import, migrations)` | save | 每个状态系统、模组 | — |
| `Profile.set_cleared(scenario_id)` | save / profile | 仅 profile 自己（订阅 `ScenarioCleared`）与原版导入器（`12-ui-save/02` §8） | 画廊 / UI 不得调用 |

叙事可对所有接收方发命令。其它系统不得按 `scenario_id` 分支（D5；profile / ui 只把它当 key 记录或显示）。不存在 `Narrative.fire(trigger)`：叙事订阅事件自己判触发。

## 事件

| 规范名 | 发出 | 典型订阅 | 别名（出现处） |
| --- | --- | --- | --- |
| `DayStarted` / `MonthStarted` / `MonthEnding` / `YearStarted` | core Clock（D4） | 按 `tick.yaml` 阶段派发 | `DayAdvanced`（`01-world/07`、`10-narrative/*`）；`MonthTicked`（`02-factions/01`、`03`、`05`）、`Ocean.MonthTick`（`03-characters/06`）→ `MonthStarted`；`MonthEnded`（`01-world/07`）→ `MonthEnding`；`HalfYearStart` 废弃（订阅 `MonthStarted` 按月过滤）；`CalendarStep` 作调度门时 = `schedule: tick`（UI 可用 `TickAdvanced`）；叙事 kind 5 / 6 的事件源是 `TownTick` / `SeaTick`，不是 `CalendarStep`；`DailyTick` 是 `tick.yaml` 的 `daily` 阶段（不限海上：在港分支只恢复疲劳；海上分支 补给 → 疲劳 → 人物 HP / 身心 → 海上事件掷骰 → 精神转移 → 持续态），不是事件 |
| `ScenarioSelected { scenario_id, world_pack, event_pack, protagonist_id }` | narrative | 存档会话、世界加载、事件解释器、修正器注入 | `ScenarioStarted`、`StartScenario`（`03-characters/02`，命令形）、`NewGameSeeded`；载荷旧写 `{ scenario, protagonist }` |
| `ScenarioCleared { scenario_id }` / `ScenarioFailed { scenario_id, cause, end_code? }` | core Session | profile（写通关位）、ui；画廊只读 `Profile.cleared` | `GameOver`（`11-progression/03`、`03-characters/07`）→ `ScenarioFailed`；`GameStateChanged` 只给 ui。`cause` 是语义枚举；`end_code` 只供原版导入（流程态子码）。载荷旧写 `{ scenario }` |
| `WorldPackLoaded { world_pack, schema_version }` | world | 存档会话 | — |
| `ArrivedAtTown { fleet_id, town_id }` | ocean | town-life、economy、narrative（kind 4）、NPC 交易 | `TownEntered`（`08-town-life/01`）、`FleetArrived`（`05-ships-fleet/02`） |
| `FacilityEntered { town_id, map_point_kind }` | town-life | narrative（kind 7）、world（kind 9/10/11 遗址点：`on_enter_town_map_point` 内判首访 / 重访）、ui | `FacilityOpened`（`08-town-life/01`）、`EnterFacility`（命令形）、`MapPointClicked`（ui 输入） |
| `TownTick { town_id }` / `SeaTick { fleet_id }` | town-life 场景 / ocean 日推进 | narrative（kind 5 / 6） | — |
| `FleetSpotted { fleet_id, other_fleet_id, major_sea_id, sea_area_id?, aggressive }` | ocean | combat、ui、narrative（kind 3） | `FleetContacted`（`10-narrative/*`）。`major_sea_id` 必填；`sea_area_id` 可选、默认 `null`（原版无细分区，模组可用） |
| `DiscoveryFound { fleet_id, kind, id }` | ocean | world、items、narrative（kind 0 = 道具） | `ExplorationFound`（`10-narrative/*`）、`SearchHit`（`09-items/04`） |
| `VillageLanded { fleet_id, village_id }` | ocean | narrative（kind 8） | — |
| `VillageDiscovered` / `RemainsDiscovered { …, via, item_id? }` | world（**唯一**；首访在 `on_enter_town_map_point` 内，或收到 `World.mark_*_discovered` 命令后） | items、progression（图鉴）、ui | `RemainDiscovered`（`01-world/11`）；items 发的 `UnlockRemain` / `UnlockVillage` 作废 |
| `TownBecameKnown { town_id, force_id }` | world | routes、economy、ui | `TownKnown`（`04-economy/08`） |
| `RemainsRevisited { remains_id, town_id, kind }` | world（`01-world/11`，`on_enter_town_map_point` 内） | town-life（按日仪式） | — |
| `TownDeveloped { town_id, track, old, new, reason }` | world `Towns`（**唯一**） | ships（上架、炮种）、town-life（地图点解锁）、ui | 原 economy / factions 各自发出的写法作废 |
| `SuppliesDepleted { fleet_id, kind }` / `StormHit { fleet_id, major_sea_id, sea_area_id?, kind }` | ocean | characters、ships、ui | 载荷规则同 `FleetSpotted` |
| `EnteredMajorSea { fleet_id, major_sea_id }` / `EnteredSeaArea { fleet_id, sea_area_id }` | ocean | ui（BGM，`01-world/08`）、narrative | `EnteredSeaZone`；`EnteredSeaArea` 只在内容包有 `sea_areas` 时发 |
| `FatigueChanged { fleet_id, old, new }` | fleets | ocean、characters、ui | ocean 原文由 ocean 发，改为 fleets 发 |
| `HappeningRolled { fleet_id, happening_id, outcome }` | ocean | characters（日结）、ui | `happening_id` = 槽 key（`ocean/happenings.yaml` 的槽，不是结局）；`outcome` = 结局 key（`06-ocean/05` §8） |
| `HappeningPersist { fleet_id, outcome }` | ocean（`daily` 海上分支末步 `ocean.happening_persist`，原版 `fn_1400f0450`） | ships（`Supplies.consume` 扣当日分量）、characters（疾病持续）、ui | ocean 自己不 `supply -=`（`06-ocean/05` §4.1） |
| `CrewAttrition { fleet_id, keep_pct }` | ocean（疲劳 `>= 100`，`06-ocean/01` §8） | ships（按比例减水手，经 `Crew.lose`）、ui | — |
| `GoldRoadProgress` / `GoldRoadComplete` | ocean | narrative、ui | — |
| `TradeCommitted { fleet_id, town_id, bought[], sold[], gold_net, bargain, purser_id }` | economy | ships、progression（经验）、narrative | `CargoCommitted`（本篇旧版、`04-economy/01`、`03-decoupling` 旧版）；`CommitDeal` 是原版函数名 |
| `SoldFashionGoods { town_id, force_id, items[], qtys[], hold_sum, bargain_ok }` | economy | factions（份额、发信、影响力） | `DealCommitted`（`02-factions/03`、`04`）；NPC 版 `NpcFashionSold` 不发信，单列 |
| `SupplyChanged` / `SpecialRestocked` / `SpecialUnlocked` | economy | ui、ships | — |
| `TownStateChanged { town_id, old, new, flag }` | economy（景气状态 `+0x74`；`flag` = `+0x75` 写入值，即状态已持续月数的起值 1 / 复位 0，`04-economy/02` §2.3） | ships（买卖船、改装价）、ui | — |
| `TradeOpened { session_id, fleet_id, town_id }` | economy | items（种植询问，`09-items/03`） | — |
| `NpcTradeCommitted { fleet_id, admiral_id, town_id, sold_gold, buy_cost }` / `NpcPurchaseFailed { town_id, force_id, item_id, major_sea_id }` | economy（`04-economy/06`） | progression（提督经验）/ factions（`SeaRecord`） | — |
| `NpcFashionSold { town_id, force_id, qtys[2], hold_capacity }` | economy | factions（NPC 份额规则，不发信） | — |
| `InvestmentMade { town_id, force_id, gold, tracks[] }` | economy | factions、ui | `InvestmentSettled`（`02-factions/03`）、`InvestmentAudience`（`08-town-life/05`，UI）；旧载荷的 `share_gain` 删除（合约份额只经 `Contract.sign`） |
| `ExclusiveTributePaid { force_id, gold, town_ids[] }` | economy | factions、ui | `ExclusiveTribute`（`04-economy/07`）、`TributeDue` / `TributeCollected`（`02-factions/03`） |
| `SuzerainTributePaid { from_force, to_force, gold }` | factions（`forces` 阶段执行；金额式由 economy 提供） | economy、ui | `SuzerainTributeDue`（`04-economy/07`）、`OverlordTributePaid`（`02-factions/03`） |
| `BrandRecognized { force_id, town_id, item_id }` / `StarItemChanged` | economy | factions、ui | — |
| `KindRateChanged` / `CulturePriceChanged` | economy | ui、NPC 交易 | `CulturePriceChanged` 曾写成 factions 发（`02-factions/02`、`06`）→ economy 发 |
| `ForceGoldChanged { force_id, old, new, reason }` | factions | ui | `LedgerCredited` / `LedgerDebited` 是账本事件，另列 |
| `ShareChanged` / `ContractSigned` / `ContractBroken` | factions | economy、narrative、town-life | — |
| `InfluenceChanged` / `AmityChanged` / `WarDeclared` / `AllianceFormed` / `ForceCollapsed` / `ForceAbsorbed` | factions | narrative、ui | — |
| `FashionChanged` | factions | economy、town-life、ui | — |
| `BattleStarted` / `BattleResolved { …, town_occupied, captured_ships, gold, cargo }` | combat | economy、ships、factions、progression、narrative（kind 1 / 2） | 载荷以 `07-combat/06` 展开为准 |
| `TownOccupied { battle_id, town_id, from_force_id, to_force_id }` | combat | factions | `TownCaptured`（`02-factions/01`、`03`、`05`、`07-combat/04`） |
| `TownBombarded` | combat | ui（发展度变化另由 `TownDeveloped` 给出） | — |
| `PersonRecruited` / `PersonDismissed` / `RosterChanged` | characters | narrative、ui | — |
| `SurgeonWorked { person_id, exp: [e0, e1] }` | characters（`03-characters/07` §2.11） | progression（船医经验） | — |
| `VitalsChanged` / `PersonDied { person_id, is_protagonist, cause }` | characters | progression、ui、narrative | `ProtagonistDied`、`OfficerDied`（`03-characters/07`）、`PersonPhysicalDied`、`NotifyPhysicalDied`（`11-progression/03`） |
| `LevelChanged` / `AbilityGrown` / `SkillChanged` | progression | characters（缓存失效）、ui | `LevelUp` |
| `PersonEquipped` / `PersonUnequipped` | items | core `Modifiers`（缓存失效）、ui | `PersonEquipSet`；characters 不订阅，只调 `Modifiers.eval` |
| `CabinBound` / `CabinUnbound { ship_id, cabin_id, person_id, post_key }` | ships | characters、fleets | `PostAssigned` / `PostCleared`（`03-characters/03`、`04`）、`CabinChanged` |
| `CabinKindChanged` / `AddPartChanged` / `MastChanged` / `FigureheadChanged` | ships | fleets、ocean（派生量失效） | `FigureheadChanged` 曾写成 items 发（`09-items/01`、`02`）→ ships 发 |
| `CrewChanged { ship_id, crew, reason }` / `DurabilityChanged` | ships | combat、ocean、ui | `CrewLost`（`06-ocean/02`、`07-combat/02`）作 `reason` 区分；海战内 `ShipHullChanged` / `ShipCrewChanged` 只给 UI |
| `CrewWiped { ship_instance_id }` | ships（`05-ships-fleet/06`） | combat（沉 / 俘）、progression | `CrewEmpty`；不是 `FleetDestroyed` |
| `ShipBought` / `ShipSold` / `ShipDocked` / `ShipCreated` / `ShipScrapped` | ships | fleets、ui | — |
| `FlagshipChanged { fleet_id, ship_id }` | fleets | ships（清舱室任职） | — |
| `FleetDestroyed { fleet_id, cause }` | fleets（combat / ocean 判定后发命令 `Fleets.destroy(fleet_id, cause)`） | progression（玩家本队 → `Session.end_scenario(game_over)`）、factions、narrative | `NotifyFleetLost`（`11-progression/03`）、`FleetEliminated`、`FleetDisbanded`、`FleetWiped`；原版 `fn_14000a9ed` |
| `ItemAcquired` | items | economy、narrative | — |
| `ItemFound { item_id, presentation }` / `ProofDiscovered { … }` | items | progression（全员经验）、ui | `CrewExpGranted`、`ExpGranted` 不作事件（经验由 progression 订阅后发 `Progression.grant_exp`） |
| `ItemBargained { item, person, base_price, price }` | items | progression（经验 `grant_routes`） | — |
| `PlantOffered { town_id, slot, item_id, trading_item_id }` | items | ui（询问） | 回应后 items 发命令 `PlantSpecialProduct` |
| `SpecialPlanted { town_id, slot, trading_item, fleet_id, item_id }` | economy（**唯一**，命令 `PlantSpecialProduct` 成功后） | items（按 `fleet_id` + `item_id` 交出发现物）、ui | 原 items 发出的写法作废；旧载荷 `{ …, trading_item_id, plant_item_id }`（无 `fleet_id`） |
| `DiscoveryUnlocked { kind, id }` | progression | profile（图鉴）、ui | — |
| `MinigameStarted` / `MinigameWon` / `MinigameLost` | town-life | narrative、ocean | — |
| `RequestAccepted` / `RequestCompleted` / `RequestAbandoned` | town-life | economy、factions、items | — |
| `SchemeQueued` / `SchemeResolved` / `SchemeCleared` | town-life | factions、fleets、economy | — |
| `SaveWriting` | save（写档前） | profile 各节（图鉴同步，原版 `fn_14000a321`） | — |
| `SaveWritten` / `SaveLoaded` / `SaveRejected` / `SaveMigrated` | save | ui | — |

## 叙事触发源映射（D8）

原版 `CEvent` 触发 kind 0–8 由 `fn_14001337c` 的 8 个调用点钉死 `[确认]`（`10-narrative/03`）。重构里 kind 只是导入编码，触发器写事件名。

| 原版 kind | 含义 | 规范事件 |
| --- | --- | --- |
| 0 | 探索发现道具 | `DiscoveryFound { kind: item }` |
| 1 | 海战开始 | `BattleStarted` |
| 2 | 海战结束 | `BattleResolved` |
| 3 | 舰队接触 | `FleetSpotted` |
| 4 | 入港 | `ArrivedAtTown` |
| 5 | 港内 | `TownTick` |
| 6 | 航海（日推进） | `SeaTick` |
| 7 | 进设施（设施 `0xD` 街区按 5） | `FacilityEntered` |
| 8 | 登陆村落 | `VillageLanded` |

## 修正器（D2）

`Modifiers.eval(key, ctx)` 是人物、船、剧本对外唯一的数值口。key 在 `config/modifiers.yaml` 注册，贡献由提供方内容表登记。阶段序 `base → add → mul → clamp → override`。

key 命名：`<域>.<量>`，完整规范表（类型、stage、提供方、owner_system）见 `10-key-glossary.md` §6.2。原版包至少需要：

| key | 原版来源 | 消费方 |
| --- | --- | --- |
| `skill.<skill_key>`（10 个） | `fn_1400135cf` 链：基值 + 装备 → 身体态系数 → 封 500 | 全部 |
| `trade.npc_buy_cost` | NPC 买货 `1000 - a`（a = 提督会计术技能值，F-ECO-01） | economy |
| `trade.sell_price_pct` | 剧本 2 卖价 ×105/100 | economy |
| `factions.influence_gain`（`ctx.source`） | 剧本 2 卖流行品 ×2（`fashion_sale`）；剧本 0 遗迹首访 ×2（`remains`） | factions（`Influence.add` 之前） |
| `invest.military_min_fee_pct` | 剧本 0 八折 | economy |
| `request.reward_gold` | 剧本 0 ×3/2（无符号截断） | town-life |
| `fleet.speed` / `fleet.speed_move` | 船首像 0 `+1/10`、7 `×4/5`；海战 `/3` 只进 `speed_move`（写速度条之后） | ocean |
| `supply.food_div` / `fleet.search_radius_add` | 船首像 1 / 5（搜索半径派生量是 `Ocean.search_radius`） | ocean |
| `combat.melee_damage` | 船首像 8 `×6/5` | combat |

会计长杀价（`fn_1400137b4`，F-ECO-15）不是修正器：economy 公式取 `Fleets.post_holder(fleet, purser)` 的 `skill.accounting` 自己掷骰。

经济、战斗、航海、道具**不读六维原值**，也不自己套技能公式。

## 原版取值口备注

`[确认]` `fn_14000e1ce(person, k)` 的 `k` 是 **eSkill**：`fn_14000e1ce`（body `0x1401c9f60`）→ `fn_1400135cf`（`0x1401ca250`）→ `fn_14001406a` → `fn_140009214`（`0x1401c9d50`）用 `fn_1400099e9(k, 0/1)`（`0x1401ca010`）查 10×2 配对表 `0x1406864a0` 并断言 `0 <= k < 10`；`fn_1401cb7f0.cpp` 调用处 RTC 局部名 `eSkill`。`k = 4` 的配对是 (3, 4) = 智力 + 精神 = 会计术。NPC 买货 `1000 - a` 的 `a` 是提督的**会计术技能值**（含装备、身体态系数，封 500）；`fn_14000660e` 的 `500 - a` 用 `k = 1` 统率力；道具店还价用 `k = 4`。第三参是舱室种类（`0x18` = 无）：只对玩家势力人物按所在舱室查装备加成，否则传 `0x18`。

## 内容包选择

原版 `fn_14000ff60`（body `0x1402a89a0`）写死 `scenario_id >= 4` → World1，否则 World0。重构由剧本记录的 `world_pack` 字段给出，世界加载器只认包名，不比较剧本号（D5）。
