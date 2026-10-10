#include "gif_recorder_widget.h"
#include "../../core/export/image_exporter.h"
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QDebug>

// -------------------------------------------------------------
// GifOutlineFrame: 红色虚线录制框 (鼠标完全穿透至底层)
// -------------------------------------------------------------
GifOutlineFrame::GifOutlineFrame(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
}

void GifOutlineFrame::setOutlineRect(const QRect& rect)
{
    setGeometry(rect.adjusted(-2, -2, 2, 2));
    update();
}

void GifOutlineFrame::setRecordingState(bool isRecording)
{
    m_isRecording = isRecording;
    update();
}

void GifOutlineFrame::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor penColor = m_isRecording ? QColor(239, 68, 68, 230) : QColor(245, 158, 11, 230);
    QPen pen(penColor, 2.0, Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
}

// -------------------------------------------------------------
// 多帧动图 GIF89a 独立编码器
// -------------------------------------------------------------
static bool encodeMultiFrameGif(const QString& filePath, const std::vector<QImage>& frames, int delayCentiseconds = 10)
{
    if (frames.empty()) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;

    int width = frames[0].width();
    int height = frames[0].height();

    // 1. GIF89a Header
    file.write("GIF89a", 6);

    // 2. Logical Screen Descriptor
    QImage firstIndexed = frames[0].convertToFormat(QImage::Format_Indexed8);
    const auto firstColors = firstIndexed.colorTable();
    int colorCount = qMin(256, (int)firstColors.size());

    char lsd[7];
    lsd[0] = (char)(width & 0xFF);
    lsd[1] = (char)((width >> 8) & 0xFF);
    lsd[2] = (char)(height & 0xFF);
    lsd[3] = (char)((height >> 8) & 0xFF);
    lsd[4] = '\xF7'; // Global Color Table Flag, 256 colors
    lsd[5] = 0;
    lsd[6] = 0;
    file.write(lsd, 7);

    // 3. Global Color Table (768 字节)
    for (int i = 0; i < 256; ++i) {
        if (i < colorCount) {
            QRgb rgb = firstColors[i];
            char rgbBuf[3] = {(char)qRed(rgb), (char)qGreen(rgb), (char)qBlue(rgb)};
            file.write(rgbBuf, 3);
        } else {
            file.write("\x00\x00\x00", 3);
        }
    }

    // 4. Netscape 2.0 Loop Extension (循环播放动画)
    char netscape[19] = {
        '\x21', '\xFF', '\x0B',
        'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0',
        '\x03', '\x01', '\x00', '\x00', '\x00'
    };
    file.write(netscape, 19);

    // 5. 逐帧写入图像
    for (const auto& rawFrame : frames) {
        QImage indexed = rawFrame.convertToFormat(QImage::Format_Indexed8);
        const auto colorTable = indexed.colorTable();
        int localColorCount = qMin(256, (int)colorTable.size());

        // Graphic Control Extension (单帧延迟时间)
        char gce[8] = {
            '\x21', '\xF9', '\x04',
            '\x00',
            (char)(delayCentiseconds & 0xFF),
            (char)((delayCentiseconds >> 8) & 0xFF),
            '\x00', '\x00'
        };
        file.write(gce, 8);

        // Image Descriptor
        char id[10] = {
            '\x2C',
            '\x00', '\x00', '\x00', '\x00',
            (char)(width & 0xFF), (char)((width >> 8) & 0xFF),
            (char)(height & 0xFF), (char)((height >> 8) & 0xFF),
            '\x87' // 存在局部颜色表, 256 色
        };
        file.write(id, 10);

        // Local Color Table
        for (int i = 0; i < 256; ++i) {
            if (i < localColorCount) {
                QRgb rgb = colorTable[i];
                char rgbBuf[3] = {(char)qRed(rgb), (char)qGreen(rgb), (char)qBlue(rgb)};
                file.write(rgbBuf, 3);
            } else {
                file.write("\x00\x00\x00", 3);
            }
        }

        // 栅格像素数据 LZW 压缩流
        const int initCodeSize = 8;
        file.putChar((char)initCodeSize);
        const int clearCode = 1 << initCodeSize; // 256
        const int endCode = clearCode + 1;       // 257

        std::vector<int> prefix(5003, -1);
        std::vector<int> suffix(5003, -1);
        std::vector<int> code(5003, -1);

        auto clearTable = [&]() {
            std::fill(prefix.begin(), prefix.end(), -1);
            std::fill(suffix.begin(), suffix.end(), -1);
            std::fill(code.begin(), code.end(), -1);
        };
        clearTable();

        int n_bits = initCodeSize + 1;
        int maxcode = (1 << n_bits);
        int free_ent = clearCode + 2;

        unsigned int cur_accum = 0;
        int cur_bits = 0;
        std::vector<char> packet;

        auto flushPacket = [&]() {
            if (!packet.empty()) {
                file.putChar((char)packet.size());
                file.write(packet.data(), (qint64)packet.size());
                packet.clear();
            }
        };

        auto writeBits = [&](int c) {
            cur_accum |= (unsigned int)(c << cur_bits);
            cur_bits += n_bits;
            while (cur_bits >= 8) {
                packet.push_back((char)(cur_accum & 0xFF));
                if (packet.size() == 254) flushPacket();
                cur_accum >>= 8;
                cur_bits -= 8;
            }
        };

        writeBits(clearCode);

        int ent = -1;
        for (int y = 0; y < height; ++y) {
            const uchar* scanline = indexed.constScanLine(y);
            for (int x = 0; x < width; ++x) {
                int c = scanline[x];
                if (ent == -1) { ent = c; continue; }

                int disp = ((c << 4) ^ ent);
                int h = (ent << 8) | c;
                int i = h % 5003;
                if (disp == 0) disp = 1;

                bool found = false;
                while (code[i] != -1) {
                    if (prefix[i] == ent && suffix[i] == c) {
                        ent = code[i];
                        found = true;
                        break;
                    }
                    i = (i + disp) % 5003;
                }

                if (!found) {
                    writeBits(ent);
                    if (free_ent < 4096) {
                        prefix[i] = ent;
                        suffix[i] = c;
                        code[i] = free_ent++;
                        if (free_ent > maxcode && n_bits < 12) {
                            n_bits++;
                            maxcode = (1 << n_bits);
                        }
                    } else {
                        writeBits(clearCode);
                        clearTable();
                        n_bits = initCodeSize + 1;
                        maxcode = (1 << n_bits);
                        free_ent = clearCode + 2;
                    }
                    ent = c;
                }
            }
        }
        if (ent != -1) writeBits(ent);
        writeBits(endCode);

        if (cur_bits > 0) packet.push_back((char)(cur_accum & 0xFF));
        flushPacket();
        file.putChar('\x00');
    }

    // 6. Trailer 0x3B
    file.putChar('\x3B');
    file.close();
    return true;
}

// -------------------------------------------------------------
// GifRecordSession: 动态录屏控制器
// -------------------------------------------------------------
GifRecordSession::GifRecordSession(QObject* parent)
    : QObject(parent)
{
    m_outlineFrame = new GifOutlineFrame(nullptr);

    m_barWidget = new QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    m_barWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    m_barWidget->setStyleSheet(
        "QWidget#barContainer {"
        "  background-color: rgba(15, 23, 42, 245);"
        "  border: 1px solid rgba(255, 255, 255, 60);"
        "  border-radius: 16px;"
        "}"
        "QLabel {"
        "  color: #f8fafc;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton#recBtn {"
        "  background-color: #dc2626;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 5px 16px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#recBtn:hover { background-color: #b91c1c; }"
        "QPushButton#stopBtn {"
        "  background-color: #2563eb;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 5px 16px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#stopBtn:hover { background-color: #1d4ed8; }"
        "QPushButton#cancelBtn {"
        "  background-color: rgba(255, 255, 255, 25);"
        "  color: #e2e8f0;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 5px 14px;"
        "  font-size: 12px;"
        "}"
        "QPushButton#cancelBtn:hover { background-color: rgba(239, 68, 68, 200); color: #ffffff; }"
    );

    auto* container = new QWidget(m_barWidget);
    container->setObjectName("barContainer");

    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(16, 6, 16, 6);
    layout->setSpacing(12);

    m_statusLabel = new QLabel("录制准备就绪", container);
    layout->addWidget(m_statusLabel);

    m_recordBtn = new QPushButton("开始录制", container);
    m_recordBtn->setObjectName("recBtn");
    connect(m_recordBtn, &QPushButton::clicked, this, &GifRecordSession::startRecording);
    layout->addWidget(m_recordBtn);

    m_stopBtn = new QPushButton("停止并保存", container);
    m_stopBtn->setObjectName("stopBtn");
    m_stopBtn->hide();
    connect(m_stopBtn, &QPushButton::clicked, this, &GifRecordSession::stopAndSave);
    layout->addWidget(m_stopBtn);

    m_cancelBtn = new QPushButton("取消", container);
    m_cancelBtn->setObjectName("cancelBtn");
    connect(m_cancelBtn, &QPushButton::clicked, this, &GifRecordSession::cancel);
    layout->addWidget(m_cancelBtn);

    auto* outer = new QVBoxLayout(m_barWidget);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(container);

    // 10fps 高质量帧率定时器 (每 100ms 捕获一帧)
    m_frameTimer = new QTimer(this);
    m_frameTimer->setInterval(100);
    connect(m_frameTimer, &QTimer::timeout, this, &GifRecordSession::onFrameSampleTick);
}

GifRecordSession::~GifRecordSession()
{
    if (m_outlineFrame) { m_outlineFrame->close(); delete m_outlineFrame; }
    if (m_barWidget) { m_barWidget->close(); delete m_barWidget; }
}

void GifRecordSession::start(const QRect& captureRect)
{
    m_captureRect = captureRect.normalized();
    m_isActive = true;
    m_isRecording = false;
    m_recordedFrames.clear();

    m_outlineFrame->setOutlineRect(m_captureRect);
    m_outlineFrame->setRecordingState(false);
    m_outlineFrame->show();
    m_outlineFrame->raise();

    m_statusLabel->setText(QString("录制区域: %1×%2").arg(m_captureRect.width()).arg(m_captureRect.height()));
    m_recordBtn->show();
    m_stopBtn->hide();

    m_barWidget->adjustSize();

    int barX = m_captureRect.center().x() - m_barWidget->width() / 2;
    int barY = m_captureRect.bottom() + 12;

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeom = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);
    if (barY + m_barWidget->height() > screenGeom.bottom() - 10) {
        barY = m_captureRect.top() - m_barWidget->height() - 12;
    }
    barX = qBound(10, barX, screenGeom.right() - m_barWidget->width() - 10);
    barY = qBound(10, barY, screenGeom.bottom() - m_barWidget->height() - 10);

    m_barWidget->move(barX, barY);
    m_barWidget->show();
    m_barWidget->raise();
}

void GifRecordSession::startRecording()
{
    if (!m_isActive || m_isRecording) return;
    m_isRecording = true;
    m_recordedFrames.clear();

    m_outlineFrame->setRecordingState(true);
    m_recordBtn->hide();
    m_stopBtn->show();

    m_recordElapsedTimer.start();
    m_frameTimer->start();
    updateTimerDisplay();
}

void GifRecordSession::onFrameSampleTick()
{
    if (!m_isRecording) return;
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QPixmap pix = screen->grabWindow(0, m_captureRect.x(), m_captureRect.y(), m_captureRect.width(), m_captureRect.height());
    if (!pix.isNull()) {
        m_recordedFrames.push_back(pix.toImage());
    }

    updateTimerDisplay();

    // 安全限制：最多连续录制 500 帧 (约 50 秒)，避免内存溢出
    if (m_recordedFrames.size() >= 500) {
        stopAndSave();
    }
}

void GifRecordSession::updateTimerDisplay()
{
    int secs = static_cast<int>(m_recordElapsedTimer.elapsed() / 1000);
    int mins = secs / 60;
    secs = secs % 60;
    m_statusLabel->setText(QString("录制中 %1:%2 (%3帧)")
                           .arg(mins, 2, 10, QChar('0'))
                           .arg(secs, 2, 10, QChar('0'))
                           .arg(m_recordedFrames.size()));
}

void GifRecordSession::stopAndSave()
{
    if (!m_isActive) return;
    m_frameTimer->stop();
    m_isRecording = false;
    m_isActive = false;

    m_outlineFrame->hide();
    m_barWidget->hide();

    if (m_recordedFrames.empty()) {
        emit cancelled();
        return;
    }

    QString defaultName = QString("Evan_Record_%1.gif").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString savePath = QFileDialog::getSaveFileName(nullptr, "保存 GIF 动图", defaultName, "GIF 动画图像 (*.gif)");

    if (!savePath.isEmpty()) {
        bool ok = encodeMultiFrameGif(savePath, m_recordedFrames, 10);
        if (ok) {
            QMessageBox::information(nullptr, "GIF 录屏完成", QString("GIF 动图已成功录制并导出！\n文件路径: %1\n共录制: %2 帧").arg(savePath).arg(m_recordedFrames.size()));
            emit finished(savePath);
        } else {
            QMessageBox::warning(nullptr, "导出失败", "生成 GIF 动画文件时发生错误。");
            emit cancelled();
        }
    } else {
        emit cancelled();
    }
}

void GifRecordSession::cancel()
{
    if (m_frameTimer) m_frameTimer->stop();
    m_isRecording = false;
    m_isActive = false;
    m_recordedFrames.clear();

    if (m_outlineFrame) m_outlineFrame->hide();
    if (m_barWidget) m_barWidget->hide();

    emit cancelled();
}
