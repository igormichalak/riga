#!/bin/sh

emcc -O2 -std=c++23 src/riga.cpp -c -o riga.o && \
emcc -O2 riga.o -o web/static/riga.js \
	-sWASM=1 \
	-sEXPORTED_FUNCTIONS=_malloc,_free \
	-sEXPORTED_RUNTIME_METHODS=HEAPU8 \
	-sSAFE_HEAP=1 \
	-sINVOKE_RUN=0 \
	-sASSERTIONS=1 \
	-fno-rtti \
	-fno-exceptions && \
rm -f riga.o

