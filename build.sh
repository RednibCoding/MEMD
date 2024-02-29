#!/bin/bash

# Simple build script template for small projects
# No need for 'make' and stuff

echo "Starting compilation..."

# Define compiler
COMPILER=gcc

# Output file name
OUTPUT_FILE_NAME=example

# Define resource files to include
RES_FILES=

# Define the source files to include
SRC_FILES=example.c

# Debug build
# Uncomment the following line for debug build
# $COMPILER $SRC_FILES $RES_FILES -o $OUTPUT_FILE_NAME -std=c99 -lopengl32 -lgdi32

# Release build
$COMPILER $SRC_FILES $RES_FILES -o $OUTPUT_FILE_NAME -std=c99 -s -lopengl32 -lgdi32 -O3 -march=native -funroll-loops -flto -fomit-frame-pointer

if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit $?
else
    echo "Compilation succeeded."
fi

# Check if the executable exists before trying to execute it
if [ -f "./$OUTPUT_FILE_NAME" ]; then
    echo "Executing $OUTPUT_FILE_NAME..."
    ./$OUTPUT_FILE_NAME
else
    echo "Executable $OUTPUT_FILE_NAME not found."
fi
