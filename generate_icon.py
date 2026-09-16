import os
import math
from PIL import Image, ImageDraw, ImageFilter

def create_app_icon(size=256):
    # 创建高分辨率 RGBA 图像
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # 比例因子
    s = size / 256.0
    
    # 1. 绘制柔和现代外阴影
    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s_draw = ImageDraw.Draw(shadow)
    shadow_margin = int(14 * s)
    s_draw.rounded_rectangle(
        [shadow_margin, shadow_margin + int(6 * s), size - shadow_margin, size - shadow_margin + int(6 * s)],
        radius=int(50 * s),
        fill=(15, 23, 42, 120)
    )
    shadow = shadow.filter(ImageFilter.GaussianBlur(int(10 * s)))
    img = Image.alpha_composite(shadow, img)
    draw = ImageDraw.Draw(img)
    
    # 2. 绘制高端深蓝科技渐变背景主体 (平滑超椭圆圆角矩形)
    margin = int(16 * s)
    rect = [margin, margin, size - margin, size - margin]
    radius = int(52 * s)
    
    # 渐变底板 (深邃科技星空蓝 -> 皇家海蓝)
    for y in range(margin, size - margin):
        ratio = (y - margin) / (size - 2 * margin)
        r = int(15 + ratio * 20)
        g = int(23 + ratio * 60)
        b = int(42 + ratio * 180)
        # 用 mask 裁剪
        line_img = Image.new("RGBA", (size, 1), (r, g, b, 255))
        mask_line = Image.new("L", (size, 1), 0)
        mask_draw = ImageDraw.Draw(mask_line)
        # 简化逐行绘制：直接绘制圆角矩形
    
    # 优雅底板
    bg_canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    bg_draw = ImageDraw.Draw(bg_canvas)
    for i in range(int(rect[3] - rect[1])):
        curr_y = int(rect[1] + i)
        ratio = i / (rect[3] - rect[1])
        r = int(15 + ratio * 15)
        g = int(23 + ratio * 55)
        b = int(45 + ratio * 165)
        bg_draw.line([(rect[0], curr_y), (rect[2], curr_y)], fill=(r, g, b, 255))
    
    mask = Image.new("L", (size, size), 0)
    mask_d = ImageDraw.Draw(mask)
    mask_d.rounded_rectangle(rect, radius=radius, fill=255)
    bg_canvas.putalpha(mask)
    img = Image.alpha_composite(img, bg_canvas)
    draw = ImageDraw.Draw(img)
    
    # 3. 边框极细微光渐变边 (1.5px)
    draw.rounded_rectangle(rect, radius=radius, outline=(96, 165, 250, 160), width=max(1, int(2 * s)))
    
    # 4. 镜头取景器四角定位标 (高亮电光青绿/青蓝)
    cx, cy = size / 2.0, size / 2.0
    cw = int(68 * s) # 半宽
    ch = int(68 * s) # 半高
    arm = int(22 * s) # 拐角臂长
    bracket_color = (56, 189, 248, 255) # Cyan 400
    b_width = max(2, int(4.5 * s))
    
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
    
    # 5. 核心文字识别光束 (微光横扫描线)
    scan_y = int(cy)
    glow_line = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    g_draw = ImageDraw.Draw(glow_line)
    g_draw.line([(cx - cw + int(8 * s), scan_y), (cx + cw - int(8 * s), scan_y)], fill=(56, 189, 248, 230), width=max(1, int(2.5 * s)))
    # 扫描线微弱上下弥散光晕
    g_draw.line([(cx - cw + int(14 * s), scan_y - int(2*s)), (cx + cw - int(14 * s), scan_y - int(2*s))], fill=(96, 165, 250, 90), width=int(4 * s))
    g_draw.line([(cx - cw + int(14 * s), scan_y + int(2*s)), (cx + cw - int(14 * s), scan_y + int(2*s))], fill=(96, 165, 250, 90), width=int(4 * s))
    img = Image.alpha_composite(img, glow_line)
    draw = ImageDraw.Draw(img)
    
    # 6. 中心科技感 OCR 字符 / 标志 (大写英文字母 "T" 与 "A" 现代极简结合或快门核)
    # 绘制高雅相机光圈六边形或快门多边形
    inner_r = int(24 * s)
    points = []
    for a in range(6):
        angle = a * (2 * math.pi / 6) - math.pi / 6
        points.append((cx + inner_r * math.cos(angle), cy + inner_r * math.sin(angle)))
    draw.polygon(points, outline=(255, 255, 255, 220), width=max(1, int(2.5 * s)))
    
    # 快门中心发光核
    draw.ellipse([cx - int(7 * s), cy - int(7 * s), cx + int(7 * s), cy + int(7 * s)], fill=(255, 255, 255, 255))
    
    return img

os.makedirs("resources", exist_ok=True)

# 生成多分辨率尺寸: 256, 128, 64, 48, 32, 16
sizes = [256, 128, 64, 48, 32, 16]
images = []
for sz in sizes:
    icon_img = create_app_icon(sz)
    images.append(icon_img)
    if sz == 256:
        icon_img.save("resources/app_icon_256.png")

# 保存为专业 multi-resolution Windows .ico 文件
images[0].save("resources/app.ico", format="ICO", sizes=[(s, s) for s in sizes])
print("Successfully generated high-end app.ico and app_icon_256.png!")
