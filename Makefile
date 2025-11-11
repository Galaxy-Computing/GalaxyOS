.PHONY: build test install clean

build:
	./build.sh

test:
	./qemu.sh

install:
	./headers.sh

clean:
	./clean.sh