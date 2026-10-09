## ADDED Requirements

### Requirement: 可插拔翻译引擎抽象接口
系统 SHALL 定义通用的翻译抽象基类 `ITranslator` 与核心数据结构（`TranslationResult`、`LanguagePair`、`TranslationEngineType`），实现翻译业务逻辑与具体翻译实现（本地离线模型 / 在线引擎）的完全解耦。

#### Scenario: 异步翻译请求派发
- **WHEN** 客户端向 `ITranslator::translateAsync(const QString& text, const QString& srcLang, const QString& targetLang, Callback)` 提交待翻译文本
- **THEN** 系统在后台工作线程异步执行翻译，并通过回调返回结构化结果 `TranslationResult`（包含原文本、译文、源语言、目标语言及耗时）。

### Requirement: 离线翻译插件动态探测与加载
系统 SHALL 提供 `TranslationPluginManager`，负责扫描程序目录下的 `plugins/translation/` 独立扩展目录，探测并动态载入已安装的离线翻译组件或模型包，完全无需在主程序中强依赖或静态捆绑重型模型。

#### Scenario: 未安装离线翻译插件时的优雅降级
- **WHEN** 用户启动程序且 `plugins/translation/` 目录为空或未配置离线翻译模型
- **THEN** 系统报告离线翻译插件未就绪状态，主程序保持 0 额外内存开销，并支持降级为直连轻量免费在线翻译或提示用户按需下载模型包。

#### Scenario: 检测到离线模型包自动就绪
- **WHEN** 用户将翻译模型扩展包解压至 `plugins/translation/` 目录并刷新
- **THEN** 系统自动扫描并识别支持的语言对（如中英、日英、韩英互译），并激活离线神经网络翻译驱动。
