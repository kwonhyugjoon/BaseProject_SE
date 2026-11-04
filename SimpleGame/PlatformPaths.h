#pragma once
#include <filesystem>

std::filesystem::path ExecutableDirectory();
std::filesystem::path ResourcePath(const char* relative);
std::filesystem::path CacheDirectory();
