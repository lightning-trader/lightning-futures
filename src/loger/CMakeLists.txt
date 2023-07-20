
include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(lt_loger SHARED "log_wapper.cpp" "nanolog.cpp")
