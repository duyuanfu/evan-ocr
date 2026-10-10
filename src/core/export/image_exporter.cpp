#include "image_exporter.h"
#include <QFile>
#include <QFileInfo>
#include <QColor>
#include <vector>
#include <algorithm>

QString ImageExporter::getSaveFileFilter()
{
    return "PNG 图像 (*.png);;GIF 图像 (*.gif);;WebP 图像 (*.webp);;JPEG 图像 (*.jpg *.jpeg);;位图 (*.bmp)";
}

bool ImageExporter::saveImage(const QImage& image, const QString& filePath)
{
    if (image.isNull() || filePath.isEmpty()) {
        return false;
    }

    // 若用户选择保存为 GIF 格式，调用独立标准 GIF89a 编码器 (解决 Qt6 默认不支持写 GIF 的局限)
    if (filePath.endsWith(".gif", Qt::CaseInsensitive)) {
        return saveAsGif(image, filePath);
    }

    // 其他格式调用 Qt 原生导出 (PNG, WebP, JPG, BMP 等)
    return image.save(filePath);
}

// ----------------------------------------------------------------------------
// 标准 GIF89a LZW 编码器实现 (纯 C++/Qt 原生实现，无外部动态库依赖)
// ----------------------------------------------------------------------------
bool ImageExporter::saveAsGif(const QImage& srcImage, const QString& filePath)
{
    if (srcImage.isNull() || srcImage.width() <= 0 || srcImage.height() <= 0) {
        return false;
    }

    // 1. 使用 Qt 高质量量化算法将 32 位图像转为 256 色索引图 (Format_Indexed8)
    QImage indexed = srcImage.convertToFormat(QImage::Format_Indexed8);
    const auto colorTable = indexed.colorTable();
    int colorCount = colorTable.size();
    if (colorCount > 256) colorCount = 256;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    int width = indexed.width();
    int height = indexed.height();

    // 2. 写入 GIF89a 文件头
    file.write("GIF89a", 6);

    // 3. 逻辑屏幕描述符 (Logical Screen Descriptor, 7 字节)
    char lsd[7];
    lsd[0] = static_cast<char>(width & 0xFF);
    lsd[1] = static_cast<char>((width >> 8) & 0xFF);
    lsd[2] = static_cast<char>(height & 0xFF);
    lsd[3] = static_cast<char>((height >> 8) & 0xFF);
    lsd[4] = static_cast<char>(0xF7); // 全局颜色表存在，8-bit 色彩分辨率，256 色
    lsd[5] = 0;                        // 背景色索引
    lsd[6] = 0;                        // 像素宽高比
    file.write(lsd, 7);

    // 4. 写入全局颜色表 (Global Color Table, 256 * 3 字节)
    int transparentIndex = -1;
    for (int i = 0; i < 256; ++i) {
        if (i < colorCount) {
            QRgb rgb = colorTable[i];
            if (qAlpha(rgb) < 128 && transparentIndex == -1) {
                transparentIndex = i;
            }
            char rgbBuf[3];
            rgbBuf[0] = static_cast<char>(qRed(rgb));
            rgbBuf[1] = static_cast<char>(qGreen(rgb));
            rgbBuf[2] = static_cast<char>(qBlue(rgb));
            file.write(rgbBuf, 3);
        } else {
            file.write("\x00\x00\x00", 3);
        }
    }

    // 5. 图形控制扩展 (Graphic Control Extension，支持透明通道)
    if (transparentIndex != -1) {
        char gce[8] = {'\x21', '\xF9', '\x04', '\x01', '\x00', '\x00', static_cast<char>(transparentIndex), '\x00'};
        file.write(gce, 8);
    }

    // 6. 图像描述符 (Image Descriptor, 10 字节)
    char id[10];
    id[0] = 0x2C; // 图像分隔符
    id[1] = 0; id[2] = 0; // 局部图像 X 偏移
    id[3] = 0; id[4] = 0; // 局部图像 Y 偏移
    id[5] = static_cast<char>(width & 0xFF);
    id[6] = static_cast<char>((width >> 8) & 0xFF);
    id[7] = static_cast<char>(height & 0xFF);
    id[8] = static_cast<char>((height >> 8) & 0xFF);
    id[9] = 0;    // 无局部调色板，非隔行扫描
    file.write(id, 10);

    // 7. LZW 压缩栅格像素数据 (标准 GIF LZW 变长位流编码)
    const int initCodeSize = 8;
    file.putChar(static_cast<char>(initCodeSize));

    const int clearCode = 1 << initCodeSize; // 256
    const int endCode = clearCode + 1;       // 257

    std::vector<int> prefix(5003, -1);
    std::vector<int> suffix(5003, -1);
    std::vector<int> code(5003, -1);

    auto clearTable = [&]() {
        std::fill(prefix.begin(), prefix.end(), -1);
        std::fill(suffix.begin(), suffix.end(), -1);
        std::fill(code.begin(), code.end(), -1);
    };

    clearTable();

    int n_bits = initCodeSize + 1;
    int maxcode = (1 << n_bits);
    int free_ent = clearCode + 2;

    unsigned int cur_accum = 0;
    int cur_bits = 0;
    std::vector<char> packet;

    auto flushPacket = [&]() {
        if (!packet.empty()) {
            file.putChar(static_cast<char>(packet.size()));
            file.write(packet.data(), static_cast<qint64>(packet.size()));
            packet.clear();
        }
    };

    auto writeBits = [&](int c) {
        cur_accum |= (static_cast<unsigned int>(c) << cur_bits);
        cur_bits += n_bits;
        while (cur_bits >= 8) {
            packet.push_back(static_cast<char>(cur_accum & 0xFF));
            if (packet.size() == 254) {
                flushPacket();
            }
            cur_accum >>= 8;
            cur_bits -= 8;
        }
    };

    // 写入起始 Clear Code
    writeBits(clearCode);

    int ent = -1;
    for (int y = 0; y < height; ++y) {
        const uchar* scanline = indexed.constScanLine(y);
        for (int x = 0; x < width; ++x) {
            int c = scanline[x];
            if (ent == -1) {
                ent = c;
                continue;
            }

            int disp = ((c << 4) ^ ent);
            int h = (ent << 8) | c;
            int i = h % 5003;
            if (disp == 0) disp = 1;

            bool found = false;
            while (code[i] != -1) {
                if (prefix[i] == ent && suffix[i] == c) {
                    ent = code[i];
                    found = true;
                    break;
                }
                i = (i + disp) % 5003;
            }

            if (!found) {
                writeBits(ent);
                if (free_ent < 4096) {
                    prefix[i] = ent;
                    suffix[i] = c;
                    code[i] = free_ent++;
                    if (free_ent > maxcode && n_bits < 12) {
                        n_bits++;
                        maxcode = (1 << n_bits);
                    }
                } else {
                    writeBits(clearCode);
                    clearTable();
                    n_bits = initCodeSize + 1;
                    maxcode = (1 << n_bits);
                    free_ent = clearCode + 2;
                }
                ent = c;
            }
        }
    }

    if (ent != -1) {
        writeBits(ent);
    }
    writeBits(endCode);

    // 刷新剩余未对齐的字节
    if (cur_bits > 0) {
        packet.push_back(static_cast<char>(cur_accum & 0xFF));
    }
    flushPacket();

    // 写入数据块终止符 (Block Terminator, 0x00)
    file.putChar('\x00');

    // 8. 写入 GIF 文件终止符 (Trailer, 0x3B)
    file.putChar('\x3B');
    file.close();

    return true;
}
