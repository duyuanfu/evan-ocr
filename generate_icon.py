import os
import math
from PIL import Image, ImageDraw, ImageFilter

def create_crisp_vibrant_icon(size=256):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s = size / 256.0

    # 针对小尺寸 (16, 24, 32, 48) 特别优化边距与线条
    if size <= 20:
        margin = 1
        radius = 4
    elif size <= 24:
        margin = 1
        radius = 5
    elif size <= 32:
        margin = 2
        radius = 7
    elif size <= 48:
        margin = 3
        radius = 10
    else:
        margin = int(14 * s)
        radius = int(52 * s)

    rect = [margin, margin, size - margin, size - margin]

    # 1. 外层投射柔和阴影 (增强浅色/白底桌面与任务栏上的立体对比度)
    if size >= 48:
        shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        s_draw = ImageDraw.Draw(shadow)
        s_draw.rounded_rectangle(
            [margin, margin + int(5 * s), size - margin, size - margin + int(5 * s)],
            radius=radius,
            fill=(0, 35, 90, 120)
        )
        shadow = shadow.filter(ImageFilter.GaussianBlur(int(7 * s)))
        img = Image.alpha_composite(shadow, img)

    # 2. 鲜明透亮的高饱和度科技蔚蓝渐变底板 (绝不暗沉，明度饱和度拉满)
    # 顶部: 电光亮蓝 #00a8ff (0, 168, 255) -> 底部: 活力皇家蓝 #0052cc (0, 82, 204)
    bg_canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    bg_draw = ImageDraw.Draw(bg_canvas)
    h_total = int(rect[3] - rect[1])

    for i in range(h_total):
        curr_y = int(rect[1] + i)
        ratio = i / float(max(1, h_total - 1))
        r = 0
        g = int(168 - ratio * 86)  # 168 -> 82
        b = int(255 - ratio * 51)  # 255 -> 204
        bg_draw.line([(rect[0], curr_y), (rect[2], curr_y)], fill=(r, g, b, 255))

    mask = Image.new("L", (size, size), 0)
    mask_d = ImageDraw.Draw(mask)
    mask_d.rounded_rectangle(rect, radius=radius, fill=255)
    bg_canvas.putalpha(mask)
    img = Image.alpha_composite(img, bg_canvas)

    draw = ImageDraw.Draw(img)

    # 3. 晶莹高透边缘内高光描边 (大尺寸呈现)
    if size >= 48:
        draw.rounded_rectangle(rect, radius=radius, outline=(255, 255, 255, 160), width=max(1, int(1.5 * s)))

    # 4. 核心截图标靶：【加粗高反差纯白取景裁切角标】
    cx, cy = size / 2.0, size / 2.0

    if size <= 20:
        cw = 6
        ch = 6
        arm = 4
        b_width = 2
    elif size <= 24:
        cw = 7
        ch = 7
        arm = 5
        b_width = 2
    elif size <= 32:
        cw = 10
        ch = 10
        arm = 7
        b_width = 3
    elif size <= 48:
        cw = 15
        ch = 15
        arm = 9
        b_width = 4
    else:
        cw = int(72 * s)
        ch = int(72 * s)
        arm = int(28 * s)
        b_width = max(2, int(7.0 * s))

    white_color = (255, 255, 255, 255)

    # 绘制纯白加粗四角标
    # 左上
    draw.line([(cx - cw, cy - ch), (cx - cw + arm, cy - ch)], fill=white_color, width=b_width)
    draw.line([(cx - cw, cy - ch), (cx - cw, cy - ch + arm)], fill=white_color, width=b_width)
    # 右上
    draw.line([(cx + cw, cy - ch), (cx + cw - arm, cy - ch)], fill=white_color, width=b_width)
    draw.line([(cx + cw, cy - ch), (cx + cw, cy - ch + arm)], fill=white_color, width=b_width)
    # 左下
    draw.line([(cx - cw, cy + ch), (cx - cw + arm, cy + ch)], fill=white_color, width=b_width)
    draw.line([(cx - cw, cy + ch), (cx - cw, cy + ch - arm)], fill=white_color, width=b_width)
    # 右下
    draw.line([(cx + cw, cy + ch), (cx + cw - arm, cy + ch)], fill=white_color, width=b_width)
    draw.line([(cx + cw, cy + ch), (cx + cw, cy + ch - arm)], fill=white_color, width=b_width)

    # 5. 贯穿式高亮【耀眼金橙色 OCR 扫描光束】(#FFB300)
    scan_y = int(cy)
    beam_color = (255, 179, 0, 255)

    if size >= 48:
        beam_glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        bglow_draw = ImageDraw.Draw(beam_glow)
        bglow_draw.line([(int(cx - cw + 4 * s), scan_y), (int(cx + cw - 4 * s), scan_y)], fill=(255, 195, 0, 160), width=int(10 * s))
        beam_glow = beam_glow.filter(ImageFilter.GaussianBlur(int(4 * s)))
        img = Image.alpha_composite(img, beam_glow)
        draw = ImageDraw.Draw(img)

    beam_w = max(1, 2 if size <= 24 else (3 if size <= 32 else int(4.0 * s)))
    draw.line([(int(cx - cw + (2 if size <= 32 else 6 * s)), scan_y), (int(cx + cw - (2 if size <= 32 else 6 * s)), scan_y)], fill=beam_color, width=beam_w)

    # 6. 中心纯白镜头对焦圆环与光学核心
    if size <= 20:
        # 16x16 / 20x20: 居中纯白圆核 (清晰锐利)
        draw.ellipse([cx - 2, cy - 2, cx + 2, cy + 2], fill=white_color)
    elif size <= 24:
        # 24x24: 纯白小准心
        draw.ellipse([cx - 3, cy - 3, cx + 3, cy + 3], outline=white_color, width=1)
        draw.ellipse([cx - 1, cy - 1, cx + 1, cy + 1], fill=(255, 200, 0, 255))
    elif size <= 32:
        # 32x32: 纯白圆环 + 核心金色光点
        draw.ellipse([cx - 5, cy - 5, cx + 5, cy + 5], outline=white_color, width=2)
        draw.ellipse([cx - 2, cy - 2, cx + 2, cy + 2], fill=(255, 200, 0, 255))
    elif size <= 48:
        ring_r = int(7)
        draw.ellipse([cx - ring_r, cy - ring_r, cx + ring_r, cy + ring_r], outline=white_color, width=2)
        draw.ellipse([cx - 3, cy - 3, cx + 3, cy + 3], fill=(255, 200, 0, 255))
    else:
        # 64, 128, 256 大尺寸: 现代化透镜聚焦圆环 + 十字准星 + 发光核
        ring_r = int(24 * s)
        draw.ellipse([cx - ring_r, cy - ring_r, cx + ring_r, cy + ring_r], outline=white_color, width=max(2, int(4.5 * s)))
        ch_len = int(14 * s)
        draw.line([(cx - ch_len, cy), (cx + ch_len, cy)], fill=white_color, width=max(1, int(2.5 * s)))
        draw.line([(cx, cy - ch_len), (cx, cy + ch_len)], fill=white_color, width=max(1, int(2.5 * s)))
        draw.ellipse([cx - int(6 * s), cy - int(6 * s), cx + int(6 * s), cy + int(6 * s)], fill=(255, 195, 0, 255))
        draw.ellipse([cx - int(3 * s), cy - int(3 * s), cx + int(3 * s), cy + int(3 * s)], fill=white_color)

    return img

if __name__ == "__main__":
    os.makedirs("resources", exist_ok=True)

    # 1. 生成 256x256 高清原图 (明亮鲜艳高对比)
    icon_256 = create_crisp_vibrant_icon(256)
    icon_256.save("resources/app_icon_256.png", "PNG")
    print("Saved resources/app_icon_256.png (Crisp & High Contrast)")

    # 2. 生成多分辨率 Windows 高清 .ico 阵列
    sizes = [16, 20, 24, 32, 48, 64, 128, 256]
    ico_images = [create_crisp_vibrant_icon(sz) for sz in sizes]

    icon_256.save("resources/app.ico", format="ICO", sizes=[(sz, sz) for sz in sizes], append_images=ico_images)
    print("Saved resources/app.ico with multiple sizes [16, 20, 24, 32, 48, 64, 128, 256]")
