cmake_minimum_required(VERSION 3.5.0)
set(CMAKE_CXX_STANDARD 23)
project(halcyon VERSION 0.1.0 LANGUAGES C CXX)


file(GLOB_RECURSE HEADERS RELATIVE ${CMAKE_SOURCE_DIR} "src/*.h")
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*.cpp")

include(C:/Users/owcar/vcpkg/scripts/buildsystems/vcpkg.cmake)

find_package(cpr CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

add_executable(halcyon ${SOURCES} ${HEADERS})

target_link_libraries(halcyon PRIVATE cpr::cpr)
target_link_libraries(halcyon PRIVATE rapidjson)
