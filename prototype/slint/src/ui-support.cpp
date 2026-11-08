#include "ui-support.h"

Image fromSlint(const slint::Image &image)
{
    const auto buffer = image.to_rgba8();
    if (!buffer) {
        return {};
    }
    Image result(static_cast<int>(buffer->width()), static_cast<int>(buffer->height()));
    std::size_t i = 0;
    for (const slint::Rgba8Pixel &p : *buffer) {
        result.pixels[i++] = {p.r, p.g, p.b, p.a};
    }
    return result;
}

slint::Image toSlint(const Image &image)
{
    if (image.isNull()) {
        return {};
    }
    slint::SharedPixelBuffer<slint::Rgba8Pixel> buffer(static_cast<uint32_t>(image.width),
                                                       static_cast<uint32_t>(image.height));
    auto *out = buffer.begin();
    for (const Rgba &p : image.pixels) {
        *out++ = {p.r, p.g, p.b, p.a};
    }
    return slint::Image(buffer);
}

slint::Color toSlint(Color color)
{
    return slint::Color::from_rgb_uint8(color.r, color.g, color.b);
}
