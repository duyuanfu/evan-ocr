#include "plugin_market_dialog.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QUrl>
#include <QDir>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>

// -------------------------------------------------------------
// PluginCardWidget: 单个扩展插件卡片组件
// -------------------------------------------------------------
PluginCardWidget::PluginCardWidget(const PluginInfo& info, QWidget* parent)
    : QWidget(parent)
    , m_info(info)
{
    setupUi();
    refreshState();
}

void PluginCardWidget::setupUi()
{
    setObjectName("pluginCard");
    setStyleSheet(
        "QWidget#pluginCard {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 8px;"
        "}"
        "QWidget#pluginCard:hover {"
        "  border-color: #cbd5e1;"
        "}"
    );

    auto* cardLayout = new QVBoxLayout(this);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(8);

    // 顶部行：图标 + 标题 + Badge + 大小 + 操作按钮
    auto* topRow = new QHBoxLayout();
    topRow->setSpacing(10);

    m_iconLabel = new QLabel(m_info.iconEmoji, this);
    m_iconLabel->setStyleSheet("font-size: 26px; padding: 2px;");
    topRow->addWidget(m_iconLabel);

    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);

    auto* titleLine = new QHBoxLayout();
    titleLine->setSpacing(6);

    m_titleLabel = new QLabel(m_info.name, this);
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #0f172a;");
    titleLine->addWidget(m_titleLabel);

    m_categoryBadge = new QLabel(m_info.category, this);
    m_categoryBadge->setStyleSheet("font-size: 11px; color: #475569; background: #f1f5f9; padding: 1px 6px; border-radius: 4px; font-weight: 500;");
    titleLine->addWidget(m_categoryBadge);

    m_sizeLabel = new QLabel(QString("大小: %1").arg(m_info.sizeDisplay), this);
    m_sizeLabel->setStyleSheet("font-size: 11px; color: #64748b;");
    titleLine->addWidget(m_sizeLabel);

    titleLine->addStretch();
    titleLayout->addLayout(titleLine);

    topRow->addLayout(titleLayout, 1);

    // 状态标签
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    topRow->addWidget(m_statusLabel);

    // 主操作按钮 (安装 / 取消 / 重试)
    m_actionBtn = new QPushButton(this);
    m_actionBtn->setCursor(Qt::PointingHandCursor);
    m_actionBtn->setFixedHeight(32);
    topRow->addWidget(m_actionBtn);

    // 卸载按钮 (平时隐藏或已安装时出现)
    m_uninstallBtn = new QPushButton("🗑️ 卸载", this);
    m_uninstallBtn->setCursor(Qt::PointingHandCursor);
    m_uninstallBtn->setFixedHeight(32);
    m_uninstallBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  color: #ef4444;"
        "  border: 1px solid #fecaca;"
        "  border-radius: 5px;"
        "  padding: 4px 10px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: #fef2f2;"
        "  border-color: #ef4444;"
        "}"
    );
    topRow->addWidget(m_uninstallBtn);

    cardLayout->addLayout(topRow);

    // 描述信息
    m_descLabel = new QLabel(m_info.description, this);
    m_descLabel->setWordWrap(true);
    m_descLabel->setStyleSheet("font-size: 12px; color: #475569; line-height: 1.4;");
    cardLayout->addWidget(m_descLabel);

    // 下载/解压进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #e2e8f0;"
        "  border: none;"
        "  border-radius: 3px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #2563eb;"
        "  border-radius: 3px;"
        "}"
    );
    m_progressBar->hide();
    cardLayout->addWidget(m_progressBar);

    // 事件连接
    connect(m_actionBtn, &QPushButton::clicked, this, [this]() {
        if (m_info.status == PluginStatus::NotInstalled || m_info.status == PluginStatus::Failed) {
            emit installRequested(m_info.id);
        } else if (m_info.status == PluginStatus::Downloading) {
            emit cancelRequested(m_info.id);
        }
    });

    connect(m_uninstallBtn, &QPushButton::clicked, this, [this]() {
        emit uninstallRequested(m_info.id);
    });
}

void PluginCardWidget::refreshState()
{
    switch (m_info.status) {
    case PluginStatus::NotInstalled:
        m_statusLabel->setText("未安装");
        m_statusLabel->setStyleSheet("color: #64748b; font-size: 12px;");
        m_actionBtn->setText("📥 一键安装");
        m_actionBtn->setEnabled(true);
        m_actionBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #2563eb;"
            "  color: #ffffff;"
            "  border: 1px solid #1d4ed8;"
            "  border-radius: 5px;"
            "  padding: 4px 14px;"
            "  font-size: 12px;"
            "  font-weight: 600;"
            "}"
            "QPushButton:hover { background-color: #1d4ed8; }"
        );
        m_uninstallBtn->hide();
        m_progressBar->hide();
        break;

    case PluginStatus::Downloading:
        m_statusLabel->setText(m_info.statusMessage.isEmpty() ? "下载中..." : m_info.statusMessage);
        m_statusLabel->setStyleSheet("color: #2563eb; font-size: 12px; font-weight: bold;");
        m_actionBtn->setText("⏹ 取消");
        m_actionBtn->setEnabled(true);
        m_actionBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #ffffff;"
            "  color: #64748b;"
            "  border: 1px solid #cbd5e1;"
            "  border-radius: 5px;"
            "  padding: 4px 12px;"
            "  font-size: 12px;"
            "}"
            "QPushButton:hover { color: #ef4444; border-color: #ef4444; background: #fef2f2; }"
        );
        m_uninstallBtn->hide();
        m_progressBar->show();
        m_progressBar->setValue(m_info.progressPercent);
        break;

    case PluginStatus::Extracting:
        m_statusLabel->setText("⏳ 解压部署中...");
        m_statusLabel->setStyleSheet("color: #d97706; font-size: 12px; font-weight: bold;");
        m_actionBtn->setText("正在部署...");
        m_actionBtn->setEnabled(false);
        m_actionBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #f1f5f9;"
            "  color: #94a3b8;"
            "  border: 1px solid #e2e8f0;"
            "  border-radius: 5px;"
            "  padding: 4px 12px;"
            "  font-size: 12px;"
            "}"
        );
        m_uninstallBtn->hide();
        m_progressBar->show();
        m_progressBar->setValue(100);
        break;

    case PluginStatus::Installed:
        m_statusLabel->setText("✓ 已就绪");
        m_statusLabel->setStyleSheet("color: #059669; font-size: 12px; font-weight: bold;");
        m_actionBtn->setText("已安装");
        m_actionBtn->setEnabled(false);
        m_actionBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #ecfdf5;"
            "  color: #059669;"
            "  border: 1px solid #a7f3d0;"
            "  border-radius: 5px;"
            "  padding: 4px 12px;"
            "  font-size: 12px;"
            "  font-weight: 500;"
            "}"
        );
        m_uninstallBtn->show();
        m_progressBar->hide();
        break;

    case PluginStatus::Failed:
        {
            QString errSummary = "下载失败";
            if (m_info.statusMessage.contains("404") || m_info.statusMessage.contains("Not Found", Qt::CaseInsensitive)) {
                errSummary = "失败 (404 资源未上传)";
            } else if (m_info.statusMessage.contains("timeout", Qt::CaseInsensitive) || m_info.statusMessage.contains("timed out", Qt::CaseInsensitive)) {
                errSummary = "失败 (网络超时)";
            } else if (m_info.statusMessage.contains("Connection refused", Qt::CaseInsensitive) || m_info.statusMessage.contains("Host not found", Qt::CaseInsensitive)) {
                errSummary = "失败 (网络无法连通)";
            }
            m_statusLabel->setText(errSummary);
            m_statusLabel->setToolTip(m_info.statusMessage.isEmpty() ? "下载或部署发生错误，请检查网络或检查 GitHub Release 资源是否发布" : m_info.statusMessage);
        }
        m_statusLabel->setStyleSheet("color: #ef4444; font-size: 12px; font-weight: bold;");
        m_actionBtn->setText("🔄 重试");
        m_actionBtn->setEnabled(true);
        m_actionBtn->setStyleSheet(
            "QPushButton {"
            "  background-color: #2563eb;"
            "  color: #ffffff;"
            "  border: 1px solid #1d4ed8;"
            "  border-radius: 5px;"
            "  padding: 4px 12px;"
            "  font-size: 12px;"
            "  font-weight: 600;"
            "}"
            "QPushButton:hover { background-color: #1d4ed8; }"
        );
        m_uninstallBtn->hide();
        m_progressBar->hide();
        break;
    }
}

void PluginCardWidget::updateInfo(const PluginInfo& info)
{
    m_info = info;
    refreshState();
}

void PluginCardWidget::updateProgress(int percent, qint64 downloaded, qint64 total, double speedMBs)
{
    m_info.progressPercent = percent;
    m_info.downloadedBytes = downloaded;
    m_info.totalBytes = total;
    m_info.speedBytesPerSec = speedMBs;

    double dlMB = downloaded / (1024.0 * 1024.0);
    double totMB = (total > 0 ? total : m_info.sizeBytes) / (1024.0 * 1024.0);

    m_statusLabel->setText(QString("下载中 %1% (%2/%3 MB · %4 MB/s)")
                           .arg(percent)
                           .arg(dlMB, 0, 'f', 1)
                           .arg(totMB, 0, 'f', 1)
                           .arg(speedMBs, 0, 'f', 1));
    m_progressBar->show();
    m_progressBar->setValue(percent);
}

// -------------------------------------------------------------
// PluginMarketDialog: 插件中心主窗口
// -------------------------------------------------------------
PluginMarketDialog::PluginMarketDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Evan 扩展插件中心 (Plugin Market)");
    resize(820, 600);
    setMinimumSize(740, 520);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint);

    setupUi();

    // 监听 PluginManager 信号
    connect(&PluginManager::instance(), &PluginManager::pluginsRefreshed, this, &PluginMarketDialog::renderPluginList);

    connect(&PluginManager::instance(), &PluginManager::pluginStatusChanged, this, [this](const QString& id, PluginStatus, const QString&) {
        if (m_cardMap.contains(id)) {
            m_cardMap[id]->updateInfo(PluginManager::instance().pluginInfo(id));
        }
    });

    connect(&PluginManager::instance(), &PluginManager::pluginProgressChanged, this, [this](const QString& id, int percent, qint64 downloaded, qint64 total, double speedMBs) {
        if (m_cardMap.contains(id)) {
            m_cardMap[id]->updateProgress(percent, downloaded, total, speedMBs);
        }
    });

    PluginManager::instance().refreshAllStatus();
}

void PluginMarketDialog::setupUi()
{
    setStyleSheet(
        "QDialog {"
        "  background-color: #f8fafc;"
        "  color: #0f172a;"
        "}"
        "QLineEdit {"
        "  background-color: #ffffff;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 6px 12px;"
        "  font-size: 13px;"
        "  color: #0f172a;"
        "}"
        "QLineEdit:focus {"
        "  border-color: #3b82f6;"
        "}"
        "QScrollArea {"
        "  border: none;"
        "  background-color: transparent;"
        "}"
        "QScrollBar:vertical {"
        "  background: #f1f5f9;"
        "  width: 8px;"
        "  margin: 0px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #cbd5e1;"
        "  min-height: 20px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #94a3b8;"
        "}"
    );

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 18, 20, 16);
    mainLayout->setSpacing(12);

    // ==========================================================
    // 顶部 Word 经典菜单布局区 (Command Bar / Ribbon Action Bar)
    // ==========================================================
    auto* topMenuLayout = new QHBoxLayout();
    topMenuLayout->setSpacing(10);

    auto* titleLabel = new QLabel("插件中心", this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #0f172a; margin-right: 6px;");
    topMenuLayout->addWidget(titleLabel);

    // 经典顶部功能菜单动作按钮组
    m_refreshBtn = new QPushButton("刷新商城", this);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setToolTip("重新检查本地与云端扩展组件状态");
    m_refreshBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  color: #334155;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 5px 12px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover { background-color: #f1f5f9; border-color: #94a3b8; }"
    );
    connect(m_refreshBtn, &QPushButton::clicked, this, []() {
        PluginManager::instance().refreshAllStatus();
    });
    topMenuLayout->addWidget(m_refreshBtn);

    auto* installZipBtn = new QPushButton("本地导入 ZIP", this);
    installZipBtn->setCursor(Qt::PointingHandCursor);
    installZipBtn->setToolTip("从本地选择下载好的离线扩展压缩包 (.zip) 一键自动导入部署");
    installZipBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #eff6ff;"
        "  color: #2563eb;"
        "  border: 1px solid #bfdbfe;"
        "  border-radius: 6px;"
        "  padding: 5px 12px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover { background-color: #dbeafe; border-color: #3b82f6; }"
    );
    connect(installZipBtn, &QPushButton::clicked, this, [this]() {
        QString zipPath = QFileDialog::getOpenFileName(
            this,
            "选择离线插件扩展包 (.zip)",
            "",
            "扩展包 (*.zip);;所有文件 (*.*)"
        );
        if (!zipPath.isEmpty()) {
            PluginManager::instance().installFromLocalZip(zipPath);
        }
    });
    topMenuLayout->addWidget(installZipBtn);

    auto* openDirBtn = new QPushButton("打开插件目录", this);
    openDirBtn->setCursor(Qt::PointingHandCursor);
    openDirBtn->setToolTip("在资源管理器中查看本地 plugins/ 插件安装目录");
    openDirBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  color: #334155;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 5px 12px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover { background-color: #f1f5f9; border-color: #94a3b8; }"
    );
    connect(openDirBtn, &QPushButton::clicked, this, [this]() {
        QString dir = PluginManager::instance().pluginRootDir();
        QDir().mkpath(dir);
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });
    topMenuLayout->addWidget(openDirBtn);

    topMenuLayout->addStretch();

    // 顶部右侧：嵌入式搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("搜索插件名称、描述或语言...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedWidth(240);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PluginMarketDialog::filterPlugins);
    topMenuLayout->addWidget(m_searchEdit);

    mainLayout->addLayout(topMenuLayout);

    // Word 风格细分割线
    auto* ribbonSeparator = new QFrame(this);
    ribbonSeparator->setFrameShape(QFrame::HLine);
    ribbonSeparator->setStyleSheet("background-color: #e2e8f0; max-height: 1px; border: none;");
    mainLayout->addWidget(ribbonSeparator);

    // ==========================================
    // 第二行：分类切换标签页 (Ribbon Tabs)
    // ==========================================
    auto* filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(8);

    auto styleCategoryBtn = [](QPushButton* btn, bool active) {
        if (active) {
            btn->setStyleSheet(
                "QPushButton {"
                "  background-color: #eff6ff;"
                "  color: #2563eb;"
                "  border: 1px solid #3b82f6;"
                "  border-radius: 13px;"
                "  padding: 3px 12px;"
                "  font-size: 12px;"
                "  font-weight: 600;"
                "}"
            );
        } else {
            btn->setStyleSheet(
                "QPushButton {"
                "  background-color: #ffffff;"
                "  color: #64748b;"
                "  border: 1px solid #e2e8f0;"
                "  border-radius: 13px;"
                "  padding: 3px 12px;"
                "  font-size: 12px;"
                "  font-weight: 500;"
                "}"
                "QPushButton:hover { color: #0f172a; border-color: #cbd5e1; }"
            );
        }
    };

    m_filterAllBtn = new QPushButton("全部插件", this);
    m_filterOcrBtn = new QPushButton("文字识别 (OCR)", this);
    m_filterTransBtn = new QPushButton("离线翻译", this);
    m_filterInstalledBtn = new QPushButton("已安装", this);

    auto updateCategoryButtons = [=](const QString& selected) {
        styleCategoryBtn(m_filterAllBtn, selected == "All");
        styleCategoryBtn(m_filterOcrBtn, selected == "文字识别");
        styleCategoryBtn(m_filterTransBtn, selected == "离线翻译");
        styleCategoryBtn(m_filterInstalledBtn, selected == "Installed");
    };
    updateCategoryButtons(m_currentCategory);

    connect(m_filterAllBtn, &QPushButton::clicked, this, [=]() {
        m_currentCategory = "All";
        updateCategoryButtons("All");
        filterPlugins();
    });
    connect(m_filterOcrBtn, &QPushButton::clicked, this, [=]() {
        m_currentCategory = "文字识别";
        updateCategoryButtons("文字识别");
        filterPlugins();
    });
    connect(m_filterTransBtn, &QPushButton::clicked, this, [=]() {
        m_currentCategory = "离线翻译";
        updateCategoryButtons("离线翻译");
        filterPlugins();
    });
    connect(m_filterInstalledBtn, &QPushButton::clicked, this, [=]() {
        m_currentCategory = "Installed";
        updateCategoryButtons("Installed");
        filterPlugins();
    });

    filterLayout->addWidget(m_filterAllBtn);
    filterLayout->addWidget(m_filterOcrBtn);
    filterLayout->addWidget(m_filterTransBtn);
    filterLayout->addWidget(m_filterInstalledBtn);
    filterLayout->addStretch();

    mainLayout->addLayout(filterLayout);

    // ==========================================
    // 中间区域：插件卡片工作区
    // ==========================================
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);

    m_cardsContainer = new QWidget();
    m_cardsContainer->setStyleSheet("background-color: transparent;");
    m_cardsLayout = new QVBoxLayout(m_cardsContainer);
    m_cardsLayout->setContentsMargins(0, 4, 8, 4);
    m_cardsLayout->setSpacing(10);

    m_scrollArea->setWidget(m_cardsContainer);
    mainLayout->addWidget(m_scrollArea, 1);

    renderPluginList();

    // ==========================================
    // 底部栏：极简状态与关闭
    // ==========================================
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);

    auto* hintLabel = new QLabel("提示: 扩展包下载或导入后将自动解压热加载，无需重启软件", this);
    hintLabel->setStyleSheet("font-size: 11px; color: #64748b;");
    bottomLayout->addWidget(hintLabel);

    bottomLayout->addStretch();

    auto* closeBtn = new QPushButton("关闭", this);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  color: #334155;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 5px 18px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover { background-color: #f1f5f9; color: #0f172a; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);

    mainLayout->addLayout(bottomLayout);
}

void PluginMarketDialog::renderPluginList()
{
    // 清除既有卡片
    m_cardMap.clear();
    QLayoutItem* item;
    while ((item = m_cardsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    const auto& list = PluginManager::instance().plugins();
    for (const auto& p : list) {
        auto* card = new PluginCardWidget(p, m_cardsContainer);
        connect(card, &PluginCardWidget::installRequested, this, [](const QString& id) {
            PluginManager::instance().installPlugin(id);
        });
        connect(card, &PluginCardWidget::cancelRequested, this, [](const QString& id) {
            PluginManager::instance().cancelDownload(id);
        });
        connect(card, &PluginCardWidget::uninstallRequested, this, [this](const QString& id) {
            auto reply = QMessageBox::question(
                this,
                "确认卸载插件",
                "确定要卸载该扩展插件吗？卸载后将自动删除本地模型文件并释放磁盘空间。",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            if (reply == QMessageBox::Yes) {
                PluginManager::instance().uninstallPlugin(id);
            }
        });

        m_cardsLayout->addWidget(card);
        m_cardMap[p.id] = card;
    }

    m_cardsLayout->addStretch();
    filterPlugins();
}

void PluginMarketDialog::filterPlugins()
{
    QString query = m_searchEdit ? m_searchEdit->text().trimmed().toLower() : "";
    const auto& list = PluginManager::instance().plugins();

    for (const auto& p : list) {
        if (!m_cardMap.contains(p.id)) continue;
        auto* card = m_cardMap[p.id];

        bool matchCategory = true;
        if (m_currentCategory == "离线翻译") {
            matchCategory = (p.category == "离线翻译");
        } else if (m_currentCategory == "文字识别") {
            matchCategory = (p.category == "文字识别");
        } else if (m_currentCategory == "Installed") {
            matchCategory = (p.status == PluginStatus::Installed);
        }

        bool matchQuery = true;
        if (!query.isEmpty()) {
            matchQuery = p.name.toLower().contains(query) ||
                         p.description.toLower().contains(query) ||
                         p.category.toLower().contains(query);
        }

        card->setVisible(matchCategory && matchQuery);
    }
}
