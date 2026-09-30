#pragma once

#include "ocr_engine.h"
#include "windows_media_ocr.h"
#include "rapid_ocr_engine.h"
#include <QObject>
#include <memory>

class OcrManager : public QObject
{
    Q_OBJECT
public:
    static OcrManager& instance();

    // 当前生效的识别引擎 (若首选 RapidOCR 但本地未安装，则自动平滑降级为 WindowsMediaOcr)
    IOcrEngine* activeEngine();

    // 获取引擎名称
    QString activeEngineName();

    // RapidOCR 组件是否已就绪
    bool isRapidOcrReady() const;

    // 获取首选引擎偏好 ("RapidOCR" 或 "WindowsMedia")
    QString preferredEngineType() const;
    void setPreferredEngineType(const QString& type);

    // 统一异步识别接口
    void recognizeAsync(const QImage& image, qreal dpr, std::function<void(const OcrResult& result, const QString& engineName)> callback);

    // 指定特定引擎异步识别
    void recognizeWithEngine(const QString& engineType, const QImage& image, qreal dpr, std::function<void(const OcrResult& result, const QString& engineName)> callback);

signals:
    void engineChanged(const QString& newEngineName);

private:
    explicit OcrManager(QObject* parent = nullptr);
    ~OcrManager() override = default;

    std::unique_ptr<WindowsMediaOcrEngine> m_winrtEngine;
    std::unique_ptr<RapidOcrEngine> m_rapidEngine;
};
