#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPoint>

class PinWindow : public QWidget
{
    Q_OBJECT
public:
    PinWindow(const QPixmap& pixmap, const QRect& initialGeometry, QWidget* parent = nullptr);
    ~PinWindow() override = default;

    void triggerOcr();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QPixmap m_originalPixmap;
    double m_scaleFactor = 1.0;
    double m_opacity = 1.0;
    QPoint m_dragStartPos;
    bool m_isDragging = false;
};
