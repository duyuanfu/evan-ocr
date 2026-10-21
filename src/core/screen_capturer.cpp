#include "screen_capturer.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QDebug>
#include <cmath>

/**
 * @brief 计算单块屏幕的物理像素矩形
 *
 * 【重要】Qt 6 在多显示器 + 混合缩放 (Per-Monitor DPI) 场景下，
 * QScreen::geometry() 返回的是一个「混合坐标系」：
 *   - 位置 (x/y) 直接是物理像素值，未做任何缩放；
 *   - 尺寸 (width/height) 则是 物理尺寸 / 该屏幕自身的 devicePixelRatio。
 * 直接把这些矩形 union 起来再按统一 DPR 绘制，会导致：
 *   - 屏幕内容被拉伸/放大；
 *   - 屏幕之间出现黑色缝隙；
 *   - 覆盖窗口与真实显示器错位。
 *
 * 因此这里统一换算为「纯物理像素」空间：
 *   物理位置 = geometry().topLeft()               (1:1)
 *   物理尺寸 = geometry().size() * devicePixelRatio()
 */
static QRect screenPhysicalRect(const QScreen* screen)
{
    const QRect g = screen->geometry();
    const qreal dpr = screen->devicePixelRatio();
    return QRect(g.x(), g.y(),
                 static_cast<int>(std::round(g.width() * dpr)),
                 static_cast<int>(std::round(g.height() * dpr)));
}

QRect ScreenCapturer::physicalVirtualGeometry()
{
    const auto screens = QGuiApplication::screens();
    if (screens.isEmpty()) return QRect();

    QRect physRect;
    for (auto* screen : screens) {
        QRect r = screenPhysicalRect(screen);
        if (physRect.isNull()) physRect = r;
        else physRect = physRect.united(r);
    }
    return physRect;
}

QScreen* ScreenCapturer::screenAtPhysical(const QRect& physicalRect)
{
    const auto screens = QGuiApplication::screens();
    if (screens.isEmpty()) return nullptr;
    if (physicalRect.isNull()) return QGuiApplication::primaryScreen();

    // 优先选择与物理矩形相交面积最大的屏幕
    QScreen* best = nullptr;
    int bestArea = 0;
    for (auto* screen : screens) {
        const QRect r = screenPhysicalRect(screen);
        const QRect overlap = r.intersected(physicalRect);
        if (overlap.isEmpty()) continue;
        const int area = overlap.width() * overlap.height();
        if (area > bestArea) {
            bestArea = area;
            best = screen;
        }
    }
    if (best) return best;

    // 未命中：按物理矩形中心点就近选择
    const QPoint center = physicalRect.center();
    QScreen* nearest = nullptr;
    int nearestDist = 0;
    for (auto* screen : screens) {
        const QRect r = screenPhysicalRect(screen);
        const int dx = qMax(r.left() - center.x(), center.x() - r.right());
        const int dy = qMax(r.top() - center.y(), center.y() - r.bottom());
        const int dist = dx * dx + dy * dy;
        if (!nearest || dist < nearestDist) {
            nearestDist = dist;
            nearest = screen;
        }
    }
    return nearest ? nearest : QGuiApplication::primaryScreen();
}

ScreenSnapshot ScreenCapturer::captureVirtualScreen(qreal targetDpr, const QRect& targetPhysical){
    ScreenSnapshot snapshot;
    const auto screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        qWarning() << "[ScreenCapturer] 未检测到任何屏幕";
        return snapshot;
    }

    // 1. 物理像素下的虚拟桌面总矩形
    QRect physVirt = physicalVirtualGeometry();
    if (physVirt.isNull()) {
        qWarning() << "[ScreenCapturer] 无法计算物理虚拟桌面矩形";
        return snapshot;
    }

    // 2. 画布覆盖区域：默认整块虚拟桌面；若外部指定 (覆盖窗口真实客户区) 则以其为准
    QRect captureArea = targetPhysical.isNull() ? physVirt : targetPhysical;
    qreal canvasDpr = targetDpr > 0.0 ? targetDpr : 1.0;

    snapshot.physicalVirtualRect = captureArea;

    // 3. 分配画布 (物理像素 1:1，DPR 决定其逻辑尺寸)
    QPixmap canvas(captureArea.size());
    canvas.setDevicePixelRatio(canvasDpr);
    canvas.fill(Qt::black);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 4. 逐屏按真实物理矩形精确落位绘制
    for (auto* screen : screens) {
        QRect physRect = screenPhysicalRect(screen);

        QPixmap screenPix = screen->grabWindow(0);
        if (screenPix.isNull()) continue;

        // 抓屏结果应与该屏物理像素尺寸一致，否则说明抓取被降级 (如远程桌面/受保护内容)
        if (screenPix.size() != physRect.size()) {
            qWarning() << "[ScreenCapturer] 抓屏尺寸与物理尺寸不一致，屏幕:" << screen->name()
                       << "抓取:" << screenPix.size() << "期望:" << physRect.size();
        }

        // 目标矩形：物理像素 -> 画布逻辑坐标 (使用浮点以避免整数取整产生 1px 接缝)
        QPointF physOffset(physRect.x() - captureArea.x(), physRect.y() - captureArea.y());
        QRectF targetRect(
            physOffset.x() / canvasDpr,
            physOffset.y() / canvasDpr,
            physRect.width() / canvasDpr,
            physRect.height() / canvasDpr
        );

        // 源矩形使用「设备像素」坐标：grabWindow 返回的 pixmap 携带该屏 DPR，
        // 其 rect() 为设备像素尺寸，直接使用可保证 1:1 无缩放落位。
        painter.drawPixmap(targetRect, screenPix, QRectF(screenPix.rect()));
    }
    painter.end();

    snapshot.fullSnapshot = canvas;
    snapshot.isValid = !snapshot.fullSnapshot.isNull();

    qDebug() << "[ScreenCapturer] 截屏完成，屏幕数量:" << screens.size()
             << "物理虚拟桌面:" << physVirt
             << "画布覆盖区(物理):" << captureArea
             << "画布DPR:" << canvasDpr;

    for (auto* screen : screens) {
        qDebug() << "   - 屏幕:" << screen->name()
                 << "逻辑几何:" << screen->geometry()
                 << "DPR:" << screen->devicePixelRatio()
                 << "物理矩形:" << screenPhysicalRect(screen);
    }

    return snapshot;
}
