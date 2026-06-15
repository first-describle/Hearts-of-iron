/**
 * ===========================================================================
 * SaveLoadDialog.h — 存档/读档模态对话框
 * ===========================================================================
 *
 * 【所属模块】UI 层 — 存档对话框
 * 【依赖关系】SaveManager.h (存档操作), PlayerSystem.h (玩家状态)
 * 【对应实现】SaveLoadDialog.cpp
 *
 * SaveLoadDialog 是一个模态对话框，支持两种模式:
 *   ① 保存模式 (savingMode=true):  选择槽位 → 确认覆盖 → 写入存档
 *   ② 读取模式 (savingMode=false): 选择槽位 → 确认读取 → 恢复游戏
 *
 * 界面设计:
 *   - 4 个存档槽位卡片（0=自动存档, 1~3=手动存档）
 *   - 每个卡片显示: 槽位名称、存档摘要（DLC/角色/兵种/章节）、时间戳
 *   - 底部按钮: 删除记录 | (spacer) | 取消 | 保存/读取
 *
 * 交互规则:
 *   - 保存模式:
 *     - Slot 0 (自动存档) 不可选择（tooltip提示）
 *     - 选中非自动槽位后，即使该槽位无数据也可保存
 *     - 删除按钮仅对有数据的非自动槽位可用
 *   - 读取模式:
 *     - 无数据的槽位不可选择
 *     - 仅对有数据的槽位可执行读取
 *     - 删除按钮对有数据的非自动槽位可用
 *
 * 卡片选中高亮:
 *   使用 QSS 属性选择器: QWidget[selected="true"] { border: 2px solid red; }
 *   通过 setProperty + unpolish/polish 触发样式刷新。
 *
 * 触发按钮:
 *   使用透明的 QPushButton 覆盖在卡片上作为点击触发器。
 *   这种方式比自定义 mousePressEvent 更简单可靠。
 * ===========================================================================
 */

#pragma once
#include <QDialog>
#include <QList>
#include "../engine/SaveManager.h"

class QPushButton;
class QLabel;
class QButtonGroup;

class SaveLoadDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * 构造函数
     * @param savingMode   true=保存模式, false=读取模式
     * @param saveManager  存档管理器实例
     * @param player       玩家状态引用（保存时写入, 读取时忽略）
     * @param parent       父窗口
     */
    SaveLoadDialog(bool savingMode, SaveManager *saveManager,
                   PlayerSystem *player, QWidget *parent = nullptr);
    ~SaveLoadDialog() = default;

    /**
     * setDlcInfo() — 设置存档摘要信息
     * 保存模式下调用，为存档 JSON 提供显示信息。
     *
     * @param dlcTitle    DLC 标题
     * @param className   兵种显示名称
     * @param chapterName 当前章节显示名称
     */
    void setDlcInfo(const QString &dlcTitle, const QString &className,
                    const QString &chapterName);

signals:
    /**
     * gameLoaded() — 存档已成功读取
     * 读取模式下用户确认读取后发射。
     * MainWindow::onLoadGame 接收此信号并恢复游戏状态。
     *
     * @param loadedPlayer 从存档恢复的玩家状态
     */
    void gameLoaded(const PlayerSystem &loadedPlayer);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    /** onSlotSelected() — 槽位卡片选中 */
    void onSlotSelected(int id);

    /** onActionTriggered() — 执行保存/读取 */
    void onActionTriggered();

    /** onDeleteTriggered() — 删除存档 */
    void onDeleteTriggered();

private:
    /** setupUi() — 构建 4 槽位卡片 + 按钮的布局 */
    void setupUi();

    /** refreshSlots() — 刷新所有 4 个槽位卡片的显示 */
    void refreshSlots();

    // =========================================================================
    // 成员变量
    // =========================================================================
    bool m_savingMode;                   // true=保存, false=读取
    SaveManager *m_saveManager;          // 存档管理器
    PlayerSystem *m_player;              // 玩家状态指针

    // 控件
    QButtonGroup *m_buttonGroup = nullptr;  // 槽位互斥选择组
    QPushButton *m_actionBtn = nullptr;     // "保存记录" / "开始读取"
    QPushButton *m_deleteBtn = nullptr;     // "删除记录"
    QPushButton *m_cancelBtn = nullptr;     // "取消"

    int m_selectedSlot = -1;             // 当前选中的槽位 (-1=未选择)

    // 存档摘要信息（保存时使用）
    QString m_dlcTitle;
    QString m_className;
    QString m_chapterName;

    /**
     * SlotUI — 单个槽位的 UI 控件集合
     * 用于 refreshSlots 时更新卡片内容。
     */
    struct SlotUI {
        QWidget *card = nullptr;         // 卡片容器
        QLabel *titleLabel = nullptr;    // 槽位名称（"【自动存档】" 或 "【战役存档 1】"）
        QLabel *infoLabel = nullptr;     // 存档摘要（DLC/角色/兵种/章节）
        QLabel *timeLabel = nullptr;     // 时间戳
    };
    QList<SlotUI> m_slotsUI;             // 4 个槽位的 UI 集合
};
