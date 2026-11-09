cmake_minimum_required(VERSION 3.16)
project(OrangeVisionAnalyzer VERSION 1.0 LANGUAGES C CXX)

find_package(OpenCV 4 REQUIRED COMPONENTS core highgui imgproc videoio)

add_library(vc STATIC vc.c)
target_include_directories(vc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(vc PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)

add_executable(orange_vision Laranjas.cpp)
target_compile_features(orange_vision PRIVATE cxx_std_11)
target_include_directories(orange_vision PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(orange_vision PRIVATE vc ${OpenCV_LIBS})

if(MSVC)
    target_compile_options(vc PRIVATE /utf-8)
    target_compile_options(orange_vision PRIVATE /utf-8)
endif()
