import os
import math
from PIL import Image, ImageDraw, ImageFilter

def create_high_contrast_icon(size=256):
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s = size / 256.0

    # 1. 外层柔和深色投射阴影 (让图标在浅色/白底背景上脱颖而出)
    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    s_draw = ImageDraw.Draw(shadow)
    margin = int(14 * s)
    s_draw.rounded_rectangle(
        [margin, margin + int(6 * s), size - margin, size - margin + int(6 * s)],
        radius=int(56 * s),
        fill=(0, 20, 60, 140)
    )
    shadow = shadow.filter(ImageFilter.GaussianBlur(int(8 * s)))
    img = Image.alpha_composite(shadow, img)

    # 2. 鲜艳高饱和度科技极光蓝渐变底板 (从顶部 #00C6FF 到 底部 #0052D4)
    # 高饱和、高穿透力，在 Windows 黑色/白色任务栏托盘均极其醒目！
    bg_canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    bg_draw = ImageDraw.Draw(bg_canvas)
    radius = int(54 * s)
    rect = [margin, margin, size - margin, size - margin]

    h_total = rect[3] - rect[1]
    for i in range(int(h_total)):
        curr_y = int(rect[1] + i)
        ratio = i / float(h_total)
        # 鲜亮电光青蓝 -> 皇家深海蓝
        r = int(0 + ratio * 0)
        g = int(198 - ratio * 116) # 198 -> 82
        b = int(255 - ratio * 43)  # 255 -> 212
        bg_draw.line([(rect[0], curr_y), (rect[2], curr_y)], fill=(r, g, b, 255))

    mask = Image.new("L", (size, size), 0)
    mask_d = ImageDraw.Draw(mask)
    mask_d.rounded_rectangle(rect, radius=radius, fill=255)
    bg_canvas.putalpha(mask)
    img = Image.alpha_composite(img, bg_canvas)

    draw = ImageDraw.Draw(img)

    # 3. 顶部边缘 1px 晶莹高光线
    draw.rounded_rectangle(rect, radius=radius, outline=(255, 255, 255, 180), width=max(1, int(2 * s)))

    # 4. 镜头取景/截图标靶四角定位框 (纯白色高反差，极具穿透力)
    cx, cy = size / 2.0, size / 2.0
    cw = int(68 * s) # 半宽
    ch = int(68 * s) # 半高
    arm = int(24 * s) # 拐角长
    b_width = max(2, int(6.0 * s))
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

    # 5. 中央高辨识度字符 “E” (Evan 标志) - 粗壮有力
    ew = int(24 * s)  # 竖向主干厚度
    ex = int(cx - 36 * s)
    ey_top = int(cy - 45 * s)
    ey_bot = int(cy + 45 * s)
    bar_len = int(68 * s)

    # E 的垂直主干
    draw.rounded_rectangle([ex, ey_top, ex + ew, ey_bot], radius=int(4 * s), fill=(255, 255, 255, 255))
    # E 的顶横
    draw.rounded_rectangle([ex, ey_top, ex + bar_len, ey_top + int(18 * s)], radius=int(3 * s), fill=(255, 255, 255, 255))
    # E 的中横 (稍短)
    mid_y = int(cy - 8 * s)
    draw.rounded_rectangle([ex, mid_y, ex + int(bar_len * 0.72), mid_y + int(16 * s)], radius=int(3 * s), fill=(255, 255, 255, 255))
    # E 的底横
    draw.rounded_rectangle([ex, ey_bot - int(18 * s), ex + bar_len, ey_bot], radius=int(3 * s), fill=(255, 255, 255, 255))

    # 6. 象征 OCR 文字识别的水平激光扫描线 (耀眼金橙色 #FFB800 配合发光渐层)
    beam_canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    beam_draw = ImageDraw.Draw(beam_canvas)
    beam_y = int(cy + 18 * s)
    beam_h = int(6 * s)
    # 扫描线发光底晕
    beam_draw.line([(int(cx - cw + 8 * s), beam_y), (int(cx + cw - 8 * s), beam_y)], fill=(255, 170, 0, 180), width=int(10 * s))
    beam_canvas = beam_canvas.filter(ImageFilter.GaussianBlur(int(3 * s)))
    img = Image.alpha_composite(img, beam_canvas)

    draw = ImageDraw.Draw(img)
    # 扫描线中央高亮核心
    draw.line([(int(cx - cw + 10 * s), beam_y), (int(cx + cw - 10 * s), beam_y)], fill=(255, 215, 0, 255), width=max(2, int(3.5 * s)))
    # 扫描线右侧闪耀光点
    spark_x = int(cx + cw - 14 * s)
    draw.ellipse([spark_x - int(5 * s), beam_y - int(5 * s), spark_x + int(5 * s), beam_y + int(5 * s)], fill=(255, 255, 255, 255))

    return img

if __name__ == "__main__":
    os.makedirs("resources", exist_ok=True)
    
    # 1. 生成 256x256 高清 PNG
    icon_256 = create_high_contrast_icon(256)
    icon_256.save("resources/app_icon_256.png", "PNG")
    print("Saved resources/app_icon_256.png")

    # 2. 生成多尺寸数组注入高清 .ico (包含 Windows 任务栏 16x16, 24x24, 32x32, 48x48, 64x64, 128x128, 256x256)
    sizes = [16, 24, 32, 48, 64, 128, 256]
    ico_images = []
    for sz in sizes:
        ico_images.append(create_high_contrast_icon(sz))

    # 保存复合尺寸 .ico
    icon_256.save("resources/app.ico", format="ICO", sizes=[(s, s) for s in sizes], append_images=ico_images)
    print("Saved resources/app.ico with multiple sizes [16, 24, 32, 48, 64, 128, 256]")
