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
cmake --build ".\$buildDir" --config Release # Added --config Release for release build

# Check if the build step was successful
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake build failed. Exiting."
    Read-Host -Prompt "Press Enter to exit..." # Pause on error
    exit 1
}

# --- Step 3: Change directory to build output for CPack ---
Write-Host "--- Changing directory to build output for CPack ---"
Set-Location ".\$buildDir"

# Check if changing directory was successful
if ($LASTEXITCODE -ne 0) { # $LASTEXITCODE will be 0 if Set-Location succeeds
    Write-Error "Failed to change directory to '.\$buildDir'. Exiting."
    Read-Host -Prompt "Press Enter to exit..." # Pause on error
    exit 1
}

# --- Step 4: Run CPack to generate ZIP package ---
Write-Host "--- Running CPack to generate ZIP package ---"
cpack -G ZIP

# Check if the cpack step was successful
if ($LASTEXITCODE -ne 0) {
    Write-Error "CPack failed. Exiting."
    Read-Host -Prompt "Press Enter to exit..." # Pause on error
    exit 1
}

Write-Host "--- Build and Packaging process completed successfully ---"
Read-Host -Prompt "Press Enter to exit..." # Pause on successful completion