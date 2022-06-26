cmake_minimum_required(VERSION 3.12)
project(Compiler VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ENABLE_EXPORTS TRUE)

# ======== Compiler ========
add_executable(
	inkc
	src/ast.cpp
	src/codegen.cpp
	src/errors.cpp
	src/file_table.cpp
	src/lexer.cpp
	src/main.cpp
	src/parser.cpp
	src/sizer.cpp
	src/typecheck.cpp
	src/utils.cpp
)
target_compile_options(inkc PRIVATE -Werror -Wall -Wextra -Wpedantic -Wno-deprecated-declarations)

# ======== Testing ========
add_executable(
	testing
	src/test.cpp
	src/utils.cpp
)
add_dependencies(testing inkc)
target_compile_options(testing PRIVATE -Werror -Wall -Wextra -Wpedantic -Wno-deprecated-declarations)