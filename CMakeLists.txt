cmake_minimum_required(VERSION 3.20)

project(evan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 默认优先使用本地检测到的 Qt 6.8.3
if(NOT DEFINED Qt6_DIR AND EXISTS "E:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
    set(Qt6_DIR "E:/Qt/6.8.3/msvc2022_64/lib/cmake/Qt6")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# 包含头文件目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
)

# 基础模块源文件收集
file(GLOB_RECURSE SOURCES
    "src/main.cpp"
    "src/core/*.cpp"
    "src/core/*.h"
    "src/ui/*.cpp"
    "src/ui/*.h"
    "src/platform/*.cpp"
    "src/platform/*.h"
)

# Windows 平台专用链接库与选项
if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} resources/app.rc resources/resources.qrc)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        User32
        Gdi32
        Dwmapi
        windowsapp
        runtimeobject
    )
else()
    add_executable(${PROJECT_NAME} ${SOURCES} resources/resources.qrc)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    )
endif()

# MSVC 编译参数调优 (UTF-8 字符集与快速编译)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /permissive- /W4)
endif()
