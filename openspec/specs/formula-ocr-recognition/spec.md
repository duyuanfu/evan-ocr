# formula-ocr-recognition Specification

## Purpose
TBD - created by archiving change formula-ocr-recognition. Update Purpose after archive.
## Requirements
### Requirement: 独立公式识别工具入口
截屏浮动工具栏 SHALL 提供独立的数学公式识别按钮（`∑`），支持快捷键触发并唤起框选公式识别流程。

#### Scenario: 用户点击工具栏 ∑ 按钮
- **WHEN** 用户框选屏幕选区后点击浮动工具栏上的 `∑` 按钮
- **THEN** 系统捕获选区高精度图像，并唤起公式识别结果窗口启动识别

#### Scenario: 快捷键唤醒公式识别
- **WHEN** 用户在选区有效时按下公式识别快捷键
- **THEN** 系统立即触发公式识别并展示公式工作台

### Requirement: 双轨公式识别调度引擎
系统 SHALL 提供双轨公式识别架构，默认采用免安装模型的高速在线通道，并在网络断开或用户指定时无缝降级至本地 RapidLaTeXOCR 离线插件。

#### Scenario: 在线云端通道识别
- **WHEN** 客户端网络正常且处于默认在线模式
- **THEN** 系统调用云端公式识别通道，在 100~300ms 内提取精确 LaTeX 源码

#### Scenario: 离线自动降级
- **WHEN** 客户端离线或云端请求超时，且本地已安装 `latex_ocr` 插件
- **THEN** 系统自动平滑切换至本地 RapidLaTeXOCR 离线引擎完成识别，并在界面提示已降级至本地模式

### Requirement: 多格式公式一键导出
系统 SHALL 支持将识别出的 LaTeX 公式无缝导出为 LaTeX 源码、块级 Markdown 语法以及适用于 Microsoft Word 原生公式的 MathML 标准格式。

#### Scenario: 复制 LaTeX
- **WHEN** 用户点击 `[复制 LaTeX]` 按钮
- **THEN** 系统将纯净的 LaTeX 源码复制到剪贴板，并显示已复制提示

#### Scenario: 复制 Markdown
- **WHEN** 用户点击 `[复制 $$ Markdown]` 按钮
- **THEN** 系统将包裹了 `$$` 的块级公式代码复制到剪贴板

#### Scenario: 复制 MathML 用于 Word 直粘
- **WHEN** 用户点击 `[复制 MathML (Word直粘)]` 按钮
- **THEN** 系统将包含 `<math xmlns="http://www.w3.org/1998/Math/MathML">` 的 XML 结构写入剪贴板，用户在 Word 中按 `Ctrl+V` 可直接转换为原生矢量公式

### Requirement: 公式结果工作台交互与预览
系统 SHALL 提供专属公式结果对比对话框，包含截图原图与矢量公式实时渲染预览比对、可编辑的 LaTeX 代码编辑区以及通道切换控制。

#### Scenario: 公式预览与代码联动
- **WHEN** 公式识别完成或用户在编辑区修改 LaTeX 源码
- **THEN** 预览区域同步刷新渲染效果，方便用户校对和修正符号偏差

