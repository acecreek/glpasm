#include "pch.h"
#include "SufModel.h"

namespace
{
    constexpr size_t PX = 0;
    constexpr size_t PY = 1;
    constexpr size_t PZ = 2;

    bool IsBrace(const std::string& token)
    {
        return token == "{" || token == "}" || token == "(" || token == ")";
    }

    int FindToken(const std::vector<std::string>& tokens, const char* token)
    {
        for (size_t i = 0; i < tokens.size(); ++i)
        {
            if (tokens[i] == token) return static_cast<int>(i);
        }
        return -1;
    }

    int FindNextValueToken(const std::vector<std::string>& tokens, size_t start)
    {
        for (size_t i = start; i < tokens.size(); ++i)
        {
            if (!IsBrace(tokens[i])) return static_cast<int>(i);
        }
        return -1;
    }
}

void SufScene::ClearSuf()
{
    m_objects.clear();
    m_sourcePath.Empty();
}

void SufScene::ClearAtr()
{
    m_attributes.clear();
    m_atrPath.Empty();
}

bool SufScene::LoadSuf(const CString& path, CString& error)
{
    try
    {
        const std::string text = ReadTextFile(path);
        std::istringstream input(text);
        std::string line;
        bool inBlockComment = false;
        SufObject* currentObject = nullptr;
        CString currentAtr;

        ClearSuf();

        while (std::getline(input, line))
        {
            auto tokens = TokenizeLine(line, inBlockComment);
            if (tokens.empty()) continue;

            const int objIndex = FindToken(tokens, "obj");
            if (objIndex >= 0)
            {
                SufObject object;
                int nameIndex = FindToken(tokens, "suf");
                if (nameIndex >= 0)
                {
                    const int value = FindNextValueToken(tokens, static_cast<size_t>(nameIndex) + 1);
                    if (value >= 0) object.name = ToCString(tokens[static_cast<size_t>(value)]);
                }
                if (object.name.IsEmpty())
                {
                    CString fallback;
                    fallback.Format(_T("Object %zu"), m_objects.size() + 1);
                    object.name = fallback;
                }
                m_objects.push_back(object);
                currentObject = &m_objects.back();
                currentAtr.Empty();
                continue;
            }

            const int atrIndex = FindToken(tokens, "atr");
            if (atrIndex >= 0)
            {
                const int value = FindNextValueToken(tokens, static_cast<size_t>(atrIndex) + 1);
                if (value >= 0) currentAtr = ToCString(tokens[static_cast<size_t>(value)]);
                continue;
            }

            const int primIndex = FindToken(tokens, "prim");
            if (primIndex >= 0)
            {
                if (!currentObject)
                {
                    m_objects.push_back({ _T("Object 1"), {} });
                    currentObject = &m_objects.back();
                }

                const int typeIndex = FindNextValueToken(tokens, static_cast<size_t>(primIndex) + 1);
                if (typeIndex < 0) throw std::runtime_error("prim line has no primitive type.");

                SufPrimitive prim;
                prim.atrName = currentAtr;
                prim.type = ParsePrimType(tokens[static_cast<size_t>(typeIndex)]);
                const size_t componentCount = ValueCount(prim.type);
                if (componentCount == 0) throw std::runtime_error("unsupported primitive type: " + tokens[static_cast<size_t>(typeIndex)]);

                bool readingPoints = false;
                bool primClosed = false;
                std::vector<float> pendingValues;

                auto consume = [&](const std::vector<std::string>& rowTokens)
                {
                    for (const auto& token : rowTokens)
                    {
                        if (token == "(")
                        {
                            readingPoints = true;
                            continue;
                        }
                        if (token == ")")
                        {
                            primClosed = true;
                            break;
                        }
                        if (!readingPoints || IsBrace(token)) continue;

                        pendingValues.push_back(ParseFloat(token));
                        if (pendingValues.size() == componentCount)
                        {
                            prim.points.push_back({ pendingValues });
                            pendingValues.clear();
                        }
                    }
                };

                consume(tokens);
                while (!primClosed && std::getline(input, line))
                {
                    consume(TokenizeLine(line, inBlockComment));
                }

                if (!pendingValues.empty())
                {
                    throw std::runtime_error("primitive point has too few numeric values.");
                }
                currentObject->primitives.push_back(std::move(prim));
            }
        }

        m_sourcePath = path;
        return true;
    }
    catch (const std::exception& ex)
    {
        error = ToCString(ex.what());
        return false;
    }
}

bool SufScene::LoadAtr(const CString& path, CString& error)
{
    try
    {
        const std::string text = ReadTextFile(path);
        std::istringstream input(text);
        std::string line;
        bool inBlockComment = false;
        SufAttribute* currentAtr = nullptr;

        ClearAtr();

        while (std::getline(input, line))
        {
            auto tokens = TokenizeLine(line, inBlockComment);
            if (tokens.empty()) continue;

            const int atrIndex = FindToken(tokens, "atr");
            if (atrIndex >= 0)
            {
                const int nameIndex = FindNextValueToken(tokens, static_cast<size_t>(atrIndex) + 1);
                if (nameIndex >= 0)
                {
                    SufAttribute atr;
                    atr.name = ToCString(tokens[static_cast<size_t>(nameIndex)]);
                    m_attributes.push_back(atr);
                    currentAtr = &m_attributes.back();
                }
                continue;
            }

            if (!currentAtr) continue;

            if (FindToken(tokens, "colormap") >= 0) continue;
            const int colIndex = FindToken(tokens, "col");
            if (colIndex >= 0)
            {
                int out = 0;
                for (size_t i = static_cast<size_t>(colIndex) + 1; i < tokens.size() && out < 3; ++i)
                {
                    if (tokens[i] == "rgb" || IsBrace(tokens[i])) continue;
                    currentAtr->col[static_cast<size_t>(out++)] = ParseFloat(tokens[i]);
                }
                continue;
            }

            const int ambIndex = FindToken(tokens, "amb");
            if (ambIndex >= 0)
            {
                const int value = FindNextValueToken(tokens, static_cast<size_t>(ambIndex) + 1);
                if (value >= 0) currentAtr->amb = ParseFloat(tokens[static_cast<size_t>(value)]);
                continue;
            }

            const int difIndex = FindToken(tokens, "dif");
            if (difIndex >= 0)
            {
                const int value = FindNextValueToken(tokens, static_cast<size_t>(difIndex) + 1);
                if (value >= 0) currentAtr->dif = ParseFloat(tokens[static_cast<size_t>(value)]);
                continue;
            }

            const int spcIndex = FindToken(tokens, "spc");
            if (spcIndex >= 0)
            {
                int out = 0;
                for (size_t i = static_cast<size_t>(spcIndex) + 1; i < tokens.size() && out < 3; ++i)
                {
                    if (IsBrace(tokens[i])) continue;
                    currentAtr->spc[static_cast<size_t>(out++)] = ParseFloat(tokens[i]);
                }
                continue;
            }

            const int traIndex = FindToken(tokens, "tra");
            if (traIndex >= 0)
            {
                const int value = FindNextValueToken(tokens, static_cast<size_t>(traIndex) + 1);
                if (value >= 0) currentAtr->tra = ParseFloat(tokens[static_cast<size_t>(value)]);
                continue;
            }
        }

        m_atrPath = path;
        return true;
    }
    catch (const std::exception& ex)
    {
        error = ToCString(ex.what());
        return false;
    }
}

const SufAttribute* SufScene::FindAttribute(const CString& name) const
{
    for (const auto& atr : m_attributes)
    {
        if (atr.name.Compare(name) == 0) return &atr;
    }
    return nullptr;
}

SufBounds SufScene::Bounds() const
{
    SufBounds bounds;
    for (const auto& object : m_objects)
    {
        for (const auto& prim : object.primitives)
        {
            for (const auto& point : prim.points)
            {
                if (point.values.size() < 3) continue;
                if (!bounds.valid)
                {
                    bounds.valid = true;
                    bounds.min = { point.values[PX], point.values[PY], point.values[PZ] };
                    bounds.max = bounds.min;
                }
                else
                {
                    bounds.min[0] = (std::min)(bounds.min[0], point.values[PX]);
                    bounds.min[1] = (std::min)(bounds.min[1], point.values[PY]);
                    bounds.min[2] = (std::min)(bounds.min[2], point.values[PZ]);
                    bounds.max[0] = (std::max)(bounds.max[0], point.values[PX]);
                    bounds.max[1] = (std::max)(bounds.max[1], point.values[PY]);
                    bounds.max[2] = (std::max)(bounds.max[2], point.values[PZ]);
                }
            }
        }
    }
    return bounds;
}

std::string SufScene::ReadTextFile(const CString& path)
{
    CT2A ansiPath(path, CP_ACP);
    std::ifstream file(ansiPath.m_psz, std::ios::binary);
    if (!file) throw std::runtime_error("could not open file.");
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> SufScene::TokenizeLine(const std::string& line, bool& inBlockComment)
{
    std::vector<std::string> tokens;
    std::string token;

    auto flush = [&]()
    {
        if (!token.empty())
        {
            tokens.push_back(token);
            token.clear();
        }
    };

    for (size_t i = 0; i < line.size(); ++i)
    {
        if (inBlockComment)
        {
            if (line[i] == '*' && i + 1 < line.size() && line[i + 1] == '/')
            {
                inBlockComment = false;
                ++i;
            }
            continue;
        }

        if (line[i] == '/' && i + 1 < line.size() && line[i + 1] == '*')
        {
            flush();
            inBlockComment = true;
            ++i;
            continue;
        }

        const unsigned char ch = static_cast<unsigned char>(line[i]);
        if (ch <= ' ')
        {
            flush();
            continue;
        }
        if (line[i] == '(' || line[i] == ')' || line[i] == '{' || line[i] == '}')
        {
            flush();
            tokens.emplace_back(1, line[i]);
            continue;
        }
        token.push_back(line[i]);
    }
    flush();
    return tokens;
}

CString SufScene::ToCString(const std::string& text)
{
    return CString(CA2T(text.c_str(), CP_ACP));
}

float SufScene::ParseFloat(const std::string& token)
{
    char* end = nullptr;
    const float value = std::strtof(token.c_str(), &end);
    if (end == token.c_str()) throw std::runtime_error("invalid numeric value: " + token);
    return value;
}

SufPrimType SufScene::ParsePrimType(const std::string& token)
{
    if (token == "poly") return SufPrimType::Poly;
    if (token == "uvpoly") return SufPrimType::UvPoly;
    if (token == "shade") return SufPrimType::Shade;
    if (token == "uvshade") return SufPrimType::UvShade;
    return SufPrimType::Other;
}

size_t SufScene::ValueCount(SufPrimType type)
{
    switch (type)
    {
    case SufPrimType::Poly: return 3;
    case SufPrimType::UvPoly: return 5;
    case SufPrimType::Shade: return 6;
    case SufPrimType::UvShade: return 8;
    default: return 0;
    }
}

