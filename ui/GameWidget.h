/**
 * ===========================================================================
 * GameWidget.h — 核心游戏界面控件
 * ===========================================================================
 *
 * 【所属模块】UI 层 — 游戏主界面
 * 【依赖关系】DlcTypes.h (StoryNode, Choice 等)
 * 【对应实现】GameWidget.cpp
 *
 * GameWidget 是玩家进行游戏的核心界面，包含:
 *   ① 顶部状态栏:   玩家信息（姓名/兵种）+ HP/士气进度条 + 场景/关卡
 *   ② 中央叙事区:   QTextBrowser 显示叙事文本（支持打字机动画）
 *   ③ 底部选项区:   动态生成的选项按钮 + 保存/读档/退出控制
 *
 * 特色功能:
 *   打字机效果: 使用 QTimer (20ms间隔) 逐字显示叙事文本。
 *   点击跳过:   eventFilter 拦截 MouseButtonPress → 立即显示全文。
 *   打字期间:   选项按钮隐藏 + 存读档按钮禁用，防止玩家误操作。
 *
 * 选项生成规则:
 *   - Choice 节点: 为每个符合条件的 Choice 生成按钮
 *     （跳过 classRestricted 且当前职业不在 allowedClasses 中的选项）
 *   - Narrative 节点: 生成单个【继续】按钮 (choiceIndex=0)
 *   - Victory 节点: 生成【进入下一战役】或【返回主页】按钮
 *   - Defeat 节点: 生成【返回主页 — 重新集结】按钮
 *
 * 信号发射:
 *   choiceMade(index) → MainWindow::onChoiceMade
 *     index = -2: 胜利后进入下一章节
 *     index = -1: (未使用)
 *     index >= 0: choices[] 的索引
 *   saveClicked() → MainWindow::openSaveDialog
 *   loadClicked() → MainWindow::openLoadDialog
 *   exitClicked() → MainWindow::showMainMenu
 * ===========================================================================
 */

#pragma once
#include <QWidget>
#include <QList>
#include "../engine/DlcTypes.h"

class QTextBrowser;
class QPushButton;
class QProgressBar;
class QLabel;
class QTimer;
class QVBoxLayout;

class GameWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);
    ~GameWidget() = default;

    // =========================================================================
    // 公开更新方法（由 MainWindow 调用）
    // =========================================================================

    /** updatePlayerStats() — 更新 HP/士气进度条 */
    void updatePlayerStats(int hp, int morale);

    /** updatePlayerInfo() — 更新玩家姓名和兵种标签 */
    void updatePlayerInfo(const QString &name, const QString &className);

    /**
     * showStoryNode() — 显示新的叙事节点
     * 这是核心方法: 清除旧文本 → 设置新文本 → 启动打字机 → 生成选项按钮。
     *
     * @param node          要显示的叙事节点
     * @param playerClassId 当前玩家职业（用于职业文本覆盖 + 选项过滤）
     * @param chapterName   当前章节显示名称
     * @param isLastChapter 是否为最后一章（影响胜利按钮文案）
     */
    void showStoryNode(const StoryNode *node, const QString &playerClassId,
                       const QString &chapterName, bool isLastChapter);

signals:
    /**
     * choiceMade() — 玩家做出了选择
     * @param index 选项索引: -2=下一章, -1=未使用, >=0=choices[] 索引
     */
    void choiceMade(int index);

    void saveClicked();   // 保存按钮被点击
    void loadClicked();   // 读取按钮被点击
    void exitClicked();   // 退出/返回按钮被点击

private slots:
    /** onTypeTimerTick() — 打字机定时器回调（每 20ms） */
    void onTypeTimerTick();

    /** onChoiceButtonClicked() — 选项按钮点击处理 */
    void onChoiceButtonClicked();

    /** skipTypewriter() — 跳过打字机动画（鼠标点击触发） */
    void skipTypewriter();

protected:
    /** eventFilter() — 拦截 QTextBrowser 点击事件以跳过打字机 */
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    // =========================================================================
    // 内部方法
    // =========================================================================

    /** setupUi() — 构建完整游戏界面布局 */
    void setupUi();

    /** displayFullText() — 停止打字机、显示全文、启用选项按钮 */
    void displayFullText();

    // =========================================================================
    // 控件成员
    // =========================================================================

    // 中央叙事区
    QTextBrowser *m_textBrowser = nullptr;     // 文本显示区（支持 Rich Text）

    // 顶部状态栏
    QProgressBar *m_hpBar = nullptr;           // HP 进度条（红色主题）
    QProgressBar *m_moraleBar = nullptr;       // 士气进度条（蓝色主题）
    QLabel *m_nameLabel = nullptr;             // 军官姓名
    QLabel *m_classLabel = nullptr;            // 兵种名称
    QLabel *m_locationLabel = nullptr;         // 场景位置
    QLabel *m_scenarioLabel = nullptr;         // 章节/关卡名称

    // 底部选项区
    QWidget *m_optionsContainer = nullptr;     // 选项按钮容器
    QVBoxLayout *m_optionsLayout = nullptr;    // 选项垂直布局
    QList<QPushButton*> m_optionButtons;       // 动态生成的选项按钮列表

    // 控制面板
    QPushButton *m_saveBtn = nullptr;          // "保存战役"
    QPushButton *m_loadBtn = nullptr;          // "载入战役"
    QPushButton *m_exitBtn = nullptr;          // "撤回后方"

    // 打字机动画
    QString m_fullText;                         // 完整叙事文本
    int m_currentIndex = 0;                     // 当前已显示字符数
    QTimer *m_typeTimer = nullptr;              // 打字机定时器 (20ms)
    bool m_isTyping = false;                    // 是否正在打字中
};
