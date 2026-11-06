# 存档与会话

原版存档是一份带魔法数的 packed blob。重构存档按系统分节、带版本；原版文件只通过导入器进入新 schema。经验是成长源，等级/技能读档时重算。

## 1. 职责边界

本系统管：

- 槽位、路径、魔法数、版本、校验
- 原版 `savedatXX.dk4` → 新 schema 的导入
- 会话：当前语言、当前剧本、当前 `scene_id`、未提交的 UI 叠加
- 跨存档系统档 `profile`（原版 `savedata.dk4`）的**容器**：文件、版本、节分发。通关位归 profile 自己：订阅 `ScenarioCleared` 写位，对外只经 `Profile.cleared(scenario_id)` 查（词表 §14-24）。节内规则归各系统：`gallery`（点数、简介/照片解锁的规则，`10-narrative/05-gallery-unlock.md`；画廊只读 `Profile.cleared`）、`encyclopedia`（已发现道具/遗迹）、`options`
- 把各系统提交的状态快照写成一节，读回来再分发
- 存档头 `id_map`、启用包列表、随机流状态（D1/D7）

不管：

- 各节内部字段含义（各玩法文档）
- 公式原文（只存公式包版本，见 `05-config-architecture.md`）
- 文案（语言包 id）
- 自动备份策略以外的用户文件管理

禁止 UI 直接 poke 偏移。存档 UI（`CSelectSaveDataView`）只发 `SaveSlot` / `LoadSlot`。

## 2. 原版事实

路径与槽：

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 目录 `Documents\KoeiTecmo\DK4HD\Savedata\<lang>\` | `[确认]` | `save_format.md` `Save.cpp` `ReadMe_*.txt` |
| 四个 exe 写死的语言目录：sc `zh-CN`、tc `zh-TW`、jp **`ja`**、kr **`ko`** | `[确认]` | `strings` 四个 `DK4HD_*.exe` 均含 `\KoeiTecmo\DK4HD\Savedata\<dir>\`；与 `ReadMe_*.txt` 一致 |
| `save_format.md` / `Save.cpp` 写的 `ja-JP` `ko-KR` 不是 exe 用的目录；`dk4save.c` `langs[]` 两种都探只是容错 | `[确认]` | 同上；`dk4save.c:618` |
| SC exe 创建目录时写死 `\KoeiTecmo\DK4HD\Savedata\zh-CN\` | `[确认]` | `fn_140056790.cpp` |
| 槽文件名 `"%ssavedat%02d.dk4"` | `[确认]` | `fn_1401120a0` |
| 存/读列表扫描 `0x34` = 52 个文件（`savedat00`…`savedat51`），每个只读 `0x100` 头 | `[确认]` | `fn_14001300c`（body `0x140113640`）循环 `< 0x34`；RTC `asDataHeader` 13312 B = 52×256，`abExistFile` 208 B = 52×4 |
| 交给选择框的条数 `0x32` = 50；取消返回 `0x33` | `[确认]` | 同函数 `mov r8d,0x32` → `fn_1400139f8` → `fn_1400125da` 存入 `+0x200`；尾部 `mov eax,0x33`；调用方 `fn_14000d684` `== 0x33` 即放弃 |
| 选择框一屏 5 行（`min(count,5)`），默认选中 `+0x78` 时间戳最大的已存在槽 | `[确认]` | `CSelectSaveDataView::vfunc_02` `>= 5`；`fn_1400125da` 循环比 `+0x78` |
| `savedat50` / `savedat51` 被扫描但能否从选择框选到 | `[未知]` | 计数 50 与扫描 52 不等，未见其它调用 `fn_1401120a0` 的路径 |
| `save_format.md` / `dk4save.c` 的「`00`…`10` 共 11 槽」只是工具探测范围，不是原版上限 | `[确认]` | `dk4save.c:1194` 提示文案；对照上两行 |
| 另有 `savedata.dk4`：256 字节系统档（跨存档），见下「系统档」 | `[确认]` | `fn_140012c1f` 读 `0x100`；`fn_140002045` 写 `0x100` |

槽文件头（`0x100` 字节，`CSaveDataFile+0x18` 原样读写）：

| 偏移 | 内容 | 等级 | 来源 |
| --- | --- | --- | --- |
| `+0x00` | `"DAIKOUKAI4PKW9 1"` 16 字节；**校验只比前 14 字节** `DAIKOUKAI4PKW9`，`" 1"` 不检查 | `[确认]` | `fn_140121ca0` `i < 0xe` 才比较 |
| `+0x10` | `uint32` 存档格式版本，当前 0。严格读档要求等于 `.rdata 0x140556090`（= 头模板 `+0x10`，值 0） | `[确认]` | `fn_140121700`；按段换算（.rdata 文件偏移 = VA − `0x140001200`）重读 |
| `+0x14` | `uint32` = `0x100` + 正文字节数 = 文件大小 | `[确认]` | 写：`fn_140121860` `+0x2c = cnt + 0x100`；读校验：`fn_140121d80` |
| `+0x18` | `uint32` 校验和：正文每个字段**数值**之和，mod 2³²；文件头不计入。u8 助手**零扩展**；char 助手（姓名等字符串）**符号扩展**；u16 零扩展；u32 加整个值 | `[确认]` | 每个助手 `cnt += n; sum += 值`。u8：写 `fn_14000c978`（body `0x1400e9d30`，`movzx`）/ 读 `fn_14000c6da`；char：写 `fn_140006505` / 读 `fn_14000f32b`（`movsx`）；u16：`fn_14000a19b` / `fn_140004421`；u32：`fn_14000f240` / `fn_14000def9`（另有 `fn_14000e70a`/`fn_14000439f`）。收尾 `fn_140121860` 写 `+0x14`/`+0x18` |
| `+0x1C` | `uint32` 布尔，存时 = `CDk4Game+0x78 != 0`；读档成功后代码把它置 0 并回写头 | `[确认]` 读写；语义 `[推断]`「公正进度」标志（exe 消息号 `0x151`「现在一旦结束游戏，这个储存进度就会成为公正进度…」、`0x153` `0x154`，按 `.data 0x140683620`/`0x1406836c0` 消息号表解码）；模板值 1；回写用的是只读句柄（`fn_14000193d` `GENERIC_READ`），是否真落盘 `[未知]` | `fn_14000127b` → `fn_140012c56`；`fn_140014146` |
| `+0x20` | `uint32` 剧本 = 主角 id，0–6（样例 `savedat04` = 6 蒂雅） | `[确认]` | `fn_14000127b` 取 `CDk4Game+0x7c`；样例；`NUM_SCENARIO=7` |
| `+0x24` | `uint32` 事件脚本版本，严格读档要求 = `<lang>/EV<+0x20>.dk4` 偏移 2 的 `uint16` | `[确认]` | `fn_140121700` → `fn_1400c26a0` 查表 `0x140682858`，表由 `fn_14000af9c` 读 `SC/EV0..6.dk4` 头填入；样例 `+0x20=5,+0x24=0x105` 对 `SC/EV5.dk4`=`0x105` |
| `+0x28` | `uint32` 列表显示的「势力值」 | `[确认]` | 列表格式串 `"\n主人公∶%s·%s\n势力值∶%8s  %22s"` |
| `+0x2C` | `uint16` 区域 id；`< 7` 时列表显示区域名 | `[确认]` 读写；区域含义 `[推断]` | `fn_14001300c`；`fn_14000127b` 由港口或舰队坐标求得 |
| `+0x2E` | `uint16` 所在港 `town_id`；`0x62` = 在海上（模板默认）；列表只在 `< 0x61` 时显示港名 | `[确认]` | 同上；头模板 `0x140556080+0x2E = 0x62` |
| `+0x30` / `+0x44` / `+0x58` | 各 `char[20]`：名 / 中间名 / 姓（GBK）；列表显示「名·姓」 | `[确认]` | `fn_14000a0a1` 三次 `lstrcpyA`；样例 `华梅`/`玛莉亚`/`李` |
| `+0x6C` | `uint32` 存档次数（`CDk4Game+0x8c`，每存一次 +1） | `[确认]` | `fn_14000d684` |
| `+0x70` | `uint32` 主角总等级缓存 | `[确认]` | `dk4save.c`；`fn_14000127b` |
| `+0x78` | `int64` 保存时刻（Unix 秒，CRT `_time64`） | `[确认]` | `fn_140122e10(0)`；样例解出 2026-10-17…11-01 |
| `+0x74`、`+0x80`…`+0xFF` | 样例全 0 | `[未知]` | |

新档头先拷贝 `.rdata 0x140556080` 的 256 字节模板（`fn_140006aff`），再填上表字段。模板非零字段：魔法数、`+0x1C = 1`、`+0x24 = 0x100`、`+0x2C = 8`（≥ 7，列表不显示区域名）、`+0x2E = 0x62`；其余 0。`[确认]`（按段换算后重读 `DK4HD_sc.exe` 文件偏移 `0x554E80`）

正文（`+0x100` 起）：各 `CSaveObject` 逐字段调用 1/2/4 字节读写助手顺序写入，**无标签、无长度前缀**。每个助手带一个「起始版本」参数，`since > 头+0x10` 时跳过该字段；抽查调用点（`CPlayerFleetData` `0x1401df6d0`）全部传 0。`[确认]`

即：原版有字段级前向兼容的骨架，但当前只有版本 0，且严格读档要求版本**相等**。

读档校验链（`fn_140014146`）：

| 步骤 | 失败表现 | 等级 |
| --- | --- | --- |
| 列表阶段 `fn_140010c21`：只比魔法数前 14 字节（非严格） | 不列出该槽 | `[确认]` |
| 打开 `fn_140005538`：魔法数 + `+0x10==0` + `+0x24==EV 版本` | 魔法数错：先「不是「大航海时代Ⅳ 威力加强版套装 HD Version」的存盘资料。」；版本或 EV 版本不符：不弹前句。两者都返回 0，调用方再弹「很遗憾，储存进度损坏了。」并放弃 | `[确认]`（`0x140122249` 起） |
| 读完正文 `fn_140121d80`：`cnt+0x100 == +0x14` 且 `sum == +0x18` | 「所储存的进度有可能损坏了，还要继续吗？」，选是仍载入 | `[确认]` |
| 存档 `fn_140006aff` 打不开写句柄 | 「无法储存，有可能是只读文件。」 | `[确认]` |

旧版 `dk4save.c`（2026-11-06 前）改六维/经验后不重算 `+0x18`，游戏读档弹「可能损坏」确认框，选「是」仍载入，载入后再存一次校验和即恢复。`[确认]`（旧版 `write_save()` 只读 `+0x14` 作长度提示、原样 `fwrite`；现版按改动字段增量重算，见下节「dk4save 已知问题」；读档调用方 `fn_140014146` body `0x140112490`，`0x14011255e` 处校验失败调 `0x140007315(0x140551f18)` 弹「所储存的进度有可能损坏了，还要继续吗？」，返回 0 放弃、非 0 走成功路径 `0x1401125a2`）

四语言 EV 版本不同：SC `106 104 106 103 104 105 104`，TC/JP/KR `101 101 101 102 104 105 104`。所以即使放进同一目录，SC 剧本 0–3 的档在其它语言 exe 也会被严格校验拒绝。`[确认]`（`Game/*/EV*.dk4` 偏移 2）

人物段（正文前段，`CPersonElement` 串行化）：

成帧 `[确认]`：写 `CPersonElement::vfunc_01`（body `0x1401cc3e0`）/ 读 `vfunc_02`（body `0x1401cc2e0`）按 `ePerson` 0–234 顺序循环 `0xEB` = 235 次，每人调 `CPerson` vtable `+0x30`（写 `vfunc_06` body `0x1401cb480`）/ `+0x38`（读 `vfunc_07` body `0x1401caf70`），最后处理主角 `CPlayerPerson`（`elem+0x2c18`）。无标签、无长度前缀。每条写完 `+0x22` 后判 `id >= 0x57`（87）即结束，所以 **id < 87 每条 25 B，其余 148 条每条 2 B**。

| 区段 | 文件偏移 | 内容 | 等级 |
| --- | --- | --- | --- |
| 前一对象尾 | `0x2C00..0x2C02` | 不属人物段 | `[未知]` |
| 完整记录区 | `0x2C03 + 25·id`，id 0–86（到 `0x3481`） | 25 B 记录，见下表 | `[确认]` |
| 2B 区 | `0x3482 + 2·(id−87)`，id 87–234（到 `0x35A9`） | `+0` 隶属、`+1` `+0x22` 字节；六维、经验、HP、身心、装备**不进存档**，读档取世界表 | `[确认]` |
| 主角 `CPlayerPerson` | `0x35AA..0x3610`，103 B | 见下「主角记录」 | `[确认]` |

验算：`0x2C03 + 87×25 + 148×2 = 0x35AA`。11 个样例槽中，未被工具改过的 2B 区 148 个隶属字节全等于 World 初值 26（`0x1A`），`+0x22` 都是 0/1/2；帧整体挪 ±1 字节则 0 命中。`[确认]`

25 B 记录（id 0–86；主角记录前 25 B 同构）：

| 记录偏移 | 宽 | `CPerson` 字段 | 含义 | dk4save 行偏移 |
| --- | --- | --- | --- | --- |
| `+0x00` | u8 | `+0x0C` | 隶属势力（`0x1A` = 无） | `+0x03` |
| `+0x01` | u8 | `+0x22` | 只升不降字节，语义 `[未知]` | `+0x04` |
| `+0x02..+0x07` | 6×u8 | `+0x0F` | 六维（体力 敏捷 魅力 智力 精神 运气） | `+0x05` |
| `+0x08` | u32 | `+0x18` | 经验轨 0 | `+0x0B` |
| `+0x0C` | u32 | `+0x1C` | 经验轨 1 | `+0x0F` |
| `+0x10` | u16 | `+0x20` | 当前 HP | `+0x13` |
| `+0x12` | u8 | `+0x0D` | 身体态 | `+0x15` |
| `+0x13` | u8 | `+0x0E` | 精神态 | `+0x16` |
| `+0x14..+0x18` | 5×u8 | `+0x23` | 装备 5 槽，空 = `0xE8` | `+0x17..+0x18` 及下一行 `+0x00..+0x02` |

全表 `[确认]`（`vfunc_06` `0x1401cb4a8`…`0x1401cb5c6`；`vfunc_07` 同构，`cmp eax,0x57` 在 `0x1401cafcf`）。字段全走 u8 / u16 / u32 助手，校验和按零扩展计。

主角记录（`CPlayerPerson::vfunc_06` body `0x1401e0dc0`：先调 `CPerson` 写 25 B，再写下列字段）：

| 文件偏移 | 宽 | 对象字段 | 含义 | 等级 |
| --- | --- | --- | --- | --- |
| `0x35AA..0x35C2` | 25 B | — | 同上记录：`0x35AA` 隶属、`0x35AC` 六维、`0x35B2`/`0x35B6` 两轨经验、`0x35BA` HP、`0x35BC`/`0x35BD` 身心、`0x35BE..0x35C2` 装备 | `[确认]` |
| `0x35C3` | char[17] | `+0x30` | 名（GBK） | `[确认]` |
| `0x35D4` | char[17] | `+0x41` | 姓 | `[确认]` |
| `0x35E5` | char[17] | `+0x52` | 中间名（注意与头里「名/中/姓」顺序不同；样例 `华梅`/`李`/`玛莉亚`） | `[确认]` |
| `0x35F6` / `0x35F7` | u8 ×2 | `+0x63` / `+0x64` | 生日：月 / 日（0-based） | `[确认]`（`01-world/07` §2.5：时间触发月 `==12` 换成 `+0x63`、日 `==31` 换成 `+0x64`） |
| `0x35F8` | char[19] | `+0x65` | ? | `[未知]` |
| `0x360B` | u32 | `+0x78` | ? | `[未知]` |
| `0x360F` / `0x3610` | u8 ×2 | `+0x7C` / `+0x7D` | ? | `[未知]` |

姓名、`+0x65` 走 char 助手（符号扩展计入校验和）。名字块**只有主角这三段**；dk4save 的「名字块 `0x35C3` 步长 `0x2E9`、index 1..98 = 水手 id−1」在引擎串行化里没有对应，只有 index 0 是真的。水手显示名来自 World 表。

主角在 `0x35AA` 与记录[`+0x20`]（0–6，都 < 87）各有一份。样例多数一致，部分 `.bak` 不同步（原档如此）。导入以 `0x35AA` 为准。`[确认]` 两处并存；以谁为准 `[推断]`

dk4save 的「`0x2C00` 起 98 行 × `0x19`」是**工具常数**，不是引擎成帧：步长相同、起点差 3，字段偏移 `+3/+5/+0x0B/+0x0F` 正好抵消，所以 id 0–86 读写的是同一批字节；行 87–97 是错读（见下节）。`srcdata/source/Constants.h` 的 `SAVE_SAILOR_COUNT = 98` 也是从工具抄来的。

样例槽文件 107094–107466 字节，各不相同，与 `+0x14` 一致。人物段到 `0x3610` 为止（vanilla 235 人定长）`[确认]`；变长部分在其后 `[推断]`。

### dk4save 已知问题（2026-11-06 已修）

下面三条是旧版 `dhh4tools/dk4save.c` 的问题，已在 2026-11-06 修复：工具改用引擎成帧（`0x2C03` 起，每条 25 B，只开放 id 0–86），写盘时按改动字段的宽度把差值加到 `+0x18`（u8/u16 零扩展、u32 整值），改动落在未登记字段则拒绝写入；打开存档时检查 2B 区隶属越界并提示。导入器仍不要把旧版改过的档当作干净样本。

1. **行 87–97 错写 2B 区**。工具按 98 行 × `0x19` 访问，行 87–97 落在 2B 区：「隶属」读到的是别人的隶属或 `+0x22` 字节（只有行 87 碰巧读到本人隶属），「六维」「经验」写进 ids 88–219 的隶属 / `+0x22`（行 97 最远到 `0x358B`，碰不到主角记录）。显示上：「在队」判据 `sum(stat) > 0`，这 11 行恒显示在队；偶数行 88/90/92/94/96 的「隶属」其实是 `+0x22`（多为 0），主角是拉斐尔（势力 0）时显示「已招收」，批量修改（`on_batch`）会直接写进去。ids 87–97 本人的六维、经验不进存档，工具改不到他们本人。`[确认]`
2. **`savedat01.dk4` 有 70 字节实损**。对照 `.bak`，批量写入的行 88/90/92/94/96 改坏了 ids 101–107、126–132、151–157、176–182、201–207 共 35 个职能 / 事件 NPC 的隶属（15 个变 255、10 个变 196、10 个变 0 = 玩家势力），以及这些人和 100/125/150/175/200 的 `+0x22`（抬到 255/30/29）。游戏再存档会原样带回。`[确认]` 仓库副本已于 2026-11-06 按 `.bak` 恢复这 70 字节并增量修正 `+0x18`（恢复前文件留作 `savedat01.dk4.20261106-223939-779.bak`）；玩家机器上的原档需另行恢复。；后果（隶属 0 被当玩家方、255/196 越过势力 id 范围 0–25 去索引势力表、`+0x22` 只升不降无法自愈）`[推断]`
3. **不重算校验和**。只要改过正文，`+0x18` 必然失配，读档弹确认框（见上「读档校验链」），选「是」可继续。`[确认]`

导入原版档时：2B 区隶属不在 0–25 且不是 `0x1A` 的，记警告并按坏值处理，不猜原值。

系统档 `savedata.dk4`（256 字节，`CDk4Game` 外的全局对象 `0x144296ac0` 原样映射）：

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 读：只比魔法数前 14 字节且长度 `0x100`；不查版本/校验和 | `[确认]` | `fn_140012c1f` |
| 不存在：拷贝模板 `.rdata 0x140555f70`、写盘、再读 | `[确认]` | 同上 → `fn_140001b90` `fn_140002045` |
| 每次存档后重写；剧本通关（`CDk4Game+0x88==4`）后也重写（首通与否都写） | `[确认]` | `fn_14000d684`；`fn_14000e8a4` `0x140112a0c` |
| 模板 `.rdata 0x140555f70`（文件偏移 `0x554D70`，按段换算）：魔法数、`+0x10 = 0x100`、`+0x4E…+0x56` = 默认简介位（`80 c6 ff 77 80 ff 7e 50 01`），其余 0 | `[确认]` | 重读 exe；与两份样例 `+0x4E…` 逐字节相同 |
| `+0x10` = `0x100`（模板常量，恰等于文件长，代码未读它） | `[确认]` 值；含义 `[未知]` | 模板 |

布局（与 `10-narrative/05-gallery-unlock.md` 第 2 节同一张表，偏移由访问函数立即数钉死；本轮逐个反汇编复核）：

| 偏移 | 类型 | 含义 | 归属节 | 等级 / 来源 |
| --- | --- | --- | --- | --- |
| `+0x14` | bit[231]（29 字节，到 `+0x30`） | 已发现道具，bit = `eItem` | `encyclopedia` | `[确认]` `fn_140007b44`（body `0x140128e30`，`[rcx+rdx+0x14]` 置位）/ `fn_140008f3a`（读） |
| `+0x34` | bit[25]（4 字节） | 已发现遗迹，bit = `eRemains` | `encyclopedia` | `[确认]` `fn_1400134e4`（`0x1402aac00`）/ `fn_140011cc5` |
| `+0x38` | `uint16` 位图 | 剧本通关位，bit = `eScenario` | profile 自写（订阅 `ScenarioCleared`，§14-24）；落在 `gallery` 节的 `cleared_scenarios` 字段，画廊只读 | `[确认]` `fn_140010127`（`or`）/ `fn_1400050c4`（`and`） |
| `+0x3A` | `uint16` 位图 | 另一张剧本位图 | ? | `[确认]` 存在（读取函数 `fn_1400022b1`，调用点 `0x1401219b2` `0x140121ad2`）；写入者未找到；语义 `[未知]` |
| `+0x3C` / `+0x3D` | 各一字节，只用 bit7 | 两个开关：`fn_14000c121` / `fn_14000e8fe` 在参数 **为 0 时置** bit7、非 0 时清，即 bit7 = 关；值变了才通知 `0x14429d550`（声音管理器）。启动时从全局 `0x14429439c` / `0x1442943a0` 同步 | `options` | `[确认]` 极性与写入；对应「背景音乐 / 音效」`[推断]`；两份样例 `+0x3C = 0xC0`，bit6 写入者未找到 `[未知]` |
| `+0x40` | `uint16` | 画廊点数。首通 `+4`（`fn_14000d021(sidecar, 4)`，u16 加法无上限）；扣点 `fn_140008828`；读 `fn_1400146f5` | `gallery` | `[确认]`（旧稿「通关时写入剧本号」错：`dx = 4` 是加数） |
| `+0x42` | bit[70] | 额外照片已解锁，bit = 画廊条目号 | `gallery` | `[确认]` `fn_14000b2ad`（`0x14011f5b0`） |
| `+0x4E` | bit[70] | 简介已解锁，bit = 画廊条目号 | `gallery` | `[确认]` `fn_1400118d8`（`0x14011f670`） |

| 事实 | 等级 | 来源 |
| --- | --- | --- |
| 通关（`fn_14000e8a4` `0x1401129ae`）与存档（`fn_14000d684` `0x140112ba3`）时 `fn_14000a321`（RTC `eRemains,sItemOwner,eItem`）把当前已发现道具（`< 0xE7`）与遗迹（`< 0x19`）写进 `+0x14` / `+0x34`。即「博物馆」读的是系统档，不读槽 | `[确认]` | `0x1402ab044` `cmp 0xe7`、`0x1402ab141` `cmp 0x19` |
| 标题菜单「音乐厅 / 博物馆 / 摄影沙龙 / 照相馆」仅当 `+0x38` 任一位为 1 才可选 | `[确认]` | `fn_14000d783` 四次 `fn_14000e1c9` |
| 样例 `zh-CN`：`+0x14…+0x30` 道具位、`+0x34…+0x37` = `ed ff ff 01`（25 位中 23 位）、`+0x38 = 0`、`+0x40 = 0`；`zh-TW`：除 `+0x3C` 外与模板同 | `[确认]` | `Savedata/*/savedata.dk4` |

`CSaveDataFile`（`0x120`）：`+0x00` `CFile`（vtable + `+0x08` HANDLE，共 16 字节）；`+0x10` 已读写字节计数；`+0x14` 累计校验和；`+0x18` 头 `0x100`；`+0x118` 状态：3 空闲、2 已探头（列表）、0 已严格打开读、1 已打开写。`[确认]`（`fn_140010c21` `fn_140005538` `fn_140006aff` `fn_140121860`）

随机数：唯一玩法 RNG `fn_140005272(n)`（body `0x140140a70`）= `n > 0 ? rand() % n : 0`，首次调用 `srand(time(NULL))`，全 exe 只此一处播种；种子与 CRT 状态**不进存档**，读档不重播种，原版不可重现。`[确认]`（第一轮 00 组核查；`DECISIONS.md` D7）

对象模型：几乎所有模拟状态继承 `CSaveObject -> CObject`（人物、城镇、势力、舰队、事件、遗迹、文化、战斗结果…）。

编码：SC 姓名 GBK（代码页 936）。四语言存档目录分开，**不**把四种语言写进同一文件。

## 3. 实体与 ID

| ID | 含义 |
| --- | --- |
| `slot_id` | 整数槽号；vanilla 选择框 0–49（扫描到 51），上限进配置 |
| `profile` | 跨存档系统档（替代 `savedata.dk4`）：节 `gallery` / `encyclopedia` / `options` / `mod:<id>`。**不是**会话，也不是槽 |
| `section_key` | 节名：系统 key（`characters`、`world`…）或 `mod:<mod_id>` |
| `id_map` | 每个 ID 域：运行时整数 ↔ 命名空间 key（`vanilla:london`） |
| `rng_stream` | 命名随机流 key（`combat`、`ocean.happening`…），见 `05-config-architecture.md` `rng.yaml` |
| `save_schema_id` | `dk4.save` |
| `save_version` | 整数，破坏性变更 +1 |
| `section_version` | 每节自己的版本，节内迁移不牵动别节 |
| `content_pack_id` + `content_pack_version` | 如 `vanilla-world0@1`；对应原版 `+0x24` 的「EV 版本锁」 |
| `formula_pack_version` | 读档后用**当前**公式重算派生值 |
| `language_id` | `zh-CN` 等，见 `03-localization.md` |
| `scenario_id` | 剧本；原版 `+0x20` |

哨兵：空槽 = 文件不存在，不是写一圈 0。导入失败 = 拒绝读档，不 fallback 进残局。存档里所有空引用一律 `null`（`Option<Id>`）。原版哨兵（头 `+0x2E` `0x62` 在海上、人物 `0xEC`、货 `0x6A`、势力 `0x1A`…）只在导入/导出层 `ids/legacy_codec.yaml` 翻译成 `null`，节里不得出现。

## 4. 算法 / 公式

无玩法公式。持久化规则：

```
write:
  header { magic, schema, version, packs[{id,version}], id_map, language, scenario_id, stored_size, checksum }
  rng: { streams: { <stream_key>: { seed, state } } }   # 每条命名流的种子与当前状态都存；原版不存
  for sys in registered_sections:                       # 含 mod:<id>
    sys.export_state() -> section { v: section_version, data }   # 引用一律存运行时 int，由头 id_map 解释
  未知节（读入时保留的）原样写回
  派生值（等级、技能）不写，或写 cache 并标 dirty
  Profile.write()                 # 原版每存一次重写 savedata.dk4；之前 encyclopedia 节已订阅 SaveWriting 同步（原版 fn_14000a321）

read:
  校验 magic / size / checksum            # 坏了按原版语义：提示后允许继续
  校验 schema version → 跑迁移链 migrate[v→v+1]
  比对 packs：缺包 / 版本不同 → 按 id_map 重映射（见下），不直接拒绝
  分发 section；未知节原样保留（opaque），回写时不丢
  Rng.restore(rng.streams)        # 缺流（新模组加的流）按 rng.yaml 默认种子新建
  Progression.RebuildDerived(all persons)
  Scene.Enter(saved scene_id)     # 非法 scene 则进 title 并报错，不猜
```

迁移与模组扩展（与 `05-config-architecture.md`「与存档的关系」一致）：

```
# 节内实体引用存运行时整数；存档头带 id_map（每个 ID 域：key → 写档时 int）
# 读档时用当前注册表重映射，不假设两次运行整数相同
remap(section, save.id_map):
  for (domain, int_old) in section.refs:
    key = inverse(save.id_map[domain])[int_old]
    int_new = Registry.resolve(domain, key)
    if int_new is None:                # 模组被卸载 / 条目被删
      apply orphan_policy[domain]      # drop | replace_with:<key> | refuse_load
# 列表一律存「(id, 值)」对或按 id 的映射，不存「下标即 id」的定长数组
#   → 模组加人/加港/加商品，表变长不改任何偏移
# 模组自有状态进 sections["mod:<mod_id>"]，主程序不解析，模组缺席时原样保留
# 迁移按节注册：migrate[section][v→v+1]，纯函数 + 测试样本，不跳步
```

原版兼容：

```
import_original(path):
  校验魔法数前 14 字节（原版即如此）
  +0x10 != 0 → 拒绝（未知格式版本）
  +0x14 / +0x18 不符 → 警告（原版也只是提示）
  +0x24 != EV 版本 → 警告，不拒绝（新引擎不依赖 EV 字节流位置；叙事节兼容按 event_pack + state_schema_version 判，见 10-narrative/02）
  按已命名偏移抽出确认字段；+0x20 → scenario_id
  原版整数 → vanilla_index → key → 当前运行时 int；哨兵按 ids/legacy_codec.yaml → null
  RNG 按 rng.yaml 新建（原版无可导入状态）
  未命名字节进 original_unknown[]
  经验 → 人物状态；丢弃 +0x70 缓存，重算
```

导出回原版格式**不是**引擎职责；需要的话做独立工具，缺字段不得瞎填；必须重算 `+0x14` `+0x18`；任何非 vanilla key 或越过 `vanilla_count` 的 id 按 `legacy_codec.on_export_overflow: refuse` 拒绝导出。

饱和：导入时单轨经验若大到原版会显示 0，按 `overflow: saturate` 收进 `level_per_track`（见成长文档），并记警告。

## 5. 耦合分析

原版：一份 blob 里人物、主角体、世界、经济全按偏移挤在一起；人物段按 id 变长（id < 87 存 25 B，其余只存 2 B），外部工具按 `0x2C00` × 98 行定长去读，就把别人的字节写坏了（§2「dk4save 已知问题」），改一处漏一处就裂。技能不存却把等级缓存存了。正文无标签，字段顺序即 schema；整档用一个全局版本（`+0x10`）+ 一个内容版本（`+0x24` = 剧本 EV 版本）硬锁，任一不等即拒绝，没有迁移。头里还复制了一份显示用数据（姓名、势力值、位置），与正文冗余。

重构：一节一个系统。人物节存 `person_id`、六维、两轨经验、所属势力、熟练技能 key、HP、身体态、精神态（三者原版对 id < 87 与主角进存档，§2）、主角生日；技能值与等级是派生量，不存。原版没有名声与爵位，`progression` 节不留 `fame`/`titles` 占位。城镇发展度在 world/factions 节。UI 节只存 `scene_id` 与叠加栈。列表预览（原版头 `+0x28`…`+0x78`）放 `meta`，读档忽略。

公式不进存档，避免旧档锁死数值。跨存档数据（原版 `savedata.dk4`）单独 `profile`，不混进槽；原版一个 256 字节文件混了画廊、图鉴、选项三个系统，profile 按系统拆节。

## 6. 解耦后的配置 / 新 schema

运行时文件建议：

```
<user>/DK4HD/saves/<language_id>/slot-00.json   # 或等价带长度前缀的二进制
<user>/DK4HD/saves/profile.json                  # 替代 savedata.dk4；跨语言共用
```

JSON 便于对照；正式包可用同 schema 的二进制。两者必须能互转。

```yaml
# config/ui/save.yaml（schema 说明 + 旋钮，不是内容包）
schema: dk4.save
version: 1
magic: DK4HDSAVE1              # 新魔法数，与 DAIKOUKAI4PKW9 1 区分
slots: 50                      # vanilla 选择框条数；原版扫描 52
rows_per_page: 5
checksum: crc32                # 原版是字段值求和，弱
on_checksum_mismatch: ask      # 原版：提示后可继续
on_pack_mismatch: remap        # 原版：EV 版本不等即拒绝
orphan_policy:                 # 按 ID 域；取值 drop | replace_with:<key> | refuse_load（D1）
  default: refuse_load
  person: drop
  town: refuse_load             # 港口被删后舰队 / 人物无处落脚；要宽松可配 replace_with:<key>
  item: drop
legacy_codec: ids/legacy_codec.yaml   # 原版哨兵只在导入/导出层翻译
unknown_sections: keep         # 未注册节（含缺席模组的 mod:<id>）原样保留
rng_state: save                # save | reseed；原版等价于 reseed（time 播种、不存）
```

```json
{
  "schema": "dk4.save",
  "version": 1,
  "content_packs": [ { "id": "vanilla-core", "version": 1 }, { "id": "vanilla-world1", "version": 1 }, { "id": "example_mod", "version": 3 } ],
  "formula_pack_version": 1,
  "language_id": "zh-CN",
  "scenario_id": "vanilla:woodin",
  "saved_at": "2026-11-02T20:23:00Z",
  "id_map": {
    "person":   { "vanilla:abraham_woodin": 5, "vanilla:raphael_castro": 0 },
    "town":     { "vanilla:basra": 38 },
    "force":    { "vanilla:woodin": 13 },
    "skill":    { "vanilla:helmsmanship": 2, "vanilla:accounting": 4, "vanilla:gunnery": 6 },
    "scenario": { "vanilla:woodin": 5 }
  },
  "rng": {
    "streams": {
      "combat":           { "seed": 12345, "state": "0x3a9f01c2" },
      "ocean.happening":  { "seed": 12346, "state": "0x00c0ffee" },
      "economy.market":   { "seed": 12347, "state": "0x7e11a5d0" }
    }
  },
  "meta": {
    "display_name": "亚伯拉罕·伍丁",
    "force_value": 669,
    "location": { "town": 38, "area": 3 },
    "save_count": 4,
    "total_level_cache": 120
  },
  "session": {
    "scene_id": "ocean",
    "town": null,
    "fleet": 0,
    "overlays": []
  },
  "sections": {
    "characters": {
      "v": 1,
      "protagonist": 5,
      "persons": {
        "5": {
          "force": 13,
          "abilities": { "vanilla:stamina": 78, "vanilla:agility": 85, "vanilla:charm": 60, "vanilla:intelligence": 55, "vanilla:spirit": 81, "vanilla:luck": 60 },
          "exp": { "vanilla:track_a": 54000, "vanilla:track_b": 30000 },
          "good_skills": [2, 4, 6],
          "hp": 96,
          "physical_state": "vanilla:healthy",
          "mental_state": "vanilla:normal",
          "birthday": { "month": 3, "day": 14 }
        }
      }
    },
    "world": { "v": 1 },
    "narrative": { "v": 1, "event_pack": "vanilla-ev5", "state_schema_version": 1, "flags": {}, "trigger_state": {} },
    "mod:example_mod": { "v": 3 }
  },
  "original_unknown": null
}
```

约定（D1/D7）：

- 节内一切实体引用（`persons` 的键、`force`、`good_skills`、`town`…）都是**本档写入时**的运行时整数，含义只由头 `id_map` 给出；读档先按 key 重映射成当前整数，再交给系统。`id_map` 只列本档实际引用到的条目。
- 列表一律按 id 键的映射或 `(id, 值)` 对，**不存「下标即 id」的定长数组**；模组加人/加港/加商品，表变长不动任何节。
- 空引用写 `null`，不写原版哨兵。
- `abilities` / `exp` 按 key 存，不按维下标；六维上限 100（`ability_cap`），技能值、等级是派生量，不存（`total_level_cache` 只在 `meta`）。
- `hp`、`physical_state`、`mental_state` 原版对 id < 87 与主角进存档（记录 `+0x10` u16、`+0x12`、`+0x13`，即 `CPerson+0x20`、`+0x0D`、`+0x0E`），重构对所有人照存；状态用 key。
- `mod:<mod_id>` 节由模组 `register_section` 注册；模组缺席时主程序不解析、原样保留、回写不丢。
- `rng.streams` 每条命名流存种子与当前状态，读档后续抽结果与不读档一致（原版做不到）。

`display_name` 按原版列表「名·姓」拼（格式串 `主人公∶%s·%s` 取头 `+0x30` 与 `+0x58`）。空节表示该系统尚未还原字段时仍占位。JSON 无偏移问题；二进制实现用 TLV / 长度前缀，禁止再按原版人物段偏移写死。示例 key、数值只是形状示意，实际 key 由各内容表给。剧本 5 属 `vanilla-world1`（`scenario_id >= 4`）。

`meta` 可选；读档忽略，只给存档列表 UI 当预览。列表 UI 也可以现场算。

`profile.json`（跨存档；节归属见 §2「系统档」布局表最后一列）：

```json
{
  "schema": "dk4.profile",
  "version": 1,
  "id_map": {
    "scenario": { "vanilla:woodin": 5 },
    "item":     { "vanilla:pumpkin_seed": 0 },
    "remains":  { "vanilla:stonehenge": 0 },
    "gallery_entry": { "vanilla:gallery_raphael": 0 }
  },
  "sections": {
    "gallery":      { "v": 2, "cleared_scenarios": [5], "points": 4, "unlocked": [ { "entry": 0, "kind": "bio" } ] },
    "encyclopedia": { "v": 1, "items_seen": [0], "remains_seen": [0] },
    "options":      { "v": 1, "bgm": true, "se": true },
    "mod:example_mod": { "v": 1 }
  },
  "original_unknown": null
}
```

`gallery` 节字段由 `10-narrative/05-gallery-unlock.md` 定义（原版 `+0x38` `+0x40` `+0x42` `+0x4E`），本篇只管容器；其中 `cleared_scenarios`（`+0x38`）由 profile 在 `ScenarioCleared` 时写，画廊只读 `Profile.cleared`。原版 `+0x3A` 语义 `[未知]`，暂进 `original_unknown`，钉死归属后再开字段。

导入器映射（原版 → 新）。所有哨兵经 `ids/legacy_codec.yaml` 转 `null`；所有原版下标经 `vanilla_index` 转 key 后再写 `id_map`：

| 原版 | 新字段 |
| --- | --- |
| `+0x20` | `scenario_id`；同时定主角 `person` |
| `+0x24` | 只记日志（对应哪个 EV 版本） |
| `+0x28` `+0x2C` `+0x2E` `+0x6C` `+0x78` | `meta.force_value` `meta.location`（`+0x2E` 的 `0x62` → `null`）`meta.save_count` `saved_at` |
| `+0x1C` | `session.fair_mode`（语义 `[推断]`） |
| `+0x30/+0x44/+0x58` | 仅校验与主角姓名 `0x35C3` / `0x35E5` / `0x35D4` 一致；不单独存 |
| 人物段 25 B 记录（id 0–86，`0x2C03 + 25·id`）与主角记录 `0x35AA` 前 25 B（§2） | `characters.persons{}`：隶属 → `force`（`0x1A` → `null`）、六维、两轨经验；主角以 `0x35AA` 为准 |
| 人物段 2B 区（id 87–234，`0x3482 + 2·(id−87)`） | 只有 `force`；其余字段取世界表初值 |
| 记录 `+0x01`（`CPerson+0x22`，语义 `[未知]`） | `original_unknown` |
| 记录 `+0x14..+0x18` 装备 5 槽 | 人物装备（`0xE8` → `null`）；字段名归 `09-items` |
| `+0x70` | 丢弃或写入 cache |
| 主角姓名 `0x35C3` / `0x35D4` / `0x35E5`（名 / 姓 / 中间名） | `characters.persons[prot].name` |
| 主角生日 `0x35F6` / `0x35F7`（月 / 日，0-based） | `characters.persons[prot].birthday` |
| `CWorldData` 段首 u32 = 日历打包时刻（`01-world/07` §2.2；文件绝对偏移未算） | `Clock` 的 `packed`（core） |
| 主角 `0x35F8..0x3610`（`+0x65` / `+0x78` / `+0x7C` / `+0x7D`） | `original_unknown` |
| 人物 HP / 身体态 / 精神态（记录 `+0x10` / `+0x12` / `+0x13`，即 `CPerson+0x20` / `+0x0D` / `+0x0E`；仅 id < 87 与主角） | `characters.persons[*].hp` / `physical_state` / `mental_state`（整数经 legacy_codec 转 key） |
| RNG | 原版不存；导入时按 `rng.yaml` 新建各流 |
| `savedata.dk4` `+0x38` | `profile.gallery.cleared_scenarios`（profile 写，`Profile.cleared` 查） |
| `savedata.dk4` `+0x40` `+0x42` `+0x4E` | `profile.gallery`（规则见 `10-narrative/05`） |
| `savedata.dk4` `+0x14` / `+0x34` | `profile.encyclopedia.items_seen` / `remains_seen`（bit = 原版 `eItem` / `eRemains`，经 `vanilla_index` 转 key） |
| `savedata.dk4` `+0x3C` / `+0x3D` bit7 | `profile.options`（bit7 = 关；哪个是 BGM `[推断]`） |
| `savedata.dk4` `+0x3A`、`+0x3C` bit6、`+0x57…+0xFF` | `original_unknown` |
| 其余字节 | `original_unknown` hex |

vanilla 导入后 `formula_pack_version` 设为当前包，不设为「存档当时 exe」。

## 7. 独立旋钮

| 旋钮 | 说明 |
| --- | --- |
| `slots` / `rows_per_page` | vanilla 50 / 5；加大不必改人物系统 |
| 路径模板 | `{docs}/KoeiTecmo/DK4HD/Savedata/{save_dir}/` 仅作导入默认（`save_dir` 见语言包：`zh-CN` `zh-TW` `ja` `ko`）；新档可用新根 |
| `save_version` / 每节 `v` | 加字段只升节版本；拆节升主版本；迁移函数按节注册 |
| `on_pack_mismatch` / `orphan_policy` | 模组增删后的读档行为，按 ID 域分别配 |
| `unknown_sections` | 未注册节（缺席模组）保留或丢弃 |
| `rng_state` | `save`：流状态进存档，可重现；`reseed`：原版行为 |
| `legacy_codec` | 原版哨兵与字段宽度表路径，只影响导入/导出 |
| `checksum` / `on_checksum_mismatch` | 算法与坏档处理 |
| 是否写 cache | 可关 |
| `original_unknown` 保留 | 关则导入丢未命名字节（不可逆） |

显示名预览宽度由 UI 自己截，不倒逼姓名字节 17 / 20。

## 8. 对外接口

```
Save.list(language_id) -> [SlotInfo]
Save.write(slot_id)
Save.read(slot_id) -> Result
Save.import_original(path) -> Result
Save.delete(slot_id)
Save.register_section(name, version, export, import, migrations[])   # 模组也走这里
Profile.read() / Profile.write()                 # 容器；原版每存档、每通关重写
Profile.cleared(scenario_id) -> bool             # 通关位唯一查询；画廊 / 标题额外菜单只读它
Profile.set_cleared(scenario_id)                 # 仅 profile 的 ScenarioCleared 订阅与导入器调用
Profile.register_section(name, version, export, import, migrations[])   # gallery / encyclopedia / options / mod:<id>
Rng.export() / Rng.restore(streams)
Session.get() -> { language_id, scenario_id, scene_id }
Session.set_language(language_id)     # 不改玩法状态
```

`SlotInfo`：`slot_id`、是否空、`display_name`、`scenario_id`、`total_level`（派生）、`force_value`、`location`、时间、`pack_mismatch`。

事件：`SaveWriting`（写档前，profile 各节可同步）`SaveWritten` `SaveLoaded` `SaveRejected` `SaveMigrated`。`ScenarioCleared { scenario_id }` 由会话层发（`01-scenes-flow.md`）；profile 订阅：写通关位、按 `gallery.yaml` 规则首通加点 / 免费解锁、同步 encyclopedia 节，再写盘（词表 §14-24）。画廊不订阅、不自写通关位，只读 `Profile.cleared`。

各系统实现 `export_state` / `import_state`，参数为 ID 与 POD，禁止塞 `CSaveObject*`。

## 9. 未核实字段

- 槽文件头 `+0x74`、`+0x80`…`+0xFF`（样例全 0）
- `+0x1C` 是否就是「公正进度」，以及读档回写在只读句柄上是否生效
- `+0x2C` 区域 id 的 7 个取值对应什么（`fn_140004403` 取名）
- `savedat50` / `savedat51` 的用途（扫描 52、可选 50）
- 正文 `0x100` 起到人物段之前对应哪些 `CSaveObject`
- 人物段：`0x2C00..0x2C02` 属哪个对象；记录 `+0x01`（`CPerson+0x22`）语义；主角 `+0x65` char[19]、`+0x78` u32、`+0x7C` / `+0x7D`
- `savedata.dk4` `+0x3A` 位图语义与写入者、`+0x3C/+0x3D` 哪个是 BGM 哪个是音效、`+0x3C` bit6（样例 `0xC0`）
- 存档是否含当前 `scene_id` / 坐标；日期在 `CWorldData` 段首 u32（`01-world/07`），文件绝对偏移未算
- `CSaveObject` 序列化顺序与块边界；有无 `since > 0` 的字段（抽查均为 0）
