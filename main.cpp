#include <iostream>
#include "BTree.hpp"
#include <cstdio>

double update_clock() {
    static clock_t start = std::clock();
    double duration;
    duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;
    start = std::clock();
    return duration;
}

BTree<int, int> *m;

void test_insert() {
    m = new BTree<int, int>("data.db");
    const int test_size = 1e7;
    update_clock();
    for (int i = 0; i < test_size; i++) {
        m->insert(i, i);
    }
    std::cout << "Insertion: " << update_clock() << std::endl;
    m->storage->stat.stat();
    delete m;
    std::cout << "Save: " << update_clock() << std::endl;
}

void test_remove() {
    m = new BTree<int, int>("data.db");
    const int remove_size = 1e7;
    std::cout << "Read: " << update_clock() << std::endl;
    for (int i = 0; i < remove_size; i++) {
        m->find(i);
    }
    std::cout << "Find: " << update_clock() << std::endl;
    for (int i = 0; i < remove_size; i++) {
        m->remove(i);
    }
    std::cout << "Removal: " << update_clock() << std::endl;
    m->storage->stat.stat();
    delete m;
}

void test_no_persistence() {
    m = new BTree<int, int>;
    const int test_size = 1e7;
    update_clock();
    int *test_data = new int[test_size];
    for (int i = 0; i < test_size; i++) {
        test_data[i] = i;
    }
    for (int i = test_size - 1; i >= 0; i--) {
        int j = rand() % (i + 1);
        std::swap(test_data[i], test_data[j]);
    }
    std::cout << "Generation: " << update_clock() << std::endl;
    for (int i = 0; i < test_size; i++) {
        m->insert(test_data[i], test_data[i]);
    }
    std::cout << "Insertion: " << update_clock() << std::endl;
    m->storage->stat.stat();
    for (int i = 0; i < test_size; i++) {
        m->find(i);
    }
    std::cout << "Find: " << update_clock() << std::endl;
    m->storage->stat.stat();
    for (int i = 0; i < test_size; i++) {
        m->remove(test_data[i]);
    }
    std::cout << "Removal: " << update_clock() << std::endl;
    m->storage->stat.stat();
    delete m;
}

int main() {
    remove("data.db");
    test_no_persistence();
    return 0;
}
