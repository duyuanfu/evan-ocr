#pragma once

#include "ocr_types.h"
#include <QImage>
#include <functional>
#include <memory>

class IOcrEngine {
public:
    virtual ~IOcrEngine() = default;

    // 引擎名称
    virtual QString name() const = 0;

    // 是否支持该平台 / 当前硬件是否可用
    virtual bool isAvailable() const = 0;

    // 同步执行识别 (内部阻塞，推荐在工作线程中调用)
    virtual OcrResult recognize(const QImage& image, qreal dpr = 1.0) = 0;

    // 异步执行识别，完成时触发 callback (主线程或工作线程均可调用)
    virtual void recognizeAsync(const QImage& image, qreal dpr, std::function<void(const OcrResult&)> callback) = 0;
};
