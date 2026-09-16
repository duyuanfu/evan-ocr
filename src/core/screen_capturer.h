#pragma once

#include <QPixmap>
#include <QRect>
#include <QList>

struct ScreenSnapshot {
    QPixmap fullSnapshot;       // 拼接了所有屏幕的超大虚拟桌面快照
    QRect virtualGeometry;      // 虚拟桌面总矩形 (包含负坐标，如 (-1920, 0, 3840, 1080))
    QList<QRect> screenGeometries; // 各物理屏幕在虚拟桌面上的相对坐标
    bool isValid = false;
};

class ScreenCapturer {
public:
    // 捕获所有显示器并合成为统一坐标系下的虚拟桌面快照
    static ScreenSnapshot captureVirtualScreen();
};
