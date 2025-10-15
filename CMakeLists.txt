cmake_minimum_required(VERSION 2.8.12)
project(sklm_test)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# better to use target_compile_options or CMAKE_CXX_STANDARD
# depending on target
# add_compile_options(-std=c++14)

set(SRC test_cpp/Image.cpp
        test_cpp/ImageProcessing.cpp
        test_cpp/Test.cpp
        test_cpp/Timer.cpp
        test_cpp/main.cpp
)

add_executable(sklm_test ${SRC})
target_include_directories(sklm_test PRIVATE test_cpp)
