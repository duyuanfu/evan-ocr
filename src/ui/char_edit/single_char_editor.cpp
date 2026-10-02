#include "single_char_editor.h"
#include <QColorDialog>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>
#include <algorithm>

SingleCharEditor::SingleCharEditor(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);

    setupUi();
}

void SingleCharEditor::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    // 1. 单字原位输入框
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setAlignment(Qt::AlignCenter);
    m_lineEdit->installEventFilter(this);
    mainLayout->addWidget(m_lineEdit);

    // 2. 悬浮微型微调栏
    m_styleBar = new QWidget(this);
    m_styleBar->setStyleSheet(
        "QWidget {"
        "  background-color: #1e1e24;"
        "  border: 1px solid #3c3c44;"
        "  border-radius: 5px;"
        "}"
        "QPushButton {"
        "  background-color: #2b2b32;"
        "  color: #f0f0f5;"
        "  border: 1px solid #444450;"
        "  border-radius: 3px;"
        "  padding: 2px 6px;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover { background-color: #0078d7; border-color: #1e90ff; }"
        "QPushButton:checked { background-color: #0078d7; border-color: #1e90ff; color: #fff; }"
        "QComboBox {"
        "  background-color: #2b2b32;"
        "  color: #f0f0f5;"
        "  border: 1px solid #444450;"
        "  border-radius: 3px;"
        "  padding: 1px 4px;"
        "  font-size: 11px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #242428;"
        "  color: #f0f0f5;"
        "  selection-background-color: #0078d7;"
        "}"
    );

    auto* barLayout = new QHBoxLayout(m_styleBar);
    barLayout->setContentsMargins(4, 2, 4, 2);
    barLayout->setSpacing(4);

    // 字体流派下拉
    m_fontCombo = new QComboBox(m_styleBar);
    m_fontCombo->addItem("黑体", "Microsoft YaHei");
    m_fontCombo->addItem("宋体", "SimSun");
    m_fontCombo->addItem("楷体", "KaiTi");
    m_fontCombo->addItem("等宽", "Consolas");
    connect(m_fontCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        QString family = m_fontCombo->itemData(index).toString();
        m_font.setFamily(family);
        updateInputStyle();
    });
    barLayout->addWidget(m_fontCombo);

    // 加粗切换
    m_boldBtn = new QPushButton("B", m_styleBar);
    m_boldBtn->setCheckable(true);
    m_boldBtn->setFixedWidth(22);
    m_boldBtn->setToolTip("加粗 (Bold)");
    connect(m_boldBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_isBold = checked;
        m_font.setBold(checked);
        updateInputStyle();
    });
    barLayout->addWidget(m_boldBtn);

    // 字号微调 - / +
    m_sizeMinusBtn = new QPushButton("A-", m_styleBar);
    m_sizeMinusBtn->setToolTip("缩小字号");
    connect(m_sizeMinusBtn, &QPushButton::clicked, this, [this]() {
        if (m_fontSizePt > 6) {
            m_fontSizePt--;
            m_font.setPointSize(m_fontSizePt);
            updateInputStyle();
        }
    });
    barLayout->addWidget(m_sizeMinusBtn);

    m_sizePlusBtn = new QPushButton("A+", m_styleBar);
    m_sizePlusBtn->setToolTip("增大字号");
    connect(m_sizePlusBtn, &QPushButton::clicked, this, [this]() {
        if (m_fontSizePt < 72) {
            m_fontSizePt++;
            m_font.setPointSize(m_fontSizePt);
            updateInputStyle();
        }
    });
    barLayout->addWidget(m_sizePlusBtn);

    // 颜色拾取按钮
    m_colorBtn = new QPushButton("🎨", m_styleBar);
    m_colorBtn->setToolTip("选择文字颜色");
    m_colorBtn->setFixedWidth(26);
    connect(m_colorBtn, &QPushButton::clicked, this, [this]() {
        QColor chosen = QColorDialog::getColor(m_textColor, this, "选择文字颜色");
        if (chosen.isValid()) {
            m_textColor = chosen;
            updateInputStyle();
        }
    });
    barLayout->addWidget(m_colorBtn);

    // 一键无痕抹除 (纯背景修补，不保留字)
    m_eraseBtn = new QPushButton("无痕抹除", m_styleBar);
    m_eraseBtn->setToolTip("将该字纯背景擦除（清空文字）");
    m_eraseBtn->setStyleSheet("QPushButton:hover { background-color: #d93838; }");
    connect(m_eraseBtn, &QPushButton::clicked, this, [this]() {
        m_lineEdit->clear();
        commitAndClose();
    });
    barLayout->addWidget(m_eraseBtn);

    mainLayout->addWidget(m_styleBar);

    // 阴影效果
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 150));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);
}

void SingleCharEditor::updateInputStyle()
{
    m_font.setPointSize(m_fontSizePt);
    m_font.setBold(m_isBold);
    m_lineEdit->setFont(m_font);

    // 计算高对比度外框与半透明背景
    QString bgStr = QString("rgba(%1, %2, %3, 230)")
                    .arg(m_bgColor.red())
                    .arg(m_bgColor.green())
                    .arg(m_bgColor.blue());

    m_lineEdit->setStyleSheet(QString(
        "QLineEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1.5px dashed #0078d7;"
        "  border-radius: 3px;"
        "  padding: 0px 2px;"
        "}"
    ).arg(bgStr, m_textColor.name()));
}

void SingleCharEditor::startEdit(const SingleCharUnit& unit, const QRect& containerBoundary)
{
    m_unit = unit;
    m_textColor = unit.estimatedFgColor;
    m_bgColor = unit.estimatedBgColor;
    m_fontSizePt = unit.estimatedFontSize;
    m_isBold = unit.isBold;

    // 匹配字体流派
    m_font = QFont(unit.estimatedFontFamily, m_fontSizePt, m_isBold ? QFont::Bold : QFont::Normal);
    for (int i = 0; i < m_fontCombo->count(); ++i) {
        if (m_fontCombo->itemData(i).toString() == unit.estimatedFontFamily) {
            m_fontCombo->setCurrentIndex(i);
            break;
        }
    }
    m_boldBtn->setChecked(m_isBold);

    m_lineEdit->setText(unit.character);
    m_lineEdit->selectAll();
    updateInputStyle();

    // 布局与尺寸计算：输入框精准对齐原字符包围盒
    int editW = (std::max)(32, unit.logicalBox.width() + 6);
    int editH = (std::max)(24, unit.logicalBox.height() + 4);
    m_lineEdit->setFixedSize(editW, editH);

    adjustSize();

    // 确定在全景画布上的放置坐标
    int targetX = unit.logicalBox.center().x() - editW / 2;
    int targetY = unit.logicalBox.top() - 2;

    // 防止微调栏或输入框溢出屏幕容器
    int totalW = (std::max)(editW, m_styleBar->sizeHint().width());
    if (targetX < containerBoundary.left() + 4) targetX = containerBoundary.left() + 4;
    if (targetX + totalW > containerBoundary.right() - 4) targetX = containerBoundary.right() - totalW - 4;

    int totalH = editH + 4 + m_styleBar->sizeHint().height();
    if (targetY + totalH > containerBoundary.bottom() - 4) {
        // 向上翻转
        targetY = unit.logicalBox.top() - totalH - 4;
        if (targetY < containerBoundary.top() + 4) {
            targetY = containerBoundary.top() + 4;
        }
    }

    move(targetX, targetY);
    show();
    raise();
    m_lineEdit->setFocus();
}

QString SingleCharEditor::text() const
{
    return m_lineEdit ? m_lineEdit->text() : QString();
}

void SingleCharEditor::commitAndClose()
{
    QString newText = text();
    hide();
    emit editingCommitted(m_unit, newText, m_textColor, m_font);
}

void SingleCharEditor::cancelAndClose()
{
    hide();
    emit editingCancelled();
}

bool SingleCharEditor::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_lineEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            commitAndClose();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Tab) {
            QString newText = text();
            int idx = m_unit.charIndexInLine;
            hide();
            emit editingCommitted(m_unit, newText, m_textColor, m_font);
            emit requestNavigateNext(idx);
            return true;
        }
        if (keyEvent->key() == Qt::Key_Backtab) {
            QString newText = text();
            int idx = m_unit.charIndexInLine;
            hide();
            emit editingCommitted(m_unit, newText, m_textColor, m_font);
            emit requestNavigatePrev(idx);
            return true;
        }
        if (keyEvent->key() == Qt::Key_Escape) {
            cancelAndClose();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}
