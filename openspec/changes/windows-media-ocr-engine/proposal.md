## Why

截取屏幕信息后，用户最高频的下游诉求是快速复制其中的文本、代码、表格及图片内字样。第一阶段已完成高精度截图、智能吸附、矢量标注及置顶贴图，现急需接入第一款原生高性能 OCR 引擎，为用户提供零依赖、毫秒级启动的文本提取与排版交互能力。

## What Changes

- **OCR 引擎抽象基类 (`IOcrEngine`)**：定义通用的异步文字识别接口与结构化结果实体（文本行、置信度、矩形包围盒）。
- **Windows 原生 WinRT OCR 实现 (`WindowsMediaOcrEngine`)**：基于 Windows 10/11 系统的 `Windows.Media.Ocr.OcrEngine` API，利用 C++/WinRT 或 WinRT COM 接口直接对内存中的图像帧进行 OCR 识别，完全无需携带重量级第三方权重模型与环境依赖，识别速度快且常驻内存开销接近零。
- **浮动工具栏与贴图窗口入口**：
  - 截图工具栏新增 **`🔤` 识别文字** 按钮；
  - 贴图窗口右键菜单新增 **`识别文字 (Ctrl+O)`** 选项。
- **OCR 结果展示与文本处理弹窗 (`OcrResultDialog`)**：
  - 识别成功后弹出现代化暗色轻量弹窗，展示文本内容；
  - 提供一键复制、段落合并/换行保持切换、自动去多余空格功能；
  - 支持直接在原图上叠加字符高亮框进行对照核对。

## Capabilities

### New Capabilities
- `ocr-engine`: 提供可插拔 OCR 引擎抽象层、Windows 原生 WinRT OCR 识别驱动，以及识别结果的数据结构解析。
- `ocr-interaction`: 提供截图选区与贴图窗口的 OCR 唤醒交互、识别结果展示弹窗、排版优化与一键复制功能。

### Modified Capabilities
- `screen-capture`: 工具栏新增 OCR 识别按钮动作流转与图像提取联动。
- `image-pinning`: 贴图右键菜单与快捷键触发当前贴图画面的 OCR 识别任务。

## Impact

- **编译与构建**：引入 Windows SDK WinRT 相关头文件与依赖库（`windowsapp.lib` / `runtimeobject.lib`），启用 WinRT 运行时支持。
- **UI 交互**：截屏浮动工具栏与独立贴图窗口增加 OCR 相关按钮及响应事件。
