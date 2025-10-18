# CMake configuration for Foundry Game Engine Test Suite

cmake_minimum_required(VERSION 3.16)
project(FoundryEngineTests VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Find required packages
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${GTEST_INCLUDE_DIRS}
)

# Source files
set(TEST_SOURCES
    TestRunner.cpp
    TestMemoryPool.cpp
    TestAssetSystem.cpp
    TestSerializationSystem.cpp
    TestPlatformInterface.cpp
    TestCoreSystems.cpp
    TestEngineCoreComplete.cpp
    TestPlatformSpecificComplete.cpp
    TestGraphicsSystems.cpp
    TestPhysicsSystems.cpp
    TestMathSystems.cpp
    TestAISystems.cpp
    TestAnimationSystems.cpp
    TestNetworkSystems.cpp
    TestPlatformSystems.cpp
    TestOptimizationSystems.cpp
    TestWorldSystems.cpp
)

# Engine source files needed for tests
set(ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/MemoryPool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/SerializationSystem.cpp
)

# Create test executable
add_executable(FoundryEngineTests ${TEST_SOURCES} ${ENGINE_SOURCES})

# Link libraries
target_link_libraries(FoundryEngineTests
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Compiler flags
target_compile_options(FoundryEngineTests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -Wno-unused-variable
)

# Define for tests
target_compile_definitions(FoundryEngineTests PRIVATE
    FOUNDRY_ENGINE_TESTS
    $<$<CONFIG:Debug>:FOUNDRY_ENGINE_DEBUG>
    $<$<CONFIG:Release>:FOUNDRY_ENGINE_RELEASE>
)

# Output directory
set_target_properties(FoundryEngineTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
)

# Custom test target
add_custom_target(run_tests
    COMMAND FoundryEngineTests
    DEPENDS FoundryEngineTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    COMMENT "Running Foundry Game Engine test suite"
)

# Performance test target
add_custom_target(run_performance_tests
    COMMAND FoundryEngineTests --gtest_filter="*Performance*"
    DEPENDS FoundryEngineTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    COMMENT "Running performance tests only"
)

# Thread safety test target
add_custom_target(run_thread_safety_tests
    COMMAND FoundryEngineTests --gtest_filter="*Thread*"
    DEPENDS FoundryEngineTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    COMMENT "Running thread safety tests only"
)

# Memory test target
add_custom_target(run_memory_tests
    COMMAND FoundryEngineTests --gtest_filter="*Memory*"
    DEPENDS FoundryEngineTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
    COMMENT "Running memory management tests only"
)
# Install test executable
install(TARGETS FoundryEngineTests
    RUNTIME DESTINATION bin/tests
)

# Generate test report
add_custom_target(generate_test_report
    COMMAND ${CMAKE_COMMAND} -E echo "Generating test report..."
    COMMAND FoundryEngineTests --gtest_output=xml:${CMAKE_BINARY_DIR}/test_results.xml
    DEPENDS FoundryEngineTests
    COMMENT "Generating XML test report"
)

# Print test summary
add_custom_target(test_summary
    COMMAND ${CMAKE_COMMAND} -E echo "========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "  Foundry Game Engine - Test Summary"
    COMMAND ${CMAKE_COMMAND} -E echo "========================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Test executable: $<TARGET_FILE:FoundryEngineTests>"
    COMMAND ${CMAKE_COMMAND} -E echo "Run all tests: make run_tests"
    COMMAND ${CMAKE_COMMAND} -E echo "Run performance tests: make run_performance_tests"
    COMMAND ${CMAKE_COMMAND} -E echo "Run thread safety tests: make run_thread_safety_tests"
    COMMAND ${CMAKE_COMMAND} -E echo "Run memory tests: make run_memory_tests"
    COMMAND ${CMAKE_COMMAND} -E echo "Generate test report: make generate_test_report"
    COMMAND ${CMAKE_COMMAND} -E echo "========================================="
    DEPENDS FoundryEngineTests
    COMMENT "Display test summary information"
)

