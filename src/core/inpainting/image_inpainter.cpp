#include "image_inpainter.h"
#include <vector>
#include <cmath>
#include <map>
#include <algorithm>

namespace {
    // 将 QColor 降采样量化为 15-bit 色彩编码，用于直方图众数统计
    uint16_t quantizeColor(const QColor& c) {
        uint16_t r = (c.red() >> 3) & 0x1F;
        uint16_t g = (c.green() >> 3) & 0x1F;
        uint16_t b = (c.blue() >> 3) & 0x1F;
        return (r << 10) | (g << 5) | b;
    }

    double colorDistance(const QColor& c1, const QColor& c2) {
        int dr = c1.red() - c2.red();
        int dg = c1.green() - c2.green();
        int db = c1.blue() - c2.blue();
        return std::sqrt(dr * dr + dg * dg + db * db);
    }
}

InpaintResult ImageInpainter::inpaintTextRegion(const QImage& image, const QRect& targetRect, int padding)
{
    InpaintResult result;
    if (image.isNull() || targetRect.isEmpty()) {
        return result;
    }

    // 1. 待修复矩形向外微扩 padding 像素
    QRect padded = targetRect.adjusted(-padding, -padding, padding, padding);
    padded = padded.intersected(image.rect());

    if (padded.width() <= 0 || padded.height() <= 0) {
        return result;
    }

    result.paddedRect = padded;
    const int w = padded.width();
    const int h = padded.height();

    // 2. 环绕采样四边外侧 1~3 像素带的所有候选背景像素
    std::vector<QColor> allSamples;
    allSamples.reserve(2 * (w + h) * 3);

    for (int offset = 1; offset <= 3; ++offset) {
        // 上外边
        int yTop = (std::max)(0, padded.top() - offset);
        for (int x = padded.left(); x <= padded.right(); ++x) {
            allSamples.push_back(image.pixelColor(x, yTop));
        }
        // 下外边
        int yBottom = (std::min)(image.height() - 1, padded.bottom() + offset);
        for (int x = padded.left(); x <= padded.right(); ++x) {
            allSamples.push_back(image.pixelColor(x, yBottom));
        }
        // 左外边
        int xLeft = (std::max)(0, padded.left() - offset);
        for (int y = padded.top(); y <= padded.bottom(); ++y) {
            allSamples.push_back(image.pixelColor(xLeft, y));
        }
        // 右外边
        int xRight = (std::min)(image.width() - 1, padded.right() + offset);
        for (int y = padded.top(); y <= padded.bottom(); ++y) {
            allSamples.push_back(image.pixelColor(xRight, y));
        }
    }

    if (allSamples.empty()) {
        result.estimatedBgColor = QColor(255, 255, 255);
        result.inpaintedPatch = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
        result.inpaintedPatch.fill(result.estimatedBgColor);
        result.success = true;
        return result;
    }

    // 3. 直方图众数统计：找出压倒性优势的真实纯净背景色，彻底过滤掉黄色文字残墨噪点
    std::map<uint16_t, int> hist;
    for (const auto& c : allSamples) {
        hist[quantizeColor(c)]++;
    }

    uint16_t dominantKey = hist.begin()->first;
    int maxCount = 0;
    for (const auto& kv : hist) {
        if (kv.second > maxCount) {
            maxCount = kv.second;
            dominantKey = kv.first;
        }
    }

    // 聚合属于该优势色彩箱的所有纯净背景像素
    int sumR = 0, sumG = 0, sumB = 0, validBgCount = 0;
    for (const auto& c : allSamples) {
        if (quantizeColor(c) == dominantKey) {
            sumR += c.red();
            sumG += c.green();
            sumB += c.blue();
            validBgCount++;
        }
    }

    QColor dominantBg = (validBgCount > 0)
        ? QColor(sumR / validBgCount, sumG / validBgCount, sumB / validBgCount)
        : allSamples.front();

    result.estimatedBgColor = dominantBg;

    // 4. 采集经过纯净背景色过滤的四周边沿像素
    auto getCleanBorderPixel = [&](int x, int y) -> QColor {
        QColor pixel = image.pixelColor(x, y);
        // 如果该像素与主要背景色差异过大 (说明踩到了旁边字的黄色笔画噪点)，强制用纯净背景色替代！
        if (colorDistance(pixel, dominantBg) > 30.0) {
            return dominantBg;
        }
        return pixel;
    };

    std::vector<QColor> topBorder(w);
    int yT = (std::max)(0, padded.top() - 1);
    for (int x = 0; x < w; ++x) {
        topBorder[x] = getCleanBorderPixel(padded.left() + x, yT);
    }

    std::vector<QColor> bottomBorder(w);
    int yB = (std::min)(image.height() - 1, padded.bottom() + 1);
    for (int x = 0; x < w; ++x) {
        bottomBorder[x] = getCleanBorderPixel(padded.left() + x, yB);
    }

    std::vector<QColor> leftBorder(h);
    int xL = (std::max)(0, padded.left() - 1);
    for (int y = 0; y < h; ++y) {
        leftBorder[y] = getCleanBorderPixel(xL, padded.top() + y);
    }

    std::vector<QColor> rightBorder(h);
    int xR = (std::min)(image.width() - 1, padded.right() + 1);
    for (int y = 0; y < h; ++y) {
        rightBorder[y] = getCleanBorderPixel(xR, padded.top() + y);
    }

    // 5. 纯净修复底图生成 (绝大多数场景为平坦单色底，直接用纯净背景色填充，杜绝一切脏黄残影；若四周存在平滑渐变则进行双线性插值)
    QImage patch(w, h, QImage::Format_ARGB32_Premultiplied);

    bool isFlatBackground = true;
    for (const auto& c : topBorder) {
        if (colorDistance(c, dominantBg) > 12.0) { isFlatBackground = false; break; }
    }
    if (isFlatBackground) {
        for (const auto& c : bottomBorder) {
            if (colorDistance(c, dominantBg) > 12.0) { isFlatBackground = false; break; }
        }
    }

    if (isFlatBackground) {
        // 平坦纯色背景：直接填充纯净背景色，100% 零虚影零瑕疵！
        patch.fill(dominantBg);
    } else {
        // 微渐变背景：执行无噪点双线性插值
        for (int y = 0; y < h; ++y) {
            double v = (h > 1) ? (static_cast<double>(y) / (h - 1)) : 0.5;
            const QColor& cLeft = leftBorder[y];
            const QColor& cRight = rightBorder[y];

            for (int x = 0; x < w; ++x) {
                double u = (w > 1) ? (static_cast<double>(x) / (w - 1)) : 0.5;
                const QColor& cTop = topBorder[x];
                const QColor& cBottom = bottomBorder[x];

                double rH = (1.0 - u) * cLeft.red() + u * cRight.red();
                double gH = (1.0 - u) * cLeft.green() + u * cRight.green();
                double bH = (1.0 - u) * cLeft.blue() + u * cRight.blue();

                double rV = (1.0 - v) * cTop.red() + v * cBottom.red();
                double gV = (1.0 - v) * cTop.green() + v * cBottom.green();
                double bV = (1.0 - v) * cTop.blue() + v * cBottom.blue();

                int finalR = (std::clamp)(static_cast<int>(std::round((rH + rV) * 0.5)), 0, 255);
                int finalG = (std::clamp)(static_cast<int>(std::round((gH + gV) * 0.5)), 0, 255);
                int finalB = (std::clamp)(static_cast<int>(std::round((bH + bV) * 0.5)), 0, 255);

                patch.setPixelColor(x, y, QColor(finalR, finalG, finalB));
            }
        }
    }

    result.inpaintedPatch = patch;
    result.success = true;
    return result;
}
