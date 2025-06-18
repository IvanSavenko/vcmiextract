#include "vcmiextract.h"

#include <array>
#include <vector>

void vcmiextract::extract_lod(memory_file& file, const std::filesystem::path& destination)
{
	struct archive_entry
	{
		std::array<char, 16> name;

		uint32_t offset = 0;
		uint32_t full_size = 0;
		uint32_t unused = 0;
		uint32_t compressed_size = 0;
	};

	file.set(8);

	uint32_t total_files = file.read<uint32_t>();

	file.set(0x5c);

	std::vector<archive_entry> entries;

	for (uint32_t i = 0; i < total_files; ++i)
	{
		archive_entry entry;

		file.read(entry.name.data(), entry.name.size());
		file.read(entry.offset);
		file.read(entry.full_size);
		file.read(entry.unused);
		file.read(entry.compressed_size);

		entries.push_back(entry);
	}

	for (auto const& entry : entries)
	{
		file.set(entry.offset);

		if (entry.compressed_size != 0)
		{
			memory_file compressed(file.ptr(), entry.compressed_size);
			memory_file file_data(entry.full_size);

			vcmiextract::decompress_file(compressed, file_data);
			vcmiextract::save_file(file_data, destination, entry.name.data());
		}
		else
		{
			memory_file file_data(file.ptr(), entry.full_size);
			vcmiextract::save_file(file_data, destination, entry.name.data());
		}
	}
}

void vcmiextract::extract_snd(memory_file& file, const std::filesystem::path& destination)
{
	struct archive_entry
	{
		std::array<char, 40> name;

		uint32_t offset = 0;
		uint32_t full_size = 0;
	};

	uint32_t total_files = file.read<uint32_t>();

	std::vector<archive_entry> entries;

	for (uint32_t i = 0; i < total_files; ++i)
	{
		archive_entry entry;

		file.read(entry.name.data(), entry.name.size());
		file.read(entry.offset);
		file.read(entry.full_size);

		entries.push_back(entry);
	}

	for (auto const& entry : entries)
	{
		file.set(entry.offset);
		memory_file file_data(file.ptr(), entry.full_size);
		vcmiextract::save_file(file_data, destination, std::string(entry.name.data()) + ".wav");
	}
}

void vcmiextract::extract_vid(memory_file& file, const std::filesystem::path& destination)
{
	struct archive_entry
	{
		std::array<char, 40> name;

		uint32_t begin = 0;
		uint32_t end = 0;
	};

	uint32_t total_files = file.read<uint32_t>();

	std::vector<archive_entry> entries;

	for (uint32_t i = 0; i < total_files; ++i)
	{
		archive_entry entry;

		file.read(entry.name.data(), entry.name.size());
		file.read(entry.begin);

		if (!entries.empty())
			entries.back().end = entry.begin;
		entries.push_back(entry);
	}

	if (!entries.empty())
		entries.back().end = file.size();

	for (auto const& entry : entries)
	{
		file.set(entry.begin);
		memory_file file_data(file.ptr(), entry.end - entry.begin);
		vcmiextract::save_file(file_data, destination, std::string(entry.name.data()));
	}
}
