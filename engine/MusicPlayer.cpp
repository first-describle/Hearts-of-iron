/**
 * ===========================================================================
 * MusicPlayer.cpp — 背景音乐管理系统实现
 * ===========================================================================
 *
 * 【所属模块】引擎层 — 音频播放
 * 【依赖头文件】MusicPlayer.h, QFileInfo (文件存在性检查)
 *
 * 本文件实现了基于 Qt6 Multimedia 的背景音乐播放:
 *   - QMediaPlayer:  负责加载媒体源（本地文件 URL）和播放控制
 *   - QAudioOutput:  负责音频输出到扬声器
 *   - 循环机制:      播放结束后自动重播（mediaStatusChanged 信号）
 *   - 音量映射:      [0, 100] ↔ [0.0, 1.0] (QAudioOutput 使用浮点音量)
 *
 * Qt6 vs Qt5 差异:
 *   Qt6 将媒体播放和音频输出分离为两个独立对象。
 *   QMediaPlayer::setAudioOutput() 将两者绑定。
 *   音量通过 QAudioOutput::setVolume() 设置。
 *
 * 架构: QMediaPlayer → QAudioOutput → 系统音频设备
 *        (媒体源)     (音量/静音)      (扬声器)
 * ===========================================================================
 */

#include "MusicPlayer.h"
#include <QFileInfo>  // 检查音乐文件是否存在

// ===========================================================================
// 构造函数
// ===========================================================================
/**
 * 【初始化流程】
 *   ① 创建 QMediaPlayer 实例（父对象为 this，自动管理生命周期）
 *   ② 创建 QAudioOutput 实例
 *   ③ 将两者绑定: m_player->setAudioOutput(m_audioOutput)
 *   ④ 设置初始音量: m_volume / 100.0 → [0.0, 1.0]
 *   ⑤ 连接循环播放信号:
 *      mediaStatusChanged 在媒体状态变化时发射。
 *      当状态变为 EndOfMedia（播放完毕）且循环开启时，
 *      将播放位置归零并重新播放，实现无缝循环。
 */
MusicPlayer::MusicPlayer(QObject *parent)
    : QObject(parent)
{
    // ── 创建 Qt6 媒体组件 ──
    m_player      = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);

    // ── 绑定播放器与音频输出 ──
    m_player->setAudioOutput(m_audioOutput);

    // ── 设置初始音量 ──
    // QAudioOutput 使用浮点音量 [0.0, 1.0]
    m_audioOutput->setVolume(m_volume / 100.0f);

    // ── 循环播放机制 ──
    // 当曲目播放完毕时，将位置复位到开头并重新播放。
    // 这实现的是"单曲循环"（repeat-one），而非"列表循环"（repeat-all）。
    // 游戏中的音乐切换由 StoryNode::musicKey 驱动。
    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, [this](QMediaPlayer::MediaStatus status) {
        // EndOfMedia: 播放器已到达媒体末尾
        if (m_loop && status == QMediaPlayer::EndOfMedia) {
            m_player->setPosition(0);   // 回到开头
            m_player->play();           // 重新播放
        }
    });
}

// ===========================================================================
// 析构函数
// ===========================================================================
// QObject 父子机制自动管理 m_player 和 m_audioOutput 的生命周期。
// 无需手动 delete。
MusicPlayer::~MusicPlayer() {}

// ===========================================================================
// registerTrack() — 注册音乐 (公开方法)
// ===========================================================================
/**
 * 将音乐键名映射到文件路径，存入 m_tracks 表。
 * 通常在 MainWindow::onDlcSelected() 中批量调用，
 * 遍历 DlcManifest::music 映射进行注册。
 *
 * @param key      音乐键名（如 "main_theme", "battle_ambient"）
 * @param filePath 音乐文件的绝对路径
 */
void MusicPlayer::registerTrack(const QString &key, const QString &filePath) {
    m_tracks[key] = filePath;
}

// ===========================================================================
// play() — 播放音乐 (公开方法)
// ===========================================================================
/**
 * 【播放逻辑】
 *   ① 去重检查: 若请求的与当前播放的一致且处于播放状态 → 跳过
 *   ② 键名存在性: 若 key 不在 m_tracks 中 → 静默跳过
 *   ③ 文件存在性: 若路径指向的文件不存在 → 静默跳过
 *   ④ 设置媒体源并播放:
 *      - setSource(QUrl::fromLocalFile(path)): 加载本地文件
 *      - play(): 开始播放
 *
 * 【静默跳过的原因】
 *   音乐是可选的增强功能，不应因文件缺失而中断游戏体验。
 *   若需要调试音乐问题，可在此处添加 qWarning() 日志。
 */
void MusicPlayer::play(const QString &key) {
    // ── 去重: 已在播放同一首歌 → 不中断 ──
    if (key == m_currentKey && m_player->playbackState() == QMediaPlayer::PlayingState)
        return;

    // ── 键名检查 ──
    if (!m_tracks.contains(key)) return;

    const QString &path = m_tracks[key];

    // ── 文件存在性检查 ──
    if (!QFileInfo::exists(path)) return;

    // ── 加载并播放 ──
    m_currentKey = key;
    m_player->setSource(QUrl::fromLocalFile(path));  // 加载本地文件 URL
    m_player->play();                                 // 开始播放
}

// ===========================================================================
// stop() — 停止播放 (公开方法)
// ===========================================================================
void MusicPlayer::stop() {
    m_player->stop();
    m_currentKey.clear();
}

// ===========================================================================
// setVolume() — 设置音量 (公开方法)
// ===========================================================================
/**
 * 【音量范围】[0, 100] → 内部转为 [0.0, 1.0]
 * 使用 qBound 确保输入在有效范围内。
 * 若当前处于静音状态，不更新音频输出音量（保持 0.0）。
 *
 * @param vol 音量值 [0, 100]
 */
void MusicPlayer::setVolume(int vol) {
    m_volume = qBound(0, vol, 100);     // 钳制在 [0, 100]
    if (!m_muted)
        m_audioOutput->setVolume(m_volume / 100.0f);  // 转为 [0.0, 1.0]
}

// ===========================================================================
// isPlaying() — 播放状态查询 (公开方法)
// ===========================================================================
bool MusicPlayer::isPlaying() const {
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}

// ===========================================================================
// setLoop() — 循环开关 (公开方法)
// ===========================================================================
void MusicPlayer::setLoop(bool loop) {
    m_loop = loop;
}

// ===========================================================================
// setMuted() — 静音开关 (公开方法)
// ===========================================================================
/**
 * 【静音实现】
 *   不修改 m_volume 值！静音时将输出音量设为 0.0，
 *   取消静音时恢复 m_volume 对应的音量。
 *   这样用户设置的音量偏好不会因静音操作而丢失。
 */
void MusicPlayer::setMuted(bool muted) {
    m_muted = muted;
    m_audioOutput->setVolume(muted ? 0.0f : m_volume / 100.0f);
}
