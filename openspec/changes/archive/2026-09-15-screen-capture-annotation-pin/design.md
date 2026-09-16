## Context

本项目旨在构建一款轻量级、跨平台（首发 Windows，架构支持 macOS）、毫秒级响应的现代化截贴图与生产力工具。针对日常中文/英文办公场景，第一阶段聚焦打磨“极速截屏 + 算法智能选框 + 实时放大取色 + 矢量标注 + 独立多实例贴图”的核心体验，并为二期引入双引擎 OCR（Windows.Media.Ocr 与 ONNX RapidOCR）及抽屉式文本面板提供清晰稳定的架构基石。

## Goals / Non-Goals

**Goals:**
- 基于 CMake + C++20 + Qt 6 搭建清晰的分层模块体系（`core`、`ui`、`platform`）。
- 实现多物理显示器与异构 DPI 统一几何坐标系下的毫秒级屏幕快照捕获。
- 采用 OpenCV 轮廓分析算法（下采样金字塔 + Canny + findContours `RETR_TREE`）实现无需 Win32 句柄的图像级智能吸附，支持 Tab 键父子层级跳转。
- 实现单像素网格放大镜，提供物理坐标与 HEX/RGB 色值显示及一键复制（C 键）。
- 实现自适应吸附浮动工具栏，包含针对屏幕边缘溢出的上下翻转与全屏内嵌避让算法。
- 构建基于 Qt `QUndoStack` 的 Command 模式非破坏性矢量标注系统（矩形、椭圆、箭头、平滑贝塞尔画笔、文本、马赛克），提供完备的撤销与重做支持。
- 实现独立置顶的无边框轻量贴图窗口（PinWindow），支持鼠标拖拽平移、滚轮无极平滑缩放、Ctrl+滚轮透明度无级调节及多实例共存。

**Non-Goals:**
- 第一阶段不直接引入并运行 OCR 模型推理或文本识别服务（预留数据接口，由二期专项实现）。
- 第一阶段暂不处理视频录屏或动态 GIF 录制功能。
- 第一阶段暂不引入复杂的云端图片同步或云端图床上传。

## Decisions

### 1. 跨平台 GUI 框架选型：C++20 + Qt 6
- **Rationale**: 截贴图工具对常驻内存（目标 <40MB）、冷启动唤醒速度（<100ms）及多屏不同 DPI 坐标转换极为敏感。Qt 6 具备优秀的跨平台渲染管线与高 DPI 支持，后续可无缝移植至 macOS。
- **Alternatives Considered**: 
  - *Tauri 2.0 (Rust + Web)*: 前端 DOM 在复杂多屏穿透、极速局部重绘与低内存常驻上调优成本高于原生 Qt。
  - *C# WPF / WinUI 3*: 无法原生跨平台至 macOS。

### 2. 智能选框实现策略：OpenCV RETR_TREE 图像轮廓树
- **Rationale**: 现代应用广泛采用自绘机制（如 Chrome、Electron、WPF、游戏界面），基于 Win32 `WindowFromPoint` 无法探测到无句柄的子控件。采用图像处理在后台下采样后提取闭合几何轮廓，构建 R-Tree/嵌套树结构，能普遍适用于任何渲染窗口。
- **Alternatives Considered**:
  - *Win32 UI Automation*: 依赖应用程序支持 Accessibility，部分无障碍支持差的应用会产生明显的卡顿与探测失败。

### 3. 矢量标注与历史记录：Command 模式 + QUndoStack
- **Rationale**: 用户的标注过程绝不能破坏原始快照的高保真像素。所有图元继承自定义图元抽象类，图形的所有属性修改、位移、增删全部封装为 `QUndoCommand`，由 `QUndoStack` 统一管理。
- **Alternatives Considered**:
  - *直接在 QPixmap 上进行 QPainter 光栅化绘制*: 内存消耗低但无法实现对象级二次编辑，且撤销需要保存多份位图副本，极度浪费内存。

### 4. 工具栏动态自适应布局算法
- **Rationale**: 工具栏紧贴选区下方，但在贴近屏幕底部、顶部或全屏截图等边界极端情况下，必须自动计算屏幕可视区域（Available Geometry），提供「下方吸附 -> 翻转至上方 -> 内嵌至选区右下角」的三级退避策略。

## Risks / Trade-offs

- **[Risk] OpenCV 轮廓计算耗时导致截屏卡顿** → **Mitigation**: 截屏后仅在工作线程对下采样（1/2 或 1/4）的灰度图进行边缘提取；鼠标移动时只做基于边界框包含关系的二分/几何查询（微秒级响应）。
- **[Risk] 多显示器不同 DPI 下窗口缩放与坐标漂移** → **Mitigation**: 统一以物理像素作为快照基准，在 Qt 窗口层统一维护 DPI 转换因子，所有选区几何计算与贴图渲染严格基于物理坐标解耦。
- **[Risk] 贴图窗口在高频滚轮缩放时的抗锯齿与重绘开销** → **Mitigation**: 缓存原始高保真位图，缩放时使用双线性平滑插值（SmoothPixmapTransform），仅在窗口大小确定后执行局部刷新。
