#include "file_format_png.h"

#include <vector>
#include <array>
#if __has_include(<libpng16/png.h>)
#include <libpng16/png.h>
#else
#include <libpng/png.h>
#endif

static uint8_t get_bytes_per_pixel_by_format(basic_image::image_format format)
{
	switch(format)
	{
		case basic_image::image_format::p8:
			return 1;
		case basic_image::image_format::g8:
			return 1;
		case basic_image::image_format::rgb24:
			return 3;
		case basic_image::image_format::rgba32:
			return 4;
		default:
			assert(0);
			return 0;
	}
}

basic_image::basic_image(uint32_t height, uint32_t width, uint32_t scanline, image_format format)
	: pixels(new uint8_t[static_cast<size_t>(scanline) * height])
	, scanline(scanline)
	, height(height)
	, width(width)
	, format(format)
	, bytes_per_pixel(get_bytes_per_pixel_by_format(format))
{
	assert(width > 0);
	assert(height > 0);
	assert(scanline > 0);
	assert(scanline >= width * bytes_per_pixel);

	if(format == image_format::p8)
		palette.reset(new uint8_t[256 * 3]);

	std::fill_n(pixels.get(), scanline * height, uint8_t(0));
}

static uint8_t get_png_color_type_from_format(basic_image::image_format format)
{
	switch(format)
	{
		case basic_image::image_format::p8:
			return PNG_COLOR_TYPE_PALETTE;

		case basic_image::image_format::g8:
			return PNG_COLOR_TYPE_GRAY;

		case basic_image::image_format::rgb24:
			return PNG_COLOR_TYPE_RGB;

		case basic_image::image_format::rgba32:
			return PNG_COLOR_TYPE_RGB_ALPHA;

		default:
			break;
	}
	return 0;
}

basic_image_ptr file_format_png::optimize_try_drop_alpha(const basic_image_ptr & image)
{
	if(image->format != basic_image::image_format::rgba32)
		return image;

	for(uint32_t y = 0; y < image->height; y++)
		for(uint32_t x = 0; x < image->width; x++)
			if(image->rgba(x, y).alpha() != 0xff)
				return image;

	auto temp_image = std::make_shared<basic_image>(image->height, image->width, image->width * 3, basic_image::image_format::rgb24);

	for(uint32_t y = 0; y < image->height; y++)
	{
		for(uint32_t x = 0; x < image->width; x++)
		{
			auto dst = temp_image->rgb(x, y);
			auto src = image->rgba(x, y);

			dst.set_red(src.red());
			dst.set_green(src.green());
			dst.set_blue(src.blue());
		}
	}
	return temp_image;
}

void file_format_png::optimize_and_save(const basic_image_ptr & image, const std::filesystem::path & filename)
{
	save_image(optimize_try_drop_alpha(image), filename);
}

void file_format_png::save_image(const basic_image_ptr & image, const std::filesystem::path & filename)
{
	FILE * fp = fopen(filename.string().c_str(), "wb");
	assert(fp);

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	assert(png);

	png_infop info = png_create_info_struct(png);
	assert(info);

	png_init_io(png, fp);

	png_set_IHDR(
		png,
		info,
		image->width,
		image->height,
		8,
		get_png_color_type_from_format(image->format),
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);

	png_set_compression_level(png, 9);

	if(image->palette)
	{
		std::array<png_color, 256> palette;

		for(int i = 0; i < 256; i++)
		{
			palette[i].red = image->color(i).red();
			palette[i].green = image->color(i).green();
			palette[i].blue = image->color(i).blue();
		}

		png_set_PLTE(png, info, palette.data(), 256);
	}

	std::vector<uint8_t *> row_pointers(image->height);
	for(uint32_t y = 0; y < image->height; y++)
		row_pointers[y] = image->pixels.get() + static_cast<size_t>(image->scanline) * y;

	png_set_rows(png, info, row_pointers.data());
	png_write_png(png, info, PNG_TRANSFORM_BGR, nullptr);

	png_destroy_write_struct(&png, &info);
	fclose(fp);
}
