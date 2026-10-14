import sys
import os
import json
import warnings
warnings.filterwarnings("ignore")

def main():
    img_path = None
    for i in range(1, len(sys.argv)):
        if sys.argv[i] in ("--image", "-i", "-img") and i + 1 < len(sys.argv):
            img_path = sys.argv[i + 1]
            break
        elif not sys.argv[i].startswith("-") and os.path.exists(sys.argv[i]):
            img_path = sys.argv[i]

    if not img_path or not os.path.exists(img_path):
        print(json.dumps({"code": 400, "error": f"Image not found: {img_path}"}))
        return

    base_dir = os.path.dirname(os.path.abspath(__file__))
    models_dir = os.path.join(base_dir, "models")

    resizer = os.path.join(models_dir, "image_resizer.onnx")
    encoder = os.path.join(models_dir, "encoder.onnx")
    decoder = os.path.join(models_dir, "decoder.onnx")
    tokenizer = os.path.join(models_dir, "tokenizer.json")

    try:
        from rapid_latex_ocr import LaTeXOCR
        kwargs = {}
        if os.path.exists(resizer): kwargs["image_resizer_path"] = resizer
        if os.path.exists(encoder): kwargs["encoder_path"] = encoder
        if os.path.exists(decoder): kwargs["decoder_path"] = decoder
        if os.path.exists(tokenizer): kwargs["tokenizer_json"] = tokenizer

        model = LaTeXOCR(**kwargs)
        result, elapse = model(img_path)
        output = {
            "code": 200,
            "latex": result,
            "elapse": elapse
        }
        print(json.dumps(output, ensure_ascii=False))
    except Exception as e:
        print(json.dumps({"code": 500, "error": str(e)}, ensure_ascii=False))

if __name__ == "__main__":
    main()
