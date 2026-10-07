#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QColor>
#include <QList>
#include <QMap>

enum class ToolAction {
    Rect,       // 矩形
    Arrow,      // 箭头
    Pencil,     // 画笔
    Text,       // 文本
    Mosaic,     // 马赛克
    CharEdit,   // 单字修改 (E)
    Ocr,        // 文字识别 (O)
    Undo,       // 撤销
    Pin,        // 贴图
    Save,       // 保存为文件 (Ctrl+S)
    Cancel,     // 取消
    Confirm     // 确定 / 复制到剪贴板
};

class FloatingToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit FloatingToolbar(QWidget* parent = nullptr);
    ~FloatingToolbar() override = default;

    // 自适应重计算工具栏位置 (下方优先 -> 翻转至上方 -> 内嵌至右下角)
    void updatePosition(const QRect& targetRect, const QRect& screenBoundary);

    // 动态刷新按钮 ToolTip 中的快捷键文字
    void refreshTooltips();

    // 动态切换深浅色主题样式
    void updateThemeStyle();

    // 激活并高亮指定标注工具
    void setActiveTool(ToolAction action);

    QColor currentColor() const { return m_currentColor; }
    int currentStrokeWidth() const { return m_currentStrokeWidth; }
    int currentFontSize() const { return m_currentFontSize; }

signals:
    void actionTriggered(ToolAction action);
    void colorSelected(const QColor& color);
    void thicknessSelected(int strokeWidth, int fontSize);

private:
    QPushButton* createToolBtn(const QString& text, const QString& tooltip, ToolAction action);
    void setupColorPalette();
    void updateActiveColorButton(QPushButton* activeBtn, const QColor& color);
    void updateActiveThicknessButton(QPushButton* activeBtn, int strokeWidth, int fontSize);

    QVBoxLayout* m_mainLayout = nullptr;
    QHBoxLayout* m_toolsLayout = nullptr;
    QWidget* m_colorBarWidget = nullptr;
    QHBoxLayout* m_colorsLayout = nullptr;

    QColor m_currentColor = QColor(235, 30, 30);
    int m_currentStrokeWidth = 3;
    int m_currentFontSize = 16;

    QList<QPushButton*> m_colorBtns;
    QList<QPushButton*> m_thicknessBtns;
    QMap<ToolAction, QPushButton*> m_actionBtns;
    ToolAction m_activeTool = ToolAction::Cancel;

    static constexpr int MARGIN = 8;
    static constexpr int PADDING = 6;
};
