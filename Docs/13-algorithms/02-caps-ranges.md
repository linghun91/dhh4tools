# 上限与值域

`caps.yaml` 是设计上限，必须 ≤ `engine_limits.yaml` 硬顶。`NUM_*` 是原版**内容包实例计数**，不是引擎循环上界：运行时读表长。硬编码 `NUM_TOWN=97` 视为 bug。

列含义：

| 列 | 含义 |
| --- | --- |
| vanilla | 原版内容包 / 原版 exe 立即数。未还原写 `[未知]` |
| engine_limit | 实现硬顶（整数宽度或已证实溢出点） |
| recommended | 重构默认。玩法身份数字跟 vanilla；溢出与循环跟引擎表长；bug 夹紧独立成参 |

`recommended` 不是「把等级改成 255」。原版手感走 `packs/vanilla-world0`。引擎必须能承受更大值且饱和，而不是绕回。

来源：`srcdata/catalog/constants.json`、`num_values.json`、`srcdata/source/Constants.h`、`srcdata/source/*ID.h`、`srcdata/docs/exe_caps.md`、`srcdata/docs/save_format.md`、`srcdata/docs/trade_stack.md`。

---

## 职责边界

管：一切 MIN/MAX/NUM/哨兵、存档槽宽、补丁硬顶。

不管：公式形状（见 [01-formulas.md](01-formulas.md)）；港口/人物/商品行。

## 溢出总则

| 宽度 | 硬顶 | 重构 |
| --- | --- | --- |
| `UCHAR` | 255 | 饱和 |
| `USHORT` | 65535 | 饱和 |
| `uint32` / `int32` | 2³²−1 / `INT_MAX` | 金币与经验用 64 位中间值再饱和 |
| 32 位 `isqrt` | 单段等级 255，再高变 0 | 饱和，禁止绕回 |

`AccessMacro.h`：`_GetDataSizeMax_BYTE2DWORD(1/2/4) = 0xFF / 0xFFFF / 0xFFFFFFFF`。

---

## 1. 玩法上限（写入 `caps.yaml`）

| key | vanilla | engine_limit | recommended | 源 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `level_per_track` | 100 | 255 | 100 | `Constants.h` `VANILLA_TRACK_LEVEL` / `HARD_TRACK_LEVEL`；`exe_caps.md` | `level=isqrt(exp/30)`。再高 32 位开方溢出成 0 |
| `track_count` | 2 | 实现自定 | 2 | 存档两段经验 | 总等级 = 两段之和 |
| `total_level_max` | 200 | 510 | 200 | 100×2 / 255×2 | 派生：`level_per_track * track_count` |
| `exp_divisor` | 30 | （参数，非宽顶） | 30 | `save_format.md` | 公式参数，也进 `formulas/progression.yaml` |
| `exp_per_track_at_cap` | 300000 | `30*255*255=1_950_750` | 由公式重算 | `0x493E0`；`fn_140006c67` | 禁止写死 300000 |
| `ability_growth_cap` | 100 | 255 | 100 | `exe_caps.md` 基础属性成长 | 单字节。对应 `ABILITY_AMOUNT_*` 之一 |
| `ability_settled_cap` | 500 | 65535 | 500 | `exe_caps.md`；`fn_1401cbb40` `0x1F4` | 结算后属性/技能夹紧 |
| `buy_ability_clamp` | 500 | 同 `ability_settled_cap` 宽 | 500 | `exe_caps.md` 买货补丁 | **独立键**。提高结算上限时此项可仍为 500 |
| `battle_ability_clamp` | 500 | 同上 | 500 | `500-a` | 独立键 |
| `skill_cap` | 200 | 510（两段之和） | 200 | `VANILLA_SKILL_CAP`；技能公式两处立即数 | `min(total_level, skill_cap)` |
| `town_develop` | 9999 | 65535 uint16 | 9999 | `VANILLA_PORT_CAP`；港 `+0x1e` `movzx` | 商业/军事。制海 9999 是另一立即数，未改 |
| `cargo_stack` | 1 | 255（STK3 件数 `uint8`） | 1 | `VANILLA_STACK` | 原版不堆叠。UI `%6d` 不是规则 |
| `display_stack_max` | 9 | UI 自理 | （不进 caps） | `DISPLAY_STACK_MAX`；`trade_stack.md` | 显示层；禁止倒逼 `cargo_stack` |
| `fleet_ships` | 5 | 表长 | 5 | `NUM_FLEETSHIP` | 玩家船位 |
| `hold_slots_per_ship` | 5 | 表长 | 5 | `NUM_SHIPLOAD` / `NUM_SHIPLOAD_EXCH` | 交易货格 |
| `hold_scan` | 10 | 表长 | 10 | `NUM_HOLD_SCAN`；`CountHolds(0x0A)` | 扫描格 ≥ 货格 |
| `sailor_roster` | 98 | 存档 schema | 98 | `SAVE_SAILOR_COUNT` | 名册槽，≠ `NUM_PERSON` |
| `stat_count` | 6 | 表长 | 6 | `SAVE_STAT_COUNT` / `NUM_ABILITY` | 六维 |
| `max_new_slots` | 25 | `fleet_ships*hold_slots` 或 `25*stack` | 由船×格推导 | `AddLoad` `0x19` | 禁止写死 25 |
| `share_high` | 80 | `SHARE_MAX` 一旦还原 | 80 | `trade_stack.md` `0x50` | 流行品信件分支 |
| `fashion_div` | 6 | — | 6 | `mov ecx,6` | `floor(x/6)+1` |
| `fashion_threshold_factor` | 1 | — | 1 | 工具 `fthr` | `(slots*fthr+1)>>1` |
| `letter_lo` | 10 | — | 10 | `fl1` | 介绍信 |
| `letter_hi` | 16 | — | 16 | `fl2` | 推荐信 |
| `same_item_adjust_threshold` | 15 | — | 15 | `CommitDeal` | 同品调市价，式子 `[未知]` |
| `local_fleet_founding_gold` | 100000 | 金币宽 | 100000 | mesfile | 文案常数 |
| `ref_1000` | 1000 | — | 1000 | 买货 | 见公式表 |
| `ref_500` | 500 | — | 500 | 战斗 | |
| `ref_100` | 100 | — | 100 | 道具价/份额封顶 | |
| `rand_span` | 100 | — | 100 | `rand(100)` | |
| `ability_div` | 50 | — | 50 | `a/50` | |
| `level_bias` | 100 | — | 100 | 技能 `+100` | |
| `skill_divisor` | 150 | — | 150 | 技能 `/150` | |

`FATIGUE_*`、`FORCEAMITY_*`、`FORCE_INFLUENCE_*`、`SHARE_*`、`MARKET_PRICE_*`、`SHIP_CREW_*`、`SHIP_DURABILITY_*`、`WIND_*_ADJUST_*` 见第 4 节（符号有、整数无）。

---

## 2. `NUM_*` 已还原整数

来源优先级：`num_values.json` RTC cmp → `*ID.h` 手写 → `Constants.h`。`constants.json` 的 `NUM_` 列表是断言符号全集。

运行时 **一律读表长**。vanilla 列只描述原版包。recommended 列写「表长」表示禁止再写死该数字。

### 2.1 世界 / 叙事

| key | vanilla | engine_limit | recommended | 源 |
| --- | --- | --- | --- | --- |
| `NUM_TOWN` | 97 | 表长 | 表长 | `TownID.h`；`num_values.json`；GetFixTownProc |
| `NUM_TOWN2` | 100 | 表长 | 表长 | `num_values.json` only。`[推断]` 港口数组垫到 100 |
| `NUM_TOWNSHARE` | 97 | 表长 | 表长 | `TownShareID.h`；与港 1:1 |
| `NUM_TOWNFASHION` | 26 | 表长 | 表长 | `TownFashionID.h` |
| `NUM_VILLAGE` | 24 | 表长 | 表长 | `VillageID.h` |
| `NUM_SEAAREA` | 25 | 表长 | 表长 | `SeaAreaID.h` |
| `NUM_NEXTBASETOWN` | 25 | 表长 | 表长 | `num_values.json`。与海域同数 `[推断]` |
| `NUM_ROUTE` | 200 | 表长 | 表长 | `RouteID.h` |
| `NUM_MAPPOINTKIND` | 12 | 表长 | 表长 | `MapPointID.h`；`num_values.json` |
| `NUM_SCENARIO` | 7 | 表长 | 表长 | `ScenarioID.h` |
| `NUM_REQUEST` | 69 | 表长 | 表长 | `RequestID.h` |
| `NUM_SCHEME` | 24 | 表长 | 表长 | `SchemeID.h` |

### 2.2 人物 / 势力

| key | vanilla | engine_limit | recommended | 源 |
| --- | --- | --- | --- | --- |
| `NUM_PERSON` | 235 | 表长 | 表长 | `PersonID.h`；GetFixPersonProc |
| `NUM_ABILITY` | 6 | 表长 | 6 | `AbilityID.h`（`constants.json` 有符号；**不在** `num_values.json`） |
| `NUM_SKILL` | 10 | 表长 | 表长 | `SkillID.h` |
| `NUM_PERSONEQUIP` | 5 | 表长 | 表长 | `PersonEquipID.h` |
| `NUM_EQUIP` | 5 | 表长 | 表长 | `num_values.json`。与装备槽同数 `[推断]` |
| `NUM_FORCE` | 25 | 表长 | 表长 | `ForceID.h`。0–23 具名，24 私家舰队 |
| `NUM_FORCECOLOR` | 8 | 表长 | 表长 | `ForceColorID.h` |
| `NUM_THISFORCE` | 4 | 表长 | 表长 | `num_values.json` |
| `NUM_NOWPERSON` | 4 | 表长 | 表长 | `num_values.json` |

### 2.3 经济 / 道具

| key | vanilla | engine_limit | recommended | 源 |
| --- | --- | --- | --- | --- |
| `NUM_TRADINGITEM` | 105 | 表长 | 表长 | `TradingItemID.h`。合法 id 0–104 |
| `NUM_TRADINGITEMKIND` | 19 | 表长 | 表长 | `TradingItemKindID.h` |
| `NUM_SPECIALPRODUCT` | 5 | 表长 | 5/港 | `SpecialProductID.h`。**每港** 5 槽 |
| `NUM_SPECIALPRODUCTCNT` | 5 | 表长 | 表长 | `num_values.json` |
| `NUM_ITEM` | 231 | 表长 | 表长 | `ItemID.h`。装备/发现物，不是交易品 |
| `ITEM_EMPTY` | 106 | 哨兵 | 106（原版包） | `Constants.h`；`num_values.json`。**不是** `NUM_*`，见第 5 节 |

### 2.4 船 / 舰队 / 战斗

| key | vanilla | engine_limit | recommended | 源 |
| --- | --- | --- | --- | --- |
| `NUM_SHIP` | 100 | 表长 | 表长 | `ShipID.h` |
| `NUM_SHIPTYPE` | 5 | 表长 | 表长 | `ShipTypeID.h` |
| `NUM_SHIPLOAD` | 5 | 表长 | 表长 | `ShipLoadID.h` |
| `NUM_SHIPLOADCARGO` | 5 | 表长 | 表长 | `num_values.json` |
| `NUM_SHIPLOAD_EXCH` | 5 | 表长 | 表长 | `Constants.h`；`num_values.json` |
| `NUM_SHIPCABIN` | 31 | 表长 | 表长 | `ShipCabinID.h` |
| `NUM_CABINSURFACE` | 31 | 表长 | 表长 | `num_values.json`。与舱室同数 `[推断]` |
| `NUM_SHIPMAST` | 100 | 表长 | 表长 | `ShipMastID.h` |
| `NUM_FLEET` | 69 | 表长 | 表长 | `FleetID.h`；GetFixFleetProc |
| `NUM_FLEETSHIP` | 5 | 表长 | 表长 | `FleetShipID.h` |
| `NUM_ATTACKERFLEETSHIP` | 5 | 表长 | 表长 | `num_values.json` |
| `NUM_INVADEFLEETSHIP` | 5 | 表长 | 表长 | `num_values.json` |
| `NUM_TARGETFLEETSHIP` | 5 | 表长 | 表长 | `num_values.json` |
| `NUM_GUNTYPE` | 6 | 表长 | 表长 | `GunTypeID.h` RTC cmp（不在 `num_values.json`） |
| `NUM_BATTLEGUN` | 5 | 表长 | 表长 | `BattleGunID.h` |
| `NUM_BATTLERESULT` | 5 | 表长 | 表长 | `BattleResultID.h` |
| `NUM_SHELL` | 100 | 表长 | 表长 | `ShellID.h` |
| `NUM_MOBILESHIP` | 20 | 表长 | 表长 | `MobileShipID.h` |
| `NUM_MOBILEFLEET` | 4 | 表长 | 表长 | `MobileFleetID.h` |
| `NUM_ADDPARTS` | 9 | 表长 | 表长 | `num_values.json`。`AddPartsID.h` 整数值未写入枚举体 |
| `NUM_DECKADDPARTS` | 4 | 表长 | 表长 | `num_values.json` |

### 2.5 RTC 杂项（在 `num_values.json`，不一定是玩法枚举）

| key | vanilla | engine_limit | recommended | 源 / 备注 |
| --- | --- | --- | --- | --- |
| `NUM_BUTTON` | 4 | UI | 表长 | RTC。可能是控件数，不是 `e*` |
| `NUM_HAPPENING` | 5 | 表长 | 表长 | RTC `eHappening` |
| `NUM_INDEX` | 5 | — | 表长 | RTC `eIndex` |
| `NUM_KIND` | 9 | 表长 | 表长 | RTC。与 `NUM_TRIGGERKIND` 同值 `[推断]` 勿合并直到命名 |
| `NUM_TRIGGERKIND` | 9 | 表长 | 表长 | `num_values.json` |
| `NUM_SELECTITEMTAG` | 6 | 表长 | 表长 | RTC |
| `NUM_SMOKE` | 8 | 特效 | 表长 | RTC `eSmoke` |
| `NUM_HOLD_SCAN` | 10 | 表长 | 表长 | `Constants.h` only，不在 json |

---

## 3. `NUM_*` 仅符号（整数未还原）

`constants.json` → `NUM_` 有名；头文件写 `NUM_X, /* integer not recovered */` 或空枚举体。vanilla / recommended 都是 `[未知]`，engine_limit = 表长。

| symbol | 头文件 | 备注 |
| --- | --- | --- |
| `NUM_ABSORBED` | `AbsorbedID.h` | |
| `NUM_BATTLEALGO` | `BattlealgoID.h` | |
| `NUM_BUSTUP` | `BustupID.h` | |
| `NUM_CABINKIND` | `CabinKindID.h` / `CabinID.h` | |
| `NUM_COUNTRY` | `CountryID.h` | 港 `region` 0–20 共 21 档 `[推断]`，未证 = eCountry |
| `NUM_CULTURE` | `CultureID.h` | NPC 文案 9 个文化圈 `[推断]` |
| `NUM_FACELOCATION` | `FacelocationID.h` | |
| `NUM_FASHIONSPEED` | `FashionspeedID.h` | |
| `NUM_FIGUREHEAD` | `FigureHeadID.h` | |
| `NUM_FLEETAIMKIND` | `FleetaimkindID.h` | |
| `NUM_FLEETCOLOR` | `FleetcolorID.h` | 与 `NUM_FORCECOLOR=8` 是否同一 `[未知]` |
| `NUM_FLEETSTATE` | `FleetstateID.h` | |
| `NUM_FORCEAIMKIND` | `ForceaimkindID.h` | |
| `NUM_FORCEFAILUREKIND` | `ForcefailurekindID.h` | |
| `NUM_FORCEKIND` | `ForcekindID.h` | |
| `NUM_FORCERELATION` | `ForcerelationID.h` | |
| `NUM_ITEMKIND` | `ItemKindID.h` | |
| `NUM_JOBKIND` | `JobkindID.h` | |
| `NUM_JUNCTION` | `JunctionID.h` | |
| `NUM_LICENSELOCATIONCHOICES` | `LicenselocationchoicesID.h` | |
| `NUM_MENTALSTATE` | `MentalstateID.h` | 另有 `MENTALLEVEL_MIN/MAX` |
| `NUM_MESSAGETYPE` | `MessageTypeID.h` | |
| `NUM_MOBILEFLEETKIND` | `MobilefleetkindID.h` | |
| `NUM_MOBILESHIPSTATE` | `MobileshipstateID.h` | |
| `NUM_NORMALSHIPTYPE` | `NormalshiptypeID.h` | 断言 `eShipType < NUM_NORMALSHIPTYPE` |
| `NUM_PERSONKNOWN` | `PersonknownID.h` | |
| `NUM_PHYSICALSTATE` | `PhysicalstateID.h` | 另有 `PHYSICALLEVEL_MIN/MAX` |
| `NUM_PLAYEREVENTSHIP` | `PlayereventshipID.h` | |
| `NUM_POST` | `PostID.h` | 舱室岗位 |
| `NUM_PROXYFLEET` | `ProxyfleetID.h` | |
| `NUM_REMAINS` | `RemainsID.h` | |
| `NUM_REQUESTKIND` | `RequestkindID.h` | |
| `NUM_REQUESTPERSON` | `RequestPersonID.h` | |
| `NUM_ROUTELOCATOR` | `RouteLocatorID.h` | |
| `NUM_SCHEMEKIND` | `SchemekindID.h` | |
| `NUM_SELECTTERGET` | `SelecttergetID.h` | 原拼写 Terget |
| `NUM_SEX` | `SexID.h` | |
| `NUM_SHIPGRADE` | `ShipgradeID.h` | |
| `NUM_SHIPSUPPLY` | `ShipSupplyID.h` | |
| `NUM_SKILLABILITY` | `SkillabilityID.h` | `[推断]` 技能×双属性配对表 |
| `NUM_TOWNACCESS` | `TownaccessID.h` | |
| `NUM_TOWNDEVELOP` | `TownDevelopID.h` | 与 `town_develop` 上限不是同一层 |
| `NUM_TOWNKIND` | `TownKindID.h` | |
| `NUM_TOWNSTATEKIND` | `TownstatekindID.h` | |
| `NUM_VILLAGEBLINDAREA` | `VillageblindareaID.h` | |
| `NUM_WALLSKY` | `WallskyID.h` | |
| `NUM_ZODIAC` | `ZodiacID.h` | |

还原整数后：补 `num_values.json` / 头文件，改本表 vanilla，**不要**把该数写回 for 循环。

---

## 4. MIN / MAX 符号（`constants.json` → `MAXMIN`）

断言里的值域名。除已在第 1 节钉死的外，整数 `[未知]`。engine_limit 按注释类型；无注释则 `[未知]`。

### 4.1 已能对应玩法上限的

| symbol | vanilla | engine_limit | recommended | 类型线索 | 备注 |
| --- | --- | --- | --- | --- | --- |
| `LEVEL_MIN` | 0 `[推断]` | 0 | 0 | `Person.h` UCHAR | |
| `LEVEL_MAX` | 设计 100；存储 255 | 255 | 100（设计） | UCHAR | 与 `level_per_track` 对齐 |
| `ABILITY_AMOUNT_MIN` | 0 `[推断]` | 0 | 0 | | 成长 vs 结算未拆符号 |
| `ABILITY_AMOUNT_MAX` | 成长 100 / 结算 500 | 255 / 65535 | 跟 caps 两键 | | **不要**合成一个 MAX |
| `SKILL_AMOUNT_MIN` | 0 `[推断]` | 0 | 0 | | |
| `SKILL_AMOUNT_MAX` | 技能封 200；输出再夹 500 | 510 / 65535 | 跟 `skill_cap` + `ability_settled_cap` | | 公式封顶 ≠ 输出夹紧 |
| `TOWNDEVELOPMENT_MIN` | 0 `[推断]` | 0 | 0 | uint16 | |
| `TOWNDEVELOPMENT_MAX` | 9999 | 65535 | 9999 | uint16 `+0x1e` | = `town_develop` |
| `SHIP_CREW_MIN` / `MAX` | `[未知]` | 65535 | `[未知]` | USHORT `ShipBase.h` | |
| `SHIP_DURABILITY_MIN` / `MAX` | `[未知]` | 65535 | `[未知]` | USHORT | |
| `MARKET_PRICE_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | `Price.h` | |
| `TRADINGITEM_DATAPRICE_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | `Price.h` | 定义价，不是市价 |
| `TRADINGITEMGRADE_MIN` / `MAX` | 伦敦特产品可见 12–25 | `[未知]` | `[未知]` | | 样本不是全域 |
| `SPECIALPRODUCT_SUPPLY_MIN` / `MAX` | 伦敦可见 2000–4500 | `[未知]` | `[未知]` | `SpecialProduct.h` | |
| `SHARE_MIN` / `MAX` | 加份额封 `100−已占` | `[未知]` | 0 / 100 `[推断]` | | 100 来自 F-ECO-07，MIN/MAX 符号未核 |
| `FATIGUE_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | | |
| `FORCEAMITY_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | `Force.h` | 友好度 |
| `FORCE_INFLUENCE_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | `Force.h` | |
| `WIND_DIRECTION_ADJUST_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | | |
| `WIND_VELOCITY_ADJUST_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | | |
| `ITEMEFFECT_AMOUNT_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | | |
| `SUPPLYAMOUNT_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | | |
| `MENTALLEVEL_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | | |
| `PHYSICALLEVEL_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | | |
| `LC_MIN` / `LC_MAX` | `[未知]` | `[未知]` | `[未知]` | | 或与许可证选择有关 |

### 4.2 ID / 位域符号（不是玩法上下限）

这些出现在 MAXMIN 列表，语义是「该类型的 id 或 bitfield 宽」，不是 `caps.yaml` 的等级上限。engine_limit 由 `_GetBitTotal_BYTE2DWORD` / 表长决定。vanilla 整数值 `[未知]`，除非已有 `NUM_*`。

`ABSORBED_ID` `ADDPARTS_BITS` `BATTLEALGO_ID` `BATTLEGUN_ID` `BATTLERESULTSTATE_BITS` `BATTLERESULT_ID` `BUSTUP_ID` `CABINKIND_ID` `COUNTRY_ID` `CULTURE_ID` `FACELOCATION_ID` `FASHIONSPEED_ID` `FIGUREHEAD_ID` `FLAGSHIPSTATE_BITS` `FLEETAIMKIND_ID` `FLEETATTRIBUTE_BITS` `FLEETSHIP_ID` `FLEETSTATE_ID` `FLEET_ID` `FORCEAIMKIND_ID` `FORCECOLOR_ID` `FORCEFAILUREKIND_ID` `FORCEKIND_ID` `FORCERELATION_ID` `FORCE_ID` `GUNTYPE_ID` `INT_MAX` `ITEMSTATE_BITS` `ITEM_ID` `JOBKIND_ID` `JUNCTION_ID` `LICENSELOCATIONCHOICES_ID` `MAPPOINTKIND_BITS` `MAPPOINTKIND_ID` `MASTSTATE_BITS` `MENTALSTATE_ID` `MESSAGETYPE_ID` `MOBILEFLEETKIND_ID` `MOBILEFLEET_ID` `MOBILESHIPBATTLESTATE_BITS` `MOBILESHIPSTATE_ID` `MOBILESHIP_ID` `PERSONKNOWN_ID` `PERSON_ID` `PHYSICALSTATE_ID` `PLAYERCOMPANION_BITS` `PLAYERSTATE_BITS` `REMAINSSTATE_BITS` `REMAINS_ID` `REQUESTKIND_ID` `REQUESTPERSON_ID` `REQUEST_ID` `ROUTELOCATOR_ID` `ROUTE_ID` `SCHEMEKIND_ID` `SEAAREA_BITS` `SEAAREA_ID` `SELECTTERGET_ID` `SEX_ID` `SHELL_ID` `SHIPGRADE_ID` `SHIPTYPE_ID` `SHIP_ID` `SKILL_ID` `SPECIALPRODUCTSTATE_BITS` `TOWNEXIST_BITS` `TOWNKIND_ID` `TOWNSTATEKIND_ID` `TOWN_ID` `TRADINGITEMGRADE_ID` `TRADINGITEMKIND_ID` `TRADINGITEMSTATE_BITS` `TRADINGITEM_ID` `VILLAGE_ID` `WALLSKY_ID` `ZODIAC_ID`

`INT_MAX` 是 C 宏，不是游戏设计上限。不要写进 `caps.yaml`。

位域断言：`0 <= eData < _GetBitTotal_BYTE2DWORD(sizeof(X_BITS))`。

---

## 5. 哨兵与非法值

| 用途 | vanilla | engine_limit | recommended | key |
| --- | --- | --- | --- | --- |
| 空货格 / 空特产品 | 106 (`0x6A`) | 专用哨兵，**禁止**与 0 混用 | `trading_item.empty` = 106（原版包）；模组可改但须全局一致 | `ITEM_EMPTY` |
| 非法枚举 | -1 | 有符号 id | `invalid` = -1 | 各 `INVALID_*` |
| `INVALID_HANDLE` | `[未知]` 符号 | — | 不进内容表 | `constants.json` INVALID 列表独有 |

`constants.json` 的 `INVALID_` 全表（值凡头文件写出的均为 -1，其余 `[未知]` 按惯例 -1 但未断言）：

`INVALID_ABSORBED` `INVALID_BATTLEALGO` `INVALID_BATTLEGUN` `INVALID_BATTLERESULT` `INVALID_BUSTUP` `INVALID_CABINKIND` `INVALID_COUNTRY` `INVALID_CULTURE` `INVALID_FACELOCATION` `INVALID_FASHIONSPEED` `INVALID_FIGUREHEAD` `INVALID_FLEET` `INVALID_FLEETAIMKIND` `INVALID_FLEETSHIP` `INVALID_FLEETSTATE` `INVALID_FORCE` `INVALID_FORCEAIMKIND` `INVALID_FORCECOLOR` `INVALID_FORCEFAILUREKIND` `INVALID_FORCEKIND` `INVALID_FORCERELATION` `INVALID_GUNTYPE` `INVALID_HANDLE` `INVALID_ITEM` `INVALID_JOBKIND` `INVALID_JUNCTION` `INVALID_LICENSELOCATIONCHOICES` `INVALID_MAPPOINTKIND` `INVALID_MENTALSTATE` `INVALID_MESSAGETYPE` `INVALID_MOBILEFLEET` `INVALID_MOBILEFLEETKIND` `INVALID_MOBILESHIP` `INVALID_MOBILESHIPSTATE` `INVALID_PERSON` `INVALID_PERSONKNOWN` `INVALID_PHYSICALSTATE` `INVALID_REMAINS` `INVALID_REQUEST` `INVALID_REQUESTKIND` `INVALID_REQUESTPERSON` `INVALID_ROUTE` `INVALID_ROUTELOCATOR` `INVALID_SCHEMEKIND` `INVALID_SEAAREA` `INVALID_SELECTTERGET` `INVALID_SEX` `INVALID_SHELL` `INVALID_SHIP` `INVALID_SHIPGRADE` `INVALID_SHIPTYPE` `INVALID_SKILL` `INVALID_TOWN` `INVALID_TOWNKIND` `INVALID_TOWNSTATEKIND` `INVALID_TRADINGITEM` `INVALID_TRADINGITEMKIND` `INVALID_VILLAGE` `INVALID_WALLSKY` `INVALID_ZODIAC`

禁止用 0 当空货格：0 是南瓜种子（`eItem`）或交易品表第一项。

模组追加内容：`id >= 1000` 或独立命名空间，避免覆盖原版 0–96 港等。

---

## 6. 存档宽度（不是玩法 caps，但限制状态）

| key | vanilla | engine_limit | recommended | 源 |
| --- | --- | --- | --- | --- |
| 名册条数 | 98 | schema | 原版包 98 | `SAVE_SAILOR_COUNT` |
| 名册步长 | `0x19` | schema | schema | `save_format.md` |
| 六维 | 6×uint8 | uint8 | 6 | |
| 单段经验 | uint32 | uint32 | uint32 | |
| 主角总等级缓存 | uint32 `+0x70` | 派生 | 可重建 | |
| 港发展度 | uint16 | 65535 | 9999 设计 | |
| 货格件数（补丁） | 不用 / uint8 `+3` | 255 | 原版包不存件数 | `trade_stack.md` STK3：原版存档不写 `+3` |

存档不存技能、不存公式原文。存公式包 version。

---

## 耦合与独立旋钮

改 `caps.level_per_track` 不得改贸易代码。改 `ability_settled_cap` 不得自动放飞买货：买货走 `buy_ability_clamp`。

改 `NUM_TOWN` 不得重编 exe：内容包加行，运行时 `len(towns)`。

`cargo_stack` 与价格列宽度解耦。显示层自己处理 `NxPRICE` 换行（STK3 已有两行格式）。

## 解耦后的配置

```yaml
# config/caps.yaml          设计上限
# config/engine_limits.yaml 整数宽 + 溢出策略 saturate
```

示例：`Docs/14-config-schemas/examples/caps.yaml`、`engine_limits.yaml`。

校验：启动时 `caps.* ≤ engine_limits.*`；外键指向已注册 ID 或哨兵。失败则拒绝启动。

## 未核实

- 第 3 节全部 `NUM_*` 整数
- 第 4 节除已映射外的 MIN/MAX 整数
- 制海 9999 与商业/军事 9999 是否同一 caps 键
- `NUM_TOWN2=100` 的确切用途
- `NUM_KIND` vs `NUM_TRIGGERKIND`
- `ABILITY_AMOUNT_MAX` 对应成长还是结算
