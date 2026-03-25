cmake_minimum_required(VERSION 3.20)
project(sysmon)

# 1. Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Enable Qt Automatic Tools
# This handles the Meta-Object Compiler (MOC) for your header files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 3. Find Qt6 Packages
# If you get an error here, ensure 'qt6-charts-dev' is installed on your system
find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Gui)

# 4. Find NVIDIA Management Library (NVML)
# This searches for the nvidia-ml library on your system path
find_library(NVML_LIB nvidia-ml)

# Check if NVML was found, otherwise provide a warning
if(NOT NVML_LIB)
    message(WARNING "NVML library not found. GPU monitoring will not work.")
endif()

# 5. Define the Executable and include all project files
# We include sysmon.h implicitly by having it in the same directory,
# but we must list all .cpp files here.
add_executable(sysmon
        main.cpp
        sysmon.cpp
        sysmon.h
)

# 6. Link Libraries
# This tells the linker to connect the Qt modules and NVML to your binary
target_link_libraries(sysmon PRIVATE
        Qt6::Widgets
        Qt6::Charts
        Qt6::Gui
        ${NVML_LIB}
)

# 7. Optimization (Optional)
# This ensures the binary is small and fast in Release mode
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(sysmon PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()