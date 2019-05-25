//
// Created by Alex Chi on 2019-05-25.
//

#include "BTree.hpp"
#include "catch.hpp"

using Map = BTree<int, int>;

TEST_CASE("Index", "[BTree]") {
    Map::Storage storage;
    SECTION("should split") {
        Map::Index idx;
        storage.record(&idx);
        idx.children.append(0);
        idx.insert_block(1, 1);
        idx.insert_block(3, 3);
        idx.insert_block(2, 2);
        idx.insert_block(4, 4);
        int k;
        Map::Index *that = idx.split(k);
        REQUIRE (k == 2);
        REQUIRE (idx.keys.size == 1);
        REQUIRE (idx.keys[0] == 1);
        REQUIRE (that->keys.size == 2);
        REQUIRE (that->keys[0] == 3);
        REQUIRE (that->keys[1] == 4);

        REQUIRE (idx.children[0] == 0);
        REQUIRE (idx.children[1] == 1);
        REQUIRE (that->children[0] == 2);
        REQUIRE (that->children[1] == 3);
        REQUIRE (that->children[2] == 4);
    }

    SECTION("should borrow from left") {
        Map::Index idx1;
        storage.record(&idx1);
        idx1.children.append(0);
        idx1.insert_block(1, 1);
        idx1.insert_block(2, 2);
        idx1.insert_block(3, 3);

        Map::Index idx2;
        storage.record(&idx2);
        idx2.children.append(4);
        idx2.insert_block(5, 5);
        idx2.insert_block(6, 6);
        idx2.insert_block(7, 7);

        REQUIRE (idx2.borrow_from_left(&idx1, 4) == 3);
        REQUIRE (idx1.keys.size == 2);
        REQUIRE (idx1.children.size == 3);
        REQUIRE (idx2.keys.size == 4);
        REQUIRE (idx2.children.size == 5);
        int idx1_keys_should_be[] = {1, 2};
        for (int i = 0; i < idx1.keys.size; i++) REQUIRE(idx1.keys[i] == idx1_keys_should_be[i]);
        int idx1_children_should_be[] = {0, 1, 2};
        for (int i = 0; i < idx1.children.size; i++) REQUIRE(idx1.children[i] == idx1_children_should_be[i]);
        int idx2_keys_should_be[] = {4, 5, 6, 7};
        for (int i = 0; i < idx2.keys.size; i++) REQUIRE(idx2.keys[i] == idx2_keys_should_be[i]);
        int idx2_children_should_be[] = {3, 4, 5, 6, 7};
        for (int i = 0; i < idx2.children.size; i++) REQUIRE(idx2.children[i] == idx2_children_should_be[i]);
    }

    SECTION("should borrow from right") {
        Map::Index idx1;
        storage.record(&idx1);
        idx1.children.append(0);
        idx1.insert_block(1, 1);
        idx1.insert_block(2, 2);
        idx1.insert_block(3, 3);

        Map::Index idx2;
        storage.record(&idx2);
        idx2.children.append(4);
        idx2.insert_block(5, 5);
        idx2.insert_block(6, 6);
        idx2.insert_block(7, 7);

        REQUIRE (idx1.borrow_from_right(&idx2, 4) == 5);
        REQUIRE (idx1.keys.size == 4);
        REQUIRE (idx1.children.size == 5);
        REQUIRE (idx2.keys.size == 2);
        REQUIRE (idx2.children.size == 3);
        int idx1_keys_should_be[] = {1, 2, 3, 4};
        for (int i = 0; i < idx1.keys.size; i++) REQUIRE(idx1.keys[i] == idx1_keys_should_be[i]);
        int idx1_children_should_be[] = {0, 1, 2, 3, 4};
        for (int i = 0; i < idx1.children.size; i++) REQUIRE(idx1.children[i] == idx1_children_should_be[i]);
        int idx2_keys_should_be[] = {6, 7};
        for (int i = 0; i < idx2.keys.size; i++) REQUIRE(idx2.keys[i] == idx2_keys_should_be[i]);
        int idx2_children_should_be[] = {5, 6, 7};
        for (int i = 0; i < idx2.children.size; i++) REQUIRE(idx2.children[i] == idx2_children_should_be[i]);
    }

    SECTION("should merge with left") {
        Map::Index idx1;
        storage.record(&idx1);
        idx1.children.append(0);
        idx1.insert_block(1, 1);
        idx1.insert_block(2, 2);

        Map::Index idx2;
        storage.record(&idx2);
        idx2.children.append(3);
        idx2.insert_block(4, 4);

        idx2.merge_with_left(&idx1, 3);
        REQUIRE (idx1.keys.size == 0);
        REQUIRE (idx1.children.size == 0);
        REQUIRE (idx2.keys.size == 4);
        REQUIRE (idx2.children.size == 5);
        int idx2_keys_should_be[] = {1, 2, 3, 4};
        for (int i = 0; i < idx2.keys.size; i++) REQUIRE(idx2.keys[i] == idx2_keys_should_be[i]);
        int idx2_children_should_be[] = {0, 1, 2, 3, 4};
        for (int i = 0; i < idx2.children.size; i++) REQUIRE(idx2.children[i] == idx2_children_should_be[i]);
    }

    SECTION("should merge with right") {
        Map::Index idx1;
        storage.record(&idx1);
        idx1.children.append(0);
        idx1.insert_block(1, 1);
        idx1.insert_block(2, 2);

        Map::Index idx2;
        storage.record(&idx2);
        idx2.children.append(3);
        idx2.insert_block(4, 4);

        idx1.merge_with_right(&idx2, 3);
        REQUIRE (idx1.keys.size == 4);
        REQUIRE (idx1.children.size == 5);
        REQUIRE (idx2.keys.size == 0);
        REQUIRE (idx2.children.size == 0);
        int idx1_keys_should_be[] = {1, 2, 3, 4};
        for (int i = 0; i < idx1.keys.size; i++) REQUIRE(idx1.keys[i] == idx1_keys_should_be[i]);
        int idx1_children_should_be[] = {0, 1, 2, 3, 4};
        for (int i = 0; i < idx1.children.size; i++) REQUIRE(idx1.children[i] == idx1_children_should_be[i]);
    }
}