# 势力实体（CForce）

本篇管「谁是一个势力」：25 个具名 `eForce`、World 表 0x58 记录、静态定义与运行时状态的分界。友好度、份额、合约、上缴见 [03-relations-influence.md](03-relations-influence.md)。国家 / 文化 / 流行见 [02-countries-culture-fashion.md](02-countries-culture-fashion.md)。

## 1. 职责边界

**管：**

- 势力作为可存档实体：ID、显示名、种类、领袖人物、母港、起始金币
- World0 / World1 两份内容包里的 25 条实例
- 势力集合容器 `CForceElement`（25 个 `CForce` + 附属失败/战果槽）
- 地图着色用的 `eForceColor`（8 色）
- 运行时「此势力是否已出场」（母港哨兵）

**不管：**

- 港口坐标、海域、航线 → `world`
- 人物六维、招募 → `characters`（本系统只持 `leader_person_id`）
- 市价、买卖公式 → `economy`
- 海战拿港、接舷 → `combat`（本系统只收事件）
- 设施对话、广场 NPC → `town-life`
- 友好度 / 份额 / 合约 / 上缴的规则 → 本目录 `03-relations-influence.md`

系统之间只传 `force_id`、`town_id`、`person_id` 和标量，不传 `CForce*`。

## 2. 原版事实

- `[确认]` `eForce` 0–24 共 25 个，`INVALID_FORCE = -1`，`NUM_FORCE = 25`。来源：`srcdata/source/ForceID.h`、`GetFixForceProc` 断言。
- `[确认]` 第 24 号（id=24）中文名为「私家舰队」。0–23 为具名商会 / 家 / 军。来源：`ForceID.h`、`Docs/_data/forces_world0.json`。
- `[确认]` World DLL 记录步长 **0x58 = 88 字节**。`DllMain.cpp` 的 `SForce`：前 4 字节是 GBK 名指针，其余 0x50 未在头文件命名。`CForceElement` 构造把 25 个对象按 stride `0x58` 排在 `+8`。来源：`World0_force.c`、`CForceElement.cpp` ctor、`Force.h` 注释。
- `[确认]` `CForce : CSaveObject`。`CForce::vfunc_05` 从 `GetFixForceProc` `rep movsb` 拷 88 字节再写运行时字段；`vfunc_06` / `vfunc_07` 是存盘 / 读盘。来源：`srcdata/source/game/world/CForce.cpp`。
- `[确认]` 存盘读写的偏移（`vfunc_06/07`，循环上界 0x19=25、7 个 word）：

  | 偏移 | 宽度 | 次数 | IO |
  | --- | --- | --- | --- |
  | +0x0A | 1 | 1 | 字节 |
  | +0x0C | 4 | 1 | `fn_14000def9` / `fn_14000f240`（与金币 uint32 同宽） |
  | +0x10 | 2 | 7 | word |
  | +0x1E | 1 | 1 | 字节 |
  | +0x1F+i | 1 | 最多 25 | 仅当 `fn_140010bc7(self, i)` 真 |
  | +0x38+i | 1 | 最多 25 | 同上 |
  | +0x51 | 1 | 1 | 字节 |
  | +0x52 | 1 | 1 | 字节 |
  | +0x53 | 1 | 1 | 字节 |

- `[确认]` `vfunc_04`：`+0x0A == 0x62` 视为空。`0x62 = 98`，与 `NUM_TOWN=97` 越界、名册 `SAVE_SAILOR_COUNT=98` 同一数字，World 表里母港也用 98 表示「无港」。来源：`CForce.cpp`、`World0_force.c`。
- `[确认]` 初始化把 `+0x51 = 0x1A`、`+0x52 = 4`、`+0x53 = 0x1A`。`0x1A = 26` 在多处当「非 0–24 的空势力」哨兵（与头文件 `INVALID_FORCE=-1` 并存）。来源：`CForce::vfunc_05`、`CForceElement::vfunc_04`（`s64 == 0x1a` 则跳过）、`CForceFailure`。
- `[确认]` `CForceElement` 还构造：`+0x8A0` 起 7 个 stride `0x10` 对象；`+0x910` 起 5 个 stride `0x10` 对象。7 = `NUM_SCENARIO`，5 = `NUM_BATTLERESULT`。前者与 `CForceFailure` 同类大小。来源：ctor 里 `fn_140007dce`。
- `[确认]` `eForceColor` 共 8，`INVALID_FORCECOLOR=-1`。海域图 `CSeaAreaMapView` RTC 局部名 `eForceColor`。来源：`ForceColorID.h`、`num_values.json`。
- `[确认]` `NUM_FORCEKIND` / `NUM_FORCEAIMKIND` / `NUM_FORCERELATION` / `NUM_FORCEFAILUREKIND` 只有符号，整数未还原。来源：对应 `*ID.h`、`asserts.txt`。
- `[确认]` World1 25 个名字与 World0 相同，数值不同（母港、金币、7×uint16）。来源：`World1_force.c`。
- `[确认]` UI：`CForceInformationView`（舰队侧势力情报）、`CSeaAreaForceView`（海域）、`CForceSelectView`（选择，内部 3 个指针槽）。不进入玩法配置。

### World 表 0x58 已对齐字段

对照 25 条 hex 与人物 / 港口表，下列偏移在两条世界包上稳定。字段名原版未恢复，标 `[推断]` 的只根据布局与对照，不升格为确认。

| 偏移 | 宽 | 等级 | 含义 |
| --- | --- | --- | --- |
| +0x00 | 4 | `[确认]` | GBK 名指针（DLL 内） |
| +0x04 | 4 | `[未知]` | World0/World1 全是 `01 00 00 00` |
| +0x08 | 1 | `[推断]` | 领袖 `person_id`。0=拉斐尔、1=赫德拉姆、2=丽璐、3=华梅、4=杏太郎、5=亚伯拉罕、6=蒂雅，与 `ForceID` 注释一致；其余对得上 `persons_world0.json` |
| +0x09 | 1 | `[推断]` | `eForceKind`，观测值 0–3。0 多「商会」、1 多「家」、2 多「军/水师/私家舰队」、3 仅玛尔德纳德与埃斯康特 |
| +0x0A | 2 | `[推断]` | 母港 `town_id`。里斯本=11、阿姆斯特丹=2、杭州=54 等与港口表一致；**98 表示未出场** |
| +0x0C | 4 | `[推断]` | 起始金币 uint32。30000 / 35000 / 45000 / 120000 等整千；未出场势力 World0 为 0 |
| +0x10 | 7×2 | 下标是 **七大海域** | 7 个 uint16。存盘循环 `>= 7`。RTC 把同形循环叫 `eSeaArea` 且上界 7。宽度 `FORCE_INFLUENCE`（USHORT）。活跃势力通常一格非 0 |
| +0x1E | 1 | `[未知]` | World0 全 `0x1A`；World1 仅「昆廷韦拉斯」为 `0x13`（= 势力 19 埃斯康特） |
| +0x1F | 25×1 | `[推断]` | 按对方 `force_id` 的 0/1 旗。三对对称：6↔11、7↔18、9↔10 |
| +0x38 | 25×1 | `[推断]` | 对 25 个势力的初始友好度。默认 `0x32=50`，偏离值 10–70；宽度等于 `FORCEAMITY`（UCHAR） |
| +0x51 | 7 | `[未知]` | World 表全 0；运行时 init 写成 `1A 04 1A` 再 4 字节 0 |

`CForce.cpp` **没有**给这些字节起字段名。友好度 / 影响力的语义在 [03-relations-influence.md](03-relations-influence.md) 展开。

### 7×uint16 = 七大海域

大佬纠正：这 7 格是 **七大海域**，不是文化圈。文化圈是 21（`CCulture` 槽）。见 [06-seven-culture-circles.md](06-seven-culture-circles.md)。

- `[确认]` 存盘循环 7；RTC 局部名 `eSeaArea` 上界 7（`fn_140008b6b`、`fn_14010be70`）。
- `[确认]` 短名 exe `0x58ab4c`：北海、地中海、非洲、印度洋、东南亚、东亚、新大陆。
- `[确认]` `NUM_SEAAREA=25` 仍在 `SeaAreaID.h`。7 与 25 同时存在，不要改头文件。
- 备选「7 = `eScenario`」已否：卡斯特罗非零格是 1（里斯本 / 地中海），不是剧本 0。

## 3. 实体与 ID

| 枚举 | 原版计数 | 空 / 非法 | 来源 |
| --- | --- | --- | --- |
| `eForce` | 25 | 头文件 `-1`；运行时另用 `26`（`0x1A`） | `ForceID.h`、`CForce` init |
| `eForceKind` | `[未知]`，观测 0–3 | `INVALID_FORCEKIND=-1` | `ForcekindID.h` |
| `eForceAimKind` | `[未知]` | `INVALID_FORCEAIMKIND=-1` | `ForceaimkindID.h` |
| `eForceColor` | 8 | `INVALID_FORCECOLOR=-1` | `ForceColorID.h` |
| `eForceRelation` | `[未知]` | `INVALID_FORCERELATION=-1` | `ForcerelationID.h` |
| `eForceFailureKind` | `[未知]` | `INVALID_FORCEFAILUREKIND=-1` | `ForcefailurekindID.h` |

母港空值：内容包用 `town_id = 98`（`0x62`），**不要**用 `0`（0 是伦敦）。

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
- 昆廷韦拉斯 `+0x1E = 19`（指向埃斯康特）
- 已出场势力的 7×uint16 数值与 World0 不同，格号相同

### 静态 vs 运行时

| 字段 | 定义（内容包，只读） | 状态（存档） |
| --- | --- | --- |
| `id` / `key` / `name` | ✓ | 不存（按 id 再绑内容包） |
| `leader_person_id`、`kind` | ✓ 来自 World 表 | 存盘例程不写 +0x08/+0x09 |
| `home_town_id` | 初值 | ✓ +0x0A，可被剧本/占领改 |
| `gold` | 初值 | ✓ +0x0C |
| 7×uint16 | 初值 | ✓ +0x10 |
| `+0x1E`、两列 25 字节、`+0x51..53` | 初值 | ✓ |
| 是否出场 | 由母港是否为 98 派生 | 改母港即出场 |

禁止把名字字符串写进存档。World0 / World1 是两份 `forces.yaml`，不是两套 `CForce` 逻辑。

## 4. 算法 / 公式

本实体几乎没有公式。出场判定：

```
active(force) := (home_town_id != town.empty_sentinel)
town.empty_sentinel = 98    # 原版 0x62；写入 caps，禁止写死
```

金币、7×uint16、友好度数组的加减与上下限见 `03-relations-influence.md` 与 `config/caps.yaml`。地方舰队买货如何扣本势力金币（账本 `+0x0C` 支出，无账本则直扣）是经济/委任侧，见 `srcdata/docs/exe_caps.md`，本系统只提供 `Force.gold` 的读写命令。

`CForceElement::vfunc_04` 按月遍历 25 势力（跳过 id=24 与未出场者）和 69 舰队，含 NPC 买船与一处 `div` 金钱。上缴公式未还原，见 03。

## 5. 耦合分析

原版硬编码：

- `GetFixForceProc` 与 25 条 packed struct 绑死；加势力要改 DLL
- `CForceElement` 循环写死 `0x19`、`7`、`5`
- 领袖是人物表下标，母港是港口表下标，同结构体里没有 key
- 海域图按 `eForceColor` 画旗，颜色个数写死 8
- 私家舰队在月结里被 `== 0x18` 特殊跳过
- 空值混用 `-1`、`26`、`98`，0 又是合法伦敦 / 拉斐尔

重构后允许的依赖：`factions → world.town_id`、`characters → factions.force_id`（人物隶属）。禁止 economy / combat 直接改 `CForce` 内存。

## 6. 解耦后的配置

落点：`config/factions/forces.yaml`，World 包各一份。上限进 `config/caps.yaml`。

```yaml
# config/factions/forces.yaml
schema: dk4.factions.forces
version: 1
pack: vanilla-world0

empty_town_id: 98          # 原版 0x62；不是 INVALID_TOWN=-1
runtime_empty_force_id: 26 # 原版 0x1A，存档哨兵
kind_ids:                  # NUM_FORCEKIND 未还原；原版包观测 0-3
  - { id: 0, key: merchant_guild, name: 商会 }   # [推断] 来自中文名
  - { id: 1, key: house,            name: 家 }
  - { id: 2, key: military,         name: 军 }
  - { id: 3, key: conquistador,     name: 征服军 } # [推断] 仅新大陆两军

colors:                    # eForceColor 0-7，具体 RGB [未知]
  - { id: 0, key: color_0 }
  - { id: 1, key: color_1 }
  - { id: 2, key: color_2 }
  - { id: 3, key: color_3 }
  - { id: 4, key: color_4 }  # CForce init 默认写入 +0x52=4
  - { id: 5, key: color_5 }
  - { id: 6, key: color_6 }
  - { id: 7, key: color_7 }

forces:
  - id: 0
    key: castro
    name: 卡斯特罗商会
    leader_person_id: 0
    kind: 0
    home_town_id: 11
    gold: 30000
    theater_influence: [0, 122, 0, 0, 0, 0, 0]  # [推断] 见 §2
    related_force_id: 26                         # +0x1E
    relation_flags: []                           # 25 槽里为 1 的对方 id
    initial_amity: { 6: 70, 7: 45 }              # 未列出的默认 50
    unknown_u32_04: 1
    unknown_bytes_51_57: [0, 0, 0, 0, 0, 0, 0]
  - id: 6
    key: albuquerque
    name: 阿博科鲁克军
    leader_person_id: 41
    kind: 2
    home_town_id: 11
    gold: 90000
    theater_influence: [0, 486, 0, 0, 0, 0, 0]
    related_force_id: 26
    relation_flags: [11]                         # 与西鲁韦拉对称
    initial_amity: { 0: 70, 4: 45, 7: 45, 11: 60 }
    unknown_u32_04: 1
    unknown_bytes_51_57: [0, 0, 0, 0, 0, 0, 0]
  - id: 24
    key: private_fleet
    name: 私家舰队
    leader_person_id: 19
    kind: 2
    home_town_id: 98
    gold: 0
    theater_influence: [0, 0, 0, 0, 0, 0, 0]
    related_force_id: 26
    relation_flags: []
    initial_amity: { 5: 35, 22: 35 }
    unknown_u32_04: 1
    unknown_bytes_51_57: [0, 0, 0, 0, 0, 0, 0]
```

提取器只输出已确认 / 已推断并带标记的字段；其余进 `unknown_bytes`。25 条完整初值以 `Docs/_data/forces_world0.json` 的 hex 为准，管线生成后人工补 `key`。

```yaml
# config/caps.yaml 片段
force_count_vanilla: 25
force_color_count: 8
town_empty_sentinel: 98
force_runtime_empty_id: 26
```

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗 / 航海改代码：

- 势力条数（运行时按表长循环，禁止再写 `0x19`）
- 每条的名字、领袖、种类、母港、起始金币
- 8 色调色板
- 空母港哨兵、运行时空势力哨兵
- 哪条势力在哪个世界包开局出场（只改 `home_town_id` / 金币）

改「友好度怎么涨」或「份额上限」走 03 与 `caps.yaml`，不改本表 schema。

## 8. 对外接口

查询（只读，ID + 标量）：

```
Force.get(force_id) -> { key, name_id, kind, leader_person_id, home_town_id, gold, active }
Force.isActive(force_id) -> bool
Force.gold(force_id) -> int
Force.homeTown(force_id) -> town_id | empty
Force.list() -> [force_id]          # 表长，不含哨兵
```

命令：

```
Force.setGold(force_id, value)      # 饱和到 caps
Force.setHomeTown(force_id, town_id | empty)
Force.activate(force_id, town_id)   # 剧本出场
```

事件（本系统发出）：

| 事件 | 载荷 |
| --- | --- |
| `ForceActivated` | `{ force_id, town_id }` |
| `ForceGoldChanged` | `{ force_id, old, new, reason }` |
| `ForceHomeChanged` | `{ force_id, old_town_id, new_town_id }` |

本系统订阅：`TownCaptured`（combat 发，只改隶属/母港，不算战斗）、`MonthTicked`（推进 NPC 月结，公式在 03）。不订阅市价。

`CForceSelectView` / `CForceInformationView` / `CSeaAreaForceView` 只读上述查询。

## 9. 未核实字段

必须单列，禁止在实现里猜成确定语义。

| 位置 | 观察 | 不能写成 |
| --- | --- | --- |
| World +0x04 | 恒为 1 | 「版本号」或「有效位」 |
| World +0x08 | 与人物 id 对齐 | 在 `CForce.cpp` 尚未命名之前升格为确认 |
| World +0x09 | 0–3 与中文名类别相关 | `NUM_FORCEKIND=4`（头文件未给整数） |
| World +0x0A | 与港口 id 对齐 | 升格为确认字段名 `home` |
| World +0x0C | 整千金币 | 升格为确认字段名 `gold` |
| World +0x10 七个 uint16 | 地理带 / USHORT | `eForceAimKind`；`eScenario` 下标 |
| World +0x1E | 多 0x1A，World1 昆廷=19 | 「宗主」或「同盟」 |
| World +0x1F 25 字节 | 三对对称 1 | `eForceRelation` 枚举值（只有 0/1） |
| World +0x38 25 字节 | 默认 50 | 在关系文档里当确认公式输入可以，字段名仍是推断 |
| World +0x51–0x57 | 表内 0；init `1A 04 1A` | `+0x52` 一定是 `eForceColor`（只是恰好写成 4） |
| `eForceAimKind` 全体 | 仅有枚举符号 | 任何具体 aim 列表 |
| `CForceFailure` 4 字节 ×7 | 空哨兵 0x1A | 失败种类表 |
| `CForceElement +0x910` ×5 | 与 `eBattleResult` 同数 | 本系统拥有战斗结果（那是 combat 状态） |
| 私家舰队领袖 person 19 | 表上是格尔哈特·阿迪肯 | 「玩家本人」；玩家隶属是存档里另一处 |

hex 原文：`Docs/_data/forces_world0.json`、`srcdata/source/world/World0_force.c`、`World1_force.c`。
