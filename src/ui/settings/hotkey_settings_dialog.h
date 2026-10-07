#pragma once

#include <QDialog>
#include <QKeySequenceEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include "../../core/hotkey_config.h"

class HotkeySettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HotkeySettingsDialog(QWidget* parent = nullptr);
    ~HotkeySettingsDialog() override = default;

signals:
    void hotkeysSaved();

private:
    void setupUi();
    void loadFromConfig();
    bool saveToConfig();
    void resetAllDefaults();

    QWidget* createGeneralTab();
    QWidget* createHotkeysTab();

    QWidget* createKeyRow(const QString& title, const QString& desc, QKeySequenceEdit*& editOut);

    // 全局热键
    QKeySequenceEdit* m_globalSnippingEdit = nullptr;
    QKeySequenceEdit* m_globalPinEdit = nullptr;

    // 截屏操作快捷键
    QKeySequenceEdit* m_snippingUndoEdit = nullptr;
    QKeySequenceEdit* m_snippingPinEdit = nullptr;
    QKeySequenceEdit* m_snippingConfirmEdit = nullptr;
    QKeySequenceEdit* m_snippingSaveEdit = nullptr;
    QKeySequenceEdit* m_snippingOcrEdit = nullptr;
    QKeySequenceEdit* m_snippingCancelEdit = nullptr;

    // 标注工具切换快捷键
    QKeySequenceEdit* m_toolRectEdit = nullptr;
    QKeySequenceEdit* m_toolArrowEdit = nullptr;
    QKeySequenceEdit* m_toolPencilEdit = nullptr;
    QKeySequenceEdit* m_toolTextEdit = nullptr;
    QKeySequenceEdit* m_toolMosaicEdit = nullptr;
    QKeySequenceEdit* m_toolCharEdit = nullptr;

    // 贴图窗口快捷键
    QKeySequenceEdit* m_pinCloseEdit = nullptr;
    QKeySequenceEdit* m_pinOcrEdit = nullptr;
    QKeySequenceEdit* m_pinCopyEdit = nullptr;
    QKeySequenceEdit* m_pinSaveEdit = nullptr;

    // 通用与外观设置
    QComboBox* m_themeCombo = nullptr;
    QCheckBox* m_enableSmartSnappingCheck = nullptr;

    QTabWidget* m_tabWidget = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};
