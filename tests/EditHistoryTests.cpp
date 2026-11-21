#include "pch.h"
#include "AssemblyHistory.hpp"
#include <iostream>

int wmain()
{
    constexpr auto success = 0, failure = 1, first = 0, copied = 1;
    constexpr auto initialCount = 1u, copiedCount = 2u, emptyCount = 0u;
    constexpr auto initialPosition = 10.0f, editedPosition = 200.0f;
    constexpr auto name = L"履歴テスト";
    constexpr auto materialName = L"朱色";
    constexpr auto passed = L"編集履歴の復元・分岐・上限・共有形状の確認に成功しました。\n";
    if (!AfxWinInit(GetModuleHandleW(nullptr), nullptr, GetCommandLineW(), SW_HIDE)) return failure;
    AssemblyHistory history;
    std::vector<PartInstance> parts;
    if (history.Undo(parts, first) || history.Redo(parts, first)) return failure;
    PartInstance part;
    part.name = name;
    part.materialName = materialName;
    part.material = SufAttribute{};
    part.scene = std::make_shared<SufScene>();
    part.move.x = initialPosition;
    // 追加を取り消し、再実行したときも材質と共有形状を復元する。
    history.Record(parts, AssemblyHistory::noSelection);
    parts.push_back(part);
    auto state = history.Undo(parts, first);
    if (!state || !state->parts.empty() || state->selection != AssemblyHistory::noSelection) return failure;
    parts = std::move(state->parts);
    state = history.Redo(parts, AssemblyHistory::noSelection);
    if (!state || state->parts.size() != initialCount || state->parts.front().scene != part.scene
        || !state->parts.front().material || state->parts.front().materialName != materialName) return failure;
    parts = std::move(state->parts);
    // 複製後の配置変更は元パーツへ波及しない。
    history.Record(parts, first);
    parts.push_back(parts.front());
    parts.back().move.x = editedPosition;
    if (parts.front().move.x != initialPosition) return failure;
    state = history.Undo(parts, copied);
    if (!state || state->parts.size() != initialCount || state->selection != first) return failure;
    parts = std::move(state->parts);
    state = history.Redo(parts, first);
    if (!state || state->parts.size() != copiedCount || state->selection != copied
        || state->parts.back().move.x != editedPosition) return failure;
    parts = std::move(state->parts);
    history.Record(parts, copied);
    parts.clear();
    state = history.Undo(parts, AssemblyHistory::noSelection);
    if (!state || state->parts.size() != copiedCount || state->selection != copied) return failure;
    parts = std::move(state->parts);
    // 元に戻した後の新しい操作で、やり直し履歴を破棄する。
    history.Record(parts, copied);
    if (history.CanRedo()) return failure;
    history.Clear();
    if (history.CanUndo() || history.CanRedo()) return failure;
    for (size_t index = emptyCount; index <= AssemblyHistory::limit; ++index)
    {
        history.Record(parts, first);
        parts.front().move.x = static_cast<float>(index);
    }
    size_t undoCount = emptyCount;
    while (auto previous = history.Undo(parts, first))
    {
        parts = std::move(previous->parts);
        ++undoCount;
    }
    if (undoCount != AssemblyHistory::limit) return failure;
    std::wcout << passed;
    return success;
}
