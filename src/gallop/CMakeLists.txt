# CMakeList.txt: gallop 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)


include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})

AUX_SOURCE_DIRECTORY(strategy   STRATEGY_DIR_VAR)
AUX_SOURCE_DIRECTORY(strategy/signal   SIGNAL_DIR_VAR)
AUX_SOURCE_DIRECTORY(strategy/pms   PMS_DIR_VAR)
# 将源代码添加到此项目的可执行文件。
add_executable (gallop "gallop.cpp" "pugixml.cpp" ${STRATEGY_DIR_VAR} ${SIGNAL_DIR_VAR} ${PMS_DIR_VAR})

target_link_libraries(gallop "libltpp")


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET gallop PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
