cmake_minimum_required(VERSION 3.16)
project(FoundryEngineJNI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find JNI
find_package(JNI REQUIRED)

# Include directories
include_directories(${JNI_INCLUDE_DIRS})
include_directories(../../include)
include_directories(.)

# Source files
set(SOURCES
    FoundryEngineJNI.cpp
)

# Create shared library
add_library(FoundryEngineJNI SHARED ${SOURCES})

# Link libraries
target_link_libraries(FoundryEngineJNI ${JNI_LIBRARIES})

# Set output properties
set_target_properties(FoundryEngineJNI PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Platform-specific settings
if(WIN32)
    set_target_properties(FoundryEngineJNI PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
    )
elseif(APPLE)
    set_target_properties(FoundryEngineJNI PROPERTIES
        PREFIX "lib"
        SUFFIX ".dylib"
    )
else() # Linux
    set_target_properties(FoundryEngineJNI PROPERTIES
        PREFIX "lib"
        SUFFIX ".so"
    )
endif()

# Installation
install(TARGETS FoundryEngineJNI
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

message(STATUS "Building JNI bridge for Foundry Engine")
message(STATUS "JNI Include: ${JNI_INCLUDE_DIRS}")
message(STATUS "JNI Libraries: ${JNI_LIBRARIES}")
