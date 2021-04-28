// PDFBoxConverter.cpp
//
#include "PDFBoxConverter.h"
#include <jni.h>
#include <string>
#define _ASSERTE

static const char* const PDFBOX_JAR_CLASSPATH = "-Djava.class.path=./PDFBoxModule.jar;./lib/PDFBoxModule.jar;";
static const char* const PDFBOX_CLASS_NAME = "PDFBoxModule";
static const char* const PDFBOX_CONVERT_IMAGE_METHOD_NAME = "ConvertPDFToImage";
static const char* const PDFBOX_CONVERT_TEXT_METHOD_NAME = "ConvertPDFToText";
static const char* const PDFBOX_INITIALIZE_METHOD_NAME = "PDFModuleInitialize";
static const char* const PDFBOX_GETPAGECOUNT_METHOD_NAME = "GetPDFPageCount";

namespace Hnc { namespace Converter {

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

			return ustr;
		}
	}

	PDFBox::PDFBox()
	: m_Env(nullptr)
	, m_JavaVM(nullptr)
	, m_TargetClass(nullptr)
	, m_PDFToImageMethodID(nullptr)
	, m_PDFToTextMethodID(nullptr)
	, m_initializeMethodID(nullptr)
	, m_getPageCountMethodID(nullptr)
	{
	}

	PDFBox::~PDFBox()
	{
		Fini();
	}

	bool PDFBox::Init()
	{
		JavaVMOption vmOptions[1] = { 0, };
		vmOptions[0].optionString = const_cast<char*>(PDFBOX_JAR_CLASSPATH);

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

		m_PDFToTextMethodID = m_Env->GetStaticMethodID(
			m_TargetClass, 
			PDFBOX_CONVERT_TEXT_METHOD_NAME, 
			"(Ljava/lang/String;Ljava/lang/String;)Z"
		);

		m_initializeMethodID = m_Env->GetStaticMethodID(
			m_TargetClass, 
			PDFBOX_INITIALIZE_METHOD_NAME, 
			"(Ljava/lang/String;Ljava/lang/String;)Z"
		);

		m_getPageCountMethodID = m_Env->GetStaticMethodID(
			m_TargetClass, 
			PDFBOX_GETPAGECOUNT_METHOD_NAME, 
			"()I"
		);

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

		bool result = m_Env->CallStaticBooleanMethod(
			m_TargetClass, 
			m_initializeMethodID, 
			jsoureFile,
			jtargetDir, 
			dpi
		);
		_ASSERTE(result && "m_Env->CallStaticBooleanMethod() Failed");
		if (!result) {
			return false;
		}

		int32_t pageCount = m_Env->CallStaticIntMethod(m_TargetClass, m_getPageCountMethodID);
		if (pageCount == -1) {
			return false;
		}

		result = m_Env->CallStaticBooleanMethod(
			m_TargetClass,
			m_PDFToImageMethodID, 
			jsoureFile,
			jtargetDir,
			dpi
		);
		_ASSERTE(result && "m_Env->CallStaticBooleanMethod() Failed");

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

}} // Hnc::Converter
