# 七文化圈 / 霸权区

本篇把 **7 文化圈**做成一等公民配置轴：北海、地中海、非洲、印度洋、东南亚、东亚、新大陆。

大佬 **11-06 16:58** 定调：`CForce` World 表 **+0x10 的 7×uint16** 是文化圈影响力，不是「地理带」的模糊说法，也不是剧本 / 海域 / NPC 脸。`08-expert-coverage.md` 记为 11-06 16:55 后补刀；本篇按 16:58 引用。

本轴 **只此一份**。`03-relations-influence.md` 里的 `theater_id`、`01-forces.md` 里的 `theater_influence[7]`，下标与本表 `culture_circle_id` 同一空间。

## 1. 职责边界

**管：**

- 7 个文化圈 / 霸权区的 ID、key、显示名
- 21 档 `town.region` → 7 圈的聚类表
- 97 港的 `culture_circle_id`（内容包字段，挂在 `towns.yaml`）
- 势力影响力数组的 **下标空间**（`FORCE_INFLUENCE` × 7）
- 霸者之证七件、七贤地图的 **外键**（`item_id`，物本身属 items）
- 遗迹分组用的 **外键槽**（`remains.yaml` 可写 `culture_circle_id`；25 条遗迹坐标未抽出前原版包填 `null`）

**不管（禁止塞进本表）：**

| 轴 | 计数 | 真正的主人 | 本篇 |
| --- | ---: | --- | --- |
| 9 NPC 形象圈 | 9 文案 | `02-countries-culture-fashion.md` 的 `cultures.yaml` `circles` | **不读、不写、不共用 ID** |
| 21 `CCulture` 槽 | 容器 `0x15` | 同上，`eCulture` / 105 个交易品 word | 不是本轴 |
| 21 `town.region` | 0–20 | `world/towns.yaml` + `factions/countries.yaml` | 本轴只 **读** region 做聚类 |
| 25 `eSeaArea` | 25 | `01-map-and-sea-areas.md` | 禁止用七区名填海域名 |
| 7 `eScenario` | 7 | `10-narrative/01-scenarios.md` | 同数不同空间 |
| 7 `CForceFailure` 槽 | 7 | `05-war-failure-absorb.md` 的 `failure_slot_id` | 同数；禁止改名成本轴 |
| 9 档港口 BGM | region jumptable | `01-world/08-sea-area-music-icons.md` §2.4 | 原版港口曲 **不**按 7 圈 |
| 广场 / 酒馆精灵 | person 98–107 等 | 人物 `family` 字符串 | 见下「谁读谁不读」 |

**谁读本轴：** 势力影响力、霸者之证、遗迹分组、（可选）海域/霸权 UI 文案与海曲。

**谁不读本轴：** 9 张 NPC 脸。设施换装问 `npc_face_circle_id`（02 里现叫 `circles[].id` / 曾误用名 `culture_circle_id`）。本篇收回 `culture_circle_id` 专指 **7 圈 0–6**。

系统之间只传 `culture_circle_id`、`town_id`、`force_id`、`item_id`、`remains_id` 和标量。不传 `CForce*`。

## 2. 原版事实

### 2.1 三套地理 ID，禁止混用

| 空间 | 原版计数 | 原版落点 | 干什么 | 等级 |
| --- | ---: | --- | --- | --- |
| **7 文化圈**（本篇） | 7 | `CForce+0x10` 七个 word；mesfile 霸者之证七名；exe `0x58ab4c` 短名 | 影响力下标、霸权、遗迹/证物分组 | 布局 `[确认]`；语义 `[推断]` 大佬 11-06 16:58 |
| **9 NPC 形象圈** | 9 个 `family` 文案 | 人物表 id≥98；广场店主 10 人（欧洲拆男女） | 酒馆/广场/国王精灵 | `[确认]` 文案；**无** 与 7 圈的表 |
| **21 `town.region`** | 0–20，21 首都 | `CTown` DLL `+0x25` | 国家档；`CCulture` 21 槽 `[推断]` 对齐 | `[确认]` 字节 |
| **25 `eSeaArea`** | 25 | `SeaAreaID.h` `NUM_SEAAREA=25` | 航海分区 | `[确认]` |

`eCulture` 的 `NUM_CULTURE` 头文件整数未还原（`CultureID.h`）。`CCultureElement` 构造 `fn_140007dce(..., stride=0xE8, count=0x15)` → 21 槽。那是 **region 文化块**，不是 7 圈，也不是 9 张脸。

`num_values.json` 的 `NUM_KIND=9` 不得写成「就是文化圈」——9 对得上 NPC 脸，对不上本轴。

同数陷阱：

| 也是 7 | 不是本轴 |
| --- | --- |
| `NUM_SCENARIO=7` | 卡斯特罗非零格是 **1**（里斯本/地中海），不是剧本 0。01 已否。 |
| `CForceElement+0x8A0` 7 个 `CForceFailure` | 失败记录槽，见 05 |
| RTC 局部名 `eSeaArea` 循环上界 7（`fn_140107150.cpp` / `fn_14010be70.cpp`） | **不能**改 `NUM_SEAAREA=7`；与霸权区同数 |
| 七贤地图 kind=9 共 7 张 | 本轴的 **消费者**，枚举仍是 `eItem` |

exe 短名第八条是「全世界」（`0x58ab4c` 顺序 0–7）。**不是** 第 8 个文化圈。影响力数组长度是 7，禁止 `id: 7`。

### 2.2 七个名字 `[确认]`

mesfile SC `0x1e247` 起，七句「这是%s的霸者之证」按固定顺序：

| id | key | 名 | 文案偏移 |
| ---: | --- | --- | --- |
| 0 | `north_sea` | 北海 | `0x1e247` |
| 1 | `mediterranean` | 地中海 | 同上块 |
| 2 | `africa` | 非洲 | `0x1e2a3` |
| 3 | `indian_ocean` | 印度洋 | 同上 |
| 4 | `southeast_asia` | 东南亚 | `0x1e2cf` |
| 5 | `east_asia` | 东亚 | `0x1e2e7` |
| 6 | `new_world` | 新大陆 | 同上 |

同序再出现：`0x1e313`–`0x1e41b`（七贤地图「标有%s霸者之证的位置」）、`0x1e817`–`0x1e92b`（复述七证）。

exe `.rdata` `0x58ab4c`（VA `0x14058bd4c` 一带）短名同一顺序，其后多「全世界」。`fn_140009750`（RTC `SeaAreaMessage`）长名是「北海海域 / 地中海海域 / … / 新大陆海域」加「周边」变体，**不是** 25 个 `eSeaArea` 名。来源：`01-world/08-sea-area-music-icons.md` §2.2。

mesfile 里另有「加勒比海 / 波罗的海」：闲聊或商品百科，**不要**当第 8、第 9 圈，也不要改 id 6 的正式名为加勒比海。霸者之证用「新大陆」。

### 2.3 势力 +0x10：7×uint16 `[确认]` 布局 / `[推断]` 语义

- `[确认]` `CForce::vfunc_06/07` 循环 `s20 >= 7`，每次读写 `&self + s20*2 + 0x10` 一个 word。来源：`srcdata/source/game/world/CForce.cpp`。
- `[确认]` 宽度等于 `FORCE_INFLUENCE`（USHORT）。MIN/MAX 整数未还原。来源：`Force.h` 注释、`constants.json`。
- `[确认]` World 记录步长 `0x58`，`+0x10` 起 14 字节。来源：`World0_force.c` / `World1_force.c`、`Docs/_data/forces_world0.json`。
- `[推断]` 大佬 11-06 16:58：这 7 格 = 本篇文化圈。格号 = `culture_circle_id`。母港落在哪一圈，开局哪一格非 0。
- `[确认]` 观测（解码 hex，非字段名）：活跃势力 **恰好一格非 0**；未出场（母港 98）七格全 0。

World0 非零格与母港：

| force | 名 | 母港 | 非零格 | 值 |
| ---: | --- | --- | ---: | ---: |
| 0 | 卡斯特罗商会 | 11 里斯本 | 1 | 122 |
| 1 | 阿歌特商会 | 2 阿姆斯特丹 | 0 | 225 |
| 2 | 柏格斯通商会 | 7 斯德哥尔摩 | 0 | 288 |
| 3 | 李家 | 54 杭州 | 5 | 398 |
| 4 | 克利福德军 | 0 伦敦 | 0 | 809 |
| 5 | 舒派亚商会 | 5 汉堡 | 0 | 730 |
| 6 | 阿博科鲁克军 | 11 里斯本 | 1 | 486 |
| 7 | 巴鲁迪斯军 | 13 塞维利亚 | 1 | 1323 |
| 8 | 陈特利欧商会 | 15 热那亚 | 1 | 861 |
| 9 | 巴夏军 | 22 伊斯坦布尔 | 1 | 1509 |
| 10 | 海雷丁家 | 25 亚历山大 | 1 | 921 |
| 11 | 西鲁韦拉商会 | 29 圣乔治 | 2 | 461 |
| 12 | 埃斯皮诺沙商会 | 34 索法拉 | 2 | 1027 |
| 13 | 伍丁商会 | 38 巴士拉 | 3 | 1278 |
| 14 | 纳哥普尔商会 | 42 卡利卡特 | 3 | 1079 |
| 15 | 普雷依拉商会 | 47 马六甲 | 4 | 566 |
| 16 | 库恩商会 | 50 雅加达 | 4 | 1090 |
| 17 | 来岛家 | 59 大阪 | 5 | 428 |
| 18 | 玛尔德纳德军 | 61 哈瓦那 | 6 | 537 |
| 19 | 埃斯康特军 | 65 委拉克鲁斯 | 6 | 866 |
| 20–24 | 未出场 | 98 | — | 全 0 |

World1：出场势力 **格号不变、数值不同**（观测 232–1254）。图鲁维母港 10 里加、格 0 = 524；朝鲜水师母港 57 汉城、格 5 = 232。主角家 0–3 / 20 / 21 / 24 母港 98、七格全 0。

海雷丁母港亚历山大（region **9**）非零格是 **1（地中海）**，不是 2（非洲）。这是本轴相对 9 NPC 圈最硬的反例：02 把 region 9–11 粗分到非洲脸，本轴把 region 9 放进地中海。

### 2.4 region → 7 圈聚类 `[推断]`

规则：圈 = 该港 `region` 所属首都带；首都带由 **该带上的势力母港** 钉死。21 个首都里 **20 个本身就是某势力母港**（汉城只在 World1）；唯一缺口是 region 7 雅典。

| circle | 圈 | region | 钉死用的母港 | 缺口 |
| ---: | --- | --- | --- | --- |
| 0 | 北海 | 0–3 | 伦敦 / 阿姆斯特丹 / 汉堡 / 斯德哥尔摩；World1 里加 | 无 |
| 1 | 地中海 | 4–9 | 里斯本 / 塞维利亚 / 热那亚 / 伊斯坦布尔 / 亚历山大 | region 7 无母港，见下 |
| 2 | 非洲 | 10–11 | 圣乔治 / 索法拉 | 无 |
| 3 | 印度洋 | 12–13 | 巴士拉 / 卡利卡特 | 无 |
| 4 | 东南亚 | 14–15 | 马六甲 / 雅加达 | 无 |
| 5 | 东亚 | 16–18 | 杭州 / 大阪；World1 汉城 | 无 |
| 6 | 新大陆 | 19–20 | 哈瓦那 / 委拉克鲁斯 | 无 |

region 7（雅典、克里特、塞浦路斯）：夹在 region 6 热那亚与 region 8 伊斯坦布尔之间，两邻都是格 1；港口 BGM 把 4–7 合成南欧档。`[推断]` 归地中海。标 `[邻域]`，不升格。

### 2.5 霸者之证与遗迹分组 `[推断]`

kind 字节 `SItem+0x14` 的分类见 `09-items/01-item-catalog.md`（`[推断]`）。下列 `+0x15` 连续 0–6，与七区同序：

| 包 | id | 名 | `+0x14` kind | `+0x15` | `+0x19` |
| --- | ---: | --- | ---: | ---: | ---: |
| 霸者之证 | 148–154 | 乌勒尔弓箭 … 水晶头骨 | 8 | 0–6 | 8 |
| 七贤地图 | 155–161 | 泰勒斯 … 比塔科史 | 9 | 0–6 | 8 |
| 第二组 kind 8 | 192–198 | 哈尔舒塔供品车 … 契慕斧头 | 8 | 0–6 | 10 |

依据：

1. mesfile `0x1e247` 在赠礼 147「七色鹦鹉」口味句之后 **立刻** 接七句霸者之证，与道具 id 148–154 目录顺序同向。
2. 单件说明地理对得上七区：乌勒尔=北欧神话（`0x1bbcb`）、肯毕西斯=阿契曼尼（`0x1bc0f`）、阿克斯姆=埃塞俄比亚（`0x1bc77`）、吠陀=印度、库德里=印尼、长信宫灯=秦始皇（`0x1bdab`）、水晶头骨=新大陆常见意象。
3. 七贤地图说明是「标有%s霸者之证的位置」（`0x1e313` 起）。
4. `CItem.cpp` **没有**给 `+0x15` 起名。故整表保持 `[推断]`，禁止写成确认字段 `culture_circle`。

`NUM_REMAINS` 头文件未写死；`CRemainsElement` 循环 `0x19` → `[推断]` 25 条。25 ≠ 7，遗迹 **不是** 每圈一条。本轴只提供「这条遗迹属于哪一圈」的 FK；原版 25 条坐标未抽出，**禁止**手编 25 行圈号。第二组 kind 8（192–198）像是按圈分组的遗迹宝物，归属 `09-items` / 遗迹解谜篇，本篇只把 `+0x15` 认作同一 ID 空间。

地点地图 162–173（kind 10）的 `+0x18` 是出发港 id（阿姆斯特丹、汉堡、雅典…哈瓦那），`+0x15` 恒为 8，**不是** 圈下标。

### 2.6 BGM：港口九档 ≠ 本轴；海曲「也许」

`[确认]` 港口曲 `fn_140002cbb` 读 `CTown+0x25`（region），21 路 jumptable 合成 **9** 档 `0x0B..0x13`。来源：`08-sea-area-music-icons.md` §2.4。埃及（region 9）走伊斯兰曲，与海雷丁的地中海格不一致；朝鲜与中国共用曲，日本单档——这 9 档更接近 NPC/港曲，不是 7 圈。

大地图默认只播 `0x14` / `0x15` 两档，**没有** 25 首海域曲，也 **没有** 已还原的「按 7 圈换海曲」表。`SeaAreaMessage` 却用七区长名。重构允许在本表挂可选 `ocean_bgm_id`，原版包必须 `null`。

 Soundtrack DISC04 港曲 9 首（北欧之街…新大陆之街）对 9 档港口 BGM `[推断]`；海曲若干首对七区 `[未知]`，禁止当确认绑定。

### 2.7 易混港（三轴会给出三个答案）

| 港 | region | 本轴 7 圈 | 02 的 9 脸粗分 | 港口 BGM 档 |
| --- | ---: | --- | --- | --- |
| 亚历山大 / 的黎波里 / 阿尔及尔 / 突尼斯 / 班加西 | 9 | **地中海**（海雷丁格 1） | 非洲 | 伊斯兰 `0x0D` |
| 休达 / 马德拉 | 4 | 地中海（里斯本带） | 欧洲 | 南欧 `0x0C` |
| 拉斯帕马斯 | 5 | 地中海（塞维利亚带） | 欧洲 | 南欧 |
| 巴士拉一带 | 12 | **印度洋**（伍丁格 3） | 阿拉伯 | 伊斯兰 `0x0D`（与伊斯坦布尔同曲） |
| 汉城 / 釜山 | 17 | **东亚**（World1 朝鲜水师格 5） | 韩国 | 与中国同档 `0x11` |
| 雅典 / 克里特 / 塞浦路斯 | 7 | 地中海 `[邻域]` | 欧洲 | 南欧 `0x0C` |

实现禁止「看地图像非洲就写 2」。只读 `culture_circle_id` 字段。

## 3. 实体与 ID

| 项 | 值 |
| --- | --- |
| ID | `culture_circle_id` 0–6 |
| 原版枚举名 | **无** `*ID.h`。重构枚举可叫 `eCultureCircle`，标推断 |
| 非法 | `-1`（与其它 INVALID_* 一致） |
| 禁止的空值 | `7`（那是「全世界」短名下标）、`26`、`98`、`0`（0 是北海） |
| 原版条数 | 7（内容包实例；运行时读表长） |
| 别名 | `north_sea` … `new_world` |
| 影响力元素 | `ushort`；初值见 01 / 上表 |

`03` 的 `theater_id` **就是** 本 ID。新代码用 `culture_circle_id`。`relations.yaml` 里 `iberia_med` / `arabia_india` 是旧注释 key，本表以 mesfile 为准：`mediterranean` / `indian_ocean`。

9 脸 ID 0 = 欧洲，7 圈 ID 0 = 北海：**两个 0 不可互换**。里斯本是圈 1 + 脸 0。

### 3.1 97 港 → 圈

生成规则（代码只实现这一条，不要 97 路 `if`）：

```
culture_circle_of(town) := region_to_circle[ Town.region(town) ]
region_to_circle = {0:0,1:0,2:0,3:0, 4:1,5:1,6:1,7:1,8:1,9:1,
                    10:2,11:2, 12:3,13:3, 14:4,15:4,
                    16:5,17:5,18:5, 19:6,20:6}
```

等级列：`母港` = 该港是某势力 World 表母港，格号直接观测；`同带` = 无母港，跟同 region 首都走；`邻域` = region 7。`母港` 是布局观测，**语义仍是推断**（字段名未恢复）。

计数：北海 13、地中海 23、非洲 11、印度洋 13、东南亚 13、东亚 11、新大陆 13。合计 97。

#### 0 北海（region 0–3）

| id | key | 名 | region | 等级 |
| ---: | --- | --- | ---: | --- |
| 0 | london | 伦敦 | 0 | 母港 克利福德 |
| 1 | bristol | 布里斯托尔 | 0 | 同带 |
| 2 | amsterdam | 阿姆斯特丹 | 1 | 母港 阿歌特 |
| 3 | bruges | 布鲁日 | 1 | 同带 |
| 4 | nantes | 南特 | 1 | 同带 |
| 5 | hamburg | 汉堡 | 2 | 母港 舒派亚 |
| 6 | lubeck | 吕贝克 | 2 | 同带 |
| 7 | stockholm | 斯德哥尔摩 | 3 | 母港 柏格斯通 |
| 8 | oslo | 奥斯陆 | 3 | 同带 |
| 9 | copenhagen | 哥本哈根 | 3 | 同带 |
| 10 | riga | 里加 | 3 | 母港 World1 图鲁维；World0 同带 |
| 88 | relistad | 雷利史塔特 | 1 | 同带 |
| 90 | saint_malo | 圣马罗 | 1 | 同带 |

#### 1 地中海（region 4–9）

| id | key | 名 | region | 等级 |
| ---: | --- | --- | ---: | --- |
| 11 | lisbon | 里斯本 | 4 | 母港 卡斯特罗、阿博科鲁克 |
| 12 | ceuta | 休达 | 4 | 同带 |
| 13 | seville | 塞维利亚 | 5 | 母港 巴鲁迪斯 |
| 14 | valencia | 瓦伦西亚 | 5 | 同带 |
| 15 | genoa | 热那亚 | 6 | 母港 陈特利欧 |
| 16 | marseille | 马赛 | 6 | 同带 |
| 17 | syracuse | 锡拉库萨 | 6 | 同带 |
| 18 | venice | 威尼斯 | 6 | 同带 |
| 19 | athens | 雅典 | 7 | **邻域** |
| 20 | crete | 克里特 | 7 | **邻域** |
| 21 | cyprus | 塞浦路斯 | 7 | **邻域** |
| 22 | istanbul | 伊斯坦布尔 | 8 | 母港 巴夏 |
| 23 | ragusa | 拉古札 | 8 | 同带 |
| 24 | beirut | 贝鲁特 | 8 | 同带 |
| 25 | alexandria | 亚历山大 | 9 | 母港 海雷丁（格 1，不是非洲） |
| 26 | tripoli | 的黎波里 | 9 | 同带 |
| 27 | algiers | 阿尔及尔 | 9 | 同带 |
| 28 | tunis | 突尼斯 | 9 | 同带 |
| 30 | madeira | 马德拉 | 4 | 同带 |
| 31 | las_palmas | 拉斯帕马斯 | 5 | 同带 |
| 87 | abkhazia | 阿布哈兹 | 8 | 同带 |
| 91 | naples | 拿坡里 | 6 | 同带 |
| 92 | benghazi | 班加西 | 9 | 同带 |

#### 2 非洲（region 10–11）

| id | key | 名 | region | 等级 |
| ---: | --- | --- | ---: | --- |
| 29 | saint_george | 圣乔治 | 10 | 母港 西鲁韦拉 |
| 32 | verde | 绿角 | 10 | 同带 |
| 33 | luanda | 罗安达 | 10 | 同带 |
| 34 | sofala | 索法拉 | 11 | 母港 埃斯皮诺沙 |
| 35 | cape_town | 开普敦 | 11 | 同带 |
| 36 | mozambique | 莫桑比克 | 11 | 同带 |
| 37 | mogadishu | 摩加迪沙 | 11 | 同带 |
| 72 | sierra_leone | 塞拉利昂 | 10 | 同带 |
| 73 | sao_tome | 圣多美 | 10 | 同带 |
| 74 | madagascar | 马达加斯加 | 11 | 同带 |
| 75 | mombasa | 蒙巴萨 | 11 | 同带 |

#### 3 印度洋（region 12–13）

| id | key | 名 | region | 等级 |
| ---: | --- | --- | ---: | --- |
| 38 | basra | 巴士拉 | 12 | 母港 伍丁 |
| 39 | aden | 亚丁 | 12 | 同带 |
| 40 | muscat | 马斯喀特 | 12 | 同带 |
| 41 | hormuz | 霍尔木兹 | 12 | 同带 |
| 42 | calicut | 卡利卡特 | 13 | 母港 纳哥普尔 |
| 43 | goa | 果阿 | 13 | 同带 |
| 44 | ceylon | 锡兰 | 13 | 同带 |
| 45 | calcutta | 加尔各答 | 13 | 同带 |
| 46 | anman | 安曼 | 13 | 同带 |
| 76 | socotra | 索哥德拉 | 12 | 同带 |
| 77 | karachi | 卡拉奇 | 13 | 同带 |
| 78 | madras | 马德拉斯 | 13 | 同带 |
| 79 | masulipatam | 马斯利巴丹 | 13 | 同带 |

#### 4 东南亚（region 14–15）

| id | key | 名 | region | 等级 |
| ---: | --- | --- | ---: | --- |
| 47 | malacca | 马六甲 | 14 | 母港 普雷依拉 |
| 48 | brunei | 文莱 | 14 | 同带 |
| 49 | manila | 马尼拉 | 14 | 同带 |
| 50 | jakarta | 雅加达 | 15 | 母港 库恩 |
| 51 | palembang | 巴邻旁 | 15 | 同带 |
| 52 | ternate | 德尔纳特 | 15 | 同带 |
| 53 | ambon | 安波那 | 15 | 同带 |
| 80 | aceh | 亚齐 | 15 | 同带 |
| 81 | gia_dinh | 吉阿丁 | 14 | 同带 |
| 82 | banjarmasin | 班贾尔马辛 | 15 | 同带 |
| 83 | makassar | 望加锡 | 15 | 同带 |
| 84 | surabaya | 泗水 | 15 | 同带 |
| 93 | manado | 马纳多 | 15 | 同带 |

#### 5 东亚（region 16–18）

| id | key | 名 | region | 等级 |
| ---: | --- | --- | ---: | --- |
| 54 | hangzhou | 杭州 | 16 | 母港 李家 |
| 55 | quanzhou | 泉州 | 16 | 同带 |
| 56 | macau | 澳门 | 16 | 同带 |
| 57 | seoul | 汉城 | 17 | 母港 World1 朝鲜水师；World0 同带 |
| 58 | nagasaki | 长崎 | 18 | 同带 |
| 59 | osaka | 大阪 | 18 | 母港 来岛 |
| 60 | naha | 那霸 | 18 | 同带 |
| 85 | yizhou | 沂州 | 16 | 同带 |
| 89 | tamsui | 淡水 | 16 | 同带 |
| 94 | busan | 釜山 | 17 | 同带 |
| 95 | edo | 江户 | 18 | 同带 |

#### 6 新大陆（region 19–20）

| id | key | 名 | region | 等级 |
| ---: | --- | --- | ---: | --- |
| 61 | havana | 哈瓦那 | 19 | 母港 玛尔德纳德 |
| 62 | santo_domingo | 圣多明尼各 | 19 | 同带 |
| 63 | saint_john | 圣约翰 | 19 | 同带 |
| 64 | jamaica | 牙买加 | 19 | 同带 |
| 65 | veracruz | 委拉克鲁斯 | 20 | 母港 埃斯康特 |
| 66 | merida | 梅里达 | 20 | 同带 |
| 67 | portobelo | 波托韦洛 | 20 | 同带 |
| 68 | maracaibo | 马拉开波 | 20 | 同带 |
| 69 | pernambuco | 伯南布哥 | 20 | 同带 |
| 70 | trujillo | 特鲁希略 | 20 | 同带 |
| 71 | cayenne | 卡宴 | 20 | 同带 |
| 86 | pensacola | 朋沙科拉 | 19 | 同带 |
| 96 | caracas | 卡拉卡斯 | 20 | 同带 |

World0 / World1 港名与 region 相同（`world1_names.json`、`02-towns.md`）。圈号随 region，不随世界包改。

## 4. 算法 / 公式

本轴几乎没有伤害公式。立刻数进 `caps.yaml`，禁止写死 `7`、`21`、`25`。

```
culture_circle_count_vanilla = 7
influence_width              = ushort
influence_min                = 0          # [推断] FORCE_INFLUENCE_MIN
influence_max                = 65535      # 硬顶；设计上限未还原

culture_circle_of_town(town_id) =
    r = Town.region(town_id)              # 0..20；非法则 invalid
    return region_to_circle[r]            # 表；缺键 → invalid，不猜

culture_circle_of_force_home(force_id) =
    t = Force.homeTown(force_id)
    if t == town.empty_sentinel: return invalid
    return culture_circle_of_town(t)

# 开局校验（原版包）
for each active force:
    assert exactly one influence[c] != 0
    assert argmax(influence) == culture_circle_of_force_home(force)
# 运行时是否允许多格同时非 0：[未知]。数组长度仍是表长。
```

影响力加减、月结、UI 如何展示 7 格：公式未还原，见 03。本篇只规定 **下标**。未还原前禁止发明「占港加影响力 / 发展度×N」。

霸者之证持有判定：

```
Proof.of(circle_id) -> item_id | null     # 内容表 FK
Proof.held(session, circle_id) -> bool    # 问 items 库存
# 七张齐是否通关：narrative 读 7 次 Proof.held，不在本系统写 if scenario
```

遗迹：

```
Remains.cultureCircle(remains_id) -> culture_circle_id | invalid
# 原版包 25 条未抽出 → 全 invalid；模组可填
# Ocean 发现遗迹只带 remains_id；圈号由本查询补，不让 ocean 猜
```

BGM：

```
town_bgm(town_id)     = 问 ui/bgm.yaml 的 region 九档   # 本轴不拥有
ocean_bgm(circle_id)  = circles[circle_id].ocean_bgm_id # 原版 null
# 大地图仍走 08 篇的 0x14/0x15 默认；有 ocean_bgm_id 才覆盖
```

## 5. 耦合分析

原版搅在一起的：

- 影响力 7 格打在 `CForce` 88 字节记录里，和母港、金币、友好度同结构
- RTC 把 7 区循环也标成 `eSeaArea`，和 25 海域同名
- 霸者之证是 `eItem` 子集，说明字符串和赠礼粘在同一 mesfile 块
- 港口只有 `region` 一字节，国家 / 21 文化块 / 7 圈 / 9 脸全靠后处理
- `02` 曾把 9 脸叫做 `culture_circle_id`、`caps.culture_circle_count: 9`

重构切断：

```
world.towns.yaml          region_id + culture_circle_id
factions/countries.yaml   21 档国家（region）
factions/cultures.yaml    21 CCulture + 9 npc_face_circle
factions/culture_circles.yaml   本轴 7 行
factions/forces.yaml      influence[circle_id]
items/items.yaml          proof_item_id / sage_map_item_id FK
world/remains.yaml        可选 culture_circle_id
ui/bgm.yaml               港口 9 档仍按 region；海曲可选按本轴
```

允许的依赖：`factions.culture_circles → world.town_id`（region 列表、母港例）、`→ items.item_id`（证物 FK）。禁止 economy 按圈另做一张物价表（市价键仍是 `(town_id, kind_id)`，见 08 篇 §2.8）。禁止 town-life 用本 ID 选店主精灵。

`CForceFailure` 7 槽、`eScenario` 7 个、本轴 7 格：三套并存。改一处表长不影响另外两处，除非内容包显式写「失败槽数 = 圈数」——原版没写出这个等式，默认不相等。

## 6. 解耦后的配置

落点：`config/factions/culture_circles.yaml`。港口外键在 `config/world/towns.yaml`。上限进 `config/caps.yaml`。

```yaml
# config/factions/culture_circles.yaml
schema: dk4.factions.culture_circles
version: 1
pack: vanilla-world0

# 原版无独立枚举文件。运行时读表长，禁止写死 7。
# id 7「全世界」是 SeaAreaMessage 短名，不是本表一行。

circles:
  - id: 0
    key: north_sea
    name: 北海
    name_i18n: culture_circle.north_sea
    region_ids: [0, 1, 2, 3]
    proof_item_id: 148          # [推断] 狩猎之神乌勒尔的弓箭
    sage_map_item_id: 155       # [推断] 泰勒斯的纸地图
    remain_relic_item_id: 192   # [推断] 哈尔舒塔的供品车；非霸者之证
    ocean_bgm_id: null          # 原版未还原按圈海曲
  - id: 1
    key: mediterranean
    name: 地中海
    name_i18n: culture_circle.mediterranean
    region_ids: [4, 5, 6, 7, 8, 9]   # 7 为邻域推断
    proof_item_id: 149          # 肯毕西斯的王冠
    sage_map_item_id: 156
    remain_relic_item_id: 193
    ocean_bgm_id: null
  - id: 2
    key: africa
    name: 非洲
    name_i18n: culture_circle.africa
    region_ids: [10, 11]
    proof_item_id: 150          # 阿克斯姆王的金印
    sage_map_item_id: 157
    remain_relic_item_id: 194
    ocean_bgm_id: null
  - id: 3
    key: indian_ocean
    name: 印度洋
    name_i18n: culture_circle.indian_ocean
    region_ids: [12, 13]
    proof_item_id: 151          # 吠陀本集
    sage_map_item_id: 158
    remain_relic_item_id: 195
    ocean_bgm_id: null
  - id: 4
    key: southeast_asia
    name: 东南亚
    name_i18n: culture_circle.southeast_asia
    region_ids: [14, 15]
    proof_item_id: 152          # 库德里的永久护符
    sage_map_item_id: 159
    remain_relic_item_id: 196
    ocean_bgm_id: null
  - id: 5
    key: east_asia
    name: 东亚
    name_i18n: culture_circle.east_asia
    region_ids: [16, 17, 18]
    proof_item_id: 153          # 秦始皇的长信宫灯
    sage_map_item_id: 160
    remain_relic_item_id: 197
    ocean_bgm_id: null
  - id: 6
    key: new_world
    name: 新大陆
    name_i18n: culture_circle.new_world
    region_ids: [19, 20]
    proof_item_id: 154          # 水晶头骨
    sage_map_item_id: 161
    remain_relic_item_id: 198
    ocean_bgm_id: null
```

校验：

- `region_ids` 穷尽 0–20 且无重叠
- 每个 `region_ids` 里恰好一座 `is_capital`（region 7 也是）
- `proof_item_id` / `sage_map_item_id` 指向已注册 `item_id`；原版包允许标 `unverified: true`
- 表长 ≥ 1；id 连续与否由内容包决定，原版 0–6 连续

港口字段（`towns.yaml` schema 升 version；与 `culture_id`（21 `eCulture`）分列）：

```yaml
# config/world/towns.yaml 片段
schema: dk4.world.towns
version: 2          # 相对旧示例 +culture_circle_id
pack: vanilla-world0

towns:
  - id: 0
    key: london
    name: 伦敦
    is_capital: true
    region_id: 0
    culture_circle_id: 0          # 7 圈；不是 9 脸，不是 eCulture
    culture_id: null              # 21 CCulture，待与 cultures.yaml 对齐
    npc_face_circle_id: null      # 9 脸，02 填；本轴不写
  - id: 11
    key: lisbon
    name: 里斯本
    region_id: 4
    culture_circle_id: 1
  - id: 19
    key: athens
    name: 雅典
    region_id: 7
    culture_circle_id: 1          # [邻域]
  - id: 25
    key: alexandria
    name: 亚历山大
    region_id: 9
    culture_circle_id: 1          # 母港观测格 1；不要写成 2
  - id: 29
    key: saint_george
    name: 圣乔治
    region_id: 10
    culture_circle_id: 2
```

97 行的值以 §3.1 为准，由提取器按 `region_to_circle` 生成，人工只审 `[邻域]` 三港。

势力侧（对照，定义仍在 `forces.yaml`）：

```yaml
# config/factions/forces.yaml 字段改名（schema version +1）
# 旧 theater_influence: [0, 122, 0, 0, 0, 0, 0]
  - id: 0
    key: castro
    home_town_id: 11
    influence:                    # 下标 = culture_circle_id；稀疏写出非 0
      - { culture_circle_id: 1, value: 122 }
```

存档仍是 7 个 ushort 的稠密数组，长度 = 圈表长。提取器从 hex `+0x10` 生成。

```yaml
# config/caps.yaml 片段
culture_circle_count_vanilla: 7
force_influence_min: 0            # [推断]
force_influence_max: 65535        # 硬顶
# 下面三项不是本轴，写在这里只为对照，所有权仍在 02 / world
npc_face_circle_count: 9
culture_count_vanilla: 21
sea_area_count_vanilla: 25
```

遗迹（world 拥有定义）：

```yaml
# config/world/remains.yaml 可加字段
- id: 0
  key: remains_00
  culture_circle_id: null         # 25 条未抽出；有坐标后再填 0–6
```

文案进 `config/ui/strings.zh-CN.yaml`：`culture_circle.north_sea` 等。逻辑表只留 `name` 作调试对照。

## 7. 独立旋钮

改这些不应迫使贸易单价、海战、9 脸换装改代码：

- 圈的条数（运行时表长；原版包 7）
- 显示名 / key / i18n
- `region_ids` 聚类（可把雅典改去别圈，只动本表 + 三个港的字段）
- 每圈的 `proof_item_id` / `sage_map_item_id` / `remain_relic_item_id`
- 可选 `ocean_bgm_id`
- 开局影响力数值（仍在 `forces.yaml`）

不在本表改：

- 9 NPC 脸增减、店主 person_id（02）
- 21 `CCulture` 的 105 word（02 / economy 查询）
- 25 海域多边形与海域名（world）
- 港口九档 BGM jumptable（ui/bgm）
- 友好度、份额、上缴（03）
- 失败槽 7（05）
- 剧本 7（narrative）

加第 8 圈：升 `culture_circles` 与 `forces.influence` 的 schema version，存档数组变长。9 脸表不动。

## 8. 对外接口

查询（只读，ID + 标量）：

```
CultureCircle.get(id) -> { key, name_id, region_ids }
CultureCircle.ofTown(town_id) -> culture_circle_id | invalid
CultureCircle.ofRegion(region_id) -> culture_circle_id | invalid
CultureCircle.ofForceHome(force_id) -> culture_circle_id | invalid
CultureCircle.list() -> [culture_circle_id]
CultureCircle.towns(id) -> [town_id]          # 扫 towns.yaml，不持 Town*
CultureCircle.proofItem(id) -> item_id | null
CultureCircle.sageMapItem(id) -> item_id | null
Influence.get(force_id, culture_circle_id) -> int    # 实现在 03，下标本轴
```

命令：本轴定义只读。影响力写入走 03：

```
Influence.set(force_id, culture_circle_id, value, reason)
Influence.add(force_id, culture_circle_id, delta, reason)
```

事件：本系统不发。03 已有 `InfluenceChanged { force_id, theater_id, old, new }` ——载荷字段改称 `culture_circle_id`（同一整数）。items / ocean 发 `DiscoveryFound { kind: proof, item_id, remains_id }`；订阅方用 `CultureCircle` 反查圈，**不要**让 ocean 自带圈号。

UI：霸权情报、七证收集进度问上面的查询 + `strings` 的 `culture_circle.*`。`CSeaAreaForceView` 若要按圈着色，问 `Influence.get`，不读 25 海域。

不暴露 `CForce+0x10`、不暴露 0x58 布局、不暴露 mesfile 偏移。

广场店主：

```
# 禁止
Plaza.shopkeeper(town_id) := CultureCircle.ofTown(town_id)   # 7≠9

# 正确
Plaza.shopkeeper(town_id) := NpcFace.plazaShopkeeper(town_id)  # 02
```

## 9. 未核实字段

| 项 | 观察 | 禁止写成 |
| --- | --- | --- |
| `CForce+0x10` 字段名 | 存盘 7 word；格号对母港带 | 升格为确认名 `culture`；`eForceAimKind`；`eScenario` |
| `FORCE_INFLUENCE_MIN/MAX` 整数 | 只有符号；观测 122–1509 | 设计封顶 1000 / 9999 |
| 运行时多格同时非 0 | 开局单格 | 「永远 one-hot」不变量 |
| region 7 = 地中海 | 邻域 + 南欧 BGM | 确认（无母港观测） |
| `SItem+0x15` = 圈 id | 148–154 / 155–161 / 192–198 为 0–6 | 确认字段；全部 kind 8 都是霸者之证（192–198 的 `+0x19=10`） |
| 148–154 即七证 | mesfile 顺序 + 说明地理 | 头文件已恢复的 `eProof` |
| 25 遗迹 → 7 圈 | `NUM_REMAINS` 像 25 | 手填 25 行；遗迹 1:1 海域 |
| 海曲按 7 圈 | `SeaAreaMessage` 七区名；大地图两档默认 | 确认 `ocean_bgm_id` |
| 港口 BGM 按 7 圈 | jumptable 是 9 档 region | 用本表覆盖 `fn_140002cbb` |
| 「全世界」 | 短名第 8 条 | `culture_circle_id=7` |
| 加勒比海 / 波罗的海 | 闲聊文案 | 圈名或海域名 |
| `NUM_CULTURE=7` | 容器 21 | 头文件已恢复 |
| `NUM_KIND=9` | 符号 | 本轴计数 |
| 9 脸 ↔ 7 圈 映射 | 地理常识 | 确认双射（亚历山大已经反例） |
| `CForceFailure` 7 槽 = 本轴 | 同数 | 失败槽下标改名为 `culture_circle_id` |
| 13 地名（佛兰德…墨西哥） | exe `0x5b656c` | 21 region 或 7 圈名录 |
| 影响力如何随占港/投资变 | 未还原 | 任何乘积 |

hex 原文：`Docs/_data/forces_world0.json`、`srcdata/source/world/World0_force.c`、`World1_force.c`、`Docs/_data/items_world0.json`（id 148–161、192–198）、`srcdata/catalog/mesfile_sc.txt` `0x1e247`。
