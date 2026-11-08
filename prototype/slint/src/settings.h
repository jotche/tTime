#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// An INI file in the format QSettings writes (so the Qt and the Slint tTime share
// %APPDATA%\tTime\tTime.ini): [section] headers, key=value lines, ';' or '#'
// comments, values optionally in double quotes. Keys are addressed as
// "section/key"; keys before the first section are in "General".
class IniFile
{
public:
    IniFile() = default;
    explicit IniFile(std::filesystem::path path);

    [[nodiscard]] const std::filesystem::path &path() const noexcept { return m_path; }
    [[nodiscard]] bool contains(const std::string &key) const;
    [[nodiscard]] std::optional<std::string> value(const std::string &key) const;
    void setValue(const std::string &key, const std::string &value);

    // Reads the file (a missing file is empty). Returns false if it exists but cannot be read.
    bool load();
    // Writes all entries (comments are not kept, like QSettings). Creates the directory.
    bool save() const;

    // Parses / formats INI text (for tests).
    void parse(const std::string &text);
    [[nodiscard]] std::string format() const;

private:
    struct Section
    {
        std::string name;
        std::vector<std::pair<std::string, std::string>> entries;
    };
    [[nodiscard]] static std::pair<std::string, std::string> split(const std::string &key);
    [[nodiscard]] const std::string *find(const std::string &key) const;

    std::filesystem::path m_path;
    std::vector<Section> m_sections;
};

// tTime's settings, read from two INI files:
//
//   1. the user's file      %APPDATA%\tTime\tTime.ini   (read/write)
//   2. the shipped defaults <exe dir>\tTime.ini          (read-only)
//
// value() returns the first file that has the key; setValue() only writes the user's file.
class AppSettings
{
public:
    AppSettings(std::filesystem::path userFile, std::filesystem::path defaultsFile);

    // The settings of the running application (the two files above).
    [[nodiscard]] static AppSettings forApplication();
    [[nodiscard]] static std::filesystem::path userFilePath();
    [[nodiscard]] static std::filesystem::path defaultsFilePath();

    [[nodiscard]] std::optional<std::string> value(const std::string &key) const;
    [[nodiscard]] std::string value(const std::string &key, const std::string &fallback) const;
    void setValue(const std::string &key, const std::string &value); // saved right away

    // Parses an on/off setting: on, off, true, false, yes, no, 1, 0 (any case).
    [[nodiscard]] static std::optional<bool> parseSwitch(const std::string &text);

    // QSettings' text form of QSize / QPoint: "@Size(200 130)", "@Point(250 250)".
    [[nodiscard]] static std::optional<std::pair<int, int>> parsePair(const std::string &text, const std::string &type);
    [[nodiscard]] static std::string formatPair(const std::string &type, int a, int b);

private:
    IniFile m_user;
    IniFile m_defaults;
};

// Lower-case ASCII copy, and whitespace trimmed copy.
[[nodiscard]] std::string toLowerAscii(std::string text);
[[nodiscard]] std::string trimmed(const std::string &text);
