    
 SET(CMAKE_INCLUDE_PATH 
      ${CMAKE_INCLUDE_PATH} 
      "${PROJECT_SOURCE_DIR}/../api/CTPv6.6.9_traderapi_20220920/" 
 )

message("TRADER INCLUDE PATH -> ${CMAKE_INCLUDE_PATH}")

include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(lt_trader STATIC "ctp_trader.cpp" "interface.cpp")
