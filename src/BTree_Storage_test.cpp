//
// Created by Alex Chi on 2019-05-25.
//

#include <catch.hpp>
#include "BTree.hpp"
#include <cstdio>
#include <algorithm>

using Map = BTree<int, int, 4, 65536>;
using BigMap = BTree<int, long long, 512, 65536>;
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
                REQUIRE (m.query(i));
                REQUIRE (*m.query(i) == i);
                m.remove(i);
            }
        }
        {
            Map m("persist.db");
            for (int i = 0; i < test_size; i++) {
                REQUIRE (m.query(i) == nullptr);
            }
        }
        remove("persist.db");
    }

    SECTION("should persist size") {
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
            REQUIRE (m.size() == test_size);
        }
        remove("persist.db");
    }

    SECTION("should persist data when root is offloaded") {
        remove("persist.db");
        const int test_size = 16;
        {
            Map m("persist.db");
            for (int i = 0; i < test_size; i++) {
                if (m.storage->is_loaded((m.root_idx()))) m.storage->offload_page(m.root_idx());
                m.insert(i, i);
            }
        }
        {
            Map m("persist.db");
            for (int i = 0; i < test_size; i++) {
                if (m.storage->is_loaded((m.root_idx()))) m.storage->offload_page(m.root_idx());
                REQUIRE (m.query(i));
                REQUIRE (*m.query(i) == i);
                m.remove(i);
            }
        }
        {
            Map m("persist.db");
            for (int i = 0; i < test_size; i++) {
                if (m.storage->is_loaded((m.root_idx()))) m.storage->offload_page(m.root_idx());
                REQUIRE (m.query(i) == nullptr);
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
                REQUIRE (*m.query(i) == i);
                m.remove(i);
            }
        }
        {
            BigMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                REQUIRE (m.query(i) == nullptr);
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
                REQUIRE (*m.query(i) == i);
                m.remove(i);
            }
        }
        {
            BigLimitedMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                m.insert(i, i);
            }
        }
        {
            BigLimitedMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                REQUIRE (*m.query(i) == i);
                m.remove(i);
            }
        }
        {
            BigLimitedMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                REQUIRE (m.query(i) == nullptr);
            }
        }
        remove("persist_long_long.db");
    }


    SECTION("should use empty slot") {
        const int test_size = 100000;
        remove("persist_long_long.db");
        {
            BigLimitedMap m("persist_long_long.db");
            for (int i = 0; i < test_size; i++) {
                m.insert(i, i);
            }
            unsigned long lst_page_use = 0;
            for (int i = 0; i < m.MaxPage(); i++) {
                if (m.storage->pages[i] != nullptr)
                    lst_page_use = std::max(lst_page_use, m.storage->persistence_index->page_offset[i]);
            }
            for (int i = 0; i < test_size; i++) m.remove(i);
            for (int i = 0; i < test_size; i++) m.insert(i, i);
            for (int i = 0; i < test_size; i++) m.remove(i);
            for (int i = 0; i < test_size; i++) m.insert(i, i);
            unsigned long _lst_page_use = 0;
            for (int i = 0; i < m.MaxPage(); i++) {
                if (m.storage->pages[i] != nullptr)
                    _lst_page_use = std::max(_lst_page_use, m.storage->persistence_index->page_offset[i]);
            }
            REQUIRE(_lst_page_use - lst_page_use <= 1000);
        }
        remove("persist_long_long.db");
    }
}
