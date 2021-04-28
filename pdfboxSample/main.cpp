// main.cpp

#include "PDFBoxConverter.h"
#include <vector> // std::vector
#include <string> // std::string
#include <memory> // std::unique_ptr
#include <chrono> // std::chrono
#include <iostream> // std::cout
#include <algorithm> // std::transform
#include <stdlib.h> // mbstowcs()
#include "cmdline.h"

#ifdef _WIN32
#else
#   include <string.h> // strdup
#   include <sys/stat.h> // stat
#   include <libgen.h> // dirname, basename
#	include <unistd.h> // access
#endif

int main(int argc, char* argv[])
{
	// 로케일 설정
	setlocale(LC_ALL, "" );

	cmdline::parser parser;
    parser.add<std::string>("source", 's', "PDF absolute file path", true, "");
    parser.add<std::string>("result", 'r', "result absolute dir", true, "");
    parser.add<std::string>("type", 't', "convert type", false, "png", cmdline::oneof<std::string>("png", "txt"));
    parser.add("help", 0, "print this message");
    parser.set_program_name("pdfboxTester");

    bool ok = parser.parse(argc, argv);

    if (argc == 1 || parser.exist("help") || ok == false){
        std::cerr << parser.usage();
        return 0;
    }

    std::string source = parser.get<std::string>("source");
    std::string result = parser.get<std::string>("result");
    std::string type = parser.get<std::string>("type");
	// type 문자열 소문자로 변경
	std::transform(type.begin(), type.end(), type.begin(), ::tolower);

    struct FreeDeleter
    {
        inline void operator()(void* ptr) const
        {
            free(ptr);
        }
    }; // struct FreeDeleter
    using AutoMemoryPtr = std::unique_ptr<char, FreeDeleter>;

	std::string rawFileName;
#ifdef _WIN32

#else
    {
        auto _dirExists = [](const char* const dirPath) -> bool {
            struct stat info;
            if (stat(dirPath, &info) == 0 && S_ISDIR(info.st_mode)) {
                return true;
            }

            return false;
        };
        auto _addDirSeparator = [](const std::string& dirPath) -> std::string {
            std::string addDirPath = dirPath;
            if (dirPath.back() != '/') {
                addDirPath += "/";
            }
            return addDirPath;
        };
        auto _removeExt = [](const std::string& fileName) -> std::string {
            size_t lastIndex = fileName.find_last_of("."); 
            std::string rawName = fileName.substr(0, lastIndex);
            return rawName;
        };

        // PDF 파일이 존재하는지 체크
        if (access(source.c_str(), F_OK)) {
            std::cout << "source file is not valid path" << std::endl;
            return 0;
        }

        // 결과 폴더가 존재하는지 체크
        if (!_dirExists(result.c_str())) {
            std::cout << "result directory is not exist" << std::endl;
            return 0;
        }
        result = _addDirSeparator(result);

        rawFileName = _removeExt(basename(AutoMemoryPtr(strdup(source.c_str())).get()));
    }
#endif

	auto _A2U = [](const std::string& str) -> std::wstring {
		std::vector<wchar_t> wstrVector(str.length() + 1, 0);
		mbstowcs(&wstrVector[0], str.c_str(), wstrVector.size());
		return &wstrVector[0];
	};
	const std::wstring samplePath = _A2U(source);
    const std::wstring resultDir= _A2U(result);

	// PDF -> PNG, PDF -> TXT 변환
	{
		PDF::Converter::PDFBox pdfConverter;
		bool result  = pdfConverter.Init();
		if (!result) {
			std::cerr << "PDFBox Init() Failed()";
			return 0;
		}

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		{
			if (type == "png") {
				result = pdfConverter.ToImage(samplePath.c_str(), resultDir.c_str());
				if (!result) {
					std::cout << "PDFBox ToImage() Failed()" << std::endl;
				}	
			} else if (type == "txt") {
				result = pdfConverter.ToText(samplePath.c_str(), resultDir.c_str());
				if (!result) {
					std::cerr << "PDFBox ToText() Failed()" << std::endl;
				}
			}
		}
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
        std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
        std::cout << "Time difference (sec) = " <<  (std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) /1000000.0  << std::endl;

		pdfConverter.Fini();
	}

	return 0;
}

/* 	임시 코드
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
*/
/*  임시 코드
	// #include <filesystem> // std::filesystem::path

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