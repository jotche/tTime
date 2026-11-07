#include "font-weight.h"

namespace {

struct NamedWeight
{
    const char *name;
    int weight;
};

const NamedWeight namedWeights[] = {
    {"thin", 100},      {"hairline", 100},  {"extralight", 200}, {"ultralight", 200},
    {"light", 300},     {"regular", 400},   {"normal", 400},     {"book", 400},
    {"medium", 500},    {"semibold", 600},  {"demibold", 600},   {"bold", 700},
    {"extrabold", 800}, {"ultrabold", 800}, {"black", 900},      {"heavy", 900},
};

} // namespace

bool parseFontWeight(const QString &text, int *weight)
{
    QString key = text.trimmed().toLower();

    bool isNumber = false;
    const int number = key.toInt(&isNumber);
    int parsed = 0;
    if (isNumber) {
        if (number < 1 || number > 1000) {
            return false;
        }
        parsed = number;
    } else {
        key.remove(QLatin1Char('-')).remove(QLatin1Char(' ')).remove(QLatin1Char('_'));
        const int count = sizeof namedWeights / sizeof namedWeights[0];
        int i = 0;
        while (i < count && key != QLatin1String(namedWeights[i].name)) {
            ++i;
        }
        if (i == count) {
            return false;
        }
        parsed = namedWeights[i].weight;
    }
    if (weight) {
        *weight = parsed;
    }
    return true;
}
