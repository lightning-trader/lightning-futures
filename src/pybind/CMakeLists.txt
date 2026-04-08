include_directories(${CMAKE_INCLUDE_PATH} "./" "../../api/LTD_V3_20260407")
link_directories(${CMAKE_LIBRARY_PATH})

find_package(Python 3.9 REQUIRED COMPONENTS Interpreter Development.Module OPTIONAL_COMPONENTS Development.SABIModule)

set(NB_USE_STABLE_ABI ON CACHE BOOL "Enable Stable ABI for cross-version compatibility" FORCE)

FetchContent_Declare(
    nanobind
    GIT_REPOSITORY https://github.com/wjakob/nanobind.git
    GIT_TAG        v2.12.0
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(nanobind)

nanobind_add_module(lightures
    STABLE_ABI
    "bridge.cpp"
    "pyapi.cpp"
)

target_include_directories(lightures PRIVATE "./")
target_link_libraries(lightures PRIVATE
    profitnest::common
    "latf-core-v3x"
    "latf-adapter-v3x"
)

# Generate stub file (.pyi) for the module
if(NANOBIND_STUBGEN_FOUND)
    nanobind_stubgen(lightures OUTPUT_NAME lightures)
endif()
