import subprocess
import urllib.request
import urllib.error
import json
import os

def get_token():
    p = subprocess.Popen(['git', 'credential', 'fill'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    out, _ = p.communicate('protocol=https\nhost=github.com\n\n')
    for line in out.splitlines():
        if line.startswith('password='):
            return line[len('password='):]
    return None

def main():
    token = get_token()
    if not token:
        print("[ERROR] Failed to retrieve GitHub token from Git Credential Manager.")
        return

    proxy_handler = urllib.request.ProxyHandler({'http': 'http://127.0.0.1:7890', 'https': 'http://127.0.0.1:7890'})
    opener = urllib.request.build_opener(proxy_handler)

    headers = {
        'Authorization': f'token {token}',
        'Accept': 'application/vnd.github.v3+json',
        'User-Agent': 'Evan-Release-Uploader'
    }

    tag_name = 'v1.2.0'

    # 1. 检查 release v1.2.0 是否已经存在
    check_req = urllib.request.Request(f'https://api.github.com/repos/duyuanfu/evan-ocr/releases/tags/{tag_name}', headers=headers)
    rel_info = None
    try:
        with opener.open(check_req) as resp:
            rel_info = json.loads(resp.read().decode('utf-8'))
            print(f"[INFO] Release {tag_name} already exists (ID: {rel_info['id']})")
    except urllib.error.HTTPError as e:
        if e.code == 404:
            print(f"[INFO] Release {tag_name} not found, creating new release...")
        else:
            raise

    release_body = (
        "## ✨ Evan v1.2.0 发布说明\n\n"
        "### 🚀 新增与重磅特性\n\n"
        "1. **∑ 数学公式识别与转换 (全新上线)**\n"
        "   - 截屏浮动工具栏新增独立的 `∑` 公式识别工具按钮，框选公式一键直达；\n"
        "   - **多格式一键导出**：纯净 LaTeX 源码、块级 Markdown (`$$ ... $$`) 以及 **MathML (Word 原生公式直粘)**；\n"
        "   - **Word 直粘神级体验**：点击复制 MathML 后直接在 Word 中按 `Ctrl+V`，瞬间全自动转化为原生的 Word 可编辑矢量公式对象 (OMML)；\n"
        "   - **超高清 300 DPI 矢量渲染预览**：通过 300 DPI 高清通道彻底消除文字发虚与模糊，在 Retina 与 4K 屏幕下纤毫毕现；\n"
        "   - **双轨架构与容灾**：默认免安装模型的极速云端通道（支持自定义 SimpleTeX Token 授权），网络异常或未配置时自动无缝降级至本地离线插件。\n\n"
        "2. **全界面 UI-UX-Pro-Max 规范重构**\n"
        "   - **常规设置面板重构**：采用流畅的 `QScrollArea` 视口封装，彻底杜绝高分屏或多项目下输入框被压缩变形、高度过低错乱的问题；\n"
        "   - **卡片式风格设计**：常规偏好全面升级为 Fluent 卡片容器，统一舒适的 32px 控件高度、清晰的排版层级与微交互动效；\n"
        "   - **公式工作台极简排版**：采用精美圆角卡片、原图与矢量渲染同屏对照，配合主次分明的复制动作栏。\n\n"
        "3. **文字标注多行排版与回车规则配置**\n"
        "   - 就地文字标注升级支持多行动态排版，逐行基线 1:1 像素级精准对齐；\n"
        "   - 设置面板支持自由定制提交快捷键（`Enter 提交，Shift+Enter 换行 (默认)` 或 `Ctrl+Enter 提交，Enter 换行`）。\n\n"
        "4. **OCR 智能基金理财直达胶囊**\n"
        "   - 内置 `FundSniffer` 上下文嗅探，精准匹配公募基金代码与金融语境，底栏高光胶囊一键复制并直达实时估值小站。\n\n"
        "---\n\n"
        "### 🧩 离线插件扩展包安装指南\n\n"
        "为了让 Evan 主程序保持约 21MB 的极致轻量身材，重量级离线 AI 模型（如 RapidOCR 文字识别库、Bergamot 离线翻译、RapidLaTeXOCR 公式引擎）采用可选独立插件包分发。\n\n"
        "#### 方法一：软件内一键导入 (推荐·最简单)\n"
        "1. 下载对应的插件压缩包（如 `evan-plugin-latex-ocr.zip` 或 `evan-plugin-rapidocr-engine.zip`）；\n"
        "2. 启动 Evan，右键桌面右下角托盘图标，点击 **`🧩 插件中心...`**；\n"
        "3. 点击窗口顶部的 **`[📦 本地导入 ZIP]`**，选取刚下载的 `.zip` 文件；\n"
        "4. 系统将全自动完成解压、规范目录组织与部署校验，**无需重启软件即刻热生效**！\n\n"
        "#### 方法二：手动绿色解压部署\n"
        "1. 进入 Evan 软件所在的根目录，找到 `plugins/` 文件夹（若无则新建一个）；\n"
        "2. 将下载的压缩包直接解压到 `plugins/` 目录下；\n"
        "   - 支持解压为标准目录（如 `plugins/latex_ocr/`）；\n"
        "   - 也支持解压为带包名目录（如 `plugins/evan-plugin-latex-ocr/`）；\n"
        "3. Evan 内置多层级智能递归扫描器，会自动探测到模型文件并显示为「已安装并就绪」。\n\n"
        "---\n\n"
        "### 📦 本次发布的资产包 (Assets)\n\n"
        "| 文件名 | 类型 | 说明 |\n"
        "| :--- | :---: | :--- |\n"
        "| **`evan-v1.2.0-windows-x64.zip`** | 主程序 | 免安装便携绿色版，解压即用 (仅约 21MB) |\n"
        "| **`evan-plugin-latex-ocr.zip`** | 离线插件包 | RapidLaTeXOCR 纯本地离线公式识别引擎 (~178MB 模型资产) |\n"
    )

    if not rel_info:
        release_payload = {
            'tag_name': tag_name,
            'target_commitish': 'main',
            'name': f'{tag_name} - 数学公式识别、UI/UX全面重构优化与离线扩展生态',
            'body': release_body,
            'draft': False,
            'prerelease': False
        }
        create_req = urllib.request.Request(
            'https://api.github.com/repos/duyuanfu/evan-ocr/releases',
            data=json.dumps(release_payload).encode('utf-8'),
            headers={**headers, 'Content-Type': 'application/json'}
        )
        with opener.open(create_req) as resp:
            rel_info = json.loads(resp.read().decode('utf-8'))
            print(f"[OK] Created release: ID {rel_info['id']}, tag: {rel_info['tag_name']}")
    else:
        # 更新已存在 release 的标题与说明
        update_req = urllib.request.Request(
            f"https://api.github.com/repos/duyuanfu/evan-ocr/releases/{rel_info['id']}",
            data=json.dumps({
                'name': f'{tag_name} - 数学公式识别、UI/UX全面重构优化与离线扩展生态',
                'body': release_body
            }).encode('utf-8'),
            headers={**headers, 'Content-Type': 'application/json'},
            method='PATCH'
        )
        with opener.open(update_req) as resp:
            print(f"[OK] Updated release body for ID {rel_info['id']}")

    release_id = rel_info['id']
    upload_url_template = rel_info['upload_url']

    # 待上传资产列表
    upload_assets = [
        os.path.join('dist', f'evan-{tag_name}-windows-x64.zip'),
        os.path.join('dist', 'evan-plugin-latex-ocr.zip')
    ]

    # 检查并清理已有同名资产
    existing_assets = rel_info.get('assets', [])
    for asset_path in upload_assets:
        if not os.path.exists(asset_path):
            print(f"[WARN] File not found, skip: {asset_path}")
            continue

        asset_name = os.path.basename(asset_path)
        for a in existing_assets:
            if a['name'] == asset_name:
                print(f"[INFO] Deleting existing asset {asset_name} (ID {a['id']}) ...")
                del_req = urllib.request.Request(a['url'], headers=headers, method='DELETE')
                with opener.open(del_req) as del_resp:
                    print(f"[OK] Deleted old asset: {del_resp.status}")

        upload_url = upload_url_template.replace('{?name,label}', f'?name={asset_name}')
        file_size_mb = os.path.getsize(asset_path) / (1024 * 1024)
        print(f"[INFO] Uploading {asset_name} ({file_size_mb:.2f} MB)...")

        with open(asset_path, 'rb') as f:
            zip_bytes = f.read()

        upload_req = urllib.request.Request(
            upload_url,
            data=zip_bytes,
            headers={**headers, 'Content-Type': 'application/zip'}
        )

        with opener.open(upload_req) as resp:
            asset_info = json.loads(resp.read().decode('utf-8'))
            print(f"[SUCCESS] Asset uploaded successfully!")
            print(f"  Name: {asset_info['name']}")
            print(f"  Size: {asset_info['size'] / (1024 * 1024):.2f} MB")
            print(f"  Download URL: {asset_info['browser_download_url']}")

if __name__ == '__main__':
    main()
