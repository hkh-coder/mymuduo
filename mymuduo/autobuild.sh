#! /bin/bash

set -e

# 如何当前目录下没有build就创建一个build
if [ ! -d `pwd`/build ]; then
    mkdir -p `pwd`/build
fi

# 每次重新编译时都将原来的build下的文件全部删除
rm -rf `pwd`/build/*

# 编译
cd `pwd`/build &&
    cmake .. &&
    make

cd ..

# 在系统的/usr/include下创建一个mymuduo
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

# 将所有的头文件拷贝到/usr/include/mymuduo下
for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

# 将动态库拷贝到/usr/lib
cp `pwd`/lib/libmymuduo.so /usr/lib

# 刷新一下动态库
ldconfig
