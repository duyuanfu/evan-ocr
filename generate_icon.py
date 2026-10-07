import os
import math
from PIL import Image, ImageDraw, ImageFilter

def create_crisp_icon(size=256):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s = size / 256.0

    # 针对极小尺寸 (16, 24, 32) 优化内边距，确保主体图案尽可能饱满撑满空间
    if size <= 24:
        margin = 1
        radius = 4
    elif size <= 32:
        margin = 2
        radius = 6
    elif size <= 48:
        margin = 3
        radius = 10
    else:
        margin = int(14 * s)
        radius = int(52 * s)

    rect = [margin, margin, size - margin, size - margin]

    # 1. 投射柔和投影 (仅大尺寸绘制，小尺寸保持像素纯粹清晰)
    if size >= 48:
        shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        s_draw = ImageDraw.Draw(shadow)
        s_draw.rounded_rectangle(
            [margin, margin + int(5 * s), size - margin, size - margin + int(5 * s)],
            radius=radius,
            fill=(2, 62, 138, 120)
        )
        shadow = shadow.filter(ImageFilter.GaussianBlur(int(7 * s)))
        img = Image.alpha_composite(shadow, img)

    # 2. 鲜明透亮的高饱和活力科技蓝渐变底板 (从 #0096c7 活力天青 到 #023e8a 皇家深邃蓝)
    bg_canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    bg_draw = ImageDraw.Draw(bg_canvas)
    h_total = int(rect[3] - rect[1])

    for i in range(h_total):
        curr_y = int(rect[1] + i)
        ratio = i / float(max(1, h_total - 1))
        # 顶部活力亮天蓝 -> 底部稳重大气海蓝 (色彩极其饱和透亮，告别暗黑)
        r = int(0 + ratio * 2)        # 0 -> 2
        g = int(160 - ratio * 98)     # 160 -> 62
        b = int(225 - ratio * 87)     # 225 -> 138
        bg_draw.line([(rect[0], curr_y), (rect[2], curr_y)], fill=(r, g, b, 255))

    mask = Image.new("L", (size, size), 0)
    mask_d = ImageDraw.Draw(mask)
    mask_d.rounded_rectangle(rect, radius=radius, fill=255)
    bg_canvas.putalpha(mask)
    img = Image.alpha_composite(img, bg_canvas)

    draw = ImageDraw.Draw(img)

    # 3. 底板精致高光边 (大尺寸呈现)
    if size >= 48:
        draw.rounded_rectangle(rect, radius=radius, outline=(255, 255, 255, 140), width=max(1, int(1.5 * s)))

    # 4. 核心符号设计：【大比例粗壮纯白截屏取景角标 + 极简中心十字焦点】
    # 彻底摒弃复杂模糊的细线，确保哪怕在 16x16 下也是清晰锐利的粗线条！
    cx, cy = size / 2.0, size / 2.0

    if size <= 24:
        # 16x16 / 24x24 极小尺寸：极限像素级优化
        cw = int(size * 0.36)
        ch = int(size * 0.36)
        arm = int(size * 0.22)
        b_width = 2
    elif size <= 32:
        cw = int(size * 0.36)
        ch = int(size * 0.36)
        arm = int(size * 0.22)
        b_width = 3
    else:
        cw = int(72 * s)
        ch = int(72 * s)
        arm = int(28 * s)
        b_width = max(2, int(7.0 * s)) # 大尺寸加粗到 7px，极其醒目！

    bracket_color = (255, 255, 255, 255)

    # 绘制取景器四角定位标
    # 左上
    draw.line([(cx - cw, cy - ch), (cx - cw + arm, cy - ch)], fill=bracket_color, width=b_width)
    draw.line([(cx - cw, cy - ch), (cx - cw, cy - ch + arm)], fill=bracket_color, width=b_width)
    # 右上
    draw.line([(cx + cw, cy - ch), (cx + cw - arm, cy - ch)], fill=bracket_color, width=b_width)
    draw.line([(cx + cw, cy - ch), (cx + cw, cy - ch + arm)], fill=bracket_color, width=b_width)
    # 左下
    draw.line([(cx - cw, cy + ch), (cx - cw + arm, cy + ch)], fill=bracket_color, width=b_width)
    draw.line([(cx - cw, cy + ch), (cx - cw, cy + ch - arm)], fill=bracket_color, width=b_width)
    # 右下
    draw.line([(cx + cw, cy + ch), (cx + cw - arm, cy + ch)], fill=bracket_color, width=b_width)
    draw.line([(cx + cw, cy + ch), (cx + cw, cy + ch - arm)], fill=bracket_color, width=b_width)

    # 5. 贯穿式高对比度【耀眼金橙色 OCR 文字识别扫描线】
    scan_y = int(cy)
    beam_color = (255, 183, 3, 255) # 极具辨识度的鲜艳金橙色 #FFB703

    if size >= 48:
        # 发光光晕
        beam_glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        bglow_draw = ImageDraw.Draw(beam_glow)
        bglow_draw.line([(int(cx - cw + 6 * s), scan_y), (int(cx + cw - 6 * s), scan_y)], fill=(255, 195, 0, 160), width=int(10 * s))
        beam_glow = beam_glow.filter(ImageFilter.GaussianBlur(int(4 * s)))
        img = Image.alpha_composite(img, beam_glow)
        draw = ImageDraw.Draw(img)

    # 核心扫描线
    beam_w = max(1, 2 if size <= 24 else (3 if size <= 32 else int(4.0 * s)))
    draw.line([(int(cx - cw + (4 if size <= 32 else 8 * s)), scan_y), (int(cx + cw - (4 if size <= 32 else 8 * s)), scan_y)], fill=beam_color, width=beam_w)

    # 6. 中心纯白清晰十字准心与透镜对焦点 (聚焦与截图标靶象征，结构清晰绝不杂乱)
    if size <= 24:
        # 16x16 / 24x24：画一个清晰纯白对焦圆核 (直径 4~5px)
        dot_r = 2
        draw.ellipse([cx - dot_r, cy - dot_r, cx + dot_r, cy + dot_r], fill=(255, 255, 255, 255))
    elif size <= 32:
        # 32x32：纯白中心圆环 + 对焦点
        draw.ellipse([cx - 4, cy - 4, cx + 4, cy + 4], outline=(255, 255, 255, 255), width=2)
        draw.ellipse([cx - 1, cy - 1, cx + 1, cy + 1], fill=(255, 210, 0, 255))
    else:
        # 大尺寸：高雅镜头对焦圆环 + 中心纯白发光点
        ring_r = int(22 * s)
        draw.ellipse([cx - ring_r, cy - ring_r, cx + ring_r, cy + ring_r], outline=(255, 255, 255, 255), width=max(2, int(4.0 * s)))
        # 中心十字丝
        ch_len = int(12 * s)
        draw.line([(cx - ch_len, cy), (cx + ch_len, cy)], fill=(255, 255, 255, 255), width=max(1, int(2.5 * s)))
        draw.line([(cx, cy - ch_len), (cx, cy + ch_len)], fill=(255, 255, 255, 255), width=max(1, int(2.5 * s)))
        # 核心金光焦点
        draw.ellipse([cx - int(5 * s), cy - int(5 * s), cx + int(5 * s), cy + int(5 * s)], fill=(255, 200, 0, 255))

    return img

if __name__ == "__main__":
    os.makedirs("resources", exist_ok=True)

    # 1. 生成 256x256 高清原图 (高饱和、高辨识度)
    icon_256 = create_crisp_icon(256)
    icon_256.save("resources/app_icon_256.png", "PNG")
    print("Saved resources/app_icon_256.png (Crisp & High Contrast)")

    # 2. 生成包含专有像素级优化尺寸的 Windows 高清 .ico 阵列
    sizes = [16, 24, 32, 48, 64, 128, 256]
    ico_images = [create_crisp_icon(sz) for sz in sizes]

    icon_256.save("resources/app.ico", format="ICO", sizes=[(sz, sz) for sz in sizes], append_images=ico_images)
    print("Saved resources/app.ico with multiple sizes [16, 24, 32, 48, 64, 128, 256]")
