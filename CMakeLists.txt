cmake_minimum_required (VERSION 3.16)
project(1brc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
	execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json)
endif()

option(1BRC_ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(1BRC_ENABLE_UBSAN "Enable Undefined Behaviour Sanitizer" OFF)
option(1BRC_ENABLE_LEAK "Enable Leak Sanitizer" OFF)
option(1BRC_ENABLE_THREAD "Enable Thread Sanitizer" OFF)
option(1BRC_ENABLE_CLANG_TIDY "Enable Clang Tidy" OFF)

set(BENCHMARK_ENABLE_TESTING ON)
set(BENCHMARK_ENABLE_INSTALL OFF)
set(BENCHMARK_INSTALL_DOCS OFF)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
set(BENCHMARK_ENABLE_LIBPFM ON)
include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark
    GIT_TAG v1.8.3
)
FetchContent_GetProperties(benchmark)
if (NOT benchmark_POPULATED)
    FetchContent_Populate(benchmark)
endif()
add_subdirectory(${benchmark_SOURCE_DIR} ${benchmark_BINARY_DIR} EXCLUDE_FROM_ALL)

FetchContent_Declare(
    boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Populate(boost)
add_subdirectory(${boost_SOURCE_DIR} ${boost_BINARY_DIR} EXCLUDE_FROM_ALL)

if (1BRC_ENABLE_ASAN)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

if (1BRC_ENABLE_LEAK)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=leak")
endif()

if (1BRC_ENABLE_CLANG_OPT_REPORT)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsave-optimization-record -O3 -g")
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsave-optimization-record -O3 -g")
endif()

if (1BRC_ENABLE_UBSAN)
	# Add -fno-sanitize=vptr to remove error of no typeinfo.
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-sanitize=vptr")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -fno-sanitize=vptr")
endif()

if (1BRC_ENABLE_THREAD)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
add_subdirectory(cpp)