# Building LF OS

This is a short guide about building LF OS and running it in `qemu`. These build instructions are for Debian and Arch based distributions, however they can be modified for any recent distribution.

The build system of LF OS is based on `cmake`, so you'll need this. Dependencies are listed below. If you have installed the dependencies and you still have problems building, please file a bug.

LF OS can only be built out-of-tree, which means creating a seperate `build` directory and running `cmake` from there.

## Installing Dependencies - apt

```
sudo apt install libyaml-perl libgtest-dev libgmock-dev gdisk ovmf
```

## Installing Dependencies - pacman

```
sudo pacman -S perl-yaml yq gtest gdisk ovmf
```

## Required tools

To compile the toolchain and LF OS components, you need:

- cmake
- a C/C++ compiler
- python (3.x)
- anything cmake can generate configs for (e.g. make or ninja)

The syscall code generate runs with `perl` and requires the `YAML` module (libperl-yaml, perl-YAML, `cpan YAML`, ..).

The testing framework requires the `googletest-git` package.

To produce disk images, you'll need `gdisk` and maybe `xz`.

For QEMU you need `OVMF`(UEFI reference implementation) firmware installed.

## Installing the pre-build toolchain from apt repository

There is an apt repository containing the toolchain and some other packages at https://apt.svc.0x0a.network - following that link will give you instructions how to use it as well.

This is the recommended way if this works for you, it's used by the CI/CD and the main developer.

## Installing the pre-built toolchain from CI
Download the latest toolchain from https://praios.lf-net.org/littlefox/lf-os_amd64/-/packages check for an archive named `toolchain`. Download the `.deb` for Debian based systems or `.xz` for all other distributions.
For all other distributions, `cd` to the download directory and run the commands below. Change the version in the filename below to the version of the toolchain you downloaded.

```
tar -xf lf_os-toolchain_x.y.z+build_Linux-x86_64.tar.xz #Extract the .tar.xz
sudo mv -rvf lf_os-toolchain_0.1.1+1654_Linux-x86_64/opt/lf_os /opt/ #Move the toolchain to /opt
```

For Debian based distributions install the `deb` by running

`sudo dpkg -i lf_os-toolchain_0.1.1+1654_Linux-x86_64.deb`

## Compiling the toolchain
**Beware**: Compiling a LLVM toolchain, is a very time and space consuming process as there are around 4.5k source files.

The toolchain installs to `/opt/lf_os/toolchain` by default, you can change this by adding `-DCMAKE_INSTALL_PREFIX=$whereYouWantToInstallIt` to the `cmake` command.

```
# from the root of the source directory
mkdir build-toolchain && cd build-toolchain
cmake -Dsubproject=toolchain ..
make
sudo make install
```

## Build targets

* `hd.img` and `hd.img.xz` - generates hard disk images to use with qemu or to write to a USB stick
* `run`, `run-kvm`, `debug`, `debug-kvm` run in qemu (optionally with kvm acceleration) and optionally attach a debugger
  - debugging with kvm acceleration only supports hardware breakpoints (`hbreak`)
* `package` - generates debian packages for LF OS and the toolchain (-dev)
* `test` - runs some unit tests


## Full example

These steps should only be run after you have compiled your own toolchain or installed the pre-built toolchain
```
git clone https://praios.lf-net.org/littlefox/lf-os_amd64.git
cd lf-os_amd64
git submodule update --init
mkdir build && cd build
cmake ..
make run
```

This will clone the repository to your computer and start an out-of-tree build of LF OS. It will generate a disk image and run it with `qemu`.
