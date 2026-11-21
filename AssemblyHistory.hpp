#pragma once
#include "PartAssembly.h"
#include <deque>
#include <optional>

// 形状データは共有し、配置・材質・選択位置だけを履歴に保持する。
class AssemblyHistory
{
public:
    static constexpr int noSelection = -1;
    static constexpr size_t limit = 100;
    struct State
    {
        std::vector<PartInstance> parts;
        int selection = noSelection;
    };

    void Record(const std::vector<PartInstance>& parts, int selection)
    {
        Push(m_undo, State{parts, selection});
        m_redo.clear();
    }
    std::optional<State> Undo(const std::vector<PartInstance>& parts, int selection)
    {
        return Exchange(m_undo, m_redo, parts, selection);
    }
    std::optional<State> Redo(const std::vector<PartInstance>& parts, int selection)
    {
        return Exchange(m_redo, m_undo, parts, selection);
    }
    bool CanUndo() const { return !m_undo.empty(); }
    bool CanRedo() const { return !m_redo.empty(); }
    void Clear() { m_undo.clear(); m_redo.clear(); }

private:
    static void Push(std::deque<State>& entries, State state)
    {
        if (entries.size() == limit) entries.pop_front();
        entries.push_back(std::move(state));
    }
    static std::optional<State> Exchange(std::deque<State>& source, std::deque<State>& destination,
        const std::vector<PartInstance>& parts, int selection)
    {
        if (source.empty()) return std::nullopt;
        Push(destination, State{parts, selection});
        auto result = std::move(source.back());
        source.pop_back();
        return result;
    }
    std::deque<State> m_undo;
    std::deque<State> m_redo;
};
