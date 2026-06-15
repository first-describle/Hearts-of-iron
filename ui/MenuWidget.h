/**
 * ===========================================================================
 * MenuWidget.h — 主菜单控件（标题画面 + DLC 选择器）
 * ===========================================================================
 *
 * 【所属模块】UI 层 — 主菜单
 * 【依赖关系】DlcTypes.h (DlcManifest 结构体)
 * 【对应实现】MenuWidget.cpp
 *
 * MenuWidget 是玩家看到的第一个界面，包含两个内部面板:
 *   ① 主菜单面板: 游戏标题、副标题、三个功能按钮（新游戏/继续/退出）
 *   ② DLC 选择面板: 动态生成的 DLC 卡片列表 + 返回按钮
 *
 * 页面切换机制:
 *   使用内部 QStackedWidget 在两个面板之间切换，
 *   不依赖 MainWindow 的页面堆栈。
 *   - Index 0: 主菜单面板
 *   - Index 1: DLC 选择面板
 *
 * 发射的信号:
 *   newGameClicked()  → MainWindow::showDlcSelect() → 显示 DLC 列表
 *   loadGameClicked() → MainWindow::openLoadDialog() → 打开读档对话框
 *   exitGameClicked() → MainWindow::close()         → 退出应用
 *   dlcSelected(id)   → MainWindow::onDlcSelected() → 进入角色创建
 * ===========================================================================
 */

#pragma once
#include <QWidget>
#include <QList>
#include "../engine/DlcTypes.h"

class QPushButton;
class QStackedWidget;
class QVBoxLayout;

class MenuWidget : public QWidget {
    Q_OBJECT
public:
    explicit MenuWidget(QWidget *parent = nullptr);
    ~MenuWidget() = default;

    /**
     * showDlcList() — 显示 DLC 列表
     * 由 MainWindow::showDlcSelect() 调用，传入扫描结果。
     * 动态生成 DLC 卡片按钮 → 切换到内部面板 Index 1。
     *
     * @param manifests  DlcManager 扫描后得到的 DLC 清单列表
     */
    void showDlcList(const QList<DlcManifest> &manifests);

signals:
    // ── 主菜单按钮信号 ──
    void newGameClicked();               // "开始新的战役" 按钮
    void loadGameClicked();              // "继续旧的战役" 按钮
    void exitGameClicked();              // "退出战役" 按钮

    // ── DLC 选择信号 ──
    void dlcSelected(const QString &dlcId);  // 用户点击了某个 DLC 卡片

private:
    /**
     * setupUi() — 构建两个面板的完整布局
     * 面板 0: 标题 + 装饰线 + 按钮 + 底部标语
     * 面板 1: DLC 标题 + 动态卡片列表 + 返回按钮
     */
    void setupUi();

    // ── 主菜单面板控件 ──
    QWidget *m_mainMenuPanel = nullptr;     // 面板容器
    QPushButton *m_newGameBtn = nullptr;    // "开始新的战役"
    QPushButton *m_loadGameBtn = nullptr;   // "继续旧的战役"
    QPushButton *m_exitGameBtn = nullptr;   // "退出战役"

    // ── DLC 列表面板控件 ──
    QWidget *m_dlcListPanel = nullptr;      // 面板容器
    QVBoxLayout *m_dlcListLayout = nullptr; // 动态卡片列表的布局（每次 showDlcList 重置）
    QPushButton *m_dlcBackBtn = nullptr;    // "返回主菜单" 按钮
};
