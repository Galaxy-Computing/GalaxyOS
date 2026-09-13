<h1 align="center">GalaxyOS</h1>
<p align="center">An OS made in C for x86</p>
<div align="center">
    <img alt="GitHub License" src="https://img.shields.io/github/license/Galaxy-Computing/GalaxyOS">
    <img alt="Static Badge" src="https://img.shields.io/badge/version-neptune_0.1.0-blue">
    <br>
    <a href="https://discord.gg/D483Rhz82G">
        <img src="https://discord.com/api/guilds/1455046419353178226/widget.png?style=banner2" alt="Discord Banner 2"/>
    </a>
</div>

## Building
Building this OS requires a copy of i686-galaxyos-gcc, which can be built using `toolchain.sh` if you haven't already.

After you have obtained the cross compiler, you can run `make` in the root directory of the repository to build the OS. The resulting ISO image will be placed in the root and will be called `galaxyos.iso`.

Running `make test` will build the OS and automatically start it in QEMU, and will wait for `gdb` to connect. If you do not like this behaviour, you can change it in the `qemu.sh` file.

## Supported Machines/VMs
There is no guarantee that the OS will function on anything other than QEMU currently. Issues created about bugs that are in any other VM or on real hardware will not be responded to. This may change in the future.