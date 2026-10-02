#pragma once

#include <QLineEdit>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QFont>
#include <QFontMetrics>

class InPlaceTextEditor : public QLineEdit
{
    Q_OBJECT
public:
    explicit InPlaceTextEditor(QWidget* parent = nullptr)
        : QLineEdit(parent)
    {
        setAttribute(Qt::WA_DeleteOnClose, false);
        setPlaceholderText("输入文字，按 Enter 确定");
        updateEditorStyle();

        connect(this, &QLineEdit::textChanged, this, [this](const QString& str) {
            if (!m_isReplaceMode) {
                int neededW = fontMetrics().horizontalAdvance(str.isEmpty() ? placeholderText() : str) + 24;
                int neededH = fontMetrics().height() + 8;
                resize(qMax(90, neededW), qMax(30, neededH));
            }
        });
    }

    void setTextColor(const QColor& color)
    {
        m_currentColor = color;
        updateEditorStyle();
    }

    void setFontSize(int size)
    {
        m_fontSize = size;
        updateEditorStyle();
    }

    void updateEditorStyle()
    {
        if (m_isReplaceMode) {
            QFont f = font();
            f.setPixelSize(m_fontSize);
            f.setWeight(static_cast<QFont::Weight>(m_fontWeight));
            f.setFamily(m_fontFamily);
            setFont(f);

            setStyleSheet(QString(
                "QLineEdit {"
                "  background-color: %1;"
                "  color: %2;"
                "  border: 1.5px solid #2563eb;"
                "  border-radius: 2px;"
                "  padding: 0px 1px;"
                "  margin: 0px;"
                "  font-size: %3px;"
                "  font-family: '%4', 'Segoe UI', 'Microsoft YaHei', sans-serif;"
                "}"
            ).arg(m_currentBgColor.name(), m_currentColor.name(), QString::number(m_fontSize), m_fontFamily));
        } else {
            QFont f = font();
            f.setPointSize(m_fontSize);
            f.setBold(true);
            setFont(f);

            setStyleSheet(QString(
                "QLineEdit {"
                "  background-color: rgba(20, 20, 20, 210);"
                "  color: %1;"
                "  border: 1.5px dashed %1;"
                "  border-radius: 3px;"
                "  padding: 2px 6px;"
                "  font-size: %2pt;"
                "  font-weight: bold;"
                "  font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;"
                "}"
            ).arg(m_currentColor.name()).arg(m_fontSize));

            int neededW = fontMetrics().horizontalAdvance(text().isEmpty() ? placeholderText() : text()) + 24;
            int neededH = fontMetrics().height() + 8;
            resize(qMax(90, neededW), qMax(30, neededH));
        }
    }

    QColor textColor() const { return m_currentColor; }
    int fontSize() const { return m_fontSize; }

    // 普通添加文字标注
    void startEdit(const QPoint& pos)
    {
        m_isReplaceMode = false;
        m_startPos = pos;
        clear();
        updateEditorStyle();
        move(pos);
        show();
        setFocus();
        activateWindow();
    }

    // 原位替换/擦除编辑模式 (无缝拟合原字盒与基线)
    void startReplaceEdit(const QRect& logicalRect,
                          const QString& initialText,
                          const QColor& textColor,
                          const QColor& bgColor,
                          int pixelSize,
                          int fontWeight = 400,
                          const QString& fontFamily = "Microsoft YaHei",
                          int baselineY = 0)
    {
        m_isReplaceMode = true;
        m_replaceRect = logicalRect;
        m_currentColor = textColor;
        m_currentBgColor = bgColor;
        m_fontSize = (std::max)(10, pixelSize);
        m_fontWeight = fontWeight;
        m_fontFamily = fontFamily;
        m_baselineY = baselineY;

        setText(initialText);
        updateEditorStyle();

        // 输入框直接完全严密覆盖在矫正后的字盒矩形上
        setGeometry(logicalRect);

        show();
        setFocus();
        selectAll(); // 选中全部初始文字，敲击键盘即可一秒直接替换！
        activateWindow();
    }

    QPoint startPos() const { return m_startPos; }
    QRect replaceRect() const { return m_replaceRect; }
    int baselineY() const { return m_baselineY; }
    bool isReplaceMode() const { return m_isReplaceMode; }

signals:
    void editingCommitted(const QPoint& pos, const QString& text);
    void replaceEditingCommitted(const QRect& logicalRect, const QString& text, const QColor& textColor, int fontSize, int fontWeight, const QString& fontFamily, int baselineY);
    void editingCancelled();

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            commitEdit();
            event->accept();
        } else if (event->key() == Qt::Key_Escape) {
            hide();
            emit editingCancelled();
            event->accept();
        } else {
            QLineEdit::keyPressEvent(event);
        }
    }

    void focusOutEvent(QFocusEvent* event) override
    {
        QLineEdit::focusOutEvent(event);
        commitEdit();
    }

private:
    void commitEdit()
    {
        if (!isVisible()) return;

        QString str = text();
        if (m_isReplaceMode) {
            emit replaceEditingCommitted(m_replaceRect, str, m_currentColor, m_fontSize, m_fontWeight, m_fontFamily, m_baselineY);
        } else {
            if (!str.trimmed().isEmpty()) {
                emit editingCommitted(m_startPos, str);
            } else {
                emit editingCancelled();
            }
        }
        hide();
    }

    QPoint m_startPos;
    QRect m_replaceRect;
    int m_baselineY = 0;
    bool m_isReplaceMode = false;
    QColor m_currentColor = QColor(235, 30, 30);
    QColor m_currentBgColor = QColor(255, 255, 255);
    int m_fontSize = 16;
    int m_fontWeight = 400;
    QString m_fontFamily = "Microsoft YaHei";
};
