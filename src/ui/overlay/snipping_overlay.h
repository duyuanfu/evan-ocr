#pragma once

#include <QWidget>
#include <QRect>
#include <QPoint>
#include "../../core/screen_capturer.h"
#include "../../core/smart_snapper.h"
#include "../../core/ocr/windows_media_ocr.h"
#include "../toolbar/floating_toolbar.h"
#include "../annotation/annotation_manager.h"
#include <vector>

class MagnifierWidget;
class InPlaceTextEditor;

enum class SnippingState {
    Idle,           // 未激活/智能候选吸附中
    Selecting,      // 正在拖拽框选中
    Selected        // 选区已建立，可微调/标注
};

enum class HandleType {
    None,
    TopLeft, Top, TopRight,
    Right, BottomRight, Bottom,
    BottomLeft, Left,
    Inside
};

class SnippingOverlay : public QWidget
{
    Q_OBJECT
public:
    static SnippingOverlay& instance();

    // 启动截屏覆盖
    void startSnipping();

    // 获取当前建立的选区 (基于全景画布的绝对坐标)
    QRect selectedRect() const { return m_selectionRect; }
    ScreenSnapshot currentSnapshot() const { return m_snapshot; }

signals:
    void snippingCancelled();
    void snippingFinished(const QPixmap& croppedPixmap, const QRect& region);
    void pinRequested(const QPixmap& croppedPixmap, const QRect& screenPos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    explicit SnippingOverlay(QWidget* parent = nullptr);
    ~SnippingOverlay() override = default;

    HandleType getHandleAt(const QPoint& pt) const;
    void updateCursorForHandle(HandleType handle);
    void resizeSelection(const QPoint& pt);
    QPixmap renderSelectedArea();

    ScreenSnapshot m_snapshot;
    SnippingState m_state = SnippingState::Idle;
    QPoint m_dragStartPos;
    QRect m_selectionRect;
    MagnifierWidget* m_magnifier = nullptr;
    FloatingToolbar* m_toolbar = nullptr;

    // 8 控制手柄与拖拽位移状态
    HandleType m_activeHandle = HandleType::None;
    QPoint m_moveOffset;
    static constexpr int HANDLE_SIZE = 8;

    // 矢量标注管理器与当前激活工具
    AnnotationManager m_annotationMgr;
    ToolAction m_currentTool = ToolAction::Cancel;
    QPoint m_annotStartPos;
    QPoint m_currentAnnotMousePos;
    bool m_isDrawingAnnotation = false;
    std::shared_ptr<PencilAnnotation> m_currentPencil;
    InPlaceTextEditor* m_textEditor = nullptr;

    // 当前标注颜色 / 线条粗细 / 文字大小 (由工具栏信号同步)
    QColor m_annotColor = QColor(235, 30, 30);
    int m_annotStrokeWidth = 4;
    int m_annotFontSize = 17;

    // 智能选框分析器与候选层级
    SmartSnapper m_snapper;
    std::vector<QRect> m_smartCandidates;
    size_t m_candidateIndex = 0;

    // Windows 原生 WinRT OCR 识别引擎
    WindowsMediaOcrEngine m_ocrEngine;
};
