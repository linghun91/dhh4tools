# ID 注册表

来源：`srcdata/source/*ID.h`、`srcdata/source/Constants.h`、`srcdata/catalog/constants.json`、`num_values.json`、`num_from_rtc.json`、World GetFix* 表、exe 反汇编。

运行时以表长为准。本表是原版内容包的实例计数，加上原版存档 / DLL 里的**遗留编码**（哨兵值、存储宽度）。新引擎的 ID 规则以 [09-architecture-decisions.md](09-architecture-decisions.md) D1 与 [05-config-architecture.md](05-config-architecture.md)「ID 与命名空间」为准；本篇末尾「扩展风险」说明为什么原版整数不能直接当主键。

## 计数的证据类型（D1）

每个 `NUM_*` 都标证据类型。等级上限按类型定：

| 类型 | 含义 | 能否 `[确认]` |
| --- | --- | --- |
| **表长** | 内容表 / 元素构造的条数（World DLL `GetFix*`、`CXxxElement` 构造计数、exe 静态表条数） | 能 |
| **循环** | 遍历该枚举的循环上界（`cmp e, N; jge`），在多处或唯一但语义清楚 | 能；只见一处时 `[推断]` |
| **判界** | 取表函数 / 断言的 `0 <= e < N` | 能 |
| **投票** | `srcdata/tools/fill_gaps.py::collect_num_from_rtc` 按「RTC 局部名 → `cmp` 立即数」众数猜（过滤 `<4` 的值，先取 ≥4 的最高频）；`num_values.json` 与「Original:」头的 `*ID.h` 整数都来自这里，只有 `KNOWN_NUM`（港 97、人 235、道具 231、舰队 69、势力 25、舰队船位 5、`ITEM_EMPTY`）例外 | **不能**，最高 `[推断]` |

**`*ID.h` 头里写了整数不等于有证据。** 头文件注释「Original: …\SkillID.h」只说明原版有这个文件名，整数是投票填的。已发现投票误采：`NUM_SHIP=100`（实例池 207）、`NUM_SHIPTYPE=5`（实为 39）、`NUM_SHIPMAST=100`（实为 3，众数 3 被 `<4` 过滤）、`NUM_BATTLEGUN=5`（实为 21）、`NUM_TOWNFASHION=26`（疑为 `0x1A` 误采）、`NUM_HOLD_SCAN=10`（实为舱室种类 10）。另：`eTownShare` 立即数众数是 3（27 次，被过滤），投票值 97 存疑。

## 世界

| 枚举 | 原版计数 | 证据类型 | 等级 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| eTown | 97 | 表长 + 循环 | [确认] | INVALID_TOWN=-1；空港字节 98 | GetFixTownProc 97 条；`fn_140007806` `eTown < 0x61`；海域图 `>= 0x61` | 名称见 `Docs/_data/towns.tsv`。98 见「遗留哨兵」 |
| eTownShare | 97（投票误采） | 投票 | [推断] 实为份额槽下标 0–2 | INVALID_TOWNSHARE=-1 | `TownShareID.h` 97 = 投票；RTC 立即数 3×27、97×6；`CTown+0x18` 3 槽扫描 | 份额槽初值来自港口 `+0x4C`（02-factions 第二轮） |
| eTownDevelop | 2 | 投票 + 表长旁证 | [推断] | | RTC 立即数 2（1 处）；港口表 `+0x52` 2×u16 → `CTown+0x1E` | k=0 是特产品门槛用的发展度（`15-catalogs/towns.md`） |
| eVillage | 24 | 循环 | [确认] | INVALID_VILLAGE=-1；空 25 | `fn_140001186` `eVillage < 0x18`；海域图 `>= 0x18`；`.data 0x140687b40` 24×0x38 | |
| eVillageBlindArea | 2 | 循环 | [推断] | | 海域图同函数 `eBlindArea >= 2` | 只一处 |
| eTownKind | 2 | 循环 | [推断] | | 海域图同函数 `eTownKind >= 2` | 只一处 |
| eSeaArea | 25（投票误采） | 投票 | 25 无证据；[推断] eSeaArea = 七大海域 7 | INVALID_SEAAREA=-1 | `SeaAreaID.h` 的 25 = `fill_gaps.py` 投票（扫描窗口固定 0x2000 越过函数尾、只收 imm≥4）；两个含 `eSeaArea` 的函数体内上界都是 **7** | 「25 航海分区」失去证据（02-factions 第二轮）；`NUM_NEXTBASETOWN=25` 同为投票 |
| eRoute | 200 | 循环 + 表长 | [确认] | INVALID_ROUTE=-1；空线 201 | `CRouteNavigator::vfunc_03` `eRoute < 0xC8`；`aeSeaAreaRoute` int32[200]；`.data 0x140687070` 200×6 | 201 见 06-ocean |
| eJunction | 164 | 循环 + 表长 | [确认] | INVALID_JUNCTION=-1 | `fn_140012f08`（body `0x140138280`）`< 0xA4`；`.data 0x140682f50` | 见 01-world/05 |
| eRemains | 25 | 表长 + 循环 | [确认] | INVALID_REMAINS=-1；港口表空字节 26 | `CRemainsElement` 构造 `fn_140007dce(…,0x10,0x19)`；`fn_14000a321` `< 0x19`；名字指针表 25 条 + NULL `0x1405b2f80` | |
| eMapPointKind | 12 | 循环 | [确认] | INVALID_MAPPOINTKIND=-1 | `CTownSceneView` 构造 `eMapPointKind >= 0xC` 清 12 指针；`MapPointID.h` 的 12 是投票 | |
| eZodiac | 12 | 判界 + 表长 | [确认] | INVALID_ZODIAC=-1 | `fn_14000b852`（body `0x1402aeef0`）`0 <= i < 0xC`，表 `0x140688970` 步长 0x20 | 第 13 槽全 0 |
| eWallsky | `[未知]` | — | — | INVALID_WALLSKY=-1 | WallskyID.h 无整数 | |

## 势力 / 文化

| 枚举 | 原版计数 | 证据类型 | 等级 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| eForce | 25 | 表长 | [确认] | INVALID_FORCE=-1；空字节 26 [确认]（`fn_140012fee` / `fn_140004c19` 比 `0x1A`） | GetFixForceProc 25 条；RTC 25×53 | 0–23 具名势力，24 私家舰队。空 26 见「遗留哨兵」 |
| eCountry | `[未知]` | — | — | INVALID_COUNTRY=-1 | CountryID.h 无整数 | 不要写成 21 |
| eCulture | 21 | 表长 | [确认] | INVALID_CULTURE=-1；港口表空字节 22 | `CCultureElement` 构造 `(…,0xE8,0x15)`；`.rdata 0x1405b6160` 21×`0xE0`；`fn_140013a02` 遇 `0x16` 返回空 | 21 名见文末与 [06-seven-culture-circles.md](../02-factions/06-seven-culture-circles.md) |
| 七大海域 `major_sea_id` | 7 | 循环 + 表长 | [确认] | 无第 8 格；海域表第 8 条「全世界」 | `CForce` 存盘循环 `>= 7`（RTC 名 `eSeaArea`）；文化表 `+0x08` 0–6；`.rdata 0x14058bb30` 8×0x38 | RTC 名就叫 `eSeaArea`；`SeaAreaID.h` 的 25 是投票误采，不存在独立的 25 分区枚举（见上表 eSeaArea） |
| eTownFashion | 槽 2；目录 `[未知]` | 循环（槽）/ 投票（26 误采） | 槽 [确认]；26 无证据 | INVALID_TOWNFASHION=-1；槽空值 106 | 所有 `eTownFashion` 循环上界 2（`CPriceInformationView`、`fn_14025ae30` 等）；`TownFashionID.h` 26 = 投票（立即数 26 仅 2 次） | 每港 2 格流行，每格 `(u8 trading_item, u8 等级)`（DLL `+0x7E` 初值 `(0x6A, 0)`，原版按 word 拷贝）。见 [02-factions/02](../02-factions/02-countries-culture-fashion.md) |
| eForceColor | 8 | 循环 | [确认] | INVALID_FORCECOLOR=-1 | 海域图同函数 `eForceColor >= 8`；`ForceColorID.h` 的 8 是投票 | |
| eForceKind / Aim / Relation / Failure | `[未知]` | — | — | 各 INVALID_*=-1 | 对应 *ID.h | |

## 人物

| 枚举 | 原版计数 | 证据类型 | 等级 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| ePerson | 235 | 表长 | [确认] | INVALID_PERSON=-1；空字节 236（`0xEC`） | GetFixPersonProc 235 条 | 0–6 剧本提督；7–97 具名；98 起大量职能 NPC。区间语义改显式字段（扩展风险 4） |
| eAbility | 6 | 表长 | [确认] | INVALID_ABILITY=-1 | 人物记录 `W+0x20` 6 字节六维；`AbilityID.h` 的 6 是投票 | 体力 敏捷 魅力 智力 精神 运气 |
| eSkill | 10 | 循环 + 表长 | [确认] | INVALID_SKILL=-1 | `fn_14000dd69`（body `0x1401d4e50`）`< 0x0A`；`fn_1400099e9` 断言 `k<10`；配对表 `.data 0x1406864a0` 10×2 | 生命力 统率力 驾驶术 测量术 会计术 剑术 命中力 说服力 谋略术 观察力 |
| eJobKind | `[未知]` | — | — | INVALID_JOBKIND=-1 | JobkindID.h 无整数 | |
| ePost | 16 | 判界 + 表长 | [确认] | INVALID_POST=-1；空人 `0xEC` | 取名函数 `0x1401ecaa0` `< 0x10`；`.rdata 0x1405867f0` 16 名；岗位→舱室 `fn_140007e19` `[0,15]` 跳表 | 舰长 副官 参谋 会计长 测量员 操帆手 舵手 了望手 冲锋队长 炮手 传教士 船医 大厨 船木工 饲养员 甲板主管。舱室→岗位查无返回 `0x11`（`fn_140012dd2`） |
| ePersonEquip | 5 | 循环 | [确认] | INVALID_PERSONEQUIP=-1；空道具 232 | `fn_14000e0b6` 5 槽累加；RTC 5×6 | |
| eSex | 2 | 判界 | [确认] | INVALID_SEX=-1 | `fn_140006b90`：`CPerson+0x08 == 1` 女，否则男 | 头文件无整数 |
| eMentalState | 6 | 表长 | [确认] | INVALID_MENTALSTATE=-1 | 名称指针表 6 项；转移表行宽 6 | 表内 7 = 不变 |
| ePhysicalState | 7 | 表长 | [确认] | INVALID_PHYSICALSTATE=-1 | 名称指针表 7 项；`vfunc_04` 以 6 判死亡 | 健康 疲劳 轻伤 重伤 濒死 疾病 死亡 |
| eRequestPerson | 100 | 循环 | [推断] | | `fn_14000e23c` `>= 0x64` | 委托人表，非同伴 |

存档人物段 `[确认]`（`CPersonElement::vfunc_01/02` body `0x1401cc3e0` / `0x1401cc2e0`，循环 `< 0xEB`；`CPerson::vfunc_06/07` `cmp id, 0x57`）：从 `save+0x2C03` 起按 `ePerson` 0..234 顺序无标签写 235 条记录。`id < 87` 每条 25 B（`+0x00` 隶属、`+0x01` 字节 `CPerson+0x22`、`+0x02` 六维、`+0x08` / `+0x0C` 两段经验 u32、`+0x10` HP、`+0x12` 身体态、`+0x13` 精神态、`+0x14` 装备 5 槽）；`id >= 87` 每条 2 B（隶属与 `+0x22`）；随后 `0x35AA` 起是主角 `CPlayerPerson` 103 B。`SAVE_SAILOR_COUNT=98`（`Constants.h`）是 dk4save 工具的常数，**不是**引擎值；dk4save「`0x2C00` 起 98 行、步长 `0x19`」的行偏移比引擎记录早 3 字节，对 0–86 恰好抵消，87–97 行是错读。详见 `12-ui-save/02-save-session.md`。

`[确认]` 存档记录 i 就是 `ePerson` i（i < 87 完整，87–234 只有 2 B）。旁证：`savedat00.dk4`（World1 剧本）73 行六维与 World1 人物表同下标逐字节相等，全部是 id < 87；87–97 在所有存档里 0 行相等。0–97 只能当「具名可招募」的**内容**区间，不能当存档区间。后果见「扩展风险」第 4 条。

## 经济

| 枚举 | 原版计数 | 证据类型 | 等级 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| eTradingItem | 105 | 表长 + 循环 | [确认] | INVALID_TRADINGITEM=-1；空 `ITEM_EMPTY=0x6A=106` | 名称串 `.rdata` VA `0x1405b77e4`（文件偏移 `0x5b65e4`）、目录 `.data` VA `0x140688190`（文件偏移 `0x686b90`，16 字节/条，含 kind）各 105 条；`CCulture::vfunc_05` `< 0x69` | 106 不在 0–104 内。全名见 `Docs/15-catalogs/trading-items.md` |
| eTradingItemKind | 19 | 循环 | [确认] | INVALID_TRADINGITEMKIND=-1 | `CTown::vfunc_05` `< 0x13`（`CTown+0x4A` 19×u16） | |
| eSpecialProduct | 5 | 表长 | [确认] | INVALID_SPECIALPRODUCT=-1 | 港口记录 `+0x28` 5×6 字节；RTC 5×32 | **每港** 5 个特产品槽 |
| eItem | 231 | 表长 + 循环 | [确认] | INVALID_ITEM=-1；空槽 232（`0xE8`） | GetFixItemProc 231 条；`fn_14001266b` `eItem < 0xE7` | 装备/发现物/地图，不是交易品 |
| eItemKind | 13 | 判界 + 表取值 | [推断] | INVALID_ITEMKIND=-1 | `CItemKindMenuData` `< 0xD`；道具表 `+0x14` 取值 0–12 | 见 09-items/01 |
| eShipSupply | 2 | 循环 | [推断] | -1 | RTC 与 2 比较 5 次；日耗 / 求和循环 2 种 | 0 水 / 1 粮 `[推断]`（13-algorithms/03） |

## 船 / 舰队

| 枚举 | 原版计数 | 证据类型 | 等级 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| eShip（实例槽） | **207** | 表长 + 循环 | [确认] | INVALID_SHIP=-1；空船 `0xD0`=208 | `CShipElement` 构造 `(…,0x80,0xCF)`；`fn_14000ceaa`（`0x14022fdf0`）扫 `0..0xCF` 查无返回 `0xD0` | 0–7 不参与分配；8–99 动态船（`fn_1400018e8` 从 8 扫到 `<0x64`）；100–206 = 船体定义 `id-100`；`NUM_SHIP=100` 是投票，[推断] 实为 207。见 05-ships-fleet/01 |
| 船体定义 `hull` | 107 | 表长 | [确认] | — | `.rdata 0x1405b42e0` 步长 0x30（`fn_14000cff9` `(id-0x64)*0x30`） | 100–106 是安娜希莉雅、拉乌卡、怪鱼、大墨鱼、鲨鱼、白鲸、幽灵船 |
| eShipType（船模） | **39** | 循环 | [推断] | INVALID_SHIPTYPE=-1；实例内空 `0x28`=40 | 所有 `eShipType` 循环上界 `0x27`（可买 `0x22`=34） | `NUM_SHIPTYPE=5` 投票误采，作废 |
| eShipGrade | `[未知]` | — | — | INVALID_SHIPGRADE=-1 | 头文件无整数 | `[推断]` 即船模 `+0x08` 体型档 0/1/2（无船模回 4）；grade 表 `{0,4,10}`/`{3,5,10}` 3 项 |
| eFleet | 69 | 表长 | [确认] | INVALID_FLEET=-1；空槽 70 | GetFixFleetProc 69 条 | 70 见 06-ocean |
| eFleetShip | 5 | 表长 + 循环 | [确认] | INVALID_FLEETSHIP=-1；空船 `0xD0` | 舰队模板 5 船位；RTC 5×236 | 玩家舰队船位 |
| eShipLoad | 5 | 循环 | [确认] | INVALID_SHIPLOAD=-1 | F-ECO-08 5 船 × 5 格；可用货格 = min(货舱间数, 5) | 每船交易货格 |
| eShipCabin | 31 | 循环 | [确认] | INVALID_SHIPCABIN=-1 | `fn_140006b95` `< 0x1F`；RTC 31×46 | 船上房间**槽位** |
| eCabinKind | 23（0–22） | 判界 + 表长 | [推断] | INVALID_CABINKIND=-1；表内「无」`0x17`，运行时空 `0x18` | 取表 `0x140068f30` `< 0x17`；静态构造 `0x1400256d0` 24 行（第 24 行「无」）；`fn_140007e19` 越界返回 `0x18` | 见 05-ships-fleet/03 |
| eShipMast | **3** | 投票（众数） | [推断] | INVALID_SHIPMAST=-1 | RTC 立即数 3×21（被 `<4` 过滤后投票取 100） | `NUM_SHIPMAST=100` 作废；桅下标 0–2 |
| eFigureHead | 12 | 表长旁证 | [推断] | INVALID_FIGUREHEAD=-1；`CShip+0x53` 空 `0x0D` | FigureHeadID.h 无整数；道具 kind 5 共 12 件（114–123、199 怪鱼像、200 提督像），`+0x53` 比较值 0–11 都有对应道具；无判界 | 见 05-ships-fleet/04 |
| eGunType | 6 | 判界 | [确认] | INVALID_GUNTYPE=-1 | `0x140123300` 判界 | |
| eBattleGun | **21** | 循环（唯一函数） | [推断] | INVALID_BATTLEGUN=-1 | `fn_14000ee8a`（`0x1401c3210`）`cmp eBattleGun, 0x15` | 场景炮对象池下标 0–20；`NUM_BATTLEGUN=5` 投票误采，作废 |
| eShell | 100 | 循环 | [确认] 值；语义 [推断] | INVALID_SHELL=-1 | `fn_14000ee8a` `eShell >= 0x64` | 更像实例池；另有 `eShell >= 4` |
| eMobileShip | 20 | 循环 | [确认] | INVALID_MOBILESHIP=-1 | `fn_14000ee8a` `eMobileShip >= 0x14` | |
| eMobileFleet | 4 | 循环 | [确认] | INVALID_MOBILEFLEET=-1 | `CFleetElement+0x35f0` 存档循环 4 条 | |
| eProxyFleet | `[未知]` | — | — | INVALID_PROXYFLEET=-1 | ProxyfleetID.h 无整数 | |
| eAddParts | 9 | 循环 | [确认] | INVALID_ADDPARTS=-1 | 循环 `< 9`（05-ships-fleet/04） | 改装件 |
| eDeckAddParts | 4 | 投票 | [推断] | | `num_values.json` | |

## 战斗 / 海洋

| 枚举 | 原版计数 | 证据类型 | 等级 | 空/非法 | 来源 |
| --- | --- | --- | --- | --- | --- |
| eBattleAlgo | `[未知]` | — | — | INVALID_BATTLEALGO=-1 | BattlealgoID.h |
| eBattleResult | 5 | 投票 | [推断] | INVALID_BATTLERESULT=-1 | RTC 5×8 |
| eFleetAimKind / FleetState | `[未知]` | — | — | 各 INVALID_*=-1 | 对应头文件 |
| happening 槽 / 结局 | 20 / 0–25（27 空闲） | 判界 | [确认] | 未命中 21；空闲 27 | `fn_1400f55d0` `slot > 0x13` 失败（06-ocean/05） |

## 叙事 / 城镇活动

| 枚举 | 原版计数 | 证据类型 | 等级 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| eScenario | 7 | 表长 | [确认] | INVALID_SCENARIO=-1 | `fn_140009df9` 剧本→势力 7 元素表 `{0,2,1,3,20,13,21}`；RTC 7×19 | 0–3 用 World0，4–6 用 World1：`fn_14000ff60`（body `0x1402a89a0`）`scenario >= 4` 返回 1。重构用剧本记录 `world_pack`，不比较剧本号（D5） |
| eRequest | 69 | 循环 | [确认] | INVALID_REQUEST=-1 | `fn_14000182f`（body `0x1401f0020`）`eRequest < 0x45` | 港口公告槽 |
| eRequestKind | `[未知]` | — | — | INVALID_REQUESTKIND=-1 | RequestkindID.h | |
| eScheme | 24 | 投票 | [推断] | INVALID_SCHEME=-1 | RTC 24×6 | |
| eSchemeKind | 10 | 判界 | [推断] | INVALID_SCHEMEKIND=-1 | 所有校验 `< 0xA`（08-town-life/04） | |
| eLicenseLocationChoices | `[未知]` | — | — | INVALID_LICENSELOCATIONCHOICES=-1 | LicenselocationchoicesID.h | |

## 原版上限（写入 caps.yaml 的默认值）

来源：`srcdata/docs/exe_caps.md`、`srcdata/source/Constants.h`、`constants.json` 的 MAXMIN 符号。键名与 [13-algorithms/02-caps-ranges.md](../13-algorithms/02-caps-ranges.md) 一致；公式参数（`exp_divisor` 等）不进 caps，在 `formulas/`。

| 键 | 原版 | 硬顶（实现） | 说明 |
| --- | --- | --- | --- |
| level_per_track | 100 | 255 | `level=min(isqrt16(exp/30)/256,100)`；exp 30–59 为 0 级；≥256 级时 `q*0x10000` 32 位回绕（F-PROG-01） |
| ability_cap | 100 | 255 | 六维上限（成长后夹；旧名 `ability_growth_cap`） |
| skill_value_cap | 500 | 65535 | 技能值 / Ability 输出夹紧（`fn_1401cbb40`），不夹六维（旧名 `ability_settled_cap`） |
| skill_level_cap | 200 | 两段之和 510 | 技能式里的等级项上限 `min(total_level, 200)`（旧名 `skill_cap`） |
| calendar_ticks_per_day | 48 | | `0x30`，D4 |
| town_develop | 9999 | 65535 uint16 | 商业/军事 |
| cargo_stack | 1 | 显示层曾用 9 | 原版不堆叠 |
| stat_count | 6 | | 六维 |
| fleet_ships | 5 | | 玩家船位 |
| hold_slots_per_ship | 5 | | 交易货格 = min(货物仓库间数, 5)；「扫描 10」是舱室种类 10 的误读 |
| fatigue | FATIGUE_MIN/MAX | | 符号在 constants.json |
| force_amity | FORCEAMITY_MIN/MAX | | |
| force_influence | FORCE_INFLUENCE_MIN/MAX | | |
| share | SHARE_MIN/MAX | | 市场份额 |
| market_price | MARKET_PRICE_MIN/MAX | | |
| ship_crew | SHIP_CREW_MIN/MAX | | |
| ship_durability | SHIP_DURABILITY_MIN/MAX | | |
| wind_dir_adjust | WIND_DIRECTION_ADJUST_MIN/MAX | | |
| wind_vel_adjust | WIND_VELOCITY_ADJUST_MIN/MAX | | |

具体整数未在头文件写出的 MIN/MAX，实现时从断言或 RTC 再填，文档里保持符号名。

不进 caps：存档完整人物记录数 87（`cmp id, 0x57`）是原版存档 schema，只在 `ids/legacy_codec.yaml` 的 `save.persisted_full_person`；98（`SAVE_SAILOR_COUNT`）只是 dk4save 工具值，旧键 `sailor_roster` 作废（词表 §7）。

## 遗留哨兵与存储宽度

原版有两套「空」：

- `INVALID_*=-1`：只出现在 `enum` 与断言 `(0 <= e && e < NUM_*) || e == INVALID_*`，是 int 形参的非法值。
- **数据里的空字节**：存档、World 表、运行时对象里的窄字段，多数取 `NUM+1`，少数取 `NUM` 本身。下表全部是立即数，不是由 NUM 算出来的。

| 枚举 | NUM | 空值 | 与 NUM 关系 | 已见宽度 | 证据 | 等级 |
| --- | --- | --- | --- | --- | --- | --- |
| eTradingItem | 105 | 106 `0x6A` | NUM+1 | 港口特产品槽 **u8**（`fn_140266800` `movzx byte`） | `Constants.h ITEM_EMPTY`；`cmp …,0x6a` 95 处，RTC 多为 `eSpecialProduct`/`eTradingItem` | [确认] |
| eTown | 97 | 98 `0x62` | NUM+1 | `CTownNavigator +0x10/+0x11` 字节 | `CWorldData::vfunc_05` `0x1402aeb15 cmp eax,0x62`；06-ocean | [确认] |
| eItem | 231 | 232 `0xE8` | NUM+1 | 装备字节 u8 | 09-items/01、02；`cmp …,0xe8` 29 处（`ePersonEquip`、`sItemOwner`） | [确认] |
| ePerson | 235 | 236 `0xEC` | NUM+1 | 舰队模板字节；船舱乘员 | `cmp …,0xec` 95 处（`aePerson`、`eShipCabin`）；10-narrative/04 舰队 63–68 | [确认] 值；各字段宽度 [未知] |
| eFleet | 69 | 70 `0x46` | NUM+1 | | 06-ocean；`cmp …,0x46` 49 处（`eFleet`） | [确认] |
| eRoute | 200 | 201 `0xC9` | NUM+1 | | 06-ocean `asConnect` 初值 201 | [确认] |
| eCulture | 21 | 22 `0x16` | NUM+1 | 港口表 `+0x25` u8 | `fn_140013a02` `cmp …,0x16` → 空 | [确认] |
| eRemains | 25 | 26 `0x1A` | NUM+1 | 港口表 `+0x26` u8 | 73 港取 26，其余 0–24 | [确认] 值；「空」语义 [推断] |
| eForce | 25 | 26 `0x1A` | NUM+1 | 人物表 `+0x2A` 字节 | `CForceElement.cpp` `== 0x1a` 分支；`fn_140012fee` / `fn_140004c19` 比 `0x1A`（02-factions 第二轮） | [确认] |
| eForce（份额槽） | 25 | 26 `0x1A` | NUM+1 | 港口表 `+0x4C` 3×(u8 force, u8 值) → `CTown+0x18` | `CTown::vfunc_05`（`0x1402ae4e3`–`54c`）：share 为 0 的槽写 `(0x1A,0)`；开局合约即此初值 | [确认] |
| 流行槽 | 2 槽 | 106 `0x6A` | 交易品空 | 港口表 `+0x7E` 2 格，每格两字节：u8 trading_item + u8 等级 → `CTown+0x70`（原版按 word 拷贝） | World0 97 港全为 `(0x6A, 0)`；`eTownFashion` 循环扫这 2 格；`fn_1400131ce` 读作流行度 | [确认]（`01-world/02`） |
| eShip（实例） | 207 | 208 `0xD0` | NUM+1 | `CFleetShip+0x8`、`CBattleShip+0x8` 字节 | `fn_14000ceaa` 查无返回 `0xD0`；`vfunc_01` `== 0xD0` 判空 | [确认] |
| eShipType（船模） | 39 | 40 `0x28` | NUM+1 | `CShip+0x51` u8 | `vfunc_01` `!= 0x28` 判有效 | [推断] |
| eCabinKind | 23 | 24 `0x18`（表内写 `0x17`） | NUM+1 | 31 槽种类字节 | `CShip::vfunc_05` `0x17 → 0x18`；`fn_14000b898` 先判 `== 0x18` | [确认] |
| eFigureHead | 12 `[推断]` | 13 `0x0D` | NUM+1 | `CShip+0x53` u8 | 初始化写 `0x0D`；`CShipObject::vfunc_09` 查无也回 `0x0D` | [推断] |
| eVillage | 24 | 25 `0x19` | NUM+1 | | `fn_140001186` 空结果 `0x19` | [确认] |
| happening 结局态 | 0–25 | 27 `0x1B` 空闲 | — | `CPlayerFleetData+0x30` u8 | `fn_14000b87a` 读；非 27 不掷新 happening | [确认] 值；「空闲」语义 [推断] |
| 交易品种类数组 | 19 | — | — | `CTown+0x4A` 19×u16 初值 100 | `CTown::vfunc_05` 循环 `< 0x13` | [确认] |

同一个 `0x1A` 在不同字段里意思不同（港口 `+0x26` 遗迹空、人物 `+0x2A` / 份额槽势力空）；`0x6A` 也是 `ePerson_106`，`0x62` 也是 `ePerson_098`。导入 / 导出原版数据时必须按「枚举 × 字段」查表，不能全局把某整数当「空」。旧稿把 `+0x4C` 的 `0x1A` 读成「空流行 = NUM_TOWNFASHION」，已更正：那是份额槽的空势力；流行槽在 `+0x7E`，空值 106。

禁止用 0 当空：0 是南瓜种子（道具）、小麦（交易品）、伦敦（港口）、拉斐尔（人物）。

## 扩展风险

原版整数方案在加内容时会出的问题，按严重程度：

1. **窄宽度封顶**。港口表、装备、舰队模板的 id 是 u8：每个枚举含哨兵最多 256 个值。原版已用到交易品 106、道具 232、人物 236，道具只剩 23 个空位，人物 19 个。凡要导出到原版存档 / DLL 布局的东西都受此限。
2. **哨兵落在可分配区间里**。空值 = NUM 或 NUM+1。按「接在原版后面顺序分配」：第 1 个模组港拿到 97，第 2 个拿到 98＝原版「空港」；第 1 个模组交易品拿到 105，第 2 个是 106＝`ITEM_EMPTY`；第 2 个模组势力拿到 26＝份额槽空势力。任何沿用原版比较（`== 0x62`、`== 0x6A`）的代码或导入脚本都会把真实内容当空。
3. **模组手写大整数号**（旧稿「模组 id 从某个大整数起」）。会超出 u8/u16 存储；两个模组撞号；稀疏下标让「按表长循环」失效。D1 已废弃：模组只写 key，不写整数。
4. **位置语义藏在 id 区间里**。存档对 `ePerson` 0–86 存完整状态、87–234 只存隶属（`cmp 0x57`）；0–97 是「具名可招募」内容区间；0–6 是剧本提督；船实例槽 100–206 = 船体定义 `id-100`、槽 `< 194` 进 NPC 随机船池（`fn_1400f4080`）、0–7 不分配；214–234 是 21 个以港口命名的女性角色（family 字段 = 港名；20 个是首都港，日本圈用长崎而非大阪）；24 是私家舰队；首都 = 每文化圈一个 `is_capital`；遗迹↔港口挂在港口表 `+0x26`。新增可招募人物、第 8 个剧本、第二个私家势力都会撞这些隐含区间。
5. **按下标排的矩阵**。文化表每圈 105 个 u16（按交易品下标，`CCulture::vfunc_05` `< 0x69`）；`CTown+0x4A` 19 个 u16（按交易品种类）；势力 `+0x10` 7 格（按大海域）。新增一个交易品要给 21 个文化圈各补一格，否则越界或读到下一圈的数据。
6. **World0 / World1 共用 ID**。两包 97 港、231 道具、25 势力名全同，人物只有 202 改名；差的是记录字节。若两包各自声明 key，同一港会变成两个实体。

更稳的方案（落在 [05-config-architecture.md](05-config-architecture.md) 已有规则之上）：

- **主键只用 `<pack>:<key>`**。运行时整数是加载期分配的紧凑下标，类型 `u32`；不写进 YAML，不跨存档假定不变。原版包固定 `vanilla_index` = 原枚举，只为对照与导入。
- **空值与计数脱钩**。运行时一律 `Option<Id>`（实现可用 `u32::MAX`），永不使用 NUM 或 NUM+1。原版 98/106/232/236/26/22/70/201 只存在于下面的导入编解码表。
- **遗留编解码表单独成文件**，按「枚举 × 字段」声明，导入 / 导出原版存档与 DLL 时用；导出前校验所有 id 都 `< vanilla count`，否则拒绝导出而不是截断。
- **存档存 key 调色板 + u32 下标**（`id_map`），不存 u8；字段宽度由引擎决定，不由内容数量决定。
- **矩阵改成按 key 的稀疏映射 + 默认规则**：`culture.price_index: { vanilla:wheat: 120, … }`，缺项走 `default_from: trading_item.base_index`；加载时校验覆盖率并告警。
- **隐含区间改成显式字段 / 标签**：`person.recruitable`、`scenario.protagonist`、`force.is_private_fleet`、`town.is_capital`、`town.remains_id`、`person.home_town`、`hull.npc_pool`、`hull.dock_template_of`。名册改成「隶属 == 玩家势力的人物集合」，存档按 key 存全部人物状态，不按 87 / 98 的下标区间。
- **世界包只 patch 同一批 key**：`vanilla-world1` 对 `vanilla:*` 用 `$op: patch` 改记录字节对应的字段，不新建实体。

```yaml
# config/ids/legacy_codec.yaml —— 只给原版存档 / World DLL 导入导出用
schema: dk4.ids.legacy_codec
version: 1
enums:
  trading_item: { vanilla_count: 105, invalid: -1, empty: 106 }
  town:         { vanilla_count: 97,  invalid: -1, empty: 98 }
  item:         { vanilla_count: 231, invalid: -1, empty: 232 }
  person:       { vanilla_count: 235, invalid: -1, empty: 236 }
  fleet:        { vanilla_count: 69,  invalid: -1, empty: 70 }
  route:        { vanilla_count: 200, invalid: -1, empty: 201 }
  culture:      { vanilla_count: 21,  invalid: -1, empty: 22 }
  remains:      { vanilla_count: 25,  invalid: -1, empty: 26 }
  force:        { vanilla_count: 25,  invalid: -1, empty: 26 }
  village:      { vanilla_count: 24,  invalid: -1, empty: 25 }
  ship:         { vanilla_count: 207, invalid: -1, empty: 208 }  # 实例槽
  ship_model:   { vanilla_count: 39,  invalid: -1, empty: 40 }   # [推断]
  cabin_kind:   { vanilla_count: 23,  invalid: -1, empty: 24, also_empty: [23] }  # 表内 0x17，载入改 0x18
  figurehead:   { vanilla_count: 12,  invalid: -1, empty: 13 }   # 0x0D；计数 [推断]（道具 kind 5 共 12 件）
  gun_type:     { vanilla_count: 6,   invalid: -1, empty: 7 }    # 实例 +0x52
  post:         { vanilla_count: 16,  invalid: -1, empty: 17 }   # 0x11：舱室→岗位查无（fn_140012dd2）
fields:          # 「记录 × 偏移」→ 枚举与宽度；同一整数在不同字段里含义不同，只按这里解码
  - { record: world.town, offset: 0x25, enum: culture,  width: u8 }
  - { record: world.town, offset: 0x26, enum: remains,  width: u8 }
  - { record: world.town, offset: 0x28, stride: 6, count: 5, enum: trading_item, width: u8 }
  - { record: world.town, offset: 0x4C, stride: 2, count: 3, enum: force, width: u8 }        # 份额槽 force 字节
  - { record: world.town, offset: 0x7E, stride: 2, count: 2, enum: trading_item, width: u8 }  # 流行槽：两字节 = u8 trading_item + u8 等级（+0x7F）[确认]；原版按 word 拷贝到 CTown+0x70
  - { record: world.person, offset: 0x2A, enum: force, width: u8 }
  - { record: runtime.ship, offset: 0x51, enum: ship_model, width: u8 }
  - { record: runtime.ship, offset: 0x53, enum: figurehead, width: u8 }
  - { record: runtime.fleet_ship, offset: 0x08, enum: ship, width: u8 }
ev_time: { any: 255, year_wildcard_min: 99, month_birthday_sentinel: 12, day_birthday_sentinel: 31 }   # EV 时间条件，见 01-world/07
ev:                              # 叙事导入（10-narrative/02、03、05）；同值不同域
  wildcards: [0xFF, 0xFE]        # 触发条件里的「任意港」等通配
  no_opponent: 0x46              # 触发 kind 1–3 的「无对手」person 字段（与舰队空同值）
  gallery_empty: 0x47
  script_unloaded: 0xFFF
  interpreter_exit: 0x3FFF       # 剧情 Game Over → Session.end_scenario(game_over, cause: scripted)
save:                            # 原版存档成帧（12-ui-save/02）；不是上限，不进 caps
  persisted_full_person: 87      # id < 87 存完整记录（cmp id, 0x57）
on_export_overflow: refuse       # 越界拒绝导出，不截断
on_import_unknown: refuse        # 导入时出现不在 [0,count) ∪ {empty} 的值
```

船系统的存档字段（船记录 `+0x50` 人物、`+0x54` 所在港、`+0x58` 货格）与槽分段 `ship_slot_bands` 见 [05-ships-fleet/01](../05-ships-fleet/01-ship-model.md) §6，合并进同一个 `legacy_codec.yaml`。

## 剧本主角（ePerson 0–6）

| id | 名 | 姓 | 第三名 | 六维 体敏魅智神运（World0） |
| --- | --- | --- | --- | --- |
| 0 | 拉斐尔 | 卡斯特罗 | | 59 60 70 62 58 61 |
| 1 | 赫德拉姆 | 柏格斯通 | 约阿其姆 | 83 69 78 55 73 48 |
| 2 | 丽璐 | 阿歌特 | | 49 54 69 70 78 66 |
| 3 | 华梅 | 李 | 玛莉亚 | 66 58 81 86 69 78 |
| 4 | 杏太郎 | 佐伯 | | 79 59 80 53 71 51 |
| 5 | 亚伯拉罕 | 伍丁 | 易文 | 78 85 60 55 81 60 |
| 6 | 蒂雅 | 恰斯卡 | 瓦曼 | 45 61 68 81 62 55 |

来源：World0 `GetFixPersonProc` 记录。World1 中 1 赫德拉姆 `82 68 77 54 73 47`、2 丽璐 `48 53 68 70 77 66`、3 华梅 `66 58 80 85 68 77`，其余四人相同（`World1_person.c`；全表差异见 `15-catalogs/persons.md`）。

## 21 文化圈与首都

港口表 `+0x25` 是 `culture_id` `[确认]`（见上文 eCulture）；`+0x24` 非 0 的 21 港恰好各占一圈。「首都」这个叫法 `[推断]`。

| culture | exe 名 | 首都港 |
| --- | --- | --- |
| 0 | 英吉利 | 0 伦敦 |
| 1 | 佛兰德 | 2 阿姆斯特丹 |
| 2 | 德意志 | 5 汉堡 |
| 3 | 北欧 | 7 斯德哥尔摩 |
| 4 | 葡萄牙 | 11 里斯本 |
| 5 | 西班牙 | 13 塞维利亚 |
| 6 | 意大利 | 15 热那亚 |
| 7 | 希腊 | 19 雅典 |
| 8 | 土耳其 | 22 伊斯坦布尔 |
| 9 | 埃及 | 25 亚历山大 |
| 10 | 西非 | 29 圣乔治 |
| 11 | 东非 | 34 索法拉 |
| 12 | 阿拉伯 | 38 巴士拉 |
| 13 | 印度 | 42 卡利卡特 |
| 14 | 印度支那 | 47 马六甲 |
| 15 | 印尼 | 50 雅加达 |
| 16 | 中国 | 54 杭州 |
| 17 | 朝鲜 | 57 汉城 |
| 18 | 日本 | 59 大阪 |
| 19 | 加勒比海 | 61 哈瓦那 |
| 20 | 墨西哥 | 65 委拉克鲁斯 |

名字来源：exe 静态表 `0x1405b6160` 每条 `+0x00` 指针（本任务重新解出，与 06-seven-culture-circles 一致）。`eCountry` 与 `eCulture` 不是同一枚举；eCountry 计数仍 `[未知]`。
