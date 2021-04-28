// main.cpp
//

#include "PDFBoxConverter.h"
#include <string> // std::string
#include <memory> // std::unique_ptr
#include <chrono> // std::chrono
#include <vector> // std::vector
#include <iostream>
#include <stdlib.h> // mbstowcs()
//#include <filesystem> // std::filesystem::path

#ifdef _WIN32
#else
#   include <string.h> // strdup
#   include <libgen.h> // dirname
#endif

int main(int argc, char* argv[])
{
	// 로케일 설정
	setlocale(LC_ALL, "" );

	std::string exeDir;
#ifdef _WIN32
    char drive[_MAX_DRIVE] = { 0, }; // 드라이브 명
    char dir[_MAX_DIR] = { 0, }; // 디렉토리 경로
    _splitpath_s(argv[0], drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
    exeDir = std::string(drive) + dir;
#else
    char* exePath = strdup(argv[0]);
    exeDir = dirname(exePath);
    free(exePath);
    exeDir += "/";
#endif

	std::vector<wchar_t> buffer(exeDir.length() + 1, 0);
	mbstowcs(&buffer[0], exeDir.c_str(), 51);

	struct FreeDeleter 
    {
        inline void operator()(void* ptr) const 
        { 
            free(ptr); 
        }
    }; // struct FreeDeleter 
    using AutoMemoryPtr = std::unique_ptr<char, FreeDeleter>;

	const std::wstring wexeDir = &buffer[0];
	const std::wstring pdfName = L"sample01";
    const std::wstring samplePath = wexeDir + L"samples/" + pdfName + L".pdf";
    const std::wstring resultDir = wexeDir + L"result/";

	// PDF -> PNG, PDF -> TXT 변환
	{
		Hnc::Converter::PDFBox pdfConverter;
		bool result  = pdfConverter.Init();
		if (!result) {
			std::cerr << "PDFBox Init() Failed()";
			return 0;
		}

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		{
			pdfConverter.ToImage(samplePath.c_str(), resultDir.c_str());
			// pdfConverter.ToText(samplePath.c_str(), resultDir.c_str());	
		}
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
        std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
        std::cout << "Time difference (sec) = " <<  (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) /1000000.0  << std::endl;

		pdfConverter.Fini();
	}

	return 0;
}

/*  임시 코드
	namespace fs = std::filesystem;
	fs::path exeDir(argv[0]);
	exeDir.remove_filename();

	// 환경변수 설정
	{
		size_t requiredSize;
		_wgetenv_s(&requiredSize, nullptr, 0, L"PATH");
		std::vector<wchar_t> envPath(requiredSize, 0);
		_wgetenv_s(&requiredSize, &envPath[0], requiredSize, L"PATH");

		std::wstring addEnvPath = std::wstring(exeDir.c_str()) + L"jre/bin;" + exeDir.c_str() + L"jre/bin/server;";
		addEnvPath += &envPath[0];
		_wputenv_s(L"PATH", addEnvPath.c_str());
	}

	std::wstring samplePath = std::wstring(exeDir.c_str()) + L"samples/sample01.pdf";
	_ASSERTE(fs::exists(samplePath) && "samplePath is not exist");
	std::wstring resultDir = std::wstring(exeDir.c_str()) + L"result";
	bool isExistDir = fs::exists(resultDir);
	_ASSERTE(isExistDir && "resultDir is not exist");
	if (!isExistDir) {
		fs::create_directories(resultDir);
	}
*/