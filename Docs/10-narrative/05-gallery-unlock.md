# 跨剧本画廊

配置落点：`config/narrative/gallery.yaml`。文案 key 指向 `config/ui/strings.*.yaml`。额外照片的贴图 atlas 外键指向人物显示包（计划篇 `03-characters/05-portraits-animation.md`），本系统只持 ID。

会话层发 `ScenarioCleared { scenario_id }`（`01-scenarios.md`）；**不**在 `scenarios.yaml` 里再写一套点数。通关位由 profile 订阅 `ScenarioCleared` 写入，本系统只读 `Profile.cleared`（词表 §14-24）；首通加点与免费解锁的**规则**归本表，由 profile 的通关记账按本表执行。点数、通关位、已解锁条目是跨存档槽的 `profile` 状态（D7），原版对应 `savedata.dk4`，不是 `savedatXX.dk4`。

## 1. 职责边界

本系统管：

- 通关记账**规则**：首通加点、免费解锁一批人物简介（profile 在 `ScenarioCleared` 时按本表执行；通关位本身由 profile 写，本系统只读）
- 用点数解锁人物**简介**（profile）与**额外照片**（日文「エクストラフォト」）
- 跨剧本进度：剩余点数、每个画廊条目已解锁哪些 `kind`；哪些剧本已通关只读 `Profile.cleared`
- 画廊定义表：条目→人物、分页、每条照片张数、免费解锁表、花费规则
- 向 UI 提供只读投影（能否买、剩几点、简介 key、照片 id）

不管：

- 选哪个剧本开局、加载哪份世界包 / 事件包（`01-scenarios.md`）
- 事件脚本何时判定通关（脚本 opcode `0xC5` imm 0，见 `03-event-commands.md`）
- 人物六维、名册、短介绍（选人界面 blurb 属人物 / 剧本表）
- `bustup.dk4` / `Portrait.dk4` 的贴图像素与 atlas 切法（人物显示包）
- 标题菜单里其它「额外」项（道具图鉴、音量）——只在第 2 / 9 节记事实
- `CAcrossGameView`：那是事件脚本启动的小游戏（opcode `0xC1` 内层 2），归 `08-town-life/03-minigames.md`。旧稿把它当成画廊，**错误**

禁止：贸易 / 战斗 / 航海读 `scenario_id` 或画廊点数。人物系统不读「其它剧本是否通关」。

## 2. 原版事实

### 文案（花费与确认）

[确认] SC `mesfile.dk4` 文件偏移 `0xed0` 是**一条** C 字符串，抽出时把多句粘在一起。同一条里按语义切开：

| 片段 | 归属 |
| --- | --- |
| `我叫伍丁。在阿拉伯经营商会。` | 选人自称（伍丁），不是画廊 |
| `我是蒂雅。我一定要独立。` | 选人自称（蒂雅） |
| `可以吗？` | 日文同位置是简介确认句的开头「よろしいですか？」 |
| `如果在其他剧本中顺利过关，使用１点就能显示此人物的人物简介，要显示吗？（剩%s点）` | 画廊：买简介 |
| `使用%s点显示额外的照片吗？（剩%s点）` | 画廊：买额外照片 |
| `多亏了巨额的商业投资，%s的景气恢复了。` | 投资，属经济 |

来源：`srcdata/catalog/mesfile_sc.txt`；原始文件 `Game/SC/mesfile.dk4`（`formats.md`：头 `uint32 count`，SC 37 块，随后 GBK 0 结尾串）。

[确认] 日文 `Game/JP/mesfile.dk4` `0x15c8`：`よろしいですか？この人物のプロフィールは、他のあるシナリオをクリアすれば表示されますが、１ポイント使って表示しますか？（残り%sポイント）`；`0x1653`：`%sポイントを使って、エクストラフォトを表示しますか？（残り%sポイント）`。繁中 `Game/TC/mesfile.dk4` `0xf9b` 与简中同构。

[确认] 代码里两条确认框是消息号 `0x7e`（简介，参数 = 剩余点数）与 `0x7f`（照片，参数 = 花费、剩余点数），由 `fn_140009b1f` 弹出。来源：`0x14011a2c6`、`0x14011a3bc`。

[确认] 画廊要显示的**长传记**不是选人短介绍。短介绍在 `0xbab`–`0xd73`；长传记从 `0x1ef2f` 拉斐尔起到 `0x1f943` 蒂雅，其后（`0x1fb04` 起）是同伴传记。来源：`mesfile_sc.txt`。

### 画廊宿主：`CShowPortrait`

[确认] 标题根菜单 `fn_14000d783`（body `0x140112f50`，RTC `asRootMenu`、`abScenario`、`eScenario`、`sGameSeed`）的 `switch` 跳表 `0x140113458`（0–6）：

| case | 入口 | 观察 |
| --- | --- | --- |
| 0 | `0x1401131d3` | `fn_140002658` 读档 |
| 1 | `0x1401131f3` | 返回 0 |
| 2 | `0x140113204` | `fn_140002f86`（RTC `cSoundBoard`）音量 |
| 3 | `0x14011320e` | 扫 `eItem` 0…`0xE7`（231 = `NUM_ITEM`） |
| 4 | `0x140113350` | **画廊**：先用 `fn_1400050c4` 从 sidecar 填 7 个通关位到 `abScenario`，再 `fn_1400089e5` |
| 5 | `0x1401133ca` | 同样填 7 个通关位，再 `fn_1400032a1`（RTC `cSurfaceSprite`），[未知] |
| 6 | `0x1401131fa` | 返回 2 |

[确认] 额外项使能用 `fn_14000e1c9`（body `0x140121b20`）：7 个通关位任一置位就返回 1。即「至少通关一个剧本」才点亮。

[确认] `fn_1400089e5`（body `0x1401203d0`，RTC `cShowPortrait`、`aszMenu`、`eScenario`）：

1. 扫 `abScenario[0..3]`：任一非 0 → `fn_1400118d8(sidecar, 0x3E)`（免费解锁画廊条目 62），停
2. 扫 `abScenario[4..6]`：任一非 0 → 解锁条目 `0x25, 0x30, 0x37, 0x45`，停
3. 构造 `CShowPortrait`，三选项菜单 `伙伴篇 / 其他篇 / 取消`（rdata `0x140555a14`、`0x140555a1c`、`0x140536b54`）
4. `伙伴篇` → `fn_1400017d5(portrait, 0, 0x27)`；`其他篇` → `fn_1400017d5(portrait, 0x27, 0x46)`；即条目 `[0, 39)` 与 `[39, 70)` 两页

旧稿把这两处写成「播 SE」和「(3, 0x27)」，都错：`fn_1400118d8` 是置解锁位，`3` 是菜单项数（传给 `fn_14000e80e` 的 `edx`）。

[确认] 买简介 / 买照片在 `CShowPortrait` 的 vfunc 13（vtable `0x140555870` 第 13 槽 → vtordisp thunk `0x14011a130` → body `0x14011a140`）。按钮码 `switch`（上界 5，跳表 `0x14011a46c`）里 case 3 / 4 两支：

| 支 | 条件 | 动作 |
| --- | --- | --- |
| 简介 | 点数 `> 0`，弹 `0x7e` 选「是」 | 置简介解锁位（`+0x4E` 位图，`fn_1400118d8`），扣 1 点 |
| 照片 | 本条照片张数 `n > 0` 且点数 `≥ n+1`，弹 `0x7f(n+1, 点数)` 选「是」 | 置照片解锁位（`+0x42` 位图，`fn_14000b2ad`），扣 `n+1` 点 |
| 点数不足 | — | `fn_14000cf1d(snd, 2, 2, 1)` 提示音 |

扣点 `fn_140008828`（body `0x140122d30`）：`points = points < cost ? 0 : points − cost`，u16 饱和。

### 画廊条目表

[确认] rdata `0x140552540`，70 条 × 12 字节 `{int32 person_id, int32 photo_count, int32 photo_base}`：

- 条目 0–55 = `person_id` 0–55；56–69 = 60, 72, 86, 87, 89, 90, 91, 92, 93, 94, 95, 96, 97, 213
- 有照片的 12 条（共 14 张，`photo_base` 0–13 连续）：拉斐尔 1、赫德拉姆 1、丽璐 2、华梅 2、杏太郎 1、伍丁 1、克莉丝汀娜 1、安杰洛 1、易安 1、詹姆斯 1、芬（72）1、夏洛特（92）1
- 照片价 = `photo_count + 1`：1 张的人 2 点，2 张的人 3 点。mesfile 里的 `%s` 就是它

### 通关记账与免费解锁

[确认] 通关路径（`0x140112820` 处理 `CDk4Game` 流程态 4）：

1. `fn_14000b677(1)`：片尾动画（RTC `cEventAnimOP`）
2. `fn_14000a321(sidecar)`：把当前已发现的道具（`eItem < 0xE7`）与遗迹（`eRemains < 0x19`）写进 sidecar 图鉴位图
3. 若 `fn_1400050c4(sidecar, scenario)` 为 0（本剧本首次通关）：
   - `fn_14000d021(sidecar, 4)`：点数 `+4`（u16 加法，无上限检查）
   - `fn_140010127(sidecar, scenario)`：置通关位
   - `fn_140008c51(sidecar, scenario)`：按 `aeScenarioPortrait[scenario]` 免费解锁简介，再解锁条目号 = `scenario` 本身
4. `fn_140002045`：写回 `savedata.dk4`

重复通关：不加点、不重复解锁（第 3 步整段跳过）。

[确认] `aeScenarioPortrait`（`fn_140008c51` 栈上 7 × 6 个 `int32`，`0x47` = 空槽，循环 6 次）：

| 剧本 | 免费解锁的画廊条目（人物） |
| --- | --- |
| 0 | 8 库拉乌迪、11 阿尔加迪斯、59 汉斯、63 夏洛特 |
| 1 | 27 塞拉、61 谢乌德 |
| 2 | 12 卡米尔 |
| 3 | 13 希恩、58 理查德、69 比安卡 |
| 4 | 31 令、38 塞西莉雅、65 克丽雅、66 爱斯法妮雅、67 特欧贝尔德、68 宗九郎 |
| 5 | 32 阿米娜、33 德尼雅、34 法娣玛、35 哈希姆、66 爱斯法妮雅 |
| 6 | 25 塞维、36 柳科、56 西门、57 芬、65 克丽雅、67 特欧贝尔德 |

旧稿「首值 8 是长度」错：8 是剧本 0 的第一个条目号；栈上 `+0x20` 的 6 才是循环上界。

### 跨存档槽：`savedata.dk4`

[确认] 语言目录下 256 字节 `savedata.dk4`，魔数与槽文件相同 `"DAIKOUKAI4PKW9 1"`。读失败时 `fn_140001b90` 从 rdata 模板 `0x140555f70` 拷 `0x100` 字节重建；`fn_140002045` 整块写回 `0x100` 字节。内存对象 `DAT_144296ac0` 就是文件镜像。写回时机：每次存档后（`fn_14000d684`）和剧本通关后（流程态 4）。与 `12-ui-save/02-save-session.md`「系统档」同一张表，偏移以本表为准（下表每行都有访问函数立即数）。

[确认] 布局（偏移由各访问函数的立即数钉死）：

| 偏移 | 类型 | 含义 | 函数 |
| --- | --- | --- | --- |
| +0x00 | char[16] | 魔数 | `fn_140001b90` |
| +0x10 | uint32 | `0x100` | 模板 |
| +0x14 | bit[231] | 已发现道具（图鉴） | `fn_140007b44` / `fn_140008f3a` |
| +0x34 | bit[25] | 已发现遗迹 | `fn_1400134e4` / `fn_140011cc5` |
| +0x38 | uint16 位图 | 剧本通关位，bit = `scenario_id` | `fn_140010127` / `fn_1400050c4` |
| +0x3A | uint16 位图 | 另一张按 `eScenario` 的位图；语义 [未知]。`fn_1400067ee` / `fn_140012d32` 循环 `< 7` 扫它 | `fn_1400022b1`（body `0x140121c10`，`movzx [rcx+0x3a]; and 1<<s`） |
| +0x3C / +0x3D | 各 bit7 一个开关 | 改动即刷新；[推断] BGM / 音效类选项（归 `12-ui-save`）。两份样例 `+0x3C = 0xC0` | `fn_14000c121`（`bts/btr [+0x3c],7`）/ `fn_14000e8fe`（`+0x3d`） |
| +0x40 | uint16 | 画廊点数（**不是**剧本号） | `fn_14000d021`（body `0x1401216d0`：`[+0x40] += dx`，通关处 `mov dx,4`）/ `fn_140008828`（扣）/ `fn_1400146f5`（`0x14011b4b0` 读） |
| +0x42 | bit[70] | 额外照片已解锁，bit = 画廊条目号 | `fn_14000b2ad` |
| +0x4E | bit[70] | 简介已解锁，bit = 画廊条目号 | `fn_1400118d8` |

[确认] 模板 `+0x4E` 默认已置位的条目：7, 9, 10, 14–26, 28–30, 39–47, 49–54, 60, 62, 64。即多数同伴与对手简介开局可看；七主角（0–6）与上表免费解锁的人物要通关或花点。

[确认] 两份样例 `Savedata/zh-CN`、`zh-TW`：`+0x38 = 0`、`+0x3A = 0`、`+0x40 = 0`（都未通关、0 点）、`+0x4E` 起 `80 c6 ff 77 80 ff 7e 50 01` 与模板默认简介位一致；CN 的 `+0x14…+0x37` 有图鉴位。与上表一致。

### 与剧本 / 人物的边界

[确认] 七个剧本 `eScenario` 0–6 对应提督 `ePerson` 0–6。通关位按**剧本**记，解锁位按**画廊条目**记，条目再映射到 `person_id`。原版条目 0–6 恰好 = `person_id` 0–6 = `scenario_id`，`fn_140008c51` 直接用 `scenario` 当条目号解锁——这是原版巧合，表里必须分开写三个外键。

## 3. 实体与 ID

| 实体 | 原版 | 空值 | 稳定 key |
| --- | --- | --- | --- |
| `scenario` | 7 | `null` | 与剧本表同一套 key（`vanilla:raphael`） |
| `gallery_entry` | 70（条目表长） | `null`；原版 `0x47` 空槽只在 `legacy_codec` | 本表主键，命名空间 key（`vanilla:gallery_raphael`）；原版条目号作 `vanilla_index`（位图位号） |
| `person` | 条目外键 | `null` | 人物 key，不复制姓名 / 六维 |
| `gallery_kind` | 2 | 无 | `bio` `extra_photo` |
| `photo_index` | 0–13（`photo_base + k`） | — | atlas 在显示包 |
| `gallery_points` | uint16 | 0 | 会话标量，不是 `eFame` |
| `clear_flag` | uint16 位图 7 位 | 0 = 未通关 | 按 `scenario_id` |

`NUM_SCENARIO=7`、70 条目、14 张照片都只是 vanilla 实例数。分页、免费表、位图长度都按表长。分页用条目列表而不是下标区间，模组追加条目不必挤进 `[0,70)`。

## 4. 算法 / 公式

```
# 执行方 = save / profile（订阅 ScenarioCleared，词表 §14-24）；规则与参数 = 本表 gallery.yaml
on ScenarioCleared { scenario_id }:
    record_discoveries()                       # 道具 / 遗迹图鉴（profile.encyclopedia）
    if Profile.cleared(scenario_id):           # 原版：重复通关整段跳过
        return
    Profile.set_cleared(scenario_id)           # 通关位只由 profile 写
    points = min(points + points_on_first_clear, points_cap)   # 原版 +4，u16 无检查
    for e in free_unlocks[scenario_id]: unlock(e, bio)
    if unlock_home_entry: unlock(home_entry(scenario_id), bio)
    persist sidecar
    emit GalleryClearRecorded { scenario_id }

on GalleryOpened:
    for rule in open_unlocks:                  # 原版两条：0–3 任一通关 → 62；4–6 任一 → 37,48,55,69
        if any(Profile.cleared(s) for s in rule.scenarios): for e in rule.entries: unlock(e, bio)

cost(entry, bio)         = bio_cost                                  # 原版 1
cost(entry, extra_photo) = entry.photo_count * photo_cost_per_photo + photo_cost_base   # 原版 1, 1
can_offer(entry, kind)   = not unlocked(entry, kind)
                           and (kind != extra_photo or entry.photo_count > 0)
                           and points >= cost(entry, kind)
                           and points > 0                            # 原版简介支额外要求

spend(entry, kind):
    if not can_offer(entry, kind): no-op
    mark unlocked(entry, kind)
    points = max(0, points - cost(entry, kind))
    persist sidecar
    emit GalleryUnlocked { entry, person_id, kind }
```

| 参数 | 原版 | 证据 |
| --- | --- | --- |
| `bio_cost` | 1 | `0x14011a2e8` `mov dx,1` |
| `photo_cost_per_photo` / `photo_cost_base` | 1 / 1 | `0x14011a35c` `[entry+4]; inc` |
| `points_on_first_clear` | 4 | `0x1401129d5` `mov dx,4` |
| `repeat_clear_policy` | `no_op` | 通关位守卫 `0x1401129d1` |
| `points_cap` | 65535（u16 回绕，原版不检查） | `fn_14000d021` |
| `unlock_home_entry` | true | `0x140122b4a` |
| `pages` | `[0,39)` 伙伴篇、`[39,70)` 其他篇 | `0x140120556`、`0x14012056d` |
| 使能 | 任一剧本通关 | `fn_14000e1c9` |

原版加点不封顶会在 65535 回绕（7 本最多 28 点，实际碰不到）。新引擎用 `caps.yaml` 封顶，不复刻回绕。

扣点是事务：先检查 → 置解锁 → 扣点 → 写 sidecar。原版顺序是先置位再扣，二者间无失败点，等价。

## 5. 耦合分析

原版焊点：

1. **通关记账写在游戏主循环**（`0x140112820` 流程态 4 分支）。重构后会话层只发 `ScenarioCleared`，profile 订阅并写通关位、按本表加点 / 免费解锁；本系统只读 `Profile.cleared`。
2. **画廊条目号 = 剧本号**（`fn_140008c51` 用 `scenario` 直接当条目解锁）。重构用 `scenarios[].home_gallery_entry` 显式外键。
3. **免费解锁表写在函数栈上**（`aeScenarioPortrait`、`fn_1400089e5` 的两条硬编码）。重构进 `gallery.yaml`。
4. **sidecar 混了图鉴位图**（道具 / 遗迹）与选项开关。同一文件不等于同一系统：重构把 `savedata.dk4` 整体换成跨存档 `profile`（`12-ui-save`，DECISIONS D7），内部分 `profile.gallery`、`profile.encyclopedia`、`profile.options` 三节，各带节版本。
5. **简介文案在 mesfile、照片在 `Portrait.dk4`、进度在 sidecar**。三层不要再焊回一个 C++ 类。
6. **`CAcrossGameView` 曾被本篇误认作画廊**。它是小游戏；本篇与它无耦合。

允许的依赖：profile ← 订阅 `ScenarioCleared`（通关记账，读本表规则）；本系统 → 查询 `Profile.cleared`、`Characters.get`、`Narrative.scenario_exists`、`Ui.ask_yes_no`。禁止反向。

清档 / 换槽不得丢掉画廊。删 sidecar 才是「忘掉通关」。

## 6. 解耦后的配置

```yaml
# config/narrative/gallery.yaml
schema: dk4.narrative.gallery
version: 2

economy:
  bio_cost: 1
  photo_cost_per_photo: 1
  photo_cost_base: 1             # 原版价 = 张数 + 1
  points_on_first_clear: 4
  repeat_clear_policy: no_op     # no_op | add_again
  # points_cap 在 caps.yaml：gallery_points

store:
  section: profile.gallery       # 跨存档 profile 的一节；原版 savedata.dk4，256 字节

browse:
  host: show_portrait
  pages:
    - { key: companions, label_key: ui.gallery.page.companions }   # 伙伴篇：原版条目 [0,39)
    - { key: others,     label_key: ui.gallery.page.others }       # 其他篇：原版条目 [39,70)
  # 每个条目写 page: <key>；提取器按原版区间填，模组条目自选页
  enable_when: any_cleared       # fn_14000e1c9

# 打开画廊时补发的免费简介（fn_1400089e5）
# 下面整数是原版条目号（vanilla_index），提取器输出时换成条目 key；剧本用 key
open_unlocks:
  - { scenarios: [vanilla:raphael, vanilla:helmut, vanilla:lulu, vanilla:hua_mei], entries: [62] }
  - { scenarios: [vanilla:kyotaro, vanilla:woodin, vanilla:tia],                  entries: [37, 48, 55, 69] }

# 首通时免费简介（aeScenarioPortrait）；外加 home_gallery_entry
clear_unlocks:
  vanilla:raphael: [8, 11, 59, 63]
  vanilla:helmut:  [27, 61]
  vanilla:lulu:    [12]
  vanilla:hua_mei: [13, 58, 69]
  vanilla:kyotaro: [31, 38, 65, 66, 67, 68]
  vanilla:woodin:  [32, 33, 34, 35, 66]
  vanilla:tia:     [25, 36, 56, 57, 65, 67]

entries:                         # 70 条；提取器从 0x140552540 生成，下面是片段
  - { key: vanilla:gallery_raphael, vanilla_index: 0,  person: vanilla:raphael_castro, photos: 1, photo_base: 0,  default_bio_unlocked: false, bio_key: ui.gallery.bio.raphael }
  - { key: vanilla:gallery_lulu,    vanilla_index: 2,  person: vanilla:lulu_agate,     photos: 2, photo_base: 2,  default_bio_unlocked: false, bio_key: ui.gallery.bio.lulu }
  - { key: vanilla:gallery_p7,      vanilla_index: 7,  person: vanilla:person_7,       photos: 0,                 default_bio_unlocked: true }
  - { key: vanilla:gallery_fin,     vanilla_index: 57, person: vanilla:person_72,      photos: 1, photo_base: 12, default_bio_unlocked: false }
  - { key: vanilla:gallery_p213,    vanilla_index: 69, person: vanilla:person_213,     photos: 0,                 default_bio_unlocked: false }
```

```yaml
# config/narrative/scenarios.yaml 片段（剧本表只多一个外键）
scenarios:
  - { key: vanilla:raphael, home_gallery_entry: vanilla:gallery_raphael }
```

文案（只进 strings 包）：

```yaml
# config/ui/strings.zh-CN.yaml  片段
ui.gallery.confirm_bio: "如果在其他剧本中顺利过关，使用{bio_cost}点就能显示此人物的人物简介，要显示吗？（剩{points}点）"   # 原版消息号 0x7e
ui.gallery.confirm_extra_photo: "使用{photo_cost}点显示额外的照片吗？（剩{points}点）"   # 原版消息号 0x7f
ui.gallery.page.companions: "伙伴篇"
ui.gallery.page.others: "其他篇"
ui.gallery.bio.raphael: "葡萄牙中产阶级出身。是家中的次子……"   # mesfile 0x1ef2f；导入器以 exe 消息号表定 key，字节偏移只作抽取定位（DECISIONS D10）
```

跨存档状态（`profile` 的一节，不是槽存档，也不是内容包）：

```yaml
# profile.gallery  替代 savedata.dk4 +0x38 / +0x40…+0x56
schema: dk4.profile.gallery
version: 2
points: 0
cleared_scenarios: []            # 剧本 key，不按位下标；profile 在 ScenarioCleared 时写，画廊只读（§14-24）
unlocked:
  - { entry: vanilla:gallery_raphael, kind: bio }
  - { entry: vanilla:gallery_raphael, kind: extra_photo }
# profile.encyclopedia 另一节：items_seen / remains_seen（原版 +0x14 / +0x34，按 key）
# profile.options 另一节：原版 +0x3C / +0x3D bit7
```

导入原版 sidecar：校验魔数与 256 字节；按第 2 节偏移表转换，位下标经 `vanilla_index` 换成 key；`+0x3A` 与其余未知字节进 `original_unknown`。

加载顺序：caps → formulas → 人物 / 剧本表 → **本表**（外键 `person_id` / `scenario_id` / `home_gallery_entry` 必须已注册）→ 读 sidecar 覆盖状态。

## 7. 独立旋钮

改这些不应迫使贸易 / 战斗 / 航海 / 人物公式改代码：

- `bio_cost` / `photo_cost_*` / `points_on_first_clear` / `repeat_clear_policy`
- `clear_unlocks` / `open_unlocks` / `default_bio_unlocked`
- 某条目的照片张数、分页切法
- 给第 8 个剧本加 `clear_unlocks` 与 `home_gallery_entry`
- `enable_when`（任一通关 / 全通）

需要别人配合的只有：新 `person_id` 在 `persons.yaml`；新照片在显示包；新 `scenario_id` 在 `scenarios.yaml`。加载期外键校验，缺则拒绝启动。

## 8. 对外接口

订阅：

| 事件 | 来源 | 动作 |
| --- | --- | --- |
| `ScenarioCleared { scenario_id }` | 会话层（原版流程态 4） | 由 profile 订阅执行 `on ScenarioCleared`（本表规则）；本系统不直接订阅 |
| `Ui.GalleryOpen` | 标题 | `open_unlocks`，返回投影 |
| `Ui.GallerySpend { entry, kind }` | 确认框「是」 | `spend`；「否」no-op |

发出：

| 事件 | 载荷 | 谁听 |
| --- | --- | --- |
| `GalleryUnlocked` | `entry`, `person_id`, `kind` | UI |
| `GalleryPointsChanged` | `points` | UI |
| `GalleryClearRecorded` | `scenario_id` | 标题菜单使能（profile 通关记账完成后发） |

查询（LIVE，sidecar 读不到则当全 0，不抛进玩法）：

```
Gallery.points() -> int
Profile.cleared(scenario_id) -> bool        # save / profile 拥有（06）；本系统只读
Gallery.unlocked(entry, kind) -> bool
Gallery.can_offer(entry, kind) -> bool
Gallery.cost(entry, kind) -> int
Gallery.entry(entry) -> { person_id, bio_key, photos, photo_base } | none
Gallery.pages() -> [ {key, entries} ]
```

命令：

```
Gallery.spend(entry, kind) -> ok | no_op
Gallery.record_clear(scenario_id)            # 仅测试 / 导入器；玩法走 ScenarioCleared → profile
```

只传 ID 与纯值。禁止传 `CShowPortrait*`、`savedata` 文件句柄、`World*`。

## 9. 未核实字段

| 项 | 现状 | 禁止 |
| --- | --- | --- |
| sidecar `+0x3A` | 第二张剧本位图，读者 `fn_1400067ee` / `fn_140012d32`，写者未找到 | 不要当通关位 |
| sidecar `+0x3C/+0x3D` 选项的确切含义 | bit7 开关，[推断] 音频 | 归 `12-ui-save` |
| sidecar `+0x57…+0xFF` | 模板全 0，未见访问 | 进 `original_unknown` |
| 标题 case 5 `fn_1400032a1` | 同样先填通关位 | 不并进本表；语义 [未知] |
| 照片张序 `photo_base + k` 与 `Portrait.dk4` 贴图下标的映射 | `Portrait.dk4` G1T `+0x10 = 84`，`bustup.dk4 = 220` | 不要把 84 当照片张数 |
| `CShowPortrait` 按钮码 0–5（跳表 `0x14011a46c`） | 0 / 1 = 当前条目 −1 / +1，越过页界 `[-0x80, -0x7c]` 时回绕到另一端；3 = 买简介；4 = 买照片；2、5 只改显示状态 [推断] | 2、5 不当确认 |
| `fn_1400067ee` / `fn_14000551f`「七本全通」判定的用途 | 循环 `< 7` 全非 0 才成功 | 不要写成画廊使能 |
| `NUM_BUSTUP` | 符号 + 断言 | 不要用 G1T `+0x10` 填枚举 |
| 同伴长传记（`0x1fb04` 起）与条目的逐条对应 | 条目表只给 `person_id` | 提取器按 `person_id` 匹配，不按 mesfile 顺序 |

提取器：条目表、免费表、花费常量、sidecar 偏移按第 2 节直接生成；残余进 `unknown`。
