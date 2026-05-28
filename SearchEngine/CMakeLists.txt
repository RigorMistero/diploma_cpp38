cmake_minimum_required(VERSION 3.16)
project(SearchEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_INDEXER "Build indexer executable" ON)
option(BUILD_SEARCHER "Build searcher executable" ON)

find_package(Boost REQUIRED COMPONENTS locale system)
find_package(libpqxx REQUIRED)

# libpq из установленного PostgreSQL
set(PQ_DIR "C:/Program Files/PostgreSQL/16")
find_path(PQ_INCLUDE_DIR libpq-fe.h PATHS "${PQ_DIR}/include" REQUIRED)
find_library(PQ_LIBRARY libpq.lib PATHS "${PQ_DIR}/lib" REQUIRED)

add_library(search_engine_common STATIC
    src/config_reader.cpp
    src/db_manager.cpp
    src/file_scanner.cpp
    src/text_processor.cpp
    src/thread_pool.cpp
)

target_include_directories(search_engine_common PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${PQ_INCLUDE_DIR}
)

target_link_libraries(search_engine_common PUBLIC
    Boost::locale
    Boost::system
    libpqxx::pqxx
    ${PQ_LIBRARY}
)

if(BUILD_INDEXER)
    add_executable(indexer src/indexer_main.cpp)
    target_link_libraries(indexer PRIVATE search_engine_common)
endif()

if(BUILD_SEARCHER)
    add_executable(searcher src/searcher_main.cpp)
    target_link_libraries(searcher PRIVATE search_engine_common)
endif()

configure_file(config.ini ${CMAKE_BINARY_DIR}/config.ini COPYONLY)