include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})

#SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build_${PLATFORM}/${CMAKE_BUILD_TYPE}/bin)
AUX_SOURCE_DIRECTORY(tick_loader   TICK_LOADER_DIR_VAR)

add_library(simulator STATIC "tick_simulator.cpp" "order_container.cpp" "position_container.cpp" "contract_parser.cpp" "interface.cpp" ${TICK_LOADER_DIR_VAR})
