# 小游戏

七个原版 `*GameView` / `*GameManager` 加上教会仪式。规则、时限、棋子数、标准解全部进 `config/town/minigames.yaml`。场景代码只解释表，不写「5 枚硬币」「30 秒」「答案 4,7,6,…」。

奖励（金币、经验、仅叙事）也在表里。本系统不改船耐久、市价、六维；赢了发 `MinigameWon`，由订阅方兑现。

## 1. 职责边界

本系统管：

- 小游戏目录（稳定 `minigame_id`）
- 规则参数：棋子数、时限、桶数、标准解、可移动约束
- 从哪类设施/事件启动（触发器 key）
- 胜负判定
- 赌注/奖金的**表值**（发给 economy）

本系统不管：

- 老鼠出现在航海中的遭遇率（ocean）
- 酒馆是否开门（设施）
- 赌金从哪笔现金扣（economy 执行 `grantGold`）
- 教会休息恢复身体/精神（characters，与旅馆同一出口）
- UI 布局像素（`C*GameView` ctor 里大量坐标，属 ui）

原版七个实现类：

| key | 类 | cpp |
| --- | --- | --- |
| `coin` | `CCoinGameView` + `CCoinSprite` | `game/CCoinGameView.cpp` |
| `rat` | `CRatGameView` | `game/CRatGameView.cpp` |
| `water` | `CWaterGameView` + `CBacketSpritePushButton` | `game/CWaterGameView.cpp` |
| `stone_wall` | `CStoneWallGameView` + `CStonePushButton` | `game/CStoneWallGameView.cpp` |
| `nim` | `CNimGameManager` + `CCoinView` | `game/CNimGameManager.cpp` |
| `catch` | `CCatchGameView` + `CDollSprite` + `CMoveTergetButton` | `game/CCatchGameView.cpp` |
| `church` | `CChurchGameView` + `CSelectPushButton` | `town/CChurchGameView.cpp` |

另有 `CAcrossGameView`（`game/CAcrossGameView.cpp`），规则未还原，见第 9 节。酒吧划拳只有文案，没有独立 View 类。

## 2. 原版事实

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

`[确认]` `CCoinGameView` ctor 在 `+0x150` 起留 5 个指针槽（dtor/vfunc_02 循环 `>=5`）。RTC `apMoveCoinSprite`。

`[确认]` 赌注文案存在；从玩家身上扣多少、赢是否「通吃对方全部」未从 cpp 还原。`[未知]` 触发地点（酒馆对赌最像，但 cpp 未写死设施名）。

参数默认：`coin_count=5`，`heads=3`，`tails=2`，`npc_min_steps=10`。

### 2.2 捉老鼠 `rat`

`[确认]` `0x3507`「现在开始捉老鼠游戏。请在３０秒之内捉住各船舱出现的老鼠。」

航海催促 `0x126c3`「提督，捉老鼠吧！」失败必须靠港 `0x12747`–`0x1293f`。成功「捉住了所有老鼠」后继续航海 `0x1293f`。

`[确认]` `CRatGameView` ctor：20 个槽（`>= 0x14`）在 `+0x88`，每槽 `+0x128` 初值 `0x20`。`vfunc_13` 只接受点击号 `0x37 <= id < 0x56`。

`[确认]` `fn_14000db89`：循环 `s158 >= 0x1E`（30）且剩余 `+0x17c > 0` 才继续；RTC `asRatData`（160 字节）、`eShipCabin`。`[推断]` 30 是秒/帧拍，与文案「30 秒」同一参数。

奖励：文案没有「得到金币」，成功=继续航行，失败=必须入港。`[确认]` 无金币句；`[未知]` 是否另改疲劳/粮食。

触发：航海遭遇，不是港口设施。仍放本目录，因为实现类与规则表属于 town-life 的 minigame 包。

### 2.3 倒水 `water`

`[确认]` `CWaterGameView` ctor 清 3 个 `+0x178` 指针。`CBacketSpritePushButton`，RTC `asBacketData`（96 字节）、`rcWater` `rcBacket`。

`[确认]` `fn_140008607` RTC `sAnswerTable` 144 字节：从桶 0/1/2 读三个量 `s30,s38,s40` 填表，然后 `+0x160 = 0x1E`（30）做倒计时式循环。

`[推断]` 三桶倒水谜题（把某一桶倒成目标容积）。三桶容量与目标容积 `[未知]`（未从立即数读出 3/5/8 这类经典组）。

胜负奖励 `[未知]`（mesfile 无独立「倒水」句）。

### 2.4 石墙 `stone_wall`

`[确认]` `CStonePushButton` + `CStoneWallGameView`。`fn_14000964c` RTC `alRightAnswer` 36 字节，写入：

```
4, 7, 6, 4, 0, 3, 1, 5, 2
```

共 9 步；步进到 `==9` 判定完成。点错走失败分支（`fn_140002dd3` + 关闭）。点击 `== 0x14` 另支。

`[确认]` 这是**写死在函数里的标准解**，重构必须进 YAML。按钮个数 `[推断]` ≥8（答案含 0–7）。

奖励 `[未知]`。

### 2.5 尼姆 `nim`

`[确认]` `CNimGameManager` ctor 循环 `>= 0x11`（17）个 `+0x110` 槽。与 `CCoinView` 地址相邻，共用硬币精灵当棋子。RTC `cNimGame`、`anIndexTable`、`rcCoin`。

`[推断]` 17 枚棋子的取子游戏（Nim / 减法游戏）。每回合最多取几枚 `[未知]`。胜负与赌金 `[未知]`（硬币 UI 暗示可能对赌，无独立文案）。

### 2.6 捕捉 `catch`

`[确认]` 邻近 RTC 名 `ptChiken` `ptMoveChiken` `alMove`（`fn_14000d76a`）。`CCatchGameView` ctor：两组指针各 2 个（`+0x118`、`+0x128`），布局常量 `+0xb0=0x13E` 等。`CDollSprite`、`CMoveTergetButton`。

`[推断]` 抓鸡（符号拼写 Chiken）。目标数、时限、奖励 `[未知]`。mesfile 无「鸡」的对应句。

### 2.7 教会 `church`

设施入口见 `01-facilities.md`。View 是独立小游戏。

`[确认]` `CChurchGameView` ctor 体积 0x7E8，写 `+0xb0=5`，大量按钮坐标。`CSelectPushButton`。绘制函数 `fn_1400066cc` RTC `alIdentify`（136 字节）、`rcMaru`，资源 `InfoBox2.dk4` `EvItem.dk4`。

`[推断]` 「辨认 / 点选正确对象」，圆圈 `rcMaru` 标对错。5 可能是选项数。

仪式文案（可能是同一设施的不同活动，不一定全是 `CChurchGameView`）`[确认]` 文本：

- 「今天是喜庆的日子，来，一起祈祷吧。」`0xbcd3`
- 巡礼「按顺序祈祷」；「一起跳舞」`0xbefb`
- 「现在开始绝食，可以吗？」`0xbfa7`
- 「为取圣水要登上灵峰」`0xc06f`；时限「太阳下山前不回来就会被猛兽吃了」`0xc0b3`

奖励 `[未知]`。教会也可当休息点（「安安静静地祈祷」），恢复走 characters。

### 2.8 酒吧划拳（无 View 类）

`[确认]` `0x753f`「喂，我们划拳吧。……剪刀﹑石头……。一二三！我是%s，你是%s吗？」`0x7587`「啊，我输了。哇，我赢了。再见。」

`[推断]` 酒馆随机对局，胜负只改对话，未必有金币。不进入七个 GameView 表也可，用 `minigame_id: rps` 可选开启。

### 2.9 广场人群

`[确认]` 「广场上聚集了一群人。」`[未知]` 人群打开的是 coin / water / stone_wall / catch / nim 中的哪几个，还是只触发阿科布/流行。配置用 `triggers: [plaza_crowd]` 挂到候选游戏上，原版具体集合未还原。

### 2.10 通用奖励句

`0x343f` 长块里有「得到了%s金币。」「疲劳度恢复了%s。」「由于温泉的效能，『经验』上升了。」温泉属村落/发现，**不要**当成七个小游戏的统一奖金。各游戏 `reward` 字段独立，缺省为 `none`。

## 3. 实体与 ID

原版无 `eMinigame`。重构目录：

| id | key | 原版类 | 默认触发 |
| --- | --- | --- | --- |
| 0 | `coin` | `CCoinGameView` | `[未知]` 酒馆对赌 |
| 1 | `rat` | `CRatGameView` | 航海老鼠遭遇 `[确认]` |
| 2 | `water` | `CWaterGameView` | `[未知]` 广场/酒馆 |
| 3 | `stone_wall` | `CStoneWallGameView` | `[未知]` |
| 4 | `nim` | `CNimGameManager` | `[未知]` |
| 5 | `catch` | `CCatchGameView` | `[未知]` |
| 6 | `church` | `CChurchGameView` | 教会设施 `[确认]` |
| 7 | `rps` | （无类） | 酒馆 `[确认]` 文案 |

`CAcrossGameView` 不进原版包 id 0–7，模组或还原后再挂。

非法 id：`invalid`。不要用 0 当「没在玩」——0 是硬币游戏。会话状态用 `current_minigame_id: invalid`。

## 4. 算法 / 公式

解释器按 `rules.engine` 分支。参数全部来自 YAML。

### coin（双面排列）

```
state = sequence of coin_count faces          # 原版 5，3 正 2 反为目标
legal_move(i):
  i 与 i+1 相邻且 face[i] != face[i+1]
  可插入到区块左侧或右侧的「另一侧」空位
  该侧无邻币则不可
win:  faces == target                         # 默认 HHHTT 或等价「同面相邻」
bet:  stake_mode                              # all_npc_gold | fixed | none
```

`npc_min_steps` 只用于「破纪录」台词，不参与胜负。

### rat

```
time_limit_ticks = 30                         # 与文案秒对齐，tick 单位可配
rats_alive = spawn(cabins)
each tick: if time>=limit or rats_alive==0: end
click cabin_hotspot in [click_id_lo, click_id_hi): catch if rat present
win: rats_alive==0
lose: timeout
lose_effect: must_dock_to_clear_rats          # 文案
```

舱室集合查 ships（当前舰队 `eShipCabin`），本系统不存舱室定义。

### water

```
buckets[3].capacity = [...]                   # [未知] 原版升
buckets[3].amount   = start
goal = { index, amount }                      # [未知]
move: pour i→j until src empty or dst full
timeout_ticks = 30                            # ctor 路径 +0x160=0x1E
win: buckets match goal
```

### stone_wall

```
solution = [4, 7, 6, 4, 0, 3, 1, 5, 2]       # 原版默认包
progress p = 0
on click k:
  if k == solution[p]: p += 1; if p==len: win
  else: lose
```

模组可换 `solution` 数组，不必改代码。

### nim

```
tokens = 17
take_max = null                               # [未知]
player and npc alternate removing 1..take_max
win: last-take 或 misère，原版哪一种 [未知]
```

未还原前引擎仍按表运行；`take_max` 缺省则拒绝启动（禁止代码里 fallback 成 3）。

### catch

```
targets = chicken_count                       # [未知]
on collide(player_cursor, chicken): caught += 1
win: caught >= targets 或 timeout 分数        # [未知]
```

### church

```
options = 5                                   # +0xb0
identify_round: pick id in alIdentify
win: selection ∈ answers
```

仪式分支（跳舞/绝食/圣水）若证实不是这个 View，则做成 `church_rite` 另条，用同一 `LaunchMinigame`。

### rps

```
hands = [rock, scissors, paper]
win: 标准循环
reward: none                                  # 文案只有输赢告别
```

## 5. 耦合分析

原版：

- 规则立即数写在 ctor 与 `fn_*`（5、17、30、`0x37`、答案数组）
- 老鼠游戏读 `eShipCabin`、写航海是否继续
- 教会 View 在 `town/`，其余在 `game/`
- 赌金直接动玩家金币
- 资源名 `InfoBox2.dk4` `EvItem.dk4` 绑在绘制函数

重构：

- ocean 发 `RatsInfested { fleet_id }` → 本系统 `LaunchMinigame { rat }`
- 设施发 `LaunchMinigame { church }` 等
- 胜负只 emit；economy 订阅赌金，ocean 订阅「老鼠已清」

禁止 minigame 代码读 `Person*` 或 `Ship*` 指针，只收 ID。

## 6. 解耦后的配置

`config/town/minigames.yaml`：

```yaml
schema: dk4.town.minigames
version: 1

minigames:
  - id: 0
    key: coin
    name: 硬币
    engine: coin_rearrange
    original_class: CCoinGameView
    triggers: []              # 原版触发未还原，留空表示仅命令可启动
    rules:
      coin_count: 5
      target: [H, H, H, T, T]
      npc_min_steps: 10
    stake:
      mode: all_opponent      # 文案「我所有的钱」；兑现 [未知]
    reward:
      type: stake
    strings:
      intro: ui.minigame.coin.intro
      win_record: ui.minigame.coin.win_record   # 少于 npc_min_steps

  - id: 1
    key: rat
    engine: catch_rats
    original_class: CRatGameView
    triggers: [ocean_rats]
    rules:
      time_limit: 30
      time_unit: tick         # 与文案秒 1:1 是原版包默认
      max_rats: 20
      click_id_lo: 0x37
      click_id_hi: 0x56
      empty_slot_sentinel: 0x20
    reward:
      type: none
    lose_effect: { must_dock: true }

  - id: 2
    key: water
    engine: water_jugs
    original_class: CWaterGameView
    triggers: []
    rules:
      bucket_count: 3
      capacities: null        # [未知] 抽出后填
      start: null
      goal: null
      timeout: 30
    reward: { type: none }

  - id: 3
    key: stone_wall
    engine: sequence
    original_class: CStoneWallGameView
    triggers: []
    rules:
      solution: [4, 7, 6, 4, 0, 3, 1, 5, 2]
      extra_click: 20         # 原版 0x14 分支
    reward: { type: none }

  - id: 4
    key: nim
    engine: nim
    original_class: CNimGameManager
    triggers: []
    rules:
      tokens: 17
      take_max: null          # 缺省则启动失败，禁止代码猜 3
      misere: null
    reward: { type: none }

  - id: 5
    key: catch
    engine: catch_moving
    original_class: CCatchGameView
    triggers: []
    rules:
      target_species: chicken
      target_count: null
      mover_slots: 2
      doll_slots: 2
    reward: { type: none }

  - id: 6
    key: church
    engine: identify
    original_class: CChurchGameView
    triggers: [facility_church]
    rules:
      option_count: 5
    rites:
      - key: pray
      - key: dance
      - key: fast
      - key: holy_water
        time_limit_i18n: ui.minigame.church.holy_water_deadline
    reward: { type: none }

  - id: 7
    key: rps
    engine: rps
    original_class: null
    triggers: [facility_bar]
    rules:
      hands: [rock, scissors, paper]
    reward: { type: none }
```

`config/formulas/town.yaml` 只放跨游戏共用上限（例如 `minigame_stake_gold_cap`）；具体棋子数留在上表。

布局像素（`CChurchGameView+0xb8=0x13E` 一类）进 `config/ui/scenes.yaml`，不进规则表。

## 7. 独立旋钮

- 硬币枚数、目标排列、NPC 最少步数
- 老鼠时限、最大只数、点击号范围
- 三桶容量与目标（还原后）
- 石墙 `solution` 数组
- 尼姆棋子数、每回上限
- 教会选项数、仪式列表
- 触发器：把 `coin` 挂到 `plaza_crowd` 只需改 YAML
- 奖励类型：`none` / `stake` / `gold` / `exp`（exp 由 progression 订阅）

引擎按表长注册游戏。新增 `engine` 才需要代码。

## 8. 对外接口

事件：

```
MinigameStarted { minigame_id, source, town_id?, fleet_id? }
MinigameWon { minigame_id, score, steps? }
MinigameLost { minigame_id, reason }   # timeout | wrong | resign
RatsCleared { fleet_id }
RatsRemain { fleet_id }                # 失败，ocean 应逼迫入港
```

查询：

```
Minigames.def(minigame_id) -> rules
Minigames.legalMoves(session_id) -> [...]
```

命令：

```
LaunchMinigame { minigame_id, source }
SubmitMove { session_id, payload }     # payload 形状由 engine 决定
AbortMinigame { session_id }
```

`source` 取值：`facility_bar` `facility_church` `plaza_crowd` `ocean_rats` `script`。

存档：进行中的会话（老鼠可能跨 tick）。规则正文不存，存 `minigames.yaml` 的 schema version。

## 9. 未核实字段

- 七个游戏中除 rat / church / rps 外的触发设施。
- 广场人群具体启动哪几个游戏。
- coin 赌注兑现（是否真的「对方全部金币」）。
- water 三桶容量、初值、目标。
- nim 的 `take_max`、正常/misère。
- catch 目标数、时限、是否金币奖。
- church `alIdentify` 内容；仪式文案是否同 View。
- stone_wall 按钮贴图数；`click==20` 含义。
- rat `+0x128` 初值 `0x20` 除「空槽」外是否编码舱室。
- `CAcrossGameView` 规则、触发、是否原版可玩到。
- 各游戏 ctor 里成排的像素立即数（属 UI）。
- 「得到了%s金币」是否被任一 GameView 引用（目前只在大杂烩块出现）。
- 胜负是否加经验：原版技能不存盘，小游戏加经验 `[未知]`。
