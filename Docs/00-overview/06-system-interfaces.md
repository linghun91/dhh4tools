# 跨系统接口

系统之间只传 **ID、快照、事件**。禁止持有对方对象指针。本表是合同；细节在各系统文档。

## 查询（只读）

| 调用方 | 接口 | 提供方 | 返回 |
| --- | --- | --- | --- |
| 任意 | `ids.resolve(key) -> id` | ids | 稳定整数 |
| economy | `World.town(town_id)` 坐标/region | world | 定义快照 |
| economy | `Ships.holds(fleet_id)` | ships | 货格数组 |
| economy | `Characters.modifier_set(person_id)` | characters | `buy_cost_factor` 等 |
| economy | `Factions.share(town_id, force_id)` | factions | 0–100 |
| factions | `Economy.is_brand(town_id, slot)` | economy | bool |
| ocean | `World.wind_at(x,y,date)` | world/climate | 风向风力 |
| ocean | `World.town_points()` | world | 入港判定 |
| ocean | `Fleets.sailing_snapshot(fleet_id)` | fleets/ships | 帆、耐久、补给、岗位 |
| ocean | `Characters.modifier_set(navigator)` | characters | 航海修正 |
| combat | `Fleets.battle_snapshot(fleet_id)` | fleets/ships | 船、炮、水手、耐久 |
| combat | `Characters.modifier_set(captain)` | characters | 先手/近战 |
| town | `World.facilities(town_id)` | world | 入口列表 |
| narrative | 任意 `query_*` | 全部 | 只读 |
| ui | 任意投影 | 全部 | 只读，禁止写 |

## 命令（写状态）

| 命令 | 接收方 | 谁可以发 |
| --- | --- | --- |
| `Trade.open/commit/cancel` | economy | town（交易所入口） |
| `Economy.consume_special / invest` | economy | trade / town |
| `Ships.assign_cabin(ship, cabin, person, post)` | ships | town / deck UI 经 town |
| `Fleets.found_proxy(gold)` | fleets | town |
| `Characters.recruit / assign_roster` | characters | town（酒馆） |
| `Factions.sign_contract / apply_share` | factions | palace / narrative |
| `Ocean.set_course / start_search` | ocean | ui / narrative |
| `Combat.begin(attacker, defender)` | combat | ocean（遭遇） |
| `Narrative.fire(trigger)` | narrative | 任意系统在状态变化后 |

叙事可对所有接收方发命令。其它系统不得读 `scenario_id`。

## 事件

| 事件 | 发出 | 典型订阅 |
| --- | --- | --- |
| `ArrivedAtTown { fleet_id, town_id }` | ocean | town, economy, narrative |
| `SuppliesDepleted` / `StormHit` | ocean | characters, ships, ui |
| `FleetSpotted { other_fleet_id }` | ocean | combat, ui |
| `DiscoveryFound { village_id \| remains_id }` | ocean | world, items, narrative |
| `GoldRoadProgress` | ocean | narrative, ui |
| `CargoCommitted { buys, sells, gold_delta }` | economy | factions（流行/份额）, ships |
| `SupplyChanged { town_id, slot, item_id }` | economy | ui |
| `InvestmentMade { town_id, force_id, gold }` | economy | factions |
| `BrandRecognized { force_id, item_id }` | economy | factions, ui |
| `ShareChanged { town_id, force_id, share }` | factions | economy, narrative |
| `ContractSigned { town_id, force_id }` | factions | narrative, town |
| `TownDeveloped { town_id, commerce, military }` | factions | ships（炮种解锁） |
| `BattleResolved { loot, captured_ships, town_occupied }` | combat | economy, ships, factions, narrative |
| `PersonRecruited { person_id }` | characters | narrative, ui |
| `CabinChanged { ship_id, cabin_id, person_id }` | ships | characters |
| `FlagshipChanged { fleet_id }` | fleets | ships（清舱室任职） |
| `ScenarioStarted { scenario_id, world_pack }` | narrative | world 加载，其它只收 pack 名 |

## ModifierSet（人物对外唯一数值口）

人物系统计算技能与装备后输出因子。经济、战斗、航海、道具**不读六维原值**。

```
ModifierSet {
  buy_cost_factor        # NPC 自动采购
  sell_price_factor      # 玩家杀价；原版会计长文案，公式未钉
  item_price_factor      # 道具买卖
  battle_initiative      # 原版：旗舰舰长敏捷
  battle_melee_factor    # 原版 500-a 一类
  sailing_speed_factor   # 占位；航速主公式在 ocean
  survey_ok              # 有测量员才能建航线
  repair_discount        # 船木工
  cannon_interval_factor # 炮手
  ...
}
```

原版 `fn_14000e1ce(person, 4)` 用于买货折扣。下标 4 是 **eAbility 精神** 还是 **eSkill 会计术**，人物文档与公式总表结论不一致。实现前必须在 `fn_14000e1ce` / `fn_1400135cf` 上钉死，见 [07-open-unknowns.md](07-open-unknowns.md)。

## 内容包选择

`fn_14000ff60`：`scenario_id >= 4` → World1，否则 World0。叙事表写出 `world_pack`，世界加载器只认包名。
