## 1. 核心图像修复与字体属性逆向推断算法

- [x] 1.1 创建 `src/core/inpainting/image_inpainter.h` 与 `image_inpainter.cpp`，实现基于边缘多点采样与双线性渐变的内容感知背景修复算法
- [x] 1.2 创建 `src/core/inpainting/font_attribute_estimator.h` 与 `font_attribute_estimator.cpp`，实现从包围盒像素中自动分离背景色与前景色，估算真实字体颜色与字号

## 2. 原地修改矢量图元与撤销管理

- [x] 2.1 创建 `InplaceTextEditAnnotation` 标注图元（继承 `AnnotationItem`），将修复底图和文字图元封装为整体
- [x] 2.2 在 `AnnotationManager` 中注册该图元并打通 `QUndoStack` 撤销/重做支持

## 3. 原地交互编辑器与双击唤醒流转

- [x] 3.1 改造 `InPlaceTextEditor`，支持传入初始填充文本、自动设置估算的背景修复底图与字体属性
- [x] 3.2 在 `SnippingOverlay` 中集成点击或框选文字区域时的即时背景擦除与编辑器唤醒流转
- [x] 3.3 在 `OcrResultDialog` 左侧原图预览区域中，支持双击任意文字识别框直接触发原地编辑修改

## 4. 工具栏入口与系统验证

- [x] 4.1 在 `FloatingToolbar` 中增加文字就地修改/P图工具按钮（`ToolAction::InplaceEdit`）与快捷键
- [x] 4.2 整体端到端功能联调：单行数字修改、中英混排修改、历史撤销、高 DPI 多屏贴图与导出验证
