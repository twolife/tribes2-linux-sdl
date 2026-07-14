# Tribes 2 for Linux shim

This repository contains a shim library necessary to be able to use the
dynamically-linked binary from the Linux version of ''Tribes 2''
released in 2001 by [Loki Software](https://en.wikipedia.org/wiki/Loki_Entertainment)

You can found all the gory details (*"why is this even needed at all ??"*)
[here](https://twolife.be/wiki/lokigames:tribes2)


## Requirements

Make sure you have a 32-bit capable C compiler.

You are most likely running a Linux distribution on a shiny 64 bits CPU,
but the binaries you are trying to run are build for the classic 32 bits PC
architecture.
In the Debian/Ubuntu world, you will need to install the *gcc-multilib* package.

To run the game, you will also need to:
 - install the [OSS Proxy Daemon](https://github.com/libfuse/osspd)
 - install i386 version of a bunch of libs. In the Debian/Ubuntu world, you will need to install the *libsdl1.2debian:i386* and *libsmpeg0t64:i386* packages.
 - download and install a [really old version of libstdc++](https://snapshot.debian.org/archive/debian/20060714T000000Z/pool/main/g/gcc-2.95/libstdc%2B%2B2.10-glibc2.2_2.95.4-27_i386.deb)

## Usage

Build the code:

    $ make

Copy the resulting *tribes2shim.so* to your game directory and run the game:

    $ LD_LIBRARY_PATH=$(pwd) LD_PRELOAD='tribes2shim.so:libstdc++-libc6.2-2.so.3' ./tribes2.dynamic -nologin

