/**
 * ===========================================================================
 * PlayerSystem.cpp — 玩家状态管理系统实现
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 玩家数据管理
 * 【依赖头文件】PlayerSystem.h
 *
 * 本文件实现了玩家状态的全部操作方法:
 *   - 构造/重置: 初始化和状态清零
 *   - 属性变化: applyHpDelta / applyMoraleDelta (带边界钳制)
 *   - 章节管理: unlockChapter / chapterUnlocked
 *   - 序列化:   toJson (存档写入) / fromJson (存档读取)
 *
 * 序列化格式:
 *   JSON 对象包含 "engineVersion" 字段用于存档兼容性检查，
 *   以及完整的属性、标志、进度数据。
 *   QJsonArray 用于序列化 QSet<QString>（集合不保证顺序）。
 * ===========================================================================
 */

#include "PlayerSystem.h"

// ===========================================================================
// 构造函数
// ===========================================================================
/**
 * 使用初始化列表设置角色信息和所属 DLC。
 * HP/士气使用默认值 100（已在头文件中声明）。
 */
PlayerSystem::PlayerSystem(const QString &n, const QString &cid, const QString &did)
    : name(n), classId(cid), dlcId(did) {}

// ===========================================================================
// applyHpDelta() — 生命值变化 (公开方法)
// ===========================================================================
/**
 * 使用 std::clamp 将 HP 限制在 [0, maxHp] 范围内。
 * std::clamp(value, lo, hi): 若 value < lo 则返回 lo，若 > hi 返回 hi。
 * 例如: hp=5, delta=-20 → hp+delta=-15 → clamp → 0 (不会变成负数)
 *       hp=95, delta=+10 → hp+delta=105 → clamp → 100 (不会溢出)
 */
void PlayerSystem::applyHpDelta(int delta) {
    hp = std::clamp(hp + delta, 0, maxHp);
}

// ===========================================================================
// applyMoraleDelta() — 士气值变化 (公开方法)
// ===========================================================================
void PlayerSystem::applyMoraleDelta(int delta) {
    morale = std::clamp(morale + delta, 0, maxMorale);
}

// ===========================================================================
// 章节管理 (公开方法)
// ===========================================================================
bool PlayerSystem::chapterUnlocked(const QString &chId) const {
    return unlockedChapters.contains(chId);
}

void PlayerSystem::unlockChapter(const QString &chId) {
    unlockedChapters.insert(chId);
}

// ===========================================================================
// resetStats() — 重置战斗属性 (公开方法)
// ===========================================================================
/**
 * 在新章节开始时调用:
 *   - HP/士气回满（模拟"休息恢复"）
 *   - 清空标志（每个章节的状态独立）
 *   - 清空 currentNodeId（将由 startChapter 设置起始节点）
 *
 * 【设计决策】每章重置标志是叙事设计的选择——
 *   避免前章选择对后续章节产生意外影响。
 *   若需跨章状态，应使用章节解锁机制。
 */
void PlayerSystem::resetStats() {
    hp     = maxHp;
    morale = maxMorale;
    flags.clear();
    currentNodeId.clear();
}

// ===========================================================================
// toJson() — 玩家状态 → JSON 序列化 (公开方法)
// ===========================================================================
/**
 * 【序列化结构】
 *   {
 *     "engineVersion": "2.0",        // 存档格式版本（用于向前兼容）
 *     "dlcId": "third_reich",        // DLC 标识
 *     "playerName": "汉斯 · 缪勒",   // 角色名
 *     "playerClass": "infantry",      // 兵种 ID
 *     "hp": 85,                       // 当前生命值
 *     "maxHp": 100,                   // 最大生命值
 *     "morale": 72,                   // 当前士气值
 *     "maxMorale": 100,               // 最大士气值
 *     "currentChapter": "stalingrad", // 当前章节
 *     "currentNodeId": "node_14",     // 当前节点
 *     "flags": ["saved_wounded", ...],// 叙事标志数组
 *     "unlockedChapters": [...]       // 已解锁章节数组
 *   }
 *
 * 【注意】QSet 转为 QJsonArray 时顺序不保证，但这对存档功能无影响。
 */
QJsonObject PlayerSystem::toJson() const {
    QJsonObject obj;

    // ── 元数据 ──
    obj["engineVersion"]   = QStringLiteral("2.0");
    obj["dlcId"]           = dlcId;

    // ── 角色信息 ──
    obj["playerName"]      = name;
    obj["playerClass"]     = classId;

    // ── 战斗属性 ──
    obj["hp"]              = hp;
    obj["maxHp"]           = maxHp;
    obj["morale"]          = morale;
    obj["maxMorale"]       = maxMorale;

    // ── 进度 ──
    obj["currentChapter"]  = currentChapter;
    obj["currentNodeId"]   = currentNodeId;

    // ── 标志集合 → JSON 数组 ──
    QJsonArray flagArr;
    for (const QString &f : flags) flagArr.append(f);
    obj["flags"] = flagArr;

    // ── 已解锁章节集合 → JSON 数组 ──
    QJsonArray unlockedArr;
    for (const QString &ch : unlockedChapters) unlockedArr.append(ch);
    obj["unlockedChapters"] = unlockedArr;

    return obj;
}

// ===========================================================================
// fromJson() — JSON → 玩家状态 反序列化 (公开方法)
// ===========================================================================
/**
 * 【恢复流程】
 *   从 JSON 对象中读取所有字段，恢复玩家状态。
 *   使用 .toInt(default) 确保旧版存档或损坏数据不会导致崩溃。
 *
 * 【容错设计】
 *   每个字段都有合理的默认值:
 *     - hp/morale 默认 100 (满状态)
 *     - currentChapter 默认空 (将触发从 startChapter 开始)
 *   flags 和 unlockedChapters 每次都先 clear() 再 fill()，
 *   确保不会残留上一次加载的数据。
 */
void PlayerSystem::fromJson(const QJsonObject &obj) {
    // ── 身份信息 ──
    dlcId           = obj["dlcId"].toString();
    name            = obj["playerName"].toString();
    classId         = obj["playerClass"].toString();

    // ── 战斗属性（默认 100） ──
    hp              = obj["hp"].toInt(100);
    maxHp           = obj["maxHp"].toInt(100);
    morale          = obj["morale"].toInt(100);
    maxMorale       = obj["maxMorale"].toInt(100);

    // ── 进度 ──
    currentChapter  = obj["currentChapter"].toString();
    currentNodeId   = obj["currentNodeId"].toString();

    // ── 标志恢复 ──
    flags.clear();  // 先清空，避免累积
    for (const auto &v : obj["flags"].toArray())
        flags.insert(v.toString());

    // ── 已解锁章节恢复 ──
    unlockedChapters.clear();
    for (const auto &v : obj["unlockedChapters"].toArray())
        unlockedChapters.insert(v.toString());
}
