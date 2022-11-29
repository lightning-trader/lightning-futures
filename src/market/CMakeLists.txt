
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} 
    "${PROJECT_SOURCE_DIR}/api/ctp_6.6.8_t1"
)

SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
    "${PROJECT_SOURCE_DIR}/api/ctp_6.6.8_t1"
)

include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(market STATIC "ctp_market.cpp"  "interface.cpp")

