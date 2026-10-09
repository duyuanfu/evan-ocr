## Why

用户在完成屏幕文字识别（OCR）后，极高频的下一级诉求是将提取出的外文（如英文、日文、韩文或专业术语）即时翻译为中文，或将中文译为外文。
然而，离线神经网络翻译模型（如 MarianMT / OPUS-MT / CTranslate2 / Bergamot 等）单语种模型权重通常达到数十至数百兆。若将其硬编码捆绑在主程序中，会导致 Evan 的安装包体积从原本的 20MB 暴增至上百兆，破坏“极致轻量、零依赖秒启”的核心优势。
通过引入**轻量可插拔的插件化架构（Modular Translation Plugin Architecture）**，Evan 主程序仅提供翻译抽象接口与 UI 对照视口；用户可按需选择安装“离线翻译扩展插件包”或接入轻量在线翻译，兼顾极致轻量与强大生产力扩展性。

## What Changes

- **翻译引擎抽象接口 (`ITranslator`)**：定义统一的异步文本翻译契约、语种定义（`LanguagePair`）、置信度与状态结构。
- **可插拔插件动态探测与管理器 (`TranslationPluginManager`)**：
  - 扫描本地 `plugins/translation/` 独立目录，支持免重启热检测；
  - 若用户未安装插件：UI 保持精简，提供一键引导与可选在线直连；
  - 若检测到离线模型包：自动点亮“离线翻译”加速引擎，常驻按需加载，不占用多余内存。
- **OCR 结果弹窗支持「译文对照分栏」(`OcrResultDialog` 升级)**：
  - 在现有的「原图对照 + 提取文字」基础上，新增「🌐 翻译」折叠分栏/卡片；
  - 支持源语言与目标语言一键切换、一键复制译文、译文排版段落同步；
  - 保持现代极简明亮风格，无缝融入现有左右对照交互。

## Capabilities

### New Capabilities
- `translation-engine`: 提供可插拔翻译基类契约、动态插件扫描与加载驱动，支持单行/段落级离线神经机器翻译。
- `translation-interaction`: 在 OCR 结果对比弹窗及相关界面中呈现源文/译文对照、语言选择、段落对齐与一键复制交互。

### Modified Capabilities
<!-- None. Additive capability without altering existing capture or annotation spec requirements. -->

## Impact

- **架构与体积**：主程序二进制保持纯净轻量（零体积膨胀）；翻译模型与运行时独立置于 `plugins/` 目录隔离分发。
- **UI 界面**：`OcrResultDialog` 新增翻译对照视口与语言下拉选择器。
- **平台兼容**：保持纯 C++/Qt 原生架构，支持 Windows 10/11 x64。
