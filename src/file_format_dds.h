#pragma once

#include "file_format_png.h"
#include "memory_file.h"

namespace file_format_dds
{
    basic_image_ptr load(memory_file & data);
}
