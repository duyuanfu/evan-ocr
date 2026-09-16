#include <QApplication>
#include <QDebug>
#include <QKeySequence>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QSettings>
#include <QFile>
#include "platform/hotkey_manager.h"
#include "ui/overlay/snipping_overlay.h"
#include "ui/pin/pin_manager.h"
#include "ui/settings/hotkey_settings_dialog.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 获取高端科技风应用程序图标 (优先加载高清资源)
static QIcon getApplicationIcon()
{
    if (QFile::exists("resources/app_icon_256.png")) {
        return QIcon("resources/app_icon_256.png");
    }
    if (QFile::exists("resources/app.ico")) {
        return QIcon("resources/app.ico");
    }
    return QIcon();
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // 声明 Windows Per-Monitor DPI Aware V2，彻底消除系统二次位图拉伸造成的模糊
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // 单实例互斥锁，禁止重复点击启动多个后台进程
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"EvanOCR_SingleInstance_Mutex_Unique");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        QMessageBox::information(nullptr, "EvanOCR", "EvanOCR 已经在后台运行中！\n可在任务栏右下角托盘图标中操作，或使用预设快捷键截屏。");
        return 0;
    }
#endif

    // 启用高 DPI 缩放支持与整数缩放策略
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough
    );

    QApplication app(argc, argv);
    app.setApplicationName("EvanOCR");
    app.setApplicationDisplayName("EvanOCR 截贴图工具");
    app.setOrganizationName("EvanOCR");
    app.setQuitOnLastWindowClosed(false);

    QIcon appIcon = getApplicationIcon();
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    // 从 QSettings 读取用户配置的快捷键（默认为 F1）
    QSettings settings("EvanOCR", "EvanOCR");
    QString currentHotkeyStr = settings.value("Hotkey/Snipping", "F1").toString();
    if (currentHotkeyStr.trimmed().isEmpty()) {
        currentHotkeyStr = "F1";
    }

    // 创建系统托盘图标
    QSystemTrayIcon trayIcon(appIcon.isNull() ? QIcon() : appIcon, &app);
    trayIcon.setToolTip(QString("EvanOCR 截贴图工具 (按 %1 截屏)").arg(currentHotkeyStr));

    QMenu trayMenu;
    trayMenu.setStyleSheet(
        "QMenu { background-color: #242424; color: #ffffff; border: 1px solid #3c3c3c; padding: 4px; }"
        "QMenu::item { padding: 6px 24px; border-radius: 3px; font-size: 12px; }"
        "QMenu::item:selected { background-color: #0078d7; }"
    );

    auto* snipAction = trayMenu.addAction(QString("开始截屏 (%1)").arg(currentHotkeyStr));
    auto* settingsAction = trayMenu.addAction("⚙️ 快捷键设置...");
    trayMenu.addSeparator();
    auto* quitAction = trayMenu.addAction("退出 EvanOCR");

    QObject::connect(snipAction, &QAction::triggered, []() {
        SnippingOverlay::instance().startSnipping();
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
    static int g_hotkeyId = 0;
    auto registerUserHotkey = [&](const QString& keyStr) {
        if (g_hotkeyId != 0) {
            HotkeyManager::instance().unregisterHotkey(g_hotkeyId);
            g_hotkeyId = 0;
        }

        g_hotkeyId = HotkeyManager::instance().registerHotkey(QKeySequence(keyStr), []() {
            qDebug() << "[EvanOCR] 捕获全局热键唤醒截屏";
            SnippingOverlay::instance().startSnipping();
        });

        if (g_hotkeyId == 0) {
            qWarning() << "[EvanOCR] 全局热键" << keyStr << "注册失败 (可能已被其他软件占用)";
            QMessageBox::warning(nullptr, "快捷键冲突",
                QString("全局热键 [%1] 注册失败，可能已被系统或其他软件占用。\n建议在托盘图标右键菜单中更换其他快捷键。").arg(keyStr));
        } else {
            qDebug() << "[EvanOCR] 全局热键注册成功:" << keyStr << "ID:" << g_hotkeyId;
        }
    };

    // 初始注册快捷键
    registerUserHotkey(currentHotkeyStr);

    // 快捷键设置对话框联动
    QObject::connect(settingsAction, &QAction::triggered, [&]() {
        HotkeySettingsDialog dlg;
        QObject::connect(&dlg, &HotkeySettingsDialog::hotkeyChanged, [&](const QKeySequence& newSeq) {
            QString newKeyStr = newSeq.toString();
            if (newKeyStr.isEmpty()) return;

            registerUserHotkey(newKeyStr);
            snipAction->setText(QString("开始截屏 (%1)").arg(newKeyStr));
            trayIcon.setToolTip(QString("EvanOCR 截贴图工具 (按 %1 截屏)").arg(newKeyStr));
            trayIcon.showMessage("快捷键已更新", QString("截屏快捷键已修改为: %1").arg(newKeyStr), QSystemTrayIcon::Information, 2000);
        });
        dlg.exec();
    });

    // 弹出启动气泡提示
    trayIcon.showMessage("EvanOCR 已启动", QString("按 %1 或点击托盘图标即可随时截屏。\n可在托盘右键自定义修改快捷键。").arg(currentHotkeyStr), QSystemTrayIcon::Information, 3000);

    qDebug() << "[EvanOCR] 应用程序初始化成功，当前截屏快捷键:" << currentHotkeyStr;

    // 监听截图完成事件
    QObject::connect(&SnippingOverlay::instance(), &SnippingOverlay::snippingFinished,
                     [](const QPixmap& pixmap, const QRect& region) {
        qDebug() << "[EvanOCR] 截图完成，已写入系统剪贴板! 选区:" << region
                 << "尺寸:" << pixmap.size();
    });

    // 监听贴图请求事件 (将选区及其标注转为独立置顶 Pin 窗口)
    QObject::connect(&SnippingOverlay::instance(), &SnippingOverlay::pinRequested,
                     [](const QPixmap& pixmap, const QRect& screenPos) {
        qDebug() << "[EvanOCR] 收到贴图请求，创建独立 PinWindow 置顶贴图，物理位置:" << screenPos;
        PinManager::instance().createPin(pixmap, screenPos);
    });

    // 监听截图取消事件
    QObject::connect(&SnippingOverlay::instance(), &SnippingOverlay::snippingCancelled, []() {
        qDebug() << "[EvanOCR] 截图已取消";
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
