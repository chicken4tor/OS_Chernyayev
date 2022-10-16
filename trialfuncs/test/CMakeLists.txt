add_executable(test lab1.c)

target_include_directories(test PUBLIC
	"${PROJECT_BINARY_DIR}"
	"${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(test PUBLIC lab1)
