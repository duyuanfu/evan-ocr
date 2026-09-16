#include "smart_snapper.h"
#include <algorithm>
#include <cmath>
#include <vector>

void SmartSnapper::clear()
{
    m_boundingBoxes.clear();
}

void SmartSnapper::processSnapshot(const QImage& snapshot, qreal dpr)
{
    clear();
    if (snapshot.isNull() || snapshot.width() < 32 || snapshot.height() < 32) {
        return;
    }

    if (dpr <= 0.0) dpr = 1.0;

    // 1. 金字塔下采样 (1/2 缩放，极大降低计算量同时保留完整 UI 控件轮廓)
    constexpr int SCALE = 2;
    int downW = snapshot.width() / SCALE;
    int downH = snapshot.height() / SCALE;

    QImage downImg = snapshot.scaled(downW, downH, Qt::IgnoreAspectRatio, Qt::FastTransformation)
                             .convertToFormat(QImage::Format_Grayscale8);

    const int w = downImg.width();
    const int h = downImg.height();
    const uchar* bits = downImg.constBits();
    const qsizetype bpl = downImg.bytesPerLine();

    // 2. Sobel 快速梯度与边缘阈值提取
    std::vector<uint8_t> edges(w * h, 0);
    constexpr int EDGE_THRESH = 28;

    for (int y = 1; y < h - 1; ++y) {
        const uchar* prevRow = bits + (y - 1) * bpl;
        const uchar* currRow = bits + y * bpl;
        const uchar* nextRow = bits + (y + 1) * bpl;
        uint8_t* edgeRow = edges.data() + y * w;

        for (int x = 1; x < w - 1; ++x) {
            int gx = (prevRow[x + 1] + 2 * currRow[x + 1] + nextRow[x + 1])
                   - (prevRow[x - 1] + 2 * currRow[x - 1] + nextRow[x - 1]);
            int gy = (nextRow[x - 1] + 2 * nextRow[x] + nextRow[x + 1])
                   - (prevRow[x - 1] + 2 * prevRow[x] + prevRow[x + 1]);

            int mag = std::abs(gx) + std::abs(gy);
            if (mag > EDGE_THRESH) {
                edgeRow[x] = 255;
            }
        }
    }

    // 3. 连通域外接矩形探测 (换算为与鼠标坐标 1:1 对齐的逻辑坐标)
    std::vector<bool> visited(w * h, false);
    std::vector<QRect> rawBoxes;

    for (int y = 2; y < h - 2; y += 3) {
        for (int x = 2; x < w - 2; x += 3) {
            int idx = y * w + x;
            if (visited[idx] || edges[idx] != 0) continue;

            int minX = x, maxX = x;
            int minY = y, maxY = y;
            std::vector<std::pair<int, int>> queue;
            queue.reserve(1024);
            queue.push_back({x, y});
            visited[idx] = true;

            size_t head = 0;
            while (head < queue.size() && queue.size() < 15000) {
                auto [cx, cy] = queue[head++];
                minX = std::min(minX, cx);
                maxX = std::max(maxX, cx);
                minY = std::min(minY, cy);
                maxY = std::max(maxY, cy);

                static const int dx[] = {0, 0, -2, 2};
                static const int dy[] = {-2, 2, 0, 0};
                for (int i = 0; i < 4; ++i) {
                    int nx = cx + dx[i];
                    int ny = cy + dy[i];
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        int nidx = ny * w + nx;
                        if (!visited[nidx] && edges[nidx] == 0) {
                            visited[nidx] = true;
                            queue.push_back({nx, ny});
                        }
                    }
                }
            }

            int bw = maxX - minX + 1;
            int bh = maxY - minY + 1;
            if (bw >= 12 && bh >= 10 && bw < w - 4 && bh < h - 4) {
                // 除以 dpr 转换为逻辑像素，彻底避免候选框被放大 1.25x
                int lx = static_cast<int>(std::round((minX * SCALE) / dpr));
                int ly = static_cast<int>(std::round((minY * SCALE) / dpr));
                int lw = static_cast<int>(std::round((bw * SCALE) / dpr));
                int lh = static_cast<int>(std::round((bh * SCALE) / dpr));
                rawBoxes.emplace_back(lx, ly, lw, lh);
            }
        }
    }

    // 4. 去重与规范化
    for (const auto& box : rawBoxes) {
        bool duplicate = false;
        for (const auto& existing : m_boundingBoxes) {
            if (std::abs(box.x() - existing.x()) < 6 &&
                std::abs(box.y() - existing.y()) < 6 &&
                std::abs(box.width() - existing.width()) < 10 &&
                std::abs(box.height() - existing.height()) < 10) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            m_boundingBoxes.push_back(box);
        }
    }

    // 添加最外层全图逻辑矩形作为根节点兜底
    int rootW = static_cast<int>(std::round(snapshot.width() / dpr));
    int rootH = static_cast<int>(std::round(snapshot.height() / dpr));
    m_boundingBoxes.emplace_back(0, 0, rootW, rootH);
}

std::vector<QRect> SmartSnapper::findCandidatesAt(const QPoint& pos) const
{
    std::vector<QRect> matched;
    for (const auto& box : m_boundingBoxes) {
        if (box.contains(pos)) {
            matched.push_back(box);
        }
    }

    // 按面积升序排列：最小叶子节点 (按钮/图标) 优先，外层卡片/窗口排在后面
    std::sort(matched.begin(), matched.end(), [](const QRect& a, const QRect& b) {
        return (static_cast<int64_t>(a.width()) * a.height()) <
               (static_cast<int64_t>(b.width()) * b.height());
    });

    return matched;
}
