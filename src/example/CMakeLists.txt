# CMakeList.txt: gallop 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

IF(UNIX)
    SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
        "${PROJECT_SOURCE_DIR}/../api/CTP_V6.7.9_20250319/linux64/" 
    )
ENDIF()

include_directories(${CMAKE_INCLUDE_PATH} "./strategy" "../../api/LTD_V3_20250919" "../framework" )
link_directories(${CMAKE_LIBRARY_PATH})

aux_source_directory(strategy STRATEGY_DIR)

# 回测环境示例
add_executable(demo-evaluate "evaluate_demo.cpp" ${STRATEGY_DIR})
target_link_libraries(demo-evaluate "latf-core-v3x")

# 实盘/仿真运行示例
add_executable(demo-runtime "runtime_demo.cpp" ${STRATEGY_DIR})
target_link_libraries(demo-runtime "latf-core-v3x")

# 手动下单或者第三放干预策略示例
add_executable(demo-manual "manual_demo.cpp" ${STRATEGY_DIR})
target_link_libraries(demo-manual "latf-core-v3x")

# 策略运行器分离（动态链接策略）示例
add_executable(demo-dlstgy "dlstgy_demo.cpp")
target_link_libraries(demo-dlstgy "latf-core-v3x")

add_library(stgy-marketing SHARED 
    "strategy/marketing_strategy.cpp"
    )
target_link_libraries(stgy-marketing "latf-core-v3x")

# LT数据服务示例
add_executable(demo-ltds "ltds_demo.cpp")


# TODO: 如有需要，请添加测试并安装目标。

