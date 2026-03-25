
include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(latf-logger-v3x SHARED "log_context.cpp" "nanolog.cpp")

if(WIN32)
    target_link_libraries(latf-logger-v3x dbghelp)
elseif(UNIX)
    target_link_libraries(latf-logger-v3x ${SYS_LIBS} dl)
endif()
