#include "scroll_capture_widget.h"
#include "../../core/export/image_exporter.h"
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QDateTime>
#include <QDebug>

// -------------------------------------------------------------
// ScrollOutlineFrame: 视口边框 (设置鼠标穿透，绝对不阻挡滚轮)
// -------------------------------------------------------------
ScrollOutlineFrame::ScrollOutlineFrame(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true); // 鼠标完全穿透至底层真实窗口
    setAttribute(Qt::WA_DeleteOnClose, false);
}

void ScrollOutlineFrame::setOutlineRect(const QRect& rect)
{
    setGeometry(rect.adjusted(-2, -2, 2, 2));
    update();
}

void ScrollOutlineFrame::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 绘制 2px 活力科技蓝视口边界框
    painter.setPen(QPen(QColor(37, 99, 235, 230), 2.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));

    // 绘制内侧 1px 白色微高光
    painter.setPen(QPen(QColor(255, 255, 255, 140), 1.0));
    painter.drawRect(rect().adjusted(2, 2, -2, -2));
}

// -------------------------------------------------------------
// ScrollCaptureSession: 辅助滚动长截图控制器 (极简整齐界面，仅保存/取消)
// -------------------------------------------------------------
ScrollCaptureSession::ScrollCaptureSession(QObject* parent)
    : QObject(parent)
{
    m_outlineFrame = new ScrollOutlineFrame(nullptr);

    // 独立的悬浮控制胶囊挂件 (极简整洁，无图标累赘)
    m_pillWidget = new QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    m_pillWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    m_pillWidget->setStyleSheet(
        "QWidget#pillRoot {"
        "  background-color: rgba(15, 23, 42, 245);"
        "  border: 1px solid rgba(255, 255, 255, 50);"
        "  border-radius: 16px;"
        "}"
        "QLabel {"
        "  color: #f8fafc;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton#saveBtn {"
        "  background-color: #2563eb;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 5px 16px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#saveBtn:hover { background-color: #1d4ed8; }"
        "QPushButton#cancelBtn {"
        "  background-color: rgba(255, 255, 255, 25);"
        "  color: #e2e8f0;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 5px 14px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton#cancelBtn:hover { background-color: rgba(239, 68, 68, 200); color: #ffffff; }"
    );

    auto* pillContainer = new QWidget(m_pillWidget);
    pillContainer->setObjectName("pillRoot");

    auto* layout = new QHBoxLayout(pillContainer);
    layout->setContentsMargins(16, 6, 16, 6);
    layout->setSpacing(12);

    auto* tipLabel = new QLabel("长截图进行中", pillContainer);
    layout->addWidget(tipLabel);

    m_heightLabel = new QLabel("高度: 0px", pillContainer);
    m_heightLabel->setStyleSheet("color: #38bdf8; font-weight: bold; margin-right: 6px;");
    layout->addWidget(m_heightLabel);

    // 仅保留保存按钮 (无图标，纯文本极简)
    m_saveBtn = new QPushButton("保存", pillContainer);
    m_saveBtn->setObjectName("saveBtn");
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    connect(m_saveBtn, &QPushButton::clicked, this, &ScrollCaptureSession::save);
    layout->addWidget(m_saveBtn);

    // 仅保留取消按钮 (无图标，纯文本极简)
    m_cancelBtn = new QPushButton("取消", pillContainer);
    m_cancelBtn->setObjectName("cancelBtn");
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(m_cancelBtn, &QPushButton::clicked, this, &ScrollCaptureSession::cancel);
    layout->addWidget(m_cancelBtn);

    auto* outerLayout = new QVBoxLayout(m_pillWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(pillContainer);

    // 20fps 高速采样定时器 (每 50ms 采样一次屏幕视口画面)
    m_sampleTimer = new QTimer(this);
    m_sampleTimer->setInterval(50);
    connect(m_sampleTimer, &QTimer::timeout, this, &ScrollCaptureSession::onSampleTick);
}

ScrollCaptureSession::~ScrollCaptureSession()
{
    if (m_outlineFrame) {
        m_outlineFrame->close();
        delete m_outlineFrame;
    }
    if (m_pillWidget) {
        m_pillWidget->close();
        delete m_pillWidget;
    }
}

void ScrollCaptureSession::start(const QRect& captureRect, const QImage& initialFrame)
{
    m_captureRect = captureRect.normalized();
    m_stitcher.reset(initialFrame);
    m_isActive = true;

    // 显示穿透边框，指示捕获视口
    m_outlineFrame->setOutlineRect(m_captureRect);
    m_outlineFrame->show();
    m_outlineFrame->raise();

    // 更新悬浮控制胶囊文字与位置
    m_heightLabel->setText(QString("高度: %1px (1帧)").arg(initialFrame.height()));
    m_pillWidget->adjustSize();

    int pillX = m_captureRect.center().x() - m_pillWidget->width() / 2;
    int pillY = m_captureRect.bottom() + 12;

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeom = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);

    if (pillY + m_pillWidget->height() > screenGeom.bottom() - 10) {
        pillY = m_captureRect.top() - m_pillWidget->height() - 12;
    }
    pillX = qBound(10, pillX, screenGeom.right() - m_pillWidget->width() - 10);
    pillY = qBound(10, pillY, screenGeom.bottom() - m_pillWidget->height() - 10);

    m_pillWidget->move(pillX, pillY);
    m_pillWidget->show();
    m_pillWidget->raise();

    // 启动实时抓帧采样
    m_sampleTimer->start();
}

void ScrollCaptureSession::onSampleTick()
{
    if (!m_isActive || m_captureRect.isEmpty()) return;

    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QPixmap livePix = screen->grabWindow(0, m_captureRect.x(), m_captureRect.y(), m_captureRect.width(), m_captureRect.height());
    if (livePix.isNull()) return;

    int deltaY = 0;
    if (m_stitcher.appendFrame(livePix.toImage(), deltaY)) {
        m_heightLabel->setText(QString("高度: %1px (%2帧)")
                               .arg(m_stitcher.currentTotalHeight())
                               .arg(m_stitcher.frameCount()));
    }
}

void ScrollCaptureSession::save()
{
    if (!m_isActive) return;
    m_isActive = false;
    m_sampleTimer->stop();

    if (m_outlineFrame) m_outlineFrame->hide();
    if (m_pillWidget) m_pillWidget->hide();

    QImage stitched = m_stitcher.currentStitchedImage();
    if (!stitched.isNull()) {
        QPixmap res = QPixmap::fromImage(stitched);
        QString defaultName = QString("Evan_Long_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        QString path = QFileDialog::getSaveFileName(nullptr, "保存长截图", defaultName, ImageExporter::getSaveFileFilter());
        if (!path.isEmpty()) {
            ImageExporter::saveImage(stitched, path);
        }
        // 同步存入系统剪贴板备用
        QApplication::clipboard()->setPixmap(res);
        emit finished(res);
    } else {
        emit cancelled();
    }
}

void ScrollCaptureSession::cancel()
{
    if (!m_isActive) return;
    m_isActive = false;
    m_sampleTimer->stop();

    if (m_outlineFrame) m_outlineFrame->hide();
    if (m_pillWidget) m_pillWidget->hide();

    emit cancelled();
}
