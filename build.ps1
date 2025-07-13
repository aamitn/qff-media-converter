# build.ps1

# Define the path to your Qt installation
# IMPORTANT: Adjust this path if your Qt installation is different
$qtPrefixPath = "C:\Qt\6.9.1\msvc2022_64"

# Define the build directory name
$buildDir = "build"

# --- Step 1: Configure the CMake project ---
# -S .        : Specifies the source directory as the current directory
# -B .\build : Specifies the build directory (will be created if it doesn't exist)
# -DCMAKE_PREFIX_PATH : Tells CMake to find Qt (and other dependencies if applicable)
Write-Host "--- Configuring CMake project ---"
cmake -DCMAKE_PREFIX_PATH="$qtPrefixPath" -S . -B ".\$buildDir"

# Check if the configuration step was successful
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed. Exiting."
    Read-Host -Prompt "Press Enter to exit..." # Pause on error
    exit 1
}

# --- Step 2: Build the project ---
# --build .\build : Builds the project located in the specified build directory
Write-Host "--- Building project ---"
cmake --build ".\$buildDir"

# Check if the build step was successful
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake build failed. Exiting."
    Read-Host -Prompt "Press Enter to exit..." # Pause on error
    exit 1
}

Write-Host "--- Build process completed successfully ---"
Read-Host -Prompt "Press Enter to exit..." # Pause on successful completion