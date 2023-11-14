include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(framework STATIC "bar_generator.cpp" "price_step.cpp" "engine.cpp" "evaluate_engine.cpp" "runtime_engine.cpp" "strategy.cpp")

target_link_libraries(framework "lightning_core" "lightning_loger" "lightning_persist")
