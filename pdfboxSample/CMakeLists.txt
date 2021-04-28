# CMakeList.txt
cmake_minimum_required (VERSION 2.8)

#
# 프로젝트 이름 지정
project ("pdfboxTester")
# 메시지 출력
message(STATUS "\${PROJECT_NAME} = ${PROJECT_NAME}")

# 빌드 상세 과정을 모두 출력하는 Makefile을 생성
set (CMAKE_VERBOSE_MAKEFILE true)

###
# 패키지 찾기 - PDFiumConfig.cmake 파일 인클루드
find_package(JNI REQUIRED)
include_directories(${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
message(STATUS "\${JAVA_INCLUDE_PATH} = ${JAVA_INCLUDE_PATH}")
message(STATUS "\${JAVA_INCLUDE_PATH2} = ${JAVA_INCLUDE_PATH2}")
message(STATUS "\${CMAKE_CURRENT_LIST_DIR} = ${CMAKE_CURRENT_LIST_DIR}")

###
# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_executable (${PROJECT_NAME} 
	"main.cpp" 
	"PDFBoxConverter.cpp"
	"PDFBoxConverter.h" 
)

###
# 실행파일 생성후에 지정
target_link_libraries(${PROJECT_NAME} ${JNI_LIBRARIES})
message(STATUS "\${JNI_LIBRARIES} = ${JNI_LIBRARIES}")

# LINUX #include <experimental/type_traits> 사용위해
message(STATUS "\${CMAKE_SYSTEM_NAME} = ${CMAKE_SYSTEM_NAME}")
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	message(STATUS "\${CMAKE_COMPILER_IS_GNUCC} = ${CMAKE_COMPILER_IS_GNUCC}")
	message(STATUS "\${CMAKE_CXX_COMPILER_VERSION} = ${CMAKE_CXX_COMPILER_VERSION}")
	set(CMAKE_CXX_FLAGS "-std=c++11")
endif()

###
# DLL 실행파일 실행파일 위치로 복사
# ${CMAKE_COMMAND} -E copy_if_different : cmake -E copy_if_different
# $<TARGET_FILE:pdfium> : 주 파일 (.exe, .so.1.2, .a)의 전체 경로입니다. 여기서은 pdfium대상의 이름입니다.
# $<TARGET_FILE_DIR:pdfiumSample02> : 기본 파일 (.exe, .so.1.2, .a)의 디렉토리입니다. 여기서은 pdfiumSample02대상의 이름입니다.
add_custom_command(
	TARGET ${PROJECT_NAME} POST_BUILD
	COMMENT "pdfbox 라이브러리를 복사합니다."
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/samples/"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/samples" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/samples/"
	COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/result/"
	COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/lib/PDFBoxModule.jar" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/PDFBoxModule.jar"
)
message(STATUS "\${CMAKE_COMMAND} = ${CMAKE_COMMAND}")
