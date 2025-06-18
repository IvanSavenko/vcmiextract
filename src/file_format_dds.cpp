#include "file_format_dds.h"

#include <array>

enum dds_header_flags : uint32_t
{
	DDSD_CAPS        = 0x000001,
	DDSD_HEIGHT      = 0x000002,
	DDSD_WIDTH       = 0x000004,
	DDSD_PITCH       = 0x000008,
	DDSD_PIXELFORMAT = 0x001000,
	DDSD_MIPMAPCOUNT = 0x020000,
	DDSD_LINEARSIZE  = 0x080000,
	DDSD_DEPTH       = 0x800000,
};

enum dds_caps_flags : uint32_t
{
	DDSCAPS_COMPLEX = 0x000008, // Optional; must be used on any file that contains more than one surface
	DDSCAPS_MIPMAP  = 0x400000, // Optional; should be used for a mipmap.
	DDSCAPS_TEXTURE = 0x001000, // Required
};

enum dds_format_flags : uint32_t
{
	DDPF_ALPHAPIXELS = 0x0001,  // Texture contains alpha data; dwRGBAlphaBitMask contains valid data.
	DDPF_ALPHA       = 0x0002,  // Used in some older DDS files for alpha channel only uncompressed data (dwRGBBitCount contains the alpha channel bitcount; dwABitMask contains valid data)
	DDPF_FOURCC      = 0x0004,  // Texture contains compressed RGB data; dwFourCC contains valid data.
	DDPF_RGB         = 0x0040,  // Texture contains uncompressed RGB data; dwRGBBitCount and the RGB masks (dwRBitMask, dwGBitMask, dwBBitMask) contain valid data.
	DDPF_YUV         = 0x0200,  // Used in some older DDS files for YUV uncompressed data (dwRGBBitCount contains the YUV bit count; dwRBitMask contains the Y mask, dwGBitMask contains the U mask, dwBBitMask contains the V mask)
	DDPF_LUMINANCE   = 0x20000  // Used in some older DDS files for single channel color uncompressed data (dwRGBBitCount contains the luminance channel bit count; dwRBitMask contains the channel mask). Can be combined with DDPF_ALPHAPIXELS for a two channel DDS file.
};

enum dds_format_code : uint32_t
{
	DDS_FORMAT_DXT1 = 0x31545844,
	DDS_FORMAT_DXT2 = 0x32545844,
	DDS_FORMAT_DXT3 = 0x33545844,
	DDS_FORMAT_DXT4 = 0x34545844,
	DDS_FORMAT_DXT5 = 0x35545844,
	DDS_FORMAT_DX10 = 0x30315844,
};

struct dds_pixel_format
{
	uint32_t format_size;
	uint32_t flags;
	uint32_t format_code;
	uint32_t bits_count;
	uint32_t bitmask_r;
	uint32_t bitmask_g;
	uint32_t bitmask_b;
	uint32_t bitmask_a;
};

struct dds_header
{
	uint32_t header_size;
	uint32_t flags;
	uint32_t image_height;
	uint32_t image_width;
	uint32_t pitchOrLinearSize; // scanline for uncompressed formats, total size for compressed formats, if DDSD_LINEARSIZE is set
	uint32_t depth; // volume textures only, only if DDS_HEADER_FLAGS_VOLUME
	uint32_t mipMapCount; // total number of included mip maps, only if DDS_HEADER_FLAGS_MIPMAP
	uint32_t reserved1[11];
	dds_pixel_format pixel_format;
	uint32_t caps;
	uint32_t caps2;
	uint32_t caps3;
	uint32_t caps4;
	uint32_t reserved2;
};

struct color_dxt
{
	uint16_t r = 0;
	uint16_t g = 0;
	uint16_t b = 0;

	color_dxt() = default;

	explicit color_dxt(uint16_t r, uint16_t g, uint16_t b)
		: r(r)
		, g(g)
		, b(b)
	{
	}

	explicit color_dxt(uint16_t value)
		: r(((value) & 31) << 3)
		, g(((value >> 5) & 63) << 2)
		, b(((value >> 11)) << 3)
	{
		assert(r < 256);
		assert(g < 256);
		assert(b < 256);
	}

	color_dxt operator+(const color_dxt & other) const
	{
		return color_dxt(r + other.r, g + other.g, b + other.b);
	}

	color_dxt operator/(uint16_t other) const
	{
		return color_dxt(r / other, g / other, b / other);
	}

	color_dxt operator*(uint16_t other) const
	{
		return color_dxt(r * other, g * other, b * other);
	}
};

std::array<color_dxt, 4> load_dxt_color_block(memory_file & data)
{
	std::array<color_dxt, 4> colors;

	uint16_t color0 = data.read<uint16_t>();
	uint16_t color1 = data.read<uint16_t>();

	colors[0] = color_dxt(color0);
	colors[1] = color_dxt(color1);

	if(color0 > color1)
	{
		colors[2] = colors[0] * 2 / 3 + colors[1] / 3;
		colors[3] = colors[0] / 3 + colors[1] * 2 / 3;
	}
	else
	{
		colors[2] = colors[0] / 2 + colors[1] / 2;
		colors[3] = color_dxt(0); // black
	}
	return colors;
}

std::array<uint8_t, 8> load_dxt_alpha_block(memory_file & data)
{
	std::array<uint8_t, 8> alpha;

	alpha[0] = data.read<uint8_t>();
	alpha[1] = data.read<uint8_t>();

	if(alpha[0] > alpha[1])
	{
		alpha[2] = (alpha[0] * 6 + alpha[1] * 1) / 7;
		alpha[3] = (alpha[0] * 5 + alpha[1] * 2) / 7;
		alpha[4] = (alpha[0] * 4 + alpha[1] * 3) / 7;
		alpha[5] = (alpha[0] * 3 + alpha[1] * 4) / 7;
		alpha[6] = (alpha[0] * 2 + alpha[1] * 5) / 7;
		alpha[7] = (alpha[0] * 1 + alpha[1] * 6) / 7;
	}
	else
	{
		alpha[2] = (alpha[0] * 4 + alpha[1] * 1) / 5;
		alpha[3] = (alpha[0] * 3 + alpha[1] * 2) / 5;
		alpha[4] = (alpha[0] * 2 + alpha[1] * 3) / 5;
		alpha[5] = (alpha[0] * 1 + alpha[1] * 4) / 5;
		alpha[6] = 0;
		alpha[7] = 255;
	}
	return alpha;
}

static basic_image_ptr load_dxt1(const dds_header & header, memory_file & data)
{
	const uint32_t block_width = 4;
	const uint32_t block_height = 4;
	const uint32_t block_area = block_width * block_height;

	assert(header.image_height % block_area == 0);
	assert(header.image_width % block_area == 0);

	auto image = std::make_shared<basic_image>(header.image_height, header.image_width, header.image_width * 3, basic_image::image_format::rgb24);

	for(uint32_t by = 0; by < header.image_height; by += block_height)
	{
		for(uint32_t bx = 0; bx < header.image_width; bx += block_height)
		{
			std::array<color_dxt, 4> colors = load_dxt_color_block(data);
			uint32_t lookup_table = data.read<uint32_t>();

			for(uint32_t y = 0; y < block_height; ++y)
			{
				for(uint32_t x = 0; x < block_width; ++x)
				{
					int offset = (y * block_width + x) * 2;
					int index = (lookup_table >> offset) & 0x3;
					color_dxt c = colors[index];

					auto dst = image->rgb(bx + x, by + y);
					dst.set_red(c.r);
					dst.set_green(c.g);
					dst.set_blue(c.b);
				}
			}
		}
	}
	return image;
}

static basic_image_ptr load_dxt5(const dds_header & header, memory_file & data)
{
	const uint32_t block_width = 4;
	const uint32_t block_height = 4;
	const uint32_t block_area = block_width * block_height;

	assert(header.image_height % block_area == 0);
	assert(header.image_width % block_area == 0);

	auto image = std::make_shared<basic_image>(header.image_height, header.image_width, header.image_width * 4, basic_image::image_format::rgba32);

	for(uint32_t by = 0; by < header.image_height; by += block_height)
	{
		for(uint32_t bx = 0; bx < header.image_width; bx += block_height)
		{
			std::array<uint8_t, 8> alphas = load_dxt_alpha_block(data);
			uint64_t lookup_table_a = data.read<uint32_t>() + (uint64_t(data.read<uint16_t>()) << 32);

			std::array<color_dxt, 4> colors = load_dxt_color_block(data);
			uint32_t lookup_table_c = data.read<uint32_t>();

			for(uint32_t y = 0; y < block_height; ++y)
			{
				for(uint32_t x = 0; x < block_width; ++x)
				{
					int offset = (y * block_width + x);
					int offset_c = offset * 2;
					int offset_a = offset * 3;
					int index_c = (lookup_table_c >> offset_c) & 0x3;
					int index_a = (lookup_table_a >> offset_a) & 0x7;
					color_dxt c = colors[index_c];
					uint8_t a = alphas[index_a];

					auto dst = image->rgba(bx + x, by + y);
					dst.set_red(c.r);
					dst.set_green(c.g);
					dst.set_blue(c.b);
					dst.set_alpha(a);
				}
			}
		}
	}
	return image;
}

basic_image_ptr file_format_dds::load(memory_file & data)
{
	uint32_t magic;
	dds_header header;

	data.read(magic);
	data.read(header);

	// Some of these assertions are strictly speaking invalid - as in, such dds would still be legal
	// However they don't occur in HD Edition
	// Similarly, only DXT1 and DXT5 compressions are supported, since nothing else is needed for HD Edition

	assert(magic == 0x20534444);
	assert(header.header_size == 124);
	assert(header.flags == (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT | DDSD_LINEARSIZE));
	assert(header.depth == 0);
	assert(header.mipMapCount == 1);
	assert(header.caps == (DDSCAPS_COMPLEX | DDSCAPS_MIPMAP | DDSCAPS_TEXTURE));
	assert(header.caps2 == 0);
	assert(header.caps3 == 0);
	assert(header.caps4 == 0);
	assert(header.reserved2 == 0);

	assert(header.pixel_format.format_size == 32);
	assert(header.pixel_format.flags == (DDPF_ALPHAPIXELS | DDPF_FOURCC));
	assert(header.pixel_format.format_code == DDS_FORMAT_DXT5 || header.pixel_format.format_code == DDS_FORMAT_DXT1);
	assert(header.pixel_format.bits_count == 0);
	assert(header.pixel_format.bitmask_r == 0);
	assert(header.pixel_format.bitmask_g == 0);
	assert(header.pixel_format.bitmask_b == 0);
	assert(header.pixel_format.bitmask_a == 0);

	if(header.pixel_format.format_code == DDS_FORMAT_DXT5)
	{
		assert(header.pitchOrLinearSize == header.image_width * header.image_height);
		return load_dxt5(header, data);
	}

	if(header.pixel_format.format_code == DDS_FORMAT_DXT1)
	{
		assert(header.pitchOrLinearSize * 2 == header.image_width * header.image_height);
		return load_dxt1(header, data);
	}

	return nullptr;
}
