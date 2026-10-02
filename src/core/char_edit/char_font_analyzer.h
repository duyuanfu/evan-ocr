#pragma once

#include <QImage>
#include <QRect>
#include <QColor>
#include <QString>

struct CharFontAttributes {
    QColor fgColor = QColor(20, 20, 20);       // 油墨文字前景色
    QColor bgColor = QColor(255, 255, 255);   // 背景底色
    int fontSizePt = 14;                       // 字号 (pt)
    QString fontFamily = "Microsoft YaHei";    // 字体流派 ("Microsoft YaHei", "SimSun", "KaiTi", "Consolas")
    bool isBold = false;                       // 是否粗体
};

class CharFontAnalyzer {
public:
    // 从单字符图像切片中逆向估算字体前景色、背景底色、字号与字体流派
    static CharFontAttributes analyze(const QImage& fullSnapshot, const QRect& charPhysicalBox, const QString& charText, qreal dpr = 1.0);
};
