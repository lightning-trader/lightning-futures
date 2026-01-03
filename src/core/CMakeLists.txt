include_directories(${CMAKE_INCLUDE_PATH} "../include" "../../api/LTD_V3_20260103")

link_directories(${CMAKE_LIBRARY_PATH})


add_library(latf-core-v3x STATIC 
	"bar_generator.cpp" 
	"data_channel.cpp" 
	"trading_context.cpp"
	"time_section.cpp"
	)

target_link_libraries(latf-core-v3x "latf-adapter-v3x" "latf-simulator-v3x")