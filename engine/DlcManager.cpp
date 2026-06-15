/**
 * ===========================================================================
 * DlcManager.cpp — DLC 扫描、解析与验证管理器实现
 * ===========================================================================
 *
 * 【所属模块】引擎层 — DLC 内容管理
 * 【依赖头文件】DlcManager.h, QJsonDocument/ QJsonObject/ QJsonArray (JSON解析)
 *              QFile/QFileInfo/QDir (文件系统), QDebug (日志)
 *
 * 本文件实现了 DLC 内容加载的完整流程:
 *   scanDirectory() → 遍历 dlc/ 子目录 → parseManifest() → validateManifest()
 *   loadChapter()   → 读取章节 JSON     → parseNodeFromJson() (递归解析每个节点)
 *
 * JSON 解析策略:
 *   使用 Qt6 的 QJsonDocument 进行零依赖的 JSON 解析（无需第三方库）。
 *   所有字段都有合理的默认值（通过 .toInt(default) / .toBool(default)），
 *   确保缺失字段不会导致崩溃。
 * ===========================================================================
 */

#include "DlcManager.h"
#include <QJsonDocument>     // Qt JSON 文档对象（含解析器和序列化器）
#include <QJsonObject>       // JSON 对象类型
#include <QJsonArray>        // JSON 数组类型
#include <QFile>             // 文件读写
#include <QFileInfo>         // 文件信息（exists, absoluteFilePath 等）
#include <QDir>              // 目录操作（entryInfoList, exists 等）
#include <QDebug>            // 调试日志输出 (qWarning, qDebug)

// ===========================================================================
// 构造函数
// ===========================================================================
// 仅初始化 QObject 基类，实际的扫描工作延迟到 scanDirectory() 调用。
// 这样设计允许调用方在扫描前进行其他初始化（如设置路径）。
DlcManager::DlcManager(QObject *parent) : QObject(parent) {}

// ===========================================================================
// scanDirectory() — 扫描 DLC 根目录 (公开接口)
// ===========================================================================
/**
 * 【工作流程】
 *   ① 清空 m_manifests（支持重复扫描）
 *   ② 检查 dlcRootDir 是否存在，不存在则警告并返回
 *   ③ 遍历 dlcRootDir 下的所有子文件夹（排除 . 和 ..）
 *   ④ 对每个子文件夹:
 *      a. 检查是否包含 manifest.json，无则跳过
 *      b. 调用 parseManifest() 解析 JSON
 *      c. 调用 validateManifest() 验证完整性
 *      d. 无论验证通过与否，都加入 m_manifests 列表
 *   ⑤ 发射 dlcScanComplete(count) 信号通知调用方
 *
 * 【错误处理】
 *   解析或验证失败的 DLC 不会从列表中移除，而是标记 valid=false。
 *   这样 UI 可以显示失败的 DLC 及其错误原因（通过 tooltip）。
 */
void DlcManager::scanDirectory(const QString &dlcRootDir) {
    m_dlcRootDir = dlcRootDir;
    m_manifests.clear();    // 重置列表，支持重新扫描

    // ── 检查根目录是否存在 ──
    QDir root(dlcRootDir);
    if (!root.exists()) {
        qWarning() << "DLC directory not found:" << dlcRootDir;
        emit dlcScanComplete(0);
        return;
    }

    // ── 遍历子文件夹 ──
    // QDir::Dirs: 仅列出目录
    // QDir::NoDotAndDotDot: 排除 "." 和 ".."
    const auto entries = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        // 拼接 manifest.json 路径
        QString manifestPath = entry.absoluteFilePath() + "/manifest.json";
        if (!QFile::exists(manifestPath)) {
            qWarning() << "No manifest.json in" << entry.absoluteFilePath() << ", skipping";
            continue;
        }

        // ── 解析 manifest.json ──
        DlcManifest m = parseManifest(manifestPath);
        m.valid = true;   // 假定有效，validateManifest 会修正

        // ── 验证 DLC 数据完整性 ──
        QString basePath = entry.absoluteFilePath();
        QStringList errors = validateManifest(m, basePath);
        if (!errors.isEmpty()) {
            m.valid = false;
            m.errors = errors;
        }

        m_manifests.append(m);
    }

    emit dlcScanComplete(m_manifests.size());
}

// ===========================================================================
// parseManifest() — 解析 manifest.json (内部实现)
// ===========================================================================
/**
 * 【解析流程】
 *   ① 打开并读取 JSON 文件
 *   ② 提取顶层字段: dlcId, title, subtitle, author, version, startChapter
 *   ③ 遍历 "classes" 数组 → 填充 DlcClass 列表
 *   ④ 遍历 "chapters" 数组 → 填充 DlcChapterMeta 列表
 *   ⑤ 遍历 "music" 对象 → 填充音乐键到文件路径的映射
 *
 * 【设计要点】
 *   - 使用 .toString() / .toInt() 等方法的默认值参数，确保字段缺失不崩溃
 *   - 音乐映射使用 QJsonObject 的迭代器遍历（键值对模式）
 *   - 章节的 unlock 字段: "start" = 初始可用，否则为依赖的前置章节 ID
 */
DlcManifest DlcManager::parseManifest(const QString &path) {
    DlcManifest m;
    QFile file(path);

    // ── 第 1 步: 打开文件 ──
    if (!file.open(QIODevice::ReadOnly)) {
        m.valid = false;
        m.errors.append(QStringLiteral("Cannot open manifest.json"));
        return m;
    }

    // ── 第 2 步: 解析 JSON ──
    // QJsonDocument::fromJson() 返回解析结果，isNull() 表示格式错误
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        m.valid = false;
        m.errors.append(QStringLiteral("Invalid JSON in manifest.json"));
        return m;
    }

    QJsonObject root = doc.object();

    // ── 第 3 步: 提取顶层元数据字段 ──
    m.dlcId       = root["dlcId"].toString();        // DLC 唯一标识（即文件夹名）
    m.title       = root["title"].toString();         // 显示标题
    m.subtitle    = root["subtitle"].toString();      // 副标题
    m.author      = root["author"].toString();        // 作者
    m.version     = root["version"].toString();       // 版本号
    m.startChapter = root["startChapter"].toString();// 起始章节 ID

    // ── 第 4 步: 解析职业列表 (classes[]) ──
    // JSON 结构: "classes": [{ "id": "infantry", "name": "步兵", "desc": "..." }, ...]
    for (const auto &v : root["classes"].toArray()) {
        QJsonObject co = v.toObject();
        DlcClass dc;
        dc.id   = co["id"].toString();    // 字符串 ID
        dc.name = co["name"].toString();  // 显示名称
        dc.desc = co["desc"].toString();  // 职业描述
        m.classes.append(dc);
    }

    // ── 第 5 步: 解析章节列表 (chapters[]) ──
    // JSON 结构: "chapters": [{ "id": "fall_gelb", "file": "...", "name": "...", ... }, ...]
    for (const auto &v : root["chapters"].toArray()) {
        QJsonObject cho = v.toObject();
        DlcChapterMeta cm;
        cm.id       = cho["id"].toString();        // 章节 ID
        cm.file     = cho["file"].toString();      // JSON 文件相对路径
        cm.name     = cho["name"].toString();      // 显示名称
        cm.subtitle = cho["subtitle"].toString();  // 副标题
        cm.unlock   = cho["unlock"].toString();    // 解锁条件
        m.chapters.append(cm);
    }

    // ── 第 6 步: 解析音乐映射 (music{}) ──
    // JSON 结构: "music": { "main_theme": "music/main.mp3", ... }
    QJsonObject musicObj = root["music"].toObject();
    for (auto it = musicObj.begin(); it != musicObj.end(); ++it) {
        m.music[it.key()] = it.value().toString();  // key → 相对文件路径
    }

    return m;
}

// ===========================================================================
// validateManifest() — 验证 DLC 清单完整性 (内部实现)
// ===========================================================================
/**
 * 【验证项目】(共 11 项检查)
 *   ① dlcId 是否为空
 *   ② title 是否为空
 *   ③ classes[] 是否至少有一个职业
 *   ④ chapters[] 是否至少有一个章节
 *   ⑤ 职业 ID 是否有重复
 *   ⑥ 章节 ID 是否有重复
 *   ⑦ 每个章节的 JSON 文件是否存在
 *   ⑧ startChapter 是否在 chapters[] 中存在
 *   ⑨~⑪ 每个非 start 章节的 unlock 目标是否存在
 *
 * 【验证策略】
 *   所有检查独立执行，错误累积在 errors 列表中。
 *   即使前一项失败，后续检查仍继续执行，
 *   确保调用方能看到所有问题（而非修复一个后才看到下一个）。
 */
QStringList DlcManager::validateManifest(const DlcManifest &m, const QString &basePath) {
    QStringList errors;

    // ── 检查 ①~④: 必填字段非空 ──
    if (m.dlcId.isEmpty())
        errors.append(QStringLiteral("dlcId is empty"));
    if (m.title.isEmpty())
        errors.append(QStringLiteral("title is empty"));
    if (m.classes.isEmpty())
        errors.append(QStringLiteral("classes[] is empty — at least one class required"));
    if (m.chapters.isEmpty())
        errors.append(QStringLiteral("chapters[] is empty — at least one chapter required"));

    // ── 检查 ⑤: 职业 ID 唯一性 ──
    QSet<QString> classIds;
    for (const auto &c : m.classes) {
        if (classIds.contains(c.id))
            errors.append(QStringLiteral("Duplicate class id: ") + c.id);
        classIds.insert(c.id);
    }

    // ── 检查 ⑥~⑦: 章节 ID 唯一性 + 文件存在性 ──
    QSet<QString> chapterIds;
    for (const auto &ch : m.chapters) {
        if (chapterIds.contains(ch.id))
            errors.append(QStringLiteral("Duplicate chapter id: ") + ch.id);
        chapterIds.insert(ch.id);

        QString fullPath = basePath + "/" + ch.file;
        if (!QFile::exists(fullPath))
            errors.append(QStringLiteral("Chapter file not found: ") + ch.file);
    }

    // ── 检查 ⑧: startChapter 必须存在 ──
    if (!m.startChapter.isEmpty() && !chapterIds.contains(m.startChapter))
        errors.append(QStringLiteral("startChapter '") + m.startChapter +
                      QStringLiteral("' not found in chapters[]"));

    // ── 检查 ⑨~⑪: 非 start 章节的 unlock 目标必须引用已存在的章节 ──
    for (const auto &ch : m.chapters) {
        if (ch.unlock != "start" && !chapterIds.contains(ch.unlock))
            errors.append(QStringLiteral("Chapter '") + ch.id +
                          QStringLiteral("' unlock target '") + ch.unlock +
                          QStringLiteral("' not found"));
    }

    return errors;
}

// ===========================================================================
// getManifest() — 按 ID 查找 DLC 清单 (公开接口)
// ===========================================================================
const DlcManifest *DlcManager::getManifest(const QString &dlcId) const {
    for (const auto &m : m_manifests) {
        if (m.dlcId == dlcId)
            return &m;   // 返回指针（效率优先，调用方不应长期持有此指针）
    }
    return nullptr;
}

// ===========================================================================
// loadChapter() — 加载章节 JSON (公开接口)
// ===========================================================================
/**
 * 【加载流程】
 *   ① 拼接完整文件路径: <dlcBasePath>/<chapterFile>
 *   ② 打开并解析 JSON 文件
 *   ③ 提取章节元数据: chapterId, startNodeId, defeatNodeId
 *   ④ 遍历 "nodes" 数组 → 调用 parseNodeFromJson() 解析每个 StoryNode
 *   ⑤ 将解析结果存入 DlcChapter::nodes 映射表
 *
 * 【返回值】
 *   true  → 加载成功，outChapter 包含完整数据
 *   false → 文件不存在或 JSON 解析失败
 */
bool DlcManager::loadChapter(const QString &dlcBasePath,
                              const QString &chapterFile,
                              DlcChapter &outChapter) {
    // ── 拼接完整路径并打开文件 ──
    QString fullPath = dlcBasePath + "/" + chapterFile;
    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    // ── 解析 JSON ──
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull())
        return false;

    // ── 提取章节元数据 ──
    QJsonObject root = doc.object();
    outChapter.chapterId    = root["chapterId"].toString();     // 章节 ID
    outChapter.startNodeId  = root["startNodeId"].toString();   // 起始节点 ID
    outChapter.defeatNodeId = root["defeatNodeId"].toString();  // 失败节点 ID

    // ── 解析所有叙事节点 ──
    QJsonArray nodesArr = root["nodes"].toArray();
    for (const auto &v : nodesArr) {
        StoryNode node = parseNodeFromJson(v.toObject());
        outChapter.nodes[node.id] = node;  // 以节点 ID 为键存入映射表
    }

    return true;
}

// ===========================================================================
// parseNodeFromJson() — JSON → StoryNode 解析 (内部实现)
// ===========================================================================
/**
 * 【解析流程】将章节 JSON 中 "nodes" 数组的一个元素转换为 StoryNode 结构体。
 *
 *   ① 基础字段: id, locationTitle, text, musicKey, nextNodeId, 胜负标记
 *   ② 类型枚举: "narrative" → Narrative, "ending" → Ending, 其他 → Choice
 *   ③ 选项数组: 每个选项解析为 Choice 结构体（含战斗参数/标志/职业限制）
 *   ④ 职业文本: classText{} 对象 → QMap<职业ID, 专属文本>
 *
 * 【选项解析细节】
 *   每个 Choice 包含:
 *     - 文本 + 目标节点
 *     - HP/士气变化 (带默认值)
 *     - 战斗参数 (阈值 0~100, 加成职业, 成功/失败跳转)
 *     - 标志数组 (requiredFlags/grantedFlags)
 *     - 职业数组 (allowedClasses/bonusClasses)
 *
 * 【默认值策略】
 *   所有 .toInt(default) / .toBool(default) 都有合理的默认值:
 *     - 战斗阈值默认 50 (中等难度)
 *     - 失败 HP 损失默认 -20
 *     - 失败士气损失默认 -15
 */
StoryNode DlcManager::parseNodeFromJson(const QJsonObject &obj) {
    StoryNode n;

    // ── 第 1 步: 基础字段 ──
    n.id            = obj["id"].toString();
    n.locationTitle = obj["locationTitle"].toString();
    n.text          = obj["text"].toString();
    n.musicKey      = obj["musicKey"].toString();
    n.nextNodeId    = obj["nextNodeId"].toString();
    n.isVictory     = obj["isVictory"].toBool(false);  // 默认非胜利
    n.isDefeat      = obj["isDefeat"].toBool(false);   // 默认非失败

    // ── 第 2 步: 类型枚举转换 ──
    QString typeStr = obj["type"].toString();
    if (typeStr == "narrative")
        n.type = NodeType::Narrative;      // 纯叙事 → 单一继续按钮
    else if (typeStr == "ending")
        n.type = NodeType::Ending;         // 终止节点
    else
        n.type = NodeType::Choice;         // 默认: 选择节点

    // ── 第 3 步: 解析选项列表 (choices[]) ──
    for (const auto &cv : obj["choices"].toArray()) {
        QJsonObject co = cv.toObject();
        Choice c;

        // --- 基础字段 ---
        c.text            = co["text"].toString();            // 选项显示文本
        c.nextNodeId      = co["nextNodeId"].toString();      // 普通选项的目标节点

        // --- 属性变化（选择后立即生效） ---
        c.hpDelta         = co["hpDelta"].toInt(0);           // 默认 0 (无变化)
        c.moraleDelta     = co["moraleDelta"].toInt(0);       // 默认 0

        // --- 战斗判定参数 ---
        c.isCombat        = co["isCombat"].toBool(false);     // 默认非战斗
        c.combatThreshold = co["combatThreshold"].toInt(50);  // 默认阈值 50
        c.successNodeId   = co["successNodeId"].toString();   // 成功跳转目标
        c.failureNodeId   = co["failureNodeId"].toString();   // 失败跳转目标
        c.failHpDelta     = co["failHpDelta"].toInt(-20);     // 默认失败 HP-20
        c.failMoraleDelta = co["failMoraleDelta"].toInt(-15); // 默认失败士气-15

        // --- 职业限制 ---
        c.classRestricted = co["classRestricted"].toBool(false);

        // --- 标志集合 (QSet 去重) ---
        for (const auto &fv : co["requiredFlags"].toArray())
            c.requiredFlags.insert(fv.toString());  // 前置标志
        for (const auto &fv : co["grantedFlags"].toArray())
            c.grantedFlags.insert(fv.toString());   // 授予标志

        // --- 职业列表 ---
        for (const auto &cv : co["allowedClasses"].toArray())
            c.allowedClasses.append(cv.toString());   // 限制可选职业
        for (const auto &bv : co["bonusClasses"].toArray())
            c.bonusClasses.append(bv.toString());     // 战斗加成职业

        n.choices.append(c);
    }

    // ── 第 4 步: 解析职业专属文本 (classText{}) ──
    // JSON 结构: "classText": { "tank_crew": "坦克手的专属描写...", ... }
    QJsonObject ctObj = obj["classText"].toObject();
    for (auto it = ctObj.begin(); it != ctObj.end(); ++it) {
        n.classText[it.key()] = it.value().toString();
    }

    return n;
}
