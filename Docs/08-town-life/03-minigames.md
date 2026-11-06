# 小游戏

八个原版 `*GameView` / `*GameManager`：七个由事件脚本同一条指令按编号 0–6 启动，捉老鼠单独走航海。规则、时限、棋子数、标准解全部进 `config/town/minigames.yaml`。场景代码只解释表，不写「5 枚硬币」「30 秒」「答案 4,7,6,…」。

原版小游戏只向调用方回一个胜/负布尔值，赌金、奖品、台词全在事件脚本里（见 2.0）。本系统不改船耐久、市价、六维；赢了发 `MinigameWon`，由调用方（narrative 脚本 / ocean）兑现。

## 1. 职责边界

本系统管：

- 小游戏目录（稳定 `minigame_id`）
- 规则参数：棋子数、时限、桶数、标准解、可移动约束
- 从哪类设施/事件启动（触发器 key）
- 胜负判定
- 结果（胜/负、步数等）回传给调用方

本系统不管：

- 赌注、奖金、胜负后的对白与旗标（narrative 事件脚本；原版就是如此，见 2.0）

- 老鼠出现在航海中的遭遇率（ocean）
- 酒馆是否开门（设施）
- 赌金从哪笔现金扣（economy 执行 `grantGold`）
- 教会休息恢复身体/精神（characters，与旅馆同一出口）
- UI 布局像素（`C*GameView` ctor 里大量坐标，属 ui）

原版八个实现类（「脚本号」见 2.0）：

| key | 类 | cpp | 脚本号 |
| --- | --- | --- | --- |
| `coin` | `CCoinGameView` + `CCoinSprite` | `game/CCoinGameView.cpp` | 0 |
| `nim` | `CNimGameManager` + `CCoinView` | `game/CNimGameManager.cpp` | 1 |
| `across` | `CAcrossGameView` + `CAcrossGameCharacterPushButton` | `game/CAcrossGameView.cpp` | 2 |
| `water` | `CWaterGameView` + `CBacketSpritePushButton` | `game/CWaterGameView.cpp` | 3 |
| `catch` | `CCatchGameView` + `CDollSprite` + `CMoveTergetButton` | `game/CCatchGameView.cpp` | 4 |
| `church` | `CChurchGameView` + `CSelectPushButton` | `town/CChurchGameView.cpp` | 5 |
| `stone_wall` | `CStoneWallGameView` + `CStonePushButton` | `game/CStoneWallGameView.cpp` | 6 |
| `rat` | `CRatGameView` | `game/CRatGameView.cpp` | —（航海） |

`CAcrossGameView` 规则未还原，见第 9 节。酒吧划拳只有文案，没有独立 View 类。

## 2. 原版事实

### 2.0 启动入口：事件脚本 class 6 / sub 1

`[确认]` 七个小游戏的启动函数各只有一个调用点，全部在事件脚本 class 6 处理函数 `fn_14000847c`（body `0x1400e1970`）的 sub 1 分支（外层跳表 `0x1400e1efc` 第 1 项 → `0x1400e1a06`，脚本指令高字节 `0xC1`）。内层 `fn_140005c13` 取指令字 bits 0–7 作游戏号，`cmp 6 / ja`，跳表 `0x1400e1f18`：

| 号 | 启动函数 thunk / body | 构造 | 对象大小 |
| --- | --- | --- | --- |
| 0 | `fn_140013958` / `0x140079540` | `CCoinGameView` | 0x288 |
| 1 | `fn_140006069` / `0x140182af0` | `CNimGameManager`（栈上 RTC `cNimGame`） | 0x298 |
| 2 | `fn_14000d0df` / `0x14002c9d0` | `CAcrossGameView` | 0x210 |
| 3 | `fn_14001439e` / `0x1402a5710` | `CWaterGameView` | 0x298 |
| 4 | `fn_140007f22` / `0x14006add0` | `CCatchGameView` | 0x230 |
| 5 | `fn_14000dd32` / `0x140076a70` | `CChurchGameView` | 0x358 |
| 6 | `fn_140011144` / `0x140245e50` | `CStoneWallGameView` | 0x1f0 |

来源：`Game/DK4HD_sc.exe` objdump `0x1400e1970–0x1400e1a9b`；跳表字节；`catalog/functions.tsv`；全 exe 扫 `call rel32` 只命中上述调用点。

`[确认]` 返回值处理：启动函数返回模态结果 → `fn_14000f86c`（`result != 0 ? 1 : 0`）→ `fn_140009acf` 写回脚本谓词。游戏号 > 6 时不启动，谓词默认 1。

`[推断]` 因此原版**没有**任何设施按钮或广场人群代码直接 new 这些 View；酒馆对赌、教会仪式、「我所有的钱」等都是 Sc 脚本内容，赌金/奖励由脚本按布尔结果分支处理。哪几个剧本用了哪个号，需等 Sc 指令长度表闭合后统计（`10-narrative/03-event-commands.md`）。

### 2.1 硬币 `coin`

`[确认]` mesfile `0x192d7`–`0x196a3`：

- 正反面交错的两种硬币，要把相同面排在一起
- 「硬币共有５枚」
- 「正面３枚，反面２枚」为完成形
- 只能移动**相邻的两种不同**硬币（不能移两枚同面）
- 把这两枚原样挪到其左或右「相邻的另一侧」；侧面没有邻居则该方向不可走
- 「不选择硬币，游戏无法进行。」`0x1516f`
- 对赌：「做到了你就赢了，如果做不到我就赢了。」「赌金？」「我所有的钱。」
- NPC 宣称最少 10 步；玩家 9 步解开会触发「没想到居然超过了我发现的最少步数」

`[确认]` `CCoinGameView` 在 `+0x150` 起 5 个硬币指针，按位置排序（`vfunc_02` 循环 `>=5` 释放）。点击处理 `fn_140004e5d`（body `0x14007b760`，RTC `apMoveCoinSprite`）：点击号 4 → `fn_14000ef4d` 重置，5 → 关闭；`<4`、6/7、8 是选对/放置/取消三类分支，6 与 7 位移符号相反；落子分支内步数计数器 +1。`[推断]` 0–3 = 选以第 i 枚开头的相邻一对，6/7 = 往左/右放。

`[确认]` 胜负判定 `fn_14000a5f6`（body `0x14007e240`）：`face[0]==face[1]` 且 `face[3]==face[4]` 且 5 枚位置连续（`pos[i]==pos[0]+i`）。由 3 正 2 反推得：HHHTT 与 TTHHH **两种朝向都算赢**。每帧 `fn_14001455b` 判定成立即以结果 1 关闭。

`[确认]` 越界：`fn_140001893` 要求最左币位置 `> -5` 且 `< 9`，否则提示消息 `0xdaa` 并重置。

`[确认]` 赌注只在文案里；cpp 只回布尔（2.0）。「我所有的钱」如何兑现属脚本。`[未知]` 哪个剧本、哪个地点调用号 0。

参数默认：`coin_count=5`，`heads=3`，`tails=2`，`npc_min_steps=10`（仅台词），`pos_min_exclusive=-5`，`pos_max_exclusive=9`。

### 2.2 捉老鼠 `rat`

`[确认]` `0x3507`「现在开始捉老鼠游戏。请在３０秒之内捉住各船舱出现的老鼠。」

航海催促 `0x126c3`「提督，捉老鼠吧！」失败必须靠港 `0x12747`–`0x1293f`。成功「捉住了所有老鼠」后继续航海 `0x1293f`。

`[确认]` `CRatGameView` ctor（`CRatGameView.cpp`）：循环 `>= 0x14`（20）清 `+0x88` 起 20 个老鼠指针，并把 `+0x128` 起 20 个 dword 写 `0x20`。`+0x128[i]` 是第 i 只老鼠所在舱室，`0x20`(32) 为空哨兵（`NUM_SHIPCABIN=31`，`ShipCabinID.h`；不是 `INVALID_SHIPCABIN=-1`）。

`[确认]` `vfunc_13`（body `0x1401edfe0`）接受点击号 `0x37 <= id < 0x56`，`cabin = id - 0x37`（反汇编 `sub rax,0x37`），即 31 个舱室热点；命中则该槽写回 `0x20`，剩余 `+0x17c` 减 1。

`[确认]` 老鼠只数 `fn_140011031` → `fn_1401ee1b0`：`max(1, n / 4)`（`sar 2`，向零取整），`n` = `fn_1401ee2f0` 数 31 个舱室里返回值落在 `[0, 0x15)` 的个数。`[推断]` `n` 是当前船的有效舱室数；`0x15` 的语义未还原。场景挂 `CDeckShipView`（`fn_1400aafd0`）。

`[确认]` 流程 `fn_140013a07`：开场倒数 3（`fn_14000e534`，先等 `0x1f4`=500，再每 `0x3e8`=1000 一拍）；主循环 `fn_14000db89`：`s158` 从 0 到 `0x1E`，每过 `0x3e8` 才 +1，且剩余 `+0x17c > 0`；屏上显示 `0x1E - s158`。单位是毫秒，所以 30 是**秒**，与文案一致。老鼠到点会挪到相邻舱室（`fn_140008b39`，RTC `aeShipCabin` 4 个邻格）。返回值 = `+0x17c == 0`。

奖励：文案没有「得到金币」，成功=继续航行，失败=必须入港。`[确认]` 无金币句；`[未知]` 是否另改疲劳/粮食。

触发：航海遭遇，不是港口设施。启动函数 `fn_1400132f0`（body `0x1401ee910`）全 exe 无 `call rel32` 引用，`[未知]` 调用方（可能经函数指针）。仍放本目录，因为实现类与规则表属于 town-life 的 minigame 包。

### 2.3 倒水 `water`

`[确认]` `CWaterGameView` ctor 清 3 个 `+0x178` 指针。`CBacketSpritePushButton`，RTC `asBacketData`（96 字节）、`rcWater` `rcBacket`。

`[确认]` 三桶初值（`fn_1400104e7` body `0x1402a2210`，RTC `asBacketData` 3×32 字节，每项 `{id, amount, capacity}`）：

| 桶 | 初始量 | 容量 |
| --- | --- | --- |
| 0 | `0x64`=100 | `0x64`=100 |
| 1 | 0 | `0x46`=70 |
| 2 | 0 | `0x1e`=30 |

`[确认]` 胜负 `fn_140010474`（body `0x1402a49a0`）：`amount[2]==0` 且 `amount[0]==amount[1]`，即 50/50/0。倒水 `fn_14001325f`：`min(src.amount, dst 余量)`。

`[确认]` `fn_140008607`（RTC `sAnswerTable` 144 字节 = 9 对 src/dst）是**提示演示**，不是计时：标准解前 9 步为 `0→2, 2→1, 0→2, 2→1, 0→2, 2→1, 1→0, 2→1, 0→2`（演完为 50/20/30，玩家再倒 `2→1` 即胜）。`fn_140007f6d`（body `0x1402a54f0`）按 `+0x198` 提示次数依次演 5 / 7 / 9 步（消息 `0xdac–0xdb1`）。每演一步 `+0x19c` 计步 +1，`+0x160 = 0x1E` 是动画等待帧，倒数到 -1 再走下一步。

`[确认]` 玩家点击 `fn_14000d283`：成功倒水时计步 +1，`+0x160 = 0x15`（21 帧）后由 `fn_1400023dd` 判胜。胜利时若计步 `< 0xa`（10）额外出消息 `0xdb5/0xdb6`。`[推断]` 最优 9 步的夸奖。

`[确认]` 无倒计时失败分支；原 `+0x160=0x1E`「30 倒计时」是误读。

胜负奖励由脚本处理（2.0）；mesfile 无独立「倒水」句。

### 2.4 石墙 `stone_wall`

`[确认]` `CStonePushButton` + `CStoneWallGameView`。`fn_14000964c` RTC `alRightAnswer` 36 字节，写入：

```
4, 7, 6, 4, 0, 3, 1, 5, 2
```

共 9 步；`fn_14000964c` 是 `CStoneWallGameView::vfunc_13` 的跳转目标。点击号直接与 `alRightAnswer[progress]` 比较：`progress >= 9` 或不等 → `fn_140002dd3(..,3)` 失败并关闭；相等则 `progress+1`，到 `==9` → `fn_140002dd3(..,1)` 成功并关闭。点击 `== 0x14` 直接关闭、不调 `fn_140002dd3`，`[推断]` 退出按钮。**一步点错即失败**，无重试计数。

`[确认]` 这是**写死在函数里的标准解**，重构必须进 YAML。按钮个数 `[推断]` ≥8（答案含 0–7，4 出现两次）。

奖励由脚本处理（2.0，号 6）。

### 2.5 尼姆 `nim`

`[确认]` `CNimGameManager` ctor 循环 `>= 0x11`（17）个 `+0x110` 槽，是棋子数**上限**。RTC `cNimGame`、`rcCoin`、`posCoin`。

`[确认]` 本局棋子数 `+0x108 = fn_140183450() = rand(8) + 10`，即 10–17 随机（反汇编 `0x14018346a call fn_140005272(8); add eax,0xa`；`fn_140005272(n)=rand()%n`）。

`[确认]` 先手：`fn_1401833f0` → `fn_140182da0(0x32)` = `rand(100) < 50`，各 50%。台词是 exe 内嵌 GBK 串「决定由谁先拿。」「该你先。」/「由我先。」，不在 mesfile。

`[确认]` 每回合取 1–3 枚：玩家点击 `fn_14000281a` 已选 `>= 3` 时不再加选；确认键点击号 `0x12` 要求已选数 `1..3`，否则提示 `0xb2a`。

`[确认]` 胜负（`fn_140006069` 主循环）：一方取完后剩余 `> 1` 换手；剩 1 → 刚取的一方赢；剩 0 → 刚取的一方输。即 **misère：拿最后一枚者输**。返回 1 = 玩家赢。

`[确认]` NPC 策略 `fn_14001479a`：设剩余 `n`：`n % 4 == 1` → 取 `rand(3)+1`；否则 `n <= 4` → 取 `n-1`；否则 `r = n % 4`，`r == 0` 取 3，否则取 `r-1`。即把对手留在 `≡1 (mod 4)` 的标准必胜策略。

赌金由脚本处理（2.0，号 1）。

### 2.6 捕捉 `catch`

`[确认]` 邻近 RTC 名 `ptChiken` `ptMoveChiken` `alMove`（`fn_14000d76a`）。`CCatchGameView` ctor：两组指针各 2 个（`+0x118`、`+0x128`），布局常量 `+0xb0=0x13E` 等。`CDollSprite`、`CMoveTergetButton`。

`[确认]` 回合制棋盘：`+0x118` 两枚玩家棋子、`+0x128` 两只鸡，按 0–3 轮转（`fn_1400057a9` = `vfunc_04`）：号 `<2` 轮到玩家（`fn_140007338` 摆出移动按钮），号 `>=2` 由 AI `fn_14000d76a` 走，AI 每步前等 `0xb` 帧。初始化 `fn_1400033a0` 建 4 个 `CMoveTergetButton`（4 方向）。

`[确认]` 坐标 0–7（`fn_14006b7c0` 用 `7 - x` 做透视、8 行 y 表 `0x95…0x196`），即 8×8 棋盘。鸡 AI 按自身在 `x<4 / x>=4`、`y` 与 `x`/`7-x` 的关系选 4 方向优先序 `alMove`，依序试走；`fn_14000b69a` 返回 0 立即走，返回 1 记为备选，全无 0 时走第一个备选。

`[确认]` 四个棋子都 `fn_14000a6f0` 为真时，等 `0x50` 帧，然后以结果 1 关闭。tick 里未见回合上限或计时比较。`[推断]` 胜 = 两只鸡都被围死；负只能来自退出。

奖励由脚本处理（2.0，号 4）。mesfile 无「鸡」的对应句。

### 2.7 教会 `church`

教会设施见 `01-facilities.md`。`CChurchGameView` 不是设施类：它唯一的构造点是脚本号 5 的启动函数（2.0）。

`[确认]` `CChurchGameView` ctor 体积 0x7E8，写 `+0xb0=5`、`+0xb4=0x11`(17)，大量按钮坐标。`CSelectPushButton`。初始化 `fn_1400066cc`（body `0x140076cf0`，由启动函数调用）RTC `alIdentify`（136 字节 = 17 项）、`rcMaru`，资源 `InfoBox2.dk4` `EvItem.dk4`。

`[确认]` `alIdentify` 17 项依次为 `0,1,0,1,1,0,1,1,0,1,1,1,1,1,1,0,1`：0 有 5 个（下标 0/2/5/8/15），与 `+0xb0=5` 一致。`rcMaru`（○）按 `+0xb0` 画 5 次。

`[确认]` 点击 `fn_1400028d3`：选/取消一个热点，按其 `alIdentify` 值分别增减两个计数（`+0x258` 对、`+0x25c` 错）。`0x14` 提交 → `fn_140007ced`：`+0x258 == 5 && +0x25c == 0` 走成功分支（音 `0xdba`），否则失败分支（音 `0xdbd`）；`0x15` 关闭。

`[推断]` 「找出 5 处」类找不同 / 辨认题：17 个热点中 5 个正确，必须全选对且不能多选。

仪式文案**不属**本 View `[确认]`：「今天是喜庆的日子，来，一起祈祷吧。」（消息 `0x58a`）是教会热点仪式的问句；巡礼 `0x5a7`、跳舞 `0x5a8`、绝食 `0x5ad`、取圣水 `0x5bd` 与时限「太阳下山前不回来就会被猛兽吃了」`0x5bf` 都是城门热点（kind 11）仪式的五个 variant 里的句子。它们由 `CMpRemains` 的 `fn_140002171` / `fn_14000e840` 直接 `fn_140004b3d` 播出，不经事件脚本，也不开 `CChurchGameView`（`05-inn-church-palace.md` 2.3）。教会**不是**休息点；「安安静静地祈祷」`0x5b5` 是绝食仪式的一句。

`[未知]` 哪段剧本用了脚本号 5（本 View）。奖励由脚本处理。

### 2.8 酒吧划拳（无 View 类）

`[确认]` `0x753f`「喂，我们划拳吧。……剪刀﹑石头……。一二三！我是%s，你是%s吗？」`0x7587`「啊，我输了。哇，我赢了。再见。」

`[推断]` 酒馆随机对局，胜负只改对话，未必有金币。不进入八个 GameView 类也可，用 `minigame_id: rps` 可选开启。

### 2.9 广场人群

`[确认]` 「广场上聚集了一群人。」`[确认]` 广场/设施代码不直接启动任何小游戏（2.0：唯一入口是脚本）。`[未知]` 人群事件的脚本里是否含 `0xC1` 指令、用哪个号。配置用 `triggers: [plaza_crowd]` 只是模组挂钩，原版包留空。

### 2.10 通用奖励句

`0x343f` 长块里有「得到了%s金币。」「疲劳度恢复了%s。」「由于温泉的效能，『经验』上升了。」温泉属村落/发现，**不要**当成小游戏的统一奖金。原版小游戏不发奖（2.0），奖励绑定在调用方（脚本 / 触发器）上，不在游戏定义里。

## 3. 实体与 ID

原版无 `eMinigame` 枚举，但脚本指令有 0–6 游戏号（2.0）。按 D1，主键是 key（`vanilla:coin`）；下表「序」只是本文排序，不进配置。原版号单列 `script_game_index`，只在导入层用：

| 序 | key | 原版类 | `script_game_index` | 默认触发 |
| --- | --- | --- | --- | --- |
| 0 | `coin` | `CCoinGameView` | 0 | 事件脚本 `[确认]` |
| 1 | `rat` | `CRatGameView` | — | 航海老鼠遭遇（文案 `[确认]`，调用点 `[未知]`） |
| 2 | `water` | `CWaterGameView` | 3 | 事件脚本 `[确认]` |
| 3 | `stone_wall` | `CStoneWallGameView` | 6 | 事件脚本 `[确认]` |
| 4 | `nim` | `CNimGameManager` | 1 | 事件脚本 `[确认]` |
| 5 | `catch` | `CCatchGameView` | 4 | 事件脚本 `[确认]` |
| 6 | `church` | `CChurchGameView` | 5 | 事件脚本 `[确认]`；`[推断]` 教会剧情里 |
| 7 | `rps` | （无类） | — | 酒馆 `[确认]` 文案 |
| 8 | `across` | `CAcrossGameView` | 2 | 事件脚本 `[确认]`；规则 `[未知]` |

`across` 原版可经脚本号 2 打开，不是死代码；规则还原前 `engine` 留空，引擎拒绝启动。

脚本号 > 6 原版直接回 1（不启动）。重构里脚本只写 `minigame: <key>`，加载器按 `script_game_index` 把原版 Sc 指令翻成 key。

空值：`null`（D1）。不要用 0 当「没在玩」——0 是硬币游戏的脚本号。会话状态用 `current_minigame: null`。

## 4. 算法 / 公式

解释器按 `rules.engine` 分支。参数全部来自 YAML。

### coin（双面排列）

```
state = sequence of coin_count faces          # 原版 5，3 正 2 反为目标
legal_move(i):
  i 与 i+1 相邻且 face[i] != face[i+1]
  可插入到区块左侧或右侧的「另一侧」空位
  该侧无邻币则不可
win:  coins contiguous AND faces grouped      # 原版：f0==f1 且 f3==f4，HHHTT/TTHHH 皆可
bounds: pos_min_exclusive < leftmost < pos_max_exclusive   # 原版 -5 / 9，越界重置
```

`npc_min_steps` 只用于「破纪录」台词，不参与胜负。赌金不在本系统（2.0）。

### rat

```
rats = max(rats_min, floor(valid_cabins / rats_per_cabins))   # 原版 1 / 4
countdown = 3 拍（先等 500 ms，每拍 1000 ms）
time_limit = 30 s                                             # 每 1000 ms 一拍
each second: if t>=limit or rats_alive==0: end
click id in [click_id_lo, click_id_hi): cabin = id - click_id_lo; catch if rat there
rats hop to a neighbour cabin when their timer elapses
win: rats_alive==0
lose: timeout
lose_effect: must_dock_to_clear_rats          # 文案
```

舱室集合与 `valid_cabins` 查 ships（当前船 `eShipCabin`），本系统不存舱室定义。

### water

```
buckets = [{cap:100, amount:100}, {cap:70, amount:0}, {cap:30, amount:0}]
move: pour i→j, amount = min(src.amount, dst.cap - dst.amount)
win: amount[2]==0 AND amount[0]==amount[1]                # 原版硬编码判定
hint k (k=0,1,2..): reset, auto-play first hint_steps[min(k,2)] of demo_solution
praise if steps < praise_below                             # 原版 10
```

原版判定是写死的「桶 2 空、桶 0 = 桶 1」。重构写成谓词表达式 `goal`，不要把 50/50 当成唯一可能。

### stone_wall

```
solution = [4, 7, 6, 4, 0, 3, 1, 5, 2]       # 原版默认包
progress p = 0
on click k:
  if k == exit_click: close (no result call)
  if p < len and k == solution[p]: p += 1; if p==len: win
  else: lose                                  # 一错即负
```

模组可换 `solution` 数组，不必改代码。

### nim

```
tokens = tokens_base + rand(tokens_rand)     # 原版 10 + rand(8) → 10..17；槽上限 17
first = player if rand(100) < player_first_pct   # 原版 50
take ∈ [take_min, take_max]                   # 原版 1..3
after a move: remaining > 1 → swap; remaining == 1 → mover wins; remaining == 0 → mover loses
npc: if n % (take_max+1) == 1 → take rand(take_max)+1
     elif n <= take_max+1 → take n-1
     else r = n % (take_max+1); take (r==0 ? take_max : r-1)
```

原版是 misère（拿最后一枚者输），NPC 是完美策略，只在先手落入必败位时随机取。

### catch

```
board = 8x8
pieces: player × 2, chicken × 2；回合 0→1→2→3 轮转
chicken AI: 依象限给 4 方向优先序，走第一个空格；否则走第一个备选
win: 四个棋子均无可动（推断：鸡被围死）→ 结果 1
```

### church

```
spots = 17                                    # +0xb4
answers = indices where identify[i] == 0      # 原版 5 个：0,2,5,8,15
toggle spot i
submit: win iff correct_selected == answers.count AND wrong_selected == 0
```

仪式分支（跳舞/绝食/圣水）已证实不是这个 View，归 `05-inn-church-palace.md` 的 `rite_sites`，不进小游戏表。

### rps

```
hands = [rock, scissors, paper]
win: 标准循环
reward: none                                  # 文案只有输赢告别
```

## 5. 耦合分析

原版：

- 规则立即数写在 ctor 与 `fn_*`（5、17、10+rand(8)、30、`0x37`、100/70/30、答案数组、`alIdentify`）
- 脚本指令里的游戏号 0–6 与启动函数一一硬连（跳表 `0x1400e1f18`）
- 老鼠游戏读当前船 `eShipCabin`、写航海是否继续
- 教会 View 在 `town/`，其余在 `game/`
- 小游戏本身不碰金币；赌金与奖品在脚本里按布尔结果分支
- 资源名 `InfoBox2.dk4` `EvItem.dk4` 绑在绘制函数

重构：

- narrative 命令树节点 `Minigames.run(key) -> int`（D8），返回值回写脚本谓词
- ocean 发 `RatsInfested { fleet_id }` → happening 的 `Action.effects` 调 `Minigames.run(rat)`
- 胜负只 emit；奖励由调用方绑定（脚本动作或触发器表），ocean 订阅「老鼠已清」

禁止 minigame 代码读 `Person*` 或 `Ship*` 指针，只收 ID。

## 6. 解耦后的配置

`config/town/minigames.yaml`：

```yaml
schema: dk4.town.minigames
version: 1

# 原则：游戏定义只描述「怎么玩、怎么判胜负」，返回 outcome。
# 赌金 / 奖品 / 对白挂在调用方（narrative 脚本动作或 triggers 表），不在这里。
minigames:
  - key: vanilla:coin
    name: 硬币
    engine: coin_rearrange
    original_class: CCoinGameView
    script_game_index: 0      # 原版 Sc 指令 0xC1 低字节
    triggers: [script]
    rules:
      coin_count: 5
      start: [H, T, H, T, H]  # 文案「正反面交错」[推断] 起手正面
      win: grouped_contiguous # f0==f1 && f3==f4 && 位置连续；HHHTT / TTHHH
      pos_min_exclusive: -5
      pos_max_exclusive: 9
      npc_min_steps: 10       # 仅台词
    strings:
      intro: ui.minigame.coin.intro
      win_record: ui.minigame.coin.win_record   # 少于 npc_min_steps

  - key: vanilla:rat
    engine: catch_rats
    original_class: CRatGameView
    triggers: [ocean_rats]
    rules:
      time_limit: 30
      time_unit: second       # 原版 1000 ms 一拍
      countdown_beats: 3
      max_rats: 20            # 槽上限
      rats_min: 1
      rats_per_cabins: 4      # rats = max(1, valid_cabins / 4)
      click_id_lo: 0x37       # cabin = click - 0x37
      click_id_hi: 0x56       # 31 = NUM_SHIPCABIN
      empty_slot_sentinel: 0x20
    lose_effect: { must_dock: true }

  - key: vanilla:water
    engine: water_jugs
    original_class: CWaterGameView
    script_game_index: 3
    triggers: [script]
    rules:
      buckets:
        - { capacity: 100, start: 100 }
        - { capacity: 70,  start: 0 }
        - { capacity: 30,  start: 0 }
      goal: "amount[2] == 0 && amount[0] == amount[1]"
      demo_solution: [[0,2],[2,1],[0,2],[2,1],[0,2],[2,1],[1,0],[2,1],[0,2]]
      hint_steps: [5, 7, 9]   # 第 n 次提示演示的步数，超出取末项
      praise_below_steps: 10
      pour_anim_frames: 21    # 玩家倒水后判定前等待
      demo_anim_frames: 30

  - key: vanilla:stone_wall
    engine: sequence
    original_class: CStoneWallGameView
    script_game_index: 6
    triggers: [script]
    rules:
      solution: [4, 7, 6, 4, 0, 3, 1, 5, 2]
      exit_click: 0x14        # [推断] 退出，不计胜负
      mistakes_allowed: 0     # 原版一错即负

  - key: vanilla:nim
    engine: nim
    original_class: CNimGameManager
    script_game_index: 1
    triggers: [script]
    rules:
      tokens_base: 10
      tokens_rand: 8          # 10 + rand(8) → 10..17
      tokens_slot_cap: 17
      take_min: 1
      take_max: 3
      misere: true            # 拿最后一枚者输
      player_first_pct: 50
      npc_policy: optimal_mod # 留对手 ≡1 (mod take_max+1)

  - key: vanilla:catch
    engine: catch_board
    original_class: CCatchGameView
    script_game_index: 4
    triggers: [script]
    rules:
      board: [8, 8]
      player_pieces: 2
      targets: 2
      target_species: chicken
      npc_step_delay_frames: 0x0b
      end_delay_frames: 0x50
      win: all_pieces_blocked # [推断] 鸡被围死

  - key: vanilla:church
    engine: identify
    original_class: CChurchGameView
    script_game_index: 5
    triggers: [script]
    rules:
      spot_count: 17
      answers: [0, 2, 5, 8, 15]   # alIdentify == 0 的下标
      allow_wrong: 0
    # 仪式（祈祷 / 巡礼 / 跳舞 / 绝食 / 圣水）不在此：属 CMpRemains，见 05 篇 rite_sites

  - key: vanilla:rps
    engine: rps
    original_class: null
    triggers: [facility_bar]
    rules:
      hands: [rock, scissors, paper]

  - key: vanilla:across
    engine: null              # 规则未还原，引擎拒绝启动
    original_class: CAcrossGameView
    script_game_index: 2
    triggers: [script]
    rules: {}
```

模组命名空间：`key` 可写 `mymod:sliding_puzzle`；`script_game_index` 只给原版包的 Sc 翻译用，新内容不要占用 0–6。

`config/formulas/town.yaml` 只放跨游戏共用上限（例如 `minigame_stake_gold_cap`）；具体棋子数留在上表。

布局像素（`CChurchGameView+0xb8=0x13E` 一类）进 `config/ui/scenes.yaml`，不进规则表。

## 7. 独立旋钮

- 硬币枚数、起手排列、胜利谓词、越界范围、NPC 最少步数
- 老鼠时限、倒数拍数、只数公式（`rats_min` / `rats_per_cabins`）、点击号范围
- 三桶容量 / 初值 / `goal` 谓词 / 演示解 / 提示阶梯
- 石墙 `solution` 数组、容错次数
- 尼姆棋子数区间、每回取数、misère 开关、先手概率、NPC 策略
- 抓鸡棋盘尺寸、双方棋子数
- 教会热点数、答案下标、容错
- 触发器：把 `coin` 挂到 `plaza_crowd` 只需改 YAML
- 奖励：在调用方（脚本 / `triggers` 绑定）写 `on_win` / `on_lose` 动作，游戏定义不变

引擎按表长注册游戏。新增 `engine` 才需要代码。

## 8. 对外接口

事件：

```
MinigameStarted { minigame_id, source, town_id?, fleet_id? }
MinigameWon { minigame_id, session_id, score?, steps? }
MinigameLost { minigame_id, session_id, reason }   # timeout | wrong | resign
RatsCleared { fleet_id }
RatsRemain { fleet_id }                # 失败，ocean 应逼迫入港
```

查询：

```
Minigames.def(minigame_id) -> rules
Minigames.legalMoves(session_id) -> [...]
Minigames.byScriptIndex(n) -> minigame_key | null    # 原版 Sc 翻译用（导入层）
```

命令：

```
Minigames.run(key, ctx?) -> int                       # D8 规范接口：同步子会话，返回 outcome（原版 0/1，脚本谓词）
# 跨 tick 的会话（老鼠）也走 Minigames.run，结果另经 MinigameWon/Lost 广播；不另设异步启动命令
SubmitMove { session_id, payload }     # payload 形状由 engine 决定
AbortMinigame { session_id }
```

`source` 取值：`script` `ocean_rats` `facility_bar` `plaza_crowd`（后两者原版包未用到，模组挂钩）。原版唯一入口是叙事命令（事件指令 `0xC1`，低字节 0–6 = 游戏号）；叙事侧写成命令树节点 `Minigames.run(key)`，返回值写回脚本谓词（`10-narrative/03-event-commands.md`）。town / ocean 若要调用，也只能在 D9 `Action.effects` 里调 `Minigames.run`，不另设入口。

存档：进行中的会话（老鼠可能跨 tick）。规则正文不存，存 `minigames.yaml` 的 schema version。

## 9. 未核实字段

- 七个 Sc 游戏号在哪些剧本/事件里出现（等 Sc 指令长度表闭合后统计 `0xC1nn`）。
- 广场人群事件脚本是否调用小游戏。
- coin 起手正反顺序；点击号 0–3 / 6 / 7 / 8 的精确语义；步数计数是否回传脚本（目前只回布尔）。
- coin 赌注兑现（脚本侧）。
- water 点击号 `0xa/0xb/0xc` 各是什么按钮；`0xdb5/0xdb6` 文本。
- catch `fn_14000a6f0` 的确切含义（无路可走？被捉？）；是否存在失败条件。
- church 成功/失败分支返回给脚本的值（两支都调 `fn_140014d58(..,1)`，失败支另调 `fn_14000d86e(..,3)`）；仪式文案是否同 View。
- stone_wall 按钮贴图数；`click==0x14` 是否确为退出。
- rat `fn_1401ee2f0` 的 `[0,0x15)` 判据含义；启动函数 `fn_1400132f0` 的调用方。
- `CAcrossGameView` 规则（`CAcrossGameCharacterPushButton` 572 行未读）。
- 各游戏 ctor 里成排的像素立即数（属 UI）。
- 「得到了%s金币」是否被任一脚本在小游戏后使用。
- 胜负是否加经验：原版技能不存盘，小游戏加经验 `[未知]`。

## 附：mesfile 块偏移 → exe 消息号

按 D10，文案 key 用 exe 消息号，不用 mesfile 字节偏移。正文里 `0x` 开头、落在 mesfile 的数多为 `catalog/mesfile_sc.txt` 的 NUL 块偏移；一块常含多条消息。下表由脚本按 exe 消息号表（`.data 0x140683620` 37 块 `{first_id,size}` + `0x1406836c0` 块内偏移；`.data` 文件偏移 = VA − `0x140001600`）切句，再按正文引号句匹配生成。只给一条的是已匹配到句；给区间的需导出时按句再选。正文里已标「消息」的号（如 `0x58a`、`0xb2e`）本来就是消息号，不在此表。

| 块偏移 | 消息号 | 首句 |
| --- | --- | --- |
| `0x343f` | `0x149` | 由于温泉的效能，“经验”上升了。 |
| `0x3507` | `0x14e` | 现在开始捉老鼠游戏。请在３０秒之内捉住各船舱出现 |
| `0x753f` | `0x362` | 一二三！我是%s，你是%s吗？ |
| `0x7587` | `0x364` | 哇，我赢了。再见。 |
| `0xbcd3` | `0x58a` | 今天是喜庆的日子，来，一起祈祷吧。 |
| `0xbefb` | `0x5a7` | 是来巡礼的吧，站到那边，按顺序祈祷。 |
| `0xbfa7` | `0x5ad` | 现在开始绝食，可以吗？ |
| `0xc06f` | `0x5bd` | 为取圣水要登上灵峰，可以吗？ |
| `0xc0b3` | `0x5bf` | 太阳下山前不回来就会被猛兽吃了，请把握时间。 |
| `0x126c3` | `0x999` | 提督，捉老鼠吧。 |
| `0x12747` | `0x99d` | 提督，捉老鼠吧！ |
| `0x1293f` | `0x9aa` | 捉住了所有老鼠！喂，放心继续航海吧！ |
| `0x1516f` | `0xb2a` | 不选择硬币，游戏无法进行。 |
| `0x192d7` | `0xd8c–0xd8e`（块内多条，按句选） | 甲板要员也要去冲锋陷阵！ |
| `0x196a3` | `0xdb5–0xdb6`（块内多条，按句选） | 嗯！？最少也需要１０步吧？用９步就能解开吗？ |
