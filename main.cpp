/**
 * ===========================================================================
 * main.cpp — 应用程序入口点 (Application Entry Point)
 * ===========================================================================
 *
 * 【所属项目】钢铁意志：第三帝国的黄昏 (HeartsOfIronGame)
 * 【技术栈】  C++17 + Qt 6 (Widgets + Multimedia)
 * 【架构】    JSON 数据驱动的交互式文字叙事游戏引擎
 *
 * 本文件是整个应用程序的启动入口，负责以下工作流程:
 *   1. 创建 QApplication 实例（Qt 事件循环核心）
 *   2. 从 Qt 资源系统加载全局 QSS 样式表，统一视觉主题
 *   3. 创建并显示主窗口 MainWindow
 *   4. 进入 Qt 事件循环 (app.exec())，开始响应用户交互
 *
 * 调用链: main() → QApplication → MainWindow → 各引擎模块 + UI 页面
 * ===========================================================================
 */

#include <QApplication>   // Qt 应用程序类，管理事件循环和全局设置
#include <QFile>           // Qt 文件读写类，用于加载 QSS 样式表
#include "ui/MainWindow.h" // 主窗口类，所有 UI 和引擎模块的顶层容器

/**
 * ┌──────────────────────────────────────────────────────────────────────────┐
 * │ main() — 程序入口函数                                                    │
 * ├──────────────────────────────────────────────────────────────────────────┤
 * │ @param argc  命令行参数个数（由操作系统传入）                              │
 * │ @param argv  命令行参数字符串数组                                         │
 * │ @return      应用程序退出码（0 = 正常退出）                               │
 * │                                                                          │
 * │ 执行流程:                                                                │
 * │   ① 创建 QApplication 对象 → Qt 框架初始化，解析命令行参数               │
 * │   ② 加载 :/style.qss 资源文件 → 读取全局样式表文本                       │
 * │   ③ a.setStyleSheet() → 将 QSS 应用到整个应用程序的所有控件              │
 * │   ④ 创建 MainWindow 实例 → 初始化引擎模块、构建 UI、连接信号槽           │
 * │   ⑤ w.show() → 显示主窗口                                                │
 * │   ⑥ a.exec() → 进入 Qt 事件循环，阻塞等待用户交互直到窗口关闭            │
 * └──────────────────────────────────────────────────────────────────────────┘
 */
int main(int argc, char *argv[]) {
    // ── 第 1 步: 创建 QApplication ──────────────────────────────────────────
    // QApplication 是每个 Qt Widgets 应用必须创建的对象。
    // 它管理 GUI 程序的控制流和主要设置（如字体、调色板、剪贴板等）。
    // argc/argv 传递给 Qt 以便解析内置命令行选项（如 -style）。
    QApplication a(argc, argv);

    // ── 第 2~3 步: 加载并应用全局 QSS 样式表 ───────────────────────────────
    // QSS (Qt Style Sheets) 语法类似 CSS，用于统一控制所有控件的视觉风格。
    // ":/style.qss" 中的 ":" 前缀表示从 Qt 资源系统 (.qrc) 中读取。
    // 资源文件在编译时被嵌入到可执行文件中，无需外部文件依赖。
    //
    // 【资源路径解析】
    //   :/style.qss  → resources/resources.qrc 中注册的文件
    //                 实际磁盘文件为 resources/style.qss (~454行)
    //                 主题风格: "工业战损" 暗色调，红色 #aa3333 强调
    QFile file(QStringLiteral(":/style.qss"));
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        // QFile::ReadOnly → 只读模式打开
        // QFile::Text     → 文本模式，自动处理换行符转换
        // file.readAll()  → 读取全部字节到 QByteArray
        // QString::fromUtf8() → 将 UTF-8 字节数组转为 QString
        // a.setStyleSheet()   → 将此 QSS 应用到整个应用程序
        a.setStyleSheet(QString::fromUtf8(file.readAll()));
    }
    // 若文件打开失败（如资源缺失），样式表为空，程序使用系统默认风格运行

    // ── 第 4~5 步: 创建并显示主窗口 ─────────────────────────────────────────
    // MainWindow 构造函数内完成:
    //   • 创建 6 个引擎模块实例 (DlcManager, DiceSystem, NodeEngine,
    //                            SaveManager, MusicPlayer)
    //   • 构建 QStackedWidget 三层页面 (菜单 → 角色创建 → 游戏界面)
    //   • 连接所有信号/槽实现页面导航和游戏逻辑
    //   • 扫描 dlc/ 目录加载所有 DLC 数据包
    //   • 播放主菜单主题音乐
    MainWindow w;
    w.show();  // 显示窗口（Qt 自动调用底层平台 API 创建原生窗口）

    // ── 第 6 步: 进入 Qt 事件循环 ───────────────────────────────────────────
    // a.exec() 启动 Qt 事件循环，这是 GUI 程序的核心机制。
    // 事件循环不断从操作系统获取事件（鼠标点击、键盘输入、定时器等），
    // 将它们分发给对应的 QObject 子类处理。
    // 当最后一个窗口关闭时，exec() 返回退出码，程序结束。
    return a.exec();
}
