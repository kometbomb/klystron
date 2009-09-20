TARGET = ../bin/makebundle.exe

$(TARGET): makebundle.c ../../src/util/bundle.h
	@mkdir -p ../bin
	gcc -o $(TARGET) makebundle.c -std=c99 -I ../../src -I /mingw/include/sdl -Wall -O3