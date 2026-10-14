import os
import zipfile
import shutil

def package_formula_plugin():
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    plugin_src = os.path.join(root_dir, "plugins", "latex_ocr")
    dist_dir = os.path.join(root_dir, "dist")
    os.makedirs(dist_dir, exist_ok=True)

    zip_path = os.path.join(dist_dir, "evan-plugin-latex-ocr.zip")
    if os.path.exists(zip_path):
        os.remove(zip_path)

    print(f">>> 正在打包离线公式识别插件包: {zip_path} ...")
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for root, dirs, files in os.walk(plugin_src):
            for file in files:
                file_path = os.path.join(root, file)
                # 保持相对路径，包内根目录以 latex_ocr 或扁平组织
                arcname = os.path.relpath(file_path, os.path.dirname(plugin_src))
                zipf.write(file_path, arcname)

    size_mb = os.path.getsize(zip_path) / (1024 * 1024)
    print(f"[OK] 离线公式识别插件包打包完成: {zip_path} ({size_mb:.2f} MB)")

if __name__ == "__main__":
    package_formula_plugin()
