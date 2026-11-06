# 跨剧本画廊

配置落点：`config/narrative/gallery.yaml`。文案 key 指向 `config/ui/strings.*.yaml`。额外立绘 / 照片的贴图 atlas 外键指向人物显示包（计划篇 `03-characters/05-portraits-animation.md`），本系统只持 ID。

剧本表（`01-scenarios.md`）只发 `ScenarioCleared`；**不**在 `scenarios.yaml` 里再写一套点数。点数、通关位、已解锁条目是跨存档槽的会话状态，原版对应 `savedata.dk4`，不是 `savedatXX.dk4`。

## 1. 职责边界

本系统管：

- 通关其它剧本之后，用点数解锁人物**长传记**与**额外照片**（日文原文「エクストラフォト」）
- 跨剧本进度：哪些 `scenario_id` 已通关、剩余点数、每条 `person_id` 已买哪些 `kind`
- 画廊定义表：花费、分组、每人可解锁的 still / bustup 条目
- 向 UI 提供只读投影（能否买、剩几点、简介 key、照片 id）

不管：

- 选哪个剧本开局、加载哪份世界包 / 事件包（`01-scenarios.md`）
- 事件脚本何时判定通关（`02-event-system.md` 发 `ScenarioCleared` 即可）
- 人物六维、名册、短介绍（选人界面 blurb 属人物 / 剧本表）
- `Chara00–41` / `bustup.dk4` / `Portrait.dk4` 的贴图像素与 atlas 切法（人物显示包）
- 标题菜单里其它「额外」项（道具图鉴扫描、音量）——只在第 2 / 9 节记事实，不并进本表
- 把 `CAcrossGameView` 当成港口小游戏（`08-town-life/03-minigames.md` 现把它误列进小游戏；本系统才是归属）

禁止：贸易 / 战斗 / 航海读 `scenario_id` 或画廊点数。人物系统不读「其它剧本是否通关」。

## 2. 原版事实

### 文案（花费与确认）

[确认] SC `mesfile.dk4` 文件偏移 `0xed0` 是**一条** C 字符串，抽出时把多句粘在一起。同一条里按语义切开：

| 片段 | 归属 |
| --- | --- |
| `我叫伍丁。在阿拉伯经营商会。` | 选人自称（伍丁），不是画廊 |
| `我是蒂雅。我一定要独立。` | 选人自称（蒂雅） |
| `可以吗？` | 通用确认残片，是否独立句 [未知] |
| `如果在其他剧本中顺利过关，使用１点就能显示此人物的人物简介，要显示吗？（剩%s点）` | 画廊：买简介 |
| `使用%s点显示额外的照片吗？（剩%s点）` | 画廊：买额外照片 |
| `多亏了巨额的商业投资，%s的景气恢复了。` | 投资，属经济 |

来源：`srcdata/catalog/mesfile_sc.txt`；原始文件 `Game/SC/mesfile.dk4`（`formats.md`：头 `uint32 count`，SC 37 块，随后 GBK 0 结尾串）。

[确认] 日文把两句确认拆成两条独立串，语义与 SC 切开后一致：

- `Game/JP/mesfile.dk4` 文件偏移 `0x15c8`：`よろしいですか？この人物のプロフィールは、他のあるシナリオをクリアすれば表示されますが、１ポイント使って表示しますか？（残り%sポイント）`
- 同文件 `0x1653`：`%sポイントを使って、エクストラフォトを表示しますか？（残り%sポイント）`

[确认] 繁中 `Game/TC/mesfile.dk4` `0xf9b` 与简中同构（简介写死「１點」，照片价是 `%s`）。

因此 vanilla 默认：

| 参数 | 原版 | 证据 |
| --- | --- | --- |
| `bio_cost` | 1 | 中日文都写死「１点 / １ポイント」 |
| `extra_photo_cost` | 运行时填 `%s` | 四语言都是占位符，**整数未还原** |
| 余额显示 | `剩%s点` / `残り%sポイント` | 同一句里的第二个 `%s`（简介）或第二个 `%s`（照片） |

[确认] 画廊要显示的**长传记**不是选人短介绍。短介绍在 `0xbab`–`0xd73`；长传记从 `0x1ef2f` 拉斐尔起到 `0x1f943` 蒂雅，其后是同伴传记。来源：`mesfile_sc.txt`、`03-characters/02-protagonists-scenarios.md`。抽出时华梅与杏太郎粘在 `0x1f447` 同一块，按人切开。

### 类与画面

[确认] RTTI 名即「跨游戏」：

| 类 | 体积（分配 / 析构） | 继承（节选） | 源 |
| --- | --- | --- | --- |
| `CAcrossGameView` | `0x210` | `CSurfaceSpriteView` → `CView` → … | `game/CAcrossGameView.cpp`；工厂 `fn_14000d0df` `malloc(0x210)` |
| `CAcrossGameCharacterPushButton` | `0x1d0` | `CCharacterPushButton` → `CPushButton` → `CCharacter` → … | 同目录；工厂 `fn_140012e5e` `malloc(0x1d0)` |
| `CShowPortrait` | `0x268` | 与 View 同族 | `person/CShowPortrait.cpp`；RTC 局部 616 字节 |

`class_files.tsv` 把 `CAcrossGameCharacterPushButton` 的个别 vfunc 标进 `combat/`、`ui/`，那是特征分类，不是三个类。

[确认] `CAcrossGameView` 构造写入一组 HD 布局常数（`+0xb0`…`+0xfc`），子对象指针 `+0x100 = 0`，`+0x108` / `+0x10c = 0xFFFFFFFF`，`+0x110 = 0`。`vfunc_02` 释放 `+0x100`。`vfunc_20` 读输入位：bit1 走一条虚函数；`0x7c00` 再扫 `eFunctionKey` 0–4（掩码从 `0x400` 左移）。来源：同 cpp。

[确认] `CAcrossGameCharacterPushButton` 构造同样写布局（`+0xb0`…`+0xdc`），`+0xf4` / `+0xfc = -1`，`+0x108 = 4`（4 的语义未还原），`+0x110 = 0`。`fn_140012e5e` 按调用方给出的**表长**循环：`r8` = 个数，`rdx` 指向步长 `0x10` 的记录表，每条 new 一个按钮并挂到 View。运行时按表长，禁止写死 7。

[确认] 打开 `CAcrossGameView` 的调用在 `fn_14000847c`（body `0x1400e1970`）：外层 `switch` 上界 6，内层再 `switch` 0–6，其中一臂 `call fn_14000d0df`。这是 7 槽菜单里的一格，不是港口设施。

### 标题菜单里的额外照片浏览器

[确认] 标题根菜单 `fn_14000d783`（body `0x140112f50`，RTC：`asRootMenu` 168 字节、两份 `abScenario` 各 28 字节、`eScenario`、`sGameSeed`）：

- 7 个菜单项，每项 stride `0x18`
- 项 2–5 的使能字节被 `fn_14000e1c9` 覆盖（未通关时额外项关掉，[推断] 依据：只改这四项、且与通关查询同一文件）
- `switch` 跳表 0–6（表 VA `0x140113458`）：

| case | 入口 | 观察 |
| --- | --- | --- |
| 0 | `0x1401131d3` | `fn_140002658` 读档 |
| 1 | `0x1401131f3` | 返回 0 |
| 2 | `0x140113204` | `fn_140002f86`（RTC `cSoundBoard`）音量 |
| 3 | `0x14011320e` | 扫 `eItem` 0…`0xE7`（231，与 `NUM_ITEM` 同数） |
| 4 | `0x140113350` | **额外照片**：先填 7 个通关位，再 `fn_1400089e5` |
| 5 | `0x1401133ca` | 同样扫 7 个通关位，再 `fn_1400032a1`（RTC `cSurfaceSprite`） |
| 6 | `0x1401131fa` | 返回 2 |

[确认] case 4 的路径：

1. `eScenario = 0..6`，`fn_1400050c4(ctx, id)` 写入 `abScenario[id]`（28 字节 = 7×`uint32`）
2. `fn_1400089e5(&abScenario)`
3. 该函数先扫 id 0–3：某个 `abScenario[i] != 0` 就播 SE `0x3E` 并停
4. 再扫 id 4–6：命中则连播 SE `0x25, 0x30, 0x37, 0x45`
5. 构造 `CShowPortrait`，三选项菜单：0 → `fn_1400017d5(portrait, 3, 0x27)`；1 → `(0x27, 0x46)`；其它退出。两端是否闭开区间未证明，只记这一对整数。

来源：`fn_1401200a0.cpp` 的 `fn_1400089e5` / `fn_1400017d5`；反汇编 `0x140112f50`–`0x140113456`。

这与 `fn_14000ff60` 的「`scenario_id >= 4` → World1」是同一条 4 分界，但是**内容包选择规则**，不是画廊引擎语义。重构按 `gallery.yaml` 的 `groups` 显式写两组。

[确认] `fn_1400067ee` / `fn_14000551f` 都按 `eScenario >= 7` 循环：前者对每 id 调 `fn_1400022b1`，后者调 `fn_1400050c4`；有一票 0 就整体失败，7 票全非 0 才成功。[推断] 「是否七本都通了」一类总开关，用来点亮标题额外项。未证明与点数增减是同一条路径。

[确认] `fn_140008c51` RTC `aeScenarioPortrait` 168 字节，局部第一个值写成 8，其后一串 `uint32`（大量 `0x47`），循环 6 次：值 `== 0x47` 则跳过，否则 `fn_1400118d8`。[未知] 8 是长度、哨兵还是第一张图 id；`0x47` 当跳过哨兵是观察，不是头文件枚举。

### 额外照片与 bustup 资源

[确认] 对话半身像文件 `bustup.dk4`，与 `mainbox.dk4` 一起被 `fn_140010e6f(..., "bustup.dk4", ...)` 加载。来源：`fn_1400c8390.cpp` `fn_140141950.cpp` `fn_1401c8290.cpp`。

[确认] `Portrait.dk4` 由 `fn_14011ea50` 加载。`CShowPortrait` 正是额外照片浏览器的宿主。

[确认] 两文件头都是 G1T 魔数 `GT1G4600`。`+0x08` = 文件大小（与实文件一致）。`+0x10` 为 `bustup.dk4=220`、`Portrait.dk4=84`，`+0x14=10`。按常见 G1T 布局 `+0x10` 是贴图张数，[推断] 尚未用偏移表闭合证明（`+0x20` 起 8 个 `uint32` 为 0，表不在这个位置）。

[确认] `eBUSTUP` 只有 `NUM_BUSTUP` / `INVALID_BUSTUP = -1`，整数未还原。`asserts.txt` 有界断言。人物世界表 `+0x2B` 曾被猜成立绘号，人物文档标 [未知]、未对上 `eBustup`。画廊**不要**复用那个字节当 extra photo id。

[推断] 「额外的照片 / エクストラフォト」是 `CShowPortrait` + `Portrait.dk4` 这一路；`bustup.dk4` 是对话 bustup。用户侧「extra bustup」按可解锁 still 配置：每条写 `resource: portrait | bustup` 与 `index`，贴图像素仍归人物显示包。禁止在没映射表之前把 `3..0x46` 写成确认的 bustup 枚举。

### 跨存档槽：`savedata.dk4`

[确认] 语言目录下另有 256 字节 `savedata.dk4`，魔法数与槽文件相同 `"DAIKOUKAI4PKW9 1"`。`fn_140012c1f` 按 `0x100` 读入；读失败则 `fn_140001b90` + `fn_140002045` 再建。`fn_140002045` 写回 `savedata.dk4` 同样 `0x100`。来源：`fn_1401200a0.cpp`、`12-ui-save/02-save-session.md`、仓库样例。

[确认] 样例 `Savedata/zh-CN/savedata.dk4` 与 `zh-TW/savedata.dk4` 都是 256 字节，`+0x10 = 0x00000100`。CN 样例 `+0x14` 起有非零载荷，TW 样例同区几乎为 0——说明这不是空模板填满，而是有无跨局进度的差异。字节含义未命名。

[确认] 标题 case 4/5 填完 `abScenario` 之后，同编译单元里多次 `fn_140002045(DAT_144296ac0, ...)` 写 sidecar。画廊进度**不**走 `savedatXX` 槽文件。

[未知] 点数整数、通关 7 位、已买 bio/photo 位在这 256 字节里的偏移。禁止把 CN 样例的 `+0x14` 乱码猜成点数。

### 与剧本 / 人物的边界

[确认] 七个剧本 `eScenario` 0–6 对应提督 `ePerson` 0–6。画廊按**人物**买简介，按**剧本**记通关。原版内容包里二者数值相等，模组可以让 `protagonist_person_id != scenario_id`，所以表里必须同时写两个外键，引擎禁止假设相等。

## 3. 实体与 ID

| 实体 | 原版 | 空值 | 稳定 key |
| --- | --- | --- | --- |
| `scenario_id` | 7 | `invalid` = -1 | 与剧本表同一套 |
| `person_id` | 画廊条目默认 0–6；表长运行时读 | `invalid` = -1 | 外键到人物，不复制姓名 / 六维 |
| `gallery_kind` | 文案两种 | 无 | `bio` `extra_photo` |
| `still_id` | 浏览器用 `3, 0x27, 0x46` 当分页界 | `invalid` = -1 | 只在画廊表出现；atlas 在显示包 |
| `bustup_id` | `eBUSTUP`，计数未知 | `invalid` = -1 | 可选；未映射则条目不写此字段 |
| `gallery_points` | 非负整数，上限未还原 | 0 | 会话标量，不是 `eFame` |
| `clear_flag` | `abScenario` 7×`uint32` | 0 = 未通关 | 按 `scenario_id` |

`NUM_SCENARIO=7` 只是 vanilla 实例数。分组、按钮个数、通关位数组都按 `gallery.yaml` / `scenarios.yaml` 表长。

哨兵：`0xFFFFFFFF` 出现在两个 AcrossGame 对象的「当前选中」槽；`aeScenarioPortrait` 里的 `0x47` 是**局部表**跳过值，不要提升成全局 still 空值（空 still 用 `-1`）。

## 4. 算法 / 公式

本系统没有战斗式公式。原版能钉住的只有花费与谓词。命名参数全部进 YAML，禁止引擎写死 `1` 或 `>= 4`。

```
cleared(scenario_id) =
    Gallery.store.cleared[scenario_id] == true

other_cleared(entry) =
    match entry.clear_rule:
        any_other:
            exists s in scenarios
                where s != entry.home_scenario_id and cleared(s)
        all:
            every s in scenarios: cleared(s)
        specific:
            every s in entry.requires_cleared: cleared(s)

can_offer(entry, kind) =
    other_cleared(entry)
    and not unlocked(entry.person_id, kind)
    and points >= cost(kind)

cost(bio)          = bio_cost              # vanilla 1
cost(extra_photo)  = extra_photo_cost      # vanilla 未还原，缺字段 = 拒绝加载

spend(entry, kind):
    if not can_offer(entry, kind): no-op
    points -= cost(kind)
    if points < 0: saturate 0            # 防御；正常路径减前已检查
    mark unlocked(entry.person_id, kind)
    persist sidecar
    emit GalleryUnlocked { person_id, kind }

on ScenarioCleared { scenario_id }:
    if already cleared(scenario_id):
        apply repeat_clear_policy        # vanilla 未还原，默认 no-op
        return
    mark cleared(scenario_id)
    points += points_on_clear            # vanilla 未还原；缺字段则只置位不加分
    persist sidecar
```

| 参数 | 原版 | 落点 |
| --- | --- | --- |
| `bio_cost` | 1 | `gallery.yaml` `economy.bio_cost` |
| `extra_photo_cost` | `%s` | 同文件；**禁止**用 1 当确认默认 |
| `points_on_clear` | 未还原 | 同文件，可 `null` |
| `repeat_clear_policy` | 未还原 | `no_op` / `add_again`；vanilla 先 `no_op` |
| `clear_rule` | 「其他剧本」 | 每条 `entry`；vanilla 用 `any_other` 并标可改 |
| `points_cap` | 未还原 | `caps.yaml` `gallery_points`；无值则不封顶 |
| `group_split` | 先 0–3 再 4–6 | 只作提取规则；运行时读 `groups` |

额外照片浏览器分页（只影响 UI 投影，不影响花费）：

```
page_i covers still ids between pages[i].start and pages[i].end
    # 闭开 / 闭闭未还原，UI 按表原样传给显示包
```

原版参数对是 `(3, 0x27)` 与 `(0x27, 0x46)`。模组改页界不必改 `CShowPortrait` 等价物的代码，只改表。未解锁的 still UI 自己决定遮罩，逻辑层只回答 `unlocked?`。

扣点是事务：先检查 → 扣点 → 置解锁 → 写 sidecar。禁止先显示简介再扣点（失败时源仍有、目标已有）。扣点失败整笔 no-op，不弹半开窗口。

## 5. 耦合分析

原版焊点：

1. **标题菜单、选人、画廊、音量挤在 `fn_14000d783`。** 重构后标题只发 `Ui.OpenGallery` / `Ui.OpenExtraPhotos`，本系统回答投影。
2. **`scenario_id >= 4` 既选 World DLL 又给画廊分组。** 世界包选择归剧本表；画廊分组归本表。两条表可以碰巧相同，代码不得共用一个 `if >= 4`。
3. **简介正文在 mesfile，逻辑在 AcrossGame 对象，进度在 sidecar。** 三层不要再焊回一个 C++ 类。
4. **`CAcrossGameView` 被小游戏文档点名。** 那是类名检索误伤。本系统打开条件是跨剧本进度，不是广场设施。
5. **人物显示包**（`bustup` / `Portrait` / `CharaXX`）若在画廊里再复制一份 id→文件映射，会与计划中的肖像文档双轨。本表只存 `still_id` / `bustup_id`。
6. **01-scenarios.md 曾把 `gallery:` 嵌进 `scenarios.yaml`。** 那是规则入口占位。花费、分组、条目以本文件的 `gallery.yaml` 为唯一表；剧本表只保留 `ScenarioCleared` 合同。

允许的依赖：本系统 ← 订阅 `ScenarioCleared`；本系统 → 查询 `Characters.get`、`Narrative.scenario_exists`、`Ui.ask_yes_no`。禁止反向。

sidecar 与槽文件：清档 / 换槽不得丢掉画廊（那正是 AcrossGame 的点）。删 sidecar 才是「忘掉通关」。实现上 `Save.delete(slot)` 不动 gallery 节。

## 6. 解耦后的配置

```yaml
# config/narrative/gallery.yaml
schema: dk4.narrative.gallery
version: 1

economy:
  bio_cost: 1
  extra_photo_cost: null          # 原版 %s，未还原；null = 加载器拒绝「用 1 凑」
  points_on_clear: null
  repeat_clear_policy: no_op      # no_op | add_again
  # points_cap 在 caps.yaml；此处不重复

store:
  sidecar: session                # 原版 savedata.dk4，256 字节
  original_blob_bytes: 256

browse:
  # fn_1400089e5 → CShowPortrait 两页。原版参数对 (3, 0x27) / (0x27, 0x46)。
  # end 是否开区间未证明；先按调用参数原样存。
  host: show_portrait
  resource: portrait              # Portrait.dk4；bustup 另见 entries
  pages:
    - { start: 3, end: 0x27 }
    - { start: 0x27, end: 0x46 }
  groups:
    - key: era_world0
      scenario_ids: [0, 1, 2, 3]
      se_on_any_cleared: [0x3e]
    - key: era_world1
      scenario_ids: [4, 5, 6]
      se_on_any_cleared: [0x25, 0x30, 0x37, 0x45]

kinds:
  - id: bio
    key: bio
    confirm_key: ui.gallery.confirm_bio
  - id: extra_photo
    key: extra_photo
    confirm_key: ui.gallery.confirm_extra_photo

entries:
  - person_id: 0
    key: raphael
    home_scenario_id: 0
    clear_rule: any_other
    requires_cleared: []          # specific 时才填
    bio_key: ui.gallery.bio.raphael
    extra_stills:
      - { kind: extra_photo, still_id: null, bustup_id: null, resource: portrait }
  - person_id: 1
    key: helmut
    home_scenario_id: 1
    clear_rule: any_other
    bio_key: ui.gallery.bio.helmut
    extra_stills:
      - { kind: extra_photo, still_id: null, resource: portrait }
  - person_id: 2
    key: lulu
    home_scenario_id: 2
    clear_rule: any_other
    bio_key: ui.gallery.bio.lulu
  - person_id: 3
    key: hua_mei
    home_scenario_id: 3
    clear_rule: any_other
    bio_key: ui.gallery.bio.hua_mei
  - person_id: 4
    key: kyotaro
    home_scenario_id: 4
    clear_rule: any_other
    bio_key: ui.gallery.bio.kyotaro
  - person_id: 5
    key: woodin
    home_scenario_id: 5
    clear_rule: any_other
    bio_key: ui.gallery.bio.woodin
  - person_id: 6
    key: tia
    home_scenario_id: 6
    clear_rule: any_other
    bio_key: ui.gallery.bio.tia
```

`still_id: null` 表示原版下标未从 `CShowPortrait` 分页对到人。提取器先输出确认的花费与分组；每人照片行留空，等映射再升 schema。禁止用选人短介绍冒充 `bio_key`。

文案（只进 strings 包）：

```yaml
# config/ui/strings.zh-CN.yaml  片段
ui.gallery.confirm_bio: "如果在其他剧本中顺利过关，使用{bio_cost}点就能显示此人物的人物简介，要显示吗？（剩{points}点）"
ui.gallery.confirm_extra_photo: "使用{photo_cost}点显示额外的照片吗？（剩{points}点）"
ui.gallery.bio.raphael: "葡萄牙中产阶级出身。是家中的次子……"   # 从 0x1ef2f 切出
```

占位符 `{key}`，由 `MessageManager` 替换。逻辑 YAML 里不准出现「１点」。

会话状态（存档节，不是内容包）：

```yaml
# session.gallery  替代 savedata.dk4 画廊段
schema: dk4.save.gallery
version: 1
points: 0
cleared_scenario_ids: []
unlocked:
  - { person_id: 0, kind: bio }
  - { person_id: 0, kind: extra_photo, still_id: 3 }
```

导入原版 sidecar：校验 16 字节魔法数与 256 字节长度；已命名字段（目前只有长度）进上表；其余进 `original_unknown` hex。缺映射就当「进度未还原」，不要猜通关位。

加载顺序：caps → formulas → 人物 / 剧本表 → **本表**（外键 `person_id` / `scenario_id` 必须已注册）→ 读 sidecar 覆盖状态。`extra_photo_cost` 为 `null` 时：简介购买仍可用，照片购买入口对 UI 返回 `unavailable`，不 fallback 成 1。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗 / 航海 / 人物公式改代码：

- `bio_cost` / `extra_photo_cost` / `points_on_clear`
- 某条目的 `clear_rule` 与 `requires_cleared`
- 给第 8 个剧本加画廊行（新 `person_id` 必须已在人物表）
- 分组从「4 人 + 3 人」改成别的切法
- 额外照片换 `Portrait` / `bustup` 资源包或改分页界
- 把某条 extra still 从一张改成多张（`extra_stills` 列表）
- 关掉标题「全部通关才显示额外菜单」（那是 UI 使能，逻辑层仍按 `can_offer`）

需要别人配合的只有：新 `person_id` 在 `persons.yaml`；新 `still_id` 在显示包；新 `scenario_id` 在 `scenarios.yaml`。加载期外键校验，缺则拒绝启动。

## 8. 对外接口

订阅：

| 事件 | 来源 | 动作 |
| --- | --- | --- |
| `ScenarioCleared { scenario_id }` | 剧本 / 事件 | `on ScenarioCleared` |
| `Ui.GalleryOpen` | 标题 / 选人 | 返回投影；不改点数 |
| `Ui.GallerySpend { person_id, kind }` | 确认框 Yes | `spend`；No 则 no-op |

发出：

| 事件 | 载荷 | 谁听 |
| --- | --- | --- |
| `GalleryUnlocked` | `person_id`, `kind`, `still_id?` | UI |
| `GalleryPointsChanged` | `points` | UI |
| `GalleryClearRecorded` | `scenario_id` | 标题菜单使能 |

查询（LIVE，sidecar 读不到则当全 0，不抛进玩法）：

```
Gallery.points() -> int
Gallery.cleared(scenario_id) -> bool
Gallery.unlocked(person_id, kind) -> bool
Gallery.can_offer(person_id, kind) -> bool
Gallery.entry(person_id) -> { bio_key, extra_stills, costs } | none
Gallery.browse_pages() -> [ {start, end} ]
Characters.get(person_id) -> PersonDef          # 只要姓名；不要六维
Narrative.scenario_exists(scenario_id) -> bool
```

命令：

```
Gallery.spend(person_id, kind) -> ok | no_op
Gallery.record_clear(scenario_id)            # 仅测试 / 导入器；玩法走事件
```

只传 ID 与纯值。禁止传 `CAcrossGameView*`、`savedata` 文件句柄、`World*`。

UI 合同：`CAcrossGameView` 等价物只显示投影 + 把点击当 `GallerySpend` 信号；`CAcrossGameCharacterPushButton` 是人物槽按钮，不是物品搬运。`CShowPortrait` 等价物只按已解锁 `still_id` 向显示包要图。确认框文案用 `confirm_key`，剩点用查询结果填 `{points}`。

标题额外项使能：UI 可问 `Gallery.cleared` 是否达到「至少一本」或「七本全通」；具体用哪条是 UI 表，不在引擎写死。`fn_14000e1c9` 的返回值未还原，故使能规则标进 `ui/scenes.yaml` 的 gallery 边，可改。

## 9. 未核实字段

| 项 | 现状 | 禁止 |
| --- | --- | --- |
| `extra_photo_cost` 整数 | 四语言都是 `%s` | 不要写成确认的 1 |
| `points_on_clear`、点数上限 | 只有余额文案 | 不要编「通一关 +1、上限 99」 |
| sidecar 256 字节布局 | 魔法数、长度 `0x100`、读写函数 | 不要把 CN 样例 `+0x14` 当点数 / 通关 bit |
| `abScenario` 的 `uint32` 是 0/1 还是计数 | 与 `!= 0` 比较 | 当 bool 投影，不把非 0 当「通了 N 次」 |
| 「其他剧本」是 any_other 还是指定 id | 日文「他のあるシナリオ」可两种读 | YAML 可配；vanilla 用 `any_other` 并保持 [推断] |
| `CAcrossGameCharacterPushButton+0x108 = 4` | 构造写入 | 不要当成「每组 4 张照片」 |
| `aeScenarioPortrait` 首值 8、哨兵 `0x47`、循环 6 | 局部数组 | 不要写成 8 张确认肖像表 |
| `CShowPortrait` 分页 `3 / 0x27 / 0x46` 与 `person_id` 的对应 | 只有界 | `still_id` 留 null |
| extra photo 用 `Portrait.dk4` 还是 `bustup.dk4` 的哪些张 | 浏览器加载 Portrait；对话加载 bustup | 双资源都进显示包；画廊条目写 `resource` |
| `NUM_BUSTUP` | 符号 + 断言 | 不要用 G1T `+0x10=220` 填枚举 |
| 人物 `+0x2B` | 人物文档 [未知] | 不要当 gallery bustup_id |
| 标题 case 3 的 231 道具扫描、case 5 的 `fn_1400032a1` | 与画廊同菜单 | 不并进 `gallery.yaml`；另篇或第 9 节保持未知 |
| `fn_14000e1c9` 使能条件 | 只见写入菜单项 2–5 | 不要写成「通一关就全开」 |
| SC `0xed0` 里的「可以吗？」 | 粘在简介句前 | 不单独当画廊确认句 |
| 同伴长传记（`0x1fb04` 起）是否也走点数 | 在蒂雅传记之后 | 本表 vanilla 只收 0–6；同伴要加必须显式 `entries` |
| 重复通关是否加分 | 无代码闭合 | `repeat_clear_policy: no_op` |

提取器：确认的花费、分组、类名、sidecar 大小进 YAML；照片下标与 sidecar 残余进 `unknown` / `null`。直到有闭合映射再升 `schema version`。
