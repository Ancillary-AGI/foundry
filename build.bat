@echo off
setlocal enabledelayedexpansion

REM FoundryEngine Build Script for Windows
REM This script builds the FoundryEngine for different platforms

set "BUILD_TYPE=Release"
set "BUILD_DIR=build_%BUILD_TYPE%"
set "JOBS=%NUMBER_OF_PROCESSORS%"

REM Function to print colored output
:print_status
echo [INFO] %~1
goto :eof

:print_success
echo [SUCCESS] %~1
goto :eof

:print_warning
echo [WARNING] %~1
goto :eof

:print_error
echo [ERROR] %~1
goto :eof

REM Function to check prerequisites
:check_prerequisites
call :print_status "Checking prerequisites..."

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "CMake is required but not installed"
    exit /b 1
)

where msbuild >nul 2>&1
if %errorlevel% neq 0 (
    call :print_warning "MSBuild not found, using make instead"
)

call :print_success "Prerequisites check passed"
goto :eof

REM Function to build for native platform
:build_native
call :print_status "Building for native platform (%BUILD_TYPE%)..."

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

where msbuild >nul 2>&1
if %errorlevel% equ 0 (
    msbuild FoundryEngine.sln /p:Configuration=%BUILD_TYPE% /m:%JOBS%
) else (
    cmake --build . --config %BUILD_TYPE% --parallel %JOBS%
)

cd ..
call :print_success "Native build completed"
goto :eof

REM Function to build for WebAssembly
:build_web
call :print_status "Building for WebAssembly..."

where emcmake >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "Emscripten is required for WebAssembly builds"
    call :print_status "Please install Emscripten: https://emscripten.org/docs/getting_started/downloads.html"
    exit /b 1
)

if not exist "build_web" mkdir "build_web"
cd "build_web"

emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make -j%JOBS%

cd ..
call :print_success "WebAssembly build completed"
goto :eof

REM Function to build for Android
:build_android
call :print_status "Building for Android..."

if "%ANDROID_NDK%"=="" (
    call :print_error "ANDROID_NDK environment variable is not set"
    exit /b 1
)

if not exist "build_android" mkdir "build_android"
cd "build_android"

cmake .. ^
    -DCMAKE_TOOLCHAIN_FILE="%ANDROID_NDK%/build/cmake/android.toolchain.cmake" ^
    -DANDROID_ABI=arm64-v8a ^
    -DANDROID_PLATFORM=android-21 ^
    -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release --parallel %JOBS%

cd ..
call :print_success "Android build completed"
goto :eof

REM Function to build IDE
:build_ide
call :print_status "Building IDE..."

where gradle >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "Gradle is required for IDE builds"
    exit /b 1
)

cd ide
gradle build
cd ..

call :print_success "IDE build completed"
goto :eof

REM Function to run tests
:run_tests
call :print_status "Running tests..."

if exist "build_%BUILD_TYPE%" (
    cd "build_%BUILD_TYPE%"
    ctest --output-on-failure
    cd ..
    call :print_success "Tests completed"
) else (
    call :print_warning "No build directory found. Please build the project first."
)
goto :eof

REM Function to clean build directories
:clean
call :print_status "Cleaning build directories..."

if exist "build_*" rmdir /s /q "build_*"
if exist "ide\build" rmdir /s /q "ide\build"

call :print_success "Clean completed"
goto :eof

REM Function to show help
:show_help
echo FoundryEngine Build Script for Windows
echo.
echo Usage: %~nx0 [OPTIONS] [TARGET]
echo.
echo Options:
echo   -h, --help     Show this help message
echo   -c, --clean    Clean build directories
echo   -t, --test     Run tests
echo   -j, --jobs N   Number of parallel jobs (default: auto)
echo.
echo Targets:
echo   native         Build for native platform (default)
echo   web            Build for WebAssembly
echo   android        Build for Android
echo   ide            Build the IDE
echo   all            Build everything
echo.
echo Examples:
echo   %~nx0                    # Build for native platform
echo   %~nx0 web               # Build for WebAssembly
echo   %~nx0 android           # Build for Android
echo   %~nx0 ide               # Build the IDE
echo   %~nx0 all               # Build everything
echo   %~nx0 -c                # Clean build directories
echo   %~nx0 -t                # Run tests
goto :eof

REM Main script logic
:main
set "target=native"
set "clean_build=false"
set "run_tests_flag=false"

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :args_done
if "%~1"=="-h" goto :help
if "%~1"=="--help" goto :help
if "%~1"=="-c" goto :clean_flag
if "%~1"=="--clean" goto :clean_flag
if "%~1"=="-t" goto :test_flag
if "%~1"=="--test" goto :test_flag
if "%~1"=="-j" goto :jobs_flag
if "%~1"=="--jobs" goto :jobs_flag
if "%~1"=="native" goto :target_native
if "%~1"=="web" goto :target_web
if "%~1"=="android" goto :target_android
if "%~1"=="ide" goto :target_ide
if "%~1"=="all" goto :target_all
call :print_error "Unknown option: %~1"
call :show_help
exit /b 1

:help
call :show_help
exit /b 0

:clean_flag
set "clean_build=true"
shift
goto :parse_args

:test_flag
set "run_tests_flag=true"
shift
goto :parse_args

:jobs_flag
set "JOBS=%~2"
shift
shift
goto :parse_args

:target_native
set "target=native"
shift
goto :parse_args

:target_web
set "target=web"
shift
goto :parse_args

:target_android
set "target=android"
shift
goto :parse_args

:target_ide
set "target=ide"
shift
goto :parse_args

:target_all
set "target=all"
shift
goto :parse_args

:args_done

REM Clean if requested
if "%clean_build%"=="true" (
    call :clean
    if "%target%"=="clean" exit /b 0
)

REM Check prerequisites
call :check_prerequisites

REM Build based on target
if "%target%"=="native" call :build_native
if "%target%"=="web" call :build_web
if "%target%"=="android" call :build_android
if "%target%"=="ide" call :build_ide
if "%target%"=="all" (
    call :build_native
    call :build_web
    call :build_ide
)

REM Run tests if requested
if "%run_tests_flag%"=="true" call :run_tests

call :print_success "Build process completed successfully!"
exit /b 0
