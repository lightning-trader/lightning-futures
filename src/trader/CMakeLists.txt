
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} 
    "${PROJECT_SOURCE_DIR}/api/ctp_6.6.8" 
)
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
    "${PROJECT_SOURCE_DIR}/api/ctp_6.6.8"
)

include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(trader STATIC "ctp_trader.cpp" "interface.cpp")
