BENCHMARK_NAME = carve
SOURCES = carve.c 

include ../../Makefile.common

test: clean ${BENCHMARK_NAME}.tasks
	./run_prog
	./mt_prog

dtest: clean ${BENCHMARK_NAME}.tasks
	./run_prog

support:
	g++ -g -DNO_FREETYPE -lpng -lpngwriter -lz -lfreetype pngtorigel.cpp -o pngtorigel
	g++ -g -DNO_FREETYPE -lpng -lpngwriter -lz -lfreetype rigeltopng.cpp -o rigeltopng

