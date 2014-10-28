#!/bin/bash

mkdir out
sudo apt-get install nasm

./thumbs.sh make
./thumbs.sh check
cp build/sharedlib/*.so build
objdump -f build/*.so | grep ^architecture
ldd build/*.so
tar -zcf out/libjpeg-turbo-x64.tar.gz --transform 's/.\/build\///;s/\.\///' $(./thumbs.sh list)
./thumbs.sh clean

sudo apt-get -y update > /dev/null
sudo apt-get -y install gcc-multilib > /dev/null

mkdir out
sudo apt-get install nasm

export tbs_arch=x86
./thumbs.sh make
./thumbs.sh check
cp build/sharedlib/*.so build
objdump -f build/*.so | grep ^architecture
ldd build/*.so
tar -zcf out/libjpeg-turbo-x86.tar.gz --transform 's/.\/build\///;s/\.\///' $(./thumbs.sh list)
./thumbs.sh clean
