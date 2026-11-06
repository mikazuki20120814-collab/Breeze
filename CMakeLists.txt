cmake_minimum_required(VERSION 3.21)
project(Breeze CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CEFの標準配布版(Windows 64-bit)を展開したフォルダを指定する
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCEF_ROOT=C:/cef/cef_binary_131...
set(CEF_ROOT "" CACHE PATH "Path to the extracted CEF binary distribution")
if(NOT CEF_ROOT)
  message(FATAL_ERROR "CEF_ROOT を指定してください (-DCEF_ROOT=...)")
endif()

# サンドボックスは使わない(cef_sandbox.libが不要になる)
set(USE_SANDBOX OFF)

list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
find_package(CEF REQUIRED)

# libcef_dll_wrapper をビルド
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

set(CEF_TARGET_OUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_executable(Breeze WIN32
  src/main.cc
  src/breeze_window.h
)

SET_EXECUTABLE_TARGET_PROPERTIES(Breeze)
add_dependencies(Breeze libcef_dll_wrapper)

target_include_directories(Breeze PRIVATE "${CEF_ROOT}" "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(Breeze PRIVATE libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS} shell32 ole32)
target_compile_options(Breeze PRIVATE /utf-8)

# マニフェスト(DPI対応など)
ADD_WINDOWS_MANIFEST("${CMAKE_CURRENT_SOURCE_DIR}/resources" "Breeze" "exe")

# libcef.dll やリソースを実行ファイルの隣にコピー
COPY_FILES("Breeze" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES("Breeze" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
