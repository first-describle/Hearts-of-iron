/**
 * ===========================================================================
 * MainWindow.h — 应用程序主窗口（顶层 UI + 引擎编排器）
 * ===========================================================================
 *
 * 【所属模块】UI 层 — 顶层窗口
 * 【依赖关系】所有引擎模块 + 所有 UI 控件
 * 【对应实现】MainWindow.cpp
 *
 * MainWindow 是整个应用程序的"中央指挥所"，负责:
 *   ① 拥有并管理所有引擎实例（DlcManager, DiceSystem, NodeEngine,
 *                              SaveManager, MusicPlayer）
 *   ② 构建 QStackedWidget 三层页面结构（菜单 → 角色创建 → 游戏界面）
 *   ③ 连接所有跨模块的信号/槽，实现页面导航和游戏逻辑编排
 *   ④ 持有当前游戏状态（m_player, m_currentDlc, m_currentDlcBasePath）
 *
 * 页面结构 (QStackedWidget):
 *   [0] MenuWidget           — 主菜单标题画面 + DLC 选择列表
 *   [1] CharacterCreateWidget — 兵种选择 + 姓名输入
 *   [2] GameWidget           — 核心游戏界面（叙事/选项/状态栏）
 *
 * 核心数据流 (信号驱动):
 *   用户点击 → UI 控件发射信号 → MainWindow 槽函数 → 调用引擎方法
 *   引擎事件 → 引擎发射信号 → MainWindow 槽函数 → 更新 UI 控件
 *
 *   典型流程示例:
 *     玩家点击选项 → GameWidget::choiceMade(index)
 *       → MainWindow::onChoiceMade(index)
 *       → NodeEngine::makeChoice(index)
 *       → NodeEngine::nodeChanged(node) / statsChanged(hp,morale)
 *       → MainWindow::onNodeChanged / onStatsChanged
 *       → GameWidget::showStoryNode / updatePlayerStats
 *       → SaveManager::autoSave (自动存档)
 * ===========================================================================
 */

#pragma once
#include <QMainWindow>
#include "../engine/DlcTypes.h"
#include "../engine/PlayerSystem.h"

// ── 前向声明: UI 层不需要知道引擎的完整实现细节 ──
class QStackedWidget;
class DlcManager;
class NodeEngine;
class DiceSystem;
class SaveManager;
class MusicPlayer;
class MenuWidget;
class CharacterCreateWidget;
class GameWidget;
struct StoryNode;
struct DlcManifest;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    // =========================================================================
    // 页面导航槽
    // =========================================================================

    /** showMainMenu() — 切换到主菜单页面（index 0），播放主菜单音乐 */
    void showMainMenu();

    /** showDlcSelect() — 获取 DLC 列表并让 MenuWidget 显示其内部 DLC 面板 */
    void showDlcSelect();

    /** showCharacterCreate() — 切换到角色创建页面（index 1） */
    void showCharacterCreate();

    /** showGameScreen() — 切换到游戏界面（index 2） */
    void showGameScreen();

    // =========================================================================
    // 游戏生命周期槽
    // =========================================================================

    /** onDlcSelected() — 用户选中了一个 DLC → 注册音乐、设置职业列表 */
    void onDlcSelected(const QString &dlcId);

    /** onStartGame() — 用户确认角色创建 → 创建 PlayerSystem、启动 NodeEngine */
    void onStartGame(const QString &name, const QString &classId);

    /** onLoadGame() — 用户选择读取存档 → 恢复状态、重新启动引擎 */
    void onLoadGame(const PlayerSystem &loadedPlayer);

    // =========================================================================
    // 游戏交互槽
    // =========================================================================

    /** onChoiceMade() — 玩家点击了选项 → 调用 NodeEngine::makeChoice() */
    void onChoiceMade(int index);

    /** openSaveDialog() — 打开保存对话框（模态） */
    void openSaveDialog();

    /** openLoadDialog() — 打开读取对话框（模态） */
    void openLoadDialog();

    // =========================================================================
    // 引擎事件响应槽
    // =========================================================================

    /** onNodeChanged() — 节点切换 → 更新 GameWidget 显示 + 自动存档 */
    void onNodeChanged(const StoryNode *node);

    /** onStatsChanged() — HP/士气变化 → 更新 GameWidget 血条 */
    void onStatsChanged(int hp, int morale);

    /** onCombatResult() — 战斗判定结果 → 弹出 QMessageBox */
    void onCombatResult(bool success, int hpChange, int moraleChange);

    /** onChapterVictory() — 章节胜利 → 自动存档 + 提示弹窗 */
    void onChapterVictory(const QString &chapterId);

    /** onChapterDefeat() — 章节失败 → 播放失败音乐 */
    void onChapterDefeat(const QString &chapterId);

private:
    // =========================================================================
    // 内部初始化方法
    // =========================================================================

    /** setupUi() — 构建三层 QStackedWidget 页面结构 */
    void setupUi();

    /** connectSignals() — 连接所有跨模块信号/槽 */
    void connectSignals();

    /** registerMusicFromDlc() — 从当前 DLC 清单注册所有音乐到 MusicPlayer */
    void registerMusicFromDlc();

    /** startChapterMusic() — 播放章节的第一首音乐 */
    void startChapterMusic();

    // =========================================================================
    // 成员变量 — UI 层
    // =========================================================================
    QStackedWidget *m_stackedWidget = nullptr;   // 页面切换容器

    MenuWidget            *m_menuWidget   = nullptr;  // [0] 主菜单 + DLC 选择
    CharacterCreateWidget *m_createWidget = nullptr;  // [1] 角色创建
    GameWidget            *m_gameWidget   = nullptr;  // [2] 游戏界面

    // =========================================================================
    // 成员变量 — 引擎层（MainWindow 拥有所有实例的完整生命周期）
    // =========================================================================
    DlcManager  *m_dlcManager  = nullptr;   // DLC 扫描/解析/验证
    DiceSystem  *m_diceSystem  = nullptr;   // 骰子随机判定
    NodeEngine  *m_nodeEngine  = nullptr;   // 叙事节点引擎
    SaveManager *m_saveManager = nullptr;   // 存档管理
    MusicPlayer *m_musicPlayer = nullptr;   // 背景音乐

    // =========================================================================
    // 成员变量 — 当前游戏状态
    // =========================================================================
    PlayerSystem m_player;               // 当前玩家状态（值类型，非指针）
    DlcManifest  m_currentDlc;           // 当前选择的 DLC 清单
    QString      m_currentDlcBasePath;   // 当前 DLC 的文件夹绝对路径
    QString      m_currentClassName;     // 当前兵种显示名称
    QString      m_currentChapterName;   // 当前章节显示名称
};
