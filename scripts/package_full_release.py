import os
import shutil
import zipfile
import hashlib
import json

root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
dist_dir = os.path.join(root_dir, "dist")
dist_evan = os.path.join(dist_dir, "evan")
dist_full = os.path.join(dist_dir, "evan-full")

print(">>> [1/5] 清理并准备全量包目录结构...")
if os.path.exists(dist_full):
    shutil.rmtree(dist_full)

# 从 dist/evan 复制基础运行环境 (主程序、Qt 库、MSVC 运行库、图标)
shutil.copytree(dist_evan, dist_full, ignore=shutil.ignore_patterns("config", "logs", "*.log", "*.ini"))
print("  [OK] 基础运行环境已就绪")

# 确保 plugins 目录存在
full_plugins = os.path.join(dist_full, "plugins")
os.makedirs(full_plugins, exist_ok=True)

print(">>> [2/5] 装配 RapidOCR 文字识别引擎 (本地离线 AI)...")
ocr_dst = os.path.join(full_plugins, "ocr")
os.makedirs(ocr_dst, exist_ok=True)
rapid_src = os.path.join(root_dir, "build", "rapidocr")
if os.path.exists(rapid_src):
    shutil.copytree(rapid_src, ocr_dst, dirs_exist_ok=True)
    print(f"  [OK] RapidOCR 引擎已装配: {ocr_dst}")
else:
    print(f"  [WARN] 找不到 {rapid_src}")

print(">>> [3/5] 装配 RapidLaTeXOCR 数学公式识别引擎...")
latex_dst = os.path.join(full_plugins, "latex_ocr")
os.makedirs(latex_dst, exist_ok=True)
latex_zip = os.path.join(dist_dir, "evan-plugin-latex-ocr.zip")
if os.path.exists(latex_zip):
    with zipfile.ZipFile(latex_zip, 'r') as zf:
        for member in zf.namelist():
            # 压缩包内顶层为 latex_ocr/
            rel = member
            if rel.startswith("latex_ocr/"):
                rel = rel[len("latex_ocr/"):]
            if not rel:
                continue
            target_path = os.path.join(latex_dst, rel)
            if member.endswith('/'):
                os.makedirs(target_path, exist_ok=True)
            else:
                os.makedirs(os.path.dirname(target_path), exist_ok=True)
                with zf.open(member) as src, open(target_path, 'wb') as dst:
                    shutil.copyfileobj(src, dst)
    print(f"  [OK] RapidLaTeXOCR 公式引擎已装配: {latex_dst}")
else:
    print(f"  [WARN] 找不到 {latex_zip}")

print(">>> [4/5] 装配离线中英神经翻译与 GIF 动态录屏扩展...")
trans_dst = os.path.join(full_plugins, "translation")
os.makedirs(trans_dst, exist_ok=True)
trans_zip = os.path.join(dist_dir, "evan-plugin-translation-en-zh.zip")
if os.path.exists(trans_zip):
    with zipfile.ZipFile(trans_zip, 'r') as zf:
        zf.extractall(trans_dst)
    print(f"  [OK] 离线翻译扩展已装配: {trans_dst}")

rec_dst = os.path.join(full_plugins, "recorder")
os.makedirs(rec_dst, exist_ok=True)
rec_zip = os.path.join(dist_dir, "evan-plugin-gif-recorder.zip")
if os.path.exists(rec_zip):
    with zipfile.ZipFile(rec_zip, 'r') as zf:
        zf.extractall(rec_dst)
    print(f"  [OK] GIF 录屏扩展已装配: {rec_dst}")

print(">>> [5/5] 正在压制全插件免安装便携版压缩包...")
full_zip_path = os.path.join(dist_dir, "evan-v1.2.1-windows-x64-full.zip")
if os.path.exists(full_zip_path):
    os.remove(full_zip_path)

with zipfile.ZipFile(full_zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
    for root, dirs, files in os.walk(dist_full):
        for file in files:
            file_path = os.path.join(root, file)
            arcname = os.path.relpath(file_path, dist_full)
            zipf.write(file_path, arcname)

size_mb = os.path.getsize(full_zip_path) / (1024 * 1024)
print("==================================================")
print(f"  全插件版打包完成: {full_zip_path} ({size_mb:.2f} MB)")
print("==================================================")
