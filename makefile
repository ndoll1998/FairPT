CC = g++
CFLAGS = -g -Wall -msse -msse2 -msse4.1 -mfma -O3
LFLAGS = -lpthread
IFLAGS = -Iinclude

default: main

main: src/main.cpp build/vec.o build/primitive.o build/scene.o build/camera.o build/texture.o build/material.o build/mesh.o build/renderer.o build/framebuffer.o
	$(CC) $(CFLAGS) $(IFLAGS) -o main src/main.cpp build/*.o $(LFLAGS)

build/mesh.o: src/mesh.cpp src/vec.hpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/mesh.o -c src/mesh.cpp

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

build/vec.o: src/vec.cpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/vec.o -c src/vec.cpp

build/framebuffer.o: src/framebuffer.cpp
	$(CC) $(CFLAGS) $(IFLAGS) -o build/framebuffer.o -c src/framebuffer.cpp

clean:
	$(RM) main build/*.o
