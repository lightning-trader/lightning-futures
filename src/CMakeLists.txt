# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project ("lightning")

if(APPLE)
    message(STATUS "MacOS!!!")
    add_definitions( "-fPIC" )
    set(SYS_LIBS pthread)
elseif(UNIX)
    add_definitions( "-fPIC" )
    set(CTP_LIBS thostmduserapi_se thosttraderapi_se)
    set(SYS_LIBS pthread)
endif()


set(CMAKE_CXX_STANDARD 17)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)
set(CMAKE_DEBUG_POSTFIX "_d")
#set(CMAKE_CXX_FLAGS_RELEASE"-O2")

set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}
    "${PROJECT_SOURCE_DIR}/share" 
    "${PROJECT_SOURCE_DIR}/include"
)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
    "${PROJECT_SOURCE_DIR}/../bin"
)

# 包含子项目。
add_subdirectory ("example")

add_subdirectory("trader")

add_subdirectory("core")

add_subdirectory("market")

add_subdirectory("simulator")

add_subdirectory("framework")

add_subdirectory("loger")

add_subdirectory("persist")

