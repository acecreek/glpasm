#pragma once
#include <array>
#include <filesystem>
#include <format>

// 既存のDoGA設定と同じ、実行ファイル横のiniに保存する。
namespace WindowSettings
{
    inline constexpr auto section = L"Window";
    inline constexpr auto positionKey = L"NormalRect";
    inline constexpr auto maximizedKey = L"Maximized";
    inline constexpr auto defaultWidth = 1600;
    inline constexpr auto defaultHeight = 900;
    inline constexpr auto minimumWidth = 640;
    inline constexpr auto minimumHeight = 480;
    inline constexpr auto zero = 0;
    inline constexpr auto half = 2;
    inline constexpr auto fieldCount = 4;
    inline constexpr auto rectFormat = L"%d,%d,%d,%d";
    inline constexpr auto saveFormat = L"{},{},{},{}";
    inline constexpr auto iniExtension = L".ini";
    inline constexpr auto empty = L"";
    inline constexpr auto enabled = L"1";
    inline constexpr auto disabled = L"0";
    inline constexpr size_t pathCapacity = 32768;
    inline constexpr size_t valueCapacity = 128;

    inline std::wstring IniPath()
    {
        std::array<wchar_t, pathCapacity> buffer{};
        const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == zero || length >= buffer.size()) return {};
        auto path = std::filesystem::path(buffer.data());
        path.replace_extension(iniExtension);
        return path.wstring();
    }

    inline int Restore(CWnd& window)
    {
        const auto path = IniPath();
        CRect rect;
        window.GetWindowRect(rect);
        std::array<wchar_t, valueCapacity> value{};
        GetPrivateProfileStringW(section, positionKey, empty, value.data(),
            static_cast<DWORD>(value.size()), path.c_str());
        RECT saved{};
        const bool valid = swscanf_s(value.data(), rectFormat,
            &saved.left, &saved.top, &saved.right, &saved.bottom) == fieldCount
            && static_cast<long long>(saved.right) - saved.left >= minimumWidth
            && static_cast<long long>(saved.bottom) - saved.top >= minimumHeight;
        if (valid) rect = saved;
        MONITORINFO monitor{};
        monitor.cbSize = sizeof(monitor);
        if (GetMonitorInfoW(MonitorFromRect(rect, MONITOR_DEFAULTTONEAREST), &monitor))
        {
            const CRect work(monitor.rcWork);
            const auto width = valid ? static_cast<long long>(rect.right) - rect.left : defaultWidth;
            const auto height = valid ? static_cast<long long>(rect.bottom) - rect.top : defaultHeight;
            const auto fittedWidth = static_cast<int>((std::min)(width, static_cast<long long>(work.Width())));
            const auto fittedHeight = static_cast<int>((std::min)(height, static_cast<long long>(work.Height())));
            const auto left = valid ? std::clamp(rect.left, work.left, work.right - fittedWidth)
                : work.left + (work.Width() - fittedWidth) / half;
            const auto top = valid ? std::clamp(rect.top, work.top, work.bottom - fittedHeight)
                : work.top + (work.Height() - fittedHeight) / half;
            window.MoveWindow(left, top, fittedWidth, fittedHeight);
        }
        return GetPrivateProfileIntW(section, maximizedKey, zero, path.c_str()) != zero
            ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
    }

    inline bool Save(CWnd& window)
    {
        WINDOWPLACEMENT placement{};
        placement.length = sizeof(placement);
        if (!window.GetWindowPlacement(&placement)) return false;
        CRect rect(placement.rcNormalPosition);
        // WINDOWPLACEMENTの通常位置は作業領域基準なので、画面座標に直す。
        MONITORINFO monitor{};
        monitor.cbSize = sizeof(monitor);
        if (!GetMonitorInfoW(MonitorFromWindow(window.GetSafeHwnd(), MONITOR_DEFAULTTONEAREST), &monitor))
            return false;
        rect.OffsetRect(monitor.rcWork.left - monitor.rcMonitor.left,
            monitor.rcWork.top - monitor.rcMonitor.top);
        const auto path = IniPath();
        const auto value = std::format(saveFormat, rect.left, rect.top, rect.right, rect.bottom);
        const bool maximized = placement.showCmd == SW_SHOWMAXIMIZED
            || (placement.showCmd == SW_SHOWMINIMIZED && (placement.flags & WPF_RESTORETOMAXIMIZED) != zero);
        return WritePrivateProfileStringW(section, positionKey, value.c_str(), path.c_str())
            && WritePrivateProfileStringW(section, maximizedKey, maximized ? enabled : disabled, path.c_str());
    }
}
