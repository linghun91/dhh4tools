# 15 原版内容目录

这些是**原版内容包（vanilla pack）的实例表**，不是系统设计，也不是运行时 YAML。

- 默认内容包 = World0 表 + SC mesfile + exe 交易品名。模组另起包，不改这些 ID。
- 表里的整数是 `vanilla_index`，与原版枚举一致：`eTown` / `ePerson` / `eItem` / `eForce` / `eTradingItem`。原版存档、DLL、mesfile 对照用它；新引擎的主键是 `vanilla:<key>`，整数只是加载期下标（[09-architecture-decisions.md](../00-overview/09-architecture-decisions.md) D1、[05-config-architecture.md](../00-overview/05-config-architecture.md)「ID 与命名空间」）。
- `NUM_*` 是原版包的实例计数，运行时读表长，不把 97/235/231/25/105 写进引擎循环。这五个计数都是**表长**证据（DLL `GetFix*` 条数 / exe 名表条数），见 04-id-registry「计数的证据类型」。
- World0 / World1 是同一 schema 的两份数据。本目录以 World0 为默认包；World1 仅在姓名或六维不同时另注。
- 空货格 / 空特产品哨兵是 `106`（`0x6A`，`ITEM_EMPTY`），不在 `0–104` 内，也不是道具 0。它是原版字节，运行时空值是 `null`，106 只在 `ids/legacy_codec.yaml`（D1）。
- 其它表的空字节各不相同（港 98、道具 232、人物 236、文化 22、遗迹 26、份额槽空势力 26、流行槽 106），全表见 [04-id-registry.md](../00-overview/04-id-registry.md)「遗留哨兵与存储宽度」。这些值只用于原版数据导入导出；新引擎用 `Option<Id>`，不按 NUM 推算。
- 本目录的表与 `Docs/_data` 由脚本逐格比对过（World0/World1 `.c`、`world_tables.json`、`*ID.h`、DLL 原字节）。

| 文件 | 枚举 | 行数 | 空/非法 |
| --- | --- | --- | --- |
| [towns.md](towns.md) | eTown | 97 | INVALID_TOWN=-1；空 98 |
| [persons.md](persons.md) | ePerson | 235 | INVALID_PERSON=-1；空 236 |
| [items.md](items.md) | eItem | 231 | INVALID_ITEM=-1；空 232 |
| [forces.md](forces.md) | eForce | 25 | INVALID_FORCE=-1；空 26 `[确认]` |
| [trading-items.md](trading-items.md) | eTradingItem | 105 | INVALID=-1；empty=106 |

系统文档（职责、公式、YAML schema）在 `Docs/01-world` … `Docs/14-config-schemas`。本目录只给对照用的原版行。

抽出表：`Docs/_data/*.json` `*.tsv`。`towns_world0.json` 键名已语义化（`culture_id`、`remains_id`、`listing_threshold`、`supply_base`），旧键对照见 [towns.md](towns.md)。`constants.json` 的 `values` 已修正投票误采项，`evidence` 给每个 `NUM_*` 的证据类型。头文件：`srcdata/source/TownID.h` `PersonID.h` `ItemID.h` `ForceID.h` `TradingItemID.h` `TradingItemKindID.h`。
