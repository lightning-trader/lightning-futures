include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})
add_library(recorder STATIC "csv_recorder.cpp" "interface.cpp" "csv_recorder.h")
