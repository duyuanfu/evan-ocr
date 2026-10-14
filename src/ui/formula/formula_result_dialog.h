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
#include "../../core/formula/formula_ocr_manager.h"

class FormulaResultDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FormulaResultDialog(QWidget* parent = nullptr);
    ~FormulaResultDialog() override = default;

    // 设置选区原图并异步发起公式识别
    void startRecognition(const QPixmap& pixmap, qreal dpr = 1.0);

private:
    void setupUi();
    void updateCopyButtonsState();
    void refreshEngineComboState();

    QPixmap m_sourceImage;
    qreal m_dpr = 1.0;
    FormulaOcrResult m_lastResult;

    // UI 组件
    QLabel* m_originalImgLabel = nullptr;
    QScrollArea* m_originalScrollArea = nullptr;

    QLabel* m_renderedFormulaLabel = nullptr;
    QScrollArea* m_renderedScrollArea = nullptr;

    QComboBox* m_engineCombo = nullptr;
    QPlainTextEdit* m_latexEdit = nullptr;
    QLabel* m_statusLabel = nullptr;

    QPushButton* m_copyLatexBtn = nullptr;
    QPushButton* m_copyMarkdownBtn = nullptr;
    QPushButton* m_copyWordBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
};
