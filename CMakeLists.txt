cmake_minimum_required(VERSION 3.16)
project(GCVM)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)

set(MEDIAPIPE_DIR "$ENV{HOME}/.pyenv/versions/3.10.12/lib/python3.10/site-packages/mediapipe/tasks/c")
set(MEDIAPIPE_DYLIB "${MEDIAPIPE_DIR}/libmediapipe.dylib")

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(gcvm_app
    src/main.cpp
    src/HandTracker.cpp
    src/MacMouse.cpp
)

target_link_libraries(gcvm_app
    ${OpenCV_LIBS}
    ${MEDIAPIPE_DYLIB}
    "-framework ApplicationServices"
    "-framework Carbon"
)

set_target_properties(gcvm_app PROPERTIES
    BUILD_RPATH "${MEDIAPIPE_DIR}"
    INSTALL_RPATH "${MEDIAPIPE_DIR}"
)