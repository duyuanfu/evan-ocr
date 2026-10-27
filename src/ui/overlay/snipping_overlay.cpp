#include "snipping_overlay.h"
#include "../../core/app_settings.h"
#include "../../core/hotkey_config.h"
#include "../../core/ocr/ocr_manager.h"
#include "../../core/char_edit/char_segmentation.h"
#include "../../core/char_edit/char_inpainter.h"
#include "../../core/char_edit/char_font_analyzer.h"
#include "../char_edit/single_char_editor.h"
#include "../magnifier/magnifier_widget.h"
#include "../annotation/inplace_text_editor.h"
#include "../ocr/ocr_result_dialog.h"
#include "../formula/formula_result_dialog.h"
#include "../plugin/plugin_market_dialog.h"
#include "../capture/scroll_capture_widget.h"
#include "../capture/gif_recorder_widget.h"
#include "../common/save_helper.h"
#include "../../core/export/image_exporter.h"
#include <QPointer>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <QSettings>
#include <QDateTime>
#include <QTimer>
#include <QProcess>
#include <QScreen>
#include <QGuiApplication>
#include <QMessageBox>
#include <QDebug>
#include <QRectF>
#include <QPointF>
#include <cmath>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * @brief 获取覆盖窗口真实客户区在物理像素下的矩形
 *
 * 在多显示器混合缩放场景下，Qt 逻辑坐标与 Windows 物理坐标的换算关系
 * 会因窗口所处屏幕的 DPI 而变化，直接用 setGeometry 无法保证覆盖区域
 * 与真实显示器像素级对齐。此处直接向 Win32 查询客户区物理矩形，
 * 作为截图画布的唯一真值基准。
 */
static QRect clientPhysicalRect(QWidget* widget)
{
#ifdef Q_OS_WIN
    if (!widget) return QRect();
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    if (!hwnd) return QRect();
    RECT rc{};
    if (!GetClientRect(hwnd, &rc)) return QRect();
    POINT pt{rc.left, rc.top};
    if (!ClientToScreen(hwnd, &pt)) return QRect();
    return QRect(pt.x, pt.y, rc.right - rc.left, rc.bottom - rc.top);
#else
    Q_UNUSED(widget);
    return QRect();
#endif
}

/**
 * @brief 以「物理像素」精确定位并铺满指定物理区域
 *
 * Windows 对 Per-Monitor-V2 感知进程的 SetWindowPos 使用物理像素坐标，
 * 可绕过 Qt 逻辑坐标在混合缩放多屏下的换算不一致问题，实现 1:1 像素级铺满。
 */
void SnippingOverlay::applyPhysicalGeometry(const QRect& physicalRect)
{
    if (physicalRect.isNull()) return;
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (!hwnd) return;
    SetWindowPos(hwnd, HWND_TOPMOST,
                 physicalRect.x(), physicalRect.y(),
                 physicalRect.width(), physicalRect.height(),
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
#else
    setGeometry(physicalRect);
#endif
}

/**
 * @brief 当前选区对应的全局逻辑矩形
 *
 * 选区保存的是「覆盖窗口局部坐标」，而贴图窗口、长截图边框等顶层窗口
 * 需要「全局坐标」才能正确定位，因此在对外抛出前统一换算一次。
 */
QRect SnippingOverlay::globalSelectionRect() const
{
    if (m_selectionRect.isNull() || !m_selectionRect.isValid()) return QRect();
    const QRect norm = m_selectionRect.normalized();
    QRect global(norm.topLeft(), norm.size());
    global.moveTopLeft(mapToGlobal(norm.topLeft()));
    return global;
}

/**
 * @brief 当前选区对应的物理像素矩形 (相对物理虚拟桌面原点)
 *
 * 用于跨屏精确采样 (长截图实时抓帧) 等必须使用真实物理像素的场景。
 */
QRect SnippingOverlay::physicalSelectionRect() const
{
    if (m_selectionRect.isNull() || !m_selectionRect.isValid()) return QRect();
    if (m_snapshot.fullSnapshot.isNull()) return QRect();

    const QRect norm = m_selectionRect.normalized();
    const qreal dpr = m_snapshot.fullSnapshot.devicePixelRatio();
    if (dpr <= 0.0) return QRect();

    const QRect phys = m_snapshot.physicalVirtualRect;
    return QRect(phys.x() + static_cast<int>(std::round(norm.x() * dpr)),
                 phys.y() + static_cast<int>(std::round(norm.y() * dpr)),
                 static_cast<int>(std::round(norm.width() * dpr)),
                 static_cast<int>(std::round(norm.height() * dpr)));
}

SnippingOverlay::SnippingOverlay(QWidget* parent)
    : QWidget(parent)
{
    // 设置无边框、置顶、工具悬浮窗属性，不产生任务栏孤立图标
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    // 挂载悬浮放大镜挂件
    m_magnifier = new MagnifierWidget(this);
    m_magnifier->hide();

    // 挂载原位所见即所得文字输入框 (参考 Snipaste / PixPin)
    m_textEditor = new InPlaceTextEditor(this);
    m_textEditor->hide();
    connect(m_textEditor, &InPlaceTextEditor::editingCommitted, this, [this](const QPoint& pos, const QString& text) {
        m_annotationMgr.addItem(std::make_shared<TextAnnotation>(pos, text, m_annotColor, m_annotFontSize));
        update();
    });

    // 挂载原位单字符就地修改气泡编辑器
    m_singleCharEditor = new SingleCharEditor(this);
    m_singleCharEditor->hide();
    connect(m_singleCharEditor, &SingleCharEditor::editingCommitted, this, [this](const SingleCharUnit& unit, const QString& newText, const QColor& color, const QFont& font) {
        qreal dpr = m_snapshot.fullSnapshot.devicePixelRatio();
        if (dpr <= 0.0) dpr = 1.0;
        CharInpaintResult inpaintRes = CharInpainter::inpaintChar(m_snapshot.fullSnapshot.toImage(), unit.physicalBox, 2);
        QPixmap patchPix;
        if (inpaintRes.success && !inpaintRes.patchImage.isNull()) {
            patchPix = QPixmap::fromImage(inpaintRes.patchImage);
            patchPix.setDevicePixelRatio(dpr);
        }
        auto annot = std::make_shared<SingleCharEditAnnotation>(
            unit.logicalBox,
            patchPix,
            newText,
            color,
            font
        );
        m_annotationMgr.addItem(annot);
        update();
    });

    connect(m_singleCharEditor, &SingleCharEditor::requestNavigateNext, this, [this](int currentIndex) {
        int nextIdx = currentIndex + 1;
        if (nextIdx < m_charUnits.size()) {
            qreal dpr = m_snapshot.fullSnapshot.devicePixelRatio();
            auto attrs = CharFontAnalyzer::analyze(m_snapshot.fullSnapshot.toImage(), m_charUnits[nextIdx].physicalBox, m_charUnits[nextIdx].character, dpr);
            m_charUnits[nextIdx].estimatedFgColor = attrs.fgColor;
            m_charUnits[nextIdx].estimatedBgColor = attrs.bgColor;
            m_charUnits[nextIdx].estimatedFontSize = attrs.fontSizePt;
            m_charUnits[nextIdx].estimatedFontFamily = attrs.fontFamily;
            m_charUnits[nextIdx].isBold = attrs.isBold;
            m_hoveredCharIndex = nextIdx;
            m_singleCharEditor->startEdit(m_charUnits[nextIdx], rect());
            update();
        }
    });

    connect(m_singleCharEditor, &SingleCharEditor::requestNavigatePrev, this, [this](int currentIndex) {
        int prevIdx = currentIndex - 1;
        if (prevIdx >= 0) {
            qreal dpr = m_snapshot.fullSnapshot.devicePixelRatio();
            auto attrs = CharFontAnalyzer::analyze(m_snapshot.fullSnapshot.toImage(), m_charUnits[prevIdx].physicalBox, m_charUnits[prevIdx].character, dpr);
            m_charUnits[prevIdx].estimatedFgColor = attrs.fgColor;
            m_charUnits[prevIdx].estimatedBgColor = attrs.bgColor;
            m_charUnits[prevIdx].estimatedFontSize = attrs.fontSizePt;
            m_charUnits[prevIdx].estimatedFontFamily = attrs.fontFamily;
            m_charUnits[prevIdx].isBold = attrs.isBold;
            m_hoveredCharIndex = prevIdx;
            m_singleCharEditor->startEdit(m_charUnits[prevIdx], rect());
            update();
        }
    });

    // 挂载自适应吸附工具栏
    m_toolbar = new FloatingToolbar(this);
    m_toolbar->hide();

    // 颜色选择信号 → 同步标注颜色 + 文字编辑器颜色
    connect(m_toolbar, &FloatingToolbar::colorSelected, this, [this](const QColor& color) {
        m_annotColor = color;
        if (m_textEditor) m_textEditor->setTextColor(color);
    });

    // 粗细/字号选择信号 → 同步标注线宽 + 文字编辑器字号
    connect(m_toolbar, &FloatingToolbar::thicknessSelected, this, [this](int strokeWidth, int fontSize) {
        m_annotStrokeWidth = strokeWidth;
        m_annotFontSize = fontSize;
        if (m_textEditor) m_textEditor->setFontSize(fontSize);
    });

    connect(m_toolbar, &FloatingToolbar::actionTriggered, this, [this](ToolAction action) {
        if (action == ToolAction::Confirm) {
            triggerConfirmAction();
        } else if (action == ToolAction::Cancel) {
            triggerCancelAction();
        } else if (action == ToolAction::Pin) {
            triggerPinAction();
        } else if (action == ToolAction::Save) {
            triggerSaveAction();
        } else if (action == ToolAction::Undo) {
            triggerUndoAction();
        } else if (action == ToolAction::Ocr) {
            triggerOcrAction();
        } else if (action == ToolAction::FormulaOcr) {
            triggerFormulaOcrAction();
        } else if (action == ToolAction::ScrollCapture) {
            startScrollCapture();
        } else if (action == ToolAction::GifRecord) {
            triggerGifRecord();
        } else {
            selectTool(action);
        }
    });
}

SnippingOverlay& SnippingOverlay::instance()
{
    static SnippingOverlay overlay;
    return overlay;
}

QPixmap SnippingOverlay::renderSelectedArea()
{
    if (m_selectionRect.isNull() || !m_selectionRect.isValid()) {
        return QPixmap();
    }

    QRect norm = m_selectionRect.normalized();
    qreal dpr = m_snapshot.fullSnapshot.devicePixelRatio();
    if (dpr <= 0.0) dpr = 1.0;

    // 将逻辑选区换算为物理像素坐标切图，保证 1:1 绝对清晰且无放大失真
    QRect physicalNorm(
        static_cast<int>(std::round(norm.x() * dpr)),
        static_cast<int>(std::round(norm.y() * dpr)),
        static_cast<int>(std::round(norm.width() * dpr)),
        static_cast<int>(std::round(norm.height() * dpr))
    );

    QPixmap cropped = m_snapshot.fullSnapshot.copy(physicalNorm);
    cropped.setDevicePixelRatio(dpr);
    QImage base = cropped.toImage();
    base.setDevicePixelRatio(dpr);

    // 渲染标注 (马赛克与矢量图元统一绘制)
    QPainter painter(&base);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(-norm.topLeft());
    m_annotationMgr.renderAnnotations(painter);
    painter.end();

    QPixmap result = QPixmap::fromImage(base);
    result.setDevicePixelRatio(dpr);
    return result;
}

void SnippingOverlay::startSnipping()
{
    // 防止重复唤醒叠加截图导致遮罩越来越黑
    if (isVisible()) {
        return;
    }

    // ============================================================
    // 步骤 0：让覆盖窗口以「物理像素」精确铺满整块虚拟桌面
    // 【多屏混合缩放关键修复】
    //   实测结论（Windows + Qt6 Per-Monitor-V2）：
    //     - Qt 的 setGeometry() 走「全局逻辑坐标」，在混合缩放多屏下
    //       位置与尺寸换算基准不一致，会被 Windows 钳制，产生
    //       「屏幕缩进 / 内容放大 / 另一侧黑边」的错位；
    //     - 而 Win32 SetWindowPos() 对 Per-Monitor-V2 进程使用
    //       「物理像素」坐标，可 100% 精确铺满真实虚拟桌面。
    //   因此统一用 SetWindowPos 铺满物理桌面，再按窗口实际渲染 DPR
    //   合成物理像素画布，保证屏幕内容与真实显示器 1:1 像素级对齐。
    // ============================================================
    const QRect physVirt = ScreenCapturer::physicalVirtualGeometry();
    if (physVirt.isNull()) {
        qWarning() << "[SnippingOverlay] 无法获取物理虚拟桌面矩形";
        return;
    }

    // 探测阶段先重置会话状态，避免 processEvents 期间到达的按键/鼠标
    // 命中上一轮会话的残留状态 (例如 Esc 会直接把遮罩关掉)
    m_state = SnippingState::Idle;
    m_selectionRect = QRect();
    m_smartCandidates.clear();
    m_candidateIndex = 0;
    m_currentTool = ToolAction::Cancel;
    m_annotationMgr.clear();
    m_isDrawingAnnotation = false;
    m_activeHandle = HandleType::None;
    if (m_textEditor) m_textEditor->hide();
    if (m_singleCharEditor) m_singleCharEditor->hide();
    m_charUnits.clear();
    m_hoveredCharIndex = -1;
    m_ocrRunningForSelection = false;

    // 探测阶段：完全透明显示，避免用户看到闪烁
    m_initializing = true;
    setWindowOpacity(0.0);
    show();
    QApplication::processEvents();

    // 以物理像素精确铺满虚拟桌面
    applyPhysicalGeometry(physVirt);
    QApplication::processEvents();

    // 读取窗口「真实客户区」的物理矩形与渲染 DPR
    // (以实际客户区为准，而非请求值 —— Windows 可能对窗口做尺寸钳制)
    QRect clientRect = clientPhysicalRect(this);
    const QRect captureArea = clientRect.isNull() ? physVirt : clientRect;

    qreal winDpr = devicePixelRatio();
    if (width() > 0) {
        qreal derived = static_cast<qreal>(captureArea.width()) / static_cast<qreal>(width());
        if (derived > 0.0) winDpr = derived;
    }

    // 隐藏探测窗口，避免被后续截图捕获
    hide();
    setWindowOpacity(1.0);
    m_initializing = false;
    QApplication::processEvents();

    // 1. 获取全屏快照 (按窗口实际渲染 DPR 合成物理像素画布)
    m_snapshot = ScreenCapturer::captureVirtualScreen(winDpr, captureArea);
    if (!m_snapshot.isValid) {
        qWarning() << "[SnippingOverlay] 快照截取失败，无法唤醒遮罩";
        return;
    }

    // 从配置读取智能吸附开关
    QSettings& settings = appSettings();
    m_enableSmartSnapping = settings.value("Snapping/EnableSmartSnapping", true).toBool();

    // 2. 以物理像素精确覆盖整个虚拟屏幕 (窗口显示后再次校正，确保不被 Windows 钳制)
    show();
    applyPhysicalGeometry(captureArea);
    QApplication::processEvents();
    activateWindow();
    raise();

    // 复核窗口最终客户区与渲染 DPR，确保与画布覆盖区域完全一致
    {
        QRect finalClient = clientPhysicalRect(this);
        if (!finalClient.isNull() && finalClient.size() != captureArea.size()) {
            qWarning() << "[SnippingOverlay] 窗口客户区与画布基准不一致:"
                       << finalClient << "vs" << captureArea;
        }
        qreal finalDpr = width() > 0 ? static_cast<qreal>(captureArea.width()) / static_cast<qreal>(width()) : 0.0;
        if (finalDpr > 0.0 && qAbs(finalDpr - winDpr) > 0.01) {
            qWarning() << "[SnippingOverlay] 窗口渲染 DPR 发生变化:" << finalDpr << "vs" << winDpr;
        }
    }

    // 3. 后台图像边缘金字塔下采样与轮廓解析 (传入 dpr 自动转换为逻辑坐标候选框)
    qreal dpr = m_snapshot.fullSnapshot.devicePixelRatio();
    m_snapper.processSnapshot(m_snapshot.fullSnapshot.toImage(), dpr);

    // 4. 唤醒放大镜挂件
    QPoint initialPos = mapFromGlobal(QCursor::pos());
    if (m_enableSmartSnapping) {
        m_smartCandidates = m_snapper.findCandidatesAt(initialPos);
    } else {
        m_smartCandidates.clear();
    }
    if (m_magnifier) {
        m_magnifier->updatePosition(initialPos, m_snapshot.fullSnapshot, rect());
        m_magnifier->show();
    }

    update();
}

void SnippingOverlay::startFullScreenCapture()
{
    // 一键全屏截图：直接抓取整个虚拟桌面 (含多显示器) 并写入剪贴板
    if (isVisible()) {
        return;   // 已在截屏模式中，避免叠加遮罩
    }

    // 物理像素 1:1 采集整块虚拟桌面
    m_snapshot = ScreenCapturer::captureVirtualScreen(1.0, ScreenCapturer::physicalVirtualGeometry());
    if (!m_snapshot.isValid) {
        qWarning() << "[SnippingOverlay] 全屏截图失败：快照无效";
        return;
    }

    QImage fullImage = m_snapshot.fullSnapshot.toImage();
    if (fullImage.isNull()) {
        qWarning() << "[SnippingOverlay] 全屏截图失败：图像为空";
        return;
    }

    QApplication::clipboard()->setPixmap(m_snapshot.fullSnapshot);

    qDebug() << "[SnippingOverlay] 一键全屏截图完成，已复制到剪贴板，尺寸:"
             << m_snapshot.fullSnapshot.size()
             << "DPR:" << m_snapshot.fullSnapshot.devicePixelRatio();
}

HandleType SnippingOverlay::getHandleAt(const QPoint& pt) const
{
    if (m_selectionRect.isNull() || !m_selectionRect.isValid()) {
        return HandleType::None;
    }

    QRect norm = m_selectionRect.normalized();
    const int half = HANDLE_SIZE / 2;

    auto makeHandleRect = [half](int x, int y) {
        return QRect(x - half, y - half, HANDLE_SIZE, HANDLE_SIZE);
    };

    if (makeHandleRect(norm.left(), norm.top()).contains(pt)) return HandleType::TopLeft;
    if (makeHandleRect(norm.center().x(), norm.top()).contains(pt)) return HandleType::Top;
    if (makeHandleRect(norm.right(), norm.top()).contains(pt)) return HandleType::TopRight;
    if (makeHandleRect(norm.right(), norm.center().y()).contains(pt)) return HandleType::Right;
    if (makeHandleRect(norm.right(), norm.bottom()).contains(pt)) return HandleType::BottomRight;
    if (makeHandleRect(norm.center().x(), norm.bottom()).contains(pt)) return HandleType::Bottom;
    if (makeHandleRect(norm.left(), norm.bottom()).contains(pt)) return HandleType::BottomLeft;
    if (makeHandleRect(norm.left(), norm.center().y()).contains(pt)) return HandleType::Left;

    if (norm.contains(pt)) return HandleType::Inside;

    return HandleType::None;
}

void SnippingOverlay::updateCursorForHandle(HandleType handle)
{
    switch (handle) {
    case HandleType::TopLeft:
    case HandleType::BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case HandleType::TopRight:
    case HandleType::BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case HandleType::Top:
    case HandleType::Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case HandleType::Left:
    case HandleType::Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case HandleType::Inside:
        setCursor(Qt::SizeAllCursor);
        break;
    default:
        setCursor(Qt::CrossCursor);
        break;
    }
}

void SnippingOverlay::resizeSelection(const QPoint& pt)
{
    QRect norm = m_selectionRect.normalized();

    switch (m_activeHandle) {
    case HandleType::TopLeft:
        norm.setTopLeft(pt);
        break;
    case HandleType::Top:
        norm.setTop(pt.y());
        break;
    case HandleType::TopRight:
        norm.setTopRight(pt);
        break;
    case HandleType::Right:
        norm.setRight(pt.x());
        break;
    case HandleType::BottomRight:
        norm.setBottomRight(pt);
        break;
    case HandleType::Bottom:
        norm.setBottom(pt.y());
        break;
    case HandleType::BottomLeft:
        norm.setBottomLeft(pt);
        break;
    case HandleType::Left:
        norm.setLeft(pt.x());
        break;
    default:
        break;
    }

    m_selectionRect = norm.normalized();
}

void SnippingOverlay::paintEvent(QPaintEvent* /*event*/)
{
    if (!m_snapshot.isValid) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. 绘制底图快照
    //    源矩形使用「设备像素」坐标，目标是控件客户区：
    //    这样即便 Windows 在显示过程中二次调整了窗口 DPR，画布依旧与客户区严格 1:1，
    //    不会出现缩放、错位或黑边。
    painter.drawPixmap(rect(), m_snapshot.fullSnapshot, QRectF(m_snapshot.fullSnapshot.rect()));

    // 2. 绘制半透明遮罩
    QColor maskColor(0, 0, 0, 110);
    if (m_selectionRect.isNull() || !m_selectionRect.isValid()) {
        // 空闲状态且有智能候选框时：使用 QPainterPath 单次镂空候选框，严禁二次叠加 drawPixmap 造成画面错位和放大
        if (m_state == SnippingState::Idle && !m_smartCandidates.empty() && m_candidateIndex < m_smartCandidates.size()) {
            QRect candidate = m_smartCandidates[m_candidateIndex];

            QPainterPath maskPath;
            maskPath.setFillRule(Qt::OddEvenFill);
            maskPath.addRect(rect());
            maskPath.addRect(candidate);
            painter.fillPath(maskPath, maskColor);

            // 绘制高亮虚线候选框
            painter.setRenderHint(QPainter::Antialiasing, false);
            QPen candidatePen(QColor(0, 210, 255), 2, Qt::DashLine);
            painter.setPen(candidatePen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(candidate);
            painter.setRenderHint(QPainter::Antialiasing, true);
        } else {
            // 全屏暗色遮罩
            painter.fillRect(rect(), maskColor);
        }
    } else {
        // 已有选区：使用 QPainterPath 单通道镂空，无拼接缝
        QRect norm = m_selectionRect.normalized();
        QPainterPath maskPath;
        maskPath.setFillRule(Qt::OddEvenFill);
        maskPath.addRect(rect());
        maskPath.addRect(norm);
        painter.fillPath(maskPath, maskColor);

        // 3. 绘制矢量标注图元
        painter.save();
        painter.setClipRect(norm);
        m_annotationMgr.renderAnnotations(painter);

        // 绘制正在拖拽绘制中的图元实时预览 (矩形/箭头/画笔/马赛克)
        if (m_isDrawingAnnotation) {
            if (m_currentTool == ToolAction::Rect) {
                painter.setPen(QPen(m_annotColor, m_annotStrokeWidth));
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(QRect(m_annotStartPos, m_currentAnnotMousePos).normalized());
            } else if (m_currentTool == ToolAction::Arrow) {
                ArrowAnnotation tempArrow(m_annotStartPos, m_currentAnnotMousePos, m_annotColor, m_annotStrokeWidth);
                tempArrow.draw(painter);
            } else if (m_currentTool == ToolAction::Mosaic) {
                QRect previewRect = QRect(m_annotStartPos, m_currentAnnotMousePos).normalized();
                painter.setPen(QPen(QColor(0, 180, 255), 1.5, Qt::DashLine));
                painter.setBrush(QColor(255, 255, 255, 50));
                painter.drawRect(previewRect);
            } else if (m_currentTool == ToolAction::Pencil && m_currentPencil) {
                m_currentPencil->draw(painter);
            }
        }
        painter.restore();

        // 4. 绘制选区高亮边框 (关闭抗锯齿，确保边框精准对齐物理像素，不向外溢色)
        painter.setRenderHint(QPainter::Antialiasing, false);
        QPen borderPen(QColor(0, 120, 215), 2, Qt::SolidLine);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(norm);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 绘制单字符就地修改模式下的高亮磁吸字符框
        if (m_currentTool == ToolAction::CharEdit && m_hoveredCharIndex >= 0 && m_hoveredCharIndex < m_charUnits.size()) {
            QRect hBox = m_charUnits[m_hoveredCharIndex].logicalBox;
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing, true);
            painter.setPen(QPen(QColor(0, 150, 255, 230), 1.5, Qt::SolidLine));
            painter.setBrush(QColor(56, 189, 248, 50));
            painter.drawRoundedRect(hBox.adjusted(-1, -1, 1, 1), 3, 3);
            painter.restore();
        }

        // 5. 绘制 8 个几何控制手柄 (选区固定状态下显示)
        if (m_state == SnippingState::Selected && m_currentTool == ToolAction::Cancel) {
            painter.setBrush(Qt::white);
            painter.setPen(QPen(QColor(0, 120, 215), 1));
            const int half = HANDLE_SIZE / 2;

            QPoint pts[8] = {
                norm.topLeft(),
                QPoint(norm.center().x(), norm.top()),
                norm.topRight(),
                QPoint(norm.right(), norm.center().y()),
                norm.bottomRight(),
                QPoint(norm.center().x(), norm.bottom()),
                norm.bottomLeft(),
                QPoint(norm.left(), norm.center().y())
            };

            for (const auto& p : pts) {
                painter.drawRect(p.x() - half, p.y() - half, HANDLE_SIZE, HANDLE_SIZE);
            }
        }
    }
}

void SnippingOverlay::mousePressEvent(QMouseEvent* event)
{
    // 初始化探测阶段忽略一切用户输入
    if (m_initializing) {
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        if (m_state == SnippingState::Selected) {
            // 如果处于标注工具模式，优先进入标注绘制
            if (m_currentTool != ToolAction::Cancel && m_selectionRect.normalized().contains(event->pos())) {
                if (m_currentTool == ToolAction::Text) {
                    if (m_textEditor) {
                        m_textEditor->startEdit(event->pos());
                    }
                    return;
                }
                if (m_currentTool == ToolAction::CharEdit) {
                    int clickIndex = CharSegmentation::findCharUnitAt(m_charUnits, event->pos());
                    if (clickIndex == -1) {
                        clickIndex = m_hoveredCharIndex;
                    }
                    if (clickIndex >= 0 && clickIndex < m_charUnits.size()) {
                        m_hoveredCharIndex = clickIndex;
                        qreal dpr = m_snapshot.fullSnapshot.devicePixelRatio();
                        if (dpr <= 0.0) dpr = 1.0;
                        auto attrs = CharFontAnalyzer::analyze(m_snapshot.fullSnapshot.toImage(), m_charUnits[clickIndex].physicalBox, m_charUnits[clickIndex].character, dpr);
                        m_charUnits[clickIndex].estimatedFgColor = attrs.fgColor;
                        m_charUnits[clickIndex].estimatedBgColor = attrs.bgColor;
                        m_charUnits[clickIndex].estimatedFontSize = attrs.fontSizePt;
                        m_charUnits[clickIndex].estimatedFontFamily = attrs.fontFamily;
                        m_charUnits[clickIndex].isBold = attrs.isBold;

                        if (m_singleCharEditor) {
                            m_singleCharEditor->startEdit(m_charUnits[clickIndex], rect());
                        }
                    }
                    return;
                }
                m_isDrawingAnnotation = true;
                m_annotStartPos = event->pos();
                m_currentAnnotMousePos = event->pos();
                if (m_currentTool == ToolAction::Pencil) {
                    m_currentPencil = std::make_shared<PencilAnnotation>(m_annotColor, m_annotStrokeWidth);
                    m_currentPencil->addPoint(event->pos());
                }
                update();
                return;
            }

            m_activeHandle = getHandleAt(event->pos());
            if (m_activeHandle == HandleType::Inside) {
                m_moveOffset = event->pos() - m_selectionRect.topLeft();
            } else if (m_activeHandle == HandleType::None) {
                m_dragStartPos = event->pos();
                m_selectionRect = QRect(m_dragStartPos, QSize(0, 0));
                m_state = SnippingState::Selecting;
                if (m_toolbar) m_toolbar->hide();
            }
            update();
            return;
        }

        if (m_state == SnippingState::Idle) {
            m_dragStartPos = event->pos();
            m_selectionRect = QRect(m_dragStartPos, QSize(0, 0));
            m_state = SnippingState::Selecting;
            if (m_magnifier) m_magnifier->hide();
            if (m_toolbar) m_toolbar->hide();
            update();
            return;
        }
    } else if (event->button() == Qt::RightButton) {
        if (m_state == SnippingState::Selected) {
            if (m_currentTool != ToolAction::Cancel) {
                m_currentTool = ToolAction::Cancel;
                setCursor(Qt::ArrowCursor);
                update();
                return;
            }
            m_selectionRect = QRect();
            m_state = SnippingState::Idle;
            if (m_toolbar) m_toolbar->hide();
            m_smartCandidates = m_snapper.findCandidatesAt(event->pos());
            m_candidateIndex = 0;
            if (m_magnifier) {
                m_magnifier->updatePosition(event->pos(), m_snapshot.fullSnapshot, rect());
                m_magnifier->show();
            }
            updateCursorForHandle(HandleType::None);
            update();
        } else {
            if (m_magnifier) m_magnifier->hide();
            if (m_toolbar) m_toolbar->hide();
            hide();
            emit snippingCancelled();
        }
    }
}

void SnippingOverlay::mouseMoveEvent(QMouseEvent* event)
{
    // 初始化探测阶段忽略一切用户输入
    if (m_initializing) {
        event->accept();
        return;
    }

    if (m_state == SnippingState::Selecting) {
        m_selectionRect = QRect(m_dragStartPos, event->pos()).normalized();
        update();
    } else if (m_state == SnippingState::Selected) {
        if (event->buttons() & Qt::LeftButton) {
            if (m_isDrawingAnnotation) {
                m_currentAnnotMousePos = event->pos();
                if (m_currentTool == ToolAction::Pencil && m_currentPencil) {
                    m_currentPencil->addPoint(event->pos());
                }
                update();
                return;
            }

            if (m_activeHandle == HandleType::Inside) {
                QPoint newTopLeft = event->pos() - m_moveOffset;
                m_selectionRect.moveTo(newTopLeft);
            } else if (m_activeHandle != HandleType::None) {
                resizeSelection(event->pos());
            }
            if (m_toolbar) {
                m_toolbar->updatePosition(m_selectionRect, rect());
            }
            update();
        } else {
            if (m_currentTool == ToolAction::Cancel) {
                HandleType handle = getHandleAt(event->pos());
                updateCursorForHandle(handle);
            } else if (m_currentTool == ToolAction::CharEdit) {
                int idx = CharSegmentation::findCharUnitAt(m_charUnits, event->pos());
                if (idx != m_hoveredCharIndex) {
                    m_hoveredCharIndex = idx;
                    update();
                }
            }
        }
    } else if (m_state == SnippingState::Idle) {
        if (m_enableSmartSnapping) {
            m_smartCandidates = m_snapper.findCandidatesAt(event->pos());
            m_candidateIndex = 0;
        } else {
            m_smartCandidates.clear();
        }
        if (m_magnifier) {
            m_magnifier->updatePosition(event->pos(), m_snapshot.fullSnapshot, rect());
            if (!m_magnifier->isVisible()) {
                m_magnifier->show();
            }
        }
        update();
    }
}

void SnippingOverlay::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_state == SnippingState::Selecting) {
            int dist = (event->pos() - m_dragStartPos).manhattanLength();
            if (dist <= 4) {
                // 单击：吸附智能候选框 (仅在开启智能吸附且有候选时吸附)
                if (m_enableSmartSnapping && !m_smartCandidates.empty() && m_candidateIndex < m_smartCandidates.size()) {
                    m_selectionRect = m_smartCandidates[m_candidateIndex];
                    m_state = SnippingState::Selected;
                    if (m_magnifier) m_magnifier->hide();
                    if (m_toolbar) {
                        m_toolbar->updatePosition(m_selectionRect, rect());
                        m_toolbar->show();
                    }
                    detectCharsInSelection();
                } else {
                    m_selectionRect = QRect();
                    m_state = SnippingState::Idle;
                    if (m_toolbar) m_toolbar->hide();
                    if (m_magnifier) {
                        m_magnifier->updatePosition(event->pos(), m_snapshot.fullSnapshot, rect());
                        m_magnifier->show();
                    }
                }
            } else {
                // 拖拽：建立手动选区
                m_selectionRect = QRect(m_dragStartPos, event->pos()).normalized();
                if (m_selectionRect.width() > 3 && m_selectionRect.height() > 3) {
                    m_state = SnippingState::Selected;
                    if (m_magnifier) m_magnifier->hide();
                    if (m_toolbar) {
                        m_toolbar->updatePosition(m_selectionRect, rect());
                        m_toolbar->show();
                    }
                    detectCharsInSelection();
                } else {
                    m_selectionRect = QRect();
                    m_state = SnippingState::Idle;
                    if (m_toolbar) m_toolbar->hide();
                    m_smartCandidates = m_snapper.findCandidatesAt(event->pos());
                    m_candidateIndex = 0;
                    if (m_magnifier) {
                        m_magnifier->updatePosition(event->pos(), m_snapshot.fullSnapshot, rect());
                        m_magnifier->show();
                    }
                }
            }
            update();
        } else if (m_state == SnippingState::Selected) {
            if (m_isDrawingAnnotation) {
                if (m_currentTool == ToolAction::Rect) {
                    m_annotationMgr.addItem(std::make_shared<RectAnnotation>(
                        QRect(m_annotStartPos, event->pos()).normalized(), m_annotColor, m_annotStrokeWidth));
                } else if (m_currentTool == ToolAction::Arrow) {
                    m_annotationMgr.addItem(std::make_shared<ArrowAnnotation>(
                        m_annotStartPos, event->pos(), m_annotColor, m_annotStrokeWidth));
                } else if (m_currentTool == ToolAction::Pencil) {
                    if (m_currentPencil) {
                        m_annotationMgr.addItem(m_currentPencil);
                        m_currentPencil.reset();
                    }
                } else if (m_currentTool == ToolAction::Mosaic) {
                    QRect mosaicRect = QRect(m_annotStartPos, event->pos()).normalized();
                    if (mosaicRect.width() > 3 && mosaicRect.height() > 3) {
                        m_annotationMgr.addItem(std::make_shared<MosaicAnnotation>(
                            mosaicRect, m_snapshot.fullSnapshot, m_snapshot.fullSnapshot.devicePixelRatio(), 12));
                    }
                }
                m_isDrawingAnnotation = false;
                update();
                return;
            }

            bool wasResizingOrMoving = (m_activeHandle != HandleType::None && m_activeHandle != HandleType::Inside);
            m_activeHandle = HandleType::None;
            if (m_toolbar) {
                m_toolbar->updatePosition(m_selectionRect, rect());
                m_toolbar->show();
            }
            if (wasResizingOrMoving) {
                detectCharsInSelection();
            }
            update();
        }
    }
}

void SnippingOverlay::detectCharsInSelection()
{
    if (m_selectionRect.isNull() || !m_selectionRect.isValid() || m_ocrRunningForSelection) {
        return;
    }
    m_ocrRunningForSelection = true;

    QRect norm = m_selectionRect.normalized();
    qreal dpr = m_snapshot.fullSnapshot.devicePixelRatio();
    if (dpr <= 0.0) dpr = 1.0;

    QRect physicalNorm(
        static_cast<int>(std::round(norm.x() * dpr)),
        static_cast<int>(std::round(norm.y() * dpr)),
        static_cast<int>(std::round(norm.width() * dpr)),
        static_cast<int>(std::round(norm.height() * dpr))
    );

    QImage crop = m_snapshot.fullSnapshot.copy(physicalNorm).toImage();
    OcrManager::instance().recognizeAsync(crop, dpr, [this, physicalNorm, dpr](const OcrResult& result, const QString& /*engineName*/) {
        m_ocrRunningForSelection = false;
        if (!result.success || result.lines.isEmpty()) {
            if (m_currentTool == ToolAction::CharEdit) {
                setCursor(Qt::IBeamCursor);
            }
            update();
            return;
        }

        // 坐标偏移转换回全屏全景物理坐标
        QList<OcrLine> globalLines;
        for (const auto& line : result.lines) {
            OcrLine gLine = line;
            gLine.boundingBox.translate(physicalNorm.topLeft());
            gLine.logicalBox = QRect(
                static_cast<int>(std::round(gLine.boundingBox.x() / dpr)),
                static_cast<int>(std::round(gLine.boundingBox.y() / dpr)),
                static_cast<int>(std::round(gLine.boundingBox.width() / dpr)),
                static_cast<int>(std::round(gLine.boundingBox.height() / dpr))
            );
            for (auto& w : gLine.words) {
                w.boundingBox.translate(physicalNorm.topLeft());
                w.logicalBox = QRect(
                    static_cast<int>(std::round(w.boundingBox.x() / dpr)),
                    static_cast<int>(std::round(w.boundingBox.y() / dpr)),
                    static_cast<int>(std::round(w.boundingBox.width() / dpr)),
                    static_cast<int>(std::round(w.boundingBox.height() / dpr))
                );
            }
            globalLines.append(gLine);
        }

        m_charUnits = CharSegmentation::segmentAllLines(m_snapshot.fullSnapshot.toImage(), globalLines, dpr);

        // 如果用户已处于单字修改模式，立即恢复光标并直接吸附当前鼠标指针下的字符
        if (m_currentTool == ToolAction::CharEdit) {
            setCursor(Qt::IBeamCursor);
            QPoint mousePos = mapFromGlobal(QCursor::pos());
            m_hoveredCharIndex = CharSegmentation::findCharUnitAt(m_charUnits, mousePos);
        }
        update();
    });
}

void SnippingOverlay::selectTool(ToolAction tool)
{
    m_currentTool = tool;
    if (m_toolbar) {
        m_toolbar->setActiveTool(tool);
    }
    if (tool == ToolAction::Text) {
        setCursor(Qt::IBeamCursor);
    } else if (tool == ToolAction::CharEdit) {
        if (m_ocrRunningForSelection) {
            // 后台 OCR 正在分析中，显示等待光标与即时反馈
            setCursor(Qt::WaitCursor);
        } else {
            setCursor(Qt::IBeamCursor);
            if (m_charUnits.isEmpty()) {
                detectCharsInSelection();
            } else {
                QPoint mousePos = mapFromGlobal(QCursor::pos());
                m_hoveredCharIndex = CharSegmentation::findCharUnitAt(m_charUnits, mousePos);
            }
        }
        update();
        return;
    } else {
        setCursor(Qt::CrossCursor);
    }
}

void SnippingOverlay::triggerUndoAction()
{
    if (m_textEditor && m_textEditor->isVisible()) {
        return;
    }
    m_annotationMgr.undo();
    update();
}

void SnippingOverlay::triggerPinAction()
{
    if (hasValidSelection()) {
        QRect norm = m_selectionRect.normalized();
        QPixmap composite = renderSelectedArea();
        if (m_magnifier) m_magnifier->hide();
        if (m_toolbar) m_toolbar->hide();
        hide();
        // 选区是「覆盖窗口局部坐标」，贴图窗口需要「全局坐标」才能落到正确位置
        emit pinRequested(composite, globalSelectionRect());
    }
}

void SnippingOverlay::triggerConfirmAction()
{
    if (hasValidSelection()) {
        if (m_magnifier) m_magnifier->hide();
        if (m_toolbar) m_toolbar->hide();
        QRect norm = m_selectionRect.normalized();
        QPixmap composite = renderSelectedArea();
        QApplication::clipboard()->setPixmap(composite);
        hide();
        emit snippingFinished(composite, norm);
    }
}

void SnippingOverlay::triggerSaveAction()
{
    if (hasValidSelection()) {
        QRect norm = m_selectionRect.normalized();
        QPixmap composite = renderSelectedArea();
        const QString defaultName = QString("Evan_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

        // 统一保存策略：已配置自定义目录则直接落盘, 否则弹窗并记忆上次位置
        const SaveHelper::Result res = SaveHelper::save(
            this, defaultName, QStringLiteral("保存截图"), ImageExporter::getSaveFileFilter(),
            [&composite](const QString& filePath) {
                return ImageExporter::saveImage(composite.toImage(), filePath);
            });

        if (res.saved) {
            if (m_magnifier) m_magnifier->hide();
            if (m_toolbar) m_toolbar->hide();
            hide();
            emit snippingFinished(composite, norm);
        }
    }
}

void SnippingOverlay::triggerOcrAction()
{
    if (hasValidSelection()) {
        QRect norm = m_selectionRect.normalized();
        QPixmap composite = renderSelectedArea();

        // 立即关闭截屏全屏底图遮罩及所有工具挂件，恢复正常桌面
        if (m_magnifier) m_magnifier->hide();
        if (m_toolbar) m_toolbar->hide();
        hide();
        emit snippingFinished(composite, norm);

        auto* dialog = new OcrResultDialog(nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setImage(composite);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();

        // 使用 QPointer 守卫，避免用户在识别完成前关闭弹窗导致野指针崩溃 (软件整体闪退)
        QPointer<OcrResultDialog> guard(dialog);
        OcrManager::instance().recognizeAsync(composite.toImage(), composite.devicePixelRatio(), [guard](const OcrResult& res, const QString& engineName) {
            if (!guard) return;   // 弹窗已被用户关闭，安全丢弃结果
            guard->setResult(res, engineName);
        });
    }
}

void SnippingOverlay::triggerFormulaOcrAction()
{
    if (hasValidSelection()) {
        QRect norm = m_selectionRect.normalized();
        QPixmap composite = renderSelectedArea();

        // 立即关闭截屏全屏底图遮罩及所有工具挂件，恢复正常桌面
        if (m_magnifier) m_magnifier->hide();
        if (m_toolbar) m_toolbar->hide();
        hide();
        emit snippingFinished(composite, norm);

        auto* dialog = new FormulaResultDialog(nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
        dialog->startRecognition(composite, composite.devicePixelRatio());
    }
}

void SnippingOverlay::triggerCancelAction()
{
    if (m_magnifier) m_magnifier->hide();
    if (m_toolbar) m_toolbar->hide();
    hide();
    emit snippingCancelled();
}

void SnippingOverlay::keyPressEvent(QKeyEvent* event)
{
    // 初始化探测阶段忽略一切用户输入，避免误触发取消/确认
    if (m_initializing) {
        event->accept();
        return;
    }

    const auto& c = HotkeyConfig::instance().data();

    // 1. 取消截屏
    if (HotkeyConfig::matches(event, c.snippingCancel) || event->key() == Qt::Key_Escape) {
        triggerCancelAction();
        return;
    }

    // 2. 撤销上一步标注 (修复快捷键)
    if (HotkeyConfig::matches(event, c.snippingUndo) ||
        (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Z)) {
        triggerUndoAction();
        return;
    }

    // 3. 贴图置顶 (修复快捷键)
    if (HotkeyConfig::matches(event, c.snippingPin) || event->key() == Qt::Key_F3) {
        triggerPinAction();
        return;
    }

    // 4. 完成截屏并复制到剪贴板
    if (HotkeyConfig::matches(event, c.snippingConfirm) ||
        event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        triggerConfirmAction();
        return;
    }

    // 5. 保存到本地文件
    if (HotkeyConfig::matches(event, c.snippingSave) ||
        ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_S)) {
        triggerSaveAction();
        return;
    }

    // 6. 文字识别 OCR
    if (HotkeyConfig::matches(event, c.snippingOcr) ||
        ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_O) ||
        (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_O)) {
        triggerOcrAction();
        return;
    }

    // 7. 标注工具切换快捷键 (仅在选区建立后生效)
    if (m_state == SnippingState::Selected && (!m_textEditor || !m_textEditor->isVisible())) {
        if (HotkeyConfig::matches(event, c.toolRect)) {
            selectTool(ToolAction::Rect);
            return;
        }
        if (HotkeyConfig::matches(event, c.toolArrow)) {
            selectTool(ToolAction::Arrow);
            return;
        }
        if (HotkeyConfig::matches(event, c.toolPencil)) {
            selectTool(ToolAction::Pencil);
            return;
        }
        if (HotkeyConfig::matches(event, c.toolText)) {
            selectTool(ToolAction::Text);
            return;
        }
        if (HotkeyConfig::matches(event, c.toolMosaic)) {
            selectTool(ToolAction::Mosaic);
            return;
        }
        if (HotkeyConfig::matches(event, c.toolCharEdit)) {
            selectTool(ToolAction::CharEdit);
            return;
        }
    }

    // 8. 放大镜复制颜色
    if (event->key() == Qt::Key_C && event->modifiers() == Qt::NoModifier) {
        if (m_magnifier && m_magnifier->isVisible()) {
            QString hex = m_magnifier->hexColor();
            QApplication::clipboard()->setText(hex);
            qDebug() << "[SnippingOverlay] 颜色代码已复制到剪贴板:" << hex;
        }
        return;
    }

    // 9. 智能吸附候选框 Tab 切换 (仅在开启智能吸附时生效)
    if (event->key() == Qt::Key_Tab && m_enableSmartSnapping) {
        if (m_state == SnippingState::Idle && !m_smartCandidates.empty()) {
            m_candidateIndex = (m_candidateIndex + 1) % m_smartCandidates.size();
            update();
        }
        return;
    } else if (event->key() == Qt::Key_Backtab && m_enableSmartSnapping) {
        if (m_state == SnippingState::Idle && !m_smartCandidates.empty()) {
            if (m_candidateIndex == 0) {
                m_candidateIndex = m_smartCandidates.size() - 1;
            } else {
                --m_candidateIndex;
            }
            update();
        }
        return;
    }

    QWidget::keyPressEvent(event);
}

void SnippingOverlay::startScrollCapture()
{
    if (!hasValidSelection()) return;

    QRect norm = m_selectionRect.normalized();
    QPixmap initialPix = renderSelectedArea();

    // 1. 关闭全屏暗色遮罩与所有工具条，彻底把桌面和底层窗口的真实交互与滚轮权归还给浏览器/文档
    if (m_magnifier) m_magnifier->hide();
    if (m_toolbar) m_toolbar->hide();
    hide();

    // 2. 启动长截图会话 (显示独立鼠标穿透边框与悬浮控制胶囊，启动20fps实时差分采样缝合)
    if (!m_scrollSession) {
        m_scrollSession = new ScrollCaptureSession(this);
        connect(m_scrollSession, &ScrollCaptureSession::finished, this, [this](const QPixmap& stitchedPix) {
            QApplication::clipboard()->setPixmap(stitchedPix);
            QRect n = m_selectionRect.normalized();
            emit snippingFinished(stitchedPix, n);
        });
        connect(m_scrollSession, &ScrollCaptureSession::cancelled, this, [this]() {
            QRect n = m_selectionRect.normalized();
            emit snippingFinished(QPixmap(), n);
        });
    }

    // 同时传入全局逻辑矩形 (用于边框定位) 与物理像素矩形 (用于跨屏精确采样)
    m_scrollSession->start(globalSelectionRect(), initialPix.toImage(), physicalSelectionRect());
}

void SnippingOverlay::triggerGifRecord()
{
    if (!hasValidSelection()) return;

    // 关闭全屏暗色遮罩与工具栏，将画面控制权交给所见即所得 GIF 录屏会话
    if (m_magnifier) m_magnifier->hide();
    if (m_toolbar) m_toolbar->hide();
    hide();

    if (!m_gifSession) {
        m_gifSession = new GifRecordSession(this);
        connect(m_gifSession, &GifRecordSession::finished, this, [this](const QString&) {
            QRect n = m_selectionRect.normalized();
            emit snippingFinished(QPixmap(), n);
        });
        connect(m_gifSession, &GifRecordSession::cancelled, this, [this]() {
            QRect n = m_selectionRect.normalized();
            emit snippingFinished(QPixmap(), n);
        });
    }

    // 同时传入全局逻辑矩形 (用于边框与控制条定位) 与物理像素矩形 (用于跨屏精确采样)
    m_gifSession->start(globalSelectionRect(), physicalSelectionRect());
}
