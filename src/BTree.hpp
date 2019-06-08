//
// Created by Alex Chi on 2019-05-23.
//ac

#ifndef BPLUSTREE_BTREE_HPP
#define BPLUSTREE_BTREE_HPP

#include "Container.hpp"
#include "Persistence.hpp"
#include <cassert>
#include <iostream>
#include <fstream>

template<typename K>
constexpr unsigned Default_Ord() {
    return (4 * 1024 - sizeof(unsigned) * 3) / (sizeof(K) + sizeof(unsigned));
}

template<typename K>
constexpr unsigned Default_Max_Page_In_Memory() {
    // maximum is about 6GB in memory
    return 3 * 1024 * 1024 / Default_Ord<K>() / sizeof(K) * 1024;
}

template<typename K, typename V,
        unsigned Ord = Default_Ord<K>(),
        unsigned Max_Page_In_Memory = Default_Max_Page_In_Memory<K>(),
        unsigned Max_Page = 1048576>
class BTree {
public:
    static constexpr unsigned MaxPageInMemory() { return Max_Page_In_Memory; }

    static constexpr unsigned MaxPage() { return Max_Page; }

    using BlockIdx = unsigned;

    static constexpr unsigned Order() { return Ord; }

    static constexpr unsigned HalfOrder() { return Order() >> 1; }

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

    class Leaf;

    class Index;

    class Block;

    using BPersistence = Persistence<Block, Index, Leaf, Max_Page, Max_Page_In_Memory>;

    struct Block : public Serializable {
        BlockIdx idx;
        Set<K, Order()> keys;
        BPersistence *storage;

        Block() : storage(nullptr) {}

        virtual ~Block() {}

        virtual bool is_leaf() const = 0;

        virtual Block *split(K &split_key) = 0;

        virtual void insert(const K &k, const V &v) = 0;

        virtual bool remove(const K &k) = 0;

        virtual V *find(const K &k) = 0;

        bool should_split() const { return keys.size == Order(); }

        bool should_merge() const { return keys.size * 2 < Order(); }

        bool may_borrow() const { return keys.size * 2 > Order(); }

        virtual K borrow_from_left(Block *left, const K &split_key) = 0;

        virtual K borrow_from_right(Block *right, const K &split_key) = 0;

        virtual void merge_with_left(Block *left, const K &split_key) = 0;

        virtual void merge_with_right(Block *right, const K &split_key) = 0;

        inline static Leaf *into_leaf(Block *b) {
            assert(b->is_leaf());
            Leaf *l = reinterpret_cast<Leaf *>(b);
            assert(l);
            return l;
        }

        inline static Index *into_index(Block *b) {
            assert(!b->is_leaf());
            Index *i = reinterpret_cast<Index *>(b);
            assert(i);
            return i;
        }
    };

    struct Index : public Block {
        Index() : Block() {}

        Vector<BlockIdx, Order() + 1> children;

        constexpr bool is_leaf() const override { return false; }

        Index *split(K &k) override {
            Index *that = new Index;
            this->storage->record(that);
            that->keys.move_from(this->keys, HalfOrder(), HalfOrder());
            that->children.move_from(this->children, HalfOrder(), HalfOrder() + 1);
            k = this->keys.pop();
            return that;
        }

        void insert_block(const K &k, BlockIdx v) {
            unsigned pos = this->keys.insert(k);
            this->children.insert(pos + 1, v);
        }

        V *find(const K &k) override {
            // {left: key < index_key} {right: key >= index_key}
            unsigned pos = this->keys.upper_bound(k);
            Block *block = this->storage->get(children[pos]);
            return block->find(k);
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

        bool remove(const K &k) override {
            unsigned pos = this->keys.upper_bound(k);
            Block *block = this->storage->get(children[pos]);
            bool result = block->remove(k);
            if (!result) return false;
            if (block->should_merge()) {
                if (pos != 0) {
                    K split_key = this->keys[pos - 1];
                    Block *left = this->storage->get(children[pos - 1]);
                    if (left->may_borrow()) {
                        this->keys[pos - 1] = block->borrow_from_left(left, split_key);
                        return true;
                    }
                }
                if (pos != this->children.size - 1) {
                    K split_key = this->keys[pos];
                    Block *right = this->storage->get(children[pos + 1]);
                    if (right->may_borrow()) {
                        this->keys[pos] = block->borrow_from_right(right, split_key);
                        return true;
                    }
                }
                if (pos != 0) {
                    K split_key = this->keys[pos - 1];
                    Block *left = this->storage->get(children.remove(pos - 1));
                    block->merge_with_left(left, split_key);
                    delete left;
                    this->keys.remove(pos - 1);
                    return true;
                }
                if (pos != this->children.size - 1) {
                    K split_key = this->keys[pos];
                    Block *right = this->storage->get(children.remove(pos + 1));
                    block->merge_with_right(right, split_key);
                    delete right;
                    this->keys.remove(pos);
                    return true;
                }
                assert(false);
            }
            return true;
        }

        static constexpr unsigned Storage_Size() {
            return Set<K, Order()>::Storage_Size() + Vector<BlockIdx, Order() + 1>::Storage_Size();
        }

        K borrow_from_left(Block *_left, const K &split_key) override {
            Index *left = this->into_index(_left);
            // TODO: we should verify that split_key is always the minimum
            this->keys.insert(split_key);
            // TODO: wish I were writing in Rust... therefore there'll be no copy overhead
            K new_split_key = left->keys.pop();
            this->children.move_insert_from(left->children, left->children.size - 1, 1, 0);
            return new_split_key;
        };

        K borrow_from_right(Block *_right, const K &split_key) override {
            Index *right = this->into_index(_right);
            this->keys.insert(split_key);
            K new_split_key = right->keys.remove(0);
            this->children.move_insert_from(right->children, 0, 1, this->children.size);
            return new_split_key;
        };

        void merge_with_left(Block *_left, const K &split_key) override {
            Index *left = this->into_index(_left);
            this->keys.insert(split_key);
            this->keys.move_insert_from(left->keys, 0, left->keys.size, 0);
            this->children.move_insert_from(left->children, 0, left->children.size, 0);
            this->storage->deregister(left);
        };

        void merge_with_right(Block *_right, const K &split_key) override {
            Index *right = this->into_index(_right);
            this->keys.insert(split_key);
            this->keys.move_insert_from(right->keys, 0, right->keys.size, this->keys.size);
            this->children.move_insert_from(right->children, 0, right->children.size, this->children.size);
            this->storage->deregister(right);
        };

        unsigned storage_size() const override { return Storage_Size(); }

        /*
         * Storage Mapping
         * | 8 size | Order() K keys |
         * | 8 size | Order()+1 BlockIdx children |
         */
        void serialize(std::ostream &out) const override {
            this->keys.serialize(out);
            this->children.serialize(out);
        };

        void deserialize(std::istream &in) override {
            this->keys.deserialize(in);
            this->children.deserialize(in);
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

        bool remove(const K &k) override {
            unsigned pos = this->keys.lower_bound(k);
            if (pos >= this->keys.size || this->keys[pos] != k)
                return false;
            this->keys.remove(pos);
            this->data.remove(pos);
            return true;
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
            that->keys.move_from(this->keys, HalfOrder(), HalfOrder());
            that->data.move_from(this->data, HalfOrder(), HalfOrder());
            k = that->keys[0];
            return that;
        }

        K borrow_from_left(Block *_left, const K &) override {
            Leaf *left = this->into_leaf(_left);
            this->keys.move_insert_from(left->keys, left->keys.size - 1, 1, 0);
            this->data.move_insert_from(left->data, left->data.size - 1, 1, 0);
            return this->keys[0];
        };

        K borrow_from_right(Block *_right, const K &) override {
            Leaf *right = this->into_leaf(_right);
            this->keys.move_insert_from(right->keys, 0, 1, this->keys.size);
            this->data.move_insert_from(right->data, 0, 1, this->data.size);
            return right->keys[0];
        };

        void merge_with_left(Block *_left, const K &) override {
            Leaf *left = this->into_leaf(_left);
            this->keys.move_insert_from(left->keys, 0, left->keys.size, 0);
            this->data.move_insert_from(left->data, 0, left->data.size, 0);
            this->prev = left->prev;
            this->storage->deregister(left);
        };

        void merge_with_right(Block *_right, const K &) override {
            Leaf *right = this->into_leaf(_right);
            this->keys.move_insert_from(right->keys, 0, right->keys.size, this->keys.size);
            this->data.move_insert_from(right->data, 0, right->data.size, this->data.size);
            this->next = right->next;
            this->storage->deregister(right);
        };

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

        void serialize(std::ostream &out) const override {
            out.write(reinterpret_cast<const char *>(&prev), sizeof(prev));
            out.write(reinterpret_cast<const char *>(&next), sizeof(next));
            this->keys.serialize(out);
            this->data.serialize(out);
        };

        void deserialize(std::istream &in) override {
            in.read(reinterpret_cast<char *>(&prev), sizeof(prev));
            in.read(reinterpret_cast<char *>(&next), sizeof(next));
            this->keys.deserialize(in);
            this->data.deserialize(in);
        };
    };

    /*

    Iterator begin() {
        Block *blk = root();
        while (!blk->is_leaf()) blk = storage->get(Block::into_index(blk)->children[0]);
        return Iterator(this, Block::into_leaf(blk), 0);
    }

    Iterator end() {
        Block *blk = root();
        while (!blk->is_leaf()) {
            Index *idx = Block::into_index(blk);
            blk = storage->get(idx->children[idx->children.size - 1]);
        }
        Leaf *leaf = Block::into_leaf(blk);
        return Iterator(this, leaf, leaf->keys.size);
    }
     */

    BPersistence *storage;
    const char *path;

    unsigned &root_idx() {
        return storage->persistence_index->root_idx;
    }

    Block *root() {
        return storage->get(root_idx());
    }

    BTree(const char *path = nullptr) : path(path) {
        storage = new BPersistence(path);
    }

    ~BTree() {
        delete storage;
    }

    V *find(const K &k) {
        if (!root_idx()) return nullptr;
        return storage->get(root_idx())->find(k);
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
        if (!root_idx()) root_idx() = create_leaf()->idx;
        auto root = storage->get(root_idx());
        storage->get(root_idx())->insert(k, v);
        if (root->should_split()) {
            K k;
            Block *next = root->split(k);
            Block *prev = root;
            Index *idx = create_index();
            idx->children.append(prev->idx);
            idx->children.append(next->idx);
            idx->keys.append(k);
            root_idx() = idx->idx;
        }
        storage->swap_out_pages();
    }

    bool remove(const K &k) {
        if (!root_idx()) return false;
        auto root = storage->get(root_idx());
        bool result = storage->get(root_idx())->remove(k);
        if (!result) return false;
        if (root->keys.size == 0) {
            if (!root->is_leaf()) {
                Index *prev_root = Block::into_index(root);
                root_idx() = prev_root->children[0];
                storage->deregister(prev_root);
                delete prev_root;
            }
        }
        return true;
    }

    void debug(Block *block) {
        std::cerr << "Block ID: " << block->idx << " ";
        if (block->is_leaf()) std::cerr << "(Leaf)" << std::endl;
        else std::cerr << "(Index)" << std::endl;
        if (block->is_leaf()) {
            Leaf *leaf = Block::into_leaf(block);
            for (int i = 0; i < leaf->keys.size; i++) {
                std::cerr << leaf->keys[i] << "=" << leaf->data[i] << " ";
            }
            std::cerr << std::endl;
        } else {
            Index *index = Block::into_index(block);
            for (int i = 0; i < index->keys.size; i++) {
                std::cerr << index->keys[i] << " ";
            }
            std::cerr << std::endl;
            for (int i = 0; i < index->children.size; i++) {
                std::cerr << index->children[i] << " ";
            }
            std::cerr << std::endl;
            for (int i = 0; i < index->children.size; i++) {
                debug(storage->get(index->children[i]));
            }
        }
    }
};

#endif //BPLUSTREE_BTREE_HPP
