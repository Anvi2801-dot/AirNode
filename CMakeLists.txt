cmake_minimum_required(VERSION 3.10)
project(GestureVirtualMouse)

set(CMAKE_CXX_STANDARD 17)

# 1. Look for OpenCV (Mac and Windows compatible)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 2. Define the OOPS source structure
set(SOURCES 
    src/main.cpp 
    src/HandTracker.cpp
)

# 3. Add OS-specific "Action" modules
if(APPLE)
    list(APPEND SOURCES src/MacMouse.cpp)
    find_library(CORE_GRAPHICS ApplicationServices)
elseif(WIN32)
    list(APPEND SOURCES src/WindowsMouse.cpp)
endif()

add_executable(GCVM ${SOURCES})

# 4. Link everything
target_link_libraries(GCVM ${OpenCV_LIBS})
if(APPLE)
    target_link_libraries(GCVM ${CORE_GRAPHICS})
endif()