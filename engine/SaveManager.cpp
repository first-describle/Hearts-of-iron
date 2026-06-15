/**
 * ===========================================================================
 * SaveManager.cpp — 游戏存档管理系统实现
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 数据持久化
 * 【依赖头文件】SaveManager.h, QStandardPaths (应用数据路径),
 *              QDir/QFile (目录与文件操作), QJsonDocument (JSON 读写),
 *              QDateTime (时间戳)
 *
 * 本文件实现了 4 槽位 JSON 存档系统的完整逻辑:
 *   - 自动存档: slot 0，每次节点切换自动写入
 *   - 手动存档: slot 1~3，玩家手动操作保存/读取/删除
 *
 * 存档文件格式 (以 save_1.json 为例):
 *   {
 *     "slot": 1,
 *     "autoSave": false,
 *     "timestamp": "2026-06-15 14:30:00",
 *     "dlcTitle": "钢铁意志：第三帝国的黄昏",
 *     "className": "步兵",
 *     "chapterName": "黄色方案",
 *     "player": { ... PlayerSystem::toJson() 输出 ... }
 *   }
 *
 * 存储位置:
 *   QStandardPaths::AppDataLocation 解析为:
 *     Windows: C:/Users/<用户名>/AppData/Roaming/HeartsOfIronGame/saves/
 *   在构造时自动创建目录（QDir::mkpath），无需手动创建。
 * ===========================================================================
 */

#include "SaveManager.h"
#include <QStandardPaths>   // 获取操作系统标准应用数据路径
#include <QDir>              // 目录操作（mkpath 创建目录）
#include <QFile>             // 文件读写
#include <QJsonDocument>     // JSON 序列化
#include <QJsonObject>       // JSON 对象
#include <QDateTime>         // 时间戳生成

// ===========================================================================
// 构造函数
// ===========================================================================
/**
 * 初始化时自动创建存档目录。
 * QDir().mkpath() 递归创建所有需要的父目录。
 * 若目录已存在，mkpath 返回 true（幂等操作）。
 */
SaveManager::SaveManager(QObject *parent) : QObject(parent) {
    QDir().mkpath(saveDir());  // 确保存档目录存在
}

// ===========================================================================
// saveDir() — 获取存档目录路径 (内部方法)
// ===========================================================================
/**
 * QStandardPaths::AppDataLocation:
 *   Windows:  C:/Users/<user>/AppData/Roaming/<APPNAME>
 *   macOS:    ~/Library/Application Support/<APPNAME>
 *   Linux:    ~/.local/share/<APPNAME>
 *
 * 在此路径下追加 "/saves" 子目录以组织存档文件。
 */
QString SaveManager::saveDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + "/saves";
}

// ===========================================================================
// savePath() — 获取指定槽位的完整文件路径 (内部方法)
// ===========================================================================
/**
 * 生成格式: "<saveDir>/save_<slot>.json"
 * 例如: "C:/Users/.../saves/save_0.json"
 */
QString SaveManager::savePath(int slot) const {
    return saveDir() + QString("/save_%1.json").arg(slot);
}

// ===========================================================================
// saveGame() — 保存游戏 (公开方法)
// ===========================================================================
/**
 * 【保存流程】
 *   ① 构建 JSON 根对象: slot, autoSave, timestamp, dlcTitle, className, chapterName
 *   ② 嵌入玩家状态: root["player"] = player.toJson()
 *   ③ 创建 QJsonDocument 并格式化为缩进 JSON（方便人工查看）
 *   ④ 写入文件: WriteOnly + Truncate（覆盖已有内容）
 *
 * 【QJsonDocument::Indented 说明】
 *   使用缩进格式输出，便于调试存档问题。
 *   生产环境中可改为 QJsonDocument::Compact 节省磁盘空间。
 */
bool SaveManager::saveGame(int slot, const PlayerSystem &player,
                            const QString &dlcTitle,
                            const QString &className,
                            const QString &chapterName) {
    // ── 构建存档 JSON ──
    QJsonObject root;
    root["slot"]        = slot;
    root["autoSave"]    = (slot == AUTO_SLOT);  // slot 0 = 自动存档
    root["timestamp"]   = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    root["dlcTitle"]    = dlcTitle;
    root["className"]   = className;
    root["chapterName"] = chapterName;
    root["player"]      = player.toJson();  // 玩家完整状态

    // ── 序列化并写入文件 ──
    QJsonDocument doc(root);
    QFile file(savePath(slot));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    // QJsonDocument::Indented: 人类可读的格式化 JSON
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

// ===========================================================================
// autoSave() — 自动存档 (公开方法)
// ===========================================================================
/**
 * 直接调用 saveGame(AUTO_SLOT, ...) 写入 slot 0。
 * 每次节点切换时由 MainWindow::onNodeChanged 调用，
 * 确保玩家不会因意外退出而丢失进度。
 */
bool SaveManager::autoSave(const PlayerSystem &player,
                            const QString &dlcTitle,
                            const QString &className,
                            const QString &chapterName) {
    return saveGame(AUTO_SLOT, player, dlcTitle, className, chapterName);
}

// ===========================================================================
// loadGame() — 读取存档 (公开方法)
// ===========================================================================
/**
 * 【读取流程】
 *   ① 打开存档文件 → 失败返回 false
 *   ② 解析 JSON → 格式错误返回 false
 *   ③ 检查 "player" 子对象存在 → 不存在返回 false
 *   ④ 调用 outPlayer.fromJson() 恢复玩家状态 → 完成
 *
 * 【容错设计】
 *   每个步骤独立检查并返回 false，不会在损坏的文件上崩溃。
 */
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

// ===========================================================================
// listSaves() — 列出所有存档摘要 (公开方法)
// ===========================================================================
/**
 * 遍历槽位 0~3，调用 slotInfo() 获取每个槽的摘要。
 * 用于 SaveLoadDialog 初始化时填充所有卡片。
 *
 * @return 包含 4 个 SaveInfo 的列表（含空槽位）
 */
QList<SaveInfo> SaveManager::listSaves() const {
    QList<SaveInfo> result;
    for (int slot = 0; slot <= MAX_MANUAL; ++slot)
        result.append(slotInfo(slot));
    return result;
}

// ===========================================================================
// slotInfo() — 获取单个槽位摘要 (公开方法)
// ===========================================================================
/**
 * 【摘要提取】
 *   仅读取存档文件的元数据部分（slot/timestamp/dlcTitle 等），
 *   不反序列化完整的玩家状态，节省内存和加载时间。
 *
 * 【空槽位处理】
 *   文件不存在或 JSON 无效时，返回 valid=false 的 SaveInfo。
 *   UI 层据此显示"无存档数据"。
 */
SaveInfo SaveManager::slotInfo(int slot) const {
    SaveInfo info;
    info.slot = slot;
    info.isAutoSave = (slot == AUTO_SLOT);

    // ── 尝试打开存档文件 ──
    QFile file(savePath(slot));
    if (!file.open(QIODevice::ReadOnly)) { info.valid = false; return info; }

    // ── 解析 JSON ──
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) { info.valid = false; return info; }

    QJsonObject root = doc.object();
    QJsonObject playerObj = root["player"].toObject();

    // ── 提取摘要字段 ──
    info.valid       = true;
    info.dlcId       = playerObj["dlcId"].toString();        // 从 player 子对象提取
    info.dlcTitle    = root["dlcTitle"].toString();          // 从根对象提取
    info.playerName  = playerObj["playerName"].toString();   // 从 player 子对象
    info.className   = root["className"].toString();         // 从根对象
    info.chapterName = root["chapterName"].toString();       // 从根对象
    info.timestamp   = root["timestamp"].toString();         // 从根对象
    return info;
}

// ===========================================================================
// deleteSlot() — 删除存档 (公开方法)
// ===========================================================================
/**
 * 直接删除对应的 JSON 文件。
 * QFile::remove() 若文件不存在返回 false（但无副作用）。
 *
 * @param slot 要删除的槽位编号（不允许删除 AUTO_SLOT，由 UI 层保证）
 * @return true = 删除成功
 */
bool SaveManager::deleteSlot(int slot) {
    return QFile::remove(savePath(slot));
}
