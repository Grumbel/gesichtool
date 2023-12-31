cmake_minimum_required(VERSION 3.12)
project(FaceDetectionTool)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV 4 REQUIRED)
find_package(dlib REQUIRED)
find_package(fmt REQUIRED)

add_executable(gesichtool src/gesichtool.cpp)
target_link_libraries(gesichtool PRIVATE
  fmt::fmt
  dlib::dlib
  ${OpenCV_LIBS})
target_include_directories(gesichtool PRIVATE ${OpenCV_INCLUDE_DIRS})

install(TARGETS gesichtool
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# EOF #
