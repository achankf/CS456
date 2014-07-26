#!/bin/bash

#./nse-linux386 localhost 9999 &

./router 1 localhost 9999 9991 > out1.txt 2>&1 &
./router 2 localhost 9999 9992 > out2.txt 2>&1 &
./router 3 localhost 9999 9993 > out3.txt 2>&1 &
./router 4 localhost 9999 9994 > out4.txt 2>&1 &
./router 5 localhost 9999 9995 > out5.txt 2>&1 &

# ps aux | grep router | awk '{print $2}' | xargs kill
