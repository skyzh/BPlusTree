#!/bin/bash

cat <<EOF > BTree.hpp

// THIS FILE IS AUTOMATICALLY GENERATED, DO NOT EDIT

#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
#include <map>
#include <fstream>

#define ONLINE_JUDGE

namespace sjtu {
EOF

cat src/LRU.hpp >> BTree.hpp
cat src/Persistence.hpp >> BTree.hpp
cat src/Container.hpp >> BTree.hpp
cat src/Iterator.hpp >> BTree.hpp
cat src/BTree.hpp >> BTree.hpp

cat <<EOF >> BTree.hpp
}  // namespace sjtu

// THIS FILE IS AUTOMATICALLY GENERATED, DO NOT EDIT
EOF