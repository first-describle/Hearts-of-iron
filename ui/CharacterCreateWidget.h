/**
 * ===========================================================================
 * CharacterCreateWidget.h — 角色创建界面
 * ===========================================================================
 *
 * 【所属模块】UI 层 — 角色创建
 * 【依赖关系】DlcTypes.h (DlcClass 结构体)
 * 【对应实现】CharacterCreateWidget.cpp
 *
 * CharacterCreateWidget 是玩家创建角色的界面，提供:
 *   ① 姓名输入: QLineEdit（默认 "汉斯 · 缪勒"，最多 15 个字符）
 *   ② 兵种选择: 动态生成的职业卡片网格（2 列排列）
 *   ③ 属性预览: 右侧信息面板显示 HP/士气/描述/关卡方向
 *
 * 交互流程:
 *   ① MainWindow 调用 setClasses(classes) → 生成兵种卡片
 *   ② 玩家点击卡片 → eventFilter 捕获 → onClassSelected(index)
 *   ③ 选中卡片高亮 → updatePreview 更新右侧预览
 *   ④ 玩家输入姓名 + 点击"投身战场" → emit startGame(name, classId)
 *
 * 兵种卡片设计:
 *   - 使用 QWidget 作为卡片容器
 *   - 通过 property("classIndex") 标记索引
 *   - 通过 property("selected") 驱动 QSS 高亮
 *   - eventFilter 拦截点击事件（包括子控件的点击）
 * ===========================================================================
 */

#pragma once
#include <QWidget>
#include <QList>
#include "../engine/DlcTypes.h"

class QLineEdit;
class QLabel;
class QGridLayout;
class QPushButton;

class CharacterCreateWidget : public QWidget {
    Q_OBJECT
public:
    explicit CharacterCreateWidget(QWidget *parent = nullptr);
    ~CharacterCreateWidget() = default;

    /**
     * setClasses() — 设置可选兵种列表（由 DLC 数据驱动）
     * 清除旧卡片 → 生成新卡片网格 → 默认选中第一个
     *
     * @param classes  DLC 清单中的职业列表
     */
    void setClasses(const QList<DlcClass> &classes);

signals:
    /** backToMenu() — 返回主菜单 */
    void backToMenu();

    /**
     * startGame() — 确认角色创建，开始游戏
     * @param name    玩家输入的姓名
     * @param classId 选中的兵种 ID
     */
    void startGame(const QString &name, const QString &classId);

protected:
    /**
     * eventFilter() — 事件过滤器
     * 拦截兵种卡片上的鼠标点击事件，实现卡片选择功能。
     * 沿父控件链向上查找带 classIndex 属性的控件。
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    /** onClassSelected() — 兵种卡片选中处理 */
    void onClassSelected(int id);

    /** onStartClicked() — "投身战场" 按钮点击处理 */
    void onStartClicked();

private:
    // =========================================================================
    // 内部方法
    // =========================================================================

    /** setupUi() — 构建完整布局（姓名区 + 兵种网格 + 预览面板 + 按钮） */
    void setupUi();

    /** updatePreview() — 更新右侧兵种预览面板 */
    void updatePreview(const DlcClass &cls);

    /** rebuildClassCards() — 根据 m_classes 重建兵种卡片网格 */
    void rebuildClassCards();

    // =========================================================================
    // 控件成员
    // =========================================================================
    QLineEdit *m_nameEdit = nullptr;        // 姓名输入框
    QGridLayout *m_grid = nullptr;          // 兵种卡片网格布局（2列）
    QList<QWidget*> m_classCards;           // 兵种卡片控件列表（用于高亮切换）
    QList<DlcClass> m_classes;              // 当前可选的兵种列表

    // 右侧预览面板
    QLabel *m_hpLabel = nullptr;            // 初始生命值
    QLabel *m_moraleLabel = nullptr;        // 初始士气值
    QLabel *m_descLabel = nullptr;          // 兵种描述
    QLabel *m_scenarioLabel = nullptr;      // 关卡方向

    // 底部按钮
    QPushButton *m_startBtn = nullptr;      // "投身战场"
    QPushButton *m_backBtn = nullptr;       // "返回主菜单"

    int m_selectedIndex = 0;                // 当前选中的兵种索引
};
