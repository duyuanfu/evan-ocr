# vector-annotation Specification

## Purpose
TBD - created by archiving change screen-capture-annotation-pin. Update Purpose after archive.
## Requirements
### Requirement: 选区内矢量图元绘制与交互
系统 SHALL 在选区固定后提供交互式矢量标注画布，支持矩形、椭圆、箭头、平滑贝塞尔画笔、文本及局部马赛克等标注工具，所有图元作为独立图形对象存在，且均支持二次选中、拖拽位移与尺寸拉伸。

#### Scenario: 绘制并微调带箭头的标注线
- **WHEN** 用户选择工具栏中的箭头工具并在选区内拖拽鼠标
- **THEN** 系统生成平滑矢量箭头并在松开鼠标后显示端点控制手柄供后续微调

### Requirement: 基于命令模式的无限撤销与重做
系统 SHALL 基于 Qt `QUndoStack` 实现全面的 Command 模式，记录所有的图元新增、修改、属性变更（颜色/粗细/字号）及删除动作，支持用户随时通过快捷键（Ctrl+Z / Ctrl+Y）进行撤销与重做。

#### Scenario: 连续撤销图元绘制
- **WHEN** 用户绘制了多个方框和箭头后连续按下 Ctrl+Z
- **THEN** 界面上的标注图元按后进先出的严格顺序逐个安全移除，原始截图底图不受任何破坏

### Requirement: 吸附工具栏与动态碰撞避让
系统 SHALL 提供紧贴选区下方的吸附式浮动工具栏，并具备针对屏幕边界与紧凑空间的自适应避让能力。

#### Scenario: 选区靠近屏幕底部边沿
- **WHEN** 用户框选的区域下边缘与屏幕底边界距离小于工具栏高度
- **THEN** 工具栏自动翻转至选区上方显示，避免溢出屏幕可视区域

#### Scenario: 选区充满全屏可视空间
- **WHEN** 选区上下左右均无容纳工具栏的外部边距
- **THEN** 工具栏自动内嵌显示在选区内部右下角并保持半透明悬浮

