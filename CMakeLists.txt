# ===========================================================================
# CMakeLists.txt — 项目构建配置文件
# ===========================================================================
# 【项目名称】HeartsOfIronGame (输出名: "钢铁意志_第三帝国")
# 【版本号】  2.0.0
# 【语言标准】C++17
# 【构建系统】CMake 3.16+
# 【依赖项】  Qt 6 (Core + Widgets + Multimedia)
#
# 本文件定义了:
#   ① 编译器设置（C++标准、Qt 自动代码生成）
#   ② 源文件与头文件列表
#   ③ Qt 可执行文件构建规则
#   ④ 链接库与头文件搜索路径
#   ⑤ 构建后 DLC 目录复制
#   ⑥ 输出文件命名
# ===========================================================================

# ── 第 1 节: CMake 版本要求与项目声明 ────────────────────────────────────────
# cmake_minimum_required: 指定构建所需的最低 CMake 版本
cmake_minimum_required(VERSION 3.16)

# project(): 声明项目名称、版本号、编程语言
#   HeartsOfIronGame — 内部项目名称，用于引用 target
#   VERSION 2.0.0    — 语义化版本号
#   LANGUAGES CXX    — 使用 C++ 编译器
project(HeartsOfIronGame VERSION 2.0.0 LANGUAGES CXX)

# ── 第 2 节: 编译器与 Qt 自动化设置 ──────────────────────────────────────────
# C++17 标准: 必须启用（代码使用了 C++17 特性如 std::clamp, if constexpr）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt Meta-Object Compiler (MOC) 自动化
#   AUTOMOC: 自动处理 Q_OBJECT 宏 → 生成元对象代码（信号/槽机制核心）
#   AUTORCC: 自动编译 .qrc 资源文件 → 将资源嵌入可执行文件
#   AUTOUIC: 自动编译 .ui 界面文件 → 本项目无 .ui 文件（纯代码手写UI）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── 第 3 节: Qt6 依赖查找 ────────────────────────────────────────────────────
# find_package 查找系统中安装的 Qt6 库
#   REQUIRED: 必须找到，否则 CMake 配置失败
#   COMPONENTS:
#     Core        — Qt 核心库（QString, QObject, 事件循环, JSON 解析等）
#     Widgets     — Qt 桌面控件库（QMainWindow, QPushButton, QProgressBar 等）
#     Multimedia  — Qt 多媒体库（QMediaPlayer, QAudioOutput — 背景音乐播放）
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)

# ── 第 4 节: 源文件列表 ──────────────────────────────────────────────────────
# 将所有 .cpp 实现文件集中列出，便于统一管理

set(SOURCES
    main.cpp                          # 【入口点】创建 QApplication + 加载 QSS + 启动主窗口

    # --- 引擎层 (engine/) — 6 个模块，负责所有游戏逻辑 ---
    engine/PlayerSystem.cpp           # 【玩家系统】HP/士气/标志/章节进度管理 + JSON 序列化
    engine/DiceSystem.cpp             # 【骰子系统】D100 随机掷骰 + 职业加成计算
    engine/DlcManager.cpp             # 【DLC管理】扫描/解析/验证 manifest.json + 加载章节 JSON
    engine/NodeEngine.cpp             # 【节点引擎】叙事节点导航/选项执行/战斗投骰/章节切换
    engine/SaveManager.cpp            # 【存档管理】4 槽位 (1自动+3手动) JSON 存档系统
    engine/MusicPlayer.cpp            # 【音乐播放】背景音乐注册/播放/循环/静音控制

    # --- UI 层 (ui/) — 5 个控件，负责所有用户界面 ---
    ui/MainWindow.cpp                 # 【主窗口】引擎实例持有者 + QStackedWidget 页面路由 + 信号槽编排
    ui/MenuWidget.cpp                 # 【主菜单】标题画面 + DLC 选择列表
    ui/GameWidget.cpp                 # 【游戏界面】打字机文本/选项按钮/状态栏/控制面板
    ui/CharacterCreateWidget.cpp      # 【角色创建】兵种卡片网格 + 姓名输入 + 属性预览
    ui/SaveLoadDialog.cpp             # 【存档对话框】4 槽位卡片 + 保存/读取/删除操作
)

# ── 第 5 节: 头文件列表 ──────────────────────────────────────────────────────
# 头文件列出用于 MOC 处理（AUTOMOC 会扫描这些文件中的 Q_OBJECT 宏）

set(HEADERS
    # --- 引擎层头文件 ---
    engine/DlcTypes.h                 # 【共享类型】所有运行时数据结构定义（无 .cpp 对应）
    engine/PlayerSystem.h             # 【玩家系统】接口声明
    engine/DiceSystem.h               # 【骰子系统】接口声明
    engine/DlcManager.h               # 【DLC管理】接口声明
    engine/NodeEngine.h               # 【节点引擎】接口声明
    engine/SaveManager.h              # 【存档管理】接口声明
    engine/MusicPlayer.h              # 【音乐播放】接口声明

    # --- UI 层头文件 ---
    ui/MainWindow.h                   # 【主窗口】接口声明
    ui/MenuWidget.h                   # 【主菜单】接口声明
    ui/GameWidget.h                   # 【游戏界面】接口声明
    ui/CharacterCreateWidget.h        # 【角色创建】接口声明
    ui/SaveLoadDialog.h               # 【存档对话框】接口声明
)

# ── 第 6 节: 构建可执行文件 ──────────────────────────────────────────────────
# qt_add_executable: Qt6 提供的增强版 add_executable
#   自动处理 MOC/RCC/UIC 生成文件的编译链接
#   参数: 目标名 + 源文件 + 头文件 + 资源文件
qt_add_executable(HeartsOfIronGame
    ${SOURCES}
    ${HEADERS}
    resources/resources.qrc           # Qt 资源集合（当前仅含 style.qss）
)

# ── 第 7 节: 链接依赖库 ──────────────────────────────────────────────────────
# target_link_libraries: 将 Qt6 库链接到目标可执行文件
#   PRIVATE: 链接关系不传递给依赖本目标的其他目标
target_link_libraries(HeartsOfIronGame PRIVATE
    Qt6::Core                         # 核心功能（字符串、容器、JSON、事件循环）
    Qt6::Widgets                      # 桌面控件（窗口、按钮、布局、进度条等）
    Qt6::Multimedia                   # 多媒体播放（背景音乐）
)

# ── 第 8 节: 头文件搜索路径 ──────────────────────────────────────────────────
# 将项目根目录添加到 include 路径，使 #include "engine/DlcTypes.h"
# 和 #include "ui/MainWindow.h" 等相对路径可被解析
target_include_directories(HeartsOfIronGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}       # 即 d:\Creater\Hearts of iron\
)

# ── 第 9 节: 构建后复制 DLC 目录 ─────────────────────────────────────────────
# add_custom_command + POST_BUILD: 每次编译成功后自动执行
#   copy_directory: 将 dlc/ 文件夹复制到可执行文件所在目录
#   效果: 开发时修改 DLC JSON 无需重新编译，但需手动复制；此命令确保发布版本包含 DLC
#   路径说明:
#     ${CMAKE_CURRENT_SOURCE_DIR}/dlc  → 源码树中的 dlc/ 目录
#     $<TARGET_FILE_DIR:HeartsOfIronGame>/dlc → 编译产物目录下的 dlc/
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dlc")
    add_custom_command(TARGET HeartsOfIronGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/dlc"
        "$<TARGET_FILE_DIR:HeartsOfIronGame>/dlc"
    )
endif()

# ── 第 10 节: 目标属性设置 ───────────────────────────────────────────────────
# WIN32_EXECUTABLE: Windows 下不显示控制台窗口（GUI 应用）
# OUTPUT_NAME: 最终生成的可执行文件名（中文名）
set_target_properties(HeartsOfIronGame PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "钢铁意志_第三帝国"
)
