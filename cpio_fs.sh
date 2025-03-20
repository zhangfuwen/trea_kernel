#!/bin/bash
if [[ ! -d data ]]; then
    mkdir data  
fi

cd build/rootfs
find . -print | cpio -ov --format=newc > ../../data/rootfs.cpio.newc
cd -