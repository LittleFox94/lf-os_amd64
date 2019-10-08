# Building LF OS

This is a short guide about building LF OS and running it in qemu.

The build system of LF OS is simply based around make. There are some Makefiles with some (or more) targets,
mostly with correct dependencies\*. You'll need some tools for building, though.

(\*: *known missing: dependencies on header files*)


## Required tools

You need `clang`, `ld.lld`, and `make` at a minimum. This will allow to build the kernel and programs. The
compilers and linker must be able to generate ELF files. You also need `gcc` and `gnu-efi` to build the loader.

To produce disk images, you'll also need `dosfstools`, `mtools`, `genisoimage` and `gdisk`.

For QEMU you also need `OVMF` firmware for qemu in `/usr/share/ovmf/OVMF.fd`.


## Noteworthy make targets

Kernel (`src/kernel/arch/amd64/kernel`) and init (`src/init/init`), The LF OS loader `src/loader/loader.efi`
and disk images (`runnable-image`, `lf-os.iso`).

**Beware:** `hd.img` generates an **empty** disk images. `runnable-image` is used to make that usable.


## Running, debugging, testing

There are some tests requiring the compilers and linkers and make. You can run them with `make test`.

To compile everything and run qemu use either `make run` or `make run-kvm` (the later using KVM hardware
virtual machines). To start and attach gdb use `make debug` (loader, kernel and init are loaded as symbol files
automatically - loader address might be incorrect as it is dynamically set by the firmware).
