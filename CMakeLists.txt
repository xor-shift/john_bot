cmake_minimum_required(VERSION 3.25)
project(john_bot)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

set(warning_flags -Wall -Wextra -Wno-missing-field-initializers)

find_package(OpenSSL REQUIRED)

add_subdirectory(thirdparty/boost)
add_subdirectory(thirdparty/cpptrace)
add_subdirectory(thirdparty/googletest)
add_subdirectory(thirdparty/magic_enum)
add_subdirectory(thirdparty/spdlog)

add_subdirectory(thirdparty/stuff)

# everything besides main.cpp
add_library(${PROJECT_NAME}_lib
    inc/error.hpp

    src/bot.cpp
    src/irc/replies.cpp
    src/irc/client.cpp
    src/telegram/api.cpp
    src/telegram/client.cpp
    src/telegram/connection.cpp
)

target_compile_options(${PROJECT_NAME}_lib PUBLIC -fsanitize=address -fsanitize=undefined)
target_link_options(${PROJECT_NAME}_lib PUBLIC -fsanitize=address -fsanitize=undefined)
target_compile_options(${PROJECT_NAME}_lib PUBLIC ${warning_flags})

target_link_libraries(${PROJECT_NAME}_lib PUBLIC
    OpenSSL::SSL

    cpptrace::cpptrace
    magic_enum::magic_enum
    spdlog::spdlog
    Boost::asio
    Boost::beast
    Boost::json

    stf::intro
)

target_include_directories(${PROJECT_NAME}_lib PUBLIC
    inc
)


add_executable(${PROJECT_NAME} main.cpp)
target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fsanitize=undefined)
target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fsanitize=undefined)
target_compile_options(${PROJECT_NAME} PUBLIC ${warning_flags})

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)

add_executable(${PROJECT_NAME}_test
    test/error.cpp
    test/message.cpp
    test/kv.cpp
)
target_compile_options(${PROJECT_NAME}_test PUBLIC -fsanitize=address -fsanitize=undefined)
target_link_options(${PROJECT_NAME}_test PUBLIC -fsanitize=address -fsanitize=undefined)
target_link_libraries(${PROJECT_NAME}_test PRIVATE ${PROJECT_NAME}_lib gtest gtest_main)

