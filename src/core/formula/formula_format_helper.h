#pragma once

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QMap>

class FormulaFormatHelper
{
public:
    // 将 LaTeX 包装为块级 Markdown ($$ ... $$)
    static QString toBlockMarkdown(const QString& latex)
    {
        QString trimmed = latex.trimmed();
        if (trimmed.isEmpty()) return QString();
        return QString("$$\n%1\n$$").arg(trimmed);
    }

    // 将 LaTeX 包装为行内 Markdown ($ ... $)
    static QString toInlineMarkdown(const QString& latex)
    {
        QString trimmed = latex.trimmed();
        if (trimmed.isEmpty()) return QString();
        return QString("$%1$").arg(trimmed);
    }

    // 将 LaTeX 转换为适用于 Microsoft Word 直接粘贴为原生公式对象的 MathML XML
    static QString toWordMathML(const QString& latex)
    {
        QString trimmed = latex.trimmed();
        if (trimmed.isEmpty()) return QString();

        // 如果传入的内容本身已经是标准 MathML 标签
        if (trimmed.startsWith("<math", Qt::CaseInsensitive)) {
            return trimmed;
        }

        // 解析并转换为 MathML 结构
        QString mathmlBody = convertLatexToMathMLBody(trimmed);

        // 构造 Word 原生识别的标准 MathML XML 包裹头
        return QString(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<math xmlns=\"http://www.w3.org/1998/Math/MathML\" display=\"block\">\n"
            "  <mrow>\n"
            "    %1\n"
            "  </mrow>\n"
            "</math>"
        ).arg(mathmlBody);
    }

private:
    static QString convertLatexToMathMLBody(const QString& latex)
    {
        QString s = latex;

        // 替换常见的转义与空格
        s.replace(QRegularExpression(R"(\\quad|\\qquad|\\,|\\;|\\!)"), " ");
        s.replace(QRegularExpression(R"(\\text\{([^}]+)\})"), "<mtext>\\1</mtext>");

        // 希腊字母与常用数学算子表
        static const QMap<QString, QString> symbolMap = {
            {"\\alpha", "&#x03B1;"}, {"\\beta", "&#x03B2;"}, {"\\gamma", "&#x03B3;"},
            {"\\delta", "&#x03B4;"}, {"\\epsilon", "&#x03B5;"}, {"\\zeta", "&#x03B6;"},
            {"\\eta", "&#x03B7;"}, {"\\theta", "&#x03B8;"}, {"\\iota", "&#x03B9;"},
            {"\\kappa", "&#x03BA;"}, {"\\lambda", "&#x03BB;"}, {"\\mu", "&#x03BC;"},
            {"\\nu", "&#x03BD;"}, {"\\xi", "&#x03BE;"}, {"\\pi", "&#x03C0;"},
            {"\\rho", "&#x03C1;"}, {"\\sigma", "&#x03C3;"}, {"\\tau", "&#x03C4;"},
            {"\\upsilon", "&#x03C5;"}, {"\\phi", "&#x03C6;"}, {"\\chi", "&#x03C7;"},
            {"\\psi", "&#x03C8;"}, {"\\omega", "&#x03C9;"},
            {"\\Gamma", "&#x0393;"}, {"\\Delta", "&#x0394;"}, {"\\Theta", "&#x0398;"},
            {"\\Lambda", "&#x039B;"}, {"\\Xi", "&#x039E;"}, {"\\Pi", "&#x03A0;"},
            {"\\Sigma", "&#x03A3;"}, {"\\Upsilon", "&#x03A5;"}, {"\\Phi", "&#x03A6;"},
            {"\\Psi", "&#x03A8;"}, {"\\Omega", "&#x03A9;"},
            {"\\infty", "&#x221E;"}, {"\\pm", "&#x00B1;"}, {"\\times", "&#x00D7;"},
            {"\\div", "&#x00F7;"}, {"\\cdot", "&#x22C5;"}, {"\\neq", "&#x2260;"},
            {"\\leq", "&#x2264;"}, {"\\geq", "&#x2265;"}, {"\\approx", "&#x2248;"},
            {"\\forall", "&#x2200;"}, {"\\exists", "&#x2203;"}, {"\\in", "&#x2208;"},
            {"\\notin", "&#x2209;"}, {"\\subset", "&#x2282;"}, {"\\subseteq", "&#x2286;"},
            {"\\cup", "&#x222A;"}, {"\\cap", "&#x2229;"}, {"\\to", "&#x2192;"},
            {"\\leftarrow", "&#x2190;"}, {"\\rightarrow", "&#x2192;"},
            {"\\Leftarrow", "&#x21D0;"}, {"\\Rightarrow", "&#x21D2;"},
            {"\\sum", "&#x2211;"}, {"\\prod", "&#x220F;"}, {"\\int", "&#x222B;"},
            {"\\oint", "&#x222E;"}, {"\\partial", "&#x2202;"}, {"\\nabla", "&#x2207;"}
        };

        for (auto it = symbolMap.begin(); it != symbolMap.end(); ++it) {
            s.replace(it.key(), QString("<mo>%1</mo>").arg(it.value()));
        }

        // 分数转换: \frac{a}{b} -> <mfrac><mrow>a</mrow><mrow>b</mrow></mfrac>
        static const QRegularExpression fracRegex(R"(\\frac\{([^{}]+)\}\{([^{}]+)\})");
        while (s.contains(fracRegex)) {
            s.replace(fracRegex, "<mfrac><mrow>\\1</mrow><mrow>\\2</mrow></mfrac>");
        }

        // 开方根号转换: \sqrt[n]{x} -> <mroot><mrow>x</mrow><mrow>n</mrow></mroot>
        static const QRegularExpression rootNRegex(R"(\\sqrt\[([^{}\]]+)\]\{([^{}]+)\})");
        s.replace(rootNRegex, "<mroot><mrow>\\2</mrow><mrow>\\1</mrow></mroot>");

        // 默认根号: \sqrt{x} -> <msqrt><mrow>x</mrow></msqrt>
        static const QRegularExpression sqrtRegex(R"(\\sqrt\{([^{}]+)\})");
        while (s.contains(sqrtRegex)) {
            s.replace(sqrtRegex, "<msqrt><mrow>\\1</mrow></msqrt>");
        }

        // 上下标同时存在: X_{sub}^{sup} 或 X^{sup}_{sub}
        static const QRegularExpression subSupRegex(R"(([a-zA-Z0-9&#;]+|\<m[a-z]+\>[^<]+\<\/m[a-z]+\>)\_\{([^{}]+)\}\^\{([^{}]+)\})");
        s.replace(subSupRegex, "<msubsup><mrow>\\1</mrow><mrow>\\2</mrow><mrow>\\3</mrow></msubsup>");
        static const QRegularExpression supSubRegex(R"(([a-zA-Z0-9&#;]+|\<m[a-z]+\>[^<]+\<\/m[a-z]+\>)\^\{([^{}]+)\}\_\{([^{}]+)\})");
        s.replace(supSubRegex, "<msubsup><mrow>\\1</mrow><mrow>\\3</mrow><mrow>\\2</mrow></msubsup>");

        // 单独下标: X_{sub}
        static const QRegularExpression subRegex(R"(([a-zA-Z0-9&#;]+|\<m[a-z]+\>[^<]+\<\/m[a-z]+\>)\_\{([^{}]+)\})");
        s.replace(subRegex, "<msub><mrow>\\1</mrow><mrow>\\2</mrow></msub>");

        // 单独上标: X^{sup}
        static const QRegularExpression supRegex(R"(([a-zA-Z0-9&#;]+|\<m[a-z]+\>[^<]+\<\/m[a-z]+\>)\^\{([^{}]+)\})");
        s.replace(supRegex, "<msup><mrow>\\1</mrow><mrow>\\2</mrow></msup>");

        // 基础数字与标识符包裹
        static const QRegularExpression numRegex(R"(\b(\d+)\b)");
        s.replace(numRegex, "<mn>\\1</mn>");

        static const QRegularExpression identRegex(R"(\b([a-zA-Z])\b)");
        s.replace(identRegex, "<mi>\\1</mi>");

        // 基础运算符号包裹
        static const QRegularExpression opRegex(R"(([\+\-\=\<\>\(\)\[\]]))");
        s.replace(opRegex, "<mo>\\1</mo>");

        return s;
    }
};
