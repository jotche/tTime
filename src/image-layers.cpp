#include "image-layers.h"

#include <algorithm>
#include <utility>

namespace imagelayers {

int whiteness(QRgb color)
{
    return std::min({qRed(color), qGreen(color), qBlue(color)});
}

bool isWhiteArtwork(QRgb color)
{
    return qAlpha(color) >= 128 && whiteness(color) >= 200
        && std::max({qRed(color), qGreen(color), qBlue(color)}) - whiteness(color) <= 40;
}

void fillUnknown(QImage &image, std::vector<char> &known)
{
    const int w = image.width();
    const int h = image.height();
    const auto index = [w](int x, int y) {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x);
    };

    bool progress = true;
    while (progress) {
        progress = false;
        std::vector<std::pair<std::pair<int, int>, QRgb>> filled;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (known[index(x, y)]) {
                    continue;
                }
                int r = 0, g = 0, b = 0, a = 0, n = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = x + dx;
                        const int ny = y + dy;
                        if (nx >= 0 && ny >= 0 && nx < w && ny < h && known[index(nx, ny)]) {
                            const QRgb c = image.pixel(nx, ny);
                            r += qRed(c);
                            g += qGreen(c);
                            b += qBlue(c);
                            a += qAlpha(c);
                            ++n;
                        }
                    }
                }
                if (n > 0) {
                    filled.push_back({{x, y}, qRgba(r / n, g / n, b / n, a / n)});
                }
            }
        }
        for (const auto &[pos, color] : filled) {
            image.setPixel(pos.first, pos.second, color);
            known[index(pos.first, pos.second)] = 1;
            progress = true;
        }
    }
}

QRgb whiteMatte(QRgb original, QRgb background)
{
    const int bg = whiteness(background);
    const int fg = whiteness(original);
    const int alpha = bg >= 255 ? 0 : std::clamp((fg - bg) * 255 / (255 - bg), 0, 255);
    return qPremultiply(qRgba(255, 255, 255, alpha * qAlpha(original) / 255));
}

} // namespace imagelayers
