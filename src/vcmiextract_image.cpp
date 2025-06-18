#include "vcmiextract.h"

#include <array>
#include <map>
#include <vector>

basic_image_ptr vcmiextract::load_image_pcx(memory_file & input)
{
	if(input.peek<uint32_t>() == 0x46323350) //P32F
	{
		input.set(0);

		uint32_t magic = input.read<uint32_t>();
		uint32_t unknown1 = input.read<uint32_t>();
		uint32_t bits_per_pixel = input.read<uint32_t>();
		uint32_t size_raw = input.read<uint32_t>();
		uint32_t size_header = input.read<uint32_t>();
		uint32_t size_data = input.read<uint32_t>();
		uint32_t width = input.read<uint32_t>();
		uint32_t height = input.read<uint32_t>();
		uint32_t unknown8 = input.read<uint32_t>();
		uint32_t unknown9 = input.read<uint32_t>();

		assert(magic == 0x46323350);
		assert(size_header == 40);
		assert(size_raw == size_header + size_data);
		assert(size_data == width * height * bits_per_pixel / 8);
		assert(bits_per_pixel == 32);
		assert(unknown1 == 0);
		assert(unknown8 == 8);
		assert(unknown9 == 0);

		auto img = std::make_shared<basic_image>(height, width, width * 4, basic_image::image_format::rgba32);

		for(uint32_t y = 0; y < height; ++y)
			input.read(img->rgba(0, height - y - 1).ptr, width * 4);
		return img;
	}
	else // h3 pcx
	{
		uint32_t size = input.read<uint32_t>();
		uint32_t width = input.read<uint32_t>();
		uint32_t height = input.read<uint32_t>();

		if(size == width * height)
		{
			auto img = std::make_shared<basic_image>(height, width, width, basic_image::image_format::p8);

			input.read(img->pixels.get(), height * width);
			input.read(img->palette.get(), 256 * 3);

			return img;
		}

		if(size == width * height * 3)
		{
			auto img = std::make_shared<basic_image>(height, width, width * 3, basic_image::image_format::rgb24);

			input.read(img->pixels.get(), height * width * 3);

			return img;
		}
	}

	assert(0);
	return basic_image_ptr();
}

struct image_entry_def
{
	uint32_t size = 0;
	uint32_t format = 0;
	uint32_t full_width = 0;
	uint32_t full_height = 0;
	uint32_t stored_width = 0;
	uint32_t stored_height = 0;
	uint32_t margin_left = 0;
	uint32_t margin_top = 0;
};

static basic_image_ptr load_image_def(memory_file & file, const image_entry_def & entry, const std::array<uint8_t, 256 * 3> & palette)
{
	auto image = std::make_shared<basic_image>(entry.full_height, entry.full_width, entry.full_width, basic_image::image_format::p8);

	std::copy(palette.begin(), palette.end(), image->palette.get());

	uint32_t start_x = entry.margin_left;
	uint32_t start_y = entry.margin_top;

	size_t offset = file.tell();

	switch(entry.format)
	{
		case 0:
		{
			for(uint32_t y = 0; y < entry.stored_height; ++y)
			{
				file.read(image->indexed(start_x, start_y + y).ptr, entry.stored_width);
			}
			break;
		}
		case 1:
		{
			std::vector<uint32_t> pixel_data_offset(entry.stored_height);

			file.read(pixel_data_offset.data(), pixel_data_offset.size());

			for(uint32_t y = 0; y < entry.stored_height; ++y)
			{
				file.set(offset + pixel_data_offset[y]);

				for(uint32_t x = 0; x < entry.stored_width;)
				{
					uint8_t segment_type = file.read<uint8_t>();
					uint32_t segment_length = file.read<uint8_t>() + 1;

					if(segment_type == 0xff)
					{
						file.read(image->indexed(start_x + x, start_y + y).ptr, segment_length);
					}
					else
					{
						std::fill_n(image->indexed(start_x + x, start_y + y).ptr, segment_length, segment_type);
					}
					x += segment_length;
				}
			}
			break;
		}
		case 2:
		{
			uint16_t pixel_data_offset = file.read<uint16_t>();

			file.set(offset + pixel_data_offset);

			for(uint32_t y = 0; y < entry.stored_height; ++y)
			{
				for(uint32_t x = 0; x < entry.stored_width;)
				{
					uint8_t segment_value = file.read<uint8_t>();
					uint8_t segment_type = segment_value / 32;
					uint8_t segment_length = (segment_value & 31) + 1;

					if(segment_type == 7)
					{
						file.read(image->indexed(start_x + x, start_y + y).ptr, segment_length);
					}
					else
					{
						std::fill_n(image->indexed(start_x + x, start_y + y).ptr, segment_length, segment_type);
					}
					x += segment_length;
				}
			}
			break;
		}
		case 3:
		{
			for(uint32_t y = 0; y < entry.stored_height; ++y)
			{
				file.set(offset + y * 2 * (entry.stored_width / 32));

				uint16_t pixel_data_offset = file.read<uint16_t>();

				file.set(offset + pixel_data_offset);

				for(uint32_t x = 0; x < entry.stored_width;)
				{
					uint8_t segment_value = file.read<uint8_t>();
					uint8_t segment_type = segment_value / 32;
					uint8_t segment_length = (segment_value & 31) + 1;

					if(segment_type == 7)
					{
						file.read(image->indexed(start_x + x, start_y + y).ptr, segment_length);
					}
					else
					{
						std::fill_n(image->indexed(start_x + x, start_y + y).ptr, segment_length, segment_type);
					}
					x += segment_length;
				}
			}
			break;
		}
		default:
			assert(0);
	}

	return image;
}

static void extract_def_h3(memory_file & file, const std::filesystem::path & destination)
{
	[[maybe_unused]] uint32_t type = file.read<uint32_t>();
	[[maybe_unused]] uint32_t width = file.read<uint32_t>();
	[[maybe_unused]] uint32_t height = file.read<uint32_t>();
	[[maybe_unused]] uint32_t total_groups = file.read<uint32_t>();

	std::array<uint8_t, 256 * 3> palette;

	for(auto & entry : palette)
		file.read(entry);

	struct archive_entry
	{
		std::array<char, 13> name{};
		uint32_t offset = 0;
	};

	struct archive_block_entry
	{
		std::vector<archive_entry> entries;

		uint32_t index = 0;
		uint32_t size = 0;
		uint32_t unknown1 = 0;
		uint32_t unknown2 = 0;
	};

	std::map<uint32_t, archive_block_entry> groups;

	for(uint32_t i = 0; i < total_groups; ++i)
	{
		archive_block_entry group;

		file.read(group.index);
		file.read(group.size);
		file.read(group.unknown1);
		file.read(group.unknown2);

		group.entries.resize(group.size);

		for(uint32_t j = 0; j < group.size; ++j)
			file.read(group.entries[j].name.data(), group.entries[j].name.size());

		for(uint32_t j = 0; j < group.size; ++j)
			file.read(group.entries[j].offset);

		assert(groups.count(group.index) == 0);
		groups[group.index] = group;
	}

	std::string file_listing;
	file_listing += "{\n";
	file_listing += "\t\"images\" : [\n";

	for(const auto & group : groups)
	{
		for (size_t i = 0; i < group.second.entries.size(); ++i)
		{
			const auto & entry = group.second.entries[i];

			image_entry_def header;

			file.set(entry.offset);

			file.read(header.size);
			file.read(header.format);
			file.read(header.full_width);
			file.read(header.full_height);

			file.read(header.stored_width);
			file.read(header.stored_height);
			file.read(header.margin_left);
			file.read(header.margin_top);

			// special case for some "old" format defs (SGTWMTA.DEF and SGTWMTB.DEF)
			if(header.format == 1 && header.stored_width > header.full_width && header.stored_height > header.full_height)
			{
				header.stored_height = header.full_height;
				header.stored_width = header.full_width;
				header.margin_left = 0;
				header.margin_top = 0;

				file.set(file.tell() - 16);
			}

			basic_image_ptr image = load_image_def(file, header, palette);

			file_listing += "\t\t{ ";
			if (groups.size() > 1)
			{
				file_listing += "\"group\" : ";
				file_listing += std::to_string(group.second.index);
				file_listing += ", ";
			}

			file_listing += "\"frame\" : ";
			file_listing += std::to_string(i);

			file_listing += ", \"file\" : \"";
			file_listing += std::filesystem::path(entry.name.data()).replace_extension(".png").string();
			file_listing += "\" },\n";

			vcmiextract::save_image(image, destination, entry.name.data());
		}
	}

	file_listing.pop_back();
	file_listing.pop_back();
	file_listing += "\n\t]\n}\n";

	memory_file listing_file(reinterpret_cast<uint8_t *>(file_listing.data()), file_listing.size());

	vcmiextract::save_file(listing_file, destination, "animation.json");
}

static void extract_def_d32f(memory_file & file, const std::filesystem::path & destination)
{
	uint32_t magic = file.read<uint32_t>();
	uint32_t unknown1 = file.read<uint32_t>();
	uint32_t unknown2 = file.read<uint32_t>();
	[[maybe_unused]] uint32_t width = file.read<uint32_t>();
	[[maybe_unused]] uint32_t height = file.read<uint32_t>();
	uint32_t total_groups = file.read<uint32_t>();
	uint32_t unknown6 = file.read<uint32_t>();
	uint32_t unknown7 = file.read<uint32_t>();

	assert(magic == 0x46323344);
	assert(unknown1 == 1);
	assert(unknown2 == 24);
	assert(unknown6 == 8);
	assert(unknown7 == 1 || unknown7 == 22); // groups?

	struct archive_entry
	{
		std::array<char, 13> name{};
		uint32_t offset = 0;
	};

	struct archive_block_entry
	{
		std::vector<archive_entry> entries;

		uint32_t header_size = 0;
		uint32_t index = 0;
		uint32_t size = 0;
		uint32_t unknown2 = 0;
	};

	std::map<uint32_t, archive_block_entry> groups;

	for(uint32_t i = 0; i < total_groups; ++i)
	{
		archive_block_entry group;

		file.read(group.header_size);
		file.read(group.index);
		file.read(group.size);
		file.read(group.unknown2);
		assert(group.header_size == 17 * group.size + 16);

		group.entries.resize(group.size);

		for(uint32_t j = 0; j < group.size; ++j)
			file.read(group.entries[j].name.data(), group.entries[j].name.size());

		for(uint32_t j = 0; j < group.size; ++j)
			file.read(group.entries[j].offset);

		assert(groups.count(group.index) == 0);
		groups[group.index] = group;
	}

	std::string file_listing;
	file_listing += "{\n";
	file_listing += "\t\"images\" : [\n";

	for(const auto & group : groups)
	{
		for (size_t i = 0; i < group.second.entries.size(); ++i)
		{
			const auto & entry = group.second.entries[i];
			file.set(entry.offset);

			uint32_t bits_per_pixel = file.read<uint32_t>();
			uint32_t image_size = file.read<uint32_t>();
			uint32_t full_width = file.read<uint32_t>();
			uint32_t full_height = file.read<uint32_t>();
			uint32_t stored_width = file.read<uint32_t>();
			uint32_t stored_height = file.read<uint32_t>();
			uint32_t margin_left = file.read<uint32_t>();
			uint32_t margin_top = file.read<uint32_t>();
			uint32_t entry_unknown1 = file.read<uint32_t>();
			uint32_t entry_unknown2 = file.read<uint32_t>();

			assert(stored_width <= full_width);
			assert(stored_height <= full_height);
			assert(entry_unknown1 == 8);
			assert(entry_unknown2 == 0 || entry_unknown2 == 1);
			assert(bits_per_pixel == 32);
			assert(image_size == stored_width * stored_height * 4);

			auto image = std::make_shared<basic_image>(full_height, full_width, full_width * 4, basic_image::image_format::rgba32);

			for(uint32_t y = 0; y < stored_height; ++y)
			{
				file.read(image->rgba(margin_left, margin_top + stored_height - y - 1).ptr, stored_width * 4);
			}

			file_listing += "\t\t{ ";
			if (groups.size() > 1)
			{
				file_listing += "\"group\" : ";
				file_listing += std::to_string(group.second.index);
				file_listing += ", ";
			}

			file_listing += "\"frame\" : ";
			file_listing += std::to_string(i);

			file_listing += ", \"file\" : \"";
			file_listing += std::filesystem::path(entry.name.data()).replace_extension(".png").string();
			file_listing += "\" },\n";

			vcmiextract::save_image(image, destination, entry.name.data());
		}
	}

	assert(file.eof());

	file_listing.pop_back();
	file_listing.pop_back();
	file_listing += "\n\t]\n}\n";

	memory_file listing_file(reinterpret_cast<uint8_t *>(file_listing.data()), file_listing.size());

	vcmiextract::save_file(listing_file, destination, "animation.json");
}

void vcmiextract::extract_def(memory_file & file, const std::filesystem::path & destination)
{
	if(file.peek<uint32_t>() == 0x46323344) // D32F
		extract_def_d32f(file, destination);
	else
		extract_def_h3(file, destination);
}
