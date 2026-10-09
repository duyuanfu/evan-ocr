#include "ocr_result_dialog.h"
#include "../../core/ocr/ocr_manager.h"
#include "../../core/translation/translation_plugin_manager.h"
#include "../plugin/plugin_market_dialog.h"
#include <QApplication>
#include <QClipboard>
#include <QTimer>
#include <QPainter>
#include <QRegularExpression>
#include <QMessageBox>
#include <QDebug>

// -------------------------------------------------------------
// OcrImagePreviewWidget: 左侧原图对比与识别框高亮挂件
// -------------------------------------------------------------
OcrImagePreviewWidget::OcrImagePreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setStyleSheet("background-color: #f8fafc;");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void OcrImagePreviewWidget::setImage(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
    if (!m_pixmap.isNull()) {
        qreal dpr = m_pixmap.devicePixelRatio();
        if (dpr <= 0.0) dpr = 1.0;
        int w = static_cast<int>(m_pixmap.width() / dpr);
        int h = static_cast<int>(m_pixmap.height() / dpr);
        setMinimumSize(w, h);
    }
    update();
}

QSize OcrImagePreviewWidget::sizeHint() const
{
    if (!m_pixmap.isNull()) {
        qreal dpr = m_pixmap.devicePixelRatio();
        if (dpr <= 0.0) dpr = 1.0;
        return QSize(static_cast<int>(m_pixmap.width() / dpr), static_cast<int>(m_pixmap.height() / dpr));
    }
    return QSize(400, 300);
}

void OcrImagePreviewWidget::setResult(const OcrResult& result)
{
    m_result = result;
    update();
}

void OcrImagePreviewWidget::setShowBoundingBoxes(bool show)
{
    if (m_showBoundingBoxes != show) {
        m_showBoundingBoxes = show;
        update();
    }
}

void OcrImagePreviewWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_pixmap.isNull()) {
        painter.fillRect(rect(), QColor("#f8fafc"));
        painter.setPen(QColor("#94a3b8"));
        painter.drawText(rect(), Qt::AlignCenter, "暂无图像");
        return;
    }

    qreal dpr = m_pixmap.devicePixelRatio();
    if (dpr <= 0.0) dpr = 1.0;
    int imgW = static_cast<int>(m_pixmap.width() / dpr);
    int imgH = static_cast<int>(m_pixmap.height() / dpr);

    // 居中偏移量：使拖动拉大左侧预览区域时，图像自然平滑居中展现
    int offsetX = qMax(0, (width() - imgW) / 2);
    int offsetY = qMax(0, (height() - imgH) / 2);

    QRect targetRect(offsetX, offsetY, imgW, imgH);
    painter.drawPixmap(targetRect, m_pixmap);

    if (m_showBoundingBoxes && m_result.success) {
        QColor boxColor(37, 99, 235, 200);      // #2563eb
        QColor fillColor(37, 99, 235, 25);
        QPen boxPen(boxColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

        for (const auto& line : m_result.lines) {
            QRect box = line.logicalBox.isValid() ? line.logicalBox : line.boundingBox;
            if (box.isValid()) {
                QRect adjustedBox = box.translated(offsetX, offsetY);
                painter.setPen(boxPen);
                painter.setBrush(fillColor);
                painter.drawRoundedRect(adjustedBox, 3, 3);
            }
        }
    }
}

// -------------------------------------------------------------
// OcrResultDialog: 纯净明亮极简现代风格界面 (全下拉框设计 + 极简UI)
// -------------------------------------------------------------
OcrResultDialog::OcrResultDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint);
    setWindowTitle("文字识别与翻译");
    resize(1040, 660);
    setMinimumSize(840, 520);

    setupUi();
}

void OcrResultDialog::setupUi()
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
        "QSplitter::handle:hover {"
        "  background-color: #3b82f6;"
        "}"
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
        "  font-family: 'Segoe UI Variable Text', 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "  font-size: 13px;"
        "  line-height: 1.5;"
        "  selection-background-color: #bfdbfe;"
        "  selection-color: #1e3a8a;"
        "}"
        "QPlainTextEdit:focus {"
        "  border-color: #3b82f6;"
        "}"
        "QPushButton {"
        "  background-color: #ffffff;"
        "  color: #334155;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 5px 12px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background-color: #f1f5f9;"
        "  color: #0f172a;"
        "  border-color: #94a3b8;"
        "}"
        "QPushButton:checked {"
        "  background-color: #eff6ff;"
        "  color: #2563eb;"
        "  border-color: #3b82f6;"
        "  font-weight: 600;"
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
        "QComboBox:hover {"
        "  border-color: #3b82f6;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 20px;"
        "  border: none;"
        "  background: transparent;"
        "}"
        "QComboBox::down-arrow {"
        "  width: 8px;"
        "  height: 6px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #ffffff;"
        "  color: #0f172a;"
        "  selection-background-color: #eff6ff;"
        "  selection-color: #2563eb;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 4px;"
        "  outline: none;"
        "}"
        "QScrollBar:vertical {"
        "  background: #f1f5f9;"
        "  width: 7px;"
        "  margin: 0px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #cbd5e1;"
        "  min-height: 20px;"
        "  border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #94a3b8;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    );

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(10);

    // 核心分割器 (QSplitter)
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);

    // ==========================================
    // 左侧面板：原图对照
    // ==========================================
    auto* leftContainer = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 6, 0);
    leftLayout->setSpacing(8);

    auto* leftHeaderLayout = new QHBoxLayout();
    auto* leftTitle = new QLabel("原图", leftContainer);
    leftTitle->setStyleSheet("font-size: 13px; color: #1e293b; font-weight: bold;");
    leftHeaderLayout->addWidget(leftTitle);
    leftHeaderLayout->addStretch();

    m_toggleBoxesBtn = new QPushButton("识别框", leftContainer);
    m_toggleBoxesBtn->setCheckable(true);
    m_toggleBoxesBtn->setChecked(true);
    m_toggleBoxesBtn->setCursor(Qt::PointingHandCursor);
    m_toggleBoxesBtn->setToolTip("显示或隐藏原图上的文字定位框");
    connect(m_toggleBoxesBtn, &QPushButton::toggled, this, [this](bool checked) {
        if (m_imagePreview) {
            m_imagePreview->setShowBoundingBoxes(checked);
        }
    });
    leftHeaderLayout->addWidget(m_toggleBoxesBtn);
    leftLayout->addLayout(leftHeaderLayout);

    m_imageScrollArea = new QScrollArea(leftContainer);
    m_imageScrollArea->setWidgetResizable(true);
    m_imageScrollArea->setAlignment(Qt::AlignCenter);
    m_imagePreview = new OcrImagePreviewWidget(m_imageScrollArea);
    m_imageScrollArea->setWidget(m_imagePreview);
    leftLayout->addWidget(m_imageScrollArea, 1);

    m_splitter->addWidget(leftContainer);

    // ==========================================
    // 右侧面板：识别文字与翻译
    // ==========================================
    auto* rightContainer = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(6, 0, 0, 0);
    rightLayout->setSpacing(8);

    // 顶部 OCR 引擎控制与工具栏 (精简无冗余按钮)
    auto* rightHeaderLayout = new QHBoxLayout();
    rightHeaderLayout->setSpacing(8);

    auto* rightTitle = new QLabel("提取文字", rightContainer);
    rightTitle->setStyleSheet("font-size: 13px; color: #1e293b; font-weight: bold;");
    rightHeaderLayout->addWidget(rightTitle);

    // OCR 引擎下拉框 (宽度拓宽至 180px，彻底避免右侧文字及箭头被遮挡)
    m_ocrEngineCombo = new QComboBox(rightContainer);
    m_ocrEngineCombo->setFixedWidth(180);
    refreshEngineComboState();

    connect(&PluginManager::instance(), &PluginManager::pluginsRefreshed, this, &OcrResultDialog::refreshEngineComboState);
    connect(&PluginManager::instance(), &PluginManager::pluginStatusChanged, this, [this](const QString&, PluginStatus, const QString&) {
        refreshEngineComboState();
    });

    connect(m_ocrEngineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        QString selectedData = m_ocrEngineCombo->currentData().toString();
        if (selectedData == "RapidOCR_missing") {
            auto reply = QMessageBox::information(
                this,
                "提示",
                "RapidOCR 高精度离线文字识别引擎尚未安装。\n是否打开插件中心一键安装？",
                QMessageBox::Ok | QMessageBox::Cancel
            );
            if (reply == QMessageBox::Ok) {
                static PluginMarketDialog* dlg = nullptr;
                if (!dlg) dlg = new PluginMarketDialog();
                dlg->show();
                dlg->raise();
                dlg->activateWindow();
            }
            int winIdx = m_ocrEngineCombo->findData("WindowsMedia");
            if (winIdx >= 0) {
                m_ocrEngineCombo->setCurrentIndex(winIdx);
            }
            return;
        }

        if (m_image.isNull()) return;
        m_currentEngineType = selectedData;
        OcrManager::instance().setPreferredEngineType(selectedData);

        // 如果当前截图此前已识别过该引擎，直接瞬间读取缓存（0毫秒秒切，消除重复识别！）
        if (m_engineResultCache.contains(selectedData)) {
            setResult(m_engineResultCache[selectedData], selectedData);
            return;
        }

        m_textEdit->setPlainText("正在识别文字中...");
        OcrManager::instance().recognizeWithEngine(selectedData, m_image.toImage(), m_image.devicePixelRatio(),
            [this, selectedData](const OcrResult& res, const QString& engineName) {
                setResult(res, engineName.isEmpty() ? selectedData : engineName);
            });
    });

    rightHeaderLayout->addWidget(m_ocrEngineCombo);
    rightHeaderLayout->addStretch();

    // 格式化按钮：段落合并与清除空格 (无冗余图标)
    m_mergeBtn = new QPushButton("合并段落", rightContainer);
    m_mergeBtn->setCheckable(true);
    m_mergeBtn->setCursor(Qt::PointingHandCursor);
    m_mergeBtn->setToolTip("将断行拼接为完整自然段落");
    connect(m_mergeBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_mergeParagraphs = checked;
        updateFormattedText();
    });
    rightHeaderLayout->addWidget(m_mergeBtn);

    m_removeSpacesBtn = new QPushButton("清除空格", rightContainer);
    m_removeSpacesBtn->setCheckable(true);
    m_removeSpacesBtn->setCursor(Qt::PointingHandCursor);
    m_removeSpacesBtn->setToolTip("消除汉字之间的多余空格");
    connect(m_removeSpacesBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_removeExtraSpaces = checked;
        updateFormattedText();
    });
    rightHeaderLayout->addWidget(m_removeSpacesBtn);
    rightLayout->addLayout(rightHeaderLayout);

    // 提取文字输入框
    m_textEdit = new QPlainTextEdit(rightContainer);
    m_textEdit->setPlaceholderText("正在识别文字中，请稍候...");
    rightLayout->addWidget(m_textEdit, 1);

    // 下部：嵌入式「译文对照」面板 (全下拉框设计 + 极简UI)
    setupTranslationPanel(rightLayout);

    m_splitter->addWidget(rightContainer);

    // 默认比例 48 : 52，且两侧均允许自由平滑拖拽伸缩
    m_splitter->setSizes({480, 520});
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(m_splitter, 1);

    // ==========================================
    // 底部工具与状态栏
    // ==========================================
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);

    m_statusLabel = new QLabel("字数: 0  |  行数: 0", this);
    m_statusLabel->setStyleSheet("color: #64748b; font-size: 12px;");
    bottomLayout->addWidget(m_statusLabel);

    m_transElapsedLabel = new QLabel(this);
    m_transElapsedLabel->setStyleSheet("color: #94a3b8; font-size: 11px; margin-left: 12px;");
    bottomLayout->addWidget(m_transElapsedLabel);

    bottomLayout->addStretch();

    m_copyBtn = new QPushButton("复制原文", this);
    m_copyBtn->setCursor(Qt::PointingHandCursor);
    m_copyBtn->setStyleSheet(
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
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        QString text = m_textEdit->toPlainText();
        if (text.trimmed().isEmpty()) return;
        QApplication::clipboard()->setText(text);
        m_copyBtn->setText("已复制");
        QTimer::singleShot(1500, this, [this]() {
            if (m_copyBtn) m_copyBtn->setText("复制原文");
        });
    });
    bottomLayout->addWidget(m_copyBtn);

    m_closeBtn = new QPushButton("关闭", this);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(bottomLayout);
}

void OcrResultDialog::refreshEngineComboState()
{
    if (!m_ocrEngineCombo) return;
    QSignalBlocker blocker(m_ocrEngineCombo);
    m_ocrEngineCombo->clear();

    bool rapidOk = OcrManager::instance().isRapidOcrReady();
    if (rapidOk) {
        m_ocrEngineCombo->addItem("RapidOCR (离线)", "RapidOCR");
    } else {
        m_ocrEngineCombo->addItem("RapidOCR (未安装)", "RapidOCR_missing");
    }
    m_ocrEngineCombo->addItem("Windows 原生", "WindowsMedia");

    int curIdx = m_ocrEngineCombo->findData(m_currentEngineType);
    if (curIdx >= 0) {
        m_ocrEngineCombo->setCurrentIndex(curIdx);
    } else {
        m_ocrEngineCombo->setCurrentIndex(rapidOk ? 0 : 1);
    }
}

void OcrResultDialog::setupTranslationPanel(QVBoxLayout* rightLayout)
{
    m_translationSection = new QWidget(this);
    m_translationSection->setObjectName("transSection");
    m_translationSection->setStyleSheet(
        "QWidget#transSection {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 8px;"
        "}"
    );

    auto* transLayout = new QVBoxLayout(m_translationSection);
    transLayout->setContentsMargins(10, 8, 10, 8);
    transLayout->setSpacing(6);

    // 翻译顶部控制栏
    auto* barLayout = new QHBoxLayout();
    barLayout->setSpacing(8);

    auto* transTitle = new QLabel("译文", m_translationSection);
    transTitle->setStyleSheet("font-size: 13px; color: #1e293b; font-weight: bold;");
    barLayout->addWidget(transTitle);

    // 翻译模式下拉框 (在线直连 vs 离线插件)
    m_transModeCombo = new QComboBox(m_translationSection);
    m_transModeCombo->setFixedWidth(180);
    m_transModeCombo->addItem("在线直连 (国内高速)", "Online");
    m_transModeCombo->addItem("离线插件 (纯本地)", "Offline");

    QString pref = TranslationPluginManager::instance().preferredEnginePreference();
    bool isOfflineInstalled = TranslationPluginManager::instance().isOfflinePluginReady();

    if (pref == "Offline" && isOfflineInstalled) {
        m_transModeCombo->setCurrentIndex(1);
    } else {
        m_transModeCombo->setCurrentIndex(0);
    }

    // 状态胶囊标签
    m_transStatusBadge = new QLabel(m_translationSection);
    auto updateTransBadge = [this]() {
        bool offlineMode = (m_transModeCombo->currentData().toString() == "Offline");
        bool isReady = TranslationPluginManager::instance().isOfflinePluginReady();

        if (offlineMode) {
            if (isReady) {
                m_transStatusBadge->setText("本地就绪");
                m_transStatusBadge->setStyleSheet("font-size: 11px; color: #059669; background: #ecfdf5; padding: 1px 6px; border-radius: 4px; border: 1px solid #a7f3d0; font-weight: 500;");
                if (m_translationEdit && m_translationEdit->toPlainText().isEmpty()) {
                    m_translationEdit->setPlaceholderText("点击上方【翻译】获取译文...");
                }
            } else {
                m_transStatusBadge->setText("未安装插件");
                m_transStatusBadge->setStyleSheet("font-size: 11px; color: #d97706; background: #fef3c7; padding: 1px 6px; border-radius: 4px; border: 1px solid #fde68a; font-weight: 500;");
                if (m_translationEdit && m_translationEdit->toPlainText().isEmpty()) {
                    m_translationEdit->setPlaceholderText("未检测到离线神经翻译插件，请前往系统托盘【插件中心】安装。");
                }
            }
        } else {
            m_transStatusBadge->setText("国内通道");
            m_transStatusBadge->setStyleSheet("font-size: 11px; color: #2563eb; background: #eff6ff; padding: 1px 6px; border-radius: 4px; border: 1px solid #bfdbfe; font-weight: 500;");
            if (m_translationEdit && m_translationEdit->toPlainText().isEmpty()) {
                m_translationEdit->setPlaceholderText("点击上方【翻译】获取译文...");
            }
        }
    };
    updateTransBadge();

    connect(m_transModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, updateTransBadge](int) {
        QString mode = m_transModeCombo->currentData().toString();
        if (mode == "Offline") {
            if (!TranslationPluginManager::instance().isOfflinePluginReady()) {
                auto reply = QMessageBox::information(
                    this,
                    "提示",
                    "尚未安装离线翻译插件。\n是否打开插件中心一键安装？",
                    QMessageBox::Ok | QMessageBox::Cancel
                );
                if (reply == QMessageBox::Ok) {
                    static PluginMarketDialog* dlg = nullptr;
                    if (!dlg) dlg = new PluginMarketDialog();
                    dlg->show();
                    dlg->raise();
                    dlg->activateWindow();
                }
            }
            TranslationPluginManager::instance().setPreferredEnginePreference("Offline");
        } else {
            TranslationPluginManager::instance().setPreferredEnginePreference("Online");
        }
        updateTransBadge();
        triggerTranslation();
    });

    connect(&TranslationPluginManager::instance(), &TranslationPluginManager::engineChanged, this, [updateTransBadge](const QString&, bool) {
        updateTransBadge();
    });

    barLayout->addWidget(m_transModeCombo);
    barLayout->addWidget(m_transStatusBadge);
    barLayout->addStretch();

    // 语言对选择下拉框 (支持双向互译，选择后自动触发翻译)
    m_langPairCombo = new QComboBox(m_translationSection);
    m_langPairCombo->addItem("中文 ➔ 英语", "zh:en");
    m_langPairCombo->addItem("英语 ➔ 中文", "en:zh");
    m_langPairCombo->addItem("自动 ➔ 中文", "auto:zh");
    m_langPairCombo->addItem("自动 ➔ 英语", "auto:en");
    m_langPairCombo->addItem("中文 ➔ 日语", "zh:ja");
    m_langPairCombo->addItem("日语 ➔ 中文", "ja:zh");
    m_langPairCombo->addItem("中文 ➔ 韩语", "zh:ko");
    m_langPairCombo->addItem("韩语 ➔ 中文", "ko:zh");
    m_langPairCombo->addItem("中文 ➔ 俄语", "zh:ru");
    m_langPairCombo->addItem("俄语 ➔ 中文", "ru:zh");
    m_langPairCombo->addItem("中文 ➔ 法语", "zh:fr");
    m_langPairCombo->addItem("法语 ➔ 中文", "fr:zh");
    m_langPairCombo->addItem("中文 ➔ 德语", "zh:de");
    m_langPairCombo->addItem("德语 ➔ 中文", "de:zh");
    m_langPairCombo->setFixedWidth(130);

    connect(m_langPairCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        triggerTranslation();
    });

    barLayout->addWidget(m_langPairCombo);

    // 翻译按钮
    m_translateBtn = new QPushButton("翻译", m_translationSection);
    m_translateBtn->setCursor(Qt::PointingHandCursor);
    m_translateBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: #ffffff;"
        "  border: 1px solid #1d4ed8;"
        "  font-weight: 600;"
        "  padding: 4px 14px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
        "QPushButton:disabled { background-color: #94a3b8; border-color: #cbd5e1; }"
    );
    connect(m_translateBtn, &QPushButton::clicked, this, &OcrResultDialog::triggerTranslation);
    barLayout->addWidget(m_translateBtn);

    // 复制译文按钮
    m_copyTransBtn = new QPushButton("复制译文", m_translationSection);
    m_copyTransBtn->setCursor(Qt::PointingHandCursor);
    m_copyTransBtn->setToolTip("复制当前翻译结果");
    connect(m_copyTransBtn, &QPushButton::clicked, this, [this]() {
        if (!m_translationEdit) return;
        QString text = m_translationEdit->toPlainText().trimmed();
        if (text.isEmpty()) return;
        QApplication::clipboard()->setText(text);
        m_copyTransBtn->setText("已复制");
        QTimer::singleShot(1500, this, [this]() {
            if (m_copyTransBtn) m_copyTransBtn->setText("复制译文");
        });
    });
    barLayout->addWidget(m_copyTransBtn);

    transLayout->addLayout(barLayout);

    // 译文展示编辑框
    m_translationEdit = new QPlainTextEdit(m_translationSection);
    m_translationEdit->setPlaceholderText("点击上方【翻译】获取译文...");
    m_translationEdit->setMaximumHeight(140);
    m_translationEdit->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #f8fafc;"
        "  color: #0f172a;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 6px;"
        "  padding: 8px 10px;"
        "  font-family: 'Segoe UI Variable Text', 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "  font-size: 13px;"
        "  line-height: 1.5;"
        "}"
        "QPlainTextEdit:focus { border-color: #3b82f6; background-color: #ffffff; }"
    );
    transLayout->addWidget(m_translationEdit);

    rightLayout->addWidget(m_translationSection);
}

void OcrResultDialog::setImage(const QPixmap& pixmap)
{
    m_image = pixmap;
    m_engineResultCache.clear(); // 截图换新时，清空旧图缓存
    if (m_imagePreview) {
        m_imagePreview->setImage(pixmap);
    }
}

void OcrResultDialog::setResult(const OcrResult& result, const QString& engineName)
{
    QString curEngine = engineName.isEmpty() ? m_currentEngineType : engineName;
    m_result = result;
    if (result.success) {
        m_engineResultCache[curEngine] = result;
    }
    if (m_imagePreview) {
        m_imagePreview->setResult(result);
    }

    if (!result.success) {
        m_textEdit->setPlainText(QString("识别提示: %1").arg(result.errorMessage));
        m_statusLabel->setText("识别异常");
        return;
    }

    updateFormattedText();
    autoDetectSourceLanguage();
}

void OcrResultDialog::updateFormattedText()
{
    QString formatted = processText(m_mergeParagraphs, m_removeExtraSpaces);
    m_textEdit->setPlainText(formatted);

    int charCount = formatted.length();
    int lineCount = formatted.isEmpty() ? 0 : formatted.split('\n').size();
    m_statusLabel->setText(QString("字数: %1  |  行数: %2").arg(charCount).arg(lineCount));
}

QString OcrResultDialog::processText(bool mergeParagraphs, bool removeExtraSpaces)
{
    if (!m_result.success || m_result.lines.isEmpty()) {
        return QString();
    }

    QStringList rawLines;
    for (const auto& line : m_result.lines) {
        if (!line.text.trimmed().isEmpty()) {
            rawLines.append(line.text.trimmed());
        }
    }

    if (mergeParagraphs) {
        QString merged;
        for (const auto& line : rawLines) {
            if (merged.isEmpty()) {
                merged = line;
            } else {
                QChar lastChar = merged.back();
                if (lastChar == '.' || lastChar == '?' || lastChar == '!' ||
                    lastChar == QChar(0x3002) /* 。 */ || lastChar == QChar(0xFF01) /* ！ */ || lastChar == QChar(0xFF1F) /* ？ */) {
                    merged += "\n" + line;
                } else {
                    bool isChinese = (lastChar.unicode() >= 0x4E00 && lastChar.unicode() <= 0x9FA5);
                    QChar firstChar = line.front();
                    bool nextIsChinese = (firstChar.unicode() >= 0x4E00 && firstChar.unicode() <= 0x9FA5);

                    if (isChinese && nextIsChinese) {
                        merged += line;
                    } else {
                        merged += " " + line;
                    }
                }
            }
        }
        rawLines = merged.split('\n');
    }

    QString resultText = rawLines.join('\n');

    if (removeExtraSpaces) {
        static QRegularExpression zhSpaceZh("([\\x{4e00}-\\x{9fa5}])\\s+([\\x{4e00}-\\x{9fa5}])");
        resultText.replace(zhSpaceZh, "\\1\\2");
        static QRegularExpression multiSpaces("[ \\t]{2,}");
        resultText.replace(multiSpaces, " ");
    }

    return resultText;
}

void OcrResultDialog::triggerTranslation()
{
    if (m_isTranslating) {
        return; // 防抖保护：避免高频重复并发请求触发服务防火墙
    }

    QString textToTranslate = processText(m_mergeParagraphs, m_removeExtraSpaces).trimmed();
    if (textToTranslate.isEmpty()) {
        textToTranslate = m_textEdit->toPlainText().trimmed();
    }
    if (textToTranslate.isEmpty()) {
        if (m_translationEdit) m_translationEdit->setPlainText("没有可供翻译的文本内容");
        return;
    }

    m_isTranslating = true;

    QString langPairData = m_langPairCombo ? m_langPairCombo->currentData().toString() : "zh:en";
    QStringList parts = langPairData.split(':');
    QString srcLang = parts.value(0, "auto");
    QString targetLang = parts.value(1, "zh");

    QString mode = m_transModeCombo ? m_transModeCombo->currentData().toString() : "Online";

    if (m_translationEdit) {
        m_translationEdit->setPlainText("正在翻译中...");
    }
    if (m_translateBtn) {
        m_translateBtn->setEnabled(false);
    }

    auto handleResult = [this](const TranslationResult& res) {
        m_isTranslating = false;
        if (m_translateBtn) m_translateBtn->setEnabled(true);
        if (!m_translationEdit) return;

        if (res.success) {
            m_translationEdit->setPlainText(res.translatedText);
            if (m_transStatusBadge) {
                QString badgeText = (res.engineType == TranslationEngineType::OfflinePlugin) ? "本地就绪" : "国内通道";
                m_transStatusBadge->setText(badgeText);
            }
            if (m_transElapsedLabel) {
                m_transElapsedLabel->setText(QString("翻译用时: %1ms (%2)").arg(res.elapsedMs).arg(res.engineType == TranslationEngineType::OfflinePlugin ? "离线" : "在线"));
            }
        } else {
            m_translationEdit->setPlainText("翻译失败: " + res.errorMessage);
            if (m_transElapsedLabel) {
                m_transElapsedLabel->setText("");
            }
        }
    };

    if (mode == "Online") {
        TranslationPluginManager::instance().onlineTranslator()->translateAsync(textToTranslate, srcLang, targetLang, handleResult);
    } else {
        if (!TranslationPluginManager::instance().isOfflinePluginReady()) {
            m_isTranslating = false;
            if (m_translateBtn) m_translateBtn->setEnabled(true);
            if (m_translationEdit) {
                m_translationEdit->setPlaceholderText("未检测到离线神经翻译插件，请前往系统托盘【插件中心】安装。");
            }
            return;
        }
        TranslationPluginManager::instance().offlineTranslator()->translateAsync(textToTranslate, srcLang, targetLang, handleResult);
    }
}

void OcrResultDialog::autoDetectSourceLanguage()
{
    if (!m_textEdit || !m_langPairCombo) return;
    QString sample = m_textEdit->toPlainText();
    QString detected = TranslationPluginManager::detectLanguageHeuristic(sample);

    int targetIdx = -1;
    if (detected == "zh") {
        targetIdx = m_langPairCombo->findData("zh:en");
    } else if (detected == "en") {
        targetIdx = m_langPairCombo->findData("en:zh");
    } else if (detected == "ja") {
        targetIdx = m_langPairCombo->findData("ja:zh");
    } else if (detected == "ko") {
        targetIdx = m_langPairCombo->findData("ko:zh");
    } else {
        targetIdx = m_langPairCombo->findData("auto:zh");
    }

    if (targetIdx >= 0 && m_langPairCombo->currentIndex() != targetIdx) {
        // 索引变化会自动触发 currentIndexChanged 并调用 triggerTranslation()
        m_langPairCombo->setCurrentIndex(targetIdx);
    } else {
        // 索引未变化时，手动发起一次翻译
        triggerTranslation();
    }
}
