## 1. 字符级分割与属性推断算法

- [x] 1.1 创建 `src/core/char_edit/char_segmentation.h` 与 `char_segmentation.cpp`，实现基于 OCR 行文本与垂直投影波谷探测的单字符高精切分算法
- [x] 1.2 创建 `src/core/char_edit/char_inpainter.h` 与 `char_inpainter.cpp`，实现单字符微创局部背景感知修复（纯色平涂/双线性插值）
- [x] 1.3 创建 `src/core/char_edit/char_font_analyzer.h` 与 `char_font_analyzer.cpp`，实现单字油墨前景颜色、背景色、字号与字体流派（宋体/黑体/等宽）逆向推测

## 2. 单字符就地编辑矢量图元与撤销管理

- [x] 2.1 创建 `SingleCharEditAnnotation` 图元（继承 `AnnotationItem`），封装微创背景修复图与新单字符排版渲染对象
- [x] 2.2 在 `AnnotationManager` 中注册该图元并打通 `QUndoStack` 撤销/重做支持

## 3. 原位单字编辑气泡与快速流转交互

- [x] 3.1 创建 `src/ui/char_edit/single_char_editor.h` 与 `single_char_editor.cpp`，实现微型单字输入框与悬浮微调栏（流派切换/粗细/取色）
- [x] 3.2 支持 `Tab` / `Shift+Tab` 在行内相邻字符间极速流转编辑，`Enter` 即刻提交，`Esc` 取消
- [x] 3.3 在 `SnippingOverlay` 中集成改字模式：鼠标悬停显示字符级磁吸微光框，单击直接就地唤醒编辑气泡

## 4. 工具栏集成与端到端验证

- [x] 4.1 在 `FloatingToolbar` 中新增“改字/P图”工具按钮（`ToolAction::CharEdit`）与快捷键联动
- [x] 4.2 整体端到端功能联调与构建验证：单字修改、数字篡改、Tab 连续流转、Ctrl+Z 撤销及高 DPI 多屏贴图验证
