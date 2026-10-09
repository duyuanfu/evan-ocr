# Evan 离线翻译插件规范与部署指南

为保持 **Evan** 主程序极致轻量（压缩包仅约 20MB 出头），离线神经机器翻译（NMT）运行引擎与各语种模型权重采用**独立扩展插件包**形式提供。

当本目录未放置离线模型时，Evan 主程序自动启用免费、免配置的在线直连备用引擎；当本目录安装离线插件后，Evan 将自动点亮【⚡ 离线插件就绪】，进入纯本地离线翻译模式（0网络请求，100% 本地隐私安全）。

---

## 一、 目录组织结构规范

离线插件扩展包解压后应置于 `plugins/translation/` 目录下，标准结构如下：

```text
plugins/translation/
├── README.md                      # 本规范说明文件
├── config.json                    # 插件元信息与支持语言声明 (可选)
├── translator-engine.exe          # 离线推理主程序 (或 offline-translator.exe / ctranslate2-runner.exe)
└── models/                        # 离线语言模型权重目录
    ├── opus-mt-en-zh/             # 英语 ➔ 中文翻译模型
    ├── opus-mt-zh-en/             # 中文 ➔ 英语翻译模型
    ├── opus-mt-ja-zh/             # 日语 ➔ 中文翻译模型
    └── opus-mt-ko-zh/             # 韩语 ➔ 中文翻译模型
```

---

## 二、 进程 IPC 通信协议规范

Evan 主程序采用**进程隔离 IPC** 方式驱动离线翻译引擎，确保 100% 内存安全，且第三方引擎崩溃不会牵连主程序。

### 1. 命令行参数契约
主程序调用 `translator-engine.exe` 时会传入以下标准参数：

```bash
translator-engine.exe --from <源语言代码> --to <目标语言代码> --input <输入文件路径> --output <输出文件路径> [--models <模型目录路径>]
```

- `--from`：源语言标识符（如 `auto`, `en`, `zh`, `ja`, `ko`, `ru`, `fr`, `de`）；
- `--to`：目标语言标识符（如 `zh`, `en`, `ja`, `ko`）；
- `--input`：存放待翻译 UTF-8 文本的临时文件绝对路径；
- `--output`：要求引擎将翻译完成的 UTF-8 文本写入的目标文件绝对路径；
- `--models`：模型根目录绝对路径（可选）。

### 2. 标准输出降级机制
若引擎未生成 `--output` 文件，Evan 会自动捕获引擎标准输出（stdout），支持直接解析纯文本或 JSON 响应：
```json
{
  "code": 200,
  "from": "en",
  "to": "zh",
  "result": "这里是翻译后的目标语言文本"
}
```

---

## 三、 `config.json` 描述文件示例

```json
{
  "name": "Evan Offline NMT Plugin",
  "version": "1.0.0",
  "engine": "translator-engine.exe",
  "supported_languages": [
    { "from": "auto", "to": "zh", "name": "自动检测 ➔ 中文 (离线)" },
    { "from": "en", "to": "zh", "name": "英语 ➔ 中文 (离线)" },
    { "from": "zh", "to": "en", "name": "中文 ➔ 英语 (离线)" },
    { "from": "ja", "to": "zh", "name": "日语 ➔ 中文 (离线)" },
    { "from": "ko", "to": "zh", "name": "韩语 ➔ 中文 (离线)" }
  ]
}
```

---

## 四、 用户安装与体验方法

1. 从官方 Releases 页面下载对应的语种扩展包（例如 `evan-plugin-translation-en-zh.zip`）；
2. 解压到程序所在目录的 `plugins/translation/` 文件夹下；
3. 打开 Evan 截图识别后，在结果弹窗中即可直接享受纯本地、无依赖的毫秒级离线翻译体验！
