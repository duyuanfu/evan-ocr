#pragma once

#include <QString>
#include <QRegularExpression>
#include <QStringList>

struct FundSniffResult
{
    bool matched = false;
    QString code;
    QString name;
};

class FundSniffer
{
public:
    static FundSniffResult sniff(const QString& text)
    {
        FundSniffResult result;
        if (text.trimmed().isEmpty()) {
            return result;
        }

        // 1. 金融/理财语境关键词集合
        const QStringList contextKeywords = {
            "基金", "净值", "估值", "持仓", "收益", "涨跌", "定投",
            "份额", "混合", "ETF", "联接", "精选", "重仓", "股票型",
            "指数", "分红", "理财", "资产", "天天基金", "蚂蚁财富", "招商银行"
        };

        // 2. 知名公募基金简称集合 (用于没有代码时或代码相近时提取名称)
        const QStringList famousFunds = {
            "易方达", "华夏", "广发", "招商中证白酒", "招商白酒",
            "中欧医疗", "诺安成长", "景顺长城", "富国", "天弘",
            "汇添富", "南方", "嘉实", "博时", "工银瑞信"
        };

        // 检查文本是否含有金融语境
        bool hasContext = false;
        for (const auto& kw : contextKeywords) {
            if (text.contains(kw, Qt::CaseInsensitive)) {
                hasContext = true;
                break;
            }
        }

        // 提取可能的基金名称
        for (const auto& name : famousFunds) {
            if (text.contains(name, Qt::CaseInsensitive)) {
                result.name = name;
                hasContext = true;
                break;
            }
        }

        // 3. 扫描 6 位数字 (\b\d{6}\b)
        static const QRegularExpression codeRegex(R"(\b(\d{6})\b)");
        auto matchIt = codeRegex.globalMatch(text);
        while (matchIt.hasNext()) {
            auto match = matchIt.next();
            QString candidateCode = match.captured(1);

            // 校验常见的公募基金代码号段:
            // 00xxxx, 01xxxx, 02xxxx (开放式偏股/混合/债券)
            // 15xxxx, 16xxxx (深市 ETF / LOF)
            // 50xxxx, 51xxxx, 56xxxx, 58xxxx (沪市封闭/ETF)
            // 96xxxx (互认基金)
            QString prefix2 = candidateCode.left(2);
            bool isTypicalFundPrefix = (
                prefix2 == "00" || prefix2 == "01" || prefix2 == "02" ||
                prefix2 == "15" || prefix2 == "16" ||
                prefix2 == "50" || prefix2 == "51" || prefix2 == "56" || prefix2 == "58" ||
                prefix2 == "96"
            );

            // 若命中典型号段且文本有金融语境，或出现强金融关键词
            if ((isTypicalFundPrefix && hasContext) || (hasContext && !candidateCode.isEmpty())) {
                result.matched = true;
                result.code = candidateCode;
                return result;
            }
        }

        // 若虽然没有精准 6 位数字，但文本中明确含有知名基金或高频基金词
        if (hasContext && !result.name.isEmpty()) {
            result.matched = true;
            return result;
        }

        return result;
    }
};
