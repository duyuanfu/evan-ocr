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

    // 3. 决定文字前景色与字重
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

    // 4. 方案 1：基于笔画几何与横竖粗细比 (Stroke Aspect Ratio) 的字体流派启发式推断
    // 统计水平穿越笔画厚度 (代表竖画厚度 Tv) 与垂直穿越笔画厚度 (代表横画厚度 Th)
    std::vector<int> vStrokeThicknesses; // 竖笔厚度采样
    std::vector<int> hStrokeThicknesses; // 横笔厚度采样

    // 水平扫描线截取竖向笔画宽度
    for (int y = valid.top(); y <= valid.bottom(); y += 2) {
        int runLen = 0;
        for (int x = valid.left(); x <= valid.right(); ++x) {
            QColor c = image.pixelColor(x, y);
            int dr = c.red() - bgR;
            int dg = c.green() - bgG;
            int db = c.blue() - bgB;
            double dist = std::sqrt(dr * dr + dg * dg + db * db);
            if (dist > 38.0) {
                runLen++;
            } else {
                if (runLen >= 1 && runLen <= valid.width() * 0.4) {
                    vStrokeThicknesses.push_back(runLen);
                }
                runLen = 0;
            }
        }
        if (runLen >= 1 && runLen <= valid.width() * 0.4) {
            vStrokeThicknesses.push_back(runLen);
        }
    }

    // 垂直扫描线截取横向笔画高度
    for (int x = valid.left(); x <= valid.right(); x += 2) {
        int runLen = 0;
        for (int y = valid.top(); y <= valid.bottom(); ++y) {
            QColor c = image.pixelColor(x, y);
            int dr = c.red() - bgR;
            int dg = c.green() - bgG;
            int db = c.blue() - bgB;
            double dist = std::sqrt(dr * dr + dg * dg + db * db);
            if (dist > 38.0) {
                runLen++;
            } else {
                if (runLen >= 1 && runLen <= valid.height() * 0.4) {
                    hStrokeThicknesses.push_back(runLen);
                }
                runLen = 0;
            }
        }
        if (runLen >= 1 && runLen <= valid.height() * 0.4) {
            hStrokeThicknesses.push_back(runLen);
        }
    }

    // 计算横竖笔画厚度中位数
    double medianTv = 2.0;
    double medianTh = 2.0;

    if (!vStrokeThicknesses.empty()) {
        std::sort(vStrokeThicknesses.begin(), vStrokeThicknesses.end());
        medianTv = vStrokeThicknesses[vStrokeThicknesses.size() / 2];
    }
    if (!hStrokeThicknesses.empty()) {
        std::sort(hStrokeThicknesses.begin(), hStrokeThicknesses.end());
        medianTh = hStrokeThicknesses[hStrokeThicknesses.size() / 2];
    }

    // 计算竖横比 (Tv / Th)
    double strokeRatio = (medianTh > 0.0) ? (medianTv / medianTh) : 1.0;

    // 判定流派规则：
    // - 宋体 (SimSun)：典型特征横细竖粗，竖横比 >= 1.85
    // - 等宽代码体 (Consolas)：字符扁窄，常用于纯代码界面
    // - 楷体 (KaiTi)：笔画有明显倾斜与粗细连续过渡
    // - 黑体 (Microsoft YaHei)：笔画横竖基本等宽 (0.75 <= strokeRatio <= 1.45)
    if (strokeRatio >= 1.80) {
        attr.fontFamily = "SimSun"; // 宋体
    } else if (strokeRatio >= 1.45 && strokeRatio < 1.80) {
        attr.fontFamily = "KaiTi";  // 楷体/仿宋
    } else {
        attr.fontFamily = "Microsoft YaHei"; // 黑体
    }

    return attr;
}
