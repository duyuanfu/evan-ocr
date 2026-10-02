#pragma once

#include <QImage>
#include <QRect>
#include <QColor>

struct InpaintResult {
    bool success = false;
    QImage inpaintedPatch;  // 修复后的局部补丁位图 (尺寸与 paddedRect 一致)
    QRect paddedRect;       // 实际修复应用的物理像素矩形
    QColor estimatedBgColor; // 估算出的主背景色
};

class ImageInpainter {
public:
    // 基于边缘采样与双线性渐变插值，将 image 中 targetRect 内部的文字擦除抹平
    // targetRect: 物理像素坐标
    // padding: 向外微扩的边缘像素宽度 (默认 2px)
    static InpaintResult inpaintTextRegion(const QImage& image, const QRect& targetRect, int padding = 2);
};
