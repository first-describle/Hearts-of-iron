# 互动叙事引擎

> *万物皆有其终章，唯故事长存。*

---

## 目录

- [项目概述](#项目概述)
- [技术栈](#技术栈)
- [构建与运行](#构建与运行)
- [项目结构](#项目结构)
- [引擎架构](#引擎架构)
  - [DlcManager — DLC 管理](#dlcmanager--dlc-管理)
  - [NodeEngine — 节点引擎](#nodeengine--节点引擎)
  - [PlayerSystem — 玩家系统](#playersystem--玩家系统)
  - [DiceSystem — 骰子系统](#dicesystem--骰子系统)
  - [SaveManager — 存档管理](#savemanager--存档管理)
  - [MusicPlayer — 音乐管理](#musicplayer--音乐管理)
- [游戏系统](#游戏系统)
  - [骰子判定](#骰子判定)
  - [叙事标志](#叙事标志)
  - [打字机效果](#打字机效果)
  - [自动存档](#自动存档)
- [官方 DLC：钢铁意志——第三帝国的黄昏](#官方-dlc钢铁意志第三帝国的黄昏)
- [DLC 创作指南](#dlc-创作指南)
  - [快速开始](#快速开始)
  - [文件夹结构](#文件夹结构)
  - [manifest.json 参考](#manifestjson-参考)
  - [chapter JSON 参考](#chapter-json-参考)
  - [节点类型](#节点类型)
  - [Choice 字段速查](#choice-字段速查)
  - [校验规则](#校验规则)
  - [创作建议](#创作建议)
- [UI 界面](#ui-界面)
- [美术风格](#美术风格)
- [存档格式](#存档格式)
- [已知局限与未来改进](#已知局限与未来改进)

---

## 项目概述

**互动叙事引擎** 是一个 JSON 数据驱动的通用文字冒险游戏框架。引擎本身不绑定任何特定题材，所有故事内容——职业、章节、剧情文本、战斗判定——均由 **DLC 数据包** 定义。

创作者只需编写 JSON 文件并放入 `dlc/` 目录，引擎启动时自动扫描加载，无需编译任何 C++ 代码。

### 引擎特性

- **DLC 即插即用**：一个文件夹 = 一个完整游戏，丢入 `dlc/` 即生效
- **动态职业系统**：每个 DLC 自行定义职业，互不干扰
- **多章节分支**：章节间通过 `unlock` 字段串接，支持线性或多分支推进
- **D100 骰子战斗**：基于阈值的随机判定，职业匹配获得 +20 加成
- **叙事标志系统**：玩家的选择留下永久标志，影响后续选项可见性
- **打字机文本渲染**：逐字打印的故事叙述
- **多槽位存档**：1 个自动存档位 + 3 个手动存档位，JSON 格式持久化

### 官方 DLC

项目附带 **钢铁意志：第三帝国的黄昏** 作为官方 DLC 示例——以二战德军士兵视角，经历五个历史战役的文字悲剧。

---

## 技术栈

| 层级 | 技术 |
|------|------|
| 语言 | C++17 |
| GUI 框架 | Qt 6 (Core / Widgets / Multimedia) |
| 构建系统 | CMake 3.16+ |
| 音频 | QMediaPlayer + QAudioOutput |
| 数据序列化 | QJsonDocument / QJsonObject |
| 样式 | Qt Style Sheets (QSS) |

---

## 构建与运行

### 前置依赖

- **Qt 6** (Core + Widgets + Multimedia 模块)
- **CMake 3.16+**
- **支持 C++17 的编译器** (MSVC 2019+, GCC 9+, Clang 10+)

### 构建步骤

```bash
# 1. 进入项目根目录
cd "Hearts of iron"

# 2. CMake 配置（指定你的 Qt6 安装路径）
cmake -B build -S . -DCMAKE_PREFIX_PATH="你的Qt6安装路径"

# 3. 编译
cmake --build build --config Release

# 4. 部署 Qt 运行时 DLL
windeployqt build/Release/钢铁意志_第三帝国.exe

# 5. 运行
./build/Release/钢铁意志_第三帝国.exe
```

构建完成后，`dlc/` 目录会自动复制到可执行文件旁边，引擎启动时扫描加载。

---

## 项目结构

```
Hearts of iron/
├── CMakeLists.txt
├── main.cpp                       # 应用入口
├── engine/                        # 通用引擎层（不依赖任何 DLC）
│   ├── DlcTypes.h                 # 运行时数据结构定义
│   ├── DlcManager.h / .cpp       # DLC 扫描、加载、校验
│   ├── NodeEngine.h / .cpp       # 故事节点导航与选择执行
│   ├── PlayerSystem.h / .cpp     # 动态职业玩家状态管理
│   ├── DiceSystem.h / .cpp       # D100 骰子判定
│   ├── SaveManager.h / .cpp      # JSON 存档管理
│   └── MusicPlayer.h / .cpp      # 背景音乐管理
├── ui/                            # 通用 UI 层
│   ├── MainWindow.h / .cpp        # 主窗口与路由控制
│   ├── MenuWidget.h / .cpp        # 主菜单 + DLC 选择
│   ├── CharacterCreateWidget.h/.cpp # 角色创建（动态职业卡片）
│   ├── GameWidget.h / .cpp        # 游戏主界面（含打字机效果）
│   └── SaveLoadDialog.h / .cpp    # 存档/读档对话框
├── dlc/                           # 🔑 DLC 内容目录（创作者只需关心这里）
│   └── third_reich/               # 官方 DLC 示例
│       ├── manifest.json           # DLC 元信息 + 职业定义 + 章节列表
│       ├── chapters/               # 各章节 JSON 文件
│       │   ├── ch01_fall_gelb.json
│       │   ├── ch02_britain.json
│       │   ├── ch03_wolf_pack.json
│       │   ├── ch04_stalingrad.json
│       │   └── ch05_berlin.json
│       └── music/                  # 背景音乐文件
├── resources/
│   ├── resources.qrc
│   └── style.qss                   # 全局 QSS 样式表
└── docs/
    └── superpowers/
        ├── specs/2026-06-15-engine-refactor-design.md
        └── plans/2026-06-15-engine-refactor.md
```

---

## 引擎架构

引擎由 **6 个独立模块** 组成，全部位于 `engine/` 目录。模块之间通过明确的接口解耦，UI 层通过 Qt 信号槽与引擎通信。

```
main.cpp
  └── MainWindow (UI 路由)
        ├── DlcManager     ← 扫描 dlc/ 目录，解析 manifest.json
        ├── NodeEngine     ← 故事节点导航 + 选择执行 + 章节跳转
        ├── PlayerSystem   ← 动态职业 + 属性管理 + 标志位
        ├── DiceSystem     ← D100 骰子判定
        ├── SaveManager    ← JSON 存档
        └── MusicPlayer    ← 背景音乐管理
```

### DlcManager — DLC 管理

**文件：** `engine/DlcManager.h`, `engine/DlcManager.cpp`

启动时扫描 `dlc/` 目录，递归查找每个子文件夹中的 `manifest.json`，解析并校验。

| 方法 | 说明 |
|------|------|
| `scanDirectory(path)` | 扫描目录，解析所有 manifest.json |
| `getManifest(dlcId)` | 按 ID 获取 DLC 清单 |
| `loadChapter(basePath, file, out)` | 加载单个章节 JSON 到运行时结构 |
| `manifests()` | 获取所有已扫描的 DLC 列表 |

**校验项（共 11 条）：** dlcId/title 非空、classes[] 非空、chapters[] 非空、class id 唯一性、chapter id 唯一性、章节文件存在性、startChapter 有效性、unlock 引用完整性。校验失败的 DLC 在菜单中灰显并提示错误。

### NodeEngine — 节点引擎

**文件：** `engine/NodeEngine.h`, `engine/NodeEngine.cpp`

核心玩法循环：载入 DLC → 导航节点 → 处理选择 → 战斗判定 → 章节推进。

| 方法 | 说明 |
|------|------|
| `startDlc(manifest, basePath, player)` | 初始化 DLC，进入起始/存档章节 |
| `startChapter(chapterId)` | 开始指定章节，重置状态，导航到起始节点 |
| `makeChoice(index)` | 执行选择：Narrative 跳转 / 非战斗应用后果 / 战斗骰子判定 |
| `currentNode()` | 返回当前 StoryNode 指针 |

**信号：** `nodeChanged`, `statsChanged`, `chapterVictory`, `chapterDefeat`, `combatResult`, `flagSet`

### PlayerSystem — 玩家系统

**文件：** `engine/PlayerSystem.h`, `engine/PlayerSystem.cpp`

玩家状态完全由 DLC JSON 定义，无硬编码职业枚举。

```cpp
class PlayerSystem {
    QString name;              // 玩家姓名
    QString classId;           // 职业 ID（字符串，来自 DLC manifest）
    QString dlcId;             // 所属 DLC ID
    int     hp        = 100;
    int     morale    = 100;
    QSet<QString> flags;     // 叙事标志集合
    QString currentChapter;   // 当前章节 ID
    QString currentNodeId;    // 当前节点 ID
    QSet<QString> unlockedChapters; // 已解锁章节
};
```

### DiceSystem — 骰子系统

**文件：** `engine/DiceSystem.h`, `engine/DiceSystem.cpp`

```cpp
int roll()                        // D100 基础掷骰 [1, 100]
int rollWithBonus(bonus, classId) // 职业匹配则 +20，钳制 [1, 120]
static bool checkSuccess(roll, threshold) // roll >= threshold
```

| 阈值 | 无加成成功率 | 有加成成功率 | 典型场景 |
|------|------------|------------|---------|
| 40 | 61% | 81% | 谨慎策略 |
| 50 | 51% | 71% | 标准战斗 |
| 60 | 41% | 61% | 危险行动 |
| 70 | 31% | 51% | 绝望行动 |

### SaveManager — 存档管理

**文件：** `engine/SaveManager.h`, `engine/SaveManager.cpp`

| 槽位 | 用途 | 可手动覆盖 |
|------|------|-----------|
| Slot 0 | 自动存档（每次节点切换自动写入） | ❌ |
| Slot 1-3 | 手动存档 | ✅ |

存储路径：`%APPDATA%/HeartsOfIronGame/saves/save_X.json`

### MusicPlayer — 音乐管理

**文件：** `engine/MusicPlayer.h`, `engine/MusicPlayer.cpp`

通过键-路径映射管理音轨。DLC manifest 中定义 `music` 映射表，引擎启动时批量注册。循环播放、静音跳过、文件缺失静默容错。

---

## 游戏系统

### 骰子判定

所有战斗选项通过 D100 骰子系统随机判定：

```
最终值 = 基础 1-100 随机 + (职业在 bonusClasses 中 ? 20 : 0)
结果：最终值 ≥ combatThreshold → 成功，否则失败
```

### 叙事标志

玩家选择可设置永久标志（`grantedFlags`），后续选项可通过 `requiredFlags` 检查标志来决定是否可见。

### 打字机效果

`QTimer` 每 20ms 逐字打印，打字期间禁用所有交互按钮。点击文本区域可跳过动画直接显示全文。

### 自动存档

每次 `onNodeChanged` 触发时自动写入 Slot 0。

---

## 官方 DLC：钢铁意志——第三帝国的黄昏

以二战德军士兵视角，经历 1940-1945 年五场历史战役的文字悲剧。

| 章节 | 历史背景 | 可用职业 | 节点数 |
|------|---------|---------|--------|
| 黄色方案 | 1940年5月，阿登森林突破 | 步兵、坦克兵 | 28 |
| 不列颠空战 | 1940年7-10月，英吉利海峡 | 战斗机/轰炸机飞行员 | 27 |
| 群狼海战 | 1941-1943年，大西洋 | 潜艇/战列舰驾驶员 | 25 |
| 斯大林格勒战役 | 1942-1943年，伏尔加河畔 | 步兵、坦克兵 | 27 |
| 柏林战役 | 1945年4月，帝国末日 | 全部六种职业 | 22 |

总计 129 个故事节点，15+ 种结局路径。

---

## DLC 创作指南

> **你不需要懂 C++，只需要会写 JSON。**

### 快速开始

1. 在 `dlc/` 下新建文件夹，命名如 `dlc/my_story/`
2. 创建 `manifest.json`（元信息 + 职业 + 章节列表）
3. 创建 `chapters/` 目录，编写各章节 JSON 文件
4. （可选）放入 `music/` 音频文件
5. 启动游戏 → 引擎自动加载 → DLC 列表中出现你的作品

### 文件夹结构

```
dlc/my_story/
├── manifest.json        # 必需：DLC 元信息
├── chapters/            # 必需：章节 JSON 文件
│   ├── ch01.json
│   ├── ch02.json
│   └── ...
└── music/               # 可选：背景音乐
    ├── main_theme.mp3
    └── battle.mp3
```

### manifest.json 参考

```jsonc
{
  // ===== 基本信息 =====
  "dlcId": "my_story",              // 唯一标识（字母+下划线）
  "title": "我的故事",               // 显示标题
  "subtitle": "一段传奇冒险",        // 副标题
  "author": "你的名字",              // 作者署名
  "version": "1.0.0",               // 版本号

  // ===== 职业定义 =====
  "classes": [
    {
      "id": "warrior",              // 职业唯一ID
      "name": "战士",               // 显示名称
      "desc": "勇猛的前线斗士。"     // 描述文本（显示在角色创建预览中）
    },
    {
      "id": "mage",
      "name": "法师",
      "desc": "掌控元素之力。"
    }
    // ... 至少 1 个职业
  ],

  // ===== 章节列表 =====
  "chapters": [
    {
      "id": "ch01",                 // 章节唯一ID
      "file": "chapters/ch01.json", // JSON 文件路径（相对于 DLC 文件夹）
      "name": "第一章",              // 显示名称
      "subtitle": "命运的起点",      // 副标题
      "unlock": "start"             // 解锁条件："start" = 初始可用，
    },                              //             其他章节ID = 完成该章节后解锁
    {
      "id": "ch02",
      "file": "chapters/ch02.json",
      "name": "第二章",
      "subtitle": "黑暗降临",
      "unlock": "ch01"              // 完成 ch01 后解锁
    }
  ],

  // ===== 起始章节 =====
  "startChapter": "ch01",           // 新游戏从哪个章节开始

  // ===== 音乐映射 =====
  "music": {
    "intro": "music/main_theme.mp3",   // 键 → 文件路径（相对于 DLC 文件夹）
    "battle": "music/battle.mp3"
    // 章节节点中通过 musicKey 引用这里的键
  }
}
```

### chapter JSON 参考

```jsonc
{
  "chapterId": "ch01",              // 章节ID
  "startNodeId": "start",           // 起始节点ID
  "defeatNodeId": "defeated",       // 可选：战斗死亡后跳转的失败节点

  "nodes": [
    // ===== Narrative 节点 =====
    {
      "id": "start",
      "type": "narrative",          // 纯叙事 → 一个"继续"按钮
      "locationTitle": "第一章 · 启程",
      "musicKey": "intro",
      "text": "故事从这里开始...\n\n多段落用 \\n\\n 分隔。",
      "nextNodeId": "choice_1"      // 继续后跳转的节点
    },

    // ===== Choice 节点（普通选择） =====
    {
      "id": "choice_1",
      "type": "choice",
      "locationTitle": "十字路口",
      "musicKey": "intro",
      "text": "前方出现两条路，你选择——",
      "choices": [
        {
          "text": "【左转】走向森林",
          "nextNodeId": "forest_path",
          "moraleDelta": 5,          // 士气变化（可选）
          "grantedFlags": ["chose_forest"]  // 设置叙事标志（可选）
        },
        {
          "text": "【右转】走向城镇",
          "nextNodeId": "town_path",
          "hpDelta": -10,            // 生命变化（可选）
          "classRestricted": true,   // 职业限制（可选）
          "allowedClasses": ["warrior"]  // 仅战士可选
        }
      ]
    },

    // ===== Choice 节点（战斗选项） =====
    {
      "id": "battle_1",
      "type": "choice",
      "locationTitle": "遭遇敌人",
      "text": "敌人拦住了去路！",
      "choices": [
        {
          "text": "【正面迎战】",
          "isCombat": true,           // 战斗选项
          "combatThreshold": 50,      // 骰子阈值（>= 此值成功）
          "bonusClasses": ["warrior"],// 这些职业骰子 +20
          "successNodeId": "win",     // 成功跳转
          "failureNodeId": "lose",    // 失败跳转
          "hpDelta": 0,
          "moraleDelta": 10,
          "failHpDelta": -25,        // 失败时生命损失
          "failMoraleDelta": -15     // 失败时士气损失
        },
        {
          "text": "【逃跑】",
          "nextNodeId": "escape",
          "moraleDelta": -10
        }
      ]
    },

    // ===== Ending 节点（胜利） =====
    {
      "id": "victory_end",
      "type": "ending",
      "isVictory": true,
      "locationTitle": "第一章 — 结束",
      "text": "你成功穿越了森林..."
    },

    // ===== Ending 节点（失败） =====
    {
      "id": "defeated",
      "type": "ending",
      "isDefeat": true,
      "locationTitle": "第一章 — 阵亡",
      "text": "你倒在了战场上..."
    }
  ]
}
```

### 节点类型

| type | 说明 | 必要字段 | 界面行为 |
|------|------|---------|---------|
| `narrative` | 纯叙事 | `nextNodeId` | 显示"【继续】"按钮 |
| `choice` | 玩家选择（含战斗） | `choices[]` | 生成选项按钮 |
| `ending` | 终止节点 | `isVictory` 或 `isDefeat` | 胜利→进入下一章/返回主页；失败→返回主页 |

### Choice 字段速查

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `text` | string | ✅ | 按钮显示文字 |
| `nextNodeId` | string | 非战斗时必填 | 跳转目标节点 ID |
| `hpDelta` | int | 否 | 生命变化（正数恢复，负数受伤） |
| `moraleDelta` | int | 否 | 士气变化 |
| `requiredFlags` | string[] | 否 | 需拥有这些标志才能看到此选项 |
| `grantedFlags` | string[] | 否 | 选择后设置的标志 |
| `classRestricted` | bool | 否 | 是否职业限制 |
| `allowedClasses` | string[] | classRestricted=true 时必填 | 允许的职业 ID 列表 |
| `isCombat` | bool | 否 | 是否战斗选项 |
| `combatThreshold` | int | isCombat=true 时必填 | 骰子成功阈值 [1, 100] |
| `bonusClasses` | string[] | 否 | 这些职业掷骰 +20 |
| `successNodeId` | string | isCombat=true 时必填 | 战斗成功跳转 |
| `failureNodeId` | string | isCombat=true 时必填 | 战斗失败跳转 |
| `failHpDelta` | int | 否 | 战斗失败生命损失（默认 -20） |
| `failMoraleDelta` | int | 否 | 战斗失败士气损失（默认 -15） |

### 校验规则

引擎加载 DLC 时自动执行以下检查，不通过则 DLC 灰显不可选：

1. `manifest.json` 必须存在且为合法 JSON
2. `dlcId` 不能为空
3. `title` 不能为空
4. `classes[]` 至少 1 个职业
5. `chapters[]` 至少 1 个章节
6. 所有 class `id` 唯一
7. 所有 chapter `id` 唯一
8. 每个章节的 `file` 指向的 JSON 文件必须存在
9. `startChapter` 必须对应一个已定义的章节
10. 非起始章节的 `unlock` 必须引用已定义的章节（或 `"start"`）
11. 所有节点的跳转目标 ID 必须存在（`nextNodeId` / `successNodeId` / `failureNodeId`）

### 创作建议

- **从官方 DLC 复制改**：打开 `dlc/third_reich/` 目录，复制 `manifest.json` 和一个 chapter JSON 作为模板，改字段值即可
- **节点 ID 用前缀**：如 `ch01_intro`、`ch01_battle`，避免跨章节 ID 冲突
- **先写大纲再填文本**：先用几个简单节点跑通章节流程（start → choice → ending），再逐步填充叙事文本
- **战斗阈值参考**：简单战斗 40-50，普通战斗 50-60，困难战斗 60-70，绝境 70+
- **标志命名**：使用描述性名称如 `saved_village`、`chose_dark_path`，便于记忆和复用
- **测试**：写完丢进 `dlc/` 目录启动游戏即可测试，无需重新编译

---

## UI 界面

### 主菜单

- 标题 + 副标题 + 三个按钮（开始征程 / 继续征程 / 退出征程）
- 点击"开始征程" → 进入 DLC 选择列表
- 点击"继续征程" → 打开存档读取对话框

### DLC 选择

- 自动列出 `dlc/` 下所有已安装的 DLC
- 显示 DLC 标题、副标题、作者
- 校验失败的 DLC 灰显，悬停显示错误详情
- 点击有效 DLC → 进入角色创建

### 角色创建

- 姓名输入（默认 "汉斯 · 缪勒"）
- **动态职业卡片**：根据 DLC manifest 的 `classes[]` 自动生成，2 列网格布局
- 右侧预览面板：生命/士气值 + 职业描述
- 点击卡片高亮选中 → 点击"投身战场"开始游戏

### 游戏主界面

```
┌──────────────────────────────────────────────────┐
│  顶部状态栏                                       │
│  ┌──────────┬──────────────────┬────────────────┐│
│  │ 军官姓名  │  生命: ████░░    │  章节名称      ││
│  │ 兵种      │  士气: ██████   │  当前地点      ││
│  └──────────┴──────────────────┴────────────────┘│
│                                                  │
│  ┌──────────────────────────────────────────────┐│
│  │           中央文本叙事区（打字机效果）         ││
│  └──────────────────────────────────────────────┘│
│                                                  │
│  ┌───────────────────────────┬──────────────────┐│
│  │  选项按钮区               │  保存 / 载入     ││
│  │  [选项1] [选项2]         │  撤回后方        ││
│  └───────────────────────────┴──────────────────┘│
└──────────────────────────────────────────────────┘
```

### 存档对话框

- 4 个槽位（Slot 0 自动存档 + Slot 1-3 手动存档）
- 显示 DLC 名称、玩家姓名、职业、章节、时间戳
- 空槽位显示"无存档数据"
- 自动存档位在保存模式下不可选

---

## 美术风格

**军工战损风（Industrial War-Worn）** — `resources/style.qss`

- 暗黑色系 `#121212` 背景 + 灰白 `#e2e2e2` 文字
- 暗红强调色 `#aa3333`（血/铁锈/战火）
- HP 条血红 `#992222` / 士气条冷蓝 `#226699`
- 战斗选项按钮红色调区分
- 无圆角，军工冷硬直线
- 等宽/工业字体回退

---

## 存档格式

```json
{
  "engineVersion": "2.0",
  "slot": 1,
  "timestamp": "2026-06-15 12:00:00",
  "dlcTitle": "钢铁意志：第三帝国的黄昏",
  "className": "步兵",
  "chapterName": "黄色方案",
  "player": {
    "engineVersion": "2.0",
    "dlcId": "third_reich",
    "playerName": "汉斯 · 缪勒",
    "playerClass": "infantry",
    "hp": 85,
    "maxHp": 100,
    "morale": 90,
    "maxMorale": 100,
    "currentChapter": "fall_gelb",
    "currentNodeId": "fg_village",
    "flags": ["spared_village"],
    "unlockedChapters": ["fall_gelb", "britain"]
  }
}
```

存档按 `dlcId` 区分，读取时自动校验对应 DLC 是否已安装。

---

## 已知局限与未来改进

### 当前局限

1. **章节推进仅支持线性解锁**：完成章节 N → 解锁 N+1，暂不支持 JSON 定义分支跳转关系
2. **无国际化**：所有 UI 文本为中文
3. **窗口固定尺寸**：最小 850×650，未做响应式缩放
4. **音乐需自行准备**：DLC 的 `music/` 目录下 mp3 文件需手动放入，缺失时静默跳过
5. **无可视化编辑器**：纯手写 JSON，需要对照本文档编写
6. **成就/统计系统未实现**

### 未来方向

- 章节间支持任意分支跳转（已在数据模型中预留）
- DLC 压缩包 (.zip) 加载支持
- DLC 自定义 UI 皮肤/主题
- 可配置的游戏规则参数（骰子公式、HP 系统开关等）
- Steam Workshop 集成

---

## 致谢

本引擎的官方 DLC 受以下作品启发：
- 战史著作《第三帝国的兴亡》《斯大林格勒》《大西洋战役》
- 德剧《我们的父辈》(Unsere Mütter, unsere Väter)
- 电影《从海底出击》(Das Boot)、《帝国的毁灭》(Der Untergang)

---

*愿世界永无战争。*
