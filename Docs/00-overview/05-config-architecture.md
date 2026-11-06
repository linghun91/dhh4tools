# 配置架构

## 目录

运行时只读 `config/`。本 Docs 里的 YAML 是规格示例，真正的表由内容管线从原版 DLL / mesfile 生成后再人工补名字。

```
config/
  caps.yaml
  engine_limits.yaml
  formulas/
    progression.yaml
    economy.yaml
    combat.yaml
    ocean.yaml
    character.yaml
  ids/
    generated.json          # 构建时从各表收集
  world/
    map.yaml                # World.dat 引用 + 坐标系
    sea_areas.yaml
    towns.yaml
    villages.yaml
    remains.yaml
    routes.yaml
    junctions.yaml
    climate.yaml
  factions/
    countries.yaml
    cultures.yaml
    forces.yaml
    relations.yaml
    fashion.yaml
  characters/
    abilities.yaml
    skills.yaml
    jobs.yaml
    posts.yaml
    persons.yaml
    protagonists.yaml
  economy/
    trading_item_kinds.yaml
    trading_items.yaml
    special_products.yaml
    price_curves.yaml
    investment.yaml
  ships/
    types.yaml
    hulls.yaml
    cabins.yaml
    guns.yaml
    shells.yaml
    figureheads.yaml
    supplies.yaml
  fleets/
    templates.yaml
    proxy_rules.yaml
    formations.yaml
  combat/
    naval.yaml
    boarding.yaml
    ai.yaml
  ocean/
    sailing.yaml
    wind.yaml
    search.yaml
    encounters.yaml
    gold_road.yaml
  town/
    facilities.yaml
    requests.yaml
    schemes.yaml
    minigames.yaml
  items/
    kinds.yaml
    items.yaml
    effects.yaml
  narrative/
    scenarios.yaml
    events.yaml
    triggers.yaml
  progression/
    level.yaml
    fame.yaml
  ui/
    scenes.yaml
    strings.zh-CN.yaml      # 只文案
  localization/
    languages.yaml
```

## 加载顺序

1. `engine_limits.yaml` — 实现硬顶
2. `caps.yaml` — 设计上限，必须 ≤ 硬顶
3. `formulas/`
4. 内容表（可并行，外键在校验阶段检查）
5. `narrative/scenarios.yaml` 选择本次运行的世界包
6. 存档覆盖状态字段

校验失败则拒绝启动。缺字段不回退到代码默认值（原版「双轨 fallback」禁止）。

## 内容包

```
packs/
  vanilla-world0/     # 对应 World0.dll
  vanilla-world1/     # 对应 World1.dll
  vanilla-sc-text/    # mesfile SC
```

剧本记录：

```yaml
# config/narrative/scenarios.yaml
- id: 0
  key: raphael
  world_pack: vanilla-world0
  event_pack: EV0
  protagonist_person_id: 0
```

## ID 别名

整数 ID 保持与原版枚举一致，便于对照存档与 DLL。同时给稳定 key：

```yaml
- id: 0
  key: london
  name: 伦敦
```

模组追加时从 `id >= 1000` 或使用独立命名空间，避免覆盖原版 0–96。

## Schema 版本

每个 YAML 头：

```yaml
schema: dk4.world.towns
version: 1
```

引擎按 `schema` 选解析器。加字段必须升 version，旧包可只读。

## 生成管线（建议）

```
World0.dll ──┐
mesfile.dk4 ─┼─→ tools/extract_content.py → packs/vanilla-*/ ─→ 人工补 key/注释
World.dat  ──┘
```

提取器只输出「确认」字段。hex 残余进 `unknown_bytes` 数组，等字段命名后再升级 schema。

## 与存档的关系

存档存：

- 内容包 id + schema version
- 各系统状态（见 `12-ui-save/02-save-session.md`）
- 不存公式原文（避免旧存档锁死数值）；存「公式包版本」，读档时用当前包重算派生值（技能、等级缓存）

原版技能不存、等级有缓存（`save+0x70`）。重构：经验是源，等级/技能是派生，缓存可有但必须能从源重建。
