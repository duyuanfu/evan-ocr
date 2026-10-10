import urllib.request
import json
import os
import mimetypes
import subprocess

def get_token():
    token = os.environ.get('GITHUB_TOKEN')
    if token:
        return token
    try:
        p = subprocess.run(['git', 'credential', 'fill'], input='protocol=https\nhost=github.com\n', capture_output=True, text=True)
        for line in p.stdout.splitlines():
            if line.startswith('password='):
                return line.split('=', 1)[1].strip()
    except Exception:
        pass
    return ''

token = get_token()
repo = 'duyuanfu/evan-ocr'

headers_api = {
    'Authorization': f'token {token}',
    'Accept': 'application/vnd.github.v3+json',
    'User-Agent': 'Evan-Uploader'
}

# 1. 查找是否存在 tag 为 plugins 的 Release，不存在则创建
get_rel_url = f'https://api.github.com/repos/{repo}/releases'
req = urllib.request.Request(get_rel_url, headers=headers_api)
releases = []
try:
    with urllib.request.urlopen(req) as resp:
        releases = json.loads(resp.read().decode('utf-8'))
except Exception as e:
    print('Failed to list releases:', e)

plugins_release = None
for r in releases:
    if r.get('tag_name') == 'plugins':
        plugins_release = r
        break

if not plugins_release:
    print("Release 'plugins' does not exist, creating it now...")
    create_url = f'https://api.github.com/repos/{repo}/releases'
    payload = {
        'tag_name': 'plugins',
        'target_commitish': 'main',
        'name': 'Evan Plugins & Models 扩展模型库',
        'body': 'Evan 官方离线扩展插件包与模型库，供应用内一键下载或离线部署。',
        'draft': False,
        'prerelease': False
    }
    req_create = urllib.request.Request(
        create_url,
        data=json.dumps(payload).encode('utf-8'),
        headers=headers_api,
        method='POST'
    )
    try:
        with urllib.request.urlopen(req_create) as resp:
            plugins_release = json.loads(resp.read().decode('utf-8'))
            print("Successfully created release 'plugins', ID:", plugins_release.get('id'))
    except Exception as e:
        print("Failed to create 'plugins' release, will fallback to latest release (v1.0.2):", e)
        # fallback to the latest existing release if tag creation fails
        plugins_release = releases[0] if releases else None

if not plugins_release:
    print("Error: No release found to upload asset!")
    exit(1)

release_id = plugins_release['id']
print(f"Target release ID: {release_id}, tag: {plugins_release.get('tag_name')}")

def upload_file(file_path):
    filename = os.path.basename(file_path)
    file_size = os.path.getsize(file_path)
    print(f"\nUploading {filename} ({file_size / (1024*1024):.2f} MB)...")

    # 检查是否已存在同名 asset，若存在先删除
    for asset in plugins_release.get('assets', []):
        if asset.get('name') == filename:
            print(f"Asset {filename} already exists (ID: {asset['id']}), deleting first...")
            del_url = f"https://api.github.com/repos/{repo}/releases/assets/{asset['id']}"
            req_del = urllib.request.Request(del_url, headers=headers_api, method='DELETE')
            try:
                with urllib.request.urlopen(req_del) as d_resp:
                    print("Deleted old asset.")
            except Exception as de:
                print("Failed to delete old asset:", de)

    upload_url = f"https://uploads.github.com/repos/{repo}/releases/{release_id}/assets?name={filename}"
    headers_upload = {
        'Authorization': f'token {token}',
        'Content-Type': 'application/zip',
        'Content-Length': str(file_size),
        'User-Agent': 'Evan-Uploader'
    }

    with open(file_path, 'rb') as f:
        file_data = f.read()

    req_upload = urllib.request.Request(upload_url, data=file_data, headers=headers_upload, method='POST')
    try:
        with urllib.request.urlopen(req_upload, timeout=120) as resp:
            result = json.loads(resp.read().decode('utf-8'))
            print(f"Successfully uploaded {filename}!")
            print("Download URL:", result.get('browser_download_url'))
            return result.get('browser_download_url')
    except Exception as e:
        print(f"Error uploading {filename}:", e)
        return None

# 上传各插件包 (若文件存在则执行上传与覆盖)
for pkg in [
    'dist/evan-plugin-rapidocr-engine.zip',
    'dist/evan-plugin-translation-en-zh.zip',
    'dist/evan-plugin-gif-recorder.zip'
]:
    if os.path.exists(pkg):
        upload_file(pkg)
    else:
        print(f"Skipping {pkg} (not present in dist/)")
