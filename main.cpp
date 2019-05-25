#include <iostream>
#include "BTree.hpp"

int main() {
    const int test_size = 100000;
    remove("persist_long_long.db");
    {
        BTree<int, long long, 512> m("persist_long_long.db");
        for (int i = 0; i < test_size; i++) {
            m.insert(i, i);
        }
    }
    {
        BTree<int, long long, 512> m("persist_long_long.db");
        for (int i = 0; i < test_size; i++) {
            assert (*m.find(i) == i);
            m.remove(i);
        }
    }
    {
        BTree<int, long long, 512> m("persist_long_long.db");
        for (int i = 0; i < test_size; i++) {
            assert (m.find(i) == nullptr);
        }
    }
    remove("persist_long_long.db");
    return 0;
}
