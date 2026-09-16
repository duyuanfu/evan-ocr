# image-pinning Specification

## Purpose
TBD - created by archiving change screen-capture-annotation-pin. Update Purpose after archive.
## Requirements
### Requirement: 选区独立置顶贴图化
系统 SHALL 支持用户通过快捷键（如 F3）或点击工具栏“贴图”按钮，将当前选区（含其所有矢量标注图元）合成为高保真位图，并瞬间生成独立的置顶无边框浮动窗口（Pin Window）。

#### Scenario: 触发选区贴图
- **WHEN** 用户完成截图和标注后点击“贴图”按钮
- **THEN** 全屏截图遮罩立即退出，在原选区物理坐标处无缝呈现一个无边框、始终置顶且任务栏不显示独立图标的贴图窗口

### Requirement: 贴图多点交互与手势控制
贴图窗口 SHALL 支持鼠标左键拖拽平移、鼠标滚轮等比例平滑缩放、以及 Ctrl + 滚轮动态调节窗口半透明度（透明度范围 10% ~ 100%）。

#### Scenario: 滚轮缩放贴图
- **WHEN** 鼠标光标位于贴图窗口上方并滚动滚轮
- **THEN** 贴图窗口以鼠标所在点为锚点进行等比例平滑高质量插值放大或缩小

#### Scenario: 调节贴图透明度
- **WHEN** 用户按住 Ctrl 键并滚动鼠标滚轮
- **THEN** 贴图窗口的整体不透明度随滚轮滚动平滑增减

### Requirement: 贴图窗口多实例生命周期管理
系统 SHALL 维护一个轻量级贴图管理器，允许屏幕上同时并存多个贴图窗口实例，并提供右键快捷菜单支持复制到剪贴板、另存为文件、置顶锁定切换及一键关闭。

#### Scenario: 双击或按 Esc 销毁贴图
- **WHEN** 用户激活某个贴图窗口并按下 Esc 键或双击贴图
- **THEN** 该贴图窗口安全淡出销毁并释放其占用的图形内存资源

