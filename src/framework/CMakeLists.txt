include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(framework STATIC "bar_generator.cpp" "price_step.cpp" "engine.cpp" "evaluate_engine.cpp" "runtime_engine.cpp" "strategy.cpp" "context.cpp"  "csv_recorder.cpp" "trading_section.cpp")

target_link_libraries(framework "lightning_loger" "lightning_adapter" "lightning_simulator" )
