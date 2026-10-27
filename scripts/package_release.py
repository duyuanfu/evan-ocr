import os
import shutil
import subprocess
import glob
import zipfile

def build_and_package():
    root_dir = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
    os.chdir(root_dir)

    print("==================================================")
    print("  Evan - Windows Release Packaging Script")
    print("==================================================")

    vcvars_bat = r"E:\VisualStudio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    cmake_exe = r"E:\Qt\Tools\CMake_64\bin\cmake.exe"
    ninja_exe = r"E:\Qt\Tools\Ninja\ninja.exe"
    windeployqt_exe = r"E:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe"
    vc_redist_dir = r"E:\VisualStudio\2022\Community\VC\Redist\MSVC\14.44.35112\x64\Microsoft.VC143.CRT"

    build_dir = os.path.join(root_dir, "build-release")
    dist_dir = os.path.join(root_dir, "dist")
    dist_evan = os.path.join(dist_dir, "evan")
    rapid_dir = os.path.join(dist_evan, "rapidocr")
    temp_rapid = os.path.join(root_dir, "media_kit", "temp_rapidocr")

    # 1. 备份已有 RapidOCR 模型库
    if os.path.exists(rapid_dir):
        if os.path.exists(temp_rapid):
            shutil.rmtree(temp_rapid)
        shutil.copytree(rapid_dir, temp_rapid)
        print("[OK] 已备份 RapidOCR 模型库")

    # 2. CMake 配置与编译 Release 版
    print(">>> 正在使用 CMake 配置 Release 工程...")
    cfg_cmd = f'call "{vcvars_bat}" && "{cmake_exe}" -B "{build_dir}" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="{ninja_exe}"'
    subprocess.run(cfg_cmd, shell=True, check=True)

    print(">>> 正在使用 Ninja 编译纯 Release 版二进制...")
    build_cmd = f'call "{vcvars_bat}" && "{ninja_exe}" -C "{build_dir}"'
    subprocess.run(build_cmd, shell=True, check=True)

    # 3. 准备发布目录
    if os.path.exists(dist_evan):
        try:
            shutil.rmtree(dist_evan)
        except Exception as e:
            print(f"  [提示] 清理发布目录跳过受锁文件: {e}")
    os.makedirs(dist_evan, exist_ok=True)

    release_exe = os.path.join(build_dir, "evan.exe")
    dst_exe = os.path.join(dist_evan, "evan.exe")
    shutil.copy2(release_exe, dst_exe)
    print(f"[OK] 已复制 Release 主程序: {dst_exe}")

    res_src = os.path.join(root_dir, "resources")
    res_dst = os.path.join(dist_evan, "resources")
    if os.path.exists(res_src):
        shutil.copytree(res_src, res_dst, dirs_exist_ok=True)
        print("[OK] 已复制图标资源")

    if os.path.exists(temp_rapid):
        shutil.copytree(temp_rapid, rapid_dir)
        # 统一同步至 plugins/ocr/ 目录，保持插件体系一致
        plugins_ocr = os.path.join(dist_evan, "plugins", "ocr")
        os.makedirs(plugins_ocr, exist_ok=True)
        shutil.copytree(temp_rapid, plugins_ocr, dirs_exist_ok=True)
        shutil.rmtree(temp_rapid)
        print("[OK] 已还原 RapidOCR 并统一结构至 plugins/ocr/")

    plugins_src = os.path.join(root_dir, "plugins")
    plugins_dst = os.path.join(dist_evan, "plugins")
    os.makedirs(plugins_dst, exist_ok=True)
    # 保持主程序便携包 21MB 轻量解耦：不预打包 170MB 的重量级 onnx 离线模型，由插件中心按需下载或 ZIP 导入
    if os.path.exists(plugins_src):
        shutil.copytree(plugins_src, plugins_dst, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns("*.onnx", "models"))
        print("[OK] 已复制插件规范与目录结构 (已解耦离线模型，保持极致轻量)")

    # 4. 运行 windeployqt 部署纯 Release 依赖
    print(">>> 正在运行 windeployqt 部署纯 Release 依赖...")
    cmd = [windeployqt_exe, "--release", "--no-translations", dst_exe]
    subprocess.run(cmd, check=True)

    # 5. 复制 MSVC 官方 64 位纯 Release 运行库
    if os.path.exists(vc_redist_dir):
        print(">>> 正在复制 MSVC 64位官方 Release 运行库...")
        for f in glob.glob(os.path.join(vc_redist_dir, "*.dll")):
            shutil.copy2(f, dist_evan)

    # 6. 清除可能误入的 Debug DLL 与运行期便携数据
    for root, dirs, files in os.walk(dist_evan):
        for file in files:
            low = file.lower()
            if (low.startswith("qt6") and low.endswith("d.dll")) or \
               (low.startswith("vcruntime") and low.endswith("d.dll")) or \
               (low.startswith("msvcp") and low.endswith("d.dll")) or \
               (low == "ucrtbased.dll"):
                full_p = os.path.join(root, file)
                os.remove(full_p)
                print(f"  [清理残留调试库] {file}")

    for unwanted_dir in ["config", "logs"]:
        unwanted_path = os.path.join(dist_evan, unwanted_dir)
        if os.path.exists(unwanted_path):
            shutil.rmtree(unwanted_path)
            print(f"  [清理测试数据] {unwanted_dir}/")

    # 7. 打包压缩为 zip
    zip_path = os.path.join(dist_dir, "evan-v1.2.1-windows-x64.zip")
    if os.path.exists(zip_path):
        os.remove(zip_path)

    print(f">>> 正在压制便携压缩包: {zip_path} ...")
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for root, dirs, files in os.walk(dist_evan):
            for file in files:
                file_path = os.path.join(root, file)
                arcname = os.path.relpath(file_path, dist_evan)
                zipf.write(file_path, arcname)

    size_mb = os.path.getsize(zip_path) / (1024 * 1024)

    # 8. 计算 SHA-256 并生成 version.json 供软件内自动升级与校验
    import hashlib
    import json
    sha256_hash = hashlib.sha256()
    with open(zip_path, "rb") as f:
        for byte_block in iter(lambda: f.read(65536), b""):
            sha256_hash.update(byte_block)
    zip_sha256 = sha256_hash.hexdigest()

    version_json_path = os.path.join(dist_dir, "version.json")
    version_data = {
        "version": "1.2.1",
        "tagName": "v1.2.1",
        "zipName": "evan-v1.2.1-windows-x64.zip",
        "zipUrl": "https://github.com/duyuanfu/evan-ocr/releases/download/v1.2.1/evan-v1.2.1-windows-x64.zip",
        "sha256": zip_sha256,
        "sizeBytes": os.path.getsize(zip_path),
        "notes": "Evan v1.2.1:\n- 彻底修复多显示器混合缩放下的截屏错位 (缩进 / 放大 / 黑边)\n- 自定义保存目录后一键自动保存, 不再弹出保存对话框\n- 真绿色便携化改造：配置 config/evan.ini 与日志 logs/evan.log，彻底移除开机自启与系统残留",
        "minAllowed": "1.0.0"
    }
    with open(version_json_path, "w", encoding="utf-8") as vf:
        json.dump(version_data, vf, indent=2, ensure_ascii=False)

    print("==================================================")
    print(f"  轻量便携包打包完成！产物路径: {zip_path} ({size_mb:.2f} MB)")
    print(f"  升级元数据: {version_json_path} (SHA256: {zip_sha256[:16]}...)")
    print("==================================================")

    # 9. 压制内置全部离线 AI 模型的全插件大包
    print("\n>>> 开始打包全插件离线大包 (全功能开箱即用)...")
    full_script = os.path.join(root_dir, "scripts", "package_full_release.py")
    if os.path.exists(full_script):
        subprocess.run(["python", full_script], check=True)

if __name__ == "__main__":
    build_and_package()
