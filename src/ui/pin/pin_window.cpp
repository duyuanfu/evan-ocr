#include "pin_window.h"
#include "../../core/hotkey_config.h"
#include "../../core/theme_manager.h"
#include "../../core/ocr/ocr_manager.h"
#include <QDateTime>
#include "../ocr/ocr_result_dialog.h"
#include <QPainter>
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
    // 设置无边框、置顶、独立工具窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setCursor(Qt::SizeAllCursor);

    setGeometry(initialGeometry);
    show();
    activateWindow();
    raise();
}

void PinWindow::paintEvent(QPaintEvent* /*event*/)
{
    if (m_originalPixmap.isNull()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setOpacity(m_opacity);

    // 缩放绘制原始快照位图
    painter.drawPixmap(rect(), m_originalPixmap);

    // 绘制精细白色半透明内描边
    painter.setPen(QColor(255, 255, 255, 60));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
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
        setWindowOpacity(m_opacity);
        update();
    } else {
        // 普通滚轮：以鼠标当前位置为锚点进行等比缩放
        double factor = (numSteps > 0) ? 1.10 : 0.90;
        double newScale = m_scaleFactor * factor;

        // 限制缩放范围在 20% ~ 500%
        if (newScale >= 0.20 && newScale <= 5.0) {
            m_scaleFactor = newScale;

            QPoint mousePos = event->position().toPoint();
            int newW = static_cast<int>(m_originalPixmap.width() * m_scaleFactor);
            int newH = static_cast<int>(m_originalPixmap.height() * m_scaleFactor);

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
    auto* copyAct = menu.addAction(QString("复制图片 (%1)").arg(c.pinCopy.isEmpty() ? "Ctrl+C" : c.pinCopy));
    auto* saveAct = menu.addAction(QString("另存为... (%1)").arg(c.pinSave.isEmpty() ? "Ctrl+S" : c.pinSave));
    menu.addSeparator();
    auto* closeAct = menu.addAction(QString("关闭贴图 (%1 / 双击)").arg(c.pinClose.isEmpty() ? "Esc" : c.pinClose));

    QAction* selected = menu.exec(event->globalPos());
    if (selected == ocrAct) {
        triggerOcr();
    } else if (selected == copyAct) {
        QApplication::clipboard()->setPixmap(m_originalPixmap);
    } else if (selected == saveAct) {
        QString defaultName = QString("Evan_Pin_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        QString path = QFileDialog::getSaveFileName(this, "保存贴图", defaultName, "PNG 图像 (*.png);;WebP 图像 (*.webp);;JPEG 图像 (*.jpg *.jpeg);;位图 (*.bmp)");
        if (!path.isEmpty()) {
            m_originalPixmap.save(path);
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
        QString path = QFileDialog::getSaveFileName(this, "保存贴图", defaultName, "PNG 图像 (*.png);;WebP 图像 (*.webp);;JPEG 图像 (*.jpg *.jpeg);;位图 (*.bmp)");
        if (!path.isEmpty()) {
            m_originalPixmap.save(path);
        }
        return;
    }
    QWidget::keyPressEvent(event);
}
