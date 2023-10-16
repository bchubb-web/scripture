cmake_minimum_required(VERSION 3.1...3.27)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/)

# This is your project statement. You should always list languages;
# Listing the version is nice here since it sets lots of useful variables
project(
    ScriptureSource
  VERSION 1.0
  LANGUAGES CXX)

# If you set any CMAKE_ variables, that can go here.
# (But usually don't do this, except maybe for C++ standard)

# Find packages go here.

# You should usually split this into folders, but this is a simple example

# This is a "default" library, and will match the *** variable setting.
# Other common choices are STATIC, SHARED, and MODULE
# Including header files here helps IDEs but is not required.
# Output libname matches target name, with the usual extensions on your system
#add_library(RawMode src/enableRaw.cpp src/enableRaw.hpp)

# Link each target with other targets or add options, etc.

# Adding something we can run - Output name matches target name
add_executable(Scripture main.cpp)

# Make sure you link your targets with this command. It can also link libraries and
# even flags, so linking a target that does not exist will not give a configure-time error.
#target_link_libraries(Scripture Lib)
target_link_libraries(Scripture)

## [main]

# This part is so the Modern CMake book can verify this example builds. For your code,
# you'll probably want tests too
#enable_testing()
#add_test(NAME MyExample COMMAND MyExample)

