/**
 * ===========================================================================
 * DiceSystem.h — D100 骰子随机判定系统
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 战斗随机判定
 * 【依赖关系】QObject (信号槽), QStringList (职业列表)
 * 【对应实现】DiceSystem.cpp
 *
 * DiceSystem 是战斗判定的随机数引擎，负责:
 *   ① D100 基础掷骰: 生成 [1, 100] 的均匀随机整数
 *   ② 职业加成: 若玩家职业在 bonusClasses 中，结果 +20
 *   ③ 结果钳制: 最终结果限制在 [1, 120] 范围内
 *   ④ 成功判定: roll >= threshold 为成功
 *
 * 随机数生成器:
 *   使用 QRandomGenerator::global() —— Qt6 提供的全局 CSPRNG。
 *   这是一个密码学安全的伪随机数生成器，种子由系统熵源提供。
 *
 * 设计意图:
 *   骰子系统独立于 NodeEngine，便于:
 *     - 单元测试（可 mock 骰子结果）
 *     - 未来扩展（如 D20 系统 / 其他骰子类型）
 *     - 依赖注入（MainWindow 创建后传入 NodeEngine）
 * ===========================================================================
 */

#pragma once
#include <QObject>
#include <QStringList>

class DiceSystem : public QObject {
    Q_OBJECT
public:
    explicit DiceSystem(QObject *parent = nullptr);

    /**
     * roll() — 基础 D100 掷骰
     * @return [1, 100] 范围内的随机整数（两端均包含）
     */
    int roll() const;

    /**
     * rollWithBonus() — 带职业加成的掷骰
     * 算法: result = roll() + (playerClass ∈ bonusClasses ? 20 : 0)
     *       最终钳制在 [1, 120]
     *
     * @param bonusClasses  享有 +20 加成的职业 ID 列表（来自 Choice 配置）
     * @param playerClass   当前玩家的职业 ID
     * @return [1, 120] 范围内的整数
     */
    int rollWithBonus(const QStringList &bonusClasses,
                      const QString &playerClass) const;

    /**
     * checkSuccess() — 判定掷骰是否成功
     * 判定标准: roll >= threshold
     *
     * @param roll      掷骰结果
     * @param threshold 难度阈值（来自 Choice::combatThreshold，默认 50）
     * @return true = 判定成功
     */
    static bool checkSuccess(int roll, int threshold);
};
