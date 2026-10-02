#include "font_attribute_estimator.h"
#include <vector>
#include <cmath>
#include <algorithm>

EstimatedFontAttributes FontAttributeEstimator::estimate(const QImage& image, const QRect& targetRect, const QColor& knownBgColor, qreal dpr)
{
    EstimatedFontAttributes attr;
    attr.bgColor = knownBgColor;

    if (image.isNull() || targetRect.isEmpty()) {
        return attr;
    }

    if (dpr <= 0.0) dpr = 1.0;

    QRect valid = targetRect.intersected(image.rect());
    if (valid.width() <= 0 || valid.height() <= 0) {
        return attr;
    }

    // 1. 根据包围盒高度计算逻辑字号
    double logicalHeight = valid.height() / dpr;
    // 汉字与西文字符通常占字盒高度的 72% ~ 82%
    attr.fontSize = (std::max)(9, static_cast<int>(std::round(logicalHeight * 0.78)));

    // 2. 统计与背景色存在明显色差的前景文字像素
    const int bgR = knownBgColor.red();
    const int bgG = knownBgColor.green();
    const int bgB = knownBgColor.blue();

    int sumR = 0, sumG = 0, sumB = 0;
    int fgCount = 0;
    double maxDistance = 0.0;
    QColor mostDistantColor = (knownBgColor.lightness() > 128) ? QColor(0, 0, 0) : QColor(255, 255, 255);

    // 采样步长 (大尺寸图片进行步进，小尺寸逐像素，确保在 1ms 内完成)
    int step = (valid.width() * valid.height() > 10000) ? 2 : 1;

    for (int y = valid.top(); y <= valid.bottom(); y += step) {
        for (int x = valid.left(); x <= valid.right(); x += step) {
            QColor c = image.pixelColor(x, y);
            int dr = c.red() - bgR;
            int dg = c.green() - bgG;
            int db = c.blue() - bgB;
            double dist = std::sqrt(dr * dr + dg * dg + db * db);

            // 与背景色彩差异大于阈值 (排除背景抗锯齿边缘过度像素)
            if (dist > 38.0) {
                sumR += c.red();
                sumG += c.green();
                sumB += c.blue();
                fgCount++;

                if (dist > maxDistance) {
                    maxDistance = dist;
                    mostDistantColor = c;
                }
            }
        }
    }

    // 3. 决定文字前景色
    if (fgCount > 0) {
        // 使用前景文字像素的均值作为字体前景色
        attr.textColor = QColor(sumR / fgCount, sumG / fgCount, sumB / fgCount);

        // 如果前景像素占比较高，推断为粗体
        double fgDensity = static_cast<double>(fgCount * step * step) / (valid.width() * valid.height());
        if (fgDensity > 0.35) {
            attr.fontWeight = 700; // 粗体
        } else {
            attr.fontWeight = 400; // 正常
        }
    } else {
        // 未检测到前景像素时，选择与背景反差强烈的对比色
        attr.textColor = (knownBgColor.lightness() > 128) ? QColor(20, 20, 20) : QColor(245, 245, 245);
        attr.fontWeight = 400;
    }

    return attr;
}
