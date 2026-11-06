# 存档与会话

原版存档是一份带魔法数的 packed blob。重构存档按系统分节、带版本；原版文件只通过导入器进入新 schema。经验是成长源，等级/技能读档时重算。

## 1. 职责边界

本系统管：

- 槽位、路径、魔法数、版本、校验
- 原版 `savedatXX.dk4` → 新 schema 的导入
- 会话：当前语言、当前剧本、当前 `scene_id`、未提交的 UI 叠加
- 把各系统提交的状态快照写成一节，读回来再分发

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
| 语言目录：`zh-CN` `zh-TW` `ja-JP` `ko-KR` | `[确认]` | `save_format.md` `Save.cpp` `dk4save.c` |
| ReadMe 写 `ja` / `ko`（无 `-JP/-KR`）；工具两种都探 | `[确认]` | `ReadMe_JP.txt` `ReadMe_KR.txt` `dk4save.c` `langs[]` |
| SC exe 创建目录时写死 `\KoeiTecmo\DK4HD\Savedata\zh-CN\` | `[确认]` | `fn_140056790.cpp` |
| 槽文件 `savedat00.dk4` … `savedat10.dk4`（11 个） | `[确认]` | `save_format.md`；`fn_1401120a0` `"%ssavedat%02d.dk4"` |
| 另有 `savedata.dk4`（仓库样例 256 字节，同魔法数） | `[确认]` | `Savedata/zh-CN/savedata.dk4`；`fn_1401200a0.cpp` 字符串 `savedata.dk4` |
| `savedata.dk4` 的 `+0x14` 不像槽文件那样等于文件长度 | `[确认]` | 样例 256 字节，`+0x14` 不是 256；**用途 `[未知]`**（会话/最后槽/校验） |

头与主角：

| 偏移 | 内容 | 等级 |
| --- | --- | --- |
| `+0x00` | `"DAIKOUKAI4PKW9 1"` 16 字节 | `[确认]` |
| `+0x10` | `uint32`，槽文件样例为 0 | `[未知]` 语义 |
| `+0x14` | `uint32` 写入时记录的长度；槽文件等于文件大小 | `[确认]` |
| `+0x18`…`+0x1F` | 未命名 | `[未知]` |
| `+0x20` | `uint32` 主角名册 id | `[确认]` |
| `+0x70` | `uint32` 主角总等级缓存 | `[确认]` |
| `+0x24`…`+0x6F` 除已点名外 | 样例里可见 GBK 姓名碎片 | `[未知]` 完整布局 |

名册与主角体：

| 偏移 | 内容 | 等级 |
| --- | --- | --- |
| `+0x2C00` | 98 条名册，步长 `0x19` | `[确认]` |
| 名册 `+0x03` | 所属 | `[确认]` |
| 名册 `+0x05` | 六维 6 字节 | `[确认]` |
| 名册 `+0x0B` / `+0x0F` | 经验轨 0 / 1，`uint32` | `[确认]` |
| `+0x35AA` | 主角所属（状态画面） | `[确认]` |
| `+0x35AC` | 主角六维 | `[确认]` |
| `+0x35B2` / `+0x35B6` | 主角两轨经验 | `[确认]` |
| `+0x35C3` | 名字块，步长 `0x2E9` | `[确认]` |
| 名字块 index 0 | 主角：名 `+0x00`、姓 `+0x11`、中间名 `+0x22`，每字段 17 字节 GBK | `[确认]` | `dk4save.c` `NAME_FIELD=17` |
| 名字块 1..98 | 对应水手 id-1；水手显示名来自 World 表，不来自这块 | `[确认]` | `Save.cpp` |

样例槽文件大小约 107k（`savedat00.dk4` = 107278 = `0x1A30E`，与 `+0x14` 一致）。不同槽长度不同，说明后部不是定长填满。

对象模型：几乎所有模拟状态继承 `CSaveObject -> CObject`（人物、城镇、势力、舰队、事件、遗迹、文化、战斗结果…）。HD 的 `CSaveDataFile` 体积 `0x120`，`+0x118=3`（`[未知]`）。`CFile` 是 16 字节文件句柄包装。

编码：SC 姓名 GBK（代码页 936）。其它语言存档目录分开，**不**把四种语言写进同一文件。

## 3. 实体与 ID

| ID | 含义 |
| --- | --- |
| `slot_id` | 0–10 对应 `savedatXX`；`sidecar` 表示 `savedata.dk4` |
| `save_schema_id` | `dk4.save` |
| `save_version` | 整数，破坏性变更 +1 |
| `content_pack_id` | 如 `vanilla-world0` |
| `formula_pack_version` | 读档后用**当前**公式重算派生值 |
| `language_id` | `zh-CN` 等，见 `03-localization.md` |
| `scenario_id` | 剧本 |

哨兵：空槽 = 文件不存在，不是写一圈 0。导入失败 = 拒绝读档，不 fallback 进残局。

## 4. 算法 / 公式

无玩法公式。持久化规则：

```
write:
  header { magic, schema, version, packs, language, scenario, stored_size }
  for sys in systems:
    sys.export_state() -> section
  派生值（等级、技能）不写，或写 cache 并标 dirty

read:
  校验 magic/version/packs
  分发 section
  Progression.RebuildDerived(all persons)
  Scene.Enter(saved scene_id)     # 非法 scene 则进 title 并报错，不猜
```

原版兼容：

```
import_original(path):
  校验 "DAIKOUKAI4PKW9 1"
  按 save_format.md 已命名偏移抽出确认字段
  未命名字节进 original_unknown[]
  经验 → 人物状态；丢弃 +0x70 缓存，重算
```

导出回原版格式**不是**引擎职责；需要的话做独立工具，缺字段不得瞎填。

饱和：导入时单轨经验若大到原版会显示 0，按 `overflow: saturate` 收进 `level_per_track`（见成长文档），并记警告。

## 5. 耦合分析

原版：一份 blob 里名册、主角体、世界、经济全按偏移挤在一起；状态画面读 `0x35AA` 而名单读 `0x2C00`，改一处漏一处就裂。技能不存却把等级缓存存了。

重构：一节一个系统。人物节只存 `person_id`、六维、两轨经验、派系、熟练技能 id。城镇发展度在 world/factions 节。UI 节只存 `scene_id` 与叠加栈。

公式不进存档，避免旧档锁死数值。

## 6. 解耦后的配置 / 新 schema

运行时文件建议：

```
<user>/DK4HD/saves/<language_id>/slot-00.json   # 或等价带长度前缀的二进制
<user>/DK4HD/saves/<language_id>/session.json    # 替代 savedata.dk4
```

JSON 便于对照；正式包可用同 schema 的二进制。两者必须能互转。

```yaml
# schema 说明（不是内容包）
schema: dk4.save
version: 1
magic: DK4HDSAVE1              # 新魔法数，与 DAIKOUKAI4PKW9 1 区分
```

```json
{
  "schema": "dk4.save",
  "version": 1,
  "content_pack": "vanilla-world0",
  "formula_pack_version": 1,
  "language_id": "zh-CN",
  "scenario_id": 5,
  "saved_at": "2026-11-02T20:23:00Z",
  "meta": {
    "display_name": "亚伯拉罕·伍丁·易文",
    "total_level_cache": 510
  },
  "session": {
    "scene_id": "ocean",
    "town_id": null,
    "fleet_id": 0,
    "overlays": []
  },
  "progression": {
    "good_skills": [2, 4, 6],
    "discoveries": { "villages": [], "remains": [], "items": [] },
    "titles": [],
    "fame": null
  },
  "characters": {
    "protagonist_person_id": 5,
    "roster": [
      {
        "slot": 0,
        "person_id": 5,
        "force_id": 13,
        "stats": [255, 255, 255, 255, 255, 255],
        "exp": [1950750, 1950750]
      }
    ]
  },
  "world": {},
  "factions": {},
  "economy": {},
  "ships": {},
  "fleets": {},
  "ocean": {},
  "combat": {},
  "town": {},
  "items": {},
  "narrative": {},
  "original_unknown": null
}
```

空对象表示该系统尚未还原字段时仍占节，避免以后插节改偏移。JSON 无偏移问题；二进制实现用 TLV / 长度前缀，禁止再按 `0x2C00` 写死。

`meta.total_level_cache` 可选；读档忽略，只给存档列表 UI 当预览。列表 UI 也可以只读 `characters.roster[protagonist]` 现场算。

导入器映射（原版 → 新）：

| 原版 | 新字段 |
| --- | --- |
| `+0x20` | `characters.protagonist_person_id`（再经剧本表对到 `person_id`） |
| 名册 / 主角体 | `characters.roster[]`（两份经验取主角体为源，名册行必须一致，否则导入警告并以主角体为准） |
| `+0x70` | 丢弃或写入 cache |
| 名字块 index 0 | `characters.roster[prot].name` |
| 其余字节 | `original_unknown` hex |

vanilla 导入后 `formula_pack_version` 设为当前包，不设为「存档当时 exe」。

## 7. 独立旋钮

| 旋钮 | 说明 |
| --- | --- |
| 槽数 | 默认 11；加大不必改人物系统 |
| 路径模板 | `{docs}/KoeiTecmo/DK4HD/Savedata/{language_id}/` 仅作导入默认；新档可用新根 |
| `save_version` | 加字段只升小版本；拆节升主版本 |
| 是否写 cache | 可关 |
| `original_unknown` 保留 | 关则导入丢未命名字节（不可逆） |

显示名预览宽度由 UI 自己截，不倒逼姓名字节 17。

## 8. 对外接口

```
Save.list(language_id) -> [SlotInfo]
Save.write(slot_id)
Save.read(slot_id) -> Result
Save.import_original(path) -> Result
Save.delete(slot_id)
Session.get() -> { language_id, scenario_id, scene_id }
Session.set_language(language_id)     # 不改玩法状态
```

`SlotInfo`：`slot_id`、是否空、`display_name`、`scenario_id`、`total_level`（派生）、时间。

事件：`SaveWritten` `SaveLoaded` `SaveRejected`。

各系统实现 `export_state` / `import_state`，参数为 ID 与 POD，禁止塞 `CSaveObject*`。

## 9. 未核实字段

- 槽文件 `+0x10`、`+0x18` 起至 `+0x2C00` 之间除 `+0x20` `+0x70` 外几乎全部
- 名册每行 `0x19` 里除 `+0x03/05/0B/0F` 外的字节
- `0x3592`（名册末）到 `0x35AA` 的间隙
- `0x35BA` 到 `0x35C3` 的间隙
- 名字块 `0x2E9` 里 17×3 姓名之外的 ~694 字节（是否含肖像、生日、熟练技能位）
- `savedata.dk4` 256 字节布局
- `CSaveDataFile+0x118=3`
- 存档是否含当前 `scene_id` / 坐标 / 日期（`CPlayerStatusView` 能改月日，日期必在某处）
- 校验和（样例 `+0x18` 一带不像简单长度）
- 四语言存档是否可互读（目录分开；编码不同则姓名字段不能当同一 GBK）
- `CSaveObject` 序列化顺序与块边界
