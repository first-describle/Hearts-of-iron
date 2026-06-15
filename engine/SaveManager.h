/**
 * ===========================================================================
 * SaveManager.h — 游戏存档管理系统
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 数据持久化
 * 【依赖关系】PlayerSystem.h (玩家数据序列化)
 * 【对应实现】SaveManager.cpp
 *
 * SaveManager 负责游戏进度的持久化存储，采用:
 *   - 4 槽位设计: 1 个自动存档 (slot 0) + 3 个手动存档 (slot 1~3)
 *   - JSON 文件格式: 人类可读、易于调试和存档修改
 *   - 标准路径存储: QStandardPaths::AppDataLocation (Windows: %APPDATA%)
 *
 * 存档文件结构:
 *   每个槽位对应一个 JSON 文件，包含:
 *     - 槽位信息 (slot, autoSave, timestamp)
 *     - DLC 信息 (dlcTitle, className, chapterName)
 *     - 玩家完整状态 (player: PlayerSystem::toJson() 的结果)
 *
 * 存储路径:
 *   Windows: %APPDATA%/HeartsOfIronGame/saves/save_0.json
 *   (自动存档固定使用 save_0.json)
 *
 * SaveInfo — 存档元数据结构
 *   用于在 UI 中展示存档列表（不加载完整玩家状态），
 *   相比直接加载 JSON 更轻量。
 * ===========================================================================
 */

#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include "PlayerSystem.h"

/**
 * SaveInfo — 存档槽位的元数据摘要
 * 用于 SaveLoadDialog 中的卡片展示。
 * 不包含完整的玩家状态数据（节省内存）。
 */
struct SaveInfo {
    int     slot        = -1;      // 槽位编号 0~3
    bool    isAutoSave  = false;   // 是否自动存档
    QString dlcId;                  // DLC 标识
    QString dlcTitle;               // DLC 标题（显示用）
    QString playerName;             // 角色名
    QString className;              // 兵种名称
    QString chapterName;            // 章节名称
    QString timestamp;              // 存档时间戳（"yyyy-MM-dd HH:mm:ss"）
    bool    valid       = false;    // 槽位是否有有效数据
};

class SaveManager : public QObject {
    Q_OBJECT
public:
    explicit SaveManager(QObject *parent = nullptr);

    // =========================================================================
    // 槽位常量
    // =========================================================================
    static const int AUTO_SLOT    = 0;   // 自动存档槽位编号
    static const int MAX_MANUAL   = 3;   // 最大手动存档槽位编号 (0~3 共 4 个)

    // =========================================================================
    // 存档操作
    // =========================================================================

    /**
     * saveGame() — 保存游戏到指定槽位
     * @param slot        槽位编号 (0~3)
     * @param player      玩家状态（将被序列化为 JSON）
     * @param dlcTitle    DLC 标题（存档摘要用）
     * @param className   兵种名称（存档摘要用）
     * @param chapterName 章节名称（存档摘要用）
     * @return true = 保存成功
     */
    bool saveGame(int slot, const PlayerSystem &player,
                  const QString &dlcTitle = {},
                  const QString &className = {},
                  const QString &chapterName = {});

    /**
     * autoSave() — 自动存档（固定写入 slot 0）
     * 每次节点切换时由 MainWindow::onNodeChanged 调用。
     */
    bool autoSave(const PlayerSystem &player,
                  const QString &dlcTitle = {},
                  const QString &className = {},
                  const QString &chapterName = {});

    /**
     * loadGame() — 从指定槽位读取存档
     * @param slot       槽位编号
     * @param outPlayer  [出参] 恢复后的玩家状态
     * @return true = 读取成功
     */
    bool loadGame(int slot, PlayerSystem &outPlayer);

    // =========================================================================
    // 槽位查询
    // =========================================================================

    /** listSaves() — 获取所有 4 个槽位的摘要信息 */
    QList<SaveInfo> listSaves() const;

    /** slotInfo() — 获取单个槽位的摘要信息 */
    SaveInfo slotInfo(int slot) const;

    /** deleteSlot() — 删除指定槽位的存档文件 */
    bool deleteSlot(int slot);

private:
    // =========================================================================
    // 内部路径计算
    // =========================================================================

    /** savePath() — 获取指定槽位的完整文件路径 */
    QString savePath(int slot) const;

    /** saveDir() — 获取存档目录路径（自动创建） */
    QString saveDir() const;
};
