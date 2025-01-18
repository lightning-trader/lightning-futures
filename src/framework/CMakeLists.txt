include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(framework STATIC "bar_generator.cpp" "price_step.cpp" "engine.cpp" "evaluate_engine.cpp" "runtime_engine.cpp" "strategy.cpp" "context.cpp"  "csv_recorder.cpp" "trading_section.cpp")

target_link_libraries(framework "liblt-adapter-v3x" "liblt-simulator-v3x" ${SYS_LIBS})
