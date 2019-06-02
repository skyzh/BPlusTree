# BPlusTree

[![Build Status](https://travis-ci.com/SkyZH/bplustree.svg?token=szB6fz2m5vb2KyfAiZ3B&branch=master)](https://travis-ci.com/SkyZH/bplustree)

A simple B+ tree implemented in cpp. Fully unit-tested.

## Implementations

Container: some basic containers implementation including `Vector` and `Set`.

LRU: least recently used cache, eliminating memory usage when processing huge chunks.

Persistence: manage so-called 'pages', which store BTree data.

Not yet ported to upstream https://github.com/peterzheng98/CS158-DS_Project

## Limitations

Number of pages in memory must be larger than those required for a single search, because page swapping is done after one operation.
