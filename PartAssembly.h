#pragma once
#include "glsuf/SufModel.h"
#include <optional>

class SufScene;
struct SufBounds;

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct PartInstance
{
    CString name;
    CString sufPath;
    CString materialName;
    std::optional<SufAttribute> material;
    std::shared_ptr<SufScene> scene;
    Vec3 move;
    Vec3 rotateDeg;
    Vec3 scale{ 1.0f, 1.0f, 1.0f };
};

class PartAssembly
{
public:
    bool LoadLxP(const CString& path, CString& error);
    void Clear();
    bool LoadPart(PartInstance& part, const CString& baseDirectory, CString& error) const;
    void SetResourcePaths(const CString& partsDirectory, const CString& atrPath);
    SufBounds Bounds() const;

    // Read-only access
    const std::vector<PartInstance>& Parts() const { return m_parts; }
    // Mutable access for editors
    std::vector<PartInstance>& Parts() { return m_parts; }

    const CString& SourcePath() const { return m_sourcePath; }

private:
    std::vector<PartInstance> m_parts;
    CString m_sourcePath;
    CString m_partsDirectory;
    CString m_atrPath;
};
