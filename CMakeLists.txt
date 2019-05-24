cmake_minimum_required(VERSION 3.14)
project(BPlusTree)

set(CMAKE_CXX_STANDARD 14)

find_package(catch2 REQUIRED)

find_path(CATCH_INCLUDE_DIR NAMES catch.hpp PATH_SUFFIXES catch2)
include_directories(${CATCH_INCLUDE_DIR})

add_executable(BPlusTree main.cpp BTree.hpp Container.hpp)
add_executable(BPlusTreeTest test_main.cpp BTree.hpp BTree_test.cpp Container.hpp test_container.cpp)
