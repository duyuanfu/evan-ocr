#include "char_inpainter.h"
#include <QPainter>
#include <cmath>
#include <vector>
#include <algorithm>

namespace {

struct ColorStats {
    int r = 0, g = 0, b = 0;
};

static int colorDist(const QColor& c1, const QColor& c2)
{
    int dr = c1.red() - c2.red();
    int dg = c1.green() - c2.green();
    int db = c1.blue() - c2.blue();
    return static_cast<int>(std::sqrt(dr * dr + dg * dg + db * db));
}

} // namespace

CharInpaintResult CharInpainter::inpaintChar(const QImage& fullSnapshot, const QRect& charPhysicalBox, int padding)
{
    CharInpaintResult res;
    if (fullSnapshot.isNull() || charPhysicalBox.isEmpty()) {
        return res;
    }

    // 1. 局部微创包围盒向外微扩 1~2px
    QRect patch = charPhysicalBox.adjusted(-padding, -padding, padding, padding).intersected(fullSnapshot.rect());
    if (patch.width() < 2 || patch.height() < 2) {
        return res;
    }

    res.patchRect = patch;
    int pw = patch.width();
    int ph = patch.height();

    // 2. 收集顶部与底部边缘背景样本 (文字行上下边界基本均为纯净背景，受相邻字符影响最小)
    std::vector<QColor> topSamples;
    std::vector<QColor> btmSamples;

    for (int x = 0; x < pw; ++x) {
        topSamples.push_back(fullSnapshot.pixelColor(patch.left() + x, patch.top()));
        btmSamples.push_back(fullSnapshot.pixelColor(patch.left() + x, patch.bottom()));
    }

    // 计算均值
    long trSum = 0, tgSum = 0, tbSum = 0;
    for (const auto& c : topSamples) {
        trSum += c.red(); tgSum += c.green(); tbSum += c.blue();
    }
    QColor avgTop(trSum / pw, tgSum / pw, tbSum / pw);

    long brSum = 0, bgSum = 0, bbSum = 0;
    for (const auto& c : btmSamples) {
        brSum += c.red(); bgSum += c.green(); bbSum += c.blue();
    }
    QColor avgBtm(brSum / pw, bgSum / pw, bbSum / pw);

    // 估算全局背景色
    QColor overallBg(
        (avgTop.red() + avgBtm.red()) / 2,
        (avgTop.green() + avgBtm.green()) / 2,
        (avgTop.blue() + avgBtm.blue()) / 2
    );
    res.estimatedBgColor = overallBg;

    // 3. 构建修补图像
    QImage patchImg(pw, ph, QImage::Format_ARGB32_Premultiplied);

    bool hasVerticalGradient = (colorDist(avgTop, avgBtm) > 6);

    for (int y = 0; y < ph; ++y) {
        double v = (ph > 1) ? (y / static_cast<double>(ph - 1)) : 0.0;
        int r = hasVerticalGradient ? static_cast<int>(std::round((1.0 - v) * avgTop.red() + v * avgBtm.red())) : overallBg.red();
        int g = hasVerticalGradient ? static_cast<int>(std::round((1.0 - v) * avgTop.green() + v * avgBtm.green())) : overallBg.green();
        int b = hasVerticalGradient ? static_cast<int>(std::round((1.0 - v) * avgTop.blue() + v * avgBtm.blue())) : overallBg.blue();

        QRgb* scanline = reinterpret_cast<QRgb*>(patchImg.scanLine(y));
        for (int x = 0; x < pw; ++x) {
            scanline[x] = qRgba(r, g, b, 255);
        }
    }

    res.patchImage = patchImg;
    res.success = true;
    return res;
}

bool CharInpainter::applyPatch(QImage& targetImage, const CharInpaintResult& patch)
{
    if (!patch.success || targetImage.isNull() || patch.patchRect.isEmpty() || patch.patchImage.isNull()) {
        return false;
    }

    QPainter painter(&targetImage);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(patch.patchRect.topLeft(), patch.patchImage);
    painter.end();
    return true;
}
