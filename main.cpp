#include <iostream>
#include "BTree.hpp"

using Map = BTree<int, int>;

int main() {
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
        assert(m.remove(test_data[i]));
        m.debug(m.root);
        std::cerr << test_data[i] << " removed" << std::endl;
        for (int j = 0; j < test_size; j++) {
            int idx = test_data[j];
            int *query_result = m.find(idx);
            if (j <= i) assert(query_result == nullptr);
            else {
                assert(query_result);
                assert(*query_result == idx);
            }
        }
    }
    return 0;
}
