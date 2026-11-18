#include "pch.h"
#include "PartAssembly.h"
#include "glsuf/SufModel.h"
#include <filesystem>
#include <numbers>
#include "LxpMaterials.hpp"

namespace
{
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

    CString SufPath(const std::string& text)
    {
        constexpr auto commentStart = "/*";
        constexpr auto commentEnd = "*/";
        constexpr auto quote = '"';
        const auto start = text.find(commentStart);
        if (start == std::string::npos) return QuotedText(text);
        const auto bodyStart = start + std::char_traits<char>::length(commentStart);
        const auto end = text.find(commentEnd, bodyStart);
        const auto body = Trim(text.substr(bodyStart, end - bodyStart));
        if (body.empty()) return {};
        if (body.front() == quote) return QuotedText(body);
        // L2Pの旧形式ではコメント内のSUFパスが引用符で囲まれていない。
        std::istringstream input(body);
        std::string path;
        input >> path;
        return ToCString(path);
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

    std::ifstream file(std::filesystem::path(path.GetString()));
    if (!file)
    {
        error.Format(_T("Could not open: %s"), path.GetString());
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const auto text = buffer.str();
    std::vector<SufAttribute> palette;
    try { palette = lxp::Palette(text); }
    catch (const std::exception& ex) { error = atr::Name(ex.what()); return false; }
    std::istringstream input(text);
    PartInstance current;
    bool haveTransform = false;
    std::string line;

    while (std::getline(input, line))
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
                    current.rotateDeg = { nums[11], nums[8], nums[5] };
                }
                else if (nums.size() >= 6)
                {
                    // ファイルの rotz、roty、rotx を X・Y・Z 順に格納する。
                    current.rotateDeg = { nums[5], nums[4], nums[3] };
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
            current.sufPath = SufPath(t);
            try
            {
                constexpr auto attributeMarker = "atr";
                constexpr auto next = 1;
                const auto settings = lxp::Settings(t);
                const auto marker = std::ranges::find(settings, attributeMarker);
                if (marker != settings.end())
                {
                    if (std::distance(marker, settings.end()) <= next)
                        throw std::runtime_error("パーツの材質名がありません。");
                    current.materialName = atr::Name(*std::next(marker));
                    const auto material = std::ranges::find_if(palette, [&](const auto& value)
                    {
                        return value.name == current.materialName;
                    });
                    if (material != palette.end()) current.material = *material;
                }
                else current.material = lxp::LegacyMaterial(settings, m_atrPath);
            }
            catch (const std::exception& ex) { error = atr::Name(ex.what()); return false; }
            if (current.name.IsEmpty())
            {
                current.name.Format(_T("Part %zu"), m_parts.size() + 1);
            }
            const auto baseDirectory = std::filesystem::path(path.GetString()).parent_path();
            if (!LoadPart(current, CString(baseDirectory.c_str()), error)) return false;
            if (!current.materialName.IsEmpty() && !current.material)
            {
                const auto material = current.scene->Material(current.materialName);
                if (!material)
                {
                    constexpr auto missingMaterial = L"指定された材質が見つかりません: %s";
                    error.Format(missingMaterial, current.materialName.GetString());
                    return false;
                }
                current.material = *material;
            }
            m_parts.push_back(current);
            haveTransform = false;
        }
    }

    if (m_parts.empty())
    {
        constexpr auto emptyError = L"描画できるパーツがファイルにありません。";
        error = emptyError;
        return false;
    }
    m_sourcePath = path;
    return true;
}

void PartAssembly::SetResourcePaths(const CString& partsDirectory, const CString& atrPath)
{
    m_partsDirectory = partsDirectory;
    m_atrPath = atrPath;
}

bool PartAssembly::LoadPart(PartInstance& part, const CString& baseDirectory, CString& error) const
{
    namespace fs = std::filesystem;
    constexpr auto failure = L"SUFを読み込めませんでした。\n%s\n%s";
    constexpr auto emptyGeometry = L"描画できる頂点がありません。";
    constexpr auto missingPath = L"パーツのSUFパスが指定されていません。";
    if (part.sufPath.IsEmpty())
    {
        error = missingPath;
        return false;
    }
    const fs::path requested(part.sufPath.GetString());
    auto resolved = requested;
    std::error_code ec;
    if (requested.is_relative())
    {
        resolved = fs::path(baseDirectory.GetString()) / requested;
        if (!fs::is_regular_file(resolved, ec) && !m_partsDirectory.IsEmpty())
            resolved = fs::path(m_partsDirectory.GetString()) / requested;
    }
    auto scene = std::make_shared<SufScene>();
    CString detail;
    if (!scene->LoadSuf(CString(resolved.c_str()), detail))
    {
        error.Format(failure, resolved.c_str(), detail.GetString());
        return false;
    }
    constexpr size_t minimumVertices = 3;
    const auto hasPolygon = std::ranges::any_of(scene->Objects(), [](const auto& object)
    {
        return std::ranges::any_of(object.primitives, [](const auto& primitive)
        {
            return primitive.points.size() >= minimumVertices;
        });
    });
    if (!hasPolygon || !scene->Bounds().valid)
    {
        error.Format(failure, resolved.c_str(), emptyGeometry);
        return false;
    }
    if (!m_atrPath.IsEmpty() && !scene->LoadAtr(m_atrPath, detail))
    {
        constexpr auto atrFailure = L"材質ファイルを読み込めませんでした。\n%s\n%s";
        error.Format(atrFailure, m_atrPath.GetString(), detail.GetString());
        return false;
    }
    part.sufPath = resolved.c_str();
    part.scene = std::move(scene);
    return true;
}

SufBounds PartAssembly::Bounds() const
{
    constexpr auto halfTurn = 180.0f;
    constexpr auto radians = std::numbers::pi_v<float> / halfTurn;
    constexpr size_t x = 0, y = 1, z = 2;
    SufBounds result;
    for (const auto& part : m_parts)
    {
        if (!part.scene) continue;
        const auto bounds = part.scene->Bounds();
        if (!bounds.valid) continue;
        const auto cx = std::cos(part.rotateDeg.x * radians), sx = std::sin(part.rotateDeg.x * radians);
        const auto cy = std::cos(part.rotateDeg.y * radians), sy = std::sin(part.rotateDeg.y * radians);
        const auto cz = std::cos(part.rotateDeg.z * radians), sz = std::sin(part.rotateDeg.z * radians);
        // 描画と同じ順序（拡大縮小 → X回転 → Y回転 → Z回転 → 移動）で範囲の8頂点を変換する。
        for (const auto px : { bounds.min[x], bounds.max[x] })
        for (const auto py : { bounds.min[y], bounds.max[y] })
        for (const auto pz : { bounds.min[z], bounds.max[z] })
        {
            const auto ax = px * part.scale.x;
            const auto ay = py * part.scale.y;
            const auto az = pz * part.scale.z;
            const auto by = ay * cx - az * sx, bz = ay * sx + az * cx;
            const auto dx = ax * cy + bz * sy, dz = -ax * sy + bz * cy;
            const std::array point{ dx * cz - by * sz + part.move.x,
                dx * sz + by * cz + part.move.y, dz + part.move.z };
            if (!result.valid) { result.min = point; result.max = point; result.valid = true; }
            else
            {
                for (size_t axis = x; axis < point.size(); ++axis)
                {
                    result.min[axis] = (std::min)(result.min[axis], point[axis]);
                    result.max[axis] = (std::max)(result.max[axis], point[axis]);
                }
            }
        }
    }
    return result;
}
