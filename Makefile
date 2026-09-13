.PHONY: full kernel test install clean

full:
	./iso.sh

kernel:
	export PROJECTS="libc kernel"; export SYSTEM_HEADER_PROJECTS="libc kernel"; ./build.sh

test:
	./qemu.sh

install:
	./headers.sh

clean:
	./clean.sh