#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QRect>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include "../../core/capture/scroll_stitcher.h"

// 辅助滚动长截图独立视口边框挂件 (鼠标穿透，不阻挡滚轮)
class ScrollOutlineFrame : public QWidget
{
    Q_OBJECT
public:
    explicit ScrollOutlineFrame(QWidget* parent = nullptr);
    void setOutlineRect(const QRect& rect);

protected:
    void paintEvent(QPaintEvent* event) override;
};

// 辅助滚动长截图控制器 (管理穿透边框、实时抓帧采样与浮动控制胶囊)
class ScrollCaptureSession : public QObject
{
    Q_OBJECT
public:
    explicit ScrollCaptureSession(QObject* parent = nullptr);
    ~ScrollCaptureSession() override;

    // 启动长截图会话 (传入截取视口区域与起始基准帧)
    void start(const QRect& captureRect, const QImage& initialFrame);

    // 完成长截图并弹出保存文件对话框
    void save();

    // 取消长截图
    void cancel();

    bool isActive() const { return m_isActive; }

signals:
    void finished(const QPixmap& stitchedPixmap);
    void cancelled();

private:
    void onSampleTick();

    QRect m_captureRect;
    ScrollStitcher m_stitcher;
    QTimer* m_sampleTimer = nullptr;

    ScrollOutlineFrame* m_outlineFrame = nullptr;
    QWidget* m_pillWidget = nullptr;
    QLabel* m_heightLabel = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    bool m_isActive = false;
};
