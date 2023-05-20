include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})
add_library(lt_recorder STATIC "csv_recorder.cpp" "recorder_proxy.cpp" "interface.cpp")
