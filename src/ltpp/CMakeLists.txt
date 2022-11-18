include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(libltpp STATIC "evaluate_engine.cpp" "runtime_engine.cpp" "strategy.cpp" "strategy_manager.cpp")

target_link_libraries(libltpp "lightning")
