/**
 * ===========================================================================
 * PlayerSystem.h — 玩家状态管理系统
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 玩家数据管理
 * 【依赖关系】DlcTypes.h (基础类型), QtCore (QString/QSet/QJsonObject)
 * 【对应实现】PlayerSystem.cpp
 *
 * PlayerSystem 是玩家所有运行时数据的容器，负责:
 *   ① 基础属性: HP (生命值)、 morale (士气值)，均带最大值钳制
 *   ② 叙事标志: 选择驱动的持久化状态（如 "saved_wounded" 标志）
 *   ③ 章节进度: 当前章节、当前节点、已解锁章节集合
 *   ④ JSON 序列化: toJson() / fromJson() 用于存档读写
 *   ⑤ 状态重置: resetStats() 用于章节切换时的状态清理
 *
 * 数据流:
 *   创建: PlayerSystem(name, classId, dlcId) → 初始 HP/士气 = 100
 *   修改: NodeEngine::applyChoice → applyHpDelta / applyMoraleDelta
 *   保存: SaveManager::saveGame → player.toJson() → JSON 文件
 *   读取: SaveManager::loadGame → player.fromJson(json) → 恢复状态
 *
 * HP/士气边界:
 *   HP ∈ [0, 100]    — 使用 std::clamp 确保不越界
 *   morale ∈ [0, 100] — 同上
 *   死亡判定: hp <= 0 || morale <= 0
 * ===========================================================================
 */

#pragma once
#include <QString>
#include <QSet>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>       // std::clamp
#include "DlcTypes.h"

class PlayerSystem {
public:
    // =========================================================================
    // 构造与析构
    // =========================================================================
    PlayerSystem() = default;

    /**
     * 完整构造 — 创建新游戏时的玩家
     * @param name    玩家角色名
     * @param classId 选择的兵种 ID（如 "infantry"）
     * @param dlcId   所属 DLC ID
     */
    PlayerSystem(const QString &name, const QString &classId, const QString &dlcId);

    // =========================================================================
    // 基础属性 (公开成员)
    // =========================================================================
    QString name;               // 角色名（如 "汉斯 · 缪勒"）
    QString classId;            // 兵种字符串 ID（如 "infantry", "tank_crew"）
    QString dlcId;              // 所属 DLC 标识
    int     hp        = 100;    // 当前生命值
    int     maxHp     = 100;    // 生命值上限
    int     morale    = 100;    // 当前士气值
    int     maxMorale = 100;    // 士气值上限

    // =========================================================================
    // 叙事标志 (公开成员)
    // =========================================================================
    /**
     * flags — 叙事标志集合
     * 用于追踪玩家的叙事选择，影响后续选项的可见性。
     * 例如: 选择了"救助伤员" → 获得 "saved_wounded" 标志
     *       在后续章节中，拥有该标志可看到特殊的对话选项。
     * 使用 QSet 保证标志唯一性。
     */
    QSet<QString> flags;

    // =========================================================================
    // 章节进度 (公开成员)
    // =========================================================================
    QString   currentChapter;          // 当前所在章节 ID
    QString   currentNodeId;           // 当前所在节点 ID
    QSet<QString> unlockedChapters;    // 已解锁的章节 ID 集合

    // =========================================================================
    // 操作方法
    // =========================================================================

    /** applyHpDelta() — 应用 HP 变化，使用 clamp 确保在 [0, maxHp] 范围内 */
    void applyHpDelta(int delta);

    /** applyMoraleDelta() — 应用士气变化，使用 clamp 确保在 [0, maxMorale] 范围内 */
    void applyMoraleDelta(int delta);

    /** isAlive() — HP > 0？ */
    bool isAlive()    const { return hp > 0; }

    /** hasMorale() — 士气 > 0？ */
    bool hasMorale()  const { return morale > 0; }

    /** isDead() — 死亡判定: HP <= 0 或 士气 <= 0 */
    bool isDead()     const { return !isAlive() || !hasMorale(); }

    /** hasFlag() — 检查是否拥有某标志 */
    bool hasFlag(const QString &flag) const { return flags.contains(flag); }

    /** setFlag() — 设置标志（幂等操作） */
    void setFlag(const QString &flag)       { flags.insert(flag); }

    /** chapterUnlocked() — 某章节是否已解锁 */
    bool chapterUnlocked(const QString &chId) const;

    /** unlockChapter() — 解锁章节 */
    void unlockChapter(const QString &chId);

    // =========================================================================
    // 序列化 (存档 ↔ JSON)
    // =========================================================================

    /**
     * toJson() — 将玩家状态序列化为 JSON 对象
     * 包含: 引擎版本、DLC ID、角色信息、属性、标志、进度
     * @return QJsonObject — 可直接写入存档文件
     */
    QJsonObject toJson() const;

    /**
     * fromJson() — 从 JSON 对象恢复玩家状态
     * @param obj 存档中的 "player" 子对象
     */
    void fromJson(const QJsonObject &obj);

    // =========================================================================
    // 重置
    // =========================================================================

    /**
     * resetStats() — 重置战斗属性
     * HP/士气回满，清空标志和当前节点。
     * 在新章节开始时调用，确保玩家以全盛状态进入每章。
     */
    void resetStats();
};
