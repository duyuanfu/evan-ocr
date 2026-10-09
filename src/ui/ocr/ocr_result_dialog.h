#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QScrollArea>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPixmap>
#include <QPaintEvent>
#include "../../core/ocr/ocr_types.h"

// 具备文字包围框绘制的高清原图预览挂件
class OcrImagePreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OcrImagePreviewWidget(QWidget* parent = nullptr);

    void setImage(const QPixmap& pixmap);
    void setResult(const OcrResult& result);
    void setShowBoundingBoxes(bool show);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_pixmap;
    OcrResult m_result;
    bool m_showBoundingBoxes = true;
};

// 左右对比形式的 OCR 结果对话框
class OcrResultDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OcrResultDialog(QWidget* parent = nullptr);
    ~OcrResultDialog() override = default;

    // 设置截图原图与 OCR 识别结果
    void setImage(const QPixmap& pixmap);
    void setResult(const OcrResult& result, const QString& engineName = "");

private:
    void setupUi();
    void updateFormattedText();
    QString processText(bool mergeParagraphs, bool removeExtraSpaces);
    void setupTranslationPanel(QVBoxLayout* rightLayout);
    void triggerTranslation();
    void autoDetectSourceLanguage();
    void updateTranslationLanguages();

    QPixmap m_image;
    OcrResult m_result;
    bool m_mergeParagraphs = false;
    bool m_removeExtraSpaces = false;
    bool m_showBoundingBoxes = true;

    // 左右分栏 Splitter
    QSplitter* m_splitter = nullptr;

    // 左侧：原图预览与文字框展示
    OcrImagePreviewWidget* m_imagePreview = nullptr;
    QScrollArea* m_imageScrollArea = nullptr;
    QPushButton* m_toggleBoxesBtn = nullptr;

    // 右侧：提取文字框与操作
    QPlainTextEdit* m_textEdit = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_engineLabel = nullptr;
    QPushButton* m_switchEngineBtn = nullptr;
    QPushButton* m_copyBtn = nullptr;
    QPushButton* m_mergeBtn = nullptr;
    QPushButton* m_removeSpacesBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;

    QString m_currentEngineType = "RapidOCR";

    // 翻译相关挂件与状态
    QWidget* m_translationSection = nullptr;
    QPlainTextEdit* m_translationEdit = nullptr;
    QComboBox* m_srcLangCombo = nullptr;
    QComboBox* m_targetLangCombo = nullptr;
    QPushButton* m_translateBtn = nullptr;
    QPushButton* m_copyTransBtn = nullptr;
    QLabel* m_transEngineLabel = nullptr;
    QPushButton* m_pluginHelpBtn = nullptr;
    bool m_isTranslating = false;
};
