#pragma once

#include <QObject>
#include <QKeySequence>
#include <functional>

#ifdef Q_OS_WIN
#include <windows.h>
#include <QAbstractNativeEventFilter>
#endif

class HotkeyManager : public QObject
#ifdef Q_OS_WIN
    , public QAbstractNativeEventFilter
#endif
{
    Q_OBJECT
public:
    static HotkeyManager& instance();

    // 注册全局热键，返回热键 ID (失败返回 0)
    int registerHotkey(const QKeySequence& sequence, std::function<void()> callback);
    void unregisterHotkey(int id);
    void unregisterAll();

#ifdef Q_OS_WIN
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private:
    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager() override;

    struct HotkeyItem {
        int id;
        uint32_t mod;
        uint32_t vk;
        std::function<void()> callback;
    };

    int m_nextId = 100;
    QList<HotkeyItem> m_hotkeys;
};
