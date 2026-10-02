#pragma once

#include <QLineEdit>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QFont>
#include <QFontMetrics>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QWidget>

class InPlaceTextEditor : public QLineEdit
{
    Q_OBJECT
public:
    explicit InPlaceTextEditor(QWidget* parent = nullptr)
        : QLineEdit(parent)
    {
        setAttribute(Qt::WA_DeleteOnClose, false);
        setPlaceholderText("输入文字，按 Enter 确定");

        setupStyleBar();
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
        if (m_styleBar) m_styleBar->hide();

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

        // 同步微调栏状态 (自动根据算法推断的流派预选中)
        syncStyleBarToAttributes();

        updateEditorStyle();
        setGeometry(logicalRect);

        show();
        updateStyleBarPosition();
        if (m_styleBar) m_styleBar->show();

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
            hideAll();
            emit editingCancelled();
            event->accept();
        } else {
            QLineEdit::keyPressEvent(event);
        }
    }

    void focusOutEvent(QFocusEvent* event) override
    {
        QLineEdit::focusOutEvent(event);
        // 如果焦点移到了微调栏上，不关闭
        if (m_styleBar && m_styleBar->hasFocus()) {
            return;
        }
        commitEdit();
    }

private:
    void hideAll()
    {
        hide();
        if (m_styleBar) m_styleBar->hide();
    }

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
        hideAll();
    }

    void setupStyleBar()
    {
        if (!parentWidget()) return;
        m_styleBar = new QWidget(parentWidget());
        m_styleBar->setObjectName("quickStyleBar");
        m_styleBar->setStyleSheet(
            "QWidget#quickStyleBar {"
            "  background-color: #1f1f23;"
            "  border: 1px solid #3f3f46;"
            "  border-radius: 4px;"
            "}"
            "QComboBox {"
            "  background-color: #27272a;"
            "  color: #f4f4f5;"
            "  border: 1px solid #3f3f46;"
            "  border-radius: 3px;"
            "  padding: 1px 6px;"
            "  font-size: 11px;"
            "  font-weight: 500;"
            "}"
            "QComboBox::drop-down { border: none; width: 14px; }"
            "QPushButton {"
            "  background-color: #27272a;"
            "  color: #d4d4d8;"
            "  border: 1px solid #3f3f46;"
            "  border-radius: 3px;"
            "  padding: 2px 6px;"
            "  font-size: 11px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:checked {"
            "  background-color: #2563eb;"
            "  border-color: #3b82f6;"
            "  color: #ffffff;"
            "}"
            "QPushButton:hover:!checked {"
            "  background-color: #3f3f46;"
            "  color: #ffffff;"
            "}"
        );

        auto* layout = new QHBoxLayout(m_styleBar);
        layout->setContentsMargins(4, 2, 4, 2);
        layout->setSpacing(4);

        m_fontCombo = new QComboBox(m_styleBar);
        m_fontCombo->addItem("黑体 (微软雅黑)", "Microsoft YaHei");
        m_fontCombo->addItem("宋体 (SimSun)", "SimSun");
        m_fontCombo->addItem("代码体 (Consolas)", "Consolas");
        m_fontCombo->addItem("楷体 (KaiTi)", "KaiTi");
        m_fontCombo->addItem("西文 (Arial)", "Arial");
        m_fontCombo->setToolTip("快捷切换字体流派");
        connect(m_fontCombo, &QComboBox::currentIndexChanged, this, [this](int idx) {
            if (idx >= 0) {
                m_fontFamily = m_fontCombo->itemData(idx).toString();
                updateEditorStyle();
                setFocus();
            }
        });
        layout->addWidget(m_fontCombo);

        m_boldBtn = new QPushButton("B", m_styleBar);
        m_boldBtn->setCheckable(true);
        m_boldBtn->setFixedSize(20, 20);
        m_boldBtn->setToolTip("切换粗体 / 正常字重");
        connect(m_boldBtn, &QPushButton::toggled, this, [this](bool checked) {
            m_fontWeight = checked ? 700 : 400;
            updateEditorStyle();
            setFocus();
        });
        layout->addWidget(m_boldBtn);

        m_styleBar->adjustSize();
        m_styleBar->hide();
    }

    void syncStyleBarToAttributes()
    {
        if (!m_fontCombo || !m_boldBtn) return;

        m_fontCombo->blockSignals(true);
        int matchIdx = -1;
        for (int i = 0; i < m_fontCombo->count(); ++i) {
            if (m_fontCombo->itemData(i).toString().compare(m_fontFamily, Qt::CaseInsensitive) == 0) {
                matchIdx = i;
                break;
            }
        }
        if (matchIdx >= 0) {
            m_fontCombo->setCurrentIndex(matchIdx);
        } else {
            m_fontCombo->setCurrentIndex(0);
        }
        m_fontCombo->blockSignals(false);

        m_boldBtn->blockSignals(true);
        m_boldBtn->setChecked(m_fontWeight >= 600);
        m_boldBtn->blockSignals(false);
    }

    void updateStyleBarPosition()
    {
        if (!m_styleBar || !parentWidget()) return;
        m_styleBar->adjustSize();

        int barW = m_styleBar->width();
        int barH = m_styleBar->height();

        // 默认放置在输入框上方
        int posX = m_replaceRect.left();
        int posY = m_replaceRect.top() - barH - 4;

        if (posY < 4) {
            // 上方超出屏幕则翻转到下方
            posY = m_replaceRect.bottom() + 4;
        }

        if (posX + barW > parentWidget()->width() - 4) {
            posX = parentWidget()->width() - barW - 4;
        }
        if (posX < 4) posX = 4;

        m_styleBar->move(posX, posY);
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

    // 方案 4：快捷微调悬浮条
    QWidget* m_styleBar = nullptr;
    QComboBox* m_fontCombo = nullptr;
    QPushButton* m_boldBtn = nullptr;
};
