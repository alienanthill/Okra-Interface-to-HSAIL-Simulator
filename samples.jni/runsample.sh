# parameter 1 = name of sample
# dlls are in okra/dist/bin
java  -Xms256m -Xmx256m  -classpath ../../dist/okra.jar:$1.jar  $2 $3 $4 $5 $6 $7 $8 $9  com.amd.okra.sample.$1.Main


