import os
import math
import asyncio
import subprocess
import wave
from PIL import Image, ImageDraw, ImageFont
import edge_tts

WIDTH = 1920
HEIGHT = 1080
FPS = 30
FFMPEG_PATH = r"E:\ffmpeg\bin\ffmpeg.exe"
VOICE = "zh-CN-YunxiNeural"  # 微软超逼真自然活泼男声 (B站科技数码 UP 顶流音色)
VOICE_RATE = "+22%"           # 语速提升 22%，紧凑抓耳不拖沓

SCENES = [
    {
        "id": "s1",
        "title": "EvanOCR - 全新原生截贴图文字提取神器",
        "sub": "C++20 & Qt6 纯原生打造 · Windows 10/11 极速离线硬件加速",
        "text": "小伙伴们好！今天给大家推荐一款真正称手的全新截贴图文字提取神器——EvanOCR！C++20 和 Qt6 原生打造，纯离线、零体积、开箱即用！"
    },
    {
        "id": "s2",
        "title": "首创「左右分栏」1:1 像素对照排版",
        "sub": "原图天蓝定位框精准核对 · 告别错漏字核对盲区",
        "text": "截屏选区后按 O 键，全屏底图自动退出，弹出独家左右分栏对照窗口！左侧原图蓝框精准标记文字，右侧不仅能自动合并段落，还能一键消除汉字间多余空格！"
    },
    {
        "id": "s3",
        "title": "为什么比 Snipaste 与 PixPin 更实用？",
        "sub": "直击传统截屏工具核心痛点 · 兼具轻量与强大",
        "text": "比起 Snipaste 免费版没有离线 OCR，或者 PixPin 动辄捆绑上百兆庞大模型包、占用高，EvanOCR 直接调用 Windows 原生加速，零兆额外模型包，毫秒级秒出文字，100% 本地隐私安全！"
    },
    {
        "id": "s4",
        "title": "丝滑矢量标注 · 实时所见即所得",
        "sub": "矩形与箭头拖拽拉伸实时旋转跟随 · 原位打字输入",
        "text": "标注体验极其丝滑！矩形、箭头支持实时拖拽拉伸旋转预览，所见即所得；文字标注可直接在原图原地打字，支持三档粗细与全色谱自由调节！"
    },
    {
        "id": "s5",
        "title": "独立置顶贴图窗 · 快捷键自由录制",
        "sub": "右键按 Ctrl+O 二次文字提取 · 托盘随时自由配置热键",
        "text": "贴图窗口支持无极滚轮缩放与透明度调节，右键随时按 Ctrl+O 对贴图二次提取文字！托盘菜单还能自由录制快捷键，F1、F4 随心设置！"
    },
    {
        "id": "s6",
        "title": "开箱即用 · 纯绿色便携免安装",
        "sub": "单压缩包仅 20 多兆 · 零后台常驻广告",
        "text": "单压缩包只有 20 多兆，解压即开即用！纯离线运行。求大家一键三连支持一下，开源打包成品在置顶评论，快去体验吧！"
    }
]

# 1. 生成自然真人配音 (Edge-TTS)
async def generate_voice_tracks():
    os.makedirs("audio_v3", exist_ok=True)
    durations = []
    print("正在生成自然拟真语音 (zh-CN-YunxiNeural, rate=+22%)...")
    
    for idx, sc in enumerate(SCENES):
        mp3_path = f"audio_v3/scene_{idx}.mp3"
        wav_path = f"audio_v3/scene_{idx}.wav"
        
        tts = edge_tts.Communicate(sc["text"], VOICE, rate=VOICE_RATE)
        await tts.save(mp3_path)
        
        # 转为标准 WAV
        subprocess.run([FFMPEG_PATH, "-y", "-i", mp3_path, "-ac", "1", "-ar", "24000", wav_path],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
        
        with wave.open(wav_path, "rb") as wf:
            dur = wf.getnframes() / float(wf.getframerate())
            dur = max(dur + 0.5, 3.5)
            durations.append(dur)
            print(f"Scene {idx+1}: {dur:.2f}s")
            
    return durations

# 2. 混音处理：人声 + 真实科技轻快 BGM
def mix_audio_tracks(durations):
    total_duration = sum(durations)
    print(f"合成总音频时长: {total_duration:.2f}s")
    
    # 拼接所有人声段落
    concat_txt = "audio_v3/concat.txt"
    with open(concat_txt, "w", encoding="utf-8") as f:
        for idx in range(len(SCENES)):
            wav_p = os.path.abspath(f"audio_v3/scene_{idx}.wav").replace("\\", "/")
            f.write(f"file '{wav_p}'\n")
            
    merged_voice = "audio_v3/voice_merged.wav"
    subprocess.run([FFMPEG_PATH, "-y", "-f", "concat", "-safe", "0", "-i", concat_txt, "-c", "copy", merged_voice],
                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    
    final_audio = "audio_v3/final_soundtrack.wav"
    bgm_file = "bilibili_bgm.mp3"
    
    if os.path.exists(bgm_file):
        # 混音真实 BGM (循环播放并降低音量至 0.08，不抢人声)
        mix_filter = (
            f"[1:a]aloop=loop=-1:size=2e+09,atrim=0:{total_duration},volume=0.08[bgm];"
            f"[0:a]volume=1.2[vox];"
            f"[vox][bgm]amix=inputs=2:duration=first:dropout_transition=2[out]"
        )
        cmd = [
            FFMPEG_PATH, "-y",
            "-i", merged_voice,
            "-i", bgm_file,
            "-filter_complex", mix_filter,
            "-map", "[out]",
            final_audio
        ]
        subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    else:
        final_audio = merged_voice
        
    return final_audio, total_duration

# 3. 字体加载与排版
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

font_title = get_font(64, bold=True)
font_sub_title = get_font(32, bold=False)
font_card_h = get_font(36, bold=True)
font_body_bold = get_font(28, bold=True)
font_body = get_font(26, bold=False)
font_badge = get_font(22, bold=True)
font_sub = get_font(38, bold=True)

# 浅白清爽大气质感背景 (全屏撑满)
def draw_bg(draw):
    for y in range(0, HEIGHT, 4):
        ratio = y / HEIGHT
        r = int(248 - ratio * 10)
        g = int(250 - ratio * 8)
        b = int(254 - ratio * 6)
        draw.rectangle([0, y, WIDTH, y + 4], fill=(r, g, b))
        
    # 精致浅灰蓝色点阵
    for x in range(30, WIDTH, 60):
        for y in range(30, HEIGHT, 60):
            draw.ellipse([x-1, y-1, x+1, y+1], fill=(226, 232, 240))

# 无黑底大气字幕 (直接大字 + 柔和阴影)
def draw_clean_subtitle(draw, text):
    bbox = font_sub.getbbox(text)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    x = (WIDTH - tw) // 2
    y = HEIGHT - 85
    
    # 柔和深色阴影 (代替突兀的纯黑矩形)
    draw.text((x + 2, y + 2), text, font=font_sub, fill=(15, 23, 42, 60))
    # 优雅深海军蓝主文字
    draw.text((x, y), text, font=font_sub, fill=(15, 23, 42))

# 绘制原生矢量徽章列表项 (解决符号显示为空格的问题)
def draw_feature_item(draw, x, y, title, subtitle=None, is_highlight=False):
    # 徽章背景
    badge_color = (37, 99, 235) if is_highlight else (14, 165, 233)
    draw.rounded_rectangle([x, y + 4, x + 30, y + 34], radius=6, fill=badge_color)
    # 纯净矢量白勾
    draw.line([(x + 8, y + 19), (x + 13, y + 25)], fill=(255, 255, 255), width=3)
    draw.line([(x + 13, y + 25), (x + 23, y + 13)], fill=(255, 255, 255), width=3)
    
    # 文字
    text_color = (15, 23, 42) if not is_highlight else (30, 58, 138)
    draw.text((x + 44, y), title, font=font_body_bold if is_highlight else font_body, fill=text_color)
    if subtitle:
        draw.text((x + 44, y + 38), subtitle, font=font_body, fill=(100, 116, 139))

# 场景 1: 产品介绍 (全屏撑满 1680px 宽度)
def render_s1(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    # 动画：平滑缓入位移
    anim_y = int(25 * math.exp(-t * 3.5))
    
    # 主标题
    title = SCENES[0]["title"]
    t_box = font_title.getbbox(title)
    draw.text(((WIDTH - (t_box[2] - t_box[0])) // 2, 70 - anim_y), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENES[0]["sub"]
    s_box = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_box[2] - s_box[0])) // 2, 150 - anim_y), sub, font=font_sub_title, fill=(37, 99, 235))
    
    # 全屏宽卡片 (1720px 宽度，彻底消除左右两边太空)
    card_w, card_h = 1720, 680
    cx = (WIDTH - card_w) // 2
    cy = 210 - anim_y
    draw.rounded_rectangle([cx, cy, cx + card_w, cy + card_h], radius=20, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    
    # 左侧：大图标与软件定位
    icon_w = 420
    draw.rounded_rectangle([cx + 20, cy + 20, cx + icon_w, cy + card_h - 20], radius=16, fill=(248, 250, 252))
    if os.path.exists("resources/app_icon_256.png"):
        icon = Image.open("resources/app_icon_256.png").convert("RGBA")
        icon = icon.resize((240, 240), Image.Resampling.LANCZOS)
        img.paste(icon, (cx + 110, cy + 140), icon)
    draw.text((cx + 105, cy + 420), "EvanOCR v1.0", font=font_card_h, fill=(15, 23, 42))
    draw.text((cx + 85, cy + 475), "全新 Windows 截贴图工具", font=font_body, fill=(100, 116, 139))
    
    # 右侧：六大核心亮点卡片矩阵
    rx = cx + icon_w + 50
    draw.text((rx, cy + 40), "专为极致生产力打造的核心特性", font=font_card_h, fill=(30, 58, 138))
    
    items = [
        ("Windows 10/11 原生 WinRT 硬件加速", "0MB 外部模型包体积，0ms 启动，零内存常驻"),
        ("独家「左右分栏 1:1 像素对照」排版界面", "原图天蓝色定位框标记，文字核对零盲区"),
        ("智能文本排版清洗：段落合并 (¶) 与空格消除 (␣)", "告别截图提取后的多余折行与中文字间冗余空格"),
        ("丝滑所见即所得矢量标注体系", "矩形/箭头实时旋转拉伸跟随，原位直接打字"),
        ("独立置顶贴图窗 (Pin Window) 深度联动", "无极缩放/透明度调节，右键随时 Ctrl+O 二次提取"),
        ("完全自定义全局快捷键", "托盘菜单一键录制 F1/F4 任意热键，绿色免安装")
    ]
    
    for i, (head, desc) in enumerate(items):
        col = i % 2
        row = i // 2
        ix = rx + col * 610
        iy = cy + 120 + row * 160
        draw.rounded_rectangle([ix, iy, ix + 580, iy + 135], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
        draw_feature_item(draw, ix + 25, iy + 25, head, desc, is_highlight=(i < 2))
        
    draw_clean_subtitle(draw, "全新截贴图文字提取神器 EvanOCR：C++20 与 Qt6 原生打造，极速流畅！")
    return img

# 场景 2: 左右对照实机演示 (带动态激光扫描线动画)
def render_s2(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENES[1]["title"]
    t_box = font_title.getbbox(title)
    draw.text(((WIDTH - (t_box[2] - t_box[0])) // 2, 60), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENES[1]["sub"]
    s_box = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_box[2] - s_box[0])) // 2, 135), sub, font=font_sub_title, fill=(37, 99, 235))
    
    # 全屏宽实机窗口 (1720px 宽度)
    dlg_w, dlg_h = 1720, 700
    dlg_x = (WIDTH - dlg_w) // 2
    dlg_y = 190
    
    draw.rounded_rectangle([dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + dlg_h], radius=16, fill=(241, 245, 249), outline=(203, 213, 225), width=2)
    # 标题栏
    draw.rounded_rectangle([dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + 55], radius=16, fill=(226, 232, 240))
    draw.text((dlg_x + 30, dlg_y + 15), "文本识别提取 - EvanOCR (左右对照模式)", font=font_body_bold, fill=(30, 41, 59))
    
    half_w = (dlg_w - 40) // 2
    
    # 左侧原图视口
    lx = dlg_x + 15
    ly = dlg_y + 70
    lh = dlg_h - 150
    draw.rounded_rectangle([lx, ly, lx + half_w, ly + lh], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((lx + 30, ly + 20), "📷 原图对照 (像素级定位核对)", font=font_card_h, fill=(15, 23, 42))
    
    # 模拟原图文字及定位框
    boxes = [
        (ly + 90, "EvanOCR 屏幕生产力桌面工具"),
        (ly + 175, "C++20 与 Qt6 原生打造 · Windows 10/11 原生硬件加速"),
        (ly + 260, "独家左右分栏 1:1 像素对照排版设计"),
        (ly + 345, "毫秒级纯离线提取 · 零网络数据上传")
    ]
    for by, btxt in boxes:
        draw.rounded_rectangle([lx + 35, by, lx + half_w - 35, by + 65], radius=8, fill=(239, 246, 255), outline=(37, 99, 235), width=2)
        draw.text((lx + 55, by + 18), btxt, font=font_body_bold, fill=(30, 58, 138))
        
    # 动态激光扫描线动画 (沿原图上下往复平滑扫描)
    scan_offset = int((math.sin(t * 3.0) * 0.5 + 0.5) * (lh - 120))
    scan_y = ly + 80 + scan_offset
    draw.line([(lx + 30, scan_y), (lx + half_w - 30, scan_y)], fill=(56, 189, 248), width=3)
    draw.line([(lx + 30, scan_y + 1), (lx + half_w - 30, scan_y + 1)], fill=(96, 165, 250, 100), width=6)
    
    draw.text((lx + 35, ly + 435), "✓ 天蓝色包围框实时对应，核对错漏字零死角！", font=font_body_bold, fill=(37, 99, 235))
    
    # 右侧提取文字结果
    rx = lx + half_w + 10
    draw.rounded_rectangle([rx, ly, rx + half_w, ly + lh], radius=12, fill=(255, 255, 255), outline=(226, 232, 240), width=1)
    draw.text((rx + 30, ly + 20), "📝 提取文字结果", font=font_card_h, fill=(15, 23, 42))
    
    # 两个核心按钮
    draw.rounded_rectangle([rx + 420, ly + 16, rx + 590, ly + 56], radius=6, fill=(239, 246, 255), outline=(37, 99, 235), width=2)
    draw.text((rx + 440, ly + 23), "¶ 合并段落 [开]", font=font_badge, fill=(37, 99, 235))
    
    draw.rounded_rectangle([rx + 610, ly + 16, rx + 780, ly + 56], radius=6, fill=(239, 246, 255), outline=(37, 99, 235), width=2)
    draw.text((rx + 630, ly + 23), "␣ 清除空格 [开]", font=font_badge, fill=(37, 99, 235))
    
    sample_text = (
        "EvanOCR 屏幕生产力桌面工具\n\n"
        "C++20 与 Qt6 原生打造 · Windows 10/11 原生硬件加速。\n\n"
        "独家左右分栏 1:1 像素对照排版设计，彻底消除孤立折行与汉字间算法多余空格！"
    )
    for li, line in enumerate(sample_text.split("\n")):
        draw.text((rx + 35, ly + 95 + li * 42), line, font=font_body, fill=(15, 23, 42))
        
    # 底部主操作条
    draw.text((dlg_x + 30, dlg_y + dlg_h - 50), "字数: 86  |  行数: 3", font=font_body, fill=(100, 116, 139))
    draw.rounded_rectangle([dlg_x + dlg_w - 320, dlg_y + dlg_h - 60, dlg_x + dlg_w - 30, dlg_y + dlg_h - 15], radius=8, fill=(37, 99, 235))
    draw.text((dlg_x + dlg_w - 270, dlg_y + dlg_h - 52), "📋 复制全部文本", font=font_body_bold, fill=(255, 255, 255))
    
    draw_clean_subtitle(draw, "按下字母O瞬间退出全屏底图，弹出独家左右分栏对照排版窗口！")
    return img

# 场景 3: 竞品痛点对比 (撑满 1720px 宽度)
def render_s3(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENES[2]["title"]
    t_box = font_title.getbbox(title)
    draw.text(((WIDTH - (t_box[2] - t_box[0])) // 2, 70), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENES[2]["sub"]
    s_box = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_box[2] - s_box[0])) // 2, 145), sub, font=font_sub_title, fill=(71, 85, 105))
    
    # 三张大卡片：Snipaste | PixPin | EvanOCR
    col_w = 540
    col_h = 660
    c_start_x = (WIDTH - (3 * col_w + 2 * 40)) // 2
    cy = 200
    
    # 1. Snipaste
    x1 = c_start_x
    draw.rounded_rectangle([x1, cy, x1 + col_w, cy + col_h], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x1 + 35, cy + 35), "Snipaste", font=font_card_h, fill=(239, 68, 68))
    draw.text((x1 + 35, cy + 85), "老牌截贴图，但 OCR 极其不便", font=font_body, fill=(100, 116, 139))
    draw.line([(x1 + 35, cy + 125), (x1 + col_w - 35, cy + 125)], fill=(241, 245, 249), width=2)
    
    p1 = [
        "❌ 免费版没有离线 OCR 文字提取功能",
        "❌ 提取文字必须繁琐配置第三方云端 API",
        "❌ 贴图置顶后无法对图中文字再次识别",
        "❌ 标注过程无法实时所见即所得动态预览"
    ]
    for i, it in enumerate(p1):
        draw.text((x1 + 35, cy + 160 + i * 110), it, font=font_body_bold, fill=(71, 85, 105))
        
    # 2. PixPin
    x2 = x1 + col_w + 40
    draw.rounded_rectangle([x2, cy, x2 + col_w, cy + col_h], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x2 + 35, cy + 35), "PixPin", font=font_card_h, fill=(249, 115, 22))
    draw.text((x2 + 35, cy + 85), "集成 OCR，但体积庞大且排版差", font=font_body, fill=(100, 116, 139))
    draw.line([(x2 + 35, cy + 125), (x2 + col_w - 35, cy + 125)], fill=(241, 245, 249), width=2)
    
    p2 = [
        "❌ 离线需携带上百兆庞大模型包",
        "❌ 后台常驻内存开销大，启动滞后",
        "❌ 提取结果大量多余断行与冗余空格",
        "❌ 闭源商业黑盒软件，隐私难保证"
    ]
    for i, it in enumerate(p2):
        draw.text((x2 + 35, cy + 160 + i * 110), it, font=font_body_bold, fill=(71, 85, 105))
        
    # 3. EvanOCR (高亮推荐)
    x3 = x2 + col_w + 40
    draw.rounded_rectangle([x3, cy, x3 + col_w, cy + col_h], radius=16, fill=(239, 246, 255), outline=(37, 99, 235), width=3)
    draw.text((x3 + 35, cy + 35), "EvanOCR (极力推荐)", font=font_card_h, fill=(37, 99, 235))
    draw.text((x3 + 35, cy + 85), "Windows 10/11 原生硬件加速", font=font_body_bold, fill=(30, 58, 138))
    draw.line([(x3 + 35, cy + 125), (x3 + col_w - 35, cy + 125)], fill=(191, 219, 254), width=2)
    
    p3 = [
        "✓ 0MB 模型！直接调用 WinRT 原生加速",
        "✓ 独家左右 1:1 对照，自带段落与空格清洗",
        "✓ 贴图窗口随时按 Ctrl+O 二次提取文字",
        "✓ 毫秒级极速唤醒，100% 纯本地离线隐私"
    ]
    for i, it in enumerate(p3):
        draw.text((x3 + 35, cy + 160 + i * 110), it, font=font_body_bold, fill=(30, 58, 138))
        
    draw_clean_subtitle(draw, "EvanOCR 零体积、零依赖，毫秒级秒出文字，安全无死角！")
    return img

# 场景 4: 矢量标注实机体验 (撑满 1720px)
def render_s4(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENES[3]["title"]
    t_box = font_title.getbbox(title)
    draw.text(((WIDTH - (t_box[2] - t_box[0])) // 2, 70), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENES[3]["sub"]
    s_box = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_box[2] - s_box[0])) // 2, 145), sub, font=font_sub_title, fill=(37, 99, 235))
    
    card_w, card_h = 1720, 680
    cx = (WIDTH - card_w) // 2
    cy = 205
    draw.rounded_rectangle([cx, cy, cx + card_w, cy + card_h], radius=16, fill=(255, 255, 255), outline=(203, 213, 225), width=2)
    
    # 截屏选区模拟
    draw.rounded_rectangle([cx + 60, cy + 50, cx + card_w - 60, cy + card_h - 150], radius=8, outline=(0, 120, 215), width=2)
    
    # 动态拉伸矩形 (带呼吸微光)
    rect_grow = int(math.sin(t * 4.0) * 15)
    draw.rounded_rectangle([cx + 100, cy + 90, cx + 580 + rect_grow, cy + 280], radius=4, outline=(235, 30, 30), width=4)
    draw.text((cx + 120, cy + 110), "动态拉伸矩形标注 (4px / 鲜红)", font=font_body_bold, fill=(235, 30, 30))
    
    # 动态旋转指向箭头
    arrow_end_x = cx + 960 + int(math.cos(t * 3.0) * 25)
    arrow_end_y = cy + 150 + int(math.sin(t * 3.0) * 20)
    draw.line([(cx + 680, cy + 290), (arrow_end_x, arrow_end_y)], fill=(250, 140, 22), width=5)
    draw.polygon([(arrow_end_x, arrow_end_y), (arrow_end_x - 30, arrow_end_y + 15), (arrow_end_x - 15, arrow_end_y + 35)], fill=(250, 140, 22))
    draw.text((cx + 740, cy + 210), "实时旋转指向箭头", font=font_body_bold, fill=(250, 140, 22))
    
    # 原位打字输入框 (Snipaste / PixPin 风格)
    draw.rounded_rectangle([cx + 100, cy + 340, cx + 800, cy + 405], radius=6, fill=(20, 20, 20, 240), outline=(235, 30, 30), width=2)
    draw.text((cx + 125, cy + 356), "原位打字所见即所得 | 直接按 Enter 固化提交", font=font_body_bold, fill=(235, 30, 30))
    
    # 底部浮动工具栏 (带粗细选择与色谱)
    tb_w = 780
    tb_x = cx + card_w - tb_w - 60
    tb_y = cy + card_h - 130
    draw.rounded_rectangle([tb_x, tb_y, tb_x + tb_w, tb_y + 90], radius=10, fill=(38, 38, 38), outline=(60, 60, 60), width=1)
    
    tools = ["▢", "➔", "✎", "T", "▚", "🔤", "↺", "📌", "💾", "✓"]
    for i, t_sym in enumerate(tools):
        bx = tb_x + 18 + i * 58
        by = tb_y + 10
        draw.rounded_rectangle([bx, by, bx + 48, by + 36], radius=4, fill=(50, 50, 50))
        draw.text((bx + 14, by + 4), t_sym, font=font_body_bold, fill=(240, 240, 240))
        
    draw.text((tb_x + 25, tb_y + 55), "粗细:  • 细(2px)  ● 中(4px)  ⬤ 粗(7px)   |   色谱: 🔴 🟠 🟡 🟢 🔵 🟣 ⚪", font=font_body, fill=(220, 220, 220))
    
    draw_clean_subtitle(draw, "矩形、箭头实时旋转拉伸跟随，原位直接打字，三档粗细与色谱自由切换！")
    return img

# 场景 5: 置顶贴图与快捷键录制 (撑满 1720px)
def render_s5(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENES[4]["title"]
    t_box = font_title.getbbox(title)
    draw.text(((WIDTH - (t_box[2] - t_box[0])) // 2, 70), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENES[4]["sub"]
    s_box = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_box[2] - s_box[0])) // 2, 145), sub, font=font_sub_title, fill=(37, 99, 235))
    
    card_w = 840
    card_h = 670
    x1 = (WIDTH - (2 * card_w + 40)) // 2
    x2 = x1 + card_w + 40
    cy = 205
    
    # 1. 置顶贴图卡片
    draw.rounded_rectangle([x1, cy, x1 + card_w, cy + card_h], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x1 + 40, cy + 35), "📌 独立置顶贴图窗 (Pin Window)", font=font_card_h, fill=(30, 58, 138))
    
    draw.rounded_rectangle([x1 + 40, cy + 100, x1 + card_w - 40, cy + 420], radius=12, fill=(241, 245, 249), outline=(59, 130, 246), width=2)
    draw.text((x1 + 60, cy + 130), "任意截屏选区一键置顶浮动桌面", font=font_body_bold, fill=(15, 23, 42))
    draw.text((x1 + 60, cy + 190), "• 滚轮无极平滑放大 / 缩小", font=font_body, fill=(51, 65, 85))
    draw.text((x1 + 60, cy + 245), "• Ctrl + 滚轮动态调节窗口透明度", font=font_body, fill=(51, 65, 85))
    draw.text((x1 + 60, cy + 300), "• 右键菜单随时按 Ctrl+O 二次文字提取！", font=font_body_bold, fill=(37, 99, 235))
    draw.text((x1 + 60, cy + 355), "• 双击或按 Esc 快速关闭贴图", font=font_body, fill=(51, 65, 85))
    
    draw.text((x1 + 40, cy + 460), "彻底解决日常办公对比数据、抄写代码来回切屏的痛苦！", font=font_body, fill=(100, 116, 139))
    
    # 2. 快捷键设置卡片
    draw.rounded_rectangle([x2, cy, x2 + card_w, cy + card_h], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x2 + 40, cy + 35), "⚙️ 快捷键自由录制偏好设置", font=font_card_h, fill=(30, 58, 138))
    
    draw.rounded_rectangle([x2 + 40, cy + 100, x2 + card_w - 40, cy + 420], radius=12, fill=(248, 250, 252), outline=(203, 213, 225), width=1)
    draw.text((x2 + 60, cy + 130), "全局截屏快捷键自由配置：", font=font_body_bold, fill=(15, 23, 42))
    
    # 模拟快捷键录制框
    draw.rounded_rectangle([x2 + 60, cy + 185, x2 + card_w - 60, cy + 260], radius=8, fill=(255, 255, 255), outline=(37, 99, 235), width=2)
    draw.text((x2 + 80, cy + 208), "当前热键:  F1  (点击输入框并在键盘敲键直接修改)", font=font_body_bold, fill=(37, 99, 235))
    
    draw.text((x2 + 60, cy + 295), "✓ 支持 F1 ~ F12、Alt+A、Ctrl+Shift+A 任意组合", font=font_body, fill=(51, 65, 85))
    draw.text((x2 + 60, cy + 345), "✓ 托盘随时修改，无需重启，保存即刻热生效！", font=font_body_bold, fill=(16, 185, 129))
    
    draw.text((x2 + 40, cy + 460), "配置跨重启持久化，完美贴合每个人独有的按键习惯！", font=font_body, fill=(100, 116, 139))
    
    draw_clean_subtitle(draw, "置顶贴图随时按 Ctrl+O 二次提取文字，托盘随时自由录制修改快捷键！")
    return img

# 场景 6: 结尾号召与绿色免安装 (撑满 1720px)
def render_s6(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENES[5]["title"]
    t_box = font_title.getbbox(title)
    draw.text(((WIDTH - (t_box[2] - t_box[0])) // 2, 80), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENES[5]["sub"]
    s_box = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (s_box[2] - s_box[0])) // 2, 160), sub, font=font_sub_title, fill=(37, 99, 235))
    
    card_w, card_h = 1400, 580
    cx = (WIDTH - card_w) // 2
    cy = 230
    draw.rounded_rectangle([cx, cy, cx + card_w, cy + card_h], radius=20, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    
    draw.text((cx + 80, cy + 60), "📦 完整免安装便携包：EvanOCR-v1.0.0-windows-x64.zip", font=font_card_h, fill=(30, 58, 138))
    
    details = [
        "• 仅 20 多兆轻巧体积，解压即开即用，无需任何繁琐环境配置",
        "• 100% 纯本地离线运行，绝密工作数据与代码绝对安全无忧",
        "• 彻底替代 Snipaste / PixPin 的全功能截贴图文字提取利器",
        "• 欢迎在评论区提出更多宝贵功能需求与建议！"
    ]
    for i, d in enumerate(details):
        draw.text((cx + 80, cy + 140 + i * 55), d, font=font_body, fill=(51, 65, 85))
        
    # 三连求赞高光卡片
    draw.rounded_rectangle([cx + 80, cy + 390, cx + card_w - 80, cy + 500], radius=14, fill=(254, 240, 138), outline=(234, 179, 8), width=2)
    draw.text((cx + 160, cy + 425), "求一键三连支持！源码与绿色版下载地址在置顶评论 ↓", font=font_card_h, fill=(133, 77, 14))
    
    draw_clean_subtitle(draw, "单文件解压即用！求一键三连支持，下载地址在置顶评论，快去体验吧！")
    return img

async def main():
    durations = await generate_voice_tracks()
    final_audio, total_duration = mix_audio_tracks(durations)
    
    output_mp4 = "EvanOCR_B站专业宣传视频_1080P60.mp4"
    print(f"开始渲染并压制输出: {output_mp4} ...")
    
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
    
    time_boundaries = []
    accum = 0.0
    for d in durations:
        accum += d
        time_boundaries.append(accum)
        
    total_frames = int(total_duration * FPS)
    print(f"正在渲染 {total_frames} 帧 (1080P 30FPS)...")
    
    for f_idx in range(total_frames):
        current_time = f_idx / FPS
        
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
            print(f"渲染进度: {f_idx // FPS}s / {int(total_duration)}s")
            
    proc.stdin.close()
    proc.wait()
    print(f"Finished successfully: {os.path.abspath(output_mp4)}")

if __name__ == "__main__":
    asyncio.run(main())
