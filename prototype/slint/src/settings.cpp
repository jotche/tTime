#include "settings.h"

#include "platform.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

std::string toLowerAscii(std::string text)
{
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::string trimmed(const std::string &text)
{
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

// ---------------------------------------------------------------------------
// IniFile
// ---------------------------------------------------------------------------

IniFile::IniFile(fs::path path)
    : m_path(std::move(path))
{
    load();
}

std::pair<std::string, std::string> IniFile::split(const std::string &key)
{
    const auto slash = key.find('/');
    if (slash == std::string::npos) {
        return {"General", key};
    }
    return {key.substr(0, slash), key.substr(slash + 1)};
}

const std::string *IniFile::find(const std::string &key) const
{
    const auto [section, name] = split(key);
    for (const Section &s : m_sections) {
        if (s.name != section) {
            continue;
        }
        for (const auto &[k, v] : s.entries) {
            if (k == name) {
                return &v;
            }
        }
    }
    return nullptr;
}

bool IniFile::contains(const std::string &key) const
{
    return find(key) != nullptr;
}

std::optional<std::string> IniFile::value(const std::string &key) const
{
    if (const std::string *v = find(key)) {
        return *v;
    }
    return std::nullopt;
}

void IniFile::setValue(const std::string &key, const std::string &value)
{
    const auto [section, name] = split(key);
    auto it = std::ranges::find(m_sections, section, &Section::name);
    if (it == m_sections.end()) {
        m_sections.push_back({section, {}});
        it = std::prev(m_sections.end());
    }
    for (auto &[k, v] : it->entries) {
        if (k == name) {
            v = value;
            return;
        }
    }
    it->entries.emplace_back(name, value);
}

void IniFile::parse(const std::string &text)
{
    m_sections.clear();
    std::istringstream in(text);
    std::string line;
    std::string section = "General";
    while (std::getline(in, line)) {
        line = trimmed(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            section = trimmed(line.substr(1, line.size() - 2));
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        std::string value = trimmed(line.substr(equals + 1));
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }
        setValue(section + "/" + trimmed(line.substr(0, equals)), value);
    }
}

std::string IniFile::format() const
{
    std::string out;
    for (const Section &s : m_sections) {
        if (!out.empty()) {
            out += '\n';
        }
        out += '[' + s.name + "]\n";
        for (const auto &[k, v] : s.entries) {
            // Quote values QSettings would quote (leading/trailing spaces, separators).
            const bool quote = !v.empty() && (v.front() == ' ' || v.back() == ' ' || v.find_first_of(";,=\"") != std::string::npos);
            out += k + '=' + (quote ? '"' + v + '"' : v) + '\n';
        }
    }
    return out;
}

bool IniFile::load()
{
    m_sections.clear();
    std::error_code error;
    if (m_path.empty() || !fs::exists(m_path, error)) {
        return true;
    }
    std::ifstream file(m_path, std::ios::binary);
    if (!file) {
        return false;
    }
    std::ostringstream text;
    text << file.rdbuf();
    std::string content = text.str();
    if (content.starts_with("\xEF\xBB\xBF")) {
        content.erase(0, 3); // UTF-8 byte order mark
    }
    parse(content);
    return true;
}

bool IniFile::save() const
{
    std::error_code error;
    fs::create_directories(m_path.parent_path(), error);
    std::ofstream file(m_path, std::ios::binary | std::ios::trunc);
    if (!file) {
        std::fprintf(stderr, "tTime: cannot write %s\n", m_path.string().c_str());
        return false;
    }
    file << format();
    return static_cast<bool>(file);
}

// ---------------------------------------------------------------------------
// AppSettings
// ---------------------------------------------------------------------------

AppSettings::AppSettings(fs::path userFile, fs::path defaultsFile)
    : m_user(std::move(userFile))
    , m_defaults(std::move(defaultsFile))
{
}

AppSettings AppSettings::forApplication()
{
    return {userFilePath(), defaultsFilePath()};
}

fs::path AppSettings::userFilePath()
{
    // %APPDATA%\tTime\tTime.ini on Windows, ~/.config/tTime/tTime.ini elsewhere
    // (where QSettings::IniFormat with UserScope puts it).
    return platform::userConfigDirectory() / "tTime" / "tTime.ini";
}

fs::path AppSettings::defaultsFilePath()
{
#ifdef __APPLE__
    // tTime.app/Contents/MacOS/tTime -> tTime.app/Contents/Resources/tTime.ini
    return platform::executableDirectory() / ".." / "Resources" / "tTime.ini";
#else
    return platform::executableDirectory() / "tTime.ini";
#endif
}

std::optional<std::string> AppSettings::value(const std::string &key) const
{
    if (auto v = m_user.value(key)) {
        return v;
    }
    return m_defaults.value(key);
}

std::string AppSettings::value(const std::string &key, const std::string &fallback) const
{
    return value(key).value_or(fallback);
}

void AppSettings::setValue(const std::string &key, const std::string &value)
{
    // Re-read first: another tTime process may have saved its own window meanwhile.
    m_user.load();
    m_user.setValue(key, value);
    m_user.save();
}

std::optional<bool> AppSettings::parseSwitch(const std::string &text)
{
    const std::string key = toLowerAscii(trimmed(text));
    if (key == "on" || key == "true" || key == "yes" || key == "1") {
        return true;
    }
    if (key == "off" || key == "false" || key == "no" || key == "0") {
        return false;
    }
    return std::nullopt;
}

std::optional<std::pair<int, int>> AppSettings::parsePair(const std::string &text, const std::string &type)
{
    const std::string prefix = '@' + type + '(';
    const std::string t = trimmed(text);
    if (!t.starts_with(prefix) || !t.ends_with(")")) {
        return std::nullopt;
    }
    int a = 0;
    int b = 0;
    char end = 0;
    if (std::sscanf(t.c_str() + prefix.size(), "%d %d%c", &a, &b, &end) != 3 || end != ')') {
        return std::nullopt;
    }
    return std::pair{a, b};
}

std::string AppSettings::formatPair(const std::string &type, int a, int b)
{
    return '@' + type + '(' + std::to_string(a) + ' ' + std::to_string(b) + ')';
}
