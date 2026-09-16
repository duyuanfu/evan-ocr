## 1. WinRT 环境配置与数据结构定义

- [x] 1.1 在 CMakeLists.txt 中配置 Windows SDK WinRT 相关依赖库（windowsapp.lib、runtimeobject.lib）并确保 C++20 兼容
- [x] 1.2 创建 `src/core/ocr/ocr_types.h`，定义 `OcrWord`、`OcrLine`、`OcrResult` 实体数据结构
- [x] 1.3 创建 `src/core/ocr/ocr_engine.h`，定义抽象接口基类 `IOcrEngine`

## 2. Windows 原生 WinRT OCR 引擎驱动实现

- [x] 2.1 创建 `src/core/ocr/windows_media_ocr.h` 与 `windows_media_ocr.cpp`
- [x] 2.2 实现 QImage 内存数据转 Windows SDK `SoftwareBitmap`，避免磁盘临时文件读写
- [x] 2.3 调用 `winrt::Windows::Media::Ocr::OcrEngine::RecognizeAsync` 实现高精度异步文本识别与包围盒解析
- [x] 2.4 实现支持可用 OCR 语言包自动检测与智能回退策略

## 3. OCR 结果展示与文本提取弹窗 (OcrResultDialog)

- [x] 3.1 创建 `src/ui/ocr/ocr_result_dialog.h` 与 `ocr_result_dialog.cpp`
- [x] 3.2 布局实现现代化暗色磨砂结果展示界面、文本编辑框及字数统计
- [x] 3.3 实现「一键复制全部文本」、「段落合并 / 保持原始换行」切换、「自动清理多余空格」排版工具

## 4. UI 交互层集成 (截图选区与贴图窗口全打通)

- [x] 4.1 在 `FloatingToolbar` 中增加 `🔤` OCR 文字识别按钮及 ToolAction::Ocr 动作路由
- [x] 4.2 在 `SnippingOverlay` 中响应 OCR 触发，异步截取当前区域并弹出加载提示与结果窗口
- [x] 4.3 在 `PinWindow` 右键上下文菜单与快捷键中增加「识别图中文字 (Ctrl+O)」功能
- [x] 4.4 整体编译链接验证与全链路端到端功能测试
