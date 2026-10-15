import os
import math
import asyncio
import subprocess
import wave
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import edge_tts

WORKDIR = "media_kit"
os.makedirs(WORKDIR, exist_ok=True)

WIDTH = 1920
HEIGHT = 1080
FPS = 30
FFMPEG_PATH = r"E:\ffmpeg\bin\ffmpeg.exe"
VOICE = "zh-CN-YunxiNeural"
VOICE_RATE = "+36%"

# 7 幕精炼台词 (每句 15~20 字，严格控制真实时长在 3.2~4.2 秒以内，加静音后 < 5.0 秒)
SCENES = [
    {
        "id": 1,
        "badge": "全新架构 · 体积暴降 77%",
        "title": "Evan v1.2.0 重磅进化",
        "sub": "C++20 & Qt6 纯原生打造 · 安装包从 96MB 骤降至 21MB · 极致轻量秒开",
        "voice": "截贴图神器 Evan 迎来全新升级，体积暴降百分之七十七仅二十兆，秒速启动！"
    },
    {
        "id": 2,
        "badge": "重磅杀手锏 · 学术与办公神器",
        "title": "∑ 数学公式秒转 LaTeX & Word 直粘",
        "sub": "双轨调度免模型云端 + 本地离线 · 300 DPI 超高清渲染 · MathML 贴入即转原生公式",
        "voice": "新增独立求和按钮，框选公式秒变 LaTeX，Word 贴入即转原生公式！"
    },
    {
        "id": 3,
        "badge": "双轨互备 · 100% 隐私安全",
        "title": "双轨智能翻译 · 免梯直连与本地大模型",
        "sub": "国内高速专线直连 + Mozilla Bergamot 神经翻译 / 本地离线 Ollama 端点",
        "voice": "双轨翻译上线，国内专线免梯直连，更支持本地大模型离线推理！"
    },
    {
        "id": 4,
        "badge": "长图与动图 · 创作力跃升",
        "title": "滚轮穿透长截图 · 动态 GIF 录屏",
        "sub": "全重叠 MSE 算法 1:1 物理缝合无黑条 · 录制红框精准可视化与多帧导出",
        "voice": "滚轮长截图支持鼠标顺畅穿透，更带来所见即所得动态 GIF 录屏！"
    },
    {
        "id": 5,
        "badge": "UI-UX-Pro-Max · 体验全面升级",
        "title": "现代卡片规范重构 · 4K 全真抗锯齿",
        "sub": "流畅 QScrollArea 视口封装告别挤压变形 · 文字标注支持多行排版与回车规则自选",
        "voice": "界面采用卡片重构，4K 屏幕全真抗锯齿，文字标注更支持多行换行！"
    },
    {
        "id": 6,
        "badge": "精致细节 · 办公贴心彩蛋",
        "title": "3D 立体落影贴图 · 智能理财估值嗅探",
        "sub": "16px 多层柔和立体落影 · 截图自动识别 6 位基金代码并点亮实时估值直达胶囊",
        "voice": "贴图新增十六像素 3D 悬浮落影，更内置理财代码实时估值嗅探！"
    },
    {
        "id": 7,
        "badge": "纯开源 · 绿色免安装",
        "title": "Windows 截贴图终极选择",
        "sub": "适配 Windows 10/11 · GitHub 完全开源 · 零广告纯便携绿色版",
        "voice": "单包仅二十兆开箱即用，求一键三连，完整更新包置顶评论自取！"
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

font_hero = get_font(56, bold=True)
font_title = get_font(46, bold=True)
font_sub_title = get_font(24, bold=False)
font_card_h = get_font(28, bold=True)
font_body_bold = get_font(22, bold=True)
font_body = get_font(20, bold=False)
font_small = get_font(16, bold=False)
font_badge = get_font(16, bold=True)
font_sub = get_font(34, bold=True)
font_code = get_font(22, bold=False)

# 加载应用图标
ICON_IMG = None
icon_path = os.path.join("resources", "app_icon_256.png")
if os.path.exists(icon_path):
    try:
        ICON_IMG = Image.open(icon_path).convert("RGBA")
    except Exception:
        ICON_IMG = None

# 音频管线：精准生成语音、垫齐静音、精确计算各幕物理帧数并混入 BGM
async def generate_audio_pipeline():
    audio_dir = os.path.join(WORKDIR, "audio_segments_v120")
    os.makedirs(audio_dir, exist_ok=True)

    scene_items = []
    wav_files = []

    print(">>> 正在生成各幕真人配音，严格对齐时长与视频帧数...")

    for idx, sc in enumerate(SCENES):
        padded_wav = os.path.join(audio_dir, f"scene_{idx}.wav")

        if not os.path.exists(padded_wav):
            raw_mp3 = os.path.join(audio_dir, f"raw_{idx}.mp3")
            raw_wav = os.path.join(audio_dir, f"raw_{idx}.wav")
            tts = edge_tts.Communicate(sc["voice"], VOICE, rate=VOICE_RATE)
            await tts.save(raw_mp3)

            subprocess.run([FFMPEG_PATH, "-y", "-i", raw_mp3, "-ac", "1", "-ar", "24000", raw_wav],
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

            # 末尾精确追加 0.20 秒停顿
            subprocess.run([FFMPEG_PATH, "-y", "-i", raw_wav, "-af", "apad=pad_dur=0.20", padded_wav],
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

            if os.path.exists(raw_mp3): os.remove(raw_mp3)
            if os.path.exists(raw_wav): os.remove(raw_wav)

        with wave.open(padded_wav, "rb") as wf:
            actual_dur = wf.getnframes() / float(wf.getframerate())

        exact_frames = int(round(actual_dur * FPS))
        video_dur = exact_frames / float(FPS)

        scene_items.append({
            "scene_id": sc["id"],
            "badge": sc["badge"],
            "title": sc["title"],
            "sub": sc["sub"],
            "text": sc["voice"],
            "actual_dur": actual_dur,
            "video_dur": video_dur,
            "frames": exact_frames,
            "file": padded_wav
        })
        wav_files.append(padded_wav)

        print(f"  [Scene {sc['id']}] 时长={actual_dur:.3f}s (帧数={exact_frames}, <5.0s: {actual_dur < 5.0}) | {sc['voice']}")

    total_audio_dur = sum(s["actual_dur"] for s in scene_items)
    total_video_dur = sum(s["video_dur"] for s in scene_items)
    print(f"[OK] 全部 {len(scene_items)} 幕音频生成完毕！总时长: {total_audio_dur:.2f}s，音画累积总误差: {(total_video_dur - total_audio_dur)*1000:+.1f}ms")

    concat_txt = os.path.join(audio_dir, "concat.txt")
    with open(concat_txt, "w", encoding="utf-8") as f:
        for p in wav_files:
            abs_p = os.path.abspath(p).replace("\\", "/")
            f.write(f"file '{abs_p}'\n")

    merged_voice = os.path.join(WORKDIR, "voice_track_v120.wav")
    subprocess.run([FFMPEG_PATH, "-y", "-f", "concat", "-safe", "0", "-i", concat_txt, "-c", "copy", merged_voice],
                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

    final_audio = os.path.join(WORKDIR, "soundtrack_v120.wav")
    bgm_p = os.path.join(WORKDIR, "bgm.mp3")

    if os.path.exists(bgm_p):
        mix_filter = (
            f"[1:a]aloop=loop=-1:size=2e+09,atrim=0:{total_audio_dur},volume=0.075[bgm];"
            f"[0:a]volume=1.25[vox];"
            f"[vox][bgm]amix=inputs=2:duration=first:dropout_transition=2[out]"
        )
        subprocess.run([
            FFMPEG_PATH, "-y",
            "-i", merged_voice,
            "-i", bgm_p,
            "-filter_complex", mix_filter,
            "-map", "[out]",
            final_audio
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    else:
        final_audio = merged_voice

    return scene_items, final_audio

# 预渲染静态渐变与微点阵背景 (单次计算常驻内存，每帧仅毫秒级 copy，性能飙升 10 倍！)
def init_base_background():
    img = Image.new("RGB", (WIDTH, HEIGHT), (255, 255, 255))
    draw = ImageDraw.Draw(img)
    for y in range(0, HEIGHT, 4):
        ratio = y / HEIGHT
        r = int(247 - ratio * 9)
        g = int(250 - ratio * 7)
        b = int(254 - ratio * 5)
        draw.rectangle([0, y, WIDTH, y + 4], fill=(r, g, b))
    for x in range(30, WIDTH, 60):
        for y in range(30, HEIGHT, 60):
            draw.ellipse([x-1, y-1, x+1, y+1], fill=(226, 232, 240))
    return img

BASE_BG = init_base_background()

# 纯净字幕绘制：高对比度黑蓝字 + 柔和投影
def draw_subtitle(draw, text):
    b = font_sub.getbbox(text)
    tw = b[2] - b[0]
    x = (WIDTH - tw) // 2
    y = HEIGHT - 82
    draw.text((x + 2, y + 2), text, font=font_sub, fill=(15, 23, 42, 45))
    draw.text((x, y), text, font=font_sub, fill=(15, 23, 42))

# 通用顶部横幅
def draw_header_banner(draw, badge_text, title_text, sub_text):
    # 顶部徽章
    bw = font_badge.getbbox(badge_text)[2] - font_badge.getbbox(badge_text)[0] + 28
    bx = (WIDTH - bw) // 2
    by = 38
    draw.rounded_rectangle([bx, by, bx + bw, by + 30], radius=15, fill=(239, 246, 255), outline=(191, 219, 254), width=1)
    draw.text((bx + 14, by + 5), badge_text, font=font_badge, fill=(37, 99, 235))

    # 主标题
    tw = font_title.getbbox(title_text)[2] - font_title.getbbox(title_text)[0]
    tx = (WIDTH - tw) // 2
    draw.text((tx + 2, 78 + 2), title_text, font=font_title, fill=(15, 23, 42, 40))
    draw.text((tx, 78), title_text, font=font_title, fill=(15, 23, 42))

    # 副标题
    sw = font_sub_title.getbbox(sub_text)[2] - font_sub_title.getbbox(sub_text)[0]
    sx = (WIDTH - sw) // 2
    draw.text((sx, 140), sub_text, font=font_sub_title, fill=(100, 116, 139))

# =========================================================================
# 分镜 1：开场金色钩子 (Evan v1.2.0 重磅进化 · 体积骤降 77% · 仅 21MB)
# =========================================================================
def render_scene_1(progress):
    img = BASE_BG.copy()
    draw = ImageDraw.Draw(img)
    draw_header_banner(draw, "全新架构升级 · v1.2.0", "Evan 截贴图工具 · 重磅进化", "基于 C++20 与 Qt6 原生开发 · 安装包暴降 77% · 仅 21MB 极致秒启")

    # 1700px 宽幅主体卡片
    cx = 110
    cy = 195
    cw = 1700
    ch = 770
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=1)

    # 左侧：200px 超清应用图标与呼吸彩色科技光晕
    glow_pulse = int(20 * math.sin(progress * math.pi * 2))
    glow_box = [cx + 120 - glow_pulse, cy + 180 - glow_pulse, cx + 340 + glow_pulse, cy + 400 + glow_pulse]
    draw.ellipse(glow_box, fill=(238, 242, 255))

    if ICON_IMG:
        scaled_icon = ICON_IMG.resize((210, 210), Image.Resampling.LANCZOS)
        img.paste(scaled_icon, (cx + 125, cy + 185), scaled_icon)
    else:
        draw.rounded_rectangle([cx + 130, cy + 190, cx + 330, cy + 390], radius=40, fill=(37, 99, 235))
        draw.text((cx + 190, cy + 245), "E", font=font_hero, fill=(255, 255, 255))

    draw.text((cx + 140, cy + 430), "Evan v1.2.0", font=font_card_h, fill=(15, 23, 42))
    draw.text((cx + 130, cy + 475), "Windows 10/11 x64", font=font_body, fill=(148, 163, 184))

    # 右侧：三大硬核升级卡片
    rx = cx + 500
    # 卡片 1: 体积暴降 77%
    draw.rounded_rectangle([rx, cy + 50, rx + 1120, cy + 240], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((rx + 36, cy + 75), "📦 安装包体积锐减 77% (96MB ➔ 21MB)", font=font_card_h, fill=(37, 99, 235))
    draw.text((rx + 36, cy + 125), "解耦重量级 AI 离线模型，主程序保持 21MB 极致身材，冷启动毫秒级响应，开机自启几乎零内存占用。", font=font_body, fill=(71, 85, 105))
    draw.text((rx + 36, cy + 165), "• 启动耗时 < 50ms   • 内存占用仅约 15MB   • 极速绿色免安装", font=font_body_bold, fill=(16, 185, 129))

    # 卡片 2: 可插拔插件中心 (Plugin Market)
    draw.rounded_rectangle([rx, cy + 265, rx + 1120, cy + 455], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((rx + 36, cy + 290), "🧩 可插拔插件中心架构 (Plugin Market)", font=font_card_h, fill=(14, 165, 233))
    draw.text((rx + 36, cy + 340), "按需安装扩展包：RapidOCR 离线文字识别、Mozilla 离线翻译、RapidLaTeX 公式识别，随装随用。", font=font_body, fill=(71, 85, 105))
    draw.text((rx + 36, cy + 380), "• 支持软件内一键安装   • 本地 ZIP 导入即刻生效   • 多层级目录递归自适应", font=font_body_bold, fill=(37, 99, 235))

    # 卡片 3: 全新 v1.2.0 专属特性矩阵
    draw.rounded_rectangle([rx, cy + 480, rx + 1120, cy + 685], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((rx + 36, cy + 505), "✨ v1.2.0 全新功能矩阵", font=font_card_h, fill=(217, 70, 239))
    draw.text((rx + 36, cy + 555), "∑ 数学公式识别与 Word 直粘  |  双轨机器翻译  |  穿透长截图  |  GIF 录屏  |  3D 贴图落影", font=font_body_bold, fill=(15, 23, 42))
    draw.text((rx + 36, cy + 600), "全界面 UI-UX-Pro-Max 现代卡片规范重构，4K 屏幕全真抗锯齿，文字标注多行排版自由换行。", font=font_body, fill=(100, 116, 139))

    return img

# =========================================================================
# 分镜 2：核心杀手锏 (∑ 数学公式秒转 LaTeX & Word 直粘)
# =========================================================================
def render_scene_2(progress):
    img = BASE_BG.copy()
    draw = ImageDraw.Draw(img)
    draw_header_banner(draw, "科研与办公利器 · 独立入口", "∑ 数学公式识别 · LaTeX / MathML / Word 直粘", "截屏工具栏专属 ∑ 按钮 · 300 DPI 超清矢量渲染预览 · 贴入 Word 自动转为原生矢量公式")

    cx = 110
    cy = 195
    cw = 1700
    ch = 770
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=1)

    # 顶部迷你控制栏模拟
    draw.text((cx + 36, cy + 24), "∑ 公式识别与转换", font=font_card_h, fill=(15, 23, 42))
    draw.rounded_rectangle([cx + 310, cy + 24, cx + 510, cy + 58], radius=6, fill=(239, 246, 255), outline=(191, 219, 254), width=1)
    draw.text((cx + 326, cy + 30), "LaTeX / MathML / Word", font=font_badge, fill=(37, 99, 235))

    draw.rounded_rectangle([cx + cw - 380, cy + 22, cx + cw - 180, cy + 60], radius=6, fill=(248, 250, 252), outline=(203, 213, 225), width=1)
    draw.text((cx + cw - 364, cy + 30), "在线云端通道 (高速)", font=font_small, fill=(15, 23, 42))
    draw.rounded_rectangle([cx + cw - 165, cy + 22, cx + cw - 36, cy + 60], radius=6, fill=(248, 250, 252), outline=(203, 213, 225), width=1)
    draw.text((cx + cw - 150, cy + 30), "⚙️ 配置 Token", font=font_small, fill=(71, 85, 105))

    # 双卡片镜像对比
    split_y = cy + 78
    box_w = (cw - 92) // 2
    box_h = 290

    # 左卡片：选区原图
    lx = cx + 36
    draw.rounded_rectangle([lx, split_y, lx + box_w, split_y + box_h], radius=10, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((lx + 20, split_y + 14), "选区原图 (积分高斯分布公式)", font=font_body_bold, fill=(100, 116, 139))
    # 模拟手写/印刷体积分公式
    draw.line([(lx + 100, split_y + 90), (lx + 100, split_y + 220)], fill=(30, 41, 59), width=5)
    draw.arc([lx + 90, split_y + 70, lx + 120, split_y + 110], 180, 360, fill=(30, 41, 59), width=5)
    draw.arc([lx + 80, split_y + 200, lx + 110, split_y + 240], 0, 180, fill=(30, 41, 59), width=5)
    draw.text((lx + 125, split_y + 130), "e^(-x^2) dx  =  √π / 2", font=font_hero, fill=(30, 41, 59))
    draw.text((lx + 106, split_y + 75), "+∞", font=font_body_bold, fill=(100, 116, 139))
    draw.text((lx + 106, split_y + 215), "-∞", font=font_body_bold, fill=(100, 116, 139))

    # 右卡片：300 DPI 超清公式矢量渲染图
    rx = lx + box_w + 20
    draw.rounded_rectangle([rx, split_y, rx + box_w, split_y + box_h], radius=10, fill=(255, 255, 255), outline=(191, 219, 254), width=2)
    draw.text((rx + 20, split_y + 14), "公式矢量渲染预览 (300 DPI 超清通道 · 纤毫毕现)", font=font_body_bold, fill=(37, 99, 235))
    draw.text((rx + 80, split_y + 110), "∫ e   dx  =  ───", font=font_hero, fill=(15, 23, 42))
    draw.text((rx + 155, split_y + 85), "-x²", font=font_card_h, fill=(15, 23, 42))
    draw.text((rx + 420, split_y + 75), "√π", font=font_card_h, fill=(15, 23, 42))
    draw.text((rx + 430, split_y + 165), "2", font=font_card_h, fill=(15, 23, 42))

    # 动态激光扫描线效果
    scan_offset = int((progress * 3.0 % 1.0) * (box_w - 40))
    draw.line([(rx + 20 + scan_offset, split_y + 40), (rx + 20 + scan_offset, split_y + box_h - 20)], fill=(37, 99, 235, 180), width=2)

    # 下半部：LaTeX 源码多行编辑区
    code_y = split_y + box_h + 20
    code_h = 240
    draw.rounded_rectangle([lx, code_y, lx + cw - 72, code_y + code_h], radius=10, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((lx + 20, code_y + 14), "LaTeX 源码 (可直接修改编辑):", font=font_body_bold, fill=(100, 116, 139))
    latex_sample = "\\int_{-\\infty}^{+\\infty} e^{-x^2} \\, dx = \\frac{\\sqrt{\\pi}}{2}"
    draw.text((lx + 24, code_y + 60), latex_sample, font=font_card_h, fill=(30, 41, 59))
    draw.text((lx + 24, code_y + 115), "• 格式转换：自动转换为标准 MathML 与 Markdown 块级语法 ($$ ... $$)", font=font_body, fill=(100, 116, 139))
    draw.text((lx + 24, code_y + 150), "• 双轨调度：默认云端免模型高速出字；断网或未配置 Token 自动无缝降级离线插件！", font=font_body_bold, fill=(16, 185, 129))

    # 底部神级操作栏
    btn_y = code_y + code_h + 24
    draw.rounded_rectangle([lx, btn_y, lx + 180, btn_y + 48], radius=6, fill=(255, 255, 255), outline=(203, 213, 225), width=1)
    draw.text((lx + 32, btn_y + 12), "复制 LaTeX", font=font_body_bold, fill=(51, 65, 85))

    draw.rounded_rectangle([lx + 200, btn_y, lx + 410, btn_y + 48], radius=6, fill=(255, 255, 255), outline=(203, 213, 225), width=1)
    draw.text((lx + 225, btn_y + 12), "复制 Markdown", font=font_body_bold, fill=(51, 65, 85))

    # 视觉主按钮：复制 Word (MathML)
    draw.rounded_rectangle([lx + 430, btn_y, lx + 760, btn_y + 48], radius=6, fill=(37, 99, 235))
    draw.text((lx + 458, btn_y + 12), "📄 复制 Word (MathML) [推荐]", font=font_body_bold, fill=(255, 255, 255))

    draw.text((lx + 790, btn_y + 14), "👉 切换到 Word 直接 Ctrl+V，瞬间转为原生可编辑矢量数学公式！", font=font_body_bold, fill=(234, 88, 12))

    return img

# =========================================================================
# 分镜 3：双轨机器翻译系统 (免梯直连 + 纯本地离线大模型)
# =========================================================================
def render_scene_3(progress):
    img = BASE_BG.copy()
    draw = ImageDraw.Draw(img)
    draw_header_banner(draw, "双轨智能互备 · 绝无泄密", "双轨机器翻译 · 在线专线与纯本地大模型", "国内免梯专线毫秒级出字 · 支持 Mozilla Bergamot 神经翻译与本地 Ollama (Qwen/DeepSeek)")

    cx = 110
    cy = 195
    cw = 1700
    ch = 770
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=1)

    # 左右双分栏卡片：左侧在线通道，右侧纯本地离线模型
    col_w = (cw - 80) // 2
    col_h = 670
    col_y = cy + 50

    # 轨道 1：国内高速在线专线
    c1_x = cx + 30
    draw.rounded_rectangle([c1_x, col_y, c1_x + col_w, col_y + col_h], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.rounded_rectangle([c1_x + 24, col_y + 24, c1_x + 190, col_y + 60], radius=18, fill=(239, 246, 255), outline=(191, 219, 254), width=1)
    draw.text((c1_x + 40, col_y + 30), "通道 A · 在线直连", font=font_badge, fill=(37, 99, 235))
    draw.text((c1_x + 24, col_y + 80), "国内高速专线 (免配置·免梯直连)", font=font_card_h, fill=(15, 23, 42))
    draw.text((c1_x + 24, col_y + 130), "• 免 Key 免注册，开箱即用\n• 中译英、中译日毫秒级流式出字\n• 请求防抖与 Google GTX 自动容灾热备\n• 支持全语种自动嗅探双向翻译", font=font_body, fill=(71, 85, 105))

    # 演示文本框
    draw.rounded_rectangle([c1_x + 24, col_y + 280, c1_x + col_w - 24, col_y + 440], radius=8, fill=(255, 255, 255), outline=(203, 213, 225), width=1)
    draw.text((c1_x + 38, col_y + 295), "原文：Evan 是一款现代化轻量级截贴图与离线 OCR 工具。", font=font_body_bold, fill=(15, 23, 42))
    draw.text((c1_x + 38, col_y + 355), "译文：Evan is a modern, lightweight screenshot and offline OCR tool.", font=font_body, fill=(37, 99, 235))
    draw.text((c1_x + 24, col_y + 470), "⚡ 耗时: 128ms  |  通道状态: 国内高速直连", font=font_small, fill=(16, 185, 129))

    # 轨道 2：纯本地离线神经网络 / 本地大模型
    c2_x = c1_x + col_w + 20
    draw.rounded_rectangle([c2_x, col_y, c2_x + col_w, col_y + col_h], radius=12, fill=(248, 250, 252), outline=(167, 243, 208), width=2)
    draw.rounded_rectangle([c2_x + 24, col_y + 24, c2_x + 210, col_y + 60], radius=18, fill=(236, 253, 245), outline=(167, 243, 208), width=1)
    draw.text((c2_x + 40, col_y + 30), "通道 B · 纯本地离线", font=font_badge, fill=(5, 150, 105))
    draw.text((c2_x + 24, col_y + 80), "本地大模型 / Bergamot (0联网·100%隐私)", font=font_card_h, fill=(15, 23, 42))
    draw.text((c2_x + 24, col_y + 130), "• Mozilla Bergamot 神经翻译架构轻量插件\n• 自动检测直连本地 Ollama (Qwen/DeepSeek)\n• 0 网络请求，私密合同与商业代码绝对安全\n• 断网离线可用，响应极其稳定", font=font_body, fill=(71, 85, 105))

    draw.rounded_rectangle([c2_x + 24, col_y + 280, c2_x + col_w - 24, col_y + 440], radius=8, fill=(255, 255, 255), outline=(203, 213, 225), width=1)
    draw.text((c2_x + 38, col_y + 295), "私密代码/技术方案：无需联网即可精准直译", font=font_body_bold, fill=(15, 23, 42))
    draw.text((c2_x + 38, col_y + 355), "本地推理：Ollama 通用端点 (11434) 自动命中挂载", font=font_body, fill=(5, 150, 105))
    draw.text((c2_x + 24, col_y + 470), "🔒 隐私保护: 纯本地运算  |  网络外发: 0 字节", font=font_small, fill=(5, 150, 105))

    return img

# =========================================================================
# 分镜 4：长截图与所见即所得 GIF 录屏
# =========================================================================
def render_scene_4(progress):
    img = BASE_BG.copy()
    draw = ImageDraw.Draw(img)
    draw_header_banner(draw, "动态采集 · 顺滑如丝", "辅助滚动长截图 · 可视化 GIF 录屏", "鼠标 100% 穿透顺畅滚动 · 全重叠 MSE 算法零黑条 · 红黄录制边框与实时计时器")

    cx = 110
    cy = 195
    cw = 1700
    ch = 770
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=1)

    box_w = (cw - 80) // 2
    box_h = 670
    b_y = cy + 50

    # 左侧：穿透长截图演示
    b1_x = cx + 30
    draw.rounded_rectangle([b1_x, b_y, b1_x + box_w, b_y + box_h], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((b1_x + 24, b_y + 24), "📜 滚轮辅助长截图 (100% 鼠标穿透)", font=font_card_h, fill=(37, 99, 235))
    draw.text((b1_x + 24, b_y + 70), "全重叠 MSE 算法 1:1 物理缝合，彻底根除高分屏黑条与重复切片", font=font_body, fill=(71, 85, 105))

    # 长截图视口与模拟长网页
    vp_x = b1_x + 30
    vp_y = b_y + 120
    vp_w = box_w - 60
    vp_h = 420
    draw.rectangle([vp_x, vp_y, vp_x + vp_w, vp_y + vp_h], fill=(255, 255, 255), outline=(59, 130, 246), width=2)
    # 模拟网页多行内容滚动
    scroll_shift = int((progress * 2.0 % 1.0) * 120)
    for i in range(12):
        ly = vp_y + 20 + i * 45 - scroll_shift
        if vp_y + 10 < ly < vp_y + vp_h - 20:
            draw.text((vp_x + 25, ly), f"Chapter {i+1}: 深度排查高 DPI 像素比算法，强制物理对齐无缝拼接...", font=font_small, fill=(71, 85, 105))

    # 悬浮极简控制胶囊
    cap_w = 340
    cap_x = vp_x + (vp_w - cap_w) // 2
    cap_y = vp_y + vp_h - 60
    draw.rounded_rectangle([cap_x, cap_y, cap_x + cap_w, cap_y + 44], radius=22, fill=(15, 23, 42), outline=(59, 130, 246), width=2)
    draw.text((cap_x + 20, cap_y + 12), "高度: 2840px (9帧)  |  保存  |  取消", font=font_body_bold, fill=(255, 255, 255))

    # 右侧：所见即所得 GIF 录屏演示
    b2_x = b1_x + box_w + 20
    draw.rounded_rectangle([b2_x, b_y, b2_x + box_w, b_y + box_h], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((b2_x + 24, b_y + 24), "🎬 所见即所得 GIF 动态录屏", font=font_card_h, fill=(234, 88, 12))
    draw.text((b2_x + 24, b_y + 70), "告别盲盒录制！红黄虚线高亮选区，原生态高效变长 LZW 89a 视频编码", font=font_body, fill=(71, 85, 105))

    # 红黄虚线录制选区
    rec_x = b2_x + 30
    rec_y = b_y + 120
    rec_w = box_w - 60
    rec_h = 420
    draw.rectangle([rec_x, rec_y, rec_x + rec_w, rec_y + rec_h], fill=(255, 255, 255), outline=(239, 68, 68), width=3)
    # 模拟录制中动效元素
    draw.rounded_rectangle([rec_x + 60, rec_y + 100, rec_x + rec_w - 60, rec_y + 260], radius=12, fill=(241, 245, 249))
    draw.text((rec_x + 100, rec_y + 150), "▶ 正在实时采集选区动态画面...", font=font_card_h, fill=(30, 41, 59))

    # 录制控制栏
    ctl_w = 400
    ctl_x = rec_x + (rec_w - ctl_w) // 2
    ctl_y = rec_y + rec_h - 60
    draw.rounded_rectangle([ctl_x, ctl_y, ctl_x + ctl_w, ctl_y + 44], radius=22, fill=(255, 255, 255), outline=(203, 213, 225), width=1)
    # 红点呼吸
    draw.ellipse([ctl_x + 24, ctl_y + 14, ctl_x + 40, ctl_y + 30], fill=(239, 68, 68))
    draw.text((ctl_x + 50, ctl_y + 12), "00:04.8 (15 FPS)  |  [停止并保存]", font=font_body_bold, fill=(15, 23, 42))

    return img

# =========================================================================
# 分镜 5：UI-UX-Pro-Max 规范重构 & 4K 高分屏优化
# =========================================================================
def render_scene_5(progress):
    img = BASE_BG.copy()
    draw = ImageDraw.Draw(img)
    draw_header_banner(draw, "现代卡片规范 · 极致精致", "全界面 UI-UX-Pro-Max 重构 · 4K 矢量渲染", "QScrollArea 流畅视口封装告别挤压变形 · 文字标注多行排版与回车规则自选")

    cx = 110
    cy = 195
    cw = 1700
    ch = 770
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=1)

    box_w = (cw - 80) // 2
    box_h = 670
    b_y = cy + 50

    # 左侧：全新卡片式设置面板展示
    b1_x = cx + 30
    draw.rounded_rectangle([b1_x, b_y, b1_x + box_w, b_y + box_h], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((b1_x + 24, b_y + 24), "⚙️ 设置面板全面重构 (Fluent 卡片风格)", font=font_card_h, fill=(37, 99, 235))
    draw.text((b1_x + 24, b_y + 68), "QScrollArea 滚动视口封装，彻底杜绝输入框被压缩变小、高度错乱！", font=font_body, fill=(71, 85, 105))

    # 4 块模拟设置卡片
    cards = [
        ("🎨 主题外观风格", "跟随系统设置 / Fluent Light / Fluent Dark 自适应切换"),
        ("🎯 截屏辅助与智能吸附", "窗口轮廓与元素精准嗅探，Tab 键循环切换候选框"),
        ("🚀 系统启动偏好", "Windows Run 注册表常驻托盘，开机毫秒响应全局热键"),
        ("⌨️ 截图标注文字习惯", "支持自选 [Enter提交 / Shift+Enter换行] 或 [Ctrl+Enter提交]")
    ]
    for i, (ctitle, cdesc) in enumerate(cards):
        cy_card = b_y + 115 + i * 130
        draw.rounded_rectangle([b1_x + 24, cy_card, b1_x + box_w - 24, cy_card + 115], radius=8, fill=(255, 255, 255), outline=(226, 232, 240), width=1)
        draw.text((b1_x + 44, cy_card + 16), ctitle, font=font_body_bold, fill=(37, 99, 235))
        draw.text((b1_x + 44, cy_card + 54), cdesc, font=font_body, fill=(71, 85, 105))
        # 模拟 32px 舒适控件
        draw.rounded_rectangle([b1_x + box_w - 180, cy_card + 18, b1_x + box_w - 44, cy_card + 52], radius=6, fill=(248, 250, 252), outline=(203, 213, 225), width=1)
        draw.text((b1_x + box_w - 165, cy_card + 24), "[已配置就绪]", font=font_small, fill=(16, 185, 129))

    # 右侧：4K 全真抗锯齿与多行文字标注
    b2_x = b1_x + box_w + 20
    draw.rounded_rectangle([b2_x, b_y, b2_x + box_w, b_y + box_h], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((b2_x + 24, b_y + 24), "4K 屏幕次像素抗锯齿 & 文字多行排版", font=font_card_h, fill=(147, 51, 234))
    draw.text((b2_x + 24, b_y + 68), "Microsoft YaHei UI 全局平滑抗锯齿策略，告别模糊锯齿！", font=font_body, fill=(71, 85, 105))

    # 文字多行标注编辑器示意
    mb_y = b_y + 120
    draw.rounded_rectangle([b2_x + 24, mb_y, b2_x + box_w - 24, mb_y + 260], radius=8, fill=(20, 20, 24), outline=(235, 30, 30), width=2)
    draw.text((b2_x + 44, mb_y + 24), "✓ 架构升级：轻量解耦，性能翻倍", font=font_card_h, fill=(239, 68, 68))
    draw.text((b2_x + 44, mb_y + 80), "✓ 多行换行：Shift+Enter 自然换行", font=font_card_h, fill=(239, 68, 68))
    draw.text((b2_x + 44, mb_y + 136), "✓ 像素对齐：基线垂直重合，绝不上浮跳动", font=font_card_h, fill=(239, 68, 68))
    draw.text((b2_x + 44, mb_y + 192), "✓ 习惯切换：设置中支持一键切换 Enter/Ctrl+Enter", font=font_card_h, fill=(239, 68, 68))

    # 4K 高清特性卡片
    draw.rounded_rectangle([b2_x + 24, mb_y + 290, b2_x + box_w - 24, mb_y + 510], radius=8, fill=(255, 255, 255), outline=(226, 232, 240), width=1)
    draw.text((b2_x + 44, mb_y + 315), "🖥️ 4K / High DPI 屏幕优化", font=font_card_h, fill=(15, 23, 42))
    draw.text((b2_x + 44, mb_y + 365), "• QFont::PreferAntialias | QFont::PreferQuality 全局注入\n• 浮动工具栏 Segoe UI Symbol 矢量字体抗锯齿\n• Qt::PassThrough 整数缩放，边框像素级精准对齐\n• 4K 150%/200%/250% 缩放下边缘无色边无毛刺", font=font_body, fill=(71, 85, 105))

    return img

# =========================================================================
# 分镜 6：贴图 3D 立体落影与基金估值彩蛋
# =========================================================================
def render_scene_6(progress):
    img = BASE_BG.copy()
    draw = ImageDraw.Draw(img)
    draw_header_banner(draw, "立体质感 · 智能场景扩展", "3D 立体落影贴图 · 智能理财估值嗅探", "预留 16px 多层柔和落影看清同色边界 · 截图命中 6 位基金代码智能点亮估值直达胶囊")

    cx = 110
    cy = 195
    cw = 1700
    ch = 770
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=1)

    box_w = (cw - 80) // 2
    box_h = 670
    b_y = cy + 50

    # 左侧：3D 立体落影贴图窗口
    b1_x = cx + 30
    draw.rounded_rectangle([b1_x, b_y, b1_x + box_w, b_y + box_h], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((b1_x + 24, b_y + 24), "📌 贴图窗口 3D 立体落影 (F3 置顶)", font=font_card_h, fill=(37, 99, 235))
    draw.text((b1_x + 24, b_y + 68), "16px 多层柔和立体落影与科技蓝悬浮外框，同色桌面一眼分清边界！", font=font_body, fill=(71, 85, 105))

    # 绘制带深度阴影的 Pin 贴图窗口
    pw_x = b1_x + 80
    pw_y = b_y + 160
    pw_w = box_w - 160
    pw_h = 360
    # 多层阴影
    for s in range(16, 0, -2):
        alpha = int(120 - s * 6)
        draw.rounded_rectangle([pw_x - s, pw_y - s + 4, pw_x + pw_w + s, pw_y + pw_h + s + 8], radius=10, outline=(0, 0, 0, alpha))
    # 窗口本体
    draw.rounded_rectangle([pw_x, pw_y, pw_x + pw_w, pw_y + pw_h], radius=8, fill=(255, 255, 255), outline=(59, 130, 246), width=2)
    draw.text((pw_x + 40, pw_y + 60), "置顶贴图窗口 (F3)", font=font_card_h, fill=(15, 23, 42))
    draw.text((pw_x + 40, pw_y + 120), "• 滚轮以鼠标为中心平滑缩放\n• Ctrl + 滚轮 动态调节透明度 (10%~100%)\n• 右键随时再次 OCR 提取或翻译\n• 双击或按 Esc 瞬间关闭销毁", font=font_body, fill=(71, 85, 105))

    # 右侧：基金上下文嗅探彩蛋 (FundSniffer)
    b2_x = b1_x + box_w + 20
    draw.rounded_rectangle([b2_x, b_y, b2_x + box_w, b_y + box_h], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((b2_x + 24, b_y + 24), "📈 OCR 智能理财上下文嗅探彩蛋", font=font_card_h, fill=(16, 185, 129))
    draw.text((b2_x + 24, b_y + 68), "智能感知：截取持仓或基金走势，底栏高光胶囊一键复制直达估值！", font=font_body, fill=(71, 85, 105))

    # 模拟 OCR 识别对话框底栏
    dlg_y = b_y + 160
    dlg_w = box_w - 60
    dlg_x = b2_x + 30
    draw.rounded_rectangle([dlg_x, dlg_y, dlg_x + dlg_w, dlg_y + 360], radius=10, fill=(255, 255, 255), outline=(226, 232, 240), width=1)
    draw.text((dlg_x + 30, dlg_y + 24), "提取文字对照区", font=font_body_bold, fill=(15, 23, 42))

    draw.rounded_rectangle([dlg_x + 30, dlg_y + 70, dlg_x + dlg_w - 30, dlg_y + 220], radius=8, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((dlg_x + 44, dlg_y + 85), "易方达蓝筹精选混合 (005827)", font=font_card_h, fill=(15, 23, 42))
    draw.text((dlg_x + 44, dlg_y + 130), "单位净值: 2.1450  |  日涨幅: +1.28%", font=font_body_bold, fill=(239, 68, 68))
    draw.text((dlg_x + 44, dlg_y + 170), "持仓收益: +8,620.00 元", font=font_body, fill=(71, 85, 105))

    # 点亮底栏高光胶囊
    cap_y = dlg_y + 260
    draw.rounded_rectangle([dlg_x + 30, cap_y, dlg_x + dlg_w - 30, cap_y + 54], radius=27, fill=(239, 246, 255), outline=(191, 219, 254), width=2)
    draw.text((dlg_x + 50, cap_y + 15), "📈 发现基金: 005827 (易方达蓝筹) · 查看实时估值 ↗", font=font_body_bold, fill=(29, 78, 216))

    draw.text((b2_x + 34, b_y + 560), "👉 点击胶囊：自动将代码写入剪贴板，秒级直达估值看板，无感且贴心！", font=font_body_bold, fill=(37, 99, 235))

    return img

# =========================================================================
# 分镜 7：收尾与一键三连 (极致轻量 · 纯开源绿色免安装)
# =========================================================================
def render_scene_7(progress):
    img = BASE_BG.copy()
    draw = ImageDraw.Draw(img)
    draw_header_banner(draw, "开源免费 · 绿色纯净", "Evan v1.2.0 · 办公与科研必备生产力", "Windows 10/11 极速免安装绿色便携包 · GitHub 完整开源 · 解压即用")

    cx = 110
    cy = 195
    cw = 1700
    ch = 770
    draw.rounded_rectangle([cx, cy, cx + cw, cy + ch], radius=16, fill=(255, 255, 255), outline=(226, 232, 240), width=1)

    # 中心巨幅卡片
    draw.text((cx + 120, cy + 80), "🚀 纯绿色便携免安装 · 零依赖秒启", font=font_hero, fill=(15, 23, 42))
    draw.text((cx + 120, cy + 160), "• 官方免安装包仅 21.5 MB，解压即用，不写死注册表，零后台广告流氓常驻", font=font_card_h, fill=(71, 85, 105))
    draw.text((cx + 120, cy + 220), "• 全功能支持：截贴图 / 离线改字 / 滚轮长截图 / GIF录屏 / 双轨翻译 / ∑ 公式识别", font=font_card_h, fill=(37, 99, 235))
    draw.text((cx + 120, cy + 280), "• 遵循 MIT 宽松开源协议，代码 100% 公开透明，数据完全属于你自己", font=font_card_h, fill=(5, 150, 105))

    # GitHub 仓库徽章
    draw.rounded_rectangle([cx + 120, cy + 360, cx + 1580, cy + 500], radius=12, fill=(248, 250, 252), outline=(226, 232, 240), width=1)
    draw.text((cx + 160, cy + 395), "GitHub 源码与发布页：github.com/duyuanfu/evan-ocr", font=font_card_h, fill=(15, 23, 42))
    draw.text((cx + 160, cy + 445), "Release: v1.2.0 (包含 Windows x64 便携包与离线 RapidLaTeXOCR 模型包)", font=font_body, fill=(100, 116, 139))

    # 一键三连呼吁徽章 (带呼吸动画)
    bounce = int(6 * math.sin(progress * math.pi * 3))
    btn_y = cy + 550 + bounce
    draw.rounded_rectangle([cx + 450, btn_y, cx + 1250, btn_y + 90], radius=45, fill=(234, 88, 12))
    draw.text((cx + 520, btn_y + 24), "👍 点赞 · 投币 · 收藏「一键三连」", font=font_hero, fill=(255, 255, 255))

    draw.text((cx + 540, cy + 680), "👇 下载链接与完整更新包已在【置顶评论】免费自取！", font=font_card_h, fill=(225, 29, 72))

    return img

SCENE_RENDERERS = [
    render_scene_1,
    render_scene_2,
    render_scene_3,
    render_scene_4,
    render_scene_5,
    render_scene_6,
    render_scene_7
]

# 视频渲染主流程：管道对接 FFmpeg H.264
def render_full_video(scene_items, audio_path):
    output_mp4 = os.path.join(WORKDIR, "Evan_v1.2.0_功能更新介绍.mp4")
    if os.path.exists(output_mp4):
        os.remove(output_mp4)

    total_frames = sum(s["frames"] for s in scene_items)
    print(f"\n>>> 开始全画幅 1080P 视频帧渲染与 FFmpeg H.264 硬件编码...")
    print(f"    总帧数: {total_frames} 帧 (30 FPS, 总时长约 {total_frames / FPS:.2f} 秒)")
    print(f"    输出文件: {output_mp4}")

    ffmpeg_cmd = [
        FFMPEG_PATH, "-y",
        "-f", "rawvideo",
        "-vcodec", "rawvideo",
        "-s", f"{WIDTH}x{HEIGHT}",
        "-pix_fmt", "rgb24",
        "-r", str(FPS),
        "-i", "-",               # 从标准输入管道接收图像数据
        "-i", audio_path,        # 音轨
        "-c:v", "libx264",
        "-preset", "medium",
        "-crf", "18",            # 极高画质
        "-pix_fmt", "yuv420p",
        "-c:a", "aac",
        "-b:a", "192k",
        "-movflags", "+faststart",
        output_mp4
    ]

    proc = subprocess.Popen(ffmpeg_cmd, stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    rendered_count = 0

    for s_idx, s_info in enumerate(scene_items):
        renderer = SCENE_RENDERERS[s_idx]
        num_frames = s_info["frames"]
        sub_text = s_info["text"]

        for f in range(num_frames):
            prog = f / float(num_frames) if num_frames > 1 else 0.0
            # 渲染画面主体
            frame_img = renderer(prog)
            draw = ImageDraw.Draw(frame_img)
            # 叠加精准字幕
            draw_subtitle(draw, sub_text)

            # 写入管道
            proc.stdin.write(frame_img.tobytes())
            rendered_count += 1

            if rendered_count % 30 == 0 or rendered_count == total_frames:
                pct = rendered_count / total_frames * 100
                print(f"  [渲染进度] {rendered_count}/{total_frames} 帧 ({pct:.1f}%) ...", end="\r", flush=True)

    proc.stdin.close()
    proc.wait()
    print(f"\n[SUCCESS] 视频压制交付完成！文件路径: {output_mp4}")
    size_mb = os.path.getsize(output_mp4) / (1024 * 1024)
    print(f"          视频规格: 1920x1080 @ 30fps | 体积: {size_mb:.2f} MB")

async def main():
    scene_items, audio_path = await generate_audio_pipeline()
    render_full_video(scene_items, audio_path)

if __name__ == "__main__":
    asyncio.run(main())
