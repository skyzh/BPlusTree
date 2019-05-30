//
// Created by Alex Chi on 2019-05-25.
//

#include "BTree.hpp"
#include "catch.hpp"
#include <cstdio>

using Map = BTree<int, int, 4>;
using BigMap = BTree<int, long long, 512>;
using BigLimitedMap = BTree<int, long long, 512, 32>;

TEST_CASE("Storage", "[Storage]") {
    SECTION("should persist data") {
        remove("persist.db");
        const int test_size = 16;
        {
            Map m("persist.db");
            for (int i = 0; i < test_size; i++) {
                m.insert(i, i);
            }
        }
        {
            Map m("persist.db");
            for (int i = 0; i < test_size; i++) {
                REQUIRE (m.find(i));
                REQUIRE (*m.find(i) == i);
                m.remove(i);
            }
        }
        {
            Map m("persist.db");
            for (int i = 0; i < test_size; i++) {
                REQUIRE (m.find(i) == nullptr);
            }
        }
        remove("persist.db");
    }

    SECTION("should persist even more data") {
        const int test_size = 100000;
        remove("persist_long_long.db");
        {
            BigMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                m.insert(i, i);
            }
        }
        {
            BigMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                REQUIRE (*m.find(i) == i);
                m.remove(i);
            }
        }
        {
            BigMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                REQUIRE (m.find(i) == nullptr);
            }
        }
        remove("persist_long_long.db");
    }

    SECTION("should persist even more data when memory is small") {
        const int test_size = 100000;
        remove("persist_long_long.db");
        {
            BigLimitedMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                m.insert(i, i);
            }
        }
        {
            BigLimitedMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                REQUIRE (*m.find(i) == i);
                m.remove(i);
            }
        }
        {
            BigLimitedMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                REQUIRE (m.find(i) == nullptr);
            }
        }
        remove("persist_long_long.db");
    }
}
