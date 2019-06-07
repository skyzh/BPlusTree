//
// Created by Alex Chi on 2019-05-24.
//

#include <catch.hpp>
#include "Container.hpp"
#include <sstream>

struct TestClass {
    static int construct_cnt;
    static int destruct_cnt;
    int x;

    TestClass(int x) : x(x) { ++construct_cnt; }

    TestClass(const TestClass &that) : x(that.x) { ++construct_cnt; }

    friend bool operator==(const TestClass &x, int y) { return x.x == y; }

    ~TestClass() { ++destruct_cnt; }
};

int TestClass::construct_cnt = 0;
int TestClass::destruct_cnt = 0;

TEST_CASE("Vector", "[Vector]") {

    SECTION("should save data") {
        Vector<int, 8> v;
        v.append(1);
        v.append(2);
        v.append(3);
        v.append(4);
        v.append(5);
        v.append(6);
        v.append(7);
        v.append(8);
        REQUIRE (v[0] == 1);
        REQUIRE (v[1] == 2);
        REQUIRE (v[2] == 3);
        REQUIRE (v[3] == 4);
        REQUIRE (v[4] == 5);
        REQUIRE (v[5] == 6);
        REQUIRE (v[6] == 7);
        REQUIRE (v[7] == 8);
    }

    SECTION("should save data without default constructor", "[Vector]") {
        TestClass::construct_cnt = 0;
        TestClass::destruct_cnt = 0;
        {
            Vector<TestClass, 8> v;
            v.append(TestClass(1));
            v.append(TestClass(2));
            v.append(TestClass(3));
            v.append(TestClass(4));
            v.append(TestClass(5));
            v.append(TestClass(6));
            v.append(TestClass(7));
            v.append(TestClass(8));
            REQUIRE (v[0] == 1);
            REQUIRE (v[1] == 2);
            REQUIRE (v[2] == 3);
            REQUIRE (v[3] == 4);
            REQUIRE (v[4] == 5);
            REQUIRE (v[5] == 6);
            REQUIRE (v[6] == 7);
            REQUIRE (v[7] == 8);
            REQUIRE (v.size == 8);
        }
        REQUIRE (TestClass::construct_cnt == 16);
        REQUIRE (TestClass::destruct_cnt == 16);
    }

    SECTION("should insert", "[Vector]") {
        Vector<int, 8> v;
        v.append(2);
        v.append(4);
        v.append(6);
        v.insert(0, 1);
        v.insert(2, 3);
        v.insert(5, 7);
        REQUIRE (v[0] == 1);
        REQUIRE (v[1] == 2);
        REQUIRE (v[2] == 3);
        REQUIRE (v[3] == 4);
        REQUIRE (v[4] == 6);
        REQUIRE (v[5] == 7);
    }

    SECTION("should pop", "[Vector]") {
        Vector<int, 8> v;
        v.append(2);
        v.append(4);
        v.append(6);
        REQUIRE (v.pop() == 6);
        REQUIRE (v[0] == 2);
        REQUIRE (v[1] == 4);
    }

    SECTION("should move from", "[Vector]") {
        Vector<int, 8> v;
        Vector<int, 8> b;
        v.append(1);
        v.append(2);
        v.append(3);
        v.append(4);
        v.append(5);
        v.append(6);
        v.append(7);
        b.move_from(v, 1, 3);
        REQUIRE (v[0] == 1);
        REQUIRE (v[1] == 5);
        REQUIRE (v[2] == 6);
        REQUIRE (v[3] == 7);
        REQUIRE (v.size == 4);
        REQUIRE (b[0] == 2);
        REQUIRE (b[1] == 3);
        REQUIRE (b[2] == 4);
        REQUIRE (b.size == 3);
    }

    SECTION("should move insert", "[Vector]") {
        Vector<int, 10> v;
        Vector<int, 10> b;
        v.append(1);
        v.append(2);
        v.append(3);
        v.append(4);
        v.append(5);
        v.append(6);
        v.append(7);
        b.append(-5);
        b.append(-4);
        b.append(-3);
        b.append(-2);
        b.append(-1);
        b.append(2333);
        b.move_insert_from(v, 1, 3, 2);
        REQUIRE (v[0] == 1);
        REQUIRE (v[1] == 5);
        REQUIRE (v[2] == 6);
        REQUIRE (v[3] == 7);
        REQUIRE (v.size == 4);
        REQUIRE (b[0] == -5);
        REQUIRE (b[1] == -4);
        REQUIRE (b[2] == 2);
        REQUIRE (b[3] == 3);
        REQUIRE (b[4] == 4);
        REQUIRE (b[5] == -3);
        REQUIRE (b[6] == -2);
        REQUIRE (b[7] == -1);
        REQUIRE (b[8] == 2333);
        REQUIRE (b.size == 9);
    }

    SECTION("should remove", "[Vector]") {
        Vector<int, 8> v;
        v.append(1);
        v.append(2);
        v.append(3);
        v.append(4);
        v.append(5);
        v.append(6);
        v.append(7);
        v.append(8);
        REQUIRE(v.remove(0) == 1);
        REQUIRE(v.remove(2) == 4);
        REQUIRE(v.remove(5) == 8);
        REQUIRE (v[0] == 2);
        REQUIRE (v[1] == 3);
        REQUIRE (v[2] == 5);
        REQUIRE (v[3] == 6);
        REQUIRE (v[4] == 7);
        REQUIRE (v.size == 5);
        v.remove_range(1, 3);
        REQUIRE (v[0] == 2);
        REQUIRE (v[1] == 7);
        REQUIRE (v.size == 2);
    }

    SECTION("should remove data without default constructor", "[Vector]") {
        TestClass::construct_cnt = 0;
        TestClass::destruct_cnt = 0;
        {
            Vector<TestClass, 8> v;
            v.append(TestClass(1));
            v.append(TestClass(2));
            v.append(TestClass(3));
            v.append(TestClass(4));
            v.append(TestClass(5));
            v.append(TestClass(6));
            v.append(TestClass(7));
            v.append(TestClass(8));
            v.remove_range(0, 8);
        }
        REQUIRE (TestClass::construct_cnt == 16);
        REQUIRE (TestClass::destruct_cnt == 16);
    }

    SECTION("should be serialized") {
        std::stringstream s;
        const int test_num = 2333;
        using V = Vector<int, test_num>;
        char memory[V::Storage_Size()];
        {
            V data;
            for (int i = 0; i < test_num; i++) {
                data.append(i);
            }
            data.serialize(s);
        }
        {
            V data;
            data.deserialize(s);
            REQUIRE(data.size == test_num);
            for (int i = 0; i < test_num; i++) {
                REQUIRE (data[i] == i);
            }
        }
    }
}

TEST_CASE("Set", "[Set]") {

    SECTION("should insert") {
        Set<int, 8> s;
        s.insert(3);
        s.insert(5);
        s.insert(4);
        s.insert(4);
        REQUIRE (s.insert(1) == 0);
        REQUIRE (s.insert(5) == 5);
        s.insert(1);
        REQUIRE (s[0] == 1);
        REQUIRE (s[1] == 1);
        REQUIRE (s[2] == 3);
        REQUIRE (s[3] == 4);
        REQUIRE (s[4] == 4);
        REQUIRE (s[5] == 5);
        REQUIRE (s[6] == 5);
    }

    SECTION("should calc bound") {
        Set<int, 8> s;
        s.insert(1);
        s.insert(1);
        s.insert(3);
        s.insert(3);
        s.insert(7);
        s.insert(7);
        REQUIRE (s.lower_bound(0) == 0);
        REQUIRE (s.lower_bound(1) == 0);
        REQUIRE (s.lower_bound(2) == 2);
        REQUIRE (s.lower_bound(3) == 2);
        REQUIRE (s.lower_bound(4) == 4);
        REQUIRE (s.lower_bound(5) == 4);
        REQUIRE (s.lower_bound(6) == 4);
        REQUIRE (s.lower_bound(7) == 4);
        REQUIRE (s.lower_bound(8) == 6);
        REQUIRE (s.upper_bound(0) == 0);
        REQUIRE (s.upper_bound(1) == 2);
        REQUIRE (s.upper_bound(2) == 2);
        REQUIRE (s.upper_bound(3) == 4);
        REQUIRE (s.upper_bound(4) == 4);
        REQUIRE (s.upper_bound(5) == 4);
        REQUIRE (s.upper_bound(6) == 4);
        REQUIRE (s.upper_bound(7) == 6);
        REQUIRE (s.upper_bound(8) == 6);
    }
}
