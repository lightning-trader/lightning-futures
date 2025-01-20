
include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(latf-logger-v3x SHARED "log_context.cpp" "nanolog.cpp")
