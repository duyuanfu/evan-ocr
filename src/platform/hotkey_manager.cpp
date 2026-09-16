#include "hotkey_manager.h"
#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>

static uint32_t convertQtModifiersToWin(Qt::KeyboardModifiers modifiers)
{
    uint32_t winMod = 0;
    if (modifiers & Qt::AltModifier)     winMod |= MOD_ALT;
    if (modifiers & Qt::ControlModifier) winMod |= MOD_CONTROL;
    if (modifiers & Qt::ShiftModifier)   winMod |= MOD_SHIFT;
    if (modifiers & Qt::MetaModifier)    winMod |= MOD_WIN;
    return winMod;
}

static uint32_t convertQtKeyToWin(Qt::Key key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return static_cast<uint32_t>('A' + (key - Qt::Key_A));
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return static_cast<uint32_t>('0' + (key - Qt::Key_0));
    }
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
        return static_cast<uint32_t>(VK_F1 + (key - Qt::Key_F1));
    }

    switch (key) {
    case Qt::Key_Escape:    return VK_ESCAPE;
    case Qt::Key_Tab:       return VK_TAB;
    case Qt::Key_Backspace: return VK_BACK;
    case Qt::Key_Return:
    case Qt::Key_Enter:     return VK_RETURN;
    case Qt::Key_Insert:    return VK_INSERT;
    case Qt::Key_Delete:    return VK_DELETE;
    case Qt::Key_Pause:     return VK_PAUSE;
    case Qt::Key_Print:     return VK_SNAPSHOT;
    case Qt::Key_Clear:     return VK_CLEAR;
    case Qt::Key_Home:      return VK_HOME;
    case Qt::Key_End:       return VK_END;
    case Qt::Key_Left:      return VK_LEFT;
    case Qt::Key_Up:        return VK_UP;
    case Qt::Key_Right:     return VK_RIGHT;
    case Qt::Key_Down:      return VK_DOWN;
    case Qt::Key_PageUp:    return VK_PRIOR;
    case Qt::Key_PageDown:  return VK_NEXT;
    case Qt::Key_Space:     return VK_SPACE;
    default:                return 0;
    }
}
#endif

HotkeyManager::HotkeyManager(QObject* parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    QCoreApplication::instance()->installNativeEventFilter(this);
#endif
}

HotkeyManager::~HotkeyManager()
{
    unregisterAll();
#ifdef Q_OS_WIN
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->removeNativeEventFilter(this);
    }
#endif
}

HotkeyManager& HotkeyManager::instance()
{
    static HotkeyManager mgr;
    return mgr;
}

int HotkeyManager::registerHotkey(const QKeySequence& sequence, std::function<void()> callback)
{
    if (sequence.isEmpty() || !callback) {
        return 0;
    }

#ifdef Q_OS_WIN
    auto combination = sequence[0];
    auto key = combination.key();
    auto modifiers = combination.keyboardModifiers();

    uint32_t winMod = convertQtModifiersToWin(modifiers);
    uint32_t winVk = convertQtKeyToWin(key);

    if (winVk == 0) {
        qWarning() << "[HotkeyManager] 无法识别的按键:" << sequence.toString();
        return 0;
    }

    int id = m_nextId++;
    if (!RegisterHotKey(nullptr, id, winMod, winVk)) {
        qWarning() << "[HotkeyManager] RegisterHotKey 失败 (可能被其他程序占用):" << sequence.toString();
        return 0;
    }

    m_hotkeys.append({id, winMod, winVk, std::move(callback)});
    qDebug() << "[HotkeyManager] 全局热键注册成功:" << sequence.toString() << "ID:" << id;
    return id;
#else
    return 0;
#endif
}

void HotkeyManager::unregisterHotkey(int id)
{
#ifdef Q_OS_WIN
    for (int i = 0; i < m_hotkeys.size(); ++i) {
        if (m_hotkeys[i].id == id) {
            UnregisterHotKey(nullptr, id);
            m_hotkeys.removeAt(i);
            break;
        }
    }
#endif
}

void HotkeyManager::unregisterAll()
{
#ifdef Q_OS_WIN
    for (const auto& item : m_hotkeys) {
        UnregisterHotKey(nullptr, item.id);
    }
    m_hotkeys.clear();
#endif
}

#ifdef Q_OS_WIN
bool HotkeyManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_HOTKEY) {
            int id = static_cast<int>(msg->wParam);
            for (const auto& item : m_hotkeys) {
                if (item.id == id && item.callback) {
                    item.callback();
                    return true;
                }
            }
        }
    }
    return false;
}
#endif
