
include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(lt_persist SHARED "mmf_wapper.cpp")

target_link_libraries(lt_persist "lt_loger")
