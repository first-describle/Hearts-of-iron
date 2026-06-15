/**
 * ===========================================================================
 * NodeEngine.cpp — 叙事节点引擎实现
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 游戏逻辑控制
 * 【依赖头文件】NodeEngine.h, DlcManager.h (章节加载), QDebug (日志)
 *
 * 本文件实现了游戏的核心叙事逻辑:
 *   startDlc → startChapter → loadChapterInternal → navigateTo → (等待玩家)
 *   makeChoice → applyChoice / 战斗判定 → navigateTo → (循环)
 *
 * 关键设计决策:
 *   - 章节缓存: m_loadedChapters 避免重复加载同一章节（读档后已加载的不再读磁盘）
 *   - 线性解锁: 第 N 章胜利 → 解锁第 N+1 章 (通过 player.unlockChapter)
 *   - 战斗投骰: D100 + 职业匹配 +20 → 判定成功/失败分支
 *   - 死亡处理: HP <= 0 或 morale <= 0 → 跳转 defeatNodeId
 * ===========================================================================
 */

#include "NodeEngine.h"
#include "DlcManager.h"
#include <QDebug>

// ===========================================================================
// 构造函数
// ===========================================================================
// 依赖注入模式: DiceSystem 由 MainWindow 创建后传入，NodeEngine 不负责其生命周期。
NodeEngine::NodeEngine(DiceSystem *dice, QObject *parent)
    : QObject(parent), m_dice(dice) {}

// ===========================================================================
// startDlc() — 启动 DLC (公开接口)
// ===========================================================================
/**
 * 【执行流程】
 *   ① 记录 DLC 清单和路径到成员变量
 *   ② 记录玩家引用（指针，不拷贝）
 *   ③ 清空章节缓存
 *   ④ 决定起始章节:
 *      - 新游戏: player.currentChapter 为空 → 使用 manifest.startChapter
 *      - 读取存档: player.currentChapter 已有值 → 从存档章节继续
 *   ⑤ 调用 startChapter() 开始
 */
bool NodeEngine::startDlc(const DlcManifest &manifest, const QString &dlcBasePath,
                           PlayerSystem &player) {
    m_manifest = manifest;
    m_dlcBasePath = dlcBasePath;
    m_player = &player;
    m_loadedChapters.clear();

    return startChapter(player.currentChapter.isEmpty()
                        ? manifest.startChapter       // 新游戏: 从起始章节开始
                        : player.currentChapter);      // 读档: 从存档章节继续
}

// ===========================================================================
// startChapter() — 开始章节 (公开接口)
// ===========================================================================
/**
 * 【执行流程】
 *   ① 调用 loadChapterInternal() 加载章节数据
 *   ② 设置 player.currentChapter = chapterId
 *   ③ 重置玩家状态 (resetStats: HP/士气回满，清空标志)
 *   ④ 导航到章节起始节点
 *
 * 【注意】resetStats() 会清空所有标志！
 *         这是设计上的选择 —— 每个章节开始时的状态重置。
 *         若需跨章节保留状态，应通过 manifest 级别的机制实现。
 */
bool NodeEngine::startChapter(const QString &chapterId) {
    if (!loadChapterInternal(chapterId))
        return false;

    m_player->currentChapter = chapterId;
    m_player->resetStats();  // 重置 HP/士气/标志

    QString startId = m_currentChapter.startNodeId;
    navigateTo(startId);     // 导航到第一个节点 → 发射 nodeChanged 信号
    return true;
}

// ===========================================================================
// loadChapterInternal() — 内部章节加载（含缓存） (内部实现)
// ===========================================================================
/**
 * 【缓存策略】
 *   先检查 m_loadedChapters 缓存 → 命中则直接使用，避免重复读磁盘
 *   缓存未命中 → 在 m_manifest.chapters 中查找章节元数据 → 调用 DlcManager 加载 JSON
 *
 * 【设计原因】
 *   玩家可能在存档间切换章节，缓存可避免每次切换都重新读取 JSON。
 *   缓存生命周期: 与 NodeEngine 实例相同（即单次游戏运行）。
 */
bool NodeEngine::loadChapterInternal(const QString &chapterId) {
    // ── 缓存命中: 直接使用已加载的章节 ──
    if (m_loadedChapters.contains(chapterId)) {
        m_currentChapter = m_loadedChapters[chapterId];
        return true;
    }

    // ── 缓存未命中: 在 manifest 中查找章节元数据 ──
    const DlcChapterMeta *meta = nullptr;
    for (const auto &ch : m_manifest.chapters) {
        if (ch.id == chapterId) { meta = &ch; break; }
    }
    if (!meta) return false;   // 章节不存在

    // ── 从磁盘加载章节 JSON ──
    DlcManager loader;
    DlcChapter chapter;
    if (!loader.loadChapter(m_dlcBasePath, meta->file, chapter))
        return false;

    // ── 存入缓存 ──
    m_loadedChapters[chapterId] = chapter;
    m_currentChapter = chapter;
    return true;
}

// ===========================================================================
// currentNode() — 获取当前节点 (公开接口)
// ===========================================================================
// 从当前章节的节点映射表中按 player.currentNodeId 查找。
// 返回 const 指针，调用方不可修改节点内容。
const StoryNode *NodeEngine::currentNode() const {
    auto it = m_currentChapter.nodes.find(m_player->currentNodeId);
    if (it != m_currentChapter.nodes.end())
        return &it.value();
    return nullptr;
}

// ===========================================================================
// makeChoice() — 执行玩家选择 (公开接口 | 核心逻辑)
// ===========================================================================
/**
 * 【分支处理流程】
 *
 *   ┌─ 情况 A: Narrative 节点 ──────────────────────────────────────────┐
 *   │   choiceIndex = -1 (由 GameWidget 自动传入)                        │
 *   │   → navigateTo(node.nextNodeId)                                    │
 *   │   不改变任何属性或标志                                              │
 *   └────────────────────────────────────────────────────────────────────┘
 *
 *   ┌─ 情况 B: 普通选项 (isCombat = false) ──────────────────────────────┐
 *   │   ① 职业限制检查: classRestricted 且 classId 不在 allowedClasses   │
 *   │      → 返回 false（UI 应提前隐藏此类选项）                          │
 *   │   ② 标志条件检查: requiredFlags 中有玩家未拥有的标志                │
 *   │      → 返回 false                                                  │
 *   │   ③ applyChoice(choice): 应用 HP/士气/标志变化                     │
 *   │   ④ navigateTo(choice.nextNodeId): 跳转目标节点                    │
 *   └────────────────────────────────────────────────────────────────────┘
 *
 *   ┌─ 情况 C: 战斗判定选项 (isCombat = true) ───────────────────────────┐
 *   │   ① 职业限制 + 标志检查（同上）                                     │
 *   │   ② 掷骰: m_dice->rollWithBonus(bonusClasses, classId)             │
 *   │      - D100 基础 [1,100] + 职业匹配时 +20 → 结果 [1,120]          │
 *   │   ③ 判定: roll >= combatThreshold                                  │
 *   │                                                                     │
 *   │   ┌── 成功分支:                                                    │
 *   │   │   applyChoice(choice) → 应用正面效果                           │
 *   │   │   emit combatResult(true, ...) → UI 弹窗                       │
 *   │   │   navigateTo(choice.successNodeId)                             │
 *   │   │                                                                 │
 *   │   └── 失败分支:                                                    │
 *   │       apply failHpDelta / failMoraleDelta → HP/士气扣减            │
 *   │       授予 grantedFlags（失败也授予标志，影响后续叙事）              │
 *   │       emit combatResult(false, ...) → UI 弹窗                      │
 *   │       emit statsChanged(...) → 更新 UI 血条                        │
 *   │                                                                     │
 *   │       若玩家死亡 (isDead() = true):                                │
 *   │         navigateTo(defeatNodeId) → 章节失败                        │
 *   │       否则:                                                         │
 *   │         navigateTo(failureNodeId) → 失败后继续                     │
 *   └────────────────────────────────────────────────────────────────────┘
 */
bool NodeEngine::makeChoice(int choiceIndex) {
    const StoryNode *node = currentNode();
    if (!node) return false;

    // ── 情况 A: Narrative 节点 → 直接跳转 ──
    if (node->type == NodeType::Narrative) {
        navigateTo(node->nextNodeId);
        return true;
    }

    // ── 边界检查 ──
    if (choiceIndex < 0 || choiceIndex >= node->choices.size())
        return false;

    const Choice &choice = node->choices[choiceIndex];

    // ── 职业限制检查 ──
    if (choice.classRestricted &&
        !choice.allowedClasses.contains(m_player->classId))
        return false;

    // ── 标志条件检查 ──
    for (const QString &flag : choice.requiredFlags) {
        if (!m_player->hasFlag(flag)) return false;
    }

    // ── 情况 C: 战斗判定 ──
    if (choice.isCombat) {
        // --- 掷骰: 将 QList<QString> 转为 QStringList 传给 DiceSystem ---
        // Qt6 中 QStringList 继承 QList<QString>，此处显式转换以确保类型匹配
        QStringList bonusList;
        for (const QString &cls : choice.bonusClasses)
            bonusList.append(cls);

        // 掷 D100 + 职业加成
        int roll = m_dice->rollWithBonus(bonusList, m_player->classId);
        // 判定: roll >= threshold 为成功
        bool success = DiceSystem::checkSuccess(roll, choice.combatThreshold);

        if (success) {
            // ── 战斗成功 ──
            applyChoice(choice);
            emit combatResult(true, choice.hpDelta, choice.moraleDelta);
            navigateTo(choice.successNodeId);
        } else {
            // ── 战斗失败 ──
            m_player->applyHpDelta(choice.failHpDelta);       // 扣除失败 HP
            m_player->applyMoraleDelta(choice.failMoraleDelta); // 扣除失败士气

            // 失败也授予标志（如 "受伤" 标志，影响后续叙事）
            for (const QString &flag : choice.grantedFlags) {
                m_player->setFlag(flag);
                emit flagSet(flag);
            }

            emit combatResult(false, choice.failHpDelta, choice.failMoraleDelta);
            emit statsChanged(m_player->hp, m_player->morale);

            // 检查玩家是否死亡
            if (m_player->isDead()) {
                QString defeatId = m_currentChapter.defeatNodeId;
                if (defeatId.isEmpty())
                    defeatId = QStringLiteral("__defeat__");  // 兜底
                navigateTo(defeatId);
                return true;
            }
            // 未死亡 → 跳转失败节点（继续叙事）
            navigateTo(choice.failureNodeId);
        }
    } else {
        // ── 情况 B: 普通选项 ──
        applyChoice(choice);
        navigateTo(choice.nextNodeId);
    }
    return true;
}

// ===========================================================================
// applyChoice() — 应用选项效果 (内部实现)
// ===========================================================================
/**
 * 修改玩家 HP/士气（使用 clamp 确保不越界）→ 设置标志 → 发射信号通知 UI。
 */
void NodeEngine::applyChoice(const Choice &choice) {
    m_player->applyHpDelta(choice.hpDelta);         // HP += delta (clamped [0, maxHp])
    m_player->applyMoraleDelta(choice.moraleDelta);  // morale += delta (clamped [0, maxMorale])

    for (const QString &flag : choice.grantedFlags) {
        m_player->setFlag(flag);
        emit flagSet(flag);
    }

    emit statsChanged(m_player->hp, m_player->morale);
}

// ===========================================================================
// navigateTo() — 导航到指定节点 (内部实现 | 关键)
// ===========================================================================
/**
 * 【执行流程】
 *   ① 在 m_currentChapter.nodes 中查找目标节点
 *   ② 更新 player.currentNodeId = nodeId
 *   ③ 检查节点特殊属性:
 *      - isVictory:
 *          → 线性解锁下一章节（在 manifest.chapters 中找当前章节位置）
 *          → 将下一章 ID 加入 player.unlockedChapters
 *          → emit nodeChanged + chapterVictory
 *          → return (不发常规 nodeChanged)
 *      - isDefeat:
 *          → emit nodeChanged + chapterDefeat
 *          → return
 *      - 普通节点:
 *          → emit nodeChanged (MainWindow 响应 → 更新 UI + 自动存档)
 *
 * 【章节解锁机制】
 *   线性查找: 在 manifest.chapters 列表中找到当前章节的索引位置，
 *   若其后还有章节，则将下一章 ID 加入 unlockedChapters。
 *   引擎不支持分支章节选择（设计简化）。
 */
void NodeEngine::navigateTo(const QString &nodeId) {
    auto it = m_currentChapter.nodes.find(nodeId);
    if (it == m_currentChapter.nodes.end()) return;  // 节点不存在，静默失败

    const StoryNode *node = &it.value();
    m_player->currentNodeId = nodeId;

    // ── 胜利节点: 解锁下一章 ──
    if (node->isVictory) {
        const QList<DlcChapterMeta> &chapters = m_manifest.chapters;
        for (int i = 0; i < chapters.size(); ++i) {
            if (chapters[i].id == m_player->currentChapter) {
                // 若当前章节后还有章节 → 解锁下一章
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

    // ── 失败节点: 章节失败 ──
    if (node->isDefeat) {
        emit nodeChanged(node);
        emit chapterDefeat(m_player->currentChapter);
        return;
    }

    // ── 普通节点: 仅通知 UI 更新 ──
    emit nodeChanged(node);
}

// ===========================================================================
// 状态查询方法 (公开接口)
// ===========================================================================
bool NodeEngine::isGameOver() const { return m_player->isDead(); }

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
