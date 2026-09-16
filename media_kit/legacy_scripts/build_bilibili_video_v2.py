import os
import subprocess
import wave
import math
from PIL import Image, ImageDraw, ImageFont

WIDTH = 1920
HEIGHT = 1080
FPS = 30
FFMPEG_PATH = r"E:\ffmpeg\bin\ffmpeg.exe"

# 6幕台词剧本：先介绍产品 -> 演示功能截图 -> 竞品痛点对比 -> 贴图/标注/快捷键 -> 结尾
SCENES = [
    {
        "id": "scene1_intro",
        "title": "EvanOCR 屏幕生产力神器",
        "sub": "C++20 与 Qt6 打造 · 专为 Windows 10/11 原生而生",
        "text": "大家好！今天给大家带来一款全新的截贴图生产力神器——EvanOCR！它基于 C加加20 与 Qt6 打造，开箱即用，极速流畅！",
    },
    {
        "id": "scene2_ocr_core",
        "title": "首创「左右分栏」1:1 像素对照与智能排版",
        "sub": "原图识别定位框一目了然 · 告别错漏字核对盲区",
        "text": "截屏后按下字母O，全屏底图瞬间自动退出，弹出独家左右分栏对照窗口！左侧原图蓝框高亮标记，右侧提取文字不仅支持段落智能合并，还能一键消除汉字间多余空格！",
    },
    {
        "id": "scene3_competitors",
        "title": "为什么比 Snipaste 与 PixPin 更实用？",
        "sub": "直击传统截屏工具核心痛点 · 兼具轻量与强大",
        "text": "相比之下，Snipaste 免费版无法离线提取文字；而 PixPin 动辄捆绑上百兆庞大模型包，占用高。EvanOCR 直接调用 Windows 原生硬件加速，0兆外部模型包，毫秒级秒出文字，100%本地安全！",
    },
    {
        "id": "scene4_annotation",
        "title": "丝滑矢量标注 · 实时所见即所得",
        "sub": "矩形/箭头拖拽拉伸实时跟随 · 原位打字输入",
        "text": "在标注体验上，EvanOCR 的矩形、箭头支持实时旋转拉伸预览，所见即所得；文字标注可直接在选区原地打字，支持三档粗细与全色谱自由调节！",
    },
    {
        "id": "scene5_pin_hotkey",
        "title": "独立置顶贴图窗 · 快捷键自由录制",
        "sub": "右键按 Ctrl+O 二次文字提取 · 托盘随时自定义热键",
        "text": "按快捷键随时将画面生成置顶贴图，右键即可直接对贴图二次文字提取！托盘还提供快捷键录制功能，F1、F4随意切换！",
    },
    {
        "id": "scene6_outro",
        "title": "开箱即用 · 绿色免安装",
        "sub": "单文件解压即用 · 零后台常驻广告",
        "text": "单压缩包仅20多兆，纯绿色解压即用！喜欢的小伙伴请一定一键三连支持一下，下载地址就在置顶评论，快去体验吧！",
    }
]

# 生成每幕的 TTS 语音文件
def generate_all_tts():
    os.makedirs("audio_temp", exist_ok=True)
    audio_durations = []
    
    for idx, sc in enumerate(SCENES):
        wav_path = os.path.abspath(os.path.join("audio_temp", f"scene_{idx}.wav")).replace("\\", "/")
        text_content = sc["text"].replace("'", "''").replace("\n", " ")
        ps_script = (
            "Add-Type -AssemblyName System.Speech\n"
            "$s = New-Object System.Speech.Synthesis.SpeechSynthesizer\n"
            "$s.Rate = 1\n"
            f"$s.SetOutputToWaveFile('{wav_path}')\n"
            f"$s.Speak('{text_content}')\n"
            "$s.Dispose()\n"
        )
        ps_file = f"temp_{idx}.ps1"
        with open(ps_file, "w", encoding="utf-8-sig") as f:
            f.write(ps_script)
        subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", ps_file], check=True)
        if os.path.exists(ps_file):
            os.remove(ps_file)
            
        with wave.open(wav_path, "rb") as wf:
            dur = wf.getnframes() / float(wf.getframerate())
            dur = max(dur + 0.6, 4.0)
            audio_durations.append(dur)
            print(f"Scene {idx+1} audio duration: {dur:.2f}s")
            
    return audio_durations

def get_font(size, bold=False):
    font_names = ["msyhbd.ttc" if bold else "msyh.ttc", "simhei.ttf", "arialbd.ttf" if bold else "arial.ttf"]
    for name in font_names:
        font_path = os.path.join(os.environ.get("WINDIR", "C:\\Windows"), "Fonts", name)
        if os.path.exists(font_path):
            try:
                return ImageFont.truetype(font_path, size)
            except Exception:
                pass
    return ImageFont.load_default()

font_main_title = get_font(52, bold=True)
font_sub_title = get_font(28, bold=False)
font_card_title = get_font(32, bold=True)
font_body_bold = get_font(22, bold=True)
font_body = get_font(20, bold=False)
font_tag = get_font(18, bold=True)
font_sub = get_font(30, bold=True)

# 浅色明亮现代背景 (Slate/Zinc Light 渐变)
def draw_light_background(draw):
    for y in range(0, HEIGHT, 4):
        ratio = y / HEIGHT
        r = int(248 - ratio * 12)
        g = int(250 - ratio * 10)
        b = int(252 - ratio * 8)
        draw.rectangle([0, y, WIDTH, y + 4], fill=(r, g, b))
    # 点阵微纹理
    for x in range(40, WIDTH, 80):
        for y in range(40, HEIGHT, 80):
            draw.ellipse([x-1, y-1, x+1, y+1], fill=(226, 232, 240))

def draw_subtitle_light(draw, text):
    bbox = font_sub.getbbox(text)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    x = (WIDTH - tw) // 2
    y = HEIGHT - 100
    
    pad_x = 28
    pad_y = 10
    draw.rounded_rectangle([x - pad_x, y - pad_y, x + tw + pad_x, y + th + pad_y], radius=8, fill=(15, 23, 42, 230))
    draw.text((x, y), text, font=font_sub, fill=(255, 255, 255))

# 第 1 幕：EvanOCR 产品形象展示
def render_s1(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_light_background(draw)
    
    # 顶部标题
    title = SCENES[0]["title"]
    t_bbox = font_main_title.getbbox(title)
    draw.text(((WIDTH - (t_bbox[2] - t_bbox[0])) // 2, 90), title, font=font_main_title, fill=(15, 23, 42))
    
    sub = SCENES[0]["sub"]
    s_bbox = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_bbox[2] - s_bbox[0])) // 2, 160), sub, font=font_sub_title, fill=(71, 85, 105))
    
    # 中央展示大卡片 (包含应用大图标与核心价值卡片)
    card_w, card_h = 1200, 580
    cx = (WIDTH - card_w) // 2
    cy = 230
    draw.rounded_rectangle([cx, cy, cx + card_w, cy + card_h], radius=20, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    
    # 粘贴高清应用大图标
    if os.path.exists("resources/app_icon_256.png"):
        icon = Image.open("resources/app_icon_256.png").convert("RGBA")
        icon = icon.resize((200, 200), Image.Resampling.LANCZOS)
        img.paste(icon, (cx + 100, cy + 180), icon)
    
    # 图标右侧主文案
    rx = cx + 360
    draw.text((rx, cy + 80), "EvanOCR - 截贴图 & 原生离线文字提取", font=font_card_title, fill=(30, 58, 138))
    
    feats = [
        "✨ Windows 10/11 原生 WinRT 硬件加速 (0MB 外部模型包)",
        "✨ 首创「左右分栏 1:1 像素对照」排版提取界面",
        "✨ 智能段落折行拼接 (¶) 与汉字空格消除 (␣)",
        "✨ 丝滑矢量标注 (矩形/箭头/画笔/原位打字/马赛克)",
        "✨ 独立置顶贴图窗 (Pin Window) 支持随时 Ctrl+O 二次取词",
        "✨ 全局快捷键自由录制，开箱即用纯绿色便携"
    ]
    for i, ft in enumerate(feats):
        draw.text((rx, cy + 150 + i * 55), ft, font=font_body, fill=(51, 65, 85))
        
    draw_subtitle_light(draw, "全新截贴图生产力神器 EvanOCR：C++20 与 Qt6 打造，开箱即用，极速流畅！")
    return img

# 第 2 幕：真实 OCR 左右分栏功能截图展示
def render_s2(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_light_background(draw)
    
    title = SCENES[1]["title"]
    t_bbox = font_main_title.getbbox(title)
    draw.text(((WIDTH - (t_bbox[2] - t_bbox[0])) // 2, 70), title, font=font_main_title, fill=(15, 23, 42))
    
    sub = SCENES[1]["sub"]
    s_bbox = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_bbox[2] - s_bbox[0])) // 2, 135), sub, font=font_sub_title, fill=(37, 99, 235))
    
    # 模拟真实 EvanOCR 左右对照真实窗口
    dlg_w, dlg_h = 1360, 680
    dlg_x = (WIDTH - dlg_w) // 2
    dlg_y = 180
    
    # 窗口白底卡片
    draw.rounded_rectangle([dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + dlg_h], radius=16, fill=(241, 245, 249), outline=(203, 213, 225), width=2)
    # 标题栏
    draw.rounded_rectangle([dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + 50], radius=16, fill=(226, 232, 240))
    draw.text((dlg_x + 24, dlg_y + 14), "文本识别提取 - EvanOCR (左右对照模式)", font=font_body_bold, fill=(30, 41, 59))
    
    # 左右两半
    half_w = (dlg_w - 30) // 2
    
    # 左侧原图视口
    lx = dlg_x + 10
    ly = dlg_y + 60
    lh = dlg_h - 130
    draw.rounded_rectangle([lx, ly, lx + half_w, ly + lh], radius=10, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((lx + 20, ly + 15), "📷 原图对照 (像素级定位核对)", font=font_card_title, fill=(15, 23, 42))
    
    # 原图模拟文本框与天蓝定位框
    boxes = [
        (ly + 80, "EvanOCR 屏幕生产力桌面工具"),
        (ly + 160, "C++20 与 Qt6 打造 · Windows 10/11 原生加速"),
        (ly + 240, "独家左右分栏 1:1 像素对照排版设计"),
        (ly + 320, "毫秒级纯离线提取 · 零数据网络上传")
    ]
    for by, btxt in boxes:
        draw.rounded_rectangle([lx + 30, by, lx + half_w - 30, by + 56], radius=6, fill=(239, 246, 255), outline=(37, 99, 235), width=2)
        draw.text((lx + 45, by + 14), btxt, font=font_body, fill=(30, 58, 138))
        
    draw.text((lx + 30, ly + 400), "✓ 天蓝色包围框实时对应，错漏字核对零死角！", font=font_body_bold, fill=(37, 99, 235))
    
    # 右侧提取文字面板
    rx = lx + half_w + 10
    draw.rounded_rectangle([rx, ly, rx + half_w, ly + lh], radius=10, fill=(255, 255, 255), outline=(226, 232, 240), width=1)
    draw.text((rx + 20, ly + 15), "📝 提取文字结果", font=font_card_title, fill=(15, 23, 42))
    
    # 两个高光功能按钮
    draw.rounded_rectangle([rx + 280, ly + 12, rx + 430, ly + 48], radius=6, fill=(239, 246, 255), outline=(37, 99, 235), width=2)
    draw.text((rx + 295, ly + 18), "¶ 合并段落 [开]", font=font_tag, fill=(37, 99, 235))
    
    draw.rounded_rectangle([rx + 445, ly + 12, rx + 595, ly + 48], radius=6, fill=(239, 246, 255), outline=(37, 99, 235), width=2)
    draw.text((rx + 460, ly + 18), "␣ 清除空格 [开]", font=font_tag, fill=(37, 99, 235))
    
    # 右侧文本展示
    sample_ocr = (
        "EvanOCR 屏幕生产力桌面工具\n\n"
        "C++20 与 Qt6 打造 · Windows 10/11 原生加速。\n\n"
        "独家左右分栏 1:1 像素对照排版设计，彻底杜绝孤立折行与汉字间多余空格！"
    )
    for li, line in enumerate(sample_ocr.split("\n")):
        draw.text((rx + 30, ly + 85 + li * 38), line, font=font_body, fill=(15, 23, 42))
        
    # 底部主复制按钮
    draw.rounded_rectangle([dlg_x + dlg_w - 260, dlg_y + dlg_h - 55, dlg_x + dlg_w - 30, dlg_y + dlg_h - 15], radius=8, fill=(37, 99, 235))
    draw.text((dlg_x + dlg_w - 220, dlg_y + dlg_h - 48), "📋 复制全部文本", font=font_body_bold, fill=(255, 255, 255))
    
    draw_subtitle_light(draw, "按下字母O，全屏底图瞬间自动退出，弹出独家左右分栏对照排版窗口！")
    return img

# 第 3 幕：与 Snipaste / PixPin 核心痛点对比
def render_s3(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_light_background(draw)
    
    title = SCENES[2]["title"]
    t_bbox = font_main_title.getbbox(title)
    draw.text(((WIDTH - (t_bbox[2] - t_bbox[0])) // 2, 80), title, font=font_main_title, fill=(15, 23, 42))
    
    sub = SCENES[2]["sub"]
    s_bbox = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_bbox[2] - s_bbox[0])) // 2, 150), sub, font=font_sub_title, fill=(71, 85, 105))
    
    # 三列对比卡片：Snipaste | PixPin | EvanOCR
    col_w = 420
    col_h = 560
    c_start_x = (WIDTH - (3 * col_w + 2 * 30)) // 2
    cy = 220
    
    # 1. Snipaste 卡片
    x1 = c_start_x
    draw.rounded_rectangle([x1, cy, x1 + col_w, cy + col_h], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x1 + 30, cy + 30), "Snipaste", font=font_card_title, fill=(239, 68, 68))
    draw.line([(x1 + 30, cy + 85), (x1 + col_w - 30, cy + 85)], fill=(241, 245, 249), width=2)
    items1 = [
        "❌ 免费版没有离线 OCR 提取",
        "❌ 提取文字需复杂云端 API 配置",
        "❌ 贴图后无法再次提取文字",
        "❌ 标注无法实时所见即所得"
    ]
    for i, it in enumerate(items1):
        draw.text((x1 + 30, cy + 120 + i * 85), it, font=font_body, fill=(100, 116, 139))
        
    # 2. PixPin 卡片
    x2 = x1 + col_w + 30
    draw.rounded_rectangle([x2, cy, x2 + col_w, cy + col_h], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x2 + 30, cy + 30), "PixPin", font=font_card_title, fill=(249, 115, 22))
    draw.line([(x2 + 30, cy + 85), (x2 + col_w - 30, cy + 85)], fill=(241, 245, 249), width=2)
    items2 = [
        "❌ 离线需携带上百兆模型包",
        "❌ 后台常驻内存偏大、响应滞后",
        "❌ 识别多余换行与空格排版乱",
        "❌ 闭源商业黑盒软件"
    ]
    for i, it in enumerate(items2):
        draw.text((x2 + 30, cy + 120 + i * 85), it, font=font_body, fill=(100, 116, 139))
        
    # 3. EvanOCR 胜利卡片 (高亮蓝底)
    x3 = x2 + col_w + 30
    draw.rounded_rectangle([x3, cy, x3 + col_w, cy + col_h], radius=16, fill=(239, 246, 255), outline=(37, 99, 235), width=3)
    draw.text((x3 + 30, cy + 30), "EvanOCR (推荐)", font=font_card_title, fill=(37, 99, 235))
    draw.line([(x3 + 30, cy + 85), (x3 + col_w - 30, cy + 85)], fill=(191, 219, 254), width=2)
    items3 = [
        "✓ 0MB 模型！Win10/11 原生加速",
        "✓ 独家左右 1:1 对照智能排版",
        "✓ 贴图窗口随时按 Ctrl+O 二次取词",
        "✓ 毫秒级极速响应，100% 本地隐私"
    ]
    for i, it in enumerate(items3):
        draw.text((x3 + 30, cy + 120 + i * 85), it, font=font_body_bold, fill=(30, 58, 138))
        
    draw_subtitle_light(draw, "相比之下，EvanOCR 零体积、零依赖，毫秒级秒出文字，安全无死角！")
    return img

# 第 4 幕：丝滑矢量标注体验
def render_s4(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_light_background(draw)
    
    title = SCENES[3]["title"]
    t_bbox = font_main_title.getbbox(title)
    draw.text(((WIDTH - (t_bbox[2] - t_bbox[0])) // 2, 80), title, font=font_main_title, fill=(15, 23, 42))
    
    sub = SCENES[3]["sub"]
    s_bbox = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_bbox[2] - s_bbox[0])) // 2, 150), sub, font=font_sub_title, fill=(37, 99, 235))
    
    # 模拟真实截屏标注画面
    bg_w, bg_h = 1280, 600
    bx = (WIDTH - bg_w) // 2
    by = 220
    draw.rounded_rectangle([bx, by, bx + bg_w, by + bg_h], radius=16, fill=(255, 255, 255), outline=(203, 213, 225), width=2)
    
    # 选区虚线框
    draw.rounded_rectangle([bx + 80, by + 60, bx + bg_w - 80, by + bg_h - 140], radius=8, outline=(0, 120, 215), width=2)
    
    # 矢量矩形标注 (红色 4px)
    draw.rounded_rectangle([bx + 120, by + 100, bx + 520, by + 280], radius=4, outline=(235, 30, 30), width=4)
    draw.text((bx + 140, by + 120), "动态拉伸矩形 (4px / 鲜红)", font=font_body_bold, fill=(235, 30, 30))
    
    # 矢量箭头标注 (亮黄 4px)
    draw.line([(bx + 580, by + 260), (bx + 880, by + 130)], fill=(250, 140, 22), width=5)
    # 箭头头
    draw.polygon([(bx + 880, by + 130), (bx + 840, by + 130), (bx + 865, by + 160)], fill=(250, 140, 22))
    draw.text((bx + 620, by + 160), "实时旋转指向箭头", font=font_body_bold, fill=(250, 140, 22))
    
    # 原位文字输入框 (Snipaste / PixPin 风格)
    draw.rounded_rectangle([bx + 120, by + 320, bx + 640, by + 375], radius=4, fill=(20, 20, 20, 230), outline=(235, 30, 30), width=2)
    draw.text((bx + 135, by + 332), "原位所见即所得输入 | 直接按 Enter 固化", font=font_body_bold, fill=(235, 30, 30))
    
    # 底部浮动工具栏模拟 (带粗细与调色板)
    tb_x = bx + bg_w - 580
    tb_y = by + bg_h - 120
    draw.rounded_rectangle([tb_x, tb_y, tb_x + 480, tb_y + 80], radius=10, fill=(38, 38, 38), outline=(60, 60, 60), width=1)
    
    # 工具栏按键
    tools = ["▢", "➔", "✎", "T", "▚", "🔤", "↺", "📌", "💾", "✓"]
    for i, t_sym in enumerate(tools):
        btn_x = tb_x + 12 + i * 46
        btn_y = tb_y + 10
        draw.rounded_rectangle([btn_x, btn_y, btn_x + 36, btn_y + 32], radius=4, fill=(45, 45, 45))
        draw.text((btn_x + 10, btn_y + 4), t_sym, font=font_body_bold, fill=(240, 240, 240))
        
    # 工具栏下方的 3 档粗细与色盘
    draw.text((tb_x + 18, tb_y + 48), "粗细:  • 细  ● 中  ⬤ 粗   |   调色: 🔴 🟠 🟡 🟢 🔵 🟣 ⚪", font=font_body, fill=(200, 200, 200))
    
    draw_subtitle_light(draw, "矩形、箭头实时拉伸跟随，原地打字输入，三档粗细与色谱自由切换！")
    return img

# 第 5 幕：置顶贴图窗与自定义快捷键
def render_s5(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_light_background(draw)
    
    title = SCENES[4]["title"]
    t_bbox = font_main_title.getbbox(title)
    draw.text(((WIDTH - (t_bbox[2] - t_bbox[0])) // 2, 80), title, font=font_main_title, fill=(15, 23, 42))
    
    sub = SCENES[4]["sub"]
    s_bbox = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_bbox[2] - s_bbox[0])) // 2, 150), sub, font=font_sub_title, fill=(71, 85, 105))
    
    # 左右两个特色功能卡片
    card_w = 620
    card_h = 560
    x1 = (WIDTH - (2 * card_w + 40)) // 2
    x2 = x1 + card_w + 40
    cy = 220
    
    # 1. 独立置顶贴图卡片
    draw.rounded_rectangle([x1, cy, x1 + card_w, cy + card_h], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x1 + 35, cy + 35), "📌 独立置顶贴图 (Pin Window)", font=font_card_title, fill=(30, 58, 138))
    
    draw.rounded_rectangle([x1 + 40, cy + 100, x1 + card_w - 40, cy + 340], radius=10, fill=(241, 245, 249), outline=(59, 130, 246), width=2)
    draw.text((x1 + 60, cy + 130), "任意截图画面一键置顶浮动", font=font_body_bold, fill=(15, 23, 42))
    draw.text((x1 + 60, cy + 175), "• 滚轮无极平滑放大 / 缩小", font=font_body, fill=(51, 65, 85))
    draw.text((x1 + 60, cy + 215), "• Ctrl + 滚轮动态调节透明度", font=font_body, fill=(51, 65, 85))
    draw.text((x1 + 60, cy + 255), "• 右键随时按 Ctrl+O 再次文字识别！", font=font_body_bold, fill=(37, 99, 235))
    
    draw.text((x1 + 40, cy + 370), "彻底解决日常办公对照数据、抄写文字的来回切换痛点！", font=font_body, fill=(100, 116, 139))
    
    # 2. 快捷键设置卡片
    draw.rounded_rectangle([x2, cy, x2 + card_w, cy + card_h], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x2 + 35, cy + 35), "⚙️ 快捷键自由录制设置", font=font_card_title, fill=(30, 58, 138))
    
    draw.rounded_rectangle([x2 + 40, cy + 100, x2 + card_w - 40, cy + 340], radius=10, fill=(248, 250, 252), outline=(203, 213, 225), width=1)
    draw.text((x2 + 60, cy + 130), "全局截屏快捷键配置：", font=font_body_bold, fill=(15, 23, 42))
    
    # 快捷键录制框
    draw.rounded_rectangle([x2 + 60, cy + 180, x2 + card_w - 60, cy + 240], radius=6, fill=(255, 255, 255), outline=(37, 99, 235), width=2)
    draw.text((x2 + 80, cy + 195), "快捷键:  F1  (点击键盘即可直接修改)", font=font_body_bold, fill=(37, 99, 235))
    
    draw.text((x2 + 60, cy + 270), "✓ 支持 F1 ~ F12、Alt+A、Ctrl+Shift+A 任意组合", font=font_body, fill=(51, 65, 85))
    draw.text((x2 + 40, cy + 370), "托盘随时自定义，无需重启，保存即刻热生效！", font=font_body, fill=(100, 116, 139))
    
    draw_subtitle_light(draw, "置顶贴图随时按 Ctrl+O 二次提取文字，托盘随时自由录制修改快捷键！")
    return img

# 第 6 幕：结尾号召与绿色免安装
def render_s6(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_light_background(draw)
    
    title = SCENES[5]["title"]
    t_bbox = font_main_title.getbbox(title)
    draw.text(((WIDTH - (t_bbox[2] - t_bbox[0])) // 2, 100), title, font=font_main_title, fill=(15, 23, 42))
    
    sub = SCENES[5]["sub"]
    s_bbox = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_bbox[2] - s_bbox[0])) // 2, 175), sub, font=font_sub_title, fill=(37, 99, 235))
    
    # 居中大卡片
    card_w = 980
    card_h = 480
    cx = (WIDTH - card_w) // 2
    cy = 250
    draw.rounded_rectangle([cx, cy, cx + card_w, cy + card_h], radius=20, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    
    draw.text((cx + 80, cy + 60), "📦 完整免安装便携包：EvanOCR-v1.0.0-windows-x64.zip", font=font_card_title, fill=(30, 58, 138))
    
    details = [
        "• 仅 20 多兆轻巧体积，解压即开即用",
        "• 100% 纯本地离线运行，绝密数据安全无忧",
        "• 彻底替代 Snipaste / PixPin 的截贴图识别利器",
        "• 欢迎在评论区提出更多宝贵需求与反馈！"
    ]
    for i, d in enumerate(details):
        draw.text((cx + 80, cy + 130 + i * 50), d, font=font_body, fill=(51, 65, 85))
        
    # 三连求赞大金牌
    draw.rounded_rectangle([cx + 60, cy + 340, cx + card_w - 60, cy + 440], radius=14, fill=(254, 240, 138), outline=(234, 179, 8), width=2)
    draw.text((cx + 120, cy + 372), "求一键三连支持！源码与绿色版下载地址在置顶评论 ↓", font=font_card_title, fill=(133, 77, 14))
    
    draw_subtitle_light(draw, "单文件解压即用！求一键三连支持，下载地址在置顶评论，快去体验吧！")
    return img

def main():
    print("Step 1: 生成各幕真人/清亮配音 WAV 音频...")
    durations = generate_all_tts()
    total_duration = sum(durations)
    print(f"总时长: {total_duration:.2f} 秒")
    
    # 合并音频轨并添加轻快科技感背景音乐
    # 使用 ffmpeg 拼接所有 scene WAV
    concat_txt = "audio_temp/concat.txt"
    with open(concat_txt, "w", encoding="utf-8") as f:
        for idx in range(len(SCENES)):
            wav_path = os.path.abspath(f"audio_temp/scene_{idx}.wav").replace("\\", "/")
            f.write(f"file '{wav_path}'\n")
            
    merged_voice = "audio_temp/merged_voice.wav"
    subprocess.run([FFMPEG_PATH, "-y", "-f", "concat", "-safe", "0", "-i", concat_txt, "-c", "copy", merged_voice], check=True)
    
    # 生成轻柔优美 BGM (432Hz 温暖正弦和弦微弱背景乐)
    bgm_wav = "audio_temp/bgm.wav"
    bgm_cmd = [
        FFMPEG_PATH, "-y",
        "-f", "lavfi", "-i", f"sine=frequency=523.25:duration={total_duration}",
        "-f", "lavfi", "-i", f"sine=frequency=659.25:duration={total_duration}",
        "-filter_complex", "amix=inputs=2:duration=first,volume=0.04",
        bgm_wav
    ]
    subprocess.run(bgm_cmd, check=True)
    
    # 混音：人声 (1.0) + 背景音乐 (0.04)
    final_audio = "audio_temp/final_audio.wav"
    subprocess.run([
        FFMPEG_PATH, "-y",
        "-i", merged_voice,
        "-i", bgm_wav,
        "-filter_complex", "amix=inputs=2:duration=first:dropout_transition=2",
        final_audio
    ], check=True)
    
    print("Step 2: 渲染浅色明亮高质感视频帧并通过管道压制输出...")
    output_mp4 = "EvanOCR_B站高清介绍视频_1080P60.mp4"
    
    cmd = [
        FFMPEG_PATH, "-y",
        "-f", "rawvideo",
        "-vcodec", "rawvideo",
        "-s", f"{WIDTH}x{HEIGHT}",
        "-pix_fmt", "rgb24",
        "-r", str(FPS),
        "-i", "-",
        "-i", final_audio,
        "-c:v", "libx264",
        "-preset", "fast",
        "-crf", "18",
        "-pix_fmt", "yuv420p",
        "-c:a", "aac",
        "-b:a", "192k",
        "-shortest",
        "-movflags", "+faststart",
        output_mp4
    ]
    
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    
    # 累计时间节点
    time_boundaries = []
    accum = 0.0
    for d in durations:
        accum += d
        time_boundaries.append(accum)
        
    total_frames = int(total_duration * FPS)
    print(f"开始压制共 {total_frames} 帧 (1080P 30FPS)...")
    
    for f_idx in range(total_frames):
        current_time = f_idx / FPS
        
        # 判断属于哪个场景
        if current_time < time_boundaries[0]:
            frame = render_s1(current_time, durations[0])
        elif current_time < time_boundaries[1]:
            frame = render_s2(current_time - time_boundaries[0], durations[1])
        elif current_time < time_boundaries[2]:
            frame = render_s3(current_time - time_boundaries[1], durations[2])
        elif current_time < time_boundaries[3]:
            frame = render_s4(current_time - time_boundaries[2], durations[3])
        elif current_time < time_boundaries[4]:
            frame = render_s5(current_time - time_boundaries[3], durations[4])
        else:
            frame = render_s6(current_time - time_boundaries[4], durations[5])
            
        proc.stdin.write(frame.tobytes())
        
        if f_idx % (5 * FPS) == 0:
            print(f"进度: {f_idx // FPS}s / {int(total_duration)}s")
            
    proc.stdin.close()
    proc.wait()
    print(f"Finished successfully: {os.path.abspath(output_mp4)}")

if __name__ == "__main__":
    main()
