//
// Created by Alex Chi on 2019-05-23.
//

#ifndef BPLUSTREE_BTREE_HPP
#define BPLUSTREE_BTREE_HPP

#include "Container.hpp"
#include <cassert>

template<typename K, typename V, unsigned Ord = 4>
class BTree {
public:
    using BlockIdx = unsigned;

    static constexpr unsigned Order() { return Ord; }

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

    struct Block : public Serializable {
        BlockIdx idx;
        Set<K, Order()> keys;
        Storage *storage;

        Block() : storage(nullptr) {}

        virtual constexpr bool is_leaf() const = 0;

        virtual Block *split(K &k) = 0;

        virtual void insert(const K &k, const V &v) = 0;

        virtual V *find(const K &k) = 0;

        bool should_split() const { return keys.size == Order(); }

        bool should_merge() const { return keys.size * 2 <= Order(); }


    };

    class Storage {
        Block *blocks[65536];
        int size;
    public:
        Storage() : size(100) {};

        Block *get(BlockIdx idx) { return blocks[idx]; }

        void record(Block *block) {
            block->idx = size;
            block->storage = this;
            blocks[size++] = block;
        }

        void deregister(Block *block) {
            blocks[block->idx] = nullptr;
        }
    };

    struct Index : public Block {
        Index() : Block() {}

        Vector<BlockIdx, Order() + 1> children;

        constexpr bool is_leaf() const override { return false; }

        Index *split(K &k) override {
            assert(this->should_split());
            Index *that = new Index;
            this->storage->record(that);
            unsigned half_order = Order() / 2;
            that->keys.move_from(this->keys, half_order, half_order);
            that->children.move_from(this->children, half_order, half_order + 1);
            k = this->keys.pop();
            return that;
        }

        void insert_block(const K &k, BlockIdx v) {
            unsigned pos = this->keys.insert(k);
            this->children.insert(pos + 1, v);
        }

        void insert(const K &k, const V &v) override {
            // {left: key < index_key} {right: key >= index_key}
            unsigned pos = this->keys.upper_bound(k);
            Block *block = this->storage->get(children[pos]);
            block->insert(k, v);
            if (block->should_split()) {
                K k;
                Block *that = block->split(k);
                insert_block(k, that->idx);
            }
        };

        V *find(const K &k) override {
            // {left: key < index_key} {right: key >= index_key}
            unsigned pos = this->keys.upper_bound(k);
            Block *block = this->storage->get(children[pos]);
            return block->find(k);
        }

        static constexpr unsigned Storage_Size() {
            return Set<K, Order()>::Storage_Size() + Vector<BlockIdx, Order() + 1>::Storage_Size();
        }

        unsigned storage_size() const override { return Storage_Size(); }

        /*
         * Storage Mapping
         * | 8 size | Order() K keys |
         * | 8 size | Order()+1 BlockIdx children |
         */
        void serialize(char *x) const override {
            int blk = 0;
            this->keys.serialize(x);              // VectorA
            blk += this->keys.Storage_Size();
            this->children.serialize(x + blk);    // VectorA + Vector B
        };

        void deserialize(const char *x) override {
            int blk = 0;
            this->keys.deserialize(x);            // VectorA
            blk += this->keys.Storage_Size();
            this->children.deserialize(x + blk);  // VectorA + Vector B
        };
    };

    struct Leaf : public Block {
        BlockIdx prev, next;
        Vector<V, Order()> data;

        Leaf() : Block(), prev(0), next(0) {}

        constexpr bool is_leaf() const override { return true; }

        V *find(const K &k) override {
            unsigned pos = this->keys.lower_bound(k);
            if (pos >= this->keys.size || this->keys[pos] != k)
                return nullptr;
            else
                return &this->data[pos];
        }

        void insert(const K &k, const V &v) override {
            unsigned pos = this->keys.insert(k);
            this->data.insert(pos, v);
        }

        /* split :: Leaf -> (k, Leaf, Leaf)
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
            that->data.move_from(this->data, half_order, half_order);
            k = that->keys[0];
            return that;
        }

        static constexpr unsigned Storage_Size() {
            return Set<K, Order()>::Storage_Size() + Vector<V, Order()>::Storage_Size() + sizeof(BlockIdx) * 2;
        }

        unsigned storage_size() const override { return Storage_Size(); }

        /*
         * Storage Mapping
         * | 8 BlockIdx prev | 8 BlockIdx next |
         * | 8 size | Order() K keys |
         * | 8 size | Order() V data |
         */
        void serialize(char *x) const override {
            int blk = 0;
            memcpy(x, &prev, sizeof(BlockIdx));         // 0
            blk += sizeof(BlockIdx);
            memcpy(x + blk, &next, sizeof(BlockIdx));   // 8
            blk += sizeof(BlockIdx);
            this->keys.serialize(x + blk);              // 8 + VectorA
            blk += this->keys.Storage_Size();
            this->data.serialize(x + blk);              // 8 + VectorA + Vector B
        };

        void deserialize(const char *x) override {
            int blk = 0;
            memcpy(&prev, x, sizeof(BlockIdx));         // 0
            blk += sizeof(BlockIdx);
            memcpy(&next, x + blk, sizeof(BlockIdx));   // 8
            blk += sizeof(BlockIdx);
            this->keys.deserialize(x + blk);            // 8 + VectorA
            blk += this->keys.Storage_Size();
            this->data.deserialize(x + blk);            // 8 + VectorA + Vector B
        };
    };

    Block *root;
    Storage *storage;

    BTree() : root(nullptr) {
        storage = new Storage;
    }

    ~BTree() {
        delete storage;
    }

    V *find(const K &k) {
        if (!root) return nullptr;
        return root->find(k);
    }

    Leaf *create_leaf() {
        Leaf *block = new Leaf;
        storage->record(block);
        return block;
    }

    Index *create_index() {
        Index *block = new Index;
        storage->record(block);
        return block;
    }

    void insert(const K &k, const V &v) {
        if (!root) root = create_leaf();
        root->insert(k, v);
        if (root->should_split()) {
            K k;
            Block *next = root->split(k);
            Block *prev = root;
            Index *idx = create_index();
            idx->children.append(prev->idx);
            idx->children.append(next->idx);
            idx->keys.append(k);
            root = idx;
        }
    }
};

#endif //BPLUSTREE_BTREE_HPP
