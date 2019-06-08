#include <iostream>
#include "BTree.hpp"
#include <cstdio>
#include <ctime>
#include <algorithm>

#include "bench1.hpp"
#include "bench2.hpp"

int main() {
    remove("data.db");
    bench1::bench_1();
    return 0;
}
