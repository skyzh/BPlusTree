#include <iostream>
#include "BTree.hpp"
#include <cstdio>
#include <ctime>
#include <algorithm>

double update_clock() {
    static clock_t start = std::clock();
    double duration;
    duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;
    start = std::clock();
    return duration;
}

void print_clock(const char *message) {
    std::cout << "[" << message << "] " << update_clock() << "s" << std::endl;
}

BTree<int, int> *m;

const int test_size = 1e6;
int test_data[test_size];

void generate_test_data() {
    for (int i = 0; i < test_size; i++) test_data[i] = i;
    for (int i = test_size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(test_data[i], test_data[j]);
    }
}

void test_insert() {
    m = new BTree<int, int>("data.db");
    update_clock();
    for (int i = 0; i < test_size; i++) {
        m->insert(test_data[i], test_data[i]);
    }
    print_clock("Insertion");
    m->storage->stat.stat();
    delete m;
    print_clock("Save");
}

void test_remove() {
    m = new BTree<int, int>("data.db");
    print_clock("Read");
    for (int i = 0; i < test_size; i++) {
        m->find(test_data[i]);
    }
    print_clock("Find");
    m->storage->stat.stat();
    for (int i = 0; i < test_size; i++) {
        m->remove(test_data[i]);
    }
    print_clock("Remove");
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
    for (int i = 0; i < test_size; i++) {
        m->find(i);
    }
    std::cout << "Find: " << update_clock() << std::endl;
    for (int i = 0; i < test_size; i++) {
        m->remove(test_data[i]);
    }
    std::cout << "Removal: " << update_clock() << std::endl;
    m->storage->stat.stat();
    delete m;
}

int main() {
    remove("data.db");
    generate_test_data();
    update_clock();
    std::sort(test_data, test_data + test_size);
    print_clock("Sort");
    generate_test_data();
    std::cout << "Generation complete" << std::endl;
    test_insert();
    test_remove();

    // test_no_persistence();
    return 0;
}
