g++ -g -I../../okra/dist/include -Isrc/cpp -L../../okra/dist/bin -o ./dist/$1 src/cpp/$1/$1.cpp -lokra_x86_64
cp src/cpp/$1/$1.hsail dist
