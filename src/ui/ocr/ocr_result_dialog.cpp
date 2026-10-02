#include "ocr_result_dialog.h"
#include "../../core/ocr/ocr_manager.h"
#include "../../core/inpainting/image_inpainter.h"
#include "../../core/inpainting/font_attribute_estimator.h"
#include <QApplication>
#include <QInputDialog>
#include <QClipboard>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QGraphicsDropShadowEffect>
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

void OcrImagePreviewWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QPoint pt = event->pos();
    for (int i = 0; i < m_result.lines.size(); ++i) {
        const auto& line = m_result.lines[i];
        if (line.logicalBox.contains(pt)) {
            emit textBlockDoubleClicked(i, line.logicalBox, line.text);
            event->accept();
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(event);
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
    setWindowTitle("文本识别提取");
    resize(960, 580);

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
    auto* leftTitle = new QLabel("原图对照 (💡双击文字框可原地P图修改)", leftContainer);
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
    connect(m_imagePreview, &OcrImagePreviewWidget::textBlockDoubleClicked, this, [this](int lineIdx, const QRect&, const QString& oldText) {
        bool ok = false;
        QString newText = QInputDialog::getText(
            this,
            "原地文字修改 / P图",
            "请输入替换后的文字（原图背景将自动无痕擦除）：",
            QLineEdit::Normal,
            oldText,
            &ok
        );
        if (ok) {
            applyInplaceTextEdit(lineIdx, newText);
        }
    });
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
}

void OcrResultDialog::applyInplaceTextEdit(int lineIndex, const QString& newText)
{
    if (lineIndex < 0 || lineIndex >= m_result.lines.size() || m_image.isNull()) {
        return;
    }

    qreal dpr = m_image.devicePixelRatio();
    if (dpr <= 0.0) dpr = 1.0;

    const auto& line = m_result.lines[lineIndex];
    QRect physBox = line.boundingBox;
    if (physBox.isEmpty()) {
        physBox = QRect(
            static_cast<int>(std::round(line.logicalBox.x() * dpr)),
            static_cast<int>(std::round(line.logicalBox.y() * dpr)),
            static_cast<int>(std::round(line.logicalBox.width() * dpr)),
            static_cast<int>(std::round(line.logicalBox.height() * dpr))
        );
    }

    QImage baseImg = m_image.toImage();

    // 1. 调用背景无痕内容感知擦除修复
    InpaintResult inpaintRes = ImageInpainter::inpaintTextRegion(baseImg, physBox, 2);
    if (!inpaintRes.success) return;

    // 2. 估算字体颜色与字号
    EstimatedFontAttributes attr = FontAttributeEstimator::estimate(baseImg, physBox, inpaintRes.estimatedBgColor, dpr);

    // 3. 在原图上覆盖合成背景抹平补丁与新文字
    QPainter painter(&m_image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 绘制修复底色
    QRect logicalPadded(
        static_cast<int>(std::round(inpaintRes.paddedRect.x() / dpr)),
        static_cast<int>(std::round(inpaintRes.paddedRect.y() / dpr)),
        static_cast<int>(std::round(inpaintRes.paddedRect.width() / dpr)),
        static_cast<int>(std::round(inpaintRes.paddedRect.height() / dpr))
    );
    QPixmap patchPix = QPixmap::fromImage(inpaintRes.inpaintedPatch);
    patchPix.setDevicePixelRatio(dpr);
    painter.drawPixmap(logicalPadded, patchPix);

    // 覆盖绘制新文字
    if (!newText.isEmpty()) {
        painter.setPen(attr.textColor);
        QFont f(attr.fontFamily);
        f.setPixelSize(attr.fontSize);
        f.setWeight(static_cast<QFont::Weight>(attr.fontWeight));
        painter.setFont(f);
        painter.drawText(line.logicalBox, Qt::AlignLeft | Qt::AlignVCenter, newText);
    }
    painter.end();

    // 4. 更新内部结构并实时刷新左右双栏视图
    m_result.lines[lineIndex].text = newText;
    setImage(m_image);
    updateFormattedText();
    m_statusLabel->setText(QString("已成功修改第 %1 行文字为: \"%2\" ✓").arg(lineIndex + 1).arg(newText));
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
