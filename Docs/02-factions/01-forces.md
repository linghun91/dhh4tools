# 势力实体（CForce）

本篇管「谁是一个势力」：25 个具名 `eForce`、World 表 0x58 记录、静态定义与运行时状态的分界。友好度、份额、合约、上缴见 [03-relations-influence.md](03-relations-influence.md)。国家 / 文化 / 流行见 [02-countries-culture-fashion.md](02-countries-culture-fashion.md)。

## 1. 职责边界

**管：**

- 势力作为可存档实体：ID、显示名、种类、领袖人物、母港、起始金币
- World0 / World1 两份内容包里的 25 条实例
- 势力集合容器 `CForceElement`（25 个 `CForce` + 附属失败/战果槽）
- 地图着色用的 `eForceColor`（8 色）
- 运行时「此势力是否已出场」（由母港是否为空派生）

**不管：**

- 港口坐标、海域、航线 → `world`
- 人物六维、招募 → `characters`（本系统只持 `leader_person_id`）
- 市价、买卖公式 → `economy`
- 海战拿港、接舷 → `combat`（本系统只收事件）
- 设施对话、广场 NPC → `town-life`
- 友好度 / 份额 / 合约 / 上缴的规则 → 本目录 `03-relations-influence.md`

系统之间只传 `force_id`、`town_id`、`person_id` 和标量，不传 `CForce*`。

## 2. 原版事实

- `[确认]` `eForce` 0–24 共 25 个，`INVALID_FORCE = -1`，`NUM_FORCE = 25`。证据类型：**容器计数 + 循环上界**（`CForceElement` ctor `fn_140007dce(..., 0x58, 0x19)`、存盘/月结循环 `>= 0x19`）；头文件整数是 `fill_gaps.py` 的 `KNOWN_NUM` 手填，不单独作证据。来源：`srcdata/source/ForceID.h`、`GetFixForceProc` 断言。
- `[确认]` 第 24 号（id=24）中文名为「私家舰队」。0–23 为具名商会 / 家 / 军。来源：`ForceID.h`、`Docs/_data/forces_world0.json`。
- `[确认]` World DLL 记录步长 **0x58 = 88 字节**。`DllMain.cpp` 的 `SForce`：前 4 字节是 GBK 名指针，其余 0x50 未在头文件命名。`CForceElement` 构造把 25 个对象按 stride `0x58` 排在 `+8`。来源：`World0_force.c`、`CForceElement.cpp` ctor、`Force.h` 注释。
- `[确认]` `CForce : CSaveObject`。`CForce::vfunc_05` 先把 `GetFixForceProc` 的 88 字节 `rep movsb` 到栈上 `r_sFixForce`，再**逐字段**拷到对象同偏移：`+0x08/+0x09/+0x0A` 各 1 字节、`+0x0C` dword、`+0x10` 7 word、`+0x1E` 1 字节、`+0x1F` 与 `+0x38` 各 25 字节；然后写 `+0x51/+0x52/+0x53`。World 表 `+0x00..+0x07`（名字指针、`+0x04` 的 1）和 `+0x51..+0x57` **不拷**。`vfunc_06` / `vfunc_07` 是存盘 / 读盘。来源：`srcdata/source/game/world/CForce.cpp`、objdump `0x1402aceb6`。
- `[确认]` 存盘读写的偏移（`vfunc_06/07`，循环上界 0x19=25、7 个 word）：

  | 偏移 | 宽度 | 次数 | IO |
  | --- | --- | --- | --- |
  | +0x0A | 1 | 1 | 字节 |
  | +0x0C | 4 | 1 | `fn_14000def9` / `fn_14000f240`（与金币 uint32 同宽） |
  | +0x10 | 2 | 7 | word |
  | +0x1E | 1 | 1 | 字节 |
  | +0x1F+i | 1 | 最多 25 | 仅当 `fn_140010bc7(self, i)` 真，即 `self.id < i` |
  | +0x38+i | 1 | 最多 25 | 同上 |
  | +0x51 | 1 | 1 | 字节 |
  | +0x52 | 1 | 1 | 字节 |
  | +0x53 | 1 | 1 | 字节 |

- `[确认]` `vfunc_04`：`+0x0A == 0x62` 视为空。`0x62 = 98`，即 `NUM_TOWN=97` 越界的「计数+1」，World 表里母港也用 98 表示「无港」。（与 dk4save 的「98 行名册」只是数字巧合：引擎存档人物段是 `0x2C03` 起 235 条记录（`CPersonElement::vfunc_01` 循环 `0xeb`），id < 87 存完整记录、其余只存隶属与 `+0x22`，见 `12-ui-save/02-save-session.md`。玩家麾下 = 隶属 == 玩家势力的人物集合，没有「98 名册」。）来源：`CForce.cpp`、`World0_force.c`。
- `[确认]` 初始化把 `+0x51 = 0x1A`、`+0x52 = 4`、`+0x53 = 0x1A`。`0x1A = 26` 在多处当「非 0–24 的空势力」哨兵（与头文件 `INVALID_FORCE=-1` 并存）。来源：`CForce::vfunc_05`、`CForceElement::vfunc_04`（`s64 == 0x1a` 则跳过）、`CForceFailure`。重构里 26 / 98 只出现在 `config/ids/legacy_codec.yaml`（导入导出原版格式），运行时与内容表一律 `null`。
- `[确认]` 母港 `+0x0A` 的运行时写入点：份额加成 `fn_1400122f6` 内对任何势力，若该势力 `+0x0A == 0x62` 则 `fn_1400092ff` 把本港写成母港；新合约、投资优待、卖流行品加份额、占领 +10 都走这条。来源：`04-economy/07-investment-tribute.md` §4.3、`07-combat/04-ship-vs-port.md` §2.8。灭亡时写回 `0x62`（05 §2.5）。
- `[确认]` `CForceElement` 还构造：`+0x8A0` 起 7 个 stride `0x10` 的 `CForceFailure`；`+0x910` 起 5 个 stride `0x10` 的战果槽（RTC `eBattleResult`，`NUM_BATTLERESULT=5`）。来源：ctor 里 `fn_140007dce`。
- `[确认]` 7 个 `CForceFailure` 按 **`major_sea_id`** 下标：`fn_140006573(i)` → `fn_1400bba00`：`CForceElement + 0x8A0 + i*0x10`；调用者用 RTC `eSeaArea`（上界 7）或 `town → culture → major_sea_id`（`fn_140014e48`）当 `i`。**不是** `NUM_SCENARIO`。见 [06](06-seven-culture-circles.md) §2.2。
- `[确认]` 关系 / 友好度是**半矩阵**：读 `fn_140012dc8`（关系）/ `fn_140008f4e`（友好度）在 `self.id < other` 时读 `self[+0x1F/+0x38 + other]`，否则交换双方递归；`a == b` 不读。写 `fn_140004e08` 同样只写低 id 一侧；存盘也只存 `self.id < i` 的格。World 表 `other < self` 的下三角**从来不读**。来源：`fn_140107150.cpp`、`fn_14010be70.cpp`、`fn_140010bc7`（body `0x14010c230`）。
- `[确认]` 金币加减 `fn_140002cb6`（body `0x140106490`）：加时 `min(gold + d, 0x5F5E0FF)`（**99,999,999**），减时不足则置 0。`fn_14001117b` = 减。
- `[确认]` 剧本 → 势力：`fn_140009df9`（RTC `aeForce`）常量表 `[0, 2, 1, 3, 20, 13, 21]`，即剧本 0–6 → 卡斯特罗、柏格斯通、阿歌特、李家、佐伯、伍丁、昆廷韦拉斯。`fn_140004381` = 当前玩家势力 = 该表[`DAT_144296500+0x7C`]。`[确认]` `+0x7C` 是 `eScenario`：7 元素表、`fn_14000864d` 以 `>= 4` 判 World1、`fn_140010f46` 原样返回它（`fn_14000d486` 是恒等）。见 `04-economy/07-investment-tribute.md` §2.1。重构里这张表是剧本包字段 `scenarios[].player_force`（narrative 拥有），factions 只查询。
- `[确认]` `eForceColor` 共 8，`INVALID_FORCECOLOR=-1`。证据类型：**循环上界**——`fn_1400018b1`（body `0x14020f020`，RTC `eForceColor @ +0x2b4`）`cmp [rsp+0x2b4], 8`（`0x14020f919`）。`num_values.json` 的 8 是 RTC 投票，单独不作证据。
- `[确认]` `NUM_FORCEKIND` / `NUM_FORCEAIMKIND` / `NUM_FORCERELATION` / `NUM_FORCEFAILUREKIND` 只有符号，整数未还原。来源：对应 `*ID.h`、`asserts.txt`。
- `[确认]` World1 25 个名字与 World0 相同，数值不同（母港、金币、7×uint16、少数友好度、`+0x1E`）。来源：`World1_force.c`。
- `[确认]` UI：`CForceInformationView`（舰队侧势力情报）、`CSeaAreaForceView`（海域）、`CForceSelectView`（选择，内部 3 个指针槽）。不进入玩法配置。

### World 表 0x58 已对齐字段

对照 25 条 hex 与人物 / 港口表，下列偏移在两条世界包上稳定。字段名原版未恢复，标 `[推断]` 的只根据布局与对照，不升格为确认。

| 偏移 | 宽 | 等级 | 含义 |
| --- | --- | --- | --- |
| +0x00 | 4 | `[确认]` | GBK 名指针（DLL 内） |
| +0x04 | 4 | `[未知]` | World0/World1 全是 `01 00 00 00` |
| +0x08 | 1 | `[推断]` | 领袖 `person_id`。0=拉斐尔、1=赫德拉姆、2=丽璐、3=华梅、4=杏太郎、5=亚伯拉罕、6=蒂雅，与 `ForceID` 注释一致；其余对得上 `persons_world0.json` |
| +0x09 | 1 | `[推断]` | `eForceKind`，观测值 0–3。0 多「商会」、1 多「家」、2 多「军/水师/私家舰队」、3 仅玛尔德纳德与埃斯康特 |
| +0x0A | 1 | `[推断]` | 母港 `town_id`。里斯本=11、阿姆斯特丹=2、杭州=54 等与港口表一致；**98 表示未出场**。对象侧按字节读写（`vfunc_05` `movzx BYTE`、`vfunc_04` 比 `0x62`、灭亡写 `0x62`） |
| +0x0B | 1 | `[未知]` | 全 0；`vfunc_05` 不拷，填充 |
| +0x0C | 4 | `[推断]` | 起始金币 uint32。30000 / 35000 / 45000 / 120000 等整千；未出场势力 World0 为 0 |
| +0x10 | 7×2 | 下标是 **七大海域** | 7 个 uint16。存盘循环 `>= 7`。RTC 把同形循环叫 `eSeaArea` 且上界 7。宽度 `FORCE_INFLUENCE`（USHORT）。活跃势力通常一格非 0 |
| +0x1E | 1 | `[确认]` 机制 / `[推断]` 名 | **宗主（上缴对象）`force_id`**，空 `0x1A`。World0 全空；World1 仅「昆廷韦拉斯」为 19（埃斯康特）。月结：剧本 6 时玩家势力每月把 `gold/100` 交给它（03 §2）；与它宣战（关系写 2）或它灭亡时清回 `0x1A` |
| +0x1F | 25×1 | `[确认]` 半矩阵 | 关系值；只看 `other > self` 的格。开局 0/1，三对 1：6↔11、7↔18、9↔10。运行时还有 2（交战）、4（对方已灭），见 05 |
| +0x38 | 25×1 | `[确认]` 半矩阵 / `[推断]` 名 | 友好度；只看 `other > self` 的格。默认 `0x32=50`，偏离值 10–70；宽度等于 `FORCEAMITY`（UCHAR） |
| +0x51 | 7 | `[未知]` | World 表全 0，`vfunc_05` 不拷；运行时 `+0x51/+0x52/+0x53` 被直接写成 `1A 04 1A`（语义见 05） |

`CForce.cpp` **没有**给这些字节起字段名。友好度 / 影响力的语义在 [03-relations-influence.md](03-relations-influence.md) 展开。

### 7×uint16 = 七大海域

大佬纠正：这 7 格是 **七大海域**，不是文化圈。文化圈是 21（`CCulture` 槽）。见 [06-seven-culture-circles.md](06-seven-culture-circles.md)。

- `[确认]` 存盘循环 7；RTC 局部名 `eSeaArea` 上界 7（`fn_140008b6b` body `0x14010a6c0` 对 7 格求和；`fn_14000a8c1` body `0x14010cf80`，在文件 `fn_14010be70.cpp`）。
- `[确认]` 短名：文件偏移 `0x58ab4c`（VA `0x14058bd4c`）：北海、地中海、非洲、印度洋、东南亚、东亚、新大陆。
- `[确认]` 每条活跃势力的非零格 = 母港 `culture_id` 在 exe 文化圈表里的 `major_sea_id`（06 §2.3），World0/World1 逐条对过。
- `[推断]` `NUM_SEAAREA=25` 是 RTC **投票误采**：`fill_gaps.py` 从函数入口固定扫 `0x2000` 字节，越过函数尾把邻函数的 `cmp [rsp+0x34], 0x19` 记到 `eSeaArea` 名下（`fn_140008b6b` 窗口内 7 次，函数体内只有 `cmp …, 7`）。真正带 `eSeaArea` 局部名的两个函数上界都是 7。所以 `eSeaArea` 就是七大海域轴；「25 航海分区」没有源码证据。见 06 §2.1。
- 备选「7 = `eScenario`」已否：卡斯特罗非零格是 1（里斯本 / 地中海），不是剧本 0。

## 3. 实体与 ID

| 枚举 | 原版计数 | 计数证据类型 | 空 / 非法 | 来源 |
| --- | --- | --- | --- | --- |
| `eForce` | 25 | 容器计数 + 循环上界 | 头文件 `-1`；运行时另用 `26`（`0x1A`） | `ForceID.h`、`CForce` init |
| `eForceKind` | `[未知]`，观测 0–3 | 数据观测 | `INVALID_FORCEKIND=-1` | `ForcekindID.h` |
| `eForceAimKind` | `[未知]` | — | `INVALID_FORCEAIMKIND=-1` | `ForceaimkindID.h` |
| `eForceColor` | 8 | 循环上界（`0x14020f919`） | `INVALID_FORCECOLOR=-1` | `ForceColorID.h` |
| `eForceRelation` | `[未知]` | — | `INVALID_FORCERELATION=-1` | `ForcerelationID.h` |
| `eForceFailureKind` | `[未知]` | — | `INVALID_FORCEFAILUREKIND=-1` | `ForcefailurekindID.h` |

母港空值：原版 World 表与存档用 `98`（`0x62`），**不要**用 `0`（0 是伦敦）。重构内容表写 `home_town: null`；98 只在 `ids/legacy_codec.yaml` 的 `town.empty` 出现。

### 25 个具名势力（World0 定义快照）

`leader` / `kind` / `home` / `gold` 来自 hex 解码，前两项为 `[推断]`。`home=98` 表示该包开局未出场。

| id | key | 名 | 领袖 person_id | kind | 母港 | 起始金币 |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | castro | 卡斯特罗商会 | 0 拉斐尔·卡斯特罗 | 0 | 11 里斯本 | 30000 |
| 1 | agate | 阿歌特商会 | 2 丽璐·阿歌特 | 0 | 2 阿姆斯特丹 | 35000 |
| 2 | bergstone | 柏格斯通商会 | 1 赫德拉姆·柏格斯通 | 2 | 7 斯德哥尔摩 | 45000 |
| 3 | li | 李家 | 3 华梅·李 | 1 | 54 杭州 | 40000 |
| 4 | clifford | 克利福德军 | 39 詹姆斯·克利福德 | 2 | 0 伦敦 | 120000 |
| 5 | shupea | 舒派亚商会 | 40 马丁·舒派亚 | 0 | 5 汉堡 | 30000 |
| 6 | albuquerque | 阿博科鲁克军 | 41 迪斯尼·阿博科鲁克 | 2 | 11 里斯本 | 90000 |
| 7 | barudis | 巴鲁迪斯军 | 42 彼德罗·巴鲁迪斯 | 2 | 13 塞维利亚 | 150000 |
| 8 | centorio | 陈特利欧商会 | 43 帕欧罗·陈特利欧 | 0 | 15 热那亚 | 35000 |
| 9 | pasha | 巴夏军 | 44 赫德姆·巴夏 | 2 | 22 伊斯坦布尔 | 140000 |
| 10 | hayreddin | 海雷丁家 | 45 巴巴洛沙·海雷丁 | 1 | 25 亚历山大 | 45000 |
| 11 | silvaela | 西鲁韦拉商会 | 46 菲南·西鲁韦拉 | 0 | 29 圣乔治 | 35000 |
| 12 | espinosa | 埃斯皮诺沙商会 | 47 詹洛尼默·埃斯皮诺沙 | 0 | 34 索法拉 | 60000 |
| 13 | woodin | 伍丁商会 | 5 亚伯拉罕·伍丁 | 0 | 38 巴士拉 | 70000 |
| 14 | nagpur | 纳哥普尔商会 | 49 柴门林·纳哥普尔 | 0 | 42 卡利卡特 | 80000 |
| 15 | pereira | 普雷依拉商会 | 50 道阿尔泰·普雷依拉 | 0 | 47 马六甲 | 50000 |
| 16 | kuhn | 库恩商会 | 51 安东尼奥·库恩 | 0 | 50 雅加达 | 100000 |
| 17 | kurushima | 来岛家 | 52 索静·来岛 | 1 | 59 大阪 | 30000 |
| 18 | maldonado | 玛尔德纳德军 | 53 巴斯克斯·玛尔德纳德 | 3 | 61 哈瓦那 | 45000 |
| 19 | esconte | 埃斯康特军 | 54 迪欧歌·埃斯康特 | 3 | 65 委拉克鲁斯 | 100000 |
| 20 | saeki | 佐伯家 | 4 杏太郎·佐伯 | 1 | 98 无 | 0 |
| 21 | quentin_velas | 昆廷韦拉斯 | 6 蒂雅·恰斯卡 | 0 | 98 无 | 0 |
| 22 | turuvi | 图鲁维队 | 48 柴门林·图鲁维 | 1 | 98 无 | 0 |
| 23 | korean_navy | 朝鲜水师 | 55 瑜·文 | 2 | 98 无 | 0 |
| 24 | private_fleet | 私家舰队 | 19 格尔哈特·阿迪肯 | 2 | 98 无 | 0 |

World1 相对 World0 的内容差（同一 schema）：

- 剧本主角家 0–3、20、21、24：母港 98，7×uint16 全 0；20 / 21 起始金币改为 50000
- 图鲁维：母港 10 里加，金币 35000，格 0 = 524
- 朝鲜水师：母港 57 汉城，金币 30000，格 5 = 232
- 昆廷韦拉斯 `+0x1E = 19`（宗主 = 埃斯康特；剧本 6 的主角家）
- 已出场势力的 7×uint16 数值与 World0 不同，格号相同
- 友好度：来岛家行 `[23]=30` 改为 `[22]=30`（生效：(17,23) 回到 50，(17,22) 变 30）；朝鲜水师、私家舰队行去掉 `[5]=35`（下三角，原版本来就不读，无效果）

### 静态 vs 运行时

| 字段 | 定义（内容包，只读） | 状态（存档） |
| --- | --- | --- |
| `id` / `key` / `name` | ✓ | 不存（按 id 再绑内容包） |
| `leader_person_id`、`kind` | ✓ 来自 World 表 | 存盘例程不写 +0x08/+0x09 |
| `home_town_id` | 初值 | ✓ +0x0A，可被剧本/占领改 |
| `gold` | 初值 | ✓ +0x0C |
| 7×uint16 | 初值 | ✓ +0x10 |
| `+0x1E` | 初值 | ✓ |
| 两列 25 字节（关系 / 友好度） | 初值在 03 `relations.yaml`（无序对） | ✓ 按对存，不按 25×25 下标 |
| `+0x51..53` | 无（init 固定写空 / 4 / 空） | ✓ |
| 是否出场 | 由母港是否为 98 派生 | 改母港即出场 |

禁止把名字字符串写进存档。World0 / World1 是同一批 key 的两份初值（World1 `$op: patch`），不是两套 `CForce` 逻辑。存档按 key 存势力节，读档经 `id_map` 重映射。

## 4. 算法 / 公式

本实体几乎没有公式。出场判定：

```
active(force) := force.home_town != null     # 原版比 0x62；导入时 98 → null
```

金币、7×uint16、友好度数组的加减与上下限见 `03-relations-influence.md` 与 `config/caps.yaml`。地方舰队买货如何扣本势力金币（账本 `+0x0C` 支出，无账本则直扣）是经济/委任侧，见 `srcdata/docs/exe_caps.md`，本系统只提供 `Force.gold` 的读写命令。

`CForceElement::vfunc_04` 在**月初第一拍**（`fn_14000e9da`，`SCalendar+0x1c == 0`）遍历 25 势力（跳过未出场者、**当前玩家势力**、id=24）和 69 舰队，含 NPC 买船。其中 `div 100` 是宗主上缴（`gold/100`），原版只在剧本 6 开；重构由剧本旗标 `scenario.flags.pay_suzerain_tribute` 打开，本系统在 `forces` 阶段执行步 `factions.suzerain_tribute`，金额式引用 economy `formulas/tribute.yaml#suzerain`，见 03。

母港写入：`Share.add` 成功且该势力 `home_town == null` 时，本系统把本港写成母港并发 `ForceActivated`（原版 `fn_1400122f6` → `fn_1400092ff`）。

## 5. 耦合分析

原版硬编码：

- `GetFixForceProc` 与 25 条 packed struct 绑死；加势力要改 DLL
- `CForceElement` 循环写死 `0x19`、`7`、`5`
- 领袖是人物表下标，母港是港口表下标，同结构体里没有 key
- 海域图按 `eForceColor` 画旗，颜色个数写死 8
- 私家舰队在月结里被 `== 0x18` 特殊跳过
- 空值混用 `-1`、`26`、`98`，0 又是合法伦敦 / 拉斐尔
- 25×25 关系/友好度只存半矩阵（低 id 行），World 表却给了满矩阵，下三角是死数据
- 剧本→主角家 `[0,2,1,3,20,13,21]` 是 exe 常量表，不在 World 包

重构后允许的依赖：`factions → world.town_id`、`characters → factions.force_id`（人物隶属）。禁止 economy / combat 直接改 `CForce` 内存。

## 6. 解耦后的配置

落点：`config/factions/forces.yaml`，World 包各一份。上限进 `config/caps.yaml`。

```yaml
# config/factions/forces.yaml
schema: dk4.factions.forces
version: 1
pack: vanilla-world0

# 主键 = key，定义行与引用处都写全命名空间（vanilla:castro，词表 §14-4）。vanilla_index 只由导入器写，供对照 DLL / 旧存档。
# 外键写对方 key；原版包里对方 key 未定的，导入器先写 { vanilla_index: N }，加载期解析成 key。
# 空值一律 null；原版 98 / 26 只在 config/ids/legacy_codec.yaml。

kinds:                     # NUM_FORCEKIND 未还原；原版包观测 0-3
  - { key: vanilla:merchant_guild, vanilla_index: 0, name: 商会 }   # [推断] 来自中文名
  - { key: vanilla:house,          vanilla_index: 1, name: 家 }
  - { key: vanilla:military,       vanilla_index: 2, name: 军 }
  - { key: vanilla:conquistador,   vanilla_index: 3, name: 征服军 } # [推断] 仅新大陆两军

colors:                    # eForceColor 0-7，具体 RGB [未知]；表长 = 色数
  - { key: vanilla:color_0, vanilla_index: 0 }
  - { key: vanilla:color_1, vanilla_index: 1 }
  - { key: vanilla:color_2, vanilla_index: 2 }
  - { key: vanilla:color_3, vanilla_index: 3 }
  - { key: vanilla:color_4, vanilla_index: 4 }
  - { key: vanilla:color_5, vanilla_index: 5 }
  - { key: vanilla:color_6, vanilla_index: 6 }
  - { key: vanilla:color_7, vanilla_index: 7 }

forces:
  - key: vanilla:castro
    vanilla_index: 0
    name: 卡斯特罗商会
    leader_person: vanilla:raphael_castro
    kind: vanilla:merchant_guild
    home_town: vanilla:lisbon
    gold: 30000
    major_sea_influence: { vanilla:mediterranean: 122 }  # 键 = major_sea key（06）；缺省 0
    overlord: null                               # +0x1E
  - key: vanilla:albuquerque
    vanilla_index: 6
    name: 阿博科鲁克军
    leader_person: { vanilla_index: 41 }
    kind: vanilla:military
    home_town: vanilla:lisbon
    gold: 90000
    major_sea_influence: { vanilla:mediterranean: 486 }
    overlord: null
  - key: vanilla:private_fleet
    vanilla_index: 24
    name: 私家舰队
    leader_person: { vanilla_index: 19 }
    kind: vanilla:military
    home_town: null
    gold: 0
    major_sea_influence: {}
    overlord: null
    is_private_fleet: true                       # 原版 == 0x18；月结 AI 跳过（05）
unknown:                                         # 原版不拷进对象；可不出
  vanilla:castro: { u32_04: 1, bytes_51_57: [0, 0, 0, 0, 0, 0, 0] }
```

提取器只输出已确认 / 已推断并带标记的字段；其余进 `unknown`。关系与友好度**不在本表**：它们是无序对表 `config/factions/relations.yaml`（03 §6），从低 id 行取值，丢弃下三角——与原版读法一致，模组加势力时也不会出现两边不一致。25 条完整初值以 `Docs/_data/forces_world0.json` 的 hex 为准，管线生成后人工补 `key`。World1 用 `$op: patch` 改同一批 key，不新建实体。

```yaml
# config/caps.yaml 片段（只放可调上限；不放哨兵、不放原版计数）
force_gold_max: 99999999        # fn_140002cb6 饱和 0x5F5E0FF [确认]；须 ≤ engine_limits.force_gold
force_gold_min: 0               # 不足即置 0 [确认]
```

```yaml
# config/ids/legacy_codec.yaml 片段（只给原版 DLL / 存档导入导出）
enums:
  force: { vanilla_count: 25, invalid: -1, empty: 26 }   # 0x1A
  town:  { vanilla_count: 97, invalid: -1, empty: 98 }   # 0x62
fields:
  - { record: world.force, offset: 0x0A, enum: town,  width: u8 }
  - { record: world.force, offset: 0x1E, enum: force, width: u8 }
  - { record: save.force,  offset: 0x51, enum: force, width: u8 }
  - { record: save.force,  offset: 0x53, enum: force, width: u8 }
```

```yaml
# config/narrative/scenarios.yaml 片段（narrative 拥有；原版 fn_140009df9 常量）
- { key: vanilla:raphael, player_force: vanilla:castro }
- { key: vanilla:helmut,  player_force: vanilla:bergstone }
- { key: vanilla:lulu,    player_force: vanilla:agate }
- { key: vanilla:hua_mei, player_force: vanilla:li }
- { key: vanilla:kyotaro, player_force: vanilla:saeki }
- { key: vanilla:woodin,  player_force: vanilla:woodin }
- { key: vanilla:tia,     player_force: vanilla:quentin_velas, flags: { pay_suzerain_tribute: true } }
```

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗 / 航海改代码：

- 势力条数（运行时按表长循环，禁止再写 `0x19`）
- 每条的名字、领袖、种类、母港、起始金币
- 8 色调色板
- 私家舰队标记 `is_private_fleet`（原版只有 24；可多条或没有）
- 哪条势力在哪个世界包开局出场（只改 `home_town_id` / 金币）

改「友好度怎么涨」或「份额上限」走 03 与 `caps.yaml`，不改本表 schema。

## 8. 对外接口

查询（只读，ID + 标量）：

```
Force.get(force_id) -> { key, name_id, kind, leader_person_id, home_town_id, gold, active }
Force.isActive(force_id) -> bool
Force.gold(force_id) -> int
Force.homeTown(force_id) -> town_id | null
Force.list() -> [force_id]          # 表长；运行时无哨兵
Force.player() -> force_id          # 当前剧本 player_force（原版 fn_140004381）
```

命令：

```
Force.add_gold(force_id, delta, reason)   # 饱和到 caps；成功后发 ForceGoldChanged
Force.setHomeTown(force_id, town_id | empty)
Force.activate(force_id, town_id)   # 剧本出场
```

事件（本系统发出）：

| 事件 | 载荷 |
| --- | --- |
| `ForceActivated` | `{ force_id, town_id }` |
| `ForceGoldChanged` | `{ force_id, old, new, reason }` |
| `ForceHomeChanged` | `{ force_id, old_town_id, new_town_id }` |

本系统订阅：`TownOccupied`（combat 发，只改份额/母港，不算战斗；处理见 03 §2）、`MonthStarted`（`tick.yaml` 阶段 `month_start`，推进 NPC 月结，见 05 §2.8）。不订阅市价。

外部文档取势力与颜色一律写 `Force.get(force_id)` / `Force.get(force_id).color`，同一出口，不另起别名。

`CForceSelectView` / `CForceInformationView` / `CSeaAreaForceView` 只读上述查询。

## 9. 未核实字段

必须单列，禁止在实现里猜成确定语义。

| 位置 | 观察 | 不能写成 |
| --- | --- | --- |
| World +0x04 | 恒为 1 | 「版本号」或「有效位」 |
| World +0x08 | 与人物 id 对齐 | 在 `CForce.cpp` 尚未命名之前升格为确认 |
| World +0x09 | 0–3 与中文名类别相关 | `NUM_FORCEKIND=4`（头文件未给整数） |
| World +0x0A | 与港口 id 对齐；对象侧 1 字节；`fn_1400092ff` 在空时写入 | 升格为确认字段名 `home`；宽 2 |
| World +0x0C | 整千金币 | 升格为确认字段名 `gold` |
| World +0x10 七个 uint16 | 下标 = `major_sea_id` 已确认；USHORT，加减封 9999 | `eForceAimKind`；`eScenario` 下标；「25 航海分区」下标（25 是投票误采） |
| World +0x1E | 上缴/宣战/灭亡三处用法已钉 | 「同盟」；原版字段名（仍未恢复） |
| World +0x1F 25 字节 | 半矩阵；运行时 0/1/2/4 | `eForceRelation` 枚举值只有 0/1 |
| World +0x38 25 字节 | 半矩阵；默认 50 | 在关系文档里当确认公式输入可以，字段名仍是推断 |
| World 下三角（`other < self`） | 原版不读 | 有效初值 |
| World +0x51–0x57 | 表内 0、不拷；init `1A 04 1A` | `+0x52` 一定是 `eForceColor`（只是恰好写成 4） |
| `eForceAimKind` 全体 | 仅有枚举符号 | 任何具体 aim 列表 |
| `CForceFailure` 4 字节 ×7 | 下标 = `major_sea_id`；空哨兵 0x1A | 失败种类表；`NUM_SCENARIO` 下标 |
| `CForceElement +0x910` ×5 | 与 `eBattleResult` 同数 | 本系统拥有战斗结果（那是 combat 状态） |
| 私家舰队领袖 person 19 | 表上是格尔哈特·阿迪肯 | 「玩家本人」；玩家隶属是存档里另一处 |

hex 原文：`Docs/_data/forces_world0.json`、`srcdata/source/world/World0_force.c`、`World1_force.c`。
