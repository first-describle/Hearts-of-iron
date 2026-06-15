/**
 * ===========================================================================
 * GameWidget.cpp — 核心游戏界面实现
 * ===========================================================================
 *
 * 【所属模块】UI 层 — 游戏主界面
 * 【依赖头文件】GameWidget.h, Qt Widgets 全家桶 +
 *              QTimer (打字机), QEvent/QMouseEvent (点击跳过),
 *              QScrollBar (自动滚动)
 *
 * 本文件实现了游戏的核心用户界面，包括:
 *
 *   ① 打字机动画:
 *      使用 QTimer (20ms间隔) 逐字插入文本到 QTextBrowser。
 *      每插入一个字符后自动滚动到底部。
 *      鼠标点击 → eventFilter → skipTypewriter → displayFullText (立即完成)
 *
 *   ② 选项按钮生成:
 *      根据节点类型和选项列表动态创建 QPushButton。
 *      通过 property("choiceIndex") 传递选项索引。
 *
 *   ③ 状态显示:
 *      顶部三栏布局: 玩家信息 | HP/士气进度条 | 场景/关卡
 *
 * 界面布局结构:
 *
 *   GameWidget
 *   ├── 顶部状态栏 (gameHeader)
 *   │   ├── 左侧: 军官姓名 + 兵种
 *   │   ├── 中间: HP进度条 + 士气进度条
 *   │   └── 右侧: 章节名 + 场景位置
 *   │
 *   ├── 中央叙事区 (QTextBrowser, stretch=1)
 *   │   └── 打字机动画逐字显示
 *   │
 *   └── 底部区 (QHBoxLayout)
 *       ├── 左侧 75%: 选项按钮容器 (QVBoxLayout, 动态)
 *       └── 右侧 25%: 控制面板 (保存/读档/退出)
 * ===========================================================================
 */

#include "GameWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyle>
#include <QDebug>

// ===========================================================================
// 构造函数
// ===========================================================================
/**
 * 初始化定时器 (20ms 间隔 = 每秒 50 字符的打字速度)。
 * 为 QTextBrowser 及其 viewport 安装事件过滤器以支持点击跳过。
 */
GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent), m_typeTimer(new QTimer(this))
{
    setupUi();

    m_typeTimer->setInterval(20);  // 20ms 一个字符 → 50 字符/秒
    connect(m_typeTimer, &QTimer::timeout, this, &GameWidget::onTypeTimerTick);

    // ── 点击叙事文本区跳过打字机动画 ──
    // QTextBrowser 本身和其内部的 viewport 都需要安装事件过滤器。
    // viewport 是实际接收鼠标事件的部分。
    m_textBrowser->installEventFilter(this);
    m_textBrowser->viewport()->installEventFilter(this);
}

// ===========================================================================
// setupUi() — 构建完整游戏界面布局
// ===========================================================================
/**
 * 【布局结构详解】
 *
 *   整体: QVBoxLayout (垂直三板斧)
 *   ├── [0] 顶部状态栏 header (QWidget)
 *   │       三列水平布局 (2:3:3 比例)
 *   │
 *   ├── [1] 中央叙事区 m_textBrowser (QTextBrowser, stretch=1)
 *   │       只读、无焦点、始终显示滚动条
 *   │
 *   └── [2] 底部区 bottomLayout (QHBoxLayout)
 *           ├── 选项容器 m_optionsContainer (3/4 宽)
 *           └── 控制面板 ctrlPanel (1/4 宽)
 *               ├── 保存战役
 *               ├── 载入战役
 *               └── 撤回后方
 */
void GameWidget::setupUi() {
    setObjectName(QStringLiteral("GameWidget"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(20);

    // ========================================================================
    // 第 1 部分: 顶部状态栏 (gameHeader)
    // ========================================================================
    QWidget *header = new QWidget(this);
    header->setObjectName(QStringLiteral("gameHeader"));  // QSS: 深色背景条
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(15, 10, 15, 10);
    headerLayout->setSpacing(25);

    // --- 左侧: 玩家基本信息 (stretch=2) ---
    QVBoxLayout *playerInfoLayout = new QVBoxLayout();
    playerInfoLayout->setSpacing(2);

    m_nameLabel = new QLabel(QStringLiteral("军官: 汉斯 · 缪勒"), header);
    m_nameLabel->setObjectName(QStringLiteral("headerNameLabel"));  // QSS: 金色文字

    m_classLabel = new QLabel(QStringLiteral("兵种: 步兵"), header);
    m_classLabel->setObjectName(QStringLiteral("headerClassLabel"));

    playerInfoLayout->addWidget(m_nameLabel);
    playerInfoLayout->addWidget(m_classLabel);
    headerLayout->addLayout(playerInfoLayout, 2);

    // --- 中间: 属性进度条 (stretch=3) ---
    QVBoxLayout *statsLayout = new QVBoxLayout();
    statsLayout->setSpacing(6);

    // HP 进度条行
    QHBoxLayout *hpRow = new QHBoxLayout();
    QLabel *hpl = new QLabel(QStringLiteral("生命:"), header);
    hpl->setObjectName(QStringLiteral("headerStatLabel"));
    m_hpBar = new QProgressBar(header);
    m_hpBar->setObjectName(QStringLiteral("gameHpBar"));  // QSS: 红色进度条
    m_hpBar->setRange(0, 100);
    m_hpBar->setValue(100);
    m_hpBar->setTextVisible(true);
    m_hpBar->setFormat(QStringLiteral("%v%"));  // 显示 "85%"
    hpRow->addWidget(hpl);
    hpRow->addWidget(m_hpBar);

    // 士气进度条行
    QHBoxLayout *morRow = new QHBoxLayout();
    QLabel *morl = new QLabel(QStringLiteral("士气:"), header);
    morl->setObjectName(QStringLiteral("headerStatLabel"));
    m_moraleBar = new QProgressBar(header);
    m_moraleBar->setObjectName(QStringLiteral("gameMoraleBar"));  // QSS: 蓝色进度条
    m_moraleBar->setRange(0, 100);
    m_moraleBar->setValue(100);
    m_moraleBar->setTextVisible(true);
    m_moraleBar->setFormat(QStringLiteral("%v%"));
    morRow->addWidget(morl);
    morRow->addWidget(m_moraleBar);

    statsLayout->addLayout(hpRow);
    statsLayout->addLayout(morRow);
    headerLayout->addLayout(statsLayout, 3);

    // --- 右侧: 场景/关卡信息 (stretch=3, 右对齐) ---
    QVBoxLayout *scenLayout = new QVBoxLayout();
    scenLayout->setSpacing(2);
    scenLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_scenarioLabel = new QLabel(QStringLiteral("黄色方案"), header);
    m_scenarioLabel->setObjectName(QStringLiteral("headerScenarioLabel"));
    m_scenarioLabel->setAlignment(Qt::AlignRight);

    m_locationLabel = new QLabel(QStringLiteral("阿登森林"), header);
    m_locationLabel->setObjectName(QStringLiteral("headerLocationLabel"));
    m_locationLabel->setAlignment(Qt::AlignRight);

    scenLayout->addWidget(m_scenarioLabel);
    scenLayout->addWidget(m_locationLabel);
    headerLayout->addLayout(scenLayout, 3);

    mainLayout->addWidget(header);

    // ========================================================================
    // 第 2 部分: 中央文本叙事区
    // ========================================================================
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setObjectName(QStringLiteral("gameTextBrowser"));
    m_textBrowser->setReadOnly(true);                            // 只读
    m_textBrowser->setFocusPolicy(Qt::NoFocus);                  // 不接受键盘焦点
    m_textBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);   // 始终显示垂直滚动条
    m_textBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
    mainLayout->addWidget(m_textBrowser, 1);  // stretch=1: 占用所有剩余空间

    // ========================================================================
    // 第 3 部分: 底部选项与控制区
    // ========================================================================
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(20);

    // --- 左侧: 选项按钮容器 (stretch=3) ---
    m_optionsContainer = new QWidget(this);
    m_optionsContainer->setObjectName(QStringLiteral("optionsContainer"));
    m_optionsLayout = new QVBoxLayout(m_optionsContainer);
    m_optionsLayout->setContentsMargins(0, 0, 0, 0);
    m_optionsLayout->setSpacing(8);
    bottomLayout->addWidget(m_optionsContainer, 3);

    // --- 右侧: 游戏控制面板 (stretch=1) ---
    QWidget *ctrlPanel = new QWidget(this);
    ctrlPanel->setObjectName(QStringLiteral("gameCtrlPanel"));
    QVBoxLayout *ctrlLayout = new QVBoxLayout(ctrlPanel);
    ctrlLayout->setContentsMargins(10, 10, 10, 10);
    ctrlLayout->setSpacing(8);

    m_saveBtn = new QPushButton(QStringLiteral("保存战役"), ctrlPanel);
    m_saveBtn->setObjectName(QStringLiteral("gameCtrlBtn"));
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    connect(m_saveBtn, &QPushButton::clicked, this, &GameWidget::saveClicked);

    m_loadBtn = new QPushButton(QStringLiteral("载入战役"), ctrlPanel);
    m_loadBtn->setObjectName(QStringLiteral("gameCtrlBtn"));
    m_loadBtn->setCursor(Qt::PointingHandCursor);
    connect(m_loadBtn, &QPushButton::clicked, this, &GameWidget::loadClicked);

    m_exitBtn = new QPushButton(QStringLiteral("撤回后方"), ctrlPanel);
    m_exitBtn->setObjectName(QStringLiteral("gameCtrlBtn"));
    m_exitBtn->setCursor(Qt::PointingHandCursor);
    connect(m_exitBtn, &QPushButton::clicked, this, &GameWidget::exitClicked);

    ctrlLayout->addWidget(m_saveBtn);
    ctrlLayout->addWidget(m_loadBtn);
    ctrlLayout->addWidget(m_exitBtn);
    bottomLayout->addWidget(ctrlPanel, 1);

    mainLayout->addLayout(bottomLayout);
}

// ===========================================================================
// updatePlayerStats() / updatePlayerInfo() — 状态更新
// ===========================================================================
void GameWidget::updatePlayerStats(int hp, int morale) {
    m_hpBar->setValue(hp);
    m_moraleBar->setValue(morale);
}

void GameWidget::updatePlayerInfo(const QString &name, const QString &className) {
    m_nameLabel->setText(QStringLiteral("军官: %1").arg(name));
    m_classLabel->setText(QStringLiteral("兵种: %1").arg(className));
}

// ===========================================================================
// showStoryNode() — 显示叙事节点（核心方法）
// ===========================================================================
/**
 * 【执行流程】
 *
 *   ① 停止打字机定时器（若有正在进行的打字动画）
 *   ② 更新场景/关卡标签
 *   ③ 获取文本: node->textFor(playerClassId) —— 职业专属文本优先
 *   ④ 重置打字机状态: m_currentIndex=0, m_isTyping=true
 *   ⑤ 清空 QTextBrowser
 *   ⑥ 禁用存读档按钮（打字期间防止误操作）
 *   ⑦ 清除旧选项按钮
 *   ⑧ 根据节点类型生成新选项按钮:
 *
 *      ◆ Choice 节点:
 *        遍历 choices[]，跳过:
 *          - classRestricted 且职业不在 allowedClasses 中的选项
 *       每个有效选项生成一个 QPushButton
 *         - 战斗选项: objectName="combatOptionBtn" (QSS: 红色边框)
 *         - 普通选项: objectName="choiceOptionBtn"
 *       按钮初始隐藏 (setVisible(false))，待打字结束后显示
 *
 *      ◆ Narrative 节点:
 *        生成单个【继续】按钮，choiceIndex=0
 *
 *      ◆ Victory 节点:
 *        非最后一章 → 【进入下一战役】(emit choiceMade(-2))
 *        最后一章   → 【返回主页 — 铭记历史】(emit exitClicked)
 *
 *      ◆ Defeat 节点:
 *        【返回主页 — 重新集结】(emit exitClicked)
 *
 *   ⑨ 启动打字机定时器 → onTypeTimerTick 开始逐字输出
 */
void GameWidget::showStoryNode(const StoryNode *node, const QString &playerClassId,
                                const QString &chapterName, bool isLastChapter) {
    if (!node) return;

    // ── 停止当前打字机 ──
    m_typeTimer->stop();

    // ── 更新场景信息 ──
    m_locationLabel->setText(node->locationTitle);
    m_scenarioLabel->setText(chapterName);

    // ── 获取职业专属文本（有则用，无则回退到泛用文本） ──
    m_fullText = node->textFor(playerClassId);
    m_currentIndex = 0;
    m_isTyping = true;
    m_textBrowser->clear();

    // ── 打字期间禁用存读档 ──
    m_saveBtn->setEnabled(false);
    m_loadBtn->setEnabled(false);

    // ── 清空旧选项按钮 ──
    QLayoutItem *child;
    while ((child = m_optionsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    m_optionButtons.clear();

    // ========================================================================
    // 生成选项按钮
    // ========================================================================
    int choiceIdx = 0;

    // --- Choice 节点: 遍历 choices[] 生成按钮 ---
    for (const Choice &choice : node->choices) {
        // 职业限制过滤: 跳过不允许当前职业的选项
        if (choice.classRestricted && !choice.allowedClasses.contains(playerClassId)) {
            continue;
        }

        QPushButton *btn = new QPushButton(choice.text, m_optionsContainer);

        // 战斗选项 vs 普通选项，使用不同的 QSS 样式
        btn->setObjectName(choice.isCombat
                           ? QStringLiteral("combatOptionBtn")   // QSS: 红色边框
                           : QStringLiteral("choiceOptionBtn")); // QSS: 普通样式
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);
        btn->setVisible(false);  // 打字结束前隐藏

        m_optionsLayout->addWidget(btn);
        m_optionButtons.append(btn);

        connect(btn, &QPushButton::clicked, this, &GameWidget::onChoiceButtonClicked);
        btn->setProperty("choiceIndex", choiceIdx);  // 存储选项索引
        choiceIdx++;
    }

    // --- Narrative 节点: 单个【继续】按钮 ---
    if (node->type == NodeType::Narrative) {
        QPushButton *btn = new QPushButton(QStringLiteral("【继续】"), m_optionsContainer);
        btn->setObjectName(QStringLiteral("choiceOptionBtn"));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);
        btn->setVisible(false);
        m_optionsLayout->addWidget(btn);
        m_optionButtons.append(btn);

        connect(btn, &QPushButton::clicked, this, &GameWidget::onChoiceButtonClicked);
        btn->setProperty("choiceIndex", 0);  // Narrative 的继续使用 index=0
    }

    // --- Ending / Victory / Defeat 节点: 特殊跳转按钮 ---
    if (node->type == NodeType::Ending || node->isVictory || node->isDefeat) {
        QPushButton *btn = new QPushButton(m_optionsContainer);
        btn->setObjectName(QStringLiteral("choiceOptionBtn"));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);
        btn->setVisible(false);

        if (node->isVictory) {
            if (isLastChapter) {
                // 最后一章胜利 → 返回主页（通关）
                btn->setText(QStringLiteral("【返回主页 — 铭记历史】"));
                connect(btn, &QPushButton::clicked, this, &GameWidget::exitClicked);
            } else {
                // 非最后一章胜利 → 进入下一战役
                btn->setText(QStringLiteral("【进入下一战役】"));
                connect(btn, &QPushButton::clicked, this, [this]() {
                    emit choiceMade(-2);  // -2 = 特殊值: 进入下一章节
                });
            }
        } else {
            // 失败结局 → 返回主页
            btn->setText(QStringLiteral("【返回主页 — 重新集结】"));
            connect(btn, &QPushButton::clicked, this, &GameWidget::exitClicked);
        }

        m_optionsLayout->addWidget(btn);
        m_optionButtons.append(btn);
    }

    // ── 启动打字机 ──
    m_typeTimer->start();
}

// ===========================================================================
// onTypeTimerTick() — 打字机定时器回调
// ===========================================================================
/**
 * 每 20ms 触发一次: 向 QTextBrowser 插入一个字符。
 *
 * 【实现方式】
 *   使用 QTextCursor 在文本末尾插入单个字符。
 *   比 setPlainText(partial) 更高效 —— 不需要每次重写全部文本。
 *   插入后自动滚动到底部。
 *
 * 【终止条件】
 *   当 m_currentIndex >= m_fullText.length() 时 → displayFullText()
 */
void GameWidget::onTypeTimerTick() {
    if (m_currentIndex < m_fullText.length()) {
        QTextCursor cursor = m_textBrowser->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(m_fullText.at(m_currentIndex));  // 插入单个字符
        m_currentIndex++;

        // 强制滚动到底部
        m_textBrowser->verticalScrollBar()->setValue(
            m_textBrowser->verticalScrollBar()->maximum());
    } else {
        // 打字完成 → 显示全文并启用按钮
        displayFullText();
    }
}

// ===========================================================================
// skipTypewriter() — 跳过打字机动画
// ===========================================================================
void GameWidget::skipTypewriter() {
    if (m_isTyping) {
        displayFullText();
    }
}

// ===========================================================================
// displayFullText() — 显示全文并启用交互
// ===========================================================================
/**
 * 【执行步骤】
 *   ① 停止定时器
 *   ② 用 setPlainText 一次性显示全部文本（确保 Rich Text 正确渲染）
 *   ③ 显示所有选项按钮（打字期间它们被隐藏）
 *   ④ 重新启用存读档按钮
 */
void GameWidget::displayFullText() {
    m_typeTimer->stop();
    m_isTyping = false;

    // 渲染全部文本（保证 QSS 样式正确）
    m_textBrowser->setPlainText(m_fullText);
    m_textBrowser->verticalScrollBar()->setValue(
        m_textBrowser->verticalScrollBar()->maximum());

    // ── 显示选项按钮 ──
    for (QPushButton *btn : m_optionButtons) {
        btn->setVisible(true);
    }

    // ── 启用控制按钮 ──
    m_saveBtn->setEnabled(true);
    m_loadBtn->setEnabled(true);
}

// ===========================================================================
// onChoiceButtonClicked() — 选项按钮点击处理
// ===========================================================================
/**
 * 通过 sender() 获取被点击的按钮，读取其 choiceIndex 属性值，
 * 发射 choiceMade(index) 信号。
 *
 * choiceIndex 的值含义:
 *   >= 0: 选项在 choices[] 中的索引
 *   其他: 特殊值（如 -2 = 进入下一章），由 showStoryNode 直接连接
 */
void GameWidget::onChoiceButtonClicked() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int idx = btn->property("choiceIndex").toInt();
    emit choiceMade(idx);
}

// ===========================================================================
// eventFilter() — 点击跳过打字机
// ===========================================================================
/**
 * 拦截 QTextBrowser 及其 viewport 上的鼠标左键点击。
 * 若正在打字中 → skipTypewriter() → 立即显示全文。
 * 若打字已完成 → 事件正常传播（允许文本选择和滚动）。
 */
bool GameWidget::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            skipTypewriter();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
