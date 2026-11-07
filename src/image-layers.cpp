#include "image-layers.h"

#include <utility>

namespace imagelayers {

namespace {

int darkest(QRgb color)
{
    return qMin(qRed(color), qMin(qGreen(color), qBlue(color)));
}

int brightest(QRgb color)
{
    return qMax(qRed(color), qMax(qGreen(color), qBlue(color)));
}

struct FilledPixel
{
    int x;
    int y;
    QRgb color;
};

} // namespace

int whiteness(QRgb color)
{
    return darkest(color);
}

bool isWhiteArtwork(QRgb color)
{
    return qAlpha(color) >= 128 && whiteness(color) >= 200 && brightest(color) - whiteness(color) <= 40;
}

void fillUnknown(QImage &image, std::vector<char> &known)
{
    const int w = image.width();
    const int h = image.height();

    bool progress = true;
    while (progress) {
        progress = false;
        std::vector<FilledPixel> filled;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (known[static_cast<std::size_t>(y) * w + x]) {
                    continue;
                }
                int r = 0, g = 0, b = 0, a = 0, n = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = x + dx;
                        const int ny = y + dy;
                        if (nx >= 0 && ny >= 0 && nx < w && ny < h && known[static_cast<std::size_t>(ny) * w + nx]) {
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
                    const FilledPixel pixel = {x, y, qRgba(r / n, g / n, b / n, a / n)};
                    filled.push_back(pixel);
                }
            }
        }
        for (std::size_t i = 0; i < filled.size(); ++i) {
            image.setPixel(filled[i].x, filled[i].y, filled[i].color);
            known[static_cast<std::size_t>(filled[i].y) * w + filled[i].x] = 1;
            progress = true;
        }
    }
}

QRgb whiteMatte(QRgb original, QRgb background)
{
    const int bg = whiteness(background);
    const int fg = whiteness(original);
    const int alpha = bg >= 255 ? 0 : qBound(0, (fg - bg) * 255 / (255 - bg), 255);
    return premultiply(qRgba(255, 255, 255, alpha * qAlpha(original) / 255));
}

QRgb premultiply(QRgb color)
{
    const int a = qAlpha(color);
    if (a == 255) {
        return color;
    }
    if (a == 0) {
        return 0;
    }
    return qRgba((qRed(color) * a + 127) / 255, (qGreen(color) * a + 127) / 255, (qBlue(color) * a + 127) / 255, a);
}

QRgb unpremultiply(QRgb color)
{
    const int a = qAlpha(color);
    if (a == 255 || a == 0) {
        return color;
    }
    return qRgba(qMin(255, (qRed(color) * 255 + a / 2) / a), qMin(255, (qGreen(color) * 255 + a / 2) / a),
                 qMin(255, (qBlue(color) * 255 + a / 2) / a), a);
}

} // namespace imagelayers
