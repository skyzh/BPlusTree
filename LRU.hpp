//
// Created by Alex Chi on 2019-05-30.
//

#ifndef BPLUSTREE_LRU_HPP
#define BPLUSTREE_LRU_HPP

#include <cstring>

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
