# 势力目录（eForce）

原版内容包。`NUM_FORCE=25`，`INVALID_FORCE=-1`。来源：World0 `GetFixForceProc`，`Docs/_data/forces_world0.json`，`srcdata/source/ForceID.h`。

id 0–23 具名势力，id 24 是私家舰队。World1 25 个中文名与 World0 **全部相同**；但 25 条记录的非名字字节都不同（母港、资金、关系等，见 [04-world0-world1-diff.md](../10-narrative/04-world0-world1-diff.md)）。

空势力字节 `0x1A=26`（`NUM_FORCE+1`）`[确认]`：`CTown::vfunc_05` 拷港口份额槽时把值为 0 的槽写成 `(0x1A, 0)`（`01-world/02`、`04-id-registry`）；`CForceElement.cpp` 月处理里另有 `== 0x1a` 早退分支（取值函数 `fn_140012788` 的语义未钉死）；World 人物表 `+0x2A` 也用 26 当无隶属（见 10-narrative 04）。不是 `-1`，也不是 25。26 是原版字节，运行时空值是 `null`，只在 `ids/legacy_codec.yaml` 编解码（D1）。私家舰队（id 24）用显式字段 `is_private_fleet`，不按 id 判断。

记录其余字节（资金、关系、颜色等）仍在 `forces_world0.json` 的 `hex` 里，字段名未全部还原，本表不猜。

| id | name |
| --- | --- |
| 0 | 卡斯特罗商会 |
| 1 | 阿歌特商会 |
| 2 | 柏格斯通商会 |
| 3 | 李家 |
| 4 | 克利福德军 |
| 5 | 舒派亚商会 |
| 6 | 阿博科鲁克军 |
| 7 | 巴鲁迪斯军 |
| 8 | 陈特利欧商会 |
| 9 | 巴夏军 |
| 10 | 海雷丁家 |
| 11 | 西鲁韦拉商会 |
| 12 | 埃斯皮诺沙商会 |
| 13 | 伍丁商会 |
| 14 | 纳哥普尔商会 |
| 15 | 普雷依拉商会 |
| 16 | 库恩商会 |
| 17 | 来岛家 |
| 18 | 玛尔德纳德军 |
| 19 | 埃斯康特军 |
| 20 | 佐伯家 |
| 21 | 昆廷韦拉斯 |
| 22 | 图鲁维队 |
| 23 | 朝鲜水师 |
| 24 | 私家舰队 |

## 计数

- 行：25（id 0–24）
- World1 名称不同：0
