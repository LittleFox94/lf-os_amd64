# gdbserver-profiler - gsp

`gsp` is a profiling toolchain connecting to a gdbserver via UNIX socket or
TCP/IPv6. After reaching the start symbol (default: entry of provided image),
the tracer `gsp-trace` single steps through the program, until the stop symbol
is reached (default: never stops automatically).

The resulting trace file contains addresses for every executed instruction and
can be converted to a symbolized trace file in the following format with
`gsp-syms`.

```
_start
_start(+0x2)
main
main(+0x1)
main(+0x2)
main(+0x6)
...

```

Now, to have a visualized version of this, you can use `gsp-stackcollapse` to
convert the symbolized trace file to a stack trace file compatible with
[FlameGraph](https://github.com/brendangregg/FlameGraph).


## Restrictions

* only ELF64 images are supported right now
* debug symbols must be present
* bad things will happen if you run other code than the image you give to the
  gsp tools
* image cannot be relocated

## Use case

This toolchain can be used to profile low-level code running on QEMU. The tracer
connects to the gdbserver built into qemu in this case (`qemu-system-x86_64 -s`)
and single steps through the code after the start of the image is reached.

**Beware**: QEMU user interfaces update the window title when the VM state
changes. Single stepping changes the VM state very often and the title is
updated twice for every single instruction executed after the start symbol. This
bubbles through X server to your window manager and maybe even your compositor.
My work computer (Thinkpad P52, 16GB RAM, i7-8850H (6 cores + HT)) was barely
usable while the profiling was running.