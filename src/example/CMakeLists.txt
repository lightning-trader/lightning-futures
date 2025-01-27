# CMakeList.txt: gallop 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

IF(UNIX)
    SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
        "${PROJECT_SOURCE_DIR}/../api/CTP_V6.6.9_20220920/linux64/" 
    )
ENDIF()

include_directories(${CMAKE_INCLUDE_PATH} "./strategy" "../../api/LTD_V3_20250120")
link_directories(${CMAKE_LIBRARY_PATH})

aux_source_directory(strategy STRATEGY_DIR)

add_executable(evaluate "evaluate.cpp" ${STRATEGY_DIR})
target_link_libraries(evaluate "framework")

add_executable(runtime "runtime.cpp" ${STRATEGY_DIR})
target_link_libraries(runtime "framework")


# TODO: 如有需要，请添加测试并安装目标。

