/**
 * ===========================================================================
 * DiceSystem.cpp — D100 骰子随机判定系统实现
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 战斗随机判定
 * 【依赖头文件】DiceSystem.h, QRandomGenerator (随机数), QtAlgorithms (qBound)
 *
 * 本文件实现了游戏的核心战斗判定机制:
 *
 *   ① roll():          D100 基础掷骰 [1, 100]
 *   ② rollWithBonus(): 职业匹配时 +20，钳制在 [1, 120]
 *   ③ checkSuccess():  掷骰结果 >= 阈值 → 成功
 *
 * 数值平衡设计:
 *   - 基础阈值 50: 无加成职业有 51% 成功率（roll >= 50）
 *   - 加成职业 +20: 匹配职业有 ~70% 成功率（roll+20 >= 50 即 roll >= 30）
 *   - 上限 120: 确保极端高难度（threshold=100）也能通过职业加成克服
 *   - 下限 1: 避免零值/负值的逻辑问题
 * ===========================================================================
 */

#include "DiceSystem.h"
#include <QRandomGenerator>  // Qt6 全局随机数生成器
#include <algorithm>          // qBound (Qt 版本的 std::clamp)

// ===========================================================================
// 构造函数
// ===========================================================================
DiceSystem::DiceSystem(QObject *parent) : QObject(parent) {}

// ===========================================================================
// roll() — D100 基础掷骰 (公开方法)
// ===========================================================================
/**
 * 使用 Qt6 全局随机数生成器生成 [1, 100] 的整数。
 *
 * QRandomGenerator::global()->bounded(lo, hi):
 *   - lo: 下界（包含）
 *   - hi: 上界（不包含）
 *   - 因此 bounded(1, 101) 返回 [1, 100]
 *
 * QRandomGenerator::global() 是一个线程安全的全局 CSPRNG 实例，
 * 在程序启动时用系统熵源（如 /dev/urandom）自动播种。
 * 无需也不应该手动播种。
 */
int DiceSystem::roll() const {
    return QRandomGenerator::global()->bounded(1, 101); // [1, 100]
}

// ===========================================================================
// rollWithBonus() — 带职业加成的掷骰 (公开方法)
// ===========================================================================
/**
 * 【算法步骤】
 *   ① result = roll()              → 生成 [1, 100] 的基础值
 *   ② if playerClass ∈ bonusClasses → result += 20 (职业匹配加成)
 *   ③ return qBound(1, result, 120) → 钳制在 [1, 120] 范围
 *
 * 【qBound 说明】
 *   qBound(min, value, max) 等价于 std::clamp(value, min, max):
 *     若 value < min 返回 min
 *     若 value > max 返回 max
 *     否则返回 value
 *
 * 【示例计算】
 *   - 步兵 roll=45, bonusClasses=[tank_crew] → 无加成 → result=45
 *   - 坦克手 roll=45, bonusClasses=[tank_crew] → 有加成 → result=65
 *   - 坦克手 roll=95, bonusClasses=[tank_crew] → 95+20=115 → result=115 (未超上限)
 *   - 坦克手 roll=105, bonusClasses=[tank_crew] → 不可能（roll() 最大 100）
 */
int DiceSystem::rollWithBonus(const QStringList &bonusClasses,
                              const QString &playerClass) const {
    int result = roll();
    if (bonusClasses.contains(playerClass))
        result += 20;                // 职业匹配: +20 加成
    return qBound(1, result, 120);  // 钳制在 [1, 120]
}

// ===========================================================================
// checkSuccess() — 判定成功 (公开静态方法)
// ===========================================================================
/**
 * 【判定规则】
 *   掷骰结果 >= 阈值 → 成功
 *
 * 【静态方法原因】
 *   判定逻辑不依赖 DiceSystem 实例的状态，设计为 static 便于:
 *     - 直接从任何地方调用（无需获取 DiceSystem 实例）
 *     - 单元测试（无需构造对象）
 *
 * 【阈值建议】
 *   - 50: 中等难度（默认值）
 *   - 30: 简单（适合早期章节的引导性战斗）
 *   - 70: 困难（后期章节的高风险战斗）
 *   - 90: 极难（需要职业加成才有较大概率成功）
 */
bool DiceSystem::checkSuccess(int roll, int threshold) {
    return roll >= threshold;
}
