import os
import subprocess
import math
from PIL import Image, ImageDraw, ImageFont

# 视频参数
WIDTH = 1920
HEIGHT = 1080
FPS = 30
FFMPEG_PATH = r"E:\ffmpeg\bin\ffmpeg.exe"
OUTPUT_MP4 = "EvanOCR_B站宣传视频_1080P.mp4"

# 尝试加载系统字体
def get_font(size, bold=False):
    font_names = [
        "msyhbd.ttc" if bold else "msyh.ttc",
        "simhei.ttf",
        "arialbd.ttf" if bold else "arial.ttf"
    ]
    for name in font_names:
        font_path = os.path.join(os.environ.get("WINDIR", "C:\\Windows"), "Fonts", name)
        if os.path.exists(font_path):
            try:
                return ImageFont.truetype(font_path, size)
            except Exception:
                pass
    return ImageFont.load_default()

font_title = get_font(56, bold=True)
font_h1 = get_font(44, bold=True)
font_h2 = get_font(32, bold=True)
font_body = get_font(24, bold=False)
font_sub = get_font(36, bold=True)
font_tag = get_font(20, bold=True)

def draw_background(draw):
    # 绘制深邃暗色微渐变背景
    for y in range(0, HEIGHT, 4):
        ratio = y / HEIGHT
        r = int(10 + ratio * 8)
        g = int(12 + ratio * 12)
        b = int(22 + ratio * 20)
        draw.rectangle([0, y, WIDTH, y + 4], fill=(r, g, b))
    
    # 绘制极简细点网格
    for x in range(60, WIDTH, 120):
        for y in range(60, HEIGHT, 120):
            draw.ellipse([x-1, y-1, x+1, y+1], fill=(40, 50, 80, 100))

def draw_subtitle(draw, text):
    # 底部高对比度字幕
    bbox = font_sub.getbbox(text)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    x = (WIDTH - tw) // 2
    y = HEIGHT - 110
    
    # 半透明衬底
    pad_x = 24
    pad_y = 10
    draw.rounded_rectangle([x - pad_x, y - pad_y, x + tw + pad_x, y + th + pad_y], radius=8, fill=(15, 20, 30, 220))
    # 阴影与黄色高光文字
    draw.text((x + 2, y + 2), text, font=font_sub, fill=(0, 0, 0, 180))
    draw.text((x, y), text, font=font_sub, fill=(250, 204, 21))

# 场景 1: 痛点引入 (0 - 5.5s)
def render_scene1(t):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_background(draw)
    
    # 主标题
    title = "还在忍受传统截屏工具的各种痛点？"
    t_bbox = font_title.getbbox(title)
    draw.text(((WIDTH - (t_bbox[2] - t_bbox[0])) // 2, 140), title, font=font_title, fill=(248, 250, 252))
    
    # 左右两个竞品卡片
    card_w, card_h = 560, 480
    c1_x, c1_y = 340, 260
    c2_x, c2_y = 1020, 260
    
    # 卡片 1: Snipaste
    draw.rounded_rectangle([c1_x, c1_y, c1_x + card_w, c1_y + card_h], radius=16, fill=(24, 28, 40), outline=(60, 70, 95), width=2)
    draw.text((c1_x + 40, c1_y + 40), "Snipaste", font=font_h1, fill=(239, 68, 68))
    draw.line([(c1_x + 40, c1_y + 105), (c1_x + card_w - 40, c1_y + 105)], fill=(50, 60, 80), width=2)
    
    items1 = [
        "❌ 免费版没有离线 OCR 文字识别",
        "❌ 提取文字需复杂配置第三方云端 API",
        "❌ 无法随时随地一键提取屏幕文字",
        "❌ 贴图后无法再次识别或复制字样"
    ]
    for i, item in enumerate(items1):
        draw.text((c1_x + 40, c1_y + 140 + i * 75), item, font=font_body, fill=(203, 213, 225))
        
    # 卡片 2: PixPin
    draw.rounded_rectangle([c2_x, c2_y, c2_x + card_w, c2_y + card_h], radius=16, fill=(24, 28, 40), outline=(60, 70, 95), width=2)
    draw.text((c2_x + 40, c2_y + 40), "PixPin", font=font_h1, fill=(249, 115, 22))
    draw.line([(c2_x + 40, c2_y + 105), (c2_x + card_w - 40, c2_y + 105)], fill=(50, 60, 80), width=2)
    
    items2 = [
        "❌ 离线识别需捆绑上百兆庞大模型包",
        "❌ 后台常驻内存偏大、启动响应慢",
        "❌ 识别后多余换行与空格导致排版错乱",
        "❌ 闭源黑盒，存在代码隐私顾虑"
    ]
    for i, item in enumerate(items2):
        draw.text((c2_x + 40, c2_y + 140 + i * 75), item, font=font_body, fill=(203, 213, 225))
        
    draw_subtitle(draw, "还在忍受 Snipaste 无法离线 OCR，或者 PixPin 动辄上百兆模型包吗？")
    return img

# 场景 2: EvanOCR 核心优势 (5.5 - 13.5s)
def render_scene2(t):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_background(draw)
    
    title = "EvanOCR 屏幕生产力神器"
    t_bbox = font_title.getbbox(title)
    draw.text(((WIDTH - (t_bbox[2] - t_bbox[0])) // 2, 110), title, font=font_title, fill=(56, 189, 248))
    
    sub = "C++20 + Qt6 纯原生打造 · Windows 10/11 原生硬件加速 · 零体积开箱即用"
    s_bbox = font_h2.getbbox(sub)
    draw.text(((WIDTH - (s_bbox[2] - s_bbox[0])) // 2, 195), sub, font=font_h2, fill=(148, 163, 184))
    
    # 3 大金刚优势卡片
    cols = [
        ("⚡ 0MB 模型体积", "调用 WinRT 系统级原生引擎", "完全无需额外下载任何模型包\n安装包体积仅 20MB 出头\n绿色解压即开即用"),
        ("🚀 毫秒级极速唤醒", "0 内存常驻 · 瞬时识别", "常驻内存开销接近为零\n按下热键秒级唤醒并完成识别\n告别任何卡顿等待"),
        ("🔒 100% 本地隐私安全", "纯离线运行 · 无网络上传", "不发起任何网络云端请求\n所有数据在本地内存毫秒完成\n绝密代码与设计图安全无忧")
    ]
    
    card_w = 460
    card_h = 420
    start_x = (WIDTH - (3 * card_w + 2 * 40)) // 2
    
    for i, (head, tag, desc) in enumerate(cols):
        cx = start_x + i * (card_w + 40)
        cy = 280
        draw.rounded_rectangle([cx, cy, cx + card_w, cy + card_h], radius=16, fill=(22, 27, 44), outline=(59, 130, 246), width=2)
        draw.text((cx + 30, cy + 40), head, font=font_h2, fill=(96, 165, 250))
        
        # 标签
        t_box = font_tag.getbbox(tag)
        draw.rounded_rectangle([cx + 30, cy + 95, cx + 30 + (t_box[2]-t_box[0]) + 16, cy + 95 + 32], radius=6, fill=(30, 58, 138))
        draw.text((cx + 38, cy + 100), tag, font=font_tag, fill=(147, 197, 253))
        
        lines = desc.split("\n")
        for li, line in enumerate(lines):
            draw.text((cx + 30, cy + 160 + li * 50), "✓ " + line, font=font_body, fill=(226, 232, 240))
            
    draw_subtitle(draw, "EvanOCR 直接调用 Windows 原生加速，0MB 模型，毫秒级秒出文字！")
    return img

# 场景 3: 独家左右对照 UI 与排版 (13.5 - 23.5s)
def render_scene3(t):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_background(draw)
    
    title = "独家创新：左右分栏 1:1 像素对照与智能排版"
    t_bbox = font_title.getbbox(title)
    draw.text(((WIDTH - (t_bbox[2] - t_bbox[0])) // 2, 90), title, font=font_title, fill=(56, 189, 248))
    
    # 模拟真实 EvanOCR 对比窗口 (白色浅色高质感风格)
    dlg_w, dlg_h = 1320, 620
    dlg_x = (WIDTH - dlg_w) // 2
    dlg_y = 180
    
    # 对话框整体白灰色卡片
    draw.rounded_rectangle([dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + dlg_h], radius=16, fill=(241, 245, 249), outline=(203, 213, 225), width=2)
    
    # 顶部标题栏
    draw.rounded_rectangle([dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + 55], radius=16, fill=(226, 232, 240))
    draw.text((dlg_x + 24, dlg_y + 14), "文本识别提取 - EvanOCR (左右对照模式)", font=font_body, fill=(30, 41, 59))
    
    # 左侧：原图对照
    half_w = (dlg_w - 40) // 2
    left_x = dlg_x + 15
    left_y = dlg_y + 65
    left_h = dlg_h - 130
    
    draw.rounded_rectangle([left_x, left_y, left_x + half_w, left_y + left_h], radius=10, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((left_x + 20, left_y + 15), "📷 原图对照 (像素级对齐定位)", font=font_h2, fill=(15, 23, 42))
    
    # 原图模拟识别框
    draw.rounded_rectangle([left_x + 30, left_y + 80, left_x + half_w - 30, left_y + 140], radius=6, fill=(219, 234, 254), outline=(37, 99, 235), width=2)
    draw.text((left_x + 50, left_y + 92), "EvanOCR 屏幕截贴图生产力工具", font=font_body, fill=(30, 58, 138))
    
    draw.rounded_rectangle([left_x + 30, left_y + 160, left_x + half_w - 30, left_y + 220], radius=6, fill=(219, 234, 254), outline=(37, 99, 235), width=2)
    draw.text((left_x + 50, left_y + 172), "毫秒级 Windows 原生硬件加速识别", font=font_body, fill=(30, 58, 138))
    
    draw.rounded_rectangle([left_x + 30, left_y + 240, left_x + half_w - 30, left_y + 300], radius=6, fill=(219, 234, 254), outline=(37, 99, 235), width=2)
    draw.text((left_x + 50, left_y + 252), "首创左右分栏无缝校对排版设计", font=font_body, fill=(30, 58, 138))
    
    draw.text((left_x + 30, left_y + 340), "✨ 蓝色包围框实时标记识别词条，错漏校对零死角！", font=font_body, fill=(37, 99, 235))
    
    # 右侧：提取文字与排版
    right_x = left_x + half_w + 10
    draw.rounded_rectangle([right_x, left_y, right_x + half_w, left_y + left_h], radius=10, fill=(255, 255, 255), outline=(226, 232, 240), width=1)
    
    # 功能按键模拟
    draw.text((right_x + 20, left_y + 15), "📝 提取文字结果", font=font_h2, fill=(15, 23, 42))
    
    # ¶ 合并段落
    draw.rounded_rectangle([right_x + 260, left_y + 12, right_x + 400, left_y + 48], radius=6, fill=(239, 246, 255), outline=(59, 130, 246), width=1)
    draw.text((right_x + 275, left_y + 18), "¶ 合并段落 [开]", font=font_tag, fill=(37, 99, 235))
    
    # ␣ 消除空格
    draw.rounded_rectangle([right_x + 415, left_y + 12, right_x + 555, left_y + 48], radius=6, fill=(239, 246, 255), outline=(59, 130, 246), width=1)
    draw.text((right_x + 430, left_y + 18), "␣ 消除空格 [开]", font=font_tag, fill=(37, 99, 235))
    
    # 提取内容
    sample_text = (
        "EvanOCR 屏幕截贴图生产力工具\n\n"
        "毫秒级 Windows 原生硬件加速识别。\n\n"
        "首创左右分栏无缝校对排版设计，彻底杜绝孤立折行与汉字间多余空格！"
    )
    for li, line in enumerate(sample_text.split("\n")):
        draw.text((right_x + 30, left_y + 90 + li * 40), line, font=font_body, fill=(15, 23, 42))
        
    # 底部复制主按键
    draw.rounded_rectangle([dlg_x + dlg_w - 280, dlg_y + dlg_h - 55, dlg_x + dlg_w - 40, dlg_y + dlg_h - 15], radius=8, fill=(37, 99, 235))
    draw.text((dlg_x + dlg_w - 240, dlg_y + dlg_h - 48), "📋 一键复制全部文本", font=font_tag, fill=(255, 255, 255))
    
    draw_subtitle(draw, "独家左图右文 1:1 对照，识别定位框一目了然，自带段落合并与空格清洗！")
    return img

# 场景 4: 全功能与开箱即用 (23.5 - 30.0s)
def render_scene4(t):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_background(draw)
    
    title = "开箱即用 · 纯粹高效的屏幕生产力"
    t_bbox = font_title.getbbox(title)
    draw.text(((WIDTH - (t_bbox[2] - t_bbox[0])) // 2, 110), title, font=font_title, fill=(56, 189, 248))
    
    features = [
        ("🎨 丝滑矢量标注", "矩形、箭头实时拉伸动态预览，支持原地打字与马赛克遮罩"),
        ("📌 独立置顶贴图", "支持无极缩放、透明度调节，右键随时按 Ctrl+O 二次文字提取"),
        ("⌨️ 自定义快捷键", "可在托盘菜单中自由录制唤醒热键 (F1/F4/Alt+A 随意配置)"),
        ("📦 绿色免安装包", "单文件便携解压即用，无任何捆绑，无后台常驻广告")
    ]
    
    f_w, f_h = 1000, 80
    f_start_x = (WIDTH - f_w) // 2
    f_start_y = 220
    
    for i, (ft, fd) in enumerate(features):
        fy = f_start_y + i * 105
        draw.rounded_rectangle([f_start_x, fy, f_start_x + f_w, fy + f_h], radius=12, fill=(24, 30, 48), outline=(59, 130, 246), width=1)
        draw.text((f_start_x + 30, fy + 22), ft, font=font_h2, fill=(96, 165, 250))
        draw.text((f_start_x + 280, fy + 26), fd, font=font_body, fill=(203, 213, 225))
        
    # 三连转化卡片
    call_y = 660
    draw.rounded_rectangle([f_start_x, call_y, f_start_x + f_w, call_y + 110], radius=16, fill=(30, 41, 59), outline=(250, 204, 21), width=2)
    draw.text((f_start_x + 40, call_y + 35), "求一键三连支持！源码与打包成品已开源，免费自取 ↓", font=font_h2, fill=(250, 204, 21))
    
    draw_subtitle(draw, "C++20 原生打造，完全自定义快捷键，单文件解压即用，求一键三连！")
    return img

def main():
    print(f"正在准备渲染 1080P 30FPS 视频 (总时长: 30 秒)...")
    
    # 启动 ffmpeg 进程，通过 stdin 接收原始 RGB 帧并编码为 H.264 1080P MP4
    cmd = [
        FFMPEG_PATH,
        "-y",
        "-f", "rawvideo",
        "-vcodec", "rawvideo",
        "-s", f"{WIDTH}x{HEIGHT}",
        "-pix_fmt", "rgb24",
        "-r", str(FPS),
        "-i", "-",
        "-c:v", "libx264",
        "-preset", "fast",
        "-crf", "19",
        "-pix_fmt", "yuv420p",
        "-movflags", "+faststart",
        OUTPUT_MP4
    ]
    
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    
    total_frames = 30 * FPS
    print(f"开始渲染并编码共 {total_frames} 帧...")
    
    for frame_idx in range(total_frames):
        t = frame_idx / FPS
        if t < 5.5:
            frame_img = render_scene1(t)
        elif t < 13.5:
            frame_img = render_scene2(t)
        elif t < 23.5:
            frame_img = render_scene3(t)
        else:
            frame_img = render_scene4(t)
            
        proc.stdin.write(frame_img.tobytes())
        
        if frame_idx % (5 * FPS) == 0:
            print(f"已完成: {frame_idx // FPS}s / 30s...")
            
    proc.stdin.close()
    proc.wait()
    print(f"✓ 视频生成成功！产物文件: {os.path.abspath(OUTPUT_MP4)}")

if __name__ == "__main__":
    main()
