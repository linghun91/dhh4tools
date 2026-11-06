# 人物模型

人物系统的根文档。管「一个 `person_id` 是什么、三层数据怎么切、表怎么落」。六维/技能/岗位见 `03-abilities-skills-jobs.md`，剧本主角见 `02-protagonists-scenarios.md`，招募与名册见 `04-recruitment-crew.md`。

## 1. 职责边界

本系统管：

- 人物定义（姓名、六维初值、性别、世界表残余字段）
- 人物状态（两段经验、隶属、装备槽、疲劳、身心状态）
- 把定义 + 状态投影成 **ModifierSet**（原则 6），供经济 / 战斗 / 航海查询
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
- `[确认]` `CPerson::vfunc_06` / `vfunc_07` 成对读写对象字段：`+0x0C` 一字节、`+0x22` 一字节、六维循环 6 次落在 `+0x0F`、两个 `uint32` 在 `+0x18` / `+0x1C`、`+0x20` 一处、`+0x0D` / `+0x0E` 各一字节、5 个装备槽从 `+0x23` 起。来源：`CPerson.cpp`。
- `[确认]` `CPlayerPerson` 在 `CPerson` 之上再串行化三段 17 字节（`+0x30` `+0x41` `+0x52`）、`+0x63`/`+0x64` 两字节、19 字节 `+0x65`、`uint32 +0x78`、两字节 `+0x7C`/`+0x7D`。来源：`CPlayerPerson.cpp`。
- `[确认]` 存档名册 98 槽、步长 0x19、基址 `+0x2C00`。来源：`Constants.h` `SAVE_SAILOR_COUNT = 98`、`save_format.md`、`Save.cpp`、`dk4save.c`。
- `[确认]` 名册槽 `i` 的水手 id 就是 `person_id i`（0–97）。`dk4save.c` `kSailorName[98]` 与 `PersonID.h` 0–97 一一对应。
- `[确认]` 主角另有一份「状态画面体」：隶属 `+0x35AA`、六维 `+0x35AC`、两段经验 `+0x35B2` / `+0x35B6`；`+0x20` 是主角的名册 id；`+0x70` 是缓存的两段等级之和。改主角必须三处一起写。来源：`save_format.md`。
- `[确认]` 已招收：该槽隶属 == 主角隶属。在队：六维之和 > 0。来源：`save_format.md`、`Save.cpp`。
- `[确认]` 姓名块 `+0x35C3` 步长 0x2E9：index 0 = 主角 GBK 名（名 `+0x00`、姓 `+0x11`、中间名 `+0x22`）；1..98 = 水手 id−1，其姓名仍来自世界表而非此块。
- `[确认]` `NUM_PERSONEQUIP = 5`。来源：`PersonEquipID.h`。官方手册：武器、防具、装备道具 ×3。
- `[确认]` 世界 0–6 是七个剧本提督，98 起大量职能 NPC。来源：`PersonID.h` 注释、`id-registry.md`。
- `[推断]` 名册 98 槽覆盖且仅覆盖 `person_id` 0–97；98–234 永不进入这 98 槽。依据：槽数 == 0–97 的闭区间长度，且 `kSailorName` 止于宗九郎（id 97）。
- `[推断]` 世界表 `+0x18`/`+0x1C` 两个 `uint32` 是**初始两段经验**。依据：与存档经验同宽；拉斐尔两值均为 100（任务所述）；`isqrt(100/30)=1`；赫德拉姆 2400/5400、华梅 12600/7400 得到的单段等级与「新手 / 老将」叙事一致；World1 把若干人的这两值与六维一起调低。`world.h` 现名 `pad18[8]`，`Person.cpp` 未命名，故不升为确认。
- `[推断]` 世界表 `+0x26` 是性别：0 男、1 女。依据：拉斐尔/赫德拉姆/杏太郎/伍丁=0，丽璐/华梅/蒂雅=1；0–97 中 83/15；exe 在能力名旁放「女」「男」二字串。`NUM_SEX` 头文件未写出整数。

## 3. 实体与 ID

| 实体 | 原版枚举 | 计数 | 空/非法 | 稳定 key 例 |
| --- | --- | --- | --- | --- |
| 人物 | `ePerson` | 235 | `INVALID_PERSON = -1` | `raphael_castro` |
| 能力 | `eAbility` | 6 | `INVALID_ABILITY = -1` | `vitality` |
| 技能 | `eSkill` | 10 | `INVALID_SKILL = -1` | `surveying` |
| 装备槽 | `ePersonEquip` | 5 | `INVALID_PERSONEQUIP = -1` | `equip.weapon` |
| 名册槽 | （存档数组） | 98 | 无独立枚举 | `roster[i] → person_id i` |

运行时以表长循环，禁止写死 `235` / `98`。原版包的实例计数记在 `caps.yaml`：`person_count_vanilla: 235`、`sailor_roster: 98`。

### 三层：世界定义 / 存档状态 / 运行时对象

```
World DLL  SPerson (0x38, 只读内容包)
    │  开局 / 读档缺字段时
    ▼
Save       名册行 0x19 + 主角体 + 姓名块
    │  读档
    ▼
Runtime    CPerson / CPlayerPerson（派生技能、HP、心情、ModifierSet）
```

| 层 | 持有 | 不持有 |
| --- | --- | --- |
| 定义（`persons.yaml`） | 姓名、六维初值、性别、初始经验、hex 残余 | 当前经验、隶属、装备 |
| 状态（存档） | 两段 `exp`、六维当前值、隶属、装备 5 槽、疲劳/身心 | 技能值、等级（可缓存但必须能从 `exp` 重建） |
| 运行时 | 定义 + 状态 + 派生缓存 | 不得把公式常数写死在对象里 |

原则 4：定义进内容包，状态进存档。禁止开局把整张人物表拷进存档再改定义字段。

### 人物分类（原版 235）

| 类别 | id 范围 | 特征 | 名册 |
| --- | --- | --- | --- |
| 可操作主角 | 0–6 | 七剧本提督，有完整六维与姓名 | 槽 0–6，开局由剧本写入 |
| 具名航海士 | 7–97 | 有姓名；多数有六维；95–97（爱斯法妮雅、特欧贝尔德、宗九郎）六维全 0 | 槽 7–97 |
| 职能 NPC | 98–166、178–191、203–212 | 六维全 0，按设施/文化复用立绘 | 不入名册 |
| 属下模板 | 167–177 | 六维全 40，欧洲/非洲/阿拉伯/印度/东南亚/中国/韩国/日本/中南美 + 预备 1/2 | 不直接占槽；雇佣无名航海士时作模板 `[推断]` |
| 事件/杂类 | 192–202 | 私设舰队、教祖、村民、土匪、妖怪、探险家、安吉鲁/斯耐克/丹第/长老 | 不入名册 |
| 首都女性 | 213–234 | 名+港口姓（玛丽·伦敦 … 伊莎贝尔·哈瓦那），六维 0 | 不入名册 |

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
- 单段等级 `level = isqrt(exp / exp_divisor)`，原版 `exp_divisor = 30`，单段设计上限 100、实现硬顶 255。公式与饱和策略在 `config/formulas/progression.yaml`，见 `11-progression`。
- 技能不存。查询时 `Characters.skill(person_id, skill_id)` 用当前六维 + 等级 + `formulas/character.yaml` 计算，见 `03-abilities-skills-jobs.md`。
- 健康 / 心情不存。由疲劳与身心状态派生；公式未从 cpp 还原的部分进「未核实」。

`+0x70` 的总等级是缓存。读档后必须能从两段 `exp` 重建；写档可写缓存，但不得当作源。

## 5. 耦合分析

原版硬编码：

- 经济 / 地方舰队买货直接 `Ability(captain, 4)`（精神）做 `1000 - a` 折扣（`exe_caps.md`）。
- 战斗结算 `500 - a`、道具价 `100 - a/50`。
- 技能公式把六维下标写死在 exe 里。
- `NUM_PERSON = 235`、`SAVE_SAILOR_COUNT = 98` 出现在循环与断言。
- 世界表 packed struct，逻辑靠偏移；World0 / World1 是两份二进制而不是两份内容包。
- 舱室岗位与船体房间在同一套 UI（`CCrewView`）里搅在一起。

重构切断点：对外只出 ID 与 ModifierSet；舱室几何留给船；经验增量留给 progression。

## 6. 解耦后的配置

落点：`config/characters/persons.yaml`（本系统）。World0 / World1 是两份内容包，同一 schema。

```yaml
# config/characters/persons.yaml
schema: dk4.characters.persons
version: 1
pack: vanilla-world0

- id: 0
  key: raphael_castro
  given: 拉斐尔
  family: 卡斯特罗
  given_i18n: { zh-CN: 拉斐尔 }
  family_i18n: { zh-CN: 卡斯特罗 }
  name3: ""
  category: protagonist          # protagonist | officer | job_npc | crew_template | event | capital_lady
  sex: 0                         # 0 男 1 女；[推断] 自 +0x26
  abilities: [59, 60, 70, 62, 58, 61]   # 体力 敏捷 魅力 智力 精神 运气
  init_exp: [100, 100]           # [推断] 世界表 +0x18/+0x1C
  recruitable_roster: true       # id < 98
  unknown_bytes:                 # 世界表 +0x26 起 18 字节，命名前原样保存
    offset: 0x26
    hex: "000402020000010000000000000000000000"
```

职能 NPC 不写六维（全 0）；用 `job_npc_role` + `culture_key` 供城镇设施索引：

```yaml
- id: 108
  key: bartender_europe
  category: job_npc
  job_npc_role: bartender        # plaza_shop / bartender / bar_patron / exchange / shipyard / dock / guard / ruler / clergy / innkeeper / guild / crew_template / ...
  culture_key: europe
  recruitable_roster: false
```

全局上限（原版包默认，写入 `config/caps.yaml`）：

```yaml
stat_count: 6
sailor_roster: 98
person_equip_slots: 5
ability_growth_cap: 100
ability_settled_cap: 500
```

## 7. 独立旋钮

改这些不应迫使经济 / 战斗 / 航海改代码：

- 某个人的姓名、六维初值、初始经验
- 名册长度（引擎按表长；UI 自己处理滚动）
- 装备槽数（原版 5；内容包可加，显示层自适应）
- World0 / World1 换包（同一 schema）
- 把某职能 NPC 从「欧洲掌柜」换成「新文化掌柜」

会迫使别的系统改动的（禁止当旋钮）：

- 六维含义（必须走 ModifierSet）
- `person_id` 稳定别名（存档外键）

## 8. 对外接口

只传 ID 与值。

查询：

```
Characters.get(person_id) -> PersonSnapshot
  { id, key, given, family, name3, sex, abilities[6],
    exp[2], level_cache, affiliation_force_id,
    equip_item_ids[5], fatigue, mental, physical,
    in_roster, in_party }

Characters.modifiers(person_id) -> ModifierSet
  # 见 03-abilities-skills-jobs.md；经济/战斗只许调这个

Characters.skill(person_id, skill_id) -> int   # 派生，不读盘

Characters.roster() -> [person_id]             # 隶属==玩家 且 槽有效
Characters.in_party() -> [person_id]           # 六维和 > 0
```

命令（由叙事 / 城镇 / progression 发，人物系统执行）：

```
PersonRecruited     { person_id, force_id }
PersonDismissed     { person_id }
PersonStatsReplaced { person_id, abilities[6] }   # 开局/事件，不给经济用
ExperienceGranted   { person_id, track, amount }  # 只接受 progression 的量
PersonEquipSet      { person_id, slot, item_id }
```

事件（人物系统发出）：

```
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
| +0x18 | u32 | `[推断]` | 初始经验段 0；`world.h` 仍叫 pad |
| +0x1C | u32 | `[推断]` | 初始经验段 1 |
| +0x20 | 6×u8 | `[确认]` | 六维 |
| +0x26 | u8 | `[推断]` | 性别 0/1 |
| +0x27 | u8 | `[未知]` | 军官约 0–20，职能 NPC 大量为 20。像文化/国籍/jobkind，未与 `eCountry`/`eJobKind` 对上 |
| +0x28 | u8 | `[未知]` | 军官 0–8，NPC 多为 0 |
| +0x29 | u8 | `[未知]` | 0–13 |
| +0x2A | u8 | `[未知]` | 军官常见 26（49/98），主角各不相同。26 可能是哨兵 |
| +0x2B | u8 | `[未知]` | 早期军官近似递增，像立绘/bustup 号，未对上 `eBustup` |
| +0x2C | u8 | `[未知]` | 0–6 |
| +0x2D..+0x2F | 3×u8 | `[未知]` | 0–97 全 0 |
| +0x30 | u8 | `[未知]` | 0/1 标志。World0：杏太郎、蒂雅、格尔哈特、74–86 等为 1；World1 拉斐尔/赫德拉姆/丽璐/华梅 亦为 1 |
| +0x31..+0x37 | 7×u8 | `[未知]` | 0–97 全 0 |

存档名册一行 0x19：

| 偏移 | 宽 | 状态 |
| --- | --- | --- |
| +0x00..+0x02 | 3 | `[未知]` |
| +0x03 | 1 | `[确认]` 隶属 |
| +0x04 | 1 | `[未知]` |
| +0x05 | 6 | `[确认]` 六维 |
| +0x0B | u32 | `[确认]` 经验段 0 |
| +0x0F | u32 | `[确认]` 经验段 1 |
| +0x13..+0x18 | 6 | `[未知]` 可能含岗位/船/身心，未从 `CPerson` 对上 |

`CPerson` 对象 `+0x0C` `+0x0D` `+0x0E` `+0x22`、`+0x20` 的 `fn_140004421` 宽度、`CPlayerPerson +0x30` 起的三段 17 字节：全部 `[未知]`。`vfunc_04` 比较 `+0x0D == 6`，6 既是 `NUM_ABILITY` 也可能是某状态枚举，不猜。

生日（mesfile `0xbab`「请输入生日」）的存档偏移 `[未知]`。
