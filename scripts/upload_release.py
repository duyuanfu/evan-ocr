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

    tag_name = 'v1.2.1'

    # 1. 检查 release v1.2.1 是否已经存在
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
        "## ✨ Evan v1.2.1 发布说明\n\n"
        "### 🐛 关键修复：多显示器混合缩放截屏错位\n\n"
        "**症状**：左右分屏且两屏物理尺寸与缩放比例均不同时（如左侧 4K@150% + 右侧 1080P@100%），"
        "按 `F1` 截屏会出现「左侧屏幕向内缩进 / 左侧内容被放大 / 另一侧残留黑色区域」。\n\n"
        "**根因**\n"
        "- Qt 6 在 Per-Monitor-V2 下 `QScreen::geometry()` 返回的是**混合坐标系**"
        "（位置为物理像素、尺寸却按各屏自身 DPR 换算），直接 union 后按统一 DPR 绘制必然拉伸错位；\n"
        "- Qt 的 `setGeometry()` 走**全局逻辑坐标**，混合缩放下会被 Windows 钳制并整体偏移，"
        "导致遮罩窗口与真实显示器无法对齐。\n\n"
        "**修复方案**\n"
        "- 改用 Win32 `SetWindowPos`（对 Per-Monitor-V2 进程使用**物理像素**坐标）精确铺满真实虚拟桌面；\n"
        "- 按「物理尺寸 ÷ 窗口逻辑尺寸」反推真实渲染 DPR，在该 DPR 下以**纯物理像素 1:1** 合成画布；\n"
        "- 落位改用浮点矩形，消除屏幕交界处的 1px 接缝。\n\n"
        "**实测**：双屏（2560×1440@125% + 2560×1600@150%）抽样 14896 个像素点比对，逐像素完全对齐。\n\n"
        "---\n\n"
        "### 💾 新增：自定义保存目录 → 一键自动保存\n\n"
        "- 在 `⚙️ 设置 → 自定义保存文件路径` 中指定目录后，**截图 / 长截图 / GIF 录制 / 贴图**"
        "将直接落盘到该目录，**不再弹出保存对话框**；\n"
        "- 新增优雅的 Toast 浮层提示，实时展示完整落盘路径；\n"
        "- 同名文件自动避让（`Evan_xxx.png` → `Evan_xxx_1.png`），不会覆盖已有文件；\n"
        "- 未配置目录时维持原有「保存对话框 + 自动记忆上次位置」行为；\n"
        "- 目录不存在或不可写时自动降级回退到对话框流程。\n\n"
        "---\n\n"
        "### 📋 新增：运行日志落盘\n\n"
        "- Evan 为 GUI 程序，原先日志只进调试器、用户无法查看；\n"
        "- 日志写入 `%LOCALAPPDATA%\\Evan\\evan.log`（超过 2MB 自动截断）；\n"
        "- 启动即打印每块显示器的逻辑几何 / DPR / 物理矩形与物理虚拟桌面总矩形。\n\n"
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
        "| **`evan-v1.2.1-windows-x64.zip`** | 主程序 | 免安装便携绿色版，解压即用 (仅约 21MB) |\n"
        "| **`evan-plugin-latex-ocr.zip`** | 离线插件包 | RapidLaTeXOCR 纯本地离线公式识别引擎 (~178MB 模型资产) |\n"
    )

    if not rel_info:
        release_payload = {
            'tag_name': tag_name,
            'target_commitish': 'main',
            'name': f'{tag_name} - 修复多显示器混合缩放截屏错位 · 自定义目录一键自动保存',
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
                'name': f'{tag_name} - 修复多显示器混合缩放截屏错位 · 自定义目录一键自动保存',
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
        os.path.join('dist', 'evan-plugin-latex-ocr.zip'),
        os.path.join('dist', 'version.json')
    ]

    # 检查并清理已有同名资产，已上传成功的跳过
    existing_assets = rel_info.get('assets', [])
    for asset_path in upload_assets:
        if not os.path.exists(asset_path):
            print(f"[WARN] File not found, skip: {asset_path}")
            continue

        asset_name = os.path.basename(asset_path)
        file_size = os.path.getsize(asset_path)
        already_uploaded = False

        for a in existing_assets:
            if a['name'] == asset_name:
                if a.get('state') == 'uploaded' and a.get('size') == file_size:
                    print(f"[INFO] Asset {asset_name} is already uploaded and up to date, skipping.")
                    already_uploaded = True
                    break
                else:
                    print(f"[INFO] Deleting incomplete or outdated asset {asset_name} (ID {a['id']}) ...")
                    del_req = urllib.request.Request(a['url'], headers=headers, method='DELETE')
                    with opener.open(del_req) as del_resp:
                        print(f"[OK] Deleted old asset: {del_resp.status}")

        if already_uploaded:
            continue

        upload_url = upload_url_template.replace('{?name,label}', f'?name={asset_name}')
        file_size_mb = file_size / (1024 * 1024)
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
