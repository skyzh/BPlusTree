//
// Created by Alex Chi on 2019-05-23.
//

#ifndef BPLUSTREE_BTREE_HPP
#define BPLUSTREE_BTREE_HPP

#include "Container.hpp"

template<typename K, typename V>
class BTree {
public:
    using BlockIdx = unsigned;

    static constexpr unsigned Order() { return 4; }

    /*
     * data Block k v = Index { idx :: Int,
     *                          keys :: [k],
     *                          children :: [Block] } |
     *                  Leaf { idx :: Int,
     *                         prev :: Block,
     *                         next :: Block,
     *                         keys :: [k],
     *                         data :: [v] }
     */

    class Storage;

    struct Block {
        BlockIdx idx;
        Set<K, Order()> keys;
        Storage *storage;

        virtual constexpr bool is_leaf() const = 0;

        virtual Block *split(K &k) = 0;

        virtual void insert(const K &k, const V &v) = 0;

        virtual V *find(const K &k) = 0;

        bool should_split() const { return keys.size() == Order(); }
        bool should_merge() const { return keys.size() * 2 <= Order(); }
    };

    class Storage {
        Block *blocks[1024];
        int size;
    public:
        Storage() : size(0) {};

        Block *get(BlockIdx idx) { return blocks[idx]; }

        void record(Block *block) {
            block->idx = size;
            block->storage = this;
            blocks[size++] = block;
        }

        void unregister(Block *block) {
            blocks[block->idx] = nullptr;
        }
    };

    struct Index : public Block {
        Vector<BlockIdx, Order() + 1> children;

        constexpr bool is_leaf() const override { return false; }

        Index* split(const K& k) {
            assert(this->should_split());
            Index* that = new Index;
            this->storage->record(that);
            unsigned half_order = Order() / 2;
            that->keys.move_from(this->keys, half_order, half_order);
            that->values.move_from(this->data, half_order + 1, half_order);
            k = this->keys.pop();
            return that;
        }
    };

    struct Leaf : public Block {
        BlockIdx prev, next;
        Vector<V, Order()> data;

        constexpr bool is_leaf() const override { return true; }

        V *find(const K &k) {
            unsigned pos = this->keys.lower_bound(k);
            if (this->keys[pos] == k)
                return this->data[pos];
            else
                return nullptr;
        }

        void insert(const K &k, const V &v) {
            unsigned pos = this->keys.insert(k);
            this->data.insert(pos, v);
        }

        /* split :: Leaf -> (Leaf, Leaf)
         * split (Leaf a) = [k, prev, next]
         * this = prev, return = next
         */
        Leaf *split(K &k) override {
            assert(this->should_split());
            Leaf *that = new Leaf;
            this->storage->record(that);
            this->next = that->idx;
            that->prev = this->idx;
            unsigned half_order = Order() / 2;
            that->keys.move_from(this->keys, half_order, half_order);
            that->values.move_from(this->data, half_order, half_order);
            k = that->keys[0];
            return that;
        }
    };
};

#endif //BPLUSTREE_BTREE_HPP
