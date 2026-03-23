# Lightning Futures Test CMakeLists.txt
# 用于编译和运行单元测试

cmake_minimum_required(VERSION 3.10)
project(lightning_futures_tests)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/../src/include
    ${CMAKE_SOURCE_DIR}/../src/share
    ${CMAKE_SOURCE_DIR}/../src/core
    ${CMAKE_SOURCE_DIR}/../src/simulator
    ${CMAKE_SOURCE_DIR}/../src/framework
    ${CMAKE_SOURCE_DIR}/../src/logger
)

# 源文件目录
set(SRC_DIR ${CMAKE_SOURCE_DIR}/../src)

# 公共源文件
set(COMMON_SOURCES
    ${SRC_DIR}/core/time_section.cpp
    ${SRC_DIR}/core/bar_generator.cpp
    ${SRC_DIR}/core/trading_context.cpp
    ${SRC_DIR}/core/data_channel.cpp
    ${SRC_DIR}/logger/log_context.cpp
    ${SRC_DIR}/logger/nanolog.cpp
    ${SRC_DIR}/simulator/contract_parser.cpp
    ${SRC_DIR}/simulator/market_simulator.cpp
    ${SRC_DIR}/simulator/trader_simulator.cpp
    ${SRC_DIR}/simulator/tick_loader/csv_tick_loader.cpp
    ${SRC_DIR}/simulator/tick_loader/ltds_tick_loader.cpp
    ${SRC_DIR}/adapter/market/ctp_api_market.cpp
    ${SRC_DIR}/adapter/market/tap_api_market.cpp
    ${SRC_DIR}/adapter/trader/ctp_api_trader.cpp
    ${SRC_DIR}/adapter/trader/tap_api_trader.cpp
    ${SRC_DIR}/adapter/interface.cpp
)

# 测试可执行文件列表
set(TEST_SOURCES
    test_basic_types.cpp
    test_basic_types_edge.cpp
    test_basic_utils.cpp
    test_memory_pool.cpp
    test_params.cpp
    test_time_section.cpp
    test_trading_context.cpp
    test_trader_simulator.cpp
    test_trader_simulator_extended.cpp
    test_fix.cpp
    test_position_fix.cpp
    test_simulator_fix.cpp
    test_compiler.cpp
)

# 为每个测试文件创建可执行文件
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    
    # 链接必要的库
    target_link_libraries(${test_name} 
        ${CMAKE_SOURCE_DIR}/../src/core/bar_generator.cpp
        ${CMAKE_SOURCE_DIR}/../src/core/time_section.cpp
        ${CMAKE_SOURCE_DIR}/../src/core/trading_context.cpp
        ${CMAKE_SOURCE_DIR}/../src/core/data_channel.cpp
        ${CMAKE_SOURCE_DIR}/../src/logger/log_context.cpp
        ${CMAKE_SOURCE_DIR}/../src/logger/nanolog.cpp
        ${CMAKE_SOURCE_DIR}/../src/simulator/contract_parser.cpp
        ${CMAKE_SOURCE_DIR}/../src/simulator/market_simulator.cpp
        ${CMAKE_SOURCE_DIR}/../src/simulator/trader_simulator.cpp
    )
endforeach()

# 添加自定义目标以运行所有测试
add_custom_target(run_tests
    COMMAND ${CMAKE_COMMAND} -E echo "Running all tests..."
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests"
)

# 为每个测试添加后构建命令
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_custom_command(TARGET run_tests POST_BUILD
        COMMAND ${CMAKE_BINARY_DIR}/${test_name}
        COMMENT "Running ${test_name}..."
    )
endforeach()