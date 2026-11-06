# 源数据地图

给写文档的人用。所有路径相对仓库根 `/home/claude/data/config/dk4hd`。

## 必读

| 问题 | 打开 |
| --- | --- |
| 怎么找文件 | `srcdata/README.md` `srcdata/INDEX.md` |
| 原版架构 | `srcdata/docs/architecture.md` |
| 上限与公式补丁 | `srcdata/docs/exe_caps.md` |
| 交易堆叠与槽布局 | `srcdata/docs/trade_stack.md` `srcdata/include/trade.h` |
| 存档 | `srcdata/docs/save_format.md` `srcdata/include/save.h` `srcdata/source/save/Save.cpp` |
| 资源格式 | `srcdata/docs/formats.md` |
| 世界表 C 接口 | `srcdata/source/world/DllMain.cpp` `srcdata/include/world.h` |
| 世界表字节 | `srcdata/source/world/World0_*.c` |
| 世界表 JSON | `srcdata/catalog/world_tables.json` |
| 抽出的干净表 | `Docs/_data/*.json` `Docs/_data/*.tsv` |
| 文案 | `srcdata/catalog/mesfile_sc.txt` |
| 类继承 | `srcdata/catalog/class_hierarchy.txt` |
| 类→cpp | `srcdata/catalog/class_files.tsv` |
| 符号计数 | `srcdata/catalog/constants.json` `num_values.json` |
| 枚举 | `srcdata/source/*ID.h` |
| 反编译实现 | `srcdata/source/game/<feature>/*.cpp` |

## 功能目录

`srcdata/source/game/` 下：`combat` `event` `fleet` `item` `ocean` `person` `save` `ship` `town` `trade` `ui` `engine` `world` `game`。

## 大佬表

`/home/claude/data/config/dh4/*.png` 对照结果：[_meta/designer-spreadsheets.md](designer-spreadsheets.md)。

## 设计文档

成品在 `Docs/`。入口 `Docs/README.md`。系统文档按九段模板，配置示例在 `Docs/14-config-schemas/examples/`。

## 已抽出数据

| 文件 | 内容 |
| --- | --- |
| `Docs/_data/towns_world0.json` | 97 港：坐标、region、首都标记、5 个特产品槽 |
| `Docs/_data/persons_world0.json` | 235 人：姓名、六维、hex |
| `Docs/_data/items_world0.json` | 231 道具名+hex |
| `Docs/_data/forces_world0.json` | 25 势力名+hex |
| `Docs/_data/fleets_world0.json` | 69 舰队 hex |
| `Docs/_data/world1_names.json` | World1 对照名 |
| `Docs/_data/*.tsv` | 同上的扁平表 |

## 坐标注意

伦敦 x1=20080 y1=4272。`World.dat` 是 2500×2500。这些整数**不是**网格下标，换算公式未还原，文档里当「原版坐标单位」保存，不要擅自除 8。
