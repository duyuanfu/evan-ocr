import sys
import os
import json
import warnings
warnings.filterwarnings("ignore")

def init_model():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    models_dir = os.path.join(base_dir, "models")
    resizer = os.path.join(models_dir, "image_resizer.onnx")
    encoder = os.path.join(models_dir, "encoder.onnx")
    decoder = os.path.join(models_dir, "decoder.onnx")
    tokenizer = os.path.join(models_dir, "tokenizer.json")

    kwargs = {}
    if os.path.exists(resizer): kwargs["image_resizer_path"] = resizer
    if os.path.exists(encoder): kwargs["encoder_path"] = encoder
    if os.path.exists(decoder): kwargs["decoder_path"] = decoder
    if os.path.exists(tokenizer): kwargs["tokenizer_json"] = tokenizer

    from rapid_latex_ocr import LaTeXOCR
    return LaTeXOCR(**kwargs)

def process_image(model, img_path):
    if not img_path or not os.path.exists(img_path):
        return {"code": 400, "error": f"Image not found: {img_path}"}
    try:
        result, elapse = model(img_path)
        return {"code": 200, "latex": result if result else "", "elapse": elapse}
    except Exception as e:
        return {"code": 500, "error": str(e)}

def main():
    if "--daemon" in sys.argv:
        # 常驻守护模式：一次性预加载 178MB 模型，后续识别 0ms 冷启动，秒级出字！
        try:
            model = init_model()
            sys.stdout.write("READY\n")
            sys.stdout.flush()
        except Exception as e:
            sys.stdout.write(json.dumps({"code": 500, "error": f"Model init failed: {e}"}) + "\n")
            sys.stdout.flush()
            return

        while True:
            try:
                line = sys.stdin.readline()
                if not line:
                    break
                img_path = line.strip()
                if not img_path:
                    continue
                if img_path in ("QUIT", "EXIT"):
                    break
                res = process_image(model, img_path)
                sys.stdout.write(json.dumps(res, ensure_ascii=False) + "\n")
                sys.stdout.flush()
            except Exception as e:
                sys.stdout.write(json.dumps({"code": 500, "error": str(e)}) + "\n")
                sys.stdout.flush()
        return

    # 单次命令行执行模式 (向前兼容)
    img_path = None
    for i in range(1, len(sys.argv)):
        if sys.argv[i] in ("--image", "-i", "-img") and i + 1 < len(sys.argv):
            img_path = sys.argv[i + 1]
            break
        elif not sys.argv[i].startswith("-") and os.path.exists(sys.argv[i]):
            img_path = sys.argv[i]

    try:
        model = init_model()
        res = process_image(model, img_path)
        print(json.dumps(res, ensure_ascii=False))
    except Exception as e:
        print(json.dumps({"code": 500, "error": str(e)}, ensure_ascii=False))

if __name__ == "__main__":
    main()
