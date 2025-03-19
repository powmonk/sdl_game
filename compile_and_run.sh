#!/bin/bash
# Script to clone, build and run a C++ SDL project on Steam Deck
# Usage: ./build_and_run.sh <git_repository_url> [branch_name]

set -e  # Exit on any error

# Check if a repository URL was provided
if [ -z "$1" ]; then
    echo "Usage: $0 <git_repository_url> [branch_name]"
    echo "Example: $0 https://github.com/username/project.git main"
    exit 1
fi

REPO_URL="$1"
BRANCH="${2:-main}"  # Use provided branch or default to 'main'
REPO_NAME=$(basename "$REPO_URL" .git)
WORK_DIR="$HOME/sdl_projects"

echo "=== Steam Deck C++/SDL Project Builder ==="
echo "Repository: $REPO_URL"
echo "Branch: $BRANCH"
echo "Working directory: $WORK_DIR/$REPO_NAME"

# Make sure we're in desktop mode
if [ "$(systemctl is-active gamescope-session)" == "active" ]; then
    echo "ERROR: This script should be run in Desktop Mode."
    echo "Please switch to Desktop Mode from the Steam interface and try again."
    exit 1
fi

# Create working directory if it doesn't exist
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

# Install dependencies if needed
echo "Checking and installing dependencies..."
sudo pacman -Syu --noconfirm
sudo pacman -S --needed --noconfirm git base-devel cmake sdl2 sdl2_image sdl2_mixer sdl2_ttf sdl2_net

# Clone or update repository
if [ -d "$REPO_NAME" ]; then
    echo "Repository directory already exists. Updating..."
    cd "$REPO_NAME"
    git fetch
    git checkout "$BRANCH"
    git pull
else
    echo "Cloning repository..."
    git clone "$REPO_URL" "$REPO_NAME"
    cd "$REPO_NAME"
    git checkout "$BRANCH"
fi

# Look for build system
echo "Detecting build system..."

# Check for CMake
if [ -f "CMakeLists.txt" ]; then
    echo "CMake project detected. Building..."
    
    # Create build directory
    mkdir -p build
    cd build
    
    # Configure and build
    cmake ..
    make -j$(nproc)
    
    # Find executable (assuming it's in the build directory)
    EXECUTABLE=$(find . -type f -executable -not -path "*/\.*" | grep -v "CMake" | head -n 1)
    
# Check for Makefile
elif [ -f "Makefile" ] || [ -f "makefile" ]; then
    echo "Makefile project detected. Building..."
    make -j$(nproc)
    
    # Find executable
    EXECUTABLE=$(find . -type f -executable -not -path "*/\.*" | head -n 1)
    
# Check for shell scripts
elif [ -f "build.sh" ]; then
    echo "Build script detected. Running..."
    chmod +x build.sh
    ./build.sh
    
    # Find executable
    EXECUTABLE=$(find . -type f -executable -not -path "*/\.*" | head -n 1)
    
else
    echo "No recognized build system found. Attempting manual compilation..."
    
    # Look for main source file
    MAIN_SOURCE=$(find . -name "main.cpp" -o -name "main.cc" | head -n 1)
    
    if [ -z "$MAIN_SOURCE" ]; then
        MAIN_SOURCE=$(find . -name "*.cpp" -o -name "*.cc" | head -n 1)
    fi
    
    if [ -n "$MAIN_SOURCE" ]; then
        echo "Compiling $MAIN_SOURCE..."
        
        # Get directory of main source file
        SRC_DIR=$(dirname "$MAIN_SOURCE")
        
        # Find all source files
        SOURCE_FILES=$(find "$SRC_DIR" -name "*.cpp" -o -name "*.cc")
        
        # Compile
        g++ -std=c++17 -o "./game" $SOURCE_FILES -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lSDL2_net
        
        EXECUTABLE="./game"
    else
        echo "ERROR: Could not find main source file."
        exit 1
    fi
fi

# Run the executable if found
if [ -n "$EXECUTABLE" ] && [ -f "$EXECUTABLE" ]; then
    echo "Build successful. Running the application..."
    echo "--------------------------------------------"
    chmod +x "$EXECUTABLE"
    "$EXECUTABLE"
else
    echo "ERROR: Could not find executable after build."
    echo "Please check the project structure and build output."
    exit 1
fi

# Return to original directory
cd "$WORK_DIR"
echo "Done!"
