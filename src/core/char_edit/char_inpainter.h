#pragma once

#include <QImage>
#include <QRect>
#include <QColor>

struct CharInpaintResult {
    bool success = false;
    QRect patchRect;        // 全屏物理像素修补区域包围盒
    QImage patchImage;      // 修补完成的无痕背景局部底图
    QColor estimatedBgColor;// 估算的背景主色调
};

class CharInpainter {
public:
    // 对单字符区域执行微创背景感知修复，严格杜绝破坏邻近字符笔画
    static CharInpaintResult inpaintChar(const QImage& fullSnapshot, const QRect& charPhysicalBox, int padding = 2);

    // 将修补块直接覆盖到目标图像对应位置
    static bool applyPatch(QImage& targetImage, const CharInpaintResult& patch);
};
