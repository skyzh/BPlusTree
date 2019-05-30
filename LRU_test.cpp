//
// Created by Alex Chi on 2019-05-30.
//

#include "catch.hpp"
#include <cstdio>
#include "Persistence.hpp"
#include "LRU.hpp"

TEST_CASE("LRU", "[Persistence]") {
    SECTION("should expire correctly") {
        LRU<> lru;
        using Node = LRU<>::Node;
        Node *ptr0 = lru.put(0);
        Node *ptr1 = lru.put(1);
        Node *ptr2 = lru.put(2);
        Node *ptr3 = lru.put(3);
        REQUIRE(lru.expire() == 0);
        lru.get(ptr2);
        REQUIRE(lru.expire() == 1);
        lru.get(ptr3);
        REQUIRE(lru.expire() == 2);
        REQUIRE(lru.expire() == 3);
        REQUIRE(lru.size == 0);
        lru.put(0);
        REQUIRE(lru.expire() == 0);
    }
}