#include "char_font_analyzer.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace {

static int colorDist(const QColor& c1, const QColor& c2)
{
    int dr = c1.red() - c2.red();
    int dg = c1.green() - c2.green();
    int db = c1.blue() - c2.blue();
    return static_cast<int>(std::sqrt(dr * dr + dg * dg + db * db));
}

} // namespace

CharFontAttributes CharFontAnalyzer::analyze(const QImage& fullSnapshot, const QRect& charPhysicalBox, const QString& charText, qreal dpr)
{
    CharFontAttributes attrs;
    if (fullSnapshot.isNull() || charPhysicalBox.isEmpty()) {
        return attrs;
    }

    if (dpr <= 0.0) dpr = 1.0;

    QRect validBox = charPhysicalBox.intersected(fullSnapshot.rect());
    if (validBox.width() < 2 || validBox.height() < 2) {
        return attrs;
    }

    QImage crop = fullSnapshot.copy(validBox);
    int w = crop.width();
    int h = crop.height();

    // 1. 采样四周边框推测背景底色
    long bgR = 0, bgG = 0, bgB = 0, bgCount = 0;
    for (int x = 0; x < w; ++x) {
        QColor c1 = crop.pixelColor(x, 0);
        QColor c2 = crop.pixelColor(x, h - 1);
        bgR += c1.red() + c2.red();
        bgG += c1.green() + c2.green();
        bgB += c1.blue() + c2.blue();
        bgCount += 2;
    }
    for (int y = 1; y < h - 1; ++y) {
        QColor c1 = crop.pixelColor(0, y);
        QColor c2 = crop.pixelColor(w - 1, y);
        bgR += c1.red() + c2.red();
        bgG += c1.green() + c2.green();
        bgB += c1.blue() + c2.blue();
        bgCount += 2;
    }

    if (bgCount > 0) {
        attrs.bgColor = QColor(static_cast<int>(bgR / bgCount),
                               static_cast<int>(bgG / bgCount),
                               static_cast<int>(bgB / bgCount));
    }

    // 2. 收集前景油墨像素并提取核心笔画纯度色彩 (彻底消除边缘抗锯齿混合导致的颜色变暗/变淡)
    int inkCount = 0;
    int topInk = h - 1, btmInk = 0;
    int maxInkDist = 0;

    struct InkPixel {
        QColor color;
        int dist;
    };
    std::vector<InkPixel> inkPixels;

    std::vector<int> hStrokeRuns;
    std::vector<int> vStrokeRuns;

    // 水平扫描笔画宽度与墨迹像素
    for (int y = 0; y < h; ++y) {
        int run = 0;
        for (int x = 0; x < w; ++x) {
            QColor pix = crop.pixelColor(x, y);
            int d = colorDist(pix, attrs.bgColor);
            if (d > 26) {
                inkPixels.push_back({pix, d});
                if (d > maxInkDist) maxInkDist = d;
                inkCount++;

                if (y < topInk) topInk = y;
                if (y > btmInk) btmInk = y;
                run++;
            } else {
                if (run > 0) {
                    hStrokeRuns.push_back(run);
                    run = 0;
                }
            }
        }
        if (run > 0) hStrokeRuns.push_back(run);
    }

    // 垂直扫描笔画宽度
    for (int x = 0; x < w; ++x) {
        int run = 0;
        for (int y = 0; y < h; ++y) {
            QColor pix = crop.pixelColor(x, y);
            if (colorDist(pix, attrs.bgColor) > 26) {
                run++;
            } else {
                if (run > 0) {
                    vStrokeRuns.push_back(run);
                    run = 0;
                }
            }
        }
        if (run > 0) vStrokeRuns.push_back(run);
    }

    // 提取纯净核心笔画颜色：选取距离背景最远 (不透明度最高、未受抗锯齿边缘羽化混合污染) 的核心像素中值
    if (!inkPixels.empty() && maxInkDist > 20) {
        std::vector<int> coreR, coreG, coreB;
        int coreDistThreshold = static_cast<int>(maxInkDist * 0.78);

        for (const auto& ip : inkPixels) {
            if (ip.dist >= coreDistThreshold) {
                coreR.push_back(ip.color.red());
                coreG.push_back(ip.color.green());
                coreB.push_back(ip.color.blue());
            }
        }

        if (!coreR.empty()) {
            std::sort(coreR.begin(), coreR.end());
            std::sort(coreG.begin(), coreG.end());
            std::sort(coreB.begin(), coreB.end());
            size_t mid = coreR.size() / 2;
            attrs.fgColor = QColor(coreR[mid], coreG[mid], coreB[mid]);
        } else {
            std::vector<int> allR, allG, allB;
            for (const auto& ip : inkPixels) {
                allR.push_back(ip.color.red());
                allG.push_back(ip.color.green());
                allB.push_back(ip.color.blue());
            }
            std::sort(allR.begin(), allR.end());
            std::sort(allG.begin(), allG.end());
            std::sort(allB.begin(), allB.end());
            size_t mid = allR.size() / 2;
            attrs.fgColor = QColor(allR[mid], allG[mid], allB[mid]);
        }
    } else {
        // 亮度自适应回退
        int bgLum = qGray(attrs.bgColor.rgb());
        attrs.fgColor = (bgLum > 128) ? QColor(20, 20, 20) : QColor(240, 240, 240);
    }

    // 3. 估算字号 (pt)
    int inkHeight = (btmInk >= topInk) ? (btmInk - topInk + 1) : h;
    double logicalHeight = inkHeight / dpr;
    // 汉字墨迹高度通常占字号 em-box 的 75%~85%，西文字符占 70%~80%
    int estimatedPt = static_cast<int>(std::round(logicalHeight * 0.82));
    attrs.fontSizePt = std::clamp(estimatedPt, 8, 72);

    // 4. 估算粗细 (Bold)
    double inkAreaRatio = static_cast<double>(inkCount) / (w * h);
    if (inkAreaRatio > 0.38) {
        attrs.isBold = true;
    }

    // 5. 估算字体流派 (黑体 vs 宋体 vs 等宽)
    if (!charText.isEmpty()) {
        QChar ch = charText.at(0);
        if (ch.isDigit() || (ch.toLatin1() >= 'a' && ch.toLatin1() <= 'z') || (ch.toLatin1() >= 'A' && ch.toLatin1() <= 'Z')) {
            attrs.fontFamily = "Consolas";
        } else {
            // 中文字体横竖比分析
            double avgH = 2.0;
            double avgV = 2.0;
            if (!hStrokeRuns.empty()) {
                double s = 0; for (int r : hStrokeRuns) s += r;
                avgH = s / hStrokeRuns.size();
            }
            if (!vStrokeRuns.empty()) {
                double s = 0; for (int r : vStrokeRuns) s += r;
                avgV = s / vStrokeRuns.size();
            }

            // 宋体特征：竖横比显著大于 1.65 (横细竖粗)
            if (avgH > 0 && (avgV / avgH) >= 1.65) {
                attrs.fontFamily = "SimSun";
            } else {
                attrs.fontFamily = "Microsoft YaHei";
            }
        }
    }

    return attrs;
}
