/**
 * ===========================================================================
 * SaveLoadDialog.cpp — 存档/读档模态对话框实现
 * ===========================================================================
 *
 * 【所属模块】UI 层 — 存档对话框
 * 【依赖头文件】SaveLoadDialog.h, Qt Widgets 全家桶 + QMessageBox +
 *              QStyle, QDateTime, QMouseEvent
 *
 * 本文件实现了 4 槽位存档管理界面的完整交互:
 *   - 初始化: 读取 4 个存档文件的元数据，填充卡片
 *   - 选择: 点击卡片高亮，启用/禁用操作按钮
 *   - 保存: 序列化 PlayerSystem → JSON → 写入文件
 *   - 读取: 反序列化 JSON → PlayerSystem → 发射 gameLoaded 信号
 *   - 删除: 确认后删除存档文件 → 刷新卡片
 *
 * 卡片交互:
 *   使用透明 QPushButton 覆盖在卡片上作为点击触发器。
 *   QButtonGroup 管理互斥选择（同一时间只有一个槽位被选中）。
 *   触发器大小在 refreshSlots 中自适应卡片大小。
 * ===========================================================================
 */

#include "SaveLoadDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QButtonGroup>
#include <QMessageBox>
#include <QStyle>
#include <QDateTime>
#include <QMouseEvent>

// ===========================================================================
// 构造函数
// ===========================================================================
SaveLoadDialog::SaveLoadDialog(bool savingMode, SaveManager *saveManager,
                               PlayerSystem *player, QWidget *parent)
    : QDialog(parent), m_savingMode(savingMode),
      m_saveManager(saveManager), m_player(player)
{
    setupUi();
    refreshSlots();  // 填充 4 个槽位的存档信息
}

// ===========================================================================
// setDlcInfo() — 设置存档摘要信息
// ===========================================================================
void SaveLoadDialog::setDlcInfo(const QString &dlcTitle, const QString &className,
                                 const QString &chapterName) {
    m_dlcTitle = dlcTitle;
    m_className = className;
    m_chapterName = chapterName;
}

// ===========================================================================
// setupUi() — 构建 4 槽位卡片 + 控制按钮
// ===========================================================================
/**
 * 【布局结构】
 *
 *   SaveLoadDialog (QDialog, 480×520)
 *   ├── 标题: "选择要覆盖的战役存档：" / "选择要继续的战役记录："
 *   │
 *   ├── 4 个槽位卡片 (垂直排列)
 *   │   每个卡片: QWidget#slotCard
 *   │   ├── 卡片标题: "【自动存档】" / "【战役存档 1】"
 *   │   ├── 存档摘要: "DLC: xxx | 军官: xxx | 兵种: xxx | 章节: xxx"
 *   │   ├── 时间戳:   "记录时间: 2026-06-15 14:30:00"
 *   │   └── 透明 QPushButton 覆盖层 (作为点击触发器)
 *   │
 *   └── 底部按钮行
 *       ├── 删除记录 (左)
 *       ├── spacer
 *       ├── 取消 (右)
 *       └── 保存记录 / 开始读取 (最右)
 */
void SaveLoadDialog::setupUi() {
    setWindowTitle(m_savingMode ? QStringLiteral("保存战役") : QStringLiteral("继续战役"));
    setMinimumSize(480, 520);
    resize(500, 560);
    setObjectName(QStringLiteral("SaveLoadDialog"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // ── 顶部标题 ──
    QLabel *titleLabel = new QLabel(
        m_savingMode ? QStringLiteral("选择要覆盖的战役存档：")
                     : QStringLiteral("选择要继续的战役记录："),
        this);
    titleLabel->setObjectName(QStringLiteral("dialogTitle"));
    mainLayout->addWidget(titleLabel);

    // ── 槽位容器 ──
    QVBoxLayout *slotsLayout = new QVBoxLayout();
    slotsLayout->setSpacing(10);

    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);  // 互斥选择: 同时只有一个槽位被选中

    for (int i = 0; i < 4; ++i) {
        // --- 卡片容器 ---
        QWidget *card = new QWidget(this);
        card->setObjectName(QStringLiteral("slotCard"));  // QSS: 卡片样式
        card->setCursor(Qt::PointingHandCursor);

        QHBoxLayout *cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(15, 10, 15, 10);

        // --- 文本信息区 ---
        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(4);

        QLabel *sTitle = new QLabel(card);
        sTitle->setObjectName(QStringLiteral("slotTitle"));  // QSS: 加粗标题

        QLabel *sInfo = new QLabel(card);
        sInfo->setObjectName(QStringLiteral("slotInfo"));    // QSS: 灰色信息

        QLabel *sTime = new QLabel(card);
        sTime->setObjectName(QStringLiteral("slotTime"));    // QSS: 小字时间

        textLayout->addWidget(sTitle);
        textLayout->addWidget(sInfo);
        textLayout->addWidget(sTime);
        cardLayout->addLayout(textLayout);

        // --- 保存模式下自动存档位不可选 ---
        if (m_savingMode && i == SaveManager::AUTO_SLOT) {
            card->setEnabled(false);
            card->setToolTip(QStringLiteral("自动存档不支持手动覆盖"));
        }

        slotsLayout->addWidget(card);

        // --- 记录 UI 引用 ---
        SlotUI ui;
        ui.card = card;
        ui.titleLabel = sTitle;
        ui.infoLabel = sInfo;
        ui.timeLabel = sTime;
        m_slotsUI.append(ui);

        // --- 透明触发器按钮（覆盖在卡片上） ---
        // 使用透明 QPushButton 作为点击区域，比自定义 mousePressEvent 更简单可靠。
        QPushButton *trigger = new QPushButton(card);
        trigger->setObjectName(QStringLiteral("slotTrigger"));
        trigger->setGeometry(0, 0, 500, 100);  // 初始大小（会被 refreshSlots 中 resize 覆盖）
        trigger->setFlat(true);                  // 扁平化（无边框）
        trigger->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
        m_buttonGroup->addButton(trigger, i);

        connect(trigger, &QPushButton::clicked, this, [this, i]() {
            onSlotSelected(i);
        });
    }
    mainLayout->addLayout(slotsLayout);

    // ── 底部控制按钮 ──
    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    ctrlLayout->setSpacing(12);

    m_actionBtn = new QPushButton(
        m_savingMode ? QStringLiteral("保存记录") : QStringLiteral("开始读取"), this);
    m_actionBtn->setObjectName(QStringLiteral("dialogActionBtn"));  // QSS: 红色强调按钮
    m_actionBtn->setEnabled(false);  // 未选择槽位时禁用
    connect(m_actionBtn, &QPushButton::clicked, this, &SaveLoadDialog::onActionTriggered);

    m_deleteBtn = new QPushButton(QStringLiteral("删除记录"), this);
    m_deleteBtn->setObjectName(QStringLiteral("dialogDeleteBtn"));
    m_deleteBtn->setEnabled(false);  // 未选择有效槽位时禁用
    connect(m_deleteBtn, &QPushButton::clicked, this, &SaveLoadDialog::onDeleteTriggered);

    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_cancelBtn->setObjectName(QStringLiteral("dialogCancelBtn"));
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    ctrlLayout->addWidget(m_deleteBtn);
    ctrlLayout->addStretch();
    ctrlLayout->addWidget(m_cancelBtn);
    ctrlLayout->addWidget(m_actionBtn);

    mainLayout->addLayout(ctrlLayout);
}

// ===========================================================================
// refreshSlots() — 刷新所有槽位卡片
// ===========================================================================
/**
 * 从 SaveManager 读取 4 个槽位的元数据摘要，更新卡片显示。
 *
 * - 有效存档: 显示 DLC/角色/兵种/章节 + 时间戳
 * - 空槽位:   显示 "战区通讯中断 — 无存档数据"
 *
 * 读取模式下的空槽位卡片被禁用（不可选择）。
 *
 * 同时调整透明触发器的大小以匹配卡片实际尺寸。
 */
void SaveLoadDialog::refreshSlots() {
    QList<SaveInfo> infos = m_saveManager->listSaves();

    for (int i = 0; i < 4; ++i) {
        const SaveInfo &info = infos[i];
        SlotUI &ui = m_slotsUI[i];

        // ── 调整触发器大小以匹配卡片 ──
        QPushButton *trigger = qobject_cast<QPushButton*>(m_buttonGroup->button(i));
        if (trigger) {
            trigger->resize(ui.card->size());
        }

        // ── 槽位标题 ──
        QString titleText = (i == SaveManager::AUTO_SLOT)
            ? QStringLiteral("【自动存档】")
            : QString::asprintf("【战役存档 %d】", i);
        ui.titleLabel->setText(titleText);

        // ── 填充存档信息 ──
        if (info.valid) {
            ui.infoLabel->setText(
                QStringLiteral("DLC: %1 | 军官: %2 | 兵种: %3 | 章节: %4")
                    .arg(info.dlcTitle, info.playerName, info.className, info.chapterName));
            ui.timeLabel->setText(QStringLiteral("记录时间: %1").arg(info.timestamp));
            ui.card->setProperty("empty", false);
        } else {
            ui.infoLabel->setText(QStringLiteral("战区通讯中断 — 无存档数据"));
            ui.timeLabel->setText(QStringLiteral("-"));
            ui.card->setProperty("empty", true);

            // 读取模式下空槽位不可选
            if (!m_savingMode) {
                m_buttonGroup->button(i)->setEnabled(false);
                ui.card->setEnabled(false);
            }
        }

        // ── 强制刷新 QSS 样式 ──
        ui.card->style()->unpolish(ui.card);
        ui.card->style()->polish(ui.card);
    }
}

// ===========================================================================
// onSlotSelected() — 槽位选中处理
// ===========================================================================
/**
 * 更新选中卡片的高亮，并根据模式调整按钮的可用性:
 *
 *   保存模式:
 *     - 自动存档 (slot 0): 不可选择
 *     - 手动存档 (1~3): 可选择，action 按钮启用
 *     - delete 按钮: 仅槽位有数据时启用
 *
 *   读取模式:
 *     - 仅槽位有数据时才可选择
 *     - action 按钮: 选中有效槽位时启用
 *     - delete 按钮: 非自动存档 + 有数据时启用
 */
void SaveLoadDialog::onSlotSelected(int id) {
    m_selectedSlot = id;

    // ── 更新选中高亮 ──
    for (int i = 0; i < 4; ++i) {
        QWidget *card = m_slotsUI[i].card;
        card->setProperty("selected", (i == id));  // QSS: [selected="true"] 红色边框
        card->style()->unpolish(card);
        card->style()->polish(card);
    }

    // ── 检查槽位数据有效性 ──
    QList<SaveInfo> infos = m_saveManager->listSaves();
    bool slotHasData = (id >= 0 && id < 4) ? infos[id].valid : false;

    // 删除按钮: 仅非自动存档 + 有数据可用
    m_deleteBtn->setEnabled(id != SaveManager::AUTO_SLOT && slotHasData);

    // 操作按钮
    if (m_savingMode) {
        m_actionBtn->setEnabled(id != SaveManager::AUTO_SLOT);  // 保存: 非自动存档即可
    } else {
        m_actionBtn->setEnabled(slotHasData);                   // 读取: 必须有数据
    }
}

// ===========================================================================
// onActionTriggered() — 执行保存/读取
// ===========================================================================
/**
 * 【保存流程】
 *   ① 调用 m_saveManager->saveGame(selectedSlot, player, dlcInfo...)
 *   ② 成功 → QMessageBox 提示 → accept() 关闭对话框
 *   ③ 失败 → QMessageBox 警告
 *
 * 【读取流程】
 *   ① 调用 m_saveManager->loadGame(selectedSlot, loadedPlayer)
 *   ② 成功 → emit gameLoaded(loadedPlayer) → accept()
 *   ③ 失败 → QMessageBox 警告
 */
void SaveLoadDialog::onActionTriggered() {
    if (m_selectedSlot < 0) return;

    if (m_savingMode) {
        // ── 保存 ──
        if (m_saveManager->saveGame(m_selectedSlot, *m_player,
                                     m_dlcTitle, m_className, m_chapterName)) {
            QMessageBox::information(this, QStringLiteral("存档成功"),
                                     QStringLiteral("战役记录已成功保存。"));
            accept();  // 关闭对话框（返回 QDialog::Accepted）
        } else {
            QMessageBox::warning(this, QStringLiteral("错误"),
                                 QStringLiteral("无法保存战役，请检查磁盘权限。"));
        }
    } else {
        // ── 读取 ──
        PlayerSystem loadedPlayer;
        if (m_saveManager->loadGame(m_selectedSlot, loadedPlayer)) {
            emit gameLoaded(loadedPlayer);  // MainWindow::onLoadGame 接收
            accept();
        } else {
            QMessageBox::warning(this, QStringLiteral("错误"),
                                 QStringLiteral("存档读取失败或数据损坏。"));
        }
    }
}

// ===========================================================================
// onDeleteTriggered() — 删除存档
// ===========================================================================
/**
 * 弹出确认对话框 → Yes → 删除文件 → 重置选中状态 → refreshSlots
 * 删除后自动取消选中（m_selectedSlot = -1），禁用操作按钮。
 */
void SaveLoadDialog::onDeleteTriggered() {
    if (m_selectedSlot < 0 || m_selectedSlot == SaveManager::AUTO_SLOT) return;

    auto res = QMessageBox::question(this, QStringLiteral("删除确认"),
                                     QStringLiteral("确认要彻底抹去该战役记录吗？这无法恢复。"),
                                     QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::Yes) {
        m_saveManager->deleteSlot(m_selectedSlot);
        m_selectedSlot = -1;
        m_deleteBtn->setEnabled(false);
        m_actionBtn->setEnabled(false);

        // ── 重置所有卡片的高亮 ──
        for (int i = 0; i < 4; ++i) {
            m_slotsUI[i].card->setProperty("selected", false);
        }

        refreshSlots();
    }
}

// ===========================================================================
// eventFilter() — 事件过滤器（当前未使用）
// ===========================================================================
bool SaveLoadDialog::eventFilter(QObject *obj, QEvent *event) {
    return QDialog::eventFilter(obj, event);
}
