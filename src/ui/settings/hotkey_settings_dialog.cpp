#include "hotkey_settings_dialog.h"
#include "../../core/theme_manager.h"
#include <QSettings>
#include <QMessageBox>
#include <QScrollArea>

HotkeySettingsDialog::HotkeySettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    setWindowTitle("偏好设置 - Evan");
    setFixedSize(540, 480);

    setupUi();
    loadFromConfig();
}

void HotkeySettingsDialog::setupUi()
{
    setStyleSheet(ThemeManager::instance().getSettingsDialogStyle());
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        setStyleSheet(ThemeManager::instance().getSettingsDialogStyle());
    });

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 16);
    mainLayout->setSpacing(12);

    auto* headerLabel = new QLabel("⚙️ 偏好设置", this);
    headerLabel->setStyleSheet("font-size: 15px; font-weight: bold;");
    mainLayout->addWidget(headerLabel);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createGeneralTab(), "通用外观");
    m_tabWidget->addTab(createGlobalTab(), "全局热键");
    m_tabWidget->addTab(createSnippingTab(), "截图操作");
    m_tabWidget->addTab(createToolsTab(), "标注工具");
    m_tabWidget->addTab(createPinTab(), "贴图窗口");
    mainLayout->addWidget(m_tabWidget, 1);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_resetBtn = new QPushButton("恢复默认", this);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    connect(m_resetBtn, &QPushButton::clicked, this, &HotkeySettingsDialog::resetAllDefaults);
    btnLayout->addWidget(m_resetBtn);

    btnLayout->addStretch();

    m_saveBtn = new QPushButton("保存", this);
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

QWidget* HotkeySettingsDialog::createGeneralTab()
{
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    // 1. 主题外观设置
    auto* themeTitle = new QLabel("主题外观风格：", widget);
    themeTitle->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(themeTitle);

    m_themeCombo = new QComboBox(widget);
    m_themeCombo->addItem("跟随 Windows 系统设置 (自动切换)", "Auto");
    m_themeCombo->addItem("明亮浅色模式 (Fluent Light)", "Light");
    m_themeCombo->addItem("沉浸深色模式 (Fluent Dark)", "Dark");
    layout->addWidget(m_themeCombo);

    auto* themeDesc = new QLabel("切换深色/浅色配色方案，使截图工具栏、托盘与设置与系统风格无缝融合。", widget);
    themeDesc->setObjectName("rowDesc");
    layout->addWidget(themeDesc);

    layout->addSpacing(10);

    // 2. 智能吸附选框开关
    auto* snapTitle = new QLabel("截屏辅助功能：", widget);
    snapTitle->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(snapTitle);

    m_enableSmartSnappingCheck = new QCheckBox("启用窗口与界面元素智能吸附 (Smart Snapping)", widget);
    m_enableSmartSnappingCheck->setToolTip("开启后鼠标在屏幕上移动时会自动识别窗口与控件轮廓，可按 Tab 切换候选框；关闭后完全自由拖拽框选。");
    layout->addWidget(m_enableSmartSnappingCheck);

    auto* snapDesc = new QLabel("提示：若在复杂密集网页或表格中感觉频繁吸附晃动，可在此取消勾选关闭吸附。", widget);
    snapDesc->setObjectName("rowDesc");
    layout->addWidget(snapDesc);

    layout->addStretch();
    return widget;
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

    layout->addWidget(createKeyRow("撤销标注", "撤销上一步绘制的标注图元", m_snippingUndoEdit));
    layout->addWidget(createKeyRow("选区贴图", "将当前截图选区转为置顶贴图", m_snippingPinEdit));
    layout->addWidget(createKeyRow("完成复制", "复制截图内容到剪贴板并退出", m_snippingConfirmEdit));
    layout->addWidget(createKeyRow("保存截图", "另存为图片文件 (支持 WebP/PNG/JPG)", m_snippingSaveEdit));
    layout->addWidget(createKeyRow("文字识别", "提取选区中的文字内容", m_snippingOcrEdit));
    layout->addWidget(createKeyRow("取消截屏", "放弃当前截屏并退出", m_snippingCancelEdit));

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

    layout->addWidget(createKeyRow("矩形框", "绘制空心/实心矩形框", m_toolRectEdit));
    layout->addWidget(createKeyRow("箭头", "绘制指示箭头矢量", m_toolArrowEdit));
    layout->addWidget(createKeyRow("画笔", "自由涂鸦笔迹", m_toolPencilEdit));
    layout->addWidget(createKeyRow("文字", "富文本原地输入标注", m_toolTextEdit));
    layout->addWidget(createKeyRow("马赛克", "局部打码脱敏遮罩", m_toolMosaicEdit));
    layout->addWidget(createKeyRow("单字修改", "字符就地改字与P图", m_toolCharEdit));

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

    layout->addWidget(createKeyRow("文字识别", "识别当前贴图窗口中的文字", m_pinOcrEdit));
    layout->addWidget(createKeyRow("复制图片", "复制贴图到系统剪贴板", m_pinCopyEdit));
    layout->addWidget(createKeyRow("保存图片", "保存贴图为独立图像文件", m_pinSaveEdit));
    layout->addWidget(createKeyRow("关闭贴图", "关闭销毁当前置顶贴图窗口", m_pinCloseEdit));

    layout->addStretch();
    return widget;
}

void HotkeySettingsDialog::loadFromConfig()
{
    const auto& c = HotkeyConfig::instance().data();

    // 通用与外观设置
    ThemeMode curTheme = ThemeManager::instance().themeMode();
    if (curTheme == ThemeMode::Light) m_themeCombo->setCurrentIndex(1);
    else if (curTheme == ThemeMode::Dark) m_themeCombo->setCurrentIndex(2);
    else m_themeCombo->setCurrentIndex(0);

    QSettings settings("Evan", "Evan");
    bool enableSnap = settings.value("Snapping/EnableSmartSnapping", true).toBool();
    m_enableSmartSnappingCheck->setChecked(enableSnap);

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
    m_toolCharEdit->setKeySequence(QKeySequence(c.toolCharEdit));

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

    // 保存外观模式
    int themeIdx = m_themeCombo->currentIndex();
    ThemeMode selectedMode = (themeIdx == 1) ? ThemeMode::Light :
                             (themeIdx == 2) ? ThemeMode::Dark : ThemeMode::Auto;
    ThemeManager::instance().setThemeMode(selectedMode);

    // 保存智能吸附开关
    QSettings settings("Evan", "Evan");
    settings.setValue("Snapping/EnableSmartSnapping", m_enableSmartSnappingCheck->isChecked());

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
    d.toolCharEdit = m_toolCharEdit->keySequence().toString();

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
