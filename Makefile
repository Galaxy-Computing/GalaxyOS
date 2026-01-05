.PHONY: build test install clean

build:
	./iso.sh

test:
	./qemu.sh

install:
	./headers.sh

clean:
	./clean.sh