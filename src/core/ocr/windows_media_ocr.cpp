#include "windows_media_ocr.h"
#include <QThreadPool>
#include <QMetaObject>
#include <QDebug>
#include <cmath>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Storage::Streams;

namespace {
    struct WinRTApartmentScope {
        WinRTApartmentScope() {
            try {
                winrt::init_apartment(winrt::apartment_type::multi_threaded);
            } catch (...) {
                // 当前线程可能已经初始化 COM/WinRT
            }
        }
    };
}

WindowsMediaOcrEngine::WindowsMediaOcrEngine(QObject* parent)
    : QObject(parent)
{
    WinRTApartmentScope scope;
    try {
        auto langs = winrt::Windows::Media::Ocr::OcrEngine::AvailableRecognizerLanguages();
        m_isAvailable = (langs.Size() > 0);
    } catch (...) {
        m_isAvailable = false;
    }
}

bool WindowsMediaOcrEngine::isAvailable() const
{
    return m_isAvailable;
}

OcrResult WindowsMediaOcrEngine::recognize(const QImage& image, qreal dpr)
{
    WinRTApartmentScope scope;
    OcrResult result;
    result.dpr = dpr;

    if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
        result.success = false;
        result.errorMessage = "待识别图像数据为空";
        return result;
    }

    try {
        // 1. 尝试获取首选识别语言引擎或系统可用语言
        winrt::Windows::Media::Ocr::OcrEngine engine = nullptr;
        try {
            engine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromUserProfileLanguages();
        } catch (...) {}

        if (!engine) {
            auto langs = winrt::Windows::Media::Ocr::OcrEngine::AvailableRecognizerLanguages();
            if (langs.Size() > 0) {
                engine = winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(langs.GetAt(0));
            }
        }

        if (!engine) {
            result.success = false;
            result.errorMessage = "当前 Windows 系统未检测到可用的 OCR 识别语言包";
            return result;
        }

        // 2. 将 QImage 转换为内存 SoftwareBitmap (Bgra8 格式直接对齐 Format_ARGB32_Premultiplied)
        QImage bgraImg = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        const int w = bgraImg.width();
        const int h = bgraImg.height();

        DataWriter writer;
        writer.WriteBytes(winrt::array_view<const uint8_t>(bgraImg.constBits(), bgraImg.constBits() + bgraImg.sizeInBytes()));
        IBuffer buffer = writer.DetachBuffer();

        SoftwareBitmap softwareBitmap = SoftwareBitmap::CreateCopyFromBuffer(
            buffer,
            BitmapPixelFormat::Bgra8,
            w,
            h,
            BitmapAlphaMode::Premultiplied
        );

        // 3. 执行识别 (在当前工作线程阻塞等待结果)
        auto asyncOp = engine.RecognizeAsync(softwareBitmap);
        auto ocrResult = asyncOp.get();

        result.success = true;
        QString fullText;

        for (auto const& line : ocrResult.Lines()) {
            OcrLine lineObj;
            lineObj.text = QString::fromWCharArray(line.Text().c_str());

            QRect lineBox;
            for (auto const& word : line.Words()) {
                OcrWord wordObj;
                wordObj.text = QString::fromWCharArray(word.Text().c_str());
                auto r = word.BoundingRect();

                wordObj.boundingBox = QRect(
                    static_cast<int>(std::round(r.X)),
                    static_cast<int>(std::round(r.Y)),
                    static_cast<int>(std::round(r.Width)),
                    static_cast<int>(std::round(r.Height))
                );

                wordObj.logicalBox = QRect(
                    static_cast<int>(std::round(r.X / dpr)),
                    static_cast<int>(std::round(r.Y / dpr)),
                    static_cast<int>(std::round(r.Width / dpr)),
                    static_cast<int>(std::round(r.Height / dpr))
                );

                if (lineBox.isNull()) {
                    lineBox = wordObj.boundingBox;
                } else {
                    lineBox = lineBox.united(wordObj.boundingBox);
                }

                lineObj.words.append(wordObj);
            }

            lineObj.boundingBox = lineBox;
            lineObj.logicalBox = QRect(
                static_cast<int>(std::round(lineBox.x() / dpr)),
                static_cast<int>(std::round(lineBox.y() / dpr)),
                static_cast<int>(std::round(lineBox.width() / dpr)),
                static_cast<int>(std::round(lineBox.height() / dpr))
            );

            result.lines.append(lineObj);
            if (!fullText.isEmpty()) {
                fullText += "\n";
            }
            fullText += lineObj.text;
        }

        result.fullText = fullText;

    } catch (const winrt::hresult_error& e) {
        result.success = false;
        result.errorMessage = QString::fromWCharArray(e.message().c_str());
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = QString::fromUtf8(e.what());
    } catch (...) {
        result.success = false;
        result.errorMessage = "执行 OCR 识别时发生未知异常";
    }

    return result;
}

void WindowsMediaOcrEngine::recognizeAsync(const QImage& image, qreal dpr, std::function<void(const OcrResult&)> callback)
{
    QImage imgCopy = image.copy();
    QThreadPool::globalInstance()->start([this, imgCopy, dpr, callback]() {
        OcrResult res = recognize(imgCopy, dpr);
        QMetaObject::invokeMethod(this, [this, callback, res]() {
            if (callback) {
                callback(res);
            }
            emit recognitionFinished(res);
        }, Qt::QueuedConnection);
    });
}
