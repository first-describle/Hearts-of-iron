/**
 * ===========================================================================
 * DlcManager.h — DLC 扫描、解析与验证管理器
 * ===========================================================================
 *
 * 【所属模块】引擎层 — DLC 内容管理
 * 【依赖关系】DlcTypes.h (数据定义), QObject (信号槽), QDir (目录遍历)
 * 【对应实现】DlcManager.cpp
 *
 * DlcManager 是整个引擎的"内容加载器"，负责:
 *   ① 扫描 dlc/ 目录 → 发现所有 DLC 文件夹（每个含 manifest.json）
 *   ② 解析 manifest.json → 提取 DLC 元数据（职业、章节、音乐）
 *   ③ 验证 DLC 完整性 → 11 项检查确保数据可用
 *   ④ 加载章节 JSON → 将 JSON 节点转换为 StoryNode 运行时数据结构
 *
 * 数据流向:
 *   dlc/<dlcId>/manifest.json  → parseManifest()  → DlcManifest (内存)
 *   dlc/<dlcId>/chapters/*.json → loadChapter()   → DlcChapter  (内存)
 *                                                     ↓
 *                                               NodeEngine 使用
 * ===========================================================================
 */

#pragma once
#include <QObject>
#include <QDir>
#include <QList>
#include "DlcTypes.h"

class DlcManager : public QObject {
    Q_OBJECT
public:
    explicit DlcManager(QObject *parent = nullptr);

    // =========================================================================
    // 公开接口 — 扫描与查询
    // =========================================================================

    /**
     * scanDirectory() — 扫描 DLC 根目录
     * 遍历 dlcRootDir 下的所有子文件夹，查找 manifest.json，
     * 解析并验证后存入 m_manifests 列表。
     * 扫描完成后发射 dlcScanComplete 信号。
     *
     * @param dlcRootDir   DLC 根目录的绝对路径（如 "C:/.../dlc"）
     */
    void scanDirectory(const QString &dlcRootDir);

    /**
     * manifests() — 获取所有已扫描的 DLC 清单
     * @return DlcManifest 列表的常量引用（包含 valid=true 和 valid=false 的 DLC）
     */
    const QList<DlcManifest> &manifests() const { return m_manifests; }

    /**
     * getManifest() — 根据 dlcId 查找特定 DLC 清单
     * @param dlcId  DLC 唯一标识
     * @return 找到则返回指针，否则返回 nullptr
     */
    const DlcManifest *getManifest(const QString &dlcId) const;

    /**
     * dlcBasePath() — 获取 DLC 文件夹的绝对路径
     * @param dlcId   DLC 唯一标识
     * @return 完整路径: "<dlcRootDir>/<dlcId>"
     */
    QString dlcBasePath(const QString &dlcId) const { return m_dlcRootDir + "/" + dlcId; }

    /**
     * loadChapter() — 加载指定 DLC 的某一章节 JSON 文件
     * 解析章节 JSON 中的所有 StoryNode，填充到 DlcChapter 结构体中。
     *
     * @param dlcBasePath  DLC 文件夹的绝对路径
     * @param chapterFile  章节 JSON 文件的相对路径（来自 DlcChapterMeta::file）
     * @param outChapter   [出参] 解析后的章节数据
     * @return true = 加载成功, false = 文件不存在或 JSON 格式错误
     */
    bool loadChapter(const QString &dlcBasePath,
                     const QString &chapterFile,
                     DlcChapter &outChapter);

signals:
    /**
     * dlcScanComplete — DLC 扫描完成信号
     * @param count  发现的 DLC 总数（含无效的）
     */
    void dlcScanComplete(int count);

private:
    // =========================================================================
    // 内部实现 — 解析与验证
    // =========================================================================

    /**
     * parseManifest() — 解析单个 manifest.json 文件
     * 读取 JSON → 提取所有字段 → 填充 DlcManifest 结构体
     *
     * @param path  manifest.json 文件的绝对路径
     * @return 解析后的 DlcManifest（解析失败时 valid=false）
     */
    DlcManifest parseManifest(const QString &path);

    /**
     * validateManifest() — 验证 DLC 清单的完整性
     * 执行 11 项检查:
     *   1. dlcId 非空
     *   2. title 非空
     *   3. classes[] 至少 1 个
     *   4. chapters[] 至少 1 个
     *   5. class id 唯一性
     *   6. chapter id 唯一性
     *   7. 每个 chapter 的 JSON 文件存在
     *   8. startChapter 在 chapters[] 中存在
     *   9~11. 每个非 start 章节的 unlock 目标存在
     *
     * @param m        待验证的清单
     * @param basePath DLC 文件夹路径（用于检查章节文件存在性）
     * @return 错误信息列表（空 = 验证通过）
     */
    QStringList validateManifest(const DlcManifest &m, const QString &basePath);

    /**
     * parseNodeFromJson() — 将 JSON 对象解析为 StoryNode
     * 处理所有子字段: choices[], classText{}, 节点类型枚举转换等
     *
     * @param obj  章节 JSON 中 "nodes" 数组的一个元素
     * @return 解析完成的 StoryNode 结构体
     */
    StoryNode  parseNodeFromJson(const QJsonObject &obj);

    // =========================================================================
    // 成员变量
    // =========================================================================
    QString m_dlcRootDir;              // DLC 根目录路径（由 scanDirectory 设置）
    QList<DlcManifest> m_manifests;   // 所有已扫描 DLC 的清单列表
};
