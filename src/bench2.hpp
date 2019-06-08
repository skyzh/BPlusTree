//
// Created by Alex Chi on 2019-06-08.
//

#include "benchmark.hpp"

namespace bench2 {
    const int test_size = 7 * 1e8;

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
}