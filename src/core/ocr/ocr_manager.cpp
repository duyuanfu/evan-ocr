#include "ocr_manager.h"
#include <QSettings>
#include <QDebug>

OcrManager::OcrManager(QObject* parent)
    : QObject(parent)
{
    m_winrtEngine = std::make_unique<WindowsMediaOcrEngine>();
    m_rapidEngine = std::make_unique<RapidOcrEngine>();
}

OcrManager& OcrManager::instance()
{
    static OcrManager mgr;
    return mgr;
}

bool OcrManager::isRapidOcrReady() const
{
    return m_rapidEngine && m_rapidEngine->isAvailable();
}

QString OcrManager::preferredEngineType() const
{
    QSettings settings("Evan", "Evan");
    // 默认首选 RapidOCR
    return settings.value("Ocr/Engine", "RapidOCR").toString();
}

void OcrManager::setPreferredEngineType(const QString& type)
{
    QSettings settings("Evan", "Evan");
    settings.setValue("Ocr/Engine", type);
    emit engineChanged(activeEngineName());
}

IOcrEngine* OcrManager::activeEngine()
{
    QString pref = preferredEngineType();
    if (pref == "RapidOCR") {
        if (m_rapidEngine && m_rapidEngine->isAvailable()) {
            return m_rapidEngine.get();
        }
        // 若未安装 RapidOCR，自动平滑回退到 WindowsMedia
        return m_winrtEngine.get();
    }
    return m_winrtEngine.get();
}

QString OcrManager::activeEngineName()
{
    IOcrEngine* engine = activeEngine();
    return engine ? engine->name() : "未检测到可用引擎";
}

void OcrManager::recognizeAsync(const QImage& image, qreal dpr, std::function<void(const OcrResult&, const QString&)> callback)
{
    IOcrEngine* engine = activeEngine();
    if (!engine) {
        OcrResult fail;
        fail.success = false;
        fail.errorMessage = "当前无可用 OCR 引擎";
        if (callback) callback(fail, "None");
        return;
    }

    QString engineName = engine->name();
    engine->recognizeAsync(image, dpr, [callback, engineName](const OcrResult& res) {
        if (callback) {
            callback(res, engineName);
        }
    });
}

void OcrManager::recognizeWithEngine(const QString& engineType, const QImage& image, qreal dpr, std::function<void(const OcrResult&, const QString&)> callback)
{
    IOcrEngine* target = nullptr;
    if (engineType == "RapidOCR") {
        target = m_rapidEngine.get();
    } else {
        target = m_winrtEngine.get();
    }

    if (!target) {
        target = activeEngine();
    }

    QString engineName = target->name();
    target->recognizeAsync(image, dpr, [callback, engineName](const OcrResult& res) {
        if (callback) {
            callback(res, engineName);
        }
    });
}
