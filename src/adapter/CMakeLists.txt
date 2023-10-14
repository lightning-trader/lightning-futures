
include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

aux_source_directory(market MARKET_DIR)
aux_source_directory(trader TRADER_DIR)

if(APPLE)
    set(LIBS_PATH "${PROJECT_SOURCE_DIR}/../api/CTP_V6.6.9_20220920/macos")
    SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
        "${PROJECT_SOURCE_DIR}/../api/CTP_V6.6.9_20220920/macos/"
    )
    find_library(comunicationkey_lib NAMES comunicationkey PATHS ${LIBS_PATH}/libs NO_DEFAULT_PATH)
    find_library(crypto_lib NAMES crypto PATHS ${LIBS_PATH}/libs NO_DEFAULT_PATH)
    find_library(ssl_lib NAMES ssl PATHS ${LIBS_PATH}/libs NO_DEFAULT_PATH)
    find_library(thostmduserapi_mac_lib NAMES thostmduserapi_mac PATHS ${LIBS_PATH} NO_DEFAULT_PATH)
    SET(MARKET_LIBS ${comunicationkey_lib} ${crypto_lib} ${ssl_lib} ${thostmduserapi_mac_lib})

endif()

add_library(lightning_adapter SHARED "interface.cpp" ${MARKET_DIR} ${TRADER_DIR})

target_link_libraries(lightning_adapter "lightning_loger" ${MARKET_LIBS})