# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project ("gallop")

SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} 
    "C:/Boost/include/boost-1_80" 
    "${PROJECT_SOURCE_DIR}/share" 
)
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
    "C:/Boost/lib" 
)

set(CMAKE_CXX_STANDARD 17)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)
#SET(CMAKE_DEBUG_POSTFIX "_d")

FIND_PACKAGE(Boost)
IF (Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
    ADD_DEFINITIONS( "-DHAS_BOOST" )
ENDIF()

# 包含子项目。
add_subdirectory ("gallop")

add_subdirectory("trader")

add_subdirectory("core")

add_subdirectory("market")

add_subdirectory("simulator")

add_subdirectory("recorder")

#add_subdirectory("ltpy")
