g++ -g -I../dist/include -Isrc/cpp -I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux -L../dist/bin -o ./dist/$1 src/cpp/$1/$1.cpp -lokra_x86_64
cp src/cpp/$1/$1.hsail dist
