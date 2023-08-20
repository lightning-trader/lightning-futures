# CMakeList.txt: gallop 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

IF(UNIX)
    SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
        "${PROJECT_SOURCE_DIR}/../api/CTP_V6.6.9_20220920/linux64/" 
    )
ENDIF()

include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})

add_executable(example "example.cpp" "marketing_strategy.cpp" "orderflow_strategy.cpp")

target_link_libraries(example "framework")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET example PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。

