# CMakeList.txt: gallop 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)


include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})

# 将源代码添加到此项目的可执行文件。
add_executable (gallop "gallop.cpp" "demo_strategy.cpp" "hcc_strategy.cpp" "hft_1_strategy.cpp" "dm_strategy.cpp")

target_link_libraries(gallop "lightning")

target_link_libraries(gallop)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET gallop PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
