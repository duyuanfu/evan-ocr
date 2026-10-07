import os
import math
from PIL import Image, ImageDraw, ImageFilter

def create_bright_vibrant_icon(size=256):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s = size / 256.0

    # 1. 外层柔和深色投射阴影 (确保浅色/纯白桌面/任务栏下立体感强，轮廓清晰)
    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s_draw = ImageDraw.Draw(shadow)
    margin = int(14 * s)
    s_draw.rounded_rectangle(
        [margin, margin + int(5 * s), size - margin, size - margin + int(5 * s)],
        radius=int(54 * s),
        fill=(0, 30, 80, 110)
    )
    shadow = shadow.filter(ImageFilter.GaussianBlur(int(8 * s)))
    img = Image.alpha_composite(shadow, img)

    # 2. 鲜亮明快的高饱和度蔚蓝 -> 活力深海蓝渐变底板 (拒绝死沉暗黑，大幅提亮45%，极具视觉穿透力)
    rect = [margin, margin, size - margin, size - margin]
    radius = int(52 * s)

    bg_canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    bg_draw = ImageDraw.Draw(bg_canvas)
    h_total = int(rect[3] - rect[1])

    for i in range(h_total):
        curr_y = int(rect[1] + i)
        ratio = i / float(h_total)
        # 明亮电光蔚蓝 #0284c7 -> 活力皇家蓝 #0353a4
        r = int(2 + ratio * 15)      # 2 -> 17
        g = int(155 - ratio * 65)    # 155 -> 90
        b = int(245 - ratio * 45)    # 245 -> 200
        bg_draw.line([(rect[0], curr_y), (rect[2], curr_y)], fill=(r, g, b, 255))

    mask = Image.new("L", (size, size), 0)
    mask_d = ImageDraw.Draw(mask)
    mask_d.rounded_rectangle(rect, radius=radius, fill=255)
    bg_canvas.putalpha(mask)
    img = Image.alpha_composite(img, bg_canvas)

    draw = ImageDraw.Draw(img)

    # 3. 晶莹高透边缘光圈 (1.5px 纯白半透明内描边)
    draw.rounded_rectangle(rect, radius=radius, outline=(255, 255, 255, 160), width=max(1, int(2 * s)))

    # 4. 纯白加粗镜头取景四角定位标 (高反差视觉锚点，小尺寸下清晰可辨)
    cx, cy = size / 2.0, size / 2.0
    cw = int(72 * s)  # 半宽
    ch = int(72 * s)  # 半高
    arm = int(25 * s) # 拐角臂长
    b_width = max(2, int(6.5 * s))
    bracket_color = (255, 255, 255, 255)

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

    # 5. 核心 OCR 横向激光扫描线 (耀眼亮黄微光带，象征毫秒级文字识别)
    scan_y = int(cy)
    glow_line = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    g_draw = ImageDraw.Draw(glow_line)
    # 扫描发光弥散晕
    g_draw.line([(cx - cw + int(4 * s), scan_y), (cx + cw - int(4 * s), scan_y)], fill=(255, 215, 0, 160), width=int(9 * s))
    glow_line = glow_line.filter(ImageFilter.GaussianBlur(int(3 * s)))
    img = Image.alpha_composite(img, glow_line)

    draw = ImageDraw.Draw(img)
    # 激光核心高亮线
    draw.line([(cx - cw + int(6 * s), scan_y), (cx + cw - int(6 * s), scan_y)], fill=(255, 245, 160, 255), width=max(1, int(3.0 * s)))

    # 6. 中心科技感六边形相机快门光圈 + 光学聚焦点 (经典相机聚焦与光学识别元素)
    inner_r = int(27 * s)
    points = []
    for a in range(6):
        angle = a * (2 * math.pi / 6) - math.pi / 6
        points.append((cx + inner_r * math.cos(angle), cy + inner_r * math.sin(angle)))
    # 六边形快门边框 (加粗纯白)
    draw.polygon(points, outline=(255, 255, 255, 255), width=max(2, int(4.0 * s)))

    # 中心光学发光核 (金黄外晕 + 纯白高光焦点)
    draw.ellipse([cx - int(10 * s), cy - int(10 * s), cx + int(10 * s), cy + int(10 * s)], fill=(255, 225, 60, 255))
    draw.ellipse([cx - int(5 * s), cy - int(5 * s), cx + int(5 * s), cy + int(5 * s)], fill=(255, 255, 255, 255))

    return img

if __name__ == "__main__":
    os.makedirs("resources", exist_ok=True)

    # 1. 生成 256x256 高清原图 (明亮鲜艳高对比)
    icon_256 = create_bright_vibrant_icon(256)
    icon_256.save("resources/app_icon_256.png", "PNG")
    print("Saved resources/app_icon_256.png (Bright & High Contrast)")

    # 2. 生成多分辨率 Windows 高清 .ico 阵列
    sizes = [16, 24, 32, 48, 64, 128, 256]
    ico_images = [create_bright_vibrant_icon(sz) for sz in sizes]

    icon_256.save("resources/app.ico", format="ICO", sizes=[(sz, sz) for sz in sizes], append_images=ico_images)
    print("Saved resources/app.ico with multiple sizes [16, 24, 32, 48, 64, 128, 256]")
