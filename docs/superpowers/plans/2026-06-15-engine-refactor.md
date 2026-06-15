# 引擎化改造实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将「钢铁意志：第三帝国」从硬编码互动叙事游戏改造为 JSON 驱动的通用互动叙事引擎，支持 DLC 文件夹即插即用。

**Architecture:** 5 个独立引擎模块（DlcManager / NodeEngine / PlayerSystem / DiceSystem / SaveManager）+ 改编 UI 层。现有 5 个 C++ 场景类转为 `dlc/third_reich/` 下的 JSON 数据文件。

**Tech Stack:** C++17, Qt 6 (Core/Widgets/Multimedia), CMake 3.16+, JSON (QJsonDocument)

**Spec reference:** `docs/superpowers/specs/2026-06-15-engine-refactor-design.md`

---

## Phase 1: Foundation — 数据类型重构

### Task 1: Create engine/DlcTypes.h — 运行时数据结构

**Files:**
- Create: `engine/DlcTypes.h`

- [ ] **Step 1: Write DlcTypes.h — 所有引擎运行时数据结构**

```cpp
#pragma once
#include <QString>
#include <QList>
#include <QMap>
#include <QSet>

// ============================================================
//  DlcTypes — 引擎运行时数据结构（不依赖任何具体 DLC）
// ============================================================

// --- 保留的共享枚举（不依赖具体题材） ---

enum class NodeType {
    Narrative,   // 纯叙事，单个"继续"按钮
    Choice,      // 玩家做出选择
    Ending       // 胜利/失败终止节点
};

enum class GameScreen {
    MainMenu,
    DlcSelect,
    CharacterCreate,
    Playing,
    GameOver
};

// --- 运行时数据结构 ---

// DLC 中定义的职业
struct DlcClass {
    QString id;       // 字符串ID，如 "infantry"
    QString name;     // 显示名称，如 "步兵"
    QString desc;     // 描述文本
};

// 章节元信息（来自 manifest.json）
struct DlcChapterMeta {
    QString id;       // 如 "fall_gelb"
    QString file;     // JSON 文件相对路径
    QString name;     // 显示名称
    QString subtitle; // 副标题（日期等）
    QString unlock;   // 解锁条件："start" 或 chapter id
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
    QMap<QString, QString> music;       // key → 相对文件路径
    bool valid = true;                   // 加载/校验是否通过
    QStringList errors;                  // 校验错误信息
};

// --- 选择结构（职业字段改为 QString） ---

struct Choice {
    QString text;
    QString nextNodeId;

    int hpDelta      = 0;
    int moraleDelta  = 0;

    QSet<QString> requiredFlags;
    QSet<QString> grantedFlags;

    bool classRestricted = false;
    QList<QString> allowedClasses;      // ← 改为 QString，不再使用 PlayerClass 枚举

    bool isCombat        = false;
    int  combatThreshold = 50;
    QList<QString> bonusClasses;        // ← 改为 QString
    QString successNodeId;
    QString failureNodeId;
    int failHpDelta      = -20;
    int failMoraleDelta  = -15;
};

// --- 故事节点 ---

struct StoryNode {
    QString id;
    QString locationTitle;
    QString text;

    NodeType type = NodeType::Choice;

    QList<Choice> choices;

    QString nextNodeId;      // Narrative 类型用
    QString musicKey;
    bool isVictory = false;
    bool isDefeat  = false;

    // 职业专属文本（key = class id string）
    QMap<QString, QString> classText;

    QString textFor(const QString &classId) const {
        if (classText.contains(classId))
            return classText[classId];
        return text;
    }
};

// --- 单个章节（从 JSON 文件加载） ---

struct DlcChapter {
    QString chapterId;
    QString startNodeId;
    QString defeatNodeId;            // 可选
    QMap<QString, StoryNode> nodes;
};
```

- [ ] **Step 2: Commit**

```bash
git add engine/DlcTypes.h
git commit -m "feat: add DlcTypes.h — engine runtime data structures"
```

---

### Task 2: Remove GameState.h hardcoded enums

**Files:**
- Modify: `core/GameState.h`

- [ ] **Step 1: Rewrite GameState.h — strip to shared enums only**

The old file had PlayerClass, ScenarioId, ScenarioIdHash, and helper functions — all removed. Only NodeType and GameScreen (shared enums) stay, but they're now in DlcTypes.h, so GameState.h becomes empty and can be deleted.

```cpp
// This file is intentionally left empty.
// All shared types have been migrated to engine/DlcTypes.h.
// PlayerClass / ScenarioId / ScenarioIdHash / helper functions are removed.
// These are now defined dynamically by DLC JSON.
```

- [ ] **Step 2: Delete GameState.h**

Since all types are now in `engine/DlcTypes.h`, delete `core/GameState.h`.

```bash
git rm core/GameState.h
```

- [ ] **Step 3: Update all includes — replace `#include "../core/GameState.h"` with `#include "../engine/DlcTypes.h"`**

Files to update (in subsequent tasks, we'll handle these properly):
- `core/Player.h` — will be replaced by `engine/PlayerSystem.h`, skip for now
- `core/StoryNode.h` — will be deleted (merged into DlcTypes.h), skip for now
- `ui/MainWindow.h` — will update in UI phase
- `ui/CharacterCreateWidget.h` — will update in UI phase
- `ui/GameWidget.h` — will update in UI phase

For now, commit the deletion.

```bash
git commit -m "refactor: remove GameState.h — enums migrated to engine/DlcTypes.h"
```

---

### Task 3: Delete old core files superseded by engine modules

**Files:**
- Delete: `core/StoryNode.h` (merged into DlcTypes.h)
- Delete: `core/ScenarioBase.h`, `core/ScenarioBase.cpp` (replaced by NodeEngine)
- Delete: `core/Player.h`, `core/Player.cpp` (replaced by PlayerSystem)
- Delete: `core/GameEngine.h`, `core/GameEngine.cpp` (replaced by NodeEngine + DlcManager)

- [ ] **Step 1: Delete the files**

```bash
git rm core/StoryNode.h
git rm core/ScenarioBase.h core/ScenarioBase.cpp
git rm core/Player.h core/Player.cpp
git rm core/GameEngine.h core/GameEngine.cpp
```

- [ ] **Step 2: Commit**

```bash
git commit -m "refactor: remove old core files superseded by engine modules"
```

---

## Phase 2: Engine Modules

### Task 4: Create engine/DiceSystem — 骰子判定模块

**Files:**
- Create: `engine/DiceSystem.h`
- Create: `engine/DiceSystem.cpp`

- [ ] **Step 1: Write DiceSystem.h**

```cpp
#pragma once
#include <QObject>
#include <QStringList>

class DiceSystem : public QObject {
    Q_OBJECT
public:
    explicit DiceSystem(QObject *parent = nullptr);

    // 基础 D100 掷骰 [1, 100]
    int roll() const;

    // 带职业加成的掷骰
    // bonusClasses: 享有加成的职业ID列表
    // playerClass:   当前玩家职业ID
    // 若 playerClass 在 bonusClasses 中，则 +20
    int rollWithBonus(const QStringList &bonusClasses,
                      const QString &playerClass) const;

    // 判定成功：roll >= threshold
    static bool checkSuccess(int roll, int threshold);
};
```

- [ ] **Step 2: Write DiceSystem.cpp**

```cpp
#include "DiceSystem.h"
#include <QRandomGenerator>
#include <algorithm>

DiceSystem::DiceSystem(QObject *parent) : QObject(parent) {}

int DiceSystem::roll() const {
    return QRandomGenerator::global()->bounded(1, 101); // [1, 100]
}

int DiceSystem::rollWithBonus(const QStringList &bonusClasses,
                              const QString &playerClass) const {
    int result = roll();
    if (bonusClasses.contains(playerClass))
        result += 20;
    return qBound(1, result, 120);
}

bool DiceSystem::checkSuccess(int roll, int threshold) {
    return roll >= threshold;
}
```

- [ ] **Step 3: Commit**

```bash
git add engine/DiceSystem.h engine/DiceSystem.cpp
git commit -m "feat: add DiceSystem — D100 dice roll with class bonus"
```

---

### Task 5: Create engine/PlayerSystem — 动态职业玩家系统

**Files:**
- Create: `engine/PlayerSystem.h`
- Create: `engine/PlayerSystem.cpp`

- [ ] **Step 1: Write PlayerSystem.h**

```cpp
#pragma once
#include <QString>
#include <QSet>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include "DlcTypes.h"

class PlayerSystem {
public:
    PlayerSystem() = default;
    PlayerSystem(const QString &name, const QString &classId, const QString &dlcId);

    // 基础属性
    QString name;
    QString classId;       // 字符串ID，如 "infantry"
    QString dlcId;         // 所属 DLC
    int     hp        = 100;
    int     maxHp     = 100;
    int     morale    = 100;
    int     maxMorale = 100;

    // 叙事标志
    QSet<QString> flags;

    // 进度
    QString   currentChapter;
    QString   currentNodeId;
    QSet<QString> unlockedChapters;

    // 操作方法
    void applyHpDelta(int delta);
    void applyMoraleDelta(int delta);
    bool isAlive()    const { return hp > 0; }
    bool hasMorale()  const { return morale > 0; }
    bool isDead()     const { return !isAlive() || !hasMorale(); }

    bool hasFlag(const QString &flag) const { return flags.contains(flag); }
    void setFlag(const QString &flag)       { flags.insert(flag); }

    bool chapterUnlocked(const QString &chId) const;
    void unlockChapter(const QString &chId);

    // 序列化
    QJsonObject toJson()       const;
    void        fromJson(const QJsonObject &obj);

    // 重置
    void resetStats();
};
```

- [ ] **Step 2: Write PlayerSystem.cpp**

```cpp
#include "PlayerSystem.h"

PlayerSystem::PlayerSystem(const QString &n, const QString &cid, const QString &did)
    : name(n), classId(cid), dlcId(did) {}

void PlayerSystem::applyHpDelta(int delta) {
    hp = std::clamp(hp + delta, 0, maxHp);
}

void PlayerSystem::applyMoraleDelta(int delta) {
    morale = std::clamp(morale + delta, 0, maxMorale);
}

bool PlayerSystem::chapterUnlocked(const QString &chId) const {
    return unlockedChapters.contains(chId);
}

void PlayerSystem::unlockChapter(const QString &chId) {
    unlockedChapters.insert(chId);
}

void PlayerSystem::resetStats() {
    hp     = maxHp;
    morale = maxMorale;
    flags.clear();
    currentNodeId.clear();
}

QJsonObject PlayerSystem::toJson() const {
    QJsonObject obj;
    obj["engineVersion"]   = QStringLiteral("2.0");
    obj["dlcId"]           = dlcId;
    obj["playerName"]      = name;
    obj["playerClass"]     = classId;
    obj["hp"]              = hp;
    obj["maxHp"]           = maxHp;
    obj["morale"]          = morale;
    obj["maxMorale"]       = maxMorale;
    obj["currentChapter"]  = currentChapter;
    obj["currentNodeId"]   = currentNodeId;

    QJsonArray flagArr;
    for (const QString &f : flags) flagArr.append(f);
    obj["flags"] = flagArr;

    QJsonArray unlockedArr;
    for (const QString &ch : unlockedChapters) unlockedArr.append(ch);
    obj["unlockedChapters"] = unlockedArr;

    return obj;
}

void PlayerSystem::fromJson(const QJsonObject &obj) {
    // engineVersion 用于格式兼容检测
    dlcId           = obj["dlcId"].toString();
    name            = obj["playerName"].toString();
    classId         = obj["playerClass"].toString();
    hp              = obj["hp"].toInt(100);
    maxHp           = obj["maxHp"].toInt(100);
    morale          = obj["morale"].toInt(100);
    maxMorale       = obj["maxMorale"].toInt(100);
    currentChapter  = obj["currentChapter"].toString();
    currentNodeId   = obj["currentNodeId"].toString();

    flags.clear();
    for (const auto &v : obj["flags"].toArray())
        flags.insert(v.toString());

    unlockedChapters.clear();
    for (const auto &v : obj["unlockedChapters"].toArray())
        unlockedChapters.insert(v.toString());
}
```

- [ ] **Step 3: Commit**

```bash
git add engine/PlayerSystem.h engine/PlayerSystem.cpp
git commit -m "feat: add PlayerSystem — dynamic class-based player state"
```

---

### Task 6: Create engine/DlcManager — DLC 扫描/加载/校验

**Files:**
- Create: `engine/DlcManager.h`
- Create: `engine/DlcManager.cpp`

- [ ] **Step 1: Write DlcManager.h**

```cpp
#pragma once
#include <QObject>
#include <QDir>
#include <QList>
#include "DlcTypes.h"

class DlcManager : public QObject {
    Q_OBJECT
public:
    explicit DlcManager(QObject *parent = nullptr);

    // 扫描 dlc/ 目录，返回所有发现的 DLC 清单
    void scanDirectory(const QString &dlcRootDir);

    // 获取已扫描的 DLC
    const QList<DlcManifest> &manifests() const { return m_manifests; }

    // 根据 dlcId 获取清单
    const DlcManifest *getManifest(const QString &dlcId) const;

    // 加载指定 DLC 的某一章节
    bool loadChapter(const QString &dlcBasePath,
                     const QString &chapterFile,
                     DlcChapter &outChapter);

    // 获取 DLC 文件夹的绝对路径
    QString dlcBasePath(const QString &dlcId) const { return m_dlcRootDir + "/" + dlcId; }

signals:
    void dlcScanComplete(int count);

private:
    DlcManifest parseManifest(const QString &path);
    QStringList validateManifest(const DlcManifest &m, const QString &basePath);
    StoryNode  parseNodeFromJson(const QJsonObject &obj);

    QString m_dlcRootDir;
    QList<DlcManifest> m_manifests;
};
```

- [ ] **Step 2: Write DlcManager.cpp — scanDirectory + parseManifest**

```cpp
#include "DlcManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

DlcManager::DlcManager(QObject *parent) : QObject(parent) {}

void DlcManager::scanDirectory(const QString &dlcRootDir) {
    m_dlcRootDir = dlcRootDir;
    m_manifests.clear();

    QDir root(dlcRootDir);
    if (!root.exists()) {
        qWarning() << "DLC directory not found:" << dlcRootDir;
        emit dlcScanComplete(0);
        return;
    }

    const auto entries = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        QString manifestPath = entry.absoluteFilePath() + "/manifest.json";
        if (!QFile::exists(manifestPath)) {
            qWarning() << "No manifest.json in" << entry.absoluteFilePath() << ", skipping";
            continue;
        }

        DlcManifest m = parseManifest(manifestPath);
        m.valid = true;

        QString basePath = entry.absoluteFilePath();
        QStringList errors = validateManifest(m, basePath);
        if (!errors.isEmpty()) {
            m.valid = false;
            m.errors = errors;
        }

        m_manifests.append(m);
    }

    emit dlcScanComplete(m_manifests.size());
}

DlcManifest DlcManager::parseManifest(const QString &path) {
    DlcManifest m;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m.valid = false;
        m.errors.append(QStringLiteral("Cannot open manifest.json"));
        return m;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        m.valid = false;
        m.errors.append(QStringLiteral("Invalid JSON in manifest.json"));
        return m;
    }

    QJsonObject root = doc.object();

    m.dlcId       = root["dlcId"].toString();
    m.title       = root["title"].toString();
    m.subtitle    = root["subtitle"].toString();
    m.author      = root["author"].toString();
    m.version     = root["version"].toString();
    m.startChapter = root["startChapter"].toString();

    // Parse classes
    for (const auto &v : root["classes"].toArray()) {
        QJsonObject co = v.toObject();
        DlcClass dc;
        dc.id   = co["id"].toString();
        dc.name = co["name"].toString();
        dc.desc = co["desc"].toString();
        m.classes.append(dc);
    }

    // Parse chapters
    for (const auto &v : root["chapters"].toArray()) {
        QJsonObject cho = v.toObject();
        DlcChapterMeta cm;
        cm.id       = cho["id"].toString();
        cm.file     = cho["file"].toString();
        cm.name     = cho["name"].toString();
        cm.subtitle = cho["subtitle"].toString();
        cm.unlock   = cho["unlock"].toString();
        m.chapters.append(cm);
    }

    // Parse music map
    QJsonObject musicObj = root["music"].toObject();
    for (auto it = musicObj.begin(); it != musicObj.end(); ++it) {
        m.music[it.key()] = it.value().toString();
    }

    return m;
}

QStringList DlcManager::validateManifest(const DlcManifest &m, const QString &basePath) {
    QStringList errors;

    if (m.dlcId.isEmpty())
        errors.append(QStringLiteral("dlcId is empty"));
    if (m.title.isEmpty())
        errors.append(QStringLiteral("title is empty"));
    if (m.classes.isEmpty())
        errors.append(QStringLiteral("classes[] is empty — at least one class required"));
    if (m.chapters.isEmpty())
        errors.append(QStringLiteral("chapters[] is empty — at least one chapter required"));

    // Check class ID uniqueness
    QSet<QString> classIds;
    for (const auto &c : m.classes) {
        if (classIds.contains(c.id))
            errors.append(QStringLiteral("Duplicate class id: ") + c.id);
        classIds.insert(c.id);
    }

    // Check chapter ID uniqueness + file existence
    QSet<QString> chapterIds;
    for (const auto &ch : m.chapters) {
        if (chapterIds.contains(ch.id))
            errors.append(QStringLiteral("Duplicate chapter id: ") + ch.id);
        chapterIds.insert(ch.id);

        QString fullPath = basePath + "/" + ch.file;
        if (!QFile::exists(fullPath))
            errors.append(QStringLiteral("Chapter file not found: ") + ch.file);
    }

    // startChapter must exist
    if (!m.startChapter.isEmpty() && !chapterIds.contains(m.startChapter))
        errors.append(QStringLiteral("startChapter '") + m.startChapter +
                      QStringLiteral("' not found in chapters[]"));

    // Non-start chapters' unlock must reference existing chapter
    for (const auto &ch : m.chapters) {
        if (ch.unlock != "start" && !chapterIds.contains(ch.unlock))
            errors.append(QStringLiteral("Chapter '") + ch.id +
                          QStringLiteral("' unlock target '") + ch.unlock +
                          QStringLiteral("' not found"));
    }

    return errors;
}

const DlcManifest *DlcManager::getManifest(const QString &dlcId) const {
    for (const auto &m : m_manifests) {
        if (m.dlcId == dlcId)
            return &m;
    }
    return nullptr;
}

bool DlcManager::loadChapter(const QString &dlcBasePath,
                              const QString &chapterFile,
                              DlcChapter &outChapter) {
    QString fullPath = dlcBasePath + "/" + chapterFile;
    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull())
        return false;

    QJsonObject root = doc.object();
    outChapter.chapterId    = root["chapterId"].toString();
    outChapter.startNodeId  = root["startNodeId"].toString();
    outChapter.defeatNodeId = root["defeatNodeId"].toString();

    QJsonArray nodesArr = root["nodes"].toArray();
    for (const auto &v : nodesArr) {
        StoryNode node = parseNodeFromJson(v.toObject());
        outChapter.nodes[node.id] = node;
    }

    return true;
}

StoryNode DlcManager::parseNodeFromJson(const QJsonObject &obj) {
    StoryNode n;
    n.id            = obj["id"].toString();
    n.locationTitle = obj["locationTitle"].toString();
    n.text          = obj["text"].toString();
    n.musicKey      = obj["musicKey"].toString();
    n.nextNodeId    = obj["nextNodeId"].toString();
    n.isVictory     = obj["isVictory"].toBool(false);
    n.isDefeat      = obj["isDefeat"].toBool(false);

    QString typeStr = obj["type"].toString();
    if (typeStr == "narrative")
        n.type = NodeType::Narrative;
    else if (typeStr == "ending")
        n.type = NodeType::Ending;
    else
        n.type = NodeType::Choice;

    // Parse choices
    for (const auto &cv : obj["choices"].toArray()) {
        QJsonObject co = cv.toObject();
        Choice c;
        c.text            = co["text"].toString();
        c.nextNodeId      = co["nextNodeId"].toString();
        c.hpDelta         = co["hpDelta"].toInt(0);
        c.moraleDelta     = co["moraleDelta"].toInt(0);
        c.isCombat        = co["isCombat"].toBool(false);
        c.combatThreshold = co["combatThreshold"].toInt(50);
        c.successNodeId   = co["successNodeId"].toString();
        c.failureNodeId   = co["failureNodeId"].toString();
        c.failHpDelta     = co["failHpDelta"].toInt(-20);
        c.failMoraleDelta = co["failMoraleDelta"].toInt(-15);
        c.classRestricted = co["classRestricted"].toBool(false);

        for (const auto &fv : co["requiredFlags"].toArray())
            c.requiredFlags.insert(fv.toString());
        for (const auto &fv : co["grantedFlags"].toArray())
            c.grantedFlags.insert(fv.toString());
        for (const auto &cv : co["allowedClasses"].toArray())
            c.allowedClasses.append(cv.toString());
        for (const auto &bv : co["bonusClasses"].toArray())
            c.bonusClasses.append(bv.toString());

        n.choices.append(c);
    }

    // Parse classText
    QJsonObject ctObj = obj["classText"].toObject();
    for (auto it = ctObj.begin(); it != ctObj.end(); ++it) {
        n.classText[it.key()] = it.value().toString();
    }

    return n;
}
```

- [ ] **Step 3: Commit**

```bash
git add engine/DlcManager.h engine/DlcManager.cpp
git commit -m "feat: add DlcManager — scan, parse, validate DLC directory"
```

---

### Task 7: Create engine/NodeEngine — 节点导航引擎

**Files:**
- Create: `engine/NodeEngine.h`
- Create: `engine/NodeEngine.cpp`

- [ ] **Step 1: Write NodeEngine.h**

```cpp
#pragma once
#include <QObject>
#include "DlcTypes.h"
#include "DiceSystem.h"
#include "PlayerSystem.h"

class NodeEngine : public QObject {
    Q_OBJECT
public:
    explicit NodeEngine(DiceSystem *dice, QObject *parent = nullptr);

    // 初始化：载入 DLC 的起始章节
    bool startDlc(const DlcManifest &manifest, const QString &dlcBasePath,
                  PlayerSystem &player);

    // 开始某一章节
    bool startChapter(const QString &chapterId);

    // 当前节点
    const StoryNode *currentNode() const;
    const DlcChapter *currentChapterData() const { return &m_currentChapter; }

    // 玩家选择
    bool makeChoice(int choiceIndex);

    // 状态查询
    bool isGameOver() const;
    bool isVictory()  const;
    bool isInCombat() const;

signals:
    void nodeChanged(const StoryNode *node);
    void statsChanged(int hp, int morale);
    void chapterVictory(const QString &chapterId);
    void chapterDefeat(const QString &chapterId);
    void combatResult(bool success, int hpChange, int moraleChange);
    void flagSet(const QString &flag);

private:
    void navigateTo(const QString &nodeId);
    void applyChoice(const Choice &choice);

    DiceSystem *m_dice = nullptr;
    DlcManifest m_manifest;
    QString m_dlcBasePath;
    PlayerSystem *m_player = nullptr;  // 不拥有，指向外部
    DlcChapter m_currentChapter;

    // 已加载的章节缓存
    QMap<QString, DlcChapter> m_loadedChapters;

    bool loadChapterInternal(const QString &chapterId);
};
```

- [ ] **Step 2: Write NodeEngine.cpp**

```cpp
#include "NodeEngine.h"
#include "DlcManager.h"
#include <QDebug>

NodeEngine::NodeEngine(DiceSystem *dice, QObject *parent)
    : QObject(parent), m_dice(dice) {}

bool NodeEngine::startDlc(const DlcManifest &manifest, const QString &dlcBasePath,
                           PlayerSystem &player) {
    m_manifest = manifest;
    m_dlcBasePath = dlcBasePath;
    m_player = &player;
    m_loadedChapters.clear();

    return startChapter(player.currentChapter.isEmpty()
                        ? manifest.startChapter
                        : player.currentChapter);
}

bool NodeEngine::startChapter(const QString &chapterId) {
    if (!loadChapterInternal(chapterId))
        return false;

    m_player->currentChapter = chapterId;
    m_player->resetStats();

    QString startId = m_currentChapter.startNodeId;
    navigateTo(startId);
    return true;
}

bool NodeEngine::loadChapterInternal(const QString &chapterId) {
    if (m_loadedChapters.contains(chapterId)) {
        m_currentChapter = m_loadedChapters[chapterId];
        return true;
    }

    // Find chapter meta
    const DlcChapterMeta *meta = nullptr;
    for (const auto &ch : m_manifest.chapters) {
        if (ch.id == chapterId) { meta = &ch; break; }
    }
    if (!meta) return false;

    DlcManager loader; // temporary for loading
    DlcChapter chapter;
    if (!loader.loadChapter(m_dlcBasePath, meta->file, chapter))
        return false;

    m_loadedChapters[chapterId] = chapter;
    m_currentChapter = chapter;
    return true;
}

const StoryNode *NodeEngine::currentNode() const {
    return m_currentChapter.nodes.contains(m_player->currentNodeId)
           ? &m_currentChapter.nodes[m_player->currentNodeId]
           : nullptr;
}

bool NodeEngine::makeChoice(int choiceIndex) {
    const StoryNode *node = currentNode();
    if (!node) return false;

    if (node->type == NodeType::Narrative) {
        navigateTo(node->nextNodeId);
        return true;
    }

    if (choiceIndex < 0 || choiceIndex >= node->choices.size())
        return false;

    const Choice &choice = node->choices[choiceIndex];

    // 职业限制检查
    if (choice.classRestricted &&
        !choice.allowedClasses.contains(m_player->classId))
        return false;

    // 标志条件检查
    for (const QString &flag : choice.requiredFlags) {
        if (!m_player->hasFlag(flag)) return false;
    }

    if (choice.isCombat) {
        int roll = m_dice->rollWithBonus(choice.bonusClasses, m_player->classId);
        bool success = DiceSystem::checkSuccess(roll, choice.combatThreshold);

        if (success) {
            applyChoice(choice);
            emit combatResult(true, choice.hpDelta, choice.moraleDelta);
            navigateTo(choice.successNodeId);
        } else {
            m_player->applyHpDelta(choice.failHpDelta);
            m_player->applyMoraleDelta(choice.failMoraleDelta);
            for (const QString &flag : choice.grantedFlags) {
                m_player->setFlag(flag);
                emit flagSet(flag);
            }
            emit combatResult(false, choice.failHpDelta, choice.failMoraleDelta);
            emit statsChanged(m_player->hp, m_player->morale);

            if (m_player->isDead()) {
                QString defeatId = m_currentChapter.defeatNodeId;
                if (defeatId.isEmpty())
                    defeatId = QStringLiteral("__defeat__");
                navigateTo(defeatId);
                return true;
            }
            navigateTo(choice.failureNodeId);
        }
    } else {
        applyChoice(choice);
        navigateTo(choice.nextNodeId);
    }
    return true;
}

void NodeEngine::applyChoice(const Choice &choice) {
    m_player->applyHpDelta(choice.hpDelta);
    m_player->applyMoraleDelta(choice.moraleDelta);
    for (const QString &flag : choice.grantedFlags) {
        m_player->setFlag(flag);
        emit flagSet(flag);
    }
    emit statsChanged(m_player->hp, m_player->morale);
}

void NodeEngine::navigateTo(const QString &nodeId) {
    const StoryNode *node = m_currentChapter.nodes.contains(nodeId)
                           ? &m_currentChapter.nodes[nodeId]
                           : nullptr;
    if (!node) return;

    m_player->currentNodeId = nodeId;

    if (node->isVictory) {
        // 线性解锁下一章节
        const QList<DlcChapterMeta> &chapters = m_manifest.chapters;
        for (int i = 0; i < chapters.size(); ++i) {
            if (chapters[i].id == m_player->currentChapter) {
                if (i + 1 < chapters.size()) {
                    QString nextChId = chapters[i + 1].id;
                    m_player->unlockChapter(nextChId);
                }
                break;
            }
        }
        emit nodeChanged(node);
        emit chapterVictory(m_player->currentChapter);
        return;
    }

    if (node->isDefeat) {
        emit nodeChanged(node);
        emit chapterDefeat(m_player->currentChapter);
        return;
    }

    emit nodeChanged(node);
}

bool NodeEngine::isGameOver() const {
    return m_player->isDead();
}

bool NodeEngine::isVictory() const {
    const StoryNode *node = currentNode();
    return node && node->isVictory;
}

bool NodeEngine::isInCombat() const {
    const StoryNode *node = currentNode();
    if (!node) return false;
    for (const Choice &c : node->choices)
        if (c.isCombat) return true;
    return false;
}
```

- [ ] **Step 3: Commit**

```bash
git add engine/NodeEngine.h engine/NodeEngine.cpp
git commit -m "feat: add NodeEngine — story node navigation and choice execution"
```

---

### Task 8: Adapt SaveManager for new format

**Files:**
- Modify: `core/SaveManager.h` → Move to: `engine/SaveManager.h`
- Modify: `core/SaveManager.cpp` → Move to: `engine/SaveManager.cpp`

- [ ] **Step 1: Move files**

```bash
git mv core/SaveManager.h engine/SaveManager.h
git mv core/SaveManager.cpp engine/SaveManager.cpp
```

- [ ] **Step 2: Rewrite SaveManager.h**

```cpp
#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include "PlayerSystem.h"

// 存档信息摘要
struct SaveInfo {
    int     slot        = -1;
    bool    isAutoSave  = false;
    QString dlcId;
    QString dlcTitle;
    QString playerName;
    QString className;
    QString chapterName;
    QString timestamp;
    bool    valid       = false;
};

class SaveManager : public QObject {
    Q_OBJECT
public:
    explicit SaveManager(QObject *parent = nullptr);

    static const int AUTO_SLOT    = 0;
    static const int MAX_MANUAL   = 3;

    bool saveGame(int slot, const PlayerSystem &player,
                  const QString &dlcTitle = {},
                  const QString &className = {},
                  const QString &chapterName = {});
    bool autoSave(const PlayerSystem &player,
                  const QString &dlcTitle = {},
                  const QString &className = {},
                  const QString &chapterName = {});

    bool   loadGame(int slot, PlayerSystem &outPlayer);
    QList<SaveInfo> listSaves() const;
    SaveInfo        slotInfo(int slot) const;
    bool deleteSlot(int slot);

private:
    QString savePath(int slot) const;
    QString saveDir()          const;
};
```

- [ ] **Step 3: Rewrite SaveManager.cpp**

```cpp
#include "SaveManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

SaveManager::SaveManager(QObject *parent) : QObject(parent) {
    QDir().mkpath(saveDir());
}

QString SaveManager::saveDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + "/saves";
}

QString SaveManager::savePath(int slot) const {
    return saveDir() + QString("/save_%1.json").arg(slot);
}

bool SaveManager::saveGame(int slot, const PlayerSystem &player,
                            const QString &dlcTitle,
                            const QString &className,
                            const QString &chapterName) {
    QJsonObject root;
    root["slot"]        = slot;
    root["autoSave"]    = (slot == AUTO_SLOT);
    root["timestamp"]   = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    root["dlcTitle"]    = dlcTitle;
    root["className"]   = className;
    root["chapterName"] = chapterName;
    root["player"]      = player.toJson();

    QJsonDocument doc(root);
    QFile file(savePath(slot));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool SaveManager::autoSave(const PlayerSystem &player,
                            const QString &dlcTitle,
                            const QString &className,
                            const QString &chapterName) {
    return saveGame(AUTO_SLOT, player, dlcTitle, className, chapterName);
}

bool SaveManager::loadGame(int slot, PlayerSystem &outPlayer) {
    QFile file(savePath(slot));
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) return false;

    QJsonObject root = doc.object();
    if (!root.contains("player")) return false;

    outPlayer.fromJson(root["player"].toObject());
    return true;
}

QList<SaveInfo> SaveManager::listSaves() const {
    QList<SaveInfo> result;
    for (int slot = 0; slot <= MAX_MANUAL; ++slot)
        result.append(slotInfo(slot));
    return result;
}

SaveInfo SaveManager::slotInfo(int slot) const {
    SaveInfo info;
    info.slot = slot;
    info.isAutoSave = (slot == AUTO_SLOT);

    QFile file(savePath(slot));
    if (!file.open(QIODevice::ReadOnly)) { info.valid = false; return info; }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) { info.valid = false; return info; }

    QJsonObject root = doc.object();
    QJsonObject playerObj = root["player"].toObject();

    info.valid       = true;
    info.dlcId       = playerObj["dlcId"].toString();
    info.dlcTitle    = root["dlcTitle"].toString();
    info.playerName  = playerObj["playerName"].toString();
    info.className   = root["className"].toString();
    info.chapterName = root["chapterName"].toString();
    info.timestamp   = root["timestamp"].toString();
    return info;
}

bool SaveManager::deleteSlot(int slot) {
    return QFile::remove(savePath(slot));
}
```

- [ ] **Step 4: Update SaveLoadDialog.cpp includes — replace old SaveManager include**

The include changes to `#include "../engine/SaveManager.h"` will happen in the UI phase.

- [ ] **Step 5: Commit**

```bash
git add engine/SaveManager.h engine/SaveManager.cpp
git commit -m "refactor: adapt SaveManager for new PlayerSystem + metadata fields"
```

---

### Task 9: Move MusicPlayer to engine/

**Files:**
- Move: `core/MusicPlayer.h` → `engine/MusicPlayer.h`
- Move: `core/MusicPlayer.cpp` → `engine/MusicPlayer.cpp`

- [ ] **Step 1: Move MusicPlayer to engine/**

```bash
git mv core/MusicPlayer.h engine/MusicPlayer.h
git mv core/MusicPlayer.cpp engine/MusicPlayer.cpp
```

The MusicPlayer code is unchanged — just relocated.

- [ ] **Step 2: Commit**

```bash
git commit -m "refactor: move MusicPlayer from core/ to engine/"
```

---

## Phase 3: JSON Content — 现有场景转为 DLC

### Task 10: Create dlc/third_reich/manifest.json

**Files:**
- Create: `dlc/third_reich/manifest.json`

- [ ] **Step 1: Write manifest.json**

```json
{
  "dlcId": "third_reich",
  "title": "钢铁意志：第三帝国的黄昏",
  "subtitle": "一部关于战争残酷与无义的文字悲剧",
  "author": "Official",
  "version": "1.0.0",

  "classes": [
    { "id": "infantry",       "name": "步兵",             "desc": "在前线堑壕中匍匐前进，见证战争最残酷的一面" },
    { "id": "tank_crew",      "name": "坦克兵",           "desc": "驾驭钢铁巨兽，在发动机的轰鸣中突破防线" },
    { "id": "fighter_pilot",  "name": "战斗机飞行员",     "desc": "在万米高空迎着晨光进行致命的空中之舞" },
    { "id": "bomber_pilot",   "name": "轰炸机飞行员",     "desc": "在防空炮火构成的死亡蛛网中保持平稳飞行" },
    { "id": "submarine_crew", "name": "潜艇驾驶员",       "desc": "在狭窄阴暗的'铁棺材'中扮演深海狼群" },
    { "id": "battleship_crew","name": "战列舰驾驶员",     "desc": "在巨舰大炮的荣耀与没落中与战舰共沉浮" }
  ],

  "chapters": [
    {
      "id": "fall_gelb",
      "file": "chapters/ch01_fall_gelb.json",
      "name": "黄色方案",
      "subtitle": "1940年5月 · 阿登森林",
      "unlock": "start"
    },
    {
      "id": "britain",
      "file": "chapters/ch02_britain.json",
      "name": "不列颠空战",
      "subtitle": "1940年7月 · 英吉利海峡",
      "unlock": "fall_gelb"
    },
    {
      "id": "wolf_pack",
      "file": "chapters/ch03_wolf_pack.json",
      "name": "群狼海战",
      "subtitle": "1941年 · 大西洋",
      "unlock": "britain"
    },
    {
      "id": "stalingrad",
      "file": "chapters/ch04_stalingrad.json",
      "name": "斯大林格勒战役",
      "subtitle": "1942年8月 · 伏尔加河畔",
      "unlock": "wolf_pack"
    },
    {
      "id": "berlin",
      "file": "chapters/ch05_berlin.json",
      "name": "柏林战役",
      "subtitle": "1945年4月 · 帝国的黄昏",
      "unlock": "stalingrad"
    }
  ],

  "startChapter": "fall_gelb",

  "music": {
    "main_theme":       "music/main_theme.mp3",
    "fallgelb":         "music/fallgelb.mp3",
    "britain":          "music/britain.mp3",
    "wolfpack":         "music/wolfpack.mp3",
    "stalingrad":       "music/stalingrad.mp3",
    "stalingrad_winter":"music/stalingrad_winter.mp3",
    "stalingrad_end":   "music/stalingrad_end.mp3",
    "berlin":           "music/berlin.mp3",
    "berlin_elegy":     "music/berlin_elegy.mp3",
    "berlin_end":       "music/berlin_end.mp3",
    "defeat_theme":     "music/defeat_theme.mp3"
  }
}
```

- [ ] **Step 2: Copy music files**

```bash
mkdir -p dlc/third_reich/music
cp resources/music/*.mp3 dlc/third_reich/music/ 2>/dev/null || echo "Music files will be placed manually"
```

- [ ] **Step 3: Commit**

```bash
git add dlc/third_reich/manifest.json
git commit -m "feat: add third_reich DLC manifest.json"
```

---

### Task 11: Convert FallGelbScenario to JSON

**Files:**
- Create: `dlc/third_reich/chapters/ch01_fall_gelb.json`
- Read existing: `scenarios/FallGelbScenario.cpp` for node content

- [ ] **Step 1: Write ch01_fall_gelb.json**

Read `scenarios/FallGelbScenario.cpp` to extract all nodes and choices. Convert each StoryNode construction to JSON format.

The pattern for conversion:
- C++ `QStringLiteral("fg_intro")` → JSON `"fg_intro"`
- C++ `n.type = NodeType::Narrative` → JSON `"type": "narrative"`
- C++ `n.nextNodeId = ...` → JSON `"nextNodeId": "..."`
- C++ `Choice c1; c1.text = ...; c1.nextNodeId = ...;` → JSON `{ "text": "...", "nextNodeId": "..." }`
- C++ `PlayerClass::Infantry` → JSON `"infantry"`
- C++ `QSet<QString>{...}` → JSON `[...]`
- C++ `QList<PlayerClass>{...}` → JSON `[...]`

Example structure (abbreviated — full file to be written during implementation):

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
      "musicKey": "fallgelb",
      "text": "...",
      "nextNodeId": "fg_choice_1"
    },
    ...
  ]
}
```

- [ ] **Step 2: Commit**

```bash
git add dlc/third_reich/chapters/ch01_fall_gelb.json
git commit -m "feat: convert FallGelb scenario to JSON chapter"
```

---

### Task 12: Convert remaining 4 scenarios to JSON

**Files:**
- Create: `dlc/third_reich/chapters/ch02_britain.json`
- Create: `dlc/third_reich/chapters/ch03_wolf_pack.json`
- Create: `dlc/third_reich/chapters/ch04_stalingrad.json`
- Create: `dlc/third_reich/chapters/ch05_berlin.json`

- [ ] **Step 1: Convert each scenario**

Read each C++ scenario file and convert all nodes to JSON format.
Follow the same pattern as Task 11.

- [ ] **Step 2: Commit each**

```bash
git add dlc/third_reich/chapters/
git commit -m "feat: convert all 5 scenarios to JSON chapters"
```

---

### Task 13: Delete scenarios/ directory

**Files:**
- Delete: `scenarios/FallGelbScenario.h`, `scenarios/FallGelbScenario.cpp`
- Delete: `scenarios/BritainScenario.h`, `scenarios/BritainScenario.cpp`
- Delete: `scenarios/WolfPackScenario.h`, `scenarios/WolfPackScenario.cpp`
- Delete: `scenarios/StalingradScenario.h`, `scenarios/StalingradScenario.cpp`
- Delete: `scenarios/BerlinScenario.h`, `scenarios/BerlinScenario.cpp`

- [ ] **Step 1: Delete all scenario files**

```bash
git rm scenarios/*.h scenarios/*.cpp
```

- [ ] **Step 2: Commit**

```bash
git commit -m "refactor: remove C++ scenario files — all converted to JSON DLC"
```

---

## Phase 4: UI Adaptation

### Task 14: Adapt MainWindow — new engine integration

**Files:**
- Modify: `ui/MainWindow.h`
- Modify: `ui/MainWindow.cpp`

- [ ] **Step 1: Rewrite MainWindow.h**

```cpp
#pragma once
#include <QMainWindow>
#include "../engine/DlcTypes.h"
#include "../engine/PlayerSystem.h"

class QStackedWidget;
class DlcManager;
class NodeEngine;
class DiceSystem;
class SaveManager;
class MusicPlayer;
class MenuWidget;
class CharacterCreateWidget;
class GameWidget;
struct StoryNode;
struct DlcManifest;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void showMainMenu();
    void showDlcSelect();
    void showCharacterCreate();
    void showGameScreen();

    void onDlcSelected(const QString &dlcId);
    void onStartGame(const QString &name, const QString &classId);
    void onLoadGame(const PlayerSystem &loadedPlayer);

    void onChoiceMade(int index);

    void openSaveDialog();
    void openLoadDialog();

    void onNodeChanged(const StoryNode *node);
    void onStatsChanged(int hp, int morale);
    void onCombatResult(bool success, int hpChange, int moraleChange);
    void onChapterVictory(const QString &chapterId);
    void onChapterDefeat(const QString &chapterId);

private:
    void setupUi();
    void connectSignals();
    void registerMusicFromDlc();
    void startChapterMusic();

    QStackedWidget *m_stackedWidget = nullptr;

    MenuWidget *m_menuWidget = nullptr;
    CharacterCreateWidget *m_createWidget = nullptr;
    GameWidget *m_gameWidget = nullptr;

    // Engine modules
    DlcManager  *m_dlcManager  = nullptr;
    DiceSystem  *m_diceSystem  = nullptr;
    NodeEngine  *m_nodeEngine  = nullptr;
    SaveManager *m_saveManager = nullptr;
    MusicPlayer *m_musicPlayer = nullptr;

    // State
    PlayerSystem m_player;
    DlcManifest  m_currentDlc;
    QString      m_currentDlcBasePath;
    QString      m_currentChapterName;
    QString      m_currentClassName;
};
```

- [ ] **Step 2: Rewrite MainWindow.cpp**

```cpp
#include "MainWindow.h"
#include "MenuWidget.h"
#include "CharacterCreateWidget.h"
#include "GameWidget.h"
#include "SaveLoadDialog.h"
#include "../engine/DlcManager.h"
#include "../engine/NodeEngine.h"
#include "../engine/DiceSystem.h"
#include "../engine/SaveManager.h"
#include "../engine/MusicPlayer.h"

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_dlcManager  = new DlcManager(this);
    m_diceSystem  = new DiceSystem(this);
    m_nodeEngine  = new NodeEngine(m_diceSystem, this);
    m_saveManager = new SaveManager(this);
    m_musicPlayer = new MusicPlayer(this);

    setupUi();
    connectSignals();

    // 扫描 DLC
    QString dlcDir = QCoreApplication::applicationDirPath() + "/dlc";
    if (!QDir(dlcDir).exists())
        dlcDir = QDir::currentPath() + "/dlc";
    m_dlcManager->scanDirectory(dlcDir);

    m_musicPlayer->play(QStringLiteral("main_theme"));
}

void MainWindow::setupUi() {
    setMinimumSize(850, 650);
    resize(960, 700);
    setWindowTitle(QStringLiteral("互动叙事引擎"));

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    m_menuWidget   = new MenuWidget(this);
    m_createWidget = new CharacterCreateWidget(this);
    m_gameWidget   = new GameWidget(this);

    m_stackedWidget->addWidget(m_menuWidget);       // 0
    m_stackedWidget->addWidget(m_createWidget);     // 1
    m_stackedWidget->addWidget(m_gameWidget);       // 2

    showMainMenu();
}

void MainWindow::connectSignals() {
    // Menu
    connect(m_menuWidget, &MenuWidget::newGameClicked, this, &MainWindow::showDlcSelect);
    connect(m_menuWidget, &MenuWidget::loadGameClicked, this, &MainWindow::openLoadDialog);
    connect(m_menuWidget, &MenuWidget::exitGameClicked, this, &MainWindow::close);

    // Character create
    connect(m_createWidget, &CharacterCreateWidget::backToMenu, this, &MainWindow::showMainMenu);
    connect(m_createWidget, &CharacterCreateWidget::startGame, this, &MainWindow::onStartGame);

    // Game widget
    connect(m_gameWidget, &GameWidget::choiceMade, this, &MainWindow::onChoiceMade);
    connect(m_gameWidget, &GameWidget::saveClicked, this, &MainWindow::openSaveDialog);
    connect(m_gameWidget, &GameWidget::loadClicked, this, &MainWindow::openLoadDialog);
    connect(m_gameWidget, &GameWidget::exitClicked, this, &MainWindow::showMainMenu);

    // Node engine
    connect(m_nodeEngine, &NodeEngine::nodeChanged, this, &MainWindow::onNodeChanged);
    connect(m_nodeEngine, &NodeEngine::statsChanged, this, &MainWindow::onStatsChanged);
    connect(m_nodeEngine, &NodeEngine::combatResult, this, &MainWindow::onCombatResult);
    connect(m_nodeEngine, &NodeEngine::chapterVictory, this, &MainWindow::onChapterVictory);
    connect(m_nodeEngine, &NodeEngine::chapterDefeat, this, &MainWindow::onChapterDefeat);
}

void MainWindow::registerMusicFromDlc() {
    for (auto it = m_currentDlc.music.begin(); it != m_currentDlc.music.end(); ++it) {
        QString fullPath = m_currentDlcBasePath + "/" + it.value();
        m_musicPlayer->registerTrack(it.key(), fullPath);
    }
}

void MainWindow::startChapterMusic() {
    if (!m_currentDlc.music.isEmpty()) {
        QString firstKey = m_currentDlc.music.firstKey();
        m_musicPlayer->play(firstKey);
    }
}

// --- Navigation ---

void MainWindow::showMainMenu() {
    m_stackedWidget->setCurrentWidget(m_menuWidget);
    m_musicPlayer->play(QStringLiteral("main_theme"));
}

void MainWindow::showDlcSelect() {
    // Populate DLC list on the menu widget and show
    QList<DlcManifest> manifests = m_dlcManager->manifests();
    m_menuWidget->showDlcList(manifests);
    // MenuWidget will emit dlcSelected(QString dlcId)
}

void MainWindow::showCharacterCreate() {
    m_stackedWidget->setCurrentWidget(m_createWidget);
}

void MainWindow::showGameScreen() {
    m_stackedWidget->setCurrentWidget(m_gameWidget);
}

// --- DLC / Game Start ---

void MainWindow::onDlcSelected(const QString &dlcId) {
    const DlcManifest *m = m_dlcManager->getManifest(dlcId);
    if (!m) return;

    m_currentDlc = *m;
    m_currentDlcBasePath = m_dlcManager->dlcBasePath(dlcId);
    registerMusicFromDlc();

    // Pass classes to character create widget
    m_createWidget->setClasses(m_currentDlc.classes);
    showCharacterCreate();
}

void MainWindow::onStartGame(const QString &name, const QString &classId) {
    m_player = PlayerSystem(name, classId, m_currentDlc.dlcId);
    m_player.currentChapter = m_currentDlc.startChapter;
    m_player.unlockChapter(m_currentDlc.startChapter);

    // Find class name for display
    for (const auto &c : m_currentDlc.classes) {
        if (c.id == classId) {
            m_currentClassName = c.name;
            break;
        }
    }

    if (m_nodeEngine->startDlc(m_currentDlc, m_currentDlcBasePath, m_player)) {
        showGameScreen();
        startChapterMusic();
    }
}

void MainWindow::onLoadGame(const PlayerSystem &loadedPlayer) {
    m_player = loadedPlayer;

    // Find and load the DLC
    const DlcManifest *m = m_dlcManager->getManifest(m_player.dlcId);
    if (!m) {
        QMessageBox::warning(this, QStringLiteral("错误"),
                             QStringLiteral("该存档所属的DLC未安装。"));
        return;
    }

    m_currentDlc = *m;
    m_currentDlcBasePath = m_dlcManager->dlcBasePath(m_player.dlcId);
    registerMusicFromDlc();

    // Find class name
    for (const auto &c : m_currentDlc.classes) {
        if (c.id == m_player.classId) {
            m_currentClassName = c.name;
            break;
        }
    }

    if (m_nodeEngine->startDlc(m_currentDlc, m_currentDlcBasePath, m_player)) {
        showGameScreen();
        startChapterMusic();
    }
}

// --- Choice ---

void MainWindow::onChoiceMade(int index) {
    if (index == -2) {
        // Victory: go to next chapter
        const QList<DlcChapterMeta> &chapters = m_currentDlc.chapters;
        QString currentCh = m_player.currentChapter;
        for (int i = 0; i < chapters.size(); ++i) {
            if (chapters[i].id == currentCh && i + 1 < chapters.size()) {
                QString nextChId = chapters[i + 1].id;
                if (m_nodeEngine->startChapter(nextChId)) {
                    startChapterMusic();
                }
                break;
            }
        }
    } else {
        m_nodeEngine->makeChoice(index);
    }
}

// --- Save/Load Dialogs ---

void MainWindow::openSaveDialog() {
    // Determine current chapter name
    QString chName;
    for (const auto &ch : m_currentDlc.chapters) {
        if (ch.id == m_player.currentChapter) {
            chName = ch.name;
            break;
        }
    }

    PlayerSystem mutablePlayer = m_player;
    SaveLoadDialog dlg(true, m_saveManager, &mutablePlayer, this);
    dlg.setDlcInfo(m_currentDlc.dlcId, m_currentDlc.title,
                   m_currentClassName, chName);
    dlg.exec();
}

void MainWindow::openLoadDialog() {
    PlayerSystem mutablePlayer = m_player;
    SaveLoadDialog dlg(false, m_saveManager, &mutablePlayer, this);
    connect(&dlg, &SaveLoadDialog::gameLoaded, this, &MainWindow::onLoadGame);
    dlg.exec();
}

// --- Engine Signal Handlers ---

void MainWindow::onNodeChanged(const StoryNode *node) {
    if (!node) return;

    m_gameWidget->showStoryNode(node, m_player.classId, m_currentDlc);

    if (!node->musicKey.isEmpty())
        m_musicPlayer->play(node->musicKey);

    // Auto-save
    QString chName;
    for (const auto &ch : m_currentDlc.chapters) {
        if (ch.id == m_player.currentChapter) { chName = ch.name; break; }
    }
    m_saveManager->autoSave(m_player, m_currentDlc.title,
                            m_currentClassName, chName);
}

void MainWindow::onStatsChanged(int hp, int morale) {
    m_gameWidget->updatePlayerStats(hp, morale);
}

void MainWindow::onCombatResult(bool success, int hpChange, int moraleChange) {
    QString title = success ? QStringLiteral("判定成功") : QStringLiteral("判定失败");
    QString msg = success
        ? QStringLiteral("【判定结果：成功】\n您通过敏捷的动作或精湛的特技成功化险为夷！")
        : QStringLiteral("【判定结果：失败】\n局势恶化！您在交火或事故中遭受了重创。\n生命值损失：%1 | 士气值损失：%2")
          .arg(qAbs(hpChange)).arg(qAbs(moraleChange));
    QMessageBox::information(this, title, msg);
}

void MainWindow::onChapterVictory(const QString &chapterId) {
    // Show victory message for non-final chapters
    bool isLastChapter = (chapterId == m_currentDlc.chapters.last().id);
    if (!isLastChapter) {
        QMessageBox::information(this, QStringLiteral("战役胜利"),
                                 QStringLiteral("【当前战役已顺利通过】\n下一章战役已经解锁。"));
    }
    m_saveManager->autoSave(m_player, m_currentDlc.title,
                            m_currentClassName, m_currentChapterName);
}

void MainWindow::onChapterDefeat(const QString & /*chapterId*/) {
    m_musicPlayer->play(QStringLiteral("defeat_theme"));
}
```

- [ ] **Step 3: Commit**

```bash
git add ui/MainWindow.h ui/MainWindow.cpp
git commit -m "refactor: adapt MainWindow for engine modules + DLC flow"
```

---

### Task 15: Adapt MenuWidget — add DLC list display

**Files:**
- Modify: `ui/MenuWidget.h`
- Modify: `ui/MenuWidget.cpp`

- [ ] **Step 1: Add DLC list to MenuWidget.h**

Add to the class:
```cpp
// New signal
signals:
    void dlcSelected(const QString &dlcId);

// New method
public:
    void showDlcList(const QList<DlcManifest> &manifests);

private:
    QWidget *m_dlcListContainer = nullptr;  // DLC selection panel
    QWidget *m_mainMenuPanel = nullptr;      // Original menu panel
```

Forward declare `struct DlcManifest;` and include the types header.

- [ ] **Step 2: Implement showDlcList in MenuWidget.cpp**

```cpp
void MenuWidget::showDlcList(const QList<DlcManifest> &manifests) {
    // Clear existing DLC cards
    if (m_dlcListContainer) {
        QLayout *layout = m_dlcListContainer->layout();
        QLayoutItem *child;
        while ((child = layout->takeAt(1)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
    }

    // Add a card for each valid DLC
    for (const DlcManifest &m : manifests) {
        QPushButton *btn = new QPushButton(
            QStringLiteral("%1\n%2").arg(m.title, m.subtitle),
            m_dlcListContainer);
        btn->setObjectName(m.valid ? "menuBtn" : "menuBtnDisabled");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumSize(350, 60);
        btn->setEnabled(m.valid);

        if (m.valid) {
            connect(btn, &QPushButton::clicked, this, [this, id = m.dlcId]() {
                emit dlcSelected(id);
            });
        }

        m_dlcListContainer->layout()->addWidget(btn);
    }

    // Switch to DLC list view
    m_mainMenuPanel->hide();
    m_dlcListContainer->show();
}
```

- [ ] **Step 3: Commit**

```bash
git add ui/MenuWidget.h ui/MenuWidget.cpp
git commit -m "feat: add DLC list display to MenuWidget"
```

---

### Task 16: Adapt CharacterCreateWidget — dynamic classes

**Files:**
- Modify: `ui/CharacterCreateWidget.h`
- Modify: `ui/CharacterCreateWidget.cpp`

- [ ] **Step 1: Rewrite CharacterCreateWidget.h**

```cpp
#pragma once
#include <QWidget>
#include <QList>
#include "../engine/DlcTypes.h"

class QLineEdit;
class QLabel;
class QPushButton;

class CharacterCreateWidget : public QWidget {
    Q_OBJECT
public:
    explicit CharacterCreateWidget(QWidget *parent = nullptr);
    ~CharacterCreateWidget() = default;

    // Set classes from DLC manifest (called before showing)
    void setClasses(const QList<DlcClass> &classes);

signals:
    void backToMenu();
    void startGame(const QString &name, const QString &classId); // classId is QString now

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onClassSelected(int id);
    void onStartClicked();

private:
    void setupUi();
    void updatePreview(const DlcClass &cls);

    QLineEdit *m_nameEdit = nullptr;
    QList<QWidget*> m_classCards;
    QList<DlcClass> m_classes;

    QLabel *m_hpLabel = nullptr;
    QLabel *m_moraleLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    QLabel *m_scenarioLabel = nullptr;

    QPushButton *m_startBtn = nullptr;
    QPushButton *m_backBtn = nullptr;

    int m_selectedIndex = 0;
};
```

- [ ] **Step 2: Rewrite CharacterCreateWidget.cpp**

Key changes:
- Replace hardcoded `classesText` with dynamic card creation from `m_classes`
- `updatePreview(DlcClass)` instead of `updatePreview(PlayerClass)`
- `emit startGame(name, m_classes[m_selectedIndex].id)` — QString classId instead of enum
- Remove all switch-case blocks for PlayerClass — use DlcClass members directly

```cpp
void CharacterCreateWidget::setClasses(const QList<DlcClass> &classes) {
    m_classes = classes;
    m_selectedIndex = 0;

    // Clear existing cards
    for (QWidget *card : m_classCards) {
        m_classCardsLayout->removeWidget(card);  // pseudo — actual: remove from grid
        card->deleteLater();
    }
    m_classCards.clear();

    // Create cards dynamically
    for (int i = 0; i < m_classes.size(); ++i) {
        QWidget *card = new QWidget(this);
        card->setObjectName(QStringLiteral("classCard"));
        card->setCursor(Qt::PointingHandCursor);
        card->setProperty("classIndex", i);
        card->installEventFilter(this);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 10, 10, 10);

        QLabel *cardText = new QLabel(m_classes[i].name, card);
        cardText->setObjectName(QStringLiteral("classCardText"));
        cardText->setAlignment(Qt::AlignCenter);
        cardText->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        cardLayout->addWidget(cardText);

        grid->addWidget(card, i / 2, i % 2);
        m_classCards.append(card);
    }

    if (!m_classes.isEmpty())
        onClassSelected(0);
}

void CharacterCreateWidget::updatePreview(const DlcClass &cls) {
    m_hpLabel->setText(QStringLiteral("初始生命值：100"));
    m_moraleLabel->setText(QStringLiteral("初始士气值：100"));
    m_descLabel->setText(cls.desc);
    // Scenario availability is now generic — any class can enter start chapter
}
```

- [ ] **Step 3: Commit**

```bash
git add ui/CharacterCreateWidget.h ui/CharacterCreateWidget.cpp
git commit -m "refactor: dynamic class cards from DLC manifest"
```

---

### Task 17: Adapt GameWidget — remove hardcoded scenario references

**Files:**
- Modify: `ui/GameWidget.h`
- Modify: `ui/GameWidget.cpp`

- [ ] **Step 1: Update GameWidget.h**

Change `showStoryNode` signature:
```cpp
void showStoryNode(const StoryNode *node, const QString &playerClassId, const DlcManifest &dlc);
```

Remove `PlayerClass` and `ScenarioId` from the signature. Add `DlcManifest` for context.

- [ ] **Step 2: Update GameWidget.cpp**

Key changes:
- `showStoryNode` uses `node->textFor(playerClassId)` instead of `node->textFor(playerClass)` — now takes QString
- `m_scenarioLabel->setText(dlc.title)` or chapter name instead of `scenarioName(scenarioId)` — no more enum
- Ending node handling: use `dlc.chapters.last().id` to check if current chapter is the last one, instead of `ScenarioId::Berlin`
- Remove `scenarioName()` / `playerClassName()` calls — use DLC metadata

```cpp
void GameWidget::showStoryNode(const StoryNode *node, const QString &playerClassId,
                                const DlcManifest &dlc) {
    // ... existing setup ...

    m_fullText = node->textFor(playerClassId);  // QString key instead of enum

    // For ending nodes
    if (node->type == NodeType::Ending || node->isVictory || node->isDefeat) {
        // Check if current chapter is last
        QString currentChId = /* from MainWindow context */;
        bool isLastChapter = (currentChId == dlc.chapters.last().id);

        if (node->isVictory) {
            if (isLastChapter) {
                btn->setText(QStringLiteral("【返回主页 — 铭记历史】"));
                connect(btn, &QPushButton::clicked, this, &GameWidget::exitClicked);
            } else {
                btn->setText(QStringLiteral("【进入下一战役】"));
                connect(btn, &QPushButton::clicked, this, [this]() {
                    emit choiceMade(-2);
                });
            }
        }
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add ui/GameWidget.h ui/GameWidget.cpp
git commit -m "refactor: remove hardcoded scenario/class enums from GameWidget"
```

---

### Task 18: Adapt SaveLoadDialog — show DLC metadata

**Files:**
- Modify: `ui/SaveLoadDialog.h`
- Modify: `ui/SaveLoadDialog.cpp`

- [ ] **Step 1: Add DLC info method to SaveLoadDialog.h**

```cpp
public:
    void setDlcInfo(const QString &dlcId, const QString &dlcTitle,
                    const QString &className, const QString &chapterName);

signals:
    void gameLoaded(const PlayerSystem &loadedPlayer); // Changed from Player to PlayerSystem

private:
    PlayerSystem *m_player; // Changed from Player*
    QString m_dlcTitle;
    QString m_className;
    QString m_chapterName;
```

- [ ] **Step 2: Update SaveLoadDialog.cpp**

Changes:
- `m_player` type changes from `Player*` to `PlayerSystem*`
- `refreshSlots` uses `info.dlcId`, `info.className`, `info.chapterName` from SaveInfo instead of old format
- `onActionTriggered` passes save slot info back
- Save path creates PlayerSystem JSON

- [ ] **Step 3: Commit**

```bash
git add ui/SaveLoadDialog.h ui/SaveLoadDialog.cpp
git commit -m "refactor: adapt SaveLoadDialog for PlayerSystem + DLC metadata"
```

---

## Phase 5: Build System & Final Integration

### Task 19: Update CMakeLists.txt

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Rewrite SOURCES and HEADERS lists**

```cmake
set(SOURCES
    main.cpp
    engine/PlayerSystem.cpp
    engine/DiceSystem.cpp
    engine/DlcManager.cpp
    engine/NodeEngine.cpp
    engine/SaveManager.cpp
    engine/MusicPlayer.cpp
    ui/MainWindow.cpp
    ui/MenuWidget.cpp
    ui/GameWidget.cpp
    ui/CharacterCreateWidget.cpp
    ui/SaveLoadDialog.cpp
)

set(HEADERS
    engine/DlcTypes.h
    engine/PlayerSystem.h
    engine/DiceSystem.h
    engine/DlcManager.h
    engine/NodeEngine.h
    engine/SaveManager.h
    engine/MusicPlayer.h
    ui/MainWindow.h
    ui/MenuWidget.h
    ui/GameWidget.h
    ui/CharacterCreateWidget.h
    ui/SaveLoadDialog.h
)
```

Remove all `scenarios/` and `core/` entries.

- [ ] **Step 2: Update music copy path**

Change from `resources/music` to `dlc/third_reich/music`:
```cmake
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dlc/third_reich/music")
    add_custom_command(TARGET HeartsOfIronGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/dlc"
        "$<TARGET_FILE_DIR:HeartsOfIronGame>/dlc"
    )
endif()
```

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: update CMakeLists for engine/ modules + dlc/ copy"
```

---

### Task 20: Update resources.qrc

**Files:**
- Modify: `resources/resources.qrc`

- [ ] **Step 1: Remove scenario-specific resource references**

Keep only `style.qss` and any engine-level resources. Remove any references to old scenario assets.

- [ ] **Step 2: Commit**

```bash
git add resources/resources.qrc
git commit -m "chore: clean up resources.qrc"
```

---

## Phase 6: Build Verification

### Task 21: Build and fix compilation errors

- [ ] **Step 1: Configure and build**

```bash
cd "D:/Creater/Hearts of iron"
cmake -B build -S .
cmake --build build --config Release 2>&1
```

- [ ] **Step 2: Fix any remaining compilation errors**

Common issues to check:
- Missing forward declarations
- `#include` path changes (old `../core/` → `../engine/`)
- Qt MOC issues with new signal signatures (QString instead of enums)
- `StoryNode` now in `engine/DlcTypes.h` instead of `core/StoryNode.h`

- [ ] **Step 3: Commit fixes**

```bash
git add -u
git commit -m "fix: compilation errors after engine refactor"
```

---

### Task 22: Runtime verification

- [ ] **Step 1: Run the application**

```bash
./build/Release/钢铁意志_第三帝国.exe
```

- [ ] **Step 2: Verify key flows**

1. Main menu loads → shows DLC list (third_reich)
2. Select third_reich → character create shows 6 class cards
3. Create character → game starts at FallGelb chapter
4. Navigate through nodes → choices work, combat dice rolls trigger
5. Chapter victory → next chapter unlocks
6. Save game → save slot shows DLC metadata
7. Load game → restores to correct chapter/node
8. Berlin chapter end → shows final ending

- [ ] **Step 3: Commit any final fixes**

```bash
git add -u
git commit -m "fix: runtime issues found during verification"
```

---

## Task Summary

| Phase | Task | Description |
|-------|------|-------------|
| 1 | T1 | Create `engine/DlcTypes.h` |
| 1 | T2 | Remove `core/GameState.h` |
| 1 | T3 | Delete old core files |
| 2 | T4 | Create `engine/DiceSystem` |
| 2 | T5 | Create `engine/PlayerSystem` |
| 2 | T6 | Create `engine/DlcManager` |
| 2 | T7 | Create `engine/NodeEngine` |
| 2 | T8 | Adapt `engine/SaveManager` |
| 2 | T9 | Move `MusicPlayer` to engine/ |
| 3 | T10 | Create `dlc/third_reich/manifest.json` |
| 3 | T11 | Convert FallGelb to JSON |
| 3 | T12 | Convert remaining 4 scenarios to JSON |
| 3 | T13 | Delete `scenarios/` directory |
| 4 | T14 | Adapt `MainWindow` |
| 4 | T15 | Adapt `MenuWidget` (DLC list) |
| 4 | T16 | Adapt `CharacterCreateWidget` (dynamic classes) |
| 4 | T17 | Adapt `GameWidget` (remove enums) |
| 4 | T18 | Adapt `SaveLoadDialog` |
| 5 | T19 | Update `CMakeLists.txt` |
| 5 | T20 | Update `resources.qrc` |
| 6 | T21 | Build and fix compilation |
| 6 | T22 | Runtime verification |
