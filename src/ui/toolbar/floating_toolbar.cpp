#include "floating_toolbar.h"
#include "../../core/hotkey_config.h"
#include <QPainter>
#include <QFrame>
#include <QColorDialog>
#include <QGraphicsDropShadowEffect>

FloatingToolbar::FloatingToolbar(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating, true);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(PADDING, PADDING, PADDING, PADDING);
    m_mainLayout->setSpacing(4);

    // 第一行：主要工具按钮
    auto* toolsWidget = new QWidget(this);
    m_toolsLayout = new QHBoxLayout(toolsWidget);
    m_toolsLayout->setContentsMargins(0, 0, 0, 0);
    m_toolsLayout->setSpacing(3);

    createToolBtn("▢", "矩形标注", ToolAction::Rect);
    createToolBtn("➔", "箭头标注", ToolAction::Arrow);
    createToolBtn("✎", "自由画笔", ToolAction::Pencil);
    createToolBtn("T", "文字标注", ToolAction::Text);
    createToolBtn("▚", "马赛克遮罩", ToolAction::Mosaic);
    createToolBtn("✏", "单字修改", ToolAction::CharEdit);
    createToolBtn("🔍", "提取图中文字", ToolAction::Ocr);
    createToolBtn("↺", "撤销上一步", ToolAction::Undo);
    createToolBtn("📌", "贴图置顶", ToolAction::Pin);
    createToolBtn("💾", "保存到文件", ToolAction::Save);
    createToolBtn("✕", "取消截屏", ToolAction::Cancel);
    createToolBtn("✓", "完成并复制到剪贴板", ToolAction::Confirm);

    refreshTooltips();
    connect(&HotkeyConfig::instance(), &HotkeyConfig::configChanged, this, &FloatingToolbar::refreshTooltips);

    m_mainLayout->addWidget(toolsWidget);

    // 第二行：调色板挂件 (仅在激活矩形/箭头/画笔/文字等标注工具时展开)
    setupColorPalette();
    m_mainLayout->addWidget(m_colorBarWidget);

    // 阴影特效
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(16);
    shadow->setColor(QColor(0, 0, 0, 160));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);

    adjustSize();
}

void FloatingToolbar::setupColorPalette()
{
    m_colorBarWidget = new QWidget(this);
    m_colorsLayout = new QHBoxLayout(m_colorBarWidget);
    m_colorsLayout->setContentsMargins(2, 2, 2, 2);
    m_colorsLayout->setSpacing(3);

    // 1. 线条粗细与文字大小三档选择 (Snipaste / PixPin 风格)
    const struct { QString symbol; QString tip; int stroke; int font; } thicknessPresets[] = {
        {"•", "细线条 / 小字号 (2px, 13pt)", 2, 13},
        {"●", "中等线条 / 中字号 (4px, 17pt)", 4, 17},
        {"⬤", "粗线条 / 大字号 (7px, 24pt)", 7, 24}
    };

    for (const auto& t : thicknessPresets) {
        auto* btn = new QPushButton(t.symbol, m_colorBarWidget);
        btn->setToolTip(t.tip);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(18, 18);
        m_thicknessBtns.append(btn);

        int stroke = t.stroke;
        int fontSize = t.font;
        connect(btn, &QPushButton::clicked, this, [this, btn, stroke, fontSize]() {
            updateActiveThicknessButton(btn, stroke, fontSize);
        });

        m_colorsLayout->addWidget(btn);
    }

    // 默认激活“中”档
    if (m_thicknessBtns.size() >= 2) {
        updateActiveThicknessButton(m_thicknessBtns[1], 4, 17);
    }

    // 细微垂直分隔线
    auto* sep = new QFrame(m_colorBarWidget);
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setStyleSheet("QFrame { background-color: #444; width: 1px; margin: 2px 2px; }");
    m_colorsLayout->addWidget(sep);

    // 2. 常用高频颜色预设 (Snipaste / PixPin 风格色谱)
    const struct { QString hex; QString name; } presets[] = {
        {"#eb1e1e", "鲜红"},
        {"#ff8800", "橙色"},
        {"#fadb14", "亮黄"},
        {"#52c41a", "翠绿"},
        {"#1677ff", "天蓝"},
        {"#722ed1", "紫色"},
        {"#ffffff", "纯白"},
        {"#141414", "纯黑"}
    };

    for (const auto& p : presets) {
        QColor col(p.hex);
        auto* btn = new QPushButton(m_colorBarWidget);
        btn->setToolTip(QString("选择%1色").arg(p.name));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(18, 18);

        m_colorBtns.append(btn);

        connect(btn, &QPushButton::clicked, this, [this, btn, col]() {
            updateActiveColorButton(btn, col);
        });

        m_colorsLayout->addWidget(btn);
    }

    // 默认激活鲜红色
    if (!m_colorBtns.isEmpty()) {
        updateActiveColorButton(m_colorBtns[0], QColor("#eb1e1e"));
    }

    // 3. 自定义拾色盘按钮
    auto* customBtn = new QPushButton("🎨", m_colorBarWidget);
    customBtn->setToolTip("自定义取色盘...");
    customBtn->setCursor(Qt::PointingHandCursor);
    customBtn->setFixedSize(22, 18);
    customBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2b2b2b;"
        "  color: #ffffff;"
        "  border: 1px solid #444;"
        "  border-radius: 3px;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover { background-color: #0078d7; border-color: #1e90ff; }"
    );

    connect(customBtn, &QPushButton::clicked, this, [this, customBtn]() {
        QColor chosen = QColorDialog::getColor(m_currentColor, this, "选择标注颜色");
        if (chosen.isValid()) {
            updateActiveColorButton(customBtn, chosen);
        }
    });

    m_colorsLayout->addWidget(customBtn);
    m_colorsLayout->addStretch();

    // 默认隐藏颜色行，点击绘图工具后再展现
    m_colorBarWidget->hide();
}

void FloatingToolbar::updateActiveThicknessButton(QPushButton* activeBtn, int strokeWidth, int fontSize)
{
    m_currentStrokeWidth = strokeWidth;
    m_currentFontSize = fontSize;

    for (auto* btn : m_thicknessBtns) {
        if (btn == activeBtn) {
            btn->setStyleSheet(
                "QPushButton {"
                "  background-color: #0078d7;"
                "  color: #ffffff;"
                "  border: 1px solid #1e90ff;"
                "  border-radius: 3px;"
                "  font-size: 11px;"
                "}"
            );
        } else {
            btn->setStyleSheet(
                "QPushButton {"
                "  background-color: #262626;"
                "  color: #b0b0b0;"
                "  border: 1px solid #3c3c3c;"
                "  border-radius: 3px;"
                "  font-size: 11px;"
                "}"
                "QPushButton:hover {"
                "  background-color: #383838;"
                "  color: #ffffff;"
                "}"
            );
        }
    }

    emit thicknessSelected(m_currentStrokeWidth, m_currentFontSize);
}

void FloatingToolbar::updateActiveColorButton(QPushButton* activeBtn, const QColor& color)
{
    m_currentColor = color;

    // 刷新所有预设按钮样式
    for (int i = 0; i < m_colorBtns.size(); ++i) {
        auto* btn = m_colorBtns[i];
        QString baseHex = (i == 0) ? "#eb1e1e" : (i == 1) ? "#ff8800" : (i == 2) ? "#fadb14" :
                          (i == 3) ? "#52c41a" : (i == 4) ? "#1677ff" : (i == 5) ? "#722ed1" :
                          (i == 6) ? "#ffffff" : "#141414";

        if (btn == activeBtn) {
            btn->setStyleSheet(QString(
                "QPushButton {"
                "  background-color: %1;"
                "  border: 2.5px solid #ffffff;"
                "  border-radius: 9px;"
                "}"
            ).arg(baseHex));
        } else {
            btn->setStyleSheet(QString(
                "QPushButton {"
                "  background-color: %1;"
                "  border: 1px solid rgba(255, 255, 255, 60);"
                "  border-radius: 9px;"
                "}"
                "QPushButton:hover {"
                "  border: 2px solid #ffffff;"
                "}"
            ).arg(baseHex));
        }
    }

    emit colorSelected(m_currentColor);
}

QPushButton* FloatingToolbar::createToolBtn(const QString& symbol, const QString& tooltip, ToolAction action)
{
    auto* btn = new QPushButton(symbol, this);
    btn->setToolTip(tooltip);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFixedSize(28, 28); // 紧凑精致的正方形符号按钮

    // 优化：针对矩形标注“▢”，加大字号与粗细，视觉更醒目饱满
    int fontSize = (symbol == "▢") ? 17 : 13;
    QString fontWeight = (symbol == "▢") ? "900" : "bold";

    btn->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: #262626;"
        "  color: #e6e6e6;"
        "  border: 1px solid #383838;"
        "  border-radius: 4px;"
        "  font-size: %1px;"
        "  font-weight: %2;"
        "  font-family: 'Segoe UI Symbol', 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0078d7;"
        "  border-color: #1e90ff;"
        "  color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #005a9e;"
        "}"
        "QToolTip {"
        "  background-color: #1f1f1f;"
        "  color: #ffffff;"
        "  border: 1px solid #4a4a4a;"
        "  padding: 4px 8px;"
        "  border-radius: 3px;"
        "  font-size: 12px;"
        "}"
    ).arg(fontSize).arg(fontWeight));

    connect(btn, &QPushButton::clicked, this, [this, action]() {
        setActiveTool(action);
        emit actionTriggered(action);
    });

    m_actionBtns.insert(action, btn);
    m_toolsLayout->addWidget(btn);
    return btn;
}

void FloatingToolbar::refreshTooltips()
{
    const auto& c = HotkeyConfig::instance().data();
    auto setTip = [this](ToolAction act, const QString& name, const QString& key) {
        if (m_actionBtns.contains(act)) {
            m_actionBtns[act]->setToolTip(key.isEmpty() ? name : QString("%1 (%2)").arg(name, key));
        }
    };

    setTip(ToolAction::Rect, "矩形标注", c.toolRect);
    setTip(ToolAction::Arrow, "箭头标注", c.toolArrow);
    setTip(ToolAction::Pencil, "自由画笔", c.toolPencil);
    setTip(ToolAction::Text, "文字标注", c.toolText);
    setTip(ToolAction::Mosaic, "马赛克遮罩", c.toolMosaic);
    setTip(ToolAction::CharEdit, "单字修改", c.toolCharEdit);
    setTip(ToolAction::Ocr, "提取图中文字", c.snippingOcr);
    setTip(ToolAction::Undo, "撤销上一步", c.snippingUndo);
    setTip(ToolAction::Pin, "贴图置顶", c.snippingPin);
    setTip(ToolAction::Save, "保存到文件", c.snippingSave);
    setTip(ToolAction::Cancel, "取消截屏", c.snippingCancel);
    setTip(ToolAction::Confirm, "完成并复制到剪贴板", c.snippingConfirm);
}

void FloatingToolbar::setActiveTool(ToolAction action)
{
    m_activeTool = action;

    // 当切换到绘图类工具时展开调色板，其它工具（撤销/贴图/保存/取消/完成/马赛克）收起调色板
    if (action == ToolAction::Rect || action == ToolAction::Arrow ||
        action == ToolAction::Pencil || action == ToolAction::Text) {
        if (m_colorBarWidget && !m_colorBarWidget->isVisible()) {
            m_colorBarWidget->show();
            adjustSize();
        }
    } else {
        if (m_colorBarWidget && m_colorBarWidget->isVisible()) {
            m_colorBarWidget->hide();
            adjustSize();
        }
    }

    // 标注工具选中样式高亮
    for (auto it = m_actionBtns.begin(); it != m_actionBtns.end(); ++it) {
        ToolAction act = it.key();
        QPushButton* btn = it.value();
        bool isDrawingTool = (act == ToolAction::Rect || act == ToolAction::Arrow ||
                              act == ToolAction::Pencil || act == ToolAction::Text ||
                              act == ToolAction::Mosaic);
        if (isDrawingTool) {
            int fontSize = (act == ToolAction::Rect) ? 17 : 13;
            QString fontWeight = (act == ToolAction::Rect) ? "900" : "bold";
            if (act == action) {
                btn->setStyleSheet(QString(
                    "QPushButton {"
                    "  background-color: #0078d7;"
                    "  color: #ffffff;"
                    "  border: 1px solid #1e90ff;"
                    "  border-radius: 4px;"
                    "  font-size: %1px;"
                    "  font-weight: %2;"
                    "  font-family: 'Segoe UI Symbol', 'Segoe UI', 'Microsoft YaHei', sans-serif;"
                    "}"
                ).arg(fontSize).arg(fontWeight));
            } else {
                btn->setStyleSheet(QString(
                    "QPushButton {"
                    "  background-color: #262626;"
                    "  color: #e6e6e6;"
                    "  border: 1px solid #383838;"
                    "  border-radius: 4px;"
                    "  font-size: %1px;"
                    "  font-weight: %2;"
                    "  font-family: 'Segoe UI Symbol', 'Segoe UI', 'Microsoft YaHei', sans-serif;"
                    "}"
                    "QPushButton:hover {"
                    "  background-color: #383838;"
                    "  color: #ffffff;"
                    "}"
                ).arg(fontSize).arg(fontWeight));
            }
        }
    }
}

void FloatingToolbar::updatePosition(const QRect& targetRect, const QRect& screenBoundary)
{
    adjustSize();
    const int tw = width();
    const int th = height();

    // 1. 水平对齐：默认靠右对齐选区右边界
    int targetX = targetRect.right() - tw;
    if (targetX < screenBoundary.left() + MARGIN) {
        targetX = screenBoundary.left() + MARGIN;
    }
    if (targetX + tw > screenBoundary.right() - MARGIN) {
        targetX = screenBoundary.right() - MARGIN - tw;
    }

    // 2. 垂直自适应翻转算法：
    int targetY = targetRect.bottom() + MARGIN;

    if (targetY + th > screenBoundary.bottom() - MARGIN) {
        targetY = targetRect.top() - th - MARGIN;

        if (targetY < screenBoundary.top() + MARGIN) {
            targetY = targetRect.bottom() - th - PADDING - 4;
            targetX = targetRect.right() - tw - PADDING - 4;
        }
    }

    move(targetX, targetY);
}
