# 引擎化改造设计文档

> **目标**：将「钢铁意志：第三帝国」从硬编码互动叙事游戏改造为 JSON 数据驱动的通用互动叙事引擎，支持 DLC 文件夹即插即用。
>
> **日期**：2026-06-15
>
> **状态**：已确认

---

## 一、需求摘要

| 维度 | 决策 |
|------|------|
| DLC 内容格式 | JSON 数据文件（创作者无需懂 C++） |
| 玩法框架 | 同一类型（回合式文字冒险 + 骰子判定），任意题材 |
| DLC 分发形式 | 单个文件夹，放入 `dlc/` 目录，引擎自动扫描加载 |
| 现有内容处理 | 5 个战役转为 JSON DLC，引擎纯净化 |
| 定制范围 | 最小可行：剧本数据 + 职业定义 + 背景音乐 |
| 职业系统 | 每个 DLC 独立定义职业，DLC 之间不互通 |
| DLC 规模 | 一个 DLC = 一个完整战役（多个章节串联） |
| 章节推进 | 线性解锁（完成章节 1 解锁章节 2） |
| 创作工具 | 纯手写 JSON，不做可视化编辑器 |

---

## 二、引擎模块架构

改造后引擎由 **5 个独立模块** 组成，替代当前单一的 `GameEngine` + 硬编码场景。

### 2.1 模块总览

```
main.cpp
  └── MainWindow (UI 路由)
        ├── DlcManager     ← 扫描 dlc/ 目录，解析 manifest.json
        ├── NodeEngine     ← 故事节点导航 + 选择执行 + 章节跳转
        ├── PlayerSystem   ← 动态职业 + 属性管理 + 标志位
        ├── DiceSystem     ← D100 骰子判定
        ├── SaveManager    ← JSON 存档（适配新格式）
        └── MusicPlayer    ← 背景音乐管理
```

### 2.2 模块职责

#### DlcManager `engine/DlcManager.h/.cpp`

- 启动时扫描 `dlc/` 目录
- 解析每个 DLC 的 `manifest.json`
- 校验 JSON 合法性（节点 ID 唯一性、引用完整性）
- 提供已加载的 DLC 列表给 UI
- **替代**：`registerScenarios()`、`ScenarioId` 枚举

#### NodeEngine `engine/NodeEngine.h/.cpp`

- 当前节点定位与导航
- 执行 Choice（骰子判定/叙事/分支）
- 章节间跳转（根据 manifest 中 chapter 的 `unlock` 字段）
- 标志位检查与设置
- **替代**：`GameEngine` 的场景导航部分

#### PlayerSystem `engine/PlayerSystem.h/.cpp`

- 从 DLC manifest 读取职业定义，存入运行时结构
- 属性管理（HP/士气，可扩展）
- 标志位集合
- 存档序列化/反序列化
- **替代**：`Player` 类重构 + `PlayerClass` 枚举移除

#### DiceSystem `engine/DiceSystem.h/.cpp`

- D100 随机数生成（`[1, 100]`）
- 职业加成判定（匹配 `bonusClasses` 字段 +20）
- 阈值比较
- **替代**：`GameEngine::rollDice()`

#### SaveManager `engine/SaveManager.h/.cpp`

- 保留现有 1 自动存档 + 3 手动存档位
- JSON 序列化适配新格式（字符串 ID 替代枚举 int 值）
- 新增 `engineVersion` 和 `dlcId` 字段

#### MusicPlayer `core/MusicPlayer.h/.cpp`

- 基本不变，仍通过 `musicKey` 映射到文件路径
- 路径从 manifest 的 `music` map 中解析

### 2.3 数据结构 `engine/DlcTypes.h`

```cpp
// DLC 职业定义（运行时）
struct DlcClass {
    QString id;       // 字符串ID，如 "infantry"
    QString name;     // 显示名称，如 "步兵"
    QString desc;     // 描述文本
};

// 音乐映射
struct DlcMusicEntry {
    QString key;      // 如 "intro", "battle"
    QString file;     // 相对于 DLC 文件夹的路径
};

// 章节元信息
struct DlcChapterMeta {
    QString id;       // 如 "fall_gelb"
    QString file;     // JSON 文件路径（相对于 DLC 文件夹）
    QString name;     // 显示名称
    QString subtitle; // 副标题（日期等）
    QString unlock;   // 解锁条件（字符串："start" 或 chapter id）
};

// DLC 清单
struct DlcManifest {
    QString dlcId;
    QString title;
    QString subtitle;
    QString author;
    QString version;
    QList<DlcClass> classes;
    QList<DlcChapterMeta> chapters;
    QString startChapter;
    QMap<QString, QString> music;  // key → 相对文件路径
    QMap<QString, QString> musicByKey; // 同上，按 key 索引
};

// 单个章节（从 JSON 加载）
struct DlcChapter {
    QString chapterId;
    QString startNodeId;
    QString defeatNodeId;       // 可选
    QMap<QString, StoryNode> nodes;
};

// StoryNode 和 Choice 结构保持现有定义（StoryNode.h），
// 但 Choice 中的 classRestricted/allowedClasses 改用 QString 而非 PlayerClass 枚举
```

---

## 三、JSON 数据格式

### 3.1 manifest.json

```json
{
  "dlcId": "third_reich",
  "title": "钢铁意志：第三帝国的黄昏",
  "subtitle": "一部关于战争残酷与无义的文字悲剧",
  "author": "Official",
  "version": "1.0.0",

  "classes": [
    { "id": "infantry",      "name": "步兵",           "desc": "在前线堑壕中奋战" },
    { "id": "tank_crew",     "name": "坦克兵",         "desc": "驾驭钢铁巨兽" },
    { "id": "fighter_pilot", "name": "战斗机飞行员",   "desc": "驾机鏖战长空" },
    { "id": "bomber_pilot",  "name": "轰炸机飞行员",   "desc": "投下毁灭之雨" },
    { "id": "submarine",     "name": "潜艇驾驶员",     "desc": "深海猎手" },
    { "id": "battleship",    "name": "战列舰驾驶员",   "desc": "巨舰大炮的荣耀" }
  ],

  "chapters": [
    { "id": "fall_gelb",  "file": "chapters/ch01_fall_gelb.json",
      "name": "黄色方案",   "subtitle": "1940年5月 · 阿登森林",     "unlock": "start" },
    { "id": "britain",    "file": "chapters/ch02_britain.json",
      "name": "不列颠空战", "subtitle": "1940年7月 · 英吉利海峡",   "unlock": "fall_gelb" },
    { "id": "wolf_pack",  "file": "chapters/ch03_wolf_pack.json",
      "name": "群狼海战",   "subtitle": "1941年 · 大西洋",          "unlock": "britain" },
    { "id": "stalingrad", "file": "chapters/ch04_stalingrad.json",
      "name": "斯大林格勒", "subtitle": "1942年8月 · 伏尔加河畔",   "unlock": "wolf_pack" },
    { "id": "berlin",     "file": "chapters/ch05_berlin.json",
      "name": "柏林战役",   "subtitle": "1945年4月 · 帝国的黄昏",   "unlock": "stalingrad" }
  ],

  "startChapter": "fall_gelb",

  "music": {
    "intro":       "music/main_theme.mp3",
    "fall_gelb":   "music/fall_gelb.mp3",
    "britain":     "music/britain.mp3",
    "wolf_pack":   "music/wolf_pack.mp3",
    "stalingrad":  "music/stalingrad.mp3",
    "berlin":      "music/berlin_theme.mp3",
    "victory":     "music/victory.mp3",
    "defeat":      "music/defeat_theme.mp3"
  }
}
```

### 3.2 chapter JSON 格式

```json
{
  "chapterId": "fall_gelb",
  "startNodeId": "fg_intro",
  "defeatNodeId": "fg_defeated",

  "nodes": [
    {
      "id": "fg_intro",
      "type": "narrative",
      "locationTitle": "1940年5月10日 · 阿登森林",
      "musicKey": "fall_gelb",
      "text": "1940年5月10日凌晨。...",
      "nextNodeId": "fg_choice_1"
    },
    {
      "id": "fg_choice_1",
      "type": "choice",
      "locationTitle": "阿登森林 · 前线堑壕",
      "musicKey": "fall_gelb",
      "text": "前方传来密集的枪声...",
      "choices": [
        {
          "text": "【冲锋】跟随班长冲出堑壕",
          "isCombat": true,
          "combatThreshold": 50,
          "bonusClasses": ["infantry"],
          "successNodeId": "fg_rush_success",
          "failureNodeId": "fg_rush_fail",
          "hpDelta": 0,
          "moraleDelta": 10,
          "failHpDelta": -25,
          "failMoraleDelta": -10
        },
        {
          "text": "【隐蔽】留在堑壕里掩护战友",
          "nextNodeId": "fg_cover_fire"
        }
      ]
    },
    {
      "id": "fg_victory",
      "type": "ending",
      "isVictory": true,
      "locationTitle": "黄色方案 — 结束",
      "text": "..."
    }
  ]
}
```

### 3.3 节点类型

| type | 说明 | 必要字段 |
|------|------|----------|
| `narrative` | 纯叙事，一个"继续"按钮 | `nextNodeId` |
| `choice` | 玩家做出选择（含战斗判定） | `choices[]` |
| `ending` | 终止节点 | `isVictory` 或 `isDefeat` |

### 3.4 Choice 字段说明

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `text` | string | ✅ | 按钮显示文字 |
| `nextNodeId` | string | 非战斗时必填 | 默认跳转目标 |
| `hpDelta` | int | 否 | 生命变化 |
| `moraleDelta` | int | 否 | 士气变化 |
| `requiredFlags` | string[] | 否 | 需要拥有这些标志才能显示此选项 |
| `grantedFlags` | string[] | 否 | 选择后设置的标志 |
| `classRestricted` | bool | 否 | 是否职业限制 |
| `allowedClasses` | string[] | classRestricted=true 时必填 | 允许的职业 ID 列表 |
| `isCombat` | bool | 否 | 是否为战斗选项 |
| `combatThreshold` | int | isCombat=true 时必填 | 成功阈值 [1,100] |
| `bonusClasses` | string[] | 否 | 这些职业骰子 +20 |
| `successNodeId` | string | isCombat=true 时必填 | 成功跳转 |
| `failureNodeId` | string | isCombat=true 时必填 | 失败跳转 |
| `failHpDelta` | int | 否 | 失败时生命变化 |
| `failMoraleDelta` | int | 否 | 失败时士气变化 |

---

## 四、迁移策略

### 4.1 删除的代码

| 文件 | 原因 |
|------|------|
| `scenarios/` 整个目录 (10 个文件) | 5 个 C++ 场景类 → JSON DLC |
| `GameState.h` 中的 `PlayerClass` 枚举 | 改为 DLC JSON 动态定义 |
| `GameState.h` 中的 `ScenarioId` 枚举 | 改为 DLC JSON 动态定义 |
| `GameState.h` 中的 `ScenarioIdHash` | 不再需要 |
| `GameState.h` 中的 4 个辅助函数 | 不再需要 |
| `GameEngine::registerScenarios()` | 改为 DlcManager::scanDirectory() |
| `ScenarioBase.h/.cpp` | 被 NodeEngine 替代 |
| `GameEngine` 中的场景导航逻辑 | 迁移到 NodeEngine |

### 4.2 新增的代码

| 文件 | 说明 |
|------|------|
| `engine/DlcTypes.h` | 运行时数据结构 |
| `engine/DlcManager.h/.cpp` | DLC 扫描/加载/校验 |
| `engine/NodeEngine.h/.cpp` | 节点引擎 |
| `engine/PlayerSystem.h/.cpp` | 动态职业系统 |
| `engine/DiceSystem.h/.cpp` | 骰子系统 |
| `dlc/third_reich/manifest.json` | 官方 DLC 元信息 |
| `dlc/third_reich/chapters/*.json` | 5 个章节（从 C++ 转换） |
| `dlc/third_reich/music/` | 复制现有音乐资源 |

### 4.3 修改的代码

| 文件 | 变化 |
|------|------|
| `main.cpp` | 初始化 DlcManager |
| `CMakeLists.txt` | 新增 engine/ 文件，移除 scenarios/ |
| `core/StoryNode.h` | Choice 中职业字段改为 `QString` |
| `core/Player.h/.cpp` | 重构为 PlayerSystem，职业字段改为 `QString` |
| `core/SaveManager.h/.cpp` | 适配新存档格式 |
| `ui/MainWindow.h/.cpp` | 新增 DLC 选择步骤 |
| `ui/CharacterCreateWidget.h/.cpp` | 动态职业列表（从 DLC manifest 读取） |
| `ui/GameWidget.h/.cpp` | 适配 NodeEngine 接口 |
| `ui/MenuWidget.h/.cpp` | DLC 选择 + 角色创建流程 |
| `ui/SaveLoadDialog.h/.cpp` | 显示 dlcId，按 DLC 过滤存档 |
| `resources/resources.qrc` | 移除场景相关资源引用 |

### 4.4 存档格式变化

新版存档新增字段：
- `engineVersion`: `"2.0"` — 引擎版本标识
- `dlcId`: `"third_reich"` — 标识所属 DLC
- `playerClass`: `"infantry"` — 字符串 ID 替代枚举 int
- `currentChapter`: `"stalingrad"` — 替代 `currentScenario`
- `unlockedChapters`: `["fall_gelb", ...]` — 字符串数组替代 int 数组

**不兼容旧存档。** 引擎启动时检测到旧格式提示用户。旧存档标记为 legacy，不自动迁移。

---

## 五、UI 适配

### 5.1 主流程改造

```
旧流程: 主菜单 → 角色创建(6个固定职业) → 场景选择(5个固定场景) → 游戏
新流程: 主菜单 → DLC选择(扫描dlc/列表) → 角色创建(动态职业) → 游戏(线性推进章节)
```

### 5.2 各界面变化

| 界面 | 变化 |
|------|------|
| **主菜单** | 新增"选择 DLC"步骤，或合并到 DLC 列表页 |
| **角色创建** | 职业卡片动态生成，数量和内容来自 DLC manifest.classes[] |
| **场景选择** | 不再需要独立页面，章节由引擎按 unlock 字段线性解锁 |
| **游戏界面** | 数据来源从 C++ 对象切换为 NodeEngine 的 JSON 加载 |
| **存档界面** | 存档列表显示 `dlcId`，可考虑按 DLC 过滤 |

### 5.3 DLC 选择方式

首次进入时展示已安装的 DLC 列表（名称 + 副标题 + 作者），用户选择一个进入。之后角色创建 → 开始游戏。再次进入时默认上次选择的 DLC。

---

## 六、校验规则

DlcManager 加载 DLC 时执行以下校验：

1. `manifest.json` 必须存在且为合法 JSON
2. `dlcId` 必须全局唯一（同目录下无重复）
3. `classes[]` 非空，每个 class 有唯一的 `id`
4. `chapters[]` 非空，每个 chapter 有唯一的 `id`
5. `startChapter` 必须对应一个已定义的 chapter
6. 非起始 chapter 的 `unlock` 必须引用一个已定义的 chapter
7. 每个 chapter 的 `file` 指向的 JSON 文件必须存在
8. Chapter JSON 中 `startNodeId` 必须对应一个已定义的节点
9. 所有节点的 `nextNodeId` / `successNodeId` / `failureNodeId` 必须引用存在的节点（或为合法的特殊 ID）
10. `choices[]` 中 `classRestricted=true` 时 `allowedClasses` 非空
11. `isCombat=true` 时 `successNodeId` 和 `failureNodeId` 必填

校验失败时，该 DLC 被标记为不可用，在 DLC 列表中灰显并附错误信息。

---

## 七、目录结构（改造后）

```
Hearts of iron/
├── CMakeLists.txt
├── main.cpp
├── engine/                    ← 通用引擎层（不依赖任何 DLC）
│   ├── DlcTypes.h
│   ├── DlcManager.h/.cpp
│   ├── NodeEngine.h/.cpp
│   ├── PlayerSystem.h/.cpp
│   ├── DiceSystem.h/.cpp
│   ├── SaveManager.h/.cpp     ← 从 core/ 迁移
│   └── MusicPlayer.h/.cpp     ← 从 core/ 迁移
├── ui/                        ← 通用 UI 层
│   ├── MainWindow.h/.cpp
│   ├── MenuWidget.h/.cpp
│   ├── GameWidget.h/.cpp
│   ├── CharacterCreateWidget.h/.cpp
│   └── SaveLoadDialog.h/.cpp
├── dlc/                       ← 🔑 创作者目录
│   └── third_reich/           ← 官方 DLC（现有内容 JSON 化）
│       ├── manifest.json
│       ├── chapters/
│       │   ├── ch01_fall_gelb.json
│       │   ├── ch02_britain.json
│       │   ├── ch03_wolf_pack.json
│       │   ├── ch04_stalingrad.json
│       │   └── ch05_berlin.json
│       └── music/
├── resources/
│   ├── resources.qrc
│   └── style.qss
└── docs/
    └── dlc-author-guide.md    ← DLC 创作文档
```

---

## 八、范围限定

### 本期做

- [x] engine/ 5 个模块的 C++ 实现
- [x] 现有 5 个场景转换为 JSON（third_reich DLC）
- [x] UI 适配（DLC 选择 + 动态职业 + 引擎对接）
- [x] 存档格式升级
- [x] JSON 校验规则
- [x] DLC 创作文档

### 本期不做

- [ ] 可视化编辑器
- [ ] 章节间分支跳转（仅做线性解锁）
- [ ] Steam Workshop 集成
- [ ] DLC 压缩包(.pak)加载
- [ ] 内部脚本语言
- [ ] 自定义 UI 皮肤/主题
- [ ] 自定义骰子系统参数
