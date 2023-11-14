
include_directories(${CMAKE_INCLUDE_PATH})


link_directories(${CMAKE_LIBRARY_PATH})

add_library(lightning_core SHARED "runtime.cpp" "evaluate.cpp" "pod_chain.cpp" "context.cpp"  "lightning.cpp" "csv_recorder.cpp" "trading_section.cpp")

target_link_libraries(lightning_core "lightning_loger" "lightning_persist" "lightning_adapter" "lightning_simulator" ${SYS_LIBS})
