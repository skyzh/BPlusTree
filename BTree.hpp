#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
#include <map>
#include <fstream>
namespace sjtu {

    template<unsigned Cap, typename Idx = unsigned>
    class LRU {
        struct Node {
            Idx idx;
            Node *prev, *next;

            Node(Idx idx) : idx(idx), prev(nullptr), next(nullptr) {}
        } *head, *tail;

        void push(Node* ptr) {
            if (size == 0) { head = tail = ptr; }
            else {
                ptr->next = head;
                head->prev = ptr;
                head = ptr;
            }
            ++size;
        }

        void remove(Node* ptr) {
            if (ptr->prev) ptr->prev->next = ptr->next; else {
                head = ptr->next;
            }
            if (ptr->next) ptr->next->prev = ptr->prev; else {
                tail = ptr->prev;
            }
            ptr->next = ptr->prev = nullptr;
            --size;
        }

    public:
        unsigned size;

        Node** nodes;

        LRU() : head(nullptr), tail(nullptr), size(0) {
            nodes = new Node*[Cap];
            memset(nodes, 0, sizeof(Node*) * Cap);
        }

        ~LRU() {
            delete[] nodes;
        }

        void put(Idx idx) {
            assert(idx < Cap);
            assert(nodes[idx] == nullptr);
            Node *ptr = new Node(idx);
            push(ptr);
            nodes[idx] = ptr;
        }

        void get(Idx idx) {
            assert(idx < Cap);
            assert(nodes[idx]);
            remove(nodes[idx]);
            push(nodes[idx]);
        }

        Idx expire() {
            return tail->idx;
        }

        void remove(Idx idx) {
            assert(idx < Cap);
            assert(nodes[idx]);
            remove(nodes[idx]);
            delete nodes[idx];
            nodes[idx] = nullptr;
        }
    };

    class Serializable {
    public:
        virtual unsigned storage_size() const = 0;

        virtual void serialize(std::ostream &out) const = 0;

        virtual void deserialize(std::istream &in) = 0;

        static constexpr bool is_serializable() { return true; }
    };

    template<typename Block, typename Index, typename Leaf, unsigned MAX_PAGES = 1048576, unsigned MAX_IN_MEMORY = 65536>
    struct Persistence {
        const char *path;
        std::fstream f;

        static const unsigned VERSION = 5;

        struct PersistenceIndex {
            unsigned root_idx;
            unsigned magic_key;
            unsigned version;
            unsigned page_count;
            unsigned tail_pos;
            unsigned page_offset[MAX_PAGES];
            bool is_leaf[MAX_PAGES];

            static unsigned constexpr MAGIC_KEY() {
                return sizeof(Index) * 233
                       + sizeof(Leaf) * 23333
                       + MAX_PAGES * 23;
            }

            PersistenceIndex() : root_idx(0), magic_key(MAGIC_KEY()),
                                 version(VERSION), page_count(16),
                                 tail_pos(sizeof(PersistenceIndex)) {
                memset(page_offset, 0, sizeof(page_offset));
                memset(is_leaf, 0, sizeof(is_leaf));
            }
        } *persistence_index;

        Block **pages;

        struct Stat {
            unsigned create;
            unsigned destroy;
            unsigned access_cache_hit;
            unsigned access_cache_miss;
            unsigned request_write;
            unsigned request_read;
            unsigned swap_out;

            void stat() {
                printf("    access hit/total %d/%d %.5f%%\n",
                       access_cache_hit,
                       access_cache_miss + access_cache_hit,
                       double(access_cache_hit) / (access_cache_miss + access_cache_hit) * 100);
                printf("    create/destroy/swap_in/out %d %d %d %d\n", create, destroy, access_cache_miss, swap_out);
            }

            Stat() : create(0), destroy(0),
                     access_cache_hit(1), access_cache_miss(0),
                     request_read(0), request_write(0), swap_out(0) {}
        } stat;

        using BLRU = LRU<MAX_PAGES>;
        BLRU lru;

        void restore() {
            if (!path) return;
            f.seekg(0, f.beg);
            if (!f.read(reinterpret_cast<char *>(persistence_index), sizeof(PersistenceIndex))) {
                f.clear();
            }
            assert(persistence_index->version == VERSION);
            assert(persistence_index->magic_key == PersistenceIndex::MAGIC_KEY());
        }

        void offload_page(unsigned page_id) {
            if (!path) return;
            Block *page = pages[page_id];

            unsigned offset = persistence_index->page_offset[page_id];
            f.seekp(offset, f.beg);
            page->serialize(f);

            delete pages[page_id];
            pages[page_id] = nullptr;

            lru.remove(page_id);
        }

        Block *load_page(unsigned page_id) {
            if (pages[page_id]) {
                ++stat.access_cache_hit;
                lru.get(page_id);
                return pages[page_id];
            }
            if (!path) return nullptr;
            ++stat.access_cache_miss;
            Block *page;
            if (!persistence_index->page_offset[page_id]) return nullptr;
            if (persistence_index->is_leaf[page_id])
                page = new Leaf;
            else
                page = new Index;
            f.seekg(persistence_index->page_offset[page_id], f.beg);
            page->deserialize(f);
            pages[page_id] = page;
            page->storage = this;
            page->idx = page_id;
            lru.put(page_id);
            return page;
        }

        void save() {
            if (!path) return;
            f.seekp(0, f.beg);
            f.write(reinterpret_cast<char *>(persistence_index), sizeof(PersistenceIndex));
            for (unsigned i = 0; i < MAX_PAGES; i++) {
                if (pages[i]) offload_page(i);
            }
        }

        Persistence(const char *path = nullptr) : path(path) {
            assert(Index::is_serializable());
            assert(Leaf::is_serializable());
            persistence_index = new PersistenceIndex;
            pages = new Block *[MAX_PAGES];
            memset(pages, 0, sizeof(Block *) * MAX_PAGES);
            if (path) {
                f.open(path, std::ios::in | std::ios::out | std::ios::ate | std::ios::binary);
                if (f)
                    restore();
                else
                    f.open(path, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
            }
        }

        ~Persistence() {
            save();
            f.close();
            delete[] pages;
            delete persistence_index;
        }

        Block *get(unsigned page_id) {
            return load_page(page_id);
        }

        void create_page(Block *block) {
            unsigned offset = persistence_index->tail_pos;
            unsigned page_id = persistence_index->page_count++;
            persistence_index->tail_pos += block->storage_size();
            block->idx = page_id;
            pages[page_id] = block;
            persistence_index->is_leaf[page_id] = block->is_leaf();
            persistence_index->page_offset[page_id] = offset;
            lru.put(page_id);
        }

        void swap_out_pages() {
            while (lru.size > MAX_IN_MEMORY) {
                unsigned idx = lru.expire();
                offload_page(idx);
                ++stat.swap_out;
            }
        }

        void record(Block *block) {
            create_page(block);
            block->storage = this;
            ++stat.create;
        }

        void deregister(Block *block) {
            lru.remove(block->idx);
            pages[block->idx] = nullptr;
            block->idx = 0;
            ++stat.destroy;
        }
    };

    template<typename U>
    struct Allocator {
        U *allocate(unsigned size) { return (U *) ::operator new(sizeof(U) * size); }

        void deallocate(U *x) { ::operator delete(x); }

        void construct(U *x, const U &d) { new(x) U(d); }

        void destruct(U *x) { x->~U(); }
    };

    template<typename T, unsigned Cap>
    class Vector : Serializable {
        Allocator<T> a;
    public:
        T *x;
        unsigned size;

        static constexpr unsigned capacity() { return Cap; }

        Vector() : size(0) {
            x = a.allocate(capacity());
        }

        virtual ~Vector() {
            for (int i = 0; i < size; i++) a.destruct(&x[i]);
            a.deallocate(x);
        }

        Vector(const Vector &) = delete;

        T &operator[](unsigned i) {
            assert(i < size);
            return x[i];
        }

        void append(const T &d) {
            assert(size < Cap);
            a.construct(&x[size++], d);
        }

        T pop() {
            assert(size > 0);
            T d = x[size - 1];
            a.destruct(&x[--size]);
            return d;
        }

        void insert(unsigned pos, const T &d) {
            assert(pos >= 0 && pos <= size);
            assert(size < capacity());
            memmove(x + pos + 1, x + pos, (size - pos) * sizeof(T));
            a.construct(&x[pos], d);
            ++size;
        }

        void remove_range(unsigned pos, unsigned length = 1) {
            assert(size >= length);
            assert(pos < size);
            for (int i = pos; i < pos + length; i++) a.destruct(&x[i]);
            memmove(x + pos, x + pos + length, (size - length - pos) * sizeof(T));
            size -= length;
        }

        T remove(unsigned pos) {
            assert(size > 0);
            assert(pos < size);
            T element = x[pos];
            a.destruct(&x[pos]);
            memmove(x + pos, x + pos + 1, (size - 1 - pos) * sizeof(T));
            size -= 1;
            return element;
        }

        void move_from(Vector &that, unsigned offset, unsigned length) {
            assert(size == 0);
            move_insert_from(that, offset, length, 0);
        }

        void move_insert_from(Vector &that, unsigned offset, unsigned length, unsigned at) {
            assert(at <= size);
            assert(offset + length <= that.size);
            assert(size + length <= capacity());
            memmove(x + at + length, x + at, (size - at) * sizeof(T));
            memcpy(x + at, that.x + offset, length * sizeof(T));
            memmove(that.x + offset, that.x + offset + length, (that.size - length - offset) * sizeof(T));
            that.size -= length;
            size += length;
        }

        unsigned storage_size() const { return Storage_Size(); };
        /*
         * Storage Mapping
         * | 8 size | Cap() T x |
         */
        void serialize(std::ostream& out) const {
            out.write(reinterpret_cast<const char*>(&size), sizeof(size));
            out.write(reinterpret_cast<const char*>(x), sizeof(T) * capacity());
        };

        void deserialize(std::istream& in) {
            in.read(reinterpret_cast<char*>(&size), sizeof(size));
            in.read(reinterpret_cast<char*>(x), sizeof(T) * capacity());
            // WARNING: only applicable to primitive types because no data were constructed!!!
        };

        static constexpr unsigned Storage_Size() {
            return sizeof(T) * capacity() + sizeof(unsigned);
        }
    };

    template<typename T, unsigned Cap>
    class Set : public Vector<T, Cap> {
    public:
        unsigned lower_bound(const T &d) {
            // https://academy.realm.io/posts/how-we-beat-cpp-stl-binary-search/
            unsigned low = 0, size = this->size;
            while (size > 0) {
                unsigned half = size / 2;
                unsigned other_half = size - half;
                unsigned probe = low + half;
                unsigned other_low = low + other_half;
                size = half;
                low = this->x[probe] < d ? other_low : low;
            }
            return low;
        }

        unsigned upper_bound(const T &d) {
            // https://academy.realm.io/posts/how-we-beat-cpp-stl-binary-search/
            unsigned low = 0, size = this->size;
            while (size > 0) {
                unsigned half = size / 2;
                unsigned other_half = size - half;
                unsigned probe = low + half;
                unsigned other_low = low + other_half;
                size = half;
                low = this->x[probe] <= d ? other_low : low;
            }
            return low;
        }

        unsigned insert(const T &d) {
            unsigned pos = upper_bound(d);
            Vector<T, Cap>::insert(pos, d);
            return pos;
        }
    };

    template<typename K, typename V,
            unsigned Ord = 2 * 1024 / sizeof(K),
            unsigned Max_Page_In_Memory = 8 * 1024 * 1024 / Ord / sizeof(K) * 1024>
    class BTree {
    public:
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

        using BPersistence = Persistence<Block, Index, Leaf, 1048576, Max_Page_In_Memory>;

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

        class Iterator {
            BTree *tree;
            Leaf *leaf;
            int pos;
        public:
            Iterator(BTree *tree, Leaf *leaf, int pos) : tree(tree), leaf(leaf), pos(pos) {}

            void next() {
                ++pos;
                if (pos == leaf->keys.size) {
                    if (leaf->next) {
                        pos = 0;
                        leaf = Block::into_leaf(tree->storage->get(leaf->next));
                    }
                }
            }

            void prev() {
                --pos;
                if (pos < 0) {
                    if (leaf->prev) {
                        leaf = Block::into_leaf(tree->storage->get(leaf->prev));
                        pos = leaf->keys.size - 1;
                    }
                }
            }

            V &get() {
                return leaf->data[pos];
            }

            friend bool operator==(const Iterator& a, const Iterator& b) { return true; }
        };

        Iterator begin() {
            return Iterator(this, nullptr, 0);
        }

        Iterator end() {
            return Iterator(this, nullptr, 0);
        }

        Block *root;
        BPersistence *storage;
        const char *path;

        BTree(const char *path = "persist.db") : root(nullptr), path(path) {
            storage = new BPersistence(path);
            root = storage->get(storage->persistence_index->root_idx);
        }

        ~BTree() {
            storage->persistence_index->root_idx = root ? root->idx : 0;
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
            storage->get(root->idx);
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
            storage->swap_out_pages();
        }

        bool remove(const K &k) {
            if (!root) return false;
            storage->get(root->idx);
            bool result = root->remove(k);
            if (!result) return false;
            if (root->keys.size == 0) {
                if (!root->is_leaf()) {
                    Index *prev_root = Block::into_index(root);
                    root = storage->get(prev_root->children[0]);
                    storage->deregister(prev_root);
                    delete prev_root;
                }
            }
            return true;
        }
        // Wrapper functions
        V& at(const K& k) {
            return *find(k);
        }

        void erase(const K& k) {
            remove(k);
        }
    };

}  // namespace sjtu


