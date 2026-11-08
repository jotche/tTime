#include "platform.h"

#include <slint.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <windows.h>
#    include <shellapi.h>
#    include <shellscalingapi.h>
#else
#    include <fcntl.h>
#    include <sys/file.h>
#    include <unistd.h>
#    ifdef __APPLE__
#        include <mach-o/dyld.h>
#    endif
#endif

namespace fs = std::filesystem;

namespace platform {

namespace {

constexpr int maxSlots = 64;

#ifdef _WIN32
std::string toUtf8(const wchar_t *text)
{
    const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(std::max(size, 1)) - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring toWide(const std::string &text)
{
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring result(static_cast<std::size_t>(std::max(size, 1)) - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), size);
    return result;
}
#endif

fs::path slotDirectory()
{
    std::error_code error;
    fs::path dir = fs::temp_directory_path(error) / "tTime-window-slots";
    fs::create_directories(dir, error);
    return dir;
}

} // namespace

std::vector<std::string> arguments([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    std::vector<std::string> result;
#ifdef _WIN32
    // argv is in the ANSI code page; the wide command line keeps every character.
    int count = 0;
    if (wchar_t **wide = CommandLineToArgvW(GetCommandLineW(), &count)) {
        for (int i = 1; i < count; ++i) {
            result.push_back(toUtf8(wide[i]));
        }
        LocalFree(wide);
    }
#else
    for (int i = 1; i < argc; ++i) {
        result.emplace_back(argv[i]);
    }
#endif
    return result;
}

fs::path executableDirectory()
{
#ifdef _WIN32
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD length = 0;
    while ((length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()))) == buffer.size()) {
        buffer.resize(buffer.size() * 2);
    }
    buffer.resize(length);
    return fs::path(buffer).parent_path();
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    _NSGetExecutablePath(buffer.data(), &size);
    std::error_code error;
    return fs::canonical(buffer.c_str(), error).parent_path();
#else
    std::error_code error;
    return fs::read_symlink("/proc/self/exe", error).parent_path();
#endif
}

fs::path userConfigDirectory()
{
#ifdef _WIN32
    if (const wchar_t *appData = _wgetenv(L"APPDATA"); appData && *appData) {
        return appData;
    }
    return fs::temp_directory_path();
#else
    if (const char *xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        return xdg;
    }
    const char *home = std::getenv("HOME");
    return fs::path(home ? home : "/tmp") / ".config";
#endif
}

std::vector<Rect> availableScreenAreas()
{
    std::vector<Rect> areas;
#ifdef _WIN32
    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR monitor, HDC, LPRECT, LPARAM data) -> BOOL {
            MONITORINFO info{};
            info.cbSize = sizeof info;
            if (GetMonitorInfoW(monitor, &info)) {
                UINT dpiX = 96;
                UINT dpiY = 96;
                GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
                const double scale = dpiX / 96.0;
                const RECT &r = info.rcWork;
                reinterpret_cast<std::vector<Rect> *>(data)->push_back(
                    {static_cast<int>(r.left / scale), static_cast<int>(r.top / scale),
                     static_cast<int>((r.right - r.left) / scale), static_cast<int>((r.bottom - r.top) / scale)});
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&areas));
#endif
    // macOS / Linux: Slint has no screen API yet; windows are then placed without fitting.
    return areas;
}

Rect primaryScreenArea()
{
#ifdef _WIN32
    HMONITOR monitor = MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{};
    info.cbSize = sizeof info;
    if (GetMonitorInfoW(monitor, &info)) {
        UINT dpiX = 96;
        UINT dpiY = 96;
        GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        const double scale = dpiX / 96.0;
        const RECT &r = info.rcWork;
        return {static_cast<int>(r.left / scale), static_cast<int>(r.top / scale),
                static_cast<int>((r.right - r.left) / scale), static_cast<int>((r.bottom - r.top) / scale)};
    }
#endif
    return {};
}

namespace {

// Area-averaged downscale (premultiplied, so transparent pixels don't darken the edges).
[[maybe_unused]] Image scaledDown(const Image &image, int size)
{
    if (image.width <= size && image.height <= size) {
        return image;
    }
    const Image source = premultiplied(image);
    Image result(size, size);
    for (int y = 0; y < size; ++y) {
        const int y0 = y * source.height / size;
        const int y1 = std::max(y0 + 1, (y + 1) * source.height / size);
        for (int x = 0; x < size; ++x) {
            const int x0 = x * source.width / size;
            const int x1 = std::max(x0 + 1, (x + 1) * source.width / size);
            long r = 0, g = 0, b = 0, a = 0, n = 0;
            for (int sy = y0; sy < y1; ++sy) {
                for (int sx = x0; sx < x1; ++sx) {
                    const Rgba p = source.at(sx, sy);
                    r += p.r;
                    g += p.g;
                    b += p.b;
                    a += p.a;
                    ++n;
                }
            }
            result.at(x, y) = {static_cast<std::uint8_t>(r / n), static_cast<std::uint8_t>(g / n),
                               static_cast<std::uint8_t>(b / n), static_cast<std::uint8_t>(a / n)};
        }
    }
    return unpremultiplied(std::move(result));
}

#ifdef _WIN32
std::shared_ptr<void> createIcon(const Image &image, int size)
{
    const Image scaled = scaledDown(image, size);
    BITMAPV5HEADER header{};
    header.bV5Size = sizeof header;
    header.bV5Width = scaled.width;
    header.bV5Height = -scaled.height; // top-down
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5RedMask = 0x00FF0000;
    header.bV5GreenMask = 0x0000FF00;
    header.bV5BlueMask = 0x000000FF;
    header.bV5AlphaMask = 0xFF000000;
    void *bits = nullptr;
    HDC dc = GetDC(nullptr);
    HBITMAP color = CreateDIBSection(dc, reinterpret_cast<BITMAPINFO *>(&header), DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, dc);
    if (!color) {
        return {};
    }
    auto *out = static_cast<std::uint32_t *>(bits);
    for (const Rgba &p : scaled.pixels) {
        *out++ = (static_cast<std::uint32_t>(p.a) << 24) | (p.r << 16) | (p.g << 8) | p.b;
    }
    HBITMAP mask = CreateBitmap(scaled.width, scaled.height, 1, 1, nullptr);
    ICONINFO info{TRUE, 0, 0, mask, color};
    HICON icon = CreateIconIndirect(&info);
    DeleteObject(color);
    DeleteObject(mask);
    if (!icon) {
        return {};
    }
    return {icon, [](void *handle) { DestroyIcon(static_cast<HICON>(handle)); }};
}
#endif

} // namespace

WindowIcon::WindowIcon([[maybe_unused]] const Image &image, [[maybe_unused]] float scale)
{
#ifdef _WIN32
    if (!image.isNull()) {
        m_big = createIcon(image, static_cast<int>(std::lround(GetSystemMetrics(SM_CXICON) * scale)));
        m_small = createIcon(image, static_cast<int>(std::lround(GetSystemMetrics(SM_CXSMICON) * scale)));
    }
#endif
}

#ifdef _WIN32
namespace {

// The icon each window currently shows, kept alive until the window gets another one
// (a window must not be left with a destroyed HICON).
std::map<HWND, WindowIcon> &iconsInUse()
{
    static std::map<HWND, WindowIcon> icons;
    return icons;
}

} // namespace
#endif

void setWindowIcon([[maybe_unused]] const slint::Window &window, [[maybe_unused]] const WindowIcon &icon,
                   [[maybe_unused]] bool onlyIfUnset)
{
#ifdef _WIN32
    HWND hwnd = window.win32_hwnd();
    if (!hwnd || icon.isNull()) {
        return; // not shown yet: see applyWindowIcon()
    }
    auto &icons = iconsInUse();
    std::erase_if(icons, [](const auto &entry) { return !IsWindow(entry.first); }); // closed windows
    if (onlyIfUnset && icons.contains(hwnd)) {
        return;
    }
    SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon.m_big.get()));
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon.m_small.get()));
    icons[hwnd] = icon; // releases the previous icon only now that it is no longer shown
#endif
}

bool hasNativeWindow([[maybe_unused]] const slint::Window &window)
{
#ifdef _WIN32
    return window.win32_hwnd() != nullptr;
#else
    return true;
#endif
}

std::optional<Point> cursorPosition()
{
#ifdef _WIN32
    POINT point;
    if (GetCursorPos(&point)) {
        return Point{point.x, point.y};
    }
#endif
    return std::nullopt;
}

void activate([[maybe_unused]] const slint::Window &window)
{
#ifdef _WIN32
    if (HWND hwnd = window.win32_hwnd()) {
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        }
        SetForegroundWindow(hwnd);
    }
#endif
}

void showMessage(const std::string &title, const std::string &text, bool error)
{
#ifdef _WIN32
    MessageBoxW(nullptr, toWide(text).c_str(), toWide(title).c_str(),
                MB_OK | (error ? MB_ICONERROR : MB_ICONINFORMATION));
#else
    std::fprintf(error ? stderr : stdout, "%s\n", text.c_str());
    (void)title;
#endif
}

WindowSlot::WindowSlot()
{
    const fs::path dir = slotDirectory();
    for (int i = 0; i < maxSlots; ++i) {
        const fs::path file = dir / ("slot-" + std::to_string(i) + ".lock");
#ifdef _WIN32
        // No sharing: a second open fails while this process holds the file. Windows
        // closes (and deletes) it when the process ends, even after a crash.
        HANDLE handle = CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            m_handle = handle;
            m_index = i;
            return;
        }
#else
        const int fd = ::open(file.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
        if (fd >= 0 && ::flock(fd, LOCK_EX | LOCK_NB) == 0) {
            m_fd = fd;
            m_index = i;
            return;
        }
        if (fd >= 0) {
            ::close(fd);
        }
#endif
    }
    m_index = 0; // every slot is taken (or the temp dir is unusable): use the base position
}

WindowSlot::~WindowSlot()
{
#ifdef _WIN32
    if (m_handle) {
        CloseHandle(static_cast<HANDLE>(m_handle));
    }
#else
    if (m_fd >= 0) {
        ::close(m_fd);
    }
#endif
}

} // namespace platform
