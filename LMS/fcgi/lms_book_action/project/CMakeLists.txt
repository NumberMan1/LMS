cmake_minimum_required(VERSION 3.0.0)

SET(CMAKE_TOOLCHAIN_FILE "/home/num/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(lms_book_action VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pg -O2 -Wall -W")

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories("~/vcpkg/installed/x64-linux/include")

file(GLOB_RECURSE fcgi_lib "/home/num/vcpkg/installed/x64-linux/lib/libfcgi*.a")
file(GLOB_RECURSE src ${PROJECT_SOURCE_DIR}/src "*.cpp" "*.cc")

find_package(OpenSSL REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
# find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)


add_executable(${PROJECT_NAME} main.cc ${src})
target_link_libraries(${PROJECT_NAME} PRIVATE ${fcgi_lib})
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
target_link_libraries(${PROJECT_NAME} PRIVATE JsonCpp::JsonCpp)
# target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::mysql-connector-cpp::connector)
target_link_libraries(${PROJECT_NAME} PRIVATE resolv)
