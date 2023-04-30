    
IF(WIN32)
    IF(CMAKE_CL_64)
        SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} 
            "${PROJECT_SOURCE_DIR}/../api/v6.6.9_traderapi_20220920/v6.6.9_20220914_winApi/traderapi/20220914_traderapi64_se_windows/" 
        )
    ELSE(CMAKE_CL_64)
        SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} 
            "${PROJECT_SOURCE_DIR}/../api/v6.6.9_traderapi_20220920/v6.6.9_20220914_winApi/traderapi/20220914_traderapi_se_windows/" 
        )
    ENDIF(CMAKE_CL_64)
ELSE(WIN32)
    IF(CMAKE_CL_64)
        SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} 
        "${PROJECT_SOURCE_DIR}/../api/v6.6.9_traderapi_20220920/v6.6.9_20220914_api_linux64/" 
        )
    ELSE(CMAKE_CL_64)
        SET(CMAKE_INCLUDE_PATH 
            ${CMAKE_INCLUDE_PATH} 
            "${PROJECT_SOURCE_DIR}/../api/v6.6.9_traderapi_20220920/v6.6.9_20220914_api_linux64/" 
        )
    ENDIF(CMAKE_CL_64)
ENDIF(WIN32)

message("TRADER INCLUDE PATH -> ${CMAKE_INCLUDE_PATH}")

include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(trader STATIC "ctp_trader.cpp" "interface.cpp")
