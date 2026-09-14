#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

bool LoadAssetCache(const wchar_t* name,
                    uint32_t version,
                    size_t maximumBytes,
                    std::vector<unsigned char>& data);
bool SaveAssetCache(const wchar_t* name, uint32_t version, const std::vector<unsigned char>& data);
