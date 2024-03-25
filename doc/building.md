# Building LF OS

This is a short guide about building LF OS and running it in qemu.

The build system of LF OS is based on cmake, so you'll need this. Dependencies are worked out automatically and
this seems to work fine. CMake will tell you when it needs something you don't have, if not, please file a bug.

LF OS can only be built out-of-tree, which is actually enforced by LLVM but good practice anyway.

**Beware**: compiling LF OS involves compiling a LLVM toolchain, which is a very time and space consuming process.


## Required tools

To compile the toolchain and LF OS components, you need:

- cmake
- a C-/C++ compiler
- python (3.x)
- anything cmake can generate configs for (e.g. make or ninja)

The syscall code generate runs with `perl` and requires the `YAML` module (libperl-yaml, perl-YAML, `cpan YAML`, ..).

The testing framework requires the `googletest-git` package.

To produce disk images, you'll need `gdisk` and maybe `xz`.

For QEMU you need `OVMF` firmware installed.

## Dependenices

- perl-yaml
- yq
- googletests
- gdisk
- ovmf

## Compiling the toolchain

This will take a long time, around 4.5k source files. The toolchain installs to `/opt/lf_os/toolchain` by default, you can change this by adding `-DCMAKE_INSTALL_PREFIX=$whereYouWantToInstallIt` to the cmake command.

```
# from the root of the source directory
mkdir build-toolchain && cd build-toolchain
cmake -Dsubproject=toolchain ..
make
sudo make install
```

## Installing the pre-built toolchain from CI

Download the latest toolchain from https://praios.lf-net.org/littlefox/lf-os_amd64/-/packages and `cd` to the download directory

```
tar -xf lf_os-toolchain_x.y.z+build_Linux-x86_64.tar.xz #Extract the .tar.xz 
sudo mv -rvf lf_os-toolchain_0.1.1+1654_Linux-x86_64/opt/lf_os /opt/ #Copy the toolchain to /opt 
```

## Dependenices

- perl-yaml
- yq
- googletests
- gdisk
- ovmf

## Noteworthy targets

* `hd.img` and `hd.img.xz` - generates hard disk images to use with qemu or dump on an USB stick
* `run`, `run-kvm`, `debug`, `debug-kvm` run in qemu (optionally with kvm acceleration) and optionally attach a debugger
  - debugging with kvm acceleration only supports hardware breakpoints (`hbreak`)
* `package` - generates debian packages for LF OS and the toolchain (-dev)
* `test` - runs some unit tests


## Full example

These steps must only be run after you have compiled your own toolchain or installed the pre-built toolchain
```
git clone https://praios.lf-net.org/littlefox/lf-os_amd64.git
cd lf-os_amd64
git submodule update --init
mkdir build && cd build
cmake -G Unix\ Makefiles .. && make -j$(nproc) run
```

This will clone the repository to your computer and start an out-of-tree build of LF OS. It will generate a disk image and run it with qemu.
