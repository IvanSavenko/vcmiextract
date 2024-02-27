cmake_minimum_required(VERSION 3.10)

project (vcmiextract)

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

set(extract_SRCS
	src/file_format_png.cpp
	src/file_format_png.h
	src/memory_file.h
	src/vcmiextract.cpp
	src/vcmiextract.h
	src/vcmiextract_archive.cpp
	src/vcmiextract_image.cpp
	src/vcmiextract_zlib.cpp
)

add_executable(vcmiextract ${extract_SRCS})

target_link_libraries(vcmiextract 
	PRIVATE
		ZLIB::ZLIB
		${PNG_LIBRARY}
)

