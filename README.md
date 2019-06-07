# BPlusTree

[![Build Status](https://travis-ci.com/SkyZH/bplustree.svg?token=szB6fz2m5vb2KyfAiZ3B&branch=master)](https://travis-ci.com/SkyZH/bplustree)

A simple B+ tree implemented in cpp. Fully unit-tested.

## Implementations

`BTree.cpp` is automatically generated with source file from `src/` folder.
For original implementation, you should refer to `src/`.

Container: some basic containers implementation including `Vector` and `Set`.

LRU: least recently used cache, eliminating memory usage when processing huge chunks.

Persistence: manage so-called 'pages', which store BTree data.

Partially ported to upstream https://github.com/peterzheng98/CS158-DS_Project

## Limitations

Number of pages in memory must be larger than those required for a single search, because page swapping is done after one operation.

## Todo

- [x] Reduce overhead in serialize and deserialize by passing istream as argument
- [ ] Reduce page offload overhead by introducing read-only page request
- [ ] Use HashMap to store pages in memory to store larger dataset.
- [ ] Implement copy constructor and assign
- [ ] Implement iterator and const iterator
- [ ] Port to upstream: size, empty, iterator, return value of find and insert.
