# 人物模型

人物系统的根文档。管「一个 `person_id` 是什么、三层数据怎么切、表怎么落」。六维/技能/岗位见 `03-abilities-skills-jobs.md`，剧本主角见 `02-protagonists-scenarios.md`，招募与名册见 `04-recruitment-crew.md`。

## 1. 职责边界

本系统管：

- 人物定义（姓名、六维初值、性别、星座、世界表残余字段）；星座表 `characters/zodiac.yaml` 自 `01-world/04` 移交，本系统拥有（`03` §2）
- 人物状态（两段经验、隶属、装备槽、疲劳、身心状态）
- 把定义 + 状态投影成修正器注册表里的 `ability.<key>` / `skill.<key>` / `person.hp_max` 键（DECISIONS D2），供经济 / 战斗 / 航海按键查询
- 名册槽与 `person_id` 的对应关系

不管：

- 等级公式的实现与经验增量从哪来（`11-progression` 写 `exp`，本系统只存）
- 舱室在船体上怎么摆（`05-ships-fleet` 的 `cabins.yaml`）
- 港口设施对话树、酒馆菜单 UI（`08-town-life`）
- 剧本事件何时把某人写入名册（`10-narrative` 发命令）
- 装备效果数值（`09-items` 提供效果，本系统只持有 5 个 `item_id`）

## 2. 原版事实

- `[确认]` `ePerson` 共 235 人，`INVALID_PERSON = -1`。来源：`PersonID.h` `NUM_PERSON = 235`，`GetFixPersonProc`。
- `[确认]` 世界表步长 0x38。来源：`World0_person.c` 头注释、`Person.cpp`、`world.h` `SPerson`。
- `[确认]` 世界表前三个字段是姓名指针：`+0` 名、`+8` 姓、`+0x10` 第三名（可空）。来源：`Person.cpp`、`SPerson.{given,family,name3}`。
- `[确认]` 六维在世界表 `+0x20`，顺序 **体力 敏捷 魅力 智力 精神 运气**。来源：`Person.cpp`、`AbilityID.h`、`save_format.md`、`DllMain.cpp`。
- `[确认]` 技能、HP、心情不进存档，由属性 + 等级现场算。来源：`Person.cpp`「Skill / HP / mood are computed… not stored」、`Save.cpp`「Derived, not stored: 健康 / 心情 / 技能」。
- `[确认]` 运行时类：`CPlayerPerson → CPerson → CSaveObject → CObject`。来源：`Person.h` `PlayerPerson.h`。
- `[确认]` `CPerson::vfunc_05` 从 `GetFixPersonProc` `rep movsb` 拷 56 字节世界行到 `r_sFixPerson`，再经 `fn_140014709` 填入对象。世界布局 ≠ 对象布局。
- `[确认]` `CPerson::vfunc_06` / `vfunc_07` 成对读写对象字段：`+0x0C` 一字节、`+0x22` 一字节；**若 `person_id >= 0x57`（87）到此为止**；否则继续六维 6 字节 `+0x0F`、两个 `uint32` 经验 `+0x18` / `+0x1C`、`uint16` `+0x20`、`+0x0D` / `+0x0E` 各一字节、5 个装备槽 `+0x23` 起。合计 25 字节（ids 0–86）或 2 字节（ids 87–234）。来源：`CPerson.cpp`（`fn_140012bca` 取 id，`cmp 0x57`）。
- `[确认]` 世界行 → 对象 `fn_140014709`（body `0x1402ad280`）：`W+0x26→+0x08`（性别）、`W+0x27→+0x09`、`W+0x28→+0x0A`、`W+0x29→+0x0B`（星座，==13 时改 `id % 12`）、`W+0x2A→+0x0C`（势力）、六维 `W+0x20→+0x0F`、`W+0x18/+0x1C→+0x18/+0x1C`（经验）、`+0x0D=0`（健康）、`+0x0E=3`（普通）、`+0x22=0`、装备 5 槽全写 `0xE8`（空）、`W+0x2B→+0x28`（半身像）、`W+0x2C→+0x29`、`W+0x30`（u32）`→+0x2C`、`+0x20 = fn_14000cf22()`（HP 上限）。
- `[确认]` 对象字段语义：`+0x0C` 隶属势力（`fn_14000da85` 读，与玩家势力比较；`0x1A` = 无所属，`fn_1400140ba` 显式判 `== 0x1A`）；`+0x0D` 身体状态（7 态，`vfunc_04` 以 `== 6` 判死亡）；`+0x0E` 精神状态（6 态）；`+0x20` 当前 HP（`uint16`，setter `fn_1400141ff` 在由正变 ≤0 时置身体=濒死并给段 0 经验 400）；`+0x08` 性别；`+0x0B` 星座（`CPerson::vfunc_11` 返回它，主角由 `CPlayerPerson::vfunc_11` 从生日现算）。名称表见 `03` / `07`。
- `[确认]` `+0x22` 是一个**只升不降**的字节：setter `fn_140006b8b(p, v, only_raise)` 在 `only_raise` 且当前 `>= v` 时不写。读它的是势力/城镇/信息窗函数。语义 `[未知]`（像「熟识度」，不猜）。
- `[确认]` `CPlayerPerson` 在 `CPerson` 之上再串行化三段 17 字节（`+0x30` 名、`+0x41` 姓、`+0x52` 中间名，GBK）、`+0x63` 生日月（0 起）、`+0x64` 生日日、19 字节 `+0x65`（商会/势力名，存档里是「伍丁商会」「卡斯特罗商会」等）、`uint32 +0x78`、两字节 `+0x7C`/`+0x7D`。来源：`CPlayerPerson.cpp`、仓库 `Savedata/zh-CN/*.dk4` 对照。
- `[确认]` 主角星座 `CPlayerPerson::vfunc_11 = fn_1400034b3(month, day)`：`day < CUT[month] ? month : (month < 11 ? month+1 : 0)`，`CUT` = 表 `0x140688ca0` `[20,18,20,20,21,21,22,22,23,23,22,21]`；结果下标进星座表 `0x140688970`（0 摩羯座 … 11 射手座）。存档样本：拉斐尔 3 月 4 日 → 2 双鱼座，与世界表 `W+0x29=2` 一致。
- `[确认]` 存档人物段的真实成帧：从 `+0x2C03` 起按 `ePerson` 0–234 顺序是 235 条 `CPerson` 串行化记录；ids 0–86 每条 25 字节（隶属、`+0x22`、六维、两段经验、HP u16、身体、精神、5 装备），ids 87–234 每条 2 字节（隶属、`+0x22`）。紧接着 `+0x35AA` 是主角的 `CPlayerPerson` 记录。验算：`0x2C03 + 87×25 + 148×2 = 0x35AA`。来源：`CPerson::vfunc_06/07`、仓库 4 份存档逐字节对照。
- `[确认]` `dk4save` / `save_format.md` 的「名册 98 槽、步长 0x19、基址 `+0x2C00`、`+0x03` 隶属、`+0x05` 六维」是**错位 3 字节**的同一份数据：行 `i` 的 `+0x03` 才是记录 `i` 的开头；行的 `+0x00..+0x02` 是上一条的装备槽 2–4，`+0x04` 是 `+0x22`，`+0x13..+0x14` 是 HP，`+0x15` 身体，`+0x16` 精神，`+0x17..+0x18` 是装备 0–1。dk4save 的字段偏移 `+3/+5/+0B/+0F` 恰好等于记录 `+0/+2/+8/+0C`，3 字节起点差被抵消，所以 **ids 0–86 读写的字段是对的**。真正出错的是行 87–97：它们落在 2 字节记录区，「六维 / 经验」实际写进 ids 87–219 的隶属与 `+0x22` 字节；仓库 `savedat01.dk4` 曾有 70 字节实损（35 个职能 NPC 隶属变成 255 / 196 / 0），2026-11-06 已按 `.bak` 恢复。ids 87–97 的真实六维、经验不进存档，dk4save 改它们改不到本人。`SAVE_SAILOR_COUNT = 98` 是工具常数，不是引擎常数；引擎完整记录数 87（`CPerson::vfunc_06` body `0x1401cb480` `cmp eax,0x57`）。另：旧版 dk4save 不重算存档头 `+0x18` 校验和，改档后读档会弹「所储存的进度有可能损坏了」确认框，选「是」可载入（`0x14011255e`）。以上工具问题已于 2026-11-06 在 `dk4save.c` 修复（见 `12-ui-save/02-save-session.md`「dk4save 已知问题」）。来源：`scratchpad/reports/save-roster-verify.md`。
- `[确认]` 主角另有一份 `CPlayerPerson` 记录：隶属 `+0x35AA`、`+0x22` 字节 `+0x35AB`、六维 `+0x35AC`、两段经验 `+0x35B2` / `+0x35B6`、HP `+0x35BA`、身体 `+0x35BC`、精神 `+0x35BD`、装备 `+0x35BE`；姓名三段 `+0x35C3`/`+0x35D4`/`+0x35E5`；生日 `+0x35F6`/`+0x35F7`。`+0x20` 是主角的 `person_id`；`+0x70` 是缓存的两段等级之和。改主角必须三处一起写。来源：`save_format.md`、`CPlayerPerson.cpp`、存档对照。
- `[确认]` 已招收：该人隶属 == 主角隶属。来源：`save_format.md`、`Save.cpp`；引擎侧 `fn_14000e1ce` 同样用「隶属 == 玩家势力」决定查不查舱位。
- `[推断]` 「在队 = 六维之和 > 0」只是 `dk4save` 的判据，不是引擎规则：ids 87–234 存档里根本没有六维；95–97 的世界六维为 0。引擎的在队判据未还原。
- `[确认]` `+0x35C3` 起 3×17 字节是主角姓名（`CPlayerPerson+0x30/+0x41/+0x52`）。`[推断]` `save_format.md` 说的「步长 0x2E9、index 1..98 = 水手 id−1」未在引擎串行化里找到对应，存档样本中 `+0x35C3+0x2E9` 处不是姓名块，按 dk4save 约定记录，不作引擎事实。
- `[确认]` `NUM_PERSONEQUIP = 5`。来源：`PersonEquipID.h`。官方手册：武器、防具、装备道具 ×3。
- `[确认]` 世界 0–6 是七个剧本提督，98 起大量职能 NPC。来源：`PersonID.h` 注释、`id-registry.md`。
- `[确认]` 只有 ids 0–86 的六维、经验、HP、身心、装备进存档；87–234 读档时这些字段来自世界表（`vfunc_05` 先拷世界行）。
- `[确认]` 世界表 `+0x18`/`+0x1C` 两个 `uint32` 是**初始两段经验**：`fn_140014709` 原样拷到 `CPerson+0x18/+0x1C`，等级函数 `fn_1400146c3` / `fn_140012085` 正是读这两处做 `isqrt(exp/30)`，加经验 `fn_140011c34` 写这两处。`world.h` 的 `pad18[8]` 应改名。
- `[确认]` 世界表 `+0x26` 是性别：拷到 `CPerson+0x08`，`fn_140006b90` 以 `== 1` 取「女」（`0x14057fd70`）否则「男」（`0x14057fd74`）。World0 0–97 中 83 男 / 15 女。
- `[确认]` 世界表 `+0x29` 是星座 0–11（摩羯座…射手座，表 `0x140688970`），13 = 随机（`fn_140014709` 改成 `id % 12`）。星座决定升级时六维成长倾向（`03-abilities-skills-jobs.md`）。
- `[确认]` 世界表 `+0x2A` 是初始隶属 `force_id`：七主角为 0/2/1/3/20/13/21，与 `ForceID.h` 的七家一致；26（`0x1A`）= 无所属。

## 3. 实体与 ID

| 实体 | 原版枚举 | 计数 | 空/非法 | 稳定 key 例 |
| --- | --- | --- | --- | --- |
| 人物 | `ePerson` | 235 | 运行时 `null`；原版 `-1`，舱室空槽 `0xEC`（236） | `vanilla:raphael_castro` |
| 能力 | `eAbility` | 6 | `null`（原版 −1） | `stamina` |
| 技能 | `eSkill` | 10 | `INVALID_SKILL = -1` | `surveying` |
| 装备槽 | `ePersonEquip` | 5 | `INVALID_PERSONEQUIP = -1` | `equip.weapon` |
| 完整存档人物 | `CPerson` 串行化 | 87（id < `0x57`） | 无独立枚举 | `persisted_full: true` |
| 装备空槽 | 字节哨兵 | — | 运行时 `null`；原版 `0xE8`（232）只在 `ids/legacy_codec.yaml` | — |
| 势力空 | 字节哨兵 | — | 运行时 `null`；原版 `0x1A`（26）只在 `legacy_codec` | — |
| 身体 / 精神状态 | `ePhysicalState` / `eMentalState` | 7 / 6 | `null` | 见 `07` |
| 星座 | 表 `0x140688970` | 12（世界值 13 = 随机）；表与成长值见 `03` | `null` | `capricorn` |

原版哨兵（`0xE8`、`0x1A`、`0xEC`、星座 13）都是「计数 + 1」或越界常数，模组顺序分配 ID 时会撞号，所以只出现在导入 / 导出层（DECISIONS D1），导出原版格式时越界拒绝。运行时以表长循环，禁止写死 `235` / `87` / `98`。原版包实例计数 235 只是 `persons.yaml` 表长，不进 `caps.yaml`（`13-algorithms/02`：`NUM_*` 不是引擎上界）。「谁的状态完整进存档」不当上限用：新存档 schema 对**所有**人物写同一套状态字段（原版只写 0–86 是存档体积优化，87+ 的六维/经验/HP 读档后回落成世界值，属原版行为，导入器照此处理）。

### 三层：世界定义 / 存档状态 / 运行时对象

```
World DLL  SPerson (0x38, 只读内容包)
    │  开局 / 读档缺字段时
    ▼
Save       235 条 CPerson 记录（0–86 各 25B，87–234 各 2B）+ CPlayerPerson
    │  读档
    ▼
Runtime    CPerson / CPlayerPerson（派生技能；经 Modifiers.eval 暴露）
```

| 层 | 持有 | 不持有 |
| --- | --- | --- |
| 定义（`persons.yaml`） | 姓名、六维初值、性别、星座、初始隶属、初始经验、bustup、hex 残余 | 当前经验、隶属、装备 |
| 状态（存档） | 两段 `exp`、六维当前值、隶属、`+0x22` 字节、HP、身体/精神状态、装备 5 槽；主角另有姓名、生日、商会名 | 技能值、等级（可缓存但必须能从 `exp` 重建） |
| 运行时 | 定义 + 状态 + 派生缓存 | 不得把公式常数写死在对象里 |

原则 4：定义进内容包，状态进存档。禁止开局把整张人物表拷进存档再改定义字段。

### 人物分类（原版 235）

| 类别 | id 范围 | 特征 | 名册 |
| --- | --- | --- | --- |
| 可操作主角 | 0–6 | 七剧本提督，有完整六维与姓名 | 开局由剧本写入名册 |
| 具名航海士 | 7–97 | 有姓名；多数有六维；95–97（爱斯法妮雅、特欧贝尔德、宗九郎）六维全 0 | 招入后进名册；存档完整状态只到 id 86，87–97 读档回落世界值 |
| 职能 NPC | 98–166、178–191、203–212 | 六维全 0，按设施/文化复用立绘 | 不入名册 |
| 属下模板 | 167–177 | 六维全 40，欧洲/非洲/阿拉伯/印度/东南亚/中国/韩国/日本/中南美 + 预备 1/2 | 不直接占槽；雇佣无名航海士时作模板 `[推断]` |
| 事件/杂类 | 192–202 | 私设舰队、教祖、村民、土匪、妖怪、探险家、安吉鲁/斯耐克/丹第/长老 | 不入名册 |
| 首都女性 | 213–234 | 名+港口姓（玛丽·伦敦 … 伊莎贝尔·哈瓦那），六维 0 | 不入名册 |

id 区间只是对原版数据的观察。引擎一律读显式字段（`category`、`recruitable_roster`、`crew_template`、`home_town` 等），不按区间推（D1）。

职能 NPC 按 `given` 聚合（World0）：

| given | 数量 | family 例 |
| --- | --- | --- |
| 广场的店主 | 10 | 欧洲男子/女子、非洲、阿拉伯、印度、东南亚、中国、韩国、日本、中南美 |
| 酒馆的掌柜 | 3 | 欧洲、中国、日本 |
| 酒馆的客人 | 9 | 同上文化圈 |
| 交易所的掌柜 | 5 | 欧洲、阿拉伯、中国、日本、中南美 |
| 造船厂的老板 | 4 | 欧洲、阿拉伯、中国、日本 |
| 码头的小伙子 | 7 | 欧洲、非洲、阿拉伯、东南亚、中国、日本、中南美 |
| 卫兵 | 5 | 欧洲、阿拉伯、东南亚、中国、日本 |
| 国王 / 地方领主 / 藩主 / 县令 / 大名 / 总督 | 1–2 各 | 宫殿对话 |
| 阿訇 / 祈祷师 / 高僧 / 神父 | 若干 | 教会 |
| 旅馆的老板娘 | 7 | 欧洲、非洲、阿拉伯、东南亚、中国、日本、中南美 |
| 行会的总管 | 4 | 欧洲、阿拉伯、中国、日本 |
| 属下 | 11 | 见上 |
| 其他 | — | 贵妇人、男孩、夫妇、研究生、晕倒者、厨师（酒馆老板）等 |

职能 NPC 只提供立绘与文案 key，不参与技能公式。港口设施用 `town_id + culture` 选哪一个 `person_id` 出场，不把 NPC 指针传出人物系统。

## 4. 算法 / 公式

本文件不实现成长。人物侧只约定：

- 经验是源。两段 `exp_track_0` / `exp_track_1` 由 progression 写入。
- 单段等级 `q = exp / exp_divisor`，`level = min((q1_quirk and q == 1) ? 0 : isqrt(q), level_per_track)`（`13-algorithms/01` F-PROG-01）。原版 `exp_divisor = 30`、`q1_quirk = true`：16.16 定点开方对 `q <= 1` 原样返回，`1/256 = 0`，所以 exp 30–59 得 0 级。加经验时每段 `clamp(0, exp + d, exp_track_cap)`，原版 300000 恰好 = `exp_of_level(level_per_track=100)`。`[确认]` `fn_1400146c3`、`fn_140011c34`。公式与饱和策略在 `config/formulas/progression.yaml`，见 `11-progression`。
- HP 是存档字段（`+0x20`），上限 = 生命力基础值（`07-fatigue-health-mood.md`）；技能 / 心情不存。
- 技能不存。查询时 `Characters.skill(person_id, skill_key)` 用当前六维 + 等级 + `formulas/character.yaml` 计算，见 `03-abilities-skills-jobs.md`。
- 身体 / 精神状态是存档字段（`+0x0D` / `+0x0E`），每日转移规则见 `07`。

`+0x70` 的总等级是缓存。读档后必须能从两段 `exp` 重建；写档可写缓存，但不得当作源。

## 5. 耦合分析

原版硬编码：

- 经济 / 地方舰队买货直接 `Skill(captain, 4)`（**会计术**，不是能力「精神」）做 `1000 - a` 折扣（`exe_caps.md` 的「Ability」是误名，见 `03`）。
- 战斗结算 `500 - a`、道具价 `100 - a/50`。
- 技能公式把六维下标写死在 exe 里。
- `NUM_PERSON = 235` 出现在循环与断言；「完整存档的人 < 87」写死在 `vfunc_06/07` 的 `cmp 0x57`；`CPerson::vfunc_02` 另有 `id < 0x62`（98）分界。`SAVE_SAILOR_COUNT = 98` 只是工具常数。
- 世界表 packed struct，逻辑靠偏移；World0 / World1 是两份二进制而不是两份内容包。
- 舱室岗位与船体房间在同一套 UI（`CCrewView`）里搅在一起。

重构切断点：对外只出 key 与修正器键（`skill.<key>`）；舱室几何留给船；经验增量留给 progression。

## 6. 解耦后的配置

落点：`config/characters/persons.yaml`（本系统）。World0 / World1 是两份内容包，同一 schema。

```yaml
# config/characters/persons.yaml
schema: dk4.characters.persons
version: 1
pack: vanilla-world0

persons:
- key: vanilla:raphael_castro    # 主键；定义行也写全命名空间（词表 §14-4）
  vanilla_index: 0               # 原版 ePerson，只给 legacy_codec / 存档导入
  given: 拉斐尔
  family: 卡斯特罗
  given_i18n: { zh-CN: 拉斐尔 }
  family_i18n: { zh-CN: 卡斯特罗 }
  name3: ""
  category: protagonist          # protagonist | officer | job_npc | crew_template | event | capital_lady
  sex: male                      # 世界 +0x26：1 = female，其它 = male
  stats: { vanilla:stamina: 59, vanilla:agility: 60, vanilla:charm: 70, vanilla:intelligence: 62, vanilla:spirit: 58, vanilla:luck: 61 }   # 世界 +0x20
  init_exp: [100, 100]           # 世界 +0x18/+0x1C
  zodiac: vanilla:pisces         # 世界 +0x29 = 2；random 表示原版 13
  init_force: vanilla:castro     # 世界 +0x2A = 0；null 表示原版 26
  bustup: 0                      # 世界 +0x2B；表现层拥有，见 05
  recruitable_roster: true       # 内容包标记，不由 id 区间推
  unknown_bytes:                 # 未命名字节原样保存
    w27: 4
    w28: 2
    w2c: 1
    w30: 0
```

职能 NPC 不写六维（全 0）；用 `job_npc_role` + `culture_key` 供城镇设施索引：

```yaml
- key: vanilla:bartender_europe
  vanilla_index: 108
  category: job_npc
  job_npc_role: bartender        # plaza_shop / bartender / bar_patron / exchange / shipyard / dock / guard / ruler / clergy / innkeeper / guild / crew_template / ...
  culture_key: vanilla:europe    # NPC 脸分组（factions/npc_faces），不是海域
  recruitable_roster: false
```

全局上限（原版包默认，写入 `config/caps.yaml`；键名以 `00-overview/10-key-glossary.md` §7 为准）：

```yaml
ability_cap: 100                 # fn_1400062da
skill_value_cap: 500             # fn_1401cbb40；旧名 ability_settled_cap 作废
level_per_track: 100             # fn_1400146c3 r8d=0x64
track_count: 2
exp_track_cap: null              # null = exp_of_level(level_per_track)；原版 300000（fn_140011c34）
```

六维个数不进 caps（`abilities.yaml` 表长）；装备槽数不进 caps，只读 `items/effects.yaml` 的 `slot_groups` 槽总数（原版 5，`09-items/02` §6）。

## 7. 独立旋钮

改这些不应迫使经济 / 战斗 / 航海改代码：

- 某个人的姓名、六维初值、初始经验
- 名册长度（引擎按表长；UI 自己处理滚动）
- 装备槽数（原版 5；内容包可加，显示层自适应）
- World0 / World1 换包（同一 schema）
- 把某职能 NPC 从「欧洲掌柜」换成「新文化掌柜」

会迫使别的系统改动的（禁止当旋钮）：

- 六维含义（业务只认 `skill.<key>`，六维只进技能公式）
- `person_id` 稳定别名（存档外键）

## 8. 对外接口

只传 ID 与值。

查询：

```
Characters.get(person_id) -> PersonSnapshot
  { key, given, family, name3, sex, zodiac, birthday?,
    abilities{key: int}, exp[2], level_cache, affiliation_force,
    equip_items[5] (item key | null), hp, hp_max, mental_state, physical_state,
    in_roster, in_party }

Modifiers.eval("skill.<key>", { person })     # 见 03；经济/战斗只许走注册表
Characters.skill(person_id, skill_key) -> int # 上式便捷封装；派生，不读盘
Characters.zodiac(person_id) -> zodiac_key    # 主角由生日现算，其他人取定义

Characters.roster() -> [person_id]             # 隶属==玩家 且 槽有效
Characters.in_party() -> [person_id]           # 判据进 YAML；原版引擎判据未还原
```

命令（由叙事 / 城镇发，人物系统执行）：

```
Characters.recruit(person_id, force_id) / dismiss(person_id)
PersonStatsReplaced { person_id, abilities[6] }   # 开局/事件，不给经济用
```

别系统的命令 / 事件（人物只存结果或做缓存失效）：

```
Progression.grant_exp(person_id, add{track: n}, roll_mode, source)   # progression 执行，写两段 exp
PersonEquipped / PersonUnequipped { person_id, slot, item_id }        # items 发；人物使 skill.* 失效
```

事件（人物系统发出）：

```
PersonRecruited / PersonDismissed { person_id }
RosterChanged { person_id, in_roster, in_party }
```

禁止：`Economy` 拿 `abilities[2]`（魅力）自己算折扣；`Combat` 拿 `CPerson*`。

## 9. 未核实字段

世界表 0x38 已命名 vs 残余：

| 偏移 | 宽 | 状态 | 说明 |
| --- | --- | --- | --- |
| +0x00 | ptr8 | `[确认]` | given |
| +0x08 | ptr8 | `[确认]` | family |
| +0x10 | ptr8 | `[确认]` | name3，可 0 |
| +0x18 | u32 | `[确认]` | 初始经验段 0；`world.h` 仍叫 pad，应改名 |
| +0x1C | u32 | `[确认]` | 初始经验段 1 |
| +0x20 | 6×u8 | `[确认]` | 六维 |
| +0x26 | u8 | `[确认]` | 性别 0 男 / 1 女（`fn_140006b90`） |
| +0x27 | u8 | `[未知]` | → `CPerson+0x09`。军官约 0–20。像文化/国籍/jobkind，未与 `eCountry`/`eJobKind` 对上 |
| +0x28 | u8 | `[未知]` | → `CPerson+0x0A`。军官 0–8，NPC 多为 0 |
| +0x29 | u8 | `[确认]` | 星座 0–11，13 → `id % 12`；→ `CPerson+0x0B`，`vfunc_11` 读 |
| +0x2A | u8 | `[确认]` | 初始隶属 `force_id`，26 = 无所属；→ `CPerson+0x0C`，进存档 |
| +0x2B | u8 | `[确认]` | 半身像下标 → `CPerson+0x28`（`05-portraits-animation.md`） |
| +0x2C | u8 | `[未知]` | → `CPerson+0x29`；0–6 |
| +0x2D..+0x2F | 3×u8 | `[未知]` | 0–97 全 0 |
| +0x30 | u32 | `[未知]` | → `CPerson+0x2C`（整 4 字节拷）。0–97 只见 0/1。World0 为 1：4、6、19、30、48、55、74–86；World1 为 1：0、1、2、3、19、30（4、6、48、55、74–86 改为 0）。World1 把 48/55 由无所属改入势力 22/23 时同时清此位，把华梅/希恩改成无所属时华梅置位 |
| +0x31..+0x37 | 7×u8 | `[未知]` | 0–97 全 0 |

存档 `CPerson` 记录（ids 0–86，基址 `0x2C03 + id×25`）：

| 记录偏移 | 宽 | 对象字段 | 状态 |
| --- | --- | --- | --- |
| +0x00 | 1 | `+0x0C` 隶属 | `[确认]` |
| +0x01 | 1 | `+0x22` | `[确认]` 字段位置；语义 `[未知]` |
| +0x02 | 6 | `+0x0F` 六维 | `[确认]` |
| +0x08 | u32 | `+0x18` 经验段 0 | `[确认]` |
| +0x0C | u32 | `+0x1C` 经验段 1 | `[确认]` |
| +0x10 | u16 | `+0x20` HP | `[确认]`（样本：拉斐尔 79 = 生命力基础值） |
| +0x12 | 1 | `+0x0D` 身体状态 | `[确认]` |
| +0x13 | 1 | `+0x0E` 精神状态 | `[确认]` |
| +0x14 | 5 | `+0x23` 装备，空 `0xE8` | `[确认]` |

ids 87–234 只有 `+0x00` / `+0x01` 两字节。

仍未知：`CPerson+0x09/+0x0A/+0x29/+0x2C` 的语义；`+0x22` 的语义；`CPlayerPerson +0x78`（样本全 0）、`+0x7C`（样本 50）、`+0x7D`（样本 `0xE8`，像一个额外道具槽）。
