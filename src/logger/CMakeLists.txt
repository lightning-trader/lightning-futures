
include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(liblt-logger-v3x SHARED "log_wapper.cpp" "nanolog.cpp")
