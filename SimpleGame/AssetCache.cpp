#include "stdafx.h"
#include "AssetCache.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <iostream>
#include <utility>

namespace
{
	std::wstring CachePath(const wchar_t* name)
	{
		wchar_t folder[32768];
		DWORD size = GetEnvironmentVariableW(L"LOCALAPPDATA", folder, 32768);
		if (!size || size >= 32768)
			return {};
		std::wstring path = std::wstring(folder) + L"\\BaseProject_SE";
		CreateDirectoryW(path.c_str(), nullptr);
		path += L"\\Cache";
		CreateDirectoryW(path.c_str(), nullptr);
		return path + L"\\" + name;
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
	std::wstring path = CachePath(name);
	if (path.empty())
		return false;
	std::ifstream file(path.c_str(), std::ios::binary);
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
	std::wstring path = CachePath(name);
	if (path.empty())
		return false;
	std::wstring temporary = path + L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
	uint32_t header[] = {0x31434752, version, static_cast<uint32_t>(data.size()), Checksum(data)};
	std::ofstream file(temporary.c_str(), std::ios::binary | std::ios::trunc);
	file.write(reinterpret_cast<const char*>(header), sizeof(header));
	file.write(reinterpret_cast<const char*>(data.data()), data.size());
	file.close();
	if (!file || !MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
	{
		DeleteFileW(temporary.c_str());
		std::cerr << "Asset cache could not be saved; using generated assets in memory.\n";
		return false;
	}
	return true;
}
