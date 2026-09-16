#include "screen_capturer.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QDebug>

ScreenSnapshot ScreenCapturer::captureVirtualScreen()
{
    ScreenSnapshot snapshot;
    const auto screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        qWarning() << "[ScreenCapturer] 未检测到任何屏幕";
        return snapshot;
    }

    // 1. 计算所有屏幕联合的虚拟屏幕总矩形 (逻辑坐标)
    QRect virtualRect;
    for (auto* screen : screens) {
        if (virtualRect.isNull()) {
            virtualRect = screen->geometry();
        } else {
            virtualRect = virtualRect.united(screen->geometry());
        }
    }

    snapshot.virtualGeometry = virtualRect;

    // 获取主屏设备像素比
    qreal primaryDpr = QGuiApplication::primaryScreen()->devicePixelRatio();

    // 2. 分配高保真画布 (使用物理像素尺寸，设置 DPR)
    QPixmap canvas(virtualRect.size() * primaryDpr);
    canvas.setDevicePixelRatio(primaryDpr);
    canvas.fill(Qt::black);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    // 3. 逐个屏幕捕获并绘制到对应相对坐标上
    for (auto* screen : screens) {
        QRect screenGeom = screen->geometry();
        snapshot.screenGeometries.append(screenGeom);

        // 截取单屏画面 (Qt 6 grabWindow 默认自带对应屏幕的 devicePixelRatio)
        QPixmap screenPix = screen->grabWindow(0);
        screenPix.setDevicePixelRatio(screen->devicePixelRatio());

        // 换算到虚拟画布内的绘制逻辑偏移
        QPoint offset = screenGeom.topLeft() - virtualRect.topLeft();
        painter.drawPixmap(offset, screenPix);
    }
    painter.end();

    snapshot.fullSnapshot = canvas;
    snapshot.isValid = !snapshot.fullSnapshot.isNull();

    qDebug() << "[ScreenCapturer] 截屏完成，屏幕数量:" << screens.size()
             << "虚拟桌面尺寸:" << virtualRect
             << "主屏DPR:" << primaryDpr;

    return snapshot;
}
