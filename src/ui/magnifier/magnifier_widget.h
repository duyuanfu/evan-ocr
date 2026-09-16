#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QColor>

class MagnifierWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MagnifierWidget(QWidget* parent = nullptr);
    ~MagnifierWidget() override = default;

    // 更新放大镜位置与当前指向的像素底图及坐标
    void updatePosition(const QPoint& cursorPos, const QPixmap& fullSnapshot, const QRect& boundaryRect);

    // 获取当前采样点的颜色
    QColor currentColor() const { return m_currentColor; }
    QString hexColor() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    static constexpr int SAMPLE_RADIUS = 7; // 采样半径：中心点前后各 7 像素，总计 15x15
    static constexpr int GRID_SIZE = 15;
    static constexpr int CELL_SIZE = 8;     // 放大每个像素为 8x8
    static constexpr int PREVIEW_SIZE = GRID_SIZE * CELL_SIZE; // 120x120

    QPixmap m_magnifiedPixmap;
    QPoint m_currentPixelPos;
    QColor m_currentColor;
};
