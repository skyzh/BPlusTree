//
// Created by Alex Chi on 2019-06-08.
//

#ifndef BPLUSTREE_ITERATOR_HPP
#define BPLUSTREE_ITERATOR_HPP

template <typename BTree, typename Block, typename Leaf, typename K, typename V>
class Iterator {
    mutable BTree *tree;
    using BlockIdx = typename BTree::BlockIdx;
    BlockIdx *leaf;
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
};

#endif //BPLUSTREE_ITERATOR_HPP
