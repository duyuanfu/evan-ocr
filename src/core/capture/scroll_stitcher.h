#pragma once

#include <QImage>
#include <vector>

class ScrollStitcher
{
public:
    ScrollStitcher() = default;

    // 重置并初始化长截图画布
    void reset(const QImage& initialFrame);

    // 尝试追加新的一帧截图，若检测到向下滚动位移则缝合追加并返回 true，输出 deltaY
    bool appendFrame(const QImage& newFrame, int& outDeltaY);

    // 获取当前缝合后的完整长截图
    QImage currentStitchedImage() const;

    // 当前已拼接总高度
    int currentTotalHeight() const;

    // 当前已捕获帧数
    int frameCount() const { return m_frameCount; }

    // 是否已初始化
    bool isValid() const { return !m_stitchedImage.isNull(); }

private:
    // 计算两帧图像之间的垂直位移 (返回位移像素数与误差均值)
    bool computeVerticalOffset(const QImage& prevFrame, const QImage& currFrame, int& outDeltaY, double& outError);

    QImage m_stitchedImage; // 累积拼接的大图
    QImage m_lastFrame;     // 上一帧参考视口画面
    int m_frameCount = 0;
};
