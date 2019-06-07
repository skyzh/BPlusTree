//
// Created by Alex Chi on 2019-05-30.
//

#include "catch.hpp"
#include <cstdio>
#include "Persistence.hpp"
#include "LRU.hpp"

TEST_CASE("LRU", "[Persistence]") {
    SECTION("should expire correctly") {
        LRU<64> lru;
        lru.put(0);
        lru.put(1);
        lru.put(2);
        lru.put(3);
        REQUIRE(lru.expire() == 0);
        lru.remove(lru.expire());
        lru.get(2);
        REQUIRE(lru.expire() == 1);
        lru.remove(lru.expire());
        lru.get(3);
        REQUIRE(lru.expire() == 2);
        lru.remove(lru.expire());
        REQUIRE(lru.expire() == 3);
        lru.remove(lru.expire());
        REQUIRE(lru.size == 0);
        lru.put(0);
        REQUIRE(lru.expire() == 0);
    }
}