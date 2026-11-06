# 人物图片、动画与显示

人物系统的表现层。六维/等级已经在 `01-person-model.md`、`03-abilities-skills-jobs.md`、`11-progression` 写完；本文件只定 **一张脸、一具行走图、一块半身像怎么从 `person_id` 找到资源，以及状态画面怎么把六维画出来**。

禁止用人物中文名去猜 `CharaNN.DK4`。sheet 只来自 exe 里的 `person_id` 映射函数；bustup 只来自世界表字节。对不上的 ID 空间分开记。

## 1. 职责边界

本系统（人物·表现）管：

- `person_id` → 行走图逻辑 sheet → `Chara%02d.DK4`
- `person_id` → 半身像 `bustup.dk4` 下标
- 广场 / 甲板 / 状态卡上的 `CCharacter` 动画状态
- 状态卡六维雷达（`CHexagonSprite`）与人物信息窗的布局契约
- 把上述投影成 `config/characters/presentation.yaml`（定义，不是存档）

不管：

- 姓名、六维、经验、名册（`01-person-model.md`）
- 技能公式与岗位效果（`03-abilities-skills-jobs.md`）
- 剧本开局把谁写入名册（`02-protagonists-scenarios.md`）
- 港口流行商品的 `CFashionSprite`（`02-factions/02-countries-culture-fashion.md`；那是流行品图，不是人物换装）
- 事件脚本何时切背景、何时出 still（`10-narrative` 发 `still_id`；本系统不解析 EV opcode）
- KTGL / D3D11 / G1T 解码实现（`12-ui-save` 的场景层只拿资源 id）
- 文案（`ui/strings.*`）

UI 向人物系统要的是 **ID + 资源键**，不拿 `CPerson*`，不拿 `World*`。

## 2. 原版事实

### 资源容器

- `[确认]` `Game/Chara00.DK4` … `Game/Chara41.DK4` 共 **42** 个文件，头 `GT1G4600`（Koei Tecmo G1T，platform=10）。来源：目录列举、`srcdata/docs/formats.md`、文件头。
- `[确认]` 加载格式串是 `"Chara%02d.DK4"`。来源：`fn_140003a8a`（`fn_14006f9e0.cpp` / `0x1400749a0`）、`fn_14000df26`。
- `[确认]` `bustup.dk4`：220 张，全部 **320×416**，格式字节 `0x59`（DXT1：320×416/2=66560，加 28 字节头 = 每张 66588）。来源：G1T 头 `ntex=220`、逐张 span。
- `[确认]` `Portrait.dk4`：84 张，全部 **576×800**，格式字节 `0x5b`（DXT5：576×800+28=460828）。来源：同上。
- `[确认]` `Plaza.dk4`：28 张，混合尺寸（解析 extra=0x0C 的宽高不可靠；span 显示 7 张约 3.7MB、6 张 64KB 级、以及 60×60 / 160×96 小图）。广场代码按固定下标取图：`0x14`–`0x1b`、`6`、`7`。来源：`fn_14016cc40.cpp`。
- `[确认]` `EvItem.dk4`：39 张，含 512×640×11、1280×960×6 以及一批小物件。和 `InfoBox2.dk4` 一起出现在事件/小游戏 RTC `asCharacterData`。来源：G1T；`fn_14002b6f0.cpp` 等。
- `[确认]` 事件静帧背景：`EventBG1.dk4`–`EventBG8.dk4` 各 50 张；`EventBGEX.dk4` 16 张；`EvFrame.dk4` 1 张。来源：G1T 头。`CEvent` 继承 `CEventStillManager`。来源：`class_hierarchy.txt`、`CEventStill.cpp`。

42 个 Chara 包的贴图计数（G1T `ntex`，合计 **883**）：

| 文件 | ntex | 字节 | 备注 |
| --- | --- | --- | --- |
| Chara00–26, 28–31, 35–37 | 20–22 | ~1.3–1.9MB | 主体：宽 112 的竖条 |
| Chara27, 32, 34, 38 | 20 | 0.32–0.50MB | 大量 span=84 的空槽 |
| Chara33 | 21 | ~1.7MB | 8 个空槽 |
| Chara39 | **60** | 16.9MB | 逻辑 sheet 39–48 共用 |
| Chara40 | **6** | 0.43MB | 逻辑 sheet 49–50 共用 |
| Chara41 | **13** | 0.62MB | 逻辑 sheet 51–54 共用 |

- `[确认]` Chara 贴图几乎都是宽 **112**、高为 96 的倍数（96 / 288 / 384 / 480 / 672 / 768 / 1056 / 1344 / 1536…），格式 `0x5b`（DXT5）。来源：逐张 G1T 头。
- `[推断]` 单帧 **112×96**。依据：每个包第 0 张都是 112×96；其余高度均可被 96 整除。竖条 = 同动作的连续帧。未在 cpp 里读到「96」立即数，不升为确认。
- `[确认]` 部分槽 span=84、宽高字段写成 1280×0：空纹理占位，不是可画的行走图。来源：Chara27/32/34/38/39/41。

### 运行时对象

- `[确认]` `CCharacter : CAnimeSprite : CModelFlatSprite : … : CSprite`。体积 0x98。来源：`class_hierarchy.txt`、`CCharacter.cpp` dtor `0x98`。
- `[确认]` `CCharacter` 构造：`+0x80 = 0x38`（56）、`+0x84 = 0x1f`（31）、`+0x88 = 0`、`+0x8c = 0`。来源：`CCharacter.cpp`。
- `[确认]` `+0x80` 是**逻辑 sheet id**（0–54 合法，`0x38` 空闲哨兵）。写入：`fn_1400123ff`（`0x14006f290`）`if (id < 0 \|\| id >= 0x37) skip`。读取后经 `fn_14006f980` 变成文件号再 `sprintf Chara%02d.DK4`。来源：反汇编。
- `[确认]` `CCharacterManager` 体积 0x2a0：55（`0x37`）个 `CCharacter*` 在 `+0x8`，55 个 sheet 缓存于 `+0x1c0` 初值 `0x38`，`+0x29c` 为当前槽数（上限仍 0x37）。来源：`CCharacterManager.cpp`、`fn_1400059a2`。
- `[确认]` `CDeckCharaAnime` 内嵌 `CCharacterManager` 于 `+0x10`（体积 0x2b0）。来源：`CDeckCharaAnime.cpp`。
- `[确认]` `CSelectCharaSprite` 内嵌 `CCharacterManager` 于 `+0x168`，另有 0x1b=27 个指针槽。来源：`CSelectCharaSprite.cpp`。`CSelectPersonView` 同时继承 `CPersonInformationView` 与 `CSelectCharaSprite`。来源：`class_hierarchy.txt`。
- `[确认]` `CAnimeSprite::vfunc_04` 推进帧：`+0x5c` 计时、`+0x6c` 每帧 tick、`+0x60` 当前帧、`+0x64` 帧数、`+0x70` 结束帧、`+0x58` 状态（1=停、2=循环回 0）。`+0x78` 指向帧表。来源：`combat/CAnimeSprite.cpp`。
- `[确认]` `fn_14000c59a` 把动画复位：`+0x58..+0x64=0`、`+0x68=+0x6c=1`、`+0x70=0xffff`。来源：`fn_14006f9e0.cpp`。

### `person_id` → 逻辑 sheet（不是世界表）

- `[确认]` `fn_1400083fa`（`0x14006fca0`，488 字节）把一个整数（调用点按 `ePerson` 用）映射成逻辑 sheet。来源：反汇编；甲板 `CDeckPersonnelView`、人物信息窗 `fn_1401cb7f0.cpp`、广场 `fn_14016cc40.cpp` 均把返回值送进 `fn_1400123ff`。
- `[确认]` 规则（`ecx = person_id`）：

  1. `id < 39`（0–38）：返回 `id`。唯一例外：`id == 5` 时对全局对象 `DAT_144296500+0x7c` 做 `fn_14000864d`（`< 4` 则返回 0，否则 1）。返回 0 时 sheet=**40**（`0x28`），否则 sheet=5。
  2. `39 ≤ id ≤ 214`：`(id-39)` 查 176 字节 jump table（`0x14006fdd8`）。10 个具名出口，其余走 default。
  3. jump table 具名出口（逻辑 sheet，不是文件号）：

     | person_id | 逻辑 sheet |
     | --- | --- |
     | 39 | 39 |
     | 42 | 41 |
     | 44 | 42 |
     | 45 | 43 |
     | 52 | 44 |
     | 54 | 45 |
     | 72 | 46 |
     | 89 | 53 |
     | 178 | 51 |
     | 214 | 52 |

  4. 上表以外、且 `39 ≤ id ≤ 214` 的 default：
     - `id < 98` → sheet **48**
     - `id == 192` 或 `199 ≤ id ≤ 201` → sheet **48**
     - 其余 → sheet **56**（`0x38`，与 `CCharacter` 空闲哨兵相同）
  5. `id ≥ 215`（首都女性 215–234）不进 jump table，落到 `0x38`。

- `[确认]` 逻辑 sheet → 文件号 `fn_14006f980`（`0x14006f980`）：

  | 逻辑 sheet | 文件 |
  | --- | --- |
  | 0–38 | `Chara{id:02d}.DK4`（1:1） |
  | 39–48 | `Chara39.DK4` |
  | 49–50 | `Chara40.DK4` |
  | 51–54 | `Chara41.DK4` |
  | ≥55（含 56） | 返回 0 → `Chara00.DK4`（函数 `xor eax,eax`） |

- `[确认]` 因此 **235 人共用 42 个行走图包**：0–38 一人一文件；其余有身体的人挤进 Chara39–41；大量职能 NPC / 首都女性逻辑 sheet=56，**没有自己的行走图槽**。半身像是另一套 ID（见下），不能用行走图包数去反推 bustup。
- `[确认]` 人物信息窗若 `sC8 == 0x42` 会把 sheet **强制改成 53**（`0x35`）再 `fn_1400123ff`。来源：`fn_1401cb7f0.cpp`。`0x42` 语义 `[未知]`。
- `[确认]` 若干调用点直接写死 sheet `0x35` / `0x36`（→ 文件 Chara41）。来源：`fn_140163ac0.cpp`、`fn_140095cd0.cpp`、`fn_1401eb2a0.cpp`。

### 半身像 bustup

- `[确认]` 世界行 `SPerson+0x2B`（1 字节）被 `fn_140014709` 拷到 `CPerson+0x28`。来源：`0x1402ad3c4` `movzx ecx, BYTE [sPerson+0x2b]`。
- `[确认]` 对话框加载：`fn_140006d61(byte)` 若 `byte < 0 \|\| byte >= 0xD3`（211）则返回 **0xAA=170**，否则原样；随后 `fn_140010e6f(..., "bustup.dk4", index, &rc)`。来源：`0x1400d6150`；`fn_1400136b0`、`fn_140010384`、`fn_140004cf0`（均与 `mainbox.dk4` 同屏）。
- `[确认]` World0 的 `+0x2B`：235 人里 **197 个不同值**，范围 0–212；0–97 名册 **98 人 98 个不同值**。来源：`Docs/_data/persons_world0.json`。
- `[确认]` World0 中 `+0x2B ≥ 211` 的 4 人（id 202、208、209、210，值 212）会被 clamp 成 170。文件却有 220 张（0–219），211–219 不被这条路径使用。
- `[确认]` World1 相对 World0：`+0x2B` 有 12 处不同、`+0x2A` 有 4 处不同、性别字节无差。来源：`World1_person.c` vs `persons_world0.json`。半身像属于**内容包**，不是 exe 常数。
- `[确认]` 符号 `NUM_BUSTUP` / `INVALID_BUSTUP` / `BUSTUP_ID` 存在，整数未写进 `BustupID.h`。来源：`constants.json`、`asserts.txt`。
- `[推断]` 代码路径上的 bustup 合法区间是 `[0, 211)`（`fn_140006d61`）。文件 ntex=220。`NUM_BUSTUP` 取 211 还是 220 未钉死。

### 大肖像 Portrait 与事件 still

- `[确认]` `fn_14011ea50(index)` 从 `Portrait.dk4` 取第 `index` 张。来源：`0x14011ea50`。
- `[确认]` `CShowPortrait` 体积 0x268，构造写大量 HD 布局常数，`+0x15c` 初值 `0xffffffff`。合法时用 `+0x15c` 当 Portrait 下标；越界则下标保持 -1 再 load。来源：`CShowPortrait.cpp`、`fn_140115330.cpp`。
- `[确认]` 选剧本：RTC `aeScenarioPortrait` 168 字节 = **42 个 int**（与 42 个 Chara 包同数），`0x47` 当跳过哨兵。来源：`fn_140008c51`。循环只扫 6 格再额外调一次。
- `[确认]` `CEventStill` 体积 0x30；`CEventStillManager` 体积 0x18。事件背景文件名在 `fn_140004106` 里写成 `EventBGEX.dk4`（多槽同名）。来源：`CEventStill.cpp`、`fn_1400c8390.cpp`。
- **未找到** `person_id → Portrait 下标` 或 `person_id → EventBG 下标` 的表。84 张大肖像、8×50 张事件背景是**独立 ID 空间**。

### 性别、脸位、世界表邻字节

- `[确认]` `fn_140014709` 把世界行拷进 `CPerson`：

  | 世界偏移 | CPerson | 观察 |
  | --- | --- | --- |
  | +0x26 | +0x08 | 0/1；World0 182 男 / 53 女 |
  | +0x27 | +0x09 | 0–20（21 档） |
  | +0x28 | +0x0A | 0–8 |
  | +0x29 | +0x0B | 若值==13，改成 `person_id % 12` |
  | +0x2A | +0x0C | 0–21 与 26；186/235 人为 26；**会进存档**（`CPerson::vfunc_06/07` 第一个字节） |
  | +0x2B | +0x28 | bustup，见上 |
  | +0x2C | +0x29 | 0–6（1 占 193/235） |
  | +0x30 | +0x2C（dword） | 世界 +0x30 起 4 字节 |

  来源：`0x1402ad280` 反汇编、`CPerson.cpp` 串行化、`01-person-model.md`。
- `[确认]` `eSEX` / `NUM_SEX` / `INVALID_SEX` 只有符号。来源：`SexID.h`。
- `[确认]` `eFACELOCATION` / `NUM_FACELOCATION` / `INVALID_FACELOCATION` 只有符号与 World DLL 断言。来源：`FacelocationID.h`、`asserts.txt`。
- `[确认]` `CFaceView : CPushButton`，方法 5，构造几乎只写 vftable。来源：`CFaceView.cpp`。场景文档把它标成「肖像」控件。来源：`12-ui-save/01-scenes-flow.md`。
- `[推断]` `NUM_SEX = 2`。依据：世界字节只有 0/1，exe 旁有「女」「男」字串（`01-person-model.md`）。
- **`+0x2A` 不是行走图 sheet。** 0–38 的 sheet 等于 `person_id`，但他们的 `+0x2A` 并不等于 `person_id`（赫德拉姆 sheet=1 而 `+0x2A=2`，丽璐 sheet=2 而 `+0x2A=1`）。禁止把 `+0x2A` 写进 `sheet`。

### 动画状态（广场 / 甲板 / 状态卡同一套）

- `[确认]` `CCharacter+0x84` 是动画状态。`fn_140010cdf` 在状态变化时重载 sheet。`fn_14006f830` 先把状态 **0x16→0x13、0x17→0x15**，再按 sheet 分组查帧下标：
  - sheet `< 0x27`（0–38）：`fn_1400715a0`
  - sheet `< 0x31`（39–48）：`fn_140070630`（Chara39）
  - sheet `< 0x33`（49–50）：`fn_140071690`（Chara40）
  - 其余（51–54）：`fn_140071460`（Chara41）
- `[确认]` `fn_1400715a0`（主组）：状态 `0x15` → 纹理 0x13；状态 ≥ `0x18`（24）失败（调用方把状态清 0）；否则 **纹理下标 = 状态值**。构造默认 `+0x84=0x1f=31 ≥ 24`，第一次加载会失败并回 0。来源：`0x1400715a0`。
- `[确认]` `fn_140071690`（Chara40）：状态 0/0x12/0x13 → 纹理 0/1/2。来源：`fn_14006f9e0.cpp`。
- `[确认]` 广场：`Plaza.dk4` 是场景底板/按钮，人物本体仍是 `CCharacter`+`Chara%02d`。来源：`fn_14016cc40.cpp` 同时引用两套文件。
- `[确认]` 甲板：`CDeckPersonnelView` 用 `fn_140006f96` 读已有 `CCharacter+0x80`，`fn_140012396` 取动画，再 `fn_1400123ff` + `fn_140010cdf`。来源：`CDeckPersonnelView.cpp`。RTC `cCharaAnime`、`posCabinChara`、`aePerson`。来源：`fn_14000a641`、`fn_14000d15c`。
- `[确认]` `NUM_NOWPERSON = 4`。来源：`num_values.json`。4 个「同时出场」槽，不是 235，也不是 42。

### 状态画面 / 六维雷达

- `[确认]` `CPersonInformationView` 体积 0x440。构造写入一整屏 HD 坐标；RTC 局部 `eSkill`（循环 `>= 0x0A`，10 技能指针在 `+0x300`）、`eEquip`（循环 `>= 5`，指针在 `+0x350`）、另外 0x12=18 个指针在 `+0x378`。来源：`CPersonInformationView.cpp`。
- `[确认]` 同文件绘制路径会 new 一个 `CCharacter`（`fn_14006ee50`，0x98）挂在 `+0x2f0`，用 `fn_1400083fa` 的 sheet 做站立图。来源：`fn_1401cb7f0.cpp`。
- `[确认]` `CHexagonSprite : CSolidSprite`，体积 0xf8。`vfunc_07` 向父控件要 `rcDest`，再 `fn_140013322`。来源：`CHexagonSprite.cpp`、`0x1401d75e0`。
- `[确认]` `CPlayerStatusView` 体积 0x188。`vfunc_13` 分支 8/9/0xA 改名 / 中间名 / 姓 / 势力名 / 月日（RTC `cFirstName,cMiddleName,cFamilyName,cForceName,nMonth,nDay`）。布局含 `0x780×0x438`（1920×1080）。来源：`CPlayerStatusView.cpp`、`12-ui-save/01-scenes-flow.md`。
- `[确认]` `CSkillView` / `CSkillValView` / `CPersonInfoButton` / `CPersonButton` 同属 person 功能目录。来源：`class_files.tsv`。
- `[未知]` 六维雷达的半径公式（`fn_140013322` 未还原）：用哪一档 cap（成长帽 100 还是结算帽 500）、顶点顺序是否就是 `eAbility` 0–5。布局常数在 `CPersonInformationView` 里，公式不在构造函数。

### 枚举符号（计数未写出的）

`NUM_BUSTUP`、`NUM_FACELOCATION`、`NUM_SEX` 均只出现在 World DLL 断言，头文件没有整数。来源：`BustupID.h` `FacelocationID.h` `SexID.h` `asserts.txt`。

## 3. 实体与 ID

运行时以表长循环。下表是原版包的实例计数。

| 实体 | 原版计数 | 空/非法 | 稳定 key 例 | 谁拥有 |
| --- | --- | --- | --- | --- |
| 人物 | 235 | `INVALID_PERSON=-1` | `raphael_castro` | `persons.yaml` |
| 逻辑 sheet | 0–54 用、56=空 | `0x38` | `sheet.00` … `sheet.54` | 本文件 |
| Chara 文件 | 42（00–41） | 无 | `chara_00` | 本文件 |
| bustup 纹理 | 文件 220；代码路径 `[0,211)` | clamp → 170 | `bustup.000` | 本文件 |
| Portrait 纹理 | 84 | `CShowPortrait+0x15c=-1` | `portrait.00` | 本文件 |
| Plaza 纹理 | 28 | — | `plaza.00` | `ui`（广场场景） |
| EvItem 纹理 | 39 | — | `evitem.00` | `narrative` / 小游戏 |
| EventBG 包 | 8×50 + EX 16 | — | `eventbg.1.00` | `narrative` |
| 动画状态 | 主组有效 0–23 | 失败回 0 | `anim.idle`（0） | 本文件 |
| 脸位 | `NUM_FACELOCATION` 未知 | `INVALID_FACELOCATION=-1` | — | 本文件（占位） |
| 性别 | 观察 2 | `INVALID_SEX=-1` | `male` / `female` | `persons.yaml` 已有 `sex` |
| 同时出场 | `NUM_NOWPERSON=4` | — | — | caps |

**三套不可混用的下标：**

```
person_id     0–234     世界表 / 名册
logical_sheet 0–54, 56  CCharacter+0x80
file_index    0–41      Chara%02d.DK4
bustup_index  0–219     bustup.dk4（代码只用到 0–210）
portrait_index 0–83     Portrait.dk4
still_id                EventBG*  叙事拥有
```

`person_id → logical_sheet` 是函数，不是 235 行的世界字段。`logical_sheet → file_index` 是第二段函数。禁止在业务代码里写 `Chara{person_id}.DK4`。

## 4. 算法 / 公式

表现层几乎没有数值公式。命名参数进 `config/characters/presentation.yaml` 的 `rules:`，禁止把 `39`、`0xD3`、`170` 只写在 cpp。

### F-PRES-sheet：`person_id` → 逻辑 sheet

```
sheet = lookup_sheet(person_id, woodin_alt_flag)

lookup_sheet:
  if person_id < 0 or person_id >= person_count: return sheet_unused
  if person_id == woodin_person_id and not woodin_alt_flag:
      return woodin_alt_sheet          # 原版 40
  if person_id < unique_sheet_count:   # 原版 39
      return person_id
  if person_id in sheet_exceptions:    # 原版 10 条，见 §2 表
      return sheet_exceptions[person_id]
  if person_id < sailor_roster         # 原版 98
      or person_id in packed_extra_ids:  # 原版 {192,199,200,201}
      return packed_generic_sheet      # 原版 48
  return sheet_unused                  # 原版 56
```

原版参数：

| 参数 | 原版 |
| --- | --- |
| `unique_sheet_count` | 39 |
| `woodin_person_id` | 5 |
| `woodin_alt_sheet` | 40 |
| `woodin_alt_flag` | 全局 `+0x7c >= 4`；语义未还原 |
| `packed_generic_sheet` | 48 |
| `sheet_unused` | 56 |
| `sheet_exceptions` | §2 十行 |

### F-PRES-file：逻辑 sheet → 文件

```
file_index =
  sheet  if sheet < packed_lo          # 39
  packed_file_a if packed_lo <= sheet < packed_mid   # 39–48 → 39
  packed_file_b if packed_mid <= sheet < packed_hi   # 49–50 → 40
  packed_file_c if packed_hi <= sheet < sheet_slots  # 51–54 → 41
  0 otherwise
filename = chara_pattern % file_index   # "Chara%02d.DK4"
```

### F-PRES-bustup：半身像下标

```
raw = Persons.definition(person_id).bustup    # 世界 +0x2B
index = raw if (0 <= raw < bustup_code_count) else bustup_fallback
# 原版 bustup_code_count=211, bustup_fallback=170
```

加载：`bustup.dk4` 的 `index` 号纹理，与 `mainbox.dk4` 对白框同屏。

### F-PRES-anim：状态 → 纹理下标

```
state' = {0x16: 0x13, 0x17: 0x15}.get(state, state)   # 原版别名
tex = frame_table[sheet_group(sheet)](sheet, state')
if not tex.ok: state = 0
```

主组（sheet 0–38）：`state' < 24` 则 `tex = state'`，`0x15` 特判成 `0x13`。默认构造状态 31 会失败并回 0。

Chara39/40/41 各有一张小 switch，把状态映射到**该文件内部**的纹理下标（Chara39 有 60 张，10 个逻辑人挤在同一文件里，下标还要加人物在包内的基址）。包内基址表未从 `fn_140070630` 完整还原，进「未核实」。

### 六维雷达

```
hexagon_vertex[i] = f(ability[i], hexagon_scale)   # i = 0..stat_count-1
```

`f` 未还原。重构仍要：顶点顺序 = `eAbility` 顺序（体力…运气）；半径用 **结算后** 的 `Characters.ability`（已经 cap），缩放参数 `hexagon_full_radius` 进 YAML。禁止 View 自己读生六维。

没有「按剧本改立绘」的人物公式。World1 的 bustup 差已经在内容包里。

## 5. 耦合分析

原版硬编码：

- `Chara%02d.DK4`、`bustup.dk4`、`Portrait.dk4`、`Plaza.dk4` 文件名写在 exe 字符串里。
- `fn_1400083fa` 把 `person_id` 与 sheet 焊死（伍丁特判、10 条例外、98 分界、192/199–201）。加第 236 人必须改 exe。
- `CCharacterManager` 槽数 55、空闲哨兵 56 写死。
- 对话框、状态卡、广场、甲板、选剧本各自 load 资源，没有统一的 `Presentation` 服务。
- `CPerson+0x0C`（世界 `+0x2A`）进存档，但行走图根本不读它——存档里多了一份**不驱动显示**的字节。
- `CFashionSprite` 与人物立绘同目录出现，实际属于流行商品。
- 事件 still 挂在 `CEvent` 上，和人物表没有外键。

重构切断点：

- 人物定义继续只持有 `person_id`。表现表用同一 id 做主键，World0 / World1 两份。
- 叙事只传 `person_id` + 可选 `still_id` / `face_loc`；不传 sheet。
- 广场场景拥有 `Plaza.dk4` 下标；人物系统只回答「这个 `person_id` 用哪套 sheet/anim」。
- 事件背景包属于 narrative，本系统只登记「人物没有 per-person still」这一事实。

## 6. 解耦后的配置

落点：`config/characters/presentation.yaml`（本系统）。规则参数与实例同行：规则是原版函数的可配化，实例是世界表 `+0x2B` 等。World0 / World1 同一 schema。

```yaml
# config/characters/presentation.yaml
schema: dk4.characters.presentation
version: 1
pack: vanilla-world0

assets:
  chara_pattern: "Chara%02d.DK4"
  chara_file_count: 42          # 00–41
  bustup_file: bustup.dk4
  bustup_texture_count: 220     # 文件 ntex
  portrait_file: Portrait.dk4
  portrait_texture_count: 84
  plaza_file: Plaza.dk4         # 场景拥有；这里只登记
  plaza_texture_count: 28
  evitem_file: EvItem.dk4
  evitem_texture_count: 39

rules:
  unique_sheet_count: 39
  sheet_slots: 55               # CCharacterManager 0x37
  sheet_unused: 56
  woodin_person_id: 5
  woodin_alt_sheet: 40
  packed_generic_sheet: 48
  packed_extra_ids: [192, 199, 200, 201]
  packed_ranges:                # logical sheet → file_index
    - {lo: 0,  hi: 39, file: identity}
    - {lo: 39, hi: 49, file: 39}
    - {lo: 49, hi: 51, file: 40}
    - {lo: 51, hi: 55, file: 41}
  sheet_exceptions:
    39: 39
    42: 41
    44: 42
    45: 43
    52: 44
    54: 45
    72: 46
    89: 53
    178: 51
    214: 52
  bustup_code_count: 211        # fn_140006d61 上界，不含
  bustup_fallback: 170
  anim_alias: {22: 19, 23: 21}  # 0x16→0x13, 0x17→0x15
  anim_main_count: 24           # 主组状态 [0,24)
  anim_default: 0               # 构造 31 失败后的回落
  hexagon_full_radius: null     # fn_140013322 未还原；先占位
  hexagon_stat_order: [0, 1, 2, 3, 4, 5]  # eAbility

# 实例。sheet 也可省略，由 rules 纯函数生成；写出是为了 World1 对照与模组覆盖。
# face_loc / plaza_anim / event_still：原版没有 per-person 表，保持 null。
- person_id: 0
  key: raphael_castro
  sheet: 0                      # → Chara00.DK4
  bustup: 0                     # bustup.dk4[0]；世界 +0x2B
  face_loc: null
  plaza_anim: 0                 # 回落后的默认状态，不是另一张图
  event_still: null
- person_id: 5
  key: woodin
  sheet: 5                      # 旗未立时规则改成 40 → Chara39
  bustup: 34
  face_loc: null
  plaza_anim: 0
  event_still: null
- person_id: 98
  key: plaza_shop_europe_m
  sheet: 56                     # 无行走图
  bustup: 107
  face_loc: null
  plaza_anim: null
  event_still: null
- person_id: 215
  key: mary_london              # 玛丽·伦敦；sheet 56=无行走图
  sheet: 56
  bustup: 66
  face_loc: null
  plaza_anim: null
  event_still: null
```

`caps.yaml` 增补（原版包默认）：

```yaml
chara_file_count: 42
character_sheet_slots: 55
now_person: 4
bustup_texture_count: 220
portrait_texture_count: 84
```

Plaza / EventBG / EvItem 的**使用**分别在 `ui/scenes.yaml` 与 `narrative/`，本表只防止人物系统去 fopen 那些包。

完整 235 行由管线生成：sheet 跑 `F-PRES-sheet`，bustup 抄世界 `+0x2B`。禁止手填「看名字像不像」。

## 7. 独立旋钮

改这些不应迫使经济 / 战斗 / 航海改代码：

- 某人的 bustup 下标、sheet 例外、伍丁旗
- 给第 236 人加一行（引擎按表长；sheet 函数要能表达「用 packed / unused / 新文件」）
- 换 Chara 包分辨率或把 42 个文件合成图集（资源层）
- World1 只改 bustup（已发生）
- 状态卡雷达半径、顶点顺序
- 把某职能 NPC 从「无行走图」改成共用 Chara39 的某个逻辑 sheet

会迫使别的系统改动的（禁止当旋钮）：

- `person_id` 稳定别名
- 把 bustup 下标当成 `eAbility`
- 让叙事 opcode 直接写 `Chara%02d`

## 8. 对外接口

只传 ID 与值。

查询：

```
Presentation.sheet(person_id) -> { logical_sheet, file_index, filename, unused: bool }
Presentation.bustup(person_id) -> { index, file: "bustup.dk4", fallback_applied: bool }
Presentation.portrait(index) -> { file: "Portrait.dk4", index }   # 不经 person_id
Presentation.anim_tex(person_id, anim_state) -> { file_index, tex_index } | none
Presentation.hexagon(person_id) -> { values[6], cap, vertex_order[6] }
  # values 来自 Characters.ability，不是生字节
```

命令（UI / 叙事发，表现层执行，不改六维）：

```
ShowBustup     { person_id, face_loc? }      # face_loc 未知则默认
ShowPortrait   { portrait_index }            # 选剧本 / 状态卡大图
ShowWalkSprite { person_id, where: plaza|deck|status, anim_state }
ShowEventStill { still_id, pack: eventbgN }  # 叙事拥有 still_id
HideSprite     { person_id }
```

事件：

```
SpriteShown { person_id, kind: bustup|walk|portrait }
SpriteHidden { person_id }
```

禁止：`Town` 自己 `fopen("Chara%02d.DK4")`；`Narrative` 把 `CCharacter*` 塞进脚本；`Economy` 读雷达顶点。

## 9. 未核实字段

| 项 | 状态 | 说明 |
| --- | --- | --- |
| `NUM_BUSTUP` 整数 | `[未知]` | 代码上界 211，文件 220 |
| `NUM_FACELOCATION` | `[未知]` | 只有符号；`CFaceView` 未露出下标 |
| 世界 `+0x2C` / `CPerson+0x29`（0–6） | `[未知]` | 7 档，像脸位或动画变体，**未**接到 `eFACELOCATION` |
| 世界 `+0x2A` / `CPerson+0x0C` | `[未知]` | 进存档；**不是** sheet。World1 改了 4 人 |
| 世界 `+0x27`–`+0x29` | `[未知]` | 见 `01-person-model.md`；`+0x29==13` 时改 `id%12` |
| 伍丁 `DAT_144296500+0x7c` | `[未知]` | 何条件把 sheet 5 换成 40 |
| 信息窗 `sC8 == 0x42` | `[未知]` | 强制 sheet 53 |
| Chara39 包内 10 人的纹理基址 | `[未知]` | `fn_140070630` switch 未完整还原 |
| 六维雷达 `fn_140013322` | `[未知]` | 半径、角度、用哪一档 cap |
| `person_id → Portrait 下标` | `[未知]` | 84 张与 235 人不是 1:1；选剧本表 42 int |
| `person_id → event still` | `[未知]` | still 在 EventBG* / EV 脚本，不在人物表 |
| `NUM_NOWPERSON=4` 的槽语义 | `[未知]` | 同时对话？同时广场？ |
| `aeScenarioPortrait` 42 个具体值 | `[未知]` | 已确认长度与 `0x47` 哨兵，未对齐到 7 个提督 |
| Plaza.dk4 extra=0x0C 的宽高 | `[未知]` | 头字段读成 0xFFFFFF；用 span 分档即可 |
| 空闲 sheet=56 却 `fn_14006f980` 返回文件 0 | `[推断]` | 若有人在 unused 时仍 load，会画出 Chara00。应在重构里对 `unused` 直接 no-op |

`plaza_anim` / `event_still` 在 YAML 里允许 `null`。不要为了填满 schema 去猜一张 EventBG 或某一帧。
