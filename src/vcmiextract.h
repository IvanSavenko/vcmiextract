#pragma once

#include "file_format_png.h"
#include "memory_file.h"

namespace vcmiextract
{
	basic_image_ptr load_image_pcx(memory_file& input);

	void extract_pak(memory_file& source, const std::filesystem::path& destination);
	void extract_lod(memory_file& source, const std::filesystem::path& destination);
	void extract_snd(memory_file& source, const std::filesystem::path& destination);
	void extract_vid(memory_file& source, const std::filesystem::path& destination);
	void extract_def(memory_file& source, const std::filesystem::path& destination);

	void decompress_file(memory_file& source, memory_file& target);

	void save_image(const basic_image_ptr & data, const std::filesystem::path& destination, const std::string & filename);
	void save_file(memory_file& data, const std::filesystem::path& destination, const std::string & filename);

	void extract_file(const std::filesystem::path& source, const std::filesystem::path& destination);
}
