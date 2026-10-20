#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../../core/update/update_manager.h"

class UpdateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UpdateDialog(const UpdateInfo& info, QWidget* parent = nullptr);
    ~UpdateDialog() override;

private slots:
    void onDownloadClicked();
    void onCancelClicked();
    void onManualDownloadClicked();

    void onDownloadProgress(qint64 received, qint64 total, double speedMBs);
    void onDownloadFinished(const QString& tempZipPath);
    void onDownloadFailed(const QString& errorMsg);

private:
    void setupUi();
    void applyTheme();

    UpdateInfo m_info;
    bool m_isDownloading = false;

    QLabel* m_titleLabel = nullptr;
    QLabel* m_subTitleLabel = nullptr;
    QTextBrowser* m_notesBrowser = nullptr;

    QWidget* m_progressContainer = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_progressStatusLabel = nullptr;

    QPushButton* m_updateBtn = nullptr;
    QPushButton* m_manualBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};
