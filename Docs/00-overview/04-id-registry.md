# ID 注册表

来源：`srcdata/source/*ID.h`、`srcdata/catalog/constants.json`、`srcdata/catalog/num_values.json`、World GetFix* 表。计数凡未在头文件写死的，标 `[推断]` 或 `[未知]`。

运行时以表长为准。本表是原版内容包的实例计数。

## 世界

| 枚举 | 原版计数 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| eTown | 97 | INVALID_TOWN=-1 | GetFixTownProc | 名称见 `Docs/_data/towns.tsv` |
| eTownShare | 97 | | TownShareID.h | 与港口 1:1 |
| eVillage | 24 | INVALID_VILLAGE=-1 | VillageID.h / RTC | |
| eSeaArea | 25 | INVALID_SEAAREA=-1 | SeaAreaID.h | 与 NUM_NEXTBASETOWN=25 同数 |
| eRoute | 200 | INVALID_ROUTE=-1 | RouteID.h / RTC | |
| eJunction | `[未知]` | INVALID_JUNCTION | JunctionID.h | |
| eRemains | `[未知]` | INVALID_REMAINS | RemainsID.h | |
| eMapPointKind | 12 | INVALID_MAPPOINTKIND | RTC NUM_MAPPOINTKIND | |
| eZodiac | `[未知]` | INVALID_ZODIAC | ZodiacID.h | |
| eWallsky | `[未知]` | INVALID_WALLSKY | WallskyID.h | |

## 势力 / 文化

| 枚举 | 原版计数 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| eForce | 25 | INVALID_FORCE=-1 | GetFixForceProc | 0–23 具名势力，24 私家舰队 |
| eCountry | `[未知]` | INVALID_COUNTRY | CountryID.h | 港口 `region` 字节 0–20，共 21 档 `[推断]` |
| eCulture | `[未知]` | INVALID_CULTURE | CultureID.h | NPC 文案出现 9 个文化圈 |
| eTownFashion | 26 | | TownFashionID.h | |
| eForceKind / Aim / Color / Relation / Failure | `[未知]` | 各 INVALID_* | 对应 *ID.h | |

## 人物

| 枚举 | 原版计数 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| ePerson | 235 | INVALID_PERSON=-1 | GetFixPersonProc | 0–6 剧本提督；98 起大量职能 NPC |
| eAbility | 6 | INVALID_ABILITY=-1 | AbilityID.h | 体力 敏捷 魅力 智力 精神 运气 |
| eSkill | 10 | INVALID_SKILL=-1 | SkillID.h | 生命力 统率力 驾驶术 测量术 会计术 剑术 命中力 说服力 谋略术 观察力 |
| eJobKind | `[未知]` | INVALID_JOBKIND | JobkindID.h | |
| ePost | exe 至少 16 名 | INVALID_POST | PostID.h + exe `0x58568c` | 舰长单列；其余 15：副官 参谋 会计长 测量员 操帆手 舵手 了望手 冲锋队长 炮手 传教士 船医 大厨 船木工 饲养员 甲板主管 |
| ePersonEquip | 5 | | PersonEquipID.h / RTC | |
| eSex | `[未知]` | INVALID_SEX | SexID.h | |
| eMentalState | `[未知]` | | MentalstateID.h | MENTALLEVEL_MIN/MAX |
| ePhysicalState | `[未知]` | | PhysicalstateID.h | PHYSICALLEVEL_MIN/MAX |

存档名册：`SAVE_SAILOR_COUNT=98`（`srcdata/source/Constants.h`），步长 0x19。与 235 人物表不是同一集合：名册是可入队水手槽。

## 经济

| 枚举 | 原版计数 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| eTradingItem | 105 | INVALID_TRADINGITEM=-1；空格 0x6A=106 | TradingItemID.h, ITEM_EMPTY | 106 不在 0–104 内，是哨兵。全名见 `Docs/15-catalogs/trading-items.md`（小麦…鸟粪石） |
| eTradingItemKind | 19 | INVALID_TRADINGITEMKIND=-1 | TradingItemKindID.h | |
| eSpecialProduct | 5 | | SpecialProductID.h | **每港** 5 个特产品槽 |
| eItem | 231 | INVALID_ITEM=-1 | GetFixItemProc | 装备/发现物/地图，不是交易品 |

## 船 / 舰队

| 枚举 | 原版计数 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| eShip | 100 | INVALID_SHIP=-1 | ShipID.h | |
| eShipType | 5 | INVALID_SHIPTYPE=-1 | ShipTypeID.h | |
| eShipGrade | `[未知]` | | ShipgradeID.h | |
| eFleet | 69 | INVALID_FLEET=-1 | GetFixFleetProc | |
| eFleetShip | 5 | INVALID_FLEETSHIP=-1 | FleetShipID.h | 玩家舰队船位 |
| eShipLoad | 5 | INVALID_SHIPLOAD=-1 | ShipLoadID.h | 每船交易货格 |
| eShipCabin | 31 | | ShipCabinID.h / RTC | 船上房间**槽位** |
| eCabinKind | RTC 23/24 | | CabinKindID.h | exe 24 名：桅杆…无，见人物/船文档 |
| eShipMast | 100 | | ShipMastID.h | |
| eFigureHead | `[未知]` | | FigureHeadID.h | |
| eGunType | 6 | INVALID_GUNTYPE=-1 | GunTypeID.h RTC | |
| eBattleGun | 5 | | BattleGunID.h | 每船炮位？ |
| eShell | 100 | INVALID_SHELL=-1 | ShellID.h | |
| eMobileShip | 20 | | MobileShipID.h / RTC | |
| eMobileFleet | 4 | | MobileFleetID.h / RTC | |
| eProxyFleet | `[未知]` | | ProxyfleetID.h | |
| eAddParts | 9 | | RTC NUM_ADDPARTS | 改装件 |

## 战斗 / 海洋

| 枚举 | 原版计数 | 空/非法 | 来源 |
| --- | --- | --- | --- |
| eBattleAlgo | `[未知]` | | BattlealgoID.h |
| eBattleResult | 5 | | BattleResultID.h |
| eFleetAimKind / FleetState | `[未知]` | | 对应头文件 |

## 叙事 / 城镇活动

| 枚举 | 原版计数 | 空/非法 | 来源 | 备注 |
| --- | --- | --- | --- | --- |
| eScenario | 7 | INVALID_SCENARIO=-1 | ScenarioID.h | 0–3 用 World0，4–6 用 World1（`fn_14000ff60`） |
| eRequest | 69 | INVALID_REQUEST=-1 | RequestID.h / RTC | |
| eRequestKind | `[未知]` | | RequestkindID.h | |
| eScheme | 24 | | SchemeID.h / RTC | |
| eSchemeKind | `[未知]` | | SchemekindID.h | |
| eLicenseLocationChoices | `[未知]` | | LicenselocationchoicesID.h | |

## 原版上限（写入 caps.yaml 的默认值）

来源：`srcdata/docs/exe_caps.md`、`srcdata/source/Constants.h`、`constants.json` 的 MAXMIN 符号。

| 键 | 原版 | 硬顶（实现） | 说明 |
| --- | --- | --- | --- |
| level_per_track | 100 | 255 | `level=isqrt(exp/30)`，再高 32 位开方溢出成 0 |
| exp_divisor | 30 | | 经验公式除数 |
| ability_growth_cap | 100 | 255 | 基础属性成长 |
| ability_settled_cap | 500 | 65535 | 结算后属性 |
| skill_cap | 200 | 两段之和 510 | 技能公式封顶 |
| town_develop | 9999 | 65535 uint16 | 商业/军事 |
| cargo_stack | 1 | 显示层曾用 9 | 原版不堆叠 |
| sailor_roster | 98 | | 存档名册条数 |
| stat_count | 6 | | 六维 |
| fleet_ships | 5 | | 玩家船位 |
| hold_slots_per_ship | 5 | | 交易货格；扫描 10 |
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

## 哨兵对照

| 用途 | 原版值 | 重构 key |
| --- | --- | --- |
| 空货格 / 空特产品 | 106 (0x6A) | `trading_item.empty` |
| 非法枚举 | -1 | `invalid` |
| 未发现遗迹等 bitfield | *_STATE_BITS | 各状态文档 |

禁止用 0 当空货格：0 是南瓜种子（道具）或交易品表的第一项。

## 剧本主角（ePerson 0–6）

| id | 名 | 姓 | 第三名 | 六维 体敏魅智神运 |
| --- | --- | --- | --- | --- |
| 0 | 拉斐尔 | 卡斯特罗 | | 59 60 70 62 58 61 |
| 1 | 赫德拉姆 | 柏格斯通 | 约阿其姆 | 83 69 78 55 73 48 |
| 2 | 丽璐 | 阿歌特 | | 49 54 69 70 78 66 |
| 3 | 华梅 | 李 | 玛莉亚 | 66 58 81 86 69 78 |
| 4 | 杏太郎 | 佐伯 | | 79 59 80 53 71 51 |
| 5 | 亚伯拉罕 | 伍丁 | 易文 | 78 85 60 55 81 60 |
| 6 | 蒂雅 | 恰斯卡 | 瓦曼 | 45 61 68 81 62 55 |

来源：World0 `GetFixPersonProc` 记录。World1 可能不同，对照 `Docs/_data/world1_names.json`。

## 21 个首都 `[推断]`

港口表 +0x24 为 1 且 +0x25=region 的港口：

| region | 首都 |
| --- | --- |
| 0 | 伦敦 |
| 1 | 阿姆斯特丹 |
| 2 | 汉堡 |
| 3 | 斯德哥尔摩 |
| 4 | 里斯本 |
| 5 | 塞维利亚 |
| 6 | 热那亚 |
| 7 | 雅典 |
| 8 | 伊斯坦布尔 |
| 9 | 亚历山大 |
| 10 | 圣乔治 |
| 11 | 索法拉 |
| 12 | 巴士拉 |
| 13 | 卡利卡特 |
| 14 | 马六甲 |
| 15 | 雅加达 |
| 16 | 杭州 |
| 17 | 汉城 |
| 18 | 大阪 |
| 19 | 哈瓦那 |
| 20 | 委拉克鲁斯 |

region 字节是否等于 eCountry / eCulture，尚未从 CTown 字段名确认。
