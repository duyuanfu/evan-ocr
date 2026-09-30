#include "hotkey_settings_dialog.h"
#include <QMessageBox>
#include <QScrollArea>

HotkeySettingsDialog::HotkeySettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    setWindowTitle("快捷键偏好设置 - EvanOCR");
    setFixedSize(540, 500);

    setupUi();
    loadFromConfig();
}

void HotkeySettingsDialog::setupUi()
{
    setStyleSheet(
        "QDialog {"
        "  background-color: #18181b;"
        "  color: #f4f4f5;"
        "}"
        "QTabWidget::pane {"
        "  border: 1px solid #3f3f46;"
        "  border-radius: 8px;"
        "  background-color: #1f1f23;"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  background-color: #27272a;"
        "  color: #a1a1aa;"
        "  padding: 8px 16px;"
        "  margin-right: 4px;"
        "  border-top-left-radius: 6px;"
        "  border-top-right-radius: 6px;"
        "  font-size: 13px;"
        "  font-weight: 500;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #1f1f23;"
        "  color: #ffffff;"
        "  border-top: 2px solid #3b82f6;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background-color: #3f3f46;"
        "  color: #e4e4e7;"
        "}"
        "QLabel {"
        "  color: #e4e4e7;"
        "  font-size: 13px;"
        "}"
        "QLabel#rowDesc {"
        "  color: #71717a;"
        "  font-size: 11px;"
        "}"
        "QKeySequenceEdit {"
        "  background-color: #27272a;"
        "  color: #ffffff;"
        "  border: 1px solid #3f3f46;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  font-size: 13px;"
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
        "  padding: 6px 14px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: #3f3f46;"
        "  color: #ffffff;"
        "}"
        "QPushButton#clearBtn {"
        "  background-color: transparent;"
        "  border: none;"
        "  color: #71717a;"
        "  padding: 4px 6px;"
        "  font-size: 12px;"
        "}"
        "QPushButton#clearBtn:hover {"
        "  color: #ef4444;"
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
        "QScrollArea { border: none; background: transparent; }"
    );

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 16);
    mainLayout->setSpacing(12);

    auto* headerLabel = new QLabel("⌨️ 快捷键个性化设置", this);
    headerLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #f4f4f5;");
    mainLayout->addWidget(headerLabel);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createGlobalTab(), "全局热键");
    m_tabWidget->addTab(createSnippingTab(), "截图交互");
    m_tabWidget->addTab(createToolsTab(), "标注工具");
    m_tabWidget->addTab(createPinTab(), "贴图窗口");
    mainLayout->addWidget(m_tabWidget, 1);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_resetBtn = new QPushButton("恢复全部默认值", this);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    connect(m_resetBtn, &QPushButton::clicked, this, &HotkeySettingsDialog::resetAllDefaults);
    btnLayout->addWidget(m_resetBtn);

    btnLayout->addStretch();

    m_saveBtn = new QPushButton("保存配置", this);
    m_saveBtn->setObjectName("saveBtn");
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    connect(m_saveBtn, &QPushButton::clicked, this, [this]() {
        if (saveToConfig()) {
            accept();
        }
    });
    btnLayout->addWidget(m_saveBtn);

    m_cancelBtn = new QPushButton("取消", this);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);
}

QWidget* HotkeySettingsDialog::createKeyRow(const QString& title, const QString& desc, QKeySequenceEdit*& editOut)
{
    auto* rowWidget = new QWidget(this);
    auto* rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(6, 4, 6, 4);
    rowLayout->setSpacing(10);

    auto* labelLayout = new QVBoxLayout();
    labelLayout->setSpacing(2);
    auto* titleLbl = new QLabel(title, rowWidget);
    titleLbl->setStyleSheet("font-weight: 500; font-size: 13px; color: #f4f4f5;");
    labelLayout->addWidget(titleLbl);

    if (!desc.isEmpty()) {
        auto* descLbl = new QLabel(desc, rowWidget);
        descLbl->setObjectName("rowDesc");
        labelLayout->addWidget(descLbl);
    }
    rowLayout->addLayout(labelLayout, 1);

    editOut = new QKeySequenceEdit(rowWidget);
    editOut->setFixedWidth(150);
    rowLayout->addWidget(editOut);

    auto* clearBtn = new QPushButton("✕", rowWidget);
    clearBtn->setObjectName("clearBtn");
    clearBtn->setToolTip("清空此快捷键");
    clearBtn->setFixedSize(24, 24);
    clearBtn->setCursor(Qt::PointingHandCursor);
    connect(clearBtn, &QPushButton::clicked, editOut, &QKeySequenceEdit::clear);
    rowLayout->addWidget(clearBtn);

    return rowWidget;
}

QWidget* HotkeySettingsDialog::createGlobalTab()
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    auto* tip = new QLabel("全局热键在后台或处于其他任何应用程序时均可全局生效：", widget);
    tip->setStyleSheet("color: #a1a1aa; font-size: 12px; margin-bottom: 6px;");
    layout->addWidget(tip);

    layout->addWidget(createKeyRow("开始截屏", "唤醒全景截屏覆盖层与十字准星", m_globalSnippingEdit));
    layout->addWidget(createKeyRow("桌面贴图", "将剪贴板图像或当前截屏选区置顶贴图", m_globalPinEdit));

    layout->addStretch();
    return widget;
}

QWidget* HotkeySettingsDialog::createSnippingTab()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);

    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);

    layout->addWidget(createKeyRow("撤销上一步", "撤销最近一次绘制的标注图元", m_snippingUndoEdit));
    layout->addWidget(createKeyRow("选区贴图", "将当前选区及其标注转换为置顶贴图窗口", m_snippingPinEdit));
    layout->addWidget(createKeyRow("完成并复制", "合并截屏内容写入剪贴板并退出遮罩", m_snippingConfirmEdit));
    layout->addWidget(createKeyRow("保存到文件", "弹出文件保存对话框保存高清图像", m_snippingSaveEdit));
    layout->addWidget(createKeyRow("提取文字 (OCR)", "调用原生无依赖 WinRT OCR 识别选区内容", m_snippingOcrEdit));
    layout->addWidget(createKeyRow("取消截屏", "退出截屏遮罩放弃当前操作", m_snippingCancelEdit));

    layout->addStretch();
    scroll->setWidget(widget);
    return scroll;
}

QWidget* HotkeySettingsDialog::createToolsTab()
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);

    auto* tip = new QLabel("选区建立后，按下单键即可快速切换对应标注工具：", widget);
    tip->setStyleSheet("color: #a1a1aa; font-size: 12px; margin-bottom: 6px;");
    layout->addWidget(tip);

    layout->addWidget(createKeyRow("矩形标注", "绘制空心/实心矩形框", m_toolRectEdit));
    layout->addWidget(createKeyRow("箭头标注", "绘制指示箭头矢量", m_toolArrowEdit));
    layout->addWidget(createKeyRow("自由画笔", "自由涂鸦笔迹", m_toolPencilEdit));
    layout->addWidget(createKeyRow("文字标注", "点击建立富文本原地输入框", m_toolTextEdit));
    layout->addWidget(createKeyRow("马赛克遮罩", "打码脱敏局部图像信息", m_toolMosaicEdit));

    layout->addStretch();
    return widget;
}

QWidget* HotkeySettingsDialog::createPinTab()
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);

    auto* tip = new QLabel("在置顶贴图窗口获得焦点时生效的交互快捷键：", widget);
    tip->setStyleSheet("color: #a1a1aa; font-size: 12px; margin-bottom: 6px;");
    layout->addWidget(tip);

    layout->addWidget(createKeyRow("识别文字 (OCR)", "直接识别当前贴图窗口中的文字", m_pinOcrEdit));
    layout->addWidget(createKeyRow("复制贴图", "将当前贴图位图复制到系统剪贴板", m_pinCopyEdit));
    layout->addWidget(createKeyRow("保存贴图", "保存当前贴图为独立图像文件", m_pinSaveEdit));
    layout->addWidget(createKeyRow("关闭贴图", "销毁并关闭当前置顶贴图窗口", m_pinCloseEdit));

    layout->addStretch();
    return widget;
}

void HotkeySettingsDialog::loadFromConfig()
{
    const auto& c = HotkeyConfig::instance().data();

    // 全局热键
    m_globalSnippingEdit->setKeySequence(QKeySequence(c.globalSnipping));
    m_globalPinEdit->setKeySequence(QKeySequence(c.globalPin));

    // 截屏操作
    m_snippingUndoEdit->setKeySequence(QKeySequence(c.snippingUndo));
    m_snippingPinEdit->setKeySequence(QKeySequence(c.snippingPin));
    m_snippingConfirmEdit->setKeySequence(QKeySequence(c.snippingConfirm));
    m_snippingSaveEdit->setKeySequence(QKeySequence(c.snippingSave));
    m_snippingOcrEdit->setKeySequence(QKeySequence(c.snippingOcr));
    m_snippingCancelEdit->setKeySequence(QKeySequence(c.snippingCancel));

    // 标注工具
    m_toolRectEdit->setKeySequence(QKeySequence(c.toolRect));
    m_toolArrowEdit->setKeySequence(QKeySequence(c.toolArrow));
    m_toolPencilEdit->setKeySequence(QKeySequence(c.toolPencil));
    m_toolTextEdit->setKeySequence(QKeySequence(c.toolText));
    m_toolMosaicEdit->setKeySequence(QKeySequence(c.toolMosaic));

    // 贴图窗口
    m_pinCloseEdit->setKeySequence(QKeySequence(c.pinClose));
    m_pinOcrEdit->setKeySequence(QKeySequence(c.pinOcr));
    m_pinCopyEdit->setKeySequence(QKeySequence(c.pinCopy));
    m_pinSaveEdit->setKeySequence(QKeySequence(c.pinSave));
}

bool HotkeySettingsDialog::saveToConfig()
{
    QString globalSnip = m_globalSnippingEdit->keySequence().toString();
    QString globalPin  = m_globalPinEdit->keySequence().toString();

    // 全局截屏快捷键不能为空
    if (globalSnip.trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "全局截屏快捷键不能为空，请录制有效的快捷键。");
        m_tabWidget->setCurrentIndex(0);
        m_globalSnippingEdit->setFocus();
        return false;
    }

    // 全局快捷键冲突校验
    if (!globalPin.trimmed().isEmpty() && globalPin.compare(globalSnip, Qt::CaseInsensitive) == 0) {
        QMessageBox::warning(this, "快捷键冲突", "全局截屏与全局贴图快捷键不能相同，请调整后重新保存。");
        m_tabWidget->setCurrentIndex(0);
        m_globalPinEdit->setFocus();
        return false;
    }

    HotkeyConfigData d;
    d.globalSnipping = globalSnip;
    d.globalPin      = globalPin;

    d.snippingUndo    = m_snippingUndoEdit->keySequence().toString();
    d.snippingPin     = m_snippingPinEdit->keySequence().toString();
    d.snippingConfirm = m_snippingConfirmEdit->keySequence().toString();
    d.snippingSave    = m_snippingSaveEdit->keySequence().toString();
    d.snippingOcr     = m_snippingOcrEdit->keySequence().toString();
    d.snippingCancel  = m_snippingCancelEdit->keySequence().toString();

    d.toolRect   = m_toolRectEdit->keySequence().toString();
    d.toolArrow  = m_toolArrowEdit->keySequence().toString();
    d.toolPencil = m_toolPencilEdit->keySequence().toString();
    d.toolText   = m_toolTextEdit->keySequence().toString();
    d.toolMosaic = m_toolMosaicEdit->keySequence().toString();

    d.pinClose = m_pinCloseEdit->keySequence().toString();
    d.pinOcr   = m_pinOcrEdit->keySequence().toString();
    d.pinCopy  = m_pinCopyEdit->keySequence().toString();
    d.pinSave  = m_pinSaveEdit->keySequence().toString();

    HotkeyConfig::instance().setData(d);
    emit hotkeysSaved();
    return true;
}

void HotkeySettingsDialog::resetAllDefaults()
{
    int ret = QMessageBox::question(this, "确认恢复", "确定要将所有操作快捷键恢复为官方默认预设吗？",
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        HotkeyConfig::instance().resetToDefaults();
        loadFromConfig();
        emit hotkeysSaved();
    }
}
