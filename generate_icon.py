import os
import math
from PIL import Image, ImageDraw, ImageFilter

def create_app_icon(size=256):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s = size / 256.0

    # 1. 柔和深邃外阴影
    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s_draw = ImageDraw.Draw(shadow)
    shadow_margin = int(14 * s)
    s_draw.rounded_rectangle(
        [shadow_margin, shadow_margin + int(6 * s), size - shadow_margin, size - shadow_margin + int(6 * s)],
        radius=int(52 * s),
        fill=(10, 25, 55, 150)
    )
    shadow = shadow.filter(ImageFilter.GaussianBlur(int(9 * s)))
    img = Image.alpha_composite(shadow, img)

    # 2. 增强型深邃科技星空蓝 -> 皇家海蓝渐变底板 (旧版本框架升级：提升明度与色彩饱满度)
    margin = int(16 * s)
    rect = [margin, margin, size - margin, size - margin]
    radius = int(52 * s)

    bg_canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    bg_draw = ImageDraw.Draw(bg_canvas)
    h_total = int(rect[3] - rect[1])

    for i in range(h_total):
        curr_y = int(rect[1] + i)
        ratio = i / float(h_total)
        # 更加通透醒目的深海科技蓝
        r = int(14 + ratio * 20)   # 14 -> 34
        g = int(32 + ratio * 75)   # 32 -> 107
        b = int(72 + ratio * 168)  # 72 -> 240
        bg_draw.line([(rect[0], curr_y), (rect[2], curr_y)], fill=(r, g, b, 255))

    mask = Image.new("L", (size, size), 0)
    mask_d = ImageDraw.Draw(mask)
    mask_d.rounded_rectangle(rect, radius=radius, fill=255)
    bg_canvas.putalpha(mask)
    img = Image.alpha_composite(img, bg_canvas)

    draw = ImageDraw.Draw(img)

    # 3. 边框 1.5px 晶莹高光线 (提升立体感)
    draw.rounded_rectangle(rect, radius=radius, outline=(147, 197, 253, 180), width=max(1, int(2 * s)))

    # 4. 镜头取景器四角定位标 (旧版框架核心：加粗强化线宽至 6px，使用纯白配合电光青亮边，极具视觉穿透力)
    cx, cy = size / 2.0, size / 2.0
    cw = int(70 * s)  # 半宽
    ch = int(70 * s)  # 半高
    arm = int(24 * s) # 拐角臂长
    b_width = max(2, int(6.0 * s))
    bracket_color = (255, 255, 255, 255)

    # 左上角标
    draw.line([(cx - cw, cy - ch), (cx - cw + arm, cy - ch)], fill=bracket_color, width=b_width)
    draw.line([(cx - cw, cy - ch), (cx - cw, cy - ch + arm)], fill=bracket_color, width=b_width)
    # 右上角标
    draw.line([(cx + cw, cy - ch), (cx + cw - arm, cy - ch)], fill=bracket_color, width=b_width)
    draw.line([(cx + cw, cy - ch), (cx + cw, cy - ch + arm)], fill=bracket_color, width=b_width)
    # 左下角标
    draw.line([(cx - cw, cy + ch), (cx - cw + arm, cy + ch)], fill=bracket_color, width=b_width)
    draw.line([(cx - cw, cy + ch), (cx - cw, cy + ch - arm)], fill=bracket_color, width=b_width)
    # 右下角标
    draw.line([(cx + cw, cy + ch), (cx + cw - arm, cy + ch)], fill=bracket_color, width=b_width)
    draw.line([(cx + cw, cy + ch), (cx + cw, cy + ch - arm)], fill=bracket_color, width=b_width)

    # 5. 核心文字识别光束 (旧版微光横向扫描线：加厚并强化电光青蓝激光光晕)
    scan_y = int(cy)
    glow_line = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    g_draw = ImageDraw.Draw(glow_line)
    # 弥散发光层
    g_draw.line([(cx - cw + int(6 * s), scan_y), (cx + cw - int(6 * s), scan_y)], fill=(0, 210, 255, 120), width=int(8 * s))
    glow_line = glow_line.filter(ImageFilter.GaussianBlur(int(3 * s)))
    img = Image.alpha_composite(img, glow_line)

    draw = ImageDraw.Draw(img)
    # 核心激光锐线 (纯青白)
    draw.line([(cx - cw + int(8 * s), scan_y), (cx + cw - int(8 * s), scan_y)], fill=(125, 240, 255, 255), width=max(1, int(3.0 * s)))

    # 6. 中心科技感六边形相机快门光圈 + 发光光学核心 (旧版框架核心)
    inner_r = int(26 * s)
    points = []
    for a in range(6):
        angle = a * (2 * math.pi / 6) - math.pi / 6
        points.append((cx + inner_r * math.cos(angle), cy + inner_r * math.sin(angle)))
    # 绘制高雅快门六边形外框 (加粗为 3.5px 确保小图清晰)
    draw.polygon(points, outline=(255, 255, 255, 240), width=max(2, int(3.5 * s)))

    # 快门中心发光核 (高亮度光学透镜焦点)
    draw.ellipse([cx - int(9 * s), cy - int(9 * s), cx + int(9 * s), cy + int(9 * s)], fill=(255, 255, 255, 255))
    draw.ellipse([cx - int(5 * s), cy - int(5 * s), cx + int(5 * s), cy + int(5 * s)], fill=(0, 220, 255, 255))

    return img

if __name__ == "__main__":
    os.makedirs("resources", exist_ok=True)

    # 1. 生成 256x256 高清原图
    icon_256 = create_app_icon(256)
    icon_256.save("resources/app_icon_256.png", "PNG")
    print("Saved resources/app_icon_256.png (Enhanced Classic Frame)")

    # 2. 生成多分辨率 Windows 高清 .ico 阵列
    sizes = [16, 24, 32, 48, 64, 128, 256]
    ico_images = [create_app_icon(sz) for sz in sizes]

    icon_256.save("resources/app.ico", format="ICO", sizes=[(sz, sz) for sz in sizes], append_images=ico_images)
    print("Saved resources/app.ico with multiple sizes [16, 24, 32, 48, 64, 128, 256]")
