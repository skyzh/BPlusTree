cmake_minimum_required(VERSION 3.12)
project(BPlusTree)

set(CMAKE_CXX_STANDARD 14)

find_package(Catch2 CONFIG REQUIRED)

set (CMAKE_CXX_FLAGS "-Og")

add_executable(BPlusTree main.cpp BTree.hpp Container.hpp Persistence.hpp LRU.hpp)
add_executable(BPlusTreeTest test_main.cpp BTree.hpp BTree_test.cpp Container.hpp Container_test.cpp BTree_Leaf_test.cpp BTree_Index_test.cpp BTree_Storage_test.cpp Persistence_test.cpp LRU_test.cpp)
target_link_libraries(BPlusTreeTest PRIVATE Catch2::Catch2)
