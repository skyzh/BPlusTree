//
// Created by Alex Chi on 2019-06-08.
//

#ifndef BPLUSTREE_ITERATOR_HPP
#define BPLUSTREE_ITERATOR_HPP

template<typename BTree, typename Block, typename Leaf, typename K, typename V>
class Iterator {
    mutable BTree *tree;
    using BlockIdx = typename BTree::BlockIdx;
    BlockIdx leaf_idx;
    int pos;
public:
    Iterator(BTree *tree, BlockIdx leaf_idx, int pos) : tree(tree), leaf_idx(leaf_idx), pos(pos) {}

    Iterator(const Iterator &that) : tree(that.tree), leaf_idx(that.leaf_idx), pos(that.pos) {}

    Iterator &operator++() {
        ++pos;
        if (pos == leaf()->keys.size && leaf()->next) {
            pos = 0;
            leaf_idx = leaf()->next;
        }
        expire();
        return *this;
    }

    void expire() const {
        tree->storage->swap_out_pages();
    }

    const Leaf *leaf() const { return reinterpret_cast<const Leaf *>(tree->storage->read(leaf_idx)); }

    Leaf *leaf_mut() { return Block::into_leaf(tree->storage->get(leaf_idx)); }

    Iterator &operator--() {
        --pos;
        if (pos < 0 && leaf()->prev) {
            leaf_idx = leaf()->prev;
            pos = leaf()->keys.size - 1;
        }
        expire();
        return *this;
    }

    Iterator operator++(int) {
        auto _ = Iterator(*this);
        ++(*this);
        return _;
    }

    Iterator operator--(int) {
        auto _ = Iterator(*this);
        --(*this);
        return _;
    }

    V operator*() {
        return getValue();
    }

    friend bool operator==(const Iterator &a, const Iterator &b) {
        return a.tree == b.tree && a.leaf_idx == b.leaf_idx && a.pos == b.pos;
    }

    friend bool operator!=(const Iterator &a, const Iterator &b) {
        return !(a == b);
    }

    V getValue() {
        expire();
        return leaf_mut()->data[pos];
    }

    void modify(const V &v) {
        expire();
        leaf_mut()->data[pos] = v;
    }
};

#endif //BPLUSTREE_ITERATOR_HPP
