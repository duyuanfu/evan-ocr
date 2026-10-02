#pragma once

#include <QImage>
#include <QRect>
#include <QColor>
#include <QString>

struct SnappedTextRegion {
    QRect snappedRect;          // 智能吸附与矫正后的真实文字物理包围盒
    QRect logicalRect;          // 对应的逻辑坐标矩形
    int trueLineHeight = 0;     // 真实文字行高 (物理像素)
    int estimatedCharCount = 0; // 估算的文字/数字字符个数
    QString detectedText;       // 自动识别出的原有文字内容
    QColor textColor;           // 真实的字体颜色
    QColor bgColor;             // 真实的背景底色
    int recommendedFontSize = 16;// 推荐对齐的逻辑字号 (px)
    int fontWeight = 400;       // 推荐字重
};

class SmartTextSnapper {
public:
    // 将用户粗糙框选的文字选区，智能吸附至文字的真实顶底基线、行高与左右边界
    // image: 全景快照图像 (物理像素)
    // userPhysRect: 用户框选的原始物理矩形
    // dpr: 设备像素比
    static SnappedTextRegion snapAndAnalyze(const QImage& image, const QRect& userPhysRect, qreal dpr = 1.0);
};
