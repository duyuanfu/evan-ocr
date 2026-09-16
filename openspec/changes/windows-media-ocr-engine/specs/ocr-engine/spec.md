## ADDED Requirements

### Requirement: Pluggable OCR Engine Interface
The system SHALL define an abstract interface `IOcrEngine` to decouple OCR recognition logic from specific recognition providers.

#### Scenario: Asynchronous text recognition request
- **WHEN** an image is passed to `IOcrEngine::recognizeAsync(const QImage& image, const QString& languageHint)`
- **THEN** it SHALL return a future or invoke a callback delivering an `OcrResult` struct containing plain text, paragraphs, and word bounding boxes.

### Requirement: Windows Media OCR Engine Implementation
The system SHALL implement `WindowsMediaOcrEngine` utilizing the Windows 10/11 native `Windows.Media.Ocr.OcrEngine` APIs via C++/WinRT.

#### Scenario: Running OCR on memory QImage without file IO
- **WHEN** a valid screenshot `QImage` is submitted to `WindowsMediaOcrEngine`
- **THEN** the image buffer SHALL be converted into a `SoftwareBitmap` in memory and processed by `Windows.Media.Ocr.OcrEngine::RecognizeAsync`, returning recognized text lines and word bounding rectangles without saving temporary files to disk.

#### Scenario: Graceful fallback when language pack unavailable
- **WHEN** the user language is not installed in the Windows system OCR language list
- **THEN** the engine SHALL fallback to the first available recognized language or return a clear error code instead of crashing.
