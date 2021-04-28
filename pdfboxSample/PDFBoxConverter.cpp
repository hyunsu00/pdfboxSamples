// PDFBoxConverter.cpp
//
#include "PDFBoxConverter.h"
#include <jni.h>
#include <string>
#include <memory>
#include "debug_assert.h"

#ifdef _WIN32
#else
#	include <libgen.h> // dirname
#	include <unistd.h> // readlink
#	include <memory.h> // memset
#endif

static const char* const PDFBOX_JAR_CLASSPATH_NAME = u8"-Djava.class.path=";
static const char* const PDFBOX_MODULE_FILE_NAME = u8"PDFBoxModule.jar";
static const char* const PDFBOX_JAR_CLASSPATH = u8"-Djava.class.path=./PDFBoxModule.jar;./lib/PDFBoxModule.jar;";
static const char* const PDFBOX_CLASS_NAME = u8"PDFBoxModule";
static const char* const PDFBOX_CONVERT_IMAGE_METHOD_NAME = u8"ConvertPDFToImage";
static const char* const PDFBOX_CONVERT_TEXT_METHOD_NAME = u8"ConvertPDFToText";
static const char* const PDFBOX_INITIALIZE_METHOD_NAME = u8"PDFModuleInitialize";
static const char* const PDFBOX_GETPAGECOUNT_METHOD_NAME = u8"GetPDFPageCount";

namespace PDF { namespace Converter {

	static std::string _W2U(const std::wstring& wstr)
	{
		std::string ustr;
		for (size_t i = 0; i < wstr.size(); i++){
			wchar_t w = wstr[i];
			if (w <= 0x7f) {
				ustr.push_back((char)w);
			} else if (w <= 0x7ff) {
				ustr.push_back(0xc0 | ((w >> 6)& 0x1f));
				ustr.push_back(0x80| (w & 0x3f));
			} else if (w <= 0xffff) {
				ustr.push_back(0xe0 | ((w >> 12)& 0x0f));
				ustr.push_back(0x80| ((w >> 6) & 0x3f));
				ustr.push_back(0x80| (w & 0x3f));
			} else if (w <= 0x10ffff) {
				ustr.push_back(0xf0 | ((w >> 18)& 0x07));
				ustr.push_back(0x80| ((w >> 12) & 0x3f));
				ustr.push_back(0x80| ((w >> 6) & 0x3f));
				ustr.push_back(0x80| (w & 0x3f));
			} else {
				ustr.push_back('?');
			}
		}
		return ustr;
	}

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
		vmOptions[0].optionString = const_cast<char*>(PDFBOX_JAR_CLASSPATH);
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
		snprintf(optionString, maxPath, "%s%s/%s", PDFBOX_JAR_CLASSPATH_NAME, exePath, PDFBOX_MODULE_FILE_NAME);
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
		m_TargetClass = m_Env->FindClass(PDFBOX_CLASS_NAME);
		_ASSERTE(m_TargetClass && "m_Env->FindClass() Failed");
		if (!m_TargetClass) {
			return false;
		}

		m_PDFToImageMethodID = m_Env->GetStaticMethodID(
			m_TargetClass,
			PDFBOX_CONVERT_IMAGE_METHOD_NAME, 
			"(Ljava/lang/String;Ljava/lang/String;I)Z"
		);
		_ASSERTE(m_PDFToImageMethodID && "m_Env->GetStaticMethodID() Failed");
		if (!m_PDFToImageMethodID) {
			return false;
		}

		m_PDFToTextMethodID = m_Env->GetStaticMethodID(
			m_TargetClass, 
			PDFBOX_CONVERT_TEXT_METHOD_NAME, 
			"(Ljava/lang/String;Ljava/lang/String;)Z"
		);
		_ASSERTE(m_PDFToTextMethodID && "m_Env->GetStaticMethodID() Failed");
		if (!m_PDFToTextMethodID) {
			return false;
		}

		m_InitializeMethodID = m_Env->GetStaticMethodID(
			m_TargetClass, 
			PDFBOX_INITIALIZE_METHOD_NAME, 
			"(Ljava/lang/String;Ljava/lang/String;)Z"
		);
		_ASSERTE(m_InitializeMethodID && "m_Env->GetStaticMethodID() Failed");
		if (!m_InitializeMethodID) {
			return false;
		}

		m_GetPageCountMethodID = m_Env->GetStaticMethodID(
			m_TargetClass, 
			PDFBOX_GETPAGECOUNT_METHOD_NAME, 
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

		jstring jsoureFile = m_Env->NewStringUTF(_W2U(sourceFile).c_str());
		jstring jtargetDir = m_Env->NewStringUTF(_W2U(targetDir).c_str());

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

		jstring jsoureFile = m_Env->NewStringUTF(_W2U(sourceFile).c_str());
		jstring jtargetDir = m_Env->NewStringUTF(_W2U(targetDir).c_str());

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
