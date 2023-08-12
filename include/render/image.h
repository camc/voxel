#pragma once

#include <vector>
#include <string>

namespace render {

std::vector<char> load_png_rgb8(const std::string& path, unsigned int& width, unsigned int& height);

}