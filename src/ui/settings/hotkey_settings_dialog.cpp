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
#include <QListView>

HotkeySettingsDialog::HotkeySettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    setWindowTitle("设置 - Evan");
    setFixedSize(600, 640);

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

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(4, 2, 4, 4);
    headerLayout->setSpacing(10);

    auto* titleVBox = new QVBoxLayout();
    titleVBox->setSpacing(3);

    auto* titleLabel = new QLabel("⚙️  系统与偏好设置", this);
    titleLabel->setObjectName("dialogHeaderTitle");
    titleVBox->addWidget(titleLabel);

    auto* subTitleLabel = new QLabel("配置系统启动偏好、文字标注习惯、网络代理与版本更新", this);
    subTitleLabel->setObjectName("dialogHeaderSubtitle");
    titleVBox->addWidget(subTitleLabel);

    headerLayout->addLayout(titleVBox, 1);
    mainLayout->addLayout(headerLayout);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createGeneralTab(), "常规设置");
    m_tabWidget->addTab(createHotkeysTab(), "快捷键设置");
    mainLayout->addWidget(m_tabWidget, 1);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_resetBtn = new QPushButton("恢复默认设置", this);
    m_resetBtn->setObjectName("resetBtn");
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    connect(m_resetBtn, &QPushButton::clicked, this, &HotkeySettingsDialog::resetAllDefaults);
    btnLayout->addWidget(m_resetBtn);

    btnLayout->addStretch();

    m_cancelBtn = new QPushButton("取消", this);
    m_cancelBtn->setObjectName("cancelBtn");
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    m_saveBtn = new QPushButton("保存并应用", this);
    m_saveBtn->setObjectName("saveBtn");
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    connect(m_saveBtn, &QPushButton::clicked, this, [this]() {
        if (saveToConfig()) {
            accept();
        }
    });
    btnLayout->addWidget(m_saveBtn);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_cancelBtn);

    mainLayout->addLayout(btnLayout);
}

QWidget* HotkeySettingsDialog::createKeyRow(const QString& title, const QString& desc, QKeySequenceEdit*& editOut)
{
    auto* rowWidget = new QWidget(this);
    auto* rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(4, 3, 4, 3);
    rowLayout->setSpacing(12);

    auto* labelLayout = new QVBoxLayout();
    labelLayout->setSpacing(1);
    auto* titleLbl = new QLabel(title, rowWidget);
    titleLbl->setStyleSheet("font-weight: 600; font-size: 13px;");
    labelLayout->addWidget(titleLbl);

    if (!desc.isEmpty()) {
        auto* descLbl = new QLabel(desc, rowWidget);
        descLbl->setObjectName("rowDesc");
        labelLayout->addWidget(descLbl);
    }
    rowLayout->addLayout(labelLayout, 1);

    editOut = new QKeySequenceEdit(rowWidget);
    editOut->setFixedWidth(140);
    rowLayout->addWidget(editOut);

    auto* clearBtn = new QPushButton("✕", rowWidget);
    clearBtn->setObjectName("clearBtn");
    clearBtn->setToolTip("清空此快捷键");
    clearBtn->setFixedSize(26, 26);
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
    contentWidget->setObjectName("settingsContentWidget");
    auto* layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(12);

    auto makeCard = [](QWidget* parent) -> QFrame* {
        auto* card = new QFrame(parent);
        card->setObjectName("settingsCard");
        card->setFrameShape(QFrame::StyledPanel);
        card->setAttribute(Qt::WA_StyledBackground, true);
        return card;
    };

    auto createSettingRow = [makeCard](QWidget* parent, const QString& emoji, const QString& title, const QString& desc, QWidget* controlWidget = nullptr) -> QFrame* {
        auto* card = makeCard(parent);
        auto* rowLayout = new QHBoxLayout(card);
        rowLayout->setContentsMargins(14, 12, 14, 12);
        rowLayout->setSpacing(14);

        // 1. Icon Badge (固定 36x36 居中徽章，彻底解决 Emoji 与汉字基线不对齐问题)
        auto* iconBadge = new QLabel(emoji, card);
        iconBadge->setObjectName("settingIconBadge");
        iconBadge->setFixedSize(36, 36);
        iconBadge->setAlignment(Qt::AlignCenter);
        iconBadge->setAttribute(Qt::WA_StyledBackground, true);
        rowLayout->addWidget(iconBadge, 0, Qt::AlignVCenter);

        // 2. Title & Description
        auto* textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);

        auto* titleLbl = new QLabel(title, card);
        titleLbl->setObjectName("settingRowTitle");
        textLayout->addWidget(titleLbl);

        if (!desc.isEmpty()) {
            auto* descLbl = new QLabel(desc, card);
            descLbl->setObjectName("rowDesc");
            descLbl->setWordWrap(true);
            textLayout->addWidget(descLbl);
        }
        rowLayout->addLayout(textLayout, 1);

        // 3. Right Control Widget
        if (controlWidget) {
            rowLayout->addWidget(controlWidget, 0, Qt::AlignVCenter);
        }

        return card;
    };

    // ==========================================
    // 1. 主题外观设置行
    // ==========================================
    m_themeCombo = new QComboBox();
    m_themeCombo->setView(new QListView(m_themeCombo));
    m_themeCombo->setFixedWidth(215);
    m_themeCombo->addItem("跟随 Windows 系统设置", "Auto");
    m_themeCombo->addItem("明亮浅色模式 (Light)", "Light");
    m_themeCombo->addItem("沉浸深色模式 (Dark)", "Dark");
    layout->addWidget(createSettingRow(contentWidget, "🎨", "主题外观风格", "自适应感知 Windows 深色/浅色配色方案，工具栏、托盘与设置自适应 Fluent 风格", m_themeCombo));

    // ==========================================
    // 2. 截屏辅助与智能吸附行
    // ==========================================
    m_enableSmartSnappingCheck = new QCheckBox("启用吸附");
    m_enableSmartSnappingCheck->setToolTip("开启后鼠标在屏幕上移动时会自动识别窗口与控件轮廓，可按 Tab 切换候选框；关闭后完全自由拖拽框选。");
    layout->addWidget(createSettingRow(contentWidget, "🎯", "窗口与元素智能吸附", "鼠标在屏幕上移动时自动吸附窗口与控件轮廓，按 Tab 循环切换候选选区", m_enableSmartSnappingCheck));

    // ==========================================
    // 3. 系统自启动偏好行
    // ==========================================
    m_autoStartCheck = new QCheckBox("开机自启");
    m_autoStartCheck->setToolTip("在系统开机登录后自动启动 Evan 并静默常驻系统托盘");
    layout->addWidget(createSettingRow(contentWidget, "🚀", "Windows 开机自启动", "开机登录后自动常驻系统托盘，内存仅占数兆，毫秒级快速响应全局截屏热键", m_autoStartCheck));

    // ==========================================
    // 4. 文字标注换行与回车确认行
    // ==========================================
    m_textCommitModeCombo = new QComboBox();
    m_textCommitModeCombo->setView(new QListView(m_textCommitModeCombo));
    m_textCommitModeCombo->setFixedWidth(255);
    m_textCommitModeCombo->addItem("Enter 提交 / Shift+Enter 换行", 0);
    m_textCommitModeCombo->addItem("Ctrl+Enter 提交 / Enter 换行", 1);
    layout->addWidget(createSettingRow(contentWidget, "⌨️", "文字标注换行提交方式", "就地输入支持多行排版，可在此根据个人输入习惯自选换行与提交规则", m_textCommitModeCombo));

    // ==========================================
    // 5. 数学公式识别云端授权行
    // ==========================================
    m_simpleTexTokenEdit = new QLineEdit();
    m_simpleTexTokenEdit->setFixedWidth(230);
    m_simpleTexTokenEdit->setPlaceholderText("填入 Token (可选)");
    m_simpleTexTokenEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    layout->addWidget(createSettingRow(contentWidget, "∑", "SimpleTeX 云端公式授权", "前往 simpletex.cn 免费获取，未填时自动无缝降级使用本地离线公式识别插件", m_simpleTexTokenEdit));

    // ==========================================
    // 6. 网络代理设置卡片 (自定义代理下载)
    // ==========================================
    auto* proxyCard = makeCard(contentWidget);
    auto* proxyLayout = new QVBoxLayout(proxyCard);
    proxyLayout->setContentsMargins(14, 12, 14, 12);
    proxyLayout->setSpacing(10);

    auto* pHeaderRow = new QHBoxLayout();
    pHeaderRow->setSpacing(14);

    auto* proxyBadge = new QLabel("🌐", proxyCard);
    proxyBadge->setObjectName("settingIconBadge");
    proxyBadge->setFixedSize(36, 36);
    proxyBadge->setAlignment(Qt::AlignCenter);
    proxyBadge->setAttribute(Qt::WA_StyledBackground, true);
    pHeaderRow->addWidget(proxyBadge, 0, Qt::AlignVCenter);

    auto* pTitleLayout = new QVBoxLayout();
    pTitleLayout->setSpacing(2);
    auto* pTitleLbl = new QLabel("全局网络代理设置", proxyCard);
    pTitleLbl->setObjectName("settingRowTitle");
    pTitleLayout->addWidget(pTitleLbl);
    auto* pDescLbl = new QLabel("统一覆盖检查更新、版本升级下载、插件中心扩展安装与公式云端识别通道", proxyCard);
    pDescLbl->setObjectName("rowDesc");
    pDescLbl->setWordWrap(true);
    pTitleLayout->addWidget(pDescLbl);
    pHeaderRow->addLayout(pTitleLayout, 1);

    m_proxyTypeCombo = new QComboBox(proxyCard);
    m_proxyTypeCombo->setView(new QListView(m_proxyTypeCombo));
    m_proxyTypeCombo->setFixedWidth(215);
    m_proxyTypeCombo->addItem("跟随系统代理 (默认)", static_cast<int>(ProxyType::System));
    m_proxyTypeCombo->addItem("自定义代理服务器", static_cast<int>(ProxyType::Custom));
    m_proxyTypeCombo->addItem("直连 (不使用代理)", static_cast<int>(ProxyType::Direct));
    pHeaderRow->addWidget(m_proxyTypeCombo, 0, Qt::AlignVCenter);
    proxyLayout->addLayout(pHeaderRow);

    // 自定义代理配置子容器
    m_customProxyContainer = new QWidget(proxyCard);
    m_customProxyContainer->setObjectName("customProxyContainer");
    auto* cpLayout = new QVBoxLayout(m_customProxyContainer);
    cpLayout->setContentsMargins(50, 4, 0, 4);
    cpLayout->setSpacing(8);

    auto* hostPortLayout = new QHBoxLayout();
    hostPortLayout->setSpacing(8);

    m_proxyProtocolCombo = new QComboBox(m_customProxyContainer);
    m_proxyProtocolCombo->setView(new QListView(m_proxyProtocolCombo));
    m_proxyProtocolCombo->addItem("HTTP", static_cast<int>(ProxyProtocol::Http));
    m_proxyProtocolCombo->addItem("SOCKS5", static_cast<int>(ProxyProtocol::Socks5));
    m_proxyProtocolCombo->setFixedWidth(120);
    hostPortLayout->addWidget(m_proxyProtocolCombo);

    m_proxyHostEdit = new QLineEdit(m_customProxyContainer);
    m_proxyHostEdit->setPlaceholderText("主机或 IP (如 127.0.0.1)");
    hostPortLayout->addWidget(m_proxyHostEdit, 1);

    auto* colonLbl = new QLabel(":", m_customProxyContainer);
    hostPortLayout->addWidget(colonLbl);

    m_proxyPortEdit = new QLineEdit(m_customProxyContainer);
    m_proxyPortEdit->setPlaceholderText("端口 (如 7890)");
    m_proxyPortEdit->setFixedWidth(100);
    hostPortLayout->addWidget(m_proxyPortEdit);

    cpLayout->addLayout(hostPortLayout);

    // 测试连接与反馈行
    auto* testLayout = new QHBoxLayout();
    testLayout->setSpacing(12);

    m_testProxyBtn = new QPushButton("测试连接", m_customProxyContainer);
    m_testProxyBtn->setObjectName("testProxyBtn");
    m_testProxyBtn->setCursor(Qt::PointingHandCursor);
    m_testProxyBtn->setFixedSize(100, 30);
    testLayout->addWidget(m_testProxyBtn);

    m_testProxyResultLbl = new QLabel("未测试", m_customProxyContainer);
    m_testProxyResultLbl->setStyleSheet("color: #71717a; font-size: 12px;");
    testLayout->addWidget(m_testProxyResultLbl, 1);
    cpLayout->addLayout(testLayout);

    auto* pasteHint = new QLabel("💡 提示：支持直接粘贴 http://127.0.0.1:7890，系统将自动识别提取主机与端口。", m_customProxyContainer);
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

    connect(m_testProxyBtn, &QPushButton::clicked, this, [this]() {
        m_testProxyBtn->setEnabled(false);
        m_testProxyResultLbl->setText("正在测试连接中 (向 github.com 发起请求)...");
        m_testProxyResultLbl->setStyleSheet("color: #3b82f6; font-size: 12px;");

        // 临时同步界面设置进行测试
        ProxyType t = static_cast<ProxyType>(m_proxyTypeCombo->currentData().toInt());
        ProxyProtocol p = static_cast<ProxyProtocol>(m_proxyProtocolCombo->currentData().toInt());
        NetworkProxyManager::instance().setProxyType(t);
        NetworkProxyManager::instance().setProtocol(p);
        NetworkProxyManager::instance().setHost(m_proxyHostEdit->text().trimmed());
        NetworkProxyManager::instance().setPort(static_cast<quint16>(m_proxyPortEdit->text().trimmed().toUInt()));

        NetworkProxyManager::instance().testConnection("https://github.com", [this](bool ok, qint64 /*ms*/, const QString& msg) {
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

    layout->addWidget(proxyCard);

    // ==========================================
    // 7. 软件更新与自动升级卡片
    // ==========================================
    auto* updateCard = makeCard(contentWidget);
    auto* updateLayout = new QVBoxLayout(updateCard);
    updateLayout->setContentsMargins(14, 12, 14, 12);
    updateLayout->setSpacing(10);

    auto* uHeaderRow = new QHBoxLayout();
    uHeaderRow->setSpacing(14);

    auto* updateBadge = new QLabel("🔄", updateCard);
    updateBadge->setObjectName("settingIconBadge");
    updateBadge->setFixedSize(36, 36);
    updateBadge->setAlignment(Qt::AlignCenter);
    updateBadge->setAttribute(Qt::WA_StyledBackground, true);
    uHeaderRow->addWidget(updateBadge, 0, Qt::AlignVCenter);

    auto* uTitleLayout = new QVBoxLayout();
    uTitleLayout->setSpacing(2);
    auto* uTitleLbl = new QLabel(QString("软件版本与升级 (当前已安装 v%1)").arg(QCoreApplication::applicationVersion()), updateCard);
    uTitleLbl->setObjectName("settingRowTitle");
    uTitleLayout->addWidget(uTitleLbl);
    auto* uDescLbl = new QLabel("支持一键检查新版本、极速下载、原生无感替换并自动保护本地插件与模型库", updateCard);
    uDescLbl->setObjectName("rowDesc");
    uDescLbl->setWordWrap(true);
    uTitleLayout->addWidget(uDescLbl);
    uHeaderRow->addLayout(uTitleLayout, 1);

    m_checkUpdateBtn = new QPushButton("检查更新", updateCard);
    m_checkUpdateBtn->setObjectName("checkUpdateBtn");
    m_checkUpdateBtn->setCursor(Qt::PointingHandCursor);
    m_checkUpdateBtn->setFixedSize(100, 30);
    uHeaderRow->addWidget(m_checkUpdateBtn, 0, Qt::AlignVCenter);
    updateLayout->addLayout(uHeaderRow);

    m_autoCheckUpdateCheck = new QCheckBox("启动软件时自动静默检查新版本 (推荐)", updateCard);
    m_autoCheckUpdateCheck->setToolTip("开启后将在软件启动数秒后在后台静默检查 GitHub Releases，发现新版时提醒更新");
    updateLayout->addWidget(m_autoCheckUpdateCheck);

    connect(m_checkUpdateBtn, &QPushButton::clicked, this, [this]() {
        m_checkUpdateBtn->setEnabled(false);
        m_checkUpdateBtn->setText("正在检查...");

        auto connAvail = std::make_shared<QMetaObject::Connection>();
        auto connNone = std::make_shared<QMetaObject::Connection>();
        auto connFail = std::make_shared<QMetaObject::Connection>();

        auto cleanup = [this, connAvail, connNone, connFail]() {
            m_checkUpdateBtn->setEnabled(true);
            m_checkUpdateBtn->setText("检查更新");
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

    layout->addWidget(updateCard);

    layout->addStretch();
    scroll->setWidget(contentWidget);
    return scroll;
}

QWidget* HotkeySettingsDialog::createHotkeysTab()
{
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* contentWidget = new QWidget();
    contentWidget->setObjectName("settingsContentWidget");
    auto* layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(12);

    auto makeCard = [](QWidget* parent) -> QFrame* {
        auto* card = new QFrame(parent);
        card->setObjectName("settingsCard");
        return card;
    };

    auto makeSection = [makeCard](QWidget* parent, const QString& title) -> QPair<QFrame*, QVBoxLayout*> {
        auto* card = makeCard(parent);
        auto* cLayout = new QVBoxLayout(card);
        cLayout->setContentsMargins(14, 12, 14, 12);
        cLayout->setSpacing(8);

        auto* lbl = new QLabel(title, card);
        lbl->setObjectName("cardTitle");
        cLayout->addWidget(lbl);
        return qMakePair(card, cLayout);
    };

    // 1. 全局热键
    auto s1 = makeSection(contentWidget, "🌐 全局热键 (后台随时响应)");
    s1.second->addWidget(createKeyRow("开始截屏", "唤醒全景截屏覆盖层", m_globalSnippingEdit));
    s1.second->addWidget(createKeyRow("桌面贴图", "剪贴板图像或选区置顶贴图", m_globalPinEdit));
    layout->addWidget(s1.first);

    // 2. 截图操作
    auto s2 = makeSection(contentWidget, "✂️ 截图操作 (选区建立时生效)");
    s2.second->addWidget(createKeyRow("撤销标注", "撤销上一步绘制的标注图元", m_snippingUndoEdit));
    s2.second->addWidget(createKeyRow("选区贴图", "将当前选区转为置顶贴图", m_snippingPinEdit));
    s2.second->addWidget(createKeyRow("完成复制", "复制截图内容到剪贴板并退出", m_snippingConfirmEdit));
    s2.second->addWidget(createKeyRow("保存截图", "另存为图片文件 (支持 WebP/PNG/JPG)", m_snippingSaveEdit));
    s2.second->addWidget(createKeyRow("文字识别", "提取选区中的文字内容", m_snippingOcrEdit));
    s2.second->addWidget(createKeyRow("取消截屏", "放弃当前截屏并退出", m_snippingCancelEdit));
    layout->addWidget(s2.first);

    // 3. 标注工具
    auto s3 = makeSection(contentWidget, "🎨 标注工具 (单键快速切换)");
    s3.second->addWidget(createKeyRow("矩形框", "绘制空心/实心矩形框", m_toolRectEdit));
    s3.second->addWidget(createKeyRow("箭头", "绘制指示箭头矢量", m_toolArrowEdit));
    s3.second->addWidget(createKeyRow("画笔", "自由涂鸦笔迹", m_toolPencilEdit));
    s3.second->addWidget(createKeyRow("文字", "富文本原地输入标注", m_toolTextEdit));
    s3.second->addWidget(createKeyRow("马赛克", "局部打码脱敏遮罩", m_toolMosaicEdit));
    s3.second->addWidget(createKeyRow("单字修改", "字符就地改字与P图", m_toolCharEdit));
    layout->addWidget(s3.first);

    // 4. 贴图窗口
    auto s4 = makeSection(contentWidget, "📌 贴图窗口 (窗口聚焦时响应)");
    s4.second->addWidget(createKeyRow("文字识别", "识别当前贴图窗口中的文字", m_pinOcrEdit));
    s4.second->addWidget(createKeyRow("复制图片", "复制贴图到系统剪贴板", m_pinCopyEdit));
    s4.second->addWidget(createKeyRow("保存图片", "保存贴图为独立图像文件", m_pinSaveEdit));
    s4.second->addWidget(createKeyRow("关闭贴图", "关闭销毁当前置顶贴图窗口", m_pinCloseEdit));
    layout->addWidget(s4.first);

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
