//
// Created by Alex Chi on 2019-05-24.
//

#ifndef BPLUSTREE_CONTAINER_HPP
#define BPLUSTREE_CONTAINER_HPP

#include <cassert>
#include <cstring>
#include <iostream>
#ifndef ONLINE_JUDGE
#include "Persistence.hpp"
#endif

#ifdef __clang
#define ALLOCATOR_DISABLE_MEMORY_ALIGN
#endif
#ifdef ONLINE_JUDGE
#define ALLOCATOR_DISABLE_MEMORY_ALIGN
#endif

template<typename U>
struct Allocator {
    U *allocate(unsigned size) {
#ifdef ALLOCATOR_DISABLE_MEMORY_ALIGN
        return (U *) ::operator new(sizeof(U) * size);
#else
        return (U *) ::operator new(sizeof(U) * size, (std::align_val_t) (4 * 1024));
#endif
    }

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

    const T &operator[](unsigned i) const {
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
    void serialize(std::ostream &out) const {
        out.write(reinterpret_cast<const char *>(&size), sizeof(size));
        out.write(reinterpret_cast<const char *>(x), sizeof(T) * capacity());
    };

    void deserialize(std::istream &in) {
        in.read(reinterpret_cast<char *>(&size), sizeof(size));
        in.read(reinterpret_cast<char *>(x), sizeof(T) * capacity());
        // WARNING: only applicable to primitive types because no data were constructed!!!
    };

    static constexpr unsigned Storage_Size() {
        return sizeof(T) * capacity() + sizeof(unsigned);
    }
};

template<typename T, unsigned Cap>
class Set : public Vector<T, Cap> {
public:
    unsigned bin_lower_bound(const T &d) const {
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

    unsigned bin_upper_bound(const T &d) const {
        // https://academy.realm.io/posts/how-we-beat-cpp-stl-binary-search/
        unsigned low = 0, size = this->size;
        while (size > 0) {
            unsigned half = size / 2;
            unsigned other_half = size - half;
            unsigned probe = low + half;
            unsigned other_low = low + other_half;
            size = half;
            low = this->x[probe] < d || this->x[probe] == d ? other_low : low;
        }
        return low;
    }

    unsigned linear_upper_bound(const T &d) const {
        for (unsigned i = 0; i < this->size; i++) {
            if (this->x[i] > d) return i;
        }
        return this->size;
    }

    unsigned linear_lower_bound(const T &d) const {
        for (unsigned i = 0; i < this->size; i++) {
            if (this->x[i] >= d) return i;
        }
        return this->size;
    }

    unsigned upper_bound(const T &d) const { return bin_upper_bound(d); }

    unsigned lower_bound(const T &d) const { return bin_lower_bound(d); }

    unsigned insert(const T &d) {
        unsigned pos = upper_bound(d);
        Vector<T, Cap>::insert(pos, d);
        return pos;
    }
};

#endif //BPLUSTREE_CONTAINER_HPP
