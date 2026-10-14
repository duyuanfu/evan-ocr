## Why

截贴图与 OCR 工具在办公与学习场景中高频使用，但普通 OCR 仅能提取平铺文本流，面对科研论文、教学课件、学术报告中的复杂二维结构数学公式（分数、根号、积分、上下标、矩阵等）识别严重失真且无法直接使用。新增公式识别功能，可无缝提取公式为 LaTeX、Markdown 与 Word 原生公式 (MathML)，大幅提升科研、教学与工程人员的办公生产力。

## What Changes

- **截屏工具栏新增独立入口**：在截屏操作栏增加 `∑` (公式识别) 按钮与热键支持，用户框选公式后一键直达。
- **双轨公式识别中枢 (`FormulaOcrManager`)**：
  - 默认高精度在线云端通道（免本地安装庞大模型，毫秒级响应，主程序维持 21MB 体积）；
  - 离线独立插件降级通道（支持 Plugin Market 一键安装 RapidLaTeXOCR 离线模型包），网络不可用时自动无缝降级。
- **LaTeX 转 MathML 与多格式导出助手 (`FormulaFormatHelper`)**：
  - 提取标准 LaTeX 源码；
  - 自动封装为 Markdown 公式语法（`$$ ... $$`）；
  - 自动转换为标准 MathML XML 格式并提供剪贴板推送，用户在 Microsoft Word 中按 `Ctrl+V` 可直接转换为 Word 原生矢量数学公式（OMML）。
- **专属公式结果工作台 (`FormulaResultDialog`)**：
  - 提供截图原图与公式高保真预览对照；
  - 提供可微调编辑的 LaTeX 源码编辑框；
  - 底部操作栏提供三种格式一键复制按钮与引擎通道切换。
- **插件中心挂载**：在 `PluginManager` 中注册 `evan-plugin-latex-ocr`。

## Capabilities

### New Capabilities
- `formula-ocr-recognition`: 提供独立公式识别工具入口、在线与离线双轨识别调度、LaTeX 源码与 MathML/Markdown 格式多重转换输出以及公式结果对比编辑工作台。

### Modified Capabilities
<!-- 无现有 Spec 需求变更 -->

## Impact

- `src/ui/toolbar/floating_toolbar.h/cpp`：新增 `ToolAction::FormulaOcr` 与 `∑` 按钮。
- `src/ui/overlay/snipping_overlay.h/cpp`：对接公式识别动作与快捷键。
- `src/core/formula/`：新增 `formula_ocr_manager.h/cpp` 与 `formula_format_helper.h`。
- `src/ui/formula/`：新增 `formula_result_dialog.h/cpp` 工作台界面。
- `src/core/plugin/plugin_manager.cpp`：注册公式识别离线插件元数据。
