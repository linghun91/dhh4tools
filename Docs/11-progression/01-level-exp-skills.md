# 等级、经验、技能

经验是人物成长的唯一存档源。等级与技能值一律派生，禁止再当独立状态写回。

分工（与 `03-characters/03` 对齐）：本系统拥有**经验 → 等级**与**升级成长**；技能值是修正器键 `skill.<key>`（`00-overview/03` 原则 6 / D2），`base` 段公式归 characters，装备是 `add` 段贡献（items），身体态是 `mul` 段贡献（vitals）。本篇第 2、4.3 节保留技能链的原版证据，供对拍。

## 1. 职责边界

本系统管：

- 人物两条经验轨道的增减、封顶、溢出语义（经验字段存在人物状态里，只由本系统写）
- 由经验派生单轨等级、总等级（技能 `base` 段的输入）
- 升级时的六维成长滚动（星座表、轨→维列表）
- `grant_routes`：哪个事件给谁哪条轨加多少
- 开局「3 种熟练技能」的选择接口（只记账，不解释战斗/贸易怎么用）
- `caps.yaml` / `formulas/progression.yaml` 里与等级、成长有关的参数

不管：

- 六维基础值从哪张人物表来（`characters`）
- 技能值求值：`Modifiers.eval("skill.<key>", {person})`；`base` 公式与配对表在 `03-characters/03`，装备贡献在 `09-items/02`，身心系数在 `characters/vitals.yaml`（`03-characters/07`）
- 身体态 / 精神态怎么变化（`characters`）
- 玩法本身（贸易、战斗、医疗……）：它们只发事件或 `Progression.grant_exp` 命令，增量由本系统的 `grant_routes` 决定
- 技能值如何变成买货折扣、炮术命中（各系统按修正器键求值，不读经验原值）
- 名声、发现称号（`02-fame-titles.md`）
- UI 怎么画六边形与技能条（`12-ui-save`，只读投影）

## 2. 原版事实

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 存档人物段：从 `0x2C03` 起按 `ePerson` 0–234 共 235 条 `CPerson` 记录；id < 87（`0x57`）每条 25 字节，其余每条 2 字节（只有隶属与 `+0x22`）；`0x35AA` 接主角 `CPlayerPerson` 记录 | `[确认]` | `CPerson::vfunc_06/07`（body `0x1401cb480` / `0x1401caf70`，`cmp eax,0x57`）；`CPersonElement::vfunc_01`；`03-characters/01` §2 |
| dk4save 的「名册 98 行，步长 `0x19`，基址 `0x2C00`」是工具常数，不是引擎结构：行 i 比记录 i 早 3 字节，ids 0–86 字段对得上；行 87–97 落在 2 字节区，是错读，写入会损坏别人的隶属字节 | `[确认]` | `save.h` `save_format.md` `SAVE_SAILOR_COUNT=98`；对照 `03-characters/01` §2 |
| 记录 `+0x02` 六维各 1 字节：体力 敏捷 魅力 智力 精神 运气（dk4save 行 `+0x05`） | `[确认]` | `CPerson::vfunc_06` `0x1401cb50d`；`AbilityID.h` |
| 记录 `+0x08` / `+0x0C` 两条 `uint32` 经验（dk4save 行 `+0x0B` / `+0x0F`） | `[确认]` | `CPerson::vfunc_06` `0x1401cb529` / `0x1401cb542` |
| 主角另有一份 `CPlayerPerson` 记录：隶属 `0x35AA`、六维 `0x35AC`、经验 `0x35B2` / `0x35B6` | `[确认]` | `CPlayerPerson::vfunc_06`（body `0x1401e0dc0`，前 25 字节即 `CPerson` 部分）；状态画面读这份 |
| `save+0x70` 缓存主角**总等级**（两轨之和） | `[确认]` | `save_format.md` `dk4save.c` `write_cached_level` |
| `save+0x20` 主角 `person_id`（0 拉斐尔 … 5 亚伯拉罕） | `[确认]` | `save.h`；改主角时 `CPerson` 记录、`0x35AA` 记录与 `+0x70` 必须一起写 |
| 已招收：人物隶属 == 主角势力；在队：六维之和 > 0 | `[推断]` | `Save.cpp` `dk4save.c` 工具判据；exe 里未见同样判定（`03-characters/01`） |
| 运行时人物对象：六维 `uint8` 在 `+0x0F..+0x14`，两轨经验 `uint32` 在 `+0x18`（轨 A）/ `+0x1C`（轨 B），身体态 `+0x0D`，精神态 `+0x0E` | `[确认]` | 指令：`fn_1400090ca`（body `0x1401c88c0`，`this+0xf+e`）、`fn_1400146c3`（`0x1401c97a0`→`fn_140004507` 读 `+0x18`）、`fn_140012085`（`0x1401c92b0`→`fn_14000d29c` 读 `+0x1c`）、`fn_14000f0fb`（`+0xd`）、`fn_14000c4a5`（`+0xe`） |
| 存档记录 `+0x08/+0x0C` 与运行时 `+0x18/+0x1C` 一一对应（顺序相同）；记录六维 `+0x02` ↔ 运行时 `+0x0F` | `[确认]` | `CPerson::vfunc_06` 依次写对象 `+0x0F`×6、`+0x18`、`+0x1C`（`0x1401cb50d..0x1401cb542`） |
| 单轨等级 = `clamp_u(isqrt_fx(exp/30) / 256, 0, 100)`；`isqrt_fx(q)`：`q <= 1` 时**原样返回 q**，否则 `floor(sqrt(q*0x10000 mod 2^32))`（Newton 迭代） | `[确认]` | 指令：`fn_140012085` / `fn_1400146c3`：`div 0x1e` → `call fn_14000819d`（body `0x140140d30`）→ `div 0x100` → `fn_14001377d(0, v, 0x64)`（body `0x1401c7e10`，无符号 `ja` 夹取） |
| 推论：`exp 30–59`（`q=1`）原版显示 **0 级**，不是 1 级；其余 `q` 等价 `isqrt(q)` | `[确认]` | 同上；`Save.cpp` `level_of_exp` 是工具重写，对 `q=1` 返回 1，**与 exe 不一致** |
| `exp_of_level(1)=30` 在原版只到 0 级；1 级最低经验是 60 | `[确认]` | 同上 |
| 总等级 = 轨 A 等级 + 轨 B 等级 | `[确认]` | `fn_1400123dc`（body `0x1401c9230`） |
| 等级 100 封顶是取等级时夹取（两处 `mov r8d,0x64`），不是存档字段 | `[确认]` | `0x1401c92f5` `0x1401c97e5`；`exe_caps.md` |
| 补丁把等级上限抬过 255 时：`q >= 65536`（`exp >= 1966080`）乘 `0x10000` 回绕，等级变成 `isqrt(q mod 65536)`；`q` 为 65536 的整倍数时 `n=0`，牛顿迭代首步 `div [x=0]` **除零崩溃** | `[确认]` | `0x140140d60` `imul eax,[..],0x10000` 32 位；`0x140140d82` `div DWORD PTR [rsp+0x8]`（初值 `x=n`）；与 `13-algorithms/01` F-PROG-01 一致；工具硬顶 255 由此而来 |
| 加经验 `fn_140011c34(person, add_A, add_B, show)`（body `0x1401c8000`）：每轨 `clamp_u(exp+add, 0, 300000)` | `[确认]` | 两处 `mov r8d,0x493e0`；`exe_caps.md`「四处相同立即数」= 本函数 2 处 + `fn_140008387` + `fn_140006c67` |
| 包装：`fn_1400026d5(p,a,show)` = 只加轨 A；`fn_140006e88(p,b,show)` = 只加轨 B | `[确认]` | body `0x1401c8400` / `0x1401c8230`：另一参数 `xor` 置 0 |
| 轨 A 升级 → 成长列表 `{0,1,2}` 体力 敏捷 魅力；轨 B 升级 → `{3,4,5}` 智力 精神 运气 | `[确认]` | `fn_1401cb8d0` 读 `.data 0x1406863f0 = {0,1,2}`；`fn_1401cb7f0` 读 `0x140686400 = {3,4,5}` |
| `fn_140011c34`：本次调用某轨等级上升（`old < new` 有符号）就把该轨 3 项加入列表，**不论升了几级**只滚一次 | `[确认]` | `0x1401c809d` / `0x1401c8106` `jge` 跳过；之后单次 `fn_1400065fa` |
| `fn_140008387`（只加轨 A）/ `fn_140006c67`（只加轨 B）：按**总等级**升了几级就滚几次 | `[确认]` | 反编译 `fn_1401c8290.cpp`：`s24 = total_after - total_before`，循环 `s74 < s24` 调 `fn_1400065fa` |
| 每次成长滚动：对列表每项 `e`：`up = rand(2) + rand(g+3)/2`，`g = 星座成长表[sign][e]`；`up > 0` 才 `base[e] = clamp(base[e]+up, 0, 100)` | `[确认]` | `fn_1400065fa`（body `0x1401ca9c0`）`0x1401cac89..0x1401cacfd`；`rand(n)` = `fn_140005272`（`rand()%n`，`n<=0` 返回 0）；`fn_1400062da`（body `0x1401c7f20`）`fn_1400103d4(0,v,0x64)` 有符号夹取 |
| 滚动后若 HP 上限（生命力 `base`，不计精神）变大，把差值加到当前 HP | `[确认]` | `fn_1400065fa` 尾部；`03-characters/03` §2、`07` 2.3a |
| 星座成长表：12 星座 × 6 维，每格 0/1/2，每行和恰为 6；`sign = person->vfunc(+0x58)` = `vfunc_11` | `[确认]` | `fn_14000b852`（body `0x1402aeef0`）`0x140688970` 步长 `0x20`：`{char* name, int g[6]}`，`<0xc` 越界返回 NULL；取格 `fn_140001b8b` `<6` |
| `sign` 来源：`CPerson::vfunc_11` 返回 `+0x0B`（世界表 `+0x29`，13 → `id % 12`）；`CPlayerPerson::vfunc_11` 由生日现算（分界表 `0x140688ca0`） | `[确认]` | `03-characters/01-person-model.md` §2（`fn_140014709`、`fn_1400034b3`） |
| 成长消息：`show != 0` 且人物属玩家时提示 msg `0x169`「%s，等级上升！！」与 `0x16a`「%s上升了%s！」 | `[确认]` | `fn_1400065fa` `fn_14000fb14(0x169)` / `fn_1400012da(0x16a)`；msg id 解码见下 |
| 技能不存档，现场算；HP（`+0x20`）、身体态（`+0x0D`）、精神态（`+0x0E`）**进存档** | `[确认]` | `03-characters/07` 2.3（存档样本 HP 79 验算）；旧稿「健康、心情不存档」错 |
| 基础技能 `fn_140009214(p, skill, use_mood)`（body `0x1401c9d50`）：`L = total_level; if L > 200 (有符号) L = 200; v = (S(a)+S(b)) * (L+100) / 150`（有符号 `idiv`），再 `clamp(v, 0, 500)` | `[确认]` | `cmp 0xc8 / mov 0xc8`、`add 0x64`、`mov ecx,0x96; idiv`、`call fn_1401cbb40` |
| `S(x)`：`use_mood=0` 取基础六维字节；`use_mood=1` 取精神态修正后的值（见 4.3） | `[确认]` | `fn_1400090ca` / `fn_14000e098`（body `0x1401ca070`） |
| 技能 → 两维配对表 `0x1406864a0`（10×2 int），越界返回 7 | `[确认]` | `fn_1400099e9`（body `0x1401ca010`）`skill < 0xa`、`idx < 2` |
| 结算技能 `fn_1400135cf(p, skill, cabin)`（body `0x1401ca250`）：`v = 基础技能(use_mood=1) + Σ5 槽装备加成` → 身体态修正 → `clamp(v,0,500)` | `[确认]` | `fn_14001406a`→`fn_140009214(...,1)`；`fn_14000e0b6` 循环 `ePersonEquip<5`；`fn_1401c87a0`；`fn_1401cbb40` |
| 对外取技能 `fn_14000e1ce(p, skill)`：人物属玩家势力时 `cabin = 所在舱室种类`，否则 `cabin = 0x18`（无） | `[确认]` | body `0x1401c9f60`：`mov [rsp+0x20],0x18`；`fn_14000b9fb` |
| `exe_caps.md`「属性结算 500」补丁点 `fn_1401cbb40` 只被两个技能函数调用 → 实为**技能值上限 500**，不是六维结算 | `[确认]` | xref：`0x1401c9e4a`（`fn_140009214`）、`0x1401ca2c5`（`fn_1400135cf`），无其它调用 |
| `exe_caps.md`「技能封顶 200」= 上式等级项 `L` 的上限（两处 `0xc8`） | `[确认]` | `0x1401c9d85` `0x1401c9d8f` |
| 基础属性成长上限 100（`fn_1400062da`） | `[确认]` | `exe_caps.md` 补丁点 `0x1401c7f75` |
| 未找到「六维 + 装备」的结算路径；装备只加到技能 | `[推断]` | `fn_1400135cf` 是唯一加装备的地方；六维取值点 `fn_1400090ca` 只读字节。全量 xref 未做 |
| 存档六维是 `uint8`，物理范围 0–255 | `[确认]` | 存档记录 6 字节；`Person.h` `LEVEL_MIN/MAX` 挂在 `UCHAR` |
| `NUM_ABILITY=6` `INVALID_ABILITY=-1` | `[确认]` | `AbilityID.h` |
| `NUM_SKILL=10` `INVALID_SKILL=-1` | `[确认]` | `SkillID.h`；`CPersonInformationView` / `CGoodSkillMenuData` 循环 `>= 0xa`；`fn_1400099e9` `< 0xa` |
| 符号 `NUM_SKILLABILITY` 存在，计数值未还原；但「技能→两维」配对表已在 exe `.data 0x1406864a0` 读出 | `[确认]` | `SkillabilityID.h`；配对表见下 |
| `CGoodSkillMenuData` 按 `eSkill` 0–9 取 10 条说明，开局文案「请选择３种熟练的技能」 | `[确认]` | `CGoodSkillMenuData.cpp` `mesfile_sc.txt` `0xb63` |
| 10 条说明在 SC exe `.rdata` 连续存放 | `[确认]` | `DK4HD_sc.exe` 文件偏移 `0x5910c0` 起，见下表 |
| 技能短名表：exe 指针表 `0x140686390`（10 项） | `[确认]` | 取名函数在 `0x1401ca308` 处 `lea` 该表；GBK 串 `0x14057fd24` 起 |
| 六维名表 `0x140686350`（体力 敏捷 魅力 智力 精神 运气） | `[确认]` | `0x1401c8928` |
| 两轨的玩法语义（航海 / 战斗）——原版无轨名 | `[未知]` | 仓库无轨名符号；已确认的只有「A 轨长体敏魅、B 轨长智精运」与调用点增量（4.5） |
| 熟练技能的数值效果（成长加速？初始加成？） | `[未知]` | 只有选 3 个的 UI 与确认句 `是%s﹑%s﹑%s吗？` |

`CGoodSkillMenuData::vfunc_01` 的 10 条说明（`eSkill` 下标 = 字符串顺序），短名来自 exe 短名表，两维来自配对表 `0x1406864a0`：

| id | 原文 | 短名 `[确认]` | key | stat_a + stat_b `[确认]` |
| --- | --- | --- | --- | --- |
| 0 | ＨＰ最大值，对疾病等的抵抗力。 | 生命力 | `vitality` | 体力(0) + 精神(4) |
| 1 | 舰长、提督的资质，指挥能力。 | 统率力 | `leadership` | 魅力(2) + 精神(4) |
| 2 | 驾驶的资质，航海术、造船能力。 | 驾驶术 | `helmsmanship` | 敏捷(1) + 精神(4) |
| 3 | 方向感、土地感觉，绘制海图能力。 | 测量术 | `surveying` | 敏捷(1) + 智力(3) |
| 4 | 鉴定商品，物价情报的判断力。 | 会计术 | `accounting` | 智力(3) + 精神(4) |
| 5 | 剑的资质，白刃战的指挥能力。 | 剑术 | `fencing` | 体力(0) + 敏捷(1) |
| 6 | 炮击的资质，大炮的命中力。 | 命中力 | `gunnery` | 敏捷(1) + 运气(5) |
| 7 | 辩论的资质，外交谈判能力。 | 说服力 | `persuasion` | 魅力(2) + 智力(3) |
| 8 | 策略的资质，谍报能力。 | 谋略术 | `tactics` | 智力(3) + 运气(5) |
| 9 | 发现的资质，回避危险能力。 | 观察力 | `observation` | 精神(4) + 运气(5) |

key 与 `03-characters/03` 的 `skills.yaml` 一致（表权属 characters），是重构起的英文名，不是原版枚举成员名。文案 key `skill.<key>.name` / `skill.<key>.desc`（`00-overview/05` 文案规则）。六维 key 同 `abilities.yaml`：`stamina`(体力) `agility` `charm` `intelligence` `spirit` `luck`。

星座成长表（`0x140688970`，`g[e]` 越大该维升级时涨得多）：

| sign | 名 | 体 | 敏 | 魅 | 智 | 精 | 运 |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 0 | 摩羯座 | 0 | 2 | 1 | 2 | 0 | 1 |
| 1 | 水瓶座 | 1 | 1 | 0 | 1 | 2 | 1 |
| 2 | 双鱼座 | 1 | 1 | 1 | 2 | 1 | 0 |
| 3 | 牧羊座 | 1 | 1 | 1 | 0 | 1 | 2 |
| 4 | 金牛座 | 2 | 1 | 1 | 1 | 1 | 0 |
| 5 | 双子座 | 1 | 1 | 1 | 1 | 0 | 2 |
| 6 | 巨蟹座 | 0 | 1 | 2 | 1 | 1 | 1 |
| 7 | 狮子座 | 1 | 0 | 2 | 1 | 2 | 0 |
| 8 | 处女座 | 2 | 1 | 0 | 1 | 1 | 1 |
| 9 | 天秤座 | 2 | 0 | 1 | 0 | 1 | 2 |
| 10 | 天蝎座 | 0 | 1 | 1 | 1 | 2 | 1 |
| 11 | 射手座 | 1 | 2 | 1 | 1 | 0 | 1 |

**消息号解码** `[确认]`：`fn_1400012da(id)` / `fn_14000fb14(id)` 的 id 不是 mesfile 里按 NUL 切开的序号。exe 有两张表：`.data 0x140683620` = 37 块 `{u16 first_id, u16 block_size}`，`0x1406836c0` = 每条 `u16` 块内偏移；长度 = 下一条偏移差。按此解 SC：`0x168` 重建、`0x169` 等级上升、`0x16a` %s上升了%s、`0x16b` 解散、`0x16c` 沉没、`0x16d` 无水手、`0x16e` 得到、`0x16f` 成为伙伴、`0x170..0x173` 寿终四句、`0x174` 拒收亲笔信。导入器必须用这两张表切句。

`CHexagonSprite` 画六维雷达；`CSkillView` / `CSkillValView` 画技能。二者都是投影。

## 3. 实体与 ID

| ID | 原版计数 | 空/非法 | 备注 |
| --- | --- | --- | --- |
| `person_id` | 235（世界表） | `invalid` = -1 | 定义在 `characters`；本系统只认已注册 id |
| `persisted_full_person` | 87（id < `0x57`） | — | 存档里带六维 / 经验 / HP 的完整记录数，只是存档 schema，不是 ID 集合，不进 caps；dk4save 的 98 行是工具常数 |
| `ability_id` | 6 | `invalid` = -1 | 0–5：体力 敏捷 魅力 智力 精神 运气 |
| `skill_id` | 10 | `invalid` = -1 | 0–9 见上表 |
| `exp_track_id` | 2 | 无非法值，只有 0 和 1 | 0 = 轨 A（运行时 `+0x18`，成长体敏魅），1 = 轨 B（`+0x1C`，成长智精运）；玩法名未知 |
| `zodiac_id` | 12 | 越界 → 无成长数据 | 星座成长表行；exe `0x140688970`；表在 `characters/zodiac.yaml` |
| `physical_state_id` | 7 | `invalid` = -1 | 0 健康 1 疲劳 2 轻伤 3 重伤 4 濒死 5 疾病 6 死亡；exe 名表 `0x140686410`，取名函数 `fn_140010e6a` `< 7`；key 见 `characters/vitals.yaml` |
| `mental_state_id` | 6 | `invalid` = -1 | 0 高亢 1 精神 2 轻松 3 普通 4 不高兴 5 混乱；名表 `0x140686458`，`fn_1400056c3` `< 6`；key 同上 |
| `protagonist_id` | 剧本 0–6 | `invalid` = -1 | 与 `eScenario` 对齐的是人物表 0–6；存档 `+0x20` 存其 `person_id` |

哨兵：技能/能力查询失败返回 0 并记日志，不用 -1 当技能值（-1 是枚举非法）。

运行时循环读表长，禁止写死 `10` / `6` / `87`。原版计数只作为 vanilla 内容包实例数。配置里一律引用 key（`accounting`、`track_b`），整数只是 `vanilla_index`（D1）。

## 4. 算法 / 公式

全部常数进 `config/formulas/progression.yaml` 与 `config/caps.yaml`。代码里不得出现裸 `30`、`100`、`150`、`200`。公式编号与 `13-algorithms/01-formulas.md` 对应：F-PROG-01..04（等级）、F-PROG-05/09（技能，对照用）、F-PROG-10（成长）。

### 4.1 单轨等级

原版指令（F-PROG-01）：

```
q = exp / 30                       # 无符号 div
r = isqrt16(q)                     # q<=1 时原样返回 q；否则 n=q*0x10000（32 位回绕），牛顿迭代
level = r / 0x100                  # 无符号 div
level = clamp_u(level, 0, 100)
```

重构式（与 F-PROG-01 逐字一致）：

```
q           = floor(exp / exp_divisor)
level       = (q1_quirk and q == 1) ? 0 : isqrt(q)   # q1_quirk=true 复刻原版；false 则为数学 isqrt
track_level = min(level, level_per_track)
```

原版 `isqrt16` 是 16.16 定点开方，`r/256 = floor(sqrt(q))`；只有 `q = 1`（`exp 30–59`）例外得 **0 级**。vanilla 包必须 `quirks.q1_quirk: true`，否则开局人物等级与原版差 1。新内容包可设 false。

逆变换（写经验、工具、读档校验）：

```
exp_of_level(level) = exp_divisor * level * level
exp_of_level(1)     = 2 * exp_divisor    # 仅当 q1_quirk = true
```

原版内容包：`level_per_track=100` 时满级经验 `300000`（= 加经验的夹取上限，见 4.5）。硬顶 255 时 `1950750`。

**溢出语义（重构必须与原版分清）：**

| 实现 | 单轨 `q >= 65536`（`exp >= 1966080`） |
| --- | --- |
| 原版 exe | `q << 16` 在 32 位回绕，等级 = `isqrt(q mod 65536)`；`q` 为 65536 整倍数时除零崩溃；vanilla 被 300000 夹住，碰不到 |
| 重构引擎 | 用 64 位或直接整数开方，**饱和**到 `min(level_per_track, engine_level_hard)`，禁止回绕 |

`engine_level_hard` 即 `engine_limits.yaml` 的 `level_per_track`（默认 255，工具/补丁历史硬顶）。若模组把 `caps.level_per_track` 调过硬顶，启动校验失败，不静默截断。

### 4.2 总等级

```
total_level = track_level(track_0) + track_level(track_1)
```

主角 `save+0x70` 是这份值的缓存。重构：**经验是源**；缓存可有，读档必须能从两条经验重建。UI 与技能公式都读重建后的 `total_level`，不信缓存。

### 4.3 技能值（D2 修正器键；本系统只供 `total_level`）

原版是两层函数（`fn_140009214` → `fn_1400135cf`，见第 2 节）。重构把它摊成修正器键 `skill.<key>` 的固定段序，每段一个来源，取整与原版逐位一致：

| 段 | 来源（提供方） | 原版 | 式（vanilla 参数） |
| --- | --- | --- | --- |
| `base` | characters `dk4.skill.v1`（`03-characters/03` §4） | `fn_140009214(p,k,1)` | `L = min(total_level, skill_level_cap)`；`base = clamp((S(a)+S(b)) * (L + skill_level_bias) / skill_divisor, 0, skill_value_cap)`；`S` = 精神态修正后的六维（`vitals.yaml` `mental_states[*].ability_mult`） |
| `add` | items `items.equipment`（`09-items/02` 4.2） | `fn_14000e0b6` 5 槽 | `v += amount`（每件一条带来源的贡献） |
| `mul` | characters `vitals`（`03-characters/07`） | `fn_1401c87a0` | `v = v * num / den`，有符号截断（疲劳/轻伤 19/20、重伤 1/2、濒死 1/10、疾病 3/5） |
| `clamp` | 键定义 | `fn_1401cbb40` | `clamp(v, 0, skill_value_cap)` |

vanilla 参数：`skill_level_cap 200`、`skill_level_bias 100`、`skill_divisor 150`、`skill_value_cap 500`；`total_level` = 4.2（两段之和）。与 `03-characters/03` §4、`13-algorithms/01` F-PROG-05/09 同式。

本系统对技能只做两件事：

1. 提供 `Progression.total_level(person)` 给 `base` 段。
2. 等级变化时发 `LevelChanged`，注册表据此失效该人物的 `skill.*` 缓存。

HP 上限 = `skill.vitality` 的 `base` 段、精神修正关（`fn_140009214(p,0,0)`），归 characters。

`skill_value_cap = 500` 就是 `exe_caps.md` 里标成「属性结算」的那处立即数；原版「技能封顶 200」是等级项 `L` 的上限。两者都要进配置。

### 4.4 属性成长（F-PROG-10）

```
on track level up (每次滚动):
  for e in growth_abilities(track):               # track_a → [stamina, agility, charm]，track_b → [intelligence, spirit, luck]
    g  = zodiac.growth[e]                          # characters/zodiac.yaml；person.zodiac
    up = rand(growth_rand_a) + rand(g + growth_rand_bias) / 2    # 2 / 3；有符号 /2
    if up > 0:
      ability[e] = clamp(ability[e] + up, 0, ability_cap)        # 100
  emit AbilityGrown(...)                           # characters 收到后：HP 上限变大则把差值加到当前 HP
```

`rand(n)` = `0..n-1`，`n <= 0` 返回 0；随机流 `characters`（`engine/rng.yaml` 命名流，种子进存档；旧写 `progression.growth`）。单次涨幅：`g=0/1` 为 0–2，`g=2` 为 0–3。

存档六维是 `uint8`；`ability_cap` 若调过 255 须同时加宽存档。原版没有「六维 + 装备 → 结算六维」这一步，也没有「结算六维上限」；那处 500 是技能值上限 `skill_value_cap`（4.3）。

`exe_caps.md` 里 `Ability(captain, 4)`、`100 - a/50` 的 `a` 实为**结算技能 4 会计术**（`fn_14000e1ce` 第二参数过 `fn_1400099e9` 的 `< 0xa` 检查）。各系统按键 `skill.accounting` 求值，不读六维。

### 4.5 加经验（F-PROG-02）

```
Progression.grant_exp(person, add[track], roll_mode, source)    # show 由路由行给
  growth_list = []
  for track in tracks:
    old = track_level(track)
    exp[track] = clamp(exp[track] + add[track], 0, exp_track_cap)    # 300000；原版无符号
    if track_level(track) > old: growth_list += track.growth_abilities
  roll_growth(growth_list) 一次                                       # roll_mode = once_per_call（fn_140011c34）
  # roll_mode = per_level：按升级数逐级滚（fn_140008387 / fn_140006c67，单轨）
```

`exp_track_cap` 原版是独立立即数 `300000`，恰等于 `exp_of_level(100)`；重构按 `exp_of_level(level_per_track)` 派生，内容包可覆盖。原版「经验写入四处相同立即数 `300000`」= `fn_140011c34` 两处 + `fn_140008387` + `fn_140006c67`。重构只保留一处夹取。

**来源一律走 `grant_routes`**（`formulas/progression.yaml`）：事件或命令到达本系统 → 查路由表 → 选人、选轨、定量 → `Progression.grant_exp`。玩法系统不调成长函数。增量只有一处真相：默认写在路由行；提供方自己拥有数量表时（vitals 的 `surgeon_exp` / `captain_exp` / `zero_exp`），路由写 `amount: from_event`，禁止两处都写。原版约 90 处调用点，下表是已钉住语义的（`[确认]` 指增量立即数与调用点；玩法语义注明出处）：

| 路由事件（提供方） | 受益人 | 轨 A | 轨 B | 原版 |
| --- | --- | ---: | ---: | --- |
| `TradeCommitted`（economy，`04-economy/03` §2.6） | 主角 `[推断]`（`fn_140004e17`） | 0 | 10 | `fn_140006e88(·, 0xA, 1)` |
| 同上，`bargain: ok` | 会计长 | 0 | 10 | 同上 |
| 同上，`bargain: fail` | 会计长 | 0 | 5 | 同上 |
| `NpcTradeCommitted`（economy，`04-economy/06`） | 提督（`admiral_id`，空则不发） | 0 | 10 | `fn_140006e88(提督, 0xA, 1)` |
| `ItemBargained`（items，`09-items/02` 4.1） | 还价人（会计长） | 0 | 10 | `0x1401565c5` `fn_140006e88(p,10,1)` |
| `SurgeonWorked`（characters，`03-characters/07` 2.11） | 船医 | 10 / 100 / 250 / 50 | 10 / 50 / 250 / 100 | `fn_1400f7c10`（轻伤 / 重伤 / 濒死 / 疾病治愈）；量在 `vitals.yaml` `surgeon_exp`，事件带出 |
| 混乱恢复（characters，`07` 2.11 第 5 步） | 舰长 | 10 | 10 | `fn_1400ebfd0` `0x1400ec0de`；量在 `vitals.yaml` `captain_exp` |
| HP 由正变 ≤ 0（characters，`07` 2.3a） | 本人 | 400 | 0 | `fn_1400141ff` → `fn_1400026d5(p,0x190,0)`；量在 `vitals.yaml` `zero_exp` |
| `ProofDiscovered`（items，`09-items/04` 2.5） | 全体玩家势力人物 | 200 | 200 | `fn_14018b880(0xc8,0xc8)` `0x1401b4f3a`，不提示 |
| `ItemFound` 普通发现（items / ocean） | 全体玩家势力人物 | 10 | 5 | `fn_14018b880(0xa,5)` `0x1401b4f93`，不提示 |
| 撕毁协议（town-life，`08-town-life`） | 主角 `[推断]` | 0 | 50 | `fn_140006e88(…,0x32)` |
| `BattleResolved`（combat） | `[推断]` 白刃 / 战斗结算 | 15 | 0 | `fn_14000660e` `0x1401bc53b` |
| `FleetDestroyed`（fleets，非玩家本队分支） | 相关人物 | 100 | 500 / 600 | `fn_14000a9ed` `0x1400ef172` `0x1400ef22a` |
| 在岗经验（phase `duty_exp`：day_start、`every_n_days: 3`、`when: at_sea`；`n` = 出港天数） | 玩家舰队在舱人；船 2–5 中隶属玩家势力的船长 | 岗位表 `n*a/d − s`；无岗位 `rand(n)/2`；船长 `2n−1` | 同左（B 列）；船长 `4n−2` | `CFleetElement::vfunc_04` `0x1400ffa34..0x1400ffd68`，门 `fn_140009b24`（`packed % 0x90`）；表 `.data 0x140682bc0`（`03-characters/06` §2.1） |
| `CPersonElement::vfunc_04`（日首 `persons` 阶段，`01-world/07`） | 状态 `0x1a` 且 `fn_140013133` ∈ 0..2 的人 | `rand(2)+3` | `rand(4)+2` | body `0x1401cc4c0` |
| — | — | 10 / 300 | 10 / 500 | `fn_140011414` `0x1400eca1b`，语义 `[未知]` |

两个原版广播助手 `[确认]`：`fn_14018b880(a,b)` 遍历 235 人、属玩家势力者 `fn_140011c34(p,a,b,0)`；`fn_14018ba70(a,b)` 遍历 235 人、`fn_14000ee26` 为真（玩家势力且不是别支舰队提督 `[推断]`）者 `fn_140011c34(p,a,b,1)`。重构用路由的 `recipients: player_force | main_fleet_members` 表达，不复制循环。

`daily` 阶段（`engine/tick.yaml`，day_start）内产生的经验（船医、舰长、HP 归零）排在该阶段各步之后统一结算（海上分支：补给 → 疲劳 → 人物 HP/身心 → 掷骰 → 经验；在港分支只有疲劳恢复，不产生经验；D4、词表 §14-18）。原版在同一循环里即时加，后一个人的掷骰可能用到前一个人刚升级的技能；需要逐位回放时设 `grant_timing: immediate`。

`source` 只用于日志与成就，不进公式。禁止按 `source` 在代码里 `if combat`。

## 5. 耦合分析

原版缠在一起的点：

- 技能公式直接吃六维原值 + 总等级 + 精神/身体态 + 装备，贸易/战斗再吃结算技能（`exe_caps.md` 的 `Ability(captain, 4)` 实为技能 4 会计术）
- 约 90 处玩法代码直接调 `fn_140011c34` 系列并写死增量
- 主角经验写两份（`CPerson` 记录 + `0x35AA` 的 `CPlayerPerson` 记录）再加 `+0x70` 缓存，状态画面与名单会不一致
- `NUM_SKILL=10`、`NUM_ABILITY=6`、完整存档人物 87（`cmp 0x57`）写死在循环和断言
- 等级上限、技能等级项上限、技能值上限、经验上限散落在 exe 立即数，改一处要搜字节
- 身体态 / 精神态改技能的系数写死在 `fn_1401c87a0` / `fn_14000e098` 的 switch 里
- 升级成长按星座表 + 轨→维写死（`0x1406863f0` / `0x140686400`）
- 开局选技能的文案在 `mesfile`，逻辑在 `CGoodSkillMenuData`，说明字符串在 exe `.rdata` 而不是 mesfile

重构切断方式：业务系统只按修正器键 `skill.<key>` 求值；本系统只输出 `total_level` 与事件；经验模块不引用 `World*`、不引用港口/舰队指针，经验来源只经 `grant_routes`。

## 6. 解耦后的配置

```yaml
# config/caps.yaml  （片段，vanilla 默认；键名与 03-characters/03、13-algorithms/02 统一）
schema: dk4.caps
version: 1
level_per_track: 100
skill_level_cap: 200          # 技能 base 段等级项 L 的上限（旧名 skill_cap）
skill_value_cap: 500          # 技能值夹取上限（exe_caps「属性结算」那处；旧名 ability_settled_cap 作废）
ability_cap: 100              # 六维上限（旧名 ability_growth_cap）
exp_track_cap: null           # null = exp_of_level(level_per_track)；vanilla 等于 300000
```

`stat_count` / `skill_count` / `exp_track_count` 不进 caps：由 `abilities.yaml` / `skills.yaml` / `level.yaml` 的表长决定。

```yaml
# config/engine_limits.yaml  （片段）
schema: dk4.engine_limits
version: 1
level_per_track: 255     # 超过则启动失败，不回绕
ability_stored_hard: 255 # 原版 uint8；加宽存档时可升
exp_hard: 4294967295     # uint32
skill_value_hard: 65535
```

```yaml
# config/formulas/progression.yaml       # 形状 = 14-config-schemas README「公式层」（D3）
schema: dk4.formulas.progression
version: 2
level_from_exp:               # F-PROG-01
  impl: expr
  int_mode: { div: trunc, width: sat64 }   # 禁止 wrap / 除零
  quirks:
    q1_quirk: true            # 原版 q=1（exp 30–59）→ 0 级；false = 数学 isqrt（旧名 quirk_min_q: 2 / 1）
  params: { exp_divisor: 30 }
  inputs: [exp]
  expr: "q = exp / exp_divisor; (q1_quirk && q == 1) ? 0 : min(isqrt(q), $caps.level_per_track)"
  golden:
    - { exp: 59,     out: 0 }
    - { exp: 60,     out: 1 }
    - { exp: 300000, out: 100 }
exp_from_level:               # F-PROG-02
  impl: expr
  params: { exp_divisor: 30 }
  inputs: [level]
  expr: "exp_divisor * level * level"
total_level:                  # F-PROG-04
  impl: vanilla.total_level   # 按 $caps.track_count 求和
stat_growth:                  # F-PROG-10
  impl: vanilla.stat_growth
  params:
    growth_rand_a: 2          # rand(2)
    growth_rand_bias: 3       # rand(g + 3) / 2
    roll_mode: once_per_call  # once_per_call | per_level
  rng_stream: characters      # engine/rng.yaml 的命名流（05 只登记 characters，无 progression 流）
  # 每轨成长哪三维 = progression/level.yaml tracks[].growth_abilities；g = characters/zodiac.yaml signs[].growth；结果夹 $caps.ability_cap
exp_grant:                    # 经验授予路由：事件 → 受益人 → 增量；轨用 key
  impl: vanilla.exp_grant
  params:
    grant_timing: daily_phase # daily_phase（D4）| immediate（逐位回放）
  tables:
    grant_routes:
      - { on: TradeCommitted, who: protagonist,     add: { vanilla:track_b: 10 } }
      - { on: TradeCommitted, who: vanilla:purser, when: { bargain: ok },   add: { vanilla:track_b: 10 } }
      - { on: TradeCommitted, who: vanilla:purser, when: { bargain: fail }, add: { vanilla:track_b: 5 } }
      - { on: NpcTradeCommitted, who: event.admiral_id, add: { vanilla:track_b: 10 } }
      - { on: ItemBargained,  who: event.person,    add: { vanilla:track_b: 10 } }
      - { on: SurgeonWorked,  who: event.person,    amount: from_event }    # formulas/vitals.yaml / vitals.yaml surgeon_exp
      - { on: ProofDiscovered, who: player_force,   add: { vanilla:track_a: 200, vanilla:track_b: 200 }, show: false }
      - { on: ItemFound,      who: player_force,    add: { vanilla:track_a: 10, vanilla:track_b: 5 },    show: false }
duty_exp:                     # phase duty_exp（engine/tick.yaml：day_start、every_n_days 3、when: at_sea）；03-characters/06 §2.1
  impl: vanilla.duty_exp
  schedule: day_start         # every_n_days / when 由 engine/tick.yaml 的 phase 定
  params:
    n: days_since_departure   # (packed − fleetdata+0x2C) / 48，n <= 0 不发
  tables:
    post_table:                 # 原版 .data 0x140682bc0；每轨 [a, d, s]，e = a > 0 ? n*a/d − s : 0
      vanilla:captain:        { vanilla:track_a: [1, 1, 0], vanilla:track_b: [4, 1, 1] }
      vanilla:adjutant:       { vanilla:track_a: [1, 1, 1], vanilla:track_b: [3, 1, 2] }
      vanilla:staff:          { vanilla:track_a: [0, 1, 0], vanilla:track_b: [7, 2, 0] }
      vanilla:purser:         { vanilla:track_a: [0, 1, 0], vanilla:track_b: [3, 1, 1] }
      vanilla:surveyor:       { vanilla:track_a: [0, 1, 0], vanilla:track_b: [5, 1, 4] }
      vanilla:sail_master:    { vanilla:track_a: [4, 1, 3], vanilla:track_b: [1, 1, 1] }
      vanilla:helmsman:       { vanilla:track_a: [3, 1, 2], vanilla:track_b: [2, 1, 1] }
      vanilla:lookout:        { vanilla:track_a: [2, 1, 0], vanilla:track_b: [2, 1, 0] }
      vanilla:boarding_chief: { vanilla:track_a: [2, 1, 1], vanilla:track_b: [1, 1, 0] }
      vanilla:gunner:         { vanilla:track_a: [3, 2, 0], vanilla:track_b: [3, 2, 1] }
      vanilla:missionary:     { vanilla:track_a: [1, 1, 1], vanilla:track_b: [3, 1, 1] }
      vanilla:surgeon:        { vanilla:track_a: [1, 1, 1], vanilla:track_b: [3, 1, 1] }
      vanilla:cook:           { vanilla:track_a: [3, 1, 2], vanilla:track_b: [2, 1, 2] }
      vanilla:carpenter:      { vanilla:track_a: [2, 1, 2], vanilla:track_b: [1, 1, 1] }
      vanilla:keeper:         { vanilla:track_a: [4, 1, 3], vanilla:track_b: [1, 1, 1] }
      vanilla:deck_chief:     { vanilla:track_a: [7, 1, 3], vanilla:track_b: [0, 1, 0] }
    no_post: { vanilla:track_a: rand_n_half, vanilla:track_b: rand_n_half }    # rand(n)/2，有符号
    other_ship_captain: { vanilla:track_a: "2n-1", vanilla:track_b: "4n-2" }   # 船 2–5，船长隶属玩家势力
# fn_14000660e（+15 轨 A）等受益人 / 事件未钉的调用点不进 vanilla 包，不要臆造事件名
```

技能公式参数（`level_bias` / `skill_divisor` / 两个 cap 的引用）与精神、身体系数**不在本文件**：前者在 `formulas/character.yaml`（`03-characters/03` §6），后者在 `characters/vitals.yaml`（`03-characters/07` §6）。

```yaml
# config/progression/level.yaml
schema: dk4.progression.level
version: 1
tracks:
  - key: vanilla:track_a
    vanilla_index: 0          # 运行时 +0x18
    name_key: progression.track_a    # 玩法名 [未知]
    growth_abilities: [vanilla:stamina, vanilla:agility, vanilla:charm]       # 原版 0x1406863f0
  - key: vanilla:track_b
    vanilla_index: 1          # 运行时 +0x1C
    name_key: progression.track_b
    growth_abilities: [vanilla:intelligence, vanilla:spirit, vanilla:luck]    # 原版 0x140686400
starting_good_skills:
  pick: 3                     # mesfile「请选择３种熟练的技能」
  pool: all                   # 从 skills.yaml 选
```

```yaml
# config/characters/zodiac.yaml  （表权属 characters；本系统只读 growth）
schema: dk4.characters.zodiac
version: 1
signs:                        # growth 按六维 key 稀疏映射，缺省 0（D1；不按下标排 6 列）
  - { key: vanilla:capricorn, vanilla_index: 0, name_key: zodiac.capricorn,
      growth: { vanilla:stamina: 0, vanilla:agility: 2, vanilla:charm: 1, vanilla:intelligence: 2, vanilla:spirit: 0, vanilla:luck: 1 } }
  # … 1–11 见第 2 节表
```

World0 / World1 共用同一套公式；人物初始六维、初始经验、星座在内容包，不在本文件。

## 7. 独立旋钮

改这些值不应迫使贸易、战斗、UI 改代码：

| 旋钮 | 文件 | 影响 |
| --- | --- | --- |
| `exp_divisor` | `formulas/progression.yaml` | 升级曲线 |
| `level_per_track` | `caps.yaml` | 满级经验、总等级上限 |
| `skill_level_cap` / `skill_value_cap` | `caps.yaml`（公式在 `formulas/character.yaml`） | 技能数值尺度 |
| `ability_cap` | `caps.yaml` | 六维成长尺度 |
| 精神 / 身体系数 | `characters/vitals.yaml` | 状态对技能的影响 |
| `growth.*` / `tracks[].growth_abilities` / 星座表 | 公式 + 内容 | 升级成长速度与方向 |
| `quirks.q1_quirk` | 公式 | 原版等级取整怪癖 |
| `overflow: saturate` | 公式 | 高经验存档行为 |
| `starting_good_skills.pick` | `level.yaml` | 开局选几个 |
| `grant_routes` / `grant_timing` | 公式 | 哪类事件给谁哪条轨加多少、何时结算 |

显示层自己处理「等级数字几位」，不要用 UI 宽度倒逼 `level_per_track`。

## 8. 对外接口

只传 ID 与值。

**查询**

```
Progression.track_exp(person_id, track_id) -> u64
Progression.track_level(person_id, track_id) -> u32
Progression.total_level(person_id) -> u32        # 技能 base 段的输入
Progression.good_skills(person_id) -> [skill_id] # 开局 3 个，可空
```

技能值不在这里查：`Modifiers.eval("skill.<key>", {person})`（或 characters 的便捷包装 `Characters.skill`）。六维基础值查 characters。

取不到人：整体 no-op / 返回 0，不抛进 UI。

**命令**

```
Progression.grant_exp(person_id, add{track_key: n}, roll_mode, source)   # 仅 grant_routes / 调试 / 叙事命令；show 由路由行给
SetGoodSkills { person_id, skill_ids[pick] }      # 仅开局/编辑器
RebuildDerived { person_id }                      # 读档、改 caps 后
```

**订阅**：`grant_routes` 里列出的事件（`TradeCommitted` `NpcTradeCommitted` `ItemBargained` `SurgeonWorked` `ProofDiscovered` `ItemFound` …）；`engine/tick.yaml` 的 `duty_exp` 阶段调本系统 `progression.duty_exp`。

**事件（本系统发出）**

```
LevelChanged { person_id, track_id, old, new, total }   # 注册表据此失效 skill.* 缓存
AbilityGrown { person_id, ability_id, delta }     # 升级成长滚动结果；characters 调 HP，UI 出「%s上升了%s」
```

禁止：UI 写经验；经济 / 战斗直接调 `Progression.grant_exp` 绕过路由；战斗直接改 `save+0x70`。

## 9. 未核实字段

- 两轨的玩法名（航海 / 战斗 / 其它）；约 90 处调用点里多数的玩法场景（4.5 只列了已钉的）
- `CPersonElement::vfunc_04` 里状态 `0x1a` 与 `fn_140013133` 0..2 的含义
- `fn_14000660e`（+15 轨 A）、`fn_140011414`（300/500）、`fn_14000a9ed`（100/500、100/600）的受益人与玩法语义
- `fn_14000ee26` 是否就是「不是别支舰队提督」（`fn_14018ba70` 的受益人筛选）
- 开局 3 个熟练技能的数值效果
- 存档记录 `+0x01`（对象 `+0x22`，只增不减的一字节）的语义；记录其余字节已还原（`03-characters/01` §2）
- `+0x70` 缓存是否还被除状态画面外的系统信任
- `CPlayerPerson::vfunc_05`（`0x1402ad690`，RTC `eAbility`）反编译失败（与技能公式无关：技能路径已从指令还原）
- `LEVEL_MIN` / `LEVEL_MAX` 的整数（断言只证明挂在 `UCHAR`）
- World 人物行 `stride 0x38` 里除姓名指针和 `+0x20` 六维外，是否还有初始经验 / 熟练技能位
