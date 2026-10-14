## 1. 核心引擎与多格式转换层

- [x] 1.1 创建 `src/core/formula/formula_format_helper.h`，实现 LaTeX 源码转标准 MathML XML 以及 Markdown 块级格式转换
- [x] 1.2 创建 `src/core/formula/formula_ocr_manager.h` 与 `formula_ocr_manager.cpp`，实现云端高速识别与本地离线插件双轨调度
- [x] 1.3 在 `src/core/plugin/plugin_manager.cpp` 中增加 `latex-ocr-engine` 离线扩展包元数据与递归目录扫描支持

## 2. 交互界面与工作台

- [x] 2.1 在 `src/ui/toolbar/floating_toolbar.h` 与 `floating_toolbar.cpp` 中新增 `ToolAction::FormulaOcr` 与 `∑` (公式识别) 按钮
- [x] 2.2 创建 `src/ui/formula/formula_result_dialog.h` 与 `formula_result_dialog.cpp`，实现原图对比、LaTeX源码微调编辑、公式渲染预览与多格式复制
- [x] 2.3 在 `src/ui/overlay/snipping_overlay.h` 与 `snipping_overlay.cpp` 中接入公式识别动作与截图捕获联动

## 3. 编译验证与打包

- [x] 3.1 使用 MSVC 2022 + Ninja 进行 Release 增量编译，确保零 warning、零 error
- [x] 3.2 运行 `python scripts/package_release.py` 验证发布包与动态链接依赖完整性
