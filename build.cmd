@echo off
rem build.bat

rem Define the path to your Qt installation
rem IMPORTANT: Adjust this path if your Qt installation is different
set "qtPrefixPath=C:\Qt\6.9.1\msvc2022_64"

rem Define the build directory name
set "buildDir=build"

echo --- Configuring CMake project ---
rem -S .        : Specifies the source directory as the current directory
rem -B .\build : Specifies the build directory (will be created if it doesn't exist)
rem -DCMAKE_PREFIX_PATH : Tells CMake where to find Qt (and other dependencies if applicable)
cmake -DCMAKE_PREFIX_PATH="%qtPrefixPath%" -S . -B ".\%buildDir%"

rem Check if the configuration step was successful
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed. Exiting.
    pause
    exit /b 1
)

echo --- Building project ---
rem --build .\build : Builds the project located in the specified build directory
cmake --build ".\%buildDir%"

rem Check if the build step was successful
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake build failed. Exiting.
    pause
    exit /b 1
)

echo --- Build process completed successfully ---
echo Press any key to exit...
pause > nul