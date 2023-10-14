include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})

#SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build_${PLATFORM}/${CMAKE_BUILD_TYPE}/bin)
aux_source_directory(tick_loader   TICK_LOADER_DIR)

add_library(lightning_simulator SHARED "market_simulator.cpp" "trader_simulator.cpp" "contract_parser.cpp" "interface.cpp" ${TICK_LOADER_DIR})

target_link_libraries(lightning_simulator "lightning_loger")