#include "stdafx.h"
#include "AssetCache.h"
#include "PlatformPaths.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <fstream>
#include <string>
#include <iostream>
#include <utility>

namespace
{
	std::filesystem::path CachePath(const wchar_t* name)
	{
		auto directory = CacheDirectory();
		return directory.empty() ? std::filesystem::path() : directory / name;
	}

	uint32_t Checksum(const std::vector<unsigned char>& data)
	{
		uint32_t value = 2166136261u;
		for (unsigned char byte : data)
			value = (value ^ byte) * 16777619u;
		return value;
	}
}

bool LoadAssetCache(const wchar_t* name,
                    uint32_t version,
                    size_t maximumBytes,
                    std::vector<unsigned char>& data)
{
	auto path = CachePath(name);
	if (path.empty())
		return false;
	std::ifstream file(path, std::ios::binary);
	uint32_t header[4] = {};
	if (!file.read(reinterpret_cast<char*>(header), sizeof(header)) || header[0] != 0x31434752 ||
	    header[1] != version || header[2] > maximumBytes)
		return false;
	std::vector<unsigned char> loaded(header[2]);
	if (!file.read(reinterpret_cast<char*>(loaded.data()), loaded.size()) ||
	    file.peek() != std::char_traits<char>::eof() || Checksum(loaded) != header[3])
		return false;
	data = std::move(loaded);
	return true;
}

bool SaveAssetCache(const wchar_t* name, uint32_t version, const std::vector<unsigned char>& data)
{
	auto path = CachePath(name);
	if (path.empty())
		return false;
	auto temporary = path;
#ifdef _WIN32
	temporary += "." + std::to_string(GetCurrentProcessId()) + ".tmp";
#else
	temporary += "." + std::to_string(getpid()) + ".tmp";
#endif
	uint32_t header[] = {0x31434752, version, static_cast<uint32_t>(data.size()), Checksum(data)};
	std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
	file.write(reinterpret_cast<const char*>(header), sizeof(header));
	file.write(reinterpret_cast<const char*>(data.data()), data.size());
	file.close();
	bool replaced = false;
	if (file)
	{
#ifdef _WIN32
		replaced = MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
		std::error_code error;
		std::filesystem::rename(temporary, path, error);
		replaced = !error;
#endif
	}
	if (!replaced)
	{
		std::error_code error;
		std::filesystem::remove(temporary, error);
		std::cerr << "Asset cache could not be saved; using generated assets in memory.\n";
		return false;
	}
	return true;
}
