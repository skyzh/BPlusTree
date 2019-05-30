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
        }
        if (ptr->next) ptr->next->prev = ptr->prev; else {
            tail = ptr->prev;
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

    void debug() {
        unsigned last_idx;
        for (auto ptr = head; ptr != nullptr; ptr = ptr->next) {
            if (ptr->idx == last_idx) {
                std::clog << "Cycle detected!" << std::endl;
                return;
            }
            std::clog << ptr->idx << " ";
            last_idx = ptr->idx;
        }
        std::clog << std::endl;
    }
};

#endif //BPLUSTREE_LRU_HPP
