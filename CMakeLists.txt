cmake_minimum_required(VERSION 4.3)

project(FingerBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS
        core
        imgproc
        imgcodecs
        videoio
        highgui
)

include_directories(${OpenCV_INCLUDE_DIRS})
add_executable(FingerBot main.cpp)

target_link_libraries(FingerBot PRIVATE ${OpenCV_LIBS})