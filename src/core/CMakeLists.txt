
include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})
#AUX_SOURCE_DIRECTORY("driver"   DRIVER_DIR_VAR)
#target_sources(${target} PRIVATE ${A1_DIR_VAR})
add_library(lightning SHARED "runtime_context.cpp" "evaluate_context.cpp" "pod_chain.cpp" "context.cpp"  "lightning.cpp")

target_link_libraries(lightning "market" "simulator" "trader")
