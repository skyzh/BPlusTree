//
// Created by Alex Chi on 2019-05-30.
//

#ifndef BPLUSTREE_LRU_HPP
#define BPLUSTREE_LRU_HPP

template<typename Idx = unsigned>
struct LRU {
    struct Node {
        Idx idx;
        Node *prev, *next;

        Node(Idx idx) : idx(idx), prev(nullptr), next(nullptr) {}
    } *head, *tail;

    unsigned size;

    LRU() : head(nullptr), tail(nullptr), size(0) {}

    Node *put(Idx idx) {
        Node *ptr = new Node(idx);
        push(ptr);
        return ptr;
    }

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
            if (head) head->prev = nullptr;
        }
        if (ptr->next) ptr->next->prev = ptr->prev; else {
            tail = ptr->prev;
            if (tail) tail->next = nullptr;
        }
        --size;
    }

    void get(Node *ptr) {
        remove(ptr);
        push(ptr);
    }

    Idx get_lru() {
        return tail->idx;
    }

    Idx expire() {
        Idx idx = tail->idx;
        Node *ptr = tail;
        remove(tail);
        delete ptr;
        return idx;
    }
};

#endif //BPLUSTREE_LRU_HPP
