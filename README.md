# mini-fifa

## Screenshots

![starting-pos](./screenshots/starting-pos.png)

## State

Realistically - not fully working. Potentially - needs better meta-server discovery and game mechanics. Lots of bugs to say the least.

## Tools

* c++ 17, posix network libraries
* opengl 4, libepoxy
* freetype, assimp
* libpng, libjpeg, libtiff

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
