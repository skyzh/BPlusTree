//
// Created by Alex Chi on 2019-05-25.
//

#include "BTree.hpp"
#include "catch.hpp"

using Map = BTree<int, int>;

TEST_CASE("Storage", "[Persistence]") {
    SECTION("should hold enough space") {
        Map m;
        const int test_size = 16;
        for (int i = 0; i < test_size; i++) {
            m.insert(i, i);
        }
        REQUIRE (m.storage.block_used() >= 5);
        for (int i = 0; i < test_size; i++) {
            m.remove(i);
        }
        REQUIRE (m.storage.block_used() == 1);
    }

    SECTION("should persist data") {
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
    }
}
