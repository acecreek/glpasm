#include "pch.h"
#include "PartAssembly.h"
#include "glsuf/SufModel.h"
#include <iostream>
#include "glsuf/AtrParser.hpp"

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
    constexpr auto materialFile = L"tests/fixtures/materials.atr";
    assembly.SetResourcePaths(CString{}, materialFile);
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
    constexpr auto paletteFile = L"tests/fixtures/palette.l3p";
    constexpr size_t paletteParts = 3, greenPart = 0, blackPart = 1, defaultPart = 2;
    constexpr auto expectedGreen = 0.69f, expectedBlue = 0.85f, expectedBlack = 0.0f;
    constexpr auto specularStrength = 0.11f, specularGloss = 0.22f, specularTint = 0.33f;
    constexpr auto expectedTransparency = 0.25f, legacyRed = 1.0f, legacyGreen = 0.2f;
    if (!assembly.LoadLxP(l2pPath, error) || !assembly.Parts().front().material) return failure;
    const auto legacy = assembly.Parts().front().material.value();
    if (std::abs(legacy.col[x] - legacyRed) > tolerance || std::abs(legacy.col[y] - legacyGreen) > tolerance) return failure;
    if (!assembly.LoadLxP(paletteFile, error) || assembly.Parts().size() != paletteParts) return failure;
    const auto& firstPart = assembly.Parts()[greenPart];
    const auto& secondPart = assembly.Parts()[blackPart];
    if (!firstPart.material || !secondPart.material || assembly.Parts()[defaultPart].material) return failure;
    if (std::abs(firstPart.material->col[y] - expectedGreen) > tolerance ||
        std::abs(firstPart.material->col[z] - expectedBlue) > tolerance ||
        std::abs(firstPart.material->spc[x] - specularStrength) > tolerance ||
        std::abs(firstPart.material->spc[y] - specularGloss) > tolerance ||
        std::abs(firstPart.material->spc[z] - specularTint) > tolerance ||
        std::abs(secondPart.material->col[x] - expectedBlack) > tolerance ||
        std::abs(secondPart.material->tra - expectedTransparency) > tolerance) return failure;
    SufScene materials;
    if (!materials.LoadAtr(materialFile, error)) return failure;
    constexpr auto baseMaterial = L"BodyM", quotedMaterial = L"White material";
    const auto base = materials.Material(baseMaterial), quoted = materials.Material(quotedMaterial);
    if (!base || !quoted || std::abs(base->spc[z] - specularTint) > tolerance || quoted->col[x] != legacyRed) return failure;
    // 不正な数値・括弧・成分数を黙って受け入れない。
    constexpr auto invalidNumber = "atr bad { col ( 1garbage ) }";
    constexpr auto invalidBraces = "atr bad { col ( 1 )";
    constexpr auto invalidComponents = "atr bad { spc ( 0.1 0.2 ) }";
    for (const auto invalid : { invalidNumber, invalidBraces, invalidComponents })
    {
        bool rejected = false;
        try { atr::Parse(invalid); }
        catch (const std::exception&) { rejected = true; }
        if (!rejected) return failure;
    }
    std::wcout << passed;
    return success;
}
