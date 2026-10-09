## 1. 翻译架构契约与数据结构定义

- [x] 1.1 创建 `src/core/translation/translation_types.h`，定义 `TranslationResult`、`LanguagePair`、`TranslationEngineType` 等核心实体
- [x] 1.2 创建 `src/core/translation/translator_interface.h`，定义通用的 `ITranslator` 异步纯虚抽象基类

## 2. 插件管理与双轨引擎驱动实现

- [x] 2.1 创建 `src/core/translation/translation_plugin_manager.h` 与 `translation_plugin_manager.cpp`，实现对 `plugins/translation/` 独立目录的扫描、组件探测与动态加载驱动
- [x] 2.2 创建 `src/core/translation/online_fallback_translator.h` 与 `online_fallback_translator.cpp`，实现无需下载模型的免费轻量在线翻译驱动，确保零配置开箱即用
- [x] 2.3 创建 `src/core/translation/offline_plugin_translator.h` 与 `offline_plugin_translator.cpp`，实现基于进程隔离 IPC 通信的纯本地离线翻译插件驱动

## 3. UI 交互层集成 (OCR 对照窗口升级)

- [x] 3.1 改造 `src/ui/ocr/ocr_result_dialog.h` 与 `ocr_result_dialog.cpp`，布局新增现代极简、可折叠的「🌐 译文对照」面板
- [x] 3.2 实现源语言/目标语言下拉选择器、语种智能嗅探以及与「¶ 合并段落」「␣ 消除空格」排版过滤器的连贯输入联动
- [x] 3.3 实现「📋 复制译文」独立一键复制与微动效反馈，增加插件就绪状态图标与离线包下载指引

## 4. 构建集成、插件规范与端到端验证

- [x] 4.1 更新 `CMakeLists.txt`，确保网络与核心组件依赖配置就绪并通过 MSVC/Ninja 全量编译
- [x] 4.2 创建 `plugins/translation/README.md`，制定离线翻译插件包的文件结构、通信协议、模型说明与打包分发规范
- [x] 4.3 端到端功能测试验证：OCR 提取后一键翻译、离线插件探测、语种切换、一键复制及异常兜底测试
