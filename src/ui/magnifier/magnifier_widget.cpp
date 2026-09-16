#include "magnifier_widget.h"
#include <QPainter>
#include <QApplication>

MagnifierWidget::MagnifierWidget(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TransparentForMouseEvents, true); // 鼠标穿透，不拦截事件
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setFixedSize(PREVIEW_SIZE + 16, PREVIEW_SIZE + 64); // 136 x 184
}

QString MagnifierWidget::hexColor() const
{
    return m_currentColor.name(QColor::HexRgb).toUpper();
}

void MagnifierWidget::updatePosition(const QPoint& cursorPos, const QPixmap& fullSnapshot, const QRect& boundaryRect)
{
    m_currentPixelPos = cursorPos;

    // 换算光标逻辑坐标至物理像素坐标，确保采样点与准星 100% 重合
    qreal dpr = fullSnapshot.devicePixelRatio();
    if (dpr <= 0.0) dpr = 1.0;

    int physCenterX = static_cast<int>(std::round(cursorPos.x() * dpr));
    int physCenterY = static_cast<int>(std::round(cursorPos.y() * dpr));

    // 1. 裁剪 15x15 像素微小区域 (按物理像素精确切图)
    int cropX = physCenterX - SAMPLE_RADIUS;
    int cropY = physCenterY - SAMPLE_RADIUS;
    QRect cropRect(cropX, cropY, GRID_SIZE, GRID_SIZE);

    QPixmap sample = fullSnapshot.copy(cropRect);
    m_magnifiedPixmap = sample.scaled(PREVIEW_SIZE, PREVIEW_SIZE, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    // 2. 提取当前中心像素颜色
    QImage sampleImg = sample.toImage();
    if (SAMPLE_RADIUS < sampleImg.width() && SAMPLE_RADIUS < sampleImg.height()) {
        m_currentColor = sampleImg.pixelColor(SAMPLE_RADIUS, SAMPLE_RADIUS);
    } else {
        m_currentColor = Qt::black;
    }

    // 3. 边界动态避让计算 (默认右下 +20px，越界则翻转至左上)
    int targetX = cursorPos.x() + 20;
    int targetY = cursorPos.y() + 20;

    if (targetX + width() > boundaryRect.right()) {
        targetX = cursorPos.x() - width() - 20;
    }
    if (targetY + height() > boundaryRect.bottom()) {
        targetY = cursorPos.y() - height() - 20;
    }

    move(targetX, targetY);
    update();
}

void MagnifierWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 1. 绘制暗色半透明面板背景与圆角边框
    QRect bgRect = rect().adjusted(0, 0, -1, -1);
    painter.setPen(QColor(60, 60, 60, 200));
    painter.setBrush(QColor(25, 25, 25, 235));
    painter.drawRoundedRect(bgRect, 6, 6);

    // 2. 绘制放大网格底图
    int startX = 8;
    int startY = 8;
    QRect gridRect(startX, startY, PREVIEW_SIZE, PREVIEW_SIZE);
    if (!m_magnifiedPixmap.isNull()) {
        painter.drawPixmap(gridRect, m_magnifiedPixmap);
    }

    // 3. 绘制 15x15 像素分割网格线
    painter.setPen(QColor(255, 255, 255, 35));
    for (int i = 0; i <= GRID_SIZE; ++i) {
        // 竖线
        painter.drawLine(startX + i * CELL_SIZE, startY, startX + i * CELL_SIZE, startY + PREVIEW_SIZE);
        // 横线
        painter.drawLine(startX, startY + i * CELL_SIZE, startX + PREVIEW_SIZE, startY + i * CELL_SIZE);
    }

    // 4. 高亮绘制中心十字准星与当前像素框
    int centerX = startX + SAMPLE_RADIUS * CELL_SIZE;
    int centerY = startY + SAMPLE_RADIUS * CELL_SIZE;
    QRect centerCell(centerX, centerY, CELL_SIZE, CELL_SIZE);

    // 准星外框 (反色对比)
    QColor crossColor = (m_currentColor.lightness() > 128) ? Qt::black : Qt::cyan;
    painter.setPen(QPen(crossColor, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(centerCell);

    // 5. 绘制信息文字区域 (物理坐标与 HEX/RGB)
    painter.setRenderHint(QPainter::Antialiasing, true);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    int textY = startY + PREVIEW_SIZE + 14;
    painter.setPen(QColor(210, 210, 210));
    painter.drawText(startX, textY, QString("POS: (%1, %2)").arg(m_currentPixelPos.x()).arg(m_currentPixelPos.y()));

    textY += 14;
    painter.drawText(startX, textY, QString("RGB: (%1, %2, %3)")
                     .arg(m_currentColor.red())
                     .arg(m_currentColor.green())
                     .arg(m_currentColor.blue()));

    textY += 14;
    // 颜色色块指示
    painter.setBrush(m_currentColor);
    painter.setPen(Qt::white);
    painter.drawRect(startX, textY - 8, 10, 10);

    painter.setPen(QColor(0, 200, 255));
    painter.drawText(startX + 16, textY, hexColor() + " (C:复制)");
}
