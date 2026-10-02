#pragma once

#include <QString>
#include <QRect>
#include <QList>
#include <QColor>
#include <QImage>
#include "../ocr/ocr_types.h"

// 单字符单元实体
struct SingleCharUnit {
    QString character;          // 目标字符
    int charIndexInLine = 0;    // 所在行中的索引位置
    QRect physicalBox;          // 全屏全景物理像素包围盒
    QRect logicalBox;           // 全屏全景逻辑像素包围盒 (用于鼠标命中检测与UI渲染)
    QColor estimatedFgColor = QColor(20, 20, 20);      // 预估字体颜色
    QColor estimatedBgColor = QColor(255, 255, 255);  // 预估背景底色
    int estimatedFontSize = 14;                        // 预估字号 (pt)
    QString estimatedFontFamily = "Microsoft YaHei";   // 预估字体流派
    bool isBold = false;                               // 是否加粗
};

class CharSegmentation {
public:
    // 将一行 OCR 识别文本精细切分为单字符列表
    static QList<SingleCharUnit> segmentLine(const QImage& fullSnapshot, const OcrLine& line, qreal dpr = 1.0);

    // 将整个 OCR 结果中的全部行切分为字符列表
    static QList<SingleCharUnit> segmentAllLines(const QImage& fullSnapshot, const QList<OcrLine>& lines, qreal dpr = 1.0);

    // 根据逻辑鼠标指针位置快速检索命中的单字符索引 (未命中返回 -1)
    static int findCharUnitAt(const QList<SingleCharUnit>& units, const QPoint& logicalPos, int tolerance = 4);
};
