#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QColor>
#include <QFont>
#include "../../core/char_edit/char_segmentation.h"

class SingleCharEditor : public QWidget
{
    Q_OBJECT
public:
    explicit SingleCharEditor(QWidget* parent = nullptr);
    ~SingleCharEditor() override = default;

    // 开启单字原位编辑态
    void startEdit(const SingleCharUnit& unit, const QRect& containerBoundary);

    // 获取当前编辑的字符信息与样式
    SingleCharUnit currentUnit() const { return m_unit; }
    QString text() const;
    QColor textColor() const { return m_textColor; }
    QFont currentFont() const { return m_font; }

signals:
    void editingCommitted(const SingleCharUnit& unit, const QString& newText, const QColor& color, const QFont& font);
    void requestNavigateNext(int currentIndex);
    void requestNavigatePrev(int currentIndex);
    void editingCancelled();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUi();
    void updateInputStyle();
    void commitAndClose();
    void cancelAndClose();

    SingleCharUnit m_unit;
    QColor m_textColor = QColor(20, 20, 20);
    QColor m_bgColor = QColor(255, 255, 255);
    QFont m_font;
    int m_fontSizePt = 14;
    bool m_isBold = false;

    QLineEdit* m_lineEdit = nullptr;
    QWidget* m_styleBar = nullptr;
    QComboBox* m_fontCombo = nullptr;
    QPushButton* m_boldBtn = nullptr;
    QPushButton* m_sizeMinusBtn = nullptr;
    QPushButton* m_sizePlusBtn = nullptr;
    QPushButton* m_colorBtn = nullptr;
    QPushButton* m_eraseBtn = nullptr;
};
