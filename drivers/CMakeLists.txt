# Declares the current directory as a Zephyr library
# If no name is given, the name is derived from the directory name
zephyr_library(midi1_serial)

# List the source code files for the library
zephyr_library_sources(midi1_serial.c midi1.c)

# Add header files to the CMake search directories
zephyr_include_directories(.)
