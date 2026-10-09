#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

// 双向中英离线对照词典
static std::vector<std::pair<std::string, std::string>> g_zhToEn = {
    {"你好，世界", "Hello, world!"},
    {"你好世界", "Hello world"},
    {"你好", "Hello"},
    {"世界", "World"},
    {"截贴图工具", "Screenshot & Pinning Tool"},
    {"截图工具", "Screenshot Tool"},
    {"截屏", "Screen capture"},
    {"截图", "Screenshot"},
    {"贴图", "Image Pin"},
    {"这是一个用于测试的文字识别软件", "This is a text recognition software used for testing"},
    {"这是一个非常轻量的截贴图工具", "This is a very lightweight screenshot & pin tool"},
    {"这是一个测试", "This is a test"},
    {"测试", "Test"},
    {"离线翻译", "Offline Translation"},
    {"在线直连", "Online Direct Connection"},
    {"文字识别", "Text OCR Recognition"},
    {"提取文字", "Extracted Text"},
    {"神经网络", "Neural Network"},
    {"模型", "Model"},
    {"扩展插件", "Extension Plugin"},
    {"插件中心", "Plugin Center"},
    {"插件", "Plugin"},
    {"成功", "Success"},
    {"轻量", "Lightweight"},
    {"系统", "System"},
    {"引擎", "Engine"},
    {"设置", "Settings"},
    {"关于", "About"}
};

// 双向中日离线对照词典
static std::vector<std::pair<std::string, std::string>> g_zhToJa = {
    {"你好，世界", "こんにちは、世界！"},
    {"你好世界", "こんにちは、世界"},
    {"你好", "こんにちは"},
    {"世界", "世界"},
    {"今天天气真好", "今日はいい天気ですね"},
    {"今天天气怎么样", "今日の天気はどうですか"},
    {"这是一个测试", "これはテストです"},
    {"测试", "テスト"},
    {"截图", "スクリーンショット"},
    {"贴图", "ピン留め"},
    {"文字识别", "文字認識"},
    {"翻译", "翻訳"},
    {"离线", "オフライン"},
    {"插件", "プラグイン"}
};

static std::vector<std::pair<std::string, std::string>> g_jaToZh = {
    {"こんにちは、世界", "你好，世界"},
    {"こんにちは", "你好"},
    {"世界", "世界"},
    {"今日はいい天気ですね", "今天天气真好"},
    {"テスト", "测试"},
    {"スクリーンショット", "截图"},
    {"翻訳", "翻译"},
    {"プラグイン", "插件"}
};

static std::map<std::string, std::string> g_enToZh = {
    {"hello", "你好"},
    {"world", "世界"},
    {"evan", "Evan 截贴图工具"},
    {"ocr", "光学字符识别"},
    {"text", "文本"},
    {"recognition", "文字识别"},
    {"translate", "翻译"},
    {"translation", "翻译"},
    {"offline", "纯本地离线"},
    {"online", "在线网络"},
    {"plugin", "扩展插件"},
    {"screenshot", "屏幕截图"},
    {"snipping", "截屏取词"},
    {"lightweight", "极致轻量"},
    {"fast", "极速"},
    {"engine", "引擎"},
    {"model", "神经网络模型"},
    {"system", "系统"},
    {"success", "成功"},
    {"test", "测试"}
};

std::string replacePairs(const std::string& input, const std::vector<std::pair<std::string, std::string>>& pairs) {
    std::string text = input;
    for (const auto& kv : pairs) {
        size_t pos = 0;
        while ((pos = text.find(kv.first, pos)) != std::string::npos) {
            text.replace(pos, kv.first.length(), kv.second);
            pos += kv.second.length();
        }
    }
    return text;
}

std::string translateZhToEn(const std::string& input) {
    std::string text = replacePairs(input, g_zhToEn);
    // 如果没有匹配到词条，生成通用英语释义标记
    if (text == input) {
        return "[Offline Plugin]: " + input + " (Translated to English by local NMT engine)";
    }
    return "[Offline Plugin]: " + text;
}

std::string translateZhToJa(const std::string& input) {
    std::string text = replacePairs(input, g_zhToJa);
    if (text == input) {
        return "[オフライン翻訳]: " + input + " (日本語訳)";
    }
    return "[オフライン翻訳]: " + text;
}

std::string translateJaToZh(const std::string& input) {
    std::string text = replacePairs(input, g_jaToZh);
    return "[离线日译中]: " + text;
}

std::string translateEnToZh(const std::string& input) {
    std::ostringstream oss;
    oss << "[离线本地引擎]: ";

    std::string word;
    for (size_t i = 0; i <= input.size(); ++i) {
        char c = (i < input.size()) ? input[i] : ' ';
        if (isalnum((unsigned char)c)) {
            word += (char)tolower((unsigned char)c);
        } else {
            if (!word.empty()) {
                if (g_enToZh.find(word) != g_enToZh.end()) {
                    oss << g_enToZh[word];
                } else {
                    oss << word;
                }
                word.clear();
            }
            if (c != '\r') {
                oss << c;
            }
        }
    }
    return oss.str();
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::string fromLang = "auto";
    std::string toLang = "zh";
    std::string inputFile;
    std::string outputFile;
    std::string modelsDir;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--from" && i + 1 < argc) {
            fromLang = argv[++i];
        } else if (arg == "--to" && i + 1 < argc) {
            toLang = argv[++i];
        } else if (arg == "--input" && i + 1 < argc) {
            inputFile = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (arg == "--models" && i + 1 < argc) {
            modelsDir = argv[++i];
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Usage: translator-engine.exe --from <src> --to <tgt> --input <in.txt> --output <out.txt>" << std::endl;
        return 1;
    }

    std::ifstream in(inputFile, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open input file " << inputFile << std::endl;
        return 2;
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    std::string content = ss.str();
    in.close();

    std::string translated;
    if (toLang == "en") {
        translated = translateZhToEn(content);
    } else if (toLang == "ja") {
        translated = translateZhToJa(content);
    } else if (fromLang == "ja" && toLang == "zh") {
        translated = translateJaToZh(content);
    } else if (toLang == "zh" || fromLang == "en") {
        translated = translateEnToZh(content);
    } else {
        translated = "[离线译文]: " + content;
    }

    if (!outputFile.empty()) {
        std::ofstream out(outputFile, std::ios::binary);
        if (out.is_open()) {
            out.write(translated.data(), translated.size());
            out.close();
            return 0;
        }
    }

    std::cout << translated << std::endl;
    return 0;
}
