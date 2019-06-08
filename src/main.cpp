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

using BigTable = BTree<int, int, 512, 1572864, 16777216>;
BigTable *m;

inline void progress(long long i, long long n, BigTable *t) {
    if ((i + 1) % (n / 100) == 0) {
        std::cout << "    [" << i << "/" << n << "] " << i * 100 / n << "%" << std::endl;
        t->storage->stat.stat();
    }
}


const int test_size = 1e8;
int test_data[test_size];

void generate_test_data() {
    for (int i = 0; i < test_size; i++) test_data[i] = i;
    for (int i = test_size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(test_data[i], test_data[j]);
    }
}

void test_insert() {
    m = new BigTable("data.db");
    update_clock();
    for (int i = 0; i < test_size; i++) {
        m->insert(test_data[i], test_data[i]);
        progress(i, test_size, m);
    }
    print_clock("Insertion");
    m->storage->stat.stat();
    delete m;
    print_clock("Save");
}

void test_remove() {
    m = new BigTable("data.db");
    print_clock("Read");
    for (int i = 0; i < test_size; i++) {
        m->find(test_data[i]);
        progress(i, test_size, m);
    }
    print_clock("Find");
    m->storage->stat.stat();
    for (int i = 0; i < test_size; i++) {
        m->remove(test_data[i]);
        progress(i, test_size, m);
    }
    print_clock("Remove");
    m->storage->stat.stat();
    delete m;
}

void bench_1() {
    remove("data.db");
    generate_test_data();
    update_clock();
    std::sort(test_data, test_data + test_size);
    print_clock("Sort");
    generate_test_data();
    std::cout << "Generation complete" << std::endl;
    test_insert();
    test_remove();
}

/*
const int test_size = 2 * 1e8;

void test_insert() {
    m = new BigTable("data.db");
    update_clock();
    for (int i = 0; i < test_size; i++) {
        m->insert(i, i);
        progress(i, test_size, m);
    }
    print_clock("Insertion");
    m->storage->stat.stat();
    delete m;
    print_clock("Save");
}

void test_remove() {
    m = new BigTable("data.db");
    print_clock("Read");
    for (int i = 0; i < test_size; i++) {
        m->find(i);
        progress(i, test_size, m);
    }
    print_clock("Find");
    m->storage->stat.stat();
    for (int i = 0; i < test_size; i++) {
        m->remove(i);
        progress(i, test_size, m);
    }
    print_clock("Remove");
    m->storage->stat.stat();
    delete m;
}

 void bench_2() {
 update_clock();
    test_insert();
    test_remove();
    }
*/
int main() {
    remove("data.db");
    bench_1();
    return 0;
}