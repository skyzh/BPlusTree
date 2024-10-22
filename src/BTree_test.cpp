//
// Created by Alex Chi on 2019-05-23.
//

#include <sstream>
#include <catch.hpp>
#include "BTree.hpp"

using Map = BTree<int, int, 4, 65536>;

TEST_CASE("BTree", "[BTree]") {
    SECTION("should use unit test settings") {
        BTree<int, int> t;
        REQUIRE (t.MaxPage() == 1048576);
    }

    SECTION("should insert and query") {
        Map m;
        m.insert(3, 3);
        m.insert(2, 2);
        m.insert(1, 1);
        REQUIRE (*m.query(1) == 1);
        REQUIRE (*m.query(2) == 2);
        REQUIRE (*m.query(3) == 3);
    }

    SECTION("should handle duplicated key") {
        Map m;
        m.insert(3, 3);
        REQUIRE (m.insert(3, 3) == OperationResult::Duplicated);
    }

    SECTION("should insert and split") {
        Map m;
        m.insert(3, 3);
        m.insert(2, 2);
        m.insert(1, 1);
        REQUIRE (m.root()->is_leaf());
        m.insert(4, 4);
        REQUIRE (*m.query(1) == 1);
        REQUIRE (*m.query(2) == 2);
        REQUIRE (*m.query(3) == 3);
        REQUIRE (*m.query(4) == 4);
        REQUIRE (!m.root()->is_leaf());
    }

    SECTION("should insert and split to 3 levels") {
        Map m;
        for (int i = 20; i > 0; i--) {
            m.insert(i, i);
        }
        for (int i = 1; i <= 20; i++) {
            REQUIRE(m.query(i));
            REQUIRE(*m.query(i) == i);
        }
    }

    SECTION("should handle more insert and query") {
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
            REQUIRE(m.query(i));
            REQUIRE(i == *m.query(i));
        }
    }

    SECTION("should remove and query") {
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
                const int *query_result = m.query(idx);
                if (j <= i) REQUIRE (query_result == nullptr);
                else {
                    REQUIRE (query_result);
                    REQUIRE (*query_result == idx);
                }
            }
        }
    }

    SECTION("should handle more remove and query") {
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
                    const int *query_result = m.query(test_data[j]);
                    if (j <= i) REQUIRE(query_result == nullptr);
                    else {
                        REQUIRE(query_result);
                        REQUIRE(*query_result == test_data[j]);
                    }
                }
            }
        }
    }

    SECTION("should handle more insert and query") {
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
            REQUIRE(m.query(i));
            REQUIRE(i == *m.query(i));
        }
        delete[] test_data;
    }

    SECTION("should handle even more remove and query") {
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
            REQUIRE (m.query(i) == nullptr);
        }
        delete[] test_data;
    }

    SECTION("should handle further more insert, remove and query") {
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
            if (i >= threshold) REQUIRE (m.query(test_data[i]) == nullptr);
            else
                REQUIRE (*m.query(test_data[i]) == test_data[i]);
        }
        delete[] test_data;
    }

    SECTION("should align key to 4K") {
        using BTreeInt = BTree<int, int>;
        using BTreeLong = BTree<long long, int>;
        using BTreeChar = BTree<char, int>;
        CHECK (BTreeInt::Index::Storage_Size() <= 4 * 1024);
        CHECK (BTreeLong::Index::Storage_Size() <= 4 * 1024);
        CHECK (BTreeChar::Index::Storage_Size() <= 4 * 1024);
    }
}

TEST_CASE("Serialize", "[BTree]") {
    SECTION("leaf should be serialized") {
        std::stringstream s;
        {
            Map::Leaf leaf;
            leaf.insert(3, 3);
            leaf.insert(2, 2);
            leaf.insert(1, 1);
            leaf.insert(4, 4);
            leaf.serialize(s);
        }
        {
            Map::Leaf leaf;
            leaf.deserialize(s);
            REQUIRE(*leaf.query(1) == 1);
            REQUIRE(*leaf.query(2) == 2);
            REQUIRE(*leaf.query(3) == 3);
            REQUIRE(*leaf.query(4) == 4);
            REQUIRE(leaf.query(5) == nullptr);
        }
    }

    SECTION("index should be serialized") {
        std::stringstream s;
        {
            Map::Index idx;
            idx.children.append(0);
            idx.insert_block(1, 1);
            idx.insert_block(3, 3);
            idx.insert_block(2, 2);
            idx.insert_block(4, 4);
            idx.serialize(s);
        }
        {
            Map::Index idx;
            idx.deserialize(s);
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
