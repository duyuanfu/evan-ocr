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
VOICE = "zh-CN-YunxiNeural"  # 微软自然高拟真活力男声
VOICE_RATE = "+20%"

# 精心规划的 7 大分镜脚本与按句同步字幕 (解决字幕不同步与不完整问题)
SCENE_SCRIPTS = [
    # 场景 1: 华丽开门见山 (Hook & Intro)
    {
        "scene_id": 1,
        "title": "EvanOCR 屏幕生产力神器",
        "sub": "C++20 & Qt6 纯原生打造 · Windows 10/11 极速离线硬件加速",
        "sentences": [
            "告别动辄上百兆的模型包与收费限制！",
            "今天给大家推荐一款真正称手的全新截贴图文字提取神器——EvanOCR！",
            "基于 C++20 和 Qt6 纯原生打造，极速流畅，完全免费开源！"
        ]
    },
    # 场景 2: 截屏交互与工具栏全览 (NEW SCENE - 截图工具界面与功能总览)
    {
        "scene_id": 2,
        "title": "全能截屏交互 · 智能吸附工具栏",
        "sub": "像素级放大镜拾色 · 选区自适应工具避让算法",
        "sentences": [
            "按下快捷键瞬间唤醒！支持单像素网格放大镜与十六进制精准取色。",
            "自带自适应智能吸附工具栏，标注、贴图、OCR 一应俱全！"
        ]
    },
    # 场景 3: 独家左右分栏 1:1 对照 OCR
    {
        "scene_id": 3,
        "title": "首创「左右分栏」1:1 像素对照排版",
        "sub": "原图天蓝定位框精准核对 · 告别错漏字核对盲区",
        "sentences": [
            "截屏后按下字母 O，全屏底图瞬间自动退出，弹出独家左右分栏对照窗口！",
            "左侧原图蓝框精准标记文字，右侧不仅能自动合并段落，还能一键消除汉字间多余空格！"
        ]
    },
    # 场景 4: 直击痛点 · 为什么比 Snipaste 与 PixPin 更爽？
    {
        "scene_id": 4,
        "title": "为什么比 Snipaste 与 PixPin 更实用？",
        "sub": "直击传统截屏工具核心痛点 · 兼具轻量与强大",
        "sentences": [
            "对比 Snipaste 免费版没有离线文字识别，配置云端 API 极其繁琐；",
            "而 PixPin 捆绑上百兆庞大模型包，后台常驻内存大，排版断行空格多。",
            "EvanOCR 直接调用系统原生硬件加速，0MB 额外模型，毫秒秒出，100% 本地安全！"
        ]
    },
    # 场景 5: 丝滑矢量标注体验
    {
        "scene_id": 5,
        "title": "丝滑矢量标注 · 实时所见即所得",
        "sub": "矩形与箭头拖拽拉伸实时旋转跟随 · 原位打字输入",
        "sentences": [
            "标注体验极其丝滑！矩形、箭头支持实时拖拽拉伸旋转跟随预览；",
            "文字标注可直接在原图原地打字，支持三档粗细与全色谱自由调节！"
        ]
    },
    # 场景 6: 置顶贴图二次 OCR 与自定义热键
    {
        "scene_id": 6,
        "title": "独立置顶贴图窗 · 快捷键自由录制",
        "sub": "右键按 Ctrl+O 二次文字提取 · 托盘随时自由配置热键",
        "sentences": [
            "置顶贴图窗支持无极滚轮缩放与透明度调节，右键随时按 Ctrl+O 二次提取文字！",
            "托盘菜单还能自由录制快捷键，F1、F4 随心配置，保存即刻生效！"
        ]
    },
    # 场景 7: 结尾号召与绿色免安装
    {
        "scene_id": 7,
        "title": "开箱即用 · 纯绿色便携免安装",
        "sub": "单压缩包仅 20 多兆 · 零后台常驻广告",
        "sentences": [
            "单压缩包仅 20 多兆，解压即开即用！纯离线运行无广告。",
            "喜欢的小伙伴求一键三连支持一下，开源打包成品就在置顶评论，快去体验吧！"
        ]
    }
]

# 字体加载
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

font_hero = get_font(60, bold=True)
font_title = get_font(52, bold=True)
font_sub_title = get_font(28, bold=False)
font_card_h = get_font(32, bold=True)
font_body_bold = get_font(24, bold=True)
font_body = get_font(22, bold=False)
font_small = get_font(18, bold=False)
font_badge = get_font(18, bold=True)
font_sub = get_font(34, bold=True)

# 1. 逐句生成 Edge-TTS 语音并精确记录时间戳 (100% 解决音字不同步)
async def generate_audio_pipeline():
    os.makedirs("audio_v4", exist_ok=True)
    sentence_timeline = []
    current_time = 0.0
    wav_files = []
    
    print("正在生成自然真人语音 (Edge-TTS zh-CN-YunxiNeural)...")
    sentence_idx = 0
    
    for sc_idx, sc in enumerate(SCENE_SCRIPTS):
        scene_id = sc["scene_id"]
        for s_text in sc["sentences"]:
            mp3_path = f"audio_v4/sent_{sentence_idx}.mp3"
            wav_path = f"audio_v4/sent_{sentence_idx}.wav"
            
            # 使用 Edge-TTS 生成超自然语音
            tts = edge_tts.Communicate(s_text, VOICE, rate=VOICE_RATE)
            await tts.save(mp3_path)
            
            subprocess.run([FFMPEG_PATH, "-y", "-i", mp3_path, "-ac", "1", "-ar", "24000", wav_path],
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
            
            with wave.open(wav_path, "rb") as wf:
                dur = wf.getnframes() / float(wf.getframerate())
                dur += 0.35  # 句间微顿
                
            sentence_timeline.append({
                "scene_id": scene_id,
                "text": s_text,
                "start": current_time,
                "end": current_time + dur,
                "duration": dur,
                "file": wav_path
            })
            wav_files.append(wav_path)
            current_time += dur
            sentence_idx += 1
            
    print(f"共生成 {len(sentence_timeline)} 句配音，总时长: {current_time:.2f}s")
    
    # 拼接所有人声音频
    concat_txt = "audio_v4/concat.txt"
    with open(concat_txt, "w", encoding="utf-8") as f:
        for p in wav_files:
            abs_p = os.path.abspath(p).replace("\\", "/")
            f.write(f"file '{abs_p}'\n")
            
    merged_voice = "audio_v4/voice_all.wav"
    subprocess.run([FFMPEG_PATH, "-y", "-f", "concat", "-safe", "0", "-i", concat_txt, "-c", "copy", merged_voice],
                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    
    # 混音背景音乐 BGM
    final_audio = "audio_v4/final_mix.wav"
    bgm_file = "bilibili_bgm.mp3"
    if os.path.exists(bgm_file):
        mix_filter = (
            f"[1:a]aloop=loop=-1:size=2e+09,atrim=0:{current_time},volume=0.07[bgm];"
            f"[0:a]volume=1.25[vox];"
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
        
    return sentence_timeline, final_audio, current_time

# 优雅浅色背景
def draw_bg(draw):
    for y in range(0, HEIGHT, 4):
        ratio = y / HEIGHT
        r = int(246 - ratio * 8)
        g = int(249 - ratio * 6)
        b = int(253 - ratio * 4)
        draw.rectangle([0, y, WIDTH, y + 4], fill=(r, g, b))
    # 点阵微纹理
    for x in range(30, WIDTH, 60):
        for y in range(30, HEIGHT, 60):
            draw.ellipse([x-1, y-1, x+1, y+1], fill=(226, 232, 240))

# 无黑底大气同步字幕 (自适应分行与居中阴影，绝不超出或遮挡)
def draw_synced_subtitle(draw, text):
    if not text:
        return
    y = HEIGHT - 85
    # 如果字幕过长，自动拆分为两行或截短
    if len(text) > 34:
        mid = len(text) // 2
        lines = [text[:mid], text[mid:]]
        line_h = 36
        start_y = HEIGHT - 100
        for i, l in enumerate(lines):
            b = font_sub.getbbox(l)
            lx = (WIDTH - (b[2] - b[0])) // 2
            draw.text((lx + 2, start_y + i * line_h + 2), l, font=font_sub, fill=(15, 23, 42, 50))
            draw.text((lx, start_y + i * line_h), l, font=font_sub, fill=(15, 23, 42))
    else:
        b = font_sub.getbbox(text)
        tw = b[2] - b[0]
        x = (WIDTH - tw) // 2
        draw.text((x + 2, y + 2), text, font=font_sub, fill=(15, 23, 42, 50))
        draw.text((x, y), text, font=font_sub, fill=(15, 23, 42))

# 绘制完全解决“图标显示为空格”的专业矢量悬浮工具栏 (彻底修复图片1的问题)
def draw_realistic_toolbar(draw, tb_x, tb_y, tb_w=820, tb_h=96):
    # 工具栏深色圆角微拟物外壳
    draw.rounded_rectangle([tb_x, tb_y, tb_x + tb_w, tb_y + tb_h], radius=10, fill=(38, 38, 42), outline=(60, 60, 68), width=1)
    
    # 10 个独立按钮 (用矢量绘制替代容易缺字的 unicode)
    btn_w = 46
    btn_h = 36
    start_bx = tb_x + 16
    spacing = 54
    
    # 1. 矩形
    bx = start_bx
    by = tb_y + 10
    draw.rounded_rectangle([bx, by, bx + btn_w, by + btn_h], radius=4, fill=(50, 50, 56))
    draw.rounded_rectangle([bx + 12, by + 9, bx + btn_w - 12, by + btn_h - 9], radius=3, outline=(240, 240, 240), width=2)
    
    # 2. 箭头
    bx += spacing
    draw.rounded_rectangle([bx, by, bx + btn_w, by + btn_h], radius=4, fill=(50, 50, 56))
    draw.line([(bx + 12, by + 24), (bx + btn_w - 12, by + 12)], fill=(240, 240, 240), width=2)
    draw.polygon([(bx + btn_w - 12, by + 12), (bx + btn_w - 20, by + 11), (bx + btn_w - 13, by + 19)], fill=(240, 240, 240))
    
    # 3. 自由画笔
    bx += spacing
    draw.rounded_rectangle([bx, by, bx + btn_w, by + btn_h], radius=4, fill=(50, 50, 56))
    draw.line([(bx + 14, by + 26), (bx + 28, by + 12)], fill=(240, 240, 240), width=3)
    draw.ellipse([bx + 26, by + 10, bx + 32, by + 16], fill=(240, 240, 240))
    
    # 4. 文字标注 T
    bx += spacing
    draw.rounded_rectangle([bx, by, bx + btn_w, by + btn_h], radius=4, fill=(50, 50, 56))
    draw.text((bx + 16, by + 6), "T", font=font_body_bold, fill=(240, 240, 240))
    
    # 5. 马赛克遮罩 (4格黑白交错像素)
    bx += spacing
    draw.rounded_rectangle([bx, by, bx + btn_w, by + btn_h], radius=4, fill=(50, 50, 56))
    draw.rectangle([bx + 14, by + 10, bx + 22, by + 18], fill=(240, 240, 240))
    draw.rectangle([bx + 24, by + 10, bx + 32, by + 18], fill=(120, 120, 120))
    draw.rectangle([bx + 14, by + 20, bx + 22, by + 28], fill=(120, 120, 120))
    draw.rectangle([bx + 24, by + 20, bx + 32, by + 28], fill=(240, 240, 240))
    
    # 6. OCR 文字识别 (醒目字样)
    bx += spacing
    draw.rounded_rectangle([bx, by, bx + btn_w, by + btn_h], radius=4, fill=(37, 99, 235))
    draw.text((bx + 8, by + 8), "OCR", font=font_badge, fill=(255, 255, 255))
    
    # 7. 撤销上一步
    bx += spacing
    draw.rounded_rectangle([bx, by, bx + btn_w, by + btn_h], radius=4, fill=(50, 50, 56))
    draw.arc([bx + 14, by + 10, bx + 32, by + 26], start=45, end=270, fill=(240, 240, 240), width=2)
    draw.polygon([(bx + 14, by + 8), (bx + 14, by + 16), (bx + 20, by + 12)], fill=(240, 240, 240))
    
    # 8. 置顶贴图
    bx += spacing
    draw.rounded_rectangle([bx, by, bx + btn_w, by + btn_h], radius=4, fill=(50, 50, 56))
    draw.ellipse([bx + 18, by + 10, bx + 28, by + 20], fill=(240, 240, 240))
    draw.line([(bx + 23, by + 20), (bx + 23, by + 28)], fill=(240, 240, 240), width=2)
    
    # 9. 保存文件
    bx += spacing
    draw.rounded_rectangle([bx, by, bx + btn_w, by + btn_h], radius=4, fill=(50, 50, 56))
    draw.rounded_rectangle([bx + 14, by + 10, bx + 32, by + 27], radius=2, fill=(240, 240, 240))
    draw.rectangle([bx + 18, by + 12, bx + 28, by + 18], fill=(50, 50, 56))
    
    # 10. 完成复制
    bx += spacing
    draw.rounded_rectangle([bx, by, bx + btn_w, by + btn_h], radius=4, fill=(16, 185, 129))
    draw.line([(bx + 15, by + 19), (bx + 21, by + 25)], fill=(255, 255, 255), width=3)
    draw.line([(bx + 21, by + 25), (bx + 32, by + 12)], fill=(255, 255, 255), width=3)
    
    # 第二行：粗细档位与彩色圆点调色板 (纯矢量绘制，绝不出现缺失方块！)
    text_y = tb_y + 58
    draw.text((start_bx, text_y), "粗细:", font=font_badge, fill=(200, 200, 210))
    
    # 细 2px
    draw.ellipse([start_bx + 55, text_y + 8, start_bx + 59, text_y + 12], fill=(240, 240, 240))
    draw.text((start_bx + 66, text_y), "细(2px)", font=font_badge, fill=(220, 220, 230))
    
    # 中 4px (激活状态)
    draw.ellipse([start_bx + 145, text_y + 6, start_bx + 153, text_y + 14], fill=(56, 189, 248))
    draw.text((start_bx + 160, text_y), "中(4px)", font=font_badge, fill=(56, 189, 248))
    
    # 粗 7px
    draw.ellipse([start_bx + 240, text_y + 4, start_bx + 252, text_y + 16], fill=(240, 240, 240))
    draw.text((start_bx + 260, text_y), "粗(7px)", font=font_badge, fill=(220, 220, 230))
    
    # 分隔竖线
    draw.line([(start_bx + 345, text_y + 2), (start_bx + 345, text_y + 22)], fill=(80, 80, 90), width=1)
    
    # 色谱
    draw.text((start_bx + 365, text_y), "调色板:", font=font_badge, fill=(200, 200, 210))
    
    # 真正填色的圆点
    colors = [
        (239, 68, 68),    # 红
        (249, 115, 22),   # 橙
        (234, 179, 8),    # 黄
        (34, 197, 94),    # 绿
        (37, 99, 235),    # 蓝
        (168, 85, 247),   # 紫
        (255, 255, 255),  # 白
        (20, 20, 20)      # 黑
    ]
    for i, col in enumerate(colors):
        dot_x = start_bx + 440 + i * 32
        draw.ellipse([dot_x, text_y + 3, dot_x + 18, text_y + 21], fill=col, outline=(100, 100, 110), width=1)
        if i == 0:  # 默认选中红
            draw.ellipse([dot_x - 3, text_y, dot_x + 21, text_y + 24], outline=(255, 255, 255), width=2)

# =========================================================================
# 7 个场景的专业渲染逻辑 (严格防止任何文字超出，饱满撑满宽屏，加入呼吸动画)
# =========================================================================

# 场景 1: 震撼吸睛首页 (开门见山 · 华丽科技光晕)
def render_scene_1(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    # 华丽呼吸渐变光环 (背景动态光晕)
    pulse = math.sin(t * 3.0) * 0.1 + 0.95
    aura_r = int(380 * pulse)
    glow_box = [WIDTH // 2 - aura_r, 400 - aura_r, WIDTH // 2 + aura_r, 400 + aura_r]
    draw.ellipse(glow_box, fill=(224, 242, 254, 140))
    
    # 顶部金牌宣传标
    tag_text = "★ 2026 年度全新 Windows 原生截贴图生产力神器 ★"
    b = font_badge.getbbox(tag_text)
    tw = b[2] - b[0]
    draw.rounded_rectangle([WIDTH//2 - tw//2 - 20, 45, WIDTH//2 + tw//2 + 20, 85], radius=20, fill=(239, 246, 255), outline=(147, 197, 253), width=1)
    draw.text((WIDTH//2 - tw//2, 54), tag_text, font=font_badge, fill=(30, 58, 138))
    
    # 主大标题 (超醒目吸睛)
    title = "告别臃肿模型与收费限制！"
    b = font_hero.getbbox(title)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 105), title, font=font_hero, fill=(15, 23, 42))
    
    sub = "EvanOCR - C++20 & Qt6 纯原生打造 · Windows 原生硬件加速离线秒出"
    b = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 185), sub, font=font_sub_title, fill=(37, 99, 235))
    
    # 宽幅大卡片 (1720px，左右各 100px 黄金留白，绝不太空)
    cw, ch = 1720, 640
    cx = (WIDTH - cw) // 2
    cy = 245
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=20, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    
    # 左侧：大图标与版本号
    lw = 450
    draw.rounded_rectangle([cx + 25, cy + 25, cx + lw, cy + ch - 25], radius=16, fill=(248, 250, 252))
    if os.path.exists("resources/app_icon_256.png"):
        icon = Image.open("resources/app_icon_256.png").convert("RGBA")
        icon = icon.resize((230, 230), Image.Resampling.LANCZOS)
        img.paste(icon, (cx + 135, cy + 120), icon)
    draw.text((cx + 125, cy + 390), "EvanOCR v1.0", font=font_card_h, fill=(15, 23, 42))
    draw.text((cx + 90, cy + 445), "零体积 · 毫秒级 · 纯离线", font=font_body_bold, fill=(37, 99, 235))
    draw.text((cx + 80, cy + 485), "完全开源免费 · 单文件解压即用", font=font_body, fill=(100, 116, 139))
    
    # 右侧：6 大王炸特性卡片网格 (精简文字，杜绝任何溢出)
    rx = cx + lw + 40
    rw = cw - lw - 70
    draw.text((rx, cy + 40), "为什么选择 EvanOCR 作为日常主力？", font=font_card_h, fill=(30, 58, 138))
    
    features = [
        ("⚡ 0MB 模型！Win 原生硬件加速", "直接调用 Win10/11 WinRT 底层，0 外部模型"),
        ("📷 首创「左右分栏 1:1 对照」", "原图蓝框精准标记定位，文字校对零死角"),
        ("📝 智能排版清洗", "支持一键段落合并(¶)与消除汉字间空格(␣)"),
        ("🎨 丝滑所见即所得矢量标注", "矩形/箭头实时拖拽拉伸跟随，原位直接打字"),
        ("📌 独立置顶贴图窗 (Pin Window)", "无极缩放调节，右键随时按 Ctrl+O 二次提取"),
        ("⚙️ 完全自定义全局快捷键", "托盘菜单自由录制热键，纯绿色免安装")
    ]
    
    for i, (head, desc) in enumerate(features):
        c = i % 2
        r = i // 2
        bx = rx + c * 590
        by = cy + 110 + r * 155
        bw = 560
        bh = 135
        draw.rounded_rectangle([bx, by, bx + bw, by + bh], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
        # 图标徽章
        draw.rounded_rectangle([bx + 20, by + 22, bx + 50, by + 52], radius=6, fill=(37, 99, 235))
        draw.line([(bx + 27, by + 37), (bx + 33, by + 43)], fill=(255, 255, 255), width=3)
        draw.line([(bx + 33, by + 43), (bx + 44, by + 30)], fill=(255, 255, 255), width=3)
        # 文字 (保证单行内绝不超宽)
        draw.text((bx + 62, by + 22), head, font=font_body_bold, fill=(15, 23, 42))
        draw.text((bx + 62, by + 65), desc, font=font_body, fill=(100, 116, 139))
        
    return img

# 场景 2: 全能截屏交互与工具栏全览 (NEW SCENE - 满足第 2 点优化需求)
def render_scene_2(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENE_SCRIPTS[1]["title"]
    b = font_title.getbbox(title)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 60), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENE_SCRIPTS[1]["sub"]
    b = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 135), sub, font=font_sub_title, fill=(37, 99, 235))
    
    cw, ch = 1720, 680
    cx = (WIDTH - cw) // 2
    cy = 195
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=20, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    
    # 模拟真实截屏覆盖画面 (高质感现代桌面取景)
    vw, vh = 1100, 480
    vx = cx + 50
    vy = cy + 50
    draw.rounded_rectangle([vx, vy, vx + vw, vy + vh], radius=12, fill=(241, 245, 249), outline=(203, 213, 225), width=2)
    
    # 截屏暗色微遮罩与选区镂空
    draw.rounded_rectangle([vx + 100, vy + 60, vx + vw - 100, vy + vh - 100], radius=8, outline=(0, 120, 215), width=2)
    
    # 8 个控制手柄
    pts = [
        (vx + 100, vy + 60), (vx + vw // 2, vy + 60), (vx + vw - 100, vy + 60),
        (vx + vw - 100, vy + vh // 2), (vx + vw - 100, vy + vh - 100),
        (vx + vw // 2, vy + vh - 100), (vx + 100, vy + vh - 100), (vx + 100, vy + vh // 2)
    ]
    for px, py in pts:
        draw.rectangle([px - 4, py - 4, px + 4, py + 4], fill=(255, 255, 255), outline=(0, 120, 215), width=1)
        
    # 放大镜挂件模拟 (左上角)
    mag_x = vx + 130
    mag_y = vy + 90
    draw.rounded_rectangle([mag_x, mag_y, mag_x + 180, mag_y + 110], radius=8, fill=(24, 24, 28), outline=(60, 60, 70), width=1)
    draw.text((mag_x + 15, mag_y + 12), "像素放大镜 (15x15)", font=font_small, fill=(200, 200, 210))
    draw.text((mag_x + 15, mag_y + 42), "HEX: #0078D7", font=font_body_bold, fill=(56, 189, 248))
    draw.text((mag_x + 15, mag_y + 75), "RGB: (0, 120, 215)", font=font_small, fill=(220, 220, 220))
    
    # 在选区下方紧贴真实的专业矢量悬浮工具栏 (彻底消除图片1的方块字符)
    draw_realistic_toolbar(draw, vx + vw - 860, vy + vh - 90, tb_w=820, tb_h=96)
    
    # 右侧功能介绍看板 (3 大核心能力卡片)
    rx = vx + vw + 40
    rw = cw - vw - 120
    draw.text((rx, cy + 50), "一体化工具栏全套功能", font=font_card_h, fill=(30, 58, 138))
    
    tool_cards = [
        ("🔤 一键智能文字提取 (OCR)", "点击或按字母 O，瞬间退出底图并打开左右对照排版"),
        ("📌 独立置顶贴图 (Pin)", "将当前选区转为置顶浮动窗，支持滚轮缩放与二次 OCR"),
        ("🎨 专业矢量标注工具箱", "矩形、箭头实时旋转跟随，原位打字、马赛克全备")
    ]
    for i, (th, td) in enumerate(tool_cards):
        ty = cy + 120 + i * 155
        draw.rounded_rectangle([rx, ty, rx + rw, ty + 135], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
        draw.text((rx + 25, ty + 24), th, font=font_body_bold, fill=(15, 23, 42))
        draw.text((rx + 25, ty + 68), td, font=font_body, fill=(100, 116, 139))
        
    return img

# 场景 3: 左右分栏 1:1 像素对照排版实机
def render_scene_3(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENE_SCRIPTS[2]["title"]
    b = font_title.getbbox(title)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 60), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENE_SCRIPTS[2]["sub"]
    b = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 135), sub, font=font_sub_title, fill=(37, 99, 235))
    
    cw, ch = 1720, 680
    cx = (WIDTH - cw) // 2
    cy = 195
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=16, fill=(241, 245, 249), outline=(203, 213, 225), width=2)
    
    # 顶部标题栏
    draw.rounded_rectangle([cx, cy, cx + cw, cy + 50], radius=16, fill=(226, 232, 240))
    draw.text((cx + 25, cy + 14), "文本识别提取 - EvanOCR (左右对照模式)", font=font_body_bold, fill=(30, 41, 59))
    
    half_w = (cw - 30) // 2
    
    # 左侧原图视口
    lx = cx + 10
    ly = cy + 60
    lh = ch - 130
    draw.rounded_rectangle([lx, ly, lx + half_w, ly + lh], radius=10, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((lx + 25, ly + 15), "📷 原图对照 (1:1 像素物理核对)", font=font_card_h, fill=(15, 23, 42))
    
    # 原图模拟文本框与天蓝定位框
    boxes = [
        (ly + 75, "EvanOCR 屏幕生产力桌面工具"),
        (ly + 155, "C++20 & Qt6 原生打造 · Windows 原生硬件加速"),
        (ly + 235, "独家左右分栏 1:1 像素对照排版设计"),
        (ly + 315, "毫秒级纯离线提取 · 零网络数据上传")
    ]
    for by, btxt in boxes:
        draw.rounded_rectangle([lx + 30, by, lx + half_w - 30, by + 58], radius=6, fill=(239, 246, 255), outline=(37, 99, 235), width=2)
        draw.text((lx + 50, by + 16), btxt, font=font_body_bold, fill=(30, 58, 138))
        
    # 动态往复激光扫描线
    scan_offset = int((math.sin(t * 3.0) * 0.5 + 0.5) * (lh - 130))
    scan_y = ly + 75 + scan_offset
    draw.line([(lx + 30, scan_y), (lx + half_w - 30, scan_y)], fill=(56, 189, 248), width=3)
    draw.line([(lx + 30, scan_y + 1), (lx + half_w - 30, scan_y + 1)], fill=(96, 165, 250, 90), width=6)
    
    draw.text((lx + 30, ly + 420), "✓ 天蓝色包围框实时对应，核对错漏字零死角！", font=font_body_bold, fill=(37, 99, 235))
    
    # 右侧提取文字结果
    rx = lx + half_w + 10
    draw.rounded_rectangle([rx, ly, rx + half_w, ly + lh], radius=10, fill=(255, 255, 255), outline=(226, 232, 240), width=1)
    draw.text((rx + 25, ly + 15), "📝 提取文字结果", font=font_card_h, fill=(15, 23, 42))
    
    # 两个核心按钮
    draw.rounded_rectangle([rx + 440, ly + 12, rx + 610, ly + 50], radius=6, fill=(239, 246, 255), outline=(37, 99, 235), width=2)
    draw.text((rx + 460, ly + 18), "¶ 合并段落 [开]", font=font_badge, fill=(37, 99, 235))
    
    draw.rounded_rectangle([rx + 630, ly + 12, rx + 800, ly + 50], radius=6, fill=(239, 246, 255), outline=(37, 99, 235), width=2)
    draw.text((rx + 650, ly + 18), "␣ 清除空格 [开]", font=font_badge, fill=(37, 99, 235))
    
    sample_text = (
        "EvanOCR 屏幕生产力桌面工具\n\n"
        "C++20 & Qt6 原生打造 · Windows 原生硬件加速。\n\n"
        "独家左右分栏 1:1 像素对照排版设计，彻底消除孤立折行与汉字间多余空格！"
    )
    for li, line in enumerate(sample_text.split("\n")):
        draw.text((rx + 30, ly + 85 + li * 40), line, font=font_body, fill=(15, 23, 42))
        
    # 底部主操作条
    draw.text((cx + 30, cy + ch - 48), "字数: 86  |  行数: 3", font=font_body, fill=(100, 116, 139))
    draw.rounded_rectangle([cx + cw - 310, cy + ch - 55, cx + cw - 30, cy + ch - 15], radius=8, fill=(37, 99, 235))
    draw.text((cx + cw - 265, cy + ch - 48), "📋 复制全部文本", font=font_body_bold, fill=(255, 255, 255))
    
    return img

# 场景 4: 直击痛点 · 竞品对比
def render_scene_4(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENE_SCRIPTS[3]["title"]
    b = font_title.getbbox(title)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 70), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENE_SCRIPTS[3]["sub"]
    b = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 145), sub, font=font_sub_title, fill=(71, 85, 105))
    
    col_w = 540
    col_h = 670
    c_start_x = (WIDTH - (3 * col_w + 2 * 40)) // 2
    cy = 205
    
    # 1. Snipaste
    x1 = c_start_x
    draw.rounded_rectangle([x1, cy, x1 + col_w, cy + col_h], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x1 + 35, cy + 35), "Snipaste", font=font_card_h, fill=(239, 68, 68))
    draw.text((x1 + 35, cy + 85), "老牌截贴图，但 OCR 极其不便", font=font_body, fill=(100, 116, 139))
    draw.line([(x1 + 35, cy + 125), (x1 + col_w - 35, cy + 125)], fill=(241, 245, 249), width=2)
    
    p1 = [
        "❌ 免费版没有离线 OCR 文字提取",
        "❌ 提取文字需复杂配置第三方 API",
        "❌ 贴图后无法对图中文字二次提取",
        "❌ 标注过程无法实时拉伸动态预览"
    ]
    for i, it in enumerate(p1):
        draw.text((x1 + 35, cy + 160 + i * 115), it, font=font_body_bold, fill=(71, 85, 105))
        
    # 2. PixPin
    x2 = x1 + col_w + 40
    draw.rounded_rectangle([x2, cy, x2 + col_w, cy + col_h], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x2 + 35, cy + 35), "PixPin", font=font_card_h, fill=(249, 115, 22))
    draw.text((x2 + 35, cy + 85), "集成 OCR，但体积庞大排版差", font=font_body, fill=(100, 116, 139))
    draw.line([(x2 + 35, cy + 125), (x2 + col_w - 35, cy + 125)], fill=(241, 245, 249), width=2)
    
    p2 = [
        "❌ 离线需捆绑上百兆庞大模型包",
        "❌ 后台常驻内存开销大，启动滞后",
        "❌ 提取结果大量多余折行与空格",
        "❌ 闭源商业黑盒软件，隐私难保证"
    ]
    for i, it in enumerate(p2):
        draw.text((x2 + 35, cy + 160 + i * 115), it, font=font_body_bold, fill=(71, 85, 105))
        
    # 3. EvanOCR
    x3 = x2 + col_w + 40
    draw.rounded_rectangle([x3, cy, x3 + col_w, cy + col_h], radius=16, fill=(239, 246, 255), outline=(37, 99, 235), width=3)
    draw.text((x3 + 35, cy + 35), "EvanOCR (推荐首选)", font=font_card_h, fill=(37, 99, 235))
    draw.text((x3 + 35, cy + 85), "Windows 10/11 原生硬件加速", font=font_body_bold, fill=(30, 58, 138))
    draw.line([(x3 + 35, cy + 125), (x3 + col_w - 35, cy + 125)], fill=(191, 219, 254), width=2)
    
    p3 = [
        "✓ 0MB 模型！Win 原生硬件加速",
        "✓ 独家左右 1:1 对照，自带段落清洗",
        "✓ 贴图窗口随时按 Ctrl+O 二次提取",
        "✓ 毫秒级极速唤醒，100% 本地隐私"
    ]
    for i, it in enumerate(p3):
        draw.text((x3 + 35, cy + 160 + i * 115), it, font=font_body_bold, fill=(30, 58, 138))
        
    return img

# 场景 5: 矢量标注实机体验
def render_scene_5(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENE_SCRIPTS[4]["title"]
    b = font_title.getbbox(title)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 70), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENE_SCRIPTS[4]["sub"]
    b = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 145), sub, font=font_sub_title, fill=(37, 99, 235))
    
    cw, ch = 1720, 680
    cx = (WIDTH - cw) // 2
    cy = 205
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=16, fill=(255, 255, 255), outline=(203, 213, 225), width=2)
    
    # 选区模拟
    draw.rounded_rectangle([cx + 60, cy + 45, cx + cw - 60, cy + ch - 155], radius=8, outline=(0, 120, 215), width=2)
    
    # 动态拉伸矩形
    rect_grow = int(math.sin(t * 4.0) * 16)
    draw.rounded_rectangle([cx + 100, cy + 85, cx + 580 + rect_grow, cy + 265], radius=4, outline=(235, 30, 30), width=4)
    draw.text((cx + 120, cy + 105), "动态拉伸矩形标注 (4px / 鲜红)", font=font_body_bold, fill=(235, 30, 30))
    
    # 动态旋转指向箭头
    arrow_end_x = cx + 980 + int(math.cos(t * 3.0) * 20)
    arrow_end_y = cy + 140 + int(math.sin(t * 3.0) * 15)
    draw.line([(cx + 700, cy + 280), (arrow_end_x, arrow_end_y)], fill=(250, 140, 22), width=5)
    draw.polygon([(arrow_end_x, arrow_end_y), (arrow_end_x - 28, arrow_end_y + 14), (arrow_end_x - 14, arrow_end_y + 30)], fill=(250, 140, 22))
    draw.text((cx + 760, cy + 200), "实时旋转指向箭头", font=font_body_bold, fill=(250, 140, 22))
    
    # 原位打字输入框
    draw.rounded_rectangle([cx + 100, cy + 320, cx + 780, cy + 380], radius=6, fill=(20, 20, 20, 240), outline=(235, 30, 30), width=2)
    draw.text((cx + 120, cy + 335), "原位打字所见即所得 | 直接按 Enter 固化提交", font=font_body_bold, fill=(235, 30, 30))
    
    # 底部真实的专业矢量工具栏 (绝无缺字)
    draw_realistic_toolbar(draw, cx + cw - 880, cy + ch - 135, tb_w=820, tb_h=96)
    return img

# 场景 6: 置顶贴图与快捷键自由录制
def render_scene_6(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENE_SCRIPTS[5]["title"]
    b = font_title.getbbox(title)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 70), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENE_SCRIPTS[5]["sub"]
    b = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 145), sub, font=font_sub_title, fill=(37, 99, 235))
    
    cw = 840
    ch = 670
    x1 = (WIDTH - (2 * cw + 40)) // 2
    x2 = x1 + cw + 40
    cy = 205
    
    # 1. 置顶贴图卡片
    draw.rounded_rectangle([x1, cy, x1 + cw, cy + ch], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x1 + 40, cy + 35), "📌 独立置顶贴图窗 (Pin Window)", font=font_card_h, fill=(30, 58, 138))
    
    draw.rounded_rectangle([x1 + 40, cy + 100, x1 + cw - 40, cy + 420], radius=12, fill=(241, 245, 249), outline=(59, 130, 246), width=2)
    draw.text((x1 + 60, cy + 130), "任意截屏选区一键置顶浮动桌面", font=font_body_bold, fill=(15, 23, 42))
    draw.text((x1 + 60, cy + 190), "• 滚轮无极平滑放大 / 缩小", font=font_body, fill=(51, 65, 85))
    draw.text((x1 + 60, cy + 245), "• Ctrl + 滚轮动态调节窗口透明度", font=font_body, fill=(51, 65, 85))
    draw.text((x1 + 60, cy + 300), "• 右键菜单随时按 Ctrl+O 二次文字提取！", font=font_body_bold, fill=(37, 99, 235))
    draw.text((x1 + 60, cy + 355), "• 双击或按 Esc 快速关闭贴图", font=font_body, fill=(51, 65, 85))
    
    draw.text((x1 + 40, cy + 460), "彻底解决日常办公对比数据、抄写代码来回切屏的痛苦！", font=font_body, fill=(100, 116, 139))
    
    # 2. 快捷键设置卡片
    draw.rounded_rectangle([x2, cy, x2 + cw, cy + ch], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    draw.text((x2 + 40, cy + 35), "⚙️ 快捷键自由录制偏好设置", font=font_card_h, fill=(30, 58, 138))
    
    draw.rounded_rectangle([x2 + 40, cy + 100, x2 + cw - 40, cy + 420], radius=12, fill=(248, 250, 252), outline=(203, 213, 225), width=1)
    draw.text((x2 + 60, cy + 130), "全局截屏快捷键自由配置：", font=font_body_bold, fill=(15, 23, 42))
    
    draw.rounded_rectangle([x2 + 60, cy + 185, x2 + cw - 60, cy + 260], radius=8, fill=(255, 255, 255), outline=(37, 99, 235), width=2)
    draw.text((x2 + 80, cy + 208), "当前热键:  F1  (点击输入框并在键盘敲键直接修改)", font=font_body_bold, fill=(37, 99, 235))
    
    draw.text((x2 + 60, cy + 295), "✓ 支持 F1 ~ F12、Alt+A、Ctrl+Shift+A 任意组合", font=font_body, fill=(51, 65, 85))
    draw.text((x2 + 60, cy + 345), "✓ 托盘随时修改，无需重启，保存即刻热生效！", font=font_body_bold, fill=(16, 185, 129))
    
    draw.text((x2 + 40, cy + 460), "配置跨重启持久化，完美贴合每个人独有的按键习惯！", font=font_body, fill=(100, 116, 139))
    return img

# 场景 7: 结尾号召与绿色免安装
def render_scene_7(t, dur):
    img = Image.new("RGB", (WIDTH, HEIGHT))
    draw = ImageDraw.Draw(img)
    draw_bg(draw)
    
    title = SCENE_SCRIPTS[6]["title"]
    b = font_title.getbbox(title)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 80), title, font=font_title, fill=(15, 23, 42))
    
    sub = SCENE_SCRIPTS[6]["sub"]
    b = font_sub_title.getbbox(sub)
    draw.text(((WIDTH - (b[2] - b[0])) // 2, 160), sub, font=font_sub_title, fill=(37, 99, 235))
    
    cw, ch = 1400, 580
    cx = (WIDTH - cw) // 2
    cy = 230
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=20, fill=(255, 255, 255), outline=(226, 232, 240), width=2)
    
    draw.text((cx + 80, cy + 60), "📦 完整免安装便携包：EvanOCR-v1.0.0-windows-x64.zip", font=font_card_h, fill=(30, 58, 138))
    
    details = [
        "• 仅 20 多兆轻巧体积，解压即开即用，无需任何繁琐环境配置",
        "• 100% 纯本地离线运行，绝密工作数据与代码绝对安全无忧",
        "• 彻底替代 Snipaste / PixPin 的全功能截贴图文字提取利器",
        "• 欢迎在评论区提出更多宝贵功能需求与建议！"
    ]
    for i, d in enumerate(details):
        draw.text((cx + 80, cy + 140 + i * 55), d, font=font_body, fill=(51, 65, 85))
        
    draw.rounded_rectangle([cx + 80, cy + 390, cx + cw - 80, cy + 500], radius=14, fill=(254, 240, 138), outline=(234, 179, 8), width=2)
    draw.text((cx + 160, cy + 425), "求一键三连支持！源码与绿色版下载地址在置顶评论 ↓", font=font_card_h, fill=(133, 77, 14))
    return img

# 4. 主渲染流水线
async def main():
    timeline, final_audio, total_duration = await generate_audio_pipeline()
    
    output_mp4 = "EvanOCR_B站专业宣传视频_v4.mp4"
    print(f"开始渲染高清视频: {output_mp4} (总时长: {total_duration:.2f}s) ...")
    
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
    total_frames = int(total_duration * FPS)
    
    render_map = {
        1: render_scene_1,
        2: render_scene_2,
        3: render_scene_3,
        4: render_scene_4,
        5: render_scene_5,
        6: render_scene_6,
        7: render_scene_7
    }
    
    for f_idx in range(total_frames):
        current_time = f_idx / FPS
        
        # 查找当前播放的句子与对应场景 (精确到秒的音字同步)
        active_sentence = None
        for item in timeline:
            if item["start"] <= current_time <= item["end"]:
                active_sentence = item
                break
        if not active_sentence and timeline:
            active_sentence = timeline[-1]
            
        current_scene_id = active_sentence["scene_id"]
        render_func = render_map.get(current_scene_id, render_scene_1)
        
        # 渲染场景主帧
        frame = render_func(current_time, active_sentence["duration"])
        
        # 绘制实时完全同步的字幕 (无黑底，自然高对比)
        draw = ImageDraw.Draw(frame)
        draw_synced_subtitle(draw, active_sentence["text"])
        
        proc.stdin.write(frame.tobytes())
        
        if f_idx % (5 * FPS) == 0:
            print(f"渲染进度: {f_idx // FPS}s / {int(total_duration)}s")
            
    proc.stdin.close()
    proc.wait()
    print(f"全部完成！最终产物: {os.path.abspath(output_mp4)}")

if __name__ == "__main__":
    asyncio.run(main())
