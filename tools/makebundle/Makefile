TARGET = ../bin/makebundle.exe

ifdef COMSPEC
SDL = -I /mingw/include/sdl 
else
SDL = `sdl-config --cflags`
endif

$(TARGET): makebundle.c ../../src/util/bundle.h
	@mkdir -p ../bin
	gcc -o $(TARGET) makebundle.c -std=c99 -I ../../src $(SDL) -Wall -O3