
include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(lightning_persist SHARED "mmf_wapper.cpp")

target_link_libraries(lightning_persist "lightning_loger")
