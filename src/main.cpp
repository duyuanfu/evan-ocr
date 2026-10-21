#include <QApplication>
#include <QDebug>
#include <QKeySequence>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QSettings>
#include <QFile>
#include <QClipboard>
#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QMutex>
#include "core/hotkey_config.h"
#include "core/screen_capturer.h"
#include "core/theme_manager.h"
#include "core/autostart_helper.h"
#include "core/network/network_proxy_manager.h"
#include "core/update/update_manager.h"
#include "platform/hotkey_manager.h"
#include "ui/overlay/snipping_overlay.h"
#include "ui/pin/pin_manager.h"
#include "ui/settings/hotkey_settings_dialog.h"
#include "ui/plugin/plugin_market_dialog.h"
#include "ui/update/update_dialog.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 获取应用程序图标 (QRC 内置资源 -> 外部资源目录 -> 动态内存矢量绘制保底)
static QIcon getApplicationIcon()
{
    // 1. 优先从 Qt 二进制内嵌资源读取 (不受任何工作目录或外部路径影响)
    if (QFile::exists(":/icons/app_icon_256.png")) {
        return QIcon(":/icons/app_icon_256.png");
    }
    if (QFile::exists(":/icons/app.ico")) {
        return QIcon(":/icons/app.ico");
    }

    // 2. 尝试从可执行文件所在目录读取外部资源
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/resources/app_icon_256.png",
        appDir + "/resources/app.ico",
        "resources/app_icon_256.png",
        "resources/app.ico"
    };
    for (const auto& path : candidates) {
        if (QFile::exists(path)) {
            return QIcon(path);
        }
    }

    // 3. 终极保底：内存矢量动态绘制高科技质感应用图标 (无论何种极端环境 100% 绝对可见)
    const int s = 64;
    QPixmap fallback(s, s);
    fallback.fill(Qt::transparent);
    {
        QPainter p(&fallback);
        p.setRenderHint(QPainter::Antialiasing, true);

        // 绘制高穿透力活力科技蓝渐变底板 (高饱和高对比，黑白任务栏均极具视觉辨识度)
        QLinearGradient grad(0, 0, s, s);
        grad.setColorAt(0.0, QColor(0, 168, 255));
        grad.setColorAt(1.0, QColor(0, 82, 204));
        p.setBrush(grad);
        p.setPen(QPen(QColor(255, 255, 255, 180), 1.5));
        p.drawRoundedRect(2, 2, s - 4, s - 4, 15, 15);

        // 绘制纯白加粗取景器四角定位框
        p.setPen(QPen(Qt::white, 3.8));
        p.drawLine(8, 20, 8, 8);
        p.drawLine(8, 8, 20, 8);
        p.drawLine(s - 20, 8, s - 8, 8);
        p.drawLine(s - 8, 8, s - 8, 20);
        p.drawLine(8, s - 20, 8, s - 8);
        p.drawLine(8, s - 8, 20, s - 8);
        p.drawLine(s - 20, s - 8, s - 8, s - 8);
        p.drawLine(s - 8, s - 20, s - 8, s - 8);

        // 绘制贯穿式鲜艳金橙色 OCR 激光扫描线 (#FFB300)
        p.setPen(QPen(QColor(255, 179, 0), 2.8));
        p.drawLine(10, s / 2, s - 10, s / 2);

        // 绘制中心镜头对焦圆环与纯白核心
        p.setPen(QPen(Qt::white, 2.5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPoint(s / 2, s / 2), 9, 9);

        p.setBrush(QColor(255, 179, 0));
        p.drawEllipse(QPoint(s / 2, s / 2), 4, 4);
        p.setBrush(Qt::white);
        p.drawEllipse(QPoint(s / 2, s / 2), 2, 2);
    }
    return QIcon(fallback);
}

/**
 * @brief 安装轻量级日志处理器
 *
 * Evan 是 GUI 程序 (WIN32 子系统)，qDebug 默认只输出到调试器，
 * 用户无法查看。这里把日志同时写入 %LOCALAPPDATA%\Evan\evan.log，
 * 便于排查多屏混合缩放、截屏错位等问题。
 *
 * 注意：日志文件对象故意使用「故意泄漏」的裸指针，避免静态析构顺序问题
 * (静态 QFile 在退出时被析构，而此时仍可能有 qDebug 输出 → 写已析构对象)。
 */
static void installFileLogger()
{
    static QFile* logFile = new QFile;   // 故意泄漏，避免退出期析构顺序问题
    static QMutex* logMutex = new QMutex;
    static bool* enabled = new bool(false);
    static QtMessageHandler prevHandler = nullptr;

    const QString logDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                               .filePath(QStringLiteral("Evan"));
    if (logDir.isEmpty()) return;
    QDir().mkpath(logDir);
    const QString logPath = QDir(logDir).filePath(QStringLiteral("evan.log"));

    // 超过 2MB 时轮转为 .log.1，避免日志无限增长同时保留上一份历史
    QFileInfo fi(logPath);
    if (fi.exists() && fi.size() > 2 * 1024 * 1024) {
        const QString oldPath = logPath + QStringLiteral(".1");
        QFile::remove(oldPath);
        QFile::rename(logPath, oldPath);
    }

    logFile->setFileName(logPath);
    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    *enabled = true;

    prevHandler = qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& msg) {
        // 保留原有处理 (输出到调试器 / stderr)，避免丢失开发者调试信息
        if (prevHandler) {
            prevHandler(type, context, msg);
        }
        if (!*enabled) return;

        const char* level = "INFO";
        switch (type) {
            case QtDebugMsg:    level = "DEBUG"; break;
            case QtInfoMsg:     level = "INFO";  break;
            case QtWarningMsg:  level = "WARN";  break;
            case QtCriticalMsg: level = "CRIT";  break;
            case QtFatalMsg:    level = "FATAL"; break;
        }

        QString source;
        if (context.file) {
            source = QStringLiteral(" (%1:%2)")
                         .arg(QString::fromUtf8(context.file))
                         .arg(context.line);
        }

        QMutexLocker locker(logMutex);
        const QString line = QStringLiteral("[%1] %2: %3%4\n")
                                 .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                                      QString::fromLatin1(level), msg, source);
        logFile->write(line.toUtf8());
        logFile->flush();
    });
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // 声明 Windows Per-Monitor DPI Aware V2，彻底消除系统二次位图拉伸造成的模糊
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // 单实例互斥锁，禁止重复点击启动多个后台进程
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"Evan_SingleInstance_Mutex_Unique");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        MessageBoxW(nullptr,
            L"Evan 已经在后台运行中！\n可在任务栏右下角托盘图标中操作，或使用预设快捷键截屏。",
            L"Evan",
            MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        return 0;
    }
#endif

    // 启用高 DPI 缩放支持与整数缩放策略
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough
    );

    // 安装文件日志 (GUI 程序无法看到控制台输出)
    installFileLogger();

    QApplication app(argc, argv);
    app.setApplicationName("Evan");
    app.setApplicationDisplayName("Evan 截贴图工具");
    app.setOrganizationName("Evan");
#ifdef EVAN_VERSION
    app.setApplicationVersion(EVAN_VERSION);
#else
    app.setApplicationVersion("1.2.1");
#endif
    app.setQuitOnLastWindowClosed(false);

    // 启动时打印显示器物理布局，便于排查多屏混合缩放问题
    {
#ifdef EVAN_VERSION
        const QString evanVersion = QStringLiteral(EVAN_VERSION);
#else
        const QString evanVersion = QStringLiteral("dev");
#endif
        const auto screens = QGuiApplication::screens();
        qDebug() << "[Evan] 版本" << evanVersion << "显示器数量:" << screens.size();
        for (auto* sc : screens) {
            const QRect g = sc->geometry();
            const qreal dpr = sc->devicePixelRatio();
            qDebug() << "   - " << sc->name()
                     << "逻辑几何=(" << g.x() << "," << g.y() << " " << g.width() << "x" << g.height() << ")"
                     << "DPR=" << dpr
                     << "物理矩形=(" << g.x() << "," << g.y() << " "
                     << qRound(g.width() * dpr) << "x" << qRound(g.height() * dpr) << ")";
        }
        qDebug() << "[Evan] 物理虚拟桌面:" << ScreenCapturer::physicalVirtualGeometry();
    }

    // 全局集中初始化网络代理 (覆盖更新检查、版本下载、插件市场与公式云端识别)
    NetworkProxyManager::instance().apply();

    // 4K / 高 DPI 屏幕全真抗锯齿字体渲染策略 (消除文字与边缘锯齿感)
    QFont appFont("Microsoft YaHei UI", 9);
    appFont.setStyleStrategy(static_cast<QFont::StyleStrategy>(QFont::PreferAntialias | QFont::PreferQuality));
    app.setFont(appFont);

    QIcon appIcon = getApplicationIcon();
    app.setWindowIcon(appIcon);

    // 确保初次启动默认开启开机自启动
    AutoStartHelper::ensureInitialDefaultSetup();

    // 初始化快捷键配置
    HotkeyConfig::instance().load();
    const auto& currentConfig = HotkeyConfig::instance().data();

    // 创建系统托盘图标
    QSystemTrayIcon trayIcon(appIcon, &app);
    trayIcon.setIcon(appIcon);
    trayIcon.setVisible(true);

    QMenu trayMenu;
    auto updateTrayStyle = [&trayMenu]() {
        trayMenu.setStyleSheet(ThemeManager::instance().getTrayMenuStyle());
    };
    updateTrayStyle();
    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, updateTrayStyle);

    auto* snipAction = trayMenu.addAction("开始截屏");
    auto* fullScreenAction = trayMenu.addAction("一键全屏截图");
    auto* pinAction = trayMenu.addAction("桌面贴图");
    trayMenu.addSeparator();
    auto* pluginAction = trayMenu.addAction("🧩 插件中心...");
    auto* updateAction = trayMenu.addAction("🔄 检查更新...");
    auto* settingsAction = trayMenu.addAction("⚙️ 设置...");
    auto* aboutAction = trayMenu.addAction("ℹ️ 关于 Evan...");
    trayMenu.addSeparator();
    auto* quitAction = trayMenu.addAction("退出 Evan");

    auto doClipboardPin = [&trayIcon]() {
        if (SnippingOverlay::instance().isVisible() && SnippingOverlay::instance().hasValidSelection()) {
            SnippingOverlay::instance().triggerPinAction();
            return;
        }

        QClipboard* clipboard = QApplication::clipboard();
        QPixmap pix = clipboard->pixmap();
        if (!pix.isNull() && pix.width() > 0 && pix.height() > 0) {
            QPoint cursorPos = QCursor::pos();
            int w = static_cast<int>(pix.width() / pix.devicePixelRatio());
            int h = static_cast<int>(pix.height() / pix.devicePixelRatio());
            QRect targetRect(cursorPos.x() - w / 2, cursorPos.y() - h / 2, w, h);
            PinManager::instance().createPin(pix, targetRect);
        } else {
            trayIcon.showMessage("贴图提示", "剪贴板中未检测到有效图像数据，请先截图或复制图片。", QSystemTrayIcon::Information, 2000);
        }
    };

    QObject::connect(snipAction, &QAction::triggered, []() {
        SnippingOverlay::instance().startSnipping();
    });

    QObject::connect(fullScreenAction, &QAction::triggered, []() {
        SnippingOverlay::instance().startFullScreenCapture();
    });

    QObject::connect(pinAction, &QAction::triggered, doClipboardPin);

    QObject::connect(pluginAction, &QAction::triggered, []() {
        static PluginMarketDialog* marketDlg = nullptr;
        if (!marketDlg) {
            marketDlg = new PluginMarketDialog();
        }
        marketDlg->show();
        marketDlg->raise();
        marketDlg->activateWindow();
    });

    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    trayIcon.setContextMenu(&trayMenu);

    // 单击托盘图标直接触发截图
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, [](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            SnippingOverlay::instance().startSnipping();
        }
    });

    trayIcon.show();

    // 动态全局热键管理变量
    static int g_snipHotkeyId = 0;
    static int g_pinHotkeyId = 0;
    static int g_fullScreenHotkeyId = 0;

    auto registerAllGlobalHotkeys = [&]() {
        const auto& c = HotkeyConfig::instance().data();

        // 1. 全局截屏热键
        if (g_snipHotkeyId != 0) {
            HotkeyManager::instance().unregisterHotkey(g_snipHotkeyId);
            g_snipHotkeyId = 0;
        }
        if (!c.globalSnipping.trimmed().isEmpty()) {
            g_snipHotkeyId = HotkeyManager::instance().registerHotkey(QKeySequence(c.globalSnipping), []() {
                qDebug() << "[Evan] 捕获全局热键唤醒截屏";
                SnippingOverlay::instance().startSnipping();
            });
            if (g_snipHotkeyId == 0) {
                qWarning() << "[Evan] 全局截屏热键注册失败:" << c.globalSnipping;
            }
        }

        // 2. 全局贴图热键
        if (g_pinHotkeyId != 0) {
            HotkeyManager::instance().unregisterHotkey(g_pinHotkeyId);
            g_pinHotkeyId = 0;
        }
        if (!c.globalPin.trimmed().isEmpty()) {
            g_pinHotkeyId = HotkeyManager::instance().registerHotkey(QKeySequence(c.globalPin), doClipboardPin);
            if (g_pinHotkeyId == 0) {
                qWarning() << "[Evan] 全局贴图热键注册失败:" << c.globalPin;
            }
        }

        // 3. 全局一键全屏热键
        if (g_fullScreenHotkeyId != 0) {
            HotkeyManager::instance().unregisterHotkey(g_fullScreenHotkeyId);
            g_fullScreenHotkeyId = 0;
        }
        if (!c.globalFullScreen.trimmed().isEmpty()) {
            g_fullScreenHotkeyId = HotkeyManager::instance().registerHotkey(QKeySequence(c.globalFullScreen), []() {
                qDebug() << "[Evan] 捕获全局热键启动一键全屏截图";
                SnippingOverlay::instance().startFullScreenCapture();
            });
            if (g_fullScreenHotkeyId == 0) {
                qWarning() << "[Evan] 全局全屏热键注册失败:" << c.globalFullScreen;
            }
        }

        // 更新托盘与动作提示
        snipAction->setText(c.globalSnipping.isEmpty() ? "开始截屏" : QString("开始截屏 (%1)").arg(c.globalSnipping));
        pinAction->setText(c.globalPin.isEmpty() ? "桌面贴图" : QString("桌面贴图 (%1)").arg(c.globalPin));
        fullScreenAction->setText(c.globalFullScreen.isEmpty() ? "一键全屏截图" : QString("一键全屏截图 (%1)").arg(c.globalFullScreen));
        trayIcon.setToolTip(QString("Evan 截贴图工具\n截屏: %1 | 全屏: %2 | 贴图: %3").arg(
            c.globalSnipping.isEmpty() ? "未设置" : c.globalSnipping,
            c.globalFullScreen.isEmpty() ? "未设置" : c.globalFullScreen,
            c.globalPin.isEmpty() ? "未设置" : c.globalPin
        ));
    };

    // 初始注册全局热键
    registerAllGlobalHotkeys();

    // 监听快捷键配置变动
    QObject::connect(&HotkeyConfig::instance(), &HotkeyConfig::configChanged, [&]() {
        registerAllGlobalHotkeys();
    });

    // 软件自动更新检查与升级联动
    static UpdateInfo g_pendingUpdateInfo;
    static bool g_hasPendingUpdate = false;

    QObject::connect(&UpdateManager::instance(), &UpdateManager::updateAvailable,
                     [&trayIcon](const UpdateInfo& info, bool silent) {
        g_pendingUpdateInfo = info;
        g_hasPendingUpdate = true;
        if (silent) {
            trayIcon.showMessage(
                "发现新版本",
                QString("Evan v%1 现已就绪！点击此处查看更新说明并一键升级。").arg(info.version),
                QSystemTrayIcon::Information,
                7000
            );
        } else {
            UpdateDialog dlg(info);
            dlg.exec();
        }
    });

    QObject::connect(&UpdateManager::instance(), &UpdateManager::noUpdateAvailable,
                     [](bool silent) {
        if (!silent) {
            QMessageBox::information(
                nullptr,
                "检查更新",
                QString("当前已是最新版本 (v%1)！无需更新。").arg(QCoreApplication::applicationVersion())
            );
        }
    });

    QObject::connect(&UpdateManager::instance(), &UpdateManager::checkFailed,
                     [](const QString& errorMsg, bool silent) {
        if (!silent) {
            QMessageBox::warning(
                nullptr,
                "检查更新失败",
                QString("检查更新失败: %1\n\n请检查网络连接或在设置中配置自定义代理后再试。").arg(errorMsg)
            );
        }
    });

    QObject::connect(updateAction, &QAction::triggered, []() {
        UpdateManager::instance().checkForUpdates(false);
    });

    // 点击气泡提醒直接唤起更新弹窗
    QObject::connect(&trayIcon, &QSystemTrayIcon::messageClicked, [&]() {
        if (g_hasPendingUpdate) {
            UpdateDialog dlg(g_pendingUpdateInfo);
            dlg.exec();
        }
    });

    // 快捷键设置对话框联动
    QObject::connect(settingsAction, &QAction::triggered, [&]() {
        HotkeySettingsDialog dlg;
        dlg.exec();
    });

    // 关于与许可协议对话框
    QObject::connect(aboutAction, &QAction::triggered, [&]() {
        QMessageBox::about(nullptr, "关于 Evan",
            QString("<h3>Evan v%1</h3>"
                    "<p>轻量、低延迟的 Windows 现代化截贴图与原生离线 OCR 工具。</p>"
                    "<hr/>"
                    "<p><b>联系作者 / 交流反馈：</b></p>"
                    "<p>📧 邮箱：<a href=\"mailto:duyuanfu@yeah.net\">duyuanfu@yeah.net</a></p>"
                    "<p>💬 QQ：<b>2860421826</b></p>"
                    "<hr/>"
                    "<p>开源许可协议：<b>MIT License</b></p>"
                    "<p>Copyright &copy; 2026 evan. All rights reserved.</p>")
            .arg(QCoreApplication::applicationVersion())
        );
    });

    // 启动延迟 3.5 秒在后台静默检查一次更新 (可在设置中关闭)
    QTimer::singleShot(3500, []() {
        if (UpdateManager::instance().autoCheckOnStartup()) {
            qDebug() << "[Evan] 触发启动静默检查更新";
            UpdateManager::instance().checkForUpdates(true);
        }
    });

    // 弹出启动气泡提示
    trayIcon.showMessage("Evan 已启动",
        QString("截屏快捷键: %1，贴图快捷键: %2\n可在托盘右键个性化自定义快捷键。")
        .arg(currentConfig.globalSnipping, currentConfig.globalPin),
        QSystemTrayIcon::Information, 3000);

    qDebug() << "[Evan] 应用程序初始化成功，当前截屏快捷键:" << currentConfig.globalSnipping
             << "贴图快捷键:" << currentConfig.globalPin;

    // 监听截图完成事件
    QObject::connect(&SnippingOverlay::instance(), &SnippingOverlay::snippingFinished,
                     [](const QPixmap& pixmap, const QRect& region) {
        qDebug() << "[Evan] 截图完成，已写入系统剪贴板! 选区:" << region
                 << "尺寸:" << pixmap.size();
    });

    // 监听贴图请求事件 (将选区及其标注转为独立置顶 Pin 窗口)
    QObject::connect(&SnippingOverlay::instance(), &SnippingOverlay::pinRequested,
                     [](const QPixmap& pixmap, const QRect& screenPos) {
        qDebug() << "[Evan] 收到贴图请求，创建独立 PinWindow 置顶贴图，物理位置:" << screenPos;
        PinManager::instance().createPin(pixmap, screenPos);
    });

    // 监听截图取消事件
    QObject::connect(&SnippingOverlay::instance(), &SnippingOverlay::snippingCancelled, []() {
        qDebug() << "[Evan] 截图已取消";
    });

    int ret = app.exec();

#ifdef Q_OS_WIN
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
#endif

    return ret;
}
