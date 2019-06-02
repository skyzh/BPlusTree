//
// Created by Alex Chi on 2019-05-29.
//

#ifndef BPLUSTREE_PERSISTENCE_HPP
#define BPLUSTREE_PERSISTENCE_HPP

#include <fstream>
#include <iostream>
#include <cstring>
#include "LRU.hpp"

class Serializable {
public:
    virtual unsigned storage_size() const = 0;

    virtual void serialize(char *x) const = 0;

    virtual void deserialize(const char *x) = 0;

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
            std::clog << "[Warning] failed to restore from " << path << " " << f.gcount() << std::endl;
            f.clear();
        }
        assert(persistence_index->version == VERSION);
        assert(persistence_index->magic_key == PersistenceIndex::MAGIC_KEY());
    }

    void offload_page(unsigned page_id) {
        if (!path) return;
        Block *page = pages[page_id];
        unsigned buffer_size = page->storage_size();
        char *buffer = new char[buffer_size];
        page->serialize(buffer);
        unsigned offset = persistence_index->page_offset[page_id];
        f.seekp(offset, f.beg);
        f.write(buffer, buffer_size);

        delete[] buffer;

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
        unsigned buffer_size = page->storage_size();
        char *buffer = new char[buffer_size];
        f.seekg(persistence_index->page_offset[page_id], f.beg);
        f.read(buffer, buffer_size);
        page->deserialize(buffer);
        delete[] buffer;
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

#endif //BPLUSTREE_PERSISTENCE_HPP
