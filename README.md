# mini-fifa

## State

Realistically - not fully working. Potentially - needs better meta-server discovery and game mechanics.

## Tools

* c++ 17, gcc 7
* opengl 4, glew, glfw 3
* freetype
* libjpeg, libpng, libtiff

## Compiling

	mkdir -p build
	cd build
	cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=gcc
	make

## Usage

### Client

	./build/minififa [port=5678]

### Meta-server

	./build/metaserver [port=5679]

## Acknowledgements

* The creator of the Ninja model, which, unfortunately, can not yet be animated.
* Various tutorials on how to do things.
