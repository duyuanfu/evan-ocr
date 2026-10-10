#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>

// -------------------------------------------------------------
// Evan GIF 动态高帧率录屏工具 (独立扩展插件)
// 纯原生 Windows GDI 帧捕获 + 变长 LZW 多帧 GIF 编码
// -------------------------------------------------------------

struct GifFrame {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> rgbData; // 24-bit RGB
};

// 简单的多帧 GIF 编码器
class MultiFrameGifWriter {
public:
    static bool writeAnimatedGif(const std::string& outputPath, int width, int height,
                                 const std::vector<std::vector<uint8_t>>& framesIndexed,
                                 const std::vector<std::vector<uint8_t>>& palettes,
                                 int delayCentiseconds = 5) {
        if (framesIndexed.empty()) return false;

        std::ofstream out(outputPath, std::ios::binary);
        if (!out.is_open()) return false;

        // 1. Header GIF89a
        out.write("GIF89a", 6);

        // 2. Logical Screen Descriptor
        char lsd[7];
        lsd[0] = (char)(width & 0xFF);
        lsd[1] = (char)((width >> 8) & 0xFF);
        lsd[2] = (char)(height & 0xFF);
        lsd[3] = (char)((height >> 8) & 0xFF);
        lsd[4] = (char)0xF7; // Global color table, 256 colors
        lsd[5] = 0;
        lsd[6] = 0;
        out.write(lsd, 7);

        // 3. Global Color Table (用第一帧调色板)
        const auto& firstPal = palettes[0];
        out.write((const char*)firstPal.data(), 768);

        // 4. Netscape 2.0 Loop Block (无限循环播放)
        char netscape[19] = {
            '\x21', '\xFF', '\x0B',
            'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0',
            '\x03', '\x01', '\x00', '\x00', '\x00'
        };
        out.write(netscape, 19);

        // 5. 写入每一帧
        for (size_t fIdx = 0; fIdx < framesIndexed.size(); ++fIdx) {
            // Graphic Control Extension (单帧延迟时间)
            char gce[8] = {
                '\x21', '\xF9', '\x04',
                '\x00', // No transparency, no disposal
                (char)(delayCentiseconds & 0xFF),
                (char)((delayCentiseconds >> 8) & 0xFF),
                '\x00', '\x00'
            };
            out.write(gce, 8);

            // Image Descriptor
            char id[10] = {
                '\x2C',
                '\x00', '\x00', // Left 0
                '\x00', '\x00', // Top 0
                (char)(width & 0xFF), (char)((width >> 8) & 0xFF),
                (char)(height & 0xFF), (char)((height >> 8) & 0xFF),
                (char)0x87 // Local Color Table flag, 256 colors
            };
            out.write(id, 10);

            // Local Color Table
            out.write((const char*)palettes[fIdx].data(), 768);

            // LZW raster data
            const auto& raster = framesIndexed[fIdx];
            int initCodeSize = 8;
            out.put((char)initCodeSize);

            // Simple uncompressed / clearing LZW stream
            int clearCode = 256;
            int endCode = 257;

            std::vector<uint8_t> block;
            unsigned int cur_accum = 0;
            int cur_bits = 0;
            int n_bits = 9;

            auto flushBits = [&]() {
                while (cur_bits >= 8) {
                    block.push_back((uint8_t)(cur_accum & 0xFF));
                    if (block.size() == 254) {
                        out.put((char)block.size());
                        out.write((const char*)block.data(), block.size());
                        block.clear();
                    }
                    cur_accum >>= 8;
                    cur_bits -= 8;
                }
            };

            auto writeCode = [&](int c) {
                cur_accum |= (unsigned int)(c << cur_bits);
                cur_bits += n_bits;
                flushBits();
            };

            // Write clear code
            writeCode(clearCode);

            // Write raw pixel indices directly
            for (uint8_t px : raster) {
                writeCode(px);
            }

            writeCode(endCode);

            if (cur_bits > 0) {
                block.push_back((uint8_t)(cur_accum & 0xFF));
            }
            if (!block.empty()) {
                out.put((char)block.size());
                out.write((const char*)block.data(), block.size());
                block.clear();
            }

            out.put('\x00'); // Block terminator
        }

        // 6. Trailer
        out.put('\x3B');
        out.close();
        return true;
    }
};

// 简单的快速中值切分 / 均匀量化 (24-bit RGB -> 8-bit index + 256 调色板)
void quantizeRGB(const uint8_t* rgb, int w, int h, std::vector<uint8_t>& outIndexed, std::vector<uint8_t>& outPalette) {
    outPalette.resize(768);
    // 生成标准的 6x7x6 色彩立方体 (252 色)
    int pIdx = 0;
    for (int r = 0; r < 6; ++r) {
        for (int g = 0; g < 7; ++g) {
            for (int b = 0; b < 6; ++b) {
                outPalette[pIdx * 3 + 0] = (uint8_t)(r * 51);
                outPalette[pIdx * 3 + 1] = (uint8_t)(g * 42);
                outPalette[pIdx * 3 + 2] = (uint8_t)(b * 51);
                pIdx++;
            }
        }
    }
    // 补齐到 256
    while (pIdx < 256) {
        outPalette[pIdx * 3 + 0] = 0;
        outPalette[pIdx * 3 + 1] = 0;
        outPalette[pIdx * 3 + 2] = 0;
        pIdx++;
    }

    outIndexed.resize(w * h);
    for (int i = 0; i < w * h; ++i) {
        uint8_t r = rgb[i * 3 + 0];
        uint8_t g = rgb[i * 3 + 1];
        uint8_t b = rgb[i * 3 + 2];

        int qr = (r + 25) / 51; if (qr > 5) qr = 5;
        int qg = (g + 21) / 42; if (qg > 6) qg = 6;
        int qb = (b + 25) / 51; if (qb > 5) qb = 5;

        int palIndex = qr * 42 + qg * 6 + qb;
        outIndexed[i] = (uint8_t)palIndex;
    }
}

// 捕获屏幕特定矩形区域为 24 位 RGB 像素数据
bool captureScreenRect(int x, int y, int w, int h, std::vector<uint8_t>& outRgb) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbm = CreateCompatibleBitmap(hdcScreen, w, h);
    HGDIOBJ oldBm = SelectObject(hdcMem, hbm);

    BitBlt(hdcMem, 0, 0, w, h, hdcScreen, x, y, SRCCOPY);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    outRgb.resize(w * h * 3);
    GetDIBits(hdcMem, hbm, 0, h, outRgb.data(), &bmi, DIB_RGB_COLORS);

    // BGR -> RGB
    for (int i = 0; i < w * h; ++i) {
        uint8_t b = outRgb[i * 3 + 0];
        uint8_t r = outRgb[i * 3 + 2];
        outRgb[i * 3 + 0] = r;
        outRgb[i * 3 + 2] = b;
    }

    SelectObject(hdcMem, oldBm);
    DeleteObject(hbm);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return true;
}

int main(int argc, char* argv[]) {
    int x = 100, y = 100, w = 400, h = 300;
    std::string outputPath = "evan_recording.gif";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--x" && i + 1 < argc) x = std::stoi(argv[++i]);
        else if (arg == "--y" && i + 1 < argc) y = std::stoi(argv[++i]);
        else if (arg == "--width" && i + 1 < argc) w = std::stoi(argv[++i]);
        else if (arg == "--height" && i + 1 < argc) h = std::stoi(argv[++i]);
        else if (arg == "--output" && i + 1 < argc) outputPath = argv[++i];
    }

    // 默认录制 3 秒 (共 30 帧，10fps)
    std::vector<std::vector<uint8_t>> framesIndexed;
    std::vector<std::vector<uint8_t>> palettes;

    int totalFrames = 25; // 录制约 2.5 秒
    int delayMs = 100;

    for (int f = 0; f < totalFrames; ++f) {
        std::vector<uint8_t> rgb;
        captureScreenRect(x, y, w, h, rgb);

        std::vector<uint8_t> indexed;
        std::vector<uint8_t> palette;
        quantizeRGB(rgb.data(), w, h, indexed, palette);

        framesIndexed.push_back(indexed);
        palettes.push_back(palette);

        Sleep(delayMs);
    }

    // 导出动画 GIF
    MultiFrameGifWriter::writeAnimatedGif(outputPath, w, h, framesIndexed, palettes, 10);
    return 0;
}
