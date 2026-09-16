#pragma once

#include <QImage>
#include <QRect>
#include <QPoint>
#include <vector>

class SmartSnapper
{
public:
    SmartSnapper() = default;
    ~SmartSnapper() = default;

    // 对全屏快照进行多尺度金字塔下采样并提取结构化轮廓树 (传入 dpr 以便换算为逻辑坐标)
    void processSnapshot(const QImage& snapshot, qreal dpr = 1.0);

    // 查询包含指定光标坐标的所有闭合候选框，按包含层次由内到外（面积从小到大）排列 (逻辑坐标)
    std::vector<QRect> findCandidatesAt(const QPoint& pos) const;

    // 清除分析结果
    void clear();

private:
    std::vector<QRect> m_boundingBoxes;
};
