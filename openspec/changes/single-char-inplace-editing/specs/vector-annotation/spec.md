## ADDED Requirements

### Requirement: 单字符就地编辑矢量图元
系统 SHALL 提供 `SingleCharEditAnnotation` 矢量标注图元，继承自 `AnnotationItem`。该图元内部封装单字符区域的微创背景修复图与新输入的单字符文本对象，在图元重绘时先绘制修复底色再同轴绘制排版文字，并完整接入 `AnnotationManager` 的 `QUndoStack` 撤销/重做栈。

#### Scenario: 提交单字符修改并撤销
- **WHEN** 用户完成一个单字符的文字替换并确认提交
- **THEN** 系统生成 `SingleCharEditAnnotation` 图元并推入撤销栈，屏幕实时呈现新字无痕覆盖效果；用户按下 Ctrl+Z 后，该单字符修改完全撤销并恢复原始字样
