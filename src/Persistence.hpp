//
// Created by Alex Chi on 2019-05-29.
//

#ifndef BPLUSTREE_PERSISTENCE_HPP
#define BPLUSTREE_PERSISTENCE_HPP

#include <fstream>
#include <iostream>
#include <cstring>
#ifndef ONLINE_JUDGE
#include "LRU.hpp"
#endif

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

    static const unsigned VERSION = 6;

    struct PersistenceIndex {
        unsigned root_idx;
        unsigned magic_key;
        unsigned version;
        unsigned size;
        size_t tail_pos;
        size_t page_offset[MAX_PAGES];
        size_t page_size[MAX_PAGES];
        unsigned char is_leaf[MAX_PAGES];

        static unsigned constexpr MAGIC_KEY() {
            return sizeof(Index) * 233
                   + sizeof(Leaf) * 23333
                   + MAX_PAGES * 23;
        }

        PersistenceIndex() : root_idx(0), magic_key(MAGIC_KEY()),
                             version(VERSION),
                             tail_pos(sizeof(PersistenceIndex)),
                             size(0) {
            memset(page_offset, 0, sizeof(page_offset));
            memset(is_leaf, 0, sizeof(is_leaf));
        }
    } *persistence_index;

    Block **pages;
    bool *dirty;
    unsigned lst_empty_slot;

    struct Stat {
        long long create;
        long long destroy;
        long long access_cache_hit;
        long long access_cache_miss;
        long long dirty_write;
        long long swap_out;

        void stat() {
            printf("    access hit/total %lld/%lld %.5f%%\n",
                   access_cache_hit,
                   access_cache_miss + access_cache_hit,
                   double(access_cache_hit) / (access_cache_miss + access_cache_hit) * 100);
            printf("    create/destroy %lld %lld\n", create, destroy);
            printf("    swap_in/out/dirty %lld %lld %lld %.5f%%\n",
                   access_cache_miss, swap_out, dirty_write,
                   double(dirty_write) / (swap_out) * 100);
        }

        Stat() : create(0), destroy(0),
                 access_cache_hit(1), access_cache_miss(0), dirty_write(0), swap_out(0) {}
    } stat;

    using BLRU = LRU<MAX_PAGES>;
    BLRU lru;

    Persistence(const char *path = nullptr) : path(path), lst_empty_slot(16) {
        assert(Index::is_serializable());
        assert(Leaf::is_serializable());
        persistence_index = new PersistenceIndex;
        pages = new Block *[MAX_PAGES];
        dirty = new bool[MAX_PAGES];
        memset(pages, 0, sizeof(Block *) * MAX_PAGES);
        memset(dirty, 0, sizeof(bool) * MAX_PAGES);
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
        delete[] dirty;
        delete[] pages;
        delete persistence_index;
    }

    void restore() {
        if (!path) return;
        f.seekg(0, f.beg);
        if (!f.read(reinterpret_cast<char *>(persistence_index), sizeof(PersistenceIndex))) {
            std::clog << "[Warning] failed to restore from " << path << " " << f.gcount() << std::endl;
            f.clear();
        }
        assert(persistence_index->version == VERSION);
        assert(persistence_index->magic_key == PersistenceIndex::MAGIC_KEY());
    }

    void offload_page(unsigned page_id) {
        if (!path) return;
        Block *page = pages[page_id];

        if (dirty[page_id]) {
            auto offset = persistence_index->page_offset[page_id];
            f.seekp(offset, f.beg);
            page->serialize(f);
            ++stat.dirty_write;
        }

        delete pages[page_id];
        pages[page_id] = nullptr;

        lru.remove(page_id);
    }

    bool is_loaded(unsigned page_id) { return pages[page_id] != nullptr; }

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
        if (persistence_index->is_leaf[page_id] == 1)
            page = new Leaf;
        else if (persistence_index->is_leaf[page_id] == 0)
            page = new Index;
        else
            assert(false);
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

    const Block *read(unsigned page_id) {
        return load_page(page_id);
    }

    Block *get(unsigned page_id) {
        dirty[page_id] = true;
        return load_page(page_id);
    }

    size_t align_to_4k(size_t offset) {
        return (offset + 0xfff) & (~0xfff);
    }

    unsigned append_page(size_t &offset, size_t size) {
        offset = align_to_4k(persistence_index->tail_pos);
        persistence_index->tail_pos = offset + size;
        return lst_empty_slot++;
    }

    unsigned request_page(size_t &offset, size_t size) {
        for(;;lst_empty_slot++) {
            if (persistence_index->page_offset[lst_empty_slot] == 0) return append_page(offset, size);
            if (persistence_index->is_leaf[lst_empty_slot] == 2 && persistence_index->page_size[lst_empty_slot] >= size) {
                offset = persistence_index->page_offset[lst_empty_slot];
                return lst_empty_slot++;
            }
        }
    }

    void create_page(Block *block) {
        size_t offset;
        unsigned page_id = request_page(offset, block->storage_size());
        block->idx = page_id;
        pages[page_id] = block;
        persistence_index->is_leaf[page_id] = block->is_leaf() ? 1 : 0;
        persistence_index->page_offset[page_id] = offset;
        persistence_index->page_size[page_id] = block->storage_size();
        lru.put(page_id);
        dirty[page_id] = true;
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
        dirty[block->idx] = false;
        lst_empty_slot = std::min(lst_empty_slot, block->idx);
        persistence_index->is_leaf[block->idx] = 2;
        block->idx = 0;
        ++stat.destroy;
    }
};

#endif //BPLUSTREE_PERSISTENCE_HPP
