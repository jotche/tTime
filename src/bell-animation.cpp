#include "bell-animation.h"

#include "image-layers.h"

#include <QPainter>

#include <algorithm>
#include <utility>
#include <vector>

namespace {

using imagelayers::isWhiteArtwork;

struct Grid
{
    Grid(int w, int h)
        : width(w)
        , height(h)
        , cells(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), 0)
    {
    }

    int width;
    int height;
    std::vector<int> cells; // per pixel: 0 = outside, otherwise a label

    std::size_t index(int x, int y) const
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
    }
    int &at(int x, int y) { return cells[index(x, y)]; }
    int at(int x, int y) const { return cells[index(x, y)]; }
    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
};

// Labels 4-connected regions of foreground (-1) cells as 1, 2, ...; returns their pixel counts.
std::vector<int> labelComponents(Grid &grid)
{
    static const int dx[] = {1, -1, 0, 0};
    static const int dy[] = {0, 0, 1, -1};

    std::vector<int> sizes;
    std::vector<std::pair<int, int> > stack;
    int next = 0;
    for (int y = 0; y < grid.height; ++y) {
        for (int x = 0; x < grid.width; ++x) {
            if (grid.at(x, y) != -1) {
                continue; // -1 = unlabelled foreground
            }
            ++next;
            int count = 0;
            stack.push_back(std::make_pair(x, y));
            grid.at(x, y) = next;
            while (!stack.empty()) {
                const std::pair<int, int> cell = stack.back();
                stack.pop_back();
                ++count;
                for (int d = 0; d < 4; ++d) {
                    const int nx = cell.first + dx[d];
                    const int ny = cell.second + dy[d];
                    if (grid.inside(nx, ny) && grid.at(nx, ny) == -1) {
                        grid.at(nx, ny) = next;
                        stack.push_back(std::make_pair(nx, ny));
                    }
                }
            }
            sizes.push_back(count);
        }
    }
    return sizes;
}

// Orders labels (1, 2, ...) by the size of their region, largest first.
class LargerRegion
{
public:
    explicit LargerRegion(const std::vector<int> &sizes)
        : m_sizes(&sizes)
    {
    }
    bool operator()(int a, int b) const { return (*m_sizes)[a - 1] > (*m_sizes)[b - 1]; }

private:
    const std::vector<int> *m_sizes;
};

} // namespace

BellAnimation::BellAnimation(const QImage &icon)
    : m_valid(false)
{
    const QImage source = icon.convertToFormat(QImage::Format_ARGB32);
    const int w = source.width();
    const int h = source.height();
    if (w < 8 || h < 8) {
        return;
    }

    // 1. White artwork, grown by a small margin so the soft (antialiased) rim is included.
    const int margin = qMax(1, w / 64);
    Grid region(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (!isWhiteArtwork(source.pixel(x, y))) {
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
    std::vector<int> order;
    for (std::size_t label = 1; label <= sizes.size(); ++label) {
        order.push_back(static_cast<int>(label));
    }
    std::stable_sort(order.begin(), order.end(), LargerRegion(sizes));
    const int bellLabel = order[0];
    const int clapperLabel = order.size() > 1 ? order[1] : 0;
    if (sizes[bellLabel - 1] < (w * h) / 100) {
        return; // too small to be the bell
    }
    // Only the bell and the clapper are cut out; any other white detail stays
    // part of the (static) background.
    for (std::size_t i = 0; i < region.cells.size(); ++i) {
        if (region.cells[i] != bellLabel && region.cells[i] != clapperLabel) {
            region.cells[i] = 0;
        }
    }

    // 3. Background: fill the cut-out region from its surroundings, ring by ring.
    m_background = source;
    std::vector<char> known(region.cells.size());
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            known[region.index(x, y)] = region.at(x, y) == 0 ? 1 : 0;
        }
    }
    imagelayers::fillUnknown(m_background, known);

    // 4. Bell and clapper layers: pure white, opacity = how much whiter than the
    //    reconstructed background the original pixel is (keeps the soft edges).
    m_bell = QImage(w, h, QImage::Format_ARGB32_Premultiplied);
    m_bell.fill(0);
    m_clapper = m_bell;
    int top = h;
    double sumX = 0;
    int countX = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int label = region.at(x, y);
            if (label == 0) {
                continue;
            }
            const QRgb white = imagelayers::whiteMatte(source.pixel(x, y), m_background.pixel(x, y));
            const int alpha = qAlpha(white);
            if (label == bellLabel) {
                m_bell.setPixel(x, y, white);
                if (alpha > 128) {
                    top = qMin(top, y);
                    sumX += x;
                    ++countX;
                }
            } else if (label == clapperLabel) {
                m_clapper.setPixel(x, y, white);
            }
        }
    }
    if (countX == 0) {
        return;
    }
    // The bell hangs from the top of its handle, horizontally at its centre.
    m_pivot = QPointF(sumX / countX, top);
    m_valid = true;
}

void BellAnimation::setBackground(const QImage &background)
{
    if (m_valid && background.size() == m_background.size()) {
        m_background = background.convertToFormat(QImage::Format_ARGB32);
    }
}

void BellAnimation::drawRotated(QPainter &painter, const QImage &layer, double angle) const
{
    painter.save();
    painter.translate(m_pivot);
    painter.rotate(angle);
    painter.translate(-m_pivot);
    painter.drawImage(0, 0, layer);
    painter.restore();
}

QImage BellAnimation::frame(double bellAngle, double clapperAngle) const
{
    if (!m_valid) {
        return QImage();
    }
    QImage result = m_background.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);
    drawRotated(painter, m_clapper, clapperAngle);
    drawRotated(painter, m_bell, bellAngle);
    painter.end();
    return result;
}
