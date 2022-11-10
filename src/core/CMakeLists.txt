
include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})
AUX_SOURCE_DIRECTORY("driver"   DRIVER_DIR_VAR)
#target_sources(${target} PRIVATE ${A1_DIR_VAR})
add_library(lightning SHARED "strategy.cpp" "pod_chain.cpp" "context.cpp"  ${DRIVER_DIR_VAR} "lightning.cpp")

target_link_libraries(lightning "market" "simulator" "trader")
