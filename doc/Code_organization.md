# Code organization in this repository

## `doc`: documentation

Documents ranging from specs to some simple notes written while implementing things. Files
should be in Markdown format but may also be plain text.

## `src`: source code of the kernel and base system

Here is the actual source of LF OS. It's divided per resulting binary, where the folder name
matches at least a part of the resuling binary file. There may be directories not generating
a single binary, like the `sysroot` generator to be written...

### Notable folders

* `loader` the UEFI loader for LF OS
* `kernel` a small userspace utility maybe? nope, the kernel.
* `init`   the first program started by the kernel

## `t`: automated tests

Test code is placed here. Writing more tests is always good, test everything! When more
infrastructure is required for proper testing, make an issue instead of building it yourself.

All tests must always be OK in `master`!

## `util`: utilities not tied to LF OS

Sometimes small programs are written to calculate things (like calculating page table
indices from a memory address). Those are not tied to LF OS but not large enough for their
own repository. Their quality might also be low.

A notable exception is the `gsp` profiling toolchain. It started as a quick idea, was used
at a specific point in development and will likely used again - it should be usable for any
code running under gdbserver (or qemu with the integrated gdbserver). Quality for `gsp` shall
be ok.
