#pragma once

#include <QPlainTextEdit>
#include "../../core/app_settings.h"
#include <QKeyEvent>
#include <QFocusEvent>
#include <QSettings>
#include <QFontMetrics>
#include <algorithm>

class InPlaceTextEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit InPlaceTextEditor(QWidget* parent = nullptr)
        : QPlainTextEdit(parent)
    {
        setAttribute(Qt::WA_DeleteOnClose, false);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setLineWrapMode(QPlainTextEdit::NoWrap);

        updatePlaceholderByMode();
        updateEditorStyle();

        connect(this, &QPlainTextEdit::textChanged, this, [this]() {
            adjustEditorSize();
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

    // 0: 模式 B (Enter 确定, Shift+Enter 换行 - 默认)
    // 1: 模式 A (Ctrl+Enter 确定, Enter 换行)
    int commitMode() const
    {
        QSettings& settings = appSettings();
        return settings.value("Annotation/TextCommitMode", 0).toInt();
    }

    void updatePlaceholderByMode()
    {
        if (commitMode() == 1) {
            setPlaceholderText("输入文字 (Enter 换行, Ctrl+Enter 确定)");
        } else {
            setPlaceholderText("输入文字 (Enter 确定, Shift+Enter 换行)");
        }
    }

    void adjustEditorSize()
    {
        QString str = toPlainText();
        QFontMetrics fm(font());

        int maxLineW = 0;
        int lineCount = 1;

        if (str.isEmpty()) {
            maxLineW = fm.horizontalAdvance(placeholderText());
        } else {
            QStringList lines = str.split('\n');
            lineCount = (std::max)(1, static_cast<int>(lines.size()));
            for (const auto& line : lines) {
                maxLineW = (std::max)(maxLineW, fm.horizontalAdvance(line));
            }
        }

        int lineHeight = fm.lineSpacing();
        int neededW = maxLineW + 28;
        int neededH = lineCount * lineHeight + 14;

        resize(qMax(120, neededW), qMax(32, neededH));
    }

    void updateEditorStyle()
    {
        QFont f = font();
        f.setPointSize(m_fontSize);
        f.setBold(true);
        setFont(f);

        setStyleSheet(QString(
            "QPlainTextEdit {"
            "  background-color: rgba(20, 20, 20, 215);"
            "  color: %1;"
            "  border: 1.5px dashed %1;"
            "  border-radius: 4px;"
            "  padding: 4px 6px;"
            "  font-size: %2pt;"
            "  font-weight: bold;"
            "  font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif;"
            "}"
        ).arg(m_currentColor.name()).arg(m_fontSize));

        adjustEditorSize();
    }

    QColor textColor() const { return m_currentColor; }
    int fontSize() const { return m_fontSize; }

    void startEdit(const QPoint& pos)
    {
        m_startPos = pos;
        clear();
        updatePlaceholderByMode();
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
        int mode = commitMode();

        if (event->key() == Qt::Key_Escape) {
            hide();
            emit editingCancelled();
            event->accept();
            return;
        }

        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            bool hasShift = (event->modifiers() & Qt::ShiftModifier);
            bool hasCtrl  = (event->modifiers() & Qt::ControlModifier);

            if (mode == 0) {
                // 模式 B: Shift+Enter 换行, Enter 提交
                if (hasShift) {
                    insertPlainText("\n");
                    event->accept();
                    return;
                } else {
                    commitAndHide();
                    event->accept();
                    return;
                }
            } else {
                // 模式 A: Enter 换行, Ctrl+Enter 提交
                if (hasCtrl) {
                    commitAndHide();
                    event->accept();
                    return;
                } else {
                    insertPlainText("\n");
                    event->accept();
                    return;
                }
            }
        }

        QPlainTextEdit::keyPressEvent(event);
    }

    void focusOutEvent(QFocusEvent* event) override
    {
        QPlainTextEdit::focusOutEvent(event);
        commitAndHide();
    }

private:
    void commitAndHide()
    {
        if (!isVisible()) return;
        QString str = toPlainText().trimmed();
        if (!str.isEmpty()) {
            emit editingCommitted(m_startPos, str);
        } else {
            emit editingCancelled();
        }
        hide();
    }

    QPoint m_startPos;
    QColor m_currentColor = QColor(235, 30, 30);
    int m_fontSize = 16;
};
