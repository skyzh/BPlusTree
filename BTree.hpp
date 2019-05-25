//
// Created by Alex Chi on 2019-05-23.
//

#ifndef BPLUSTREE_BTREE_HPP
#define BPLUSTREE_BTREE_HPP

#include "Container.hpp"
#include <cassert>
#include <iostream>
#include <fstream>

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

    class Leaf;

    class Index;

    class Block;

    struct Persistence {
        static const unsigned MAX_BLOCK_NUM = 1024;
        static const unsigned VERSION = 3;
        Block *blocks[MAX_BLOCK_NUM];
        int offset;
        bool managed;
        struct PersistenceIndex {
            unsigned version;
            unsigned magic_key;
            BlockIdx root_idx;
            unsigned block_offset[MAX_BLOCK_NUM];
            bool is_leaf[MAX_BLOCK_NUM];

            static unsigned constexpr MAGIC_KEY() { return sizeof(K) * 233 + sizeof(V) * 23333 + Ord * 2333333; }
        } persistence_index;

        Persistence(bool managed = true) : offset(16), managed(managed) {
            memset(blocks, 0, sizeof(blocks));
            memset(persistence_index.block_offset, 0, sizeof(persistence_index.block_offset));
            persistence_index.root_idx = 0;
            persistence_index.version = VERSION;
            persistence_index.magic_key = PersistenceIndex::MAGIC_KEY();
        };

        ~Persistence() {
            if (managed) for (int i = 0; i < MAX_BLOCK_NUM; i++) if (blocks[i] != nullptr) delete blocks[i];
        }

        Block *get(BlockIdx idx) { return blocks[idx]; }

        unsigned find_idx() {
            for (int i = offset; i < MAX_BLOCK_NUM; i++) if (blocks[i] == nullptr) return i;
            return -1;
        }

        void record(Block *block) {
            unsigned idx = find_idx();
            block->idx = idx;
            block->storage = this;
            blocks[idx] = block;
        }

        void deregister(Block *block) {
            blocks[block->idx] = nullptr;
            block->idx = 0;
        }

        unsigned block_used() {
            unsigned cnt = 0;
            for (int i = 0; i < MAX_BLOCK_NUM; i++) if (blocks[i] != nullptr) ++cnt;
            return cnt;
        }

        bool restore(const char* path) {
            std::ifstream file(path, std::ios::in | std::ios::binary);
            if(!file.is_open()) return false;
            file.read(reinterpret_cast<char*>(&persistence_index), sizeof(persistence_index));
            if (persistence_index.version != VERSION) return false;
            if (persistence_index.magic_key != PersistenceIndex::MAGIC_KEY()) return false;
            for (int i = 0; i < MAX_BLOCK_NUM; i++) {
                if (persistence_index.block_offset[i]) {
                    Block* block;
                    if (persistence_index.is_leaf[i]) block = new Leaf; else block = new Index;
                    block->storage = this;
                    block->idx = i;
                    unsigned buffer_size =block->storage_size();
                    char* buffer = new char[buffer_size];
                    file.read(buffer, buffer_size);
                    block->deserialize(buffer);
                    delete[] buffer;
                    blocks[i] = block;
                } else {
                    blocks[i] = nullptr;
                }
            }
            return true;
        }

        void save(const char* path) {
            std::ofstream file(path, std::ios::out | std::ios::trunc | std::ios::binary);
            assert(file.is_open());
            unsigned offset = sizeof(persistence_index);
            for (int i = 0; i < MAX_BLOCK_NUM; i++) {
                auto block = blocks[i];
                if (block) {
                    persistence_index.block_offset[i] = offset;
                    persistence_index.is_leaf[i] = block->is_leaf();
                    offset += block->storage_size();
                } else {
                    persistence_index.block_offset[i] = 0;
                }
            }
            file.write(reinterpret_cast<char*>(&persistence_index), sizeof(persistence_index));
            for (int i = 0; i < MAX_BLOCK_NUM; i++) {
                if (blocks[i]) {
                    unsigned buffer_size = blocks[i]->storage_size();
                    char* buffer = new char[buffer_size];
                    blocks[i]->serialize(buffer);
                    file.write(buffer, buffer_size);
                    delete[] buffer;
                }
            }
        }
    };

    struct Block : public Serializable {
        BlockIdx idx;
        Set<K, Order()> keys;
        Persistence *storage;

        Block() : storage(nullptr) {}

        virtual ~Block() {}

        virtual constexpr bool is_leaf() const = 0;

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

        static Leaf *into_leaf(Block *b) {
            assert(b->is_leaf());
            Leaf *l = dynamic_cast<Leaf *>(b); // TODO: at runtime, we may use reinterpret_cast
            assert(l);
            return l;
        }

        static Index *into_index(Block *b) {
            assert(!b->is_leaf());
            Index *i = dynamic_cast<Index *>(b); // TODO: at runtime, we may use reinterpret_cast
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
            unsigned half_order = Order() / 2;
            that->keys.move_from(this->keys, half_order, half_order);
            that->data.move_from(this->data, half_order, half_order);
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
            this->storage->deregister(left);
        };

        void merge_with_right(Block *_right, const K &) override {
            Leaf *right = this->into_leaf(_right);
            this->keys.move_insert_from(right->keys, 0, right->keys.size, this->keys.size);
            this->data.move_insert_from(right->data, 0, right->data.size, this->data.size);
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

    class Iterator {
        BTree* tree;
        Leaf* leaf;
        int pos;
    public:
        Iterator(BTree* tree, Leaf* leaf, int pos) : tree(tree), leaf(leaf), pos(pos) {}
        void next() {
            ++pos;
            if (pos == leaf->keys.size) {
                if (leaf->next) {
                    pos = 0;
                    leaf = Block::into_leaf(tree->storage.get(leaf->next));
                }
            }
        }

        V& get() {
            return leaf->data[pos];
        }
    };

    Iterator begin() {
        Block* blk = root;
        while(!blk->is_leaf()) blk = storage.get(Block::into_index(blk)->children[0]);
        return Iterator(this, Block::into_leaf(blk), 0);
    }

    Iterator end() {
        Block* blk = root;
        while(!blk->is_leaf()) {
            Index* idx = Block::into_index(blk);
            blk = storage.get(idx->children[idx->children.size - 1]);
        }
        Leaf* leaf = Block::into_leaf(blk);
        return Iterator(this, leaf, leaf->keys.size);
    }

    Block *root;
    Persistence storage;
    const char* path;

    BTree(const char* path = nullptr) : root(nullptr), path(path) {
        if (path) storage.restore(path);
        root = storage.get(storage.persistence_index.root_idx);
    }

    ~BTree() {
        storage.persistence_index.root_idx = root ? root->idx : 0;
        if (path) storage.save(path);
    }

    V *find(const K &k) {
        if (!root) return nullptr;
        return root->find(k);
    }

    Leaf *create_leaf() {
        Leaf *block = new Leaf;
        storage.record(block);
        return block;
    }

    Index *create_index() {
        Index *block = new Index;
        storage.record(block);
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

    bool remove(const K &k) {
        if (!root) return false;
        bool result = root->remove(k);
        if (!result) return false;
        if (root->keys.size == 0) {
            if (!root->is_leaf()) {
                Index *prev_root = Block::into_index(root);
                root = storage.get(prev_root->children[0]);
                storage.deregister(prev_root);
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
                debug(storage.get(index->children[i]));
            }
        }
    }
};

#endif //BPLUSTREE_BTREE_HPP
