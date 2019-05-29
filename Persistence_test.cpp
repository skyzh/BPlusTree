//
// Created by Alex Chi on 2019-05-29.
//

#include "catch.hpp"
#include <cstdio>
#include "Persistence.hpp"
#include "Container.hpp"

using VectorA = Vector<int, 20>;

class MockBlock;

class MockIndex;

class MockLeaf;

using MPersistence = Persistence<MockBlock, MockIndex, MockLeaf>;

struct MockBlock : public Serializable {
    unsigned idx;
    MPersistence *storage;

    MockBlock() : storage(nullptr) {}

    virtual ~MockBlock() {}

    virtual bool is_leaf() const = 0;
};

struct MockLeaf : public MockBlock {
    Vector<int, 233> data;

    virtual bool is_leaf() const override { return true; }

    unsigned storage_size() const override { return data.storage_size(); }

    void serialize(char *x) const override { this->data.serialize(x); };

    void deserialize(const char *x) override { this->data.deserialize(x); }
};

struct MockIndex : public MockLeaf {

    Vector<int, 16> data;

    virtual bool is_leaf() const override { return false; }

    unsigned storage_size() const override { return data.storage_size(); }

    void serialize(char *x) const override { this->data.serialize(x); };

    void deserialize(const char *x) override { this->data.deserialize(x); }

};

TEST_CASE("Persistence", "[Persistence]") {
    SECTION("should occupy different page id") {
        MPersistence persistence;
        MockLeaf leaf1;
        MockLeaf leaf2;
        persistence.record(&leaf1);
        persistence.record(&leaf2);
        REQUIRE(leaf1.idx != leaf2.idx);
        persistence.deregister(&leaf1);
        persistence.deregister(&leaf2);
    }

    SECTION("should offload pages") {
        MPersistence persistence;
        MockLeaf *leaf1 = new MockLeaf;
        MockLeaf *leaf2 = new MockLeaf;
        persistence.record(leaf1);
        persistence.record(leaf2);
    }

    SECTION("should get correct pages") {
        MPersistence persistence;
        MockLeaf *leaf1 = new MockLeaf;
        MockLeaf *leaf2 = new MockLeaf;
        persistence.record(leaf1);
        persistence.record(leaf2);
        REQUIRE(persistence.get(leaf1->idx) == leaf1);
        REQUIRE(persistence.get(leaf2->idx) == leaf2);
    }

    SECTION("should return null on not found pages") {
        MPersistence persistence;
        REQUIRE(persistence.get(5) == nullptr);
    }

    SECTION("should persist value") {
        remove("p1.test");
        unsigned leaf_idx, index_idx;
        {
            MockLeaf *leaf = new MockLeaf;
            MockIndex *index = new MockIndex;
            for (int i = 0; i < leaf->data.capacity(); i++) leaf->data.append(i);
            for (int i = 0; i < index->data.capacity(); i++) index->data.append(i);
            MPersistence persistence("p1.test");
            persistence.record(leaf);
            persistence.record(index);
            leaf_idx = leaf->idx;
            index_idx = index->idx;
        }
        {
            MPersistence persistence("p1.test");
            REQUIRE(persistence.persistence_index->page_count == 18);
            MockLeaf *leaf = dynamic_cast<MockLeaf *>(persistence.get(leaf_idx));
            REQUIRE(leaf);
            MockIndex *index = dynamic_cast<MockIndex *>(persistence.get(index_idx));
            REQUIRE(index);
            for (int i = 0; i < leaf->data.capacity(); i++) REQUIRE(leaf->data[i] == i);
            for (int i = 0; i < index->data.capacity(); i++) REQUIRE(index->data[i] == i);
        }
    }
}
