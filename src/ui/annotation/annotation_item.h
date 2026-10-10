#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QColor>
#include <QList>
#include <QFont>
#include <QFontMetrics>
#include <cmath>
#include <algorithm>
#include <memory>

enum class AnnotationType {
    Rect,
    Arrow,
    Pencil,
    Text,
    Mosaic,
    SingleCharEdit
};

class AnnotationItem {
public:
    virtual ~AnnotationItem() = default;
    virtual AnnotationType type() const = 0;
    virtual void draw(QPainter& painter) = 0;
};

// 矩形图元
class RectAnnotation : public AnnotationItem {
public:
    RectAnnotation(const QRect& rect, const QColor& color, int width)
        : m_rect(rect), m_color(color), m_width(width) {}
    AnnotationType type() const override { return AnnotationType::Rect; }
    void draw(QPainter& painter) override {
        painter.setPen(QPen(m_color, m_width));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(m_rect);
    }
private:
    QRect m_rect;
    QColor m_color;
    int m_width;
};

// 箭头图元
class ArrowAnnotation : public AnnotationItem {
public:
    ArrowAnnotation(const QPoint& start, const QPoint& end, const QColor& color, int width)
        : m_start(start), m_end(end), m_color(color), m_width(width) {}
    AnnotationType type() const override { return AnnotationType::Arrow; }
    void draw(QPainter& painter) override {
        painter.setPen(QPen(m_color, m_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(m_start, m_end);

        // 计算箭头两个侧翼
        double dx = m_end.x() - m_start.x();
        double dy = m_end.y() - m_start.y();
        double length = std::sqrt(dx * dx + dy * dy);
        if (length < 2) return;

        double angle = std::atan2(dy, dx);
        double arrowSize = (std::max)(12.0, m_width * 3.5);
        double arrowAngle = 0.5; // ~28 度

        QPoint p1(m_end.x() - static_cast<int>(arrowSize * std::cos(angle - arrowAngle)),
                  m_end.y() - static_cast<int>(arrowSize * std::sin(angle - arrowAngle)));
        QPoint p2(m_end.x() - static_cast<int>(arrowSize * std::cos(angle + arrowAngle)),
                  m_end.y() - static_cast<int>(arrowSize * std::sin(angle + arrowAngle)));

        painter.setBrush(m_color);
        QPolygon arrowHead;
        arrowHead << m_end << p1 << p2;
        painter.drawPolygon(arrowHead);
    }
private:
    QPoint m_start;
    QPoint m_end;
    QColor m_color;
    int m_width;
};

// 平滑贝塞尔画笔图元
class PencilAnnotation : public AnnotationItem {
public:
    PencilAnnotation(const QColor& color, int width)
        : m_color(color), m_width(width) {}
    AnnotationType type() const override { return AnnotationType::Pencil; }
    void addPoint(const QPoint& pt) { m_points.append(pt); }
    void draw(QPainter& painter) override {
        if (m_points.size() < 2) return;
        painter.setPen(QPen(m_color, m_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        for (int i = 0; i < m_points.size() - 1; ++i) {
            painter.drawLine(m_points[i], m_points[i + 1]);
        }
    }
private:
    QList<QPoint> m_points;
    QColor m_color;
    int m_width;
};

// 文本图元
class TextAnnotation : public AnnotationItem {
public:
    TextAnnotation(const QPoint& pos, const QString& text, const QColor& color, int fontSize)
        : m_pos(pos), m_text(text), m_color(color), m_fontSize(fontSize) {}
    AnnotationType type() const override { return AnnotationType::Text; }
    void draw(QPainter& painter) override {
        painter.setPen(m_color);
        QFont f = painter.font();
        f.setPointSize(m_fontSize);
        f.setBold(true);
        painter.setFont(f);
        QFontMetrics fm(f);
        // m_pos 对应 InPlaceTextEditor (QLineEdit) 的左上角原点
        // 修正 baseline 垂直位移，确保回车提交后文字位置与输入框完全重合，绝不上浮跳动
        QPoint baselinePos(m_pos.x() + 7, m_pos.y() + fm.ascent() + 4);
        painter.drawText(baselinePos, m_text);
    }
private:
    QPoint m_pos;
    QString m_text;
    QColor m_color;
    int m_fontSize;
};

// 局部马赛克滤镜图元
class MosaicAnnotation : public AnnotationItem {
public:
    MosaicAnnotation(const QRect& rect, const QPixmap& baseSnapshot, qreal dpr = 1.0, int blockSize = 10)
        : m_rect(rect), m_blockSize(blockSize)
    {
        if (dpr <= 0.0) dpr = 1.0;
        // 映射为底层物理像素坐标
        QRect phys(
            static_cast<int>(std::round(rect.x() * dpr)),
            static_cast<int>(std::round(rect.y() * dpr)),
            static_cast<int>(std::round(rect.width() * dpr)),
            static_cast<int>(std::round(rect.height() * dpr))
        );

        QRect validPhys = phys.intersected(baseSnapshot.rect());
        if (!validPhys.isEmpty()) {
            QImage img = baseSnapshot.copy(validPhys).toImage();
            int physBlock = (std::max)(4, static_cast<int>(std::round(blockSize * dpr)));

            for (int y = 0; y < img.height(); y += physBlock) {
                for (int x = 0; x < img.width(); x += physBlock) {
                    int bw = (std::min)(physBlock, img.width() - x);
                    int bh = (std::min)(physBlock, img.height() - y);
                    QColor c = img.pixelColor(x + bw / 2, y + bh / 2);
                    for (int by = y; by < y + bh; ++by) {
                        for (int bx = x; bx < x + bw; ++bx) {
                            img.setPixelColor(bx, by, c);
                        }
                    }
                }
            }
            img.setDevicePixelRatio(dpr);
            m_mosaicPixmap = QPixmap::fromImage(img);
            m_mosaicPixmap.setDevicePixelRatio(dpr);
        }
    }

    AnnotationType type() const override { return AnnotationType::Mosaic; }

    void draw(QPainter& painter) override {
        if (!m_mosaicPixmap.isNull()) {
            painter.drawPixmap(m_rect, m_mosaicPixmap);
        }
    }

    QRect rect() const { return m_rect; }

private:
    QRect m_rect;
    int m_blockSize;
    QPixmap m_mosaicPixmap;
};

// 单字符就地编辑/替换图元
class SingleCharEditAnnotation : public AnnotationItem {
public:
    SingleCharEditAnnotation(const QRect& logicalBox,
                             const QPixmap& inpaintPatchPixmap,
                             const QString& replacementChar,
                             const QColor& textColor,
                             const QFont& font)
        : m_logicalBox(logicalBox)
        , m_inpaintPatchPixmap(inpaintPatchPixmap)
        , m_replacementChar(replacementChar)
        , m_textColor(textColor)
        , m_font(font)
    {}

    AnnotationType type() const override { return AnnotationType::SingleCharEdit; }

    void draw(QPainter& painter) override {
        // 1. 绘制微创背景修复底图 (无痕擦除原字笔画)
        if (!m_inpaintPatchPixmap.isNull()) {
            painter.drawPixmap(m_logicalBox, m_inpaintPatchPixmap);
        }

        // 2. 原位基线与墨迹几何中心同轴精准对齐渲染新单字符 (彻底消除向下偏移)
        if (!m_replacementChar.isEmpty()) {
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setRenderHint(QPainter::TextAntialiasing, true);
            painter.setPen(m_textColor);
            painter.setFont(m_font);

            QFontMetrics fm(m_font);
            QRect tight = fm.tightBoundingRect(m_replacementChar);

            // tight.top() 与 tight.bottom() 是墨迹顶底相对于文字基线 y=0 的精确坐标 (tight.top() 为负值)
            // 墨迹垂直几何中心相对于基线的相对偏移量:
            double inkCenterRelY = (tight.top() + tight.bottom()) / 2.0;

            // 目标垂直中心 = m_logicalBox 的精确垂直几何中心
            double targetCenterY = m_logicalBox.center().y();
            double baselineY = targetCenterY - inkCenterRelY;

            // 水平几何中心对齐
            double inkCenterRelX = (tight.left() + tight.right()) / 2.0;
            double targetCenterX = m_logicalBox.center().x();
            double baselineX = targetCenterX - inkCenterRelX;

            painter.drawText(QPointF(baselineX, baselineY), m_replacementChar);
            painter.restore();
        }
    }

    QRect logicalBox() const { return m_logicalBox; }
    QString replacementChar() const { return m_replacementChar; }
    QColor textColor() const { return m_textColor; }
    QFont font() const { return m_font; }

private:
    QRect m_logicalBox;
    QPixmap m_inpaintPatchPixmap;
    QString m_replacementChar;
    QColor m_textColor;
    QFont m_font;
};
