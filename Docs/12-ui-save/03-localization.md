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
- 缺 key 时 fallback 到另一种语言或硬编码中文

## 2. 原版事实

四套可执行文件与资源根：

| 语言 | exe | 资源目录 | 存档目录（文档） | 存档目录（ReadMe） |
| --- | --- | --- | --- | --- |
| 简中 | `DK4HD_sc.exe` | `Game/SC/` | `zh-CN` | `zh-CN` |
| 繁中 | `DK4HD_tc.exe` | `Game/TC/` | `zh-TW` | `zh-TW` |
| 日文 | `DK4HD_jp.exe` | `Game/JP/` | `ja-JP` | `ja` |
| 韩文 | `DK4HD_kr.exe` | `Game/KR/` | `ko-KR` | `ko` |

来源：`Game/` 实有四份 exe 与四份语言目录；`save_format.md` / `Save.cpp`；`ReadMe_SC/TC/JP/KR.txt`；`dk4save.c` 两种目录名都探。

每语言目录内同构：`mesfile.dk4`、`font.g1t`、`EV0..EV6.dk4` `EVA.DK4`、`Sc0..Sc6.dk4`、`World0.dll` `World1.dll`。

全游戏根上按语言分的资源：`TitleSC/TC/JP/KR.dk4`、`IconSC/TC/JP/KR.dk4`、`GoldRoadAnimeSC/TC/JP/KR.dk4`。

`mesfile.dk4`：

```
uint32 count;                 /* SC = 37 块 */
struct { uint32 off, size; } block[count];  /* 每块约 0xF00–0x1000 */
/* GBK 0 结尾字符串，SC 约 2055 条 */
```

来源：`srcdata/docs/formats.md`；抽出文本 `catalog/mesfile_sc.txt` 首行写明 2055 / 37。加载函数硬编码路径 `SC/mesfile.dk4`，块缓冲 `0x1000`（`fn_140141950.cpp` `fn_14000fe02`）。

SC exe 其它硬编码：

- `TitleSC.dk4`（标题、Game Over still）
- `IconSC.dk4`
- `\KoeiTecmo\DK4HD\Savedata\zh-CN\`

World DLL 表内字符串编码随语言（`formats.md`：GBK / SJIS / …）。SC 姓名与 mesfile 为 GBK；`dk4save.c` 用代码页 936。

排版：`CKanjiManager::LayoutGlyphs`（`0x140049fd0`）按字形宽换行；`CDk4KanjiManager` 继承之。`CString` 用 `lstrcpyA` 持有窄字符串。`CTextArea::DrawBox` 画文本框。姓名输入：`CLetterSelectView` + `CLetterSelectScrollView` + `CLetterSelectButton`（CJK 选字）。`NUM_MESSAGETYPE` 符号存在，计数 `[未知]`。

四语言函数体用指纹对齐：`catalog/lang_map.tsv`。常见平移：jp `+0x60`，tc `+0x340`，kr 不固定（`lang_offsets.md`）。这是 exe 对照表，**不是**文案系统的一部分。

窗口标题英文：`Uncharted Waters IV HD Version`（`Startup.cpp`）。

## 3. 实体与 ID

| ID | 取值 | 空 |
| --- | --- | --- |
| `language_id` | `zh-CN` `zh-TW` `ja-JP` `ko-KR` | 无默认语言 |
| `string_key` | 点分路径，如 `skill.0.desc` | 缺 key = 启动/加载失败 |
| `mes_index` | 原版 mesfile 下标 0–2054（SC） | 仅导入器用 |
| `message_type_id` | `eMessageType` | `invalid`；计数未还原 |

`string_key` 稳定，跨语言同一 key。禁止用中文当 key。

原版 mesfile **没有** key，只有顺序字符串，且一条记录里常拼接多句。导入器拆句、赋 key；对不上的进 `legacy.mes.<hex_off>`，待人工合并。

## 4. 算法 / 公式

取串：

```
text = table[language_id][key]
if missing: fail                    # 无 fallback
text = format(text, args)           # 只替换占位，不求值
```

占位符与原版对齐：`%s` `%6s` `%4s` 以及 mesfile 里已出现的 `%s枚金币` 这类。重构新文案用 `{name}` 命名占位，避免位置错位。同一 key 的占位符集合四语言必须相同，校验器检查。

编码：

| language_id | 原版编码 | 等级 |
| --- | --- | --- |
| zh-CN | GBK | `[确认]` |
| ja-JP | SJIS（World DLL / 日文包） | `[确认]` `formats.md` |
| zh-TW | 未在仓库写死 | `[未知]`（常见为 Big5，禁止当确认） |
| ko-KR | 未在仓库写死 | `[未知]` |

重构运行时内部 **UTF-8**。导入时按语言包 `source_encoding` 转码。存档姓名：新档 UTF-8；导入原版 SC 档按 GBK，其它语言按该包 `source_encoding`，失败则拒绝该字段。

mesfile 管道：

```
Game/<LANG>/mesfile.dk4
  → extract blocks (count, off, size)
  → decode source_encoding
  → split NUL strings
  → packs/vanilla-<lang>-text/strings.yaml   # key 人工/半自动
  → config/ui/strings.<lang>.yaml
```

World DLL 人名/港名走各自内容包的 `name_key`，不要把 DLL 字节直接画上屏。

## 5. 耦合分析

原版：四份 exe × 四份资源目录，逻辑相同、路径写死。改一句文案要改 mesfile；改标题图要换 `TitleXX.dk4` 文件名。技能说明甚至不在 mesfile，而在 SC exe `.rdata`（`CGoodSkillMenuData`）。

重构：一份引擎，启动时选 `language_id`。路径、编码、标题图、图标、mesfile、World 字符串、存档子目录全部来自 `languages.yaml`。技能说明改为 `skill.<id>.desc` key，从 exe 抽出的中文只进 zh-CN 包。

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
    source_encoding: [未知]
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
    save_dir: ja-JP
    save_dir_aliases: [ja]
    title_texture: TitleJP.dk4
    icon_texture: IconJP.dk4
    gold_road_anime: GoldRoadAnimeJP.dk4
    mesfile: JP/mesfile.dk4
    font: JP/font.g1t
    world_dll: [JP/World0.dll, JP/World1.dll]
  - id: ko-KR
    key: ko-KR
    name: 한국어
    source_encoding: [未知]
    original_exe: DK4HD_kr.exe
    resource_root: KR
    save_dir: ko-KR
    save_dir_aliases: [ko]
    title_texture: TitleKR.dk4
    icon_texture: IconKR.dk4
    gold_road_anime: GoldRoadAnimeKR.dk4
    mesfile: KR/mesfile.dk4
    font: KR/font.g1t
    world_dll: [KR/World0.dll, KR/World1.dll]
startup_window_title_key: window.title
```

```yaml
# config/ui/strings.zh-CN.yaml
schema: dk4.ui.strings
version: 1
language_id: zh-CN
strings:
  window.title: "Uncharted Waters IV HD Version"
  setup.pick_good_skills: "请选择３种熟练的技能。"
  setup.confirm_good_skills: "是{0}﹑{1}﹑{2}吗？"
  setup.enter_birthday: "请输入生日。"
  discovery.village: "提督！发现了村庄！"
  discovery.no_remains: "没有被发现的遗迹。"
  save.fair_progress: "这个进度是公正进度，结束游戏时请一定要存档。"
  skill.0.desc: "ＨＰ最大值，对疾病等的抵抗力。"
  skill.1.desc: "舰长、提督的资质，指挥能力。"
  skill.2.desc: "驾驶的资质，航海术、造船能力。"
  skill.3.desc: "方向感、土地感觉，绘制海图能力。"
  skill.4.desc: "鉴定商品，物价情报的判断力。"
  skill.5.desc: "剑的资质，白刃战的指挥能力。"
  skill.6.desc: "炮击的资质，大炮的命中力。"
  skill.7.desc: "辩论的资质，外交谈判能力。"
  skill.8.desc: "策略的资质，谍报能力。"
  skill.9.desc: "发现的资质，回避危险能力。"
  ability.0.name: "体力"
  ability.1.name: "敏捷"
  ability.2.name: "魅力"
  ability.3.name: "智力"
  ability.4.name: "精神"
  ability.5.name: "运气"
```

`strings.ja-JP.yaml` 等与 zh-CN **同一组 key**。抽出未赋 key 的 mesfile 行：

```yaml
legacy:
  mes_0x12c: "提督。好像广场上聚集了一群人。"
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

- zh-TW / ko-KR 的 `source_encoding`
- mesfile JP/KR/TC 的块数、条数是否与 SC 2055/37 一致
- 一条 mesfile 记录内多句如何切（SC 抽出经常是多句粘在同一 NUL 串）
- `NUM_MESSAGETYPE` 与 mesfile 下标是否同一空间
- 技能说明在 jp/tc/kr exe 中的对应偏移（SC 已在 `.rdata` `0x5910c0`）
- `CLetterSelectView` 字符表是否来自 mesfile、font，还是 exe
- 窗口标题四语言是否都用英文那句
- World DLL 人名与 mesfile 人名冲突时以谁为准
- `lang_map.tsv` 未对齐的函数（kr 平移不固定）对文案无影响，但对「按地址抽串」有影响
