//
// Created by Alex Chi on 2019-06-08.
//

#include <catch.hpp>
#include "BTree.hpp"
#include <cstdio>

using Map = BTree<int, int, 256, 8>;

TEST_CASE("Iterator", "[Iterator]") {
    SECTION("should get data") {
        BTree<int, int, 512> m;
        const int test_size = 100000;
        int test_data[test_size];
        for (int i = 0; i < test_size; i++) {
            m.insert(i, i);
        }
        auto iter = m.begin();
        for (int i = 0; i < test_size; i++) {
            REQUIRE (*iter == i);
            ++iter;
        }
    }

    SECTION("should get data after removal") {
        BTree<int, int, 512> m;
        const int test_size = 100000;
        const int remove_size = 5000;
        int test_data[test_size];
        for (int i = 0; i < test_size; i++) {
            m.insert(i, i);
        }
        for (int i = 0; i < remove_size; i++) {
            m.remove(i);
        }
        auto iter = m.begin();
        for (int i = remove_size; i < test_size; i++) {
            REQUIRE (*iter == i);
            ++iter;
        }
    }

    SECTION("should equal") {
        const int test_size = 100;
        Map m;
        for (int i = 0; i < test_size; i++) {
            m.insert(i, i);
        }
        REQUIRE (m.begin() == m.begin());
        REQUIRE (m.end() == m.end());
        REQUIRE (m.cbegin() == m.cbegin());
        REQUIRE (m.cend() == m.cend());
    }

    SECTION("should access data sequentially") {
        const int test_size = 1000;
        Map m;
        for (int i = 0; i < test_size; i++) {
            m.insert(i, i);
        }
        int i = 0;
        auto iter1 = m.begin();
        auto iter2 = m.begin();
        auto iter3 = m.cbegin();
        auto iter4 = m.cbegin();

        for (; iter1 != m.end();
               ++iter1, iter2++, ++iter3, iter4++, ++i) {
            REQUIRE (*iter1 == i);
            REQUIRE (*iter2 == i);
            REQUIRE (*iter3 == i);
            REQUIRE (*iter4 == i);
            REQUIRE (iter1 == iter2);
            REQUIRE (iter3 == iter4);
        }

        --iter1; --iter2; --iter3; --iter4; --i;
        for (; iter1 != m.begin();
               --iter1, iter2--, --iter3, iter4--, --i) {
            REQUIRE (*iter1 == i);
            REQUIRE (*iter2 == i);
            REQUIRE (*iter3 == i);
            REQUIRE (*iter4 == i);
            REQUIRE (iter1 == iter2);
            REQUIRE (iter3 == iter4);
        }
    }

    SECTION("should access data sequentially with offload") {
        const int test_size = 10000;
        remove("iterator.test");
        {
            Map m("iterator.test");
            for (int i = 0; i < test_size; i++) {
                m.insert(i, i);
            }
            int i = 0;
            auto iter1 = m.begin();
            auto iter2 = m.begin();
            auto iter3 = m.cbegin();
            auto iter4 = m.cbegin();

            for (; iter1 != m.end();
                   ++iter1, iter2++, ++iter3, iter4++, ++i) {
                REQUIRE (*iter1 == i);
                REQUIRE (*iter2 == i);
                REQUIRE (*iter3 == i);
                REQUIRE (*iter4 == i);
                REQUIRE (iter1 == iter2);
                REQUIRE (iter3 == iter4);
            }

            --iter1; --iter2; --iter3; --iter4; --i;
            for (; iter1 != m.begin();
                   --iter1, iter2--, --iter3, iter4--, --i) {
                REQUIRE (*iter1 == i);
                REQUIRE (*iter2 == i);
                REQUIRE (*iter3 == i);
                REQUIRE (*iter4 == i);
                REQUIRE (iter1 == iter2);
                REQUIRE (iter3 == iter4);
            }
            CHECK(m.storage->stat.swap_out > 10);
            for (int i = 0; i < test_size; i++) {
                REQUIRE (*m.find(i) == i);
            }
        }

        {
            Map m("iterator.test");
            int i = 0;
            auto iter1 = m.begin();
            auto iter2 = m.begin();
            auto iter3 = m.cbegin();
            auto iter4 = m.cbegin();

            for (; iter1 != m.end();
                   ++iter1, iter2++, ++iter3, iter4++, ++i) {
                REQUIRE (*iter1 == i);
                REQUIRE (*iter2 == i);
                REQUIRE (*iter3 == i);
                REQUIRE (*iter4 == i);
                REQUIRE (iter1 == iter2);
                REQUIRE (iter3 == iter4);
            }

            --iter1; --iter2; --iter3; --iter4; --i;
            for (; iter1 != m.begin();
                   --iter1, iter2--, --iter3, iter4--, --i) {
                REQUIRE (*iter1 == i);
                REQUIRE (*iter2 == i);
                REQUIRE (*iter3 == i);
                REQUIRE (*iter4 == i);
                REQUIRE (iter1 == iter2);
                REQUIRE (iter3 == iter4);
            }
            CHECK(m.storage->stat.swap_out > 10);
            for (int i = 0; i < test_size; i++) {
                REQUIRE (*m.find(i) == i);
            }
        }

        remove("iterator.test");
    }
}

