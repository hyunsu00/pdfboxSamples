// PDFBoxConverter.h
//

#pragma once

struct JNIEnv_;
struct JavaVM_;
class _jclass;
struct _jmethodID;

namespace Hnc { namespace Converter {

	class PDFBox 
	{
	public:
		PDFBox();
		~PDFBox();

	public:
		bool Init();
		void Fini();

	public:
		bool ToImage(const wchar_t* sourceFile, const wchar_t* targetDir, int dpi = 96);
		bool ToText(const wchar_t* sourceFile, const wchar_t* targetDir);

	private:
		JNIEnv_*	m_Env;
		JavaVM_*	m_JavaVM;
		_jclass*	m_TargetClass;
		_jmethodID*	m_PDFToImageMethodID;
		_jmethodID*	m_PDFToTextMethodID;
		_jmethodID*	m_initializeMethodID;
		_jmethodID*	m_getPageCountMethodID;
	}; // class PDFBox

}} // Hnc::Converter
