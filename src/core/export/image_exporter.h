#pragma once

#include <QImage>
#include <QString>

class ImageExporter
{
public:
    // 统一图像保存入口 (支持 PNG, GIF, WebP, JPG, BMP 等全格式导出)
    static bool saveImage(const QImage& image, const QString& filePath);

    // 标准 GIF89a 独立编码器 (纯原生实现，解决 Qt6 默认不支持写 GIF 的缺陷)
    static bool saveAsGif(const QImage& image, const QString& filePath);

    // 标准文件保存过滤器字符串
    static QString getSaveFileFilter();
};
