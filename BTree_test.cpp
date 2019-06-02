//
// Created by Alex Chi on 2019-05-23.
//

#include "BTree.hpp"
#include "catch.hpp"

using Map = BTree<int, int, 4, 65536>;

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

    SECTION("should handle more insert and find") {
        BTree<int, int, 512> m;
        const int test_size = 100000;
        int test_data[test_size];
        for (int i = 0; i < test_size; i++) {
            test_data[i] = i;
        }
        for (int i = test_size - 1; i >= 0; i--) {
            int j = rand() % (i + 1);
            std::swap(test_data[i], test_data[j]);
        }
        for (int i = 0; i < test_size; i++) {
            m.insert(test_data[i], test_data[i]);
        }
        for (int i = 0; i < test_size; i++) {
            REQUIRE(m.find(i));
            REQUIRE(i == *m.find(i));
        }
    }

    SECTION("should remove and find") {
        BTree<int, int> m;
        const int test_size = 16;
        int test_data[test_size];
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
            REQUIRE (m.remove(test_data[i]));
            for (int j = 0; j < test_size; j++) {
                int idx = test_data[j];
                int *query_result = m.find(idx);
                if (j <= i) REQUIRE (query_result == nullptr);
                else {
                    REQUIRE (query_result);
                    REQUIRE (*query_result == idx);
                }
            }
        }
    }

    SECTION("should handle more remove and find") {
        BTree<int, int, 512> m;
        const int test_size = 100000;
        int test_data[test_size];
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
            m.remove(test_data[i]);
            if (i % 10000 == 0) {
                for (int j = 0; j < test_size; j++) {
                    int *query_result = m.find(test_data[j]);
                    if (j <= i) REQUIRE(query_result == nullptr);
                    else {
                        REQUIRE(query_result);
                        REQUIRE(*query_result == test_data[j]);
                    }
                }
            }
        }
    }

    SECTION("should handle more insert and find") {
        BTree<int, int, 2048> m;
        const int test_size = 1000000;
        int *test_data = new int[test_size];
        for (int i = 0; i < test_size; i++) {
            test_data[i] = i;
        }
        for (int i = test_size - 1; i >= 0; i--) {
            int j = rand() % (i + 1);
            std::swap(test_data[i], test_data[j]);
        }
        for (int i = 0; i < test_size; i++) {
            m.insert(test_data[i], test_data[i]);
        }
        for (int i = 0; i < test_size; i++) {
            REQUIRE(m.find(i));
            REQUIRE(i == *m.find(i));
        }
        delete[] test_data;
    }

    SECTION("should handle even more remove and find") {
        BTree<int, int, 2048> m;
        const int test_size = 1000000;
        int *test_data = new int[test_size];
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
            m.remove(test_data[i]);
        }
        for (int i = 0; i < test_size; i++) {
            REQUIRE (m.find(i) == nullptr);
        }
        delete[] test_data;
    }

    SECTION("should handle further more insert, remove and find") {
        BTree<int, int, 2048> m;
        const int test_size = 10000;
        int *test_data = new int[test_size];
        for (int i = 0; i < test_size; i++) {
            test_data[i] = i;
        }
        for (int i = test_size - 1; i >= 0; i--) {
            int j = rand() % (i + 1);
            std::swap(test_data[i], test_data[j]);
        }
        const int threshold = 5000;
        for (int i = 0; i < threshold; i++) {
            m.insert(test_data[i], test_data[i]);
        }
        for (int j = 0; j < 5; j++) {
            for (int i = threshold; i < test_size; i++) {
                m.insert(test_data[i], test_data[i]);
            }
            for (int i = threshold; i < test_size; i++) {
                m.remove(test_data[i]);
            }
        }
        for (int i = 0; i < test_size; i++) {
            if (i >= threshold) REQUIRE (m.find(test_data[i]) == nullptr);
            else REQUIRE (*m.find(test_data[i]) == test_data[i]);
        }
        delete[] test_data;
    }
}

TEST_CASE("Serialize", "[BTree]") {
    SECTION("leaf should be serialized") {
        char memory[Map::Leaf::Storage_Size()];
        {
            Map::Leaf leaf;
            leaf.insert(3, 3);
            leaf.insert(2, 2);
            leaf.insert(1, 1);
            leaf.insert(4, 4);
            leaf.serialize(memory);
        }
        {
            Map::Leaf leaf;
            leaf.deserialize(memory);
            REQUIRE(*leaf.find(1) == 1);
            REQUIRE(*leaf.find(2) == 2);
            REQUIRE(*leaf.find(3) == 3);
            REQUIRE(*leaf.find(4) == 4);
            REQUIRE(leaf.find(5) == nullptr);
        }
    }

    SECTION("index should be serialized") {
        char memory[Map::Index::Storage_Size()];
        {
            Map::Index idx;
            idx.children.append(0);
            idx.insert_block(1, 1);
            idx.insert_block(3, 3);
            idx.insert_block(2, 2);
            idx.insert_block(4, 4);
            idx.serialize(memory);
        }
        {
            Map::Index idx;
            idx.deserialize(memory);
            REQUIRE (idx.keys.size == 4);
            REQUIRE (idx.keys[0] == 1);
            REQUIRE (idx.keys[1] == 2);
            REQUIRE (idx.keys[2] == 3);
            REQUIRE (idx.keys[3] == 4);
            REQUIRE (idx.children.size == 5);
            REQUIRE (idx.children[0] == 0);
            REQUIRE (idx.children[1] == 1);
            REQUIRE (idx.children[2] == 2);
            REQUIRE (idx.children[3] == 3);
            REQUIRE (idx.children[4] == 4);
        }
    }
}

TEST_CASE("Iterator", "[BTree]") {
    SECTION("should get data") {
        BTree<int, int, 512> m;
        const int test_size = 100000;
        int test_data[test_size];
        for (int i = 0; i < test_size; i++) {
            m.insert(i, i);
        }
        auto iter = m.begin();
        for (int i = 0; i < test_size; i++) {
            REQUIRE (iter.get() == i);
            iter.next();
        }
    }
}