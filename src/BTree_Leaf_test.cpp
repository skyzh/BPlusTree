//
// Created by Alex Chi on 2019-05-25.
//

#include <catch.hpp>
#include "BTree.hpp"

using Map = BTree<int, int, 4, 65536>;

TEST_CASE("Leaf", "[Leaf]") {
    Map::BPersistence *storage = new Map::BPersistence;
    SECTION("should insert and query") {
        Map::Leaf leaf;
        storage->record(&leaf);
        leaf.insert(3, 3);
        leaf.insert(2, 2);
        leaf.insert(1, 1);
        leaf.insert(4, 4);
        REQUIRE(*leaf.query(1) == 1);
        REQUIRE(*leaf.query(2) == 2);
        REQUIRE(*leaf.query(3) == 3);
        REQUIRE(*leaf.query(4) == 4);
        REQUIRE(leaf.query(5) == nullptr);
    }

    SECTION("should split") {
        Map::Leaf leaf;
        storage->record(&leaf);
        leaf.insert(3, 3);
        leaf.insert(2, 2);
        leaf.insert(1, 1);
        leaf.insert(4, 4);
        REQUIRE(leaf.should_split());
        int k;
        Map::Leaf *that = leaf.split(k);
        REQUIRE(k == 3);
        REQUIRE(*leaf.query(1) == 1);
        REQUIRE(*leaf.query(2) == 2);
        REQUIRE(*that->query(3) == 3);
        REQUIRE(*that->query(4) == 4);
        REQUIRE(leaf.query(3) == nullptr);
        REQUIRE(leaf.query(4) == nullptr);
        REQUIRE(that->query(1) == nullptr);
        REQUIRE(that->query(2) == nullptr);
        REQUIRE(leaf.next == that->idx);
        REQUIRE(that->prev == leaf.idx);
    }

    SECTION("should borrow from left") {
        Map::Leaf leaf1, leaf2;
        storage->record(&leaf1);
        storage->record(&leaf2);
        leaf1.insert(1, 1);
        leaf1.insert(2, 2);
        leaf2.insert(3, 3);
        leaf2.insert(4, 4);
        leaf1.next = leaf2.idx;
        leaf2.prev = leaf1.idx;
        REQUIRE(leaf2.borrow_from_left(&leaf1, 0) == 2);
        REQUIRE(leaf1.keys.size == 1);
        REQUIRE(leaf1.data.size == 1);
        REQUIRE(leaf2.keys.size == 3);
        REQUIRE(leaf2.data.size == 3);
        REQUIRE(*leaf1.query(1) == 1);
        REQUIRE(*leaf2.query(2) == 2);
        REQUIRE(leaf2.keys[0] == 2);
        REQUIRE(leaf2.keys[1] == 3);
        REQUIRE(leaf2.keys[2] == 4);
        REQUIRE(*leaf2.query(3) == 3);
        REQUIRE(*leaf2.query(4) == 4);
    }

    SECTION("should borrow from right") {
        Map::Leaf leaf1, leaf2;
        storage->record(&leaf1);
        storage->record(&leaf2);
        leaf1.insert(1, 1);
        leaf1.insert(2, 2);
        leaf2.insert(3, 3);
        leaf2.insert(4, 4);
        leaf1.next = leaf2.idx;
        leaf2.prev = leaf1.idx;
        REQUIRE(leaf1.borrow_from_right(&leaf2, 0) == 4);
        REQUIRE(leaf1.keys.size == 3);
        REQUIRE(leaf2.keys.size == 1);
        REQUIRE(leaf1.data.size == 3);
        REQUIRE(leaf2.data.size == 1);
        REQUIRE(*leaf1.query(1) == 1);
        REQUIRE(*leaf1.query(2) == 2);
        REQUIRE(*leaf1.query(3) == 3);
        REQUIRE(*leaf2.query(4) == 4);
    }

    SECTION("should merge with left") {
        Map::Leaf leaf1, leaf2;
        storage->record(&leaf1);
        storage->record(&leaf2);
        leaf1.insert(1, 1);
        leaf1.insert(2, 2);
        leaf2.insert(3, 3);
        leaf2.insert(4, 4);
        leaf1.next = leaf2.idx;
        leaf2.prev = leaf1.idx;
        leaf2.merge_with_left(&leaf1, 0);
        REQUIRE(leaf1.keys.size == 0);
        REQUIRE(leaf1.data.size == 0);
        REQUIRE(leaf2.keys.size == 4);
        REQUIRE(leaf2.data.size == 4);
        REQUIRE(*leaf2.query(1) == 1);
        REQUIRE(*leaf2.query(2) == 2);
        REQUIRE(*leaf2.query(3) == 3);
        REQUIRE(*leaf2.query(4) == 4);
    }

    SECTION("should merge with right") {
        Map::Leaf leaf1, leaf2;
        storage->record(&leaf1);
        storage->record(&leaf2);
        leaf1.insert(1, 1);
        leaf1.insert(2, 2);
        leaf2.insert(3, 3);
        leaf2.insert(4, 4);
        leaf1.next = leaf2.idx;
        leaf2.prev = leaf1.idx;
        leaf1.merge_with_right(&leaf2, 0);
        REQUIRE(leaf1.keys.size == 4);
        REQUIRE(leaf1.data.size == 4);
        REQUIRE(leaf2.keys.size == 0);
        REQUIRE(leaf2.data.size == 0);
        REQUIRE(*leaf1.query(1) == 1);
        REQUIRE(*leaf1.query(2) == 2);
        REQUIRE(*leaf1.query(3) == 3);
        REQUIRE(*leaf1.query(4) == 4);
    }

    SECTION("should remove") {
        Map::Leaf leaf;
        storage->record(&leaf);
        leaf.insert(1, 1);
        leaf.insert(2, 2);
        leaf.insert(3, 3);
        leaf.insert(4, 4);
        leaf.remove(1);
        REQUIRE(leaf.keys.size == 3);
        REQUIRE(leaf.data.size == 3);
        REQUIRE(leaf.query(1) == nullptr);
        REQUIRE(!leaf.remove(1));
    }
}
