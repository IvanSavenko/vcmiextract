cmake_minimum_required(VERSION 3.15)

project (vcmiextract)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

if (MSVC)
	add_compile_options(/W4)
else()
	add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-error=unused-variable)
endif()

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

set_property(TARGET vcmiextract PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(vcmiextract 
	PRIVATE
		ZLIB::ZLIB
		PNG::PNG
)

install(TARGETS vcmiextract RUNTIME DESTINATION .)

set(CPACK_PACKAGE_NAME "vcmiextract")
set(CPACK_PACKAGE_VERSION "1.0")
include(CPack)
