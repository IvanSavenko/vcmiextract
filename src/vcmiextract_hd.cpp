#include "vcmiextract.h"

#include "file_format_dds.h"

#include <array>
#include <vector>
#include <string>

std::vector<std::string> split_string(const std::string & input, char separator)
{
	std::vector<std::string> result;

	if(input.empty())
		return result;

	size_t position = 0;
	while(position < input.size() - 1)
	{
		position += 1;
		size_t split_pos = input.find(separator, position);

		if(split_pos == std::string::npos)
			result.push_back(input.substr(position));
		else
			result.push_back(input.substr(position, split_pos - position));

		position = split_pos;
	}

	return result;
}

std::vector<std::vector<std::string>> string_to_table(const std::string & input)
{
	std::vector<std::vector<std::string>> result;

	auto lines = split_string(input, '\n');

	for(const auto & line : lines)
		result.push_back(split_string(line, ' '));

	return result;
}

void vcmiextract::extract_pak(memory_file & file, const std::filesystem::path & destination)
{
	struct image_entry
	{
		std::string name;
		int32_t sheetIndex = 0;
		int32_t spriteOffsetX = 0;
		int32_t unknown1 = 0;
		int32_t spriteOffsetY = 0;
		int32_t unknown2 = 0;
		int32_t sheetOffsetX = 0;
		int32_t sheetOffsetY = 0;
		int32_t width = 0;
		int32_t height = 0;
		int32_t rotation = 0;
		int32_t hasShadow = 0;
		int32_t shadowSheetIndex = 0;
		int32_t shadowSheetOffsetX = 0;
		int32_t shadowSheetOffsetY = 0;
		int32_t shadowWidth = 0;
		int32_t shadowHeight = 0;
		int32_t shadowRotation = 0;
	};

	struct sheet_entry
	{
		uint32_t compressed_size = 0;
		uint32_t full_size = 0;
	};

	struct archive_entry
	{
		std::array<char, 20> name{};

		uint32_t metadata_offset = 0;
		uint32_t metadata_size = 0;

		uint32_t count_sheets = 0;
		uint32_t compressed_size = 0;
		uint32_t full_size = 0;

		std::vector<sheet_entry> sheets;
		std::vector<image_entry> images;
	};

	std::vector<archive_entry> content;

	uint32_t magic = file.read<uint32_t>();
	uint32_t headerOffset = file.read<uint32_t>();

	assert(magic == 4);
	file.set(headerOffset);

	uint32_t entriesCount = file.read<uint32_t>();

	for(uint32_t i = 0; i < entriesCount; ++i)
	{
		archive_entry entry;

		file.read(entry.name.data(), entry.name.size());
		file.read(entry.metadata_offset);
		file.read(entry.metadata_size);

		file.read(entry.count_sheets);

		for(uint32_t j = 0; j < entry.count_sheets; ++j)
		{
			sheet_entry sheet;
			file.read(sheet.compressed_size);
			file.read(sheet.full_size);

			entry.sheets.push_back(sheet);
		}

		file.read(entry.compressed_size);
		file.read(entry.full_size);

		content.push_back(entry);
	}

	for(auto & entry : content)
	{
		std::string data;
		data.resize(entry.metadata_size);

		file.set(entry.metadata_offset);
		file.read(data.data(), data.size());

		auto table = string_to_table(data);

		image_entry image;
		for(const auto & line : table)
		{
			assert(line.size() == 12 || line.size() == 18);

			image.name = line[0];
			image.sheetIndex = std::stol(line[1]);
			image.spriteOffsetX = std::stol(line[2]);
			image.unknown1 = std::stol(line[3]);
			image.spriteOffsetY = std::stol(line[4]);
			image.unknown2 = std::stol(line[5]);
			image.sheetOffsetX = std::stol(line[6]);
			image.sheetOffsetY = std::stol(line[7]);
			image.width = std::stol(line[8]);
			image.height = std::stol(line[9]);
			image.rotation = std::stol(line[10]);
			image.hasShadow = std::stol(line[11]);

			if(image.hasShadow)
			{
				image.shadowSheetIndex = std::stol(line[12]);
				image.shadowSheetOffsetX = std::stol(line[13]);
				image.shadowSheetOffsetY = std::stol(line[14]);
				image.shadowWidth = std::stol(line[15]);
				image.shadowHeight = std::stol(line[16]);
				image.shadowRotation = std::stol(line[17]);
			}
			entry.images.push_back(image);
		}

		memory_file compressed(file.ptr(), entry.sheets[0].compressed_size);
		memory_file file_data(entry.sheets[0].full_size);

		vcmiextract::decompress_file(compressed, file_data);
		auto png_image = file_format_dds::load(file_data);
		save_image(png_image, destination, std::string(entry.name.data()) + ".png");
	}
}
