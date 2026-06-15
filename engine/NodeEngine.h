/**
 * ===========================================================================
 * NodeEngine.h — 叙事节点引擎（游戏流程核心控制器）
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 游戏逻辑控制
 * 【依赖关系】DlcTypes.h (数据结构), DiceSystem (战斗掷骰), PlayerSystem (玩家状态)
 * 【对应实现】NodeEngine.cpp
 *
 * NodeEngine 是游戏逻辑的"大脑"，负责:
 *   ① 启动 DLC / 启动章节 → 初始化游戏状态
 *   ② 节点导航 → 根据 currentNodeId 查找并呈现 StoryNode
 *   ③ 选项执行 → 处理普通选项、职业限制、战斗判定三种分支
 *   ④ 章节推进 → 胜利节点自动解锁下一章节（线性链路）
 *   ⑤ 死亡处理 → HP 或士气归零时跳转 defeatNode
 *
 * 信号发射 → UI 层响应:
 *   nodeChanged     → MainWindow::onNodeChanged → GameWidget::showStoryNode (更新显示)
 *   statsChanged    → MainWindow::onStatsChanged → GameWidget::updatePlayerStats (刷新血条)
 *   combatResult    → MainWindow::onCombatResult → QMessageBox (战斗结果弹窗)
 *   chapterVictory  → MainWindow::onChapterVictory → 自动存档 + 提示
 *   chapterDefeat   → MainWindow::onChapterDefeat → 播放失败音乐
 *   flagSet         → 可用于 UI 提示（当前未连接）
 *
 * 章节解锁机制:
 *   章节之间是线性链路 —— 第 N 章胜利后自动解锁第 N+1 章。
 *   unlockChapter() 将下一章 ID 加入 PlayerSystem::unlockedChapters。
 *   不支持分支章节选择（设计上保持叙事线性）。
 * ===========================================================================
 */

#pragma once
#include <QObject>
#include "DlcTypes.h"
#include "DiceSystem.h"
#include "PlayerSystem.h"

class NodeEngine : public QObject {
    Q_OBJECT
public:
    /**
     * 构造函数
     * @param dice   骰子系统实例（由 MainWindow 创建并注入）
     * @param parent Qt 父对象
     */
    explicit NodeEngine(DiceSystem *dice, QObject *parent = nullptr);

    // =========================================================================
    // 游戏生命周期管理
    // =========================================================================

    /**
     * startDlc() — 启动一个 DLC
     * 记录 DLC 清单和路径 → 调用 startChapter() 开始首个或当前章节。
     * 读取存档时，player.currentChapter 已有值，直接从存档章节继续。
     *
     * @param manifest    DLC 清单
     * @param dlcBasePath DLC 文件夹绝对路径
     * @param player      玩家状态（引用，直接修改）
     * @return true = 启动成功
     */
    bool startDlc(const DlcManifest &manifest, const QString &dlcBasePath,
                  PlayerSystem &player);

    /**
     * startChapter() — 开始某一章节
     * 加载章节数据 → 重置玩家 HP/士气/标志 → 导航到起始节点。
     *
     * @param chapterId  章节 ID
     * @return true = 成功启动
     */
    bool startChapter(const QString &chapterId);

    // =========================================================================
    // 节点查询
    // =========================================================================

    /**
     * currentNode() — 获取当前叙事节点
     * 从 m_currentChapter.nodes 中按 m_player->currentNodeId 查找。
     * @return 当前节点指针（nullptr = 无效状态）
     */
    const StoryNode *currentNode() const;

    /** 获取当前章节数据（只读引用） */
    const DlcChapter *currentChapterData() const { return &m_currentChapter; }

    // =========================================================================
    // 玩家交互
    // =========================================================================

    /**
     * makeChoice() — 执行玩家选择
     * 这是游戏逻辑的核心入口，根据节点类型和选项类型执行不同逻辑:
     *
     *   Narrative 节点 (choiceIndex = -1):
     *     → 直接跳转到 node.nextNodeId
     *
     *   普通选项:
     *     → 应用 HP/士气变化 + 标志授予 → 跳转到 choice.nextNodeId
     *
     *   战斗判定选项:
     *     → 掷 D100 骰子（职业匹配 +20 加成）
     *     → 成功: 应用正面变化 → 跳转 successNodeId
     *     → 失败: 应用负面变化 → 若玩家死亡跳转 defeatNodeId
     *                        → 否则跳转 failureNodeId
     *
     *   职业限制 / 标志检查:
     *     → 不满足条件的选项返回 false (UI 层应提前隐藏不满足条件的选项)
     *
     * @param choiceIndex  选项在 choices[] 中的索引 (-1 = Narrative 继续)
     * @return true = 执行成功, false = 无效索引或条件不满足
     */
    bool makeChoice(int choiceIndex);

    // =========================================================================
    // 状态查询
    // =========================================================================
    bool isGameOver() const;   // 玩家是否死亡
    bool isVictory()  const;   // 当前节点是否为胜利节点
    bool isInCombat() const;   // 当前节点是否包含战斗选项

signals:
    // ── 节点切换信号 ──
    void nodeChanged(const StoryNode *node);

    // ── 属性变化信号 (hp, morale) ──
    void statsChanged(int hp, int morale);

    // ── 章节结果信号 ──
    void chapterVictory(const QString &chapterId);
    void chapterDefeat(const QString &chapterId);

    // ── 战斗结果信号 ──
    void combatResult(bool success, int hpChange, int moraleChange);

    // ── 标志设置信号 ──
    void flagSet(const QString &flag);

private:
    // =========================================================================
    // 内部实现方法
    // =========================================================================

    /**
     * navigateTo() — 导航到指定节点
     * 更新 player.currentNodeId → 检查 isVictory/isDefeat → 发射相应信号
     */
    void navigateTo(const QString &nodeId);

    /**
     * applyChoice() — 应用选项效果
     * 修改 HP/士气 → 设置标志 → 发射 statsChanged + flagSet
     */
    void applyChoice(const Choice &choice);

    /**
     * loadChapterInternal() — 内部章节加载（含缓存检查）
     * 若章节已加载（在 m_loadedChapters 中），直接复用缓存。
     * 否则通过 DlcManager::loadChapter() 从磁盘加载。
     */
    bool loadChapterInternal(const QString &chapterId);

    // =========================================================================
    // 成员变量
    // =========================================================================
    DiceSystem *m_dice = nullptr;               // 骰子系统（依赖注入）
    DlcManifest m_manifest;                      // 当前 DLC 清单
    QString m_dlcBasePath;                       // DLC 文件夹绝对路径
    PlayerSystem *m_player = nullptr;            // 当前玩家状态（引用 MainWindow 持有的实例）
    DlcChapter m_currentChapter;                 // 当前活动的章节数据
    QMap<QString, DlcChapter> m_loadedChapters;  // 已加载章节缓存 (ID → 章节数据)
};
