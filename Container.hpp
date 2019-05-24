//
// Created by Alex Chi on 2019-05-24.
//

#ifndef BPLUSTREE_CONTAINER_HPP
#define BPLUSTREE_CONTAINER_HPP

template<typename U>
struct Allocator {
    U *allocate(unsigned size) { return (U *) ::operator new(sizeof(U) * size); }

    void deallocate(U *x) { ::operator delete(x); }

    void construct(U *x, const U &d) { new(x) U(d); }

    void destruct(U *x) { x->~U(); }
};

template<typename T, unsigned Cap>
class Vector {
    Allocator<T> a;
public:
    T *x;
    unsigned size;

    static constexpr unsigned capacity() { return Cap; }

    Vector() : size(0) {
        x = a.allocate(capacity());
    }

    ~Vector() {
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
        assert(size < Cap);
        memmove(x + pos + 1, x + pos, (size - pos) * sizeof(T));
        a.construct(&x[pos], d);
        ++size;
    }

    void remove(unsigned pos, unsigned length = 1) {
        assert(size > 0);
        assert(pos < size);
        for (int i = pos; i < pos + length; i++) a.destruct(&x[i]);
        memmove(x + pos, x + pos + length, (size - length - pos) * sizeof(T));
        size -= length;
    }

    void move_from(Vector &that, unsigned offset, unsigned length) {
        assert(size == 0);
        memcpy(x, that.x + offset, length * sizeof(T));
        memmove(that.x + offset, that.x + offset + length, length * sizeof(T));
        that.size = that.size - length;
        size = length;
    }
};

template<typename T, unsigned Cap>
class Set : public Vector<T, Cap> {
public:
    unsigned lower_bound(const T &d) {
        unsigned L = 0, R = this->size;
        while (L < R) {
            unsigned M = (L + R) >> 1;
            if (this->x[M] < d) {
                L = M + 1;
            } else {
                R = M;
            }
        }
        return L;
    }

    unsigned upper_bound(const T &d) {
        unsigned L = 0, R = this->size;
        while (L < R) {
            unsigned M = (L + R) >> 1;
            if (this->x[M] <= d) {
                L = M + 1;
            } else {
                R = M;
            }
        }
        return L;
    }

    unsigned insert(const T &d) {
        unsigned pos = upper_bound(d);
        Vector<T, Cap>::insert(pos, d);
        return pos;
    }
};

#endif //BPLUSTREE_CONTAINER_HPP
