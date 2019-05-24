//
// Created by Alex Chi on 2019-05-23.
//

#include "BTree.hpp"
#include "catch.hpp"

TEST_CASE("BTree", "[BTree]") {

    BTree<int, int> t;

    SECTION("resizing bigger changes size and capacity") {
        REQUIRE(t.Order() == 4);
    }
}