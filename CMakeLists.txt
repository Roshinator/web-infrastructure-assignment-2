cmake_minimum_required(VERSION 3.8)
set(CMAKE_C_STANDARD)
set(CMAKE_CXX_STANDARD 17)

project("wi-proxy-cached")

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/${PROJECT_NAME}/*.h" "include/${PROJECT_NAME}/*.hpp")

add_executable("${PROJECT_NAME}" ${SOURCES} ${HEADERS})
target_include_directories("${PROJECT_NAME}" PUBLIC "include/${PROJECT_NAME}/")