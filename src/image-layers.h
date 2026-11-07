#pragma once

#include <QImage>

#include <vector>

// Helpers for splitting white artwork out of tTime's pictures into layers
// (used by BellAnimation; TreeSway uses the white-artwork test).
namespace imagelayers {

// "Whiteness" of a pixel: its darkest channel (white is high in every channel,
// a colored background is low in at least one).
int whiteness(QRgb color);

// Clearly white artwork: opaque, bright and nearly grey. (Opacity matters: the
// transparent corners of a rounded icon often carry white color values.)
bool isWhiteArtwork(QRgb color);

// Fills every pixel of `image` whose entry in `known` (indexed y * width + x) is 0
// from its known neighbours, ring by ring, and marks it known. Used to rebuild the
// background behind artwork that has been cut out.
void fillUnknown(QImage &image, std::vector<char> &known);

// The artwork pixel as pure white (premultiplied), with an opacity of how much
// whiter `original` is than the rebuilt `background`; keeps soft edges.
QRgb whiteMatte(QRgb original, QRgb background);

// Converts between straight and premultiplied alpha (Qt 4 has no public
// qPremultiply() / qUnpremultiply()).
QRgb premultiply(QRgb color);
QRgb unpremultiply(QRgb color);

} // namespace imagelayers
