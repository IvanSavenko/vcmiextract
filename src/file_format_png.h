#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <filesystem>

struct image_pixel_indexed
{
	uint8_t* ptr;

	void set_value(uint8_t value) { ptr[0] = value; }
	uint8_t const& value() const { return ptr[0]; }
};

struct image_pixel_gray
{
	uint8_t * ptr;

	void set_gray(uint8_t value) { ptr[0] = value; }
	uint8_t const & gray() const { return ptr[0]; }
};

struct image_pixel_rgb
{
	uint8_t * ptr;

	void set_red  (uint8_t value) { ptr[0] = value; }
	void set_green(uint8_t value) { ptr[1] = value; }
	void set_blue (uint8_t value) { ptr[2] = value; }

	uint8_t const & red  () const { return ptr[0]; }
	uint8_t const & green() const { return ptr[1]; }
	uint8_t const & blue () const { return ptr[2]; }

};

struct image_pixel_rgba
{
	uint8_t * ptr;

	void set_red  (uint8_t value) { ptr[0] = value; }
	void set_green(uint8_t value) { ptr[1] = value; }
	void set_blue (uint8_t value) { ptr[2] = value; }
	void set_alpha(uint8_t value) { ptr[3] = value; }

	uint8_t const & red  () const { return ptr[0]; }
	uint8_t const & green() const { return ptr[1]; }
	uint8_t const & blue () const { return ptr[2]; }
	uint8_t const & alpha() const { return ptr[3]; }

};

struct basic_image
{
	enum class image_format : uint8_t
	{
		p8,
		g8,
		rgb24,
		rgba32,
		invalid,
	};

	std::unique_ptr<uint8_t[]> palette;
	std::unique_ptr<uint8_t[]> pixels;
	uint32_t scanline;
	uint32_t height;
	uint32_t width;
	image_format format;
	uint8_t bytes_per_pixel;

	image_pixel_rgb color(uint32_t index)
	{
		assert(format == image_format::p8);
		assert(index < 256);
		return {palette.get() + 3 * index};
	}

	image_pixel_indexed indexed(uint32_t col, uint32_t row)
	{
		assert(format == image_format::p8);
		assert(row < height);
		assert(col < width);
		return {pixels.get() + scanline * size_t(row) + bytes_per_pixel * size_t(col)};
	}

	image_pixel_gray gray(uint32_t col, uint32_t row)
	{
		assert(format == image_format::g8);
		assert(row < height);
		assert(col < width);
		return {pixels.get() + scanline * size_t(row) + bytes_per_pixel * size_t(col)};
	}

	image_pixel_rgb rgb(uint32_t col, uint32_t row)
	{
		assert(format == image_format::rgb24);
		assert(row < height);
		assert(col < width);
		return {pixels.get() + scanline * size_t(row) + bytes_per_pixel * size_t(col)};
	}

	image_pixel_rgba rgba(uint32_t col, uint32_t row)
	{
		assert(format == image_format::rgba32);
		assert(row < height);
		assert(col < width);
		return {pixels.get() + scanline * size_t(row) + bytes_per_pixel * size_t(col)};
	}

	basic_image(uint32_t height, uint32_t width, uint32_t scanline, image_format format);
};

using basic_image_ptr = std::shared_ptr<basic_image>;

namespace file_format_png
{
	basic_image_ptr optimize_try_drop_alpha(basic_image_ptr const& image);
	void optimize_and_save(basic_image_ptr const& image, std::filesystem::path const& filename);
	void save_image( basic_image_ptr const& image, std::filesystem::path const & filename );
}
