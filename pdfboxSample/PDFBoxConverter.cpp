// PDFBoxConverter.cpp
#include "PDFBoxConverter.h"
#include <jni.h>
#include <string>
#include <memory>
#include "pdf_assert.h"
#include "pdf_utils.h"

#ifdef _WIN32
#	include <Windows.h>
#else
#	include <libgen.h> // dirname
#	include <unistd.h> // readlink
#	include <memory.h> // memset
#endif

static const wchar_t* const PDFBOX_JAR_CLASSPATH_NAME = L"-Djava.class.path=";
static const wchar_t* const PDFBOX_MODULE_FILE_NAME = L"PDFBoxModule.jar";
static const wchar_t* const PDFBOX_CLASS_NAME = L"PDFBoxModule";
static const wchar_t* const PDFBOX_CONVERT_IMAGE_METHOD_NAME = L"ConvertPDFToImage";
static const wchar_t* const PDFBOX_CONVERT_TEXT_METHOD_NAME = L"ConvertPDFToText";
static const wchar_t* const PDFBOX_INITIALIZE_METHOD_NAME = L"PDFModuleInitialize";
static const wchar_t* const PDFBOX_GETPAGECOUNT_METHOD_NAME = L"GetPDFPageCount";

namespace PDF { namespace Converter {

	

	PDFBox::PDFBox()
	: m_Env(nullptr)
	, m_JavaVM(nullptr)
	, m_TargetClass(nullptr)
	, m_PDFToImageMethodID(nullptr)
	, m_PDFToTextMethodID(nullptr)
	, m_InitializeMethodID(nullptr)
	, m_GetPageCountMethodID(nullptr)
	{
	}

	PDFBox::~PDFBox()
	{
		Fini();
	}

	bool PDFBox::Init()
	{
		JavaVMOption vmOptions[1] = { 0, };
#ifdef _WIN32
		// 윈도우에서는 자바 클래스 패스가 상대경로도 가능하지만 리눅스에서는 상대경로 지정시
		// 실패해서 리눅스와 동일하게 절대경로로 변경한다.
		wchar_t exePath[_MAX_PATH] = { 0, }; // exe 실행경로
		wchar_t drive[_MAX_DRIVE] = { 0, }; // 드라이브 명
		wchar_t dir[_MAX_DIR] = { 0, }; // 디렉토리 경로
		::GetModuleFileNameW(nullptr, exePath, sizeof(exePath));
		_wsplitpath_s(exePath, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
		std::wstring exeDir = std::wstring(drive) + dir;

		wchar_t optionStringW[_MAX_PATH] = { 0, };
		_snwprintf_s(optionStringW, _countof(optionStringW), _TRUNCATE, L"%s%s%s", PDFBOX_JAR_CLASSPATH_NAME, exeDir.c_str(), PDFBOX_MODULE_FILE_NAME);

		char optionStringA[_MAX_PATH] = { 0, };
		strcpy_s(optionStringA, sizeof(optionStringA), _U2A(optionStringW).c_str());
		vmOptions[0].optionString = optionStringA;
#else
		const uint32_t maxPath = 1024;
		char pathTemp[maxPath] = { 0, };
		ssize_t count = readlink("/proc/self/exe", pathTemp, maxPath);
		const char* exePath = nullptr;
		if (count != -1) {
			exePath = dirname(pathTemp);
		}
		char optionString[maxPath] = { 0, };
		memset(optionString, 0x00, maxPath);
		snprintf(optionString, maxPath, "%s%s/%s", _U2A(PDFBOX_JAR_CLASSPATH_NAME).c_str(), exePath, _U2A(PDFBOX_MODULE_FILE_NAME).c_str());
		vmOptions[0].optionString = optionString;
		printf("%s\n", optionString);
#endif

		JavaVMInitArgs vmArgs = { 0, };
		vmArgs.options = vmOptions;
		vmArgs.nOptions = 1;
		vmArgs.version = JNI_VERSION_1_8;

		// create java virtual mathine		
		jint result = JNI_CreateJavaVM(&m_JavaVM, (void**)&m_Env, &vmArgs);
		_ASSERTE(result == JNI_OK && m_Env && "JNI_CreateJavaVM() Failed");
		if (result != JNI_OK || !m_Env) {
			return false;
		}

		// find target class and load
		m_TargetClass = m_Env->FindClass(_U2A(PDFBOX_CLASS_NAME).c_str());
		_ASSERTE(m_TargetClass && "m_Env->FindClass() Failed");
		if (!m_TargetClass) {
			return false;
		}

		m_PDFToImageMethodID = m_Env->GetStaticMethodID(
			m_TargetClass,
			_U2A(PDFBOX_CONVERT_IMAGE_METHOD_NAME).c_str(),
			"(Ljava/lang/String;Ljava/lang/String;I)Z"
		);
		_ASSERTE(m_PDFToImageMethodID && "m_Env->GetStaticMethodID() Failed");
		if (!m_PDFToImageMethodID) {
			return false;
		}

		m_PDFToTextMethodID = m_Env->GetStaticMethodID(
			m_TargetClass, 
			_U2A(PDFBOX_CONVERT_TEXT_METHOD_NAME).c_str(),
			"(Ljava/lang/String;Ljava/lang/String;)Z"
		);
		_ASSERTE(m_PDFToTextMethodID && "m_Env->GetStaticMethodID() Failed");
		if (!m_PDFToTextMethodID) {
			return false;
		}

		m_InitializeMethodID = m_Env->GetStaticMethodID(
			m_TargetClass, 
			_U2A(PDFBOX_INITIALIZE_METHOD_NAME).c_str(),
			"(Ljava/lang/String;Ljava/lang/String;)Z"
		);
		_ASSERTE(m_InitializeMethodID && "m_Env->GetStaticMethodID() Failed");
		if (!m_InitializeMethodID) {
			return false;
		}

		m_GetPageCountMethodID = m_Env->GetStaticMethodID(
			m_TargetClass, 
			_U2A(PDFBOX_GETPAGECOUNT_METHOD_NAME).c_str(),
			"()I"
		);
		_ASSERTE(m_GetPageCountMethodID && "m_Env->GetStaticMethodID() Failed");
		if (!m_GetPageCountMethodID) {
			return false;
		}

		return true;
	}

	void PDFBox::Fini()
	{
		if (m_JavaVM) {
			m_JavaVM->DestroyJavaVM();
		}
	}

	bool PDFBox::ToImage(const wchar_t* sourceFile, const wchar_t* targetDir, int dpi /*= 96*/)
	{
		_ASSERTE(sourceFile && "sourceFile is not Null");
		_ASSERTE(targetDir && "sourceFile is not Null");
		_ASSERTE(m_Env && "m_Env is not Null");
		if (!sourceFile || !targetDir || !m_Env) {
			return false;
		}

		jstring jsoureFile = m_Env->NewStringUTF(_U2A(sourceFile).c_str());
		jstring jtargetDir = m_Env->NewStringUTF(_U2A(targetDir).c_str());

		int32_t pageCount  = 0;
		bool result = m_Env->CallStaticBooleanMethod(
			m_TargetClass, 
			m_InitializeMethodID, 
			jsoureFile,
			jtargetDir, 
			dpi
		);
		_ASSERTE(result && "m_Env->CallStaticBooleanMethod() Failed");
		if (!result) {
			goto CLEAN_UP;
		}

		pageCount = m_Env->CallStaticIntMethod(m_TargetClass, m_GetPageCountMethodID);
		if (pageCount == -1) {
			result = false;
			goto CLEAN_UP;
		}

		result = m_Env->CallStaticBooleanMethod(
			m_TargetClass,
			m_PDFToImageMethodID, 
			jsoureFile,
			jtargetDir,
			dpi
		);
		_ASSERTE(result && "m_Env->CallStaticBooleanMethod() Failed");

CLEAN_UP:
		m_Env->DeleteLocalRef(jsoureFile);
		m_Env->DeleteLocalRef(jtargetDir);

		return result;
	}

	bool PDFBox::ToText(const wchar_t* sourceFile, const wchar_t* targetDir)
	{
		_ASSERTE(sourceFile && "sourceFile is not Null");
		_ASSERTE(targetDir && "sourceFile is not Null");
		_ASSERTE(m_Env && "m_Env is not Null");
		if (!sourceFile || !targetDir || !m_Env) {
			return false;
		}

		jstring jsoureFile = m_Env->NewStringUTF(_U2A(sourceFile).c_str());
		jstring jtargetDir = m_Env->NewStringUTF(_U2A(targetDir).c_str());

		bool result = m_Env->CallStaticBooleanMethod(
			m_TargetClass,
			m_PDFToTextMethodID,
			jsoureFile,
			jtargetDir
		);
		_ASSERTE(result && "m_Env->CallStaticBooleanMethod() Failed");

		m_Env->DeleteLocalRef(jsoureFile);
		m_Env->DeleteLocalRef(jtargetDir);

		return result;
	}

}} // PDF::Converter
