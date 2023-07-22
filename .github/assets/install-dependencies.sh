#!/bin/bash

sudo sed -i 's|http://azure.archive.ubuntu.com/ubuntu/|http://mirror.arizona.edu/ubuntu/|g' /etc/apt/sources.list
sudo apt-get update
sudo apt-get install -y apt-transport-https
sudo apt-get update
sudo apt-get install -y curl gnupg2 ca-certificates software-properties-common nlohmann-json3-dev
sudo apt-get install -y git cmake build-essential sqlite3 libsqlite3-dev libssl-dev librdkafka-dev libboost-all-dev libtool libxerces-c-dev libflatbuffers-dev python3-pip
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt-get install -y python3.5-dev
sudo apt-get install -y libjsoncpp-dev libspdlog-dev pigz
curl -fsSL https://download.docker.com/linux/debian/gpg | sudo apt-key add -
sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"
sudo apt-get update
sudo apt-get install -y docker-ce-cli
git clone https://github.com/chinthakarukshan/metis.git
git clone https://github.com/mfontanini/cppkafka.git

cp ./.github/assets/CMakeLists.txt CMakeLists.txt

echo "----------- Current working directory $(pwd)"
cd ./metis
echo "----------- Current working directory $(pwd)"
tar -xzf metis-5.1.0.tar.gz
cd ./metis-5.1.0
echo "----------- Current working directory $(pwd)"
sed -i '/#define IDXTYPEWIDTH 32/c\#define IDXTYPEWIDTH 64' include/metis.h
make config shared=1 cc=gcc
make -j4 install
cd ../..

echo "----------- Current working directory $(pwd)"
cd ./cppkafka
echo "----------- Current working directory $(pwd)"
mkdir build
cd ./build
echo "----------- Current working directory $(pwd)"
cmake ..
make -j4
sudo make install

export JASMINEGRAPH_HOME=$(pwd)