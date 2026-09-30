#pragma once

#include "ocr_engine.h"
#include <QObject>
#include <QString>

class RapidOcrEngine : public QObject, public IOcrEngine
{
    Q_OBJECT
public:
    explicit RapidOcrEngine(QObject* parent = nullptr);
    ~RapidOcrEngine() override = default;

    QString name() const override { return "RapidOCR (高精度)"; }

    // 探测本地是否存在 RapidOCR 运行组件与模型文件
    bool isAvailable() const override;

    // 获取当前探测到的 RapidOCR 可执行文件/模型路径说明
    QString componentPath() const;

    // 同步执行识别
    OcrResult recognize(const QImage& image, qreal dpr = 1.0) override;

    // 异步执行识别
    void recognizeAsync(const QImage& image, qreal dpr, std::function<void(const OcrResult&)> callback) override;

private:
    QString findExecutablePath() const;
    QString findModelsDir() const;
};
