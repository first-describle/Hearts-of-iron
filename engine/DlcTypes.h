/**
 * ===========================================================================
 * DlcTypes.h — 引擎共享数据类型定义 (Engine-Wide Shared Data Types)
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 通用数据结构
 * 【依赖关系】仅依赖 QtCore（QString/QList/QMap/QSet），不依赖任何引擎模块
 * 【对应实现】无 .cpp 文件 — 纯头文件（header-only）结构体集合
 *
 * 本文件是整个引擎的"数据语言"——所有模块之间传递的数据结构都在此定义。
 * 类似于游戏开发中的"协议层"或"数据定义层"。
 *
 * 包含 5 大组成部分:
 *   ① 共享枚举: NodeType（节点类型）、GameScreen（游戏界面）
 *   ② DLC 元数据: DlcClass、DlcChapterMeta、DlcManifest
 *   ③ 选项数据:   Choice（含战斗判定参数）
 *   ④ 叙事节点:   StoryNode（含职业专属文本覆盖）
 *   ⑤ 章节数据:   DlcChapter（节点映射表）
 * ===========================================================================
 */

#pragma once
#include <QString>
#include <QList>
#include <QMap>
#include <QSet>

// ===========================================================================
// 第 1 部分: 共享枚举 (Shared Enums)
// ===========================================================================
// 这些枚举在整个引擎和 UI 层中用于判断节点行为和控制页面切换

/**
 * NodeType — 叙事节点类型枚举
 * 决定 GameWidget 如何处理当前节点:
 *   Narrative: 纯叙事文本，显示一个"继续"按钮，无分支选择
 *   Choice:    玩家面临多个选项，可包含普通选项和战斗选项
 *   Ending:    胜利或失败的终止节点（章节结束标记）
 */
enum class NodeType {
    Narrative,   // 纯叙事节点 → GameWidget 生成单个【继续】按钮
    Choice,      // 选择节点   → GameWidget 生成多个选项按钮
    Ending       // 终止节点   → GameWidget 生成特殊跳转按钮（返回主页/下一章）
};

/**
 * GameScreen — 游戏界面枚举
 * 描述当前显示的界面状态，用于 QStackedWidget 页面索引映射:
 *   0 = MainMenu       → MenuWidget (主菜单标题画面)
 *   1 = DlcSelect      → MenuWidget 内部的 DLC 列表面板
 *   2 = CharacterCreate → CharacterCreateWidget (兵种选择)
 *   3 = Playing        → GameWidget (核心游戏界面)
 *   4 = GameOver       → 游戏结束状态（当前未使用独立页面）
 */
enum class GameScreen {
    MainMenu,          // 主菜单
    DlcSelect,         // DLC 选择
    CharacterCreate,   // 角色创建
    Playing,           // 游戏中（核心叙事界面）
    GameOver           // 游戏结束
};

// ===========================================================================
// 第 2 部分: DLC 元数据结构 (DLC Metadata Structures)
// ===========================================================================
// 这些结构体对应 manifest.json 中的配置数据，
// 在 DlcManager::parseManifest() 中被填充。

/**
 * DlcClass — DLC 中定义的职业/兵种
 * 源于 manifest.json 中 "classes" 数组的每个元素。
 * 示例: { id: "infantry", name: "步兵", desc: "德意志国防军的基石..." }
 *
 * @field id   — 字符串 ID，如 "infantry", "tank_crew", "fighter_pilot"
 *               用于在 Choice::allowedClasses / bonusClasses 中匹配
 * @field name — 显示名称，如 "步兵", "坦克手", "战斗机飞行员"
 * @field desc — 描述文本，在 CharacterCreateWidget 右侧预览面板中显示
 */
struct DlcClass {
    QString id;       // 字符串ID，如 "infantry"
    QString name;     // 显示名称，如 "步兵"
    QString desc;     // 描述文本
};

/**
 * DlcChapterMeta — DLC 中某一章节的元数据
 * 源于 manifest.json 中 "chapters" 数组的每个元素。
 * 包含章节的标识信息、JSON 文件路径和解锁条件。
 *
 * @field id       — 章节唯一 ID，如 "fall_gelb", "stalingrad"
 * @field file     — 章节 JSON 文件相对于 DLC 目录的路径
 * @field name     — 显示名称
 * @field subtitle — 副标题
 * @field unlock   — 解锁条件: "start" = 初始可用，或填写其他章节 ID
 */
struct DlcChapterMeta {
    QString id;       // 如 "fall_gelb"
    QString file;     // JSON 文件相对路径，如 "chapters/ch01_fall_gelb.json"
    QString name;     // 显示名称
    QString subtitle; // 副标题
    QString unlock;   // 解锁条件："start" 或 chapter id
};

/**
 * DlcManifest — DLC 清单（一个 DLC 的完整定义）
 * 由 DlcManager::parseManifest() 从 manifest.json 解析而来。
 * 包含 DLC 的全部元信息: 身份、职业列表、章节列表、音乐映射。
 *
 * @field dlcId      — DLC 唯一标识，也是 dlc/ 下的文件夹名
 * @field title       — DLC 标题
 * @field subtitle    — 副标题
 * @field author      — 作者署名
 * @field version     — 版本号
 * @field classes     — 该 DLC 提供的所有职业
 * @field chapters    — 该 DLC 包含的所有章节（元数据）
 * @field startChapter — 起始章节 ID（新游戏从这里开始）
 * @field music       — 音乐键 → 相对文件路径 映射
 * @field valid       — 验证是否通过（false 时 DLC 不可选）
 * @field errors      — 验证错误信息列表
 */
struct DlcManifest {
    QString dlcId;
    QString title;
    QString subtitle;
    QString author;
    QString version;
    QList<DlcClass> classes;
    QList<DlcChapterMeta> chapters;
    QString startChapter;
    QMap<QString, QString> music;   // key → 相对文件路径 (如 "main_theme" → "music/main.mp3")
    bool valid = true;
    QStringList errors;
};

// ===========================================================================
// 第 3 部分: Choice — 玩家选项数据结构
// ===========================================================================
/**
 * Choice — 叙事节点中的一个玩家选项
 * 对应章节 JSON 中 "choices" 数组的每个元素。
 *
 * 支持三种选项类型:
 *   ① 普通选项: 改变 HP/士气/标志，跳转到下一节点
 *   ② 职业限制选项: 仅特定兵种可选（classRestricted = true）
 *   ③ 战斗判定选项: D100 骰子检定，成功/失败走不同分支
 *
 * 设计要点:
 *   - allowedClasses / bonusClasses 使用 QString 列表（非旧版 int 枚举）
 *     以支持 DLC 自定义职业类（JSON 驱动，引擎不硬编码）
 *   - requiredFlags / grantedFlags 实现叙事分支持久化：
 *     例如选择了"救助伤员"可在后续章节解锁特殊对话
 */
struct Choice {
    // ── 基础信息 ──
    QString text;          // 选项显示文本，如 "冲锋陷阵"
    QString nextNodeId;    // 普通选项: 选择后跳转的目标节点 ID

    // ── 属性变化（选择后立即生效） ──
    int hpDelta      = 0;  // 生命值变化（正=回复, 负=受伤）
    int moraleDelta  = 0;  // 士气值变化（正=振奋, 负=沮丧）

    // ── 标志系统（叙事分支核心） ──
    QSet<QString> requiredFlags;   // 前置标志: 必须拥有全部标志才能看到此选项
    QSet<QString> grantedFlags;    // 授予标志: 选择后获得的标志（影响后续叙事）

    // ── 职业限制（可选） ──
    bool classRestricted = false;            // 是否只允许特定兵种选择
    QList<QString> allowedClasses;          // 允许的兵种 ID 列表

    // ── 战斗判定（可选） ──
    bool isCombat        = false;           // 是否为战斗判定选项
    int  combatThreshold = 50;              // 检定难度阈值 [1,100]，掷骰需 >= 此值
    QList<QString> bonusClasses;           // 享有 +20 加成的职业列表
    QString successNodeId;                  // 战斗成功 → 跳转节点
    QString failureNodeId;                  // 战斗失败 → 跳转节点
    int failHpDelta      = -20;            // 战斗失败的额外 HP 损失
    int failMoraleDelta  = -15;            // 战斗失败的额外士气损失
};

// ===========================================================================
// 第 4 部分: StoryNode — 叙事节点数据结构
// ===========================================================================
/**
 * StoryNode — 叙事流程中的一个节点
 * 对应章节 JSON 中 "nodes" 数组的每个元素。
 *
 * 三种节点类型 (type 字段):
 *   Narrative: 纯叙事 → 使用 nextNodeId 自动继续
 *   Choice:    选项分支 → 使用 choices[] 数组提供多个选项
 *   Ending:    章节终止 → 标记 isVictory 或 isDefeat
 *
 * 职业专属文本 (classText):
 *   允许同一叙事节点对不同兵种显示不同的描述文本。
 *   例如: 坦克手看到的战场描写 vs 潜艇兵看到的描写完全不同。
 *   若当前职业在 classText 中无映射，则回退到泛用 text。
 */
struct StoryNode {
    // ── 节点标识 ──
    QString id;              // 节点唯一 ID（在章节内唯一），如 "node_01"
    QString locationTitle;   // 场景位置标题，显示在 GameWidget 右上角，如 "阿登森林 · 1940"

    // ── 叙事文本 ──
    QString text;            // 默认/泛用叙事文本（所有职业通用）

    // ── 节点类型 ──
    NodeType type = NodeType::Choice;  // 默认为选择节点

    // ── 选项列表（仅 Choice 类型有效） ──
    QList<Choice> choices;   // 可供玩家选择的选项集合

    // ── Narrative 节点专用: 自动跳转 ──
    QString nextNodeId;      // Narrative 节点 → 继续按钮的目标节点

    // ── 音乐指示 ──
    QString musicKey;        // 切换到该节点时应播放的音乐键名（空 = 不切换）

    // ── Ending 标记 ──
    bool isVictory = false;  // 是否胜利结局节点
    bool isDefeat  = false;  // 是否失败结局节点

    // ── 职业专属文本覆盖 ──
    QMap<QString, QString> classText;  // 兵种ID → 专属文本

    /**
     * textFor() — 获取当前职业应显示的文本
     * @param classId 玩家当前职业 ID
     * @return 若 classText 中有映射则返回专属文本，否则返回泛用 text
     */
    QString textFor(const QString &classId) const {
        if (classText.contains(classId))
            return classText[classId];
        return text;
    }
};

// ===========================================================================
// 第 5 部分: DlcChapter — 运行时章节数据结构
// ===========================================================================
/**
 * DlcChapter — 加载到内存中的完整章节
 * 由 DlcManager::loadChapter() 从章节 JSON 解析而来。
 * 是 NodeEngine 进行节点导航的核心数据结构。
 *
 * @field chapterId     — 章节 ID（对应 DlcChapterMeta::id）
 * @field startNodeId   — 起始节点 ID（章节打开时首先显示的节点）
 * @field defeatNodeId  — 失败节点 ID（玩家死亡时跳转的目标节点）
 * @field nodes         — 节点 ID → StoryNode 的映射表
 *                        NodeEngine 通过 currentNodeId 在此表中查找当前节点
 */
struct DlcChapter {
    QString chapterId;                // 章节标识
    QString startNodeId;              // 起始节点
    QString defeatNodeId;             // 失败节点（玩家死亡时跳转到此）
    QMap<QString, StoryNode> nodes;   // 节点映射表: ID → 内容
};
