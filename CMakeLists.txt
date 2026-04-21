cmake_minimum_required(VERSION 3.20)
project(SPA_CGAL VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CGAL REQUIRED)

add_executable(BooleanOp
    src/main.cpp
)

target_link_libraries(BooleanOp PRIVATE CGAL::CGAL)

if(MSVC)
    target_compile_options(BooleanOp PRIVATE /W3 /wd4996 /wd4267 /wd4244)
    target_compile_definitions(BooleanOp PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()
