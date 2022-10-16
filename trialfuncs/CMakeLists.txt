cmake_minimum_required(VERSION 3.13)
project(trialfuncs)
add_library(lab1 SHARED trialfuncs.c)

if(MSVC)
	include(GenerateExportHeader)
	generate_export_header(lab1
		BASE_NAME lab1
		EXPORT_MACRO_NAME LAB1_EXPORTS
		EXPORT_FILE_NAME trialfuncs_exports.h
		STATIC_DEFINE SHARED_EXPORTS_BUILT_AS_STATIC)
endif()

target_include_directories(lab1 PUBLIC
	"${PROJECT_BINARY_DIR}"
	"${PROJECT_SOURCE_DIR}/include"
)

add_subdirectory(test)

