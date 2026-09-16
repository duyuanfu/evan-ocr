## Context

在第一阶段中，EvanOCR 已经建立了高质量的全景多屏截取、像素级放大镜、智能轮廓候选吸附、非破坏性矢量标注以及独立置顶贴图窗体系。然而作为桌面生产力工具，用户截取画面后的核心痛点之一是提取其中的非可选文本（如图片中的文字、设计图、软件界面报错或不可复制的代码）。
本阶段接入 Windows 10/11 原生 `Windows.Media.Ocr` 引擎，实现无外部庞大依赖模型包的零秒启动高精度离线文字识别，并搭建可扩展的 OCR 引擎抽象架构。

## Goals / Non-Goals

**Goals:**
- **架构解耦**：设计 `IOcrEngine` 统一抽象基类与 `OcrResult` / `OcrLine` / `OcrWord` 实体结构，便于后续平滑接入 RapidOCR (ONNX Runtime) 或 Apple Vision OCR。
- **WinRT 驱动实现**：基于 Windows SDK 原生 C++/WinRT API (`winrt::Windows::Media::Ocr::OcrEngine`) 实现内存级位图识别，严禁磁盘临时文件交换。
- **UI 交互全贯通**：
  - 截图悬浮工具栏增加 `🔤` 识别文字按钮；
  - 贴图窗口增加右键上下文菜单项“识别文字 (Ctrl+O)”；
  - 识别成功后调起轻量现代化 `OcrResultDialog`，提供纯文本预览、自动换行与段落合并切换、一键复制。

**Non-Goals:**
- 本阶段不引入动辄上百兆的 ONNX/PyTorch 权重模型或 CUDA 运行时（留待后续可插拔模块扩展）。
- 本阶段不引入在线云端 OCR API（保持 100% 本地离线与数据隐私安全）。

## Decisions

### 1. 采用 C++/WinRT 内存转换 `SoftwareBitmap`
- **决定**：直接将 Qt 的 `QImage` (Format_RGBA8888 或 Format_ARGB32) 内存数据复制或包装为 Windows 原生 `SoftwareBitmap` (BitmapPixelFormat::Rgba8 / Bgra8)，并交由 `OcrEngine::RecognizeAsync` 处理。
- **替代方案**：通过 QBuffer 保存为 PNG 字节流再用 `BitmapDecoder::CreateAsync` 解码。相比之下，直接内存拷贝避免了解压与编解码 CPU 开销，性能提升近 3 倍。

### 2. 线程模型与异步流转
- **决定**：OCR 任务在 `QThreadPool` / `std::async` 工作线程中执行，通过 Qt 信号槽机制（`ocrCompleted` / `ocrFailed`）主线程回调投递结果，确保主界面 120fps 流畅无卡顿。

### 3. CMake 链接 Windows App 运行时
- **决定**：在 CMake 中为目标添加 `windowsapp.lib` 与 `runtimeobject.lib`，并启用 `/std:c++20` 与 `/permissive-`，完美适配 MSVC 2022。

## Risks / Trade-offs

- **[Windows 语言包缺失风险]**：部分精简版或特殊企业版 Windows 可能未预装中英文 OCR 识别语言包。
  - *缓解措施*：在引擎初始化时调用 `OcrEngine::AvailableRecognizerLanguages()` 检测，若默认语言不可用，自动回退到第一个可用语言并向用户提供友好提示。
- **[DPI 坐标映射]**：截图具有 DPR 缩放，OCR 识别得到的文字坐标属于物理像素。
  - *缓解措施*：在解析 `OcrResult` 时同时记录物理像素矩形与逻辑像素矩形，使后续视觉高亮和贴图框选完全对齐。
