#pragma once

#include <QImage>
#include <QRect>
#include <QColor>
#include <QString>

struct EstimatedFontAttributes {
    QColor textColor = QColor(0, 0, 0);     // 估算的文字前景色
    QColor bgColor = QColor(255, 255, 255); // 估算的文字背景底色
    int fontSize = 16;                      // 估算匹配的逻辑字号 (pt/px)
    int fontWeight = 400;                   // 估算的字重 (400 正常 / 700 粗体)
    QString fontFamily = "Microsoft YaHei"; // 推荐的系统字体族
};

class FontAttributeEstimator {
public:
    // 从文字包围盒内部像素逆向分析原文字排版属性
    // image: 包含该文字区域的图像 (物理像素)
    // targetRect: 文字包围盒 (物理像素)
    // dpr: 设备像素比
    static EstimatedFontAttributes estimate(const QImage& image, const QRect& targetRect, const QColor& knownBgColor, qreal dpr = 1.0);
};
