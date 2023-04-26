# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

project ("gallop")

set(CMAKE_CXX_STANDARD 17)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)
#SET(CMAKE_DEBUG_POSTFIX "_d")

# set(Boost_NO_SYSTEM_PATHS ON)
set(BOOST_ROOT "/home/ubuntu/boost_1_80_0/install")
set(BOOST_INCLUDE_DIRS "${BOOST_ROOT}/include")
set(BOOST_LIBRARY_DIRS "${BOOST_ROOT}/lib")

FIND_PACKAGE(Boost REQUIRED)
IF (Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
    ADD_DEFINITIONS( "-DHAS_BOOST" )
    ADD_DEFINITIONS( "-DBOOST_ALL_DYN_LINK" )
ENDIF()

IF (UNIX)
ADD_DEFINITIONS( "-fPIC" )
set(SYS_LIBS pthread rt)
set(BOOST_LIBS boost_thread boost_log boost_log_setup boost_system boost_filesystem)
set(CTP_LIBS thostmduserapi_se thosttraderapi_se)
ENDIF()

SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH}
    "${PROJECT_SOURCE_DIR}/share" 
    "${PROJECT_SOURCE_DIR}/include"
    ${Boost_INCLUDE_DIRS}
)
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH}
    ${BOOST_LIBRARY_DIRS}
    ${PROJECT_SOURCE_DIR}/api/ctp_6.6.9
    "${PROJECT_SOURCE_DIR}/../bin"
)

# 包含子项目。
add_subdirectory ("gallop")

add_subdirectory("trader")

add_subdirectory("core")

add_subdirectory("market")

add_subdirectory("simulator")

add_subdirectory("ltpp")

add_subdirectory("recorder")
