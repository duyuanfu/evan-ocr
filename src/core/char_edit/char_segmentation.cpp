#include "char_segmentation.h"
#include <cmath>
#include <algorithm>
#include <vector>

namespace {

// 判断字符是否属于东亚全宽字符 (中日韩汉字、全角标点)
static bool isFullWidthChar(const QChar& ch)
{
    ushort u = ch.unicode();
    return (u >= 0x2E80 && u <= 0x9FFF) ||
           (u >= 0xF900 && u <= 0xFAFF) ||
           (u >= 0xFF01 && u <= 0xFF60);
}

// 计算两颜色在 RGB 色彩空间的几何欧式距离
static int colorDistance(const QColor& c1, const QColor& c2)
{
    int dr = c1.red() - c2.red();
    int dg = c1.green() - c2.green();
    int db = c1.blue() - c2.blue();
    return static_cast<int>(std::sqrt(dr * dr + dg * dg + db * db));
}

// 估算切片底色：采样顶部与底部两边缘像素
static QColor estimateCropBgColor(const QImage& img)
{
    if (img.isNull() || img.width() <= 0 || img.height() <= 0) {
        return QColor(255, 255, 255);
    }

    int rSum = 0, gSum = 0, bSum = 0, count = 0;
    int w = img.width();
    int h = img.height();

    for (int x = 0; x < w; ++x) {
        QColor top = img.pixelColor(x, 0);
        QColor btm = img.pixelColor(x, h - 1);
        rSum += top.red() + btm.red();
        gSum += top.green() + btm.green();
        bSum += top.blue() + btm.blue();
        count += 2;
    }

    if (count == 0) return QColor(255, 255, 255);
    return QColor(rSum / count, gSum / count, bSum / count);
}

// 提取局部字框内的核心墨迹纯正原色 (避免边缘抗锯齿混合像素导致估算偏暗/偏淡)
static QColor extractCoreInkColor(const QImage& crop, int x1, int x2, int y1, int y2, const QColor& bgColor)
{
    int maxDist = 0;
    struct InkPixel {
        QColor col;
        int dist;
    };
    std::vector<InkPixel> inkPixels;

    int cx1 = (std::max)(0, x1);
    int cx2 = (std::min)(crop.width() - 1, x2);
    int cy1 = (std::max)(0, y1);
    int cy2 = (std::min)(crop.height() - 1, y2);

    for (int y = cy1; y <= cy2; ++y) {
        for (int x = cx1; x <= cx2; ++x) {
            QColor pix = crop.pixelColor(x, y);
            int d = colorDistance(pix, bgColor);
            if (d > 26) {
                inkPixels.push_back({pix, d});
                if (d > maxDist) maxDist = d;
            }
        }
    }

    if (inkPixels.empty() || maxDist < 20) {
        int bgLum = qGray(bgColor.rgb());
        return (bgLum > 128) ? QColor(20, 20, 20) : QColor(240, 240, 240);
    }

    std::vector<int> coreR, coreG, coreB;
    int coreThresh = static_cast<int>(maxDist * 0.78);
    for (const auto& ip : inkPixels) {
        if (ip.dist >= coreThresh) {
            coreR.push_back(ip.col.red());
            coreG.push_back(ip.col.green());
            coreB.push_back(ip.col.blue());
        }
    }

    if (coreR.empty()) {
        for (const auto& ip : inkPixels) {
            coreR.push_back(ip.col.red());
            coreG.push_back(ip.col.green());
            coreB.push_back(ip.col.blue());
        }
    }

    std::sort(coreR.begin(), coreR.end());
    std::sort(coreG.begin(), coreG.end());
    std::sort(coreB.begin(), coreB.end());
    size_t mid = coreR.size() / 2;
    return QColor(coreR[mid], coreG[mid], coreB[mid]);
}

} // namespace

QList<SingleCharUnit> CharSegmentation::segmentLine(const QImage& fullSnapshot, const OcrLine& line, qreal dpr)
{
    QList<SingleCharUnit> result;
    if (fullSnapshot.isNull() || line.text.trimmed().isEmpty() || line.boundingBox.isEmpty()) {
        return result;
    }

    if (dpr <= 0.0) dpr = 1.0;

    // 优先按 word 进行局部化切分，若 word 为空则整行切分
    QList<OcrWord> wordsToProcess = line.words;
    if (wordsToProcess.isEmpty()) {
        OcrWord fallbackWord;
        fallbackWord.text = line.text;
        fallbackWord.boundingBox = line.boundingBox;
        fallbackWord.logicalBox = line.logicalBox;
        wordsToProcess.append(fallbackWord);
    }

    int globalCharIndex = 0;

    for (const auto& word : wordsToProcess) {
        QString text = word.text;
        if (text.trimmed().isEmpty()) continue;

        QRect wordBox = word.boundingBox.intersected(fullSnapshot.rect());
        if (wordBox.width() < 2 || wordBox.height() < 2) continue;

        QImage crop = fullSnapshot.copy(wordBox);
        int w = crop.width();
        int h = crop.height();
        QColor bgColor = estimateCropBgColor(crop);

        // 1. 提取有效非空字符
        QList<QChar> chars;
        for (int i = 0; i < text.size(); ++i) {
            QChar c = text.at(i);
            if (!c.isSpace()) {
                chars.append(c);
            }
        }
        int numChars = chars.size();
        if (numChars == 0) continue;

        // 2. 统计每列与每行的有效油墨分布
        std::vector<int> colInk(w, 0);
        std::vector<int> rowInk(h, 0);
        int inkMinX = w, inkMaxX = -1;
        int inkMinY = h, inkMaxY = -1;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                QColor pix = crop.pixelColor(x, y);
                if (colorDistance(pix, bgColor) > 26) {
                    colInk[x]++;
                    rowInk[y]++;
                    if (x < inkMinX) inkMinX = x;
                    if (x > inkMaxX) inkMaxX = x;
                    if (y < inkMinY) inkMinY = y;
                    if (y > inkMaxY) inkMaxY = y;
                }
            }
        }

        // 若未探出有效油墨，回退到词框均匀切分
        if (inkMinX > inkMaxX || inkMinY > inkMaxY) {
            inkMinX = 0;
            inkMaxX = w - 1;
            inkMinY = 0;
            inkMaxY = h - 1;
        }

        // 计算整行有效墨迹的统一基准上下高度 (保证同行所有字符基线与垂直中心 100% 同轴，绝不偏下或浮动)
        int lineTopY = (std::max)(0, inkMinY - 1);
        int lineH = (std::min)(h - lineTopY, (inkMaxY - inkMinY + 1) + 2);

        // 3. 单字符特化：直接使用真实墨迹包围盒，垂直方向采用整行统一基线高度
        if (numChars == 1) {
            int charX = (std::max)(0, inkMinX - 1);
            int charW = (std::min)(w - charX, (inkMaxX - inkMinX + 1) + 2);
            int charY = lineTopY;
            int charH = lineH;

            QRect charPhys(wordBox.x() + charX, wordBox.y() + charY, (std::max)(4, charW), (std::max)(4, charH));
            QRect charLog(
                static_cast<int>(std::round(charPhys.x() / dpr)),
                static_cast<int>(std::round(charPhys.y() / dpr)),
                static_cast<int>(std::round(charPhys.width() / dpr)),
                static_cast<int>(std::round(charPhys.height() / dpr))
            );

            SingleCharUnit unit;
            unit.character = QString(chars[0]);
            unit.charIndexInLine = globalCharIndex++;
            unit.physicalBox = charPhys;
            unit.logicalBox = charLog;
            unit.estimatedBgColor = bgColor;
            unit.estimatedFgColor = extractCoreInkColor(crop, charX, charX + charW - 1, charY, charY + charH - 1, bgColor);
            result.append(unit);
            continue;
        }

        // 4. 多字符高精切分：在真实的墨迹区间 [inkMinX, inkMaxX] 内进行比例种子 + 距离加权波谷探测
        int activeWidth = inkMaxX - inkMinX + 1;
        std::vector<double> weights(numChars, 1.0);
        double totalWeight = 0.0;
        for (int i = 0; i < numChars; ++i) {
            weights[i] = isFullWidthChar(chars[i]) ? 1.0 : 0.55;
            totalWeight += weights[i];
        }

        std::vector<int> splitPoints;
        splitPoints.push_back(inkMinX);

        double accum = 0.0;
        for (int i = 0; i < numChars - 1; ++i) {
            accum += weights[i];
            int idealX = inkMinX + static_cast<int>(std::round(activeWidth * (accum / totalWeight)));

            // 搜索窗口：在理想分割线左右探测
            int avgCharW = (std::max)(4, activeWidth / numChars);
            int window = (std::max)(3, static_cast<int>(std::round(avgCharW * 0.45)));

            int leftBound = (std::max)(splitPoints.back() + 2, idealX - window);
            int rightBound = (std::min)(inkMaxX - 2, idealX + window);

            int bestX = idealX;
            int bestCost = 9999999;

            // 核心代价函数：油墨密度 * 200 + 与理论理想分割线的距离
            // 彻底杜绝全0间隙优先选中最左侧导致右偏的严重 Bug
            for (int sx = leftBound; sx <= rightBound; ++sx) {
                int cost = colInk[sx] * 200 + std::abs(sx - idealX);
                if (cost < bestCost) {
                    bestCost = cost;
                    bestX = sx;
                }
            }

            splitPoints.push_back(bestX);
        }
        splitPoints.push_back(inkMaxX + 1);

        // 5. 对每个字符切片单独精确收敛到其内部的真实墨迹边界 (Shrink to Ink)
        for (int i = 0; i < numChars; ++i) {
            int sliceX1 = splitPoints[i];
            int sliceX2 = splitPoints[i + 1];
            if (sliceX2 <= sliceX1) sliceX2 = sliceX1 + 1;

            // 在该单字切片内精确寻找实际字迹的左右和上下边界
            int cMinX = sliceX2, cMaxX = sliceX1 - 1;
            int cMinY = h, cMaxY = -1;

            for (int y = inkMinY; y <= inkMaxY; ++y) {
                for (int x = sliceX1; x < sliceX2; ++x) {
                    if (colorDistance(crop.pixelColor(x, y), bgColor) > 26) {
                        if (x < cMinX) cMinX = x;
                        if (x > cMaxX) cMaxX = x;
                        if (y < cMinY) cMinY = y;
                        if (y > cMaxY) cMaxY = y;
                    }
                }
            }

            // 水平方向精准收敛到该字真实墨迹；垂直方向采用整行统一的基线高度，保证行内所有字 100% 同轴对齐
            int charFinalX, charFinalW;
            if (cMinX <= cMaxX) {
                charFinalX = (std::max)(0, cMinX - 1);
                charFinalW = (std::min)(w - charFinalX, (cMaxX - cMinX + 1) + 2);
            } else {
                charFinalX = sliceX1;
                charFinalW = sliceX2 - sliceX1;
            }
            int charFinalY = lineTopY;
            int charFinalH = lineH;

            QRect charPhys(wordBox.x() + charFinalX, wordBox.y() + charFinalY,
                           (std::max)(4, charFinalW), (std::max)(4, charFinalH));

            QRect charLog(
                static_cast<int>(std::round(charPhys.x() / dpr)),
                static_cast<int>(std::round(charPhys.y() / dpr)),
                static_cast<int>(std::round(charPhys.width() / dpr)),
                static_cast<int>(std::round(charPhys.height() / dpr))
            );

            SingleCharUnit unit;
            unit.character = QString(chars[i]);
            unit.charIndexInLine = globalCharIndex++;
            unit.physicalBox = charPhys;
            unit.logicalBox = charLog;
            unit.estimatedBgColor = bgColor;
            unit.estimatedFgColor = extractCoreInkColor(crop, charFinalX, charFinalX + charFinalW - 1, charFinalY, charFinalY + charFinalH - 1, bgColor);
            result.append(unit);
        }
    }

    return result;
}

QList<SingleCharUnit> CharSegmentation::segmentAllLines(const QImage& fullSnapshot, const QList<OcrLine>& lines, qreal dpr)
{
    QList<SingleCharUnit> allUnits;
    for (const auto& line : lines) {
        allUnits.append(segmentLine(fullSnapshot, line, dpr));
    }
    return allUnits;
}

int CharSegmentation::findCharUnitAt(const QList<SingleCharUnit>& units, const QPoint& logicalPos, int tolerance)
{
    int bestIndex = -1;
    int minDistanceSq = 9999999;

    for (int i = 0; i < units.size(); ++i) {
        // 第一优先级：精确包含点（外扩 tolerance）
        QRect expanded = units[i].logicalBox.adjusted(-tolerance, -tolerance, tolerance, tolerance);
        if (expanded.contains(logicalPos)) {
            QPoint center = units[i].logicalBox.center();
            int dx = center.x() - logicalPos.x();
            int dy = center.y() - logicalPos.y();
            int distSq = dx * dx + dy * dy;
            if (distSq < minDistanceSq) {
                minDistanceSq = distSq;
                bestIndex = i;
            }
        }
    }

    // 第二优先级：如果光标在文字行附近微小空隙（容差 8px），吸附到最近的字符中心
    if (bestIndex == -1) {
        int fallbackTol = tolerance + 6;
        for (int i = 0; i < units.size(); ++i) {
            QRect expFallback = units[i].logicalBox.adjusted(-fallbackTol, -fallbackTol, fallbackTol, fallbackTol);
            if (expFallback.contains(logicalPos)) {
                QPoint center = units[i].logicalBox.center();
                int dx = center.x() - logicalPos.x();
                int dy = center.y() - logicalPos.y();
                int distSq = dx * dx + dy * dy;
                if (distSq < minDistanceSq) {
                    minDistanceSq = distSq;
                    bestIndex = i;
                }
            }
        }
    }

    return bestIndex;
}
