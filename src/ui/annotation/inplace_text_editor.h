#pragma once

#include <QLineEdit>
#include <QKeyEvent>
#include <QFocusEvent>

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
            int neededW = fontMetrics().horizontalAdvance(str.isEmpty() ? placeholderText() : str) + 24;
            int neededH = fontMetrics().height() + 8;
            resize(qMax(90, neededW), qMax(30, neededH));
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

    QColor textColor() const { return m_currentColor; }
    int fontSize() const { return m_fontSize; }

    void startEdit(const QPoint& pos)
    {
        m_startPos = pos;
        clear();
        updateEditorStyle();
        move(pos);
        show();
        setFocus();
        activateWindow();
    }

    QPoint startPos() const { return m_startPos; }

signals:
    void editingCommitted(const QPoint& pos, const QString& text);
    void editingCancelled();

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            QString str = text().trimmed();
            if (!str.isEmpty()) {
                emit editingCommitted(m_startPos, str);
            } else {
                emit editingCancelled();
            }
            hide();
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
        QString str = text().trimmed();
        if (!str.isEmpty()) {
            emit editingCommitted(m_startPos, str);
        } else {
            emit editingCancelled();
        }
        hide();
    }

private:
    QPoint m_startPos;
    QColor m_currentColor = QColor(235, 30, 30);
    int m_fontSize = 16;
};
