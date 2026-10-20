#include "update_dialog.h"
#include "../../core/theme_manager.h"
#include <QDesktopServices>
#include <QMessageBox>
#include <QCoreApplication>
#include <QProcess>
#include <QDir>

UpdateDialog::UpdateDialog(const UpdateInfo& info, QWidget* parent)
    : QDialog(parent)
    , m_info(info)
{
    setupUi();
    applyTheme();

    connect(&UpdateManager::instance(), &UpdateManager::downloadProgress, this, &UpdateDialog::onDownloadProgress);
    connect(&UpdateManager::instance(), &UpdateManager::downloadFinished, this, &UpdateDialog::onDownloadFinished);
    connect(&UpdateManager::instance(), &UpdateManager::downloadFailed, this, &UpdateDialog::onDownloadFailed);
}

UpdateDialog::~UpdateDialog()
{
    if (m_isDownloading) {
        UpdateManager::instance().cancelDownload();
    }
}

void UpdateDialog::setupUi()
{
    setWindowTitle("软件版本更新 - Evan");
    setFixedSize(540, 480);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    // 1. 顶部标题与版本对比信息
    auto* headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);

    m_titleLabel = new QLabel(QString("🚀 发现新版本 Evan v%1").arg(m_info.version), this);
    m_titleLabel->setStyleSheet("font-size: 17px; font-weight: bold; color: #3b82f6;");
    headerLayout->addWidget(m_titleLabel);

    QString currentVer = QCoreApplication::applicationVersion();
    double sizeMb = m_info.sizeBytes > 0 ? (m_info.sizeBytes / (1024.0 * 1024.0)) : 21.5;
    m_subTitleLabel = new QLabel(
        QString("当前安装: v%1  ➔  最新版本: v%2  (更新包体积: %3 MB)")
        .arg(currentVer, m_info.version)
        .arg(sizeMb, 0, 'f', 1),
        this
    );
    m_subTitleLabel->setStyleSheet("font-size: 12px; color: #71717a;");
    headerLayout->addWidget(m_subTitleLabel);

    mainLayout->addLayout(headerLayout);

    // 2. 更新日志查看区
    auto* notesTitle = new QLabel("📝 更新日志与新增特性：", this);
    notesTitle->setStyleSheet("font-size: 13px; font-weight: bold;");
    mainLayout->addWidget(notesTitle);

    m_notesBrowser = new QTextBrowser(this);
    m_notesBrowser->setOpenExternalLinks(true);
    if (!m_info.releaseNotes.trimmed().isEmpty()) {
        m_notesBrowser->setMarkdown(m_info.releaseNotes);
    } else {
        m_notesBrowser->setHtml(
            "<p><b>Evan v" + m_info.version + "</b> 带来全新架构升级、数学公式识别及性能优化。</p>"
            "<p>推荐所有用户升级体验更稳定流畅的截贴图生态！</p>"
        );
    }
    mainLayout->addWidget(m_notesBrowser, 1);

    // 3. 下载进度条区域 (初始隐藏)
    m_progressContainer = new QWidget(this);
    auto* progLayout = new QVBoxLayout(m_progressContainer);
    progLayout->setContentsMargins(0, 0, 0, 0);
    progLayout->setSpacing(4);

    m_progressBar = new QProgressBar(m_progressContainer);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFixedHeight(16);
    progLayout->addWidget(m_progressBar);

    m_progressStatusLabel = new QLabel("正在建立连接...", m_progressContainer);
    m_progressStatusLabel->setStyleSheet("font-size: 11px; color: #71717a;");
    progLayout->addWidget(m_progressStatusLabel);

    m_progressContainer->setVisible(false);
    mainLayout->addWidget(m_progressContainer);

    // 4. 底部动作栏
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_manualBtn = new QPushButton("🌐 浏览器下载", this);
    m_manualBtn->setCursor(Qt::PointingHandCursor);
    m_manualBtn->setToolTip("打开官方 Releases 页面手动下载便携包");
    btnLayout->addWidget(m_manualBtn);

    btnLayout->addStretch();

    m_cancelBtn = new QPushButton("稍后提醒", this);
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    btnLayout->addWidget(m_cancelBtn);

    m_updateBtn = new QPushButton("立即一键更新", this);
    m_updateBtn->setCursor(Qt::PointingHandCursor);
    m_updateBtn->setStyleSheet(
        "QPushButton { background-color: #3b82f6; color: #ffffff; font-weight: bold; border-radius: 6px; padding: 6px 16px; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:pressed { background-color: #1d4ed8; }"
        "QPushButton:disabled { background-color: #94a3b8; }"
    );
    btnLayout->addWidget(m_updateBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_updateBtn, &QPushButton::clicked, this, &UpdateDialog::onDownloadClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &UpdateDialog::onCancelClicked);
    connect(m_manualBtn, &QPushButton::clicked, this, &UpdateDialog::onManualDownloadClicked);
}

void UpdateDialog::applyTheme()
{
    bool dark = ThemeManager::instance().isDarkMode();
    if (dark) {
        setStyleSheet(
            "QDialog { background-color: #18181b; color: #f4f4f5; font-family: 'Segoe UI', 'Microsoft YaHei'; }"
            "QLabel { color: #f4f4f5; }"
            "QTextBrowser { background-color: #27272a; border: 1px solid #3f3f46; border-radius: 8px; padding: 8px; color: #e4e4e7; font-size: 12px; }"
            "QProgressBar { background-color: #27272a; border: 1px solid #3f3f46; border-radius: 8px; text-align: center; color: #ffffff; font-size: 10px; }"
            "QProgressBar::chunk { background-color: #3b82f6; border-radius: 7px; }"
            "QPushButton { background-color: #27272a; border: 1px solid #3f3f46; border-radius: 6px; color: #f4f4f5; padding: 6px 14px; font-size: 12px; }"
            "QPushButton:hover { background-color: #3f3f46; }"
        );
    } else {
        setStyleSheet(
            "QDialog { background-color: #ffffff; color: #18181b; font-family: 'Segoe UI', 'Microsoft YaHei'; }"
            "QLabel { color: #18181b; }"
            "QTextBrowser { background-color: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px; padding: 8px; color: #334155; font-size: 12px; }"
            "QProgressBar { background-color: #e2e8f0; border: none; border-radius: 8px; text-align: center; color: #1e293b; font-size: 10px; }"
            "QProgressBar::chunk { background-color: #3b82f6; border-radius: 8px; }"
            "QPushButton { background-color: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; color: #334155; padding: 6px 14px; font-size: 12px; }"
            "QPushButton:hover { background-color: #e2e8f0; }"
        );
    }
}

void UpdateDialog::onDownloadClicked()
{
    if (m_isDownloading) return;

    m_isDownloading = true;
    m_updateBtn->setEnabled(false);
    m_cancelBtn->setText("取消下载");
    m_progressContainer->setVisible(true);
    m_progressBar->setValue(0);
    m_progressStatusLabel->setText("正在连接下载服务器...");

    UpdateManager::instance().startDownload(m_info);
}

void UpdateDialog::onCancelClicked()
{
    if (m_isDownloading) {
        UpdateManager::instance().cancelDownload();
        m_isDownloading = false;
        m_updateBtn->setEnabled(true);
        m_cancelBtn->setText("稍后提醒");
        m_progressContainer->setVisible(false);
    } else {
        reject();
    }
}

void UpdateDialog::onManualDownloadClicked()
{
    QString targetUrl = m_info.downloadUrl;
    if (targetUrl.isEmpty()) {
        targetUrl = "https://github.com/duyuanfu/evan-ocr/releases/latest";
    }
    QDesktopServices::openUrl(QUrl(targetUrl));
}

void UpdateDialog::onDownloadProgress(qint64 received, qint64 total, double speedMBs)
{
    if (total > 0) {
        int percent = static_cast<int>((received * 100) / total);
        m_progressBar->setValue(percent);
        double recMb = received / (1024.0 * 1024.0);
        double totMb = total / (1024.0 * 1024.0);
        m_progressStatusLabel->setText(
            QString("正在下载: %1 MB / %2 MB (%3%) · 实时速度: %4 MB/s")
            .arg(recMb, 0, 'f', 1)
            .arg(totMb, 0, 'f', 1)
            .arg(percent)
            .arg(speedMBs, 0, 'f', 2)
        );
    } else {
        m_progressBar->setValue(0);
        double recMb = received / (1024.0 * 1024.0);
        m_progressStatusLabel->setText(QString("已下载: %1 MB (实时速度: %2 MB/s)").arg(recMb, 0, 'f', 1).arg(speedMBs, 0, 'f', 2));
    }
}

void UpdateDialog::onDownloadFinished(const QString& tempZipPath)
{
    m_isDownloading = false;
    m_progressBar->setValue(100);
    m_progressStatusLabel->setText("下载完成，正在准备自动更新...");

    // 1. 检查当前安装目录的写入权限
    QString permError;
    if (!UpdateManager::instance().checkInstallDirWritePermission(&permError)) {
        int ret = QMessageBox::warning(
            this,
            "缺少写入权限",
            permError + "\n\n更新包已下载至临时目录，是否立即打开所在文件夹并手动解压覆盖？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );
        if (ret == QMessageBox::Yes) {
            QString explorerCmd = QString("/select,\"%1\"").arg(QDir::toNativeSeparators(tempZipPath));
            QProcess::startDetached("explorer.exe", {explorerCmd});
        }
        accept();
        return;
    }

    // 2. 启动自升级外部批处理并重启
    QString execError;
    if (!UpdateManager::instance().executeUpdaterAndRestart(tempZipPath, &execError)) {
        QMessageBox::critical(this, "自升级启动失败", "无法启动升级脚本:\n" + execError);
        m_updateBtn->setEnabled(true);
        m_cancelBtn->setText("关闭");
        return;
    }

    // 3. 提示并退出当前旧程序
    QMessageBox::information(
        this,
        "即将升级重启",
        "更新包解压部署助手已启动！\n软件将自动退出并无感替换重启到最新版本。"
    );

    accept();
    QCoreApplication::quit();
}

void UpdateDialog::onDownloadFailed(const QString& errorMsg)
{
    m_isDownloading = false;
    m_updateBtn->setEnabled(true);
    m_cancelBtn->setText("稍后提醒");
    m_progressStatusLabel->setText("下载失败");

    QMessageBox::warning(this, "下载更新失败", errorMsg + "\n\n如遇网络阻断，建议在设置中配置自定义网络代理，或点击【浏览器下载】直接获取。");
}
