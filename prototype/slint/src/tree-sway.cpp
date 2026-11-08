#include "tree-sway.h"

#include "bell-animation.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace {

// 0 below `edge0`, 1 above `edge1`, smooth in between.
double smoothstep(double edge0, double edge1, double value)
{
    const double t = std::clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3 - 2 * t);
}

} // namespace

TreeSway::TreeSway(const Image &picture)
    : m_picture(picture)
    , m_premultiplied(premultiplied(picture))
{
    const int w = m_picture.width;
    const int h = m_picture.height;
    if (w < 16 || h < 16) {
        return;
    }
    const auto white = [&](int x, int y) { return imagelayers::isWhiteArtwork(m_picture.at(x, y)); };

    // 1. The frame: a white band along the left, top and right edges (plus 2 px for
    //    its soft inner edge), measured on the middle row/column.
    const auto whiteRun = [&](int x, int y, int dx, int dy) {
        int run = 0;
        while (x >= 0 && y >= 0 && x < w && y < h && white(x, y)) {
            ++run;
            x += dx;
            y += dy;
        }
        return run;
    };
    m_band = std::max({whiteRun(0, h / 2, 1, 0), whiteRun(w - 1, h / 2, -1, 0), whiteRun(w / 2, 0, 0, 1)}) + 2;
    m_corner = 3 * m_band; // the rounded top corners bulge inwards past the band
    if (m_band < 3 || 2 * m_corner >= w) {
        return;
    }

    // 2. Ground line: the first row below the frame's top band that is mostly white.
    m_groundLine = -1;
    for (int y = m_band; y < h && m_groundLine < 0; ++y) {
        int count = 0;
        for (int x = 0; x < w; ++x) {
            count += white(x, y) ? 1 : 0;
        }
        if (count * 2 >= w) {
            m_groundLine = y;
        }
    }
    if (m_groundLine <= m_band) {
        return;
    }

    // 3. Crown top: the highest white artwork inside the frame.
    m_crownTop = m_groundLine;
    for (int y = m_band; y < m_groundLine && m_crownTop == m_groundLine; ++y) {
        for (int x = 0; x < w; ++x) {
            if (insideFrame(x, y) && white(x, y)) {
                m_crownTop = y;
                break;
            }
        }
    }
    m_valid = m_crownTop < m_groundLine - 8;
}

bool TreeSway::insideFrame(int x, int y) const
{
    const int w = m_picture.width;
    const bool inTopCorner = y < m_corner && (x < m_corner || x >= w - m_corner);
    return x >= m_band && x < w - m_band && y >= m_band && !inTopCorner;
}

Image TreeSway::frame(double sway, double flutterPhase, double flutter) const
{
    if (!m_valid) {
        return {};
    }
    const int w = m_picture.width;
    const double height = m_groundLine - m_crownTop;
    Image result = m_picture;

    // Bilinear sample, kept inside the frame and above the ground.
    const auto sample = [&](double x, double y) {
        x = std::clamp(x, static_cast<double>(m_band), static_cast<double>(w - m_band - 1));
        y = std::clamp(y, static_cast<double>(m_band), static_cast<double>(m_groundLine - 1));
        const int x0 = static_cast<int>(std::floor(x));
        const int y0 = static_cast<int>(std::floor(y));
        const int x1 = std::min(x0 + 1, w - m_band - 1);
        const int y1 = std::min(y0 + 1, m_groundLine - 1);
        const double fx = x - x0;
        const double fy = y - y0;
        const Rgba c00 = m_premultiplied.at(x0, y0);
        const Rgba c10 = m_premultiplied.at(x1, y0);
        const Rgba c01 = m_premultiplied.at(x0, y1);
        const Rgba c11 = m_premultiplied.at(x1, y1);
        const auto mix = [&](std::uint8_t Rgba::*channel) {
            const double top = c00.*channel * (1 - fx) + c10.*channel * fx;
            const double bottom = c01.*channel * (1 - fx) + c11.*channel * fx;
            return static_cast<std::uint8_t>(std::lround(top * (1 - fy) + bottom * fy));
        };
        return unpremultiplied(Rgba{mix(&Rgba::r), mix(&Rgba::g), mix(&Rgba::b), mix(&Rgba::a)});
    };

    const int firstRow = std::max(m_band, m_crownTop - 3);
    for (int y = firstRow; y < m_groundLine; ++y) {
        // 0 at the ground, 1 at the crown top: the base stays put, the crown moves most.
        const double s = std::clamp((m_groundLine - y) / height, 0.0, 1.0);
        const double bend = sway * s * s;
        const double leaves = smoothstep(0.45, 0.75, s);
        if (bend == 0 && leaves * flutter == 0) {
            continue;
        }
        for (int x = 0; x < w; ++x) {
            if (!insideFrame(x, y)) {
                continue;
            }
            const double fx = flutter * leaves * std::sin(flutterPhase + 0.35 * x + 0.23 * y);
            const double fy = 0.5 * flutter * leaves * std::cos(flutterPhase + 0.29 * x - 0.31 * y);
            result.at(x, y) = sample(x - bend - fx, y - fy);
        }
    }
    return result;
}

double TreeSway::breeze(double seconds)
{
    const double phase = 2 * std::numbers::pi * seconds / breezePeriod;
    return 0.65 * std::sin(phase) + 0.35 * std::sin(3 * phase + 1.1);
}
