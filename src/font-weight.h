#pragma once

#include <QStringView>

#include <optional>

// Parses a font weight as written in a settings file: a CSS/OpenType number
// (1-1000, e.g. "300") or a common name ("light", "semibold", "semi-bold",
// "demibold", "bold", ...). Case and surrounding whitespace are ignored.
[[nodiscard]] std::optional<int> parseFontWeight(QStringView text);
