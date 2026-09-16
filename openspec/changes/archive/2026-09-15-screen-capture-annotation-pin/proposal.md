## Why

构建一款基于 C++20 与 Qt 6 的现代化、轻量级、跨平台截贴图桌面生产力工具。解决日常办公与研发场景下高频的屏幕内容捕获、精细化标注与信息对比痛点，并以极低的常驻内存与毫秒级唤醒响应，为后续双引擎 OCR（Windows.Media.Ocr / ONNX RapidOCR）及 macOS 跨平台扩展奠定坚实的系统架构基础。

## What Changes

- **CMake 模块化工程体系**：构建解耦的 `core`（核心业务与算法）、`ui`（Qt 6 视图与图元）、`platform`（系统原生 API 封装）架构。
- **全局热键与多屏捕获**：支持自定义全局唤醒热键，基于虚拟桌面几何合并与高 DPI 感知，实现跨多显示器无畸变瞬时屏幕快照。
- **像素放大镜与拾色器**：截屏唤醒后提供鼠标跟随的单像素网格放大镜，显示物理坐标与 HEX/RGB 颜色值，并支持一键复制。
- **图像轮廓智能选框 (Smart Snapping)**：引入轻量级 OpenCV 轮廓检测（Canny + findContours RETR_TREE），实现免 Win32 句柄依赖的色块/元素智能吸附，并支持 Tab 键父子层级跳转。
- **自适应吸附工具栏**：实现紧贴选区下方的快捷工具栏，具备屏幕底边翻转与极端空间内嵌的智能避让算法。
- **非破坏性矢量标注系统**：基于 Qt Graphics 与 `QUndoStack` 实现 Command 模式标注，支持矩形、箭头、平滑贝塞尔画笔、文本及马赛克，全过程支持无限撤销与重做。
- **独立置顶贴图系统 (Pin Window)**：支持将截取并标注后的画面一键转为独立置顶浮动窗口，支持无极平滑缩放、透明度动态调节及多窗口共存。

## Capabilities

### New Capabilities
- `screen-capture`: 负责多显示器全景快照、DPI 缩放映射、全屏半透明遮罩与像素级放大镜/拾色器。
- `smart-snapping`: 负责截图底图的图像边缘轮廓提取、四叉树/树形检索，以及鼠标悬停吸附与 Tab 键层级切换。
- `vector-annotation`: 负责选区内的矢量图元创建、编辑、样式修改与基于 Command 模式的 Undo/Redo 状态流转。
- `image-pinning`: 负责管理独立贴图窗口的生命周期、置顶渲染、鼠标拖拽、滚轮缩放与透明度调节。

### Modified Capabilities
<!-- None. Initial change set. -->

## Impact

- **构建系统**：引入 CMake 3.20+、C++20 编译标准。
- **外部依赖**：引入 Qt 6 (Core, Gui, Widgets)、精简版 OpenCV (core, imgproc)、QHotkey 跨平台全局热键库。
- **运行环境**：优先运行于 Windows 10/11 64位环境，保留与 macOS 架构兼容的平台抽象层接口。
