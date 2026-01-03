include_directories(${CMAKE_INCLUDE_PATH} "./" "./tick_loader" "../../api/LTD_V3_20260103")
link_directories(${CMAKE_LIBRARY_PATH})

aux_source_directory(tick_loader  TICK_LOADER_DIR)

add_library(latf-simulator-v3x SHARED "market_simulator.cpp" "trader_simulator.cpp" "contract_parser.cpp" "interface.cpp" ${TICK_LOADER_DIR})

