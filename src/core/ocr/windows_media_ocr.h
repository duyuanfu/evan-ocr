#pragma once

#include "ocr_engine.h"
#include <QObject>

class WindowsMediaOcrEngine : public QObject, public IOcrEngine
{
    Q_OBJECT
public:
    explicit WindowsMediaOcrEngine(QObject* parent = nullptr);
    ~WindowsMediaOcrEngine() override = default;

    QString name() const override { return "Windows.Media.Ocr"; }
    bool isAvailable() const override;

    OcrResult recognize(const QImage& image, qreal dpr = 1.0) override;
    void recognizeAsync(const QImage& image, qreal dpr, std::function<void(const OcrResult&)> callback) override;

signals:
    void recognitionFinished(const OcrResult& result);

private:
    bool m_isAvailable = false;
};
