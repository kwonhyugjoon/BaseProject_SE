#include "stdafx.h"
#include "PlatformPaths.h"
#include <cstdlib>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif

std::filesystem::path ExecutableDirectory()
{
#ifdef _WIN32
	wchar_t path[32768];
	DWORD length = GetModuleFileNameW(nullptr, path, 32768);
	if (length && length < 32768)
		return std::filesystem::path(std::wstring(path, length)).parent_path();
#elif defined(__APPLE__)
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	std::string path(size, '\0');
	if (_NSGetExecutablePath(&path[0], &size) == 0)
		return std::filesystem::path(path.c_str()).parent_path();
#else
	char path[4096];
	ssize_t length = readlink("/proc/self/exe", path, sizeof(path));
	if (length > 0 && length < static_cast<ssize_t>(sizeof(path)))
		return std::filesystem::path(std::string(path, length)).parent_path();
#endif
	return std::filesystem::current_path();
}

std::filesystem::path ResourcePath(const char* relative)
{
	for (const auto& root :
	     {std::filesystem::path(), std::filesystem::path("SimpleGame"), ExecutableDirectory()})
	{
		auto path = root / relative;
		std::error_code error;
		if (std::filesystem::is_regular_file(path, error))
			return path;
	}
	return ExecutableDirectory() / relative;
}

std::filesystem::path CacheDirectory()
{
	std::filesystem::path root;
#ifdef _WIN32
	wchar_t folder[32768];
	DWORD size = GetEnvironmentVariableW(L"LOCALAPPDATA", folder, 32768);
	if (size && size < 32768)
		root = folder;
#elif defined(__APPLE__)
	if (const char* home = std::getenv("HOME"))
		root = std::filesystem::path(home) / "Library/Caches";
#else
	if (const char* cache = std::getenv("XDG_CACHE_HOME"))
		root = cache;
	else if (const char* home = std::getenv("HOME"))
		root = std::filesystem::path(home) / ".cache";
#endif
	if (root.empty())
		return {};
	root /= "BaseProject_SE/Cache";
	std::error_code error;
	std::filesystem::create_directories(root, error);
	return error ? std::filesystem::path() : root;
}
