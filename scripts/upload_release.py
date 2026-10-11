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

    # 1. 检查 release v1.1.2 是否已经存在
    check_req = urllib.request.Request('https://api.github.com/repos/duyuanfu/evan-ocr/releases/tags/v1.1.2', headers=headers)
    rel_info = None
    try:
        with opener.open(check_req) as resp:
            rel_info = json.loads(resp.read().decode('utf-8'))
            print(f"[INFO] Release v1.1.2 already exists (ID: {rel_info['id']})")
    except urllib.error.HTTPError as e:
        if e.code == 404:
            print("[INFO] Release v1.1.2 not found, creating new release...")
        else:
            raise

    if not rel_info:
        release_payload = {
            'tag_name': 'v1.1.2',
            'target_commitish': 'main',
            'name': 'v1.1.2 - 开机自启、4K高DPI抗锯齿优化与插件递归识别增强',
            'body': (
                "## ✨ Evan v1.1.2 发布说明\n\n"
                "### 🚀 新增与优化内容\n\n"
                "1. **系统开机自启动配置**\n"
                "   - 新增 `AutoStartHelper`，接入 Windows 用户级 `Run` 注册表（`HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run`）。\n"
                "   - 首次启动默认开启开机自启，静默常驻系统托盘，毫秒级响应全局截屏（`F1`）与贴图（`F3`）热键。\n"
                "   - 在「设置」-「常规与偏好」中新增可视化自启动开关，支持状态实时同步与一键重置。\n\n"
                "2. **4K / 高 DPI 屏幕全真抗锯齿字体与渲染调优**\n"
                "   - 全局注入 `Microsoft YaHei UI` 次像素抗锯齿字体策略（`QFont::PreferAntialias | QFont::PreferQuality`）。\n"
                "   - 优化浮动工具栏与符号按钮字体渲染，彻底消除 4K 分辨率（150%~250% 缩放）下文字毛刺与图标边缘锯齿感。\n\n"
                "3. **插件多层级嵌套与手动解压兼容增强**\n"
                "   - 在 `RapidOcrEngine`、`OfflinePluginTranslator` 与 `PluginManager` 引入 `QDirIterator` 递归目录扫描机制。\n"
                "   - 彻底兼容用户使用各种压缩工具手动解压 ZIP 产生的带包名文件夹（如 `evan-plugin-rapidocr-engine`、`evan-plugin-translation-en-zh`）及多层嵌套目录。\n\n"
                "4. **滚轮长截图算法稳健化**\n"
                "   - 强化物理像素 1:1 绘制与边界对齐，确保暗黑主题长网页/文档拼接无缝平滑。\n\n"
                "---\n\n"
                "### 📦 资产下载与说明\n"
                "- **便携免安装包**：`evan-v1.1.2-windows-x64.zip`（仅 21.48 MB）\n"
                "- 解压后双击 `evan.exe` 即可运行，轻量免安装，开箱即用。\n"
            ),
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

    release_id = rel_info['id']
    upload_url_template = rel_info['upload_url']

    # 2. 检查并清理已有同名资产 (如重新上传)
    assets = rel_info.get('assets', [])
    asset_name = 'evan-v1.1.2-windows-x64.zip'
    for a in assets:
        if a['name'] == asset_name:
            print(f"[INFO] Deleting existing asset ID {a['id']} ...")
            del_req = urllib.request.Request(a['url'], headers=headers, method='DELETE')
            with opener.open(del_req) as del_resp:
                print(f"[OK] Deleted old asset: {del_resp.status}")

    # 3. 上传新资产
    zip_path = os.path.join('dist', asset_name)
    if not os.path.exists(zip_path):
        print(f"[ERROR] Asset file not found: {zip_path}")
        return

    upload_url = upload_url_template.replace('{?name,label}', f'?name={asset_name}')
    file_size_mb = os.path.getsize(zip_path) / (1024 * 1024)
    print(f"[INFO] Uploading {asset_name} ({file_size_mb:.2f} MB)...")

    with open(zip_path, 'rb') as f:
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
