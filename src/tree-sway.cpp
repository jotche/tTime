#include "tree-sway.h"

#include "image-layers.h"

#include <cmath>

const double TreeSway::breezePeriod = 6.0;
const double TreeSway::breezeSway = 4.0;
const double TreeSway::leafFlutter = 0.8;
const double TreeSway::leafFlutterPeriod = 1.2;

namespace {

const double pi = 3.14159265358979323846;

// 0 below `edge0`, 1 above `edge1`, smooth in between.
double smoothstep(double edge0, double edge1, double value)
{
    const double t = qBound(0.0, (value - edge0) / (edge1 - edge0), 1.0);
    return t * t * (3 - 2 * t);
}

// Bilinear mix of one channel (bit position `shift`) of four pixels.
int mixChannel(QRgb c00, QRgb c10, QRgb c01, QRgb c11, double fx, double fy, unsigned shift)
{
    const double top = ((c00 >> shift) & 0xffU) * (1 - fx) + ((c10 >> shift) & 0xffU) * fx;
    const double bottom = ((c01 >> shift) & 0xffU) * (1 - fx) + ((c11 >> shift) & 0xffU) * fx;
    return qRound(top * (1 - fy) + bottom * fy);
}

} // namespace

TreeSway::TreeSway(const QImage &picture)
    : m_picture(picture.convertToFormat(QImage::Format_ARGB32))
    , m_premultiplied(picture.convertToFormat(QImage::Format_ARGB32_Premultiplied))
    , m_groundLine(0)
    , m_crownTop(0)
    , m_band(0)
    , m_corner(0)
    , m_valid(false)
{
    const int w = m_picture.width();
    const int h = m_picture.height();
    if (w < 16 || h < 16) {
        return;
    }

    // 1. The frame: a white band along the left, top and right edges. Measure its
    //    thickness on the middle row/column (plus 2 px for its soft inner edge).
    //    It is found by position: the grass line connects frame, ground and trunk.
    m_band = qMax(whiteRun(0, h / 2, 1, 0), qMax(whiteRun(w - 1, h / 2, -1, 0), whiteRun(w / 2, 0, 0, 1))) + 2;
    // The top corners are rounded: their arcs bulge inwards past the straight band.
    m_corner = 3 * m_band;
    if (m_band < 3 || 2 * m_corner >= w) {
        return; // no recognisable frame
    }

    // 2. Ground line: the first row below the frame's top band that is mostly white.
    m_groundLine = -1;
    for (int y = m_band; y < h && m_groundLine < 0; ++y) {
        int count = 0;
        for (int x = 0; x < w; ++x) {
            count += isWhite(x, y) ? 1 : 0;
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
            if (insideFrame(x, y) && isWhite(x, y)) {
                m_crownTop = y;
                break;
            }
        }
    }
    m_valid = m_crownTop < m_groundLine - 8; // a tree worth animating
}

bool TreeSway::isWhite(int x, int y) const
{
    return imagelayers::isWhiteArtwork(m_picture.pixel(x, y));
}

int TreeSway::whiteRun(int x, int y, int dx, int dy) const
{
    int run = 0;
    while (x >= 0 && y >= 0 && x < m_picture.width() && y < m_picture.height() && isWhite(x, y)) {
        ++run;
        x += dx;
        y += dy;
    }
    return run;
}

bool TreeSway::insideFrame(int x, int y) const
{
    const int w = m_picture.width();
    const bool inTopCorner = y < m_corner && (x < m_corner || x >= w - m_corner);
    return x >= m_band && x < w - m_band && y >= m_band && !inTopCorner;
}

QRgb TreeSway::sample(double x, double y) const
{
    // Bilinear filtering. Positions are kept inside the frame and above the ground,
    // so the frame's white never smears inwards.
    const int w = m_picture.width();
    x = qBound(static_cast<double>(m_band), x, static_cast<double>(w - m_band - 1));
    y = qBound(static_cast<double>(m_band), y, static_cast<double>(m_groundLine - 1));
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = qMin(x0 + 1, w - m_band - 1);
    const int y1 = qMin(y0 + 1, m_groundLine - 1);
    const double fx = x - x0;
    const double fy = y - y0;
    const QRgb c00 = m_premultiplied.pixel(x0, y0);
    const QRgb c10 = m_premultiplied.pixel(x1, y0);
    const QRgb c01 = m_premultiplied.pixel(x0, y1);
    const QRgb c11 = m_premultiplied.pixel(x1, y1);
    // Channels of a QRgb by bit position: alpha 24, red 16, green 8, blue 0.
    return imagelayers::unpremultiply(qRgba(mixChannel(c00, c10, c01, c11, fx, fy, 16U),
                                            mixChannel(c00, c10, c01, c11, fx, fy, 8U),
                                            mixChannel(c00, c10, c01, c11, fx, fy, 0U),
                                            mixChannel(c00, c10, c01, c11, fx, fy, 24U)));
}

QImage TreeSway::frame(double sway, double flutterPhase, double flutter) const
{
    if (!m_valid) {
        return QImage();
    }
    const int w = m_picture.width();
    const double height = m_groundLine - m_crownTop;
    QImage result = m_picture;

    // Rows from just above the crown down to the ground move; the empty sky between
    // the frame's top band and the crown stays as it is.
    const int firstRow = qMax(m_band, m_crownTop - 3);
    for (int y = firstRow; y < m_groundLine; ++y) {
        // 0 at the ground, 1 at the crown top: the base stays put, the crown moves most.
        const double s = qBound(0.0, (m_groundLine - y) / height, 1.0);
        const double bend = sway * s * s;
        const double leaves = smoothstep(0.45, 0.75, s); // flutter in the crown only
        if (bend == 0 && leaves * flutter == 0) {
            continue; // row unchanged
        }
        QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < w; ++x) {
            if (!insideFrame(x, y)) {
                continue; // the frame never moves
            }
            const double fx = flutter * leaves * std::sin(flutterPhase + 0.35 * x + 0.23 * y);
            const double fy = 0.5 * flutter * leaves * std::cos(flutterPhase + 0.29 * x - 0.31 * y);
            line[x] = sample(x - bend - fx, y - fy);
        }
    }
    return result;
}

double TreeSway::breeze(double seconds)
{
    // The ripple runs exactly 3 times per period, so the motion loops seamlessly.
    const double phase = 2 * pi * seconds / breezePeriod;
    return 0.65 * std::sin(phase) + 0.35 * std::sin(3 * phase + 1.1);
}
