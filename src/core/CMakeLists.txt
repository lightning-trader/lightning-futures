
include_directories(${CMAKE_INCLUDE_PATH})

IF(UNIX)
    SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
        "${PROJECT_SOURCE_DIR}/../api/CTP_V6.6.9_20220920/linux64/" 
    )
ENDIF()
link_directories(${CMAKE_LIBRARY_PATH})
#AUX_SOURCE_DIRECTORY("driver"   DRIVER_DIR_VAR)
#target_sources(${target} PRIVATE ${A1_DIR_VAR})
add_library(lt_core SHARED "runtime.cpp" "evaluate.cpp" "pod_chain.cpp" "context.cpp"  "lightning.cpp" "trading_section.cpp" "bar_generator.cpp" "delayed_distributor.cpp" "csv_recorder.cpp")

target_link_libraries(lt_core "lt_market" "lt_simulator" "lt_trader" "lt_loger" "lt_persist" ${SYS_LIBS} ${CTP_LIBS})
