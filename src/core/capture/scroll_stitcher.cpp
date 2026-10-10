#include "scroll_stitcher.h"
#include <QPainter>
#include <cmath>
#include <algorithm>

void ScrollStitcher::reset(const QImage& initialFrame)
{
    m_stitchedImage = initialFrame.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    m_stitchedImage.setDevicePixelRatio(1.0); // 必须归一化为 1.0 物理像素，彻底杜绝 DPR 缩放产生未绘制黑色条带
    m_lastFrame = m_stitchedImage;
    m_frameCount = 1;
}

QImage ScrollStitcher::currentStitchedImage() const
{
    return m_stitchedImage;
}

int ScrollStitcher::currentTotalHeight() const
{
    return m_stitchedImage.isNull() ? 0 : m_stitchedImage.height();
}

bool ScrollStitcher::computeVerticalOffset(const QImage& prevFrame, const QImage& currFrame, int& outDeltaY, double& outError)
{
    int W = prevFrame.width();
    int H = prevFrame.height();
    if (W < 40 || H < 60) return false;

    // 两侧忽略边距 (排除滚动条与外侧阴影)
    int sideMargin = qBound(8, W / 20, 24);
    int validW = W - 2 * sideMargin;
    if (validW <= 20) return false;

    // 1. 前置静止帧检测：比较两帧在全视口的平均像素绝对差 (MAD)
    double diffZero = 0.0;
    int sampleZeroCount = 0;
    for (int y = 0; y < H; y += 4) {
        const QRgb* linePrev = reinterpret_cast<const QRgb*>(prevFrame.constScanLine(y));
        const QRgb* lineCurr = reinterpret_cast<const QRgb*>(currFrame.constScanLine(y));
        for (int x = sideMargin; x < W - sideMargin; x += 4) {
            QRgb p1 = linePrev[x];
            QRgb p2 = lineCurr[x];
            diffZero += std::abs(qRed(p1) - qRed(p2)) + std::abs(qGreen(p1) - qGreen(p2)) + std::abs(qBlue(p1) - qBlue(p2));
            sampleZeroCount++;
        }
    }
    double avgDiffZero = (sampleZeroCount > 0) ? (diffZero / (sampleZeroCount * 3.0)) : 0.0;

    // 若画面静止未发生明显位移 (平均像素差小于 3.5)，判定为静止帧，绝对不重复拼接！
    if (avgDiffZero < 3.5) {
        outDeltaY = 0;
        outError = avgDiffZero;
        return false;
    }

    // 2. 全重叠区域均方误差 (Full Overlap MSE) 对齐搜索
    int minD = 8;
    int maxD = qMin(H - 30, static_cast<int>(H * 0.85));

    double bestMSE = 1e9;
    int bestD = -1;

    int stepD = 1;
    int rowStep = (H > 500) ? 2 : 1;
    int colStep = (validW > 400) ? 2 : 1;

    for (int d = minD; d <= maxD; d += stepD) {
        int overlapH = H - d;
        double sumDiff = 0.0;
        int count = 0;

        for (int y = 0; y < overlapH; y += rowStep) {
            const QRgb* linePrev = reinterpret_cast<const QRgb*>(prevFrame.constScanLine(d + y));
            const QRgb* lineCurr = reinterpret_cast<const QRgb*>(currFrame.constScanLine(y));

            for (int x = sideMargin; x < W - sideMargin; x += colStep) {
                QRgb p1 = linePrev[x];
                QRgb p2 = lineCurr[x];

                int dr = qRed(p1) - qRed(p2);
                int dg = qGreen(p1) - qGreen(p2);
                int db = qBlue(p1) - qBlue(p2);

                sumDiff += std::abs(dr) + std::abs(dg) + std::abs(db);
                count++;
            }
        }

        double mse = (count > 0) ? (sumDiff / (count * 3.0)) : 1e9;
        if (mse < bestMSE) {
            bestMSE = mse;
            bestD = d;
        }
    }

    outError = bestMSE;

    // 严格匹配置信度判定
    if (bestMSE < 16.0 && bestMSE < avgDiffZero * 0.70 && bestD >= minD) {
        outDeltaY = bestD;
        return true;
    }

    return false;
}

bool ScrollStitcher::appendFrame(const QImage& newFrame, int& outDeltaY)
{
    if (m_stitchedImage.isNull() || newFrame.isNull()) {
        return false;
    }

    // 安全保护：长截图最大高度限制 35000 像素
    if (m_stitchedImage.height() >= 35000) {
        return false;
    }

    QImage curr = newFrame.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    curr.setDevicePixelRatio(1.0); // 严格归一化为 1.0 物理像素，彻底避免 DPR 缩放产生黑条
    int deltaY = 0;
    double error = 0.0;

    if (!computeVerticalOffset(m_lastFrame, curr, deltaY, error)) {
        return false;
    }

    int W = m_stitchedImage.width();
    int oldH = m_stitchedImage.height();
    int newTotalH = oldH + deltaY;

    // 动态扩充长图画布，仅将当前帧底部新露出的 deltaY 像素像素级追加至大图下方
    QImage newCanvas(W, newTotalH, QImage::Format_ARGB32_Premultiplied);
    newCanvas.setDevicePixelRatio(1.0);
    newCanvas.fill(Qt::transparent);

    QPainter p(&newCanvas);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawImage(0, 0, m_stitchedImage);
    p.drawImage(0, oldH, curr, 0, curr.height() - deltaY, W, deltaY);
    p.end();

    m_stitchedImage = newCanvas;
    m_lastFrame = curr;
    m_frameCount++;
    outDeltaY = deltaY;

    return true;
}
