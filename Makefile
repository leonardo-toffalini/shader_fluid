CC = clang
CFLAGS = -Wextra -Wall -O3
LINK = -L$(HOME)/Downloads/thirdparty/raylib-5.5_macos/lib \
			 -L$(HOME)/Downloads/thirdparty/glfw-3.4.bin.MACOS/lib-arm64
LIBS = -lraylib -lglfw3
FRAMEWORKS = \
						 -framework Cocoa \
						 -framework OpenGL \
						 -framework IOKit \
						 -framework CoreVideo \
						 -framework QuartzCore


.PHONY = run, clean

all: main run

main:
	CC src/main.c -o bin/main $(CFLAGS) $(LINK) $(LIBS) $(FRAMEWORKS)

run:
	./bin/main

clean:
	rm bin/*


