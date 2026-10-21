#pragma once

#include <QPixmap>
#include <QRect>
#include <QList>

class QScreen;

struct ScreenSnapshot {
    QPixmap fullSnapshot;       // 拼接了所有屏幕的超大虚拟桌面快照 (物理像素 1:1)
    QRect physicalVirtualRect;  // 画布覆盖的物理像素区域 (相对桌面原点的绝对物理坐标)
    bool isValid = false;
};

class ScreenCapturer {
public:
    // 物理像素下的虚拟桌面总矩形 (多屏混合缩放时唯一可靠的真实布局)
    static QRect physicalVirtualGeometry();

    /**
     * @brief 查找包含指定「物理像素矩形」的屏幕
     *
     * QGuiApplication::screenAt() 使用全局逻辑坐标，在混合缩放多屏下
     * 依据矩形中心点反查容易落到错误的屏幕，故此处按物理矩形判定。
     * @return 命中的屏幕；未命中时回退为包含矩形中心点的屏幕，再回退为主屏
     */
    static QScreen* screenAtPhysical(const QRect& physicalRect);

    /**
     * @brief 捕获所有显示器并合成为统一坐标系下的虚拟桌面快照
     * @param targetDpr      画布换算基准 DPR (应与覆盖窗口实际渲染 DPR 一致)
     * @param targetPhysical 画布需要覆盖的物理区域 (通常为覆盖窗口的真实客户区)
     */
    static ScreenSnapshot captureVirtualScreen(qreal targetDpr = 1.0, const QRect& targetPhysical = QRect());
};
