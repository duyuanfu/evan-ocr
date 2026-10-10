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
    void start(const QRect& captureRect);

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
