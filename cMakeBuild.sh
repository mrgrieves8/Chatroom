#!/bin/bash

# Create a build directory
mkdir -p build
cd build

# Run CMake to generate build files
cmake ..

# Build the project
make

# Return to the original directory
cd ..
