
message("TRADER INCLUDE PATH -> ${CMAKE_INCLUDE_PATH}")


include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(lt_trader SHARED "ctp_api_trader.cpp" "ctp_mini_api_trader.cpp" "ctp_api_trader.cpp" "interface.cpp")

target_link_libraries(lt_trader "lt_loger" ${TRADER_LIBS})