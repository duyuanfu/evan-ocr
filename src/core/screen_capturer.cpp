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

    // 2. 统一换算基准 DPR：取所有屏幕中最大的 devicePixelRatio
    //    【关键修复】多屏混合缩放 (例如左侧 4K@150% + 右侧 1080P@100%) 时，
    //    若沿用主屏 DPR 作为全局画布基准，副屏会被按自身逻辑尺寸绘制，
    //    导致「屏幕向一侧缩进 / 内容被放大 / 另一侧残留黑边」的严重错位 Bug。
    //    统一以最大 DPR 建画布，并按每块屏幕的「逻辑矩形」精确落位，
    //    可保证任意多屏 / 任意缩放组合下几何 1:1 对齐、无黑边、无缩放失真。
    qreal canvasDpr = 1.0;
    for (auto* screen : screens) {
        canvasDpr = qMax(canvasDpr, screen->devicePixelRatio());
    }

    // 3. 分配高保真画布 (使用物理像素尺寸，设置 DPR)
    QPixmap canvas(virtualRect.size() * canvasDpr);
    canvas.setDevicePixelRatio(canvasDpr);
    canvas.fill(Qt::black);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 4. 逐个屏幕捕获并按其逻辑矩形精确绘制到虚拟画布
    for (auto* screen : screens) {
        QRect screenGeom = screen->geometry();
        snapshot.screenGeometries.append(screenGeom);

        // 截取单屏画面 (像素内容)
        QPixmap screenPix = screen->grabWindow(0);
        if (screenPix.isNull()) continue;

        // 换算到虚拟画布内的绘制逻辑偏移 (画布 QPainter 会自动乘以 canvasDpr)
        QPoint offset = screenGeom.topLeft() - virtualRect.topLeft();
        QRect targetRect(offset, screenGeom.size());

        // 源矩形使用像素坐标，兼容 grabWindow 返回的不同 DPR 表现
        painter.drawPixmap(targetRect, screenPix, screenPix.rect());
    }
    painter.end();

    snapshot.fullSnapshot = canvas;
    snapshot.isValid = !snapshot.fullSnapshot.isNull();

    qDebug() << "[ScreenCapturer] 截屏完成，屏幕数量:" << screens.size()
             << "虚拟桌面尺寸:" << virtualRect
             << "画布DPR:" << canvasDpr;

    for (auto* screen : screens) {
        qDebug() << "   - 屏幕:" << screen->name()
                 << "逻辑几何:" << screen->geometry()
                 << "DPR:" << screen->devicePixelRatio();
    }

    return snapshot;
}
