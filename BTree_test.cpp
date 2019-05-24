//
// Created by Alex Chi on 2019-05-23.
//

#include "BTree.hpp"
#include "catch.hpp"

using Map = BTree<int, int>;

TEST_CASE("Leaf", "[BTree]") {
    Map::Storage storage;
    SECTION("should insert and find") {
        Map::Leaf leaf;
        storage.record(&leaf);
        leaf.insert(3, 3);
        leaf.insert(2, 2);
        leaf.insert(1, 1);
        leaf.insert(4, 4);
        REQUIRE(*leaf.find(1) == 1);
        REQUIRE(*leaf.find(2) == 2);
        REQUIRE(*leaf.find(3) == 3);
        REQUIRE(*leaf.find(4) == 4);
        REQUIRE(leaf.find(5) == nullptr);
    }SECTION("should split") {
        Map::Leaf leaf;
        storage.record(&leaf);
        leaf.insert(3, 3);
        leaf.insert(2, 2);
        leaf.insert(1, 1);
        leaf.insert(4, 4);
        REQUIRE(leaf.should_split());
        int k;
        Map::Leaf *that = leaf.split(k);
        REQUIRE(k == 3);
        REQUIRE(*leaf.find(1) == 1);
        REQUIRE(*leaf.find(2) == 2);
        REQUIRE(*that->find(3) == 3);
        REQUIRE(*that->find(4) == 4);
        REQUIRE(leaf.find(3) == nullptr);
        REQUIRE(leaf.find(4) == nullptr);
        REQUIRE(that->find(1) == nullptr);
        REQUIRE(that->find(2) == nullptr);
        REQUIRE(leaf.next == that->idx);
        REQUIRE(that->prev == leaf.idx);
    }
}

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
}

TEST_CASE("BTree", "[BTree]") {
    SECTION("should insert and find") {
        Map m;
        m.insert(3, 3);
        m.insert(2, 2);
        m.insert(1, 1);
        REQUIRE (*m.find(1) == 1);
        REQUIRE (*m.find(2) == 2);
        REQUIRE (*m.find(3) == 3);
    }

    SECTION("should insert and split") {
        Map m;
        m.insert(3, 3);
        m.insert(2, 2);
        m.insert(1, 1);
        REQUIRE (m.root->is_leaf());
        m.insert(4, 4);
        REQUIRE (*m.find(1) == 1);
        REQUIRE (*m.find(2) == 2);
        REQUIRE (*m.find(3) == 3);
        REQUIRE (*m.find(4) == 4);
        REQUIRE (!m.root->is_leaf());
    }

    SECTION("should insert and split to 3 levels") {
        Map m;
        for (int i = 20; i > 0; i--) {
            m.insert(i, i);
        }
        for (int i = 1; i <= 20; i++) {
            REQUIRE(m.find(i));
            REQUIRE(*m.find(i) == i);
        }
    }

    SECTION("should handle big data") {
        BTree<int, int, 512> m;
        const int test_size = 10000;
        int test_data[10000];
        for (int i = 0; i < test_size; i++) {
            test_data[i] = i;
        }
        for (int i = test_size - 1; i >= 0; i--) {
            int j = rand() % (i + 1);
            std::swap(test_data[i], test_data[j]);
        }
        for (int i = 0; i < test_size; i++) {
            m.insert(i, i);
        }
        for (int i = 0; i < test_size; i++) {
            REQUIRE(m.find(i));
            REQUIRE(i == *m.find(i));
        }
    }
}
