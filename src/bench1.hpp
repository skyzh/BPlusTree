//
// Created by Alex Chi on 2019-06-08.
//

#include "benchmark.hpp"

namespace bench1 {
    const int test_size = 3 * 1e7;
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
}