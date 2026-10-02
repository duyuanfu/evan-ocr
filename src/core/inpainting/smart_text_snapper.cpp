#include "smart_text_snapper.h"
#include "image_inpainter.h"
#include "font_attribute_estimator.h"
#include "../ocr/ocr_manager.h"
#include <vector>
#include <cmath>
#include <algorithm>

SnappedTextRegion SmartTextSnapper::snapAndAnalyze(const QImage& image, const QRect& userPhysRect, qreal dpr)
{
    SnappedTextRegion result;
    if (image.isNull() || userPhysRect.isEmpty()) {
        return result;
    }

    if (dpr <= 0.0) dpr = 1.0;

    QRect validUser = userPhysRect.intersected(image.rect());
    if (validUser.width() <= 0 || validUser.height() <= 0) {
        return result;
    }

    // 1. 初步采样背景色
    InpaintResult initialBg = ImageInpainter::inpaintTextRegion(image, validUser, 2);
    result.bgColor = initialBg.estimatedBgColor;

    // 2. 估计文字前景色
    EstimatedFontAttributes initialAttr = FontAttributeEstimator::estimate(
        image, validUser, result.bgColor, dpr);
    result.textColor = initialAttr.textColor;
    result.fontWeight = initialAttr.fontWeight;

    const int bgR = result.bgColor.red();
    const int bgG = result.bgColor.green();
    const int bgB = result.bgColor.blue();

    // 3. 构建自适应搜索窗口 (垂直方向上下大范围延展，彻底解决用户框选行高偏矮问题)
    int expandV = (std::max)(16, static_cast<int>(validUser.height() * 1.2));
    int expandH = (std::max)(12, static_cast<int>(validUser.width() * 0.3));

    int searchTop = (std::max)(0, validUser.top() - expandV);
    int searchBottom = (std::min)(image.height() - 1, validUser.bottom() + expandV);
    int searchLeft = (std::max)(0, validUser.left() - expandH);
    int searchRight = (std::min)(image.width() - 1, validUser.right() + expandH);

    int searchH = searchBottom - searchTop + 1;
    int searchW = searchRight - searchLeft + 1;

    if (searchH <= 0 || searchW <= 0) {
        result.snappedRect = validUser;
        result.logicalRect = QRect(
            static_cast<int>(std::round(validUser.x() / dpr)),
            static_cast<int>(std::round(validUser.y() / dpr)),
            static_cast<int>(std::round(validUser.width() / dpr)),
            static_cast<int>(std::round(validUser.height() / dpr))
        );
        result.recommendedFontSize = initialAttr.fontSize;
        return result;
    }

    // 4. 计算垂直投影直方图 (统计 searchTop ~ searchBottom 每一行的文字墨迹像素数)
    std::vector<int> vertProfile(searchH, 0);
    const double colorThreshold = 36.0;

    for (int y = searchTop; y <= searchBottom; ++y) {
        int count = 0;
        for (int x = searchLeft; x <= searchRight; ++x) {
            QColor c = image.pixelColor(x, y);
            int dr = c.red() - bgR;
            int dg = c.green() - bgG;
            int db = c.blue() - bgB;
            double dist = std::sqrt(dr * dr + dg * dg + db * db);
            if (dist > colorThreshold) {
                count++;
            }
        }
        vertProfile[y - searchTop] = count;
    }

    // 5. 从用户选择框的垂直中心位置向上下双向探寻文字真实上下基线
    int userCenterY = (validUser.top() + validUser.bottom()) / 2;
    int centerIdx = (std::clamp)(userCenterY - searchTop, 0, searchH - 1);

    // 计算基准噪声门限 (超过门限认定为文字行笔画)
    int maxInRow = 0;
    for (int c : vertProfile) {
        if (c > maxInRow) maxInRow = c;
    }
    int noiseThreshold = (std::max)(2, static_cast<int>(maxInRow * 0.08));

    // 向上寻找真实顶部基线
    int trueTop = userCenterY;
    int blankCountTop = 0;
    for (int i = centerIdx; i >= 0; --i) {
        if (vertProfile[i] <= noiseThreshold) {
            blankCountTop++;
            if (blankCountTop >= 3) { // 连续 3 行无笔画认定为到达行上方空白
                trueTop = searchTop + i + 2;
                break;
            }
        } else {
            blankCountTop = 0;
            trueTop = searchTop + i;
        }
    }

    // 向下寻找真实底部基线
    int trueBottom = userCenterY;
    int blankCountBottom = 0;
    for (int i = centerIdx; i < searchH; ++i) {
        if (vertProfile[i] <= noiseThreshold) {
            blankCountBottom++;
            if (blankCountBottom >= 3) { // 连续 3 行无笔画认定为到达行下方空白
                trueBottom = searchTop + i - 2;
                break;
            }
        } else {
            blankCountBottom = 0;
            trueBottom = searchTop + i;
        }
    }

    if (trueBottom <= trueTop) {
        trueTop = validUser.top();
        trueBottom = validUser.bottom();
    }

    // 6. 在确定的真实行高区间 [trueTop, trueBottom] 内，计算水平投影直方图
    std::vector<int> horizProfile(searchW, 0);

    for (int x = searchLeft; x <= searchRight; ++x) {
        int count = 0;
        for (int y = trueTop; y <= trueBottom; ++y) {
            QColor c = image.pixelColor(x, y);
            int dr = c.red() - bgR;
            int dg = c.green() - bgG;
            int db = c.blue() - bgB;
            double dist = std::sqrt(dr * dr + dg * dg + db * db);
            if (dist > colorThreshold) {
                count++;
            }
        }
        horizProfile[x - searchLeft] = count;
    }

    // 从用户框选的左端和右端分别向外微探字符整体边界
    int userLeftIdx = (std::clamp)(validUser.left() - searchLeft, 0, searchW - 1);
    int userRightIdx = (std::clamp)(validUser.right() - searchLeft, 0, searchW - 1);

    int trueLeft = validUser.left();
    for (int i = userLeftIdx; i >= 0; --i) {
        if (horizProfile[i] > 1) {
            trueLeft = searchLeft + i;
        } else if (horizProfile[i] == 0 && (searchLeft + i) < validUser.left() - 4) {
            break; // 遇到字符左侧纯空白停下
        }
    }

    int trueRight = validUser.right();
    for (int i = userRightIdx; i < searchW; ++i) {
        if (horizProfile[i] > 1) {
            trueRight = searchLeft + i;
        } else if (horizProfile[i] == 0 && (searchLeft + i) > validUser.right() + 4) {
            break; // 遇到字符右侧纯空白停下
        }
    }

    // 外加 1~2 像素保护边界，保证完整覆盖
    trueTop = (std::max)(0, trueTop - 2);
    trueBottom = (std::min)(image.height() - 1, trueBottom + 2);
    trueLeft = (std::max)(0, trueLeft - 2);
    trueRight = (std::min)(image.width() - 1, trueRight + 2);

    result.snappedRect = QRect(trueLeft, trueTop, trueRight - trueLeft + 1, trueBottom - trueTop + 1);
    result.trueLineHeight = result.snappedRect.height();

    // 7. 计算精准匹配对齐的字号 (基于真实行高换算，与原图其他文字 100% 同比例)
    double logicalHeight = result.trueLineHeight / dpr;
    result.recommendedFontSize = (std::max)(11, static_cast<int>(std::round(logicalHeight * 0.80)));

    result.logicalRect = QRect(
        static_cast<int>(std::round(result.snappedRect.x() / dpr)),
        static_cast<int>(std::round(result.snappedRect.y() / dpr)),
        static_cast<int>(std::round(result.snappedRect.width() / dpr)),
        static_cast<int>(std::round(result.snappedRect.height() / dpr))
    );

    // 8. 估算字符个数：根据常见汉字宽高比 1:1，西文 0.55:1
    double charWidth = logicalHeight * 0.95;
    if (charWidth > 0) {
        result.estimatedCharCount = (std::max)(1, static_cast<int>(std::round(result.logicalRect.width() / charWidth)));
    } else {
        result.estimatedCharCount = 1;
    }

    // 9. 智能 OCR 预识别：快速提取被选中的原文字
    QImage cropped = image.copy(result.snappedRect);
    if (!cropped.isNull()) {
        OcrResult ocrRes = OcrManager::instance().activeEngine()->recognize(cropped, dpr);
        if (ocrRes.success && !ocrRes.fullText.trimmed().isEmpty()) {
            result.detectedText = ocrRes.fullText.trimmed();
            if (!result.detectedText.isEmpty()) {
                result.estimatedCharCount = result.detectedText.length();
            }
        }
    }

    return result;
}
