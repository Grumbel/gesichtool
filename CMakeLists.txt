cmake_minimum_required(VERSION 3.12)
project(FaceDetectionTool)

find_package(OpenCV 4 REQUIRED)
find_package(fmt REQUIRED)

add_executable(gesichtool src/gesichtool.cpp)
set_target_properties(gesichtool PROPERTIES CXX_STANDARD 20)
target_link_libraries(gesichtool PRIVATE fmt::fmt ${OpenCV_LIBS})
target_include_directories(gesichtool PRIVATE ${OpenCV_INCLUDE_DIRS})

# EOF #
