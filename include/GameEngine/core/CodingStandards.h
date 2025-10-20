#pragma once

/**
 * @file CodingStandards.h
 * @brief Foundry Game Engine Coding Standards and Best Practices
 *
 * This file establishes consistent naming conventions, coding patterns,
 * and best practices for the Foundry Game Engine codebase.
 */

namespace FoundryEngine {

/**
 * @defgroup CodingStandards Coding Standards and Conventions
 * @brief Consistent coding patterns and naming conventions
 * @{
 */

/**
 * @defgroup NamingConventions Naming Conventions
 * @brief Standardized naming rules for consistency
 * @{
 */

// ==================== NAMING CONVENTIONS ====================

/**
 * @name Class and Struct Names
 * @brief Rules for class and struct identifiers
 * @{
 */
/**
 * @brief Use PascalCase for class names
 *
 * Classes should use PascalCase (also known as UpperCamelCase).
 * Examples: MemoryPool, AssetManager, Vector3, Matrix4
 */
#define CLASS_NAME_EXAMPLE MemoryPool

/**
 * @brief Use PascalCase for struct names
 *
 * Structs should use PascalCase, same as classes.
 * Examples: AssetMetadata, PlatformCapabilities, HandleInfo
 */
#define STRUCT_NAME_EXAMPLE AssetMetadata

/**
 * @brief Use PascalCase for enum names
 *
 * Enum types should use PascalCase.
 * Examples: AssetType, PlatformHandleType, LightType
 */
#define ENUM_NAME_EXAMPLE AssetType

/**
 * @brief Use PascalCase for enum values
 *
 * Enum values should use PascalCase for consistency.
 * Examples: AssetType::Texture, LightType::Directional
 */
#define ENUM_VALUE_EXAMPLE AssetType::Texture
/** @} */

/**
 * @name Function and Method Names
 * @brief Rules for function and method identifiers
 * @{
 */
/**
 * @brief Use camelCase for function names
 *
 * Functions should use camelCase (starting with lowercase).
 * Examples: loadAsset(), createWindow(), getMemoryUsage()
 */
#define FUNCTION_NAME_EXAMPLE loadAsset()

/**
 * @brief Use camelCase for method names
 *
 * Methods should use camelCase, same as functions.
 * Examples: isValid(), getNative(), setWindowTitle()
 */
#define METHOD_NAME_EXAMPLE isValid()

/**
 * @brief Use camelCase for lambda functions
 *
 * Lambda functions should use camelCase.
 * Examples: auto loadAsset = []() { ... };
 */
#define LAMBDA_NAME_EXAMPLE loadAsset
/** @} */

/**
 * @name Variable Names
 * @brief Rules for variable identifiers
 * @{
 */
/**
 * @brief Use camelCase for local variables
 *
 * Local variables should use camelCase.
 * Examples: int frameCount, float deltaTime, Vector3 playerPosition
 */
#define LOCAL_VAR_EXAMPLE frameCount

/**
 * @brief Use camelCase for member variables
 *
 * Class member variables should use camelCase.
 * Examples: MemoryPool* memoryPool_, size_t blockSize_, bool isLoaded_
 */
#define MEMBER_VAR_EXAMPLE memoryPool_

/**
 * @brief Use camelCase for parameter names
 *
 * Function parameters should use camelCase.
 * Examples: void loadAsset(const std::string& assetPath)
 */
#define PARAMETER_EXAMPLE assetPath

/**
 * @brief Use UPPER_CASE for constants
 *
 * Compile-time constants should use UPPER_CASE with underscores.
 * Examples: const int MAX_TEXTURE_SIZE = 4096;
 */
#define CONSTANT_EXAMPLE MAX_TEXTURE_SIZE

/**
 * @brief Use kPrefix for global constants
 *
 * Global constants should use k prefix with PascalCase.
 * Examples: const float kPi = 3.14159f;
 */
#define GLOBAL_CONSTANT_EXAMPLE kPi
/** @} */

/**
 * @name Template Parameters
 * @brief Rules for template parameter names
 * @{
 */
/**
 * @brief Use PascalCase for template type parameters
 *
 * Template type parameters should use PascalCase.
 * Examples: template<typename TAsset>, template<typename THandle>
 */
#define TEMPLATE_TYPE_EXAMPLE TAsset

/**
 * @brief Use camelCase for template non-type parameters
 *
 * Template non-type parameters should use camelCase.
 * Examples: template<size_t Size>, template<int MaxCount>
 */
#define TEMPLATE_NON_TYPE_EXAMPLE Size
/** @} */

/**
 * @name Namespace Names
 * @brief Rules for namespace identifiers
 * @{
 */
/**
 * @brief Use PascalCase for namespace names
 *
 * Namespace names should use PascalCase.
 * Examples: namespace FoundryEngine, namespace Serialization
 */
#define NAMESPACE_EXAMPLE FoundryEngine

/**
 * @brief Use nested namespaces for organization
 *
 * Use nested namespaces to organize related functionality.
 * Examples: FoundryEngine::Core, FoundryEngine::Systems
 */
#define NESTED_NAMESPACE_EXAMPLE FoundryEngine::Core
/** @} */

/**
 * @name File Names
 * @brief Rules for file names
 * @{
 */
/**
 * @brief Use PascalCase for header files
 *
 * Header files should use PascalCase.
 * Examples: MemoryPool.h, AssetManager.h, Vector3.h
 */
#define HEADER_FILE_EXAMPLE MemoryPool.h

/**
 * @brief Use PascalCase for source files
 *
 * Source files should use PascalCase.
 * Examples: MemoryPool.cpp, AssetManager.cpp, Vector3.cpp
 */
#define SOURCE_FILE_EXAMPLE MemoryPool.cpp

/**
 * @brief Use descriptive names for files
 *
 * Files should have descriptive names that match their primary class/struct.
 * Examples: SerializationSystem.h (not just "serial.h")
 */
#define DESCRIPTIVE_FILE_EXAMPLE SerializationSystem.h
/** @} */

/**
 * @name Macro Names
 * @brief Rules for macro identifiers
 * @{
 */
/**
 * @brief Use UPPER_CASE for macro names
 *
 * Macros should use UPPER_CASE with underscores.
 * Examples: #define MAX_BUFFER_SIZE, #define ENABLE_DEBUG_LOGGING
 */
#define MACRO_EXAMPLE MAX_BUFFER_SIZE

/**
 * @brief Use descriptive names for macros
 *
 * Macros should have descriptive names that explain their purpose.
 * Examples: #define ASSERT_VALID_HANDLE(handle)
 */
#define DESCRIPTIVE_MACRO_EXAMPLE ASSERT_VALID_HANDLE
/** @} */

/**
 * @name Type Alias Names
 * @brief Rules for type alias identifiers
 * @{
 */
/**
 * @brief Use PascalCase for type aliases
 *
 * Type aliases should use PascalCase.
 * Examples: using AssetPtr = std::unique_ptr<Asset>;
 */
#define TYPE_ALIAS_EXAMPLE AssetPtr

/**
 * @brief Use descriptive names for type aliases
 *
 * Type aliases should have descriptive names that indicate their purpose.
 * Examples: using WindowHandle = PlatformHandle<PlatformHandleType::Window>;
 */
#define DESCRIPTIVE_TYPE_ALIAS_EXAMPLE WindowHandle
/** @} */

/** @} */ // End of NamingConventions group

/**
 * @defgroup CodeStyle Code Style Guidelines
 * @brief Consistent code formatting and style rules
 * @{
 */

// ==================== CODE STYLE GUIDELINES ====================

/**
 * @name Indentation and Bracing
 * @brief Rules for code formatting
 * @{
 */
/**
 * @brief Use 4 spaces for indentation
 *
 * All code should use 4 spaces for indentation (no tabs).
 */
#define INDENTATION_EXAMPLE "    // 4 spaces"

/**
 * @brief Use Allman bracing style
 *
 * Braces should be on separate lines, aligned with the control statement.
 * Examples:
 * if (condition)
 * {
 *     // code
 * }
 */
#define BRACING_EXAMPLE if (condition)\n{\n    // code\n}

/**
 * @brief Use consistent spacing around operators
 *
 * Operators should have consistent spacing.
 * Examples: a = b + c, if (x == y), function(param1, param2)
 */
#define SPACING_EXAMPLE "a = b + c"
/** @} */

/**
 * @name Comments and Documentation
 * @brief Rules for code comments and documentation
 * @{
 */
/**
 * @brief Use Doxygen-style comments for public APIs
 *
 * Public classes, methods, and functions should use Doxygen comments.
 * Format: /** @brief Brief description */
 */
#define DOXYGEN_COMMENT_EXAMPLE "/** @brief Brief description */"

/**
 * @brief Use // for single-line comments
 *
 * Single-line comments should use // style.
 */
#define SINGLE_LINE_COMMENT_EXAMPLE "// This is a comment"

/**
 * @brief Use /* */ for multi-line comments
 *
 * Multi-line comments should use /* */ style.
 */
#define MULTI_LINE_COMMENT_EXAMPLE "/*\n * Multi-line comment\n */"

/**
 * @brief Comment complex logic and algorithms
 *
 * Complex business logic should be explained with comments.
 */
#define LOGIC_COMMENT_EXAMPLE "// Use binary search for O(log n) lookup"
/** @} */

/**
 * @name Function Design
 * @brief Rules for function design and organization
 * @{
 */
/**
 * @brief Keep functions small and focused
 *
 * Functions should do one thing and do it well (Single Responsibility Principle).
 */
#define SMALL_FUNCTION_EXAMPLE "bool loadAsset(const std::string& path)"

/**
 * @brief Use descriptive parameter names
 *
 * Parameter names should clearly indicate their purpose.
 */
#define DESCRIPTIVE_PARAMS_EXAMPLE "void setWindowSize(int width, int height)"

/**
 * @brief Limit function parameters to 5 or fewer
 *
 * Functions with many parameters should be refactored.
 */
#define LIMITED_PARAMS_EXAMPLE "void createWindow(const std::string& title, int width, int height)"

/**
 * @brief Use const correctness
 *
 * Use const for parameters and methods that don't modify state.
 */
#define CONST_CORRECTNESS_EXAMPLE "const std::string& getName() const"
/** @} */

/**
 * @name Class Design
 * @brief Rules for class design and organization
 * @{
 */
/**
 * @brief Use RAII for resource management
 *
 * Classes should manage resources using RAII (Resource Acquisition Is Initialization).
 */
#define RAII_EXAMPLE "std::unique_ptr<Resource> resource_"

/**
 * @brief Use composition over inheritance when possible
 *
 * Prefer composition for code reuse over deep inheritance hierarchies.
 */
#define COMPOSITION_EXAMPLE "class AssetManager { MemoryPool* memoryPool_; }"

/**
 * @brief Make classes non-copyable by default
 *
 * Classes should disable copy operations unless copying is intentional.
 */
#define NON_COPYABLE_EXAMPLE "Class(const Class&) = delete;"

/**
 * @brief Use smart pointers for ownership
 *
 * Use std::unique_ptr and std::shared_ptr for clear ownership semantics.
 */
#define SMART_POINTERS_EXAMPLE "std::unique_ptr<Asset> asset_"
/** @} */

/**
 * @name Error Handling
 * @brief Rules for error handling patterns
 * @{
 */
/**
 * @brief Use exceptions for exceptional conditions
 *
 * Use exceptions for error conditions that shouldn't occur during normal operation.
 */
#define EXCEPTION_EXAMPLE "throw std::runtime_error("Invalid asset path")"

/**
 * @brief Use error codes for expected failures
 *
 * Use bool or enum returns for operations that can fail during normal use.
 */
#define ERROR_CODE_EXAMPLE "bool loadAsset(const std::string& path)"

/**
 * @brief Use optional for nullable results
 *
 * Use std::optional for functions that may not return a value.
 */
#define OPTIONAL_EXAMPLE "std::optional<Asset*> findAsset(const std::string& name)"

/**
 * @brief Validate preconditions
 *
 * Check preconditions at the start of functions and throw on failure.
 */
#define PRECONDITIONS_EXAMPLE "if (!asset) throw std::invalid_argument("Asset cannot be null")"
/** @} */

/**
 * @name Performance Guidelines
 * @brief Rules for performance considerations
 * @{
 */
/**
 * @brief Avoid unnecessary allocations
 *
 * Reuse objects and buffers when possible to reduce allocations.
 */
#define AVOID_ALLOCATIONS_EXAMPLE "buffer_.reserve(estimatedSize)"

/**
 * @brief Use move semantics when appropriate
 *
 * Use std::move for expensive-to-copy objects.
 */
#define MOVE_SEMANTICS_EXAMPLE "Asset asset = std::move(otherAsset)"

/**
 * @brief Consider cache locality
 *
 * Organize data structures for better cache performance.
 */
#define CACHE_LOCALITY_EXAMPLE "// Group related data together"

/**
 * @brief Profile before optimizing
 *
 * Only optimize code that has proven performance issues.
 */
#define PROFILE_FIRST_EXAMPLE "// Profile-guided optimization"
/** @} */

/** @} */ // End of CodeStyle group

/**
 * @defgroup BestPractices Best Practices
 * @brief Recommended patterns and practices
 * @{
 */

// ==================== BEST PRACTICES ====================

/**
 * @name Memory Management
 * @brief Memory management best practices
 * @{
 */
/**
 * @brief Use RAII for all resource management
 *
 * Every resource should be managed by an object's lifetime.
 */
#define RAII_BEST_PRACTICE "std::unique_ptr<File> file"

/**
 * @brief Avoid raw pointers for ownership
 *
 * Use smart pointers instead of raw pointers for ownership.
 */
#define NO_RAW_OWNERSHIP "std::unique_ptr<Data> data_"

/**
 * @brief Use raw pointers for non-ownership references
 *
 * Raw pointers are acceptable for non-owning references.
 */
#define RAW_REFERENCE "Observer* observer_"
/** @} */

/**
 * @name Thread Safety
 * @brief Thread safety best practices
 * @{
 */
/**
 * @brief Document thread safety guarantees
 *
 * Clearly document whether classes are thread-safe.
 */
#define THREAD_SAFETY_DOC "// Thread-safe: Protected by internal mutex"

/**
 * @brief Use atomic operations for simple types
 *
 * Use std::atomic for simple thread-safe operations.
 */
#define ATOMIC_OPERATIONS "std::atomic<size_t> counter_"

/**
 * @brief Use mutexes for complex operations
 *
 * Use std::mutex for protecting complex critical sections.
 */
#define MUTEX_PROTECTION "std::mutex mutex_"
/** @} */

/**
 * @name Testing
 * @brief Testing best practices
 * @{
 */
/**
 * @brief Write unit tests for complex logic
 *
 * Complex business logic should have corresponding unit tests.
 */
#define UNIT_TESTS_EXAMPLE "// Example: Add unit tests for asset loading"

/**
 * @brief Test error conditions
 *
 * Test both success and failure paths.
 */
#define ERROR_TESTING_EXAMPLE "TEST(AssetManager, LoadNonexistentAsset)"

/**
 * @brief Use descriptive test names
 *
 * Test names should clearly describe what they're testing.
 */
#define DESCRIPTIVE_TESTS "LoadAssetFromValidPath"
/** @} */

/** @} */ // End of BestPractices group

/**
 * @defgroup CommonPatterns Common Patterns
 * @brief Frequently used patterns in the codebase
 * @{
 */

// ==================== COMMON PATTERNS ====================

/**
 * @brief PIMPL (Pointer to Implementation) pattern
 *
 * Use PIMPL to hide implementation details and reduce compile dependencies.
 *
 * Example:
 * @code
 * class PublicAPI {
 * private:
 *     class Impl;
 *     std::unique_ptr<Impl> impl_;
 * };
 * @endcode
 */
#define PIMPL_PATTERN "std::unique_ptr<Impl> impl_"

/**
 * @brief Factory pattern for object creation
 *
 * Use factory functions for creating objects with complex initialization.
 *
 * Example:
 * @code
 * std::unique_ptr<Asset> createAsset(const std::string& type);
 * @endcode
 */
#define FACTORY_PATTERN "std::unique_ptr<Asset> createAsset(const std::string& type)"

/**
 * @brief Observer pattern for event handling
 *
 * Use observer pattern for loose coupling between components.
 *
 * Example:
 * @code
 * class EventManager {
 *     void subscribe(const std::string& event, std::function<void()> callback);
 * };
 * @endcode
 */
#define OBSERVER_PATTERN "void subscribe(const std::string& event, Callback callback)"

/**
 * @brief Strategy pattern for interchangeable algorithms
 *
 * Use strategy pattern for algorithms that may vary.
 *
 * Example:
 * @code
 * class AssetLoader {
 *     virtual bool canLoad(const std::string& extension) const = 0;
 * };
 * @endcode
 */
#define STRATEGY_PATTERN "virtual bool canLoad(const std::string& extension) const = 0"

/** @} */ // End of CommonPatterns group

/** @} */ // End of CodingStandards group

} // namespace FoundryEngine
