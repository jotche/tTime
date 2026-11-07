#pragma once

#include <QString>

// Parses a font weight as written in a settings file: a CSS/OpenType number
// (1-1000, e.g. "300") or a common name ("light", "semibold", "semi-bold",
// "demibold", "bold", ...). Case and surrounding whitespace are ignored.
// Returns false if `text` is not a weight; `weight` may be 0.
bool parseFontWeight(const QString &text, int *weight = 0);
