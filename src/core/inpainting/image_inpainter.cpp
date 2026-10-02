#include "image_inpainter.h"
#include <vector>
#include <cmath>
#include <algorithm>

InpaintResult ImageInpainter::inpaintTextRegion(const QImage& image, const QRect& targetRect, int padding)
{
    InpaintResult result;
    if (image.isNull() || targetRect.isEmpty()) {
        return result;
    }

    // 1. 将待修复矩形向外扩张 padding 像素，确保完全包覆抗锯齿羽化边缘
    QRect padded = targetRect.adjusted(-padding, -padding, padding, padding);
    padded = padded.intersected(image.rect());

    if (padded.width() <= 0 || padded.height() <= 0) {
        return result;
    }

    result.paddedRect = padded;
    const int w = padded.width();
    const int h = padded.height();

    // 2. 采集四边缘外侧的背景像素色
    // 上边沿外侧 (y = top - 1 或 top)
    std::vector<QColor> topBorder(w);
    int sampleYTop = (std::max)(0, padded.top() - 1);
    for (int x = 0; x < w; ++x) {
        topBorder[x] = image.pixelColor(padded.left() + x, sampleYTop);
    }

    // 下边沿外侧 (y = bottom + 1 或 bottom)
    std::vector<QColor> bottomBorder(w);
    int sampleYBottom = (std::min)(image.height() - 1, padded.bottom() + 1);
    for (int x = 0; x < w; ++x) {
        bottomBorder[x] = image.pixelColor(padded.left() + x, sampleYBottom);
    }

    // 左边沿外侧 (x = left - 1 或 left)
    std::vector<QColor> leftBorder(h);
    int sampleXLeft = (std::max)(0, padded.left() - 1);
    for (int y = 0; y < h; ++y) {
        leftBorder[y] = image.pixelColor(sampleXLeft, padded.top() + y);
    }

    // 右边沿外侧 (x = right + 1 或 right)
    std::vector<QColor> rightBorder(h);
    int sampleXRight = (std::min)(image.width() - 1, padded.right() + 1);
    for (int y = 0; y < h; ++y) {
        rightBorder[y] = image.pixelColor(sampleXRight, padded.top() + y);
    }

    // 3. 计算四周边界的平均背景色
    int sumR = 0, sumG = 0, sumB = 0, totalSamples = 0;
    for (const auto& c : topBorder)    { sumR += c.red(); sumG += c.green(); sumB += c.blue(); totalSamples++; }
    for (const auto& c : bottomBorder) { sumR += c.red(); sumG += c.green(); sumB += c.blue(); totalSamples++; }
    for (const auto& c : leftBorder)   { sumR += c.red(); sumG += c.green(); sumB += c.blue(); totalSamples++; }
    for (const auto& c : rightBorder)  { sumR += c.red(); sumG += c.green(); sumB += c.blue(); totalSamples++; }

    result.estimatedBgColor = (totalSamples > 0)
        ? QColor(sumR / totalSamples, sumG / totalSamples, sumB / totalSamples)
        : QColor(255, 255, 255);

    // 4. 双向线性插值生成平滑渐变修补图像
    QImage patch(w, h, QImage::Format_ARGB32_Premultiplied);

    for (int y = 0; y < h; ++y) {
        double v = (h > 1) ? (static_cast<double>(y) / (h - 1)) : 0.5;
        const QColor& cLeft = leftBorder[y];
        const QColor& cRight = rightBorder[y];

        for (int x = 0; x < w; ++x) {
            double u = (w > 1) ? (static_cast<double>(x) / (w - 1)) : 0.5;
            const QColor& cTop = topBorder[x];
            const QColor& cBottom = bottomBorder[x];

            // 水平渐变分量
            double rH = (1.0 - u) * cLeft.red() + u * cRight.red();
            double gH = (1.0 - u) * cLeft.green() + u * cRight.green();
            double bH = (1.0 - u) * cLeft.blue() + u * cRight.blue();

            // 垂直渐变分量
            double rV = (1.0 - v) * cTop.red() + v * cBottom.red();
            double gV = (1.0 - v) * cTop.green() + v * cBottom.green();
            double bV = (1.0 - v) * cTop.blue() + v * cBottom.blue();

            // 双向加权平均
            int finalR = static_cast<int>(std::round((rH + rV) * 0.5));
            int finalG = static_cast<int>(std::round((gH + gV) * 0.5));
            int finalB = static_cast<int>(std::round((bH + bV) * 0.5));

            finalR = (std::clamp)(finalR, 0, 255);
            finalG = (std::clamp)(finalG, 0, 255);
            finalB = (std::clamp)(finalB, 0, 255);

            patch.setPixelColor(x, y, QColor(finalR, finalG, finalB));
        }
    }

    result.inpaintedPatch = patch;
    result.success = true;
    return result;
}
