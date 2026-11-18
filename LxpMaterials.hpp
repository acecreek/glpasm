#pragma once
#include "glsuf/AtrParser.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace lxp
{
    inline std::string Trim(std::string value)
    {
        constexpr auto whitespace = " \t\r\n";
        constexpr size_t one = 1;
        const auto first = value.find_first_not_of(whitespace);
        if (first == std::string::npos) return {};
        return value.substr(first, value.find_last_not_of(whitespace) - first + one);
    }

    inline std::vector<std::string> Settings(const std::string& line)
    {
        constexpr auto startMarker = "/*", endMarker = "*/";
        const auto start = line.find(startMarker);
        if (start == std::string::npos) return {};
        const auto body = start + std::char_traits<char>::length(startMarker);
        const auto end = line.find(endMarker, body);
        return atr::Tokens(line.substr(body, end - body));
    }

    inline std::optional<SufAttribute> LegacyMaterial(const std::vector<std::string>& settings, const CString& atrPath)
    {
        constexpr size_t colorIndex = 1, materialIndex = 3, minimumSettings = 4;
        constexpr size_t first = 0, one = 1, firstNumber = 2, colorValues = 5, materialValues = 8;
        constexpr size_t components = 3, tra = 2, amb = 3, dif = 4, spc = 5;
        constexpr auto iniName = L"atr.ini";
        constexpr auto comma = ',', section = '[', equals = '=';
        constexpr auto baseSection = "[Attribute]", colorPrefix = "Color", materialPrefix = "Material";
        if (settings.size() < minimumSettings) return std::nullopt;
        const auto path = std::filesystem::path(atrPath.GetString()).parent_path() / iniName;
        std::ifstream input(path);
        if (!input) throw std::runtime_error("旧L2Pの色・材質を取得するatr.iniが見つかりません。");
        SufAttribute result;
        result.name = atr::Name(settings[colorIndex]);
        bool foundColor = false, foundMaterial = false, inSection = false;
        std::string line;
        while (std::getline(input, line))
        {
            line = Trim(line);
            if (line == baseSection) { inSection = true; continue; }
            if (!line.empty() && line.front() == section) { inSection = false; continue; }
            if (!inSection) continue;
            const auto separator = line.find(equals);
            if (separator == std::string::npos) continue;
            const auto key = Trim(line.substr(first, separator));
            std::istringstream body(line.substr(separator + one));
            std::vector<std::string> fields;
            std::string field;
            while (std::getline(body, field, comma)) fields.push_back(Trim(field));
            if (fields.empty()) continue;
            if (key.starts_with(colorPrefix) && fields.front() == settings[colorIndex] && fields.size() >= colorValues)
            {
                for (size_t axis = first; axis < components; ++axis) result.col[axis] = atr::Number(fields[firstNumber + axis]);
                foundColor = true;
            }
            if (key.starts_with(materialPrefix) && fields.front() == settings[materialIndex] && fields.size() >= materialValues)
            {
                result.tra = atr::Number(fields[tra]);
                result.amb = atr::Number(fields[amb]);
                result.dif = atr::Number(fields[dif]);
                for (size_t axis = first; axis < components; ++axis) result.spc[axis] = atr::Number(fields[spc + axis]);
                foundMaterial = true;
            }
        }
        if (!foundColor || !foundMaterial) throw std::runtime_error("旧L2Pの色コードまたは材質コードがatr.iniにありません。");
        return result;
    }

    inline std::vector<SufAttribute> Palette(const std::string& text)
    {
        constexpr auto marker = "Palette:", commentEnd = "*/";
        const auto start = text.find(marker);
        if (start == std::string::npos) return {};
        const auto end = text.find(commentEnd, start);
        return atr::Parse(text.substr(start, end - start));
    }
}
