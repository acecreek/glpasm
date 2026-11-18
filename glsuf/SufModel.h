#pragma once
#include <optional>

enum class SufPrimType
{
    Poly,
    UvPoly,
    Shade,
    UvShade,
    Other
};

struct SufPoint
{
    std::vector<float> values;
};

struct SufPrimitive
{
    CString atrName;
    SufPrimType type = SufPrimType::Other;
    std::vector<SufPoint> points;
};

struct SufObject
{
    CString name;
    std::vector<SufPrimitive> primitives;
};

struct SufAttribute
{
    CString name;
    std::array<float, 3> col = { 0.95f, 0.95f, 0.95f };
    float tra = 0.0f;
    float amb = 1.0f;
    float dif = 0.0f;
    std::array<float, 3> spc = { 0.0f, 0.0f, 0.0f };
};

struct SufBounds
{
    bool valid = false;
    std::array<float, 3> min = { 0.0f, 0.0f, 0.0f };
    std::array<float, 3> max = { 0.0f, 0.0f, 0.0f };
};

class SufScene
{
public:
    bool LoadSuf(const CString& path, CString& error);
    bool LoadAtr(const CString& path, CString& error);

    void ClearSuf();
    void ClearAtr();

    const std::vector<SufObject>& Objects() const { return m_objects; }
    const std::vector<SufAttribute>& Attributes() const { return m_attributes; }
    const SufAttribute* FindAttribute(const CString& name) const;
    SufBounds Bounds() const;
    std::optional<SufAttribute> Material(const CString& name) const;
    CString SourcePath() const { return m_sourcePath; }
    CString AtrPath() const { return m_atrPath; }

private:
    static std::string ReadTextFile(const CString& path);
    static std::vector<std::string> TokenizeLine(const std::string& line, bool& inBlockComment);
    static CString ToCString(const std::string& text);
    static float ParseFloat(const std::string& token);
    static SufPrimType ParsePrimType(const std::string& token);
    static size_t ValueCount(SufPrimType type);

    std::vector<SufObject> m_objects;
    std::vector<SufAttribute> m_attributes;
    CString m_sourcePath;
    CString m_atrPath;
};

