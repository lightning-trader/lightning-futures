
if (UNIX)
    set_target_properties(${MODULE_NAME}
        PROPERTIES
        PREFIX ""
    )
elseif (WIN32)
    set_target_properties(${MODULE_NAME}
        PROPERTIES
        SUFFIX ".pyd"
    )
endif()


SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} 
    "C:/Users/ghuaz/AppData/Local/Programs/Python/Python310/include"
)
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} 
    "C:/Users/ghuaz/AppData/Local/Programs/Python/Python310/libs" 
)

message(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}")

message(CMAKE_LIBRARY_PATH "${CMAKE_LIBRARY_PATH}")

include_directories(${CMAKE_INCLUDE_PATH})

link_directories(${CMAKE_LIBRARY_PATH})

add_library(ltpy SHARED "ltpy.cpp" "ltpy.h")

