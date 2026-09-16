#include "hotkey_settings_dialog.h"
#include <QMessageBox>

HotkeySettingsDialog::HotkeySettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    setWindowTitle("快捷键偏好设置 - EvanOCR");
    setFixedSize(380, 210);

    setupUi();
    loadSettings();
}

void HotkeySettingsDialog::setupUi()
{
    setStyleSheet(
        "QDialog {"
        "  background-color: #18181b;"
        "  color: #f4f4f5;"
        "}"
        "QLabel {"
        "  color: #d4d4d8;"
        "  font-size: 13px;"
        "}"
        "QKeySequenceEdit {"
        "  background-color: #27272a;"
        "  color: #ffffff;"
        "  border: 1px solid #3f3f46;"
        "  border-radius: 6px;"
        "  padding: 8px 12px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QKeySequenceEdit:focus {"
        "  border-color: #3b82f6;"
        "}"
        "QPushButton {"
        "  background-color: #27272a;"
        "  color: #e4e4e7;"
        "  border: 1px solid #3f3f46;"
        "  border-radius: 6px;"
        "  padding: 7px 16px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: #3f3f46;"
        "  color: #ffffff;"
        "}"
        "QPushButton#saveBtn {"
        "  background-color: #2563eb;"
        "  border-color: #3b82f6;"
        "  color: #ffffff;"
        "  font-weight: 600;"
        "}"
        "QPushButton#saveBtn:hover {"
        "  background-color: #1d4ed8;"
        "}"
    );

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    auto* titleLabel = new QLabel("全局截屏唤醒快捷键：", this);
    titleLabel->setStyleSheet("font-weight: 600; font-size: 14px; color: #f4f4f5;");
    mainLayout->addWidget(titleLabel);

    m_keyEdit = new QKeySequenceEdit(this);
    m_keyEdit->setToolTip("点击输入框，直接在键盘上按下你想要的组合键（如 F1、F4、Alt+A 等）");
    mainLayout->addWidget(m_keyEdit);

    m_tipLabel = new QLabel("提示：点击上方输入框并按下快捷键即可直接录制", this);
    m_tipLabel->setStyleSheet("color: #71717a; font-size: 11px;");
    mainLayout->addWidget(m_tipLabel);

    mainLayout->addStretch();

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_resetBtn = new QPushButton("恢复默认 (F1)", this);
    connect(m_resetBtn, &QPushButton::clicked, this, [this]() {
        m_keyEdit->setKeySequence(QKeySequence("F1"));
    });
    btnLayout->addWidget(m_resetBtn);
    btnLayout->addStretch();

    m_saveBtn = new QPushButton("保存应用", this);
    m_saveBtn->setObjectName("saveBtn");
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    connect(m_saveBtn, &QPushButton::clicked, this, [this]() {
        if (m_keyEdit->keySequence().isEmpty()) {
            QMessageBox::warning(this, "提示", "快捷键不能为空，请录制有效的快捷键。");
            return;
        }
        saveSettings();
        emit hotkeyChanged(m_keyEdit->keySequence());
        accept();
    });
    btnLayout->addWidget(m_saveBtn);

    m_cancelBtn = new QPushButton("取消", this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);
}

void HotkeySettingsDialog::loadSettings()
{
    QSettings settings("EvanOCR", "EvanOCR");
    QString hotkeyStr = settings.value("Hotkey/Snipping", "F1").toString();
    m_keyEdit->setKeySequence(QKeySequence(hotkeyStr));
}

void HotkeySettingsDialog::saveSettings()
{
    QSettings settings("EvanOCR", "EvanOCR");
    settings.setValue("Hotkey/Snipping", m_keyEdit->keySequence().toString());
}
