#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRect>
#include <QTimer>
#include <QElapsedTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <vector>

// GIF 录制选区外边框挂件 (红色虚线呼吸框，鼠标穿透)
class GifOutlineFrame : public QWidget
{
    Q_OBJECT
public:
    explicit GifOutlineFrame(QWidget* parent = nullptr);
    void setOutlineRect(const QRect& rect);
    void setRecordingState(bool isRecording);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    bool m_isRecording = false;
};

// GIF 动态录屏控制器 (所见即所得录制悬浮面板与多帧动画导出)
class GifRecordSession : public QObject
{
    Q_OBJECT
public:
    explicit GifRecordSession(QObject* parent = nullptr);
    ~GifRecordSession() override;

    // 启动录制准备会话 (传入目标录制区域)
    /**
     * @brief 启动 GIF 录制会话
     * @param captureRect  录制视口的「全局逻辑矩形」(用于边框与控制条定位)
     * @param physicalRect 录制视口的「物理像素矩形」(用于跨屏精确采样，可为空)
     */
    void start(const QRect& captureRect, const QRect& physicalRect = QRect());

    // 开始录制
    void startRecording();

    // 停止录制并弹出文件保存对话框
    void stopAndSave();

    // 取消录制
    void cancel();

    bool isActive() const { return m_isActive; }
    bool isRecording() const { return m_isRecording; }

signals:
    void finished(const QString& savedPath);
    void cancelled();

private:
    void onFrameSampleTick();
    void updateTimerDisplay();

    QRect m_captureRect;
    QRect m_physicalRect;   // 物理像素视口 (跨屏采样时使用)
    bool m_isActive = false;
    bool m_isRecording = false;

    GifOutlineFrame* m_outlineFrame = nullptr;
    QWidget* m_barWidget = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_recordBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    QTimer* m_frameTimer = nullptr;
    QElapsedTimer m_recordElapsedTimer;
    std::vector<QImage> m_recordedFrames;
};
