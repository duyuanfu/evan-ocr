#include "hotkey_settings_dialog.h"
#include "../../core/theme_manager.h"
#include "../../core/autostart_helper.h"
#include "../../core/formula/formula_ocr_manager.h"
#include "../../core/network/network_proxy_manager.h"
#include "../../core/update/update_manager.h"
#include "../update/update_dialog.h"
#include <QSettings>
#include <QMessageBox>
#include <QScrollArea>

HotkeySettingsDialog::HotkeySettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    setWindowTitle("设置 - Evan");
    setFixedSize(580, 620);

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

    auto* headerLabel = new QLabel("⚙️ 设置", this);
    headerLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(headerLabel);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createGeneralTab(), "常规设置");
    m_tabWidget->addTab(createHotkeysTab(), "快捷键设置");
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
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* contentWidget = new QWidget();
    auto* layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(12);

    auto makeCard = [](QWidget* parent) -> QFrame* {
        auto* card = new QFrame(parent);
        card->setObjectName("settingsCard");
        return card;
    };

    // ==========================================
    // 1. 主题外观设置卡片
    // ==========================================
    auto* themeCard = makeCard(contentWidget);
    auto* themeLayout = new QVBoxLayout(themeCard);
    themeLayout->setContentsMargins(14, 12, 14, 12);
    themeLayout->setSpacing(8);

    auto* themeTitle = new QLabel("🎨 主题外观风格", themeCard);
    themeTitle->setObjectName("cardTitle");
    themeLayout->addWidget(themeTitle);

    m_themeCombo = new QComboBox(themeCard);
    m_themeCombo->addItem("跟随 Windows 系统设置 (自动切换)", "Auto");
    m_themeCombo->addItem("明亮浅色模式 (Fluent Light)", "Light");
    m_themeCombo->addItem("沉浸深色模式 (Fluent Dark)", "Dark");
    themeLayout->addWidget(m_themeCombo);

    auto* themeDesc = new QLabel("自适应感知 Windows 深色/浅色配色方案，工具栏、托盘与设置自适应 Fluent 风格。", themeCard);
    themeDesc->setObjectName("rowDesc");
    themeDesc->setWordWrap(true);
    themeLayout->addWidget(themeDesc);

    layout->addWidget(themeCard);

    // ==========================================
    // 2. 截屏辅助与智能吸附卡片
    // ==========================================
    auto* snapCard = makeCard(contentWidget);
    auto* snapLayout = new QVBoxLayout(snapCard);
    snapLayout->setContentsMargins(14, 12, 14, 12);
    snapLayout->setSpacing(8);

    auto* snapTitle = new QLabel("🎯 截屏辅助功能", snapCard);
    snapTitle->setObjectName("cardTitle");
    snapLayout->addWidget(snapTitle);

    m_enableSmartSnappingCheck = new QCheckBox("启用窗口与界面元素智能吸附 (Smart Snapping)", snapCard);
    m_enableSmartSnappingCheck->setToolTip("开启后鼠标在屏幕上移动时会自动识别窗口与控件轮廓，可按 Tab 切换候选框；关闭后完全自由拖拽框选。");
    snapLayout->addWidget(m_enableSmartSnappingCheck);

    auto* snapDesc = new QLabel("提示：若在复杂密集网页或表格中拖拽框选时感觉吸附过于灵敏，可在此取消勾选关闭吸附。", snapCard);
    snapDesc->setObjectName("rowDesc");
    snapDesc->setWordWrap(true);
    snapLayout->addWidget(snapDesc);

    layout->addWidget(snapCard);

    // ==========================================
    // 3. 系统自启动偏好卡片
    // ==========================================
    auto* bootCard = makeCard(contentWidget);
    auto* bootLayout = new QVBoxLayout(bootCard);
    bootLayout->setContentsMargins(14, 12, 14, 12);
    bootLayout->setSpacing(8);

    auto* bootTitle = new QLabel("🚀 系统启动偏好", bootCard);
    bootTitle->setObjectName("cardTitle");
    bootLayout->addWidget(bootTitle);

    m_autoStartCheck = new QCheckBox("开机自动启动 (开机自动常驻托盘，随时按下快捷键截屏)", bootCard);
    m_autoStartCheck->setToolTip("在系统开机登录后自动启动 Evan 并静默常驻系统托盘");
    bootLayout->addWidget(m_autoStartCheck);

    auto* bootDesc = new QLabel("推荐开启。Evan 启动内存仅占用数兆，常驻后台毫秒级响应截屏与贴图热键。", bootCard);
    bootDesc->setObjectName("rowDesc");
    bootDesc->setWordWrap(true);
    bootLayout->addWidget(bootDesc);

    layout->addWidget(bootCard);

    // ==========================================
    // 4. 文字标注换行与回车确认卡片
    // ==========================================
    auto* annotCard = makeCard(contentWidget);
    auto* annotLayout = new QVBoxLayout(annotCard);
    annotLayout->setContentsMargins(14, 12, 14, 12);
    annotLayout->setSpacing(8);

    auto* annotTitle = new QLabel("⌨️ 截图标注文字习惯", annotCard);
    annotTitle->setObjectName("cardTitle");
    annotLayout->addWidget(annotTitle);

    auto* commitLayout = new QHBoxLayout();
    commitLayout->setSpacing(8);
    auto* commitLbl = new QLabel("回车确认与换行方式：", annotCard);
    commitLayout->addWidget(commitLbl);

    m_textCommitModeCombo = new QComboBox(annotCard);
    m_textCommitModeCombo->addItem("Enter 提交，Shift+Enter 换行 (默认)", 0);
    m_textCommitModeCombo->addItem("Ctrl+Enter 提交，Enter 换行", 1);
    commitLayout->addWidget(m_textCommitModeCombo, 1);
    annotLayout->addLayout(commitLayout);

    auto* commitDesc = new QLabel("文字标注支持多行动态排版，可在此根据个人输入习惯自选换行与提交规则。", annotCard);
    commitDesc->setObjectName("rowDesc");
    commitDesc->setWordWrap(true);
    annotLayout->addWidget(commitDesc);

    layout->addWidget(annotCard);

    // ==========================================
    // 5. 数学公式识别云端授权卡片
    // ==========================================
    auto* formulaCard = makeCard(contentWidget);
    auto* formulaLayout = new QVBoxLayout(formulaCard);
    formulaLayout->setContentsMargins(14, 12, 14, 12);
    formulaLayout->setSpacing(8);

    auto* formulaTitle = new QLabel("∑ 数学公式识别云端授权 (可选)", formulaCard);
    formulaTitle->setObjectName("cardTitle");
    formulaLayout->addWidget(formulaTitle);

    auto* tokenLayout = new QHBoxLayout();
    tokenLayout->setSpacing(8);
    auto* tokenLbl = new QLabel("SimpleTeX Token：", formulaCard);
    tokenLayout->addWidget(tokenLbl);

    m_simpleTexTokenEdit = new QLineEdit(formulaCard);
    m_simpleTexTokenEdit->setPlaceholderText("可前往 simpletex.cn 免费申请获取，每月赠送上千次调用");
    m_simpleTexTokenEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    tokenLayout->addWidget(m_simpleTexTokenEdit, 1);
    formulaLayout->addLayout(tokenLayout);

    auto* tokenDesc = new QLabel("填入 Token 可直接极速调用免模型云端公式识别；未配置时将自动无缝使用本地离线插件。", formulaCard);
    tokenDesc->setObjectName("rowDesc");
    tokenDesc->setWordWrap(true);
    formulaLayout->addWidget(tokenDesc);

    layout->addWidget(formulaCard);

    // ==========================================
    // 6. 网络代理设置卡片 (自定义代理下载)
    // ==========================================
    auto* proxyCard = makeCard(contentWidget);
    auto* proxyLayout = new QVBoxLayout(proxyCard);
    proxyLayout->setContentsMargins(14, 12, 14, 12);
    proxyLayout->setSpacing(8);

    auto* proxyTitle = new QLabel("🌐 网络代理设置 (自定义代理加速)", proxyCard);
    proxyTitle->setObjectName("cardTitle");
    proxyLayout->addWidget(proxyTitle);

    auto* pTypeLayout = new QHBoxLayout();
    pTypeLayout->setSpacing(8);
    auto* pTypeLbl = new QLabel("代理模式：", proxyCard);
    pTypeLayout->addWidget(pTypeLbl);

    m_proxyTypeCombo = new QComboBox(proxyCard);
    m_proxyTypeCombo->addItem("跟随系统代理设置 (默认)", static_cast<int>(ProxyType::System));
    m_proxyTypeCombo->addItem("自定义代理服务器 (HTTP / SOCKS5)", static_cast<int>(ProxyType::Custom));
    m_proxyTypeCombo->addItem("直连模式 (不使用任何代理)", static_cast<int>(ProxyType::Direct));
    pTypeLayout->addWidget(m_proxyTypeCombo, 1);
    proxyLayout->addLayout(pTypeLayout);

    // 自定义代理配置子容器
    m_customProxyContainer = new QWidget(proxyCard);
    auto* cpLayout = new QVBoxLayout(m_customProxyContainer);
    cpLayout->setContentsMargins(0, 4, 0, 4);
    cpLayout->setSpacing(6);

    auto* hostPortLayout = new QHBoxLayout();
    hostPortLayout->setSpacing(8);

    m_proxyProtocolCombo = new QComboBox(m_customProxyContainer);
    m_proxyProtocolCombo->addItem("HTTP", static_cast<int>(ProxyProtocol::Http));
    m_proxyProtocolCombo->addItem("SOCKS5", static_cast<int>(ProxyProtocol::Socks5));
    m_proxyProtocolCombo->setFixedWidth(90);
    hostPortLayout->addWidget(m_proxyProtocolCombo);

    m_proxyHostEdit = new QLineEdit(m_customProxyContainer);
    m_proxyHostEdit->setPlaceholderText("主机或 IP (如 127.0.0.1)");
    hostPortLayout->addWidget(m_proxyHostEdit, 1);

    auto* colonLbl = new QLabel(":", m_customProxyContainer);
    hostPortLayout->addWidget(colonLbl);

    m_proxyPortEdit = new QLineEdit(m_customProxyContainer);
    m_proxyPortEdit->setPlaceholderText("端口 (如 7890)");
    m_proxyPortEdit->setFixedWidth(90);
    hostPortLayout->addWidget(m_proxyPortEdit);

    cpLayout->addLayout(hostPortLayout);

    auto* pasteHint = new QLabel("💡 提示：支持粘贴 http://127.0.0.1:7890 或 127.0.0.1:7890，系统将自动识别并填入主机和端口。", m_customProxyContainer);
    pasteHint->setObjectName("rowDesc");
    pasteHint->setWordWrap(true);
    cpLayout->addWidget(pasteHint);

    proxyLayout->addWidget(m_customProxyContainer);

    // 动态联动与自动解析
    connect(m_proxyTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        ProxyType t = static_cast<ProxyType>(m_proxyTypeCombo->currentData().toInt());
        m_customProxyContainer->setVisible(t == ProxyType::Custom);
    });

    auto autoParseProxyInput = [this]() {
        QString text = m_proxyHostEdit->text().trimmed();
        QString host;
        quint16 port = 0;
        ProxyProtocol proto = ProxyProtocol::Http;
        if (NetworkProxyManager::parseProxyString(text, host, port, proto)) {
            m_proxyHostEdit->setText(host);
            m_proxyPortEdit->setText(QString::number(port));
            int idx = m_proxyProtocolCombo->findData(static_cast<int>(proto));
            if (idx >= 0) m_proxyProtocolCombo->setCurrentIndex(idx);
        }
    };
    connect(m_proxyHostEdit, &QLineEdit::editingFinished, this, autoParseProxyInput);

    // 测试连接按钮与反馈
    auto* testLayout = new QHBoxLayout();
    testLayout->setSpacing(10);
    m_testProxyBtn = new QPushButton("测试连接", proxyCard);
    m_testProxyBtn->setCursor(Qt::PointingHandCursor);
    m_testProxyBtn->setFixedWidth(100);
    testLayout->addWidget(m_testProxyBtn);

    m_testProxyResultLbl = new QLabel("未测试", proxyCard);
    m_testProxyResultLbl->setStyleSheet("color: #71717a; font-size: 12px;");
    testLayout->addWidget(m_testProxyResultLbl, 1);
    proxyLayout->addLayout(testLayout);

    connect(m_testProxyBtn, &QPushButton::clicked, this, [this]() {
        m_testProxyBtn->setEnabled(false);
        m_testProxyResultLbl->setText("正在测试连接中 (向 api.github.com 发起请求)...");
        m_testProxyResultLbl->setStyleSheet("color: #3b82f6; font-size: 12px;");

        // 临时同步界面设置进行测试
        ProxyType t = static_cast<ProxyType>(m_proxyTypeCombo->currentData().toInt());
        ProxyProtocol p = static_cast<ProxyProtocol>(m_proxyProtocolCombo->currentData().toInt());
        NetworkProxyManager::instance().setProxyType(t);
        NetworkProxyManager::instance().setProtocol(p);
        NetworkProxyManager::instance().setHost(m_proxyHostEdit->text().trimmed());
        NetworkProxyManager::instance().setPort(static_cast<quint16>(m_proxyPortEdit->text().trimmed().toUInt()));

        NetworkProxyManager::instance().testConnection("https://api.github.com", [this](bool ok, qint64 /*ms*/, const QString& msg) {
            m_testProxyBtn->setEnabled(true);
            if (ok) {
                m_testProxyResultLbl->setText(QString("✅ %1").arg(msg));
                m_testProxyResultLbl->setStyleSheet("color: #10b981; font-weight: bold; font-size: 12px;");
            } else {
                m_testProxyResultLbl->setText(QString("❌ %1").arg(msg));
                m_testProxyResultLbl->setStyleSheet("color: #ef4444; font-size: 12px;");
            }
        });
    });

    auto* proxyDesc = new QLabel("全局生效：配置后统一接管软件更新检查、升级包下载、插件中心扩展安装以及公式云端识别通道。", proxyCard);
    proxyDesc->setObjectName("rowDesc");
    proxyDesc->setWordWrap(true);
    proxyLayout->addWidget(proxyDesc);

    layout->addWidget(proxyCard);

    // ==========================================
    // 7. 软件更新与自动升级卡片
    // ==========================================
    auto* updateCard = makeCard(contentWidget);
    auto* updateLayout = new QVBoxLayout(updateCard);
    updateLayout->setContentsMargins(14, 12, 14, 12);
    updateLayout->setSpacing(8);

    auto* updateTitle = new QLabel("🔄 检查更新与自动升级", updateCard);
    updateTitle->setObjectName("cardTitle");
    updateLayout->addWidget(updateTitle);

    m_autoCheckUpdateCheck = new QCheckBox("启动软件时自动静默检查新版本 (推荐)", updateCard);
    m_autoCheckUpdateCheck->setToolTip("开启后将在软件启动数秒后在后台静默检查 GitHub Releases，发现新版时提醒更新");
    updateLayout->addWidget(m_autoCheckUpdateCheck);

    auto* verActionLayout = new QHBoxLayout();
    verActionLayout->setSpacing(12);

    m_currentVersionLbl = new QLabel(QString("当前已安装版本：<b>v%1</b>").arg(QCoreApplication::applicationVersion()), updateCard);
    verActionLayout->addWidget(m_currentVersionLbl, 1);

    m_checkUpdateBtn = new QPushButton("立即检查更新", updateCard);
    m_checkUpdateBtn->setCursor(Qt::PointingHandCursor);
    m_checkUpdateBtn->setFixedWidth(120);
    verActionLayout->addWidget(m_checkUpdateBtn);

    updateLayout->addLayout(verActionLayout);

    connect(m_checkUpdateBtn, &QPushButton::clicked, this, [this]() {
        m_checkUpdateBtn->setEnabled(false);
        m_checkUpdateBtn->setText("正在检查...");

        auto connAvail = std::make_shared<QMetaObject::Connection>();
        auto connNone = std::make_shared<QMetaObject::Connection>();
        auto connFail = std::make_shared<QMetaObject::Connection>();

        auto cleanup = [this, connAvail, connNone, connFail]() {
            m_checkUpdateBtn->setEnabled(true);
            m_checkUpdateBtn->setText("立即检查更新");
            QObject::disconnect(*connAvail);
            QObject::disconnect(*connNone);
            QObject::disconnect(*connFail);
        };

        *connAvail = connect(&UpdateManager::instance(), &UpdateManager::updateAvailable, this, [this, cleanup](const UpdateInfo& info, bool) {
            cleanup();
            UpdateDialog dlg(info, this);
            dlg.exec();
        });

        *connNone = connect(&UpdateManager::instance(), &UpdateManager::noUpdateAvailable, this, [this, cleanup](bool) {
            cleanup();
            QMessageBox::information(this, "检查更新", QString("当前已是最新版本 (v%1)！无需更新。").arg(QCoreApplication::applicationVersion()));
        });

        *connFail = connect(&UpdateManager::instance(), &UpdateManager::checkFailed, this, [this, cleanup](const QString& err, bool) {
            cleanup();
            QMessageBox::warning(this, "检查更新失败", err + "\n\n请检查网络连接或在上方配置自定义代理后再试。");
        });

        UpdateManager::instance().checkForUpdates(false);
    });

    auto* updateDesc = new QLabel("采用全新自动升级管线：支持一键检查、断点下载、原生无感替换并自动保护本地插件与模型库。", updateCard);
    updateDesc->setObjectName("rowDesc");
    updateDesc->setWordWrap(true);
    updateLayout->addWidget(updateDesc);

    layout->addWidget(updateCard);

    layout->addStretch();
    scroll->setWidget(contentWidget);
    return scroll;
}

QWidget* HotkeySettingsDialog::createHotkeysTab()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);

    auto* contentWidget = new QWidget();
    auto* layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(12);

    auto makeSectionTitle = [contentWidget](const QString& title) -> QLabel* {
        auto* lbl = new QLabel(title, contentWidget);
        lbl->setStyleSheet("font-weight: bold; font-size: 13px; color: #3b82f6; margin-top: 6px;");
        return lbl;
    };

    // 1. 全局热键
    layout->addWidget(makeSectionTitle("🌐 全局热键 (后台随时响应)"));
    layout->addWidget(createKeyRow("开始截屏", "唤醒全景截屏覆盖层", m_globalSnippingEdit));
    layout->addWidget(createKeyRow("桌面贴图", "剪贴板图像或选区置顶贴图", m_globalPinEdit));

    layout->addSpacing(4);

    // 2. 截图操作
    layout->addWidget(makeSectionTitle("✂️ 截图操作 (选区建立时响应)"));
    layout->addWidget(createKeyRow("撤销标注", "撤销上一步绘制的标注图元", m_snippingUndoEdit));
    layout->addWidget(createKeyRow("选区贴图", "将当前选区转为置顶贴图", m_snippingPinEdit));
    layout->addWidget(createKeyRow("完成复制", "复制截图内容到剪贴板并退出", m_snippingConfirmEdit));
    layout->addWidget(createKeyRow("保存截图", "另存为图片文件 (支持 WebP/PNG/JPG)", m_snippingSaveEdit));
    layout->addWidget(createKeyRow("文字识别", "提取选区中的文字内容", m_snippingOcrEdit));
    layout->addWidget(createKeyRow("取消截屏", "放弃当前截屏并退出", m_snippingCancelEdit));

    layout->addSpacing(4);

    // 3. 标注工具
    layout->addWidget(makeSectionTitle("🎨 标注工具 (单键快速切换)"));
    layout->addWidget(createKeyRow("矩形框", "绘制空心/实心矩形框", m_toolRectEdit));
    layout->addWidget(createKeyRow("箭头", "绘制指示箭头矢量", m_toolArrowEdit));
    layout->addWidget(createKeyRow("画笔", "自由涂鸦笔迹", m_toolPencilEdit));
    layout->addWidget(createKeyRow("文字", "富文本原地输入标注", m_toolTextEdit));
    layout->addWidget(createKeyRow("马赛克", "局部打码脱敏遮罩", m_toolMosaicEdit));
    layout->addWidget(createKeyRow("单字修改", "字符就地改字与P图", m_toolCharEdit));

    layout->addSpacing(4);

    // 4. 贴图窗口
    layout->addWidget(makeSectionTitle("📌 贴图窗口 (窗口聚焦时响应)"));
    layout->addWidget(createKeyRow("文字识别", "识别当前贴图窗口中的文字", m_pinOcrEdit));
    layout->addWidget(createKeyRow("复制图片", "复制贴图到系统剪贴板", m_pinCopyEdit));
    layout->addWidget(createKeyRow("保存图片", "保存贴图为独立图像文件", m_pinSaveEdit));
    layout->addWidget(createKeyRow("关闭贴图", "关闭销毁当前置顶贴图窗口", m_pinCloseEdit));

    layout->addStretch();
    scroll->setWidget(contentWidget);
    return scroll;
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

    if (m_autoStartCheck) {
        m_autoStartCheck->setChecked(AutoStartHelper::isAutoStartEnabled());
    }

    if (m_textCommitModeCombo) {
        int textMode = settings.value("Annotation/TextCommitMode", 0).toInt();
        int idx = m_textCommitModeCombo->findData(textMode);
        if (idx >= 0) m_textCommitModeCombo->setCurrentIndex(idx);
    }

    if (m_simpleTexTokenEdit) {
        m_simpleTexTokenEdit->setText(FormulaOcrManager::instance().simpleTexToken());
    }

    // 网络代理设置加载
    if (m_proxyTypeCombo) {
        ProxyType pType = NetworkProxyManager::instance().proxyType();
        int idx = m_proxyTypeCombo->findData(static_cast<int>(pType));
        if (idx >= 0) m_proxyTypeCombo->setCurrentIndex(idx);
        m_customProxyContainer->setVisible(pType == ProxyType::Custom);
    }
    if (m_proxyProtocolCombo) {
        ProxyProtocol proto = NetworkProxyManager::instance().protocol();
        int idx = m_proxyProtocolCombo->findData(static_cast<int>(proto));
        if (idx >= 0) m_proxyProtocolCombo->setCurrentIndex(idx);
    }
    if (m_proxyHostEdit) {
        m_proxyHostEdit->setText(NetworkProxyManager::instance().host());
    }
    if (m_proxyPortEdit) {
        m_proxyPortEdit->setText(QString::number(NetworkProxyManager::instance().port()));
    }

    // 软件更新设置加载
    if (m_autoCheckUpdateCheck) {
        m_autoCheckUpdateCheck->setChecked(UpdateManager::instance().autoCheckOnStartup());
    }

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

    // 保存开机自启动设置
    if (m_autoStartCheck) {
        AutoStartHelper::setAutoStartEnabled(m_autoStartCheck->isChecked());
    }

    // 保存文字标注换行提交偏好
    if (m_textCommitModeCombo) {
        settings.setValue("Annotation/TextCommitMode", m_textCommitModeCombo->currentData().toInt());
    }

    // 保存 SimpleTeX 公式识别授权 Token
    if (m_simpleTexTokenEdit) {
        FormulaOcrManager::instance().setSimpleTexToken(m_simpleTexTokenEdit->text());
    }

    // 保存网络代理配置并立即全局生效
    if (m_proxyTypeCombo) {
        ProxyType t = static_cast<ProxyType>(m_proxyTypeCombo->currentData().toInt());
        NetworkProxyManager::instance().setProxyType(t);
    }
    if (m_proxyProtocolCombo) {
        ProxyProtocol p = static_cast<ProxyProtocol>(m_proxyProtocolCombo->currentData().toInt());
        NetworkProxyManager::instance().setProtocol(p);
    }
    if (m_proxyHostEdit) {
        NetworkProxyManager::instance().setHost(m_proxyHostEdit->text().trimmed());
    }
    if (m_proxyPortEdit) {
        quint16 pt = static_cast<quint16>(m_proxyPortEdit->text().trimmed().toUInt());
        if (pt >= 1 && pt <= 65535) {
            NetworkProxyManager::instance().setPort(pt);
        }
    }
    NetworkProxyManager::instance().apply();

    // 保存自动检查更新设置
    if (m_autoCheckUpdateCheck) {
        UpdateManager::instance().setAutoCheckOnStartup(m_autoCheckUpdateCheck->isChecked());
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
        if (m_autoStartCheck) {
            m_autoStartCheck->setChecked(true);
            AutoStartHelper::setAutoStartEnabled(true);
        }
        if (m_textCommitModeCombo) {
            m_textCommitModeCombo->setCurrentIndex(0);
        }
        if (m_simpleTexTokenEdit) {
            m_simpleTexTokenEdit->clear();
            FormulaOcrManager::instance().setSimpleTexToken("");
        }
        if (m_proxyTypeCombo) {
            m_proxyTypeCombo->setCurrentIndex(0);
            NetworkProxyManager::instance().setProxyType(ProxyType::System);
            NetworkProxyManager::instance().apply();
        }
        if (m_autoCheckUpdateCheck) {
            m_autoCheckUpdateCheck->setChecked(true);
            UpdateManager::instance().setAutoCheckOnStartup(true);
        }
        QSettings settings("Evan", "Evan");
        settings.setValue("Annotation/TextCommitMode", 0);
        loadFromConfig();
        emit hotkeysSaved();
    }
}
