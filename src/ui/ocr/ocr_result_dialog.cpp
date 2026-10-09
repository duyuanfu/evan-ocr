#include "ocr_result_dialog.h"
#include "../../core/ocr/ocr_manager.h"
#include "../../core/translation/translation_plugin_manager.h"
#include <QApplication>
#include <QClipboard>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <QDebug>
#include <algorithm>

// -------------------------------------------------------------
// OcrImagePreviewWidget: 左侧原图对比与识别框高亮挂件
// -------------------------------------------------------------
OcrImagePreviewWidget::OcrImagePreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setStyleSheet("background-color: #f8fafc;");
}

void OcrImagePreviewWidget::setImage(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
    if (!m_pixmap.isNull()) {
        qreal dpr = m_pixmap.devicePixelRatio();
        if (dpr <= 0.0) dpr = 1.0;
        setFixedSize(static_cast<int>(m_pixmap.width() / dpr), static_cast<int>(m_pixmap.height() / dpr));
    }
    update();
}

void OcrImagePreviewWidget::setResult(const OcrResult& result)
{
    m_result = result;
    update();
}

void OcrImagePreviewWidget::setShowBoundingBoxes(bool show)
{
    m_showBoundingBoxes = show;
    update();
}

void OcrImagePreviewWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 绘制浅色优雅画布底色
    painter.fillRect(rect(), QColor(248, 250, 252));

    if (m_pixmap.isNull()) {
        painter.setPen(QColor(148, 163, 184));
        painter.drawText(rect(), Qt::AlignCenter, "暂无图像");
        return;
    }

    // 绘制原图
    painter.drawPixmap(0, 0, m_pixmap);

    // 绘制高亮文字包围框 (清爽天蓝半透明覆盖 + 1px 细边框)
    if (m_showBoundingBoxes && m_result.success) {
        for (const auto& line : m_result.lines) {
            QRect box = line.logicalBox;
            if (box.isNull() || !box.isValid()) continue;

            // 柔和微透明天蓝背景块
            painter.fillRect(box, QColor(37, 99, 235, 30));

            // 鲜亮天蓝细边框
            painter.setPen(QPen(QColor(37, 99, 235, 180), 1.0, Qt::SolidLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(box);
        }
    }
}

// -------------------------------------------------------------
// OcrResultDialog: 纯净明亮现代风格对比界面 (非黑色背景)
// -------------------------------------------------------------
OcrResultDialog::OcrResultDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint);
    setWindowTitle("文本识别提取与翻译");
    resize(1040, 680);

    setupUi();
}

void OcrResultDialog::setupUi()
{
    // 现代明亮极简配色风格 (macOS / Windows 11 Fluent Light 美学)
    setStyleSheet(
        "QDialog {"
        "  background-color: #f1f5f9;"
        "  color: #0f172a;"
        "}"
        "QSplitter::handle {"
        "  background-color: #e2e8f0;"
        "  width: 2px;"
        "}"
        "QSplitter::handle:hover {"
        "  background-color: #3b82f6;"
        "}"
        "QScrollArea {"
        "  background-color: #f8fafc;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 8px;"
        "}"
        "QPlainTextEdit {"
        "  background-color: #ffffff;"
        "  color: #0f172a;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 8px;"
        "  padding: 14px;"
        "  font-family: 'Segoe UI Variable Text', 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "  font-size: 14px;"
        "  line-height: 1.6;"
        "  selection-background-color: #3b82f6;"
        "  selection-color: #ffffff;"
        "}"
        "QPlainTextEdit:focus {"
        "  border-color: #3b82f6;"
        "}"
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
        "  background-color: #f8fafc;"
        "  color: #0f172a;"
        "  border-color: #94a3b8;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #e2e8f0;"
        "}"
        "QPushButton:checked {"
        "  background-color: #eff6ff;"
        "  color: #2563eb;"
        "  border-color: #3b82f6;"
        "}"
        "QLabel {"
        "  color: #475569;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "}"
        "QScrollBar:vertical {"
        "  background: #f1f5f9;"
        "  width: 8px;"
        "  margin: 0px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #cbd5e1;"
        "  min-height: 20px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #94a3b8;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        "QComboBox {"
        "  background-color: #ffffff;"
        "  color: #334155;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 4px 8px;"
        "  font-size: 12px;"
        "  font-weight: 500;"
        "}"
        "QComboBox:hover { border-color: #3b82f6; }"
        "QComboBox QAbstractItemView {"
        "  background-color: #ffffff;"
        "  color: #0f172a;"
        "  selection-background-color: #eff6ff;"
        "  selection-color: #2563eb;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 4px;"
        "}"
    );

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 16);
    mainLayout->setSpacing(12);

    // 核心对比区分割器 (QSplitter)
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
    auto* leftTitle = new QLabel("原图对照", leftContainer);
    leftTitle->setStyleSheet("font-size: 13px; color: #1e293b; font-weight: bold;");
    leftHeaderLayout->addWidget(leftTitle);
    leftHeaderLayout->addStretch();

    m_toggleBoxesBtn = new QPushButton("识别框", leftContainer);
    m_toggleBoxesBtn->setCheckable(true);
    m_toggleBoxesBtn->setChecked(true);
    m_toggleBoxesBtn->setToolTip("在原图上显示/隐藏文本定位标记框");
    connect(m_toggleBoxesBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_showBoundingBoxes = checked;
        if (m_imagePreview) {
            m_imagePreview->setShowBoundingBoxes(checked);
        }
    });
    leftHeaderLayout->addWidget(m_toggleBoxesBtn);
    leftLayout->addLayout(leftHeaderLayout);

    m_imageScrollArea = new QScrollArea(leftContainer);
    m_imageScrollArea->setWidgetResizable(false);
    m_imageScrollArea->setAlignment(Qt::AlignCenter);

    m_imagePreview = new OcrImagePreviewWidget(m_imageScrollArea);
    m_imageScrollArea->setWidget(m_imagePreview);
    leftLayout->addWidget(m_imageScrollArea, 1);

    m_splitter->addWidget(leftContainer);

    // ==========================================
    // 右侧面板：识别文字编辑区
    // ==========================================
    auto* rightContainer = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(6, 0, 0, 0);
    rightLayout->setSpacing(8);

    auto* rightHeaderLayout = new QHBoxLayout();
    auto* rightTitle = new QLabel("提取文字", rightContainer);
    rightTitle->setStyleSheet("font-size: 13px; color: #1e293b; font-weight: bold;");
    rightHeaderLayout->addWidget(rightTitle);

    m_engineLabel = new QLabel("引擎: 检测中...", rightContainer);
    m_engineLabel->setStyleSheet("font-size: 11px; color: #2563eb; background: #eff6ff; padding: 2px 8px; border-radius: 4px; border: 1px solid #bfdbfe; font-weight: 500;");
    rightHeaderLayout->addWidget(m_engineLabel);

    m_switchEngineBtn = new QPushButton("🔄 切换引擎", rightContainer);
    m_switchEngineBtn->setToolTip("在 RapidOCR (高精度) 与 Windows 原生 OCR 之间切换");
    connect(m_switchEngineBtn, &QPushButton::clicked, this, [this]() {
        if (m_image.isNull()) return;

        // 切换引擎类型
        QString nextType = (m_currentEngineType == "RapidOCR") ? "WindowsMedia" : "RapidOCR";
        m_currentEngineType = nextType;
        OcrManager::instance().setPreferredEngineType(nextType);

        m_textEdit->setPlainText("正在使用 " + (nextType == "RapidOCR" ? QString("RapidOCR") : QString("Windows 原生引擎")) + " 重新识别中...");
        m_engineLabel->setText("正在切换引擎识别...");

        OcrManager::instance().recognizeWithEngine(nextType, m_image.toImage(), m_image.devicePixelRatio(), [this](const OcrResult& res, const QString& engineName) {
            setResult(res, engineName);
        });
    });
    rightHeaderLayout->addWidget(m_switchEngineBtn);

    rightHeaderLayout->addStretch();

    m_mergeBtn = new QPushButton("合并段落", rightContainer);
    m_mergeBtn->setCheckable(true);
    m_mergeBtn->setToolTip("智能根据标点符合并将断行拼接为完整自然段落");
    connect(m_mergeBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_mergeParagraphs = checked;
        updateFormattedText();
    });
    rightHeaderLayout->addWidget(m_mergeBtn);

    m_removeSpacesBtn = new QPushButton("清除空格", rightContainer);
    m_removeSpacesBtn->setCheckable(true);
    m_removeSpacesBtn->setToolTip("自动消除汉字之间的多余空格");
    connect(m_removeSpacesBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_removeExtraSpaces = checked;
        updateFormattedText();
    });
    rightHeaderLayout->addWidget(m_removeSpacesBtn);
    rightLayout->addLayout(rightHeaderLayout);

    m_textEdit = new QPlainTextEdit(rightContainer);
    m_textEdit->setPlaceholderText("正在识别文字中，请稍候...");
    rightLayout->addWidget(m_textEdit, 1);

    // 下部：嵌入式可交互「🌐 译文对照」面板 (支持离线神经网络与轻量在线双轨模式)
    setupTranslationPanel(rightLayout);

    m_splitter->addWidget(rightContainer);

    // 默认比例 48 : 52
    m_splitter->setSizes({460, 500});
    mainLayout->addWidget(m_splitter, 1);

    // ==========================================
    // 底部工具与状态栏
    // ==========================================
    auto* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);

    m_statusLabel = new QLabel("字数: 0  |  行数: 0", this);
    m_statusLabel->setStyleSheet("color: #64748b; font-size: 12px; font-weight: normal;");
    bottomLayout->addWidget(m_statusLabel);
    bottomLayout->addStretch();

    m_copyBtn = new QPushButton("复制全部文本", this);
    m_copyBtn->setCursor(Qt::PointingHandCursor);
    m_copyBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: #ffffff;"
        "  border: 1px solid #1d4ed8;"
        "  font-weight: 600;"
        "  padding: 7px 20px;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
        "QPushButton:pressed { background-color: #1e40af; }"
    );
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_textEdit->toPlainText());
        m_copyBtn->setText("已复制到剪贴板 ✓");
        QTimer::singleShot(1500, this, [this]() {
            m_copyBtn->setText("复制全部文本");
        });
    });
    bottomLayout->addWidget(m_copyBtn);

    m_closeBtn = new QPushButton("关闭", this);
    m_closeBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  border: 1px solid #cbd5e1;"
        "  color: #64748b;"
        "  padding: 7px 18px;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #f8fafc;"
        "  color: #0f172a;"
        "}"
    );
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(bottomLayout);
}

void OcrResultDialog::setImage(const QPixmap& pixmap)
{
    m_image = pixmap;
    if (m_imagePreview) {
        m_imagePreview->setImage(pixmap);
    }
}

void OcrResultDialog::setResult(const OcrResult& result, const QString& engineName)
{
    m_result = result;
    if (m_imagePreview) {
        m_imagePreview->setResult(result);
    }

    QString currentName = engineName.isEmpty() ? OcrManager::instance().activeEngineName() : engineName;
    if (m_engineLabel) {
        m_engineLabel->setText(QString("引擎: %1").arg(currentName));
    }

    if (!result.success) {
        m_textEdit->setPlainText(QString("识别提示: %1").arg(result.errorMessage));
        m_statusLabel->setText("识别异常 / 需配置");
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
    if (m_result.lines.isEmpty()) {
        return m_result.fullText;
    }

    QStringList rawLines;
    for (const auto& line : m_result.lines) {
        QString text = line.text.trimmed();
        if (!text.isEmpty()) {
            rawLines.append(text);
        }
    }

    if (mergeParagraphs) {
        QString merged;
        for (int i = 0; i < rawLines.size(); ++i) {
            const QString& line = rawLines[i];
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
        // 去除中文字符之间的空格
        static QRegularExpression zhSpaceZh("([\\x{4e00}-\\x{9fa5}])\\s+([\\x{4e00}-\\x{9fa5}])");
        resultText.replace(zhSpaceZh, "\\1\\2");
        // 去除连续多个多余空白字符
        static QRegularExpression multiSpaces("[ \\t]{2,}");
        resultText.replace(multiSpaces, " ");
    }

    return resultText;
}

void OcrResultDialog::setupTranslationPanel(QVBoxLayout* rightLayout)
{
    m_translationSection = new QWidget(this);
    m_translationSection->setObjectName("transSection");
    m_translationSection->setStyleSheet(
        "QWidget#transSection {"
        "  background-color: #f8fafc;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 8px;"
        "}"
    );

    auto* transLayout = new QVBoxLayout(m_translationSection);
    transLayout->setContentsMargins(10, 8, 10, 8);
    transLayout->setSpacing(6);

    // 翻译工具与语言选择栏
    auto* barLayout = new QHBoxLayout();
    barLayout->setSpacing(6);

    auto* transTitle = new QLabel("🌐 译文对照", m_translationSection);
    transTitle->setStyleSheet("font-size: 13px; color: #1e293b; font-weight: bold;");
    barLayout->addWidget(transTitle);

    // 引擎就绪状态指示器
    bool isOffline = TranslationPluginManager::instance().isOfflinePluginReady();
    m_transEngineLabel = new QLabel(m_translationSection);
    auto updateBadge = [this](bool offline) {
        if (offline) {
            m_transEngineLabel->setText("⚡ 离线插件就绪");
            m_transEngineLabel->setStyleSheet("font-size: 11px; color: #059669; background: #ecfdf5; padding: 2px 6px; border-radius: 4px; border: 1px solid #a7f3d0; font-weight: 500;");
            m_transEngineLabel->setToolTip("已启用纯本地离线神经网络翻译 (0网络·100%本地隐私)");
        } else {
            m_transEngineLabel->setText("🌐 在线直连备用");
            m_transEngineLabel->setStyleSheet("font-size: 11px; color: #2563eb; background: #eff6ff; padding: 2px 6px; border-radius: 4px; border: 1px solid #bfdbfe; font-weight: 500;");
            m_transEngineLabel->setToolTip("未检测到 plugins/translation/ 离线模型包，当前使用轻量免配置在线备用引擎");
        }
    };
    updateBadge(isOffline);
    connect(&TranslationPluginManager::instance(), &TranslationPluginManager::engineChanged, this, [updateBadge](const QString&, bool offline) {
        updateBadge(offline);
    });
    barLayout->addWidget(m_transEngineLabel);

    // 离线插件说明/配置引导按钮
    m_pluginHelpBtn = new QPushButton("📦 离线扩展包", m_translationSection);
    m_pluginHelpBtn->setToolTip("查看离线翻译模型扩展包安装说明或打开插件目录");
    m_pluginHelpBtn->setStyleSheet("QPushButton { font-size: 11px; padding: 3px 8px; color: #64748b; background: transparent; border: 1px dashed #cbd5e1; } QPushButton:hover { color: #0f172a; border-color: #3b82f6; }");
    connect(m_pluginHelpBtn, &QPushButton::clicked, this, [this]() {
        QString dir = TranslationPluginManager::instance().pluginDirectory();
        QString msg = QString(
            "<h3>Evan 离线翻译模型扩展插件说明</h3>"
            "<p>为保持 Evan 主程序极简轻量（仅 20MB），离线神经翻译模型作为可选扩展包独立提供。</p>"
            "<hr/>"
            "<p><b>如何开启纯本地离线翻译：</b></p>"
            "<ol>"
            "<li>前往 Releases 页面下载 <b>evan-plugin-translation-*.zip</b> 扩展模型包；</li>"
            "<li>解压到插件目录：<code>%1</code>；</li>"
            "<li>Evan 将自动热加载并点亮【⚡ 离线插件就绪】！</li>"
            "</ol>"
            "<p>未安装插件时，系统会自动启用免费在线直连备用引擎，开箱即用。</p>"
        ).arg(dir);

        QMessageBox box(this);
        box.setWindowTitle("离线翻译插件指南");
        box.setText(msg);
        box.setIcon(QMessageBox::Information);
        auto* openFolderBtn = box.addButton("打开插件目录", QMessageBox::ActionRole);
        auto* dlPageBtn = box.addButton("前往下载页面", QMessageBox::ActionRole);
        box.addButton("关闭", QMessageBox::RejectRole);
        box.exec();

        if (box.clickedButton() == openFolderBtn) {
            QDir().mkpath(dir);
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
        } else if (box.clickedButton() == dlPageBtn) {
            QDesktopServices::openUrl(QUrl(TranslationPluginManager::instance().pluginDownloadUrl()));
        }
    });
    barLayout->addWidget(m_pluginHelpBtn);

    barLayout->addStretch();

    // 源语言选择
    m_srcLangCombo = new QComboBox(m_translationSection);
    m_srcLangCombo->addItem("自动检测", "auto");
    m_srcLangCombo->addItem("英语", "en");
    m_srcLangCombo->addItem("中文 (简体)", "zh");
    m_srcLangCombo->addItem("日语", "ja");
    m_srcLangCombo->addItem("韩语", "ko");
    m_srcLangCombo->addItem("俄语", "ru");
    m_srcLangCombo->addItem("法语", "fr");
    m_srcLangCombo->addItem("德语", "de");
    m_srcLangCombo->setFixedWidth(105);
    barLayout->addWidget(m_srcLangCombo);

    auto* arrowLabel = new QLabel("➔", m_translationSection);
    arrowLabel->setStyleSheet("color: #94a3b8; font-weight: bold;");
    barLayout->addWidget(arrowLabel);

    // 目标语言选择
    m_targetLangCombo = new QComboBox(m_translationSection);
    m_targetLangCombo->addItem("中文 (简体)", "zh");
    m_targetLangCombo->addItem("英语", "en");
    m_targetLangCombo->addItem("日语", "ja");
    m_targetLangCombo->addItem("韩语", "ko");
    m_targetLangCombo->addItem("俄语", "ru");
    m_targetLangCombo->addItem("法语", "fr");
    m_targetLangCombo->addItem("德语", "de");
    m_targetLangCombo->setFixedWidth(105);
    barLayout->addWidget(m_targetLangCombo);

    // 翻译按钮
    m_translateBtn = new QPushButton("🌐 翻译", m_translationSection);
    m_translateBtn->setCursor(Qt::PointingHandCursor);
    m_translateBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #2563eb;"
        "  color: #ffffff;"
        "  border: 1px solid #1d4ed8;"
        "  font-weight: 600;"
        "  padding: 4px 12px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #1d4ed8; }"
        "QPushButton:disabled { background-color: #94a3b8; border-color: #cbd5e1; }"
    );
    connect(m_translateBtn, &QPushButton::clicked, this, &OcrResultDialog::triggerTranslation);
    barLayout->addWidget(m_translateBtn);

    // 复制译文按钮
    m_copyTransBtn = new QPushButton("📋 复制译文", m_translationSection);
    m_copyTransBtn->setCursor(Qt::PointingHandCursor);
    m_copyTransBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffffff;"
        "  color: #334155;"
        "  border: 1px solid #cbd5e1;"
        "  font-weight: 500;"
        "  padding: 4px 10px;"
        "  border-radius: 5px;"
        "}"
        "QPushButton:hover { background-color: #f1f5f9; color: #0f172a; border-color: #94a3b8; }"
    );
    connect(m_copyTransBtn, &QPushButton::clicked, this, [this]() {
        if (!m_translationEdit) return;
        QString text = m_translationEdit->toPlainText().trimmed();
        if (text.isEmpty()) return;
        QApplication::clipboard()->setText(text);
        m_copyTransBtn->setText("✓ 已复制译文");
        QTimer::singleShot(1500, this, [this]() {
            if (m_copyTransBtn) m_copyTransBtn->setText("📋 复制译文");
        });
    });
    barLayout->addWidget(m_copyTransBtn);

    transLayout->addLayout(barLayout);

    // 译文展示编辑框
    m_translationEdit = new QPlainTextEdit(m_translationSection);
    m_translationEdit->setPlaceholderText("点击上方【🌐 翻译】获取精准翻译，自动继承段落合并与空格清洗规则...");
    m_translationEdit->setMaximumHeight(160);
    m_translationEdit->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #ffffff;"
        "  color: #0f172a;"
        "  border: 1px solid #cbd5e1;"
        "  border-radius: 6px;"
        "  padding: 8px 10px;"
        "  font-family: 'Segoe UI Variable Text', 'Segoe UI', 'Microsoft YaHei', sans-serif;"
        "  font-size: 13px;"
        "  line-height: 1.5;"
        "}"
        "QPlainTextEdit:focus { border-color: #3b82f6; }"
    );
    transLayout->addWidget(m_translationEdit);

    rightLayout->addWidget(m_translationSection);
}

void OcrResultDialog::triggerTranslation()
{
    QString textToTranslate = processText(m_mergeParagraphs, m_removeExtraSpaces).trimmed();
    if (textToTranslate.isEmpty()) {
        if (m_translationEdit) m_translationEdit->setPlainText("没有可供翻译的文本内容");
        return;
    }

    QString srcLang = m_srcLangCombo ? m_srcLangCombo->currentData().toString() : "auto";
    QString targetLang = m_targetLangCombo ? m_targetLangCombo->currentData().toString() : "zh";

    if (m_translationEdit) {
        m_translationEdit->setPlainText("正在进行翻译中，请稍候...");
    }
    if (m_translateBtn) {
        m_translateBtn->setEnabled(false);
    }

    TranslationPluginManager::instance().translateAsync(textToTranslate, srcLang, targetLang, [this](const TranslationResult& res) {
        if (m_translateBtn) m_translateBtn->setEnabled(true);
        if (!m_translationEdit) return;

        if (res.success) {
            m_translationEdit->setPlainText(res.translatedText);
            if (m_transEngineLabel) {
                QString badgeText = (res.engineType == TranslationEngineType::OfflinePlugin) ? "⚡ 离线插件" : "🌐 在线直连";
                m_transEngineLabel->setText(QString("%1 (%2ms)").arg(badgeText).arg(res.elapsedMs));
            }
        } else {
            m_translationEdit->setPlainText("翻译失败: " + res.errorMessage);
        }
    });
}

void OcrResultDialog::autoDetectSourceLanguage()
{
    if (!m_textEdit || !m_srcLangCombo || !m_targetLangCombo) return;
    QString sample = m_textEdit->toPlainText();
    QString detected = TranslationPluginManager::detectLanguageHeuristic(sample);

    if (detected == "zh") {
        int srcIdx = m_srcLangCombo->findData("zh");
        if (srcIdx >= 0) m_srcLangCombo->setCurrentIndex(srcIdx);
        int tgtIdx = m_targetLangCombo->findData("en");
        if (tgtIdx >= 0) m_targetLangCombo->setCurrentIndex(tgtIdx);
    } else {
        int srcIdx = m_srcLangCombo->findData("auto");
        if (detected != "auto") {
            int specificIdx = m_srcLangCombo->findData(detected);
            if (specificIdx >= 0) srcIdx = specificIdx;
        }
        if (srcIdx >= 0) m_srcLangCombo->setCurrentIndex(srcIdx);

        int tgtIdx = m_targetLangCombo->findData("zh");
        if (tgtIdx >= 0) m_targetLangCombo->setCurrentIndex(tgtIdx);
    }
}

void OcrResultDialog::updateTranslationLanguages()
{
}
