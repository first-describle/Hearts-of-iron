/**
 * ===========================================================================
 * MusicPlayer.h — 背景音乐管理系统
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 音频播放
 * 【依赖关系】Qt6 Multimedia (QMediaPlayer + QAudioOutput)
 * 【对应实现】MusicPlayer.cpp
 *
 * MusicPlayer 封装了 Qt6 Multimedia 框架，提供游戏背景音乐管理:
 *   ① 音乐注册: 将音乐键名映射到文件路径 (registerTrack)
 *   ② 播放控制: 按 key 切换曲目，自动停止前一首 (play)
 *   ③ 循环播放: 曲目结束后自动重播 (setLoop)
 *   ④ 音量控制: [0, 100] 范围的音量 + 静音开关
 *
 * 音乐键系统:
 *   与 DLC JSON 中的 "music" 字段配合使用。
 *   DlcManifest::music 定义了 key → 文件路径的映射。
 *   StoryNode::musicKey 指定进入该节点时应播放的音乐键。
 *   特殊键:
 *     - "main_theme": 主菜单音乐（固定使用）
 *     - "defeat_theme": 失败音乐（onChapterDefeat 时播放）
 *
 * Qt6 多媒体架构:
 *   QMediaPlayer:   负责加载媒体源和控制播放
 *   QAudioOutput:   负责音频输出设备管理和音量控制
 *   两者分离是 Qt6 的设计（Qt5 中 QMediaPlayer 直接控制音量）。
 *
 * 文件丢失处理:
 *   若注册的音乐文件不存在，play() 静默跳过（不崩溃、不报错）。
 *   使用 QFileInfo::exists() 检查，不存在时不调用 setSource()。
 * ===========================================================================
 */

#pragma once
#include <QObject>
#include <QMediaPlayer>      // Qt6 媒体播放器
#include <QAudioOutput>       // Qt6 音频输出
#include <QMap>
#include <QString>
#include <QUrl>

class MusicPlayer : public QObject {
    Q_OBJECT
public:
    explicit MusicPlayer(QObject *parent = nullptr);
    ~MusicPlayer();

    // =========================================================================
    // 音乐注册
    // =========================================================================

    /**
     * registerTrack() — 注册音乐键到文件路径的映射
     * 通常在 onDlcSelected() 时批量调用（遍历 DlcManifest::music）
     *
     * @param key      音乐键名（如 "main_theme", "battle_01"）
     * @param filePath 音乐文件的绝对路径
     */
    void registerTrack(const QString &key, const QString &filePath);

    // =========================================================================
    // 播放控制
    // =========================================================================

    /**
     * play() — 播放指定键名的音乐
     * - 若已在播放同一首歌 → 不中断（避免重播导致卡顿）
     * - 若键名不存在 → 静默跳过
     * - 若文件不存在 → 静默跳过
     *
     * @param key 要播放的音乐键名
     */
    void play(const QString &key);

    /** stop() — 停止当前播放 */
    void stop();

    // =========================================================================
    // 音量控制
    // =========================================================================

    /** setVolume() — 设置音量 [0, 100]，内部转为 [0.0, 1.0] */
    void  setVolume(int vol);

    /** volume() — 获取当前音量 [0, 100] */
    int   volume() const { return m_volume; }

    // =========================================================================
    // 状态查询
    // =========================================================================

    /** currentKey() — 当前播放的音乐键名（空 = 未播放） */
    QString currentKey() const { return m_currentKey; }

    /** isPlaying() — 是否正在播放 */
    bool    isPlaying()  const;

    // =========================================================================
    // 循环与静音
    // =========================================================================

    /** setLoop() — 设置是否循环播放（默认 true） */
    void setLoop(bool loop);

    /** setMuted() — 设置静音（不影响 m_volume 值） */
    void setMuted(bool muted);

    /** isMuted() — 是否静音 */
    bool isMuted() const { return m_muted; }

private:
    QMediaPlayer  *m_player      = nullptr;   // Qt6 媒体播放器实例
    QAudioOutput  *m_audioOutput = nullptr;   // Qt6 音频输出实例
    QMap<QString, QString> m_tracks;          // key → 文件绝对路径 映射表
    QString m_currentKey;                     // 当前播放的键名
    int     m_volume = 60;                   // 音量 [0, 100]，默认 60%
    bool    m_muted  = false;                // 静音标志
    bool    m_loop   = true;                 // 循环标志（默认开启）
};
