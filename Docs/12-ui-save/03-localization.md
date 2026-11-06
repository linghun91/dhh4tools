# 本地化

四种语言是四份**内容包**，不是四套逻辑。字符串只有 key；逻辑代码不得出现玩家可见字面量。mesfile 是原文管道，不是运行时 API。

## 1. 职责边界

本系统管：

- 语言 id、编码、资源根目录、存档子目录
- 从 `mesfile.dk4` / World DLL 抽出字符串 → `ui/strings.<lang>.yaml`
- 运行时按 key 取串、代换占位符
- 字形布局入口（原版 `CKanjiManager` / `CDk4KanjiManager` / `CLetterSelectView`）

不管：

- 某句在哪个场景出现（场景只持 key）
- 港口/人物/商品的**玩法**字段（它们各有 `name_key`）
- jp/tc/kr 函数地址平移（`lang_map.tsv` 给逆向用，运行时不读）
- 字体光栅化实现细节（只要求语言包提供 `font.g1t` 或等价）

禁止：

- 在 YAML 逻辑表里写中文/日文句子（只能 `name_key`）
- 在字符串里编码分支（「若友好度>50 用这句」）
- 原版包缺 key 时 fallback 到另一种语言或硬编码中文（唯一例外：模组新增 key 可按 `fallback_language` 显示并告警，与 `05-config-architecture.md`「本地化 key」一致）

## 2. 原版事实

四套可执行文件与资源根：

| 语言 | exe | 资源目录 | 存档目录（exe 写死 = ReadMe） | `save_format.md` 写法 |
| --- | --- | --- | --- | --- |
| 简中 | `DK4HD_sc.exe` | `Game/SC/` | `zh-CN` | `zh-CN` |
| 繁中 | `DK4HD_tc.exe` | `Game/TC/` | `zh-TW` | `zh-TW` |
| 日文 | `DK4HD_jp.exe` | `Game/JP/` | `ja` | `ja-JP`（错） |
| 韩文 | `DK4HD_kr.exe` | `Game/KR/` | `ko` | `ko-KR`（错） |

来源：`Game/` 实有四份 exe 与四份语言目录；四个 exe 里的字符串 `\KoeiTecmo\DK4HD\Savedata\<dir>\` `[确认]`；`ReadMe_SC/TC/JP/KR.txt` 一致。`ja-JP` `ko-KR` 只出现在 `save_format.md` / `Save.cpp`，`dk4save.c` 两种都探只是容错。

每语言目录内同构：`mesfile.dk4`、`font.g1t`、`EV0..EV6.dk4` `EVA.DK4`、`Sc0..Sc6.dk4`、`World0.dll` `World1.dll`。

全游戏根上按语言分的资源：`TitleSC/TC/JP/KR.dk4`、`IconSC/TC/JP/KR.dk4`、`GoldRoadAnimeSC/TC/JP/KR.dk4`。

`mesfile.dk4`：

```
uint32 count;                 /* 块数 */
struct { uint32 off, size; } block[count];  /* size ≤ 4092，装进 0x1000 缓冲 */
/* 0 结尾字符串 */
```

| 包 | 块数 | 块大小 | 按 NUL 切分的非空串 | 等级 |
| --- | --- | --- | --- | --- |
| SC | 37 | 560–4092 | 2097（`mesfile_sc.txt` 按记录计 2055） | `[确认]` |
| TC | 51 | 2524–4092 | 2424 | `[确认]` |
| JP | 53 | 404–4092 | 3117 | `[确认]` |
| KR | 61 | 1276–4092 | 3187 | `[确认]` |

来源：`srcdata/docs/formats.md`；`catalog/mesfile_sc.txt` 首行 2055 / 37；其余为直接解析 `Game/<LANG>/mesfile.dk4`。**四包块数、条数都不同**，所以任何「第几条」都不能跨语言当同一下标用。加载函数硬编码路径 `SC/mesfile.dk4`，块缓冲 `0x1000`（`fn_140141950.cpp` `fn_14000fe02`）。

**消息号**（代码真正用的 id）`[确认]`：`fn_1400012da(id)` / `fn_14000fb14(id)` 的参数既不是 NUL 切分序号，也不是字节偏移。SC exe 有两张表：

| 表 | VA（文件偏移，按 .data 段 −`0x140001600`） | 内容 |
| --- | --- | --- |
| 块表 | `.data 0x140683620`（`0x682020`） | 37 项 `{u16 first_id, u16 block_size}`；`block_size` 与 `mesfile.dk4` 块头逐项相等；首项 `{0, 0xFE4}`，末项 `{0xFD2, 0x230}` |
| 偏移表 | `.data 0x1406836c0`（`0x6820C0`） | 每个消息号一个 `u16` 块内偏移；长度 = 下一条偏移差（块末条到块尾） |

按表解 SC 抽查：`0x0`「提督。好像广场上聚集了一群人。」、`0x6a`「没有被发现的遗迹。」、`0x6b`「请选择３种熟练的技能。」、`0x6c`「是%s﹑%s﹑%s吗？」、`0x6e`「请输入生日。」、`0x151`/`0x153`/`0x154` 公正进度三句、`0x170…0x173` 寿终四句、`0x608`「提督！发现了村庄！」。条目按偏移差切，部分条目**无 NUL 结尾**（如 `0x5`、`0x7`），所以按 NUL 切会把相邻两条粘成一串——这就是旧稿「一条记录里常拼接多句」与 2097 / 2055 计数不一的原因。（第一轮 09/11 组发现；本轮按段换算重读两表并解码复核）

TC/JP/KR exe 各自应有同形的两张表 `[推断]`（四包块数不同，SC 表不能套用）；地址未定位。

SC exe 其它硬编码（其它三个 exe 同形，换语言后缀 / 目录，`strings` 可见）：

- `TitleSC.dk4`（标题、Game Over still）
- `IconSC.dk4`
- `\KoeiTecmo\DK4HD\Savedata\zh-CN\`
- `SC/EV0.dk4` … `SC/EV6.dk4`（`.rdata 0x1405443d0` 起，指针表 `0x140682880`）；开机读每个文件偏移 2 的 `uint16` 作「事件版本」，存档头 `+0x24` 必须与之相等（见 `02-save-session.md`）

World DLL 表内字符串编码随语言（`formats.md`：GBK / SJIS / …）。SC 姓名与 mesfile 为 GBK；`dk4save.c` 用代码页 936。

排版：`CKanjiManager::LayoutGlyphs`（`0x140049fd0`）按字形宽换行；`CDk4KanjiManager` 继承之。`CString` 用 `lstrcpyA` 持有窄字符串。`CTextArea::DrawBox` 画文本框。姓名输入：`CLetterSelectView` + `CLetterSelectScrollView` + `CLetterSelectButton`（CJK 选字）。`NUM_MESSAGETYPE` 符号存在，计数 `[未知]`。

四语言函数体用指纹对齐：`catalog/lang_map.tsv`。常见平移：jp `+0x60`，tc `+0x340`，kr 不固定（`lang_offsets.md`）。这是 exe 对照表，**不是**文案系统的一部分。

窗口标题英文：`Uncharted Waters IV HD Version`（`Startup.cpp`）。四个 exe 都含这同一句与同一互斥体 `KOUKAI4PK_MUTEX`（`strings`），即标题不本地化，且四语言不能同时开。`[确认]`

四语言 `EV0..6.dk4` 头版本不同：SC `106 104 106 103 104 105 104`，TC/JP/KR `101 101 101 102 104 105 104`（偏移 2 的 `uint16`）。语言包不只是换文案，事件脚本也是不同版本。`[确认]`

## 3. 实体与 ID

| ID | 取值 | 空 |
| --- | --- | --- |
| `language_id` | `zh-CN` `zh-TW` `ja-JP` `ko-KR` | 无默认语言 |
| `string_key` | 点分路径，按内容 key 不按下标：`skill.accounting.desc`；模组带命名空间：`mymod:town.new_port.name` | 原版包缺 key = 启动失败 |
| `message_id` | 原版 exe 消息号（SC 块表 37 块，见 §2）；**每语言 exe 各一套** | 仅导入器用；不进运行时 |
| `message_type_id` | `eMessageType` | `null`；计数未还原 |

`string_key` 稳定，跨语言同一 key。禁止用中文当 key。

原版 mesfile 本身**没有** key；稳定 id 是 exe 消息号（§2）。导入器按消息号表切句（不按 NUL、不用字节偏移），再按 `message_id → string_key` 映射表赋 key；未映射的进 `legacy.msg_<lang>_<hex_id>`，待人工合并。映射表按语言分开维护（各 exe 消息号不同）。

## 4. 算法 / 公式

取串：

```
text = table[language_id][key]
if missing:
  if key 属模组 and languages[lang].fallback_language: 用它并告警
  else: fail                        # 原版包无 fallback
text = format(text, args)           # 只替换占位，不求值
```

占位符与原版对齐：`%s` `%6s` `%4s` 以及 mesfile 里已出现的 `%s枚金币` 这类。重构新文案用 `{name}` 命名占位，避免位置错位。同一 key 的占位符集合四语言必须相同，校验器检查。

编码：

| language_id | 原版编码 | 等级 |
| --- | --- | --- |
| zh-CN | GBK | `[确认]` |
| ja-JP | SJIS（World DLL / 日文包） | `[确认]` `formats.md` |
| zh-TW | Big5 / CP950 | `[推断]`：TC `mesfile.dk4` 2424 条中 2377 条可按 cp950 解码，按 GBK 仅 34 条；其余 47 条可能是 HKSCS/自造字，未查 |
| ko-KR | CP949（UHC） | `[推断]`：KR `mesfile.dk4` 3187 条中 3186 条可按 cp949 解码 |

重构运行时内部 **UTF-8**。导入时按语言包 `source_encoding` 转码。存档姓名：新档 UTF-8；导入原版 SC 档按 GBK，其它语言按该包 `source_encoding`，失败则拒绝该字段。

mesfile 管道：

```
Game/<LANG>/mesfile.dk4
  → extract blocks (count, off, size)
  → split by exe message table (first_id, offsets)   # 不按 NUL；SC 表 .data 0x140683620 / 0x1406836c0
  → decode source_encoding
  → map message_id → string_key（messages.<lang>.map.yaml）
  → packs/vanilla-<lang>-text/strings.yaml   # key 人工/半自动
  → config/ui/strings.<lang>.yaml
```

World DLL 人名/港名走各自内容包的 `name_key`，不要把 DLL 字节直接画上屏。

## 5. 耦合分析

原版：四份 exe × 四份资源目录，逻辑相同、路径写死。改一句文案要改 mesfile；改标题图要换 `TitleXX.dk4` 文件名。技能说明甚至不在 mesfile，而在 SC exe `.rdata`（`CGoodSkillMenuData`）。

重构：一份引擎，启动时选 `language_id`。路径、编码、标题图、图标、mesfile、World 字符串、存档子目录全部来自 `languages.yaml`。技能说明改为 `skill.<key>.desc`（key 取 `characters/skills.yaml` 的 `key`，如 `skill.accounting.desc`），从 exe 抽出的中文只进 zh-CN 包。

`lang_map.tsv` 留在工具链，用来继续还原 jp/tc/kr 函数，不进运行时。

## 6. 解耦后的配置

```yaml
# config/localization/languages.yaml
schema: dk4.localization.languages
version: 1
languages:
  - id: zh-CN
    key: zh-CN
    name: 简体中文
    source_encoding: gbk
    original_exe: DK4HD_sc.exe
    resource_root: SC
    save_dir: zh-CN
    save_dir_aliases: []
    title_texture: TitleSC.dk4
    icon_texture: IconSC.dk4
    gold_road_anime: GoldRoadAnimeSC.dk4
    mesfile: SC/mesfile.dk4
    font: SC/font.g1t
    world_dll: [SC/World0.dll, SC/World1.dll]
  - id: zh-TW
    key: zh-TW
    name: 繁體中文
    source_encoding: cp950       # [推断]，见 §4
    original_exe: DK4HD_tc.exe
    resource_root: TC
    save_dir: zh-TW
    title_texture: TitleTC.dk4
    icon_texture: IconTC.dk4
    gold_road_anime: GoldRoadAnimeTC.dk4
    mesfile: TC/mesfile.dk4
    font: TC/font.g1t
    world_dll: [TC/World0.dll, TC/World1.dll]
  - id: ja-JP
    key: ja-JP
    name: 日本語
    source_encoding: sjis
    original_exe: DK4HD_jp.exe
    resource_root: JP
    save_dir: ja                 # exe 写死
    save_dir_aliases: [ja-JP]    # 仅兼容旧工具
    title_texture: TitleJP.dk4
    icon_texture: IconJP.dk4
    gold_road_anime: GoldRoadAnimeJP.dk4
    mesfile: JP/mesfile.dk4
    font: JP/font.g1t
    world_dll: [JP/World0.dll, JP/World1.dll]
  - id: ko-KR
    key: ko-KR
    name: 한국어
    source_encoding: cp949       # [推断]，见 §4
    original_exe: DK4HD_kr.exe
    resource_root: KR
    save_dir: ko                 # exe 写死
    save_dir_aliases: [ko-KR]    # 仅兼容旧工具
    title_texture: TitleKR.dk4
    icon_texture: IconKR.dk4
    gold_road_anime: GoldRoadAnimeKR.dk4
    mesfile: KR/mesfile.dk4
    font: KR/font.g1t
    world_dll: [KR/World0.dll, KR/World1.dll]
fallback_language: null        # 只对模组 key 生效；原版包校验时必须为全集
startup_window_title_key: window.title
```

```yaml
# config/ui/strings.zh-CN.yaml
schema: dk4.ui.strings
version: 1
language_id: zh-CN
strings:
  window.title: "Uncharted Waters IV HD Version"
  setup.pick_good_skills: "请选择３种熟练的技能。"            # msg 0x6b
  setup.confirm_good_skills: "是{0}﹑{1}﹑{2}吗？"              # msg 0x6c
  setup.enter_birthday: "请输入生日。"                          # msg 0x6e
  discovery.village: "提督！发现了村庄！"                        # msg 0x608
  discovery.no_remains: "没有被发现的遗迹。"                     # msg 0x6a
  save.fair_progress: "这个进度是公正进度，结束游戏时请一定要存档。"  # msg 0x153
  skill.vitality.desc: "ＨＰ最大值，对疾病等的抵抗力。"          # exe .rdata，非 mesfile
  skill.leadership.desc: "舰长、提督的资质，指挥能力。"
  skill.helmsmanship.desc: "驾驶的资质，航海术、造船能力。"
  skill.surveying.desc: "方向感、土地感觉，绘制海图能力。"
  skill.accounting.desc: "鉴定商品，物价情报的判断力。"
  skill.fencing.desc: "剑的资质，白刃战的指挥能力。"
  skill.gunnery.desc: "炮击的资质，大炮的命中力。"
  skill.persuasion.desc: "辩论的资质，外交谈判能力。"
  skill.tactics.desc: "策略的资质，谍报能力。"
  skill.observation.desc: "发现的资质，回避危险能力。"
  ability.stamina.name: "体力"
  ability.agility.name: "敏捷"
  ability.charm.name: "魅力"
  ability.intelligence.name: "智力"
  ability.spirit.name: "精神"
  ability.luck.name: "运气"
```

`strings.ja-JP.yaml` 等与 zh-CN **同一组 key**。抽出未赋 key 的 mesfile 行：

```yaml
legacy:
  msg_sc_0x000: "提督。好像广场上聚集了一群人。"   # 消息号 0x0；同句还有 0x1、0x8 等变体
```

人工归并进正式 key 后删除 `legacy` 项。校验：四语言正式 key 集合相等；`legacy` 允许暂时不等。

人名/港名不在 `ui/strings` 重复维护，用 `characters/persons.yaml` 的 `name_key` 指到 `packs/vanilla-*-text`。

## 7. 独立旋钮

| 旋钮 | 说明 |
| --- | --- |
| `languages[]` | 加语言 = 加一份包 + 一份 strings，不改系统代码 |
| `source_encoding` | 只影响导入 |
| `save_dir` / aliases | 导入原版存档时的探路 |
| `title_texture` 等路径 | 换皮 |
| 任何 `strings.*` | 改文案 |
| 占位符风格 `%s` vs `{0}` | 按 key 逐步迁，校验器按 key 声明 |

字体、选字盘字符集随语言包，不进公式。

## 8. 对外接口

```
Loc.text(key, args...) -> string
Loc.has(key) -> bool
Loc.language() -> language_id
Loc.set_language(language_id)     # 热切换只换表与字体；不改存档玩法节
Loc.format_check(key, arg_count) -> Result
```

系统代码：

```
bubble.show(Loc.text("discovery.village"))
```

而不是 `bubble.show("提督！发现了村庄！")`。

事件脚本 EV 里的玩家可见句同样走 key；脚本字节里的原版内嵌字，导入时抽成 `event.<script>.<n>`。

## 9. 未核实字段

- zh-TW / ko-KR 的 `source_encoding` 只做了解码率推断；TC 有 47 条 cp950 解不开
- JP/KR/TC exe 的消息号块表 / 偏移表地址（SC 已解，见 §2；粘句问题随之消失）
- 原版消息号 → 重构 `string_key` 的映射表尚未建立（2000+ 条，需半自动）
- `NUM_MESSAGETYPE` 与 mesfile 下标是否同一空间
- 技能说明在 jp/tc/kr exe 中的对应偏移（SC 在文件偏移 `0x5910c0`，即 VA `0x1405922c0`，`%6s:ＨＰ最大值…` 起）
- `CLetterSelectView` 字符表是否来自 mesfile、font，还是 exe
- World DLL 人名与 mesfile 人名冲突时以谁为准
- `lang_map.tsv` 未对齐的函数（kr 平移不固定）对文案无影响，但对「按地址抽串」有影响
