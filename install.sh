#! /bin/bash
cd ./debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
make install
cd ..
