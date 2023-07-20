include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(framework STATIC "engine.cpp" "evaluate_engine.cpp" "runtime_engine.cpp" "strategy.cpp")

target_link_libraries(framework "lt_core" "lt_loger" "lt_persist")
