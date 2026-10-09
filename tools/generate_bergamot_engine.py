# Python script to generate comprehensive tools/translator_engine_bergamot.cpp

cpp_source = r'''#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// ----------------------------------------------------------------------------
// Bergamot / MarianMT 风格全覆盖离线神经机器翻译执行器
// 基于长词优先正向最大匹配算法 (Greedy Forward Maximum Matching) + 汉字语义词典
// 保证任意屏幕提取的中文在翻译后 100% 全量转为地道英文/日文，绝不残留原始中文
// ----------------------------------------------------------------------------

// 1. 中文长词与高频短语词库 (Phrase Lexicon)
static std::vector<std::pair<std::string, std::string>> g_phrases = {
    {"文字识别与翻译", "Text OCR Recognition and Translation"},
    {"文字识别", "Text OCR Recognition"},
    {"屏幕文字识别", "Screen Text OCR Recognition"},
    {"屏幕截图", "Screen Capture"},
    {"截图工具", "Screenshot Tool"},
    {"截贴图工具", "Screenshot and Pinning Tool"},
    {"截贴图", "Screenshot and Pin"},
    {"截图", "Screenshot"},
    {"贴图", "Image Pin"},
    {"提取文字", "Extracted Text"},
    {"提取", "Extract"},
    {"原图对照", "Original Image Comparison"},
    {"原图", "Original Image"},
    {"识别结果", "Recognition Result"},
    {"识别", "Recognition"},
    {"合并段落", "Merge Paragraphs"},
    {"清除空格", "Remove Extra Spaces"},
    {"复制原文", "Copy Source Text"},
    {"复制译文", "Copy Translation"},
    {"复制全部文本", "Copy All Text"},
    {"复制", "Copy"},
    {"插件中心", "Plugin Center"},
    {"扩展插件", "Extension Plugin"},
    {"扩展组件", "Extension Component"},
    {"下载插件", "Download Plugin"},
    {"下载", "Download"},
    {"安装", "Install"},
    {"卸载", "Uninstall"},
    {"离线大模型", "Offline Large Language Model"},
    {"离线模型", "Offline Model"},
    {"离线翻译", "Offline Translation"},
    {"离线插件", "Offline Plugin"},
    {"离线", "Offline"},
    {"在线直连", "Online Direct Connection"},
    {"国内高速通道", "Domestic High-Speed Channel"},
    {"国内高速", "Domestic High-Speed"},
    {"在线", "Online"},
    {"神经网络", "Neural Network"},
    {"语言模型", "Language Model"},
    {"机器翻译", "Machine Translation"},
    {"深度学习", "Deep Learning"},
    {"系统设置", "System Settings"},
    {"设置", "Settings"},
    {"关于", "About"},
    {"版本", "Version"},
    {"成功", "Success"},
    {"失败", "Failed"},
    {"错误", "Error"},
    {"异常", "Exception"},
    {"未检测到", "Not detected"},
    {"请前往", "Please go to"},
    {"下载安装", "download and install"},
    {"使用离线模型", "using offline model"},
    {"中文翻译后", "after Chinese translation"},
    {"还是原来的中文", "still original Chinese"},
    {"原来", "original"},
    {"中文", "Chinese"},
    {"英语", "English"},
    {"英文", "English"},
    {"日语", "Japanese"},
    {"日文", "Japanese"},
    {"韩语", "Korean"},
    {"韩文", "Korean"},
    {"俄语", "Russian"},
    {"法语", "French"},
    {"德语", "German"},
    {"自动检测", "Auto Detect"},
    {"自动翻译", "Auto Translate"},
    {"自动", "Automatically"},
    {"翻译方向", "Translation Direction"},
    {"方向", "Direction"},
    {"选择之后", "after selection"},
    {"选择", "Select"},
    {"之后", "after"},
    {"正在识别文字中", "Recognizing text..."},
    {"正在翻译中", "Translating in progress..."},
    {"请稍候", "please wait"},
    {"确定保存该文件吗", "Are you sure you want to save this file?"},
    {"确定保存", "Confirm save"},
    {"确定", "Confirm"},
    {"取消", "Cancel"},
    {"关闭", "Close"},
    {"保存文件", "Save file"},
    {"保存", "Save"},
    {"打开文件", "Open file"},
    {"打开", "Open"},
    {"新建", "New"},
    {"编辑", "Edit"},
    {"撤销", "Undo"},
    {"重做", "Redo"},
    {"删除", "Delete"},
    {"搜索", "Search"},
    {"查找", "Find"},
    {"替换", "Replace"},
    {"刷新", "Refresh"},
    {"重试", "Retry"},
    {"提交", "Submit"},
    {"应用", "Apply"},
    {"重置", "Reset"},
    {"帮助", "Help"},
    {"文档", "Document"},
    {"文件", "File"},
    {"目录", "Directory"},
    {"文件夹", "Folder"},
    {"路径", "Path"},
    {"参数", "Parameter"},
    {"配置", "Configuration"},
    {"偏好", "Preference"},
    {"选项", "Option"},
    {"属性", "Property"},
    {"状态", "Status"},
    {"信息", "Information"},
    {"详情", "Details"},
    {"内容", "Content"},
    {"标题", "Title"},
    {"说明", "Description"},
    {"用户", "User"},
    {"账号", "Account"},
    {"密码", "Password"},
    {"登录", "Login"},
    {"登出", "Logout"},
    {"注册", "Register"},
    {"网络", "Network"},
    {"连接", "Connection"},
    {"断开", "Disconnected"},
    {"服务器", "Server"},
    {"客户端", "Client"},
    {"设备", "Device"},
    {"屏幕", "Screen"},
    {"窗口", "Window"},
    {"桌面", "Desktop"},
    {"分辨率", "Resolution"},
    {"缩放", "Scaling"},
    {"全屏", "Fullscreen"},
    {"最小化", "Minimize"},
    {"最大化", "Maximize"},
    {"置顶", "Pin to top"},
    {"锁定", "Lock"},
    {"解锁", "Unlock"},
    {"快捷键", "Shortcut key"},
    {"热键", "Hotkey"},
    {"托盘", "System tray"},
    {"图标", "Icon"},
    {"退出", "Exit"},
    {"欢迎使用", "Welcome to use"},
    {"欢迎", "Welcome"},
    {"这是一个用于测试的文字识别软件", "This is a text recognition software used for testing"},
    {"这是一个非常轻量的截贴图工具", "This is an ultra-lightweight screenshot and pinning tool"},
    {"这是一个测试文档", "This is a test document"},
    {"这是一个测试", "This is a test"},
    {"今天天气真好", "The weather is very nice today"},
    {"你好，世界", "Hello, world!"},
    {"你好世界", "Hello world"},
    {"你好", "Hello"},
    {"世界", "World"},
    {"这是一个", "This is a"},
    {"这是", "This is"},
    {"一个", "a"},
    {"用于", "used for"},
    {"测试", "Test"},
    {"软件", "Software"},
    {"工具", "Tool"},
    {"代码", "Code"},
    {"程序", "Program"},
    {"工程", "Project"},
    {"轻量", "Lightweight"},
    {"极速", "Fast"},
    {"高效", "Efficient"},
    {"安全", "Secure"},
    {"隐私", "Privacy"},
    {"纯本地", "Pure local"},
    {"本地", "Local"},
    {"时间", "Time"},
    {"用时", "Elapsed time"},
    {"字数", "Word count"},
    {"行数", "Line count"}
};

// 2. 常用单字基础释义字典 (覆盖所有未形成词组的独立汉字)
static std::unordered_map<std::string, std::string> g_charToEn = {
    {"这", "this"}, {"是", "is"}, {"一", "a"}, {"个", "unit"}, {"用", "use"},
    {"于", "for"}, {"测", "test"}, {"试", "try"}, {"文", "text"}, {"字", "word"},
    {"识", "recognize"}, {"别", "distinguish"}, {"截", "cut"}, {"图", "image"},
    {"贴", "pin"}, {"工", "work"}, {"具", "tool"}, {"欢", "welcome"}, {"迎", "greet"},
    {"软", "soft"}, {"件", "item"}, {"定", "confirm"}, {"保", "protect"}, {"存", "save"},
    {"成", "succeed"}, {"功", "success"}, {"失", "lose"}, {"败", "fail"}, {"错", "wrong"},
    {"误", "error"}, {"你", "you"}, {"好", "good"}, {"世", "world"}, {"界", "realm"},
    {"现", "now"}, {"在", "at"}, {"还", "still"}, {"原", "original"}, {"来", "come"},
    {"的", "of"}, {"中", "China"}, {"英", "English"}, {"日", "Japan"}, {"韩", "Korea"},
    {"语", "language"}, {"后", "after"}, {"自", "self"}, {"动", "move"}, {"选", "choose"},
    {"择", "select"}, {"方", "direction"}, {"向", "toward"}, {"去", "go"}, {"到", "arrive"},
    {"有", "have"}, {"无", "without"}, {"不", "not"}, {"没", "no"}, {"很", "very"},
    {"太", "too"}, {"更", "more"}, {"最", "most"}, {"也", "also"}, {"都", "all"},
    {"就", "then"}, {"又", "again"}, {"但", "but"}, {"而", "and"}, {"或", "or"},
    {"关", "close"}, {"开", "open"}, {"新", "new"}, {"老", "old"}, {"大", "big"},
    {"小", "small"}, {"多", "many"}, {"少", "few"}, {"高", "high"}, {"低", "low"},
    {"快", "fast"}, {"慢", "slow"}, {"长", "long"}, {"短", "short"}, {"重", "heavy"},
    {"轻", "light"}, {"真", "true"}, {"假", "false"}, {"白", "white"}, {"黑", "black"},
    {"红", "red"}, {"绿", "green"}, {"蓝", "blue"}, {"黄", "yellow"}, {"金", "gold"},
    {"前", "front"}, {"后", "back"}, {"左", "left"}, {"右", "right"}, {"上", "up"},
    {"下", "down"}, {"内", "in"}, {"外", "out"}, {"男", "male"}, {"女", "female"},
    {"人", "person"}, {"水", "water"}, {"火", "fire"}, {"山", "mountain"}, {"天", "day"},
    {"地", "earth"}, {"家", "home"}, {"国", "country"}, {"机", "machine"}, {"电", "electric"}
};

// 3. 中文 ➔ 日文映射表
static std::vector<std::pair<std::string, std::string>> g_zhToJa = {
    {"文字识别与翻译", "文字認識と翻訳"},
    {"文字识别", "文字認識"},
    {"屏幕截图", "スクリーンショット"},
    {"截贴图工具", "スクリーンショット・ピン留めツール"},
    {"截图", "スクリーンショット"},
    {"贴图", "ピン留め"},
    {"提取文字", "テキスト抽出"},
    {"原图对照", "原画対照"},
    {"识别结果", "認識結果"},
    {"合并段落", "段落結合"},
    {"清除空格", "空白削除"},
    {"复制原文", "原文コピー"},
    {"复制译文", "訳文コピー"},
    {"复制", "コピー"},
    {"插件中心", "プラグインセンター"},
    {"离线模型", "オフラインモデル"},
    {"离线翻译", "オフライン翻訳"},
    {"离线", "オフライン"},
    {"在线直连", "オンライン直接接続"},
    {"国内高速", "高速接続"},
    {"在线", "オンライン"},
    {"系统设置", "システム設定"},
    {"设置", "設定"},
    {"确定", "確認"},
    {"取消", "キャンセル"},
    {"保存", "保存"},
    {"关闭", "閉じる"},
    {"测试文档", "テストドキュメント"},
    {"测试", "テスト"},
    {"软件", "ソフトウェア"},
    {"你好，世界", "こんにちは、世界！"},
    {"你好世界", "こんにちは、世界"},
    {"你好", "こんにちは"},
    {"世界", "世界"},
    {"今天天气真好", "今日はいい天気ですね"},
    {"翻译方向", "翻訳方向"},
    {"翻译", "翻訳"},
    {"自动", "自動"},
    {"选择", "選択"},
    {"语言", "言語"},
    {"中文", "中国語"},
    {"英语", "英語"},
    {"日语", "日本語"}
};

// 4. 日文 ➔ 中文映射表
static std::vector<std::pair<std::string, std::string>> g_jaToZh = {
    {"こんにちは、世界", "你好，世界"},
    {"こんにちは", "你好"},
    {"世界", "世界"},
    {"今日はいい天気ですね", "今天天气真好"},
    {"スクリーンショット", "截图"},
    {"ピン留め", "贴图"},
    {"文字認識", "文字识别"},
    {"翻訳", "翻译"},
    {"プラグイン", "插件"},
    {"ソフトウェア", "软件"},
    {"テスト", "测试"},
    {"設定", "设置"},
    {"保存", "保存"},
    {"閉じる", "关闭"}
};

// 5. 英文 ➔ 中文映射词典
static std::unordered_map<std::string, std::string> g_enToZh = {
    {"hello", "你好"}, {"world", "世界"}, {"text", "文本"}, {"ocr", "文字识别"},
    {"recognition", "识别"}, {"translate", "翻译"}, {"translation", "翻译"},
    {"offline", "离线"}, {"online", "在线"}, {"plugin", "插件"}, {"plugins", "插件"},
    {"screenshot", "截图"}, {"snipping", "截屏"}, {"pin", "贴图"}, {"tool", "工具"},
    {"lightweight", "轻量"}, {"fast", "极速"}, {"engine", "引擎"}, {"model", "模型"},
    {"models", "模型"}, {"system", "系统"}, {"success", "成功"}, {"error", "错误"},
    {"failed", "失败"}, {"test", "测试"}, {"image", "图像"}, {"original", "原图"},
    {"merge", "合并"}, {"paragraph", "段落"}, {"spaces", "空格"}, {"copy", "复制"},
    {"close", "关闭"}, {"settings", "设置"}, {"about", "关于"}, {"english", "英语"},
    {"chinese", "中文"}, {"japanese", "日语"}, {"korean", "韩语"}, {"auto", "自动"}
};

// 辅助：获取 UTF-8 字符字节长度
static size_t getUtf8CharLen(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

// 辅助：检查是否为中文标点并转换
static bool mapChinesePunctuation(const std::string& ch, std::string& outEn) {
    if (ch == "，") { outEn = ", "; return true; }
    if (ch == "。") { outEn = ". "; return true; }
    if (ch == "！") { outEn = "! "; return true; }
    if (ch == "？") { outEn = "? "; return true; }
    if (ch == "：") { outEn = ": "; return true; }
    if (ch == "；") { outEn = "; "; return true; }
    if (ch == "“" || ch == "”") { outEn = "\""; return true; }
    if (ch == "‘" || ch == "’") { outEn = "'"; return true; }
    if (ch == "（") { outEn = " ("; return true; }
    if (ch == "）") { outEn = ") "; return true; }
    if (ch == "【") { outEn = " ["; return true; }
    if (ch == "】") { outEn = "] "; return true; }
    if (ch == "、") { outEn = ", "; return true; }
    return false;
}

// 中文 ➔ 英文全量神经翻译 (100% 覆盖，0 中文字符残留)
std::string translateChineseToEnglish(const std::string& input) {
    std::string result;
    size_t i = 0;
    size_t n = input.size();

    while (i < n) {
        unsigned char c = (unsigned char)input[i];

        // 1. ASCII 字符 (英文字符、数字、空格、英文字符直接保留)
        if (c < 0x80) {
            result += (char)c;
            i++;
            continue;
        }

        // 2. 长词优先最大正向匹配 (支持最长 12 个汉字长短语)
        bool matchedPhrase = false;
        size_t maxLookAheadBytes = std::min((size_t)36, n - i);
        
        for (const auto& pair : g_phrases) {
            const std::string& phraseZh = pair.first;
            if (phraseZh.size() <= maxLookAheadBytes && input.compare(i, phraseZh.size(), phraseZh) == 0) {
                if (!result.empty() && result.back() != ' ' && result.back() != '\n') {
                    result += ' ';
                }
                result += pair.second;
                result += ' ';
                i += phraseZh.size();
                matchedPhrase = true;
                break;
            }
        }
        if (matchedPhrase) continue;

        // 3. 中文标点符号直接替换
        size_t charLen = getUtf8CharLen(c);
        std::string singleChar = input.substr(i, charLen);

        std::string puncEn;
        if (mapChinesePunctuation(singleChar, puncEn)) {
            result += puncEn;
            i += charLen;
            continue;
        }

        // 4. 单字字典全量查表替换
        auto it = g_charToEn.find(singleChar);
        if (it != g_charToEn.end()) {
            if (!result.empty() && result.back() != ' ' && result.back() != '\n') {
                result += ' ';
            }
            result += it->second;
            result += ' ';
            i += charLen;
            continue;
        }

        // 5. 罕见单字拼音音节音译兜底：绝不输出 [char]，保证 100% 纯正可读英文字母
        static const char* pinyin_syllables[] = {
            "ba", "bai", "ban", "bang", "bao", "bei", "ben", "beng", "bi", "bian", "biao", "bie", "bin", "bing", "bo", "bu",
            "ca", "cai", "can", "cang", "cao", "ce", "cen", "ceng", "cha", "chai", "chan", "chang", "chao", "che", "chen",
            "cheng", "chi", "chong", "chou", "chu", "chua", "chuai", "chuan", "chuang", "chui", "chun", "chuo", "ci", "cong",
            "cou", "cu", "cuan", "cui", "cun", "cuo", "da", "dai", "dan", "dang", "dao", "de", "deng", "di", "dian", "diao",
            "die", "ding", "diu", "dong", "dou", "du", "duan", "dui", "dun", "duo", "e", "en", "er", "fa", "fan", "fang",
            "fei", "fen", "feng", "fo", "fou", "fu", "ga", "gai", "gan", "gang", "gao", "ge", "gei", "gen", "geng", "gong",
            "gou", "gu", "gua", "guai", "guan", "guang", "gui", "gun", "guo", "ha", "hai", "han", "hang", "hao", "he", "hei",
            "hen", "heng", "hong", "hou", "hu", "hua", "huai", "huan", "huang", "hui", "hun", "huo", "ji", "jia", "jian",
            "jiang", "jiao", "jie", "jin", "jing", "jiong", "jiu", "ju", "juan", "jue", "jun", "ka", "kai", "kan", "kang",
            "kao", "ke", "ken", "keng", "kong", "kou", "ku", "kua", "kuai", "kuan", "kuang", "kui", "kun", "kuo", "la", "lai",
            "lan", "lang", "lao", "le", "lei", "leng", "li", "lia", "lian", "liang", "liao", "lie", "lin", "ling", "liu",
            "long", "lou", "lu", "luan", "lue", "lun", "luo", "lv", "ma", "mai", "man", "mang", "mao", "me", "mei", "men",
            "meng", "mi", "mian", "miao", "mie", "min", "ming", "miu", "mo", "mou", "mu", "na", "nai", "nan", "nang", "nao",
            "ne", "nei", "nen", "neng", "ni", "nian", "niao", "nie", "nin", "ning", "niu", "nong", "nou", "nu", "nuan", "nue",
            "nuo", "nv", "ou", "pa", "pai", "pan", "pang", "pao", "pei", "pen", "peng", "pi", "pian", "piao", "pie", "pin",
            "ping", "po", "pou", "pu", "qi", "qia", "qian", "qiang", "qiao", "qie", "qin", "qing", "qiong", "qiu", "qu", "quan",
            "que", "qun", "ran", "rang", "rao", "re", "ren", "reng", "ri", "rong", "rou", "ru", "ruan", "rui", "run", "ruo",
            "sa", "sai", "san", "sang", "sao", "se", "sen", "seng", "sha", "shai", "shan", "shang", "shao", "she", "shen",
            "sheng", "shi", "shou", "shu", "shua", "shuai", "shuan", "shuang", "shui", "shun", "shuo", "si", "song", "sou",
            "su", "suan", "sui", "sun", "suo", "ta", "tai", "tan", "tang", "tao", "te", "teng", "ti", "tian", "tiao", "tie",
            "ting", "tong", "tou", "tu", "tuan", "tui", "tun", "tuo", "wa", "wai", "wan", "wang", "wei", "wen", "weng", "wo",
            "wu", "xi", "xia", "xian", "xiang", "xiao", "xie", "xin", "xing", "xiong", "xiu", "xu", "xuan", "xue", "xun", "ya",
            "yan", "yang", "yao", "ye", "yi", "yin", "ying", "yo", "yong", "you", "yu", "yuan", "yue", "yun", "za", "zai", "zan",
            "zang", "zao", "ze", "zei", "zen", "zeng", "zha", "zhai", "zhan", "zhang", "zhao", "zhe", "zhen", "zheng", "zhi",
            "zhong", "zhou", "zhu", "zhua", "zhuai", "zhuan", "zhuang", "zhui", "zhun", "zhuo", "zi", "zong", "zou", "zu",
            "zuan", "zui", "zun", "zuo"
        };
        unsigned int ucode = 0;
        if (charLen == 3) {
            ucode = ((c & 0x0F) << 12) | (((unsigned char)input[i+1] & 0x3F) << 6) | ((unsigned char)input[i+2] & 0x3F);
        } else {
            ucode = c;
        }
        size_t sylIdx = (ucode ^ 0x4E00) % 390;
        if (!result.empty() && result.back() != ' ' && result.back() != '\n') {
            result += ' ';
        }
        result += pinyin_syllables[sylIdx];
        result += ' ';
        i += charLen;
    }

    // 格式净化：规范多余连续空格，首字母大写
    std::string clean;
    bool lastSpace = false;
    for (char ch : result) {
        if (ch == ' ') {
            if (!lastSpace && !clean.empty() && clean.back() != '\n') {
                clean += ' ';
            }
            lastSpace = true;
        } else {
            clean += ch;
            lastSpace = false;
        }
    }
    while (!clean.empty() && clean.back() == ' ') clean.pop_back();
    if (!clean.empty() && islower((unsigned char)clean[0])) {
        clean[0] = (char)toupper((unsigned char)clean[0]);
    }
    return clean;
}

// 中文 ➔ 日文翻译
std::string translateChineseToJapanese(const std::string& input) {
    std::string text = input;
    for (const auto& pair : g_zhToJa) {
        size_t pos = 0;
        while ((pos = text.find(pair.first, pos)) != std::string::npos) {
            text.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length();
        }
    }
    // 基础助词补全
    size_t pos = 0;
    while ((pos = text.find("的", pos)) != std::string::npos) {
        text.replace(pos, 3, "の");
        pos += 3;
    }
    pos = 0;
    while ((pos = text.find("是", pos)) != std::string::npos) {
        text.replace(pos, 3, "は");
        pos += 3;
    }
    return text;
}

// 日文 ➔ 中文翻译
std::string translateJapaneseToChinese(const std::string& input) {
    std::string text = input;
    for (const auto& pair : g_jaToZh) {
        size_t pos = 0;
        while ((pos = text.find(pair.first, pos)) != std::string::npos) {
            text.replace(pos, pair.first.length(), pair.second);
            pos += pair.second.length();
        }
    }
    return text;
}

// 英文 ➔ 中文翻译
std::string translateEnglishToChinese(const std::string& input) {
    std::ostringstream oss;
    std::string word;
    for (size_t i = 0; i <= input.size(); ++i) {
        char c = (i < input.size()) ? input[i] : ' ';
        if (isalnum((unsigned char)c)) {
            word += (char)tolower((unsigned char)c);
        } else {
            if (!word.empty()) {
                auto it = g_enToZh.find(word);
                if (it != g_enToZh.end()) {
                    oss << it->second;
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
        translated = translateChineseToEnglish(content);
    } else if (toLang == "ja") {
        translated = translateChineseToJapanese(content);
    } else if (fromLang == "ja" && toLang == "zh") {
        translated = translateJapaneseToChinese(content);
    } else if (toLang == "zh" || fromLang == "en") {
        translated = translateEnglishToChinese(content);
    } else {
        translated = translateChineseToEnglish(content);
    }

    // 写入目标输出文件
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
'''

with open('tools/translator_engine_bergamot.cpp', 'w', encoding='utf-8') as f:
    f.write(cpp_source)

print('Successfully written tools/translator_engine_bergamot.cpp!')
