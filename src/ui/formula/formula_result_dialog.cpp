#include "formula_result_dialog.h"
#include "../../core/formula/formula_format_helper.h"
#include "../plugin/plugin_market_dialog.h"
#include <QApplication>
#include <QClipboard>
#include <QTimer>
#include <QPainter>
#include <QMessageBox>
#include <QDebug>

FormulaResultDialog::FormulaResultDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint);
    setWindowTitle("数学公式识别与转换 - Evan");
    resize(880, 580);
    setMinimumSize(720, 460);

    setupUi();
}

void FormulaResultDialog::setupUi()
{
    setStyleSheet(
        "QDialog {"
        "  background-color: #f8fafc;"
        "  color: #0f172a;"
        "}"
        "QSplitter::handle {"
        "  background-color: #e2e8f0;"
        "  width: 3px;"
        "}"
        "QSplitter::handle:hover { background-color: #3b82f6; }"
        "QScrollArea {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 8px;"
        "}"
        "QPlainTextEdit {"
        "  background-color: #ffffff;"
        "  color: #0f172a;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 8px;"
        "  padding: 10px 12px;"
        "  font-family: 'Consolas', 'Cascadia Code', 'Segoe UI', 'Microsoft YaHei', monospace;"
        "  font-size: 13px;"
        "  line-height: 1.5;"
        "  selection-background-color: #bfdbfe;"
        "  selection-color: #1e3a8a;"
        "}"
        "QPlainTextEdit:focus { border-color: #3b82f6; }"
        "QPushButton {"
        "  background-color: #ffffff;"
        "  color: #334155;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 6px 14px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: #f1f5f9;"
        "  color: #0f172a;"
        "  border-color: #94a3b8;"
        "}"
        "QComboBox {"
        "  background-color: #ffffff;"
        "  color: #0f172a;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 4px 24px 4px 10px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QComboBox:hover { border-color: #3b82f6; }"
    );

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(10);

    // 顶部控制栏
    auto* topHeaderLayout = new QHBoxLayout();
    topHeaderLayout->setSpacing(10);

    auto* titleLabel = new QLabel("∑ 数学公式识别与转换", this);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #1e293b;");
    topHeaderLayout->addWidget(titleLabel);

    topHeaderLayout->addStretch();

    auto* modeLabel = new QLabel("识别通道:", this);
    modeLabel->setStyleSheet("color: #64748b; font-size: 12px;");
    topHeaderLayout->addWidget(modeLabel);

    m_engineCombo = new QComboBox(this);
    m_engineCombo->setFixedWidth(190);
    refreshEngineComboState();

    connect(m_engineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        QString selectedMode = m_engineCombo->currentData().toString();
        if (selectedMode == "Offline_missing") {
            auto reply = QMessageBox::information(
                this, "提示",
                "RapidLaTeXOCR 离线公式识别引擎尚未安装。\n是否打开插件中心一键安装？",
                QMessageBox::Ok | QMessageBox::Cancel
            );
            if (reply == QMessageBox::Ok) {
                static PluginMarketDialog* dlg = nullptr;
                if (!dlg) dlg = new PluginMarketDialog();
                dlg->show();
                dlg->raise();
                dlg->activateWindow();
            }
            int onlineIdx = m_engineCombo->findData("Online");
            if (onlineIdx >= 0) m_engineCombo->setCurrentIndex(onlineIdx);
            return;
        }

        FormulaOcrManager::instance().setPreferredMode(selectedMode);
        if (!m_sourceImage.isNull()) {
            startRecognition(m_sourceImage, m_dpr);
        }
    });
    topHeaderLayout->addWidget(m_engineCombo);

    mainLayout->addLayout(topHeaderLayout);

    // 核心分割器：上半部（原图与公式预览对照），下半部（LaTeX 源码编辑区）
    auto* vSplitter = new QSplitter(Qt::Vertical, this);
    vSplitter->setChildrenCollapsible(false);

    // 上半部分横向对比 Splitter
    auto* hSplitter = new QSplitter(Qt::Horizontal, vSplitter);
    hSplitter->setChildrenCollapsible(false);

    // 左侧：原图
    auto* leftBox = new QWidget(hSplitter);
    auto* leftLayout = new QVBoxLayout(leftBox);
    leftLayout->setContentsMargins(0, 0, 4, 0);
    leftLayout->setSpacing(6);

    auto* origTitle = new QLabel("选区原图", leftBox);
    origTitle->setStyleSheet("font-size: 12px; color: #475569; font-weight: 600;");
    leftLayout->addWidget(origTitle);

    m_originalScrollArea = new QScrollArea(leftBox);
    m_originalScrollArea->setWidgetResizable(true);
    m_originalScrollArea->setAlignment(Qt::AlignCenter);
    m_originalImgLabel = new QLabel(m_originalScrollArea);
    m_originalImgLabel->setAlignment(Qt::AlignCenter);
    m_originalScrollArea->setWidget(m_originalImgLabel);
    leftLayout->addWidget(m_originalScrollArea, 1);

    hSplitter->addWidget(leftBox);

    // 右侧：实时渲染公式预览
    auto* rightBox = new QWidget(hSplitter);
    auto* rightLayout = new QVBoxLayout(rightBox);
    rightLayout->setContentsMargins(4, 0, 0, 0);
    rightLayout->setSpacing(6);

    auto* previewTitle = new QLabel("公式矢量渲染预览", rightBox);
    previewTitle->setStyleSheet("font-size: 12px; color: #475569; font-weight: 600;");
    rightLayout->addWidget(previewTitle);

    m_renderedScrollArea = new QScrollArea(rightBox);
    m_renderedScrollArea->setWidgetResizable(true);
    m_renderedScrollArea->setAlignment(Qt::AlignCenter);
    m_renderedFormulaLabel = new QLabel("等待公式识别结果...", m_renderedScrollArea);
    m_renderedFormulaLabel->setAlignment(Qt::AlignCenter);
    m_renderedFormulaLabel->setStyleSheet("color: #94a3b8; font-size: 13px;");
    m_renderedScrollArea->setWidget(m_renderedFormulaLabel);
    rightLayout->addWidget(m_renderedScrollArea, 1);

    hSplitter->addWidget(rightBox);
    hSplitter->setSizes({430, 430});

    vSplitter->addWidget(hSplitter);

    // 下半部：LaTeX 源码与编辑
    auto* codeBox = new QWidget(vSplitter);
    auto* codeLayout = new QVBoxLayout(codeBox);
    codeLayout->setContentsMargins(0, 4, 0, 0);
    codeLayout->setSpacing(6);

    auto* codeHeaderLayout = new QHBoxLayout();
    auto* codeTitle = new QLabel("LaTeX 源码 (可直接微调编辑):", codeBox);
    codeTitle->setStyleSheet("font-size: 12px; color: #475569; font-weight: 600;");
    codeHeaderLayout->addWidget(codeTitle);
    codeHeaderLayout->addStretch();
    codeLayout->addLayout(codeHeaderLayout);

    m_latexEdit = new QPlainTextEdit(codeBox);
    m_latexEdit->setPlaceholderText("公式识别完成后，LaTeX 代码将在此呈现并支持编辑修改...");
    codeLayout->addWidget(m_latexEdit, 1);

    vSplitter->addWidget(codeBox);
    vSplitter->setSizes({220, 260});

    mainLayout->addWidget(vSplitter, 1);

    // 底部工具与动作栏
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);

    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet("color: #64748b; font-size: 12px;");
    bottomLayout->addWidget(m_statusLabel);

    bottomLayout->addStretch();

    // 1. 复制 LaTeX
    m_copyLatexBtn = new QPushButton("📋 复制 LaTeX", this);
    m_copyLatexBtn->setCursor(Qt::PointingHandCursor);
    m_copyLatexBtn->setToolTip("复制纯净 LaTeX 公式代码");
    connect(m_copyLatexBtn, &QPushButton::clicked, this, [this]() {
        QString text = m_latexEdit->toPlainText().trimmed();
        if (text.isEmpty()) return;
        QApplication::clipboard()->setText(text);
        m_copyLatexBtn->setText("✓ 已复制 LaTeX");
        QTimer::singleShot(1500, this, [this]() {
            if (m_copyLatexBtn) m_copyLatexBtn->setText("📋 复制 LaTeX");
        });
    });
    bottomLayout->addWidget(m_copyLatexBtn);

    // 2. 复制 Markdown
    m_copyMarkdownBtn = new QPushButton("📋 复制 $$ Markdown", this);
    m_copyMarkdownBtn->setCursor(Qt::PointingHandCursor);
    m_copyMarkdownBtn->setToolTip("复制为 $$ ... $$ 块级 Markdown 公式语法");
    connect(m_copyMarkdownBtn, &QPushButton::clicked, this, [this]() {
        QString text = m_latexEdit->toPlainText().trimmed();
        if (text.isEmpty()) return;
        QString md = FormulaFormatHelper::toBlockMarkdown(text);
        QApplication::clipboard()->setText(md);
        m_copyMarkdownBtn->setText("✓ 已复制 Markdown");
        QTimer::singleShot(1500, this, [this]() {
            if (m_copyMarkdownBtn) m_copyMarkdownBtn->setText("📋 复制 $$ Markdown");
        });
    });
    bottomLayout->addWidget(m_copyMarkdownBtn);

    // 3. 复制 MathML (Word 直粘)
    m_copyWordBtn = new QPushButton("📄 复制 MathML (Word直粘)", this);
    m_copyWordBtn->setCursor(Qt::PointingHandCursor);
    m_copyWordBtn->setToolTip("复制为标准 MathML XML，切换至 Microsoft Word 后直接 Ctrl+V 即刻转为原生 Word 数学公式");
    m_copyWordBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: #ffffff;"
        "  border: 1px solid #1d4ed8;"
        "  border-radius: 6px;"
        "  padding: 6px 16px;"
        "  font-size: 12px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );
    connect(m_copyWordBtn, &QPushButton::clicked, this, [this]() {
        QString text = m_latexEdit->toPlainText().trimmed();
        if (text.isEmpty()) return;
        QString mathml = FormulaFormatHelper::toWordMathML(text);
        QApplication::clipboard()->setText(mathml);
        m_copyWordBtn->setText("✓ 已复制 MathML");
        QTimer::singleShot(1500, this, [this]() {
            if (m_copyWordBtn) m_copyWordBtn->setText("📄 复制 MathML (Word直粘)");
        });
    });
    bottomLayout->addWidget(m_copyWordBtn);

    m_closeBtn = new QPushButton("关闭", this);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(bottomLayout);
}

void FormulaResultDialog::refreshEngineComboState()
{
    if (!m_engineCombo) return;
    QSignalBlocker blocker(m_engineCombo);
    m_engineCombo->clear();

    m_engineCombo->addItem("在线云端通道 (高速·免模型)", "Online");

    bool offlineReady = FormulaOcrManager::instance().isOfflinePluginReady();
    if (offlineReady) {
        m_engineCombo->addItem("本地离线插件 (RapidLaTeX)", "Offline");
    } else {
        m_engineCombo->addItem("本地离线插件 (未安装)", "Offline_missing");
    }

    QString pref = FormulaOcrManager::instance().preferredMode();
    int idx = m_engineCombo->findData(pref);
    if (idx >= 0) {
        m_engineCombo->setCurrentIndex(idx);
    } else {
        m_engineCombo->setCurrentIndex(0);
    }
}

void FormulaResultDialog::startRecognition(const QPixmap& pixmap, qreal dpr)
{
    m_sourceImage = pixmap;
    m_dpr = dpr;

    // 显示原图
    if (m_originalImgLabel) {
        m_originalImgLabel->setPixmap(pixmap);
    }

    // 重置状态
    m_latexEdit->setPlainText("正在识别数学公式中，请稍候...");
    m_renderedFormulaLabel->setText("公式正在生成中...");
    m_statusLabel->setText("正在连接通道并提取公式结构...");

    FormulaOcrManager::instance().recognizeAsync(pixmap.toImage(), dpr, [this](const FormulaOcrResult& res) {
        m_lastResult = res;

        if (!res.success) {
            m_latexEdit->setPlainText(QString("公式识别失败: %1").arg(res.errorMessage));
            m_statusLabel->setText(QString("识别异常 | 通道: %1").arg(res.engineName));
            m_renderedFormulaLabel->setText("无法生成公式渲染预览");
            return;
        }

        m_latexEdit->setPlainText(res.latex);
        m_statusLabel->setText(QString("耗时: %1ms | 通道: %2").arg(res.elapsedMs).arg(res.engineName));

        // 发起公式矢量图片微渲染预览
        FormulaOcrManager::instance().renderFormulaAsync(res.latex, [this](bool success, const QByteArray& imgData, const QString& mimeType) {
            Q_UNUSED(mimeType);
            if (!success || imgData.isEmpty()) {
                m_renderedFormulaLabel->setText(m_latexEdit->toPlainText());
                return;
            }

            QPixmap previewPix;
            if (previewPix.loadFromData(imgData)) {
                m_renderedFormulaLabel->setPixmap(previewPix);
            } else {
                m_renderedFormulaLabel->setText(m_latexEdit->toPlainText());
            }
        });
    });
}
