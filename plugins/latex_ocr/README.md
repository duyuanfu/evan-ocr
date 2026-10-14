# RapidLaTeXOCR 离线数学公式识别扩展包

本目录为 **Evan** 的纯本地离线数学公式识别插件包（RapidLaTeXOCR）。

## 目录结构
```text
plugins/latex_ocr/
├── config.json              # 插件元信息描述文件
├── README.md                # 规范与说明文件
├── rapid_latex_ocr.exe      # 离线推理主程序
├── runner.py                # Python 推理执行器
└── models/                  # ONNX 神经网络模型文件
    ├── image_resizer.onnx   # 图像预处理与尺寸自适应模型 (~37MB)
    ├── encoder.onnx         # 视觉特征编码模型 (~85MB)
    ├── decoder.onnx         # LaTeX 序列自回归解码模型 (~49MB)
    └── tokenizer.json       # 词表映射文件
```

## 功能特点
- 100% 本地运算，0 联网请求，隐私绝对安全；
- 支持识别各种二维复杂数学公式、矩阵、积分、上下标并输出标准 LaTeX 与 MathML 代码；
- Evan 主程序在无网或未配置云端 Token 时会自动无缝降级至本插件。
