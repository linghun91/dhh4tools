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
| 32 位 `isqrt`（`q*0x10000`） | 单段等级 255；≥256 结果错乱，`q` 为 65536 倍数时除零 | 饱和，禁止绕回 |

`AccessMacro.h`：`_GetDataSizeMax_BYTE2DWORD(1/2/4) = 0xFF / 0xFFFF / 0xFFFFFFFF`。

---

## 1. 玩法上限（写入 `caps.yaml`）

| key | vanilla | engine_limit | recommended | 源 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `level_per_track` | 100 | 255 | 100 | `exe_caps.md`；`0x1401c97e5` / `0x1401c92f5` `mov r8d,0x64` | `level=min(isqrt16(exp/30)/256,100)`（F-PROG-01，`exp<60` 为 0）。≥256 级 `q*0x10000` 回绕，`q` 为 65536 倍数时除零 |
| `track_count` | 2 | 实现自定 | 2 | 存档两段经验 | 总等级 = 两段之和 |
| `total_level_max` | 200 | 510 | 200 | 100×2 / 255×2 | 派生：`level_per_track * track_count` |
| `exp_divisor` | 30 | （参数，非宽顶） | 30 | `save_format.md` | 公式参数，**只**在 `formulas/progression.yaml`（列此仅对照） |
| `exp_track_cap` | 300000 | `30*255*255=1_950_750` | `null`（= `exp_from_level(level_per_track)`） | `0x493E0` 四处：`fn_140011c34`×2、`fn_140006c67`、`fn_140008387` | 单段经验上限。禁止写死 300000。旧名 `exp_cap`、`exp_per_track_at_cap` |
| `ability_cap` | 100 | 255 | 100 | `fn_1400062da` `0x1401c7f75 mov r8d,0x64` | 六维唯一上限（F-PROG-10）。单字节。旧名 `ability_growth_cap` |
| `skill_value_cap` | 500 | 65535（工具值） | 500 | `fn_1401cbb40` `0x1401cbb5c` | **技能值 / Ability 输出**夹紧；全 exe 仅 `fn_140009214`、`fn_1400135cf` 两处调用，不夹六维。旧名 `ability_settled_cap` 作废 |
| `buy_ability_clamp` | 500 | 同 `skill_value_cap` 宽 | 500 | `exe_caps.md` 买货补丁 | **独立键**。提高结算上限时此项可仍为 500 |
| `battle_ability_clamp` | 500 | 同上 | 500 | `500-a` | 独立键 |
| `skill_level_cap` | 200 | 510（两段之和） | 200 | `fn_140009214` `0x1401c9d85 cmp 0xC8` + `mov 0xC8` | 夹的是等级项：`min(total_level, skill_level_cap)`，不是技能值（旧名 `skill_cap`） |
| `town_develop` | 9999 | 65535 uint16 | 9999 | `0x14025f577 mov ecx,0x270F` → 写 `town+0x1e+track*2` | 商业/军事（F-ECO-11）。exe 另一处 `0x140106581 mov r8d,0x270F` 用途 `[未知]` |
| `cargo_stack` | 1 | 255（STK3 件数 `uint8`） | 1 | `VANILLA_STACK` | 原版不堆叠。UI `%6d` 不是规则 |
| `display_stack_max` | 9 | UI 自理 | （不进 caps） | `DISPLAY_STACK_MAX`；`trade_stack.md` | 显示层；禁止倒逼 `cargo_stack` |
| `fleet_ships` | 5 | 表长 | 5 | `NUM_FLEETSHIP`（表长） | 玩家船位 |
| `hold_slots_per_ship` | 5 | 表长 | 5 | `NUM_SHIPLOAD` / `NUM_SHIPLOAD_EXCH` | 交易货格 |
| ~~`hold_scan`~~ | — | — | 删除 | `fn_140006b95(ship, kind)` 扫 31 舱室槽数 `kind` | **误读**：`0x0A` 是 `eCabinKind` 10 货物仓库，不是扫描 10 格。改用 role `cargo_room`（`Ships.cabin_count(ship_id, cargo_room)`） |
| `persisted_full_person` | 87 | 存档 schema | 不进 caps（重构存全部人物） | `CPerson::vfunc_06/07` `cmp id, 0x57` | 原版存档完整人物记录数；98（`SAVE_SAILOR_COUNT`）是 dk4save 工具值。旧键 `sailor_roster` 作废 |
| `stat_count` | 6 | 表长 | 6 | `SAVE_STAT_COUNT` / `NUM_ABILITY` | 六维。不进 caps：由 `abilities.yaml` 表长决定 |
| `calendar_ticks_per_day` | 48 | — | 48 | `0x30`（`fn_14000b122` 48·n；旅馆每晚 `0x30`） | D4。`engine/tick.yaml` 不重复 |
| `cabins_per_ship` | 31 | 表长 | 31 | `NUM_SHIPCABIN`；`fn_140006b95` `< 0x1F` | 05-ships-fleet/03 |
| `owned_ships_max` | 92 `[推断]` | 实例表长 | 92 | 动态段 `[8, 0x64)` 槽数（`05-ships-fleet/07` §2.1） | 持有船上限；与船体表长（107）无关 |
| `gold_max` | 99,999,999 | `gold_hard` | 99,999,999 | `fn_140002cb6` 饱和 `0x5F5E0FF` | 势力金币上限。报价夹紧 `quote.gold_max` 是公式参数，不是本键 |
| `max_new_slots` | 25 | `fleet_ships*hold_slots` 或 `25*stack` | 由船×格推导 | `AddLoad` `0x19` | 禁止写死 25 |
| `share_letter_cut` | 80 | `SHARE_MAX` 一旦还原 | 80 | `trade_stack.md` `0x50` | 流行品信件分支；公式参数，在 `formulas/factions.yaml`（词表 §14-9）。旧名 `share_high` |
| `fashion_share_div` | 6 | — | 6 | `mov ecx,6` | `floor(x/6)+1`；`formulas/factions.yaml`。旧名 `fashion_div` |
| `fashion_threshold_numer` / `_denom` | 1 / 2 | — | 1 / 2 | 工具 `fthr` = numer | `ceil(slots*numer/denom)`（补丁式 `(slots*fthr+1)>>1`）；`formulas/factions.yaml`。旧名 `fashion_threshold_factor` |
| `letter_intro_min` / `letter_intro_max` | 10 / 15 | — | 10 / 15 | `fl1`；`0x140277e63` / `0x140277e71 cmp 0xF` | 介绍信件数区间；`formulas/factions.yaml`。旧名 `letter_lo` |
| `letter_recommend_min` / `letter_upgrade_min` | 16 / 16 | — | 16 / 16 | `fl2`；`0x14027805f` / `0x14027829d` `cmp 0x10` | 推荐信 / 介绍信升推荐信（两处独立立即数）；`formulas/factions.yaml`。旧名 `letter_hi` |
| `same_item_adjust_threshold` | 15 | — | 15 | `0x1402777ae cmp 0xF` | 同品调港口值，式见 F-ECO-10 |
| `local_fleet_founding_gold` | 100000 | 金币宽 | 100000 | mesfile | 文案常数 |
| `ref_1000` | 1000 | — | 1000 | 买货 | 见公式表 |
| `ref_500` | 500 | — | 500 | 战斗 | |
| `ref_100` | 100 | — | 100 | 道具价/份额封顶 | |
| `rand_span` | 100 | — | 100 | `rand(100)` | |
| `ability_div` | 50 | — | 50 | `a/50` | |
| `level_bias` | 100 | — | 100 | 技能 `+100` | |
| `skill_divisor` | 150 | — | 150 | 技能 `/150` | |
| `item_price_scale` | 100 | — | 100 | `0x14015657f` | F-ECO-02 |
| `battle_scale` | 500 | — | 500 | `idiv 0x1F4` | F-CBT-01 |

第 1 节后半（`ref_*` 起）其实是**公式参数**，单一来源在 `formulas/*.yaml`，列在这里只为对照。`caps.yaml` 只放上限，不重复这些键。

`FATIGUE_*`、`FORCEAMITY_*`、`FORCE_INFLUENCE_*`、`SHARE_*`、`MARKET_PRICE_*`、`SHIP_CREW_*`、`SHIP_DURABILITY_*`、`WIND_*_ADJUST_*` 见第 4 节（符号有、整数无）。

---

## 2. `NUM_*` 计数与证据类型

计数的**唯一登记处**是 [04-id-registry.md](../00-overview/04-id-registry.md)（每行带证据类型与等级）。本节只按 caps 视角汇总，数值以该篇为准。

证据类型（D1）：**表长**（内容表条数 / 元素构造计数）、**循环**（遍历循环上界）、**判界**（取表 / 断言 `< N`）、**投票**（`fill_gaps.py` 按 RTC 立即数众数猜；`num_values.json` 与「Original:」头 `*ID.h` 的整数除 `KNOWN_NUM` 外都是投票）。投票只能 `[推断]`。

运行时 **一律读表长**。vanilla 列只描述原版包，engine_limit 一律「表长」，下表省略。

### 2.1 已还原

| key | vanilla | 证据类型 | 等级 | 源 / 备注 |
| --- | --- | --- | --- | --- |
| `NUM_TOWN` | 97 | 表长 + 循环 | [确认] | GetFixTownProc；`fn_140007806` `< 0x61` |
| `NUM_TOWN2` | 100 | 投票 | [推断] | 港口数组垫到 100 |
| `NUM_TOWNSHARE` | 97 | 投票（误采） | 无证据 | RTC 众数 3（27 次）被过滤；`[推断]` 实为每港 3 份额槽下标 |
| `NUM_TOWNFASHION` | 26 | 投票（误采） | 无证据 | 所有 `eTownFashion` 循环上界 2（每港 2 槽） |
| `NUM_TOWNDEVELOP` | 2 | 投票 + 表长旁证 | [推断] | 港口表 `+0x52` 2×u16 |
| `NUM_TOWNKIND` | 2 | 循环 | [推断] | 海域图一处 |
| `NUM_VILLAGE` | 24 | 循环 | [确认] | `fn_140001186` `< 0x18` |
| `NUM_VILLAGEBLINDAREA` | 2 | 循环 | [推断] | 海域图一处 |
| `NUM_SEAAREA` | 25 | 投票（误采） | 无证据 | 函数体内 `eSeaArea` 上界都是 7 → `[推断]` = 七大海域；fill_gaps 扫描窗口越过函数尾 |
| `NUM_NEXTBASETOWN` | 25 | 投票 | [推断]，待重查 | 与海域同数 |
| `NUM_ROUTE` | 200 | 循环 + 表长 | [确认] | `CRouteNavigator::vfunc_03` `< 0xC8` |
| `NUM_JUNCTION` | 164 | 循环 + 表长 | [确认] | `fn_140012f08` `< 0xA4` |
| `NUM_REMAINS` | 25 | 表长 + 循环 | [确认] | `CRemainsElement` 0x19；`fn_14000a321` |
| `NUM_MAPPOINTKIND` | 12 | 循环 | [确认] | `CTownSceneView` `>= 0xC` |
| `NUM_ZODIAC` | 12 | 判界 + 表长 | [确认] | `fn_14000b852` `< 0xC` |
| `NUM_SCENARIO` | 7 | 表长 | [确认] | `fn_140009df9` 7 元素表 |
| `NUM_REQUEST` | 69 | 循环 | [确认] | `fn_14000182f` `< 0x45` |
| `NUM_REQUESTPERSON` | 100 | 循环 | [推断] | `fn_14000e23c` `>= 0x64` |
| `NUM_SCHEME` | 24 | 投票 | [推断] | |
| `NUM_SCHEMEKIND` | 10 | 判界 | [推断] | 校验 `< 0xA` |
| `NUM_PERSON` | 235 | 表长 | [确认] | GetFixPersonProc |
| `NUM_ABILITY` | 6 | 表长 | [确认] | 人物记录 6 字节六维 |
| `NUM_SKILL` | 10 | 循环 + 表长 | [确认] | `fn_14000dd69` `< 0xA`；配对表 10×2 |
| `NUM_SKILLABILITY` | 2 | 判界 | [推断] | `fn_1400099e9` 第二下标 `< 2` |
| `NUM_POST` | 16 | 判界 + 表长 | [确认] | `0x1401ecaa0` `< 0x10`；16 名 |
| `NUM_PERSONEQUIP` / `NUM_EQUIP` | 5 | 循环 | [确认] / [推断] | `fn_14000e0b6` 5 槽 |
| `NUM_SEX` | 2 | 判界 | [确认] | `fn_140006b90` |
| `NUM_MENTALSTATE` / `NUM_PHYSICALSTATE` | 6 / 7 | 表长 | [确认] | 名称指针表 |
| `NUM_FORCE` | 25 | 表长 | [确认] | GetFixForceProc |
| `NUM_FORCECOLOR` | 8 | 循环 | [确认] | 海域图 `>= 8` |
| `NUM_CULTURE` | 21 | 表长 | [确认] | `CCultureElement` 0x15；`.rdata 0x1405b6160` |
| `NUM_THISFORCE` / `NUM_NOWPERSON` | 4 / 4 | 投票 | [推断] | |
| `NUM_TRADINGITEM` | 105 | 表长 + 循环 | [确认] | 名表 / 目录 105；`CCulture::vfunc_05` `< 0x69` |
| `NUM_TRADINGITEMKIND` | 19 | 循环 | [确认] | `CTown::vfunc_05` `< 0x13` |
| `NUM_SPECIALPRODUCT` | 5/港 | 表长 | [确认] | 港口记录 5 槽 |
| `NUM_SPECIALPRODUCTCNT` | 5 | 投票 | [推断] | |
| `NUM_ITEM` | 231 | 表长 + 循环 | [确认] | GetFixItemProc；`< 0xE7` |
| `NUM_ITEMKIND` | 13 | 判界 | [推断] | `CItemKindMenuData` `< 0xD` |
| `NUM_SHIP` | **207**（投票值 100） | 表长 + 循环 | [推断] | 实例槽 `CShipElement` 0xCF；`fn_14000ceaa` 扫 `0..0xCF`。100 只是动态 / 预置分界 |
| `NUM_SHIPTYPE` | **39**（投票值 5） | 循环 | [推断] | `eShipType` 循环 `0x27`；可买 `NUM_NORMALSHIPTYPE`=34（`0x22`） |
| `NUM_SHIPLOAD` / `_EXCH` / `NUM_SHIPLOADCARGO` | 5 | 循环 | [确认] / [推断] | F-ECO-08 |
| `NUM_SHIPCABIN` | 31 | 循环 | [确认] | `fn_140006b95` `< 0x1F` |
| `NUM_CABINSURFACE` | 31 | 投票 | [推断] | 与舱室同数 |
| `NUM_CABINKIND` | 23 | 判界 | [推断] | 取表 `< 0x17`；静态表 24 行含「无」 |
| `NUM_SHIPMAST` | **3**（投票值 100） | 投票众数 | [推断] | RTC 立即数 3×21 被 `<4` 过滤 |
| `NUM_SHIPSUPPLY` | 2 | 循环 | [推断] | RTC 与 2 比较 5 次 |
| `NUM_FLEET` | 69 | 表长 | [确认] | GetFixFleetProc |
| `NUM_FLEETSHIP` | 5 | 表长 | [确认] | |
| `NUM_ATTACKERFLEETSHIP` / `NUM_INVADEFLEETSHIP` / `NUM_TARGETFLEETSHIP` | 5 | 投票 | [推断] | |
| `NUM_GUNTYPE` | 6 | 判界 | [确认] | `0x140123300` |
| `NUM_BATTLEGUN` | **21**（投票值 5） | 循环（唯一函数） | [推断] | `fn_14000ee8a` `< 0x15` |
| `NUM_BATTLERESULT` | 5 | 投票 | [推断] | |
| `NUM_SHELL` | 100 | 循环 | [确认] | `fn_14000ee8a` `>= 0x64`；实例池 [推断] |
| `NUM_MOBILESHIP` | 20 | 循环 | [确认] | `fn_14000ee8a` `>= 0x14` |
| `NUM_MOBILEFLEET` | 4 | 循环 | [确认] | `CFleetElement` 存档 4 条 |
| `NUM_ADDPARTS` | 9 | 循环 | [确认] | 循环 `< 9` |
| `NUM_DECKADDPARTS` | 4 | 投票 | [推断] | |
| `ITEM_EMPTY` | 106 | `Constants.h` | [确认] | **不是** `NUM_*`，见第 5 节 |

### 2.2 RTC 杂项（投票，不一定是玩法枚举）

| key | vanilla | 备注 |
| --- | --- | --- |
| `NUM_BUTTON` | 4 | 可能是控件数 |
| `NUM_HAPPENING` | 5 | RTC `eHappening`；海战 happening 命令，≠ 海上 20 槽 |
| `NUM_INDEX` | 5 | RTC `eIndex` |
| `NUM_KIND` / `NUM_TRIGGERKIND` | 9 / 9 | 同值 `[推断]`，勿合并直到命名；叙事触发 kind 0–8 `[确认]`（10-narrative） |
| `NUM_SELECTITEMTAG` | 6 | |
| `NUM_SMOKE` | 8 | 特效 |
| `NUM_HOLD_SCAN` | ~~10~~ | **删除**：`Constants.h` 注释误读，`0x0A` 是舱室种类 10（货物仓库） |

---

## 3. `NUM_*` 仅符号（整数未还原）

`constants.json` → `NUM_` 有名；头文件只有符号。vanilla / recommended 都是 `[未知]`，engine_limit = 表长。

| symbol | 头文件 | 备注 |
| --- | --- | --- |
| `NUM_ABSORBED` | `AbsorbedID.h` | |
| `NUM_BATTLEALGO` | `BattlealgoID.h` | |
| `NUM_BUSTUP` / `NUM_FACELOCATION` | `BustupID.h` / `FacelocationID.h` | 只在 World DLL 断言 |
| `NUM_COUNTRY` | `CountryID.h` | 港 `+0x25` 是 `culture_id`（21），**不是** eCountry |
| `NUM_FASHIONSPEED` | `FashionspeedID.h` | |
| `NUM_FIGUREHEAD` | `FigureHeadID.h` | 道具 114–123 10 件；`CShip+0x53` 空 `0x0D` |
| `NUM_FLEETAIMKIND` / `NUM_FLEETSTATE` | 对应头文件 | |
| `NUM_FLEETCOLOR` | `FleetcolorID.h` | 与 `NUM_FORCECOLOR=8` 是否同一 `[未知]` |
| `NUM_FORCEAIMKIND` / `NUM_FORCEFAILUREKIND` / `NUM_FORCEKIND` / `NUM_FORCERELATION` | 对应头文件 | |
| `NUM_JOBKIND` | `JobkindID.h` | |
| `NUM_LICENSELOCATIONCHOICES` | `LicenselocationchoicesID.h` | |
| `NUM_MESSAGETYPE` | `MessageTypeID.h` | |
| `NUM_MOBILEFLEETKIND` / `NUM_MOBILESHIPSTATE` | 对应头文件 | |
| `NUM_PERSONKNOWN` | `PersonknownID.h` | |
| `NUM_PLAYEREVENTSHIP` | `PlayereventshipID.h` | |
| `NUM_PROXYFLEET` | `ProxyfleetID.h` | |
| `NUM_REQUESTKIND` | `RequestkindID.h` | |
| `NUM_ROUTELOCATOR` | `RouteLocatorID.h` | |
| `NUM_SELECTTERGET` | `SelecttergetID.h` | 原拼写 Terget |
| `NUM_SHIPGRADE` | `ShipgradeID.h` | `[推断]` 船模体型档 3 |
| `NUM_TOWNACCESS` / `NUM_TOWNSTATEKIND` | 对应头文件 | |
| `NUM_WALLSKY` | `WallskyID.h` | |

还原整数后：改 04-id-registry 对应行并写明证据类型，**不要**把该数写回 for 循环。

---

## 4. MIN / MAX 符号（`constants.json` → `MAXMIN`）

断言里的值域名。除已在第 1 节钉死的外，整数 `[未知]`。engine_limit 按注释类型；无注释则 `[未知]`。

### 4.1 已能对应玩法上限的

| symbol | vanilla | engine_limit | recommended | 类型线索 | 备注 |
| --- | --- | --- | --- | --- | --- |
| `LEVEL_MIN` | 0 `[推断]` | 0 | 0 | `Person.h` UCHAR | |
| `LEVEL_MAX` | 设计 100；存储 255 | 255 | 100（设计） | UCHAR | 与 `level_per_track` 对齐 |
| `ABILITY_AMOUNT_MIN` | 0 `[推断]` | 0 | 0 | | 成长 vs 结算未拆符号 |
| `ABILITY_AMOUNT_MAX` | 100 `[推断]` | 255 | `ability_cap` | | 六维只被 100 夹（`fn_1400062da`）；500 夹的是技能，见 `SKILL_AMOUNT_MAX` |
| `SKILL_AMOUNT_MIN` | 0 `[推断]` | 0 | 0 | | |
| `SKILL_AMOUNT_MAX` | 500 `[推断]` | 65535 | `skill_value_cap` | | 技能值夹 500；200 是等级项 `skill_level_cap`，不是技能值 |
| `TOWNDEVELOPMENT_MIN` | 0 `[推断]` | 0 | 0 | uint16 | |
| `TOWNDEVELOPMENT_MAX` | 9999 | 65535 | 9999 | uint16 `+0x1e` | = `town_develop` |
| `SHIP_CREW_MIN` / `MAX` | `[未知]` | 65535 | `[未知]` | USHORT `ShipBase.h` | |
| `SHIP_DURABILITY_MIN` / `MAX` | `[未知]` | 65535 | `[未知]` | USHORT | |
| `MARKET_PRICE_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | `Price.h` | |
| `TRADINGITEM_DATAPRICE_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | `Price.h` | 定义价，不是市价 |
| `TRADINGITEMGRADE_MIN` / `MAX` | `[未知]` | `[未知]` | `[未知]` | | 伦敦槽 +2 的 12–25 是**上架门槛**，不是交易品等级，不能当本符号样本 |
| `SPECIALPRODUCT_SUPPLY_MIN` / `MAX` | 伦敦槽 +4 供给基数可见 2000–4500 | `[未知]` | `[未知]` | `SpecialProduct.h` | 样本不是全域 |
| `SHARE_MIN` / `MAX` | 加份额封 `100 − 本势力份额`；每港 3 槽合计亦 ≤ 100 | `[未知]` | 0 / 100 `[推断]` | uint8 槽 | 100 来自 F-ECO-07 `0x1402635f6`，MIN/MAX 符号未核 |
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
| 空货格 / 空特产品 | 106 (`0x6A`) | 原版字节，**禁止**与 0 混用 | 运行时 `null`（`Option<Id>`）；106 只在 `ids/legacy_codec.yaml` 编解码 | `ITEM_EMPTY` |
| 其它数据空字节 | 98 / 232 / 236 / 70 / 201 / 22 / 26 / 208 / 40 / `0x18` / `0x0D` / 27 | 原版字节 | 同上，按「枚举 × 字段」查 `legacy_codec`，见 [04-id-registry.md](../00-overview/04-id-registry.md)「遗留哨兵与存储宽度」 | — |
| 非法枚举 | -1 | 有符号 id | 运行时 `null`；-1 只出现在原版断言 | 各 `INVALID_*` |
| `INVALID_HANDLE` | `[未知]` 符号 | — | 不进内容表 | `constants.json` INVALID 列表独有 |

`constants.json` 的 `INVALID_` 全表（值凡头文件写出的均为 -1，其余 `[未知]` 按惯例 -1 但未断言）：

`INVALID_ABSORBED` `INVALID_BATTLEALGO` `INVALID_BATTLEGUN` `INVALID_BATTLERESULT` `INVALID_BUSTUP` `INVALID_CABINKIND` `INVALID_COUNTRY` `INVALID_CULTURE` `INVALID_FACELOCATION` `INVALID_FASHIONSPEED` `INVALID_FIGUREHEAD` `INVALID_FLEET` `INVALID_FLEETAIMKIND` `INVALID_FLEETSHIP` `INVALID_FLEETSTATE` `INVALID_FORCE` `INVALID_FORCEAIMKIND` `INVALID_FORCECOLOR` `INVALID_FORCEFAILUREKIND` `INVALID_FORCEKIND` `INVALID_FORCERELATION` `INVALID_GUNTYPE` `INVALID_HANDLE` `INVALID_ITEM` `INVALID_JOBKIND` `INVALID_JUNCTION` `INVALID_LICENSELOCATIONCHOICES` `INVALID_MAPPOINTKIND` `INVALID_MENTALSTATE` `INVALID_MESSAGETYPE` `INVALID_MOBILEFLEET` `INVALID_MOBILEFLEETKIND` `INVALID_MOBILESHIP` `INVALID_MOBILESHIPSTATE` `INVALID_PERSON` `INVALID_PERSONKNOWN` `INVALID_PHYSICALSTATE` `INVALID_REMAINS` `INVALID_REQUEST` `INVALID_REQUESTKIND` `INVALID_REQUESTPERSON` `INVALID_ROUTE` `INVALID_ROUTELOCATOR` `INVALID_SCHEMEKIND` `INVALID_SEAAREA` `INVALID_SELECTTERGET` `INVALID_SEX` `INVALID_SHELL` `INVALID_SHIP` `INVALID_SHIPGRADE` `INVALID_SHIPTYPE` `INVALID_SKILL` `INVALID_TOWN` `INVALID_TOWNKIND` `INVALID_TOWNSTATEKIND` `INVALID_TRADINGITEM` `INVALID_TRADINGITEMKIND` `INVALID_VILLAGE` `INVALID_WALLSKY` `INVALID_ZODIAC`

禁止用 0 当空货格：0 是南瓜种子（`eItem`）或交易品表第一项。

模组追加内容：用命名空间字符串 key（`mymod:new_port`），整数下标由加载器紧凑分配并写入存档 `id_map`，见 [14-config-schemas/README.md](../14-config-schemas/README.md#模组扩展)、D1。模组不写任何整数 id。哨兵 = NUM / NUM+1 正落在顺序分配区间，运行时不得由计数推导空值。

---

## 6. 存档宽度（不是玩法 caps，但限制状态）

| key | vanilla | engine_limit | recommended | 源 |
| --- | --- | --- | --- | --- |
| 人物记录 | 235 条：id<87 每条 25 B，其余 2 B，基址 `0x2C03` | schema | 按 key 存全部人物 | `CPersonElement::vfunc_01` `< 0xEB`；`CPerson::vfunc_06` `cmp 0x57` |
| dk4save 名册视图 | 98 行 × `0x19`，起 `0x2C00` | 工具 | 不用 | `save_format.md`；行 87–97 是错读 |
| 六维 | 6×uint8 | uint8 | 6 | |
| 单段经验 | uint32 | uint32 | uint32 | |
| 主角总等级缓存 | uint32 `+0x70` | 派生 | 可重建 | |
| 港发展度 | uint16 | 65535 | 9999 设计 | |
| 货格件数（补丁） | 不用 / uint8 `+3` | 255 | 原版包不存件数 | `trade_stack.md` STK3：原版存档不写 `+3` |

存档不存技能、不存公式原文。存公式包 version。

---

## 耦合与独立旋钮

改 `caps.level_per_track` 不得改贸易代码。改 `skill_value_cap` 不得自动放飞买货：买货走 `buy_ability_clamp`。改 `level_per_track` 时 `exp_track_cap`（`null`）自动跟随；`skill_level_cap` 不自动跟随（原版 200 = 100×2 是巧合，不是推导）。

改 `NUM_TOWN` 不得重编 exe：内容包加行，运行时 `len(towns)`。

`cargo_stack` 与价格列宽度解耦。显示层自己处理 `NxPRICE` 换行（STK3 已有两行格式）。

## 解耦后的配置

```yaml
# config/caps.yaml          设计上限
# config/engine_limits.yaml 整数宽 + 溢出策略 saturate
```

示例：`Docs/14-config-schemas/examples/caps.yaml`、`engine_limits.yaml`。

校验：启动时 `caps.* ≤ engine_limits.*`；外键指向已注册 key 或 `null`。失败则拒绝启动。

## 未核实

- 第 3 节全部 `NUM_*` 整数；第 2 节所有「投票」项的真实上界
- 第 4 节除已映射外的 MIN/MAX 整数
- 制海 9999 与商业/军事 9999 是否同一 caps 键
- `NUM_TOWN2=100` 的确切用途
- `NUM_KIND` vs `NUM_TRIGGERKIND`
- `ABILITY_AMOUNT_MAX` / `SKILL_AMOUNT_MAX` 整数（指令证据指向 100 / 500，断言值未见）
