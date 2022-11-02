
include_directories(${CMAKE_INCLUDE_PATH})
link_directories(${CMAKE_LIBRARY_PATH})
AUX_SOURCE_DIRECTORY("driver"   DRIVER_DIR_VAR)
AUX_SOURCE_DIRECTORY("dll_mgr"   DLL_MGR_DIR_VAR)
#target_sources(${target} PRIVATE ${A1_DIR_VAR})
add_library(core STATIC "strategy.cpp" "rl_env.cpp" "pod_chain.cpp" "exec_ctx.cpp" "condition.cpp" ${DRIVER_DIR_VAR} ${DLL_MGR_DIR_VAR})
