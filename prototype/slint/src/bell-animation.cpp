#include "bell-animation.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <numeric>
#include <utility>

namespace imagelayers {

int whiteness(Rgba color)
{
    return std::min({color.r, color.g, color.b});
}

bool isWhiteArtwork(Rgba color)
{
    return color.a >= 128 && whiteness(color) >= 200 && std::max({color.r, color.g, color.b}) - whiteness(color) <= 40;
}

namespace {

// Fills every pixel whose `known` entry is 0 from its known neighbours, ring by ring.
void fillUnknown(Image &image, std::vector<char> &known)
{
    const int w = image.width;
    const int h = image.height;
    const auto index = [w](int x, int y) { return static_cast<std::size_t>(y) * w + x; };
    bool progress = true;
    while (progress) {
        progress = false;
        std::vector<std::pair<std::size_t, Rgba>> filled;
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
                            const Rgba c = image.at(nx, ny);
                            r += c.r;
                            g += c.g;
                            b += c.b;
                            a += c.a;
                            ++n;
                        }
                    }
                }
                if (n > 0) {
                    filled.emplace_back(index(x, y),
                                        Rgba{static_cast<std::uint8_t>(r / n), static_cast<std::uint8_t>(g / n),
                                             static_cast<std::uint8_t>(b / n), static_cast<std::uint8_t>(a / n)});
                }
            }
        }
        for (const auto &[i, color] : filled) {
            image.pixels[i] = color;
            known[i] = 1;
            progress = true;
        }
    }
}

// The artwork pixel as pure white (premultiplied), with an opacity of how much whiter
// `original` is than the rebuilt `background`; keeps soft edges.
Rgba whiteMatte(Rgba original, Rgba background)
{
    const int bg = whiteness(background);
    const int fg = whiteness(original);
    const int alpha = bg >= 255 ? 0 : std::clamp((fg - bg) * 255 / (255 - bg), 0, 255);
    const auto a = static_cast<std::uint8_t>(alpha * original.a / 255);
    return {a, a, a, a};
}

} // namespace
} // namespace imagelayers

namespace {

struct Grid
{
    int width = 0;
    int height = 0;
    std::vector<int> cells; // per pixel: 0 = outside, -1 = unlabelled foreground, otherwise a label
    [[nodiscard]] std::size_t index(int x, int y) const { return static_cast<std::size_t>(y) * width + x; }
    int &at(int x, int y) { return cells[index(x, y)]; }
    [[nodiscard]] int at(int x, int y) const { return cells[index(x, y)]; }
    [[nodiscard]] bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
};

// Labels 4-connected regions of -1 cells as 1, 2, ...; returns their pixel counts.
std::vector<int> labelComponents(Grid &grid)
{
    std::vector<int> sizes;
    std::vector<std::pair<int, int>> stack;
    int next = 0;
    for (int y = 0; y < grid.height; ++y) {
        for (int x = 0; x < grid.width; ++x) {
            if (grid.at(x, y) != -1) {
                continue;
            }
            ++next;
            int count = 0;
            stack.emplace_back(x, y);
            grid.at(x, y) = next;
            while (!stack.empty()) {
                const auto [cx, cy] = stack.back();
                stack.pop_back();
                ++count;
                for (const auto &[dx, dy] : {std::pair{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
                    const int nx = cx + dx;
                    const int ny = cy + dy;
                    if (grid.inside(nx, ny) && grid.at(nx, ny) == -1) {
                        grid.at(nx, ny) = next;
                        stack.emplace_back(nx, ny);
                    }
                }
            }
            sizes.push_back(count);
        }
    }
    return sizes;
}

// Bilinear sample of a premultiplied image; transparent outside.
Rgba sampleBilinear(const Image &image, double x, double y)
{
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const double fx = x - x0;
    const double fy = y - y0;
    double acc[4] = {0, 0, 0, 0};
    for (int dy = 0; dy <= 1; ++dy) {
        for (int dx = 0; dx <= 1; ++dx) {
            const int px = x0 + dx;
            const int py = y0 + dy;
            if (px < 0 || py < 0 || px >= image.width || py >= image.height) {
                continue;
            }
            const double weight = (dx ? fx : 1 - fx) * (dy ? fy : 1 - fy);
            const Rgba c = image.at(px, py);
            acc[0] += c.r * weight;
            acc[1] += c.g * weight;
            acc[2] += c.b * weight;
            acc[3] += c.a * weight;
        }
    }
    const auto byte = [](double v) { return static_cast<std::uint8_t>(std::clamp(std::lround(v), 0L, 255L)); };
    return {byte(acc[0]), byte(acc[1]), byte(acc[2]), byte(acc[3])};
}

// Draws premultiplied `layer` rotated by `angle` degrees around the pivot onto
// premultiplied `target` (source-over).
void drawRotated(Image &target, const Image &layer, double angle, double pivotX, double pivotY)
{
    const double radians = -angle * std::numbers::pi / 180.0; // inverse rotation
    const double cosA = std::cos(radians);
    const double sinA = std::sin(radians);
    for (int y = 0; y < target.height; ++y) {
        for (int x = 0; x < target.width; ++x) {
            const double dx = x + 0.5 - pivotX;
            const double dy = y + 0.5 - pivotY;
            const Rgba src = sampleBilinear(layer, pivotX + dx * cosA - dy * sinA - 0.5, pivotY + dx * sinA + dy * cosA - 0.5);
            if (src.a == 0) {
                continue;
            }
            Rgba &dst = target.at(x, y);
            const int inverse = 255 - src.a;
            dst = {static_cast<std::uint8_t>(src.r + (dst.r * inverse + 127) / 255),
                   static_cast<std::uint8_t>(src.g + (dst.g * inverse + 127) / 255),
                   static_cast<std::uint8_t>(src.b + (dst.b * inverse + 127) / 255),
                   static_cast<std::uint8_t>(src.a + (dst.a * inverse + 127) / 255)};
        }
    }
}

} // namespace

BellAnimation::BellAnimation(const Image &source)
{
    using imagelayers::isWhiteArtwork;
    const int w = source.width;
    const int h = source.height;
    if (w < 8 || h < 8) {
        return;
    }

    // 1. White artwork, grown by a small margin so the soft (antialiased) rim is included.
    const int margin = std::max(1, w / 64);
    Grid region{w, h, std::vector<int>(static_cast<std::size_t>(w) * h, 0)};
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (!isWhiteArtwork(source.at(x, y))) {
                continue;
            }
            for (int dy = -margin; dy <= margin; ++dy) {
                for (int dx = -margin; dx <= margin; ++dx) {
                    if (region.inside(x + dx, y + dy)) {
                        region.at(x + dx, y + dy) = -1;
                    }
                }
            }
        }
    }

    // 2. Bell = largest white region, clapper = second largest (if any).
    const std::vector<int> sizes = labelComponents(region);
    if (sizes.empty()) {
        return;
    }
    const auto sizeOf = [&](int label) { return sizes[static_cast<std::size_t>(label - 1)]; };
    std::vector<int> order(sizes.size());
    std::iota(order.begin(), order.end(), 1);
    std::ranges::sort(order, [&](int a, int b) { return sizeOf(a) > sizeOf(b); });
    const int bellLabel = order[0];
    const int clapperLabel = order.size() > 1 ? order[1] : 0;
    if (sizeOf(bellLabel) < (w * h) / 100) {
        return; // too small to be the bell
    }
    for (int &cell : region.cells) {
        if (cell != bellLabel && cell != clapperLabel) {
            cell = 0; // other white details stay part of the (static) background
        }
    }

    // 3. Background: fill the cut-out region from its surroundings, ring by ring.
    m_background = source;
    std::vector<char> known(region.cells.size());
    for (std::size_t i = 0; i < known.size(); ++i) {
        known[i] = region.cells[i] == 0 ? 1 : 0;
    }
    imagelayers::fillUnknown(m_background, known);

    // 4. Bell and clapper layers: pure white, opacity = how much whiter than the
    //    reconstructed background the original pixel is (keeps the soft edges).
    m_bell = Image(w, h);
    m_clapper = Image(w, h);
    int top = h;
    double sumX = 0;
    int countX = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int label = region.at(x, y);
            if (label == 0) {
                continue;
            }
            const Rgba white = imagelayers::whiteMatte(source.at(x, y), m_background.at(x, y));
            if (label == bellLabel) {
                m_bell.at(x, y) = white;
                if (white.a > 128) {
                    top = std::min(top, y);
                    sumX += x;
                    ++countX;
                }
            } else if (label == clapperLabel) {
                m_clapper.at(x, y) = white;
            }
        }
    }
    if (countX == 0) {
        return;
    }
    // The bell hangs from the top of its handle, horizontally at its centre.
    m_pivotX = sumX / countX;
    m_pivotY = top;
    m_valid = true;
}

void BellAnimation::setBackground(const Image &background)
{
    if (m_valid && background.width == m_background.width && background.height == m_background.height) {
        m_background = background;
    }
}

Image BellAnimation::frame(double bellAngle, double clapperAngle) const
{
    if (!m_valid) {
        return {};
    }
    Image result = premultiplied(m_background);
    drawRotated(result, m_clapper, clapperAngle, m_pivotX, m_pivotY);
    drawRotated(result, m_bell, bellAngle, m_pivotX, m_pivotY);
    return unpremultiplied(std::move(result));
}
