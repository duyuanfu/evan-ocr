#pragma once

#include <QDialog>
#include <QKeySequenceEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>

class HotkeySettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HotkeySettingsDialog(QWidget* parent = nullptr);
    ~HotkeySettingsDialog() override = default;

    // 获取用户配置的快捷键序列
    QKeySequence hotkey() const { return m_keyEdit->keySequence(); }

signals:
    void hotkeyChanged(const QKeySequence& newSequence);

private:
    void setupUi();
    void loadSettings();
    void saveSettings();

    QKeySequenceEdit* m_keyEdit = nullptr;
    QLabel* m_tipLabel = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};
