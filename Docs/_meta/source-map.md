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
| 符号计数 | `srcdata/catalog/constants.json` `num_values.json`（部分值是 `tools/fill_gaps.py` 按 RTC 立即数众数猜的，引用须注明证据类型：判界 / 循环上界 / 投票；已知误采 `NUM_SHIPTYPE` `NUM_BATTLEGUN` `NUM_SHIPMAST` `NUM_HOLD_SCAN`） |
| 消息号 → 文案 | exe `.data 0x140683620`（37 块 `{first_id, size}`）+ `0x1406836c0`（每条 u16 偏移）；文案 key 用消息号，不用 mesfile 字节偏移 |
| 架构决策 / 规范接口名 / key 词表 | `Docs/00-overview/09-architecture-decisions.md`、`06-system-interfaces.md`、`10-key-glossary.md` |
| 枚举 | `srcdata/source/*ID.h` |
| 反编译实现 | `srcdata/source/game/<feature>/*.cpp` |

## 地址与文件偏移

| 段 | 文件偏移 | 备注 |
| --- | --- | --- |
| `.text` | VA − `0x140000A00` | `0x140001000`–`0x1400251f0` 是 5 字节 `jmp` 跳板，文档里 `fn_14000xxxx` 多为 thunk 名，body 查 `functions.tsv` |
| `.rdata` | VA − `0x140001200` | 字符串、常量表。按 `.text` 公式算会读错位置 |
| `.data` | VA − `0x140001600` | 只有前 `0x12400` 字节在文件里；`0x1442…` 一带全局在运行时才有 |

反编译伪影：`cdq; sub; sar` 前的寄存器残留、上一调用的参数寄存器常被反编译当成公式常数（计略对抗式的「−8」、半速键 `0x11`、探索 `-5`）；`imul` 立即数、`idiv` 除数也常丢。关键立即数一律回原始指令核对。

最省事：`objdump -d -M intel --start-address=<VA> --stop-address=<VA> Game/DK4HD_sc.exe` 看代码，`objdump -s --start-address=… --stop-address=…` 看数据表，`python3 srcdata/tools/lookup.py <addr|类名>` 查归属。来源：`objdump -h Game/DK4HD_sc.exe`。

## 功能目录

`srcdata/source/game/` 下：`combat` `event` `fleet` `item` `ocean` `person` `save` `ship` `town` `trade` `ui` `engine` `world` `game`。

## 大佬表

`/home/claude/data/config/dh4/*.png` 对照结果：[_meta/designer-spreadsheets.md](designer-spreadsheets.md)。

## 设计文档

成品在 `Docs/`。入口 `Docs/README.md`。系统文档按九段模板，配置示例在 `Docs/14-config-schemas/examples/`。

## 已抽出数据

| 文件 | 内容 |
| --- | --- |
| `Docs/_data/towns_world0.json` | 97 港：坐标、`culture_id`、`remains_id`、`byte_0x27`、首都标记、5 个特产品槽（`item_id` / `listing_threshold` / `supply_base`） |
| `Docs/_data/persons_world0.json` | 235 人：姓名、六维、hex |
| `Docs/_data/items_world0.json` | 231 道具名+hex |
| `Docs/_data/forces_world0.json` | 25 势力名+hex |
| `Docs/_data/fleets_world0.json` | 69 舰队 hex |
| `Docs/_data/world1_names.json` | World1 对照名 |
| `Docs/_data/*.tsv` | 同上的扁平表 |
| `Docs/_data/constants.json` | 符号计数副本 |

## 坐标注意

伦敦 x1=20080 y1=4272（原版坐标单位）。`World.dat` 是 2500×1250 **uint16** 贴图编号。航行坐标 `/16`（有符号、向零截断）→ chip 下标（`fn_140014a74` / `fn_14000a9e8`，`cdq; and 0xf; sar 4`），X 在 2500 环绕；港口 `x1,y1` 与航行坐标同空间是强 `[推断]`。见 `01-world/01`、`06`。不要除 8。
