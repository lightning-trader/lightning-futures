
message("TRADER INCLUDE PATH -> ${CMAKE_INCLUDE_PATH}")


if(APPLE)
    set(LIBS_PATH "${PROJECT_SOURCE_DIR}/../api/CTP_V6.6.9_20220920/macos")
    SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
        "${PROJECT_SOURCE_DIR}/../api/CTP_V6.6.9_20220920/macos/"
    )
    find_library(comunicationkey_lib NAMES comunicationkey PATHS ${LIBS_PATH}/libs NO_DEFAULT_PATH)
    find_library(crypto_lib NAMES crypto PATHS ${LIBS_PATH}/libs NO_DEFAULT_PATH)
    find_library(ssl_lib NAMES ssl PATHS ${LIBS_PATH}/libs NO_DEFAULT_PATH)
    find_library(thosttraderapi_mac_lib NAMES thosttraderapi_mac PATHS ${LIBS_PATH} NO_DEFAULT_PATH)
    SET(TRADER_LIBS ${comunicationkey_lib} ${crypto_lib} ${ssl_lib} ${thosttraderapi_mac_lib})

endif()

include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(lt_trader SHARED "ctp_trader.cpp" "ctp_mini_trader.cpp" "tap_trader.cpp" "interface.cpp")

target_link_libraries(lt_trader "lt_loger" ${TRADER_LIBS})