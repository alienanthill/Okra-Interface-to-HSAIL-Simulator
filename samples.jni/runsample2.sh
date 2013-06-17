# parameter 1 = name of sample
# parameter 2 = name of main class
# dlls are in okra/dist/bin
# LD_LIBRARY_PATH=../../../okra/dist/bin java  -Djava.library.path=../../../okra/dist/bin  -Xms256m -Xmx256m  -classpath ../../../okra/dist/okra.jar:$1.jar  $3 $4 $5 $6 $7 $8 $9  com.amd.okra.sample.$1.$2
java  -Xms256m -Xmx256m  -classpath ../../../okra/dist/okra.jar:$1.jar  $3 $4 $5 $6 $7 $8 $9  com.amd.okra.sample.$1.$2


