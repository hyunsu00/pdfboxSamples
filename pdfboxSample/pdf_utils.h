// fpdf_utils.h
#pragma once
#include <memory> // std::unique_ptr
#include <string> // std::string
#include <vector> // std::vector
#include <stdlib.h> // wcstombs, mbstowcs

#ifdef _WIN32
#	include <Shlwapi.h>
#else
#   include <sys/stat.h> // stat
#	include <unistd.h> // access
#   include <string.h> // strdup
#   include <libgen.h> // dirname, basename
#endif

namespace {

	struct FreeDeleter
	{
		inline void operator()(void* ptr) const
		{
			free(ptr);
		}
	}; // struct FreeDeleter
	using AutoMemoryPtr = std::unique_ptr<char, FreeDeleter>;

	auto getFileContents = [](const char* filename, size_t* retlen) -> AutoMemoryPtr {
		FILE* file = fopen(filename, "rb");
		if (!file) {
			fprintf(stderr, "Failed to open: %s\n", filename);
			return nullptr;
		}
		(void)fseek(file, 0, SEEK_END);
		size_t file_length = ftell(file);
		if (!file_length) {
			return nullptr;
		}
		(void)fseek(file, 0, SEEK_SET);
		AutoMemoryPtr buffer(static_cast<char*>(malloc(file_length)));
		if (!buffer) {
			return nullptr;
		}
		size_t bytes_read = fread(buffer.get(), 1, file_length, file);
		(void)fclose(file);
		if (bytes_read != file_length) {
			fprintf(stderr, "Failed to read: %s\n", filename);
			return nullptr;
		}
		*retlen = bytes_read;
		return buffer;
	};

	auto _U2A = [](const std::wstring& wstr) -> std::string {
		std::vector<char> strVector(wstr.length() + 1, 0);
		wcstombs(&strVector[0], wstr.c_str(), strVector.size());
		return &strVector[0];
	};

	auto _A2U = [](const std::string& str) -> std::wstring {
		std::vector<wchar_t> wstrVector(str.length() + 1, 0);
		mbstowcs(&wstrVector[0], str.c_str(), wstrVector.size());
		return &wstrVector[0];
	};

	auto _U2UTF8 = [](const std::wstring& wstr) -> std::string {
		std::string ustr;
		for (size_t i = 0; i < wstr.size(); i++) {
			wchar_t w = wstr[i];
			if (w <= 0x7f) {
				ustr.push_back((char)w);
			} else if (w <= 0x7ff) {
				ustr.push_back(0xc0 | ((w >> 6) & 0x1f));
				ustr.push_back(0x80 | (w & 0x3f));
			} else if (w <= 0xffff) {
				ustr.push_back(0xe0 | ((w >> 12) & 0x0f));
				ustr.push_back(0x80 | ((w >> 6) & 0x3f));
				ustr.push_back(0x80 | (w & 0x3f));
			} else if (w <= 0x10ffff) {
				ustr.push_back(0xf0 | ((w >> 18) & 0x07));
				ustr.push_back(0x80 | ((w >> 12) & 0x3f));
				ustr.push_back(0x80 | ((w >> 6) & 0x3f));
				ustr.push_back(0x80 | (w & 0x3f));
			} else {
				ustr.push_back('?');
			}
		}

		return ustr;
	};

	auto pathFileExists = [](const char* const pszPath) -> bool {
#ifdef _WIN32
		return ::PathFileExistsA(pszPath) ? true : false;
#else
		if (access(pszPath, F_OK) == 0) {
			return true;
		}
#endif
		return false;
	};

	auto pathIsDirectory = [](const char* const pszPath) -> bool {
#ifdef _WIN32
		return ::PathIsDirectoryA(pszPath) ? true : false;
#else
		struct stat info;
		if (stat(pszPath, &info) == 0 && S_ISDIR(info.st_mode)) {
			return true;
		}
#endif
		return false;
	};

	auto pathAddSeparator = [](const std::string& dirPath) -> std::string {
#ifdef _WIN32
		const char PATH_SEPARATOR = '\\';
#else
		const char PATH_SEPARATOR = '/';
#endif
		std::string addDirPath = dirPath;
		if (dirPath.back() != PATH_SEPARATOR) {
			addDirPath += PATH_SEPARATOR;
		}

		return addDirPath;
	};

	auto pathFindFilename = [](const std::string& filePath) -> std::string {
#ifdef _WIN32
		std::string fileName = ::PathFindFileNameA(filePath.c_str());
#else
		std::string fileName = basename(AutoMemoryPtr(strdup(filePath.c_str())).get());
#endif
		return fileName;
	};
	
	auto removeExt = [](const std::string& fileName) -> std::string {
		size_t lastIndex = fileName.find_last_of(".");
		std::string rawName = fileName.substr(0, lastIndex);
		return rawName;
	};
}
