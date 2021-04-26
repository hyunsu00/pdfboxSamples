// main.cpp
//

#include "PDFBoxConverter.h"
#include <filesystem> // std::filesystem::path

int main(int argc, char* argv[])
{
	bool result = false;

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

	// PDF -> PNG, PDF -> TXT 변환
	{
		Hnc::Converter::PDFBox pdfConverter;
		pdfConverter.Init();
		pdfConverter.ToImage(samplePath.c_str(), resultDir.c_str());
		pdfConverter.ToText(samplePath.c_str(), resultDir.c_str());
		pdfConverter.Fini();
	}

	return 0;
}
