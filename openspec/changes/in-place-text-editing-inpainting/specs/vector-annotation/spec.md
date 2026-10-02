## ADDED Requirements

### Requirement: 原地文字擦除替换图元与命令撤销管理
系统 SHALL 将“背景擦除修补底图”与“覆盖其上的新排版文字”统一封装为独立标注图元 `InplaceTextEditAnnotation`，并完全纳入 `QUndoStack` 撤销/重做栈管理。

#### Scenario: 撤销原地修改文字操作
- **WHEN** 用户通过就地编辑修改了画面中的文字后按下 Ctrl+Z 快捷键
- **THEN** 系统撤销该擦除与文本图元，画面完整恢复为原始截图底图文字状态
