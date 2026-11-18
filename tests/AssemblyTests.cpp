#include "pch.h"
#include "PartAssembly.h"
#include "glsuf/SufModel.h"
#include <iostream>

int wmain()
{
    constexpr auto success = 0, failure = 1;
    constexpr auto tolerance = 0.001f;
    constexpr size_t x = 0, y = 1, z = 2, expectedParts = 1;
    constexpr auto expectedMinX = -290.0f, expectedMaxX = 10.0f;
    constexpr auto expectedMinY = 20.0f, expectedMaxY = 220.0f, expectedZ = 30.0f;
    constexpr auto expectedRotationZ = 90.0f;
    constexpr auto fixtureDirectory = L"tests/fixtures";
    constexpr auto l3pPath = L"tests/fixtures/assembly.l3p";
    constexpr auto l2pPath = L"tests/fixtures/assembly.l2p";
    constexpr auto missingPath = L"tests/fixtures/missing.l3p";
    constexpr auto sufPath = L"triangle.suf";
    constexpr auto emptyPath = L"empty.suf";
    constexpr auto passed = L"SUF追加、L2P/L3P、座標変換、相対パス、読込失敗の確認に成功しました。\n";
    if (!AfxWinInit(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), SW_HIDE)) return failure;
    CString error;
    PartAssembly assembly;
    for (const auto path : { l3pPath, l2pPath })
    {
        if (!assembly.LoadLxP(path, error) || assembly.Parts().size() != expectedParts) return failure;
        const auto& part = assembly.Parts().front();
        if (!part.scene || part.rotateDeg.z != expectedRotationZ) return failure;
        const auto bounds = assembly.Bounds();
        if (!bounds.valid || std::abs(bounds.min[x] - expectedMinX) > tolerance ||
            std::abs(bounds.max[x] - expectedMaxX) > tolerance ||
            std::abs(bounds.min[y] - expectedMinY) > tolerance ||
            std::abs(bounds.max[y] - expectedMaxY) > tolerance ||
            std::abs(bounds.min[z] - expectedZ) > tolerance) return failure;
    }
    PartInstance part;
    part.sufPath = sufPath;
    if (!assembly.LoadPart(part, fixtureDirectory, error) || !part.scene) return failure;
    part.sufPath = emptyPath;
    if (assembly.LoadPart(part, fixtureDirectory, error)) return failure;
    if (assembly.LoadLxP(missingPath, error) || error.IsEmpty()) return failure;
    std::wcout << passed;
    return success;
}
