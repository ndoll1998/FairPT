CC = g++
CFLAGS = -g -Wall -msse -msse2 -msse4.1 -O3
LFLAGS = -lpthread
IFLAGS = -Iinclude

default: main

main: src/main.cpp build/vec.o build/ray.o build/primitive.o build/scene.o build/camera.o build/texture.o build/material.o build/renderer.o build/framebuffer.o
	$(CC) $(CFLAGS) $(IFLAGS) -o main src/main.cpp build/*.o $(LFLAGS)

build/material.o: src/material.cpp src/vec.hpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/material.o -c src/material.cpp

build/texture.o: src/texture.cpp src/vec.hpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/texture.o -c src/texture.cpp

build/renderer.o: src/renderer.cpp src/vec.hpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/renderer.o -c src/renderer.cpp

build/camera.o: src/camera.cpp src/vec.hpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/camera.o -c src/camera.cpp

build/scene.o: src/scene.cpp src/vec.hpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/scene.o -c src/scene.cpp

build/primitive.o: src/primitive.cpp src/vec.hpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/primitive.o -c src/primitive.cpp

build/ray.o: src/ray.cpp src/vec.hpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/ray.o -c src/ray.cpp

build/vec.o: src/vec.cpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/vec.o -c src/vec.cpp

build/framebuffer.o: src/framebuffer.cpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/framebuffer.o -c src/framebuffer.cpp

clean:
	$(RM) main build/*.o
