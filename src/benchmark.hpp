//
// Created by Alex Chi on 2019-06-08.
//

#ifndef BPLUSTREE_BENCHMARK_HPP
#define BPLUSTREE_BENCHMARK_HPP

#include <iostream>
#include "BTree.hpp"
#include <cstdio>
#include <ctime>
#include <algorithm>

using BigTable = BTree<int, int>;
BigTable *m;


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


inline void progress(long long i, long long n, BigTable *t) {
    if (n <= 10000000) return;
    if ((i + 1) % (n / 100) == 0) {
        std::cout << "    [" << i << "/" << n << "] " << i * 100 / n << "%" << std::endl;
        t->storage->stat.stat();
    }
}

#endif //BPLUSTREE_BENCHMARK_HPP
