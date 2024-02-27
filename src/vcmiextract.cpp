#include <array>
#include <string>
#include <map>

#include "memory_file.h"
#include "file_format_png.h"
#include "vcmiextract.h"

static bool string_iequals(const std::string& a, const std::string& b)
{
	return std::equal(a.begin(), a.end(), b.begin(), b.end(),
		[](char a, char b) {
			return tolower(a) == tolower(b);
		});
}

void vcmiextract::save_image(basic_image_ptr data, const std::filesystem::path& destination, std::string filename)
{
	std::filesystem::create_directories(destination);

	std::filesystem::path output_name = destination / filename;

	file_format_png::optimize_and_save(data, output_name.replace_extension(".png"));
}

void vcmiextract::save_file(memory_file& data, const std::filesystem::path& destination, std::string filename)
{
	std::filesystem::create_directories(destination);

	std::filesystem::path filename_path(filename);
	std::string extension = filename_path.extension().string();

	auto full_path = destination / filename;

	//	if (string_iequals(extension, ".def") || string_iequals(extension, ".d32"))
	//	{
	//		std::filesystem::path target = full_path;
	//		
	//		target.replace_extension();
	//
	//		file_format_def::extract(data, target);
	//		//return; // keep .def
	//	}

	if (string_iequals(extension, ".pcx") || string_iequals(extension, ".p32"))
	{
		basic_image_ptr image = vcmiextract::load_image_pcx(data);

		filename_path.replace_extension(".png");

		if (image)
		{
			save_image(image, destination, filename_path.string());
			return;
		}
	}

	FILE* fp = fopen(full_path.string().c_str(), "wb");
	assert(fp);

	data.set(0);

	fwrite(data.ptr(), 1, data.size(), fp);
	fclose(fp);
}

void vcmiextract::extract_file(const std::filesystem::path& source, const std::filesystem::path& destination)
{
	std::string extension = source.extension().string();

	memory_file file(source);

	if (string_iequals(extension, ".lod"))
	{
		vcmiextract::extract_lod(file, destination);
		return;
	}

	if (string_iequals(extension, ".snd"))
	{
		vcmiextract::extract_snd(file, destination);
		return;
	}

	if (string_iequals(extension, ".vid"))
	{
		vcmiextract::extract_vid(file, destination);
		return;
	}

	if (string_iequals(extension, ".def") || string_iequals(extension, ".d32"))
	{
		vcmiextract::extract_def(file, destination);
		return;
	}

	printf("unrecognized file type '%s'\n", source.string().c_str());
}

static void process(std::string filename)
{
	std::filesystem::path source_file = std::filesystem::absolute(filename);
	std::filesystem::path target_dir = source_file;

	target_dir.remove_filename();
	target_dir = target_dir / source_file.stem();

	if (!std::filesystem::is_regular_file(source_file))
	{
		printf("file '%s' not found!\n", filename.c_str());
		return;
	}

	if (std::filesystem::is_regular_file(target_dir))
	{
		printf("output path for '%s' is not a directory!\n", filename.c_str());
		return;
	}

	vcmiextract::extract_file(source_file, target_dir);
}

int main(int argc, char** argv)
{
	//process("S:/Games/Heroes III HotA/Data/HotA/artifbon.def");
	//process("S:/src/vcmiextract/test/HotA.lod");
	//process("S:/src/vcmiextract/test/HotA_lng.lod");
	//process("S:/src/vcmiextract/test/HotA.snd");

	for (int i = 1; i < argc; ++i)
		process(argv[i]);

	return 0;
}
