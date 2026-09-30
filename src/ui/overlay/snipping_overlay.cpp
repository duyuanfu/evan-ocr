#include "snipping_overlay.h"
#include "../../core/hotkey_config.h"
#include "../magnifier/magnifier_widget.h"
#include "../annotation/inplace_text_editor.h"
#include "../ocr/ocr_result_dialog.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QDateTime>
#include <QDebug>
#include <cmath>

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

    // 1. 获取全屏快照
    m_snapshot = ScreenCapturer::captureVirtualScreen();
    if (!m_snapshot.isValid) {
        qWarning() << "[SnippingOverlay] 快照截取失败，无法唤醒遮罩";
        return;
    }

    m_state = SnippingState::Idle;
    m_selectionRect = QRect();
    m_smartCandidates.clear();
    m_candidateIndex = 0;
    m_currentTool = ToolAction::Cancel;
    m_annotationMgr.clear();
    m_isDrawingAnnotation = false;
    if (m_textEditor) m_textEditor->hide();

    // 2. 覆盖整个虚拟屏幕几何区域
    setGeometry(m_snapshot.virtualGeometry);
    show();
    activateWindow();
    raise();

    // 3. 后台图像边缘金字塔下采样与轮廓解析 (传入 dpr 自动转换为逻辑坐标候选框)
    qreal dpr = m_snapshot.fullSnapshot.devicePixelRatio();
    m_snapper.processSnapshot(m_snapshot.fullSnapshot.toImage(), dpr);

    // 4. 唤醒放大镜挂件
    QPoint initialPos = mapFromGlobal(QCursor::pos());
    m_smartCandidates = m_snapper.findCandidatesAt(initialPos);
    if (m_magnifier) {
        m_magnifier->updatePosition(initialPos, m_snapshot.fullSnapshot, rect());
        m_magnifier->show();
    }

    update();
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
    painter.drawPixmap(0, 0, m_snapshot.fullSnapshot);

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
            }
        }
    } else if (m_state == SnippingState::Idle) {
        m_smartCandidates = m_snapper.findCandidatesAt(event->pos());
        m_candidateIndex = 0;
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
                // 单击：吸附智能候选框
                if (!m_smartCandidates.empty() && m_candidateIndex < m_smartCandidates.size()) {
                    m_selectionRect = m_smartCandidates[m_candidateIndex];
                    m_state = SnippingState::Selected;
                    if (m_magnifier) m_magnifier->hide();
                    if (m_toolbar) {
                        m_toolbar->updatePosition(m_selectionRect, rect());
                        m_toolbar->show();
                    }
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

            m_activeHandle = HandleType::None;
            if (m_toolbar) {
                m_toolbar->updatePosition(m_selectionRect, rect());
                m_toolbar->show();
            }
            update();
        }
    }
}

void SnippingOverlay::selectTool(ToolAction tool)
{
    m_currentTool = tool;
    if (m_toolbar) {
        m_toolbar->setActiveTool(tool);
    }
    if (tool == ToolAction::Text) {
        setCursor(Qt::IBeamCursor);
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
        emit pinRequested(composite, norm);
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
        QString defaultName = QString("Evan_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        QString path = QFileDialog::getSaveFileName(this, "保存截图", defaultName, "PNG 图像 (*.png);;JPEG 图像 (*.jpg);;位图 (*.bmp)");
        if (!path.isEmpty()) {
            composite.save(path);
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

        m_ocrEngine.recognizeAsync(composite.toImage(), composite.devicePixelRatio(), [dialog](const OcrResult& res) {
            dialog->setResult(res);
        });
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

    // 9. 智能吸附候选框 Tab 切换
    if (event->key() == Qt::Key_Tab) {
        if (m_state == SnippingState::Idle && !m_smartCandidates.empty()) {
            m_candidateIndex = (m_candidateIndex + 1) % m_smartCandidates.size();
            update();
        }
        return;
    } else if (event->key() == Qt::Key_Backtab) {
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
