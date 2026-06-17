#include "pch.h"
#include "PartAssembly.h"

namespace
{
    std::string NarrowPath(const CString& value)
    {
        CT2A text(value, CP_ACP);
        return std::string(text);
    }

    CString ToCString(const std::string& value)
    {
        return CString(CA2T(value.c_str(), CP_ACP));
    }

    std::string Trim(std::string value)
    {
        const char* ws = " \t\r\n";
        const auto first = value.find_first_not_of(ws);
        if (first == std::string::npos) return {};
        const auto last = value.find_last_not_of(ws);
        return value.substr(first, last - first + 1);
    }

    std::vector<float> NumbersInParens(const std::string& text)
    {
        std::vector<float> out;
        size_t pos = 0;
        while ((pos = text.find('(', pos)) != std::string::npos)
        {
            const size_t end = text.find(')', pos + 1);
            if (end == std::string::npos) break;

            std::string body = text.substr(pos + 1, end - pos - 1);
            for (char& ch : body)
            {
                if (ch == ',') ch = ' ';
            }

            std::istringstream in(body);
            float n = 0.0f;
            while (in >> n) out.push_back(n);
            pos = end + 1;
        }
        return out;
    }

    CString QuotedText(const std::string& text)
    {
        const size_t first = text.find('"');
        if (first == std::string::npos) return {};
        const size_t second = text.find('"', first + 1);
        if (second == std::string::npos) return {};
        return ToCString(text.substr(first + 1, second - first - 1));
    }

    CString ObjName(const std::string& text)
    {
        const size_t obj = text.find("obj");
        if (obj == std::string::npos) return {};
        std::string tail = Trim(text.substr(obj + 3));
        std::istringstream in(tail);
        std::string name;
        in >> name;
        return ToCString(name);
    }
}

void PartAssembly::Clear()
{
    m_parts.clear();
    m_sourcePath.Empty();
}

bool PartAssembly::LoadLxP(const CString& path, CString& error)
{
    Clear();

    std::ifstream file(NarrowPath(path));
    if (!file)
    {
        error.Format(_T("Could not open: %s"), path.GetString());
        return false;
    }

    PartInstance current;
    bool haveTransform = false;
    std::string line;

    while (std::getline(file, line))
    {
        const std::string t = Trim(line);
        if (t.empty()) continue;

        if (t.find("mov") != std::string::npos)
        {
            const auto nums = NumbersInParens(t);
            if (nums.size() >= 3)
            {
                current = PartInstance{};
                current.move = { nums[0], nums[1], nums[2] };
                if (nums.size() >= 12)
                {
                    current.rotateDeg = { nums[5], nums[8], nums[11] };
                }
                else if (nums.size() >= 6)
                {
                    current.rotateDeg = { nums[3], nums[4], nums[5] };
                }
                haveTransform = true;
            }
        }
        else if (haveTransform && t.find("scal") != std::string::npos)
        {
            const auto nums = NumbersInParens(t);
            if (nums.size() >= 3)
            {
                current.scale = { nums[0], nums[1], nums[2] };
            }

            current.name = ObjName(t);
            current.sufPath = QuotedText(t);
            if (current.name.IsEmpty())
            {
                current.name.Format(_T("Part %zu"), m_parts.size() + 1);
            }
            m_parts.push_back(current);
            haveTransform = false;
        }
    }

    m_sourcePath = path;
    return true;
}
