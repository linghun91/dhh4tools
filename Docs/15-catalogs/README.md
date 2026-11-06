# 15 原版内容目录

这些是**原版内容包（vanilla pack）的实例表**，不是系统设计，也不是运行时 YAML。

- 默认内容包 = World0 表 + SC mesfile + exe 交易品名。模组另起包，不改这些 ID。
- 整数 ID 与原版枚举稳定一致：`eTown` / `ePerson` / `eItem` / `eForce` / `eTradingItem`。存档、DLL、mesfile 对照都用这套 ID。
- `NUM_*` 是原版包的实例计数，运行时读表长，不把 97/235/231/25/105 写进引擎循环。
- World0 / World1 是同一 schema 的两份数据。本目录以 World0 为默认包；World1 仅在姓名或六维不同时另注。
- 空货格 / 空特产品哨兵是 `106`（`0x6A`，`ITEM_EMPTY`），不在 `0–104` 内，也不是道具 0。

| 文件 | 枚举 | 行数 | 空/非法 |
| --- | --- | --- | --- |
| [towns.md](towns.md) | eTown | 97 | INVALID_TOWN=-1 |
| [persons.md](persons.md) | ePerson | 235 | INVALID_PERSON=-1 |
| [items.md](items.md) | eItem | 231 | INVALID_ITEM=-1 |
| [forces.md](forces.md) | eForce | 25 | INVALID_FORCE=-1 |
| [trading-items.md](trading-items.md) | eTradingItem | 105 | INVALID=-1；empty=106 |

系统文档（职责、公式、YAML schema）在 `Docs/01-world` … `Docs/14-config-schemas`。本目录只给对照用的原版行。

抽出表：`Docs/_data/*.json` `*.tsv`。头文件：`srcdata/source/TownID.h` `PersonID.h` `ItemID.h` `ForceID.h` `TradingItemID.h` `TradingItemKindID.h`。
