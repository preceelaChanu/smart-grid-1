cmake_minimum_required(VERSION 3.16)
project(SmartGridBenchmark)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Microsoft.SEAL QUIET)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# If SEAL not found via CMake, try manual discovery
if(NOT Microsoft.SEAL_FOUND)
    message(STATUS "SEAL not found via find_package, attempting manual discovery...")
    
    # Look for SEAL in common installation directories
    find_path(SEAL_INCLUDE_DIR 
        NAMES seal/seal.h
        PATHS /usr/local/include/SEAL-4.0 /usr/local/include /usr/include
    )
    
    find_library(SEAL_LIBRARY 
        NAMES seal-4.0 seal
        PATHS /usr/local/lib /usr/local/lib64 /usr/lib /usr/lib64
    )
    
    if(SEAL_INCLUDE_DIR AND SEAL_LIBRARY)
        message(STATUS "Found SEAL manually: ${SEAL_LIBRARY}")
        add_library(Microsoft.SEAL::seal UNKNOWN IMPORTED)
        set_target_properties(Microsoft.SEAL::seal PROPERTIES
            IMPORTED_LOCATION ${SEAL_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${SEAL_INCLUDE_DIR}
        )
        set(Microsoft.SEAL_FOUND TRUE)
    else()
        message(FATAL_ERROR "Microsoft SEAL not found. Please install SEAL library.")
    endif()
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/network)

# Add network utilities library
add_library(network_utils network/network_utils.cpp)
target_link_libraries(network_utils OpenSSL::SSL OpenSSL::Crypto)

# Add executable targets
add_executable(keygen keygen/keygen.cpp)
add_executable(client client/client.cpp)
add_executable(aggregator aggregator/aggregator.cpp)
add_executable(control_center control_center/control_center.cpp)
add_executable(certgen certgen/certgen.cpp)

# Link Microsoft SEAL library to all targets
target_link_libraries(keygen Microsoft.SEAL::seal)
target_link_libraries(client Microsoft.SEAL::seal network_utils Threads::Threads)
target_link_libraries(aggregator Microsoft.SEAL::seal network_utils Threads::Threads)
target_link_libraries(control_center Microsoft.SEAL::seal network_utils Threads::Threads)
target_link_libraries(certgen network_utils)

# Set compiler flags for performance and debugging
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra")

# Optional: Set debug flags for development
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DDEBUG")
endif()

# Create output directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/keys)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)