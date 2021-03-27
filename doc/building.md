# Building LF OS

This is a short guide about building LF OS and running it in qemu.

The build system of LF OS is based on cmake, so you'll need this. Dependencies are worked out automatically and
this seems to work fine. CMake will tell you when it needs something you don't have, if not, please file a bug.

LF OS can only be built out-of-tree, which is actually enforced by LLVM but good practice anyway.

**Beware**: compiling LF OS involves compiling a LLVM toolchain, which is a very time and space consuming process.


## Required tools

To compile the toolchain and LF OS components, you need cmake, a C- and C++-Compiler, anything cmake can generate configs for (e.g. make or ninja).

To produce disk images, you'll also need `mtools`, `gdisk` and `xz`.

For QEMU you also need `OVMF` firmware installed.



## Noteworthy targets

* `hd.img` and `hd.img.xz` - generates hard disk images to use with qemu or dump on an USB stick
* `run`, `run-kvm`, `debug`, `debug-kvm` run in qemu (optionally with kvm acceleration) and optionally attach a debugger
  - debugging with kvm acceleration only supports hardware breakpoints (`hbreak`)
* `package` - generates debian packages for LF OS and the toolchain (-dev)
* `test` - runs some unit tests


## Full example

```
git clone https://praios.lf-net.org/littlefox/lf-os_amd64.git
git submodule update --init
cd lf-os_amd64 && mkdir build && cd build
cmake -G Unix\ Makefiles .. && make -j$(nproc) run
```

This will clone the repository to your computer and start an out-of-tree build of LF OS. It will generate a disk image and run it with qemu.
