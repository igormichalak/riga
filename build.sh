#!/bin/sh

emcc -O2 -std=c++23 src/riga.cpp -c -o riga.o && \
emcc -O2 riga.o -o web/static/riga.js \
	-sWASM=1 \
	-sSINGLE_FILE \
	-sEXPORTED_FUNCTIONS=_malloc \
	-fno-rtti \
	-fno-exceptions && \
rm -f riga.o

