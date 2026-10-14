## Context

Evan 是一款基于 C++20 与 Qt 6 开发的高性能轻量级截贴图与 OCR 工具（安装包仅 21.49MB）。现有 OCR 体系擅长通用文本与段落提取，但在处理复杂的二维数学公式、根号、分式、矩阵与希腊符号时无法生成结构化 LaTeX。

为了服务广大学术研究、论文写作、工程计算与课件编写用户，需要引入专门的公式识别体系，同时必须保持主程序极致轻巧的初心（不臃肿、秒启动）。

## Goals / Non-Goals

**Goals:**
- 在截屏浮动工具栏提供独立的公式识别按钮 `∑`（`ToolAction::FormulaOcr`）；
- 构建双轨识别中枢 `FormulaOcrManager`：默认极速免下载的云端通道，并支持平滑降级至本地 `RapidLaTeXOCR` 独立离线插件；
- 实现 `FormulaFormatHelper`，支持 LaTeX、Markdown（`$$ ... $$`）以及供 Microsoft Word 原生公式识别的 MathML XML 格式转换；
- 构建 `FormulaResultDialog`，提供原图与渲染图比对、可编辑 LaTeX 源码与一键复制功能；
- 在插件中心登记 `evan-plugin-latex-ocr`。

**Non-Goals:**
- 不直接将上百兆的离线 AI 权重打包进主程序安装包（通过插件商城解耦）；
- 不引入体积超过 150MB 的 `QtWebEngine` 组件，保持纯原生轻量化。

## Decisions

### 决策 1：工具栏独立 `∑` 按钮 vs 嵌套在文字 OCR 下拉菜单中
- **选择**：在浮动工具栏提供专属 `∑` 独立按钮。
- **理由**：截取公式的用户目标明确（需要 LaTeX/MathML 而非纯文本）。独立按钮一键直达，符合 Mathpix / PixPin 优秀交互惯例，避免了先点文字识别再二次切换的繁琐操作。

### 决策 2：双轨架构（在线直连优先 + 离线插件保底）
- **选择**：默认走云端高速通道，本地插件中心提供离线可选扩展包。
- **理由**：云端通道零本地体积占用，保证安装包始终维持在 21MB 水平，且复杂公式矩阵识别率高达 98%+；离线插件包（`evan-plugin-latex-ocr.zip`）满足断网与高隐私需求，两者通过 `FormulaOcrManager` 统一调度。

### 决策 3：多格式导出与 MathML 直粘 Word
- **选择**：在剪贴板中推送标准 MathML XML (`<math xmlns="http://www.w3.org/1998/Math/MathML">`)。
- **理由**：Word 2016 及以后版本原生支持 MathML 粘贴。用户点击复制 MathML 后，在 Word 中直接按 `Ctrl+V`，Word 会自动无缝转换成原生可编辑的数学公式（OMML），无需第三方 MathType 插件。

### 决策 4：公式可视化渲染方案
- **选择**：在线模式下通过轻量渲染服务返回高清 SVG/图片，Qt 内部使用 `QSvgRenderer` / `QPixmap` 呈现；离线模式下展示格式化结构。
- **理由**：完全规避了引入 `QtWebEngine` 带来的体积膨胀与冷启动延迟。

## Risks / Trade-offs

- **[网络波动导致云端请求超时]** → 提供 8 秒超时保护，超时后自动检测并提示降级至本地离线插件或重试。
- **[Word 版本兼容性]** → MathML 采用标准 W3C 命名空间，在 Windows Office 365 / Word 2016-2024 上均完美兼容。
