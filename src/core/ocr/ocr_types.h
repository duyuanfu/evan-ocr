#pragma once

#include <QString>
#include <QRect>
#include <QList>

// 单个识别单词/文字单元
struct OcrWord {
    QString text;
    QRect boundingBox;      // 物理像素坐标包围盒
    QRect logicalBox;       // 逻辑坐标包围盒
    double confidence = 1.0;
};

// 识别文本行
struct OcrLine {
    QString text;
    QRect boundingBox;
    QRect logicalBox;
    QList<OcrWord> words;
};

// OCR 最终识别结果实体
struct OcrResult {
    bool success = false;
    QString errorMessage;
    QString fullText;       // 换行拼接的完整文本
    QList<OcrLine> lines;
    qreal dpr = 1.0;        // 图像生成时的设备像素比
};
