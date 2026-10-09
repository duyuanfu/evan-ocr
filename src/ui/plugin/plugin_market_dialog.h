#pragma once

#include <QDialog>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressBar>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../../core/plugin/plugin_types.h"
#include "../../core/plugin/plugin_manager.h"

// 单个插件展示与交互卡片
class PluginCardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PluginCardWidget(const PluginInfo& info, QWidget* parent = nullptr);

    void updateInfo(const PluginInfo& info);
    void updateProgress(int percent, qint64 downloaded, qint64 total, double speedMBs);

signals:
    void installRequested(const QString& id);
    void cancelRequested(const QString& id);
    void uninstallRequested(const QString& id);

private:
    void setupUi();
    void refreshState();

    PluginInfo m_info;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_categoryBadge = nullptr;
    QLabel* m_sizeLabel = nullptr;
    QLabel* m_descLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_actionBtn = nullptr;
    QPushButton* m_uninstallBtn = nullptr;
};

// VSCode 风格独立插件中心窗口
class PluginMarketDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PluginMarketDialog(QWidget* parent = nullptr);
    ~PluginMarketDialog() override = default;

private:
    void setupUi();
    void renderPluginList();
    void filterPlugins();

    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_filterAllBtn = nullptr;
    QPushButton* m_filterTransBtn = nullptr;
    QPushButton* m_filterOcrBtn = nullptr;
    QPushButton* m_filterInstalledBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;

    QWidget* m_cardsContainer = nullptr;
    QVBoxLayout* m_cardsLayout = nullptr;
    QScrollArea* m_scrollArea = nullptr;

    QString m_currentCategory = "All"; // "All", "翻译扩展", "OCR模型", "Installed"
    QMap<QString, PluginCardWidget*> m_cardMap;
};
