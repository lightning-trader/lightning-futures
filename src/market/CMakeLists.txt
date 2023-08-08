    
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} 
        "${PROJECT_SOURCE_DIR}/../api/CTP_V6.6.9_20220920/" 
        )
include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(lt_market STATIC "ctp_market.cpp"  "interface.cpp")

