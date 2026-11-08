#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

// A simple RGBA image for tTime's pixel work (icon recoloring, bell and tree
// animation). Pixels are row-major; whether they are premultiplied is up to the user
// (documented where it matters).
struct Rgba
{
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 0;
    friend bool operator==(const Rgba &, const Rgba &) = default;
};

struct Image
{
    int width = 0;
    int height = 0;
    std::vector<Rgba> pixels;

    Image() = default;
    Image(int w, int h)
        : width(w)
        , height(h)
        , pixels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h))
    {
    }

    [[nodiscard]] bool isNull() const noexcept { return width <= 0 || height <= 0; }
    [[nodiscard]] Rgba &at(int x, int y) { return pixels[static_cast<std::size_t>(y) * width + x]; }
    [[nodiscard]] const Rgba &at(int x, int y) const { return pixels[static_cast<std::size_t>(y) * width + x]; }
    friend bool operator==(const Image &, const Image &) = default;
};

[[nodiscard]] inline Rgba premultiplied(Rgba c)
{
    return {static_cast<std::uint8_t>((c.r * c.a + 127) / 255), static_cast<std::uint8_t>((c.g * c.a + 127) / 255),
            static_cast<std::uint8_t>((c.b * c.a + 127) / 255), c.a};
}

[[nodiscard]] inline Rgba unpremultiplied(Rgba c)
{
    if (c.a == 0) {
        return {0, 0, 0, 0};
    }
    const auto channel = [&](std::uint8_t v) { return static_cast<std::uint8_t>(std::min(255, (v * 255 + c.a / 2) / c.a)); };
    return {channel(c.r), channel(c.g), channel(c.b), c.a};
}

[[nodiscard]] inline Image premultiplied(Image image)
{
    for (Rgba &p : image.pixels) {
        p = premultiplied(p);
    }
    return image;
}

[[nodiscard]] inline Image unpremultiplied(Image image)
{
    for (Rgba &p : image.pixels) {
        p = unpremultiplied(p);
    }
    return image;
}
