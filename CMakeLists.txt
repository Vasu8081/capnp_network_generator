cmake_minimum_required(VERSION 3.16)

project(capnp_generator VERSION 0.1.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Collect sources: main.cpp at root; other sources under src/
set(MAIN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# If you prefer listing files explicitly, replace the GLOB with a manual list.
file(GLOB_RECURSE LIB_SRCS
     CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(capnp_generator
    ${MAIN_SRC}
    ${LIB_SRCS}
)

# Headers live under src/
target_include_directories(capnp_generator
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# (Optional) Put the runtime next to build dir’s /bin for easy discovery
set_target_properties(capnp_generator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install
install(TARGETS capnp_generator RUNTIME DESTINATION bin)

# CPack packaging
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_GENERATOR "TGZ;ZIP")
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}"
)
include(CPack)

# Optional: respect user-provided clang-tidy if set
# Example usage: cmake -DCMAKE_CXX_CLANG_TIDY=clang-tidy ..
if(CMAKE_CXX_CLANG_TIDY)
    message(STATUS "clang-tidy enabled: ${CMAKE_CXX_CLANG_TIDY}")
endif()
