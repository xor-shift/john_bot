cmake_minimum_required(VERSION 3.25)
project(john_bot)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 26)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fsanitize=address -fsanitize=undefined")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi -fsanitize=address -fsanitize=undefined")

set(warning_flags -Wall -Wextra -Wno-missing-field-initializers)

find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)

add_subdirectory(thirdparty/boost)
add_subdirectory(thirdparty/cpptrace)
add_subdirectory(thirdparty/googletest)
add_subdirectory(thirdparty/magic_enum)
add_subdirectory(thirdparty/spdlog)

add_subdirectory(thirdparty/stuff)

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/inc/generated/schema.h
    COMMAND bash -c "xxd -i -n schema_str '${CMAKE_SOURCE_DIR}/schema.sql' | tac | sed '3s/\$/, 0x00/' | tac > '${CMAKE_SOURCE_DIR}/inc/generated/schema.h'"
    DEPENDS ${CMAKE_SOURCE_DIR}/schema.sql
    VERBATIM
)

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/inc/generated/bootstrap.h
    COMMAND bash -c "xxd -i -n bootstrap_str '${CMAKE_SOURCE_DIR}/bootstrap.sql' | tac | sed '3s/\$/, 0x00/' | tac > '${CMAKE_SOURCE_DIR}/inc/generated/bootstrap.h'"
    DEPENDS ${CMAKE_SOURCE_DIR}/bootstrap.sql
    VERBATIM
)

# everything besides main.cpp
add_library(${PROJECT_NAME}_lib
    inc/assio/as_expected.hpp

    inc/irc/client.hpp
    inc/irc/message.hpp
    inc/irc/replies.hpp

    inc/sqlite/aggregate.hpp
    inc/sqlite/database.hpp
    inc/sqlite/error.hpp
    inc/sqlite/query.hpp
    inc/sqlite/sqlite.hpp

    inc/telegram/api.hpp
    inc/telegram/client.hpp
    inc/telegram/connection.hpp

    inc/things/dummy.hpp
    inc/things/logger.hpp
    inc/things/relay.hpp

    inc/alloc.hpp
    inc/bot.hpp
    inc/error.hpp
    inc/kv.hpp

    src/irc/replies.cpp
    src/irc/client.cpp
    src/telegram/api.cpp
    src/telegram/client.cpp
    src/telegram/connection.cpp
    src/things/logger.cpp
    src/things/relay.cpp
    src/argv.cpp
    src/bot.cpp
    src/error.cpp
    src/sqlite.cpp
)

target_compile_options(${PROJECT_NAME}_lib PUBLIC -fsanitize=address -fsanitize=undefined)
target_link_options(${PROJECT_NAME}_lib PUBLIC -fsanitize=address -fsanitize=undefined)
target_compile_options(${PROJECT_NAME}_lib PUBLIC ${warning_flags})

target_link_libraries(${PROJECT_NAME}_lib PUBLIC
    OpenSSL::SSL
    SQLite::SQLite3

    cpptrace::cpptrace
    magic_enum::magic_enum
    spdlog::spdlog
    Boost::asio
    Boost::beast
    Boost::json
    Boost::mysql

    stf::hash
    stf::intro
    stf::scope
)

target_include_directories(${PROJECT_NAME}_lib PUBLIC
    inc
)

target_compile_definitions(${PROJECT_NAME}_lib PUBLIC BOOST_MYSQL_SEPARATE_COMPILATION)

add_executable(${PROJECT_NAME}
    main.cpp
    ${CMAKE_SOURCE_DIR}/inc/generated/schema.h
    ${CMAKE_SOURCE_DIR}/inc/generated/bootstrap.h
)
target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fsanitize=undefined)
target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fsanitize=undefined)
target_compile_options(${PROJECT_NAME} PUBLIC ${warning_flags})

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)

add_executable(${PROJECT_NAME}_test
    test/argv.cpp
    test/error.cpp
    test/message.cpp
    test/kv.cpp
)
target_compile_options(${PROJECT_NAME}_test PUBLIC -fsanitize=address -fsanitize=undefined)
target_link_options(${PROJECT_NAME}_test PUBLIC -fsanitize=address -fsanitize=undefined)
target_link_libraries(${PROJECT_NAME}_test PRIVATE ${PROJECT_NAME}_lib gtest gtest_main)

