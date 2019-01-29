#!/bin/bash
#gcc -g -Wl,-Bstatic -lstlv -Wl,-Bdynamic test_stlv.c -o test_stlv
#gcc -g stlv.c test_stlv.c -o test_stlv
gcc -g demo.c -Wl,-Bstatic -lstlv -Wl,-Bdynamic -lslog -lm -o demo 
