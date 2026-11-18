#pragma once
#include "SufModel.h"
#include <charconv>
#include <cctype>
#include <optional>
#include <string_view>

namespace atr
{
    inline std::string Lower(std::string value)
    {
        for (auto& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return value;
    }

    // 引用符内の空白・括弧・パスはそのまま保持し、コメントだけを除去する。
    inline std::vector<std::string> Tokens(std::string_view text)
    {
        constexpr auto utf8Bom = "\xEF\xBB\xBF";
        if (text.starts_with(utf8Bom)) text.remove_prefix(std::char_traits<char>::length(utf8Bom));
        constexpr auto quote = '"', slash = '/', star = '*', space = ' ';
        constexpr auto openParen = '(', closeParen = ')', openBrace = '{', closeBrace = '}';
        constexpr size_t first = 0, next = 1;
        std::vector<std::string> result;
        for (size_t pos = first; pos < text.size();)
        {
            const auto ch = text[pos];
            if (static_cast<unsigned char>(ch) <= space) { ++pos; continue; }
            if (ch == slash && pos + next < text.size() && text[pos + next] == star)
            {
                constexpr auto endComment = "*/";
                const auto end = text.find(endComment, pos + next);
                if (end == std::string_view::npos) throw std::runtime_error("材質のコメントが閉じられていません。");
                pos = end + std::char_traits<char>::length(endComment);
                continue;
            }
            if (ch == quote)
            {
                const auto end = text.find(quote, ++pos);
                if (end == std::string_view::npos) throw std::runtime_error("材質名の引用符が閉じられていません。");
                result.emplace_back(text.substr(pos, end - pos));
                pos = end + next;
                continue;
            }
            if (ch == openParen || ch == closeParen || ch == openBrace || ch == closeBrace)
            {
                result.emplace_back(next, ch);
                ++pos;
                continue;
            }
            const auto begin = pos;
            while (pos < text.size())
            {
                const auto value = text[pos];
                if (static_cast<unsigned char>(value) <= space || value == quote || value == openParen ||
                    value == closeParen || value == openBrace || value == closeBrace ||
                    (value == slash && pos + next < text.size() && text[pos + next] == star)) break;
                ++pos;
            }
            result.emplace_back(text.substr(begin, pos - begin));
        }
        return result;
    }

    inline CString Name(const std::string& value)
    {
        return CString(CA2T(value.c_str(), CP_ACP));
    }

    inline float Number(const std::string& token)
    {
        constexpr auto zero = 0.0f;
        constexpr auto plus = '+';
        auto view = std::string_view(token);
        if (!view.empty() && view.front() == plus) view.remove_prefix(std::string_view::size_type{1});
        float value = zero;
        const auto parsed = std::from_chars(view.data(), view.data() + view.size(), value);
        if (view.empty() || parsed.ec != std::errc{} || parsed.ptr != view.data() + view.size() || !std::isfinite(value))
            throw std::runtime_error("材質の数値が不正です: " + token);
        return value;
    }

    inline std::vector<SufAttribute> Parse(std::string_view text)
    {
        constexpr auto attribute = "atr", open = "{", close = "}", begin = "(", end = ")";
        constexpr auto col = "col", rgb = "rgb", amb = "amb", dif = "dif", spc = "spc", tra = "tra";
        constexpr size_t first = 0, one = 1, components = 3;
        const auto tokens = Tokens(text);
        std::vector<SufAttribute> result;
        size_t pos = first;
        while (pos < tokens.size())
        {
            if (Lower(tokens[pos++]) != attribute) continue;
            if (pos + one >= tokens.size()) throw std::runtime_error("材質名または開始括弧がありません。");
            SufAttribute material;
            material.name = Name(tokens[pos++]);
            if (tokens[pos++] != open) throw std::runtime_error("材質の開始括弧がありません。");
            bool closed = false;
            while (pos < tokens.size())
            {
                if (tokens[pos] == close) { ++pos; closed = true; break; }
                const auto key = Lower(tokens[pos++]);
                if (pos >= tokens.size() || tokens[pos++] != begin)
                    throw std::runtime_error("材質設定の開始括弧がありません: " + key);
                size_t depth = one;
                std::vector<std::string> values;
                while (pos < tokens.size() && depth != first)
                {
                    const auto token = tokens[pos++];
                    if (token == begin) ++depth;
                    else if (token == end) --depth;
                    else if (token == close || token == open) throw std::runtime_error("材質設定の括弧が不正です。");
                    else if (key == col || key == amb || key == dif || key == spc || key == tra) values.push_back(token);
                }
                if (depth != first) throw std::runtime_error("材質設定の括弧が閉じられていません。");
                // 未対応の模様・反射・セル調設定も括弧単位で読み飛ばし、色設定と混同しない。
                if (key == col)
                {
                    const bool isRgb = !values.empty() && Lower(values.front()) == rgb;
                    if (isRgb) values.erase(values.begin());
                    if (!isRgb && values.size() == one) material.col.fill(Number(values.front()));
                    else if (values.size() == components)
                    {
                        for (size_t axis = first; axis < components; ++axis) material.col[axis] = Number(values[axis]);
                    }
                    else throw std::runtime_error("色は単一値またはRGBの3成分で指定してください。");
                }
                else if (key == spc)
                {
                    if (values.size() != components) throw std::runtime_error("光沢は3成分で指定してください。");
                    // 範囲を確認してから各成分を一度ずつ代入する。
                    for (size_t axis = first; axis < components; ++axis) material.spc[axis] = Number(values[axis]);
                }
                else if (key == amb || key == dif || key == tra)
                {
                    if (values.size() != one) throw std::runtime_error("材質係数は単一値で指定してください。");
                    const auto value = Number(values.front());
                    if (key == amb) material.amb = value;
                    else if (key == dif) material.dif = value;
                    else material.tra = value;
                }
            }
            if (!closed) throw std::runtime_error("材質の終了括弧がありません。");
            result.push_back(std::move(material));
        }
        return result;
    }
}
