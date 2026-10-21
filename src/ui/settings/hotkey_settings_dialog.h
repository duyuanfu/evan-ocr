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
#include <QLineEdit>
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
    QKeySequenceEdit* m_globalFullScreenEdit = nullptr;

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
    QCheckBox* m_autoStartCheck = nullptr;
    QComboBox* m_textCommitModeCombo = nullptr;
    QLineEdit* m_simpleTexTokenEdit = nullptr;

    // 网络代理设置
    QComboBox* m_proxyTypeCombo = nullptr;
    QComboBox* m_proxyProtocolCombo = nullptr;
    QLineEdit* m_proxyHostEdit = nullptr;
    QLineEdit* m_proxyPortEdit = nullptr;
    QPushButton* m_testProxyBtn = nullptr;
    QLabel* m_testProxyResultLbl = nullptr;
    QWidget* m_customProxyContainer = nullptr;

    // 软件更新设置
    QCheckBox* m_autoCheckUpdateCheck = nullptr;
    QPushButton* m_checkUpdateBtn = nullptr;
    QLabel* m_currentVersionLbl = nullptr;

    // 自定义保存路径设置
    QLineEdit* m_saveDirEdit = nullptr;
    QCheckBox* m_rememberSavePathCheck = nullptr;

    QTabWidget* m_tabWidget = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};
