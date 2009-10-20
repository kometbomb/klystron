TARGET = ../bin/makebundle.exe

ifeq ($(PLATFORM),linux)
SDL = `sdl-config --cflags`
else
SDL = -I /mingw/include/sdl 
endif

$(TARGET): makebundle.c ../../src/util/bundle.h
	@mkdir -p ../bin
	gcc -o $(TARGET) makebundle.c -std=c99 -I ../../src $(SDL) -Wall -O3