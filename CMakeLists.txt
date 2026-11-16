cmake_minimum_required(VERSION 3.20)
project(ChessMSVC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG REQUIRED)

add_executable(chess
    src/main.cpp
    src/ChessTypes.hpp
    src/ChessEngine.hpp
    src/ChessEngine.cpp
    src/ChessUI.hpp
    src/ChessUI.cpp
)

target_link_libraries(chess PRIVATE raylib)

if(MSVC)
    # 开启高警告等级与 UTF-8 源码编码支持
    target_compile_options(chess PRIVATE /W4 /utf-8)
    # 提示：如果你想在 Release 版本隐藏背后的黑色控制台窗口，可取消下面两行注释
    # set_target_properties(chess PROPERTIES WIN32_EXECUTABLE TRUE)
    # target_link_options(chess PRIVATE "/ENTRY:mainCRTStartup")
else()
    target_compile_options(chess PRIVATE -Wall -Wextra)
endif()