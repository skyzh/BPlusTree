cmake_minimum_required(VERSION 3.12)
project(BPlusTree)

set(CMAKE_CXX_STANDARD 17)

find_package(Catch2 CONFIG REQUIRED)

add_executable(BPlusTree src/main.cpp src/BTree.hpp src/Container.hpp src/Persistence.hpp src/LRU.hpp)
add_executable(BPlusTreeTest src/test_main.cpp src/BTree.hpp src/BTree_test.cpp src/Container.hpp src/Container_test.cpp src/BTree_Leaf_test.cpp src/BTree_Index_test.cpp src/BTree_Storage_test.cpp src/Persistence_test.cpp src/LRU_test.cpp src/Iterator.hpp)
target_link_libraries(BPlusTreeTest PRIVATE Catch2::Catch2)
