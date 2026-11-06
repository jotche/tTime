#include "font-weight.h"

#include <QString>

#include <utility>

std::optional<int> parseFontWeight(QStringView text)
{
    QString key = text.trimmed().toString().toLower();

    bool isNumber = false;
    const int number = key.toInt(&isNumber);
    if (isNumber) {
        return number >= 1 && number <= 1000 ? std::optional<int>(number) : std::nullopt;
    }

    key.remove(QLatin1Char('-')).remove(QLatin1Char(' ')).remove(QLatin1Char('_'));
    static const std::pair<const char *, int> names[] = {
        {"thin", 100},      {"hairline", 100},  {"extralight", 200}, {"ultralight", 200},
        {"light", 300},     {"regular", 400},   {"normal", 400},     {"book", 400},
        {"medium", 500},    {"semibold", 600},  {"demibold", 600},   {"bold", 700},
        {"extrabold", 800}, {"ultrabold", 800}, {"black", 900},      {"heavy", 900},
    };
    for (const auto &[name, weight] : names) {
        if (key == QLatin1String(name)) {
            return weight;
        }
    }
    return std::nullopt;
}
