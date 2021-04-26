// PDFBoxConverter.cpp
//
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include "PDFBoxConverter.h"
#include <locale> // std::wstring_convert
#include <codecvt> // std::codecvt_utf8
#include <jni.h>

static const char* const PDFBOX_JAR_CLASSPATH = u8"-Djava.class.path=./PDFBoxModule.jar;./lib/PDFBoxModule.jar;";
static const char* const PDFBOX_CLASS_NAME = u8"PDFBoxModule";
static const char* const PDFBOX_CONVERT_IMAGE_METHOD_NAME = u8"ConvertPDFToImage";
static const char* const PDFBOX_CONVERT_TEXT_METHOD_NAME = u8"ConvertPDFToText";

namespace Hnc { namespace Converter {

	PDFBox::PDFBox()
	: m_Env(nullptr)
	, m_JavaVM(nullptr)
	, m_TargetClass(nullptr)
	, m_PDFToImageMethodID(nullptr)
	, m_PDFToTextMethodID(nullptr)
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

		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> _W2U;

		jstring jsoureFile = m_Env->NewStringUTF(_W2U.to_bytes(sourceFile).c_str());
		jstring jtargetDir = m_Env->NewStringUTF(_W2U.to_bytes(targetDir).c_str());

		bool result = m_Env->CallStaticBooleanMethod(
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

		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> _W2U;

		jstring jsoureFile = m_Env->NewStringUTF(_W2U.to_bytes(sourceFile).c_str());
		jstring jtargetDir = m_Env->NewStringUTF(_W2U.to_bytes(targetDir).c_str());

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
