#include "pin_window.h"
#include "../../core/hotkey_config.h"
#include "../../core/theme_manager.h"
#include "../../core/ocr/ocr_manager.h"
#include "../../core/export/image_exporter.h"
#include <QDateTime>
#include "../ocr/ocr_result_dialog.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QFileDialog>
#include <QApplication>
#include <QClipboard>
#include <algorithm>

PinWindow::PinWindow(const QPixmap& pixmap, const QRect& initialGeometry, QWidget* parent)
    : QWidget(parent), m_originalPixmap(pixmap)
{
    // 设置无边框、透明底色、置顶、独立工具窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setCursor(Qt::SizeAllCursor);

    // 将窗口大小向外扩展 SHADOW_MARGIN，使得内容依然精准停留在截图原位，但四周呈现立体悬浮阴影
    QRect winGeom = initialGeometry.adjusted(-SHADOW_MARGIN, -SHADOW_MARGIN, SHADOW_MARGIN, SHADOW_MARGIN);
    setGeometry(winGeom);

    show();
    activateWindow();
    raise();
}

QRect PinWindow::contentRect() const
{
    return rect().adjusted(SHADOW_MARGIN, SHADOW_MARGIN, -SHADOW_MARGIN, -SHADOW_MARGIN);
}

void PinWindow::paintEvent(QPaintEvent* /*event*/)
{
    if (m_originalPixmap.isNull()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QRect cRect = contentRect();
    if (cRect.isEmpty()) return;

    // 1. 绘制多层高柔和度立体落影 (模仿 macOS 浮动窗口与 Windows 11 Fluent 深度阴影)
    // 阴影自 16px 逐渐向外衰减，略带 y 轴下沉，即使原图与桌面背景 100% 相同也能一眼辨识贴图边界
    for (int r = SHADOW_MARGIN; r >= 1; --r) {
        QRect shadowRect = cRect.adjusted(-r, -r + 2, r, r + 2);
        double ratio = 1.0 - (static_cast<double>(r) / SHADOW_MARGIN);
        int alpha = static_cast<int>(38.0 * ratio * ratio); // 平滑非线性渐变
        if (alpha > 0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, alpha));
            painter.drawRoundedRect(shadowRect, 6, 6);
        }
    }

    // 2. 绘制贴图实体画面
    painter.setOpacity(m_opacity);
    painter.drawPixmap(cRect, m_originalPixmap);

    // 3. 绘制精致悬浮外描边 (悬停时高亮蓝色，平时为精致科技蓝灰色，清晰界定贴图与底图)
    QColor borderColor = m_isHovered ? QColor(37, 99, 235, 230) : QColor(59, 130, 246, 170);
    painter.setPen(QPen(borderColor, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(cRect.adjusted(0, 0, -1, -1), 3, 3);

    // 4. 绘制 1px 白色半透明高光内描边
    painter.setPen(QPen(QColor(255, 255, 255, 110), 1.0));
    painter.drawRoundedRect(cRect.adjusted(1, 1, -2, -2), 2, 2);
}

void PinWindow::enterEvent(QEnterEvent* /*event*/)
{
    m_isHovered = true;
    update();
}

void PinWindow::leaveEvent(QEvent* /*event*/)
{
    m_isHovered = false;
    update();
}

void PinWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void PinWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
    }
}

void PinWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        event->accept();
    }
}

void PinWindow::wheelEvent(QWheelEvent* event)
{
    int numDegrees = event->angleDelta().y() / 8;
    int numSteps = numDegrees / 15;

    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl + 滚轮：动态调节透明度 (10% ~ 100%)
        m_opacity += (numSteps * 0.05);
        m_opacity = std::clamp(m_opacity, 0.10, 1.0);
        update();
    } else {
        // 普通滚轮：以鼠标当前位置为锚点进行等比缩放
        double factor = (numSteps > 0) ? 1.10 : 0.90;
        double newScale = m_scaleFactor * factor;

        // 限制缩放范围在 20% ~ 500%
        if (newScale >= 0.20 && newScale <= 5.0) {
            m_scaleFactor = newScale;

            QPoint mousePos = event->position().toPoint();
            int newW = static_cast<int>(m_originalPixmap.width() * m_scaleFactor) + 2 * SHADOW_MARGIN;
            int newH = static_cast<int>(m_originalPixmap.height() * m_scaleFactor) + 2 * SHADOW_MARGIN;

            // 保持鼠标下的相对像素点稳定
            double rx = static_cast<double>(mousePos.x()) / width();
            double ry = static_cast<double>(mousePos.y()) / height();

            QPoint globalMouse = event->globalPosition().toPoint();
            int targetX = globalMouse.x() - static_cast<int>(newW * rx);
            int targetY = globalMouse.y() - static_cast<int>(newH * ry);

            setGeometry(targetX, targetY, newW, newH);
            update();
        }
    }
    event->accept();
}

void PinWindow::triggerOcr()
{
    auto* dialog = new OcrResultDialog();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setImage(m_originalPixmap);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();

    OcrManager::instance().recognizeAsync(m_originalPixmap.toImage(), m_originalPixmap.devicePixelRatio(), [dialog](const OcrResult& res, const QString& engineName) {
        dialog->setResult(res, engineName);
    });
}

void PinWindow::contextMenuEvent(QContextMenuEvent* event)
{
    const auto& c = HotkeyConfig::instance().data();
    QMenu menu(this);
    menu.setStyleSheet(ThemeManager::instance().getPinMenuStyle());

    auto* ocrAct = menu.addAction(QString("🔍 识别图中文字 (%1)").arg(c.pinOcr.isEmpty() ? "Ctrl+O" : c.pinOcr));
    menu.addSeparator();
    auto* copyAct = menu.addAction(QString("📋 复制图片 (%1)").arg(c.pinCopy.isEmpty() ? "Ctrl+C" : c.pinCopy));
    auto* saveAct = menu.addAction(QString("💾 保存图片为... (%1)").arg(c.pinSave.isEmpty() ? "Ctrl+S" : c.pinSave));
    menu.addSeparator();
    auto* closeAct = menu.addAction(QString("❌ 关闭贴图 (%1 / 双击)").arg(c.pinClose.isEmpty() ? "Esc" : c.pinClose));

    QAction* selected = menu.exec(event->globalPos());
    if (selected == ocrAct) {
        triggerOcr();
    } else if (selected == copyAct) {
        QApplication::clipboard()->setPixmap(m_originalPixmap);
    } else if (selected == saveAct) {
        QString defaultName = QString("Evan_Pin_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        QString path = QFileDialog::getSaveFileName(this, "保存贴图", defaultName, ImageExporter::getSaveFileFilter());
        if (!path.isEmpty()) {
            ImageExporter::saveImage(m_originalPixmap.toImage(), path);
        }
    } else if (selected == closeAct) {
        close();
    }
}

void PinWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        close();
    }
}

void PinWindow::keyPressEvent(QKeyEvent* event)
{
    const auto& c = HotkeyConfig::instance().data();
    if (HotkeyConfig::matches(event, c.pinClose) || event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    if (HotkeyConfig::matches(event, c.pinOcr) ||
        ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_O) ||
        (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_O)) {
        triggerOcr();
        return;
    }
    if (HotkeyConfig::matches(event, c.pinCopy) ||
        ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_C)) {
        QApplication::clipboard()->setPixmap(m_originalPixmap);
        return;
    }
    if (HotkeyConfig::matches(event, c.pinSave) ||
        ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_S)) {
        QString defaultName = QString("Evan_Pin_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        QString path = QFileDialog::getSaveFileName(this, "保存贴图", defaultName, ImageExporter::getSaveFileFilter());
        if (!path.isEmpty()) {
            ImageExporter::saveImage(m_originalPixmap.toImage(), path);
        }
        return;
    }
    QWidget::keyPressEvent(event);
}
