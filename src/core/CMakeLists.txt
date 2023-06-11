
include_directories(${CMAKE_INCLUDE_PATH})

IF(UNIX)
    SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
        "${PROJECT_SOURCE_DIR}/../api/v6.6.9_traderapi_20220920/v6.6.9_20220914_api_linux64/" 
    )
ENDIF()

link_directories(${CMAKE_LIBRARY_PATH})
#AUX_SOURCE_DIRECTORY("driver"   DRIVER_DIR_VAR)
#target_sources(${target} PRIVATE ${A1_DIR_VAR})
add_library(lightning SHARED "runtime.cpp" "evaluate.cpp" "pod_chain.cpp" "context.cpp"  "lightning.cpp" "log_wapper.cpp" "trading_section.cpp" "bar_generator.cpp" "delayed_distributor.cpp" "csv_recorder.cpp")

target_link_libraries(lightning "market" "simulator" "trader" ${SYS_LIBS} ${BOOST_LIBS} ${CTP_LIBS})
